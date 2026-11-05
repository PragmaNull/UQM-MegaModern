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

const static char* describeWeapon(uint8_t moduleType);

FRAME hdFuelFrame;

void RepairSISBorder(void)
{
	GFXRECT r;

	GFXCONTEXT OldContext {SetContext(ScreenContext)};

	BatchGraphics();

	// Left border
	r.corner.x = SisOrgX() - ResScale(1);
	r.corner.y = SisOrgY() - ResScale(1);
	r.extent.width = ResScale(1);
	r.extent.height = SisScreenHeight() + ResScale(2);
	SetContextForeGroundColor(SisLeftBorderColor);
	DrawFilledRectangle(&r);

	// Right border
	SetContextForeGroundColor(SisBottomRightBorderColor);
	r.corner.x += (SisScreenWidth() + ResScale(2)) - ResScale(1);
	DrawFilledRectangle(&r);

	// Bottom border
	r.corner.x = SisOrgX() - ResScale(1);
	r.corner.y += (SisScreenHeight() + ResScale(2)) - ResScale(1);
	r.extent.width = SisScreenWidth() + ResScale(1);
	r.extent.height = ResScale(1);
	DrawFilledRectangle(&r);

	DrawBorder(SIS_REPAIR_FRAME);

	UnbatchGraphics();

	SetContext(OldContext);
}

void ClearSISRect(uint8_t ClearFlags)
{
	//GFXRECT r; Unused
	Color OldColor;

	GFXCONTEXT OldContext {SetContext(StatusContext)};
	OldColor = SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08));

	//r.corner.x = ResScale (2);
	//r.extent.width = StatusWidth() - ResScale (4); Unused

	BatchGraphics();
	if (ClearFlags & DrawSisDisplay)
	{
		DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);
	}

	if (ClearFlags & ClearSisRadar)
	{
		DrawMenuStateStrings((uint8_t)~0, 1);
#ifdef NEVER
		r.corner.x = RadarX() - ResScale(1);
		r.corner.y = RadarY() - ResScale(1);
		r.extent.width = RadarWidth() + ResScale(2);
		r.extent.height = RadarHeight() + ResScale(2);

		DrawStarConBox(&r, ResScale(1), ShadowboxMediumColor,
					   ShadowboxDarkColor, true, ScanBiologicalTextColor,
					   false, Transparent);
#endif /* NEVER */
	}
	UnbatchGraphics();

	SetContextForeGroundColor(OldColor);
	SetContext(OldContext);
}

// Draw the SIS title. This is the field at the top of the screen, on the
// right hand side, containing the coordinates in HyperSpace, or the planet
// name in IP.
void DrawSISTitle(const char* pStr)
{
	TEXT t;
	GFXRECT r;
	uqstl::string buf {};

	t.baseline.x = ResScale((ResDescale(SisTitleWidth()) >> 1));
	t.baseline.y = SisTitleHeight() - ResScale(2);
	t.align = ALIGN_CENTER;
	t.pStr = pStr;
	t.CharCount = (uint16_t)~0;

	GFXCONTEXT OldContext {SetContext(OffScreenContext)};
	r.corner.x = SisOrgX() + SisScreenWidth() - SisTitleBoxWidth()
			   + ResScale(1);
	r.corner.y = SisOrgY() - SisTitleHeight();
	r.extent.width = SisTitleWidth();
	r.extent.height = SisTitleHeight() - ResScale(1);
	SetContextFGFrame(Screen);
	SetContextClipRect(&r);
	if (isPC(uqm::UQMOptions::read().whichFonts) || SaveOrLoad)
	{
		SetContextFont(TinyFont);
	}
	else
	{
		buf = pStr;

		SetContextFont(TinyFontBold);
		uqstl::ranges::replace(buf, UnicharSpace, UnicharTab);
		t.pStr = buf.c_str();
		t.CharCount = (uint16_t)~0;
	}

	BatchGraphics();

	// Background color
	SetContextBackGroundColor(SisTitleBackgroundColor);
	ClearDrawable();

	DrawBorder(SIS_TITLE_FRAME);

	// Text color
	SetContextForeGroundColor(SisTitleTextColor);
	font_DrawText(&t);

	UnbatchGraphics();

	SetContextClipRect(nullptr);

	SetContext(OldContext);
}

void DrawHyperCoords(GFXPOINT universe)
{
	char buf[100] {};
	const char* SpaceOrNull = (isPC(uqm::UQMOptions::read().whichFonts) ? StrSpace : "");

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
	char TempVer[SisNameSize] = "";

	if (SisState.SaveVersion > 0)
	{
		uqm::utf8StringCopy(TempVer, sizeof(TempVer),
							GameString(SAVEGAME_STRING_BASE + 4
									   + SisState.SaveVersion));
	}

	DrawSISMessage(TempVer);
	char emptyTitle[] {""};
	DrawSISTitle(emptyTitle);

	if (SisState.Seed)
	{
		if (SisState.Nomad != uqm::NomadMode::Off)
		{
			const static char* const NomadModeSuffix[] {"", "-", "+"};

			fmt::format_to_sz_n(TempNom, "{}{}",
								GameString(MAINMENU_STRING_BASE + 60),
								NomadModeSuffix[static_cast<int>(SisState.Nomad)]);
		}

		if (SisState.Extended)
		{
			uqm::utf8StringCopy(TempExt, sizeof(TempExt),
								GameString(MAINMENU_STRING_BASE + 59));
		}

		uqm::utf8StringCopy(TempDiff, sizeof(TempDiff),
							GameString(MAINMENU_STRING_BASE + 56
									   + static_cast<int>(SisState.Difficulty)));

		fmt::format_to_sz_n(buf, "{} {}{}{}",
							GameString(MAINMENU_STRING_BASE + 55), // Difficulty:
							TempDiff, TempExt, TempNom);
		DrawSISMessage(buf);

		fmt::format_to_sz_n(buf, "{}", SisState.Seed);
		DrawSISTitle(buf);
	}
}

void DrawSISMessage(const char* pStr)
{
	DrawSISMessageEx(pStr, -1, -1, DsmeNone);
}

