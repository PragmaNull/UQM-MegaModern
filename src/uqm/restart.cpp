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

#include "core/types/enum.h"
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
#include "UQMGame.h"
#include "uqm/session/GameSession.h"
// for StringBank_Create() & SplitString()

enum class MainMenuItem
{
	StartNewGame = 0,
	LoadSavedGame,
	PlaySuperMelee,
	SetupGame,
	QuitGame,
	Count
};
consteval bool isArithmeticEnum(MainMenuItem) { return true; }

enum class DifficultyMenuItem
{
	Easy = 0,
	Original,
	Hard
};
consteval bool isArithmeticEnum(DifficultyMenuItem) { return true; }

static auto ChooserX() { return (CanvasWidth >> 1); }
static auto ChooserY() { return ((CanvasHeight >> 1) - ResScale(12)); }

// Kruzen: Having this ref separated gains more control
// We can load and free it whenever we want and not rely on menu volume
MUSIC_REF menuMusic;

void InitMenuMusic(void)
{
	if (uqm::UQMOptions::read().mainMenuMusic && !(menuMusic))
	{
		FadeMusic(MuteVolume, TimePeriod {});
		menuMusic = loadMainMenuMusic(Rando);
		PlayMusic(menuMusic, true, 1);

		if (OkayToResume())
		{
			SeekMusic(GetMusicPosition());
		}

		FadeMusic(NormalVolume + 70, c_gameSecond * 3);
	}
}

void UninitMenuMusic(void)
{
	if (menuMusic)
	{
		SleepThreadUntil(FadeMusic(MuteVolume, c_gameSecond));

		SetMusicPosition();
		StopMusic();
		DestroyMusic(menuMusic);
		menuMusic = 0;

		FadeMusic(NormalVolume, TimePeriod {});
	}
}

void DrawToolTips(MenuState* pMS, int answer)
{
	GfxText t;
	stringbank* bank = StringBank_Create();
	const char* lines[30];
	GfxRect r;
	GfxStamp s;

	SetContextFont(TinyFont);

	GetFrameRect(SetRelFrameIndex(pMS->CurFrame, 2), &r);
	r.corner.y += ChooserY() + r.extent.height + ResScale(1);

	s.frame = SetRelFrameIndex(pMS->CurFrame, 3);
	r.extent = GetFrameBounds(s.frame);
	r.corner.x = ResScale(
		(ResDescale(CanvasWidth) - ResDescale(r.extent.width)) >> 1);
	s.origin = r.corner;
	DrawStamp(&s);

	SetContextForeGroundColor(BlackColor);

	t.pStr = GameString(GameStringBase::MainmenuString + 66 + answer);
	const int line_count {SplitString(t.pStr, '\n', 30, lines, bank)};

	t.baseline.x = r.corner.x
				 + ResScale(ResDescale(r.extent.width) >> 1);
	t.baseline.y = r.corner.y + ResScale(10)
				 + ResScale(line_count < 2 ? 8 : (line_count > 2 ? 0 : 3));
	for (uint16_t i {0}; i < line_count; i++)
	{
		t.pStr = lines[i];
		t.align = TextAlign::Center;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += ResScale(8);
	}

	StringBank_Free(bank);
}

static void DrawDiffChooser(MenuState* pMS, int answer, bool confirm)
{
	GfxStamp s;
	GfxText t;

	s.origin = MAKE_POINT(ChooserX(), ChooserY());
	s.frame = SetRelFrameIndex(pMS->CurFrame, 2);
	DrawStamp(&s);

	DrawToolTips(pMS, answer);

	FONT oldFont {SetContextFont(MicroFont)};

	t.align = TextAlign::Center;
	t.baseline.x = s.origin.x;
	t.baseline.y = s.origin.y - ResScale(20);

	for (uint16_t i {0}; i <= 2; i++)
	{
		t.pStr = GameString(GameStringBase::MainmenuString + 56
							+ (!i ? 1 : (i > 1 ? 2 : 0)));
		t.CharCount = (uint16_t)uqm::utf8StringCount(t.pStr);

		SetContextForeGroundColor(
			i == answer ?
				(confirm ? MenuBackgroundColor : MenuHighlightColor) :
				BlackColor);
		font_DrawText(&t);

		t.baseline.y += ResScale(23);
	}

	SetContextFont(oldFont);
}

