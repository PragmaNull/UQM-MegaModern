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

#include "sis.h"

#include "colors.h"
#include "races.h"
#include "starmap.h"
#include "units.h"
#include "menustat.h"
// for DrawMenuStateStrings()
#include "gamestr.h"
#include "options.h"
#include "battle.h"
// For BattleFrameRateTicks
#include "element.h"
#include "setup.h"
#include "state.h"
#include "flash.h"
#include "libs/graphics/gfx_common.h"
#include "libs/time/timelib.h"
#include "libs/callback/alarm.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "core/string/Utf8.h"
#include "hyper.h"
#include "gameopt.h"
#include <math.h>
#include "uqm/util.h"

#include <stdio.h>

static StatMsgMode curMsgMode = SMM_DEFAULT;

static const char* describeWeapon(uint8_t moduleType);

FRAME hdFuelFrame;

void RepairSISBorder(void)
{
	GFXRECT r;
	GFXCONTEXT OldContext;

	OldContext = SetContext(ScreenContext);

	BatchGraphics();

	// Left border
	r.corner.x = SIS_ORG_X - RES_SCALE(1);
	r.corner.y = SIS_ORG_Y - RES_SCALE(1);
	r.extent.width = RES_SCALE(1);
	r.extent.height = SIS_SCREEN_HEIGHT + RES_SCALE(2);
	SetContextForeGroundColor(SIS_LEFT_BORDER_COLOR);
	DrawFilledRectangle(&r);

	// Right border
	SetContextForeGroundColor(SIS_BOTTOM_RIGHT_BORDER_COLOR);
	r.corner.x += (SIS_SCREEN_WIDTH + RES_SCALE(2)) - RES_SCALE(1);
	DrawFilledRectangle(&r);

	// Bottom border
	r.corner.x = SIS_ORG_X - RES_SCALE(1);
	r.corner.y += (SIS_SCREEN_HEIGHT + RES_SCALE(2)) - RES_SCALE(1);
	r.extent.width = SIS_SCREEN_WIDTH + RES_SCALE(1);
	r.extent.height = RES_SCALE(1);
	DrawFilledRectangle(&r);

	DrawBorder(SIS_REPAIR_FRAME);

	UnbatchGraphics();

	SetContext(OldContext);
}

void ClearSISRect(uint8_t ClearFlags)
{
	//GFXRECT r; Unused
	Color OldColor;
	GFXCONTEXT OldContext;

	OldContext = SetContext(StatusContext);
	OldColor = SetContextForeGroundColor(
		BUILD_COLOR(MAKE_RGB15(0x0A, 0x0A, 0x0A), 0x08));

	//r.corner.x = RES_SCALE (2);
	//r.extent.width = STATUS_WIDTH - RES_SCALE (4); Unused

	BatchGraphics();
	if (ClearFlags & DRAW_SIS_DISPLAY)
	{
		DeltaSISGauges(UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
	}

	if (ClearFlags & CLEAR_SIS_RADAR)
	{
		DrawMenuStateStrings((uint8_t)~0, 1);
#ifdef NEVER
		r.corner.x = RADAR_X - RES_SCALE(1);
		r.corner.y = RADAR_Y - RES_SCALE(1);
		r.extent.width = RADAR_WIDTH + RES_SCALE(2);
		r.extent.height = RADAR_HEIGHT + RES_SCALE(2);

		DrawStarConBox(&r, RES_SCALE(1), SHADOWBOX_MEDIUM_COLOR,
					   SHADOWBOX_DARK_COLOR, true, SCAN_BIOLOGICAL_TEXT_COLOR,
					   false, TRANSPARENT);
#endif /* NEVER */
	}
	UnbatchGraphics();

	SetContextForeGroundColor(OldColor);
	SetContext(OldContext);
}

// Draw the SIS title. This is the field at the top of the screen, on the
// right hand side, containing the coordinates in HyperSpace, or the planet
// name in IP.
void DrawSISTitle(char* pStr)
{
	TEXT t;
	GFXCONTEXT OldContext;
	GFXRECT r;

	t.baseline.x = RES_SCALE((RES_DESCALE(SIS_TITLE_WIDTH) >> 1));
	t.baseline.y = SIS_TITLE_HEIGHT - RES_SCALE(2);
	t.align = ALIGN_CENTER;
	t.pStr = pStr;
	t.CharCount = (uint16_t)~0;

	OldContext = SetContext(OffScreenContext);
	r.corner.x = SIS_ORG_X + SIS_SCREEN_WIDTH - SIS_TITLE_BOX_WIDTH
			   + RES_SCALE(1);
	r.corner.y = SIS_ORG_Y - SIS_TITLE_HEIGHT;
	r.extent.width = SIS_TITLE_WIDTH;
	r.extent.height = SIS_TITLE_HEIGHT - RES_SCALE(1);
	SetContextFGFrame(Screen);
	SetContextClipRect(&r);
	if (isPC(uqm::UQMOptions::read().whichFonts) || SaveOrLoad)
	{
		SetContextFont(TinyFont);
	}
	else
	{
		char* buf = pStr;

		SetContextFont(TinyFontBold);
		replaceChar(buf, UNICHAR_SPACE, UNICHAR_TAB);
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
	}

	BatchGraphics();

	// Background color
	SetContextBackGroundColor(SIS_TITLE_BACKGROUND_COLOR);
	ClearDrawable();

	DrawBorder(SIS_TITLE_FRAME);

	// Text color
	SetContextForeGroundColor(SIS_TITLE_TEXT_COLOR);
	font_DrawText(&t);

	UnbatchGraphics();

	SetContextClipRect(nullptr);

	SetContext(OldContext);
}

void DrawHyperCoords(GFXPOINT universe)
{
	char buf[100] {};
	const char* SpaceOrNull = (isPC(uqm::UQMOptions::read().whichFonts) ? STR_SPACE : "");

	fmt::format_to_sz_n(buf, "{:03}.{:01}{}:{}{:03}.{:01}",
						universe.x / 10, universe.x % 10,
						SpaceOrNull, SpaceOrNull,
						universe.y / 10, universe.y % 10);

	DrawSISTitle(buf);
}

void DrawSaveInfo(SIS_STATE SisState)
{
	char buf[100];
	char TempDiff[11];
	char TempExt[12] = "";
	char TempNom[10] = "";
	char TempVer[SIS_NAME_SIZE] = "";

	if (SisState.SaveVersion > 0)
	{
		uqm::utf8StringCopy(TempVer, sizeof(TempVer),
							GAME_STRING(SAVEGAME_STRING_BASE + 4
										+ SisState.SaveVersion));
	}

	DrawSISMessage(TempVer);
	char emptyTitle[] {""};
	DrawSISTitle(emptyTitle);

	if (SisState.Seed)
	{
		if (SisState.Nomad != uqm::NomadMode::Off)
		{
			static const char* NomadModeSuffix[] {"", "-", "+"};

			fmt::format_to_sz_n(TempNom, "{}{}",
								GAME_STRING(MAINMENU_STRING_BASE + 60),
								NomadModeSuffix[static_cast<int>(SisState.Nomad)]);
		}

		if (SisState.Extended)
		{
			uqm::utf8StringCopy(TempExt, sizeof(TempExt),
								GAME_STRING(MAINMENU_STRING_BASE + 59));
		}

		uqm::utf8StringCopy(TempDiff, sizeof(TempDiff),
							GAME_STRING(MAINMENU_STRING_BASE + 56
										+ static_cast<int>(SisState.Difficulty)));

		fmt::format_to_sz_n(buf, "{} {}{}{}",
							GAME_STRING(MAINMENU_STRING_BASE + 55), // Difficulty:
							TempDiff, TempExt, TempNom);
		DrawSISMessage(buf);

		fmt::format_to_sz_n(buf, "{}", SisState.Seed);
		DrawSISTitle(buf);
	}
}

void DrawSISMessage(const char* pStr)
{
	DrawSISMessageEx(pStr, -1, -1, DSME_NONE);
}

// See sis.h for the allowed flags. This is the field at the top of the
// screen, on the left hand side.
bool DrawSISMessageEx(const char* pStr, int16_t CurPos, int16_t ExPos,
					  uint16_t flags)
{
	char buf[256];
	GFXCONTEXT OldContext;
	TEXT t;
	GFXRECT r;

	OldContext = SetContext(OffScreenContext);
	// prepare the context
	r.corner.x = SIS_ORG_X + RES_SCALE(1);
	r.corner.y = SIS_ORG_Y - SIS_MESSAGE_HEIGHT;
	r.extent.width = SIS_MESSAGE_WIDTH;
	r.extent.height = SIS_MESSAGE_HEIGHT - RES_SCALE(1);
	SetContextFGFrame(Screen);
	SetContextClipRect(&r);

	BatchGraphics();
	SetContextBackGroundColor(SIS_MESSAGE_BACKGROUND_COLOR);

	if (pStr == 0)
	{
		switch (uqm::lowByte(GLOBAL(CurrentActivity)))
		{
			default:
			case IN_ENCOUNTER:
				pStr = "";
				break;
			case IN_LAST_BATTLE:
			case IN_INTERPLANETARY:
				GetClusterName(CurStarDescPtr, buf);
				pStr = buf;
				break;
			case IN_HYPERSPACE:
				if (inHyperSpace())
				{
					pStr = GAME_STRING(NAVIGATION_STRING_BASE);
					// "HyperSpace"
				}
				else
				{
					GFXPOINT Log = MAKE_POINT(
						LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x)),
						LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y)));

					pStr = GAME_STRING(NAVIGATION_STRING_BASE + 1);
					// "QuasiSpace"

					if (GET_GAME_STATE(ARILOU_HOME_VISITS)
						&& (Log.x == ARILOU_HOME_X
							&& Log.y == ARILOU_HOME_Y))
					{
						uqm::utf8StringCopy(
							GLOBAL_SIS(PlanetName),
							sizeof GLOBAL_SIS(PlanetName),
							GAME_STRING(STAR_STRING_BASE + 148));
						// "Falayalaralfali"
					}
				}
				break;
		}
	}

	if (!(flags & DSME_MYCOLOR))
	{
		SetContextForeGroundColor(SIS_MESSAGE_TEXT_COLOR);
	}

	t.baseline.y = SIS_MESSAGE_HEIGHT - RES_SCALE(2);
	t.baseline.x = RES_SCALE(RES_DESCALE(SIS_MESSAGE_WIDTH) >> 1);
	t.pStr = pStr;
	t.CharCount = (uint16_t)~0;
	if (isPC(uqm::UQMOptions::read().whichFonts) || SaveOrLoad)
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);

		if (CurPos < 0 && ExPos < 0)
		{
			char buf[100];

			uqm::utf8StringCopy(buf, sizeof(buf), pStr);
			replaceChar(buf, UNICHAR_SPACE, UNICHAR_TAB);

			t.pStr = buf;
			t.CharCount = (uint16_t)~0;
		}
	}

	if (flags & DSME_CLEARFR)
	{
		SetFlashRect(nullptr, false);
	}

	if (CurPos < 0 && ExPos < 0)
	{ // normal state
		ClearDrawable();

		DrawBorder(SIS_MSG_FRAME);
		t.align = ALIGN_CENTER;
		font_DrawText(&t);
	}
	else
	{ // editing state
		int i;
		GFXRECT text_r;
		// XXX: 128 is currently safe, but it would be better to specify
		//   the size to TextRect()
		uint8_t char_deltas[128];
		uint8_t* pchar_deltas;

		t.baseline.x = RES_SCALE(3);
		t.align = ALIGN_LEFT;

		TextRect(&t, &text_r, char_deltas);
#if 0
		if (text_r.extent.width + t.baseline.x + RES_SCALE (2)
				>= r.extent.width)
		{	// the text does not fit the input box size and so
			// will not fit when displayed later
			// disallow the change
			UnbatchGraphics ();
			SetContextClipRect (nullptr);
			SetContext (OldContext);
			return (false);
		}
#endif

		ClearDrawable();
		DrawBorder(SIS_MSG_FRAME);

		if (CurPos >= 0 && CurPos <= t.CharCount)
		{ // calc and draw the cursor
			GFXRECT cur_r = text_r;

			pchar_deltas = char_deltas;
			for (i = CurPos; i > 0; --i)
			{
				cur_r.corner.x += (int16_t)*pchar_deltas++;
			}
			if (CurPos < t.CharCount) /* end of line */
			{
				cur_r.corner.x -= RES_SCALE(1);
			}

			if (flags & DSME_BLOCKCUR)
			{ // Use block cursor for keyboardless systems

				cur_r.corner.y = 0;
				cur_r.extent.height = r.extent.height;

				SetCursorFlashBlock(true);

				if (CurPos == t.CharCount)
				{ // cursor at end-line -- use insertion point
					cur_r.extent.width = RES_SCALE(1);
					cur_r.corner.x -= IF_HD(3);
				}
				else if (CurPos + 1 == t.CharCount)
				{ // extra pixel for last char margin
					cur_r.extent.width = (int16_t)*pchar_deltas - IF_HD(3);
					cur_r.corner.x += RES_SCALE(1);
				}
				else if (CurPos < ExPos)
				{
					cur_r.extent.width = (int16_t)*pchar_deltas
									   - RES_SCALE(1);
					cur_r.corner.x += RES_SCALE(1);
				}
				else
				{ // normal mid-line char
					cur_r.extent.width = (int16_t)*pchar_deltas;
					cur_r.corner.x += RES_SCALE(1);
				}

				if (cur_r.extent.width >= 200)
				{
					cur_r.extent.width = RES_SCALE(1);
					cur_r.corner.x -= IF_HD(3);
				}
				else
				{
					SetContextForeGroundColor(SIS_MESSAGE_CURSOR_COLOR);
					DrawFilledRectangle(&cur_r);
				}
			}
			else
			{ // Insertion point cursor
				cur_r.corner.y = RES_SCALE(1);
				cur_r.extent.height = r.extent.height - RES_SCALE(2);
				cur_r.extent.width = RES_SCALE(1);

				if (CurPos == t.CharCount)
				{
					text_r.corner.x -= IF_HD(3);
				}

				SetCursorFlashBlock(false);
			}

			SetCursorRect(&cur_r, OffScreenContext);
		}

		SetContextForeGroundColor(SIS_MESSAGE_TEXT_COLOR);

		if (ExPos >= 0 && ExPos < t.CharCount)
		{ // handle extra characters
			t.CharCount = ExPos;
			font_DrawText(&t);

			// print extra chars
			SetContextForeGroundColor(SIS_MESSAGE_EXTRA_TEXT_COLOR);
			for (i = ExPos, pchar_deltas = char_deltas; i > 0; --i)
			{
				t.baseline.x += (int16_t)*pchar_deltas++;
			}
			t.pStr = uqm::skipUTF8Chars(t.pStr, ExPos);
			t.CharCount = (uint16_t)~0;
			font_DrawText(&t);
		}
		else
		{ // just print the text
			font_DrawText(&t);
		}
	}

	if (flags & DSME_SETFR)
	{
		r.corner.x = 0;
		r.corner.y = 0;
	}

	UnbatchGraphics();

	SetContextClipRect(nullptr);
	SetContext(OldContext);

	return (true);
}

