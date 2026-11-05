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

#include "gameopt.h"

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "starmap.h"
#include "menustat.h"
#include "sis.h"
#include "units.h"
#include "gamestr.h"
#include "options.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "util.h"
#include "libs/graphics/gfx_common.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "comm.h"
#include "master.h"
#include "restart.h"

#include <ctype.h>
#include "uqm/random/UQMRandom.h"

extern FRAME PlayFrame;

static constexpr auto MaxSavedGames {50};
static auto SavesPerPage() { return (optWindowType != uqm::WindowStyle::UQM ? 2 : 5); }


static auto SummaryXOffs() { return NsafeNumScl(14); }
static auto SummarySideOffs() { return NsafeNumScl(7); }

static uint16_t lastUsedSlot;

static NamingCallback* namingCB;

bool NewGameInit;
uint8_t OutfitOrShipyard = 0;
bool SaveOrLoad = false;
bool TextEntry3DO = false;

void ConfirmSaveLoad(STAMP* MsgStamp)
{
	GFXRECT r;
	GFXRECT clip_r;
	TEXT t;

	SetContextFont(StarConFont);
	GetContextClipRect(&clip_r);

	t.baseline.x = clip_r.extent.width >> 1;
	t.baseline.y = (clip_r.extent.height >> 1) + ResScale(3);
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	if (MsgStamp)
	{
		t.pStr = GameString(SAVEGAME_STRING_BASE + 0);
	}
	// "Saving . . ."
	else
	{
		t.pStr = GameString(SAVEGAME_STRING_BASE + 1);
	}
	// "Loading . . ."
	TextRect(&t, &r, nullptr);
	r.corner.x -= ResScale(4);
	r.corner.y -= ResScale(4);
	r.extent.width += ResScale(8);
	r.extent.height += ResScale(8);
	if (MsgStamp)
	{
		*MsgStamp = SaveContextFrame(&r);
	}
	DrawStarConBox(&r, ResScale(2), ShadowboxMediumColor,
				   ShadowboxDarkColor, true, DkgrayColor, true, Transparent);
	SetContextForeGroundColor(
		isPC(uqm::UQMOptions::read().whichFonts) ? WhiteColor : LtgrayColor);
	font_DrawText(&t);
}

enum
{
	SAVE_GAME = 0,
	LOAD_GAME,
	QUIT_GAME,
	SETTINGS,
	EXIT_GAME_MENU,
};

enum
{
	SOUND_ON_SETTING,
	SOUND_OFF_SETTING,
	MUSIC_ON_SETTING,
	MUSIC_OFF_SETTING,
	CYBORG_OFF_SETTING,
	CYBORG_NORMAL_SETTING,
	CYBORG_DOUBLE_SETTING,
	CYBORG_SUPER_SETTING,
	READ_VERY_SLOW_SETTING,
	READ_SLOW_SETTING,
	READ_MODERATE_SETTING,
	READ_FAST_SETTING,
	READ_VERY_FAST_SETTING,
	CHANGE_CAPTAIN_SETTING,
	CHANGE_SHIP_SETTING,
	EXIT_SETTINGS_MENU,
};

// Shows the seed of the game in the message field at the top of the screen.
static void
DrawSeedMessage(void)
{
	const uqm::GameSeed& seed {uqm::currentSeed()};
	const char* label {"Original"};
	switch (seed.type)
	{
		case uqm::SeedType::Original:
			break;
		case uqm::SeedType::Planet:
			label = "Planets";
			break;
		case uqm::SeedType::MRQ:
			label = "MRQ";
			break;
		case uqm::SeedType::Starseed:
			label = "Starseed";
			break;
	}

	char buf[64] {};
	if (seed.type == uqm::SeedType::Original)
	{
		fmt::format_to_sz_n(buf, "Seed: {}", label);
	}
	else
	{
		fmt::format_to_sz_n(buf, "Seed: {} {}{}", label, seed.value, seed.ships ? " +Ships" : "");
	}
	DrawSISMessage(buf);
}

// Draws the message field at the top of the screen as it is outside the game options.
static void
RestoreSISMessage(bool inStarbase, uint8_t outfitOrShipyard)
{
	if (inStarbase && outfitOrShipyard > 1)
	{
		DrawSISMessage(GameString(STARBASE_STRING_BASE + outfitOrShipyard));
	}
	else
	{
		DrawSISMessage(nullptr);
	}
}

static void
FeedbackSetting(uint8_t which_setting)
{
	char buf[128] {};
	const char* tmpstr {};

	switch (which_setting)
	{
		case SOUND_ON_SETTING:
		case SOUND_OFF_SETTING:
			fmt::format_to_sz_n(buf, "{} {}",
								GameString(OPTION_STRING_BASE + 0),
								uqm::globalFlags() & SoundDisabled ? GameString(OPTION_STRING_BASE + 3) :
																	 GameString(OPTION_STRING_BASE + 4));
			break;
		case MUSIC_ON_SETTING:
		case MUSIC_OFF_SETTING:
			fmt::format_to_sz_n(buf, "{} {}",
								GameString(OPTION_STRING_BASE + 1),
								uqm::globalFlags() & MusicDisabled ? GameString(OPTION_STRING_BASE + 3) :
																	 GameString(OPTION_STRING_BASE + 4));
			break;
		case CYBORG_OFF_SETTING:
		case CYBORG_NORMAL_SETTING:
		case CYBORG_DOUBLE_SETTING:
		case CYBORG_SUPER_SETTING:
			if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC && which_setting > CYBORG_NORMAL_SETTING)
			{
				if (which_setting == CYBORG_DOUBLE_SETTING)
				{
					tmpstr = "+";
				}
				else
				{
					tmpstr = "++";
				}
			}
			else
			{
				tmpstr = "";
			}
			fmt::format_to_sz_n(buf, "{} {}{}",
								GameString(OPTION_STRING_BASE + 2),
								!(uqm::globalFlags() & CyborgEnabled) ? GameString(OPTION_STRING_BASE + 3) :
																		GameString(OPTION_STRING_BASE + 4),
								tmpstr);
			break;
		case READ_VERY_SLOW_SETTING:
		case READ_SLOW_SETTING:
		case READ_MODERATE_SETTING:
		case READ_FAST_SETTING:
		case READ_VERY_FAST_SETTING:
			fmt::format_to_sz_n(buf, "{}",
								GameString(OPTION_STRING_BASE + 5
										   + (uqm::globalFlags() & ReadSpeedMask)));
			break;
		case CHANGE_CAPTAIN_SETTING:
		case CHANGE_SHIP_SETTING:
			uqm::strncpy_safe(buf, GameString(
									   NAMING_STRING_BASE + (is3DO(uqm::UQMOptions::read().whichFonts) ? 7 : 0)));
			break;
	}

	DrawStatusMessage(buf);
}

static constexpr auto DdshsNormal {0};
static constexpr auto DdshsEdit {1};
static constexpr auto DdshsBlockcur {2};

