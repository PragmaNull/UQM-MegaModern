// Copyright Michael Martin, 2004.

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "setupmenu.h"

#include <math.h>

#include "colors.h"
#include "configuration/Configuration.h"
#include "controls.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "fmv.h"
#include "gamestr.h"
#include "libs/file/textfile.h"
#include "libs/gfxlib.h"
#include "libs/graphics/bbox.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/graphics/widgets.h"
#include "libs/inplib.h"
#include "libs/input/input_common.h"
#include "libs/math/Random.h"
#include "libs/reslib.h"
#include "libs/sound/sound.h"
#include "libs/strlib.h"
#include "libs/vidlib.h"
#include "nameref.h"
#include "options.h"
#include "resinst.h"
#include "setup.h"
#include "sounds.h"
#include "uqm/menuwidgets.h"
#include "uqm/random/GameSeed.h"
#include "uqm/setupsettings.h"

namespace uqm
{
struct SetupMenuState
{
	bool (*InputFunc)(struct SetupMenuState* pInputState);

	bool initialized;
	int anim_frame_count;
	TimeCount NextTime;
};

static SoundBankRef testSounds;

static bool DoSetupMenu(SetupMenuState* pInputState);
static Widget *current, *next;
static uqstl::unique_ptr<uqm::MenuWidgets> menuWidgets {};

static uqstl::optional<uqm::MenuDefinition> menuDefinition {};
static bool mainMenuMusicChanged {};

// One screen for each menu of the menu definition, in the same order.
static uqstl::vector<WidgetMenuScreen> menus {};
static WidgetButton backButton {};
static WidgetButton exitButton {};
static WidgetLabel spacer {};
static WidgetLabel pressToEdit {};
static uqstl::vector<const char*> pressToEditLines {};
static WidgetSlider gammaSlider {};
static WidgetTextEntry layoutName {};
static WidgetControlEntry controlentries[*BattleKey::NumKeys] {};
static constexpr const char* c_controlNames[*BattleKey::NumKeys] {"up", "down", "left", "right", "weapon", "special", "escape", "thrust"};

// The control template that the Edit Controls menu shows.
static int editTemplate {};

// The setup menu edits the settings. The new game menu edits the seed of the new game.
enum class MenuMode
{
	Setup,
	NewGame,
};
static MenuMode menuMode {MenuMode::Setup};

// The seed values that the new game menu edits. The menu does not save them.
struct NewGameSettings final : uqm::SettingsRegistry
{
	uqm::Setting<uqm::SeedType> seedType {
		*this, {.key = "seed.type", .description = "Seed type of the new game."},
		  uqm::SeedType::Original
	 };
	uqm::Setting<int> customSeed {
		*this, {.key = "seed.value", .description = "Seed of the new game."},
		  PrimeA, uqm::settings::isSaneSeed
	   };
	uqm::Setting<bool> shipSeed {
		*this, {.key = "seed.ships", .description = "Seeds the ships of the alien races."},
			false
	 };
};
static uqstl::unique_ptr<NewGameSettings> newGameSettings {};
static bool newGameStarted {};
static WidgetButton randomSeedButton {};
static const uqstl::string c_noLabel {};
static const uqstl::vector<uqstl::string> c_noTooltip {};

static FrameRef setup_frame = nullptr;

static void populate_gamma(void);
static int OnTextEntryEvent(WidgetTextEntry* widget);

// Start with reasonable gamma bounds. These will get updated
// as we find out the actual bounds.
static float minGamma = 0.4f;
static float maxGamma = 2.5f;
// The gamma slider uses an exponential curve
// We use y = e^(2.1972*(x-1)) curve to give us a nice spread of
// gamma values 0.11 < g < 9.0 centered at g=1.0
static constexpr auto GammaCurveB {2.1972f};
static float minGammaX;
static float maxGammaX;

static Widget* find_menu(uqstl::string_view id)
{
	const uqm::MenuDef* const menu {menuDefinition->find(id)};
	if (menu == nullptr)
	{
		return nullptr;
	}
	return (Widget*)(&menus[static_cast<size_t>(menu - menuDefinition->menus.data())]);
}

static int quit_main_menu(Widget* self, WidgetEvent event)
{
	if (event == WidgetEvent::Select)
	{
		next = nullptr;
		return true;
	}
	(void)self;
	return false;
}

static int quit_sub_menu(Widget* self, WidgetEvent event)
{
	if (event == WidgetEvent::Select)
	{
		ResetOffset();
		next = find_menu("main");
		(*next->receiveFocus)(next, WidgetEvent::Select);
		return true;
	}
	(void)self;
	return false;
}

static int start_new_game(Widget* self, WidgetEvent event)
{
	if (event == WidgetEvent::Select)
	{
		newGameStarted = true;
		next = nullptr;
		return true;
	}
	(void)self;
	return false;
}

static int cancel_new_game(Widget* self, WidgetEvent event)
{
	if (event == WidgetEvent::Select)
	{
		newGameStarted = false;
		next = nullptr;
		return true;
	}
	(void)self;
	return false;
}

static int random_seed(Widget* self, WidgetEvent event)
{
	if (event != WidgetEvent::Select)
	{
		(void)self;
		return false;
	}
	if (*newGameSettings->seedType == uqm::SeedType::Original)
	{
		newGameSettings->seedType = uqm::SeedType::Planet;
	}
	newGameSettings->customSeed = uqm::randomSeedValue();
	menuWidgets->refresh();
	return true;
}

// Keeps the values of the new game menu valid together. The type Original has no seed value and no ship seeding.
static void new_game_setting_changed(const uqm::SettingBase& setting)
{
	NewGameSettings& values {*newGameSettings};
	if (setting.key() == "seed.type")
	{
		if (*values.seedType == uqm::SeedType::Original)
		{
			values.customSeed = PrimeA;
			values.shipSeed = false;
		}
	}
	else if (*values.seedType == uqm::SeedType::Original && (*values.customSeed != PrimeA || *values.shipSeed))
	{
		values.seedType = uqm::SeedType::Planet;
	}
	menuWidgets->refresh();
}

static void populate_editkeys(int templat)
{
	int j {};

	uqm::strncpy_safe(layoutName.value, {uqm::input_templates[templat].name, static_cast<uint32_t>(layoutName.maxlen)});

	for (int i {0}; i < BattleKey::NumKeys; i++)
	{
		for (j = 0; j < 2; j++)
		{
			InterrogateInputState(templat, i, j,
								  controlentries[i].controlname[j],
								  WidgetControlentryWidth);
		}
	}
}

static void rename_template(WidgetTextEntry* self)
{
	uqm::strncpy_safe(uqm::input_templates[editTemplate].name, self->value);
}

static constexpr auto MenuFrameRate {c_gameSecond / 20};

static bool DoSetupMenu(SetupMenuState* pInputState)
{
	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (!pInputState->initialized)
	{
		SetDefaultMenuRepeatDelay();
		pInputState->NextTime = GetTimeCounter();
		if (menuMode == MenuMode::Setup)
		{
			uqm::setDisplayFlags(uqm::UQMOptions::edit(), g_gfxFlags);
			populate_gamma();
		}
		menuWidgets->refresh();
		Widget_SetFont(PlyrFont);
		Widget_SetWindowColors(ShadowboxBackgroundColor,
							   ShadowboxDarkColor, ShadowboxMediumColor);

		current = nullptr;
		next = find_menu("main");
		(*next->receiveFocus)(next, WidgetEvent::Down);

		pInputState->initialized = true;
	}
	if (current != next)
	{
		SetTransitionSource(nullptr);
	}

	BatchGraphics();
	(*next->draw)(next, 0, 0);

	if (current != next)
	{
		ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, nullptr);
		current = next;
	}