// See sis.h for the allowed flags. This is the field at the top of the
// screen, on the left hand side.
bool DrawSISMessageEx(const char* pStr, int16_t CurPos, int16_t ExPos,
					  uint16_t flags)
{
	char buf[256];
	TEXT t;
	GFXRECT r;

	GFXCONTEXT OldContext {SetContext(OffScreenContext)};
	// prepare the context
	r.corner.x = SisOrgX() + ResScale(1);
	r.corner.y = SisOrgY() - SisMessageHeight();
	r.extent.width = SisMessageWidth();
	r.extent.height = SisMessageHeight() - ResScale(1);
	SetContextFGFrame(Screen);
	SetContextClipRect(&r);

	BatchGraphics();
	SetContextBackGroundColor(SisMessageBackgroundColor);

	if (pStr == 0)
	{
		switch (uqm::lowByte(uqm::currentActivity()))
		{
			default:
			case IN_ENCOUNTER:
				pStr = "";
				break;
			case IN_LAST_BATTLE:
			case IN_INTERPLANETARY:
				GetClusterName(CurStarDescPtr(), buf);
				pStr = buf;
				break;
			case IN_HYPERSPACE:
				if (inHyperSpace())
				{
					pStr = GameString(NAVIGATION_STRING_BASE);
					// "HyperSpace"
				}
				else
				{
					const GFXPOINT Log = MAKE_POINT(
						LogxToUniverse(GLOBAL_SIS(log_x)),
						LogyToUniverse(GLOBAL_SIS(log_y)));

					pStr = GameString(NAVIGATION_STRING_BASE + 1);
					// "QuasiSpace"

					if (GET_GAME_STATE(ARILOU_HOME_VISITS)
						&& (Log.x == ArilouHomeX
							&& Log.y == ArilouHomeY))
					{
						uqm::utf8StringCopy(
							GLOBAL_SIS(PlanetName),
							sizeof GLOBAL_SIS(PlanetName),
							GameString(STAR_STRING_BASE + 148));
						// "Falayalaralfali"
					}
				}
				break;
		}
	}

	if (!(flags & DsmeMycolor))
	{
		SetContextForeGroundColor(SisMessageTextColor);
	}

	t.baseline.y = SisMessageHeight() - ResScale(2);
	t.baseline.x = ResScale(ResDescale(SisMessageWidth()) >> 1);
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
			replaceChar(buf, UnicharSpace, UnicharTab);

			t.pStr = buf;
			t.CharCount = (uint16_t)~0;
		}
	}

	if (flags & DsmeClearfr)
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
		int i {};
		GFXRECT text_r;
		// XXX: 128 is currently safe, but it would be better to specify
		//   the size to TextRect()
		uint8_t char_deltas[128];
		const uint8_t* pchar_deltas {};

		t.baseline.x = ResScale(3);
		t.align = ALIGN_LEFT;

		TextRect(&t, &text_r, char_deltas);
#if 0
		if (text_r.extent.width + t.baseline.x + ResScale (2)
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
				cur_r.corner.x -= ResScale(1);
			}

			if (flags & DsmeBlockcur)
			{ // Use block cursor for keyboardless systems

				cur_r.corner.y = 0;
				cur_r.extent.height = r.extent.height;

				SetCursorFlashBlock(true);

				if (CurPos == t.CharCount)
				{ // cursor at end-line -- use insertion point
					cur_r.extent.width = ResScale(1);
					cur_r.corner.x -= IfHd(3);
				}
				else if (CurPos + 1 == t.CharCount)
				{ // extra pixel for last char margin
					cur_r.extent.width = (int16_t)*pchar_deltas - IfHd(3);
					cur_r.corner.x += ResScale(1);
				}
				else if (CurPos < ExPos)
				{
					cur_r.extent.width = (int16_t)*pchar_deltas
									   - ResScale(1);
					cur_r.corner.x += ResScale(1);
				}
				else
				{ // normal mid-line char
					cur_r.extent.width = (int16_t)*pchar_deltas;
					cur_r.corner.x += ResScale(1);
				}

				if (cur_r.extent.width >= 200)
				{
					cur_r.extent.width = ResScale(1);
					cur_r.corner.x -= IfHd(3);
				}
				else
				{
					SetContextForeGroundColor(SisMessageCursorColor);
					DrawFilledRectangle(&cur_r);
				}
			}
			else
			{ // Insertion point cursor
				cur_r.corner.y = ResScale(1);
				cur_r.extent.height = r.extent.height - ResScale(2);
				cur_r.extent.width = ResScale(1);

				if (CurPos == t.CharCount)
				{
					text_r.corner.x -= IfHd(3);
				}

				SetCursorFlashBlock(false);
			}

			SetCursorRect(&cur_r, OffScreenContext);
		}

		SetContextForeGroundColor(SisMessageTextColor);

		if (ExPos >= 0 && ExPos < t.CharCount)
		{ // handle extra characters
			t.CharCount = ExPos;
			font_DrawText(&t);

			// print extra chars
			SetContextForeGroundColor(SisMessageExtraTextColor);
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

	if (flags & DsmeSetfr)
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
			fmt::format_to_sz_n(buf, bufLen, "{} {:02}{}{:04}",
								GameString(MONTHS_STRING_BASE + month_index - 1),
								day_index, StrMiddleDot, year_index);
			break;
		case uqm::DateFormat::MM_dd_yyyy: //MM.dd.yyyy  ie: 03.01.2500
			fmt::format_to_sz_n(buf, bufLen, "{:02}{}{:02}{}{:04}", month_index, StrMiddleDot,
								day_index, StrMiddleDot, year_index);
			break;
		case uqm::DateFormat::dd_MMM_yyyy: //dd MMM yyyy ie: 01 Mar 2500
			fmt::format_to_sz_n(buf, bufLen, "{:02} {} {:04}", day_index,
								GameString(MONTHS_STRING_BASE + month_index - 1),
								year_index);
			break;
		case uqm::DateFormat::dd_MM_yyyy: //dd MM yyyy  ie: 01.03.2500
			fmt::format_to_sz_n(buf, bufLen, "{:02}{}{:02}{}{:04}", day_index, StrMiddleDot,
								month_index, StrMiddleDot, year_index);
			break;
	}
}

void GetStatusMessageRect(GFXRECT* r)
{
	r->corner.x = ResScale(2);
	r->corner.y = ResScale(130);
	r->extent.width = StatusMessageWidth();
	r->extent.height = StatusMessageHeight();
}

