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
#include "libs/resource/stringbank.h"
#include "core/log/log.h"
#include "libs/memlib.h"
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
#include <scn/scan.h>

static STRING SetupTab;

typedef struct setup_menu_state
{
	bool (*InputFunc)(struct setup_menu_state* pInputState);

	bool initialized;
	int anim_frame_count;
	uint32_t NextTime;
} SETUP_MENU_STATE;

struct option_list_value
{
	const char* str;
	int value;
};

//const struct option_list_value scalerList[6] =
//	{
//		{"no",	   0							},
//		{"bilinear", TFB_GFXFLAGS_SCALE_BILINEAR	},
//		{"biadapt",	TFB_GFXFLAGS_SCALE_BIADAPT	  },
//		{"biadv",	  TFB_GFXFLAGS_SCALE_BIADAPTADV},
//		{"triscan",	TFB_GFXFLAGS_SCALE_TRISCAN	  },
//		{"hq",	   TFB_GFXFLAGS_SCALE_HQXX	  }
//};


static int SfxVol;
static int MusVol;
static int SpcVol;
static int optMScale;
static SOUND testSounds;

uqm::ScalingMode scaleTypeOptionToScalingMode(OPT_SCALETYPE opt)
{
	switch (opt)
	{
		case OPTVAL_NO_SCALE:
			return uqm::ScalingMode::None;
		case OPTVAL_BILINEAR_SCALE:
			return uqm::ScalingMode::Bilinear;
		case OPTVAL_BIADAPT_SCALE:
			return uqm::ScalingMode::BiAdapt;
		case OPTVAL_BIADV_SCALE:
			return uqm::ScalingMode::BiAdaptAdv;
		case OPTVAL_TRISCAN_SCALE:
			return uqm::ScalingMode::Triscan;
		case OPTVAL_HQXX_SCALE:
			return uqm::ScalingMode::HQ;
	}
	return uqm::ScalingMode::None;
}

int whichPlatformRef(OPT_CONSOLETYPE opt)
{
	// Returns 1 if OPTVAL_3DO and 2 of OPTVAL_PC (1 and 0 respectively)
	switch (opt)
	{
		case OPT_CONSOLETYPE::OPTVAL_PC:
			return static_cast<int>(uqm::EmulationMode::PC);
		case OPT_CONSOLETYPE::OPTVAL_3DO:
			return static_cast<int>(uqm::EmulationMode::Console3DO);
	}
	return 0;
}

OPT_CONSOLETYPE whichPlatformOpt(uqm::EmulationMode platform)
{
	switch (platform)
	{
		case uqm::EmulationMode::Console3DO:
			return OPT_CONSOLETYPE::OPTVAL_3DO;
		case uqm::EmulationMode::PC:
			return OPT_CONSOLETYPE::OPTVAL_PC;
		case uqm::EmulationMode::None:
		case uqm::EmulationMode::All:
			break;
	}
	return OPT_CONSOLETYPE::OPTVAL_PC;
}

OPT_DATETYPE dateFormatToDateTypeOpt(uqm::DateFormat fmt)
{
	switch (fmt)
	{
		case uqm::DateFormat::MMM_dd_yyyy:
			return OPTVAL_MMMDDYYYY;
		case uqm::DateFormat::MM_dd_yyyy:
			return OPTVAL_MMDDYYYY;
		case uqm::DateFormat::dd_MMM_yyyy:
			return OPTVAL_DDMMMYYYY;
		case uqm::DateFormat::dd_MM_yyyy:
			return OPTVAL_DDMMYYYY;
	}
	return OPTVAL_MMMDDYYYY;
}

template <typename GlobT, typename SetT>
bool putOpt(GlobT& glob, const SetT set, const bool reload)
{
	if (glob != static_cast<GlobT>(set))
	{
		glob = static_cast<GlobT>(set);

		if (reload)
		{
			optRequiresReload = true;
		}
		return true;
	}

	return false;
}

template <>
bool putOpt<uqm::EmulationMode, OPT_CONSOLETYPE>(uqm::EmulationMode& glob, const OPT_CONSOLETYPE set, const bool reload)
{
	const int emuMode {whichPlatformRef(set)};
	return putOpt<int, int>(reinterpret_cast<int&>(glob), emuMode, reload);
}

static bool DoSetupMenu(SETUP_MENU_STATE* pInputState);
static bool done;
static WIDGET *current, *next;

static int quit_main_menu(WIDGET* self, int event);
static int quit_sub_menu(WIDGET* self, int event);
static int do_graphics(WIDGET* self, int event);
static int do_audio(WIDGET* self, int event);
static int do_engine(WIDGET* self, int event);
static int do_cheats(WIDGET* self, int event);
static int do_keyconfig(WIDGET* self, int event);
static int do_advanced(WIDGET* self, int event);
static int do_editkeys(WIDGET* self, int event);
static int do_music(WIDGET* self, int event);
static int do_visual(WIDGET* self, int event);
static int do_qol(WIDGET* self, int event);
static int do_qol(WIDGET* self, int event);
static int do_devices(WIDGET* self, int event);
static int do_upgrades(WIDGET* self, int event);
static void change_template(WIDGET_CHOICE* self, int oldval);
static void rename_template(WIDGET_TEXTENTRY* self);
static void rebind_control(WIDGET_CONTROLENTRY* widget);
static void clear_control(WIDGET_CONTROLENTRY* widget);

/* The space for our widgets */
static WIDGET_MENU_SCREEN menus[MENU_COUNT];
static WIDGET_CHOICE choices[CHOICE_COUNT];
static WIDGET_SLIDER sliders[SLIDER_COUNT];
static WIDGET_BUTTON buttons[BUTTON_COUNT];
static WIDGET_LABEL labels[LABEL_COUNT];
static WIDGET_TEXTENTRY textentries[TEXTENTRY_COUNT];
static WIDGET_CONTROLENTRY controlentries[CONTROLENTRY_COUNT];

/* The hardcoded data that isn't strings */

typedef int (*HANDLER)(WIDGET*, int);

static HANDLER button_handlers[BUTTON_COUNT] = {
	quit_main_menu, quit_sub_menu, do_graphics, do_engine,
	do_audio, do_cheats, do_keyconfig, do_advanced, do_editkeys,
	do_keyconfig, do_music, do_visual, do_qol, do_devices, do_upgrades,
	do_cheats};

/* These refer to uninitialized widgets, but that's OK; we'll fill
 * them in before we touch them */
static WIDGET* main_widgets[] = {
	(WIDGET*)(&buttons[BTN_GFXMENU]), // Graphics
	(WIDGET*)(&buttons[BTN_ENGMENU]), // PC/3DO
	(WIDGET*)(&buttons[BTN_VISMENU]), // Visuals
	(WIDGET*)(&buttons[BTN_SNDMENU]), // Sound
	(WIDGET*)(&buttons[BTN_MUSMENU]), // Music
	(WIDGET*)(&buttons[BTN_KEYMENU]), // Controls
	(WIDGET*)(&buttons[BTN_QOLMENU]), // Quality of Life
	(WIDGET*)(&buttons[BTN_ADVMENU]), // Advanced
	(WIDGET*)(&buttons[BTN_CHTMENU]), // Cheats

	(WIDGET*)(&labels[LABEL_SPACER]),	// Spacer
	(WIDGET*)(&buttons[BTN_QUITSETUP]), // Quit Setup Menu
	nullptr};

static WIDGET* graphics_widgets[] = {
	(WIDGET*)(&choices[CHOICE_GRAPHICS]),	// Graphics
	(WIDGET*)(&choices[CHOICE_RESOLUTION]), // Resolution
	(WIDGET*)(&textentries[TEXT_CUSTMRES]), // Custom resolution entry
#if SDL_MAJOR_VERSION == 1
#if defined(HAVE_OPENGL)
	(WIDGET*)(&choices[CHOICE_FRBUFFER]), // Use Framebuffer
#endif
#endif
	(WIDGET*)(&choices[CHOICE_ASPRATIO]), // Aspect Ratio
	(WIDGET*)(&choices[CHOICE_DISPLAY]),  // Display Mode
	(WIDGET*)(&sliders[SLIDER_GAMMA]),	  // Gamma Correction
	(WIDGET*)(&choices[CHOICE_SCALER]),	  // Scaler
	(WIDGET*)(&choices[CHOICE_SCANLINE]), // Scanlines

#if SDL_MAJOR_VERSION == 2
	(WIDGET*)(&choices[CHOICE_SHOWFPS]), // Show FPS
#endif

	(WIDGET*)(&labels[LABEL_SPACER]),	  // Spacer
	(WIDGET*)(&buttons[BTN_QUITSUBMENU]), // Exit to Menu
	nullptr};

static WIDGET* engine_widgets[] = {
	(WIDGET*)(&labels[LABEL_UI]),			// UI Label
	(WIDGET*)(&choices[CHOICE_WINDOWTYPE]), // Window Type

	(WIDGET*)(&labels[LABEL_SPACER]),	   // Spacer
	(WIDGET*)(&choices[CHOICE_MENUSTYLE]), // Menu Style
	(WIDGET*)(&choices[CHOICE_DOSMENUS]),  // DOS side menu in shipyard
	(WIDGET*)(&choices[CHOICE_FONTSTYLE]), // Font Style
	(WIDGET*)(&choices[CHOICE_CUTSCENE]),  // Cutscenes

#ifdef MELEE_ZOOM
	(WIDGET*)(&choices[CHOICE_ANDRZOOM]), // Android: Melee Zoom
#else
	(WIDGET*)(&choices[CHOICE_MELEEZOOM]), // Melee Zoom
#endif

	(WIDGET*)(&choices[CHOICE_FLAGSHIP]), // Flagship Style
	(WIDGET*)(&choices[CHOICE_SCRMELT]),  // Screen Transitions

	(WIDGET*)(&labels[LABEL_SPACER]),		 // Spacer
	(WIDGET*)(&labels[LABEL_COMM]),			 // Comm Label
	(WIDGET*)(&choices[CHOICE_SCROLLSTYLE]), // Scroll Style
	(WIDGET*)(&choices[CHOICE_SPEECH]),		 // Speech
	(WIDGET*)(&choices[CHOICE_SUBTITLES]),	 // Subtitles
	(WIDGET*)(&choices[CHOICE_OSCILLSTYLE]), // Oscilloscope Style

	(WIDGET*)(&labels[LABEL_SPACER]),		  // Spacer
	(WIDGET*)(&labels[LABEL_IP]),			  // IP Label
	(WIDGET*)(&choices[CHOICE_IPSTYLE]),	  // Interplanetary Style
	(WIDGET*)(&choices[CHOICE_IPBACKGROUND]), // Star Background

	(WIDGET*)(&labels[LABEL_SPACER]),		 // Spacer
	(WIDGET*)(&labels[LABEL_SCAN]),			 // Scan Label
	(WIDGET*)(&choices[CHOICE_SCANMENU]),	 // Scan Menu Display
	(WIDGET*)(&choices[CHOICE_SLVSHIELD]),	 // Slave Shields
	(WIDGET*)(&choices[CHOICE_SCANSTYLE]),	 // Scan Style
	(WIDGET*)(&choices[CHOICE_SCANSPHERE]),	 // Scan Sphere Type
	(WIDGET*)(&choices[CHOICE_SCANTINT]),	 // Scanned Sphere Tint
	(WIDGET*)(&choices[CHOICE_LANDERSTYLE]), // Lander Style
	(WIDGET*)(&buttons[BTN_QUITSUBMENU]),	 // Exit to Menu
	nullptr};

