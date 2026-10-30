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

#include <stdlib.h>
#include <atomic>

#include "comm.h"
#include "battle.h"
#include "fmv.h"
#include "gameev.h"
#include <cstdint>
#include "globdata.h"
#include "resinst.h"
#include "restart.h"
#include "starbase.h"
#include "save.h"
#include "setup.h"
#include "master.h"
#include "controls.h"
#include "starcon.h"
#include "clock.h"
// for GameClockTick()
#include "hyper.h"
// for SeedUniverse()
#include "planets/planets.h"
// for ExploreSolarSys()
#include "uqmdebug.h"
#include "uqm/lua/luastate.h"
#include "libs/time/timelib.h"
#include "core/log/log.h"
#include "core/platform/platform.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/tfb_draw.h"
#include "core/misc/FatalExit.h"
#include "libs/luauqm/luauqm.h"
#include "build.h"
#include "uqmversion.h"
#include "options.h"
#include "setupmenu.h"
#include "build.h"
#include "nameref.h"
#include "settings.h"
#include "cons_res.h"
#include <time.h> //required to use 'srand(time(nullptr))'
#include "sounds.h"

uqstl::atomic<bool> MainExited {false};

// Open or close the periodically occuring QuasiSpace portal.
// It changes the appearant portal size when necessary.
static void
checkArilouGate(void)
{
	uint8_t counter;

	counter = GET_GAME_STATE(ARILOU_SPACE_COUNTER);
	if (GET_GAME_STATE(ARILOU_SPACE) == OPENING)
	{ // The portal is opening or fully open
		if (counter < 9)
		{
			++counter;
		}
	}
	else
	{ // The portal is closing or fully closed
		if (counter > 0)
		{
			--counter;
		}
	}
	SET_GAME_STATE(ARILOU_SPACE_COUNTER, counter);
}

// Battle frame callback function.
static void
on_battle_frame(void)
{
	GameClockTick();
	checkArilouGate();

	if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		SeedUniverse();
	}

	DrawAutoPilotMessage(false);
}

static void
BackgroundInitKernel(uint32_t TimeOut)
{
	LoadMasterShipList(TaskSwitch);
	TaskSwitch();
	InitGameKernel();

	while ((GetTimeCounter() <= TimeOut) && !(GLOBAL(CurrentActivity) & CHECK_ABORT))
	{
		UpdateInputState();
		TaskSwitch();
	}
}

// Executes on the main() thread
void SignalStopMainThread(void)
{
	GamePaused = false;
	GLOBAL(CurrentActivity) |= CHECK_ABORT;
	TaskSwitch();
}

// Executes on the main() thread
void ProcessUtilityKeys(void)
{
	if (ImmediateInputState.menu[KEY_ABORT])
	{
		//log_showBox(false, false);
		exit(EXIT_SUCCESS);
	}

	if (ImmediateInputState.menu[KEY_FULLSCREEN])
	{
		uqm::GfxFlags flags = g_gfxFlags;

		if (testFlag(flags, uqm::GfxFlags::ExclusiveFullscreen))
		{
			flags &= ~uqm::GfxFlags::Fullscreen;
			flags &= ~uqm::GfxFlags::ExclusiveFullscreen;
		}
		else
		{
			flags ^= uqm::GfxFlags::Fullscreen;
		}

		if (IS_HD)
		{
			flags ^= uqm::GfxFlags::Scale_Bilinear;
		}

		// clear ImmediateInputState so we don't repeat this next frame
		FlushInput();
		TFB_DrawScreen_ReinitVideo(GraphicsDriver, flags,
								   WindowWidth, WindowHeight);
	}

	if (ImmediateInputState.menu[KEY_SCREENSHOT])
	{
		TFB_ScreenShot();
		FlushInput();
	}

#if defined(DEBUG) || defined(USE_DEBUG_KEY)
	{ // Only call the debug func on the rising edge of
		// ImmediateInputState[KEY_DEBUG] so it does not execute repeatedly.
		// This duplicates the PulsedInputState somewhat, but we cannot
		// use PulsedInputState here because it is meant for another thread.
		static int debugKeyState, debugKey2State,
			debugKey3State, debugKey4State;

		if (ImmediateInputState.menu[KEY_DEBUG] && debugKeyState == 0)
		{
			debugKeyPressed();
		}
		debugKeyState = ImmediateInputState.menu[KEY_DEBUG];

		if (ImmediateInputState.menu[KEY_DEBUG_2] && debugKey2State == 0)
		{
			debugKey2Pressed();
		}
		debugKey2State = ImmediateInputState.menu[KEY_DEBUG_2];

		if (ImmediateInputState.menu[KEY_DEBUG_3] && debugKey3State == 0)
		{
			debugKey3Pressed();
		}
		debugKey3State = ImmediateInputState.menu[KEY_DEBUG_3];

		if (ImmediateInputState.menu[KEY_DEBUG_4] && debugKey4State == 0)
		{
			debugKey4Pressed();
		}
		debugKey4State = ImmediateInputState.menu[KEY_DEBUG_4];
	}
#endif /* DEBUG */
}

