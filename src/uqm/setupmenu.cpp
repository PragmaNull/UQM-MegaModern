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

#include "controls.h"
#include "options.h"
#include "setup.h"
#include "sounds.h"
#include "colors.h"
#include "fmv.h"
#include "core/string/StringUtils.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/widgets.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/strlib.h"
#include "libs/reslib.h"
#include "libs/inplib.h"
#include "libs/vidlib.h"
#include "libs/sound/sound.h"
#include "core/log/log.h"
#include "resinst.h"
#include "nameref.h"
#include <math.h>
#include "gamestr.h"
#include "libs/graphics/bbox.h"
#include "libs/math/random.h"
#include "libs/input/input_common.h"
#include "configuration/Configuration.h"
#include "libs/file/textfile.h"
#include "uqm/menuwidgets.h"

#include "SDL_version.h"

#if defined(HAVE_OPENGL)
#define HAVE_OPENGL_RENDERER true
#else
#define HAVE_OPENGL_RENDERER false
#endif

#if defined(DIRECTIONAL_JOY)
#define HAVE_DIRECTIONAL_JOYSTICK true
#else
#define HAVE_DIRECTIONAL_JOYSTICK false
#endif

typedef struct setup_menu_state
{
	bool (*InputFunc)(struct setup_menu_state* pInputState);

	bool initialized;
	int anim_frame_count;
	uint32_t NextTime;
} SETUP_MENU_STATE;

static SOUND testSounds;

static bool DoSetupMenu(SETUP_MENU_STATE* pInputState);
static WIDGET *current, *next;
static uqstl::unique_ptr<uqm::MenuWidgets> menuWidgets {};

static constexpr const char* c_menuDefinitionResource {"ui.setupmenu"};
static uqstl::optional<uqm::MenuDefinition> menuDefinition {};
static bool mainMenuMusicChanged {};

// One screen for each menu of the menu definition, in the same order.
static uqstl::vector<WIDGET_MENU_SCREEN> menus {};
static WIDGET_BUTTON backButton {};
static WIDGET_BUTTON exitButton {};
static WIDGET_LABEL spacer {};
static WIDGET_LABEL pressToEdit {};
static uqstl::vector<const char*> pressToEditLines {};
static WIDGET_SLIDER gammaSlider {};
static WIDGET_TEXTENTRY layoutName {};
static WIDGET_CONTROLENTRY controlentries[NUM_KEYS] {};
static constexpr const char* c_controlNames[NUM_KEYS] {"up", "down", "left", "right", "weapon", "special", "escape", "thrust"};

// The control template that the Edit Controls menu shows.
static int editTemplate {};

static FRAME setup_frame = nullptr;

static void populate_gamma(void);
static int OnTextEntryEvent(WIDGET_TEXTENTRY* widget);

// Start with reasonable gamma bounds. These will get updated
// as we find out the actual bounds.
static float minGamma = 0.4f;
static float maxGamma = 2.5f;
// The gamma slider uses an exponential curve
// We use y = e^(2.1972*(x-1)) curve to give us a nice spread of
// gamma values 0.11 < g < 9.0 centered at g=1.0
#define GAMMA_CURVE_B 2.1972f
static float minGammaX;
static float maxGammaX;

static WIDGET*
find_menu(uqstl::string_view id)
{
	for (size_t index = 0; index < menuDefinition->menus.size(); ++index)
	{
		if (menuDefinition->menus[index].id == id)
		{
			return (WIDGET*)(&menus[index]);
		}
	}
	return nullptr;
}

static int
quit_main_menu(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = nullptr;
		return true;
	}
	(void)self;
	return false;
}

static int
quit_sub_menu(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		ResetOffset();
		next = find_menu("main");
		(*next->receiveFocus)(next, WIDGET_EVENT_SELECT);
		return true;
	}
	(void)self;
	return false;
}

static void
populate_editkeys(int templat)
{
	int i, j;

	uqm::strncpy_safe(layoutName.value, {uqm::input_templates[templat].name, static_cast<uint32_t>(layoutName.maxlen)});

	for (i = 0; i < NUM_KEYS; i++)
	{
		for (j = 0; j < 2; j++)
		{
			InterrogateInputState(templat, i, j,
								  controlentries[i].controlname[j],
								  WIDGET_CONTROLENTRY_WIDTH);
		}
	}
}