void DateToString(char* buf, size_t bufLen,
				  uint8_t month_index, uint8_t day_index, uint16_t year_index)
{
	switch (uqm::UQMOptions::read().optDateFormat)
	{
		case uqm::DateFormat::MMM_dd_yyyy: //MMM dd.yyyy ie: Mar 01.2500
			fmt::format_to_sz_n(buf, bufLen, "{} {:02}" STR_MIDDLE_DOT "{:04}",
								GAME_STRING(MONTHS_STRING_BASE + month_index - 1),
								day_index, year_index);
			break;
		case uqm::DateFormat::MM_dd_yyyy: //MM.dd.yyyy  ie: 03.01.2500
			fmt::format_to_sz_n(buf, bufLen, "{:02}" STR_MIDDLE_DOT "{:02}" STR_MIDDLE_DOT "{:04}", month_index,
								day_index, year_index);
			break;
		case uqm::DateFormat::dd_MMM_yyyy: //dd MMM yyyy ie: 01 Mar 2500
			fmt::format_to_sz_n(buf, bufLen, "{:02} {} {:04}", day_index,
								GAME_STRING(MONTHS_STRING_BASE + month_index - 1),
								year_index);
			break;
		case uqm::DateFormat::dd_MM_yyyy: //dd MM yyyy  ie: 01.03.2500
			fmt::format_to_sz_n(buf, bufLen, "{:02}" STR_MIDDLE_DOT "{:02}" STR_MIDDLE_DOT "{:04}", day_index,
								month_index, year_index);
			break;
	}
}

void GetStatusMessageRect(GFXRECT* r)
{
	r->corner.x = RES_SCALE(2);
	r->corner.y = RES_SCALE(130);
	r->extent.width = STATUS_MESSAGE_WIDTH;
	r->extent.height = STATUS_MESSAGE_HEIGHT;
}