static WIDGET* audio_widgets[] = {
	(WIDGET*)(&sliders[SLIDER_MUSVOLUME]),	// Music Volume
	(WIDGET*)(&sliders[SLIDER_SFXVOLUME]),	// SFX Volume
	(WIDGET*)(&sliders[SLIDER_SPCHVOLUME]), // Speech Volume

	(WIDGET*)(&labels[LABEL_SPACER]),		// Spacer
	(WIDGET*)(&choices[CHOICE_POSAUDIO]),	// Positional Audio
	(WIDGET*)(&choices[CHOICE_SNDDRIVER]),	// Sound Driver
	(WIDGET*)(&choices[CHOICE_SNDQUALITY]), // Sound Quality

	(WIDGET*)(&labels[LABEL_SPACER]),	  // Spacer
	(WIDGET*)(&buttons[BTN_QUITSUBMENU]), // Exit to Menu
	nullptr};

// Built from the menu definition file.
static WIDGET* music_widgets[] = {nullptr};

static WIDGET* cheat_widgets[] = {
	(WIDGET*)(&buttons[BTN_DEVMENU]),		  // Devices Menu
	(WIDGET*)(&buttons[BTN_UPGMENU]),		  // Upgrades Menu
	(WIDGET*)(&choices[CHOICE_CHEATING]),	  // JMS: cheatMode on/off
	(WIDGET*)(&choices[CHOICE_CHDECLEAN]),	  // Kohr-Ah DeCleansing mode
	(WIDGET*)(&choices[CHOICE_CHGODMODE]),	  // Precursor Mode
	(WIDGET*)(&sliders[SLIDER_TIMEDILATION]), // Time Dilation
	(WIDGET*)(&choices[CHOICE_CHWARP]),		  // Bubble Warp
	(WIDGET*)(&choices[CHOICE_CHHEADSTART]),  // Head Start
	(WIDGET*)(&choices[CHOICE_CHSHIPS]),	  // Unlock Ships
											  //	(WIDGET *)(&choices[CHOICE_CHUPGRADES  ]), // Unlock Upgrades
	(WIDGET*)(&choices[CHOICE_CHINFRU]),	  // Infinite RU
	(WIDGET*)(&choices[CHOICE_CHINFFUEL]),	  // Infinite Fuel
	(WIDGET*)(&choices[CHOICE_CHINFCRD]),	  // Infinite Credits
	(WIDGET*)(&choices[CHOICE_CHCLEANHYPER]), // No HyperSpace Encounters
	(WIDGET*)(&choices[CHOICE_CHNOPLANET]),	  // No Planets in melee
	(WIDGET*)(&buttons[BTN_QUITSUBMENU]),	  // Exit to Menu
	nullptr};

static WIDGET* keyconfig_widgets[] = {

#if SDL_MAJOR_VERSION == 2				   // Refined joypad controls not supported in SDL1
	(WIDGET*)(&choices[CHOICE_INPDEVICE]), // Control Display
#endif

	(WIDGET*)(&choices[CHOICE_BTMPLAYER]), // Bottom Player
	(WIDGET*)(&choices[CHOICE_TOPPLAYER]), // Top Player

#ifdef DIRECTIONAL_JOY
	(WIDGET*)(&choices[CHOICE_JOYSTICK]), // Directional Joystick toggle
#endif

	(WIDGET*)(&labels[LABEL_SPACER]),	   // Spacer
	(WIDGET*)(&labels[LABEL_KEYSTOOLTIP]), // "To view or edit..."
	(WIDGET*)(&buttons[BTN_EDITKEYS]),	   // Edit Controls

	(WIDGET*)(&labels[LABEL_SPACER]),	  // Spacer
	(WIDGET*)(&buttons[BTN_QUITSUBMENU]), // Exit to Menu
	nullptr};

static WIDGET* advanced_widgets[] = {
	(WIDGET*)(&choices[CHOICE_SKILLLVL]),	// Difficulty
	(WIDGET*)(&choices[CHOICE_EXTENDED]),	// Extended features
	(WIDGET*)(&choices[CHOICE_NOMAD]),		// Nomad Mode
	(WIDGET*)(&choices[CHOICE_SLAUGHTER]),	// Slaughter Mode
	(WIDGET*)(&choices[CHOICE_FLEETPOINT]), // Fleet Point System

	(WIDGET*)(&labels[LABEL_SPACER]),		// Spacer
	(WIDGET*)(&choices[CHOICE_GAMESEED]),	// Seed usage selection
	(WIDGET*)(&choices[CHOICE_SHIPSEED]),	// Ship seeding toggle
	(WIDGET*)(&textentries[TEXT_GAMESEED]), // Custom Seed entry

	(WIDGET*)(&labels[LABEL_SPACER]),	  // Spacer
	(WIDGET*)(&buttons[BTN_QUITSUBMENU]), // Exit to Menu
	nullptr};

// Built from the menu definition file.
static WIDGET* visual_widgets[] = {nullptr};

// Built from the menu definition file.
static WIDGET* qol_widgets[] = {nullptr};

static WIDGET* editkeys_widgets[] = {
	(WIDGET*)(&choices[CHOICE_KBLAYOUT]),	  // Current layout
	(WIDGET*)(&textentries[TEXT_LOUTNAME]),	  // Layout name
	(WIDGET*)(&labels[LABEL_TAPTOOLTIP]),	  // "Press return to..."
	(WIDGET*)(&controlentries[CONTROL_UP]),	  // Up
	(WIDGET*)(&controlentries[CONTROL_DOWN]), // Down
	(WIDGET*)(&controlentries[CONTROL_LEFT]), // Left
	(WIDGET*)(&controlentries[CONTROL_RGHT]), // Right
	(WIDGET*)(&controlentries[CONTROL_WEAP]), // Weapon
	(WIDGET*)(&controlentries[CONTROL_SPEC]), // Special
	(WIDGET*)(&controlentries[CONTROL_ESC]),  // Escape
	(WIDGET*)(&controlentries[CONTROL_THRU]), // Thrust
	(WIDGET*)(&buttons[BTN_PREVMENU]),		  // Previous menu
	nullptr};

static WIDGET* devices_widgets[] = {
	(WIDGET*)(&choices[CHOICE_DEVSPAWNER]), // Portal Spawner
	(WIDGET*)(&choices[CHOICE_DEVPET]),		// Talking Pet
	(WIDGET*)(&choices[CHOICE_DEVBOMB]),	// Utwig Bomb
	(WIDGET*)(&choices[CHOICE_DEVSUN]),		// Sun Device
	(WIDGET*)(&choices[CHOICE_DEVSPHERE]),	// Rosy Sphere
	(WIDGET*)(&choices[CHOICE_DEVHELIX]),	// Aqua Helix
	(WIDGET*)(&choices[CHOICE_DEVSPINDLE]), // Clear Spindle
	(WIDGET*)(&choices[CHOICE_DEVULTRON0]), // Ultron (Broken)
	(WIDGET*)(&choices[CHOICE_DEVULTRON1]), // Ultron (Semi-Broken)
	(WIDGET*)(&choices[CHOICE_DEVULTRON2]), // Ultron (Semi-Fixed)
	(WIDGET*)(&choices[CHOICE_DEVULTRON3]), // Ultron (Fixed)
	(WIDGET*)(&choices[CHOICE_DEVMAIDENS]), // Shofixti Maidens
	(WIDGET*)(&choices[CHOICE_DEVCASTER0]), // Umgah Caster
	(WIDGET*)(&choices[CHOICE_DEVCASTER1]), // Burvixese Caster
	(WIDGET*)(&choices[CHOICE_DEVSHIELD]),	// Taalo Shield
	(WIDGET*)(&choices[CHOICE_DEVEGGCS0]),	// Egg Case 01
	(WIDGET*)(&choices[CHOICE_DEVEGGCS1]),	// Egg Case 02
	(WIDGET*)(&choices[CHOICE_DEVEGGCS2]),	// Egg Case 03
	(WIDGET*)(&choices[CHOICE_DEVSHUTTLE]), // Syreen Shuttle
	(WIDGET*)(&choices[CHOICE_DEVBEAST]),	// VUX Beast
	(WIDGET*)(&choices[CHOICE_DEVSLYCODE]), // Slylandro Destruct
	(WIDGET*)(&choices[CHOICE_DEVWARPPOD]), // Ur-Quan Warp Pod
	(WIDGET*)(&choices[CHOICE_DEVTRIDENT]), // Wimbli's Trident
	(WIDGET*)(&choices[CHOICE_DEVROD]),		// Glowing Rod
	(WIDGET*)(&choices[CHOICE_DEVLUNBASE]), // Lunar Base

	(WIDGET*)(&labels[LABEL_SPACER]), // Spacer
	(WIDGET*)(&buttons[BTN_CHTPREV]), // Back to Cheats
	nullptr};

static WIDGET* upgrades_widgets[] = {
	(WIDGET*)(&choices[CHOICE_UPGLANDERSPD]), // Lander Speed
	(WIDGET*)(&choices[CHOICE_UPGLANDERCRG]), // Lander Cargo
	(WIDGET*)(&choices[CHOICE_UPGLANDERFRE]), // Lander Rapid Fire
	(WIDGET*)(&choices[CHOICE_UPGLANDERBSH]), // Lander Bio Shield
	(WIDGET*)(&choices[CHOICE_UPGLANDERQSH]), // Lander Quake Shield
	(WIDGET*)(&choices[CHOICE_UPGLANDERLSH]), // Lander Lightning Shield
	(WIDGET*)(&choices[CHOICE_UPGLANDERHSH]), // Lander Heat Shield
	(WIDGET*)(&choices[CHOICE_MODPOINTDEF]),  // Point Defense Module
	(WIDGET*)(&choices[CHOICE_MODFUSBLAST]),  // Fusion Blaster Module
	(WIDGET*)(&choices[CHOICE_MODFUELTANK]),  // Hi-Eff Fuel Module
	(WIDGET*)(&choices[CHOICE_MODTRACKING]),  // Tracking Module
	(WIDGET*)(&choices[CHOICE_MODHELLBORE]),  // Hellbore Cannon Module
	(WIDGET*)(&choices[CHOICE_MODFURNACE]),	  // Shiva Furnace Module

	(WIDGET*)(&labels[LABEL_SPACER]), // Spacer
	(WIDGET*)(&buttons[BTN_CHTPREV]), // Back to Cheats
	nullptr};

static const struct
{
	WIDGET** widgets;
	int bgIndex;
} menu_defs[] =
	{
		{main_widgets,	   0 },
		{graphics_widgets,  1 },
		{audio_widgets,		2 },
		{engine_widgets,	 3 },
		{cheat_widgets,		4 },
		{keyconfig_widgets, 5 },
		{advanced_widgets,  6 },
		{editkeys_widgets,  7 },
		{music_widgets,		8 },
		{visual_widgets,	 9 },
		{qol_widgets,		  10},
		{devices_widgets,	  11},
		{upgrades_widgets,  12},
		{nullptr,			  0 }
};

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

//No longer used
//static int
//number_res_options (void)
//{
//	if (TFB_SupportsHardwareScaling ())
//	{
//		return 4;
//	}
//	else
//	{
//		return 2;
//	}
//}

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
		next = (WIDGET*)(&menus[MENU_QUITSUB]);
		(*next->receiveFocus)(next, WIDGET_EVENT_SELECT);
		return true;
	}
	(void)self;
	return false;
}

static void
populate_res(void)
{
	fmt::format_to_n(textentries[TEXT_CUSTMRES].value, WIDGET_TEXTENTRY_WIDTH - 1, "{}x{}",
					 SavedWidth, SavedHeight);
}