static bool
DrawNameString(bool nameCaptain, char* Str, uint16_t CursorPos,
			   uint16_t state)
{
	GFXRECT r;
	TEXT lf;
	Color BackGround;
	Color ForeGround;
	FONT Font {};

	{
		r.corner.x = ResScale(2);
		r.extent.width = ShipNameWidth();
		r.extent.height = ShipNameHeight();

		if (nameCaptain)
		{ // Naming the captain
			if (isPC(uqm::UQMOptions::read().whichFonts))
			{
				Font = TinyFont;
			}
			else
			{
				Font = TinyFontBold;
			}

			r.corner.y = ResScale(10);
			r.corner.x += ResScale(1);
			r.extent.width -= ResScale(2);
			lf.baseline.x =
				r.corner.x + (r.extent.width >> 1) - ResScale(1);

			BackGround = BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09);
			ForeGround = BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B);
		}
		else
		{ // Naming the flagship
			Font = StarConFont;
			r.corner.y = ResScale(20);
			lf.baseline.x = r.corner.x + (r.extent.width >> 1);

			BackGround = BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2D);
			ForeGround = BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7D);
		}
		lf.baseline.y = r.corner.y + r.extent.height - ResScale(1);
		lf.align = ALIGN_CENTER;
	}

	SetContext(StatusContext);
	SetContextFont(Font);
	lf.pStr = Str;
	lf.CharCount = (uint16_t)~0;

	if (!(state & DdshsEdit))
	{ // normal state
		if (nameCaptain)
		{
			DrawCaptainsName(nameCaptain);
		}
		else
		{
			DrawFlagshipName(true, !nameCaptain);
		}
	}
	else
	{ // editing state
		GFXRECT text_r;
		uint8_t char_deltas[SisNameSize];

		TextRect(&lf, &text_r, char_deltas);

		if ((text_r.extent.width + ResScale(2)) >= r.extent.width)
		{ // the text does not fit the input box size and so
			// will not fit when displayed later
			// disallow the change
			return (false);
		}

		SetContextForeGroundColor(BackGround);
		DrawFilledRectangle(&r);

		if (uqm::UQMOptions::read().customBorder)
		{
			DrawBorder(SIS_STAT_REPAIR_FRAME);
		}

		const uint8_t* pchar_deltas {char_deltas};
		for (uint16_t i {CursorPos}; i > 0; --i)
		{
			text_r.corner.x += *pchar_deltas++;
		}
		if (CursorPos < lf.CharCount) /* end of line */
		{
			text_r.corner.x -= ResScale(1);
		}

		if (state & DdshsBlockcur)
		{ // Use block cursor for keyboardless systems

			text_r.corner.y = r.corner.y;
			text_r.extent.height = r.extent.height;

			SetCursorFlashBlock(true);

			if (CursorPos == lf.CharCount)
			{ // cursor at end-line -- use insertion point
				text_r.extent.width = ResScale(1);
				text_r.corner.x -= IfHd(3);
			}
			else if (CursorPos + 1 == lf.CharCount)
			{ // extra pixel for last char margin
				text_r.extent.width = (int16_t)*pchar_deltas - IfHd(3);
				text_r.corner.x += ResScale(1);
			}
			else
			{ // normal mid-line char
				text_r.extent.width = (int16_t)*pchar_deltas;
				text_r.corner.x += ResScale(1);
			}

			if (text_r.extent.width >= 200)
			{
				text_r.extent.width = ResScale(1);
				text_r.corner.x -= IfHd(3);
			}
			else
			{
				SetContextForeGroundColor(BlackColor);
				DrawFilledRectangle(&text_r);
			}
		}
		else
		{ // Insertion point cursor
			text_r.corner.y = r.corner.y + ResScale(1);
			text_r.extent.height = r.extent.height - ResScale(2);
			text_r.extent.width = ResScale(1);

			if (CursorPos == lf.CharCount)
			{
				text_r.corner.x -= IfHd(3);
			}

			SetCursorFlashBlock(false);
		}

		SetCursorRect(&text_r, StatusContext);

		SetContextForeGroundColor(ForeGround);
		font_DrawText(&lf);
	}

	return (true);
}

static bool
OnNameChange(TEXTENTRY_STATE* pTES)
{
	const bool nameCaptain = (bool)pTES->CbParam;
	uint16_t hl = DdshsEdit;

	if (pTES->JoystickMode)
	{
		hl |= DdshsBlockcur;
	}

	return DrawNameString(nameCaptain, pTES->BaseStr, pTES->CursorPos, hl);
}

static void
NameCaptainOrShip(bool nameCaptain, bool gamestart)
{
	char buf[SisNameSize] = "";
	TEXTENTRY_STATE tes;
	char* Setting {};
	uint16_t CursPos = 0;

	SetContext(StatusContext);
	DrawNameString(nameCaptain, buf, CursPos, DdshsEdit);

	DrawStatusMessage(
		GameString(
			NAMING_STRING_BASE + (is3DO(uqm::UQMOptions::read().whichFonts) ? 7 : 0)));

	if (nameCaptain)
	{
		Setting = uqm::gameState().sis().CommanderName;
		tes.MaxSize = sizeof(uqm::gameState().sis().CommanderName);
		TextEntry3DO = (bool)is3DO(uqm::UQMOptions::read().whichFonts);
	}
	else
	{
		Setting = uqm::gameState().sis().ShipName;
		tes.MaxSize = sizeof(uqm::gameState().sis().ShipName);
		TextEntry3DO = false;
	}

	// text entry setup
	tes.Initialized = false;
	tes.BaseStr = buf;
	tes.CursorPos = CursPos;
	tes.CbParam = (void*)nameCaptain;
	tes.ChangeCallback = OnNameChange;
	tes.FrameCallback = 0;

	if (DoTextEntry(&tes))
	{
		uqm::utf8StringCopy(Setting, tes.MaxSize, buf);
	}
	else
	{
		uqm::utf8StringCopy(buf, sizeof(buf), Setting);
	}

	// If the Captain and/or Flagship text entries are blank
	// at New Game, fill them in with the default names.
	if (gamestart && (Setting != nullptr) && (Setting[0] == '\0'))
	{
		uqm::strncpy_safe({Setting, strlen(Setting)}, GameString // Zelnick & Vindicator
						  (NAMING_STRING_BASE + 2 + nameCaptain));
		CursPos = (uint16_t)strlen(GameString(NAMING_STRING_BASE + 2 + nameCaptain));
	}

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		NewGameInit = false;
	}

	FlushCursorRect();

	SetFlashRect(SfrMenu3do(), false);

	DrawNameString(nameCaptain, buf, CursPos, DdshsNormal);

	DrawBorder(SIS_STAT_REPAIR_FRAME);

	TextEntry3DO = false;

	if (namingCB)
	{
		namingCB();
	}
}

static bool
DrawSaveNameString(char* Str, uint16_t CursorPos, uint16_t state, uint16_t gameIndex)
{
	GFXRECT r;
	TEXT lf;
	Color BackGround;
	Color ForeGround;
	char fullStr[256] {};
	char dateStr[80] {};

	DateToString(dateStr, sizeof dateStr, uqm::gameState().clock().month(),
				 uqm::gameState().clock().day(), uqm::gameState().clock().year());
	fmt::format_to_sz_n(fullStr, "{}: {}", dateStr, Str);

	SetContextForeGroundColor(SaveSelectedColor);
	r.extent.width = ResScale(15);
	if (MaxSavedGames > 99)
	{
		r.extent.width += ResScale(5);
	}
	r.extent.height = ResScale(11);
	r.corner.x = ResScale(8);
	r.corner.y = ResScale(160 + ((gameIndex % SavesPerPage()) * 13));
	DrawRectangle(&r, IsHd());

	r.extent.width = ResScale(204) + (SisScreenWidth() - ResScale(242));
	r.corner.x = ResScale(30);
	DrawRectangle(&r, IsHd());

	FONT Font {TinyFont};
	lf.baseline.x = r.corner.x + ResScale(3);
	lf.baseline.y = r.corner.y + ResScale(8);

	BackGround = SaveSelectedColor;
	ForeGround = SaveUnselectedColor;

	lf.align = ALIGN_LEFT;

	SetContextFont(Font);
	lf.pStr = fullStr;
	lf.CharCount = (uint16_t)~0;

	if (!(state & DdshsEdit))
	{
		TEXT t;

		SetContextForeGroundColor(BlackColor);
		DrawFilledRectangle(&r);

		t.baseline.x = r.corner.x + ResScale(3);
		t.baseline.y = r.corner.y + ResScale(8);
		t.align = ALIGN_LEFT;
		t.pStr = Str;
		t.CharCount = (uint16_t)~0;
		SetContextForeGroundColor(CaptainNameTextColor);
		font_DrawText(&t);
	}
	else
	{ // editing state
		uint16_t FullCursorPos {};
		GFXRECT text_r;
		uint8_t char_deltas[256];

		TextRect(&lf, &text_r, char_deltas);
		if ((text_r.extent.width + ResScale(2)) >= r.extent.width)
		{ // the text does not fit the input box size and so
			// will not fit when displayed later
			// disallow the change
			return (false);
		}

		PreUpdateFlashRect();

		SetContextForeGroundColor(BackGround);
		DrawFilledRectangle(&r);

		const uint8_t* pchar_deltas {char_deltas};

		FullCursorPos = CursorPos + (uint16_t)uqm::utf8StringCount(dateStr);
		for (uint16_t i {FullCursorPos}; i > 0; --i)
		{
			text_r.corner.x += *pchar_deltas++;
		}

		if (FullCursorPos < lf.CharCount) /* end of line */
		{
			text_r.corner.x -= ResScale(1);
		}

		if (state & DdshsBlockcur)
		{ // Use block cursor for keyboardless systems
			if (FullCursorPos == lf.CharCount)
			{ // cursor at end-line -- use insertion point
				text_r.extent.width = ResScale(1);
			}
			else if (FullCursorPos + 1 == lf.CharCount)
			{ // extra pixel for last char margin
				text_r.extent.width = (int16_t)*pchar_deltas + ResScale(2);
			}
			else
			{ // normal mid-line char
				text_r.extent.width = (int16_t)*pchar_deltas + ResScale(1);
			}
		}
		else
		{ // Insertion point cursor
			text_r.extent.width = ResScale(1);
		}

		text_r.corner.y = r.corner.y;
		text_r.extent.height = r.extent.height;
		SetContextForeGroundColor(BlackColor);
		DrawFilledRectangle(&text_r);

		SetContextForeGroundColor(ForeGround);
		font_DrawText(&lf);
		PostUpdateFlashRect();
	}

	return (true);
}