void DrawStatusMessage(const char* pStr)
{
	GFXRECT r;
	GFXRECT ctxRect;
	TEXT t;
	char buf[128];
	GFXCONTEXT OldContext;

	OldContext = SetContext(StatusContext);
	GetContextClipRect(&ctxRect);
	// XXX: Technically, this does not need OffScreenContext. The only
	// reason it is used is to avoid preserving StatusContext settings.
	SetContext(OffScreenContext);
	SetContextFGFrame(Screen);
	GetStatusMessageRect(&r);
	r.corner.x += ctxRect.corner.x;
	r.corner.y += ctxRect.corner.y;
	SetContextClipRect(&r);

	BatchGraphics();
	SetContextBackGroundColor(STATUS_MESSAGE_BACKGROUND_COLOR);
	ClearDrawable();

	DrawBorder(STAT_MSG_FRAME);

	if (!pStr)
	{
		if (curMsgMode == SMM_CREDITS)
		{
			if (uqm::UQMOptions::read().infiniteCredits)
			{
				fmt::format_to_sz_n(buf, "{} {}",
									(isPC(uqm::UQMOptions::read().whichMenu) && isPC(uqm::UQMOptions::read().whichFonts)) ?
										GAME_STRING(STATUS_STRING_BASE + 2) :
										STR_INFINITY_SIGN,				  // "UNLIMITED"
									GAME_STRING(STATUS_STRING_BASE + 0)); // "Cr"
			}
			else
			{
				fmt::format_to_sz_n(buf, "{} {}", uqm::makeWord(GET_GAME_STATE(MELNORME_CREDIT0), GET_GAME_STATE(MELNORME_CREDIT1)), GAME_STRING(STATUS_STRING_BASE + 0)); // "Cr"
			}
		}
		else if (curMsgMode == SMM_RES_UNITS)
		{
			if (GET_GAME_STATE(CHMMR_BOMB_STATE) >= 2 || uqm::UQMOptions::read().infiniteRU)
			{
				fmt::format_to_sz_n(buf, "{} {}",
									(isPC(uqm::UQMOptions::read().whichMenu) && isPC(uqm::UQMOptions::read().whichFonts)) ?
										GAME_STRING(STATUS_STRING_BASE + 2) :
										STR_INFINITY_SIGN,				  // "UNLIMITED"
									GAME_STRING(STATUS_STRING_BASE + 1)); // "RU"
			}
			else
			{
				fmt::format_to_sz_n(buf, "{} {}", GLOBAL_SIS(ResUnits),
									GAME_STRING(STATUS_STRING_BASE + 1)); // "RU"
			}
		}
		else
		{ // Just a date
			DateToString(buf, sizeof buf,
						 GLOBAL(GameClock.month_index),
						 GLOBAL(GameClock.day_index),
						 GLOBAL(GameClock.year_index));
		}
		pStr = buf;
	}

	t.baseline.x = (STATUS_MESSAGE_WIDTH >> 1);
	t.baseline.y = STATUS_MESSAGE_HEIGHT - RES_SCALE(1);
	t.align = ALIGN_CENTER;
	t.pStr = pStr;
	t.CharCount = (uint16_t)~0;

	{
		Color statusColor = STATUS_MESSAGE_TEXT_COLOR;

		if (curMsgMode == SMM_WARNING)
		{
			statusColor = STATUS_MESSAGE_WARNING_TEXT_COLOR;
		}
		if (curMsgMode == SMM_ALERT)
		{
			statusColor = STATUS_MESSAGE_ALERT_TEXT_COLOR;
		}

		SetContextForeGroundColor(statusColor);
	}

	if (isPC(uqm::UQMOptions::read().whichFonts) || uqm::UQMOptions::read().customBorder)
	{
		SetContextFont(TinyFont);
	}
	else
	{
		char buf[100];

		SetContextFont(TinyFontBold);

		uqm::utf8StringCopy(buf, sizeof(buf), pStr);
		replaceChar(buf, UNICHAR_SPACE, UNICHAR_TAB);

		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
	}

	SetContextForeGroundColor(STATUS_MESSAGE_TEXT_COLOR);
	font_DrawText(&t);
	UnbatchGraphics();

	SetContextClipRect(nullptr);

	SetContext(OldContext);
}

StatMsgMode
SetStatusMessageMode(StatMsgMode newMode)
{
	StatMsgMode oldMode = curMsgMode;
	curMsgMode = newMode;
	return oldMode;
}

void DrawCaptainsName(bool NewGame)
{
	GFXRECT r;
	TEXT t;
	GFXCONTEXT OldContext;
	FONT OldFont;
	Color OldColor;

	OldContext = SetContext(StatusContext);
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		OldFont = SetContextFont(TinyFont);
	}
	else
	{
		OldFont = SetContextFont(TinyFontBold);
	}

	OldColor = SetContextForeGroundColor(CAPTAIN_NAME_BACKGROUND_COLOR);

	r.corner.x = RES_SCALE(2 + 1);
	r.corner.y = RES_SCALE(10);
	r.extent.width = SHIP_NAME_WIDTH - RES_SCALE(2);
	r.extent.height = SHIP_NAME_HEIGHT;
	DrawFilledRectangle(&r);

	if (!NewGame)
	{
		DrawBorder(CAP_NAME_FRAME);
	}

	t.baseline.x = (STATUS_WIDTH >> 1) - RES_SCALE(1);
	t.baseline.y = r.corner.y + RES_SCALE(6);
	t.align = ALIGN_CENTER;
	t.pStr = GLOBAL_SIS(CommanderName);
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(CAPTAIN_NAME_TEXT_COLOR);
	font_DrawText(&t);

	SetContextForeGroundColor(OldColor);
	SetContextFont(OldFont);
	SetContext(OldContext);
}

void DrawFlagshipName(bool InStatusArea, bool NewGame)
{
	GFXRECT r, rHD;
	TEXT t;
	FONT OldFont;
	Color OldColor;
	GFXCONTEXT OldContext;
	FRAME OldFontEffect;
	char buf[250];

	OldFontEffect = SetContextFontEffect(nullptr);
	OldColor = SetContextForeGroundColor(FLAGSHIP_NAME_BACKGROUND_COLOR);

	if (InStatusArea)
	{
		OldContext = SetContext(StatusContext);
		OldFont = SetContextFont(StarConFont);

		r.corner.x = RES_SCALE(2);
		r.corner.y = RES_SCALE(20);
		r.extent.width = SHIP_NAME_WIDTH;
		r.extent.height = SHIP_NAME_HEIGHT;

		t.pStr = GLOBAL_SIS(ShipName);

		DrawFilledRectangle(&r);
	}
	else
	{
		OldContext = SetContext(SpaceContext);
		OldFont = SetContextFont(MicroFont);

		r.corner.x = 0;
		r.corner.y = RES_SCALE(1);
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SHIP_NAME_HEIGHT;

		t.pStr = buf;
		fmt::format_to_sz_n(buf, "{} {}",
							GAME_STRING(NAMING_STRING_BASE + 1), GLOBAL_SIS(ShipName));
		uqm::utf8StringCopy(buf, sizeof buf, uqm::toUpperUtf8(buf).c_str());

		if (IS_HD)
		{
			rHD = r;
			rHD.extent.width *= 0.75;
			rHD.corner.x =
				(r.extent.width >> 1) - (rHD.extent.width >> 1);
			SetContextForeGroundColor(FLAGSHIP_NAME_BACKGROUND_COLOR);
		}

		DrawFilledRectangle(chooseIfHd(&r, &rHD));
	}

	if (!NewGame)
	{
		DrawBorder(SIS_STAT_REPAIR_FRAME);
	}

	t.baseline.x =
		r.corner.x + RES_SCALE(RES_DESCALE(r.extent.width) >> 1);
	t.baseline.y =
		r.corner.y + (SHIP_NAME_HEIGHT - RES_SCALE(InStatusArea));
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFontEffect(SetAbsFrameIndex(FontGradFrame,
											  InStatusArea ? 0 : 3));
	}
	else
	{
		SetContextForeGroundColor(THREEDO_FLAGSHIP_NAME_TEXT_COLOR);
	}

	font_DrawText(&t);

	SetContextFontEffect(OldFontEffect);
	SetContextForeGroundColor(OldColor);
	SetContextFont(OldFont);
	SetContext(OldContext);
}

void DrawFlagshipStats(void)
{
	GFXRECT r;
	TEXT t;
	FONT OldFont;
	Color OldColor;
	FRAME OldFontEffect;
	GFXCONTEXT OldContext;
	char buf[128];
	int16_t leading;
	uint8_t i;
	uint8_t energy_regeneration, energy_wait, turn_wait;
	uint8_t num_dynamos, num_shivas;
	uint16_t max_thrust;
	uint32_t fuel;
	int16_t base_y;

	if (is3DO(uqm::UQMOptions::read().whichFonts) || IS_PAD)
	{
		return;
	}

	/* collect stats */
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 10
#define MAX_THRUST 10
#define TURN_WAIT 17
	energy_regeneration = ENERGY_REGENERATION;
	energy_wait = ENERGY_WAIT;
	max_thrust = MAX_THRUST;
	turn_wait = TURN_WAIT;
	fuel = 10 * FUEL_TANK_SCALE;
	num_dynamos = 0;
	num_shivas = 0;

	for (i = 0; i < NUM_MODULE_SLOTS; i++)
	{
		switch (GLOBAL_SIS(ModuleSlots[i]))
		{
			case FUEL_TANK:
				fuel += FUEL_TANK_CAPACITY;
				break;
			case HIGHEFF_FUELSYS:
				fuel += HEFUEL_TANK_CAPACITY;
				break;
			case DYNAMO_UNIT:
				energy_wait -= 2;
				num_dynamos++;
				if (energy_wait < 4)
				{
					energy_wait = 4;
				}
				break;
			case SHIVA_FURNACE:
				energy_regeneration++;
				num_shivas++;
				break;
		}
	}

	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
	{
		if (GLOBAL_SIS(DriveSlots[i]) == FUSION_THRUSTER)
		{
			max_thrust += 2;
		}
	}

	for (i = 0; i < NUM_JET_SLOTS; ++i)
	{
		if (GLOBAL_SIS(JetSlots[i]) == TURNING_JETS)
		{
			turn_wait -= 2;
		}
	}
	/* END collect stats */

	OldContext = SetContext(SpaceContext);
	OldFont = SetContextFont(StarConFont);
	OldFontEffect = SetContextFontEffect(nullptr);
	GetContextFontLeading(&leading);

	r.corner.x = 0;
	r.corner.y = SIS_SCREEN_HEIGHT - (DOS_BOOL(4, 3) * leading);
	r.extent.width = SIS_SCREEN_WIDTH;
	r.extent.height = (DOS_BOOL(4, 3) * leading);

	OldColor = SetContextForeGroundColor(BLACK_COLOR);
	DrawFilledRectangle(&r);

	/*
	   now that we've cleared out our playground, compensate for the
	   fact that the leading is way more than is generally needed.
	*/
	leading -= RES_SCALE(2);
	base_y = r.corner.y + leading - RES_SCALE(2);

	t.baseline.x = RES_SCALE(ORIG_SIS_SCREEN_WIDTH / 6 + 1);
	t.baseline.y = base_y;
	t.align = ALIGN_RIGHT;
	t.CharCount = (uint16_t)~0;

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, 4));

	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 0); // "nose:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 1); // "spread:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 2); // "side:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 3); // "tail:"
	font_DrawText(&t);

	t.baseline.x += RES_SCALE(3);
	t.baseline.y = base_y;
	t.align = ALIGN_LEFT;
	t.pStr = buf;

	fmt::format_to_sz_n(buf, "{}",
						describeWeapon(GLOBAL_SIS(ModuleSlots[15])));
	font_DrawText(&t);
	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{}",
						describeWeapon(GLOBAL_SIS(ModuleSlots[14])));
	font_DrawText(&t);
	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{}",
						describeWeapon(GLOBAL_SIS(ModuleSlots[13])));
	font_DrawText(&t);
	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{}",
						describeWeapon(GLOBAL_SIS(ModuleSlots[0])));
	font_DrawText(&t);

	t.baseline.x = r.extent.width - RES_SCALE(26);
	t.baseline.y = base_y;
	t.align = ALIGN_RIGHT;

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, 5));

	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 4); // "maximum velocity:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 5); // "turning rate:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 6); // "combat energy:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GAME_STRING(FLAGSHIP_STRING_BASE + 7); // "maximum fuel:"
	font_DrawText(&t);

	t.baseline.x = r.extent.width - RES_SCALE(2);
	t.baseline.y = base_y;
	t.pStr = buf;

	fmt::format_to_sz_n(buf, "{:4}", max_thrust * 4);
	font_DrawText(&t);
	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{:4}", 1 + TURN_WAIT - turn_wait);
	font_DrawText(&t);
	t.baseline.y += leading;
	if (!IS_DOS)
	{
		unsigned int energy_per_10_sec =
			(((100 * GameTicksPerSecond * energy_regeneration) / ((1 + energy_wait) * BattleFrameRateTicks)) + 5) / 10;
		fmt::format_to_sz_n(buf, "{:2}.{:1}",
							energy_per_10_sec / 10, energy_per_10_sec % 10);
	}
	else
	{
		fmt::format_to_sz_n(buf, "{}",
							(num_dynamos * 30) + (num_shivas * 60));
	}
	font_DrawText(&t);
	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{:4}", (fuel / FUEL_TANK_SCALE));
	font_DrawText(&t);

	SetContextFontEffect(OldFontEffect);
	SetContextForeGroundColor(OldColor);
	SetContextFont(OldFont);
	SetContext(OldContext);
}