	UnbatchGraphics();

	if (PulsedInputState.menu[*MenuKey::MenuUp])
	{
		Widget_Event(WidgetEvent::Up);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuDown])
	{
		Widget_Event(WidgetEvent::Down);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuLeft])
	{
		Widget_Event(WidgetEvent::Left);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuRight])
	{
		Widget_Event(WidgetEvent::Right);
	}
	if (PulsedInputState.menu[*MenuKey::MenuSelect])
	{
		Widget_Event(WidgetEvent::Select);
	}
	if (PulsedInputState.menu[*MenuKey::MenuCancel])
	{
		Widget_Event(WidgetEvent::Cancel);
	}
	if (PulsedInputState.menu[*MenuKey::MenuDelete])
	{
		Widget_Event(WidgetEvent::Delete);
	}

	SleepThreadUntil(pInputState->NextTime + MenuFrameRate);
	pInputState->NextTime = GetTimeCounter();
	return !((uqm::currentActivity() & ActivityFlag::CheckAbort) || (next == nullptr));
}

static void redraw_menu(void)
{
	BatchGraphics();
	(*next->draw)(next, 0, 0);
	UnbatchGraphics();
}

static bool OnTextEntryChange(TextEntryState* pTES)
{
	WidgetTextEntry* widget = (WidgetTextEntry*)pTES->CbParam;

	widget->cursor_pos = pTES->CursorPos;
	if (pTES->JoystickMode)
	{
		widget->state |= TextEntryWidgetState::BlockCursor;
	}
	else
	{
		widget->state &= ~TextEntryWidgetState::BlockCursor;
	}

	// XXX TODO: Here, we can examine the text entered so far
	// to make sure it fits on the screen, for example,
	// and return false to disallow the last change

	return true; // allow change
}