static bool
OnSaveNameChange(TEXTENTRY_STATE* pTES)
{
	uint16_t hl = DdshsEdit;
	const uint16_t* gameIndex = (uint16_t*)pTES->CbParam;

	if (pTES->JoystickMode)
	{
		hl |= DdshsBlockcur;
	}

	return DrawSaveNameString(pTES->BaseStr, pTES->CursorPos, hl, *gameIndex);
}

static bool
NameSaveGame(uint16_t gameIndex, char* buf)
{
	TEXTENTRY_STATE tes;
	const uint16_t CursPos = (uint16_t)strlen(buf);
	uint16_t gIndex {gameIndex};
	GFXRECT r;

	DrawSaveNameString(buf, CursPos, DdshsEdit, gameIndex);

	tes.MaxSize = SaveNameSize;

	// text entry setup
	tes.Initialized = false;
	tes.BaseStr = buf;
	tes.CursorPos = CursPos;
	tes.CbParam = &gIndex;
	tes.ChangeCallback = OnSaveNameChange;
	tes.FrameCallback = 0;
	r.extent.width = ResScale(204)
				   + (SisScreenWidth() - ResScale(242));
	r.extent.height = ResScale(11);
	r.corner.x = ResScale(30);
	r.corner.y = (ResScale(160) + ((gameIndex % SavesPerPage()) * ResScale(13)));
	SetFlashRect(&r, false);

	if (!DoTextEntry(&tes))
	{
		buf[0] = 0;
	}

	SetFlashRect(nullptr, false);

	DrawSaveNameString(buf, CursPos, DdshsNormal, gameIndex);

	if (namingCB)
	{
		namingCB();
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	SetFlashRect(nullptr, false);

	if (tes.Success)
	{
		return (true);
	}
	else
	{
		return (false);
	}
}

void SetNamingCallback(NamingCallback* callback)
{
	namingCB = callback;
}

static bool
DoSettings(MENU_STATE* pMS)
{
	uint8_t cur_speed {};
	uint8_t read_speed {};
	static uint8_t i = 0;

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		i = 0;
		return false;
	}

	cur_speed = (uqm::globalFlags() & CombatSpeedMask)
			 >> CombatSpeedShift;
	read_speed = (uqm::globalFlags() & ReadSpeedMask);

	if (NewGameInit)
	{
		i++;
		pMS->CurState =
			i > 1 ? CHANGE_SHIP_SETTING : CHANGE_CAPTAIN_SETTING;
		PulsedInputState.menu[KEY_MENU_SELECT] = 65535;
	}

	if (PulsedInputState.menu[KEY_MENU_CANCEL]
		|| (PulsedInputState.menu[KEY_MENU_SELECT]
			&& pMS->CurState == EXIT_SETTINGS_MENU))
	{
		return false;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case SOUND_ON_SETTING:
			case SOUND_OFF_SETTING:
				ToggleSoundEffect();
				pMS->CurState ^= 1;
				DrawMenuStateStrings(PM_SOUND_ON, pMS->CurState);
				break;
			case MUSIC_ON_SETTING:
			case MUSIC_OFF_SETTING:
				ToggleMusic();
				pMS->CurState ^= 1;
				DrawMenuStateStrings(PM_SOUND_ON, pMS->CurState);
				break;
			case CHANGE_CAPTAIN_SETTING:
			case CHANGE_SHIP_SETTING:
				SetFlashRect(nullptr, false);
				DrawMenuStateStrings(PM_SOUND_ON, pMS->CurState);
				NameCaptainOrShip(pMS->CurState == CHANGE_CAPTAIN_SETTING,
								  NewGameInit);
				break;
			case CYBORG_OFF_SETTING:
			case CYBORG_NORMAL_SETTING:
			case CYBORG_DOUBLE_SETTING:
			case CYBORG_SUPER_SETTING:
				if (cur_speed++ < NumCombatSpeeds - 1)
				{
					uqm::globalFlags() |= CyborgEnabled;
				}
				else
				{
					cur_speed = 0;
					uqm::globalFlags() &= ~CyborgEnabled;
				}
				uqm::globalFlags() =
					((uqm::globalFlags() & ~CombatSpeedMask)
					 | (cur_speed << CombatSpeedShift));
				pMS->CurState = CYBORG_OFF_SETTING + cur_speed;
				DrawMenuStateStrings(PM_SOUND_ON, pMS->CurState);
				break;
			case READ_VERY_SLOW_SETTING:
			case READ_SLOW_SETTING:
			case READ_MODERATE_SETTING:
			case READ_FAST_SETTING:
			case READ_VERY_FAST_SETTING:
				if (read_speed == NumReadSpeeds - 1)
				{
					read_speed = 0;
				}
				else
				{
					read_speed++;
				}

				uqm::globalFlags() =
					((uqm::globalFlags() & ~ReadSpeedMask)
					 | read_speed);
				pMS->CurState = READ_VERY_SLOW_SETTING + read_speed;
				DrawMenuStateStrings(PM_SOUND_ON, pMS->CurState);
				break;
			default:
				DrawMenuStateStrings(PM_SOUND_ON, pMS->CurState);
		}
		if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
		{
			DrawMenuStateStrings(PM_SOUND_ON, pMS->CurState);
		}

		FeedbackSetting(pMS->CurState);
	}
	else if (DoMenuChooser(pMS, PM_SOUND_ON))
	{
		FeedbackSetting(pMS->CurState);
	}

	if (NewGameInit)
	{
		if (i == 1)
		{
			SettingsMenu(true);
			return false;
		}
		else
		{
			NewGameInit = false;
			i = 0;
		}
	}

	return true;
}

void SettingsMenu(bool NameFlagship)
{
	MENU_STATE MenuState;

	memset(&MenuState, 0, sizeof MenuState);

	if (NewGameInit)
	{
		MenuState.CurState = !NameFlagship ?
								 CHANGE_CAPTAIN_SETTING :
								 CHANGE_SHIP_SETTING;
	}
	else
	{
		MenuState.CurState = SOUND_ON_SETTING;
	}

	DrawMenuStateStrings(PM_SOUND_ON, MenuState.CurState);
	FeedbackSetting(MenuState.CurState);

	MenuState.InputFunc = DoSettings;
	DoInput(&MenuState, false);

	DrawStatusMessage(nullptr);
}

typedef struct
{
	SUMMARY_DESC summary[MaxSavedGames];
	bool saving;
	// true when saving, false when loading
	bool success;
	// true when load/save succeeded
	FRAME SummaryFrame;

} PICK_GAME_STATE;