static const char*
describeWeapon(uint8_t moduleType)
{
	switch (moduleType)
	{
		case GUN_WEAPON:
			return GAME_STRING(FLAGSHIP_STRING_BASE + 8); // "gun"
		case BLASTER_WEAPON:
			return GAME_STRING(FLAGSHIP_STRING_BASE + 9); // "blaster"
		case CANNON_WEAPON:
			return GAME_STRING(FLAGSHIP_STRING_BASE + 10); // "cannon"
		case BOMB_MODULE_0:
		case BOMB_MODULE_1:
		case BOMB_MODULE_2:
		case BOMB_MODULE_3:
		case BOMB_MODULE_4:
		case BOMB_MODULE_5:
			return GAME_STRING(FLAGSHIP_STRING_BASE + 11); // "n/a"
		default:
			return GAME_STRING(FLAGSHIP_STRING_BASE + 12); // "none"
	}
}

void DrawLanders(void)
{
	uint8_t i;
	int16_t width;
	GFXRECT r;
	STAMP s;
	GFXCONTEXT OldContext;

	OldContext = SetContext(StatusContext);

	s.frame = IncFrameIndex(FlagStatFrame);
	GetFrameRect(s.frame, &r);

	i = GLOBAL_SIS(NumLanders);
	r.corner.x = (STATUS_WIDTH >> 1) - r.corner.x;
	s.origin.x = r.corner.x
			   - RES_SCALE(
					 RES_DESCALE((r.extent.width * i) + (RES_SCALE(2) * (i - 1))) >> 1);
	s.origin.y = RES_SCALE(29);

	width = r.extent.width + RES_SCALE(2);
	r.extent.width = (r.extent.width * MAX_LANDERS)
				   + (RES_SCALE(2) * (MAX_LANDERS - 1)) + RES_SCALE(2);
	r.corner.x -= RES_SCALE(RES_DESCALE(r.extent.width) >> 1);
	r.corner.y += s.origin.y;
	SetContextForeGroundColor(BLACK_COLOR);
	DrawFilledRectangle(&r);
	while (i--)
	{
		DrawStamp(&s);
		s.origin.x += width;
	}

	SetContext(OldContext);
}

// Draw the storage bays, below the picture of the flagship.
void DrawStorageBays(bool Refresh)
{
	uint8_t i;
	GFXRECT r;
	GFXCONTEXT OldContext;

	OldContext = SetContext(StatusContext);

	r.extent.width = RES_SCALE(2);
	r.extent.height = RES_SCALE(4);
	r.corner.y = RES_SCALE(123);
	if (Refresh)
	{
		r.extent.width = NUM_MODULE_SLOTS * (r.extent.width + 1);
		r.corner.x = (STATUS_WIDTH >> 1) - (r.extent.width >> 1);

		SetContextForeGroundColor(BLACK_COLOR);
		DrawFilledRectangle(&r);
		r.extent.width = RES_SCALE(2);
	}

	i = (uint8_t)CountSISPieces(STORAGE_BAY);
	if (i)
	{
		uint16_t j;

		r.corner.x = (STATUS_WIDTH >> 1)
				   - RES_SCALE(
						 RES_DESCALE(i * (r.extent.width + RES_SCALE(1)))
						 >> 1);
		SetContextForeGroundColor(STORAGE_BAY_FULL_COLOR);
		for (j = GLOBAL_SIS(TotalElementMass);
			 j >= STORAGE_BAY_CAPACITY; j -= STORAGE_BAY_CAPACITY)
		{
			DrawFilledRectangle(&r);
			r.corner.x += r.extent.width + RES_SCALE(1);

			--i;
		}

		r.extent.height = (RES_SCALE(4) * j + (STORAGE_BAY_CAPACITY - 1))
						/ STORAGE_BAY_CAPACITY;
		if (r.extent.height)
		{
			r.corner.y += RES_SCALE(4) - r.extent.height;
			DrawFilledRectangle(&r);
			r.extent.height = RES_SCALE(4) - r.extent.height;
			if (r.extent.height)
			{
				r.corner.y = RES_SCALE(123);
				SetContextForeGroundColor(STORAGE_BAY_EMPTY_COLOR);
				DrawFilledRectangle(&r);
			}
			r.corner.x += r.extent.width + RES_SCALE(1);

			--i;
		}
		r.extent.height = RES_SCALE(4);

		SetContextForeGroundColor(STORAGE_BAY_EMPTY_COLOR);
		while (i--)
		{
			DrawFilledRectangle(&r);
			r.corner.x += r.extent.width + RES_SCALE(1);
		}
	}

	SetContext(OldContext);
}

void GetGaugeRect(GFXRECT* pRect, bool IsCrewRect)
{
	pRect->extent.width = RES_SCALE(24);
	pRect->corner.x = (STATUS_WIDTH >> 1) - (pRect->extent.width >> 1);
	pRect->extent.height = RES_SCALE(5);
	pRect->corner.y = IsCrewRect ? RES_SCALE(117) : RES_SCALE(38);
}

//static void
//DrawPC_SIS (void)
//{
//	TEXT t;
//	GFXRECT r;
//
//	GetGaugeRect (&r, false);
//	t.baseline.x = (STATUS_WIDTH >> 1);
//	t.baseline.y = r.corner.y - RES_SCALE (1);
//	t.align = ALIGN_CENTER;
//	t.CharCount = (uint16_t)~0;
//	SetContextFont (TinyFont);
//	SetContextForeGroundColor (BLACK_COLOR);
//
//	// Black rectangle behind "FUEL" text and fuel amount.
//	r.corner.y -= RES_SCALE (6);
//	r.corner.x -= RES_SCALE (1);
//	r.extent.width += RES_SCALE (2);
//	DrawFilledRectangle (&r);
//
//	SetContextFontEffect (SetAbsFrameIndex (FontGradFrame, 1));
//	t.pStr = GAME_STRING (STATUS_STRING_BASE + 3); // "FUEL"
//	font_DrawText (&t);
//
//	// Black rectangle behind "CREW" text and crew amount.
//	r.corner.y += RES_SCALE (79);
//	t.baseline.y += RES_SCALE (79);
//	DrawFilledRectangle (&r);
//
//	SetContextFontEffect (SetAbsFrameIndex (FontGradFrame, 2));
//	t.pStr = GAME_STRING (STATUS_STRING_BASE + 4); // "CREW"
//	font_DrawText (&t);
//	SetContextFontEffect (nullptr);
//
//	// Background of text "CAPTAIN".
//	r.corner.x = RES_SCALE (2 + 1);
//	r.corner.y = RES_SCALE (3);
//	r.extent.width = RES_SCALE (58);
//	r.extent.height = RES_SCALE (7);
//	SetContextForeGroundColor (PC_CAPTAIN_STRING_BACKGROUND_COLOR);
//	DrawFilledRectangle (&r);
//
//	DrawBorder (CAPTAIN_FRAME);
//
//	// Text "CAPTAIN".
//	SetContextForeGroundColor (PC_CAPTAIN_STRING_TEXT_COLOR);
//	t.baseline.y = r.corner.y + RES_SCALE (6);
//	t.baseline.x -= RES_SCALE (1);
//	t.pStr = GAME_STRING (STATUS_STRING_BASE + 5); // "CAPTAIN"
//	font_DrawText (&t);
//}

