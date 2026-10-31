// ImGui window that shows and changes the game state: flags, clock, flagship and SIS.

#include "gamewindow.h"

#include <cfloat>
#include <imgui.h>
#include <fmt/format.h>
#include "core/stl/array.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "core/string/StringUtils.h"
#include "core/thread/Mutex.h"
#include "libs/graphics/imgui/toolwindows.h"
#include "uqm/clock.h"
#include "uqm/gameev.h"
#include "uqm/globdata.h"
#include "uqm/imgui/gamethread.h"
#include "uqm/sis.h"
#include "uqm/starcon.h"

namespace uqm
{

namespace
{

constexpr double c_refreshSeconds {0.5};
// "Next event" moves the clock at most this many days.
constexpr int c_maxDaysToNextEvent {3650};
const ImVec4 c_changedColor {1.0f, 0.85f, 0.2f, 1.0f};

struct EventView
{
	uint8_t day {};
	uint8_t month {};
	uint16_t year {};
	uint8_t funcIndex {};
};

// A copy of the game state that the main thread can read.
struct GameView
{
	ACTIVITY activity {};
	uqstl::array<uint32_t, c_gameFlagCount> flags {};
	uint8_t day {};
	uint8_t month {};
	uint16_t year {};
	int16_t tickCount {};
	int16_t dayInTicks {};
	uqstl::vector<EventView> events {};
	bool hasAutopilot {};
	GFXPOINT autopilot {};
	GFXPOINT ipLocation {};
	uint16_t shipFacing {};
	uint8_t ipPlanet {};
	uint8_t inOrbit {};
	int32_t logX {};
	int32_t logY {};
	uint32_t resUnits {};
	uint32_t fuel {};
	uint16_t crew {};
	uint8_t landers {};
	uqstl::string shipName {};
	uqstl::string commanderName {};
};

// The snapshot from the game thread.
struct SnapshotResult
{
	Mutex lock {};
	// Nullopt if no game runs.
	uqstl::optional<GameView> view UQM_GUARDED_BY(lock) {};
	bool done UQM_GUARDED_BY(lock) {};
};

SnapshotResult s_result {};
uqstl::optional<GameView> s_view {};
// The flags of the snapshot before s_view, to find the flags that changed.
uqstl::optional<uqstl::array<uint32_t, c_gameFlagCount>> s_previousFlags {};
bool s_requested {};
bool s_hasSnapshot {};
double s_lastRequest {};
char s_flagFilter[64] {};
int s_daysToAdvance {1};

[[nodiscard]] GameView takeView()
{
	GameState& state {gameState()};
	GameView view {};
	view.activity = currentActivity();
	for (size_t index {0}; index < c_gameFlagCount; ++index)
	{
		view.flags[index] = state.flag(static_cast<GameFlag>(index));
	}

	GameClock& clock {state.clock()};
	view.day = clock.day();
	view.month = clock.month();
	view.year = clock.year();
	view.tickCount = clock.tickCount();
	view.dayInTicks = clock.dayInTicks();
	QUEUE& events {clock.events()};
	for (HEVENT event {GetHeadLink(&events)}; event != nullptr;)
	{
		// An EVENT starts with the LINK fields.
		const EVENT* const eventPtr {reinterpret_cast<const EVENT*>(LockLink(&events, event))};
		view.events.push_back({eventPtr->day_index, eventPtr->month_index, eventPtr->year_index, eventPtr->func_index});
		const HEVENT next {_GetSuccLink(eventPtr)};
		UnlockLink(&events, event);
		event = next;
	}

	const FlagshipState& flagship {state.flagship()};
	view.hasAutopilot = flagship.autopilot.hasTarget();
	view.autopilot = flagship.autopilot.target();
	view.ipLocation = flagship.ipLocation;
	view.shipFacing = flagship.shipFacing;
	view.ipPlanet = flagship.ipPlanet;
	view.inOrbit = flagship.inOrbit;

	const SIS_STATE& sis {state.sis()};
	view.logX = sis.log_x;
	view.logY = sis.log_y;
	view.resUnits = sis.ResUnits;
	view.fuel = sis.FuelOnBoard;
	view.crew = sis.CrewEnlisted;
	view.landers = sis.NumLanders;
	view.shipName = sis.ShipName;
	view.commanderName = sis.CommanderName;
	return view;
}

void requestSnapshot()
{
	s_requested = true;
	s_hasSnapshot = true;
	s_lastRequest = ImGui::GetTime();
	runOnGameThread([] {
		uqstl::optional<GameView> view {};
		if (IsGameInProgress())
		{
			view = takeView();
		}
		LockGuard guard {s_result.lock};
		s_result.view = uqstl::move(view);
		s_result.done = true;
	});
}

void receiveSnapshot()
{
	uqstl::optional<GameView> view {};
	{
		LockGuard guard {s_result.lock};
		if (!s_result.done)
		{
			return;
		}
		s_result.done = false;
		view = uqstl::move(s_result.view);
	}
	s_requested = false;
	if (s_view)
	{
		s_previousFlags = s_view->flags;
	}
	else
	{
		s_previousFlags.reset();
	}
	s_view = uqstl::move(view);
}

// Changes the game on the game thread, if a game runs, and takes a new snapshot after it.
void changeGame(uqstl::function<void()> change)
{
	runOnGameThread([change {uqstl::move(change)}] {
		if (IsGameInProgress())
		{
			change();
		}
	});
	requestSnapshot();
}

void textDisabledWrapped(const char* text)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	ImGui::TextWrapped("%s", text);
	ImGui::PopStyleColor();
}

[[nodiscard]] uint32_t maxFlagValue(const GameFlagInfo& info)
{
	return info.bits >= 32 ? UINT32_MAX : (uint32_t {1} << info.bits) - 1;
}

[[nodiscard]] uqstl::string activityText(ACTIVITY activity)
{
	static constexpr const char* c_names[] {"Super Melee or menu", "Last battle", "Encounter", "HyperSpace",
											"Interplanetary", "Won last battle", "QuasiSpace", "Planet orbit",
											"Starbase"};
	const uint8_t low {lowByte(activity)};
	uqstl::string text {low < uqstl::size(c_names) ? c_names[low] : fmt::format("{}", low)};
	if (activity & START_ENCOUNTER)
	{
		text += ", start encounter";
	}
	if (activity & START_INTERPLANETARY)
	{
		text += ", start interplanetary";
	}
	if (activity & IN_BATTLE)
	{
		text += ", in battle";
	}
	return text;
}

[[nodiscard]] const char* eventText(uint8_t funcIndex)
{
	const char* const name {eventIdNumToStr(funcIndex)};
	return name != nullptr ? name : "?";
}

void drawFlagRow(size_t index)
{
	const GameFlagInfo& info {c_gameFlags[index]};
	const uint32_t value {s_view->flags[index]};
	const bool changed {s_previousFlags && (*s_previousFlags)[index] != value};

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	if (changed)
	{
		ImGui::TextColored(c_changedColor, "%.*s", static_cast<int>(info.name.size()), info.name.data());
	}
	else
	{
		ImGui::TextUnformatted(info.name.data(), info.name.data() + info.name.size());
	}
	ImGui::TableNextColumn();
	ImGui::Text("%u", info.bits);
	ImGui::TableNextColumn();
	ImGui::PushID(static_cast<int>(index));
	ImGui::SetNextItemWidth(-FLT_MIN);
	uint32_t edited {value};
	const uint32_t step {1};
	if (ImGui::InputScalar("##value", ImGuiDataType_U32, &edited, &step, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue)
		&& edited != value)
	{
		const GameFlag flag {static_cast<GameFlag>(index)};
		const uint32_t clamped {uqstl::min(edited, maxFlagValue(info))};
		changeGame([flag, clamped] {
			gameState().setFlag(flag, clamped);
		});
	}
	ImGui::PopID();
}

void drawFlags()
{
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##filter", "Filter, for example SHOFIXTI", s_flagFilter, sizeof(s_flagFilter));
	textDisabledWrapped("Values that changed in the last snapshot are yellow. A value is limited to its bits.");

	constexpr ImGuiTableFlags flags {ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY};
	if (!ImGui::BeginTable("flags", 3, flags))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Flag", ImGuiTableColumnFlags_WidthStretch, 0.6f);
	ImGui::TableSetupColumn("Bits", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 3.0f);
	ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.4f);
	ImGui::TableHeadersRow();