static int
do_graphics(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_GRAPHICS]);
		populate_res();
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_audio(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_AUDIO]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_engine(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_ENGINE]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_cheats(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_CHEATS]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_keyconfig(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENY_KEYCONF]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static void
populate_seed(void)
{
	if (choices[CHOICE_GAMESEED].selected == OPTVAL_PRIME || !SANE_SEED(uqm::UQMOptions::read().customSeed.value))
	{
		uqm::UQMOptions::edit().customSeed = PrimeA;
	}
	fmt::format_to_sz_n(textentries[TEXT_GAMESEED].value,
						sizeof(textentries[TEXT_GAMESEED].value), "{}", uqm::UQMOptions::read().customSeed.value);
}

static int
do_advanced(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_ADVANCED]);
		populate_seed();
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_music(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_MUSIC]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_visual(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_VISUAL]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_qol(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_QOL]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_devices(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_DEVICES]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static int
do_upgrades(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_UPGRADES]);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static void
populate_editkeys(int templat)
{
	int i, j;

	uqm::strncpy_safe(textentries[TEXT_LOUTNAME].value, {uqm::input_templates[templat].name, static_cast<uint32_t>(textentries[TEXT_LOUTNAME].maxlen)});

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

static int
do_editkeys(WIDGET* self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET*)(&menus[MENU_EDITKEYS]);
		/* Prepare the components */
		choices[CHOICE_KBLAYOUT].selected = 0;

		populate_editkeys(0);
		(*next->receiveFocus)(next, WIDGET_EVENT_DOWN);
		return true;
	}
	(void)self;
	return false;
}

static void
change_template(WIDGET_CHOICE* self, int oldval)
{
	(void)oldval;
	populate_editkeys(self->selected);
}

static void
addon_unavailable(WIDGET_CHOICE* self, int oldval)
{
	self->selected = oldval;
	DoPopupWindow(GAME_STRING(MAINMENU_STRING_BASE + 36));
	Widget_SetFont(PlyrFont);
}

static void
check_for_hd(WIDGET_CHOICE* self, int oldval)
{
	if (self->selected != OPTVAL_REAL_1280_960)
	{
		return;
	}

	if (!uqm::isAddonAvailable(HD_MODE))
	{
		oldval = OPTVAL_320_240;
		addon_unavailable(self, OPTVAL_320_240);
	}

	(void)oldval; // Satisfy compiler
}

static bool
check_dos_3do_modes(WIDGET_CHOICE* self, int oldval)
{
	bool selected = choices[CHOICE_GRAPHICS].selected;
	int shmagoigle = (selected && !IS_HD) ? HD : IS_HD;

	switch (self->selected)
	{
		case OPTVAL_PC_WINDOW:
			if (!uqm::isAddonAvailable(DOS_MODE(shmagoigle)))
			{
				oldval = OPTVAL_UQM_WINDOW;
				addon_unavailable(self, oldval);
				return false;
			}
			break;
		case OPTVAL_3DO_WINDOW:
			if (!uqm::isAddonAvailable(THREEDO_MODE(shmagoigle)))
			{
				oldval = OPTVAL_UQM_WINDOW;
				addon_unavailable(self, OPTVAL_UQM_WINDOW);
				return false;
			}
			break;
		default:
			break;
	}

	return true;
}

static void
check_availability(WIDGET_CHOICE* self, int oldval)
{
	if (self->choice_num == CHOICE_GRAPHICS)
	{
		check_for_hd(self, oldval);
	}

	if (self->choice_num == CHOICE_WINDOWTYPE)
	{
		check_dos_3do_modes(self, oldval);
	}
}

static void
rename_template(WIDGET_TEXTENTRY* self)
{
	/* TODO: This will have to change if the size of the
	   input_templates name is changed.  It would probably be nice
	   to track this symbolically or ensure that self->value's
	   buffer is always at least this big; this will require some
	   reworking of widgets */
	uqm::strncpy_safe(uqm::input_templates[choices[CHOICE_KBLAYOUT].selected].name, self->value);
}

static void
change_seedtype(WIDGET_CHOICE* self, int oldval)
{
	if (self->selected == OPTVAL_PRIME)
	{
		uqm::UQMOptions::edit().customSeed = PrimeA;
		fmt::format_to_sz_n(textentries[TEXT_GAMESEED].value,
							sizeof(textentries[TEXT_GAMESEED].value),
							"{}", uqm::UQMOptions::read().customSeed.value);
	}

	(void)oldval; // Satisfy compiler
}

static void
change_seed(WIDGET_TEXTENTRY* self)
{
	int customSeed = atoi(self->value);
	if (choices[CHOICE_GAMESEED].selected == OPTVAL_PRIME || !SANE_SEED(uqm::UQMOptions::read().customSeed.value))
	{
		customSeed = PrimeA;
		fmt::format_to_sz_n(self->value, "{}", customSeed);
	}
	uqm::UQMOptions::edit().customSeed = customSeed;
}

static void
adjustMusic(WIDGET_SLIDER* self)
{
	musicVolumeScale = self->value / 100.0f;
	SetMusicVolume(musicVolume);
}

static void
adjustSFX(WIDGET_SLIDER* self)
{
	sfxVolumeScale = self->value / 100.0f;
}

static void
adjustSpeech(WIDGET_SLIDER* self)
{
	speechVolumeScale = self->value / 100.0f;
	SetSpeechVolume(speechVolumeScale);

	TestSpeechSound(SetAbsSoundIndex(testSounds, (self->value == 100)));
}

static void
toggle_scanlines(WIDGET_CHOICE* self, uqm::GfxFlags* NewGfxFlags)
{
	if (self->selected == 1)
	{
		*NewGfxFlags |= uqm::GfxFlags::Scanlines;
	}
	else
	{
		*NewGfxFlags &= ~uqm::GfxFlags::Scanlines;
	}
	uqm::UQMOptions::edit().scanlines = (self->selected == 1);
}

static void
change_scaling(WIDGET_CHOICE* self, int* NewWidth, int* NewHeight)
{
	if (self->selected < 6)
	{
		if (!self->selected)
		{ // No blowup
			*NewWidth = RES_SCALE(320);
			*NewHeight = RES_SCALE(DOS_BOOL(240, 200));
		}
		else
		{
			*NewWidth = 320 * (1 + self->selected);
			*NewHeight = DOS_BOOL(240, 200) * (1 + self->selected);
		}
	}

	SavedWidth = inBounds(*NewWidth, 320, 1920);
	SavedHeight = inBounds(*NewHeight, 200, 1400);

	putOpt(uqm::UQMOptions::edit().loresBlowupScale.value, (int)(self->selected), false);
	uqm::UQMOptions::edit().resolution = uqm::Vec2u {static_cast<uint32_t>(*NewWidth), static_cast<uint32_t>(*NewHeight)};
}

static void
toggle_fullscreen(WIDGET_CHOICE* self, uqm::GfxFlags* NewGfxFlags)
{
	if (self->selected == 1)
	{
		*NewGfxFlags &= ~uqm::GfxFlags::Fullscreen;
		*NewGfxFlags |= uqm::GfxFlags::ExclusiveFullscreen;
	}
	else if (self->selected == 2)
	{
		*NewGfxFlags &= ~uqm::GfxFlags::ExclusiveFullscreen;
		*NewGfxFlags |= uqm::GfxFlags::Fullscreen;
	}
	else
	{
		*NewGfxFlags &= ~(uqm::GfxFlags::Fullscreen | uqm::GfxFlags::ExclusiveFullscreen);
	}
	uqm::UQMOptions::edit().windowMode = static_cast<uqm::WindowMode>(self->selected);
}

static void
change_scaler(WIDGET_CHOICE* self, int OldVal, uqm::GfxFlags* NewGfxFlags)
{
	const uqm::ScalingMode oldMode {scaleTypeOptionToScalingMode((OPT_SCALETYPE)OldVal)};
	const uqm::ScalingMode selectedMode {scaleTypeOptionToScalingMode((OPT_SCALETYPE)self->selected)};
	*NewGfxFlags &= ~toGfxFlags(oldMode);
	*NewGfxFlags |= toGfxFlags(selectedMode);
	uqm::UQMOptions::edit().scaler = selectedMode;
}

static void
toggle_showfps(WIDGET_CHOICE* self, uqm::GfxFlags* NewGfxFlags)
{
	if (self->selected == 1)
	{
		*NewGfxFlags |= uqm::GfxFlags::ShowFPS;
	}
	else
	{
		*NewGfxFlags &= ~uqm::GfxFlags::ShowFPS;
	}
	uqm::UQMOptions::edit().showFps = (self->selected == 1);
}

static void
change_gfxdriver(WIDGET_CHOICE* self, uqm::GfxDriver* NewGfxDriver)
{
#ifdef HAVE_OPENGL
	*NewGfxDriver = (self->selected == OPTVAL_ALWAYS_GL ?
						 uqm::GfxDriver::SDL_OpenGL :
						 uqm::GfxDriver::SDL_Pure);
#else
	*NewGfxDriver = uqm::GfxDriver::SDL_Pure;
#endif
	if (GraphicsDriver != *NewGfxDriver)
	{
		uqm::UQMOptions::edit().alwaysOpenGL = (self->selected == OPTVAL_ALWAYS_GL);
		uqm::UQMOptions::edit().opengl = (*NewGfxDriver == uqm::GfxDriver::SDL_OpenGL);
	}
}

void process_graphics_options(WIDGET_CHOICE* self, int OldVal)
{
	uqm::GfxFlags NewGfxFlags = g_gfxFlags;
	uqm::GfxDriver NewGfxDriver = GraphicsDriver;
	int NewWidth = SavedWidth;
	int NewHeight = SavedHeight;
	bool isExclusive = false;

	if (OldVal == self->selected)
	{
		return;
	}

	switch (self->choice_num)
	{
		case CHOICE_FRBUFFER:
			change_gfxdriver(self, &NewGfxDriver);
			break;
		case CHOICE_SCALER:
			change_scaler(self, OldVal, &NewGfxFlags);
			break;
		case CHOICE_SCANLINE:
			toggle_scanlines(self, &NewGfxFlags);
			break;
		case CHOICE_DISPLAY:
			toggle_fullscreen(self, &NewGfxFlags);
			break;
		case CHOICE_SHOWFPS:
			toggle_showfps(self, &NewGfxFlags);
			break;
		case CHOICE_ASPRATIO:
			uqm::UQMOptions::edit().keepAspectRatio = (bool)self->selected;
			break;
		case CHOICE_RESOLUTION:
			change_scaling(self, &NewWidth, &NewHeight);
			isExclusive = testFlag(NewGfxFlags, uqm::GfxFlags::ExclusiveFullscreen);
			break;
		default:
			return;
	}

	if (uqm::UQMOptions::read().keepAspectRatio)
	{
		float threshold = 0.75f;
		float ratio = (float)NewHeight / (float)NewWidth;

		if (ratio > threshold) // screen is narrower than 4:3
		{
			NewWidth = NewHeight / threshold;
		}
		else if (ratio < threshold) // screen is wider than 4:3
		{
			NewHeight = NewWidth * threshold;
		}
	}

	if (NewWidth != WindowWidth || NewHeight != WindowHeight || NewGfxFlags != g_gfxFlags || NewGfxDriver != GraphicsDriver)
	{
		if (isExclusive)
		{
			NewGfxFlags &= ~uqm::GfxFlags::ExclusiveFullscreen;
		}

		TFB_DrawScreen_ReinitVideo(NewGfxDriver, NewGfxFlags,
								   NewWidth, NewHeight);
	}

	FlushInput();

	if (isExclusive)
	{ // needed twice to reinitialize Exclusive Full Screen after a
		// resolution change
		g_gfxFlags |= uqm::GfxFlags::ExclusiveFullscreen;
		TFB_DrawScreen_ReinitVideo(GraphicsDriver, g_gfxFlags,
								   WindowWidth, WindowHeight);
	}

	populate_res();
}