static bool OnTextEntryFrame(TextEntryState* pTES)
{
	redraw_menu();

	SleepThreadUntil(pTES->NextTime);
	pTES->NextTime = GetTimeCounter() + MenuFrameRate;

	return true; // continue
}

static int OnTextEntryEvent(WidgetTextEntry* widget)
{ // Going to edit the text
	TextEntryState tes;
	char revert_buf[256];

	// position cursor at the end of text
	widget->cursor_pos = uqm::utf8StringCount(widget->value);
	widget->state = TextEntryWidgetState::Editing;
	redraw_menu();

	// make a backup copy for revert on cancel
	uqm::utf8StringCopy(revert_buf, sizeof(revert_buf), widget->value);

	// text entry setup
	tes.Initialized = false;
	tes.NextTime = GetTimeCounter() + MenuFrameRate;
	tes.BaseStr = widget->value;
	tes.MaxSize = widget->maxlen;
	tes.CursorPos = widget->cursor_pos;
	tes.CbParam = widget;
	tes.ChangeCallback = OnTextEntryChange;
	tes.FrameCallback = OnTextEntryFrame;

	SetMenuSounds(MenuSoundNone, MenuSoundSelect);
	if (!DoTextEntry(&tes))
	{ // editing failed (canceled) -- revert the changes
		uqm::utf8StringCopy(widget->value, widget->maxlen, revert_buf);
	}
	else
	{
		if (widget->onChange)
		{
			(*(widget->onChange))(widget);
		}
	}
	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	widget->state = TextEntryWidgetState::Normal;
	redraw_menu();

	return true; // event handled
}

static inline float gammaCurve(float x)
{
	// The slider uses an exponential curve
	return exp((x - 1) * GammaCurveB);
}

static inline float solveGammaCurve(float y)
{
	return std::log(y) / GammaCurveB + 1;
}

static int gammaToSlider(float gamma)
{
	const float x = solveGammaCurve(gamma);
	const float step = (maxGammaX - minGammaX) / 100;
	return (int)((x - minGammaX) / step + 0.5);
}

static float sliderToGamma(int value)
{
	const float step = (maxGammaX - minGammaX) / 100;
	const float x = minGammaX + step * value;
	const float g = gammaCurve(x);
	// report any value that is close enough as 1.0
	return (fabs(g - 1.0f) < 0.001f) ? 1.0f : g;
}

