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

#include "restart.h"

#include "colors.h"
#include "controls.h"
#include "credits.h"
#include "starmap.h"
#include "fmv.h"
#include "menustat.h"
#include "gamestr.h"
#include "globdata.h"
#include "intel.h"
#include "supermelee/melee.h"
#include "resinst.h"
#include "nameref.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "setupmenu.h"
#include "util.h"
#include "starcon.h"
#include "uqmversion.h"
#include "core/string/StringUtils.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/graphics/sdl/pure.h"
#include "core/log/log.h"
#include "options.h"
#include "cons_res.h"
#include "build.h"
#include "master.h"
#include "StarmapGlobalData.h"

#include "libs/resource/stringbank.h"
// for StringBank_Create() & SplitString()

enum
{
	START_NEW_GAME = 0,
	LOAD_SAVED_GAME,
	PLAY_SUPER_MELEE,
	SETUP_GAME,
	QUIT_GAME,
	NUM_MENU_ELEMENTS
};

enum
{
	EASY_DIFF = 0,
	ORIGINAL_DIFF,
	HARD_DIFF
};

#define CHOOSER_X (SCREEN_WIDTH >> 1)
#define CHOOSER_Y ((SCREEN_HEIGHT >> 1) - RES_SCALE(12))

// Kruzen: Having this ref separated gains more control
// We can load and free it whenever we want and not rely on menu volume
MUSIC_REF menuMusic;

void InitMenuMusic(void)
{
	if (uqm::UQMOptions::read().mainMenuMusic && !(menuMusic))
	{
		FadeMusic(MUTE_VOLUME, 0);
		menuMusic = loadMainMenuMusic(Rando);
		PlayMusic(menuMusic, true, 1);

		if (OkayToResume())
		{
			SeekMusic(GetMusicPosition());
		}

		FadeMusic(NORMAL_VOLUME + 70, GameTicksPerSecond * 3);
	}
}

void UninitMenuMusic(void)
{
	if (menuMusic)
	{
		SleepThreadUntil(FadeMusic(MUTE_VOLUME, GameTicksPerSecond));

		SetMusicPosition();
		StopMusic();
		DestroyMusic(menuMusic);
		menuMusic = 0;

		FadeMusic(NORMAL_VOLUME, 0);
	}
}

void DrawToolTips(MENU_STATE* pMS, int answer)
{
	uint16_t i;
	TEXT t;
	stringbank* bank = StringBank_Create();
	const char* lines[30];
	int line_count;
	GFXRECT r;
	STAMP s;

	SetContextFont(TinyFont);

	GetFrameRect(SetRelFrameIndex(pMS->CurFrame, 2), &r);
	r.corner.y += CHOOSER_Y + r.extent.height + RES_SCALE(1);

	s.frame = SetRelFrameIndex(pMS->CurFrame, 3);
	r.extent = GetFrameBounds(s.frame);
	r.corner.x = RES_SCALE(
		(RES_DESCALE(CanvasWidth) - RES_DESCALE(r.extent.width)) >> 1);
	s.origin = r.corner;
	DrawStamp(&s);

	SetContextForeGroundColor(BLACK_COLOR);

	t.pStr = GAME_STRING(MAINMENU_STRING_BASE + 66 + answer);
	line_count = SplitString(t.pStr, '\n', 30, lines, bank);

	t.baseline.x = r.corner.x
				 + RES_SCALE(RES_DESCALE(r.extent.width) >> 1);
	t.baseline.y = r.corner.y + RES_SCALE(10)
				 + RES_SCALE(line_count < 2 ? 8 : (line_count > 2 ? 0 : 3));
	for (i = 0; i < line_count; i++)
	{
		t.pStr = lines[i];
		t.align = ALIGN_CENTER;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += RES_SCALE(8);
	}

	StringBank_Free(bank);
}