//static bool
//res_check (int width, int height)
//{
//	if (width % 320)
//		return false;
//
//	if (height % DOS_BOOL (240, 200))
//		return false;
//
//	if (width > 1920 || height > 1440)
//		return false;
//
//	return true;
//}

static void
change_res(WIDGET_TEXTENTRY* self)
{
	int NewWidth = SavedWidth;
	int NewHeight = SavedHeight;
	uqm::GfxFlags NewGfxFlags = g_gfxFlags;
	bool isExclusive = testFlag(NewGfxFlags, uqm::GfxFlags::ExclusiveFullscreen);

	if (const auto result {scn::scan<int, int>(self->value, "{}x{}")})
	{
		const auto [w, h] = result->values();
		NewWidth = w;
		NewHeight = h;
	}
	else
	{
		populate_res();
		return;
	}

	NewWidth = inBounds(NewWidth, 320, 1920);
	NewHeight = inBounds(NewHeight, 200, 1440);

	SavedWidth = NewWidth;
	SavedHeight = NewHeight;

	if (uqm::UQMOptions::read().keepAspectRatio)
	{
		float threshold = 0.75f;
		float ratio = (float)NewHeight / (float)NewWidth;

		if (ratio > threshold) // screen is narrower than 4:3
		{
			NewWidth = NewHeight / threshold;
		}
		else if (ratio < threshold) // screen is wider than 4:3
		{
			NewHeight = NewWidth * threshold;
		}
	}

	if (NewWidth != WindowWidth || NewHeight != WindowHeight)
	{
		if (isExclusive)
		{
			NewGfxFlags &= ~uqm::GfxFlags::ExclusiveFullscreen;
		}

		TFB_DrawScreen_ReinitVideo(GraphicsDriver, g_gfxFlags,
								   NewWidth, NewHeight);
	}
	else
	{
		return;
	}

	if (NewGfxFlags != g_gfxFlags)
	{
		g_gfxFlags = NewGfxFlags;
	}

	FlushInput();

	if (isExclusive)
	{ // needed twice to reinitialize Exclusive Full Screen after a
		// resolution change
		g_gfxFlags |= uqm::GfxFlags::ExclusiveFullscreen;
		TFB_DrawScreen_ReinitVideo(GraphicsDriver, g_gfxFlags,
								   WindowWidth, WindowHeight);
	}

	/*if (res_check(NewWidth, NewHeight))
	{
		choices[CHOICE_RESOLUTION].selected = (NewWidth / 320) - 1;
	}
	else
		choices[CHOICE_RESOLUTION].selected = 6;*/

	populate_res();

	putOpt(uqm::UQMOptions::edit().loresBlowupScale.edit(), (int)(choices[CHOICE_RESOLUTION].selected), false);
	uqm::UQMOptions::edit().resolution = uqm::Vec2u {static_cast<uint32_t>(SavedWidth), static_cast<uint32_t>(SavedHeight)};
}

#define NUM_STEPS 20
#define X_STEP (SCREEN_WIDTH / NUM_STEPS)
#define Y_STEP (SCREEN_HEIGHT / NUM_STEPS)
#define MENU_FRAME_RATE (GameTicksPerSecond / 20)

#define DEVICE_START 87
#define UPGRADE_START 112

static void
SetDefaults(void)
{
	GLOBALOPTS opts;
	uint8_t i;

	GetGlobalOptions(&opts);
	choices[CHOICE_GRAPHICS].selected = opts.screenResolution;
	choices[CHOICE_FRBUFFER].selected = opts.driver;
	choices[CHOICE_SCALER].selected = opts.scaler;
	choices[CHOICE_SCANLINE].selected = opts.scanlines;
	choices[CHOICE_MENUSTYLE].selected = static_cast<int>(opts.menu);
	choices[CHOICE_FONTSTYLE].selected = static_cast<int>(opts.text);
	choices[CHOICE_SCANMENU].selected = static_cast<int>(opts.cscan);
	choices[CHOICE_SCROLLSTYLE].selected = static_cast<int>(opts.scroll);
	choices[CHOICE_SUBTITLES].selected = opts.subtitles;
	choices[CHOICE_DISPLAY].selected = opts.fullscreen;
	choices[CHOICE_CUTSCENE].selected = static_cast<int>(opts.intro);
	choices[CHOICE_SHOWFPS].selected = opts.fps;

#ifndef MELEE_ZOOM
	choices[CHOICE_MELEEZOOM].selected = opts.meleezoom;
#endif

	choices[CHOICE_POSAUDIO].selected = opts.stereo;
	choices[CHOICE_SNDDRIVER].selected = opts.adriver;
	choices[CHOICE_SNDQUALITY].selected = opts.aquality;
	choices[CHOICE_SLVSHIELD].selected = static_cast<int>(opts.shield);
	choices[CHOICE_BTMPLAYER].selected = static_cast<int>(opts.player1);
	choices[CHOICE_TOPPLAYER].selected = static_cast<int>(opts.player2);
	choices[CHOICE_KBLAYOUT].selected = 0;
	choices[CHOICE_SPEECH].selected = opts.speech;
	choices[CHOICE_ASPRATIO].selected = opts.keepaspect;
	choices[CHOICE_CHEATING].selected = opts.cheatMode;
	choices[CHOICE_CHGODMODE].selected = opts.godModes;
	choices[CHOICE_CHWARP].selected = opts.bubbleWarp;
	choices[CHOICE_CHSHIPS].selected = opts.unlockShips;
	choices[CHOICE_CHHEADSTART].selected = opts.headStart;
	//	choices[CHOICE_CHUPGRADES   ].selected = opts.unlockUpgrades;
	choices[CHOICE_CHINFRU].selected = opts.infiniteRU;
	choices[CHOICE_CHINFFUEL].selected = opts.infiniteFuel;
	choices[CHOICE_RESOLUTION].selected = opts.loresBlowup;
	choices[CHOICE_CHINFCRD].selected = opts.infiniteCredits;
#ifdef DIRECTIONAL_JOY
	choices[CHOICE_JOYSTICK].selected = opts.directionalJoystick;
#endif
#ifdef MELEE_ZOOM
	choices[CHOICE_ANDRZOOM].selected = opts.meleezoom;
#endif
	choices[CHOICE_LANDERHOLD].selected = static_cast<int>(opts.landerHold);
	choices[CHOICE_SCRMELT].selected = static_cast<int>(opts.scrTrans);
	choices[CHOICE_SKILLLVL].selected = static_cast<int>(opts.difficulty);
	choices[CHOICE_EXTENDED].selected = opts.extended;
	choices[CHOICE_NOMAD].selected = static_cast<int>(opts.nomad);
	choices[CHOICE_INPDEVICE].selected = static_cast<int>(opts.controllerType);
	choices[CHOICE_SCANTINT].selected = static_cast<int>(opts.tintPlanSphere);
	choices[CHOICE_IPSTYLE].selected = static_cast<int>(opts.planetStyle);
	choices[CHOICE_IPBACKGROUND].selected = opts.starBackground;
	choices[CHOICE_SCANSTYLE].selected = static_cast<int>(opts.scanStyle);
	choices[CHOICE_OSCILLSTYLE].selected = static_cast<int>(opts.scopeStyle);
	choices[CHOICE_LANDERSTYLE].selected = static_cast<int>(opts.landerStyle);
	choices[CHOICE_FLAGSHIP].selected = static_cast<int>(opts.flagshipColor);
	choices[CHOICE_CHCLEANHYPER].selected = opts.noHQEncounters;
	choices[CHOICE_CHDECLEAN].selected = opts.deCleansing;
	choices[CHOICE_CHNOPLANET].selected = opts.meleeObstacles;
	choices[CHOICE_SCANSPHERE].selected = opts.sphereType;
	choices[CHOICE_SLAUGHTER].selected = opts.slaughterMode;
	choices[CHOICE_WINDOWTYPE].selected = opts.windowType;
	choices[CHOICE_GAMESEED].selected = opts.seedType;
	choices[CHOICE_FLEETPOINT].selected = opts.fleetPointSys;

	// Devices
	for (i = DEVICE_START; i < DEVICE_START
								   + std::size(opts.deviceArray);
		 i++)
	{
		choices[i].selected = opts.deviceArray[i - DEVICE_START];
	}

	for (i = UPGRADE_START; i < UPGRADE_START + NUM_UPGRADES; i++)
	{
		choices[i].selected = opts.upgradeArray[i - UPGRADE_START];
	}

	// Next choice should be choices[CHOICE_SHIPSEED]
	choices[CHOICE_SHIPSEED].selected = opts.shipSeed;
	choices[CHOICE_DOSMENUS].selected = opts.dosMenus;

	sliders[SLIDER_MUSVOLUME].value = opts.musicvol;
	sliders[SLIDER_SFXVOLUME].value = opts.sfxvol;
	sliders[SLIDER_SPCHVOLUME].value = opts.speechvol;
	sliders[SLIDER_GAMMA].value = opts.gamma;
	sliders[SLIDER_TIMEDILATION].value = opts.timeDilationPct;
}