static bool DoDiffChooser(MenuState* pMS)
{
	static TimeCount LastInputTime;
	static TimePeriod InactTimeOut;
	GfxRect oldRect;
	GfxStamp s;
	bool response = false;
	bool done = false;
	uint8_t a = 1;

	InactTimeOut = (uqm::UQMOptions::read().mainMenuMusic ? 60 : 20) * c_gameSecond;
	LastInputTime = GetTimeCounter();

	GFXCONTEXT oldContext {SetContext(ScreenContext)};
	GetContextClipRect(&oldRect);
	s = SaveContextFrame(nullptr);

	DrawDiffChooser(pMS, a, false);

	FlushGraphics();
	FlushInput();

	while (!done)
	{
		UpdateInputState();

		if (uqm::currentActivity() & ActivityFlag::CheckAbort)
		{
			return false;
		}
		else if (PulsedInputState.menu[*MenuKey::MenuSelect])
		{
			done = true;
			response = true;
			DrawDiffChooser(pMS, a, true);
			PlayMenuSound(MenuSoundEffect::Success);
		}
		else if (PulsedInputState.menu[*MenuKey::MenuCancel]
				 || CurrentInputState.menu[*MenuKey::Exit])
		{
			done = true;
			response = false;
			DrawStamp(&s);
		}
		else if (PulsedInputState.menu[*MenuKey::MenuUp] || PulsedInputState.menu[*MenuKey::MenuDown] || PulsedInputState.menu[*MenuKey::MenuLeft] || PulsedInputState.menu[*MenuKey::MenuRight])
		{
			uint8_t NewState {a};
			if (PulsedInputState.menu[*MenuKey::MenuUp]
				|| PulsedInputState.menu[*MenuKey::MenuLeft])
			{
				if (NewState == DifficultyMenuItem::Easy)
				{
					NewState = *DifficultyMenuItem::Hard;
				}
				else
				{
					--NewState;
				}
			}
			else if (PulsedInputState.menu[*MenuKey::MenuDown]
					 || PulsedInputState.menu[*MenuKey::MenuRight])
			{
				if (NewState == DifficultyMenuItem::Hard)
				{
					NewState = *DifficultyMenuItem::Easy;
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

			PlayMenuSound(MenuSoundEffect::Move);

			LastInputTime = GetTimeCounter();
		}
		else if (GetTimeCounter() - LastInputTime > InactTimeOut)
		{ // timed out
			uqm::currentActivity() = (ACTIVITY)~0;
			done = true;
			response = false;
		}

		SleepThread(c_gameSecond / 30);
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

static auto MainTextX() { return (CanvasWidth >> 1); }
static auto MainTextY() { return (ResScale(42) - DosNumScl(20)); }

FRAME TextCache[5];

static void InitPulseText(void)
{
	FRAME frame {};
	FRAME OldFrame {};
	int16_t leading {};
	GfxText t;

	if (TextCache[0] != nullptr)
	{
		return;
	}

	SetContextFont(SlabFont);
	SetContextBackGroundColor(BlackColor);
	SetContextForeGroundColor(WhiteColor);
	GetContextFontLeading(&leading);

	t.baseline.x = MainTextX();
	t.baseline.y = MainTextY();
	t.align = TextAlign::Center;
	t.CharCount = (uint16_t)~0;

	for (uint16_t i {*MainMenuItem::StartNewGame}; i < MainMenuItem::Count; i++)
	{
		t.pStr = GameString(GameStringBase::MainmenuString + 69 + i);

		frame = CaptureDrawable(CreateDrawable(WantPixmap, CanvasWidth,
											   CanvasHeight, 1));
		SetFrameTransparentColor(frame, BlackColor);
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
static void DrawRestartMenuGraphic(MenuState* pMS)
{
	GfxRect r;
	GfxStamp s;
	GfxText t;
	char buf[64];
	int16_t leading {};

	s.frame = pMS->CurFrame;
	GetFrameRect(s.frame, &r);
	s.origin.x = (CanvasWidth - r.extent.width) >> 1;
	s.origin.y = (CanvasHeight - r.extent.height) >> 1;

	SetContextBackGroundColor(BlackColor);
	BatchGraphics();

	ClearDrawable();
	FlushColorXForms();
	DrawStamp(&s);

	s.frame = IncFrameIndex(pMS->CurFrame);
	DrawStamp(&s);

	SetContextFont(SlabFont);

	GetContextFontLeading(&leading);

	t.baseline.x = MainTextX();
	t.baseline.y = MainTextY();
	t.align = TextAlign::Center;
	t.CharCount = (uint16_t)~0;

	SetContextForeGroundColor(MainMenuTextColor);

	for (uint16_t i {*MainMenuItem::StartNewGame}; i < MainMenuItem::Count; i++)
	{
		t.pStr = GameString(GameStringBase::MainmenuString + 69 + i);
		font_DrawText(&t);
		t.baseline.y += leading;
	}

	// Put the version number in the bottom right corner.
	SetContextFont(TinyFont);
	SetContextForeGroundColor(WhiteColor);
	fmt::format_to_sz_n(buf, "v{}.{}.{} {}",
						UqmMajorVersion, UqmMinorVersion, UqmPatchVersion,
						chooseIfHd<const char*>(UqmExtraVersion, UqmExtraVersionHd));
	t.pStr = buf;
	t.baseline.x = CanvasWidth - ResScale(2);
	t.baseline.y = CanvasHeight - ResScale(2);
	t.align = TextAlign::Right;
	font_DrawText(&t);

	// Put the main menu music credit in the bottom left corner.
	if (uqm::UQMOptions::read().mainMenuMusic)
	{
		fmt::format_to_sz_n(buf, "{} {}",
							GameString(GameStringBase::MainmenuString + 61),
							GameString(GameStringBase::MainmenuString + 62 + Rando));
		t.baseline.x = ResScale(2);
		t.baseline.y = CanvasHeight - ResScale(2);
		t.align = TextAlign::Left;
		font_DrawText(&t);
	}

	UnbatchGraphics();
}

static void DrawRestartMenu(MenuState* pMS, uint8_t NewState, FRAME f)
{
	GfxPoint origin;
	origin.x = 0;
	origin.y = 0;
	Flash_setOverlay(pMS->flashContext, &origin, TextCache[NewState], false);

	(void)f; // Silence compiler warnings
}

static bool RestartMessage(void)
{
	if (!optRequiresRestart)
	{
		return false;
	}

	SetFlashRect(nullptr, false);
	DoPopupWindow(GameString(GameStringBase::MainmenuString + 35));
	// Got to restart -message
	SetMenuSounds(MenuSoundUp | MenuSoundDown, MenuSoundSelect);
	SetTransitionSource(nullptr);
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
	uqm::currentActivity() = *ActivityFlag::CheckAbort;
	restartGame = true;
	return true;
}

// The save of --play-save is loaded.
static bool s_playSaveTaken {};

// True if the main menu loads the save of --play-save.
static bool HasPlaySave(void)
{
	return !s_playSaveTaken && !uqm::UQMOptions::read().playSave.empty();
}

const char* TakePlaySave(void)
{
	if (!HasPlaySave())
	{
		return nullptr;
	}
	s_playSaveTaken = true;
	return uqm::UQMOptions::read().playSave.c_str();
}

static bool DoRestart(MenuState* pMS)
{
	static TimeCount LastInputTime;
	static TimePeriod InactTimeOut;
	const TimeCount TimeIn = GetTimeCounter();

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (optWindowType != uqm::WindowStyle::UQM)
	{
		uqm::UQMOptions::edit().meleeToolTips = false;
	}

	if (optSuperMelee && !optLoadGame)
	{
		pMS->CurState = *MainMenuItem::PlaySuperMelee;
		PulsedInputState.menu[*MenuKey::MenuSelect] = 65535;
	}
	else if ((optLoadGame || HasPlaySave()) && !optSuperMelee)
	{
		pMS->CurState = *MainMenuItem::LoadSavedGame;
		PulsedInputState.menu[*MenuKey::MenuSelect] = 65535;
	}

	if (pMS->Initialized && !(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		Flash_process(pMS->flashContext);
	}

	if (!pMS->Initialized)
	{ // Kruzen: too much trouble using this one. Better to just turn it off
		pMS->hMusic = 0;

		InitMenuMusic();
		InitPulseText();
		ResetMusicResume();

		InactTimeOut = (uqm::UQMOptions::read().mainMenuMusic ? 60 : 20) * c_gameSecond;

		pMS->flashContext = Flash_createOverlay(ScreenContext,
												nullptr, nullptr);
		Flash_setMergeFactors(pMS->flashContext, -3, 3, 16);
		Flash_setSpeed(pMS->flashContext, (6 * c_gameSecond) / 14, TimePeriod {},
					   (6 * c_gameSecond) / 14, TimePeriod {});
		Flash_setFrameTime(pMS->flashContext, c_gameSecond / 16);
		Flash_setState(pMS->flashContext, FlashState::FadeIn,
					   (3 * c_gameSecond) / 16);
		Flash_setPulseBox(pMS->flashContext, false);

		DrawRestartMenu(pMS, pMS->CurState, nullptr);
		Flash_start(pMS->flashContext);

		LastInputTime = GetTimeCounter();
		pMS->Initialized = true;

		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToColor, c_gameSecond / 2));
	}
	else if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		SleepThreadUntil(
			FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
		return false;
	}
	else if (PulsedInputState.menu[*MenuKey::MenuSelect])
	{
		switch (pMS->CurState)
		{
			case *MainMenuItem::StartNewGame:
				if (uqm::UQMOptions::read().customSeed == 404)
				{
					SetFlashRect(nullptr, false);
					DoPopupWindow(
						GameString(GameStringBase::MainmenuString + 65));
					// Got to restart -message
					SetMenuSounds(
						MenuSoundUp | MenuSoundDown,
						MenuSoundSelect);
					SetTransitionSource(nullptr);
					SleepThreadUntil(
						FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
					uqm::currentActivity() = *ActivityFlag::CheckAbort;
					restartGame = true;
					break;
				}

				if (uqm::UQMOptions::read().optDiffChooser == uqm::Difficulty::ChooseYourOwn)
				{
					Flash_pause(pMS->flashContext);
					Flash_setState(pMS->flashContext, FlashState::FadeIn,
								   (3 * c_gameSecond) / 16);
					if (!DoDiffChooser(pMS))
					{
						LastInputTime = GetTimeCounter();			// if we timed out - don't start second credit roll
						if (uqm::currentActivity() != (ACTIVITY)~0) // just declined
						{
							Flash_continue(pMS->flashContext);
						}
						return true;
					}
					Flash_continue(pMS->flashContext);
				}

				Flash_pause(pMS->flashContext);
				Flash_setState(pMS->flashContext, FlashState::FadeIn,
							   (3 * c_gameSecond) / 16);
				if (const uqstl::optional<uqm::GameSeed> seed {NewGameMenu()})
				{
					uqm::UQMGame::getInstance().setNewGameSeed(*seed);
				}
				else
				{
					LastInputTime = GetTimeCounter();
					SetTransitionSource(nullptr);
					BatchGraphics();
					DrawRestartMenuGraphic(pMS);
					ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, nullptr);
					Flash_UpdateOriginal(pMS->flashContext);
					DrawRestartMenu(pMS, pMS->CurState, nullptr);
					Flash_continue(pMS->flashContext);
					UnbatchGraphics();
					return true;
				}

				LastActivity = *(ActivityFlag::CheckLoad | ActivityFlag::CheckRestart);
				uqm::currentActivity() = *ActivityKind::InInterplanetary;
				break;
			case *MainMenuItem::LoadSavedGame:
				LastActivity = *ActivityFlag::CheckLoad;
				uqm::currentActivity() = *ActivityKind::InInterplanetary;
				optLoadGame = false;
				break;
			case *MainMenuItem::PlaySuperMelee:
				ReloadMasterShipList(nullptr);
				uqm::currentActivity() = *ActivityKind::SuperMelee;
				optSuperMelee = false;
				break;
			case *MainMenuItem::SetupGame:
				Flash_pause(pMS->flashContext);
				Flash_setState(pMS->flashContext, FlashState::FadeIn,
							   (3 * c_gameSecond) / 16);

				SetupMenu();

				if (optRequiresReload)
				{
					return false;
				}

				LastInputTime = GetTimeCounter();
				InactTimeOut = (uqm::UQMOptions::read().mainMenuMusic ? 60 : 20) * c_gameSecond;

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
			case *MainMenuItem::QuitGame:
				SleepThreadUntil(
					FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
				uqm::currentActivity() = *ActivityFlag::CheckAbort;
				break;
		}

		Flash_pause(pMS->flashContext);

		return false;
	}
	else if (PulsedInputState.menu[*MenuKey::MenuUp] || PulsedInputState.menu[*MenuKey::MenuDown])
	{
		uint8_t NewState {pMS->CurState};
		if (PulsedInputState.menu[*MenuKey::MenuUp])
		{
			if (NewState == MainMenuItem::StartNewGame)
			{
				NewState = *MainMenuItem::QuitGame;
			}
			else
			{
				--NewState;
			}
		}
		else if (PulsedInputState.menu[*MenuKey::MenuDown])
		{
			if (NewState == MainMenuItem::QuitGame)
			{
				NewState = *MainMenuItem::StartNewGame;
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
	else if (PulsedInputState.menu[*MenuKey::MenuLeft] || PulsedInputState.menu[*MenuKey::MenuRight])
	{ // Does nothing, but counts as input for timeout purposes
		LastInputTime = GetTimeCounter();
	}
	//else if (MouseButtonDown)
	//{
	//	Flash_pause (pMS->flashContext);
	//	DoPopupWindow (GameString (MAINMENU_STRING_BASE + 54));
	//			// Mouse not supported message
	//	SetMenuSounds (MenuSoundUp | MenuSoundDown, MenuSoundSelect);

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
			uqm::currentActivity() = (ACTIVITY)~0;
			return false;
		}
	}

	SleepThreadUntil(TimeIn + c_gameSecond / 30);

	return true;
}

void ShowGameEnding(void)
{
	const ACTIVITY lastActivity {uqm::currentActivity()};

	SetContext(ScreenContext);

	uqm::currentActivity() |= *ActivityFlag::CheckAbort;
	if (uqm::gameState().sis().CrewEnlisted == (uint16_t)~0
		&& uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP)
		&& !uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB)
		&& DeathBySuicide)
	{ // player blew himself up with Utwig bomb
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP, static_cast<uint32_t>(0));

		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToWhite, c_gameSecond / 8)
						 + c_gameSecond / 60);
		SetContextBackGroundColor(WhiteColor);

		ClearDrawable();
		FlushColorXForms();

		uqm::currentActivity() = *ActivityKind::InEncounter;

		if (uqm::UQMOptions::read().gameOver)
		{
			GameOver(EndgameScenario::Suicide);
		}

		DeathBySuicide = false;

		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 8));
		SleepThread(c_gameSecond * 3);
	}
	else
	{
		if (uqm::gameState().sis().CrewEnlisted == (uint16_t)~0)
		{
			uqm::currentActivity() = *ActivityKind::InEncounter;

			if (DeathByMelee)
			{
				if (uqm::UQMOptions::read().gameOver)
				{
					GameOver(EndgameScenario::DiedInBattle);
				}
				DeathByMelee = false;
			}
			else if (DeathBySurrender)
			{
				if (uqm::UQMOptions::read().gameOver)
				{
					GameOver(EndgameScenario::Surrendered);
				}
				DeathBySurrender = false;
			}
		}

		if (uqm::activityKind(lastActivity) == ActivityKind::WonLastBattle)
		{
			uqm::currentActivity() = *ActivityKind::WonLastBattle;
			Victory();
			Credits(true);
		}
	}
	uqm::currentActivity() = *ActivityFlag::CheckAbort;
}