static void updateGammaBounds(bool useUpper)
{
	float g {NAN};
	float x {NAN};

	// The slider uses an exponential curve.
	// Calculate where on the curve the min and max gamma values are
	minGammaX = solveGammaCurve(minGamma);
	maxGammaX = solveGammaCurve(maxGamma);

	// We have 100 discrete steps through the range, so the slider may
	// skip over a 1.0 gamma. We need to ensure that there always is
	// a 1.0 on the slider by tweaking the range (expanding/contracting).
	const int slider {gammaToSlider(1.0f)};
	g = sliderToGamma(slider);
	if (g == 1.0f)
	{
		return; // no adjustment needed
	}

	x = solveGammaCurve(g);
	if (useUpper)
	{ // Move the upper bound up or down to land on 1.0
		const float d = (x - 1.0f) * 100 / slider;
		maxGammaX -= d;
		maxGamma = gammaCurve(maxGammaX);
	}
	else
	{ // Move the lower bound up or down to land on 1.0
		const float d = (x - 1.0f) * 100 / (100 - slider);
		minGammaX -= d;
		minGamma = gammaCurve(minGammaX);
	}
}

static int gamma_HandleEventSlider(Widget* _self, WidgetEvent event)
{
	WidgetSlider* self = (WidgetSlider*)_self;
	const int prevValue = self->value;
	float gamma {NAN};
	bool set {};

	switch (event)
	{
		case WidgetEvent::Left:
			self->value -= self->step;
			break;
		case WidgetEvent::Right:
			self->value += self->step;
			break;
		default:
			return false;
	}

	// Limit the slider to values accepted by gfx subsys
	gamma = sliderToGamma(self->value);
	set = TFB_SetGamma(gamma);
	if (!set)
	{ // revert
		self->value = prevValue;
		gamma = sliderToGamma(self->value);
	}

	// Grow or shrink the range based on accepted values
	if (gamma < minGamma || (!set && event == WidgetEvent::Left))
	{
		gamma = minGamma;
		updateGammaBounds(true);
		// at the lowest end
		self->value = 0;
	}
	else if (gamma > maxGamma || (!set && event == WidgetEvent::Right))
	{
		gamma = maxGamma;
		updateGammaBounds(false);
		// at the highest end
		self->value = 100;
	}
	uqm::UQMOptions::edit().gamma = sliderToGamma(self->value);
	return true;
}

static void populate_gamma(void)
{
	const float gamma {uqm::UQMOptions::read().gamma.value};
	const bool whichBound {gamma < maxGamma};
	// The option supplied by the user may be beyond our starting range
	// but valid nonetheless. We need to account for that.
	if (gamma <= minGamma)
	{
		minGamma = gamma - 0.03f;
	}
	else if (gamma >= maxGamma)
	{
		maxGamma = gamma + 0.3f;
	}
	updateGammaBounds(whichBound);
	gammaSlider.value = gammaToSlider(gamma);
}

static void gamma_DrawValue(WidgetSlider* self, int x, int y)
{
	GfxText t;
	char buf[16] {};
	float gamma = sliderToGamma(self->value);
	fmt::format_to_sz_n(buf, "{:.4}", gamma);

	t.baseline.x = x + ResScale(6);
	t.baseline.y = y;
	t.align = TextAlign::Left;
	t.CharCount = ~0;
	t.pStr = buf;

	font_DrawText(&t);
}

static void rebind_control(WidgetControlEntry* widget)
{
	const int templat = editTemplate;
	const int control = widget->controlindex;
	const int index = widget->highlighted;

	FlushInput();
	DrawLabelAsWindow(&pressToEdit, nullptr);
	RebindInputState(templat, control, index);
	populate_editkeys(templat);
	FlushInput();
}

static void clear_control(WidgetControlEntry* widget)
{
	const int templat = editTemplate;
	const int control = widget->controlindex;
	const int index = widget->highlighted;

	RemoveInputState(templat, control, index);
	populate_editkeys(templat);
}

