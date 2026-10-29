//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#include "setup.h"

#include "coderes.h"
#include "controls.h"
#include "options.h"
#include "nameref.h"
#ifdef NETPLAY
#include "supermelee/netplay/netmelee.h"
#endif
#include "init.h"

#include "status.h"
#include "resinst.h"
#include "sounds.h"
#include "fmv.h"
#include "libs/compiler.h"
#include "libs/uio.h"
#include "libs/file.h"
#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound.h"
#include "libs/threadlib.h"
#include "libs/vidlib.h"
#include "core/log/log.h"
#include "libs/misc.h"

#include <assert.h>
#include <errno.h>
#include <string.h>


ACTIVITY LastActivity;
PlayerControlFlags PlayerControl[NUM_PLAYERS] {};

// XXX: These declarations should really go to the file they belong to.
RESOURCE_INDEX hResIndex;
GFXCONTEXT ScreenContext;
GFXCONTEXT SpaceContext;
GFXCONTEXT StatusContext;
GFXCONTEXT OffScreenContext;
int16_t screen_width, screen_height;
FRAME Screen;
FONT StarConFont;
FONT MicroFont;
FONT TinyFont;
FONT TinyFontBold;
FONT TinyFontCond;
FONT PlyrFont;
FONT LabelFont;
FONT SlabFont;
FONT SquareFont;
FONT PlayMenuFont;
QUEUE race_q[NUM_PLAYERS];
FRAME ActivityFrame;
FRAME StatusFrame;
FRAME SubmenuFrame;
FRAME FlagStatFrame;
FRAME MiscDataFrame;
FRAME visitedStarsFrame;
FRAME FontGradFrame;
FRAME BorderFrame;
FRAME HDBorderFrame;
FRAME CustBevelFrame;
FRAME DefBevelFrame;
STRING GameStrings;
QUEUE disp_q;

bool solTexturesPresent;
bool SyreenVoiceFix;
bool EndlessSCLoaded;
uint8_t Rando;
bool HDPackPresent;
bool VolasPackPresent;
bool DeathBySurrender = false;
bool DeathByMelee = false;
bool DeathBySuicide = false;
bool SpaceMusicOK;
bool oldPlanetsPresent;
bool classicPackPresent;

uio_Repository* repository;
uio_DirHandle* rootDir;

bool usingSpeech;


static void
InitPlayerInput(void)
{
}

void UninitPlayerInput(void)
{
#if DEMO_MODE
	DestroyInputDevice(ReleaseInputDevice(DemoInput));
#endif /* DEMO_MODE */
}