void DrawStatusMessage(const char* pStr)
{
	GFXRECT r;
	GFXRECT ctxRect;
	TEXT t;
	char buf[128];

	GFXCONTEXT OldContext {SetContext(StatusContext)};
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
	SetContextBackGroundColor(StatusMessageBackgroundColor);
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
										GameString(STATUS_STRING_BASE + 2) :
										StrInfinitySign,				 // "UNLIMITED"
									GameString(STATUS_STRING_BASE + 0)); // "Cr"
			}
			else
			{
				fmt::format_to_sz_n(buf, "{} {}", uqm::makeWord(GET_GAME_STATE(MELNORME_CREDIT0), GET_GAME_STATE(MELNORME_CREDIT1)), GameString(STATUS_STRING_BASE + 0)); // "Cr"
			}
		}
		else if (curMsgMode == SMM_RES_UNITS)
		{
			if (GET_GAME_STATE(CHMMR_BOMB_STATE) >= 2 || uqm::UQMOptions::read().infiniteRU)
			{
				fmt::format_to_sz_n(buf, "{} {}",
									(isPC(uqm::UQMOptions::read().whichMenu) && isPC(uqm::UQMOptions::read().whichFonts)) ?
										GameString(STATUS_STRING_BASE + 2) :
										StrInfinitySign,				 // "UNLIMITED"
									GameString(STATUS_STRING_BASE + 1)); // "RU"
			}
			else
			{
				fmt::format_to_sz_n(buf, "{} {}", GLOBAL_SIS(ResUnits),
									GameString(STATUS_STRING_BASE + 1)); // "RU"
			}
		}
		else
		{ // Just a date
			DateToString(buf, sizeof buf,
						 uqm::gameState().clock().month(),
						 uqm::gameState().clock().day(),
						 uqm::gameState().clock().year());
		}
		pStr = buf;
	}

	t.baseline.x = (StatusMessageWidth() >> 1);
	t.baseline.y = StatusMessageHeight() - ResScale(1);
	t.align = ALIGN_CENTER;
	t.pStr = pStr;
	t.CharCount = (uint16_t)~0;

	{
		Color statusColor = StatusMessageTextColor;

		if (curMsgMode == SMM_WARNING)
		{
			statusColor = StatusMessageWarningTextColor;
		}
		if (curMsgMode == SMM_ALERT)
		{
			statusColor = StatusMessageAlertTextColor;
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
		replaceChar(buf, UnicharSpace, UnicharTab);

		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
	}

	SetContextForeGroundColor(StatusMessageTextColor);
	font_DrawText(&t);
	UnbatchGraphics();

	SetContextClipRect(nullptr);

	SetContext(OldContext);
}

StatMsgMode
SetStatusMessageMode(StatMsgMode newMode)
{
	const StatMsgMode oldMode = curMsgMode;
	curMsgMode = newMode;
	return oldMode;
}

void DrawCaptainsName(bool NewGame)
{
	GFXRECT r;
	TEXT t;
	FONT OldFont {};
	Color OldColor;

	GFXCONTEXT OldContext {SetContext(StatusContext)};
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		OldFont = SetContextFont(TinyFont);
	}
	else
	{
		OldFont = SetContextFont(TinyFontBold);
	}

	OldColor = SetContextForeGroundColor(CaptainNameBackgroundColor);

	r.corner.x = ResScale(2 + 1);
	r.corner.y = ResScale(10);
	r.extent.width = ShipNameWidth() - ResScale(2);
	r.extent.height = ShipNameHeight();
	DrawFilledRectangle(&r);

	if (!NewGame)
	{
		DrawBorder(CAP_NAME_FRAME);
	}

	t.baseline.x = (StatusWidth() >> 1) - ResScale(1);
	t.baseline.y = r.corner.y + ResScale(6);
	t.align = ALIGN_CENTER;
	t.pStr = GLOBAL_SIS(CommanderName);
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(CaptainNameTextColor);
	font_DrawText(&t);

	SetContextForeGroundColor(OldColor);
	SetContextFont(OldFont);
	SetContext(OldContext);
}

void DrawFlagshipName(bool InStatusArea, bool NewGame)
{
	GFXRECT r;
	GFXRECT rHD;
	TEXT t;
	FONT OldFont {};
	Color OldColor;
	GFXCONTEXT OldContext {};
	char buf[250];

	FRAME OldFontEffect {SetContextFontEffect(nullptr)};
	OldColor = SetContextForeGroundColor(FlagshipNameBackgroundColor);

	if (InStatusArea)
	{
		OldContext = SetContext(StatusContext);
		OldFont = SetContextFont(StarConFont);

		r.corner.x = ResScale(2);
		r.corner.y = ResScale(20);
		r.extent.width = ShipNameWidth();
		r.extent.height = ShipNameHeight();

		t.pStr = GLOBAL_SIS(ShipName);

		DrawFilledRectangle(&r);
	}
	else
	{
		OldContext = SetContext(SpaceContext);
		OldFont = SetContextFont(MicroFont);

		r.corner.x = 0;
		r.corner.y = ResScale(1);
		r.extent.width = SisScreenWidth();
		r.extent.height = ShipNameHeight();

		t.pStr = buf;
		fmt::format_to_sz_n(buf, "{} {}",
							GameString(NAMING_STRING_BASE + 1), GLOBAL_SIS(ShipName));
		uqm::utf8StringCopy(buf, sizeof buf, uqm::toUpperUtf8(buf).c_str());

		if (IsHd())
		{
			rHD = r;
			rHD.extent.width *= 0.75;
			rHD.corner.x =
				(r.extent.width >> 1) - (rHD.extent.width >> 1);
			SetContextForeGroundColor(FlagshipNameBackgroundColor);
		}

		DrawFilledRectangle(chooseIfHd(&r, &rHD));
	}

	if (!NewGame)
	{
		DrawBorder(SIS_STAT_REPAIR_FRAME);
	}

	t.baseline.x =
		r.corner.x + ResScale(ResDescale(r.extent.width) >> 1);
	t.baseline.y =
		r.corner.y + (ShipNameHeight() - ResScale(InStatusArea));
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFontEffect(SetAbsFrameIndex(FontGradFrame,
											  InStatusArea ? 0 : 3));
	}
	else
	{
		SetContextForeGroundColor(ThreedoFlagshipNameTextColor);
	}

	font_DrawText(&t);

	SetContextFontEffect(OldFontEffect);
	SetContextForeGroundColor(OldColor);
	SetContextFont(OldFont);
	SetContext(OldContext);
}

static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {10};
static constexpr auto MaxThrust {10};
static constexpr auto TurnWait {17};