static void
DrawDiffChooser(MENU_STATE* pMS, int answer, bool confirm)
{
	STAMP s;
	FONT oldFont;
	TEXT t;
	uint16_t i;

	s.origin = MAKE_POINT(CHOOSER_X, CHOOSER_Y);
	s.frame = SetRelFrameIndex(pMS->CurFrame, 2);
	DrawStamp(&s);

	DrawToolTips(pMS, answer);

	oldFont = SetContextFont(MicroFont);

	t.align = ALIGN_CENTER;
	t.baseline.x = s.origin.x;
	t.baseline.y = s.origin.y - RES_SCALE(20);

	for (i = 0; i <= 2; i++)
	{
		t.pStr = GAME_STRING(MAINMENU_STRING_BASE + 56
							 + (!i ? 1 : (i > 1 ? 2 : 0)));
		t.CharCount = (uint16_t)utf8StringCount(t.pStr);

		SetContextForeGroundColor(
			i == answer ?
				(confirm ? MENU_BACKGROUND_COLOR : MENU_HIGHLIGHT_COLOR) :
				BLACK_COLOR);
		font_DrawText(&t);

		t.baseline.y += RES_SCALE(23);
	}

	SetContextFont(oldFont);
}

static bool
DoDiffChooser(MENU_STATE* pMS)
{
	static TimeCount LastInputTime;
	static TimeCount InactTimeOut;
	GFXRECT oldRect;
	STAMP s;
	GFXCONTEXT oldContext;
	bool response = false;
	bool done = false;
	uint8_t a = 1;

	InactTimeOut = (uqm::UQMOptions::read().mainMenuMusic ? 60 : 20) * GameTicksPerSecond;
	LastInputTime = GetTimeCounter();

	oldContext = SetContext(ScreenContext);
	GetContextClipRect(&oldRect);
	s = SaveContextFrame(nullptr);

	DrawDiffChooser(pMS, a, false);

	FlushGraphics();
	FlushInput();

	while (!done)
	{
		UpdateInputState();

		if (GLOBAL(CurrentActivity) & CHECK_ABORT)
		{
			return false;
		}
		else if (PulsedInputState.menu[KEY_MENU_SELECT])
		{
			done = true;
			response = true;
			DrawDiffChooser(pMS, a, true);
			PlayMenuSound(MENU_SOUND_SUCCESS);
		}
		else if (PulsedInputState.menu[KEY_MENU_CANCEL]
				 || CurrentInputState.menu[KEY_EXIT])
		{
			done = true;
			response = false;
			DrawStamp(&s);
		}
		else if (PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_DOWN] || PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			uint8_t NewState;

			NewState = a;
			if (PulsedInputState.menu[KEY_MENU_UP]
				|| PulsedInputState.menu[KEY_MENU_LEFT])
			{
				if (NewState == EASY_DIFF)
				{
					NewState = HARD_DIFF;
				}
				else
				{
					--NewState;
				}
			}
			else if (PulsedInputState.menu[KEY_MENU_DOWN]
					 || PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				if (NewState == HARD_DIFF)
				{
					NewState = EASY_DIFF;
				}
				else
				{
					++NewState;
				}
			}
			if (NewState != a)
			{
				BatchGraphics();
				DrawDiffChooser(pMS, NewState, false);
				UnbatchGraphics();
				a = NewState;
			}

			PlayMenuSound(MENU_SOUND_MOVE);

			LastInputTime = GetTimeCounter();
		}
		else if (GetTimeCounter() - LastInputTime > InactTimeOut)
		{ // timed out
			GLOBAL(CurrentActivity) = (ACTIVITY)~0;
			done = true;
			response = false;
		}

		SleepThread(GameTicksPerSecond / 30);
	}

	if (response)
	{
		switch (a)
		{
			case 0:
				uqm::UQMOptions::edit().optDifficulty = uqm::Difficulty::Easy;
				break;
			case 2:
				uqm::UQMOptions::edit().optDifficulty = uqm::Difficulty::Hard;
				break;
			case 1:
			default:
				uqm::UQMOptions::edit().optDifficulty = uqm::Difficulty::Normal;
				break;
		}
	}

	DestroyDrawable(ReleaseDrawable(s.frame));
	FlushInput();

	SetContextClipRect(&oldRect);
	SetContext(oldContext);

	return response;
}