static int count_widgets(Widget** widgets)
{
	int count {};

	for (count = 0; *widgets != nullptr; ++widgets, ++count)
		;
	return count;
}

static bool open_menu(uqstl::string_view id)
{
	Widget* menu {find_menu(id)};
	if (menu == nullptr)
	{
		return false;
	}
	if (id == "editkeys")
	{
		editTemplate = 0;
		populate_editkeys(0);
		menuWidgets->refresh();
	}
	next = menu;
	(*next->receiveFocus)(next, WidgetEvent::Down);
	return true;
}

static void read_menu_definition(void)
{
	menuDefinition = menuMode == MenuMode::NewGame ? uqm::readMenuDefinition("ui.newgamemenu", *newGameSettings) : uqm::readSetupMenuDefinition();
	if (!menuDefinition)
	{
		uqm::log::critical("PANIC: Could not read the setup menu definition.");
		exit(EXIT_FAILURE);
	}
}

// Sets up the widgets that the menu definition does not build.
static void init_shared_widgets(void)
{
	const bool newGame {menuMode == MenuMode::NewGame};
	uqm::initButtonWidget(backButton, menuDefinition->back.label, menuDefinition->back.tooltip, newGame ? cancel_new_game : quit_sub_menu);
	uqm::initButtonWidget(exitButton, menuDefinition->exit.label, menuDefinition->exit.tooltip, newGame ? start_new_game : quit_main_menu);
	uqm::initButtonWidget(randomSeedButton, c_noLabel, c_noTooltip, random_seed);

	static uqstl::vector<const char*> noLines {};
	uqm::initLabelWidget(spacer, noLines);

	pressToEditLines.clear();
	if (const auto found {menuDefinition->messages.find("pressToEdit")}; found != menuDefinition->messages.end())
	{
		for (const uqstl::string& line : found->second)
		{
			pressToEditLines.push_back(line.c_str());
		}
	}
	uqm::initLabelWidget(pressToEdit, pressToEditLines);

	gammaSlider = {};
	gammaSlider.tag = WidgetType::Slider;
	gammaSlider.handleEvent = gamma_HandleEventSlider;
	gammaSlider.receiveFocus = Widget_ReceiveFocusSimple;
	gammaSlider.draw = Widget_DrawSlider;
	gammaSlider.height = Widget_HeightOneLine;
	gammaSlider.width = Widget_WidthFullScreen;
	gammaSlider.draw_value = gamma_DrawValue;
	gammaSlider.min = 0;
	gammaSlider.max = 100;
	gammaSlider.step = 1;

	layoutName = {};
	layoutName.tag = WidgetType::TextEntry;
	layoutName.handleEvent = Widget_HandleEventTextEntry;
	layoutName.receiveFocus = Widget_ReceiveFocusSimple;
	layoutName.draw = Widget_DrawTextEntry;
	layoutName.height = Widget_HeightOneLine;
	layoutName.width = Widget_WidthFullScreen;
	layoutName.handleEventSelect = OnTextEntryEvent;
	layoutName.maxlen = WidgetTextentryWidth - 1;
	layoutName.state = TextEntryWidgetState::Normal;
	layoutName.onChange = rename_template;

	for (int i {0}; i < BattleKey::NumKeys; i++)
	{
		WidgetControlEntry& entry {controlentries[i]};
		entry = {};
		entry.tag = WidgetType::ControlEntry;
		entry.handleEvent = Widget_HandleEventControlEntry;
		entry.receiveFocus = Widget_ReceiveFocusControlEntry;
		entry.draw = Widget_DrawControlEntry;
		entry.height = Widget_HeightOneLine;
		entry.width = Widget_WidthFullScreen;
		entry.controlindex = i;
		entry.onChange = rebind_control;
		entry.onDelete = clear_control;
	}
}