static void
DrawLines(GFXPOINT pt, int16_t width)
{
	GFXRECT r;

	// Top Bar
	r.corner = pt;
	r.extent.width = width;
	r.extent.height = ResScale(1);
	SetContextForeGroundColor(LabelLineTopColor);
	DrawFilledRectangle(&r);

	// Bottom Bar
	r.corner.y += ResScale(4);
	SetContextForeGroundColor(LabelLineBotColor);
	DrawFilledRectangle(&r);

	// Middle Bar
	r.corner.y -= ResScale(2);
	r.corner.x += ResScale(1);
	r.extent.width -= ResScale(2);
	SetContextForeGroundColor(LabelLineMidColor);
	DrawFilledRectangle(&r);
}

static GFXPOINT
SCL_POINT(GFXPOINT pt)
{
	GFXPOINT point;

	point.x = ResScale(pt.x);
	point.y = ResScale(pt.y);
	return point;
}

static void
DrawLabel(GFXPOINT pt, int16_t width, uint32_t gamestr)
{
	TEXT t;
	char buf[256];
	int16_t leading {};
	Color OldColor;

	DrawLines(SCL_POINT(pt), ResScale(width));

	uqm::utf8StringCopy((char*)buf, sizeof(buf), GameString(gamestr));

	FONT OldFont {SetContextFont(LabelFont)};
	OldColor = SetContextForeGroundColor(SummTextColor);

	GetContextFontLeading(&leading);

	t.baseline.x = ResScale((width >> 1) + pt.x);
	t.baseline.y = ResScale(pt.y + 5);
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	t.pStr = AlignText((const char*)buf, &t.baseline.x);

	font_DrawText(&t);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static auto SummLabelLeftX() { return (11 - SafeNum(9)); }
static constexpr auto SummLabelLeftW {69}; // Nice
static auto SummLabelRightX() { return (158 - SafeNum(20)); }
static constexpr auto SummLabelRightW {72};
static constexpr auto SummLabelTopY {5};

static void
DrawAllLabels(GFXRECT rect)
{
	GFXPOINT pt;
	int16_t width {};
	Color OldColor;

	for (uint16_t i {4}; i <= 8; i++)
	{
		if (!strlen(GameString(LABEL_STRING_BASE + i)))
		{
			return;
		}
	}

	OldColor = SetContextForeGroundColor(BlackColor);

	rect.corner.x += ResScale(1);
	rect.corner.y += ResScale(1);
	rect.extent.width -= ResScale(2);
	rect.extent.height = ResScale(137 - SafeNum(1));

	DrawFilledRectangle(&rect);
	SetContextForeGroundColor(OldColor);

	width = SummLabelLeftW;
	pt.x = SummLabelLeftX();
	pt.y = SummLabelTopY;
	DrawLabel(pt, width, LABEL_STRING_BASE + 4); // CARGO

	pt.y = 76;
	DrawLabel(pt, width, LABEL_STRING_BASE + 5); // LANDER

	width = SummLabelRightW;
	pt.x = SummLabelRightX();
	pt.y = SummLabelTopY;
	DrawLabel(pt, width, LABEL_STRING_BASE + 6); // DEVICES

	pt.y = 87;
	DrawLabel(pt, width, LABEL_STRING_BASE + 7); // R.U.

	pt.y = 111;
	DrawLabel(pt, width, LABEL_STRING_BASE + 8); // CREDITS
}

static void
DrawBlankSavegameDisplay(PICK_GAME_STATE* pickState)
{
	STAMP s;
	GFXRECT rect;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex(pickState->SummaryFrame,
							   GetFrameCount(pickState->SummaryFrame) - 1);
	DrawStamp(&s);

	GetFrameRect(s.frame, &rect);
	rect.corner.x += s.origin.x;
	rect.corner.y += s.origin.y;

	DrawAllLabels(rect);
}

static void
DrawSaveLoad(PICK_GAME_STATE* pickState)
{
	STAMP s;
	GFXRECT r;

	s.frame = SetAbsFrameIndex(pickState->SummaryFrame,
							   GetFrameCount(pickState->SummaryFrame) - 2);

	GetFrameRect(s.frame, &r);

	s.origin.x = ResScale((OrigSisScreenWidth()
						   - ResDescale(r.extent.width))
						  / 2);
	s.origin.y = 0;

	r.corner.x = ResScale(1);
	r.corner.y -= SafeBoolScl(2, 1);
	r.extent.width = SisScreenWidth() - SafeBoolScl(2, 1);
	r.extent.height += SafeBoolScl(4, 3);
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	if (pickState->saving)
	{
		s.frame = DecFrameIndex(s.frame);
	}
	DrawStamp(&s);
}

static void
DrawSaveLoadText(PICK_GAME_STATE* pickState)
{
	GFXRECT r;
	Color OldColor;
	TEXT text;
	char buf[256];
	int16_t leading {};

	const auto SaveLoadY = [&] {
		return ResScale(151);
	};
	const auto SaveLoadH = [&] {
		return ResScale(7);
	};

	if (!strlen(
			GameString(LABEL_STRING_BASE + (3 - pickState->saving))))
	{
		DrawSaveLoad(pickState);
		return;
	}

	r.corner.x = ResScale(1);
	r.corner.y = SaveLoadY() - SafeBoolScl(2, 1);
	r.extent.width = SisScreenWidth() - SafeBoolScl(2, 1);
	r.extent.height = SaveLoadH() + SafeBoolScl(4, 3);
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	FONT OldFont {SetContextFont(SafeBool(MicroFont, LabelFont))};
	OldColor = SetContextForeGroundColor(
		SafeBool(SummTextColor, LoadSaveLabelColor));

	GetContextFontLeading(&leading);

	text.baseline.x = ResScale(OrigSisScreenWidth() / 2);
	text.baseline.y = ResScale(5) + SaveLoadY() + SafeBoolScl(2, 1);

	uqm::utf8StringCopy(buf, sizeof(buf),
						GameString(LABEL_STRING_BASE + (3 - pickState->saving)));

	text.align = ALIGN_CENTER;
	text.CharCount = (uint16_t)~0;
	text.pStr = buf;

	font_DrawText(&text);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static constexpr auto NumColors {9};

static void
DrawSavegameCargo(SIS_STATE* sisState)
{
	uint16_t i {};
	STAMP s;
	TEXT t;
	char buf[40];
	const static Color cargo_color[NumColors] = CARGO_COLOR_TABLE;
	const auto ElementOrgY = [&] {
		return ResScale(17);
	};
	const auto ElementSpacingY = [&] {
		return ResScale(12);
	};
	const auto ElementSpacingX = [&] {
		return ResScale(36);
	};

	SetContext(SpaceContext);
	BatchGraphics();
	SetContextFont(StarConFont);

	// setup element icons
	s.frame = SetAbsFrameIndex(MiscDataFrame,
							   (NumScandotTransitions << 1) + 3);
	s.origin.x = ResScale(7) + SummaryXOffs() - SummarySideOffs()
			   + NsafeNumScl(3);
	s.origin.y = ElementOrgY();
	// setup element amounts
	t.baseline.x = ResScale(33) + SummaryXOffs() - SummarySideOffs()
				 + NsafeNumScl(3);
	t.baseline.y = ElementOrgY() + ResScale(3);
	t.align = ALIGN_RIGHT;
	t.pStr = buf;

	// draw element icons and amounts
	for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
	{
		if (i == NUM_ELEMENT_CATEGORIES / 2)
		{
			s.origin.x += ElementSpacingX();
			s.origin.y = ElementOrgY();
			t.baseline.x += ElementSpacingX();
			t.baseline.y = ElementOrgY() + ResScale(3);
		}
		// draw element icon
		DrawStamp(&s);
		s.frame = SetRelFrameIndex(s.frame, 5);
		s.origin.y += ElementSpacingY();
		// print element amount
		SetContextForeGroundColor(cargo_color[i]);
		fmt::format_to_sz_n(buf, "{}", sisState->ElementAmounts[i]);
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += ElementSpacingY();
	}

	// draw Bio icon
	s.origin.x = ResScale(24) + SummaryXOffs() - SummarySideOffs();
	s.origin.y = ResScale(68);
	s.frame = SetAbsFrameIndex(s.frame, 68);
	DrawStamp(&s);
	// print Bio amount
	t.baseline.x = ResScale(50) + SummaryXOffs();
	t.baseline.y = s.origin.y + ResScale(3);
	SetContextForeGroundColor(cargo_color[i]);
	fmt::format_to_sz_n(buf, "{}", sisState->TotalBioMass);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	UnbatchGraphics();
}

static void
DrawEmptySlot(void)
{
	GFXRECT r;
	TEXT t;
	char buf[256];
	Color oldfg;
	uint8_t stroke {};
	GFXPOINT offset;
	GFXPOINT t_baseline;
	int16_t leading {};

	r.corner.x = ResScale(1);
	r.corner.y = ResScale(1);
	r.extent.width = SisScreenWidth() - SafeBoolScl(2, 1);
	r.extent.height = SafeBoolScl(146, 136);
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	SetContextFont(LabelFont);

	GetContextFontLeading(&leading);

	t.baseline.x = SisScreenWidth() >> 1;
	t.baseline.y = ResScale(66);

	uqm::utf8StringCopy(buf, sizeof(buf),
						GameString(LABEL_STRING_BASE + 1)); // EMPTY SLOT

	t.align = ALIGN_CENTER;

	oldfg = SetContextForeGroundColor(EmptySlotStroke3Color);
	t_baseline = t.baseline;

	for (stroke = ResScale(3); stroke > 0; stroke -= ResScale(1))
	{
		if (stroke == ResScale(2))
		{
			SetContextForeGroundColor(EmptySlotStroke2Color);
		}
		else if (stroke == ResScale(1))
		{
			SetContextForeGroundColor(EmptySlotStroke1Color);
		}

		for (offset.x = -stroke; offset.x <= stroke; ++offset.x)
		{
			for (offset.y = -stroke; offset.y <= stroke; ++offset.y)
			{
				if (hypot(offset.x, offset.y) > stroke)
				{
					continue;
				}
				t.baseline =
					MAKE_POINT(
						t_baseline.x + offset.x,
						t_baseline.y + offset.y);

				t.pStr = buf;
				t.CharCount = uqm::utf8StringPos(buf, ' ');
				font_DrawText(&t);

				t.baseline.y += leading;

				t.pStr = uqm::skipUTF8Chars(buf, t.CharCount);
				t.CharCount = (uint16_t)~0;
				font_DrawText(&t);

				t.baseline.y -= leading;
			}
		}
	}

	t.baseline = t_baseline;

	SetContextForeGroundColor(SummTextColor);

	t.pStr = buf;
	t.CharCount = uqm::utf8StringPos(buf, ' ');
	font_DrawText(&t);

	t.baseline.y += leading;

	t.pStr = uqm::skipUTF8Chars(buf, t.CharCount);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	SetContextForeGroundColor(oldfg);
}

static void
DrawBombPodText(STAMP* s)
{
	TEXT t;
	char buf[256];
	int16_t leading {};
	Color OldColor;
	GFXRECT r;
	int16_t og_baseline_x = 0;

	if (!strlen(GameString(LABEL_STRING_BASE + 9))
		|| !strlen(GameString(LABEL_STRING_BASE + 10)))
	{
		return;
	}

	FONT OldFont {SetContextFont(SquareFont)};
	OldColor = SetContextForeGroundColor(BlackColor);

	GetContextFontLeading(&leading);

	GetFrameRect(s->frame, &r);

	r.corner.y += ResScale(32);
	r.extent.height = ResScale(29);
	DrawFilledRectangle(&r);

	uqm::utf8StringCopy(buf, sizeof(buf),
						GameString(LABEL_STRING_BASE + 9));
	// AMPLIFIED PRECURSOR BOMB

	t.baseline.x = (r.extent.width >> 1) + r.corner.x + s->origin.x;
	t.baseline.y = r.corner.y + ResScale(1);
	og_baseline_x = t.baseline.x;

	t.align = ALIGN_CENTER;
	SetContextForeGroundColor(BombPodTextColor);
	{
		uqstl::vector<uqstl::string> tokens;
		uqm::tokenize(uqstl::string_view {buf}, tokens, ' ', false);
		for (const auto& tok : tokens)
		{
			t.pStr = AlignText((const char*)tok.c_str(), &t.baseline.x);
			t.CharCount = (uint16_t)~0;
			font_DrawText(&t);
			t.baseline.y += leading;
		}
	}

	r.corner.y += ResScale(78);
	r.extent.height = ResScale(15);
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	uqm::utf8StringCopy((char*)buf, sizeof(buf),
						GameString(LABEL_STRING_BASE + 10));
	// ESCAPE POD

	t.baseline.x = og_baseline_x;
	t.baseline.y = r.corner.y + ResScale(1);
	SetContextForeGroundColor(BombPodTextColor);
	{
		uqstl::vector<uqstl::string> tokens;
		uqm::tokenize(uqstl::string_view {buf}, tokens, ' ', false);
		for (const auto& tok : tokens)
		{
			t.pStr = AlignText((const char*)tok.c_str(), &t.baseline.x);
			t.CharCount = (uint16_t)~0;
			font_DrawText(&t);
			t.baseline.y += leading;
		}
	}

	SetContextForeGroundColor(OldColor);
	SetContextFont(OldFont);
}

static void
DrawNoLimit(void)
{
	GFXRECT r;
	TEXT t;
	Color oldfg;
	uint8_t stroke {};
	GFXPOINT offset;
	GFXPOINT t_baseline;
	int16_t leading {};

	r.corner.x = ResScale(137) + SummaryXOffs() + SummarySideOffs();
	r.corner.y = ResScale(93);
	r.extent.width = ResScale(75);
	r.extent.height = ResScale(17);
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	FONT OldFont {SetContextFont(LabelFont)};

	GetContextFontLeading(&leading);

	t.baseline.x = (r.extent.width >> 1) + r.corner.x - IfHd(2);
	t.baseline.y = r.corner.y + leading;

	t.pStr = GameString(LABEL_STRING_BASE); // NO LIMIT
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;

	oldfg = SetContextForeGroundColor(NoLimitStroke3Color);
	t_baseline = t.baseline;

	for (stroke = ResScale(3); stroke > 0; stroke -= ResScale(1))
	{
		if (stroke == ResScale(2))
		{
			SetContextForeGroundColor(NoLimitStroke2Color);
		}
		else if (stroke == ResScale(1))
		{
			SetContextForeGroundColor(NoLimitStroke1Color);
		}

		for (offset.x = -stroke; offset.x <= stroke; ++offset.x)
		{
			for (offset.y = -stroke; offset.y <= stroke; ++offset.y)
			{
				if (hypot(offset.x, offset.y) > stroke)
				{
					continue;
				}
				t.baseline =
					MAKE_POINT(
						t_baseline.x + offset.x,
						t_baseline.y + offset.y);

				font_DrawText(&t);
			}
		}
	}

	t.baseline = t_baseline;

	SetContextForeGroundColor(NoLimitTextColor);
	font_DrawText(&t);

	SetContextForeGroundColor(oldfg);
	SetContextFont(OldFont);
}

static void
DrawSavegameSummary(PICK_GAME_STATE* pickState, uint16_t gameIndex)
{
	SUMMARY_DESC* pSD = pickState->summary + gameIndex;
	GFXRECT r;
	STAMP s;

	BatchGraphics();

	SaveOrLoad = true;

	if (pSD->year_index == 0)
	{
		// Unused save slot, draw 'Empty Game' message.
		DrawSaveInfo(SIS_STATE {0});
		DrawEmptySlot();
	}
	else
	{
		// Game slot used, draw information about save game.
		uint16_t i {};
		GFXRECT OldRect;
		TEXT t;
		uqm::LinkQueue player_built_q {};
		SIS_STATE SaveSS;
		char buf[256];
		GFXPOINT starPt;

		// Save the states because we will hack them
		SaveSS = uqm::gameState().sis();

		GFXCONTEXT OldContext {SetContext(StatusContext)};
		// Hack StatusContext so we can use standard SIS display funcs
		GetContextClipRect(&OldRect);
		r.corner.x = SisOrgX()
				   + ResScale(
						 ResDescale(SisScreenWidth() - StatusWidth()) >> 1)
				   - NsafeNumScl(16) + SummaryXOffs();
		r.corner.y = SisOrgY();
		r.extent.width = StatusWidth();
		r.extent.height = StatusHeight();
		SetContextClipRect(&r);

		// Hack the states so that we can use standard SIS display funcs
		uqm::gameState().sis() = pSD->SS;

		DrawSaveInfo(uqm::gameState().sis());
		swap(player_built_q, uqm::gameState().game().built_ship_q);
		uqm::gameState().game().built_ship_q.reset(MaxBuiltShips, sizeof(SHIP_FRAGMENT));
		for (i = 0; i < pSD->NumShips; ++i)
		{
			CloneShipFragment(
				(RACE_ID)pSD->ShipList[i], &uqm::gameState().game().built_ship_q, 0);
		}
		DateToString(buf, sizeof buf,
					 pSD->month_index, pSD->day_index, pSD->year_index);
		ClearSISRect(DrawSisDisplay);
		DrawBorder(SIS_STAT_SUMM_FRAME);
		DrawStatusMessage(buf);
		uqm::gameState().game().built_ship_q.release();

		SetContextClipRect(&OldRect);

		SetContext(SpaceContext);

		// draw devices
		s.origin.y = ResScale(13);
		for (i = 0; i < 4; ++i)
		{
			s.origin.x = ResScale(140) + SummaryXOffs()
					   + SummarySideOffs();
			for (uint16_t j {0}; j < 4; ++j)
			{
				const uint16_t devIndex = (i * 4) + j;
				if (devIndex < pSD->NumDevices)
				{
					s.frame = SetAbsFrameIndex(MiscDataFrame, 77
																  + pSD->DeviceList[devIndex]);
					DrawStamp(&s);
				}
				s.origin.x += ResScale(18);
			}
			s.origin.y += ResScale(18);
		}

		SetContextFont(StarConFont);
		t.baseline.x = ResScale(173) + SummaryXOffs()
					 + SummarySideOffs();
		t.align = ALIGN_CENTER;
		t.CharCount = (uint16_t)~0;
		t.pStr = buf;
		if (pSD->Flags & AfterBombInstalled)
		{
			// draw the bomb and the escape pod
			s.origin.x = SummaryXOffs() - SummarySideOffs()
					   + NsafeNumScl(6);
			s.origin.y = 0;
			s.frame = SetRelFrameIndex(pickState->SummaryFrame, 0);
			DrawStamp(&s);

			DrawBombPodText(&s);

			// draw RU "NO LIMIT"
			DrawNoLimit();
		}
		else
		{
			DrawSavegameCargo(&pSD->SS);

			SetContext(RadarContext);
			// Hack RadarContext so we can use standard Lander display funcs
			GetContextClipRect(&OldRect);
			r.corner.x = SisOrgX() + ResScale(10) + SummaryXOffs()
					   - SummarySideOffs();
			r.corner.y = SisOrgY() + ResScale(84);
			r.extent = OldRect.extent;
			SetContextClipRect(&r);
			// draw the lander with upgrades
			InitLander(pSD->Flags | OverrideLanderFlags);
			SetContextClipRect(&OldRect);
			SetContext(SpaceContext);

			fmt::format_to_sz_n(buf, "{}", pSD->SS.ResUnits);
			t.baseline.y = ResScale(102);
			SetContextForeGroundColor(SummValueColor);
			font_DrawText(&t);
			t.CharCount = (uint16_t)~0;
		}

		t.baseline.y = ResScale(126);
		fmt::format_to_sz_n(buf, "{}",
							uqm::makeWord(pSD->MCreditLo, pSD->MCreditHi));
		SetContextForeGroundColor(SummValueColor);
		font_DrawText(&t);

		// print the location
		t.baseline.x = ResScale(1) + (SisMessageWidth() >> 1);
		t.baseline.y = ResScale(139 + 6) + SafeNumScl(2);
		t.align = ALIGN_CENTER;
		t.pStr = buf;
		starPt.x = LogxToUniverse(pSD->SS.log_x);
		starPt.y = LogyToUniverse(pSD->SS.log_y);
		switch (pSD->Activity)
		{
			case IN_LAST_BATTLE:
			case IN_INTERPLANETARY:
			case IN_PLANET_ORBIT:
			case IN_STARBASE:
				{
					uint8_t QuasiState {};

					QuasiState = uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE);
					uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(0));
					const STAR_DESC* SDPtr {FindStar(nullptr, &starPt, 1, 1)};
					uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE_SIDE, static_cast<uint32_t>(QuasiState));
					if (SDPtr)
					{
						GetClusterName(SDPtr, buf);
						starPt = SDPtr->star_pt;
						break;
					}
					FALLTHROUGH;
				}
			default:
				buf[0] = '\0';
				break;
			case IN_HYPERSPACE:
				uqm::utf8StringCopy(buf, sizeof(buf),
									GameString(NAVIGATION_STRING_BASE + 0));
				break;
			case IN_QUASISPACE:
				uqm::utf8StringCopy(buf, sizeof(buf),
									GameString(NAVIGATION_STRING_BASE + 1));
				break;
		}

		if (isPC(uqm::UQMOptions::read().whichFonts))
		{
			SetContextFont(TinyFont);
		}
		else
		{
			SetContextFont(TinyFontBold);
		}

		SetContextForeGroundColor(SaveSelectedColor);
		t.CharCount = (uint16_t)~0;
		if (is3DO(uqm::UQMOptions::read().whichFonts))
		{
			replaceChar(buf, UnicharSpace, UnicharTab);
		}
		font_DrawText(&t);
		t.align = ALIGN_CENTER;
		t.baseline.x = SisScreenWidth() - SisTitleBoxWidth()
					 - SafeBoolScl(4, -1)
					 + ResScale(ResDescale(SisTitleWidth()) >> 1);

		switch (pSD->Activity)
		{
			case IN_STARBASE:
				uqm::strncpy_safe(buf, // Starbase
								  GameString(STARBASE_STRING_BASE));
				break;
			case IN_LAST_BATTLE:
				uqm::strncpy_safe(buf, // Sa-Matra
								  GameString(PLANET_NUMBER_BASE + 32));
				break;
			case IN_PLANET_ORBIT:
				uqm::strncpy_safe(buf, pSD->SS.PlanetName);
				break;
			default:
				fmt::format_to_sz_n(buf, "{:03}.{:01} : {:03}.{:01}",
									starPt.x / 10, starPt.x % 10,
									starPt.y / 10, starPt.y % 10);
		}
		if (is3DO(uqm::UQMOptions::read().whichFonts))
		{
			replaceChar(buf, UnicharSpace, UnicharTab);
		}
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);

		SetContext(OldContext);

		// Restore the state and queues because we hacked them
		swap(uqm::gameState().game().built_ship_q, player_built_q);
		uqm::gameState().sis() = SaveSS;
		if (uqm::UQMOptions::read().customBorder)
		{
			DrawStatusMessage(nullptr);
		}
	}

	UnbatchGraphics();

	SaveOrLoad = false;
}

