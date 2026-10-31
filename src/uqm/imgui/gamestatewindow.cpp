// ImGui window that shows the game state as a save file holds it.

#include "gamestatewindow.h"

#include <cfloat>
#include <imgui.h>
#include <yyjson.h>
#include <fmt/format.h>
#include "core/log/log.h"
#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/string/StringUtils.h"
#include "core/thread/Mutex.h"
#include "libs/graphics/imgui/toolwindows.h"
#include "uqm/imgui/gamethread.h"
#include "uqm/save.h"
#include "uqm/starcon.h"

namespace uqm
{

namespace
{

using DocPtr = uqstl::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)>;

constexpr double c_refreshSeconds {1.0};
// Arrays of values longer than this show only their visible rows.
constexpr size_t c_clipArrayLength {64};
const ImVec4 c_changedColor {1.0f, 0.85f, 0.2f, 1.0f};

// The snapshot from the game thread.
struct SnapshotResult
{
	Mutex lock {};
	// Nullopt if no game runs.
	uqstl::optional<uqstl::string> json UQM_GUARDED_BY(lock) {};
	bool done UQM_GUARDED_BY(lock) {};
};

SnapshotResult s_result {};
DocPtr s_doc {nullptr, &yyjson_doc_free};
// The snapshot before s_doc, to find the values that changed.
DocPtr s_previous {nullptr, &yyjson_doc_free};
bool s_requested {};
bool s_hasSnapshot {};
bool s_noGame {};
bool s_autoRefresh {true};
double s_lastRequest {};
char s_filter[128] {};

void requestSnapshot()
{
	s_requested = true;
	s_hasSnapshot = true;
	s_lastRequest = ImGui::GetTime();
	runOnGameThread([] {
		uqstl::optional<uqstl::string> json {};
		if (IsGameInProgress())
		{
			json = GameStateJson();
		}
		LockGuard guard {s_result.lock};
		s_result.json = uqstl::move(json);
		s_result.done = true;
	});
}

void receiveSnapshot()
{
	uqstl::optional<uqstl::string> json {};
	{
		LockGuard guard {s_result.lock};
		if (!s_result.done)
		{
			return;
		}
		s_result.done = false;
		json = uqstl::move(s_result.json);
	}
	s_requested = false;

	s_noGame = !json;
	if (!json)
	{
		s_doc.reset();
		s_previous.reset();
		return;
	}
	DocPtr doc {yyjson_read(json->data(), json->size(), YYJSON_READ_ALLOW_INVALID_UNICODE), &yyjson_doc_free};
	if (!doc)
	{
		log::warn("Could not read the game state snapshot.");
		return;
	}
	s_previous = uqstl::move(s_doc);
	s_doc = uqstl::move(doc);
}

[[nodiscard]] bool containsICase(uqstl::string_view text, uqstl::string_view part)
{
	return uqstl::search(text.begin(), text.end(), part.begin(), part.end(), compareCharICase) != text.end();
}

[[nodiscard]] uqstl::string valueText(yyjson_val* val)
{
	switch (yyjson_get_type(val))
	{
		case YYJSON_TYPE_STR:
			return fmt::format("\"{}\"", yyjson_get_str(val));
		case YYJSON_TYPE_BOOL:
			return yyjson_get_bool(val) ? "true" : "false";
		case YYJSON_TYPE_NUM:
			if (yyjson_is_uint(val))
			{
				return fmt::format("{}", yyjson_get_uint(val));
			}
			if (yyjson_is_sint(val))
			{
				return fmt::format("{}", yyjson_get_sint(val));
			}
			return fmt::format("{}", yyjson_get_real(val));
		case YYJSON_TYPE_NULL:
			return "null";
		default:
			return {};
	}
}

// True if the path or the path of a value in the container contains the filter.
[[nodiscard]] bool matches(yyjson_val* val, uqstl::string& path, uqstl::string_view filter)
{
	if (containsICase(path, filter))
	{
		return true;
	}
	if (!yyjson_is_ctn(val))
	{
		return false;
	}
	const size_t pathSize {path.size()};
	bool found {};
	if (yyjson_is_obj(val))
	{
		size_t index {};
		size_t max {};
		yyjson_val* key {};
		yyjson_val* child {};
		yyjson_obj_foreach(val, index, max, key, child)
		{
			path += '/';
			path += yyjson_get_str(key);
			found = matches(child, path, filter);
			path.resize(pathSize);
			if (found)
			{
				break;
			}
		}
	}
	else
	{
		size_t index {};
		size_t max {};
		yyjson_val* child {};
		yyjson_arr_foreach(val, index, max, child)
		{
			path += fmt::format("/{}", index);
			found = matches(child, path, filter);
			path.resize(pathSize);
			if (found)
			{
				break;
			}
		}
	}
	return found;
}

void drawChildren(yyjson_val* val, uqstl::string& path, uqstl::string_view filter);

// Draws one table row for the value, and the rows of its children if it is open. 'path' is the JSON pointer of the value.
void drawNode(uqgsl::czstring name, yyjson_val* val, uqstl::string& path, uqstl::string_view filter)
{
	if (!filter.empty() && !matches(val, path, filter))
	{
		return;
	}
	// The children of a container whose own path matches are all shown.
	const uqstl::string_view childFilter {containsICase(path, filter) ? uqstl::string_view {} : filter};

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	if (yyjson_is_ctn(val))
	{
		if (!filter.empty())
		{
			ImGui::SetNextItemOpen(true);
		}
		const bool open {ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanFullWidth)};
		ImGui::TableNextColumn();
		ImGui::TextDisabled(yyjson_is_arr(val) ? "[%zu]" : "{%zu}", yyjson_get_len(val));
		if (open)
		{
			drawChildren(val, path, childFilter);
			ImGui::TreePop();
		}
		return;
	}

	ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
	ImGui::TableNextColumn();
	const uqstl::string text {valueText(val)};
	yyjson_val* const old {s_previous ? yyjson_doc_ptr_getn(s_previous.get(), path.data(), path.size()) : nullptr};
	if (s_previous && (old == nullptr || !yyjson_equals(old, val)))
	{
		ImGui::TextColored(c_changedColor, "%s", text.c_str());
		if (old != nullptr)
		{
			ImGui::SetItemTooltip("Was %s", valueText(old).c_str());
		}
	}
	else
	{
		ImGui::TextUnformatted(text.c_str());
	}
}