static void
PropagateResults(void)
{
	GLOBALOPTS opts;
	uint8_t i;

	opts.screenResolution = (OPT_RESTYPE)choices[CHOICE_GRAPHICS].selected;
	opts.driver = (OPT_DRIVERTYPE)choices[CHOICE_FRBUFFER].selected;
	opts.scaler = (OPT_SCALETYPE)choices[CHOICE_SCALER].selected;
	opts.scanlines = (OPT_ENABLABLE)choices[CHOICE_SCANLINE].selected;
	opts.menu = (OPT_CONSOLETYPE)choices[CHOICE_MENUSTYLE].selected;
	opts.text = (OPT_CONSOLETYPE)choices[CHOICE_FONTSTYLE].selected;
	opts.cscan = (OPT_CONSOLETYPE)choices[CHOICE_SCANMENU].selected;
	opts.scroll = (OPT_CONSOLETYPE)choices[CHOICE_SCROLLSTYLE].selected;
	opts.subtitles = (OPT_ENABLABLE)choices[CHOICE_SUBTITLES].selected;
	opts.fullscreen = (OPT_FULLSCREEN)choices[CHOICE_DISPLAY].selected;
	opts.intro = (OPT_CONSOLETYPE)choices[CHOICE_CUTSCENE].selected;
	opts.fps = (OPT_ENABLABLE)choices[CHOICE_SHOWFPS].selected;

#ifndef MELEE_ZOOM
	opts.meleezoom = (OPT_MELEEZOOM)choices[CHOICE_MELEEZOOM].selected;
#endif

	opts.stereo = (OPT_ENABLABLE)choices[CHOICE_POSAUDIO].selected;
	opts.adriver = (OPT_ADRIVERTYPE)choices[CHOICE_SNDDRIVER].selected;
	opts.aquality = (OPT_AQUALITYTYPE)choices[CHOICE_SNDQUALITY].selected;
	opts.shield = (OPT_CONSOLETYPE)choices[CHOICE_SLVSHIELD].selected;
	opts.player1 = (ControlTemplate)choices[CHOICE_BTMPLAYER].selected;
	opts.player2 = (ControlTemplate)choices[CHOICE_TOPPLAYER].selected;
	opts.speech = (OPT_ENABLABLE)choices[CHOICE_SPEECH].selected;
	opts.keepaspect = (OPT_ENABLABLE)choices[CHOICE_ASPRATIO].selected;
	opts.cheatMode = (OPT_ENABLABLE)choices[CHOICE_CHEATING].selected;
	opts.godModes = (OPT_GODTYPE)choices[CHOICE_CHGODMODE].selected;
	opts.bubbleWarp = (OPT_ENABLABLE)choices[CHOICE_CHWARP].selected;
	opts.unlockShips = (OPT_ENABLABLE)choices[CHOICE_CHSHIPS].selected;
	opts.headStart = (OPT_ENABLABLE)choices[CHOICE_CHHEADSTART].selected;
	//	opts.unlockUpgrades =   choices[CHOICE_CHUPGRADES   ].selected;
	opts.infiniteRU = (OPT_ENABLABLE)choices[CHOICE_CHINFRU].selected;
	opts.infiniteFuel = (OPT_ENABLABLE)choices[CHOICE_CHINFFUEL].selected;
	opts.loresBlowup = (OPT_RESSCALER)choices[CHOICE_RESOLUTION].selected;
	opts.infiniteCredits = (OPT_ENABLABLE)choices[CHOICE_CHINFCRD].selected;
#ifdef DIRECTIONAL_JOY
	opts.directionalJoystick = choices[CHOICE_JOYSTICK].selected;
#endif
#ifdef MELEE_ZOOM
	opts.meleezoom = choices[CHOICE_ANDRZOOM].selected;
#endif
	opts.landerHold = (OPT_CONSOLETYPE)choices[CHOICE_LANDERHOLD].selected;
	opts.scrTrans = (OPT_CONSOLETYPE)choices[CHOICE_SCRMELT].selected;
	opts.difficulty = static_cast<uqm::Difficulty>(choices[CHOICE_SKILLLVL].selected);
	opts.extended = (OPT_ENABLABLE)choices[CHOICE_EXTENDED].selected;
	opts.nomad = static_cast<uqm::NomadMode>(choices[CHOICE_NOMAD].selected);
	opts.controllerType = static_cast<uqm::ControllerType>(choices[CHOICE_INPDEVICE].selected);
	opts.tintPlanSphere = (OPT_CONSOLETYPE)choices[CHOICE_SCANTINT].selected;
	opts.planetStyle = (OPT_CONSOLETYPE)choices[CHOICE_IPSTYLE].selected;
	opts.starBackground = choices[CHOICE_IPBACKGROUND].selected;
	opts.scanStyle = (OPT_CONSOLETYPE)choices[CHOICE_SCANSTYLE].selected;
	opts.scopeStyle = (OPT_CONSOLETYPE)choices[CHOICE_OSCILLSTYLE].selected;
	opts.landerStyle = (OPT_CONSOLETYPE)choices[CHOICE_LANDERSTYLE].selected;
	opts.flagshipColor = (OPT_CONSOLETYPE)choices[CHOICE_FLAGSHIP].selected;
	opts.noHQEncounters = (OPT_ENABLABLE)choices[CHOICE_CHCLEANHYPER].selected;
	opts.deCleansing = (OPT_ENABLABLE)choices[CHOICE_CHDECLEAN].selected;
	opts.meleeObstacles = (OPT_ENABLABLE)choices[CHOICE_CHNOPLANET].selected;
	opts.sphereType = (OPT_SPHERETYPE)choices[CHOICE_SCANSPHERE].selected;
	opts.slaughterMode = (OPT_ENABLABLE)choices[CHOICE_SLAUGHTER].selected;
	opts.windowType = (OPT_WINDOWTYPE)choices[CHOICE_WINDOWTYPE].selected;
	opts.seedType = (OPT_SEED)choices[CHOICE_GAMESEED].selected;
	opts.fleetPointSys = (OPT_ENABLABLE)choices[CHOICE_FLEETPOINT].selected;

	// Devices
	for (i = DEVICE_START;
		 i < DEVICE_START + std::size(opts.deviceArray); i++)
	{
		opts.deviceArray[i - DEVICE_START] = (OPT_ADD_REMOVE)choices[i].selected;
	}

	for (i = UPGRADE_START; i < UPGRADE_START + NUM_UPGRADES; i++)
	{
		opts.upgradeArray[i - UPGRADE_START] = (OPT_ADD_REMOVE)choices[i].selected;
	}

	opts.shipSeed = (OPT_ENABLABLE)choices[CHOICE_SHIPSEED].selected;
	opts.dosMenus = (OPT_ENABLABLE)choices[CHOICE_DOSMENUS].selected;

	opts.musicvol = sliders[SLIDER_MUSVOLUME].value;
	opts.sfxvol = sliders[SLIDER_SFXVOLUME].value;
	opts.speechvol = sliders[SLIDER_SPCHVOLUME].value;
	opts.gamma = sliders[SLIDER_GAMMA].value;
	opts.timeDilationPct = sliders[SLIDER_TIMEDILATION].value;

	SetGlobalOptions(&opts);
}