#define MAIN_TEXT_X (SCREEN_WIDTH >> 1)
#define MAIN_TEXT_Y (RES_SCALE(42) - DOS_NUM_SCL(20))

FRAME TextCache[5];

static void
InitPulseText(void)
{
	FRAME frame, OldFrame;
	int16_t leading;
	TEXT t;
	uint16_t i;

	if (TextCache[0] != nullptr)
	{
		return;
	}

	SetContextFont(SlabFont);
	SetContextBackGroundColor(BLACK_COLOR);
	SetContextForeGroundColor(WHITE_COLOR);
	GetContextFontLeading(&leading);

	t.baseline.x = MAIN_TEXT_X;
	t.baseline.y = MAIN_TEXT_Y;
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;

	for (i = START_NEW_GAME; i < NUM_MENU_ELEMENTS; i++)
	{
		t.pStr = GAME_STRING(MAINMENU_STRING_BASE + 69 + i);

		frame = CaptureDrawable(CreateDrawable(WANT_PIXMAP, SCREEN_WIDTH,
											   SCREEN_HEIGHT, 1));
		SetFrameTransparentColor(frame, BLACK_COLOR);
		OldFrame = SetContextFGFrame(frame);
		ClearDrawable();
		SetContextFGFrame(OldFrame);

		OldFrame = SetContextFGFrame(frame);
		font_DrawText(&t);
		SetContextFGFrame(OldFrame);

		TextCache[i] = frame;

		t.baseline.y += leading;
	}
}

// Draw the full restart menu. Nothing is done with selections.
static void
DrawRestartMenuGraphic(MENU_STATE* pMS)
{
	GFXRECT r;
	STAMP s;
	TEXT t;
	char buf[64];
	uint16_t i;
	int16_t leading;

	s.frame = pMS->CurFrame;
	GetFrameRect(s.frame, &r);
	s.origin.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	s.origin.y = (SCREEN_HEIGHT - r.extent.height) >> 1;

	SetContextBackGroundColor(BLACK_COLOR);
	BatchGraphics();

	ClearDrawable();
	FlushColorXForms();
	DrawStamp(&s);

	s.frame = IncFrameIndex(pMS->CurFrame);
	DrawStamp(&s);

	SetContextFont(SlabFont);

	GetContextFontLeading(&leading);

	t.baseline.x = MAIN_TEXT_X;
	t.baseline.y = MAIN_TEXT_Y;
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;

	SetContextForeGroundColor(MAIN_MENU_TEXT_COLOR);

	for (i = START_NEW_GAME; i < NUM_MENU_ELEMENTS; i++)
	{
		t.pStr = GAME_STRING(MAINMENU_STRING_BASE + 69 + i);
		font_DrawText(&t);
		t.baseline.y += leading;
	}

	// Put the version number in the bottom right corner.
	SetContextFont(TinyFont);
	SetContextForeGroundColor(WHITE_COLOR);
	fmt::format_to_sz_n(buf, "v{}.{}.{} {}",
						UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
						chooseIfHd<const char*>(UQM_EXTRA_VERSION, "HD " UQM_EXTRA_VERSION));
	t.pStr = buf;
	t.baseline.x = SCREEN_WIDTH - RES_SCALE(2);
	t.baseline.y = SCREEN_HEIGHT - RES_SCALE(2);
	t.align = ALIGN_RIGHT;
	font_DrawText(&t);

	// Put the main menu music credit in the bottom left corner.
	if (uqm::UQMOptions::read().mainMenuMusic)
	{
		fmt::format_to_sz_n(buf, "{} {}",
							GAME_STRING(MAINMENU_STRING_BASE + 61),
							GAME_STRING(MAINMENU_STRING_BASE + 62 + Rando));
		t.baseline.x = RES_SCALE(2);
		t.baseline.y = SCREEN_HEIGHT - RES_SCALE(2);
		t.align = ALIGN_LEFT;
		font_DrawText(&t);
	}

	UnbatchGraphics();
}