static void init_widgets(void)
{
	if (setup_frame == nullptr || optRequiresReload)
	{
		// Load the different menus depending on the resolution factor.
		setup_frame = CaptureDrawable(LoadGraphic(MenubkgPmapAnimRes));
		LoadArrows();
	}

	read_menu_definition();
	init_shared_widgets();

	uqm::MenuWidgets::Hooks hooks {};
	hooks.isAddonAvailable = uqm::isMenuAddonAvailable;
	hooks.addonUnavailable = [] {
		DoPopupWindow(GameString(GameStringBase::MainmenuString + 36));
		Widget_SetFont(PlyrFont);
	};
	hooks.settingChanged = [](const uqm::SettingBase& setting) {
		if (menuMode == MenuMode::NewGame)
		{
			new_game_setting_changed(setting);
			return;
		}
		if (uqm::applySettingChange(setting))
		{
			menuWidgets->refresh();
		}
		if (setting.key() == "audio.mainMenuMusic")
		{
			mainMenuMusicChanged = true;
		}
		else if (setting.key() == "audio.speechVolume")
		{
			TestSpeechSound(GetSound(testSounds, speechVolumeScale >= 1.0f));
		}
	};
	hooks.optionLabel = [](uqstl::string_view list, size_t index) -> const char* {
		if (list == "controlTemplates" && index < uqstl::size(uqm::input_templates))
		{
			return uqm::input_templates[index].name;
		}
		return nullptr;
	};
	hooks.getState = [](uqstl::string_view key) -> uqstl::optional<int> {
		if (key == "controls.editTemplate")
		{
			return editTemplate;
		}
		if (const uqm::CheatAction* cheat {uqm::findCheatState(key)})
		{
			return static_cast<int>(*cheat);
		}
		return uqstl::nullopt;
	};
	hooks.setState = [](uqstl::string_view key, int value) {
		if (key == "controls.editTemplate")
		{
			editTemplate = value;
			populate_editkeys(value);
		}
		else if (uqm::CheatAction * cheat {uqm::findCheatState(key)})
		{
			*cheat = static_cast<uqm::CheatAction>(value);
		}
	};
	hooks.openMenu = open_menu;
	hooks.textEntrySelect = OnTextEntryEvent;
	uqm::MenuWidgets::SharedWidgets shared {};
	shared.spacer = (Widget*)(&spacer);
	shared.back = (Widget*)(&backButton);
	shared.exit = (Widget*)(&exitButton);
	shared.legacy = {
		{"gamma",	  (Widget*)(&gammaSlider)	 },
		{"layoutName", (Widget*)(&layoutName)		 },
		{"randomSeed", (Widget*)(&randomSeedButton)},
	};
	for (int i {0}; i < BattleKey::NumKeys; i++)
	{
		shared.legacy[uqstl::string {"control."} + c_controlNames[i]] = (Widget*)(&controlentries[i]);
	}
	uqm::SettingsRegistry& settings {menuMode == MenuMode::NewGame ? static_cast<uqm::SettingsRegistry&>(*newGameSettings) : uqm::UQMOptions::edit()};
	menuWidgets = uqstl::make_unique<uqm::MenuWidgets>(settings, shared, uqstl::move(hooks));

	menus.assign(menuDefinition->menus.size(), WidgetMenuScreen {});
	for (auto&& [menu, screen] : uqstl::views::zip(menuDefinition->menus, menus))
	{
		screen.tag = WidgetType::MenuScreen;
		screen.handleEvent = Widget_HandleEventMenuScreen;
		screen.receiveFocus = Widget_ReceiveFocusMenuScreen;
		screen.draw = Widget_DrawMenuScreen;
		screen.height = Widget_HeightFullScreen;
		screen.width = Widget_WidthFullScreen;
		screen.title = menuDefinition->title.c_str();
		screen.subtitle = menu.subtitle.c_str();
		screen.bgStamp.frame = SetAbsFrameIndex(setup_frame, menu.background);
		screen.child = menuWidgets->build(menu);
		screen.num_children = count_widgets(screen.child);
	}

	testSounds = uqm::loadSoundBank(TestSoundsRes).release();
}