	const uqstl::string_view filter {s_flagFilter};
	if (filter.empty())
	{
		ImGuiListClipper clipper {};
		clipper.Begin(static_cast<int>(c_gameFlagCount));
		while (clipper.Step())
		{
			for (int row {clipper.DisplayStart}; row < clipper.DisplayEnd; ++row)
			{
				drawFlagRow(static_cast<size_t>(row));
			}
		}
	}
	else
	{
		for (size_t index {0}; index < c_gameFlagCount; ++index)
		{
			const uqstl::string_view name {c_gameFlags[index].name};
			if (uqstl::search(name.begin(), name.end(), filter.begin(), filter.end(), compareCharICase) != name.end())
			{
				drawFlagRow(index);
			}
		}
	}
	ImGui::EndTable();
}

void drawClock()
{
	const GameView& view {*s_view};
	ImGui::Text("Date: %04u-%02u-%02u", view.year, view.month, view.day);
	if (view.dayInTicks > 0)
	{
		ImGui::Text("Day: %d of %d ticks left", view.tickCount, view.dayInTicks);
	}
	else
	{
		ImGui::TextUnformatted("The clock does not run here.");
	}

	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
	ImGui::InputInt("##days", &s_daysToAdvance);
	s_daysToAdvance = uqstl::clamp(s_daysToAdvance, 1, 3650);
	ImGui::SameLine();
	if (ImGui::Button("Advance days"))
	{
		const auto days {static_cast<uint16_t>(s_daysToAdvance)};
		changeGame([days] {
			MoveGameClockDays(days);
		});
	}
	ImGui::SetItemTooltip("Moves the date forward. The events of each day run, as in the game.");
	ImGui::SameLine();
	ImGui::BeginDisabled(view.events.empty());
	if (ImGui::Button("Next event"))
	{
		changeGame([] {
			GameClock& clock {gameState().clock()};
			QUEUE& events {clock.events()};
			const HEVENT first {GetHeadLink(&events)};
			if (first == nullptr)
			{
				return;
			}
			const EVENT* const eventPtr {reinterpret_cast<const EVENT*>(LockLink(&events, first))};
			const EventView target {eventPtr->day_index, eventPtr->month_index, eventPtr->year_index, eventPtr->func_index};
			UnlockLink(&events, first);
			for (int day {0}; day < c_maxDaysToNextEvent; ++day)
			{
				if (clock.year() > target.year
					|| (clock.year() == target.year
						&& (clock.month() > target.month || (clock.month() == target.month && clock.day() >= target.day))))
				{
					break;
				}
				MoveGameClockDays(1);
			}
		});
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Moves the date forward to the first event, and runs it.");

	ImGui::SeparatorText("Events");
	constexpr ImGuiTableFlags flags {ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY};
	if (ImGui::BeginTable("events", 2, flags))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 7.0f);
		ImGui::TableSetupColumn("Event");
		ImGui::TableHeadersRow();
		for (const EventView& event : view.events)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%04u-%02u-%02u", event.year, event.month, event.day);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(eventText(event.funcIndex));
		}
		ImGui::EndTable();
	}
}