static void
TruncateSaveName(uqstl::span<char> buf, int16_t maxWidth, bool naming)
{
	TEXT t;
	GFXRECT r;

	t.pStr = buf.data();

	r = font_GetTextRect(&t);

	if (r.extent.width > maxWidth)
	{
		size_t stringLength = strlen(t.pStr);
		const char ellipses[] = "...";

		do
		{ // Shorten the save name down so it will fit the width of the save name box
			auto nameBuf = buf.subspan(--stringLength - sizeof(ellipses));
			uqm::strncpy_safe(nameBuf, ellipses);
			r = font_GetTextRect(&t);
		} while (r.extent.width > maxWidth);

		if (naming)
		{
			buf[strlen(buf.data()) - strlen(ellipses)] = '\0';
		}
	}
}

static void
DrawGameSelection(PICK_GAME_STATE* pickState, uint16_t selSlot)
{
	GFXRECT r;
	TEXT t;
	uint16_t i {};
	uint16_t curSlot {};
	char buf[256];
	char buf2[80];
	const char* SaveName {};
	const Color UnSelected = SaveUnselectedColor;
	const Color Selected = SaveSelectedColor;

	BatchGraphics();

	SetContextFont(TinyFont);

	DrawSaveLoadText(pickState);

	// Erase the selection menu
	r.corner.x = ResScale(1);
	r.corner.y = ResScale(160);
	r.extent.width = SisScreenWidth() - ResScale(2) + SafeNumScl(1);
	r.extent.height = SisScreenHeight() - r.corner.y - NsafeNumScl(1);
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	t.CharCount = (uint16_t)~0;
	t.pStr = buf;
	t.align = ALIGN_LEFT;

	// Draw savegame slots info
	curSlot = selSlot - (selSlot % SavesPerPage());
	for (i = 0; i < SavesPerPage() && curSlot < MaxSavedGames;
		 ++i, ++curSlot)
	{
		SUMMARY_DESC* desc = &pickState->summary[curSlot];

		SetContextForeGroundColor(
			(curSlot == selSlot) ? Selected : UnSelected);
		r.extent.width = ResScale(15);
		if (MaxSavedGames > 99)
		{
			r.extent.width += ResScale(5);
		}
		r.extent.height = ResScale(11);
		r.corner.x = ResScale(8);
		r.corner.y = ResScale(160 + (i * 13));
		DrawRectangle(&r, IsHd());

		t.baseline.x = r.corner.x + ResScale(3);
		t.baseline.y = r.corner.y + ResScale(8);
		fmt::format_to_sz_n(buf,
							(MaxSavedGames > 99) ? "{:03}" : "{:02}", curSlot);
		font_DrawText(&t);

		r.extent.width = ResScale(204) + (SisScreenWidth() - ResScale(242));
		r.corner.x = ResScale(30);
		DrawRectangle(&r, IsHd());

		t.baseline.x = r.corner.x + ResScale(3);
		if (desc->year_index == 0)
		{
			uqm::strncpy_safe(buf, GameString(SAVEGAME_STRING_BASE + 3));
			// "Empty Slot"
		}
		else
		{
			DateToString(buf2, sizeof(buf2), desc->month_index,
						 desc->day_index, desc->year_index);

			SaveName = desc->SaveName[0] ? desc->SaveName :
										   GameString(SAVEGAME_STRING_BASE + 4);

			fmt::format_to_sz_n(buf, "{}: {}", buf2, SaveName);

			TruncateSaveName(buf, r.extent.width - 7, false);
		}
		font_DrawText(&t);
	}

	UnbatchGraphics();
}