static void
Draw_SIS(void)
{
	TEXT t;
	GFXRECT r;
	bool flat = (bool)is3DO(uqm::UQMOptions::read().whichFonts);

	GetGaugeRect(&r, false);
	t.baseline.x = (STATUS_WIDTH >> 1);
	t.baseline.y = r.corner.y - RES_SCALE(1);
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	SetContextFont(flat ? TinyFontBold : TinyFont);
	SetContextForeGroundColor(BLACK_COLOR);

	// Black rectangle behind "FUEL" text and fuel amount.
	r.corner.y -= RES_SCALE(6);
	r.corner.x -= RES_SCALE(1);
	r.extent.width += RES_SCALE(2);
	DrawFilledRectangle(&r);

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, flat ? 10 : 1));
	t.pStr = GAME_STRING(STATUS_STRING_BASE + (flat ? 17 : 3)); // "FUEL"
	font_DrawText(&t);

	// Black rectangle behind "CREW" text and crew amount.
	r.corner.y += RES_SCALE(79);
	t.baseline.y += RES_SCALE(79);
	DrawFilledRectangle(&r);

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, flat ? 11 : 2));
	t.pStr = GAME_STRING(STATUS_STRING_BASE + (flat ? 18 : 4)); // "CREW"
	font_DrawText(&t);
	SetContextFontEffect(nullptr);

	// Background of text "CAPTAIN".
	r.corner.x = RES_SCALE(2 + 1);
	r.corner.y = RES_SCALE(3);
	r.extent.width = RES_SCALE(58);
	r.extent.height = RES_SCALE(7);
	SetContextForeGroundColor(PC_CAPTAIN_STRING_BACKGROUND_COLOR);
	DrawFilledRectangle(&r);

	DrawBorder(CAPTAIN_FRAME);

	// Text "CAPTAIN".
	SetContextForeGroundColor(flat ? THREEDO_CAPTAIN_STRING_TEXT_COLOR : PC_CAPTAIN_STRING_TEXT_COLOR);
	t.baseline.y = r.corner.y + RES_SCALE(6);
	t.baseline.x -= RES_SCALE(1);
	t.pStr = GAME_STRING(STATUS_STRING_BASE + (flat ? 19 : 5));
	// "CAPTAIN"
	font_DrawText(&t);
}

static void
DrawThrusters(void)
{
	STAMP s;
	uint16_t i;

	s.origin.x = RES_SCALE(1);
	s.origin.y = 0;
	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
	{
		uint8_t which_piece = GLOBAL_SIS(DriveSlots[i]);
		if (which_piece < EMPTY_SLOT)
		{
			s.frame = SetAbsFrameIndex(FlagStatFrame, which_piece + 1 + 0);
			DrawStamp(&s);
			s.frame = IncFrameIndex(s.frame);
			DrawStamp(&s);
		}

		s.origin.y -= RES_SCALE(3);
	}
}

static void
DrawTurningJets(void)
{
	STAMP s;
	uint16_t i;

	s.origin.x = RES_SCALE(1);
	s.origin.y = 0;
	for (i = 0; i < NUM_JET_SLOTS; ++i)
	{
		uint8_t which_piece = GLOBAL_SIS(JetSlots[i]);
		if (which_piece < EMPTY_SLOT)
		{
			s.frame = SetAbsFrameIndex(FlagStatFrame, which_piece + 1 + 1);
			DrawStamp(&s);
			s.frame = IncFrameIndex(s.frame);
			DrawStamp(&s);
		}

		s.origin.y -= RES_SCALE(3);
	}
}

static void
DrawModules(void)
{
	STAMP s;
	uint16_t i;

	// This properly centers the modules.
	s.origin.x = RES_SCALE(1);
	s.origin.y = RES_SCALE(1);
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
	{
		uint8_t which_piece = GLOBAL_SIS(ModuleSlots[i]);
		if (which_piece < EMPTY_SLOT)
		{
			s.frame = SetAbsFrameIndex(FlagStatFrame, which_piece + 1 + 2);
			DrawStamp(&s);
		}

		s.origin.y -= RES_SCALE(3);
	}
}

static void
DrawSupportShips(void)
{
	HSHIPFRAG hStarShip;
	HSHIPFRAG hNextShip;
	const GFXPOINT* pship_pos;
	const GFXPOINT ship_pos[MAX_BUILT_SHIPS] =
		{
			SUPPORT_SHIP_PTS};

	for (hStarShip = GetHeadLink(&GLOBAL(built_ship_q)),
		pship_pos = ship_pos;
		 hStarShip; hStarShip = hNextShip, ++pship_pos)
	{
		SHIP_FRAGMENT* StarShipPtr;
		STAMP s;

		StarShipPtr = LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
		hNextShip = _GetSuccLink(StarShipPtr);

		s.origin = MAKE_POINT(
			RES_SCALE(pship_pos->x), RES_SCALE(pship_pos->y));
		s.frame = SetAbsFrameIndex(StarShipPtr->icons, 0);
		DrawStamp(&s);

		UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
	}
}

static void
DeltaSISGauges_crewDelta(int16_t crew_delta)
{
	if (crew_delta == 0)
	{
		return;
	}

	if (crew_delta != UNDEFINED_DELTA)
	{
		uint16_t CrewCapacity;

		if (crew_delta < 0
			&& GLOBAL_SIS(CrewEnlisted) <= (uint16_t)-crew_delta)
		{
			GLOBAL_SIS(CrewEnlisted) = 0;
		}
		else
		{
			GLOBAL_SIS(CrewEnlisted) += crew_delta;
			CrewCapacity = GetCrewPodCapacity();
			if (GLOBAL_SIS(CrewEnlisted) > CrewCapacity)
			{
				GLOBAL_SIS(CrewEnlisted) = CrewCapacity;
			}
		}
	}

	{
		TEXT t;
		char buf[60];
		GFXRECT r;

		fmt::format_to_sz_n(buf, "{}", GLOBAL_SIS(CrewEnlisted));

		GetGaugeRect(&r, true);

		t.baseline.x = (STATUS_WIDTH >> 1);
		t.baseline.y = r.corner.y + r.extent.height;
		t.align = ALIGN_CENTER;
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;

		SetContextForeGroundColor(BLACK_COLOR);
		DrawFilledRectangle(&r);
		SetContextForeGroundColor(
			BUILD_COLOR(MAKE_RGB15(0x00, 0x0E, 0x00), 0x6C));
		font_DrawText(&t);
	}
}

static void
DeltaSISGauges_fuelDelta(int32_t fuel_delta)
{
	uint32_t OldCoarseFuel;
	uint32_t NewCoarseFuel;

	if (fuel_delta == 0)
	{
		return;
	}

	if (fuel_delta == UNDEFINED_DELTA)
	{
		OldCoarseFuel = (uint32_t)~0;
	}
	else
	{

		OldCoarseFuel = (GLOBAL_SIS(FuelOnBoard) / (!uqm::UQMOptions::read().wholeFuel ? FUEL_TANK_SCALE : 1));
		if (fuel_delta < 0
			&& GLOBAL_SIS(FuelOnBoard) <= (uint32_t)-fuel_delta)
		{
			GLOBAL_SIS(FuelOnBoard) = 0;
		}
		else
		{
			uint32_t FuelCapacity = GetFuelTankCapacity();
			GLOBAL_SIS(FuelOnBoard) += fuel_delta;
			if (GLOBAL_SIS(FuelOnBoard) > FuelCapacity)
			{
				GLOBAL_SIS(FuelOnBoard) = FuelCapacity;
			}
		}
	}

	NewCoarseFuel = (GLOBAL_SIS(FuelOnBoard) / (!uqm::UQMOptions::read().wholeFuel ? FUEL_TANK_SCALE : 1));
	if (NewCoarseFuel != OldCoarseFuel)
	{
		TEXT t;
		// buf from [60] to [7]: The max fuel anyone can ever get is 1610 (1610.00 in whole value)
		// I.E. only 4 (7) characters, we don't need that much extra padding.
		// PragmaNull: make it 8. Why waste 1 byte in padding? Nothing else will use it..
		char buf[8];
		GFXRECT r {};
		formatFuelValue(NewCoarseFuel, {buf, sizeof(buf)});

		GetGaugeRect(&r, false);

		if (uqm::UQMOptions::read().whichFonts == uqm::EmulationMode::Console3DO && !uqm::UQMOptions::read().wholeFuel)
		{
			SetContextFont(TinyFontBold);
		}
		else
		{
			SetContextFont(TinyFont);
		}

		t.baseline.x = (STATUS_WIDTH >> 1);
		t.baseline.y = r.corner.y + r.extent.height;
		t.align = ALIGN_CENTER;
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;

		SetContextForeGroundColor(BLACK_COLOR);
		if (uqm::UQMOptions::read().wholeFuel)
		{
			r.corner.x -= RES_SCALE(1);
			r.extent.width += RES_SCALE(2);
		}
		DrawFilledRectangle(&r);
		SetContextForeGroundColor(
			BUILD_COLOR(MAKE_RGB15(0x13, 0x00, 0x00), 0x2C));
		font_DrawText(&t);
	}
}

static void
DeltaSISGauges_resunitDelta(int16_t resunit_delta)
{
	if (resunit_delta == 0)
	{
		DrawStatusMessage(nullptr);
		return;
	}

	if (resunit_delta != UNDEFINED_DELTA)
	{
		if (resunit_delta < 0
			&& GLOBAL_SIS(ResUnits) <= (uint32_t)-resunit_delta)
		{
			GLOBAL_SIS(ResUnits) = 0;
		}
		else
		{
			GLOBAL_SIS(ResUnits) += resunit_delta;
		}

		assert(curMsgMode == SMM_RES_UNITS);
	}
	else
	{
		GFXRECT r;

		GetStatusMessageRect(&r);
		SetContextForeGroundColor(
			BUILD_COLOR(MAKE_RGB15(0x00, 0x08, 0x00), 0x6E));
		DrawFilledRectangle(&r);
	}

	DrawStatusMessage(nullptr);
}