bool LoadKernel(int argc, char* argv[])
{
	InitSound(argc, argv);
	InitVideoPlayer(true);

	ScreenContext = CreateContext("ScreenContext");
	if (ScreenContext == nullptr)
	{
		return false;
	}

	Screen = CaptureDrawable(CreateDisplay(WANT_MASK | WANT_PIXMAP,
										   &screen_width, &screen_height));
	if (Screen == nullptr)
	{
		return false;
	}

	SetContext(ScreenContext);
	SetContextFGFrame(Screen);
	SetContextOrigin(MAKE_POINT(0, 0));

	hResIndex = (RESOURCE_INDEX)InitResourceSystem();
	if (hResIndex == 0)
	{
		return false;
	}

	/* Load base content. */
	if (uqm::loadIndices(contentDir) == 0)
	{
		return false; // Must have at least one index in content dir
	}

	classicPackPresent = false;

	if (!IS_HD)
	{
		EndlessSCLoaded = uqm::loadAddon("EndlessSC-SD");
		solTexturesPresent = uqm::loadAddon("sol-textures-sd");
		uqm::loadAddon("yellow-fried-sd");
	}
	else if (uqm::loadAddon("mm-hd"))
	{
		HDPackPresent = true;
		solTexturesPresent = uqm::loadAddon("sol-textures-hd");
		uqm::loadAddon("yellow-fried-hd");
		if (optWindowType == uqm::WindowStyle::UQM)
		{
			classicPackPresent =
				optNoClassic ? false : uqm::loadAddon("classic-pack");
		}
	}

	if (IS_PAD && uqm::isAddonAvailable(THREEDO_MODE(IS_HD)))
	{
		uqm::loadAddon(THREEDO_MODE(IS_HD));
	}
	if (IS_DOS && uqm::isAddonAvailable(DOS_MODE(IS_HD)))
	{
		uqm::loadAddon(DOS_MODE(IS_HD));
	}

	usingSpeech = (bool)uqm::UQMOptions::read().useSpeech;
	if (uqm::UQMOptions::read().useSpeech && !uqm::loadAddon("mm-3dovoice"))
	{
		usingSpeech = false;
	}

	if (usingSpeech)
	{
		uqm::loadAddon("mm-rmx-utwig");
		// Autoload support for Soul Reaver's dialog fixes
		uqm::loadAddon("mm-MelnormeVoiceFix");
		uqm::loadAddon("mm-distorted-hayes");
		SyreenVoiceFix = uqm::loadAddon("mm-SyreenVoiceFix");
	}

	if (uqm::UQMOptions::read().use3doMusic)
	{
		uqm::loadAddon("3domusic");
	}

	if (uqm::UQMOptions::read().useRemixMusic)
	{
		uqm::loadAddon("remix");
	}

	if (uqm::UQMOptions::read().volasMusic)
	{
		VolasPackPresent = uqm::loadAddon("volasaurus-remix-pack");

		SpaceMusicOK = uqm::UQMOptions::read().spaceMusic != uqm::SphereOfInfluenceMusic::None && VolasPackPresent;
	}

	if (!VolasPackPresent)
	{
		SpaceMusicOK = uqm::UQMOptions::read().spaceMusic != uqm::SphereOfInfluenceMusic::None && uqm::loadAddon("SpaceMusic");
	}

	if (uqm::UQMOptions::read().whichIntro == uqm::EmulationMode::Console3DO)
	{
		uqm::loadAddon("3dovideo");
	}

	uqm::loadAddon("ProfanePkunk");
	uqm::loadAddon("GlaDOS");

	if (!IS_HD)
	{
		// Localization addons
		uqm::loadAddon("xlat-finnish-sd");
		uqm::loadAddon("xlat-german-sd");
		uqm::loadAddon("xlat-japanese-sd");
		uqm::loadAddon("xlat-russian-sd");

		uqm::loadAddon("automods-sd");
	}
	else if (HDPackPresent)
	{
		// Localization addons
		uqm::loadAddon("xlat-finnish-hd");
		uqm::loadAddon("xlat-german-hd");
		uqm::loadAddon("xlat-japanese-hd");
		uqm::loadAddon("xlat-russian-hd");

		uqm::loadAddon("automods-hd");
	}

	/* Now load the rest of the addons, in order. */
	uqm::prepareAddons(uqm::UQMOptions::read().addons);

	{
		COLORMAP ColorMapTab {};

		ColorMapTab = CaptureColorMap(LoadColorMap(STARCON_COLOR_MAP));
		if (ColorMapTab == nullptr)
		{
			return false; // The most basic resource is missing
		}
		SetColorMap(GetColorMapAddress(ColorMapTab));
		DestroyColorMap(ReleaseColorMap(ColorMapTab));
	}

	InitPlayerInput();
	GLOBAL(CurrentActivity) = (ACTIVITY)~0;

	return true;
}