static void
RedrawPickDisplay(PICK_GAME_STATE* pickState, uint16_t selSlot)
{
	BatchGraphics();
	DrawBlankSavegameDisplay(pickState);
	DrawSavegameSummary(pickState, selSlot);
	DrawGameSelection(pickState, selSlot);
	UnbatchGraphics();
}

static void
LoadGameDescriptions(SUMMARY_DESC* pSD)
{
	for (uint16_t i {0}; i < MaxSavedGames; ++i, ++pSD)
	{
		if (!LoadGame(i, pSD))
		{
			pSD->year_index = 0;
		}
	}
}

static bool
DoPickGame(MENU_STATE* pMS)
{
	PICK_GAME_STATE* pickState = (PICK_GAME_STATE*)pMS->privData;
	uint8_t NewState {};
	const SUMMARY_DESC* pSD {};
	const TimeCount TimeIn {GetTimeCounter()};

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		return false;
	}

	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		pickState->success = false;
		return false;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		pSD = &pickState->summary[pMS->CurState];
		if (pickState->saving || pSD->year_index)
		{ // valid slot
			const uint32_t LoadFuelScaled = loadFuel / FuelTankScale;
			const uint32_t TankCapacityScaled = GetFuelTankCapacity() / FuelTankScale;

			if (uqm::UQMOptions::read().infiniteRU)
			{
				uqm::gameState().sis().ResUnits = oldRU;
			}

			if (uqm::UQMOptions::read().infiniteFuel)
			{
				if (loadFuel <= GetFuelTankCapacity())
				{
					uqm::gameState().sis().FuelOnBoard = loadFuel;
				}
				else
				{
					uqm::gameState().sis().ResUnits += (LoadFuelScaled - TankCapacityScaled) * uqm::gameState().game().FuelCost;
					uqm::gameState().sis().FuelOnBoard = GetFuelTankCapacity();
				}
			}

			PlayMenuSound(MENU_SOUND_SUCCESS);
			pickState->success = true;
			return false;
		}
		PlayMenuSound(MENU_SOUND_FAILURE);
	}
	else
	{
		NewState = pMS->CurState;
		if (PulsedInputState.menu[KEY_MENU_LEFT]
			|| PulsedInputState.menu[KEY_MENU_PAGE_UP])
		{
			if (NewState == 0)
			{
				NewState = MaxSavedGames - 1;
			}
			else if ((NewState - SavesPerPage()) > 0)
			{
				NewState -= SavesPerPage();
			}
			else
			{
				NewState = 0;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_RIGHT]
				 || PulsedInputState.menu[KEY_MENU_PAGE_DOWN])
		{
			if (NewState == MaxSavedGames - 1)
			{
				NewState = 0;
			}
			else if ((NewState + SavesPerPage()) < MaxSavedGames - 1)
			{
				NewState += SavesPerPage();
			}
			else
			{
				NewState = MaxSavedGames - 1;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState == 0)
			{
				NewState = MaxSavedGames - 1;
			}
			else
			{
				NewState--;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState == MaxSavedGames - 1)
			{
				NewState = 0;
			}
			else
			{
				NewState++;
			}
		}

		if (NewState != pMS->CurState)
		{
			pMS->CurState = NewState;
			SetContext(SpaceContext);
			RedrawPickDisplay(pickState, pMS->CurState);
		}

		SleepThreadUntil(TimeIn + c_gameSecond / 30);
	}

	return true;
}