static bool
isUndefinedDelta(int16_t size, int32_t sdword, int integer)
{

	return ((size && sdword && integer) && (size == UNDEFINED_DELTA || sdword == UNDEFINED_DELTA || integer == UNDEFINED_DELTA));
}

void DeltaSISGauges(int16_t crew_delta, int32_t fuel_delta, int resunit_delta)
{
	GFXCONTEXT OldContext;

	if (crew_delta == 0 && fuel_delta == 0 && resunit_delta == 0)
	{
		return;
	}

	OldContext = SetContext(StatusContext);

	BatchGraphics();
	if (isUndefinedDelta(crew_delta, fuel_delta, resunit_delta))
	{
		STAMP s;
		s.origin.x = 0;
		s.origin.y = 0;

		s.frame = SetAbsFrameIndex(FlagStatFrame, 0);
		DrawStamp(&s);

		if (is3DO(uqm::UQMOptions::read().flagshipColor))
		{
			s.frame = SetAbsFrameIndex(FlagStatFrame, 23);
			DrawStamp(&s);
		}

		DrawBorder(SIS_STAT_FRAME);

		Draw_SIS();

		DrawThrusters();
		DrawTurningJets();
		DrawModules();
		DrawSupportShips();
	}

	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	DeltaSISGauges_crewDelta(crew_delta);
	DeltaSISGauges_fuelDelta(fuel_delta);

	if (isUndefinedDelta(crew_delta, fuel_delta, resunit_delta))
	{
		DrawFlagshipName(true, false);
		DrawCaptainsName(false);
		DrawLanders();
		DrawStorageBays(false);
	}

	DeltaSISGauges_resunitDelta(resunit_delta);

	UnbatchGraphics();

	SetContext(OldContext);
}


////////////////////////////////////////////////////////////////////////////
// Crew
////////////////////////////////////////////////////////////////////////////

// Get the total amount of crew aboard the SIS.
uint16_t
GetCrewCount(void)
{
	return GLOBAL_SIS(CrewEnlisted);
}

// Get the number of crew which fit in a module of a specified type.
uint16_t
GetModuleCrewCapacity(uint8_t moduleType)
{
	if (moduleType == CREW_POD)
	{
		return CREW_POD_CAPACITY;
	}

	return 0;
}

// Gets the amount of crew which currently fit in the ship's crew pods.
uint16_t
GetCrewPodCapacity(void)
{
	uint16_t capacity = 0;
	uint16_t slotI;

	for (slotI = 0; slotI < NUM_MODULE_SLOTS; slotI++)
	{
		uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
		capacity += GetModuleCrewCapacity(moduleType);
	}

	return capacity;
}

// Find the slot number of the crew pod and "seat" number in that crew pod,
// where the Nth crew member would be located.
// If the crew member does not fit, false is returned, and *slotNr and
// *seatNr are unchanged.
static bool
GetCrewPodForCrewMember(uint16_t crewNr, uint16_t* slotNr, uint16_t* seatNr)
{
	uint16_t slotI;
	uint16_t capacity = 0;

	slotI = NUM_MODULE_SLOTS;
	while (slotI--)
	{
		uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
		uint16_t moduleCapacity = GetModuleCrewCapacity(moduleType);

		if (crewNr < capacity + moduleCapacity)
		{
			*slotNr = slotI;
			*seatNr = crewNr - capacity;
			return true;
		}
		capacity += moduleCapacity;
	}

	return false;
}

// Get the point where to draw the next crew member,
// set the foreground color to the color for that crew member,
// and return GetCrewPodCapacity ().
// TODO: Split of the parts of this function into separate functions.
uint16_t
GetCPodCapacity(GFXPOINT* ppt)
{
	uint16_t crewCount;
	uint16_t slotNr;
	uint16_t seatNr;

	uint16_t rowNr;
	uint16_t colNr;

	static const Color crewRows[] = PC_CREW_COLOR_TABLE;

	crewCount = GetCrewCount();
	if (!GetCrewPodForCrewMember(crewCount, &slotNr, &seatNr))
	{
		// Crew does not fit. *ppt is unchanged.
		return GetCrewPodCapacity();
	}

	rowNr = seatNr / CREW_PER_ROW;
	colNr = seatNr % CREW_PER_ROW;

	if (uqm::UQMOptions::read().whichFonts == uqm::EmulationMode::PC)
	{
		SetContextForeGroundColor(crewRows[rowNr]);
	}
	else
	{
		SetContextForeGroundColor(THREEDO_CREW_COLOR);
	}

	ppt->x = RES_SCALE(27) + (slotNr * SHIP_PIECE_OFFSET)
		   - RES_SCALE(colNr * 2) - SAFE_PAD;
	ppt->y = RES_SCALE(34 - rowNr * 2) + IF_HD(2);

	return GetCrewPodCapacity();
}


////////////////////////////////////////////////////////////////////////////
// Storage bays
////////////////////////////////////////////////////////////////////////////

// Get the total amount of minerals aboard the SIS.
static uint16_t
GetElementMass(void)
{
	return GLOBAL_SIS(TotalElementMass);
}

// Get the number of crew which fit in a module of a specified type.
uint16_t
GetModuleStorageCapacity(uint8_t moduleType)
{
	if (moduleType == STORAGE_BAY)
	{
		return STORAGE_BAY_CAPACITY;
	}

	return 0;
}

// Gets the amount of minerals which currently fit in the ship's storage.
uint16_t
GetStorageBayCapacity(void)
{
	uint16_t capacity = 0;
	uint16_t slotI;

	for (slotI = 0; slotI < NUM_MODULE_SLOTS; slotI++)
	{
		uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
		capacity += GetModuleStorageCapacity(moduleType);
	}

	return capacity;
}

// Find the slot number of the storage bay and "storage cell" number in that
// storage bay, where the N-1th mineral unit would be located.
// If the mineral unit does not fit, false is returned, and *slotNr and
// *cellNr are unchanged.
static bool
GetStorageCellForMineralUnit(uint16_t unitNr, uint16_t* slotNr, uint16_t* cellNr)
{
	uint16_t slotI;
	uint16_t capacity = 0;

	slotI = NUM_MODULE_SLOTS;
	while (slotI--)
	{
		uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
		uint16_t moduleCapacity = GetModuleStorageCapacity(moduleType);

		if (unitNr <= capacity + moduleCapacity)
		{
			*slotNr = slotI;
			*cellNr = unitNr - capacity;
			return true;
		}
		capacity += moduleCapacity;
	}

	return false;
}

// Get the point where to draw the next mineral unit,
// set the foreground color to the color for that mineral unit,
// and return GetStorageBayCapacity ().
// TODO: Split of the parts of this function into separate functions.
uint16_t
GetSBayCapacity(GFXPOINT* ppt)
{
	uint16_t massCount;
	uint16_t slotNr;
	uint16_t cellNr;

	uint16_t rowNr;
	// uint16_t colNr; Unused

	static const Color colorBars[] = STORAGE_BAY_COLOR_TABLE;

	massCount = GetElementMass();
	if (!GetStorageCellForMineralUnit(massCount, &slotNr, &cellNr))
	{
		// Mineral does not fit. *ppt is unchanged.
		return GetStorageBayCapacity();
	}

	rowNr = cellNr / SBAY_MASS_PER_ROW;
	// colNr = cellNr % SBAY_MASS_PER_ROW; Unused

	if (rowNr == 0)
	{
		SetContextForeGroundColor(BLACK_COLOR);
	}
	else
	{
		rowNr--;
		SetContextForeGroundColor(colorBars[rowNr]);
	}

	ppt->x = RES_SCALE(19) + (slotNr * SHIP_PIECE_OFFSET) - SAFE_PAD;
	ppt->y = RES_SCALE(34 - (rowNr * 2)) + IF_HD(2);

	return GetStorageBayCapacity();
}


////////////////////////////////////////////////////////////////////////////
// Fuel tanks
////////////////////////////////////////////////////////////////////////////

// Get the total amount of fuel aboard the SIS.
static uint32_t
GetFuelTotal(void)
{
	return GLOBAL_SIS(FuelOnBoard);
}

// Get the amount of fuel which fits in a module of a specified type.
uint32_t
GetModuleFuelCapacity(uint8_t moduleType)
{
	if (moduleType == FUEL_TANK)
	{
		return FUEL_TANK_CAPACITY;
	}

	if (moduleType == HIGHEFF_FUELSYS)
	{
		return HEFUEL_TANK_CAPACITY;
	}

	return 0;
}

// Gets the amount of fuel which currently fits in the ship's fuel tanks.
uint32_t
GetFuelTankCapacity(void)
{
	uint32_t capacity = FUEL_RESERVE;
	uint16_t slotI;

	for (slotI = 0; slotI < NUM_MODULE_SLOTS; slotI++)
	{
		uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
		capacity += GetModuleFuelCapacity(moduleType);
	}

	return capacity;
}