void DrawFlagshipStats(void)
{
	GFXRECT r;
	TEXT t;
	FONT OldFont {};
	Color OldColor;
	FRAME OldFontEffect {};
	GFXCONTEXT OldContext {};
	char buf[128];
	int16_t leading {};
	uint8_t i {};
	uint8_t energy_regeneration {};
	uint8_t energy_wait {};
	uint8_t turn_wait {};
	uint8_t num_dynamos {};
	uint8_t num_shivas {};
	uint16_t max_thrust {};
	uint32_t fuel {};
	int16_t base_y {};

	if (is3DO(uqm::UQMOptions::read().whichFonts) || IsPad())
	{
		return;
	}

	/* collect stats */
	energy_regeneration = EnergyRegeneration;
	energy_wait = EnergyWait;
	max_thrust = MaxThrust;
	turn_wait = TurnWait;
	fuel = 10 * FuelTankScale;
	num_dynamos = 0;
	num_shivas = 0;

	for (i = 0; i < NumModuleSlots; i++)
	{
		switch (GLOBAL_SIS(ModuleSlots[i]))
		{
			case FUEL_TANK:
				fuel += FuelTankCapacity;
				break;
			case HIGHEFF_FUELSYS:
				fuel += HefuelTankCapacity;
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

	for (i = 0; i < NumDriveSlots; ++i)
	{
		if (GLOBAL_SIS(DriveSlots[i]) == FUSION_THRUSTER)
		{
			max_thrust += 2;
		}
	}

	for (i = 0; i < NumJetSlots; ++i)
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
	r.corner.y = SisScreenHeight() - (DosBool(4, 3) * leading);
	r.extent.width = SisScreenWidth();
	r.extent.height = (DosBool(4, 3) * leading);

	OldColor = SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	/*
	   now that we've cleared out our playground, compensate for the
	   fact that the leading is way more than is generally needed.
	*/
	leading -= ResScale(2);
	base_y = r.corner.y + leading - ResScale(2);

	t.baseline.x = ResScale(OrigSisScreenWidth() / 6 + 1);
	t.baseline.y = base_y;
	t.align = ALIGN_RIGHT;
	t.CharCount = (uint16_t)~0;

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, 4));

	t.pStr = GameString(FLAGSHIP_STRING_BASE + 0); // "nose:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GameString(FLAGSHIP_STRING_BASE + 1); // "spread:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GameString(FLAGSHIP_STRING_BASE + 2); // "side:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GameString(FLAGSHIP_STRING_BASE + 3); // "tail:"
	font_DrawText(&t);

	t.baseline.x += ResScale(3);
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

	t.baseline.x = r.extent.width - ResScale(26);
	t.baseline.y = base_y;
	t.align = ALIGN_RIGHT;

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, 5));

	t.pStr = GameString(FLAGSHIP_STRING_BASE + 4); // "maximum velocity:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GameString(FLAGSHIP_STRING_BASE + 5); // "turning rate:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GameString(FLAGSHIP_STRING_BASE + 6); // "combat energy:"
	font_DrawText(&t);
	t.baseline.y += leading;
	t.pStr = GameString(FLAGSHIP_STRING_BASE + 7); // "maximum fuel:"
	font_DrawText(&t);

	t.baseline.x = r.extent.width - ResScale(2);
	t.baseline.y = base_y;
	t.pStr = buf;

	fmt::format_to_sz_n(buf, "{:4}", max_thrust * 4);
	font_DrawText(&t);
	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{:4}", 1 + TurnWait - turn_wait);
	font_DrawText(&t);
	t.baseline.y += leading;
	if (!IsDos())
	{
		const unsigned int energy_per_10_sec =
			(((100 * GameTicksPerSecond * energy_regeneration) / ((1 + energy_wait) * BattleFrameRateTicks.count())) + 5) / 10;
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
	fmt::format_to_sz_n(buf, "{:4}", (fuel / FuelTankScale));
	font_DrawText(&t);

	SetContextFontEffect(OldFontEffect);
	SetContextForeGroundColor(OldColor);
	SetContextFont(OldFont);
	SetContext(OldContext);
}

const static char*
describeWeapon(uint8_t moduleType)
{
	switch (moduleType)
	{
		case GUN_WEAPON:
			return GameString(FLAGSHIP_STRING_BASE + 8); // "gun"
		case BLASTER_WEAPON:
			return GameString(FLAGSHIP_STRING_BASE + 9); // "blaster"
		case CANNON_WEAPON:
			return GameString(FLAGSHIP_STRING_BASE + 10); // "cannon"
		case BOMB_MODULE_0:
		case BOMB_MODULE_1:
		case BOMB_MODULE_2:
		case BOMB_MODULE_3:
		case BOMB_MODULE_4:
		case BOMB_MODULE_5:
			return GameString(FLAGSHIP_STRING_BASE + 11); // "n/a"
		default:
			return GameString(FLAGSHIP_STRING_BASE + 12); // "none"
	}
}

void DrawLanders(void)
{
	int16_t width {};
	GFXRECT r;
	STAMP s;

	GFXCONTEXT OldContext {SetContext(StatusContext)};

	s.frame = IncFrameIndex(FlagStatFrame);
	GetFrameRect(s.frame, &r);

	uint8_t i {GLOBAL_SIS(NumLanders)};
	r.corner.x = (StatusWidth() >> 1) - r.corner.x;
	s.origin.x = r.corner.x
			   - ResScale(
					 ResDescale((r.extent.width * i) + (ResScale(2) * (i - 1))) >> 1);
	s.origin.y = ResScale(29);

	width = r.extent.width + ResScale(2);
	r.extent.width = (r.extent.width * MaxLanders)
				   + (ResScale(2) * (MaxLanders - 1)) + ResScale(2);
	r.corner.x -= ResScale(ResDescale(r.extent.width) >> 1);
	r.corner.y += s.origin.y;
	SetContextForeGroundColor(BlackColor);
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
	GFXRECT r;

	GFXCONTEXT OldContext {SetContext(StatusContext)};

	r.extent.width = ResScale(2);
	r.extent.height = ResScale(4);
	r.corner.y = ResScale(123);
	if (Refresh)
	{
		r.extent.width = NumModuleSlots * (r.extent.width + 1);
		r.corner.x = (StatusWidth() >> 1) - (r.extent.width >> 1);

		SetContextForeGroundColor(BlackColor);
		DrawFilledRectangle(&r);
		r.extent.width = ResScale(2);
	}

	uint8_t i {(uint8_t)CountSISPieces(STORAGE_BAY)};
	if (i)
	{
		uint16_t j {};

		r.corner.x = (StatusWidth() >> 1)
				   - ResScale(
						 ResDescale(i * (r.extent.width + ResScale(1)))
						 >> 1);
		SetContextForeGroundColor(StorageBayFullColor);
		for (j = GLOBAL_SIS(TotalElementMass);
			 j >= StorageBayCapacity; j -= StorageBayCapacity)
		{
			DrawFilledRectangle(&r);
			r.corner.x += r.extent.width + ResScale(1);

			--i;
		}

		r.extent.height = (ResScale(4) * j + (StorageBayCapacity - 1))
						/ StorageBayCapacity;
		if (r.extent.height)
		{
			r.corner.y += ResScale(4) - r.extent.height;
			DrawFilledRectangle(&r);
			r.extent.height = ResScale(4) - r.extent.height;
			if (r.extent.height)
			{
				r.corner.y = ResScale(123);
				SetContextForeGroundColor(StorageBayEmptyColor);
				DrawFilledRectangle(&r);
			}
			r.corner.x += r.extent.width + ResScale(1);

			--i;
		}
		r.extent.height = ResScale(4);

		SetContextForeGroundColor(StorageBayEmptyColor);
		while (i--)
		{
			DrawFilledRectangle(&r);
			r.corner.x += r.extent.width + ResScale(1);
		}
	}

	SetContext(OldContext);
}