static void
DrawRestartMenu(MENU_STATE* pMS, uint8_t NewState, FRAME f)
{
	GFXPOINT origin;
	origin.x = 0;
	origin.y = 0;
	Flash_setOverlay(pMS->flashContext, &origin, TextCache[NewState], false);

	(void)f; // Silence compiler warnings
}

static bool
RestartMessage(void)
{
	if (!optRequiresRestart)
	{
		return false;
	}

	SetFlashRect(nullptr, false);
	DoPopupWindow(GAME_STRING(MAINMENU_STRING_BASE + 35));
	// Got to restart -message
	SetMenuSounds(MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
	SetTransitionSource(nullptr);
	SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));
	GLOBAL(CurrentActivity) = CHECK_ABORT;
	restartGame = true;
	return true;
}

static bool
DoRestart(MENU_STATE* pMS)
{
	static TimeCount LastInputTime;
	static TimeCount InactTimeOut;
	TimeCount TimeIn = GetTimeCounter();

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (optWindowType != uqm::WindowStyle::UQM)
	{
		uqm::UQMOptions::edit().meleeToolTips = false;
	}

	if (optSuperMelee && !optLoadGame)
	{
		pMS->CurState = PLAY_SUPER_MELEE;
		PulsedInputState.menu[KEY_MENU_SELECT] = 65535;
	}
	else if (optLoadGame && !optSuperMelee)
	{
		pMS->CurState = LOAD_SAVED_GAME;
		PulsedInputState.menu[KEY_MENU_SELECT] = 65535;
	}

	if (pMS->Initialized && !(GLOBAL(CurrentActivity) & CHECK_ABORT))
	{
		Flash_process(pMS->flashContext);
	}

	if (!pMS->Initialized)
	{ // Kruzen: too much trouble using this one. Better to just turn it off
		pMS->hMusic = 0;

		InitMenuMusic();
		InitPulseText();
		ResetMusicResume();

		InactTimeOut = (uqm::UQMOptions::read().mainMenuMusic ? 60 : 20) * GameTicksPerSecond;

		pMS->flashContext = Flash_createOverlay(ScreenContext,
												nullptr, nullptr);
		Flash_setMergeFactors(pMS->flashContext, -3, 3, 16);
		Flash_setSpeed(pMS->flashContext, (6 * GameTicksPerSecond) / 14, 0,
					   (6 * GameTicksPerSecond) / 14, 0);
		Flash_setFrameTime(pMS->flashContext, GameTicksPerSecond / 16);
		Flash_setState(pMS->flashContext, FlashState_fadeIn,
					   (3 * GameTicksPerSecond) / 16);
		Flash_setPulseBox(pMS->flashContext, false);

		DrawRestartMenu(pMS, pMS->CurState, nullptr);
		Flash_start(pMS->flashContext);

		LastInputTime = GetTimeCounter();
		pMS->Initialized = true;

		SleepThreadUntil(FadeScreen(FadeAllToColor, GameTicksPerSecond / 2));
	}
	else if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		SleepThreadUntil(
			FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));
		return false;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case START_NEW_GAME:
				if (uqm::UQMOptions::read().customSeed == 404)
				{
					SetFlashRect(nullptr, false);
					DoPopupWindow(
						GAME_STRING(MAINMENU_STRING_BASE + 65));
					// Got to restart -message
					SetMenuSounds(
						MENU_SOUND_UP | MENU_SOUND_DOWN,
						MENU_SOUND_SELECT);
					SetTransitionSource(nullptr);
					SleepThreadUntil(
						FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));
					GLOBAL(CurrentActivity) = CHECK_ABORT;
					restartGame = true;
					break;
				}

				if (uqm::UQMOptions::read().optDiffChooser == uqm::Difficulty::ChooseYourOwn)
				{
					Flash_pause(pMS->flashContext);
					Flash_setState(pMS->flashContext, FlashState_fadeIn,
								   (3 * GameTicksPerSecond) / 16);
					if (!DoDiffChooser(pMS))
					{
						LastInputTime = GetTimeCounter();			 // if we timed out - don't start second credit roll
						if (GLOBAL(CurrentActivity) != (ACTIVITY)~0) // just declined
						{
							Flash_continue(pMS->flashContext);
						}
						return true;
					}
					Flash_continue(pMS->flashContext);
				}
				LastActivity = CHECK_LOAD | CHECK_RESTART;
				GLOBAL(CurrentActivity) = IN_INTERPLANETARY;
				break;
			case LOAD_SAVED_GAME:
				LastActivity = CHECK_LOAD;
				GLOBAL(CurrentActivity) = IN_INTERPLANETARY;
				optLoadGame = false;
				break;
			case PLAY_SUPER_MELEE:
				uqm::UQMOptions::edit().shipSeed = false;
				ReloadMasterShipList(nullptr);
				GLOBAL(CurrentActivity) = SUPER_MELEE;
				optSuperMelee = false;
				break;
			case SETUP_GAME:
				Flash_pause(pMS->flashContext);
				Flash_setState(pMS->flashContext, FlashState_fadeIn,
							   (3 * GameTicksPerSecond) / 16);

				SetupMenu();

				if (optRequiresReload)
				{
					return false;
				}

				LastInputTime = GetTimeCounter();
				InactTimeOut = (uqm::UQMOptions::read().mainMenuMusic ? 60 : 20) * GameTicksPerSecond;

				SetTransitionSource(nullptr);
				BatchGraphics();
				DrawRestartMenuGraphic(pMS);
				ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, nullptr);
				Flash_UpdateOriginal(pMS->flashContext);
				DrawRestartMenu(pMS, pMS->CurState, nullptr);
				Flash_continue(pMS->flashContext);
				UnbatchGraphics();

				RestartMessage();

				return true;
			case QUIT_GAME:
				SleepThreadUntil(
					FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));
				GLOBAL(CurrentActivity) = CHECK_ABORT;
				break;
		}

		Flash_pause(pMS->flashContext);

		return false;
	}
	else if (PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_DOWN])
	{
		uint8_t NewState;

		NewState = pMS->CurState;
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState == START_NEW_GAME)
			{
				NewState = QUIT_GAME;
			}
			else
			{
				--NewState;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState == QUIT_GAME)
			{
				NewState = START_NEW_GAME;
			}
			else
			{
				++NewState;
			}
		}
		if (NewState != pMS->CurState)
		{
			BatchGraphics();
			DrawRestartMenu(pMS, NewState, nullptr);
			UnbatchGraphics();
			pMS->CurState = NewState;
		}

		LastInputTime = GetTimeCounter();
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_RIGHT])
	{ // Does nothing, but counts as input for timeout purposes
		LastInputTime = GetTimeCounter();
	}
	//else if (MouseButtonDown)
	//{
	//	Flash_pause (pMS->flashContext);
	//	DoPopupWindow (GAME_STRING (MAINMENU_STRING_BASE + 54));
	//			// Mouse not supported message
	//	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);

	//	SetTransitionSource (nullptr);
	//	BatchGraphics ();
	//	DrawRestartMenuGraphic (pMS);
	//	ScreenTransition (3, nullptr);
	//	DrawRestartMenu (pMS, pMS->CurState, nullptr);
	//	Flash_continue (pMS->flashContext);
	//	UnbatchGraphics ();

	//	LastInputTime = GetTimeCounter ();
	//}
	else
	{ // No input received, check if timed out
		if (GetTimeCounter() - LastInputTime > InactTimeOut)
		{
			GLOBAL(CurrentActivity) = (ACTIVITY)~0;
			return false;
		}
	}

	SleepThreadUntil(TimeIn + GameTicksPerSecond / 30);

	return true;
}