// Find the slot number of the fuel cell and "compartment" number in that
// crew pod, where the Nth unit of fuel would be located.
// If the unit does not fit, false is returned, and *slotNr and
// *compartmentNr are unchanged.
// Pre: unitNr >= FUEL_RESERER
static bool
GetFuelTankForFuelUnit(uint32_t unitNr, uint16_t* slotNr, uint32_t* compartmentNr)
{
	uint16_t slotI;
	uint32_t capacity = FUEL_RESERVE;

	assert(unitNr >= FUEL_RESERVE);

	slotI = NUM_MODULE_SLOTS;
	while (slotI--)
	{
		uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);

		capacity += GetModuleFuelCapacity(moduleType);
		if (unitNr < capacity)
		{
			*slotNr = slotI;
			*compartmentNr = capacity - unitNr;
			return true;
		}
	}

	return false;
}

// Get the point where to draw the next fuel unit, set the foreground color
// to the color for that unit, and return GetFuelTankCapacity ().
static uint32_t
GetFTankScreenPos(GFXPOINT* ppt)
{
	uint32_t fuelAmount;
	uint16_t slotNr;
	uint32_t compartmentNr;
	uint8_t moduleType;
	uint32_t volume;

	uint32_t rowNr;

	static const Color fuelColors[] = FUEL_COLOR_TABLE;

	fuelAmount = GetFuelTotal();
	if (fuelAmount < FUEL_RESERVE)
	{
		// Fuel is in the SIS reserve, not in a fuel tank.
		// *ppt is unchanged
		return 0;
	}

	if (!GetFuelTankForFuelUnit(fuelAmount, &slotNr, &compartmentNr))
	{
		// Fuel does not fit. *ppt is unchanged.
		return 0;
	}

	moduleType = GLOBAL_SIS(ModuleSlots[slotNr]);
	volume = GetModuleFuelCapacity(moduleType);

	rowNr = ((volume - compartmentNr) * MAX_FUEL_BARS
			 / HEFUEL_TANK_CAPACITY);

	ppt->x = RES_SCALE(21) + (slotNr * SHIP_PIECE_OFFSET)
		   + IF_HD(OutfitOrShipyard == 2 ? 0 : 2) - SAFE_PAD;
	if (volume == FUEL_TANK_CAPACITY)
	{
		ppt->y = RES_SCALE(27 - rowNr);
	}
	else
	{
		ppt->y = RES_SCALE(30 - rowNr);
	}

	assert(rowNr + 1 < (uint16_t)std::size(fuelColors));
	SetContextForeGroundColor(fuelColors[rowNr]);
	SetContextBackGroundColor(fuelColors[rowNr + 1]);

	return volume;
}


///////////////////////////////////////////////////////////////////////////

uint16_t
CountSISPieces(uint8_t piece_type)
{
	uint16_t i, num_pieces;

	num_pieces = 0;
	if (piece_type == FUSION_THRUSTER)
	{
		for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
		{
			if (GLOBAL_SIS(DriveSlots[i]) == piece_type)
			{
				++num_pieces;
			}
		}
	}
	else if (piece_type == TURNING_JETS)
	{
		for (i = 0; i < NUM_JET_SLOTS; ++i)
		{
			if (GLOBAL_SIS(JetSlots[i]) == piece_type)
			{
				++num_pieces;
			}
		}
	}
	else
	{
		for (i = 0; i < NUM_MODULE_SLOTS; ++i)
		{
			if (GLOBAL_SIS(ModuleSlots[i]) == piece_type)
			{
				++num_pieces;
			}
		}
	}

	return num_pieces;
}

#define MAX_CLUSTER 45 // "Epsilon Camelopardalis" x 2 + null terminator