static bool RestartMenu(MenuState* pMS)
{
	ReinitQueue(&race_q[0]);
	ReinitQueue(&race_q[1]);

	SetContext(ScreenContext);

	FreeGameData();
	uqm::currentActivity() = *ActivityFlag::CheckAbort;

	LastActivity = 0;
	NextActivity = 0;

	// TODO: This fade is not always necessary, especially after a splash
	//   screen. It only makes a user wait.
	// Kruzen: This fade is needed when going from SUPER-MELEE and LOAD menus
	// and when Skip Intro option is enabled
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));

	pMS->CurFrame = CaptureDrawable(LoadGraphic(RestartPmapAnimRes));

	DrawRestartMenuGraphic(pMS);
	uqm::currentActivity() &= ~ActivityFlag::CheckAbort;
	SetMenuSounds(MenuSoundUp | MenuSoundDown, MenuSoundSelect);
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

	for (uint16_t i {*MainMenuItem::StartNewGame}; i < MainMenuItem::Count; i++)
	{
		DestroyDrawable(ReleaseDrawable(TextCache[i]));
		TextCache[i] = 0;
	}

	if (optRequiresReload)
	{
		Reload();
	}

	if (uqm::currentActivity() == (ACTIVITY)~0)
	{
		return (false); // timed out
	}

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return (false); // quit
	}

	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
	FlushColorXForms();

	return (uqm::activityKind(uqm::currentActivity()) != ActivityKind::SuperMelee);
}