static void clean_up_widgets(void)
{
	menus.clear();
	menuWidgets.reset();
	menuDefinition.reset();

	if (setup_frame)
	{
		DestroyDrawable(ReleaseDrawable(setup_frame));
		setup_frame = nullptr;
		ReleaseArrows();
	}

	if (testSounds)
	{
		delete testSounds;
		testSounds = 0;
	}
}

// Logs the open setup menu for the "dumpmenu" input script command.
static void dump_setup_menu(void)
{
	if (current == nullptr || current->tag != WidgetType::MenuScreen)
	{
		uqm::log::warn("Input script: dumpmenu: no setup menu is shown.");
		return;
	}

	const auto& menu {*reinterpret_cast<const WidgetMenuScreen*>(current)};
	uqm::log::info("Menu: {}", menu.subtitle ? menu.subtitle : "");
	for (int i {0}; i < menu.num_children; ++i)
	{
		const Widget* child {menu.child[i]};
		const char marker {i == menu.highlighted ? '>' : ' '};
		switch (child->tag)
		{
			case WidgetType::Choice:
				{
					const auto& choice {*reinterpret_cast<const WidgetChoice*>(child)};
					const bool valid {choice.selected >= 0 && choice.selected < choice.numopts};
					uqm::log::info("{} {}: {}", marker, choice.category, valid ? choice.options[choice.selected].optname : "?");
					break;
				}
			case WidgetType::Slider:
				{
					const auto& slider {*reinterpret_cast<const WidgetSlider*>(child)};
					uqm::log::info("{} {}: {}", marker, slider.category, slider.value);
					break;
				}
			case WidgetType::Button:
				uqm::log::info("{} [{}]", marker, reinterpret_cast<const WidgetButton*>(child)->name);
				break;
			case WidgetType::Label:
				{
					const auto& label {*reinterpret_cast<const WidgetLabel*>(child)};
					uqm::log::info("{} -- {}", marker, label.line_count > 0 ? label.lines[0] : "");
					break;
				}
			case WidgetType::TextEntry:
				{
					const auto& entry {*reinterpret_cast<const WidgetTextEntry*>(child)};
					uqm::log::info("{} {}: \"{}\"", marker, entry.category, entry.value);
					break;
				}
			case WidgetType::ControlEntry:
				{
					const auto& entry {*reinterpret_cast<const WidgetControlEntry*>(child)};
					uqm::log::info("{} {}: {} | {}", marker, entry.category, entry.controlname[0], entry.controlname[1]);
					break;
				}
			default:
				uqm::log::info("{} (widget type {})", marker, static_cast<int>(child->tag));
				break;
		}
	}
}