static void
SetRandomMenuMusic(void)
{
	time_t t = time(nullptr);
	const uqstl::tm tm {uqm::localtime(t)};

	srand(t);
	Rando = (rand() % NUM_MM_THEMES);
	optMaskOfDeceit = tm.tm_mon == 3 && tm.tm_mday == 1;
}

/* TODO: Remove these declarations once threading is gone. */
extern AudioDriverType snddriver;
extern AudioFlags soundflags;

int Starcon2Main(void* threadArg)
{
	{
		/* TODO: Put initAudio back in main where it belongs once threading
		 *       is gone.
		 */
		extern int32_t initAudio(AudioDriverType driver, AudioFlags flags);
		initAudio(snddriver, soundflags);
	}

	if (!LoadKernel(0, 0))
	{
		uqm::log::critical("\n  *** FATAL ERROR: Could not load basic "
						   "content ***\n\nUQM requires at least the base content "
						   "pack to run properly.");
		uqm::log::critical("This file is typically called "
						   "mm-{}.{}.{}-content.uqm.  UQM was expecting",
						   UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION);
		uqm::log::critical("it in the {}/packages directory.",
						   baseContentPath);
		uqm::log::critical("Either your installation did not install the "
						   "content pack at all, or it\ninstalled it in a different "
						   "directory.\n\nFix your installation and rerun UQM.\n\n  "
						   "*******************\n");

		uqm::Logger::getInstance().showLogOnExit(true, true);

		MainExited = true;
		return EXIT_FAILURE;
	}
	uqm::log::info("We've loaded the Kernel");

	GLOBAL(CurrentActivity) = 0;
	luaUqm_initState();
	// show logo then splash and init the kernel in the meantime
	if (!uqm::UQMOptions::read().skipIntro)
	{
		Logo();
	}

	SetRandomMenuMusic();
	InitMenuMusic();

	SplashScreen(BackgroundInitKernel);

	uqm::log::info("Set Seed Type: {}", toString(g_seedType));
	uqm::log::info("Set Seed: {}", uqm::UQMOptions::read().customSeed.value);
	uqm::log::info("Set Difficulty: {:s}", uqm::UQMOptions::read().optDifficulty.value);
	uqm::log::info("Set Extended: {}", BOOL_STR(uqm::UQMOptions::read().extended));
	uqm::log::info("Set Nomad: {:s}", uqm::UQMOptions::read().nomad.value);

	while (StartGame())
	{
		// Initialise a new game
		if (!SetPlayerInputAll())
		{
			uqm::log::critical("Could not set player input.");
			uqm::fatalExit(); // Does not return;
		}

		luaUqm_reinitState();
		InitGameStructures();
		InitGameClock();
		initEventSystem();
		AddInitialGameEvents();

		// Reset Debug Key
		DebugKeyPressed = false;

		// Debug info when starting a new game
		if (LastActivity == (CHECK_LOAD | CHECK_RESTART))
		{
			uqm::log::info("New Game Seed Type: {}", toString(g_seedType));
			uqm::log::info("New Game Seed: {}", GLOBAL_SIS(Seed));
			uqm::log::info("New Game Difficulty: {:s}", getDifficulty());
			uqm::log::info("New Game Extended: {}",
						   BOOL_STR(GLOBAL_SIS(Extended)));
			uqm::log::info("New Game Nomad: {:s}", getNomadMode());
		}

		do
		{
			// No further debugHook calls unless the called
			// function resets debugHook.
			if (void (*saveDebugHook)(void) {debugHook.exchange(nullptr)})
			{
				(*saveDebugHook)();
				continue;
			}

			SetStatusMessageMode(SMM_DEFAULT);

			if (!((GLOBAL(CurrentActivity) | NextActivity) & CHECK_LOAD))
			{
				ZeroVelocityComponents(&GLOBAL(velocity));
			}
			// not going into talking pet conversation
			else if (GLOBAL(CurrentActivity) & CHECK_LOAD)
			{
				GLOBAL(CurrentActivity) = NextActivity;
			}

			if ((GLOBAL(CurrentActivity) & START_ENCOUNTER)
				|| GET_GAME_STATE(CHMMR_BOMB_STATE) == 2)
			{
				if (GET_GAME_STATE(CHMMR_BOMB_STATE) == 2
					&& !GET_GAME_STATE(STARBASE_AVAILABLE))
				{ /* BGD mode */
					InstallBombAtEarth();
				}
				else if (GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) == (uint8_t)~0
						 || GET_GAME_STATE(CHMMR_BOMB_STATE) == 2)
				{
					GLOBAL(CurrentActivity) |= START_ENCOUNTER;
					VisitStarBase();
				}
				else
				{
					GLOBAL(CurrentActivity) |= START_ENCOUNTER;
					RaceCommunication();
				}

				if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
				{
					GLOBAL(CurrentActivity) &= ~START_ENCOUNTER;
					if (uqm::lowByte(GLOBAL(CurrentActivity)) == IN_INTERPLANETARY)
					{
						GLOBAL(CurrentActivity) |= START_INTERPLANETARY;
					}
				}
			}
			else if (GLOBAL(CurrentActivity) & START_INTERPLANETARY)
			{
				GLOBAL(CurrentActivity) = uqm::makeWord(IN_INTERPLANETARY, 0);

				DrawAutoPilotMessage(true);
				SetGameClockRate(INTERPLANETARY_CLOCK_RATE);
				ExploreSolarSys();
			}
			else
			{
				// Entering HyperSpace or QuasiSpace.
				GLOBAL(CurrentActivity) = uqm::makeWord(IN_HYPERSPACE, 0);

				DrawAutoPilotMessage(true);
				SetGameClockRate(HYPERSPACE_CLOCK_RATE);
				Battle(&on_battle_frame);
			}

			SetFlashRect(nullptr, false);

			LastActivity = GLOBAL(CurrentActivity);

			if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				&& (uqm::lowByte(GLOBAL(CurrentActivity)) == WON_LAST_BATTLE
					// if died for some reason
					|| GLOBAL_SIS(CrewEnlisted) == (uint16_t)~0))
			{
				if (GET_GAME_STATE(KOHR_AH_KILLED_ALL))
				{
					InitCommunication(BLACKURQ_CONVERSATION);
					if (uqm::UQMOptions::read().gameOver)
					{
						GameOver(DEATH_MARCH);
					}
				}
				else if (GLOBAL(CurrentActivity) & CHECK_RESTART)
				{
					// surrendered to Ur-Quan
					DeathBySurrender = true;
					GLOBAL(CurrentActivity) &= ~CHECK_RESTART;
				}
				break;
			}
		} while (!(GLOBAL(CurrentActivity) & CHECK_ABORT));

		StopSound();
		uninitEventSystem();
		UninitGameClock();
		UninitGameStructures();
		ClearPlayerInputAll();
	}
	luaUqm_uninitState();

	UninitGameKernel();
	FreeMasterShipList();
	FreeKernel();

	MainExited = true;

	(void)threadArg; /* Satisfying compiler (unused parameter) */
	return 0;
}