void GetGaugeRect(GFXRECT* pRect, bool IsCrewRect)
{
	pRect->extent.width = ResScale(24);
	pRect->corner.x = (StatusWidth() >> 1) - (pRect->extent.width >> 1);
	pRect->extent.height = ResScale(5);
	pRect->corner.y = IsCrewRect ? ResScale(117) : ResScale(38);
}

//static void
//DrawPC_SIS (void)
//{
//	TEXT t;
//	GFXRECT r;
//
//	GetGaugeRect (&r, false);
//	t.baseline.x = (StatusWidth() >> 1);
//	t.baseline.y = r.corner.y - ResScale (1);
//	t.align = ALIGN_CENTER;
//	t.CharCount = (uint16_t)~0;
//	SetContextFont (TinyFont);
//	SetContextForeGroundColor (BlackColor);
//
//	// Black rectangle behind "FUEL" text and fuel amount.
//	r.corner.y -= ResScale (6);
//	r.corner.x -= ResScale (1);
//	r.extent.width += ResScale (2);
//	DrawFilledRectangle (&r);
//
//	SetContextFontEffect (SetAbsFrameIndex (FontGradFrame, 1));
//	t.pStr = GameString (STATUS_STRING_BASE + 3); // "FUEL"
//	font_DrawText (&t);
//
//	// Black rectangle behind "CREW" text and crew amount.
//	r.corner.y += ResScale (79);
//	t.baseline.y += ResScale (79);
//	DrawFilledRectangle (&r);
//
//	SetContextFontEffect (SetAbsFrameIndex (FontGradFrame, 2));
//	t.pStr = GameString (STATUS_STRING_BASE + 4); // "CREW"
//	font_DrawText (&t);
//	SetContextFontEffect (nullptr);
//
//	// Background of text "CAPTAIN".
//	r.corner.x = ResScale (2 + 1);
//	r.corner.y = ResScale (3);
//	r.extent.width = ResScale (58);
//	r.extent.height = ResScale (7);
//	SetContextForeGroundColor (PcCaptainStringBackgroundColor);
//	DrawFilledRectangle (&r);
//
//	DrawBorder (CAPTAIN_FRAME);
//
//	// Text "CAPTAIN".
//	SetContextForeGroundColor (PcCaptainStringTextColor);
//	t.baseline.y = r.corner.y + ResScale (6);
//	t.baseline.x -= ResScale (1);
//	t.pStr = GameString (STATUS_STRING_BASE + 5); // "CAPTAIN"
//	font_DrawText (&t);
//}

static void
Draw_SIS(void)
{
	TEXT t;
	GFXRECT r;
	const bool flat = (bool)is3DO(uqm::UQMOptions::read().whichFonts);

	GetGaugeRect(&r, false);
	t.baseline.x = (StatusWidth() >> 1);
	t.baseline.y = r.corner.y - ResScale(1);
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	SetContextFont(flat ? TinyFontBold : TinyFont);
	SetContextForeGroundColor(BlackColor);

	// Black rectangle behind "FUEL" text and fuel amount.
	r.corner.y -= ResScale(6);
	r.corner.x -= ResScale(1);
	r.extent.width += ResScale(2);
	DrawFilledRectangle(&r);

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, flat ? 10 : 1));
	t.pStr = GameString(STATUS_STRING_BASE + (flat ? 17 : 3)); // "FUEL"
	font_DrawText(&t);

	// Black rectangle behind "CREW" text and crew amount.
	r.corner.y += ResScale(79);
	t.baseline.y += ResScale(79);
	DrawFilledRectangle(&r);

	SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, flat ? 11 : 2));
	t.pStr = GameString(STATUS_STRING_BASE + (flat ? 18 : 4)); // "CREW"
	font_DrawText(&t);
	SetContextFontEffect(nullptr);

	// Background of text "CAPTAIN".
	r.corner.x = ResScale(2 + 1);
	r.corner.y = ResScale(3);
	r.extent.width = ResScale(58);
	r.extent.height = ResScale(7);
	SetContextForeGroundColor(PcCaptainStringBackgroundColor);
	DrawFilledRectangle(&r);

	DrawBorder(CAPTAIN_FRAME);

	// Text "CAPTAIN".
	SetContextForeGroundColor(flat ? ThreedoCaptainStringTextColor : PcCaptainStringTextColor);
	t.baseline.y = r.corner.y + ResScale(6);
	t.baseline.x -= ResScale(1);
	t.pStr = GameString(STATUS_STRING_BASE + (flat ? 19 : 5));
	// "CAPTAIN"
	font_DrawText(&t);
}

static void
DrawThrusters(void)
{
	STAMP s;

	s.origin.x = ResScale(1);
	s.origin.y = 0;
	for (uint16_t i {0}; i < NumDriveSlots; ++i)
	{
		const uint8_t which_piece = GLOBAL_SIS(DriveSlots[i]);
		if (which_piece < EmptySlot)
		{
			s.frame = SetAbsFrameIndex(FlagStatFrame, which_piece + 1 + 0);
			DrawStamp(&s);
			s.frame = IncFrameIndex(s.frame);
			DrawStamp(&s);
		}

		s.origin.y -= ResScale(3);
	}
}

static void
DrawTurningJets(void)
{
	STAMP s;

	s.origin.x = ResScale(1);
	s.origin.y = 0;
	for (uint16_t i {0}; i < NumJetSlots; ++i)
	{
		const uint8_t which_piece = GLOBAL_SIS(JetSlots[i]);
		if (which_piece < EmptySlot)
		{
			s.frame = SetAbsFrameIndex(FlagStatFrame, which_piece + 1 + 1);
			DrawStamp(&s);
			s.frame = IncFrameIndex(s.frame);
			DrawStamp(&s);
		}

		s.origin.y -= ResScale(3);
	}
}

static void
DrawModules(void)
{
	STAMP s;

	// This properly centers the modules.
	s.origin.x = ResScale(1);
	s.origin.y = ResScale(1);
	for (uint16_t i {0}; i < NumModuleSlots; ++i)
	{
		const uint8_t which_piece = GLOBAL_SIS(ModuleSlots[i]);
		if (which_piece < EmptySlot)
		{
			s.frame = SetAbsFrameIndex(FlagStatFrame, which_piece + 1 + 2);
			DrawStamp(&s);
		}

		s.origin.y -= ResScale(3);
	}
}