static bool
SaveLoadGame(PICK_GAME_STATE* pickState, uint16_t gameIndex, bool* canceled_by_user)
{
	SUMMARY_DESC* desc = pickState->summary + gameIndex;
	STAMP saveStamp;
	bool success {};
	GFXRECT r;

	GetContextClipRect(&r);

	saveStamp.frame = nullptr;

	if (pickState->saving)
	{
		TruncateSaveName(desc->SaveName, r.extent.width - 104, true);

		char nameBuf[256] {};
		// Initialize the save name with whatever name is there already
		// SaveNameSize is less than 256, so this is safe.
		uqm::strncpy_safe(nameBuf, desc->SaveName);
		if (NameSaveGame(gameIndex, nameBuf))
		{
			PlayMenuSound(MENU_SOUND_SUCCESS);
			ConfirmSaveLoad(pickState->saving ? &saveStamp : nullptr);
			success = SaveGame(gameIndex, desc, nameBuf);
		}
		else
		{
			success = false;
			*canceled_by_user = true;
		}
	}
	else
	{
		ConfirmSaveLoad(pickState->saving ? &saveStamp : nullptr);
		success = LoadGame(gameIndex, nullptr);
	}

	// TODO: the same should be done for both save and load if we also
	//   display a load problem message
	if (pickState->saving)
	{ // restore the screen under "SAVING..." message
		DrawStamp(&saveStamp);
	}

	DestroyDrawable(ReleaseDrawable(saveStamp.frame));

	return success;
}