// An input for a SIS value that sets it when the player presses Enter.
template <typename T>
void drawSisValue(const char* label, T value, ImGuiDataType type, void (*set)(SIS_STATE&, T))
{
	T edited {value};
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
	if (ImGui::InputScalar(label, type, &edited, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue) && edited != value)
	{
		changeGame([set, edited] {
			set(gameState().sis(), edited);
		});
	}
}

void drawShip()
{
	const GameView& view {*s_view};
	ImGui::Text("Ship: %s, captain %s", view.shipName.c_str(), view.commanderName.c_str());
	ImGui::Text("Activity: %s", activityText(view.activity).c_str());

	ImGui::SeparatorText("Resources");
	drawSisValue<uint32_t>("Resource units", view.resUnits, ImGuiDataType_U32, [](SIS_STATE& sis, uint32_t value) {
		sis.ResUnits = value;
	});
	drawSisValue<uint32_t>("Fuel (1/100 units)", view.fuel, ImGuiDataType_U32, [](SIS_STATE& sis, uint32_t value) {
		sis.FuelOnBoard = value;
	});
	drawSisValue<uint16_t>("Crew", view.crew, ImGuiDataType_U16, [](SIS_STATE& sis, uint16_t value) {
		sis.CrewEnlisted = value;
	});
	drawSisValue<uint8_t>("Landers", view.landers, ImGuiDataType_U8, [](SIS_STATE& sis, uint8_t value) {
		sis.NumLanders = value;
	});
	textDisabledWrapped("The status panel shows a new value when the game draws it again.");

	ImGui::SeparatorText("Location");
	ImGui::Text("Star map: %d, %d", view.logX, view.logY);
	ImGui::Text("In the star system: %d, %d", view.ipLocation.x, view.ipLocation.y);
	ImGui::Text("Planet: %u, orbit: %u, facing: %u", view.ipPlanet, view.inOrbit, view.shipFacing);
	if (view.hasAutopilot)
	{
		ImGui::Text("Autopilot: %d, %d", view.autopilot.x, view.autopilot.y);
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear"))
		{
			changeGame([] {
				flagship().autopilot.clear();
			});
		}
	}
	else
	{
		ImGui::TextUnformatted("Autopilot: none");
	}
}

void drawGameWindow(bool& open)
{
	ImGui::SetNextWindowSize(ImVec2 {560.0f, 680.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Game", &open))
	{
		ImGui::End();
		return;
	}

	receiveSnapshot();
	if (!s_requested && (!s_hasSnapshot || ImGui::GetTime() - s_lastRequest >= c_refreshSeconds))
	{
		requestSnapshot();
	}

	if (!s_view)
	{
		ImGui::TextDisabled(s_hasSnapshot && !s_requested ? "No game is in progress." : "Waiting for the game thread...");
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("game"))
	{
		if (ImGui::BeginTabItem("Flags"))
		{
			drawFlags();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Clock"))
		{
			drawClock();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Ship"))
		{
			drawShip();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

} // namespace

void addGameWindow()
{
	addToolWindow("Game", drawGameWindow);
}

} // namespace uqm