static bool
RestartMenu(MENU_STATE* pMS)
{
	TimeCount TimeOut;
	uint16_t i;

	ReinitQueue(&race_q[0]);
	ReinitQueue(&race_q[1]);

	SetContext(ScreenContext);

	GLOBAL(CurrentActivity) |= CHECK_ABORT;
	if (GLOBAL_SIS(CrewEnlisted) == (uint16_t)~0
		&& GET_GAME_STATE(UTWIG_BOMB_ON_SHIP)
		&& !GET_GAME_STATE(UTWIG_BOMB)
		&& DeathBySuicide)
	{ // player blew himself up with Utwig bomb
		SET_GAME_STATE(UTWIG_BOMB_ON_SHIP, 0);

		SleepThreadUntil(FadeScreen(FadeAllToWhite, GameTicksPerSecond / 8)
						 + GameTicksPerSecond / 60);
		SetContextBackGroundColor(WHITE_COLOR);

		ClearDrawable();
		FlushColorXForms();
		TimeOut = GameTicksPerSecond / 8;

		GLOBAL(CurrentActivity) = IN_ENCOUNTER;

		if (uqm::UQMOptions::read().gameOver)
		{
			GameOver(SUICIDE);
		}

		DeathBySuicide = false;

		FreeGameData();
		GLOBAL(CurrentActivity) = CHECK_ABORT;
	}
	else
	{
		TimeOut = GameTicksPerSecond / 2;

		if (GLOBAL_SIS(CrewEnlisted) == (uint16_t)~0)
		{
			GLOBAL(CurrentActivity) = IN_ENCOUNTER;

			if (DeathByMelee)
			{
				if (uqm::UQMOptions::read().gameOver)
				{
					GameOver(DIED_IN_BATTLE);
				}
				DeathByMelee = false;
			}
			else if (DeathBySurrender)
			{
				if (uqm::UQMOptions::read().gameOver)
				{
					GameOver(SURRENDERED);
				}
				DeathBySurrender = false;
			}
		}

		if (lowByte(LastActivity) == WON_LAST_BATTLE)
		{
			GLOBAL(CurrentActivity) = WON_LAST_BATTLE;
			Victory();
			Credits(true);
		}

		FreeGameData();
		GLOBAL(CurrentActivity) = CHECK_ABORT;
	}

	LastActivity = 0;
	NextActivity = 0;

	// TODO: This fade is not always necessary, especially after a splash
	//   screen. It only makes a user wait.
	// Kruzen: This fade is needed when going from SUPER-MELEE and LOAD menus
	// and when Skip Intro option is enabled, 3 second pause goes when
	// the player used the Utwig bomb
	SleepThreadUntil(FadeScreen(FadeAllToBlack, TimeOut));
	if (TimeOut == GameTicksPerSecond / 8)
	{
		SleepThread(GameTicksPerSecond * 3);
	}

	pMS->CurFrame = CaptureDrawable(LoadGraphic(RESTART_PMAP_ANIM));

	DrawRestartMenuGraphic(pMS);
	GLOBAL(CurrentActivity) &= ~CHECK_ABORT;
	SetMenuSounds(MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay();
	DoInput(pMS, true);

	if (!(optRequiresRestart || optRequiresReload))
	{
		UninitMenuMusic();
	}

	Flash_terminate(pMS->flashContext);
	pMS->flashContext = 0;
	DestroyDrawable(ReleaseDrawable(pMS->CurFrame));
	pMS->CurFrame = 0;

	for (i = START_NEW_GAME; i < NUM_MENU_ELEMENTS; i++)
	{
		DestroyDrawable(ReleaseDrawable(TextCache[i]));
		TextCache[i] = 0;
	}

	if (optRequiresReload)
	{
		Reload();
	}

	if (GLOBAL(CurrentActivity) == (ACTIVITY)~0)
	{
		return (false); // timed out
	}

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return (false); // quit
	}

	TimeOut = FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2);

	SleepThreadUntil(TimeOut);
	FlushColorXForms();

	SeedRandomNumbers();

	return (lowByte(GLOBAL(CurrentActivity)) != SUPER_MELEE);
}