static bool
DoSetupMenu(SETUP_MENU_STATE* pInputState)
{
	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (!pInputState->initialized)
	{
		SetDefaultMenuRepeatDelay();
		pInputState->NextTime = GetTimeCounter();
		SetDefaults();
		Widget_SetFont(PlyrFont);
		Widget_SetWindowColors(SHADOWBOX_BACKGROUND_COLOR,
							   SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

		current = nullptr;
		next = (WIDGET*)(&menus[0]);
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
	return true;
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
	int templat = choices[CHOICE_KBLAYOUT].selected;
	int control = widget->controlindex;
	int index = widget->highlighted;

	FlushInput();
	DrawLabelAsWindow(&labels[LABEL_PRESSTOEDIT], nullptr);
	RebindInputState(templat, control, index);
	populate_editkeys(templat);
	FlushInput();
}

static void
clear_control(WIDGET_CONTROLENTRY* widget)
{
	int templat = choices[CHOICE_KBLAYOUT].selected;
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

static stringbank* bank = nullptr;
static FRAME setup_frame = nullptr;

static constexpr const char* c_menuDefinitionResource {"ui.setupmenu"};
static uqstl::optional<uqm::MenuDefinition> menuDefinition {};
static uqstl::unique_ptr<uqm::MenuWidgets> menuWidgets {};
static bool mainMenuMusicChanged {};

// Menus that the menu definition file describes.
static const struct
{
	const char* id;
	int menu;
} definedMenus[] = {
	{"visual", MENU_VISUAL},
	{"music", MENU_MUSIC},
	{"qol", MENU_QOL},
};

static void
build_defined_menus(void)
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

	uqm::MenuWidgets::Hooks hooks {};
	hooks.isAddonAvailable = [](uqstl::string_view addon) {
		return uqm::isAddonAvailable(uqstl::string {addon}.c_str());
	};
	hooks.addonUnavailable = [] {
		DoPopupWindow(GAME_STRING(MAINMENU_STRING_BASE + 36));
		Widget_SetFont(PlyrFont);
	};
	hooks.settingChanged = [](const uqm::SettingBase& setting) {
		optRequiresReload = optRequiresReload || setting.requiresReload();
		optRequiresRestart = optRequiresRestart || setting.requiresRestart();
		if (setting.key() == "audio.mainMenuMusic")
		{
			mainMenuMusicChanged = true;
		}
	};
	menuWidgets = uqstl::make_unique<uqm::MenuWidgets>(uqm::UQMOptions::edit(),
													   uqm::MenuWidgets::SharedWidgets {(WIDGET*)(&labels[LABEL_SPACER]), (WIDGET*)(&buttons[BTN_QUITSUBMENU])},
													   uqstl::move(hooks));
	for (const auto& defined : definedMenus)
	{
		const uqm::MenuDef* menu {menuDefinition->find(defined.id)};
		if (menu == nullptr)
		{
			uqm::log::critical("PANIC: The setup menu definition has no '{}' menu.", defined.id);
			exit(EXIT_FAILURE);
		}

		WIDGET_MENU_SCREEN& screen {menus[defined.menu]};
		screen.subtitle = menu->subtitle.c_str();
		screen.bgStamp.frame = SetAbsFrameIndex(setup_frame, menu->background);
		screen.child = menuWidgets->build(*menu);
		screen.num_children = count_widgets(screen.child);
		screen.highlighted = 0;
	}
}

#define MAX_BUFF (MENU_COUNT + CHOICE_COUNT + SLIDER_COUNT + BUTTON_COUNT + LABEL_COUNT + TEXTENTRY_COUNT + CONTROLENTRY_COUNT)

static void
init_widgets(void)
{
	const char *buffer[MAX_BUFF], *str, *title;
	int count, i, index;

	if (bank == nullptr)
	{
		bank = StringBank_Create();
	}

	if (setup_frame == nullptr || optRequiresReload)
	{
		// Load the different menus depending on the resolution factor.
		setup_frame = CaptureDrawable(LoadGraphic(MENUBKG_PMAP_ANIM));
		LoadArrows();
	}

	count = GetStringTableCount(SetupTab);

	if (count < 3)
	{
		uqm::log::critical("PANIC: Setup string table too short to even "
						   "hold all indices!");
		exit(EXIT_FAILURE);
	}

	/* Menus */
	title = StringBank_AddOrFindString(bank,
									   GetStringAddress(SetAbsStringTableIndex(SetupTab, 0)));
	if (SplitString(GetStringAddress(SetAbsStringTableIndex(
						SetupTab, 1)),
					'\n', MAX_BUFF, buffer, bank)
		!= MENU_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		uqm::log::critical("PANIC: Incorrect number of Menu Subtitles");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < MENU_COUNT; i++)
	{
		menus[i].tag = WIDGET_TYPE_MENU_SCREEN;
		menus[i].parent = nullptr;
		menus[i].handleEvent = Widget_HandleEventMenuScreen;
		menus[i].receiveFocus = Widget_ReceiveFocusMenuScreen;
		menus[i].draw = Widget_DrawMenuScreen;
		menus[i].height = Widget_HeightFullScreen;
		menus[i].width = Widget_WidthFullScreen;
		menus[i].title = title;
		menus[i].subtitle = buffer[i];
		menus[i].bgStamp.origin.x = 0;
		menus[i].bgStamp.origin.y = 0;
		menus[i].bgStamp.frame =
			SetAbsFrameIndex(setup_frame, menu_defs[i].bgIndex);
		menus[i].num_children = count_widgets(menu_defs[i].widgets);
		menus[i].child = menu_defs[i].widgets;
		menus[i].highlighted = 0;
	}
	if (menu_defs[i].widgets != nullptr)
	{
		uqm::log::error("Menu definition array has more items!");
	}

	/* Options */
	if (SplitString(GetStringAddress(SetAbsStringTableIndex(
						SetupTab, 2)),
					'\n', MAX_BUFF, buffer, bank)
		!= CHOICE_COUNT)
	{
		const int expected {SplitString(GetStringAddress(SetAbsStringTableIndex(SetupTab, 2)), '\n', MAX_BUFF, buffer, bank)};
		uqm::log::critical("PANIC: Incorrect number of Choice Options: {}. Should be {}", static_cast<uint32_t>(CHOICE_COUNT), expected);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < CHOICE_COUNT; i++)
	{
		choices[i].tag = WIDGET_TYPE_CHOICE;
		choices[i].parent = nullptr;
		choices[i].handleEvent = Widget_HandleEventChoice;
		choices[i].receiveFocus = Widget_ReceiveFocusChoice;
		choices[i].draw = Widget_DrawChoice;
		choices[i].height = Widget_HeightChoice;
		choices[i].width = Widget_WidthFullScreen;
		choices[i].category = buffer[i];
		choices[i].numopts = 0;
		choices[i].options = nullptr;
		choices[i].selected = 0;
		choices[i].highlighted = 0;
		choices[i].onChange = nullptr;
	}

	/* Fill in the options now */
	index = 3; /* Index into string table */
	for (i = 0; i < CHOICE_COUNT; i++)
	{
		int j, optcount;

		if (index >= count)
		{
			uqm::log::critical("PANIC: String table cut short while "
							   "reading choices");
			exit(EXIT_FAILURE);
		}
		str = GetStringAddress(
			SetAbsStringTableIndex(SetupTab, index++));
		optcount = SplitString(str, '\n', MAX_BUFF, buffer, bank);
		choices[i].numopts = optcount;
		choices[i].options = (CHOICE_OPTION*)HMalloc(optcount * sizeof(CHOICE_OPTION));
		choices[i].choice_num = i;
		for (j = 0; j < optcount; j++)
		{
			choices[i].options[j].optname = buffer[j];
			choices[i].options[j].tooltip[0] = "";
			choices[i].options[j].tooltip[1] = "";
			choices[i].options[j].tooltip[2] = "";
		}
		for (j = 0; j < optcount; j++)
		{
			int k, tipcount;

			if (index >= count)
			{
				uqm::log::critical("PANIC: String table cut short while "
								   "reading choices");
				exit(EXIT_FAILURE);
			}
			str = GetStringAddress(
				SetAbsStringTableIndex(SetupTab, index++));
			tipcount = SplitString(str, '\n', MAX_BUFF, buffer, bank);
			if (tipcount > 3)
			{
				tipcount = 3;
			}
			for (k = 0; k < tipcount; k++)
			{
				choices[i].options[j].tooltip[k] = buffer[k];
			}
		}
	}

	// Code to swap resolution optnames for correct ones
	/*for (i = 0; i < choices[CHOICE_RESOLUTION].numopts - 1; i++)
	{
		fmt::format_to_sz_n(choices[CHOICE_RESOLUTION].options[i].optname,
			strlen(choices[CHOICE_RESOLUTION].options[i].optname),
			"{}x{}", RES_DESCALE (CanvasWidth)*(i+1), 
			RES_DESCALE (CanvasHeight)* (i + 1));
	}*/

	// Choices 18-20 are also special, being the names of the key
	// configurations
	for (i = 0; i < 6; i++)
	{
		choices[CHOICE_BTMPLAYER].options[i].optname = uqm::input_templates[i].name;
		choices[CHOICE_TOPPLAYER].options[i].optname = uqm::input_templates[i].name;
		choices[CHOICE_KBLAYOUT].options[i].optname = uqm::input_templates[i].name;
	}

	/* Choice 20 has a special onChange handler, too. */
	choices[CHOICE_KBLAYOUT].onChange = change_template;
	choices[CHOICE_GAMESEED].onChange = change_seedtype;

	// Check addon availability for HD mode, DOS/3DO mode, and music remixes
	choices[CHOICE_GRAPHICS].onChange = check_availability;
	choices[CHOICE_WINDOWTYPE].onChange = check_availability;

	// Handle display option
	choices[CHOICE_FRBUFFER].onChange = process_graphics_options;
	choices[CHOICE_SCALER].onChange = process_graphics_options;
	choices[CHOICE_SCANLINE].onChange = process_graphics_options;
	choices[CHOICE_DISPLAY].onChange = process_graphics_options;
	choices[CHOICE_SHOWFPS].onChange = process_graphics_options;
	choices[CHOICE_ASPRATIO].onChange = process_graphics_options;
	choices[CHOICE_RESOLUTION].onChange = process_graphics_options;

	/* Sliders */
	if (index >= count)
	{
		uqm::log::critical(
			"PANIC: String table cut short while reading sliders");
		exit(EXIT_FAILURE);
	}

	const auto rawSliderString {GetStringAddress(SetAbsStringTableIndex(SetupTab, index++))};
	if (SplitString(rawSliderString, '\n', MAX_BUFF, buffer, bank)
		!= SLIDER_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		uqm::log::critical("PANIC: Incorrect number of Slider Options");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < SLIDER_COUNT; i++)
	{
		sliders[i].tag = WIDGET_TYPE_SLIDER;
		sliders[i].parent = nullptr;
		sliders[i].handleEvent = Widget_HandleEventSlider;
		sliders[i].receiveFocus = Widget_ReceiveFocusSimple;
		sliders[i].draw = Widget_DrawSlider;
		sliders[i].height = Widget_HeightOneLine;
		sliders[i].width = Widget_WidthFullScreen;
		sliders[i].draw_value = Widget_Slider_DrawValue;
		sliders[i].min = 0;
		sliders[i].max = 100;
		sliders[i].step = 5;
		sliders[i].value = 75;
		sliders[i].category = buffer[i];
		sliders[i].tooltip[0] = "";
		sliders[i].tooltip[1] = "";
		sliders[i].tooltip[2] = "";
		sliders[i].onChange = nullptr;
	}
	sliders[SLIDER_MUSVOLUME].onChange = adjustMusic;
	sliders[SLIDER_SFXVOLUME].onChange = adjustSFX;
	sliders[SLIDER_SPCHVOLUME].onChange = adjustSpeech;

	// gamma is a special case
	sliders[SLIDER_GAMMA].step = 1;
	sliders[SLIDER_GAMMA].handleEvent = gamma_HandleEventSlider;
	sliders[SLIDER_GAMMA].draw_value = gamma_DrawValue;

	// nebulaevol is a special case
	sliders[SLIDER_NEBULA].step = 1;
	sliders[SLIDER_NEBULA].max = 50;

	// special settings for time dilation
	sliders[SLIDER_TIMEDILATION].min = 0;
	sliders[SLIDER_TIMEDILATION].max = 1000;
	sliders[SLIDER_TIMEDILATION].step = 10;
	sliders[SLIDER_TIMEDILATION].draw_value = Widget_Slider_DrawPercentageValue;

	for (i = 0; i < SLIDER_COUNT; i++)
	{
		int j, tipcount;

		if (index >= count)
		{
			uqm::log::critical(
				"PANIC: String table cut short while reading sliders");
			exit(EXIT_FAILURE);
		}
		str = GetStringAddress(
			SetAbsStringTableIndex(SetupTab, index++));
		tipcount = SplitString(str, '\n', MAX_BUFF, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			sliders[i].tooltip[j] = buffer[j];
		}
	}

	/* Buttons */
	if (index >= count)
	{
		uqm::log::critical(
			"PANIC: String table cut short while reading buttons");
		exit(EXIT_FAILURE);
	}

	if (SplitString(GetStringAddress(SetAbsStringTableIndex(
						SetupTab, index++)),
					'\n', MAX_BUFF, buffer, bank)
		!= BUTTON_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		uqm::log::critical("PANIC: Incorrect number of Button Options");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < BUTTON_COUNT; i++)
	{
		buttons[i].tag = WIDGET_TYPE_BUTTON;
		buttons[i].parent = nullptr;
		buttons[i].handleEvent = button_handlers[i];
		buttons[i].receiveFocus = Widget_ReceiveFocusSimple;
		buttons[i].draw = Widget_DrawButton;
		buttons[i].height = Widget_HeightOneLine;
		buttons[i].width = Widget_WidthFullScreen;
		buttons[i].name = buffer[i];
		buttons[i].tooltip[0] = "";
		buttons[i].tooltip[1] = "";
		buttons[i].tooltip[2] = "";
	}

	for (i = 0; i < BUTTON_COUNT; i++)
	{
		int j, tipcount;

		if (index >= count)
		{
			uqm::log::critical(
				"PANIC: String table cut short while reading buttons");
			exit(EXIT_FAILURE);
		}
		str = GetStringAddress(
			SetAbsStringTableIndex(SetupTab, index++));
		tipcount = SplitString(str, '\n', MAX_BUFF, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			buttons[i].tooltip[j] = buffer[j];
		}
	}

	/* Labels */
	if (index >= count)
	{
		uqm::log::critical(
			"PANIC: String table cut short while reading labels");
		exit(EXIT_FAILURE);
	}

	if (SplitString(GetStringAddress(SetAbsStringTableIndex(
						SetupTab, index++)),
					'\n', MAX_BUFF, buffer, bank)
		!= LABEL_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		uqm::log::critical("PANIC: Incorrect number of Label Options");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		labels[i].tag = WIDGET_TYPE_LABEL;
		labels[i].parent = nullptr;
		labels[i].handleEvent = Widget_HandleEventIgnoreAll;
		labels[i].receiveFocus = Widget_ReceiveFocusRefuseFocus;
		labels[i].draw = Widget_DrawLabel;
		labels[i].height = Widget_HeightLabel;
		labels[i].width = Widget_WidthFullScreen;
		labels[i].line_count = 0;
		labels[i].lines = nullptr;
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		int j, linecount;

		if (index >= count)
		{
			uqm::log::critical("PANIC: String table cut short while "
							   "reading labels");
			exit(EXIT_FAILURE);
		}
		str = GetStringAddress(
			SetAbsStringTableIndex(SetupTab, index++));
		linecount = SplitString(str, '\n', MAX_BUFF, buffer, bank);
		labels[i].line_count = linecount;
		labels[i].lines =
			(const char**)HMalloc(linecount * sizeof(const char*));
		for (j = 0; j < linecount; j++)
		{
			labels[i].lines[j] = buffer[j];
		}
	}

	/* Text Entry boxes */
	if (index >= count)
	{
		uqm::log::critical("PANIC: String table cut short while reading "
						   "text entries");
		exit(EXIT_FAILURE);
	}

	if (SplitString(GetStringAddress(SetAbsStringTableIndex(
						SetupTab, index++)),
					'\n', MAX_BUFF, buffer, bank)
		!= TEXTENTRY_COUNT)
	{
		uqm::log::critical("PANIC: Incorrect number of Text Entries");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < TEXTENTRY_COUNT; i++)
	{
		textentries[i].tag = WIDGET_TYPE_TEXTENTRY;
		textentries[i].parent = nullptr;
		textentries[i].handleEvent = Widget_HandleEventTextEntry;
		textentries[i].receiveFocus = Widget_ReceiveFocusSimple;
		textentries[i].draw = Widget_DrawTextEntry;
		textentries[i].height = Widget_HeightOneLine;
		textentries[i].width = Widget_WidthFullScreen;
		textentries[i].handleEventSelect = OnTextEntryEvent;
		textentries[i].category = buffer[i];
		textentries[i].value[0] = 0;
		textentries[i].maxlen = WIDGET_TEXTENTRY_WIDTH - 1;
		textentries[i].state = WTE_NORMAL;
		textentries[i].cursor_pos = 0;
		textentries[i].tooltip[0] = "";
		textentries[i].tooltip[1] = "";
		textentries[i].tooltip[2] = "";
	}
	if (SplitString(GetStringAddress(SetAbsStringTableIndex(
						SetupTab, index++)),
					'\n', MAX_BUFF, buffer, bank)
		!= TEXTENTRY_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		uqm::log::critical("PANIC: Incorrect number of Text Entries");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < TEXTENTRY_COUNT; i++)
	{
		int j, tipcount;

		uqm::strncpy_safe(textentries[i].value, {buffer[i], static_cast<uint32_t>(textentries[i].maxlen)});

		if (index >= count)
		{
			uqm::log::critical("PANIC: String table cut short while "
							   "reading text entries");
			exit(EXIT_FAILURE);
		}
		str = GetStringAddress(SetAbsStringTableIndex(SetupTab, index++));
		tipcount = SplitString(str, '\n', MAX_BUFF, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			textentries[i].tooltip[j] = buffer[j];
		}
	}

	textentries[TEXT_LOUTNAME].onChange = rename_template;
	textentries[TEXT_GAMESEED].onChange = change_seed;
	textentries[TEXT_CUSTMRES].onChange = change_res;

	/* Control Entry boxes */
	if (index >= count)
	{
		uqm::log::critical("PANIC: String table cut short while reading "
						   "control entries");
		exit(EXIT_FAILURE);
	}

	if (SplitString(GetStringAddress(SetAbsStringTableIndex(
						SetupTab, index++)),
					'\n', MAX_BUFF, buffer, bank)
		!= CONTROLENTRY_COUNT)
	{
		uqm::log::critical("PANIC: Incorrect number of Control Entries");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < CONTROLENTRY_COUNT; i++)
	{
		controlentries[i].tag = WIDGET_TYPE_CONTROLENTRY;
		controlentries[i].parent = nullptr;
		controlentries[i].handleEvent = Widget_HandleEventControlEntry;
		controlentries[i].receiveFocus = Widget_ReceiveFocusControlEntry;
		controlentries[i].draw = Widget_DrawControlEntry;
		controlentries[i].height = Widget_HeightOneLine;
		controlentries[i].width = Widget_WidthFullScreen;
		controlentries[i].category = buffer[i];
		controlentries[i].highlighted = 0;
		controlentries[i].controlname[0][0] = 0;
		controlentries[i].controlname[1][0] = 0;
		controlentries[i].controlindex = i;
		controlentries[i].onChange = rebind_control;
		controlentries[i].onDelete = clear_control;
	}

	/* Check for garbage at the end */
	if (index < count)
	{
		uqm::log::warn("WARNING: Setup strings had {} garbage "
					   "entries at the end.",
					   count - index);
	}

	build_defined_menus();

	testSounds = CaptureSound(LoadSound(TEST_SOUNDS));
}

static void
clean_up_widgets(void)
{
	int i;

	menuWidgets.reset();
	menuDefinition.reset();

	for (i = 0; i < CHOICE_COUNT; i++)
	{
		if (choices[i].options)
		{
			HFree(choices[i].options);
		}
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		if (labels[i].lines)
		{
			HFree((void*)labels[i].lines);
		}
	}

	/* Clear out the master tables */

	if (SetupTab)
	{
		DestroyStringTable(ReleaseStringTable(SetupTab));
		SetupTab = 0;
	}
	if (bank)
	{
		StringBank_Free(bank);
		bank = nullptr;
	}
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

void SetupMenu(void)
{
	SETUP_MENU_STATE s;

	s.InputFunc = DoSetupMenu;
	s.initialized = false;
	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetupTab = CaptureStringTable(LoadStringTable(SETUP_MENU_STRTAB));
	if (SetupTab)
	{
		init_widgets();
	}
	else
	{
		uqm::log::critical(
			"PANIC: Could not find strings for the setup menu!");
		exit(EXIT_FAILURE);
	}
	done = false;

	DoInput(&s, true);
	GLOBAL(CurrentActivity) &= ~CHECK_ABORT;
	PropagateResults();
	if (SetupTab)
	{
		clean_up_widgets();
	}

	SetMenuSounds(MENU_SOUND_UP | MENU_SOUND_DOWN,
				  MENU_SOUND_SELECT);
}

void GetGlobalOptions(GLOBALOPTS* opts)
{
	bool whichBound;
	uint8_t i;

	uqm::OptionsStruct& uqmOptions {uqm::UQMOptions::edit()};

	/*
 *		Graphics options
 */
	opts->screenResolution = (OPT_RESTYPE)(resolutionFactor >> 1);

	if (testFlag(g_gfxFlags, uqm::GfxFlags::Fullscreen))
	{
		opts->fullscreen = OPTVAL_BORDERED_FULLSCREEN;
	}
	else if (testFlag(g_gfxFlags, uqm::GfxFlags::ExclusiveFullscreen))
	{
		opts->fullscreen = OPTVAL_EXCLUSIVE_FULLSCREEN;
	}
	else
	{
		opts->fullscreen = OPTVAL_WINDOWED;
	}
	/*opts->fullscreen = (GfxFlags & TFB_GFXFLAGS_FULLSCREEN) ?
		OPTVAL_ENABLED : OPTVAL_DISABLED;*/
	opts->fps = testFlag(g_gfxFlags, uqm::GfxFlags::ShowFPS) ?
					OPTVAL_ENABLED :
					OPTVAL_DISABLED;
	opts->scanlines = testFlag(g_gfxFlags, uqm::GfxFlags::Scanlines) ?
						  OPTVAL_ENABLED :
						  OPTVAL_DISABLED;

	uqm::GfxFlags flags = g_gfxFlags & uqm::GfxFlagsScaleAny;

	switch (flags)
	{ // this works because there is only 1 scaler flag at a time
		case uqm::GfxFlags::Scale_Bilinear:
			opts->scaler = OPTVAL_BILINEAR_SCALE;
			break;
		case uqm::GfxFlags::Scale_BiAdapt:
			opts->scaler = OPTVAL_BIADAPT_SCALE;
			break;
		case uqm::GfxFlags::Scale_BiAdaptAdv:
			opts->scaler = OPTVAL_BIADV_SCALE;
			break;
		case uqm::GfxFlags::Scale_Triscan:
			opts->scaler = OPTVAL_TRISCAN_SCALE;
			break;
		case uqm::GfxFlags::Scale_HQXX:
			opts->scaler = OPTVAL_HQXX_SCALE;
			break;
		case uqm::GfxFlags::ExclusiveFullscreen:
		case uqm::GfxFlags::Fullscreen:
		case uqm::GfxFlags::Scanlines:
		case uqm::GfxFlags::ShowFPS:
		default:
			opts->scaler = OPTVAL_NO_SCALE;
			break;
	}

	opts->keepaspect = uqmOptions.keepAspectRatio.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;

	{
		float gamma = uqmOptions.gamma.value;
		whichBound = (gamma < maxGamma);
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
		opts->gamma = gammaToSlider(gamma);
	}


	opts->loresBlowup = (OPT_RESSCALER)uqmOptions.loresBlowupScale.value;
	/* Work out resolution.  On the way, try to guess a good default
	 * for config.alwaysgl, then overwrite it if it was set previously. */
	if ((!IS_HD && (GraphicsDriver != uqm::GfxDriver::SDL_Pure) && ((WindowWidth == 320) || (WindowWidth == 640))) || uqmOptions.alwaysOpenGL)
	{
		opts->driver = OPTVAL_ALWAYS_GL;
	}
	else
	{
		opts->driver = OPTVAL_PURE_IF_POSSIBLE;
	}

	opts->windowType = (OPT_WINDOWTYPE)optWindowType;
	switch (opts->windowType)
	{
		case OPTVAL_PC_WINDOW:
			if (!uqm::isAddonAvailable(DOS_MODE(IS_HD)))
			{
				opts->windowType = OPTVAL_UQM_WINDOW;
			}
			break;
		case OPTVAL_3DO_WINDOW:
			if (!uqm::isAddonAvailable(THREEDO_MODE(IS_HD)))
			{
				opts->windowType = OPTVAL_UQM_WINDOW;
			}
			break;
		default:
			break;
	}

	/*
 *		Audio options
 */
	opts->stereo = uqmOptions.stereoSFX.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;

	opts->speech = uqmOptions.useSpeech.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;

	switch (snddriver)
	{
		case AudioDriverType::OpenAL:
			opts->adriver = OPTVAL_OPENAL;
			break;
		case AudioDriverType::MixSDL:
			opts->adriver = OPTVAL_MIXSDL;
			break;
		default:
			opts->adriver = OPTVAL_SILENCE;
			break;
	}

	if (testFlag(soundflags, AudioFlags::QualityHigh))
	{
		opts->aquality = OPTVAL_HIGH;
	}
	else if (testFlag(soundflags, AudioFlags::QualityLow))
	{
		opts->aquality = OPTVAL_LOW;
	}
	else
	{
		opts->aquality = OPTVAL_MEDIUM;
	}

	MusVol = opts->musicvol =
		(((int)(musicVolumeScale * 100.0f) + 2) / 5) * 5;
	SfxVol = opts->sfxvol = (((int)(sfxVolumeScale * 100.0f) + 2) / 5) * 5;
	SpcVol = opts->speechvol =
		(((int)(speechVolumeScale * 100.0f) + 2) / 5) * 5;


	/*
 *		Engine&Visuals options
 */
	// Mics
	opts->subtitles = uqmOptions.subtitles.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->menu = whichPlatformOpt(uqmOptions.whichMenu.value);
	opts->text = whichPlatformOpt(uqmOptions.whichFonts.value);
	opts->scrTrans = whichPlatformOpt(uqmOptions.scrTrans.value);
	opts->intro = whichPlatformOpt(uqmOptions.whichIntro.value);
#ifdef MELEE_ZOOM
	optMScale = opts->meleezoom = (int)uqmOptions.meleeScale.value;
#else
	optMScale = opts->meleezoom =
		(OPT_MELEEZOOM)(uqm::toTFBScaleMode(uqmOptions.meleeScale.value) == uqm::TFBScaleMode::Step ? uqm::EmulationMode::PC : uqm::EmulationMode::Console3DO);
#endif
	opts->controllerType = uqmOptions.optControllerType.value;
	opts->directionalJoystick = uqmOptions.directionalJoystick.value ? OPTVAL_ENABLED : OPTVAL_DISABLED; // For Android
	opts->flagshipColor = whichPlatformOpt(uqmOptions.flagshipColor.value);
	opts->dosMenus = uqmOptions.dosMenus.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;

	// Interplanetary
	opts->starBackground = uqmOptions.starBackground.value;
	opts->planetStyle = whichPlatformOpt(uqmOptions.planetStyle.value);

	// Orbit
	opts->landerHold = whichPlatformOpt(uqmOptions.landerHold.value);
	opts->cscan = whichPlatformOpt(uqmOptions.whichCoarseScan.value);
	opts->scanStyle = whichPlatformOpt(uqmOptions.scanStyle.value);
	opts->landerStyle = whichPlatformOpt(uqmOptions.landerStyle.value);
	opts->sphereType = (OPT_SPHERETYPE)uqmOptions.sphereType.value;
	opts->tintPlanSphere = whichPlatformOpt(uqmOptions.tintPlanSphere.value);
	opts->shield = whichPlatformOpt(uqmOptions.whichShield.value);

	// Game modes
	opts->difficulty = uqmOptions.optDiffChooser.value;
	opts->extended = uqmOptions.extended.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->nomad = uqmOptions.nomad.value;
	opts->slaughterMode = uqmOptions.slaughterMode.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->seedType = (OPT_SEED)g_seedType;
	opts->shipSeed = uqmOptions.shipSeed.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->fleetPointSys = uqmOptions.fleetPointSys.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;

	// Comm screen
	opts->scroll = whichPlatformOpt(uqmOptions.smoothScroll.value);
	opts->scopeStyle = whichPlatformOpt(uqmOptions.scopeStyle.value);

	// Auto-Pilot

	// Controls
	opts->player1 = PlayerControlTemplates[0];
	opts->player2 = PlayerControlTemplates[1];

	// QoL

	/*
 *		Cheats
 */
	opts->cheatMode = uqmOptions.cheatMode.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->godModes = (OPT_GODTYPE)uqmOptions.optGodModes.value;
	opts->timeDilationPct = uqmOptions.timeDilationPct.value;
	opts->bubbleWarp = uqmOptions.bubbleWarp.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->unlockShips = uqmOptions.unlockShips.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->headStart = uqmOptions.headStart.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	//opts->unlockUpgrades = optUnlockUpgrades;
	opts->infiniteCredits = uqmOptions.infiniteCredits.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->infiniteRU = uqmOptions.infiniteRU.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->infiniteFuel = uqmOptions.infiniteFuel.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->noHQEncounters = uqmOptions.noHQEncounters.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->deCleansing = uqmOptions.deCleansing.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->meleeObstacles = uqmOptions.meleeObstacles.value ? OPTVAL_ENABLED : OPTVAL_DISABLED;

	// Devices
	for (i = 0; i < NUM_DEVICES; i++)
	{
		opts->deviceArray[i] = optDeviceArray[i];
	}

	// Upgrades
	for (i = 0; i < NUM_UPGRADES; i++)
	{
		opts->upgradeArray[i] = optUpgradeArray[i];
	}
}

void SetGlobalOptions(GLOBALOPTS* opts)
{
	AudioFlags NewSndFlags {AudioFlags::None};
	int resFactor = resolutionFactor;
	int newFactor;
	uint8_t i;

	auto& uqmOptions {uqm::UQMOptions::edit()};

	/*
 *		Graphics options
 */

	newFactor = (int)(opts->screenResolution << 1);
	putOpt(resFactor, newFactor, true);
	uqmOptions.resolutionFactor = resFactor;

	if (resFactor != (int)resolutionFactor)
	{
		SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));
		resolutionFactor = resFactor;

		switch (opts->windowType)
		{
			case OPTVAL_PC_WINDOW:
				if (!uqm::isAddonAvailable(DOS_MODE(resolutionFactor)))
				{
					opts->windowType = OPTVAL_UQM_WINDOW;
				}
				break;
			case OPTVAL_3DO_WINDOW:
				if (!uqm::isAddonAvailable(THREEDO_MODE(resolutionFactor)))
				{
					opts->windowType = OPTVAL_UQM_WINDOW;
				}
				break;
			default:
				break;
		}
	}

	if (optWindowType != opts->windowType)
	{
		putOpt((int&)optWindowType, (int)opts->windowType, true);
		uqmOptions.windowEmulationMode = (int)optWindowType;
	}

	putOpt(uqmOptions.keepAspectRatio.value, (bool)opts->keepaspect, false);

	// Avoid setting gamma when it is not necessary
	if (uqmOptions.gamma.value != 1.0f || sliderToGamma(opts->gamma) != 1.0f)
	{
		uqmOptions.gamma = sliderToGamma(opts->gamma);
		uqm::setGammaCorrection(uqmOptions.gamma.value);
	}


	/*
 *		Audio options
 */
	putOpt(uqmOptions.stereoSFX.value, (bool)opts->stereo, true);


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

	putOpt(uqmOptions.useSpeech.value, (bool)opts->speech, true);

	{
		AudioDriverType newDriver {snddriver};
		switch (opts->adriver)
		{
			case OPTVAL_SILENCE:
				newDriver = AudioDriverType::NoSound;
				break;
			case OPTVAL_MIXSDL:
				newDriver = AudioDriverType::MixSDL;
				break;
			case OPTVAL_OPENAL:
				newDriver = AudioDriverType::OpenAL;
				break;
			default:
				break;
		}
		if (snddriver != newDriver)
		{
			snddriver = newDriver;
			uqm::UQMOptions::edit().soundDriver = newDriver;
			optRequiresRestart = true;
		}
	}

	{
		AudioFlags newQualityFlag {AudioFlags::None};
		switch (opts->aquality)
		{
			case OPTVAL_LOW:
				newQualityFlag = AudioFlags::QualityLow;
				break;
			case OPTVAL_MEDIUM:
				newQualityFlag = AudioFlags::QualityMedium;
				break;
			case OPTVAL_HIGH:
				newQualityFlag = AudioFlags::QualityHigh;
				break;
			default:
				break;
		}
		if ((soundflags & AudioQualityFlagMask) != newQualityFlag)
		{
			NewSndFlags |= newQualityFlag;
			uqm::AudioQuality newQuality {uqm::AudioQuality::High};
			switch (opts->aquality)
			{
				case OPTVAL_LOW:
					newQuality = uqm::AudioQuality::Low;
					break;
				case OPTVAL_MEDIUM:
					newQuality = uqm::AudioQuality::Medium;
					break;
				default:
					break;
			}
			uqm::UQMOptions::edit().soundQuality = newQuality;
			soundflags = NewSndFlags;
			optRequiresRestart = true;
		}
	}

	// update actual volumes
	putOpt(SfxVol, opts->sfxvol, false);
	putOpt(MusVol, opts->musicvol, false);
	putOpt(SpcVol, opts->speechvol, false);
	uqmOptions.sfxVolumeScale = SfxVol / 100.0f;
	uqmOptions.musicVolumeScale = MusVol / 100.0f;
	uqmOptions.speechVolumeScale = SpcVol / 100.0f;


	/*
 *		Engine&Visuals options
 */
	// Mics
	putOpt(uqmOptions.subtitles.value, (bool)opts->subtitles, false);
	putOpt(uqmOptions.whichMenu.value, opts->menu, false);
	putOpt(uqmOptions.whichFonts.value, opts->text, false);
	putOpt(uqmOptions.scrTrans.value, opts->scrTrans, false);
	putOpt(uqmOptions.whichIntro.value, opts->intro, true);
	if (optMScale != (int)opts->meleezoom)
	{
#ifdef MELEE_ZOOM
		switch (opts->meleezoom)
		{
			case TFB_SCALE_NEAREST:
				uqmOptions.meleeScale = uqm::MeleeScaleMode::Nearest;
				break;
			case TFB_SCALE_BILINEAR:
				uqmOptions.meleeScale = uqm::MeleeScaleMode::Bilinear;
				break;
			case TFB_SCALE_TRILINEAR:
				uqmOptions.meleeScale = uqm::MeleeScaleMode::Smooth;
				break;
			case TFB_SCALE_STEP:
			default:
				uqmOptions.meleeScale = uqm::MeleeScaleMode::Step;
				break;
		}
#else
		uqm::TFBScaleMode tfbScale {((uqm::EmulationMode)opts->meleezoom == uqm::EmulationMode::Console3DO) ? uqm::TFBScaleMode::Trilinear : uqm::TFBScaleMode::Step};
		uqm::UQMOptions::edit().meleeScale = toMeleeScaleMode(tfbScale);
#endif
	}
#if SDL_MAJOR_VERSION == 1 // Refined joypad controls aren't supported on SDL1
	opts->controllerType = 0;
#endif
	putOpt(uqmOptions.optControllerType.value, opts->controllerType, false);
#ifdef DIRECTIONAL_JOY
	putOpt(uqmOptions.directionalJoystick.value, (bool)opts->directionalJoystick, false);
#endif
	putOpt(uqmOptions.flagshipColor.value, opts->flagshipColor, false);
	putOpt(uqmOptions.dosMenus.value, (bool)opts->dosMenus, false);

	// Interplanetary
	putOpt(uqmOptions.starBackground.value, opts->starBackground, false);
	putOpt(uqmOptions.planetStyle.value, opts->planetStyle, false);

	// Orbit
	putOpt(uqmOptions.landerHold.value, opts->landerHold, false);
	putOpt(uqmOptions.whichCoarseScan.value, opts->cscan, false);
	putOpt(uqmOptions.scanStyle.value, opts->scanStyle, false);
	putOpt(uqmOptions.landerStyle.value, opts->landerStyle, false);
	putOpt(uqmOptions.sphereType.value, (int)opts->sphereType, false);
	putOpt(uqmOptions.tintPlanSphere.value, opts->tintPlanSphere, false);
	putOpt(uqmOptions.whichShield.value, opts->shield, false);

	// Game modes
	{
		putOpt(g_seedType, (int)(opts->seedType), false);
		uqmOptions.seedType = g_seedType;
		int customSeed = atoi(textentries[TEXT_GAMESEED].value);
		if (!SANE_SEED(customSeed) || g_seedType == uqm::SeedType::Prime)
		{
			customSeed = PrimeA;
		}
		putOpt(uqmOptions.customSeed.value, customSeed, false);
		putOpt(uqmOptions.shipSeed.value, (bool)opts->shipSeed, false);
	}

	putOpt(uqmOptions.optDiffChooser.value, (int)opts->difficulty, false);
	if ((uqm::UQMOptions::edit().optDifficulty = (uqm::Difficulty)opts->difficulty) == uqm::Difficulty::ChooseYourOwn)
	{
		uqm::UQMOptions::edit().optDifficulty = uqm::Difficulty::Normal;
	}
	putOpt(uqmOptions.extended.value, (bool)opts->extended, false);
	putOpt(uqmOptions.nomad.value, (int)opts->nomad, false);
	putOpt(uqmOptions.slaughterMode.value, (bool)opts->slaughterMode, false);
	putOpt(uqmOptions.fleetPointSys.value, (bool)opts->fleetPointSys, false);

	// Comm screen
	putOpt(uqmOptions.smoothScroll.value, opts->scroll, false);
	putOpt(uqmOptions.scopeStyle.value, opts->scopeStyle, false);

	// Auto-Pilot

	// Controls
	PlayerControlTemplates[0] = opts->player1;
	PlayerControlTemplates[1] = opts->player2;

	uqmOptions.player1Control = static_cast<int>(opts->player1);
	uqmOptions.player2Control = static_cast<int>(opts->player2);

	res_PutString("keys.1.name", uqm::input_templates[0].name);
	res_PutString("keys.2.name", uqm::input_templates[1].name);
	res_PutString("keys.3.name", uqm::input_templates[2].name);
	res_PutString("keys.4.name", uqm::input_templates[3].name);
	res_PutString("keys.5.name", uqm::input_templates[4].name);
	res_PutString("keys.6.name", uqm::input_templates[5].name);


	/*
 *		Cheats
 */
	putOpt(uqmOptions.cheatMode.value, (bool)opts->cheatMode, false);
	putOpt(uqmOptions.optGodModes.value, (int)opts->godModes, false);
	putOpt(uqmOptions.timeDilationPct.value, opts->timeDilationPct, false);
	putOpt(uqmOptions.bubbleWarp.value, (bool)opts->bubbleWarp, false);
	putOpt(uqmOptions.unlockShips.value, (bool)opts->unlockShips, false);
	putOpt(uqmOptions.headStart.value, (bool)opts->headStart, false);
	//putOpt(optUnlockUpgrades opts->unlockUpgrades, false);
	putOpt(uqmOptions.infiniteCredits.value, (bool)opts->infiniteCredits, false);
	putOpt(uqmOptions.infiniteRU.value, (bool)opts->infiniteRU, false);
	putOpt(uqmOptions.infiniteFuel.value, (bool)opts->infiniteFuel, false);
	putOpt(uqmOptions.noHQEncounters.value, (bool)opts->noHQEncounters, false);
	putOpt(uqmOptions.deCleansing.value, (bool)opts->deCleansing, false);
	putOpt(uqmOptions.meleeObstacles.value, (bool)opts->meleeObstacles, false);

	// Devices
	for (i = 0; i < NUM_DEVICES; i++)
	{
		optDeviceArray[i] = opts->deviceArray[i];
	}

	// Upgrades
	for (i = 0; i < NUM_UPGRADES; i++)
	{
		optUpgradeArray[i] = opts->upgradeArray[i];
	}

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

		if (choices[CHOICE_RESOLUTION].selected != 6)
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