static bool
PickGame(bool saving, bool fromMainMenu)
{
	MENU_STATE MenuState;
	PICK_GAME_STATE pickState;
	GFXRECT DlgRect;
	STAMP DlgStamp;
	TimeCount TimeOut;

	memset(&pickState, 0, sizeof pickState);
	pickState.saving = saving;
	pickState.SummaryFrame = SetAbsFrameIndex(PlayFrame, 44);

	memset(&MenuState, 0, sizeof MenuState);
	MenuState.privData = &pickState;
	// select the last used slot
	MenuState.CurState = lastUsedSlot;

	TimeOut = FadeMusic(0, c_gameSecond / 2);

	// Deactivate any background drawing, like planet rotation
	InputFrameCallback* oldCallback {SetInputCallback(nullptr)};

	LoadGameDescriptions(pickState.summary);

	GFXCONTEXT OldContext {SetContext(SpaceContext)};
	// Save the current state of the screen for later restoration
	DlgStamp = SaveContextFrame(nullptr);
	GetContextClipRect(&DlgRect);

	DrawMenuStateStrings(PM_SAVE_GAME, !saving);

	SleepThreadUntil(TimeOut);
	PauseMusic();
	StopSound();
	FadeMusic(NormalVolume, TimePeriod {});

	// draw the current savegame and fade in
	SetTransitionSource(nullptr);
	BatchGraphics();

	SetContextBackGroundColor(BlackColor);
	ClearDrawable();
	RedrawPickDisplay(&pickState, MenuState.CurState);

	if (fromMainMenu)
	{
		UnbatchGraphics();
		FadeScreen(FadeAllToColor, c_gameSecond / 2);
	}
	else
	{
		GFXRECT ctxRect;

		GetContextClipRect(&ctxRect);
		ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, &ctxRect);
		UnbatchGraphics();
	}

	SetMenuSounds(MenuSoundArrows | MenuSoundPage,
				  0);
	MenuState.InputFunc = DoPickGame;

	// Save/load retry loop
	while (1)
	{
		bool canceled_by_user = false;

		pickState.success = false;
		DoInput(&MenuState, true);
		if (!pickState.success)
		{
			break; // canceled
		}

		lastUsedSlot = MenuState.CurState;

		if (SaveLoadGame(&pickState, MenuState.CurState, &canceled_by_user))
		{
			break; // all good
		}

		// something broke
		if (saving && !canceled_by_user)
		{
			SaveProblem();
		}
		// TODO: Shouldn't we have a Problem() equivalent for Load too?

		// reload and redraw everything
		LoadGameDescriptions(pickState.summary);
		RedrawPickDisplay(&pickState, MenuState.CurState);
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	if (pickState.success && !saving)
	{ // Load succeeded, signal up the chain
		uqm::currentActivity() |= CHECK_LOAD;
	}

	if (pickState.success)
	{
		SUMMARY_DESC* pSD = pickState.summary + MenuState.CurState;

		uqm::log::info(saving ? "Saving > " : "Loading > ");
		uqm::log::info("Name: {}", pSD->SaveName);
		uqm::log::info("Slot: {}", MenuState.CurState);
		uqm::log::info("Seed Type: {}", toString(uqm::currentSeed().type));
		uqm::log::info("Seed: {}", uqm::gameState().sis().Seed);
		uqm::log::info("Difficulty: {:s}", getDifficulty());
		uqm::log::info("Extended: {}", BoolStr(IsExtended()));
		uqm::log::info("Nomad: {:s}\n", getNomadMode());
	}

	if (!(uqm::currentActivity() & CHECK_ABORT) && (saving || (!pickState.success && !fromMainMenu)))
	{ // Restore previous screen
		const bool InStarbase = (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) == (uint8_t)~0);

		// Math to include the title bars in the screen transition
		DlgRect.extent.width += DlgRect.corner.x;
		DlgRect.extent.height += DlgRect.corner.y;
		DlgRect.corner.x = DlgRect.corner.y = 0;

		SetTransitionSource(&DlgRect);
		BatchGraphics();

		DrawStamp(&DlgStamp);

		// These redraw the status of the ship after saving or aborting a load/save

		// Redraws main title bar at the top-left
		RestoreSISMessage(InStarbase, OutfitOrShipyard);

		OutfitOrShipyard = 0;

		// Redraws secondary title bar to the right of the main bar
		if (inHQSpace())
		{
			DrawHyperCoords(uqm::flagship().shipStamp.origin);
		}
		else if (InStarbase)
		{
			DrawSISTitle(GameString(STARBASE_STRING_BASE + 0));
		}
		else if (uqm::flagship().ipPlanet == 0)
		{
			DrawHyperCoords(CurStarDescPtr()->star_pt);
		}
		else
		{
			DrawSISTitle(uqm::gameState().sis().PlanetName);
		}

		// Redraws fuel, crew, and status message (green box)
		DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);

		ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, &DlgRect);
		UnbatchGraphics();
	}

	DestroyDrawable(ReleaseDrawable(DlgStamp.frame));

	SetContext(OldContext);

	ResumeMusic();

	// Reactivate any background drawing, like planet rotation
	SetInputCallback(oldCallback);

	return pickState.success;
}

static bool
DoGameOptions(MENU_STATE* pMS)
{
	if (uqm::currentActivity() & CHECK_ABORT)
	{
		return false;
	}

	if (PulsedInputState.menu[KEY_MENU_CANCEL]
		|| (PulsedInputState.menu[KEY_MENU_SELECT]
			&& pMS->CurState == EXIT_GAME_MENU))
	{
		return false;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case SAVE_GAME:
			case LOAD_GAME:
				SetFlashRect(nullptr, false);
				if (PickGame(pMS->CurState == SAVE_GAME, false))
				{
					return false;
				}
				DrawMenuStateStrings(PM_SAVE_GAME, pMS->CurState);
				DrawSeedMessage();
				SetFlashRect(SfrMenu3do(), false);
				break;
			case QUIT_GAME:
				DrawMenuStateStrings(PM_SAVE_GAME, pMS->CurState);
				if (ConfirmExit())
				{
					return false;
				}
				else
				{
					DrawMenuStateStrings(PM_SAVE_GAME, pMS->CurState);
					DrawSeedMessage();
				}
				break;
			case SETTINGS:
				SettingsMenu(false);
				DrawMenuStateStrings(PM_SAVE_GAME, pMS->CurState);
				DrawSeedMessage();
				break;
		}
	}
	else
	{
		DoMenuChooser(pMS, PM_SAVE_GAME);
	}

	return true;
}

// Returns true when the owner menu should continue
bool GameOptions(void)
{
	MENU_STATE MenuState;

	memset(&MenuState, 0, sizeof MenuState);

	if (LastActivity == CHECK_LOAD)
	{ // Selected LOAD from main menu
		if (const char* path {TakePlaySave()})
		{
			const bool loaded {LoadGameFile(path)};
			if (loaded)
			{
				uqm::log::info("Loaded the save file '{}'.", path);
			}
			// Fades the screen in, as the load menu does.
			FadeScreen(FadeAllToColor, TimePeriod {});
			uqm::currentActivity() |= loaded ? CHECK_LOAD : CHECK_ABORT;
			return false;
		}

		DrawMenuStateStrings(PM_SAVE_GAME, LOAD_GAME);
		const bool success {PickGame(false, true)};
		if (!success)
		{ // Selected LOAD from main menu, and canceled
			uqm::currentActivity() |= CHECK_ABORT;
		}
		return false;
	}

	const bool inStarbase {uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) == (uint8_t)~0};
	const uint8_t outfitOrShipyard {OutfitOrShipyard};

	MenuState.CurState = SAVE_GAME;
	DrawMenuStateStrings(PM_SAVE_GAME, MenuState.CurState);
	DrawSeedMessage();

	SetFlashRect(SfrMenu3do(), false);

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
	MenuState.InputFunc = DoGameOptions;
	DoInput(&MenuState, true);
	SetFlashRect(nullptr, false);

	if (!(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
	{
		RestoreSISMessage(inStarbase, outfitOrShipyard);
	}

	return !(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD));
}