static void
rename_template(WIDGET_TEXTENTRY* self)
{
	uqm::strncpy_safe(uqm::input_templates[editTemplate].name, self->value);
}

// Returns the cheat value of a device or upgrade state, or nullptr if there is no such state.
static uqm::CheatAction*
find_cheat_state(uqstl::string_view key)
{
	const auto indexed = [key](uqstl::string_view prefix, size_t count) -> uqstl::optional<size_t> {
		size_t index {};
		if (!key.starts_with(prefix) || uqm::parseStr(key.substr(prefix.size()), index) != uqstl::errc {} || index >= count)
		{
			return uqstl::nullopt;
		}
		return index;
	};
	if (const auto device {indexed("device.", uqstl::size(optDeviceArray))})
	{
		return &optDeviceArray[*device];
	}
	if (const auto upgrade {indexed("upgrade.", uqstl::size(optUpgradeArray))})
	{
		return &optUpgradeArray[*upgrade];
	}
	return nullptr;
}

// Sets the window size and the display flags from the settings.
static void
apply_graphics_from_settings(void)
{
	const uqm::OptionsStruct& options {uqm::UQMOptions::read()};

	uqm::GfxFlags newFlags {g_gfxFlags & ~(uqm::GfxFlagsScaleAny | uqm::GfxFlags::Fullscreen | uqm::GfxFlags::ExclusiveFullscreen | uqm::GfxFlags::Scanlines | uqm::GfxFlags::ShowFPS)};
	newFlags |= toGfxFlags(*options.scaler);
	switch (*options.windowMode)
	{
		case uqm::WindowMode::Windowed:
			break;
		case uqm::WindowMode::WindowedFullscreen:
			newFlags |= uqm::GfxFlags::Fullscreen;
			break;
		case uqm::WindowMode::Fullscreen:
			newFlags |= uqm::GfxFlags::ExclusiveFullscreen;
			break;
	}
	if (*options.scanlines)
	{
		newFlags |= uqm::GfxFlags::Scanlines;
	}
	if (*options.showFps)
	{
		newFlags |= uqm::GfxFlags::ShowFPS;
	}

	uqm::GfxDriver newDriver {GraphicsDriver};
#if SDL_MAJOR_VERSION == 1 && defined(HAVE_OPENGL)
	newDriver = *options.alwaysOpenGL ? uqm::GfxDriver::SDL_OpenGL : uqm::GfxDriver::SDL_Pure;
#endif

	const int blowup {*options.loresBlowupScale};
	if (blowup == 0)
	{
		SavedWidth = RES_SCALE(320);
		SavedHeight = RES_SCALE(DOS_BOOL(240, 200));
	}
	else if (blowup < 6)
	{
		SavedWidth = 320 * (1 + blowup);
		SavedHeight = DOS_BOOL(240, 200) * (1 + blowup);
	}
	else
	{
		SavedWidth = inBounds(static_cast<int>(options.resolution.value.x), 320, 1920);
		SavedHeight = inBounds(static_cast<int>(options.resolution.value.y), 200, 1440);
	}

	int newWidth {SavedWidth};
	int newHeight {SavedHeight};
	if (*options.keepAspectRatio)
	{
		const float threshold {0.75f};
		const float ratio {(float)newHeight / (float)newWidth};
		if (ratio > threshold) // screen is narrower than 4:3
		{
			newWidth = newHeight / threshold;
		}
		else if (ratio < threshold) // screen is wider than 4:3
		{
			newHeight = newWidth * threshold;
		}
	}

	if (newWidth != WindowWidth || newHeight != WindowHeight || newFlags != g_gfxFlags || newDriver != GraphicsDriver)
	{
		// Exclusive fullscreen needs a second reinit after a size change.
		const bool exclusive {testFlag(newFlags, uqm::GfxFlags::ExclusiveFullscreen)};
		const bool resized {newWidth != WindowWidth || newHeight != WindowHeight};
		if (exclusive && resized)
		{
			TFB_DrawScreen_ReinitVideo(newDriver, newFlags & ~uqm::GfxFlags::ExclusiveFullscreen, newWidth, newHeight);
			FlushInput();
		}
		TFB_DrawScreen_ReinitVideo(newDriver, newFlags, newWidth, newHeight);
	}
	FlushInput();
}