// Applies the settings that take effect when the player leaves the setup menu.
static void apply_settings(void)
{
	auto& uqmOptions {uqm::UQMOptions::edit()};

	const int factor {uqm::isAddonAvailable(HdMode) ? *uqmOptions.resolutionFactor : 0};
	if (factor != (int)resolutionFactor)
	{
		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
		resolutionFactor = factor;
	}

	uqm::WindowStyle windowStyle {*uqmOptions.windowEmulationMode};
	if (windowStyle != uqm::WindowStyle::UQM && !uqm::isAddonAvailable(WindowModeName(resolutionFactor, uqstl::to_underlying(windowStyle))))
	{
		windowStyle = uqm::WindowStyle::UQM;
		uqmOptions.windowEmulationMode = windowStyle;
	}
	if (optWindowType != windowStyle)
	{
		optWindowType = windowStyle;
		optRequiresReload = true;
	}

	if (mainMenuMusicChanged)
	{
		mainMenuMusicChanged = false;
		if (uqmOptions.mainMenuMusic)
		{
			InitMenuMusic();
		}
		else
		{
			UninitMenuMusic();
		}
	}

	if ((uqm::UQMOptions::edit().optDifficulty = *uqmOptions.optDiffChooser) == uqm::Difficulty::ChooseYourOwn)
	{
		uqm::UQMOptions::edit().optDifficulty = uqm::Difficulty::Normal;
	}

	res_PutString("keys.1.name", uqm::input_templates[0].name);
	res_PutString("keys.2.name", uqm::input_templates[1].name);
	res_PutString("keys.3.name", uqm::input_templates[2].name);
	res_PutString("keys.4.name", uqm::input_templates[3].name);
	res_PutString("keys.5.name", uqm::input_templates[4].name);
	res_PutString("keys.6.name", uqm::input_templates[5].name);

	uqm::saveUserSettings(uqmOptions);
	SaveKeyConfiguration(uqm::configDir());

	if (optRequiresReload)
	{
		int w = WindowWidth;
		int h = WindowHeight;

		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));

		FlushGraphics();
		UninitVideoPlayer();

		ResetOffset();

		resolutionFactor = resolutionFactor;
		CanvasWidth = 320 << resolutionFactor;
		CanvasHeight = DosBool(240, 200) << resolutionFactor;

		if (*uqmOptions.loresBlowupScale < 6)
		{
			h = DosBool(240, 200) * (uqmOptions.loresBlowupScale.value + 1);
			SavedHeight = h;
			uqmOptions.resolution.value.y = static_cast<uint32_t>(SavedHeight);
		}

		if (uqmOptions.keepAspectRatio)
		{
			const float threshold = 0.75f;
			const float ratio = (float)h / (float)w;

			if (ratio > threshold) // screen is narrower than 4:3
			{
				w = h / threshold;
			}
			else if (ratio < threshold) // screen is wider than 4:3
			{
				h = w * threshold;
			}
		}

		uqm::log::debug("ScreenWidth:{}, ScreenHeight:{}, "
						"Wactual:{}, Hactual:{}",
						CanvasWidth, CanvasHeight,
						w, h);

		// These solve the context problem that plagued the setupmenu
		// when changing to higher resolution.
		TFB_BBox_Reset();
		TFB_BBox_Init(CanvasWidth, CanvasHeight);
		FlushColorXForms();

		TFB_DrawScreen_ReinitVideo(GraphicsDriver, g_gfxFlags, w, h);
		InitVideoPlayer(true);

		Reload();
	}
}

void SetupMenu(void)
{
	SetupMenuState s;

	s.InputFunc = DoSetupMenu;
	s.initialized = false;
	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
	init_widgets();

	SetInputScriptMenuDump(dump_setup_menu);
	DoInput(&s, true);
	SetInputScriptMenuDump(nullptr);
	uqm::currentActivity() &= ~ActivityFlag::CheckAbort;
	apply_settings();
	clean_up_widgets();

	SetMenuSounds(MenuSoundUp | MenuSoundDown,
				  MenuSoundSelect);
}

uqstl::optional<uqm::GameSeed> NewGameMenu(void)
{
	const uqm::GameSeed defaults {uqm::newGameSeed(uqm::UQMOptions::read())};
	newGameSettings = uqstl::make_unique<NewGameSettings>();
	newGameSettings->seedType = defaults.type;
	newGameSettings->customSeed = defaults.value;
	newGameSettings->shipSeed = defaults.ships;
	menuMode = MenuMode::NewGame;
	newGameStarted = false;

	SetupMenuState s;
	s.InputFunc = DoSetupMenu;
	s.initialized = false;
	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
	init_widgets();

	SetInputScriptMenuDump(dump_setup_menu);
	DoInput(&s, true);
	SetInputScriptMenuDump(nullptr);
	clean_up_widgets();

	uqstl::optional<uqm::GameSeed> result {};
	if (newGameStarted && !(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		result = uqm::normalized({.type = *newGameSettings->seedType, .value = *newGameSettings->customSeed, .ships = *newGameSettings->shipSeed});
	}
	newGameSettings.reset();
	menuMode = MenuMode::Setup;

	SetMenuSounds(MenuSoundUp | MenuSoundDown,
				  MenuSoundSelect);
	return result;
}

} // namespace uqm