static bool
TryStartGame(void)
{
	MENU_STATE MenuState;

	LastActivity = GLOBAL(CurrentActivity);
	GLOBAL(CurrentActivity) = 0;

	memset(&MenuState, 0, sizeof(MenuState));
	MenuState.InputFunc = DoRestart;

	while (!RestartMenu(&MenuState))
	{ // spin until a game is started or loaded
		if (lowByte(GLOBAL(CurrentActivity)) == SUPER_MELEE && !(GLOBAL(CurrentActivity) & CHECK_ABORT))
		{
			FreeGameData();
			Melee();
			MenuState.Initialized = false;
		}
		else if (GLOBAL(CurrentActivity) == (ACTIVITY)~0)
		{ // timed out
			SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));
			return (false);
		}
		else if (GLOBAL(CurrentActivity) & CHECK_ABORT)
		{ // quit
			return (false);
		}
	}

	return true;
}

bool StartGame(void)
{
	do
	{
		while (!TryStartGame())
		{
			if (GLOBAL(CurrentActivity) == (ACTIVITY)~0)
			{ // timed out
				GLOBAL(CurrentActivity) = 0;

				if (optRequiresRestart || optRequiresReload)
				{
					optRequiresRestart = optRequiresReload = false;
				}
				else
				{
					SplashScreen(0);
					if (uqm::UQMOptions::read().whichIntro == uqm::EmulationMode::Console3DO)
					{
						Drumall();
					}
					Credits(false);
				}
			}

			if (GLOBAL(CurrentActivity) & CHECK_ABORT)
			{
				return (false); // quit
			}
		}

		if (LastActivity & CHECK_RESTART)
		{ // starting a new game
			if (!uqm::UQMOptions::read().skipIntro)
			{
				Introduction();
			}
		}

	} while (GLOBAL(CurrentActivity) & CHECK_ABORT);

#ifdef DEBUG_STARSEED
	fmt::print(stderr, "StartGame called for {} mode with seed {} shipseed {}.\n",
			   optSeedType, uqm::UQMOptions::read().customSeed, uqm::UQMOptions::read().shipSeed ? "on" : "off");
#endif
	{
		// We no longer make a global pointer to the static starmap,
		// we make our own global copy in static memory so it behaves
		// the same throughout the code but can be reset as needed.
		//
		// As a reminder, the array has three extra entries beyond
		// NUM_SOLAR_SYSTEMS and NUM_HYPER_VORTICES due to Arilou
		// Quasispace home and the two endpoint dummy systems used by
		// FindStar as a boundary.
		//
		// While the starseed init code should always force a
		// reset of the starmap_array, we will do it here because
		// paranoia is its own reward.
		uint16_t i;
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Initializing star_array, just in case...\n");
#endif
		for (i = 0; i < NUM_SOLAR_SYSTEMS + 1 + NUM_HYPER_VORTICES + 1 + 1; i++)
		{
			star_array[i] = StarmapArray[i];
		}
		Elements = ElementsArray;
		PlanData = PlanetsArray;
		constel_array = (const GFXPOINT*)ConstellationsArray;
	}
	PlayerControl[0] = PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard;
	PlayerControl[1] = PlayerControlFlags::Cyborg | PlayerControlFlags::Psytron | PlayerControlFlags::DifficultyAwesome;

	return (true);
}