static void
AutoPilotTextLogic(void)
{
	char buf[PATH_MAX];
	char star_cluster[MAX_CLUSTER];
	GFXPOINT Falayalaralfali;
	GFXPOINT destination;
	GFXPOINT current_position;
	STAR_DESC* StarPointer;
	double target_distance;
	TEXT temp;
	GFXRECT r;

	if (GLOBAL_SIS(FuelOnBoard) == 0)
	{
		DrawSISMessageEx(
			GAME_STRING(NAVIGATION_STRING_BASE + 2),
			-1, -1, DSME_MYCOLOR); // "OUT OF FUEL"
		return;
	}

	if (!uqm::UQMOptions::read().smartAutoPilot)
	{
		DrawSISMessageEx(
			GAME_STRING(NAVIGATION_STRING_BASE + 3),
			-1, -1, DSME_MYCOLOR); // "AUTO-PILOT"
		return;
	}

	// Show destination and distance to destination
	Falayalaralfali.x = ARILOU_HOME_X;
	Falayalaralfali.y = ARILOU_HOME_Y;
	current_position.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
	current_position.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
	destination = GLOBAL(autopilot);
	target_distance = ptDistance(current_position, destination) / 10;
	StarPointer = FindStar(nullptr, &destination, 1, 1);

	if (inQuasiSpace() && (!pointsEqual(destination, Falayalaralfali) || (pointsEqual(destination, Falayalaralfali) && !(GET_GAME_STATE(KNOW_QS_PORTAL) & (1 << 15)))))
	{
		StarPointer = nullptr;
	}

	if (!StarPointer)
	{ // Show the destination coordinates if the
		// destination is not a star
		// AUTO-PILOT to ###.#:###.# - [TargetDistance]
		fmt::format_to_sz_n(buf, "{} {} {:03}.{:01}:{:03}.{:01} - {:.1}",
							GAME_STRING(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
							GAME_STRING(NAVIGATION_STRING_BASE + 6), // "to"
							destination.x / 10, destination.x % 10,	 // X Coordinates
							destination.y / 10, destination.y % 10,	 // Y Coordinates
							target_distance);

		DrawSISMessageEx(buf, -1, -1, DSME_MYCOLOR);
		return;
	}

	if (pointsEqual(LoadLastLoc(), destination))
	{
		fmt::format_to_sz_n(buf, "{} {} {}",
							GAME_STRING(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
							GAME_STRING(NAVIGATION_STRING_BASE + 6), // "to"
							GAME_STRING(NAVIGATION_STRING_BASE)		 // "HyperSpace"
		);

		DrawSISMessageEx(buf, -1, -1, DSME_MYCOLOR);
		return;
	}

	GetClusterName(StarPointer, star_cluster);

	// AUTO-PILOT to [StarCluster] - [TargetDistance]
	fmt::format_to_sz_n(buf, "{} {} {} - {:.1}",
						GAME_STRING(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
						GAME_STRING(NAVIGATION_STRING_BASE + 6), // "to"
						star_cluster,
						target_distance);

	temp.pStr = buf;
	r = font_GetTextRect(&temp);

	if (r.extent.width > SIS_MESSAGE_WIDTH)
	{ // If the full text is too large then
		// use "->" instead of "AUTO-PILOT"
		// -> to [StarCluster] - [TargetDistance]
		fmt::format_to_sz_n(buf, "{} {} - {:.1}",
							GAME_STRING(NAVIGATION_STRING_BASE + 7), // "->"
							star_cluster,
							target_distance);
	}

	temp.pStr = buf;
	r = font_GetTextRect(&temp);

	if (r.extent.width > SIS_MESSAGE_WIDTH)
	{ // If shortened text is *still* too
		// large then just show distance
		fmt::format_to_sz_n(buf, "{} - {:.1}",
							GAME_STRING(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
							target_distance);
	}

	DrawSISMessageEx(buf, -1, -1, DSME_MYCOLOR);
}

void DrawAutoPilotMessage(bool Reset)
{
	static bool LastPilot = false;
	static TimeCount NextTime = 0;
	static uint32_t cycle_index = 0;
	bool OnAutoPilot;

	static const Color cycle_tab[] = AUTOPILOT_COLOR_CYCLE_TABLE;
	const size_t cycleCount = std::size(cycle_tab);
#define BLINK_RATE (GameTicksPerSecond * 3 / 40) // 9 @ 120 ticks/second

	if (Reset || uqm::UQMOptions::read().bubbleWarp)
	{ // Just a reset, not drawing
		LastPilot = false;
		return;
	}

	OnAutoPilot = (GLOBAL(autopilot.x) != ~0
				   && GLOBAL(autopilot.y) != ~0)
			   || GLOBAL_SIS(FuelOnBoard) == 0;

	if (OnAutoPilot || LastPilot)
	{
		if (!OnAutoPilot)
		{ // AutoPilot aborted -- clear the AUTO-PILOT message
			DrawSISMessage(nullptr);
			cycle_index = 0;

			if (EXTENDED)
			{
				ZeroLastLoc();
			}
		}
		else if (GetTimeCounter() >= NextTime)
		{
			if (!(GLOBAL(CurrentActivity) & CHECK_ABORT)
				&& GLOBAL_SIS(CrewEnlisted) != (uint16_t)~0)
			{
				GFXCONTEXT OldContext;

				OldContext = SetContext(OffScreenContext);
				SetContextForeGroundColor(cycle_tab[cycle_index]);

				AutoPilotTextLogic();

				SetContext(OldContext);
			}

			cycle_index = (cycle_index + 1) % cycleCount;
			NextTime = GetTimeCounter() + BLINK_RATE;
		}

		LastPilot = OnAutoPilot;
	}
}

// Kruzen: The caller should set the context correctly and batch graphics
void DrawFuelInFTanks(bool isOutfit)
{
	GFXRECT r;
	const uint32_t FuelVolume = GLOBAL_SIS(FuelOnBoard);
	uint32_t capacity = GetFuelTankCapacity();
	uint32_t volume;
	Color c;

	if (isOutfit)
	{
		c = BUILD_COLOR(MAKE_RGB15(0x0B, 0x00, 0x00), 0x2E);
	}
	else
	{
		c = BLACK_COLOR;
	}

	r.extent.height = RES_SCALE(1);

	// Loop through all the rows to draw
	for (GLOBAL_SIS(FuelOnBoard) = FUEL_RESERVE;
		 GLOBAL_SIS(FuelOnBoard) < capacity;)
	{
		if (IS_HD && hdFuelFrame)
		{
			volume = GetFTankScreenPos(&r.corner);

			if (GLOBAL_SIS(FuelOnBoard) < FuelVolume)
			{
				STAMP s;

				s.origin.x = r.corner.x;
				s.origin.y = 0;
				s.frame = SetAbsFrameIndex(hdFuelFrame,
										   volume > 5000 ? 0 : 1);
				DrawStamp(&s);

				GLOBAL_SIS(FuelOnBoard) += volume;

				if (GLOBAL_SIS(FuelOnBoard) > FuelVolume)
				{ // this tank is not full, draw rect on top
					r.extent.width = RES_SCALE(5);
					r.extent.height =
						(GLOBAL_SIS(FuelOnBoard) - FuelVolume)
						/ (FUEL_VOLUME_PER_ROW >> 2);
					r.corner.y = -(GetFrameHot(s.frame).y);
					SetContextForeGroundColor(c);
					DrawFilledRectangle(&r);
				}
			}
			else
			{
				r.extent.width = RES_SCALE(5);
				r.extent.height = (volume > 5000 ? 40 : 20);
				r.corner.y -= r.extent.height - 4;
				// 1 bar lower because GetFTankScreenPos() doesn't
				// return exact corner
				SetContextForeGroundColor(c);
				DrawFilledRectangle(&r);
				GLOBAL_SIS(FuelOnBoard) += volume;
			}
		}
		else
		{
			GetFTankScreenPos(&r.corner);
			if (GLOBAL_SIS(FuelOnBoard) < FuelVolume)
			{ // If we're less than the fuel level, draw fuel.
				r.extent.width = RES_SCALE(5);
				DrawFilledRectangle(&r);

				r.extent.width = RES_SCALE(3);
				r.corner.x += RES_SCALE(1);

				SetContextForeGroundColor(
					SetContextBackGroundColor(BLACK_COLOR));
			}
			else
			{ // Otherwise, draw an empty bar.
				r.extent.width = RES_SCALE(5);
				SetContextForeGroundColor(c);
			}
			DrawFilledRectangle(&r);
			GLOBAL_SIS(FuelOnBoard) += FUEL_VOLUME_PER_ROW;
		}
	}
	GLOBAL_SIS(FuelOnBoard) = FuelVolume;
}

#define MAX_NUM_RECTS 5 // 5 flashing rects at once should be enough
#define NUM_RECTS 1

static FlashContext* flashContext[MAX_NUM_RECTS] =
	{nullptr, nullptr, nullptr, nullptr, nullptr};
static GFXRECT flash_rect[MAX_NUM_RECTS];
static Alarm* flashAlarm = nullptr;
static bool flashPaused = false;
static uint8_t count_r = NUM_RECTS;

static void scheduleFlashAlarm(void);

static void
updateFlashRect(void* arg)
{
	uint16_t i;

	if (flashContext[0] == nullptr)
	{
		return;
	}

	for (i = 0; i < count_r; i++)
	{
		Flash_process(flashContext[i]);
	}
	scheduleFlashAlarm();
	(void)arg;
}

static void
scheduleFlashAlarm(void)
{
	TimeCount nextTime = Flash_nextTime(flashContext[0]);
	uint32_t nextTimeMs = (nextTime / GameTicksPerSecond) * 1000 + ((nextTime % GameTicksPerSecond) * 1000 / GameTicksPerSecond);
	// Overflow-safe conversion.
	flashAlarm = Alarm_addAbsoluteMs(nextTimeMs, updateFlashRect, nullptr);
}

void SetAdditionalRect(const GFXRECT* pRect, uint16_t number)
{ // Add new flashing rect (Max 5)
	// Must be called one by one and in incremental order
	if (pRect != nullptr && count_r != MAX_NUM_RECTS)
	{
		GFXRECT clip_r = {
			{0, 0},
			{0, 0}
		  };
		GetContextClipRect(&clip_r);

		flash_rect[number] = *pRect;
		flash_rect[number].corner.x += clip_r.corner.x;
		flash_rect[number].corner.y += clip_r.corner.y;

		if (number == count_r)
		{
			count_r++;
		}
	}
}

void DumpAdditionalRect(void)
{ // Dump all additional rects
	uint16_t i;

	for (i = count_r; i > 0; i--)
	{
		if (flashContext[i] != nullptr)
		{
			Flash_terminate(flashContext[i]);
			flashContext[i] = nullptr;
		}
	}
	count_r = NUM_RECTS;
}

void SetFlashRect(const GFXRECT* pRect, bool pcRect)
{
	GFXRECT clip_r = {
		{0, 0},
		{0, 0}
	  };
	GFXRECT temp_r;
	uint16_t i;

	if (pRect != SFR_MENU_3DO && pRect != SFR_MENU_ANY
		&& pRect != SFR_MENU_NON)
	{ // The caller specified their own flash area, or nullptr (stop flashing).
		GetContextClipRect(&clip_r);
	}
	else
	{
		if ((uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC && pRect != SFR_MENU_ANY)
			|| pRect == SFR_MENU_NON)
		{
			// The player wants PC menus and this flash is not used
			// for a PC menu.
			// Don't flash.
			pRect = 0;
		}
		else
		{
			// The player wants 3DO menus, or the flash is used in both
			// 3DO and PC mode.
			GFXCONTEXT OldContext = SetContext(StatusContext);
			GetContextClipRect(&clip_r);
			pRect = &temp_r;
			temp_r.corner.x = RADAR_X - clip_r.corner.x;
			temp_r.corner.y = RADAR_Y - clip_r.corner.y;
			temp_r.corner.y += DOS_NUM_SCL(10);
			temp_r.extent.width = RADAR_WIDTH;
			temp_r.extent.height = RADAR_HEIGHT;
			temp_r.extent.height -= DOS_NUM_SCL(10);
			SetContext(OldContext);
		}
	}

	if (pRect != 0 && pRect->extent.width != 0)
	{
		// Flash rectangle is not empty, start or continue flashing.
		flash_rect[0] = *pRect;

		flash_rect[0].corner.x += clip_r.corner.x;
		flash_rect[0].corner.y += clip_r.corner.y;

		// Create a new flash context(s).
		for (i = 0; i < count_r; i++)
		{
			if (flashContext[i] == nullptr)
			{
				flashContext[i] = Flash_createHighlight(ScreenContext, &flash_rect[i]);
				Flash_setMergeFactors(flashContext[i], 3, 2, 2);
				Flash_setSpeed(flashContext[i], 0, GameTicksPerSecond / 16, 0, GameTicksPerSecond / 16);
				Flash_setFrameTime(flashContext[i], GameTicksPerSecond / 16);
				Flash_setPulseBox(flashContext[i], pcRect);
				Flash_start(flashContext[i]);
				if (i == (count_r - 1))
				{
					scheduleFlashAlarm();
				}
			}
			else
			{
				Flash_setPulseBox(flashContext[i], pcRect);
				Flash_setRect(flashContext[i], &flash_rect[i]);
			}
		}
	}
	else
	{
		// Flash rectangle is empty. Stop flashing.
		if (flashContext[0] != nullptr)
		{
			Alarm_remove(flashAlarm);
			flashAlarm = 0;

			for (i = 0; i < count_r; i++)
			{
				Flash_terminate(flashContext[i]);
				flashContext[i] = nullptr;
			}
			count_r = NUM_RECTS;
		}
	}
}

uint16_t updateFlashRectRecursion = 0;
// XXX This is necessary at least because DMS_AddEscortShip() calls
// DrawRaceStrings() in an UpdateFlashRect block, which calls
// ClearSISRect(), which calls DrawMenuStateStrings(), which starts its own
// UpdateFlashRect block. This should probably be cleaned up.

void PreUpdateFlashRect(void)
{
	uint16_t i;

	if (flashAlarm)
	{
		updateFlashRectRecursion++;
		if (updateFlashRectRecursion > 1)
		{
			return;
		}
		for (i = 0; i < count_r; i++)
		{
			Flash_preUpdate(flashContext[i]);
		}
	}
}

void PostUpdateFlashRect(void)
{
	uint16_t i;

	if (flashAlarm)
	{
		updateFlashRectRecursion--;
		if (updateFlashRectRecursion > 0)
		{
			return;
		}
		for (i = 0; i < count_r; i++)
		{
			Flash_postUpdate(flashContext[i]);
		}
	}
}

// Stop flashing if flashing is active.
bool PauseFlash(void)
{
	uint8_t i;

	if (flashContext[0] != nullptr)
	{
		if (flashPaused)
		{
			return false;
		}

		Alarm_remove(flashAlarm);
		flashAlarm = 0;
		flashPaused = true;
	}
	for (i = 0; i < count_r; i++)
	{
		if (flashContext[i] != nullptr && Flash_getPulseBox(flashContext[i]))
		{
			Flash_pause(flashContext[i]);
		}
	}

	return true;
}

// Continue flashing after PauseFlash (), if flashing was active.
void ContinueFlash(void)
{
	uint8_t i;

	if (flashPaused)
	{
		for (i = 0; i < count_r; i++) // need to do before setting clock
		{
			if (flashContext[i] != nullptr && Flash_getPulseBox(flashContext[i]))
			{
				Flash_continue(flashContext[i]);
			}
		}

		scheduleFlashAlarm();
		flashPaused = false;
	}
}