static void
DrawSupportShips(void)
{
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};
	const GFXPOINT* pship_pos {};
	const GFXPOINT ship_pos[MaxBuiltShips] =
		{
			SUPPORT_SHIP_PTS};

	for (hStarShip = GetHeadLink(&GLOBAL(built_ship_q)),
		pship_pos = ship_pos;
		 hStarShip; hStarShip = hNextShip, ++pship_pos)
	{
		STAMP s;

		const SHIP_FRAGMENT* StarShipPtr {LockShipFrag(&GLOBAL(built_ship_q), hStarShip)};
		hNextShip = _GetSuccLink(StarShipPtr);

		s.origin = MAKE_POINT(
			ResScale(pship_pos->x), ResScale(pship_pos->y));
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

	if (crew_delta != UndefinedDelta)
	{
		uint16_t CrewCapacity {};

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

		t.baseline.x = (StatusWidth() >> 1);
		t.baseline.y = r.corner.y + r.extent.height;
		t.align = ALIGN_CENTER;
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;

		SetContextForeGroundColor(BlackColor);
		DrawFilledRectangle(&r);
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x00, 0x0E, 0x00), 0x6C));
		font_DrawText(&t);
	}
}

static void
DeltaSISGauges_fuelDelta(int32_t fuel_delta)
{
	uint32_t OldCoarseFuel {};

	if (fuel_delta == 0)
	{
		return;
	}

	if (fuel_delta == UndefinedDelta)
	{
		OldCoarseFuel = (uint32_t)~0;
	}
	else
	{

		OldCoarseFuel = (GLOBAL_SIS(FuelOnBoard) / (!uqm::UQMOptions::read().wholeFuel ? FuelTankScale : 1));
		if (fuel_delta < 0
			&& GLOBAL_SIS(FuelOnBoard) <= (uint32_t)-fuel_delta)
		{
			GLOBAL_SIS(FuelOnBoard) = 0;
		}
		else
		{
			const uint32_t FuelCapacity = GetFuelTankCapacity();
			GLOBAL_SIS(FuelOnBoard) += fuel_delta;
			if (GLOBAL_SIS(FuelOnBoard) > FuelCapacity)
			{
				GLOBAL_SIS(FuelOnBoard) = FuelCapacity;
			}
		}
	}

	const uint32_t NewCoarseFuel {(GLOBAL_SIS(FuelOnBoard) / (!uqm::UQMOptions::read().wholeFuel ? FuelTankScale : 1))};
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

		t.baseline.x = (StatusWidth() >> 1);
		t.baseline.y = r.corner.y + r.extent.height;
		t.align = ALIGN_CENTER;
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;

		SetContextForeGroundColor(BlackColor);
		if (uqm::UQMOptions::read().wholeFuel)
		{
			r.corner.x -= ResScale(1);
			r.extent.width += ResScale(2);
		}
		DrawFilledRectangle(&r);
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C));
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

	if (resunit_delta != UndefinedDelta)
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
			BuildColor(MakeRgb15(0x00, 0x08, 0x00), 0x6E));
		DrawFilledRectangle(&r);
	}

	DrawStatusMessage(nullptr);
}

static bool
isUndefinedDelta(int16_t size, int32_t sdword, int integer)
{

	return ((size && sdword && integer) && (size == UndefinedDelta || sdword == UndefinedDelta || integer == UndefinedDelta));
}

void DeltaSISGauges(int16_t crew_delta, int32_t fuel_delta, int resunit_delta)
{
	if (crew_delta == 0 && fuel_delta == 0 && resunit_delta == 0)
	{
		return;
	}

	GFXCONTEXT OldContext {SetContext(StatusContext)};

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
		return CrewPodCapacity;
	}

	return 0;
}