#define MENU_FRAME_RATE (GameTicksPerSecond / 20)

static bool
DoSetupMenu(SETUP_MENU_STATE* pInputState)
{
	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (!pInputState->initialized)
	{
		SetDefaultMenuRepeatDelay();
		pInputState->NextTime = GetTimeCounter();
		populate_gamma();
		menuWidgets->refresh();
		Widget_SetFont(PlyrFont);
		Widget_SetWindowColors(SHADOWBOX_BACKGROUND_COLOR,
							   SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

		current = nullptr;
		next = find_menu("main");
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);

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

	if (PulsedInputState.menu[KEY_MENU_UP])
	{
		Widget_Event(WIDGET_EVENT_UP);
	}
	else if (PulsedInputState.menu[KEY_MENU_DOWN])
	{
		Widget_Event(WIDGET_EVENT_DOWN);
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		Widget_Event(WIDGET_EVENT_LEFT);
	}
	else if (PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		Widget_Event(WIDGET_EVENT_RIGHT);
	}
	if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		Widget_Event(WIDGET_EVENT_SELECT);
	}
	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		Widget_Event(WIDGET_EVENT_CANCEL);
	}
	if (PulsedInputState.menu[KEY_MENU_DELETE])
	{
		Widget_Event(WIDGET_EVENT_DELETE);
	}

	SleepThreadUntil(pInputState->NextTime + MENU_FRAME_RATE);
	pInputState->NextTime = GetTimeCounter();
	return !((GLOBAL(CurrentActivity) & CHECK_ABORT) || (next == nullptr));
}

static void
redraw_menu(void)
{
	BatchGraphics();
	(*next->draw)(next, 0, 0);
	UnbatchGraphics();
}

static bool
OnTextEntryChange(TEXTENTRY_STATE* pTES)
{
	WIDGET_TEXTENTRY* widget = (WIDGET_TEXTENTRY*)pTES->CbParam;

	widget->cursor_pos = pTES->CursorPos;
	if (pTES->JoystickMode)
	{
		widget->state |= WTE_BLOCKCUR;
	}
	else
	{
		widget->state &= ~WTE_BLOCKCUR;
	}

	// XXX TODO: Here, we can examine the text entered so far
	// to make sure it fits on the screen, for example,
	// and return false to disallow the last change

	return true; // allow change
}

static bool
OnTextEntryFrame(TEXTENTRY_STATE* pTES)
{
	redraw_menu();

	SleepThreadUntil(pTES->NextTime);
	pTES->NextTime = GetTimeCounter() + MENU_FRAME_RATE;

	return true; // continue
}

static int
OnTextEntryEvent(WIDGET_TEXTENTRY* widget)
{ // Going to edit the text
	TEXTENTRY_STATE tes;
	char revert_buf[256];

	// position cursor at the end of text
	widget->cursor_pos = utf8StringCount(widget->value);
	widget->state = WTE_EDITING;
	redraw_menu();

	// make a backup copy for revert on cancel
	utf8StringCopy(revert_buf, sizeof(revert_buf), widget->value);

	// text entry setup
	tes.Initialized = false;
	tes.NextTime = GetTimeCounter() + MENU_FRAME_RATE;
	tes.BaseStr = widget->value;
	tes.MaxSize = widget->maxlen;
	tes.CursorPos = widget->cursor_pos;
	tes.CbParam = widget;
	tes.ChangeCallback = OnTextEntryChange;
	tes.FrameCallback = OnTextEntryFrame;

	SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_SELECT);
	if (!DoTextEntry(&tes))
	{ // editing failed (canceled) -- revert the changes
		utf8StringCopy(widget->value, widget->maxlen, revert_buf);
	}
	else
	{
		if (widget->onChange)
		{
			(*(widget->onChange))(widget);
		}
	}
	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	widget->state = WTE_NORMAL;
	redraw_menu();

	return true; // event handled
}

static inline float
gammaCurve(float x)
{
	// The slider uses an exponential curve
	return exp((x - 1) * GAMMA_CURVE_B);
}

static inline float
solveGammaCurve(float y)
{
	return log(y) / GAMMA_CURVE_B + 1;
}