void drawArrayElement(yyjson_val* array, size_t index, uqstl::string& path, uqstl::string_view filter)
{
	const size_t pathSize {path.size()};
	const uqstl::string name {fmt::format("{}", index)};
	path += '/';
	path += name;
	drawNode(name.c_str(), yyjson_arr_get(array, index), path, filter);
	path.resize(pathSize);
}

void drawChildren(yyjson_val* val, uqstl::string& path, uqstl::string_view filter)
{
	const size_t pathSize {path.size()};
	if (yyjson_is_obj(val))
	{
		size_t index {};
		size_t max {};
		yyjson_val* key {};
		yyjson_val* child {};
		yyjson_obj_foreach(val, index, max, key, child)
		{
			const char* const name {yyjson_get_str(key)};
			path += '/';
			path += name;
			ImGui::PushID(static_cast<int>(index));
			drawNode(name, child, path, filter);
			ImGui::PopID();
			path.resize(pathSize);
		}
		return;
	}

	const size_t length {yyjson_arr_size(val)};
	if (filter.empty() && length > c_clipArrayLength && !yyjson_is_ctn(yyjson_arr_get_first(val)))
	{
		ImGuiListClipper clipper {};
		clipper.Begin(static_cast<int>(length));
		while (clipper.Step())
		{
			for (int row {clipper.DisplayStart}; row < clipper.DisplayEnd; ++row)
			{
				ImGui::PushID(row);
				drawArrayElement(val, static_cast<size_t>(row), path, filter);
				ImGui::PopID();
			}
		}
		return;
	}
	for (size_t index {0}; index < length; ++index)
	{
		ImGui::PushID(static_cast<int>(index));
		drawArrayElement(val, index, path, filter);
		ImGui::PopID();
	}
}

void copyJson()
{
	size_t length {};
	const uqstl::unique_ptr<char, decltype(&free)> text {
		yyjson_write(s_doc.get(), YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INVALID_UNICODE, &length), &free};
	if (text)
	{
		ImGui::SetClipboardText(text.get());
	}
}

void drawGameStateWindow(bool& open)
{
	ImGui::SetNextWindowSize(ImVec2 {520.0f, 700.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Game State", &open))
	{
		ImGui::End();
		return;
	}

	receiveSnapshot();
	if (!s_requested && (!s_hasSnapshot || (s_autoRefresh && ImGui::GetTime() - s_lastRequest >= c_refreshSeconds)))
	{
		requestSnapshot();
	}

	ImGui::BeginDisabled(s_requested);
	if (ImGui::Button("Refresh"))
	{
		requestSnapshot();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Checkbox("Auto refresh", &s_autoRefresh);
	ImGui::SetItemTooltip("Take a new snapshot each second.");
	ImGui::SameLine();
	ImGui::BeginDisabled(!s_doc);
	if (ImGui::Button("Copy JSON"))
	{
		copyJson();
	}
	ImGui::EndDisabled();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##filter", "Filter, for example gameState/shofixti or sis/fuel", s_filter, sizeof(s_filter));

	if (s_noGame)
	{
		ImGui::TextDisabled("No game is in progress.");
	}
	else if (!s_doc)
	{
		ImGui::TextDisabled("Waiting for the game thread...");
	}
	else
	{
		ImGui::TextDisabled("Values that changed in the last snapshot are yellow. (?)");
		ImGui::SetItemTooltip("The snapshot does not update the location and facing of the flagship, so they are as of the last change of activity.");
		constexpr ImGuiTableFlags flags {ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable
										 | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY};
		if (ImGui::BeginTable("state", 2, flags))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.6f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.4f);
			ImGui::TableHeadersRow();
			uqstl::string path {};
			drawChildren(yyjson_doc_get_root(s_doc.get()), path, s_filter);
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

} // namespace

void addGameStateWindow()
{
	addToolWindow("Game State", drawGameStateWindow);
}

} // namespace uqm