static bool TryStartGame(void)
{
	MenuState menuState;

	LastActivity = uqm::currentActivity();
	uqm::currentActivity() = 0;

	memset(&menuState, 0, sizeof(menuState));
	menuState.InputFunc = DoRestart;

	while (!RestartMenu(&menuState))
	{ // spin until a game is started or loaded
		if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::SuperMelee && !(uqm::currentActivity() & ActivityFlag::CheckAbort))
		{
			FreeGameData();
			uqm::UQMGame::getInstance().beginSession(uqm::SessionKind::SuperMelee);
			Melee();
			uqm::UQMGame::getInstance().endSession();
			menuState.Initialized = false;
		}
		else if (uqm::currentActivity() == (ACTIVITY)~0)
		{ // timed out
			SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
			return (false);
		}
		else if (uqm::currentActivity() & ActivityFlag::CheckAbort)
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
			if (uqm::currentActivity() == (ACTIVITY)~0)
			{ // timed out
				uqm::currentActivity() = 0;

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

			if (uqm::currentActivity() & ActivityFlag::CheckAbort)
			{
				return (false); // quit
			}
		}

		if (LastActivity & ActivityFlag::CheckRestart)
		{ // starting a new game
			if (!uqm::UQMOptions::read().skipIntro)
			{
				Introduction();
			}
		}

	} while (uqm::currentActivity() & ActivityFlag::CheckAbort);

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
		// NumSolarSystems and NumHyperVortices due to Arilou
		// Quasispace home and the two endpoint dummy systems used by
		// FindStar as a boundary.
		//
		// While the starseed init code should always force a
		// reset of the starmap_array, we will do it here because
		// paranoia is its own reward.
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Initializing star_array, just in case...\n");
#endif
		for (uint16_t i {0}; i < NumSolarSystems + 1 + NumHyperVortices + 1 + 1; i++)
		{
			star_array[i] = StarmapArray[i];
		}
		Elements = ElementsArray;
		PlanData = PlanetsArray;
		constel_array = (const GfxPoint*)ConstellationsArray;
	}
	PlayerControl[0] = PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard;
	PlayerControl[1] = PlayerControlFlags::Cyborg | PlayerControlFlags::Psytron | PlayerControlFlags::DifficultyAwesome;

	return (true);
}