bool InitContexts(void)
{
	GFXRECT r;
	GFXCONTEXT oldContext;

	StatusContext = CreateContext("StatusContext");
	if (StatusContext == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	oldContext = SetContext(StatusContext);
	SetContextFGFrame(Screen);
	r.corner.x = SPACE_WIDTH + SAFE_X;
	r.corner.y = SAFE_Y;
	r.extent.width = STATUS_WIDTH;
	r.extent.height = STATUS_HEIGHT;
	SetContextClipRect(&r);

	SpaceContext = CreateContext("SpaceContext");
	if (SpaceContext == nullptr)
	{
		return false;
	}
	SetContext(oldContext);
	AdvanceLoadProgress();

	OffScreenContext = CreateContext("OffScreenContext");
	if (OffScreenContext == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	if (!InitQueue(&disp_q, MAX_DISPLAY_ELEMENTS, sizeof(ELEMENT)))
	{
		return false;
	}
	AdvanceLoadProgress();

	return true;
}

static bool
InitKernel(void)
{
	uint16_t counter;

	for (counter = 0; counter < NUM_PLAYERS; ++counter)
	{
		InitQueue(&race_q[counter], MAX_SHIPS_PER_SIDE, sizeof(STARSHIP));
	}

	StarConFont = LoadFont(STARCON_FONT);
	if (StarConFont == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	TinyFont = LoadFont(TINY_FONT);
	if (TinyFont == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	TinyFontBold = LoadFont(TINY_FONT_BOLD);
	if (TinyFontBold == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	TinyFontCond = LoadFont(TINY_FONT_COND);
	if (TinyFontCond == nullptr)
	{
		return false;
	}

	PlyrFont = LoadFont(PLAYER_FONT);
	if (PlyrFont == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	PlayMenuFont = LoadFont(PLAYMENU_FONT);
	if (PlayMenuFont == nullptr)
	{
		return false;
	}

	BorderFrame = CaptureDrawable(LoadGraphic(BORDER_MASK_PMAP_ANIM));
	if (BorderFrame == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	CustBevelFrame = CaptureDrawable(LoadGraphic(CUST_BEVEL_MASK_PMAP_ANIM));
	if (CustBevelFrame == nullptr)
	{
		return false;
	}

	if (HDPackPresent)
	{
		HDBorderFrame = CaptureDrawable(LoadGraphic(HD_BORDER_MASK_PMAP_ANIM));
		if (HDBorderFrame == nullptr)
		{
			return false;
		}

		DefBevelFrame = CaptureDrawable(LoadGraphic(DEF_BEVEL_MASK_PMAP_ANIM));
		if (DefBevelFrame == nullptr)
		{
			return false;
		}
	}

	ActivityFrame = CaptureDrawable(LoadGraphic(ACTIVITY_ANIM));
	if (ActivityFrame == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	StatusFrame = CaptureDrawable(LoadGraphic(STATUS_MASK_PMAP_ANIM));
	if (StatusFrame == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	SubmenuFrame = CaptureDrawable(LoadGraphic(SUBMENU_MASK_PMAP_ANIM));
	if (SubmenuFrame == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	GameStrings = CaptureStringTable(LoadStringTable(STARCON_GAME_STRINGS));
	if (GameStrings == 0)
	{
		return false;
	}
	AdvanceLoadProgress();

	MicroFont = LoadFont(MICRO_FONT);
	if (MicroFont == nullptr)
	{
		return false;
	}
	AdvanceLoadProgress();

	LabelFont = LoadFont(LABEL_FONT);
	if (LabelFont == nullptr)
	{
		return false;
	}

	SquareFont = LoadFont(SQUARE_FONT);
	if (SquareFont == nullptr)
	{
		return false;
	}

	SlabFont = LoadFont(SLAB_FONT);
	if (SlabFont == nullptr)
	{
		return false;
	}

	MenuSounds = CaptureSound(LoadSound(MENU_SOUNDS));
	if (MenuSounds == 0)
	{
		return false;
	}
	AdvanceLoadProgress();

	InitStatusOffsets();
	InitSpace();
	AdvanceLoadProgress();

	return true;
}

bool InitGameKernel(void)
{
	if (ActivityFrame == 0)
	{
		InitKernel();
		InitContexts();
	}
	return true;
}

bool SetPlayerInput(uint16_t playerI)
{
	assert(PlayerInput[playerI] == nullptr);

	const auto extractedControls {PlayerControl[playerI] & ControlFlagsMask};
	if (testFlag(extractedControls, PlayerControlFlags::Human))
	{
		PlayerInput[playerI] = (InputContext*)HumanInputContext_new(playerI);
		uqm::log::debug("Player {} is human-controlled.", playerI);
	}
	else if (testFlag(extractedControls, PlayerControlFlags::Cyborg | PlayerControlFlags::Psytron))
	{
		// ComputerControlFlags is used in SuperMelee; the computer chooses
		// the ships and fights the battles.
		// PlayerControlFlags::Cyborg is used in the full game; the computer only
		// fights the battles. XXX: This will need to be handled
		// separately in the future if we want to remove the special
		// cases for ship selection with PlayerControlFlags::Cyborg from the
		// computer handlers.
		PlayerInput[playerI] = (InputContext*)ComputerInputContext_new(playerI);
		uqm::log::debug("Player {} is computer-controlled (Cyborg).", playerI);
	}
#ifdef NETPLAY
	else if (testFlag(extractedControls, PlayerControlFlags::Network))
	{
		PlayerInput[playerI] = (InputContext*)NetworkInputContext_new(playerI);
		uqm::log::debug("Player {} is network-controlled.", playerI);
	}
#endif
	else
	{
		uqm::log::critical(
			"Invalid control method {} ,in SetPlayerInput().", extractedControls);
		explode(); /* Does not return */
	}

	return PlayerInput[playerI] != nullptr;
}

bool SetPlayerInputAll(void)
{
	uint16_t playerI;
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
	{
		if (!SetPlayerInput(playerI))
		{
			return false;
		}
	}
	return true;
}

void ClearPlayerInput(uint16_t playerI)
{
	if (PlayerInput[playerI] == nullptr)
	{
		uqm::log::debug("ClearPlayerInput(): PlayerInput[{}] was nullptr.",
						playerI);
		return;
	}

	PlayerInput[playerI]->handlers->deleteContext(PlayerInput[playerI]);
	PlayerInput[playerI] = nullptr;
}

void ClearPlayerInputAll(void)
{
	uint16_t playerI;
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
	{
		ClearPlayerInput(playerI);
	}
}

int initIO(void)
{
	uio_init();
	repository = uio_openRepository(0);

	rootDir = uio_openDir(repository, "/", 0);
	if (rootDir == nullptr)
	{
		uqm::log::critical("Could not open '/' dir.");
		return -1;
	}
	return 0;
}

void uninitIO(void)
{
	uio_closeDir(rootDir);
	uio_closeRepository(repository);
	uio_unInit();
}