static int
gammaToSlider(float gamma)
{
	const float x = solveGammaCurve(gamma);
	const float step = (maxGammaX - minGammaX) / 100;
	return (int)((x - minGammaX) / step + 0.5);
}

static float
sliderToGamma(int value)
{
	const float step = (maxGammaX - minGammaX) / 100;
	const float x = minGammaX + step * value;
	const float g = gammaCurve(x);
	// report any value that is close enough as 1.0
	return (fabs(g - 1.0f) < 0.001f) ? 1.0f : g;
}

static void
updateGammaBounds(bool useUpper)
{
	float g, x;
	int slider;

	// The slider uses an exponential curve.
	// Calculate where on the curve the min and max gamma values are
	minGammaX = solveGammaCurve(minGamma);
	maxGammaX = solveGammaCurve(maxGamma);

	// We have 100 discrete steps through the range, so the slider may
	// skip over a 1.0 gamma. We need to ensure that there always is
	// a 1.0 on the slider by tweaking the range (expanding/contracting).
	slider = gammaToSlider(1.0f);
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

static int
gamma_HandleEventSlider(WIDGET* _self, int event)
{
	WIDGET_SLIDER* self = (WIDGET_SLIDER*)_self;
	int prevValue = self->value;
	float gamma;
	bool set;

	switch (event)
	{
		case WIDGET_EVENT_LEFT:
			self->value -= self->step;
			break;
		case WIDGET_EVENT_RIGHT:
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
	if (gamma < minGamma || (!set && event == WIDGET_EVENT_LEFT))
	{
		gamma = minGamma;
		updateGammaBounds(true);
		// at the lowest end
		self->value = 0;
	}
	else if (gamma > maxGamma || (!set && event == WIDGET_EVENT_RIGHT))
	{
		gamma = maxGamma;
		updateGammaBounds(false);
		// at the highest end
		self->value = 100;
	}
	uqm::UQMOptions::edit().gamma = sliderToGamma(self->value);
	return true;
}

static void
populate_gamma(void)
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

static void
gamma_DrawValue(WIDGET_SLIDER* self, int x, int y)
{
	TEXT t;
	char buf[16] {};
	float gamma = sliderToGamma(self->value);
	fmt::format_to_sz_n(buf, "{:.4}", gamma);

	t.baseline.x = x + RES_SCALE(6);
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.CharCount = ~0;
	t.pStr = buf;

	font_DrawText(&t);
}

static void
rebind_control(WIDGET_CONTROLENTRY* widget)
{
	int templat = editTemplate;
	int control = widget->controlindex;
	int index = widget->highlighted;

	FlushInput();
	DrawLabelAsWindow(&pressToEdit, nullptr);
	RebindInputState(templat, control, index);
	populate_editkeys(templat);
	FlushInput();
}

static void
clear_control(WIDGET_CONTROLENTRY* widget)
{
	int templat = editTemplate;
	int control = widget->controlindex;
	int index = widget->highlighted;

	RemoveInputState(templat, control, index);
	populate_editkeys(templat);
}

static int
count_widgets(WIDGET** widgets)
{
	int count;

	for (count = 0; *widgets != nullptr; ++widgets, ++count)
		;
	return count;
}

static bool
open_menu(uqstl::string_view id)
{
	WIDGET* menu {find_menu(id)};
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
	(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
	return true;
}

static void
read_menu_definition(void)
{
	const char* path {res_GetString(c_menuDefinitionResource)};
	const auto json {uqm::readTextFile(contentDir, path)};
	if (!json)
	{
		uqm::log::critical("PANIC: Could not read the setup menu definition '{}'.", path);
		exit(EXIT_FAILURE);
	}

	menuDefinition = uqm::menuDefinitionFromJson(*json);
	if (!menuDefinition)
	{
		uqm::log::critical("PANIC: The setup menu definition '{}' is not valid.", path);
		exit(EXIT_FAILURE);
	}
	for (const uqstl::string& error : uqm::validateMenuDefinition(*menuDefinition, uqm::UQMOptions::edit()))
	{
		uqm::log::error("{}", error);
	}
	if (menuDefinition->find("main") == nullptr)
	{
		uqm::log::critical("PANIC: The setup menu definition has no 'main' menu.");
		exit(EXIT_FAILURE);
	}
}

static void
init_button(WIDGET_BUTTON& button, const uqm::MenuButtonDef& def, int (*handler)(WIDGET*, int))
{
	button = {};
	button.tag = WIDGET_TYPE_BUTTON;
	button.handleEvent = handler;
	button.receiveFocus = Widget_ReceiveFocusSimple;
	button.draw = Widget_DrawButton;
	button.height = Widget_HeightOneLine;
	button.width = Widget_WidthFullScreen;
	button.name = def.label.c_str();
	for (size_t line = 0; line < uqstl::size(button.tooltip); ++line)
	{
		button.tooltip[line] = line < def.tooltip.size() ? def.tooltip[line].c_str() : "";
	}
}

static void
init_label(WIDGET_LABEL& label, uqstl::vector<const char*>& lines)
{
	label = {};
	label.tag = WIDGET_TYPE_LABEL;
	label.handleEvent = Widget_HandleEventIgnoreAll;
	label.receiveFocus = Widget_ReceiveFocusRefuseFocus;
	label.draw = Widget_DrawLabel;
	label.height = Widget_HeightLabel;
	label.width = Widget_WidthFullScreen;
	label.line_count = static_cast<int>(lines.size());
	label.lines = lines.data();
}

// Sets up the widgets that the menu definition does not build.
static void
init_shared_widgets(void)
{
	init_button(backButton, menuDefinition->back, quit_sub_menu);
	init_button(exitButton, menuDefinition->exit, quit_main_menu);

	static uqstl::vector<const char*> noLines {};
	init_label(spacer, noLines);

	pressToEditLines.clear();
	if (const auto found {menuDefinition->messages.find("pressToEdit")}; found != menuDefinition->messages.end())
	{
		for (const uqstl::string& line : found->second)
		{
			pressToEditLines.push_back(line.c_str());
		}
	}
	init_label(pressToEdit, pressToEditLines);

	gammaSlider = {};
	gammaSlider.tag = WIDGET_TYPE_SLIDER;
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
	layoutName.tag = WIDGET_TYPE_TEXTENTRY;
	layoutName.handleEvent = Widget_HandleEventTextEntry;
	layoutName.receiveFocus = Widget_ReceiveFocusSimple;
	layoutName.draw = Widget_DrawTextEntry;
	layoutName.height = Widget_HeightOneLine;
	layoutName.width = Widget_WidthFullScreen;
	layoutName.handleEventSelect = OnTextEntryEvent;
	layoutName.maxlen = WIDGET_TEXTENTRY_WIDTH - 1;
	layoutName.state = WTE_NORMAL;
	layoutName.onChange = rename_template;

	for (int i = 0; i < NUM_KEYS; i++)
	{
		WIDGET_CONTROLENTRY& entry {controlentries[i]};
		entry = {};
		entry.tag = WIDGET_TYPE_CONTROLENTRY;
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

static void
init_widgets(void)
{
	if (setup_frame == nullptr || optRequiresReload)
	{
		// Load the different menus depending on the resolution factor.
		setup_frame = CaptureDrawable(LoadGraphic(MENUBKG_PMAP_ANIM));
		LoadArrows();
	}

	read_menu_definition();
	init_shared_widgets();

	uqm::MenuWidgets::Hooks hooks {};
	hooks.isAddonAvailable = [](uqstl::string_view addon) {
		// The window type addons depend on the selected graphics mode.
		const bool hd {*uqm::UQMOptions::read().resolutionFactor != 0};
		if (addon == "dos-mode")
		{
			return uqm::isAddonAvailable(DOS_MODE(hd));
		}
		if (addon == "3do-mode")
		{
			return uqm::isAddonAvailable(THREEDO_MODE(hd));
		}
		if (addon == "opengl-renderer")
		{
			return SDL_MAJOR_VERSION == 1 && HAVE_OPENGL_RENDERER;
		}
		if (addon == "directional-joystick")
		{
			return HAVE_DIRECTIONAL_JOYSTICK;
		}
		return uqm::isAddonAvailable(uqstl::string {addon}.c_str());
	};
	hooks.addonUnavailable = [] {
		DoPopupWindow(GAME_STRING(MAINMENU_STRING_BASE + 36));
		Widget_SetFont(PlyrFont);
	};
	hooks.settingChanged = [](const uqm::SettingBase& setting) {
		optRequiresReload = optRequiresReload || setting.requiresReload();
		optRequiresRestart = optRequiresRestart || setting.requiresRestart();
		const uqm::OptionsStruct& options {uqm::UQMOptions::read()};
		if (setting.key() == "audio.mainMenuMusic")
		{
			mainMenuMusicChanged = true;
		}
		else if (setting.key() == "audio.musicVolume")
		{
			musicVolumeScale = *options.musicVolumeScale;
			SetMusicVolume(musicVolume);
		}
		else if (setting.key() == "audio.sfxVolume")
		{
			sfxVolumeScale = *options.sfxVolumeScale;
		}
		else if (setting.key() == "audio.speechVolume")
		{
			speechVolumeScale = *options.speechVolumeScale;
			SetSpeechVolume(speechVolumeScale);
			TestSpeechSound(SetAbsSoundIndex(testSounds, speechVolumeScale >= 1.0f));
		}
		else if (setting.key() == "seed.type" || setting.key() == "seed.value")
		{
			if (*options.seedType == uqm::SeedType::None && *options.customSeed != PrimeA)
			{
				uqm::UQMOptions::edit().customSeed = PrimeA;
				menuWidgets->refresh();
			}
			g_seedType = *options.seedType;
		}
		else if (setting.key() == "graphics.loresBlowupScale" || setting.key() == "graphics.resolution")
		{
			auto& edit {uqm::UQMOptions::edit()};
			if (setting.key() == "graphics.resolution")
			{
				edit.loresBlowupScale = 6;
			}
			apply_graphics_from_settings();
			edit.resolution = uqm::Vec2u {static_cast<uint32_t>(SavedWidth), static_cast<uint32_t>(SavedHeight)};
			menuWidgets->refresh();
		}
		else if (setting.key() == "graphics.alwaysOpenGL")
		{
			uqm::UQMOptions::edit().opengl = *options.alwaysOpenGL;
			apply_graphics_from_settings();
		}
		else if (setting.key() == "graphics.windowMode" || setting.key() == "graphics.scaler" || setting.key() == "graphics.scanlines"
				 || setting.key() == "graphics.showFps" || setting.key() == "graphics.keepAspectRatio")
		{
			apply_graphics_from_settings();
		}
		else if (setting.key() == "controls.player1Template")
		{
			PlayerControlTemplates[0] = static_cast<ControlTemplate>(*options.player1Control);
		}
		else if (setting.key() == "controls.player2Template")
		{
			PlayerControlTemplates[1] = static_cast<ControlTemplate>(*options.player2Control);
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
		if (const uqm::CheatAction* cheat {find_cheat_state(key)})
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
		else if (uqm::CheatAction * cheat {find_cheat_state(key)})
		{
			*cheat = static_cast<uqm::CheatAction>(value);
		}
	};
	hooks.openMenu = open_menu;
	hooks.textEntrySelect = OnTextEntryEvent;
	uqm::MenuWidgets::SharedWidgets shared {};
	shared.spacer = (WIDGET*)(&spacer);
	shared.back = (WIDGET*)(&backButton);
	shared.exit = (WIDGET*)(&exitButton);
	shared.legacy = {
		{"gamma",	  (WIDGET*)(&gammaSlider)},
		{"layoutName", (WIDGET*)(&layoutName) },
	};
	for (int i = 0; i < NUM_KEYS; i++)
	{
		shared.legacy[uqstl::string {"control."} + c_controlNames[i]] = (WIDGET*)(&controlentries[i]);
	}
	menuWidgets = uqstl::make_unique<uqm::MenuWidgets>(uqm::UQMOptions::edit(), shared, uqstl::move(hooks));

	menus.assign(menuDefinition->menus.size(), WIDGET_MENU_SCREEN {});
	for (size_t index = 0; index < menus.size(); ++index)
	{
		const uqm::MenuDef& menu {menuDefinition->menus[index]};
		WIDGET_MENU_SCREEN& screen {menus[index]};
		screen.tag = WIDGET_TYPE_MENU_SCREEN;
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

	testSounds = CaptureSound(LoadSound(TEST_SOUNDS));
}

static void
clean_up_widgets(void)
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
		DestroySound(ReleaseSound(testSounds));
		testSounds = 0;
	}
}

// Logs the open setup menu for the "dumpmenu" input script command.
static void
dump_setup_menu(void)
{
	if (current == nullptr || current->tag != WIDGET_TYPE_MENU_SCREEN)
	{
		uqm::log::warn("Input script: dumpmenu: no setup menu is shown.");
		return;
	}

	const auto& menu {*reinterpret_cast<const WIDGET_MENU_SCREEN*>(current)};
	uqm::log::info("Menu: {}", menu.subtitle ? menu.subtitle : "");
	for (int i = 0; i < menu.num_children; ++i)
	{
		const WIDGET* child {menu.child[i]};
		const char marker {i == menu.highlighted ? '>' : ' '};
		switch (child->tag)
		{
			case WIDGET_TYPE_CHOICE:
				{
					const auto& choice {*reinterpret_cast<const WIDGET_CHOICE*>(child)};
					const bool valid {choice.selected >= 0 && choice.selected < choice.numopts};
					uqm::log::info("{} {}: {}", marker, choice.category, valid ? choice.options[choice.selected].optname : "?");
					break;
				}
			case WIDGET_TYPE_SLIDER:
				{
					const auto& slider {*reinterpret_cast<const WIDGET_SLIDER*>(child)};
					uqm::log::info("{} {}: {}", marker, slider.category, slider.value);
					break;
				}
			case WIDGET_TYPE_BUTTON:
				uqm::log::info("{} [{}]", marker, reinterpret_cast<const WIDGET_BUTTON*>(child)->name);
				break;
			case WIDGET_TYPE_LABEL:
				{
					const auto& label {*reinterpret_cast<const WIDGET_LABEL*>(child)};
					uqm::log::info("{} -- {}", marker, label.line_count > 0 ? label.lines[0] : "");
					break;
				}
			case WIDGET_TYPE_TEXTENTRY:
				{
					const auto& entry {*reinterpret_cast<const WIDGET_TEXTENTRY*>(child)};
					uqm::log::info("{} {}: \"{}\"", marker, entry.category, entry.value);
					break;
				}
			case WIDGET_TYPE_CONTROLENTRY:
				{
					const auto& entry {*reinterpret_cast<const WIDGET_CONTROLENTRY*>(child)};
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
static void
apply_settings(void)
{
	auto& uqmOptions {uqm::UQMOptions::edit()};

	const int factor {uqm::isAddonAvailable(HD_MODE) ? *uqmOptions.resolutionFactor : 0};
	if (factor != (int)resolutionFactor)
	{
		SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));
		resolutionFactor = factor;
	}

	uqm::WindowStyle windowStyle {*uqmOptions.windowEmulationMode};
	if (windowStyle != uqm::WindowStyle::UQM && !uqm::isAddonAvailable(WINDOW_MODE(resolutionFactor, static_cast<int>(windowStyle))))
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
	SaveKeyConfiguration(configDir);

	if (optRequiresReload)
	{
		int w = WindowWidth;
		int h = WindowHeight;

		SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));

		FlushGraphics();
		UninitVideoPlayer();

		ResetOffset();

		RESOLUTION_FACTOR = resolutionFactor;
		CanvasWidth = 320 << resolutionFactor;
		CanvasHeight = DOS_BOOL(240, 200) << resolutionFactor;

		if (*uqmOptions.loresBlowupScale < 6)
		{
			h = DOS_BOOL(240, 200) * (uqmOptions.loresBlowupScale.value + 1);
			SavedHeight = h;
			uqmOptions.resolution.value.y = static_cast<uint32_t>(SavedHeight);
		}

		if (uqmOptions.keepAspectRatio)
		{
			float threshold = 0.75f;
			float ratio = (float)h / (float)w;

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
	SETUP_MENU_STATE s;

	s.InputFunc = DoSetupMenu;
	s.initialized = false;
	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	init_widgets();

	SetInputScriptMenuDump(dump_setup_menu);
	DoInput(&s, true);
	SetInputScriptMenuDump(nullptr);
	GLOBAL(CurrentActivity) &= ~CHECK_ABORT;
	apply_settings();
	clean_up_widgets();

	SetMenuSounds(MENU_SOUND_UP | MENU_SOUND_DOWN,
				  MENU_SOUND_SELECT);
}