// Gets the amount of crew which currently fit in the ship's crew pods.
uint16_t
GetCrewPodCapacity(void)
{
	uint16_t capacity = 0;

	for (uint16_t slotI {0}; slotI < NumModuleSlots; slotI++)
	{
		const uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
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
	uint16_t capacity = 0;

	uint16_t slotI {NumModuleSlots};
	while (slotI--)
	{
		const uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
		const uint16_t moduleCapacity = GetModuleCrewCapacity(moduleType);

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
	uint16_t slotNr {};
	uint16_t seatNr {};

	uint16_t rowNr {};
	uint16_t colNr {};

	const static Color crewRows[] = PC_CREW_COLOR_TABLE;

	const uint16_t crewCount {GetCrewCount()};
	if (!GetCrewPodForCrewMember(crewCount, &slotNr, &seatNr))
	{
		// Crew does not fit. *ppt is unchanged.
		return GetCrewPodCapacity();
	}

	rowNr = seatNr / CrewPerRow;
	colNr = seatNr % CrewPerRow;

	if (uqm::UQMOptions::read().whichFonts == uqm::EmulationMode::PC)
	{
		SetContextForeGroundColor(crewRows[rowNr]);
	}
	else
	{
		SetContextForeGroundColor(ThreedoCrewColor);
	}

	ppt->x = ResScale(27) + (slotNr * ShipPieceOffset())
		   - ResScale(colNr * 2) - SafePad();
	ppt->y = ResScale(34 - rowNr * 2) + IfHd(2);

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
		return StorageBayCapacity;
	}

	return 0;
}

// Gets the amount of minerals which currently fit in the ship's storage.
uint16_t
GetStorageBayCapacity(void)
{
	uint16_t capacity = 0;

	for (uint16_t slotI {0}; slotI < NumModuleSlots; slotI++)
	{
		const uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
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
	uint16_t capacity = 0;

	uint16_t slotI {NumModuleSlots};
	while (slotI--)
	{
		const uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
		const uint16_t moduleCapacity = GetModuleStorageCapacity(moduleType);

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
	uint16_t slotNr {};
	uint16_t cellNr {};

	uint16_t rowNr {};
	// uint16_t colNr; Unused

	const static Color colorBars[] = STORAGE_BAY_COLOR_TABLE;

	const uint16_t massCount {GetElementMass()};
	if (!GetStorageCellForMineralUnit(massCount, &slotNr, &cellNr))
	{
		// Mineral does not fit. *ppt is unchanged.
		return GetStorageBayCapacity();
	}

	rowNr = cellNr / SbayMassPerRow;
	// colNr = cellNr % SbayMassPerRow; Unused

	if (rowNr == 0)
	{
		SetContextForeGroundColor(BlackColor);
	}
	else
	{
		rowNr--;
		SetContextForeGroundColor(colorBars[rowNr]);
	}

	ppt->x = ResScale(19) + (slotNr * ShipPieceOffset()) - SafePad();
	ppt->y = ResScale(34 - (rowNr * 2)) + IfHd(2);

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
		return FuelTankCapacity;
	}

	if (moduleType == HIGHEFF_FUELSYS)
	{
		return HefuelTankCapacity;
	}

	return 0;
}

// Gets the amount of fuel which currently fits in the ship's fuel tanks.
uint32_t
GetFuelTankCapacity(void)
{
	uint32_t capacity = FuelReserve;

	for (uint16_t slotI {0}; slotI < NumModuleSlots; slotI++)
	{
		const uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);
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
	uint32_t capacity = FuelReserve;

	assert(unitNr >= FuelReserve);

	uint16_t slotI {NumModuleSlots};
	while (slotI--)
	{
		const uint8_t moduleType = GLOBAL_SIS(ModuleSlots[slotI]);

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
	uint16_t slotNr {};
	uint32_t compartmentNr {};

	uint32_t rowNr {};

	const static Color fuelColors[] = FUEL_COLOR_TABLE;

	const uint32_t fuelAmount {GetFuelTotal()};
	if (fuelAmount < FuelReserve)
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

	const uint8_t moduleType {GLOBAL_SIS(ModuleSlots[slotNr])};
	const uint32_t volume {GetModuleFuelCapacity(moduleType)};

	rowNr = ((volume - compartmentNr) * MaxFuelBars
			 / HefuelTankCapacity);

	ppt->x = ResScale(21) + (slotNr * ShipPieceOffset())
		   + IfHd(OutfitOrShipyard == 2 ? 0 : 2) - SafePad();
	if (volume == FuelTankCapacity)
	{
		ppt->y = ResScale(27 - rowNr);
	}
	else
	{
		ppt->y = ResScale(30 - rowNr);
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
	uint16_t i {};

	uint16_t num_pieces {0};
	if (piece_type == FUSION_THRUSTER)
	{
		for (i = 0; i < NumDriveSlots; ++i)
		{
			if (GLOBAL_SIS(DriveSlots[i]) == piece_type)
			{
				++num_pieces;
			}
		}
	}
	else if (piece_type == TURNING_JETS)
	{
		for (i = 0; i < NumJetSlots; ++i)
		{
			if (GLOBAL_SIS(JetSlots[i]) == piece_type)
			{
				++num_pieces;
			}
		}
	}
	else
	{
		for (i = 0; i < NumModuleSlots; ++i)
		{
			if (GLOBAL_SIS(ModuleSlots[i]) == piece_type)
			{
				++num_pieces;
			}
		}
	}

	return num_pieces;
}

static constexpr auto MaxCluster {45}; // "Epsilon Camelopardalis" x 2 + null terminator

static void
AutoPilotTextLogic(void)
{
	char buf[PATH_MAX];
	char star_cluster[MaxCluster];
	GFXPOINT Falayalaralfali;
	GFXPOINT destination;
	GFXPOINT current_position;
	double target_distance {NAN};
	TEXT temp;
	GFXRECT r;

	if (GLOBAL_SIS(FuelOnBoard) == 0)
	{
		DrawSISMessageEx(
			GameString(NAVIGATION_STRING_BASE + 2),
			-1, -1, DsmeMycolor); // "OUT OF FUEL"
		return;
	}

	if (!uqm::UQMOptions::read().smartAutoPilot)
	{
		DrawSISMessageEx(
			GameString(NAVIGATION_STRING_BASE + 3),
			-1, -1, DsmeMycolor); // "AUTO-PILOT"
		return;
	}

	// Show destination and distance to destination
	Falayalaralfali.x = ArilouHomeX;
	Falayalaralfali.y = ArilouHomeY;
	current_position.x = LogxToUniverse(GLOBAL_SIS(log_x));
	current_position.y = LogyToUniverse(GLOBAL_SIS(log_y));
	destination = uqm::flagship().autopilot.target();
	target_distance = ptDistance(current_position, destination) / 10;
	const STAR_DESC* StarPointer {FindStar(nullptr, &destination, 1, 1)};

	if (inQuasiSpace() && (!pointsEqual(destination, Falayalaralfali) || (pointsEqual(destination, Falayalaralfali) && !(GET_GAME_STATE(KNOW_QS_PORTAL) & (1 << 15)))))
	{
		StarPointer = nullptr;
	}

	if (!StarPointer)
	{ // Show the destination coordinates if the
		// destination is not a star
		// AUTO-PILOT to ###.#:###.# - [TargetDistance]
		fmt::format_to_sz_n(buf, "{} {} {:03}.{:01}:{:03}.{:01} - {:.1}",
							GameString(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
							GameString(NAVIGATION_STRING_BASE + 6), // "to"
							destination.x / 10, destination.x % 10, // X Coordinates
							destination.y / 10, destination.y % 10, // Y Coordinates
							target_distance);

		DrawSISMessageEx(buf, -1, -1, DsmeMycolor);
		return;
	}

	if (pointsEqual(LoadLastLoc(), destination))
	{
		fmt::format_to_sz_n(buf, "{} {} {}",
							GameString(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
							GameString(NAVIGATION_STRING_BASE + 6), // "to"
							GameString(NAVIGATION_STRING_BASE)		// "HyperSpace"
		);

		DrawSISMessageEx(buf, -1, -1, DsmeMycolor);
		return;
	}

	GetClusterName(StarPointer, star_cluster);

	// AUTO-PILOT to [StarCluster] - [TargetDistance]
	fmt::format_to_sz_n(buf, "{} {} {} - {:.1}",
						GameString(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
						GameString(NAVIGATION_STRING_BASE + 6), // "to"
						star_cluster,
						target_distance);

	temp.pStr = buf;
	r = font_GetTextRect(&temp);

	if (r.extent.width > SisMessageWidth())
	{ // If the full text is too large then
		// use "->" instead of "AUTO-PILOT"
		// -> to [StarCluster] - [TargetDistance]
		fmt::format_to_sz_n(buf, "{} {} - {:.1}",
							GameString(NAVIGATION_STRING_BASE + 7), // "->"
							star_cluster,
							target_distance);
	}

	temp.pStr = buf;
	r = font_GetTextRect(&temp);

	if (r.extent.width > SisMessageWidth())
	{ // If shortened text is *still* too
		// large then just show distance
		fmt::format_to_sz_n(buf, "{} - {:.1}",
							GameString(NAVIGATION_STRING_BASE + 3), // "AUTO-PILOT"
							target_distance);
	}

	DrawSISMessageEx(buf, -1, -1, DsmeMycolor);
}

static constexpr auto BlinkRate {(c_gameSecond * 3 / 40)}; // 9 @ 120 ticks/second

void DrawAutoPilotMessage(bool Reset)
{
	static bool LastPilot = false;
	static TimeCount NextTime {};
	static uint32_t cycle_index = 0;
	bool OnAutoPilot {};

	const static Color cycle_tab[] = AUTOPILOT_COLOR_CYCLE_TABLE;
	const size_t cycleCount = std::size(cycle_tab);

	if (Reset || uqm::UQMOptions::read().bubbleWarp)
	{ // Just a reset, not drawing
		LastPilot = false;
		return;
	}

	OnAutoPilot = (uqm::flagship().autopilot.hasTarget())
			   || GLOBAL_SIS(FuelOnBoard) == 0;

	if (OnAutoPilot || LastPilot)
	{
		if (!OnAutoPilot)
		{ // AutoPilot aborted -- clear the AUTO-PILOT message
			DrawSISMessage(nullptr);
			cycle_index = 0;

			if (IsExtended())
			{
				ZeroLastLoc();
			}
		}
		else if (GetTimeCounter() >= NextTime)
		{
			if (!(uqm::currentActivity() & CHECK_ABORT)
				&& GLOBAL_SIS(CrewEnlisted) != (uint16_t)~0)
			{
				GFXCONTEXT OldContext {SetContext(OffScreenContext)};
				SetContextForeGroundColor(cycle_tab[cycle_index]);

				AutoPilotTextLogic();

				SetContext(OldContext);
			}

			cycle_index = (cycle_index + 1) % cycleCount;
			NextTime = GetTimeCounter() + BlinkRate;
		}

		LastPilot = OnAutoPilot;
	}
}

// Kruzen: The caller should set the context correctly and batch graphics
void DrawFuelInFTanks(bool isOutfit)
{
	GFXRECT r;
	const uint32_t FuelVolume = GLOBAL_SIS(FuelOnBoard);
	const uint32_t capacity = GetFuelTankCapacity();
	uint32_t volume {};
	Color c;

	if (isOutfit)
	{
		c = BuildColor(MakeRgb15(0x0B, 0x00, 0x00), 0x2E);
	}
	else
	{
		c = BlackColor;
	}

	r.extent.height = ResScale(1);

	// Loop through all the rows to draw
	for (GLOBAL_SIS(FuelOnBoard) = FuelReserve;
		 GLOBAL_SIS(FuelOnBoard) < capacity;)
	{
		if (IsHd() && hdFuelFrame)
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
					r.extent.width = ResScale(5);
					r.extent.height =
						(GLOBAL_SIS(FuelOnBoard) - FuelVolume)
						/ (FuelVolumePerRow >> 2);
					r.corner.y = -(GetFrameHot(s.frame).y);
					SetContextForeGroundColor(c);
					DrawFilledRectangle(&r);
				}
			}
			else
			{
				r.extent.width = ResScale(5);
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
				r.extent.width = ResScale(5);
				DrawFilledRectangle(&r);

				r.extent.width = ResScale(3);
				r.corner.x += ResScale(1);

				SetContextForeGroundColor(
					SetContextBackGroundColor(BlackColor));
			}
			else
			{ // Otherwise, draw an empty bar.
				r.extent.width = ResScale(5);
				SetContextForeGroundColor(c);
			}
			DrawFilledRectangle(&r);
			GLOBAL_SIS(FuelOnBoard) += FuelVolumePerRow;
		}
	}
	GLOBAL_SIS(FuelOnBoard) = FuelVolume;
}

static constexpr auto MaxNumRects {5}; // 5 flashing rects at once should be enough
static constexpr auto NumRects {1};

static FlashContext* flashContext[MaxNumRects] =
	{nullptr, nullptr, nullptr, nullptr, nullptr};
static GFXRECT flash_rect[MaxNumRects];
static Alarm* flashAlarm = nullptr;
static bool flashPaused = false;
static uint8_t count_r = NumRects;

static void scheduleFlashAlarm(void);

static void
updateFlashRect(void* arg)
{
	if (flashContext[0] == nullptr)
	{
		return;
	}

	for (uint16_t i {0}; i < count_r; i++)
	{
		Flash_process(flashContext[i]);
	}
	scheduleFlashAlarm();
	(void)arg;
}

static void
scheduleFlashAlarm(void)
{
	const TimeCount nextTime {Flash_nextTime(flashContext[0])};
	const AlarmTime alarmTime {nextTime == TimeCount::max() ? AlarmTime::max() : uqstl::chrono::duration_cast<AlarmTime>(nextTime.time_since_epoch())};
	flashAlarm = Alarm_addAbsolute(alarmTime, updateFlashRect, nullptr);
}

void SetAdditionalRect(const GFXRECT* pRect, uint16_t number)
{ // Add new flashing rect (Max 5)
	// Must be called one by one and in incremental order
	if (pRect != nullptr && count_r != MaxNumRects)
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

	for (uint16_t i {count_r}; i > 0; i--)
	{
		if (flashContext[i] != nullptr)
		{
			Flash_terminate(flashContext[i]);
			flashContext[i] = nullptr;
		}
	}
	count_r = NumRects;
}

void SetFlashRect(const GFXRECT* pRect, bool pcRect)
{
	GFXRECT clip_r = {
		{0, 0},
		{0, 0}
	  };
	GFXRECT temp_r;
	uint16_t i {};

	if (pRect != SfrMenu3do() && pRect != SfrMenuAny()
		&& pRect != SfrMenuNon())
	{ // The caller specified their own flash area, or nullptr (stop flashing).
		GetContextClipRect(&clip_r);
	}
	else
	{
		if ((uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC && pRect != SfrMenuAny())
			|| pRect == SfrMenuNon())
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
			temp_r.corner.x = RadarX() - clip_r.corner.x;
			temp_r.corner.y = RadarY() - clip_r.corner.y;
			temp_r.corner.y += DosNumScl(10);
			temp_r.extent.width = RadarWidth();
			temp_r.extent.height = RadarHeight();
			temp_r.extent.height -= DosNumScl(10);
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
				Flash_setSpeed(flashContext[i], TimePeriod {}, c_gameSecond / 16, TimePeriod {}, c_gameSecond / 16);
				Flash_setFrameTime(flashContext[i], c_gameSecond / 16);
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
			count_r = NumRects;
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
	uint16_t i {};

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
	uint16_t i {};

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
	for (uint8_t i {0}; i < count_r; i++)
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
	uint8_t i {};

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