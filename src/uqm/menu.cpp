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

#include "menustat.h"

#include "colors.h"
#include "controls.h"
#include "units.h"
#include "options.h"
#include "setup.h"
#include "gamestr.h"
#include "libs/graphics/gfx_common.h"
#include "libs/time/timelib.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "util.h"
#include "planets/planets.h"
#include "shipcont.h"
#include "nameref.h"
#include "ifontres.h"
#include "libs/graphics/drawable.h"

static uint8_t GetEndMenuState(uint8_t BaseState);
static uint8_t GetBeginMenuState(uint8_t BaseState);
static uint8_t FixMenuState(uint8_t BadState);
static uint8_t NextMenuState(uint8_t BaseState, uint8_t CurState);
static uint8_t PreviousMenuState(uint8_t BaseState, uint8_t CurState);
static bool GetAlternateMenu(uint8_t* BaseState, uint8_t* CurState);
static uint8_t ConvertAlternateMenu(uint8_t BaseState, uint8_t NewState);
void FunkyMenu(uint8_t m, STAMP stmp);

/* Draw the blue background for PC Menu Text, with a border around it.
 * The specified rectangle includes the border. */
static void DrawPCMenuFrame(GFXRECT* r)
{

	if (IsHd() || uqm::UQMOptions::read().customBorder)
	{
		DrawRenderedBox(r, true, PcmenuBackgroundColor,
						uqm::UQMOptions::read().customBorder ? SPECIAL_BEVEL : THIN_INNER_BEVEL,
						uqm::UQMOptions::read().customBorder);
	}
	else
	{
		DrawStarConBox(r, ResScale(1), PcmenuTopLeftBorderColor,
					   PcmenuBottomRightBorderColor, true,
					   PcmenuBackgroundColor, false, NullColor);
	}

	// Draw the right and bottom of the outer border.
	// This actually draws a rectangle, with the top and left segments just
	// within the total area, but these segments are drawn over in the next
	// step.
	r->corner.x += ResScale(1);
	r->corner.y += ResScale(1);
	r->extent.height -= ResScale(2);
	r->extent.width -= ResScale(2);
}

static constexpr auto AltManifest {0x80};
static constexpr auto AltExitManifest {0x81};

static auto PcMenuHeight() { return (ResScale(8)); }

static char pm_crew_str[128];
static char pm_fuel_str[128];

/* Actually display the menu text */
static void DrawPCMenu(uint8_t beg_index, uint8_t end_index, uint8_t NewState, uint8_t hilite, GFXRECT* r)
{
	uint8_t pos {};
	uint16_t j {};
	static uint16_t rd = 0;
	TEXT t;
	char buf[256];
	GFXRECT rt;

	pos = beg_index + NewState;
	const int num_items {1 + end_index - beg_index};
	r->extent.width -= ResScale(1);

	// Gray rectangle behind PC menu
	rt = *r;
	rt.corner.x -= ResScale(1);
	rt.corner.y += PcMenuHeight() - DosBoolScl(12, 3);
	rt.extent.width += ResScale(2);
	rt.extent.height += DosBoolScl(2, -6);
	if (!uqm::UQMOptions::read().customBorder)
	{
		DrawFilledRectangle(&rt);
	}

	DrawBorder(SIS_RADAR_FRAME);

	if (num_items * PcMenuHeight() > r->extent.height)
	{
		uqm::log::error("Warning, no room for all menu items!");
	}
	else
	{
		r->corner.y += (r->extent.height - (num_items * PcMenuHeight())) / 2;
	}
	r->extent.height = num_items * PcMenuHeight() + ResScale(3);

	r->corner.y += DosNumScl(1);
	r->corner.y -= SafeNumScl(4);

	DrawPCMenuFrame(r);

	FONT OldFont {SetContextFont(StarConFont)};
	t.align = ALIGN_LEFT;
	t.baseline.x = r->corner.x + ResScale(2);
	t.baseline.y = r->corner.y + PcMenuHeight() - ResScale(1);
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	r->corner.x += ResScale(1);
	r->extent.width -= ResScale(2);
	for (uint16_t i {beg_index}; i <= end_index; i++)
	{
		// To compensate for the read speed menu entries
		j = i > PM_EXIT_SETTINGS ?
				(i - (PM_CHANGE_CAPTAIN - PM_READ_VERY_SLOW)) :
				i;

		uqm::utf8StringCopy(buf, sizeof buf,
							(i == PM_FUEL) ? pm_fuel_str :
							(i == PM_CREW) ? pm_crew_str :
											 GameString(MAINMENU_STRING_BASE + j));
		if (hilite && pos == i)
		{
			// Currently selected menu option.
			if (pos != rd)
			{ // Draw the background of the selection.
				SetContextForeGroundColor(
					PcmenuSelectionBackgroundColor);
				r->corner.y = t.baseline.y - PcMenuHeight()
							+ ResScale(2);
				r->extent.height = PcMenuHeight() - ResScale(1);
				if (uqm::UQMOptions::read().customBorder)
				{
					STAMP s;

					s.origin = r->corner;

					s.frame = SetAbsFrameIndex(CustBevelFrame,
											   DOS_MENU_HILITE);
					DrawStamp(&s);
				}
				else
				{
					DrawFilledRectangle(r);
				}

				rd = pos;
			}
			else
			{
				rd = 0;
			}

			// Draw the text of the selected item.
			SetContextForeGroundColor(PcmenuSelectionTextColor);
			font_DrawText(&t);
		}
		else
		{
			// Draw the text of an unselected item.
			SetContextForeGroundColor(PcmenuTextColor);
			font_DrawText(&t);
		}
		t.baseline.y += PcMenuHeight();
	}
	SetContextFont(OldFont);
}

/* Determine the last text item to display */
static uint8_t GetEndMenuState(uint8_t BaseState)
{
	switch (BaseState)
	{
		case PM_SCAN:
		case PM_STARMAP:
			return PM_NAVIGATE;
			break;
		case PM_MIN_SCAN:
			return PM_LAUNCH_LANDER;
			break;
		case PM_SAVE_GAME:
			return PM_EXIT_GAME_MENU;
			break;
		case PM_CONVERSE:
			return PM_ENCOUNTER_GAME_MENU;
			break;
		case PM_FUEL:
			return PM_EXIT_OUTFIT;
			break;
		case PM_CREW:
			return PM_EXIT_SHIPYARD;
			break;
		case PM_SOUND_ON:
			return PM_EXIT_SETTINGS;
			break;
		case PM_ALT_SCAN:
		case PM_ALT_STARMAP:
			return PM_ALT_NAVIGATE;
			break;
		case PM_ALT_CARGO:
			return PM_ALT_EXIT_MANIFEST;
			break;
		case PM_ALT_MSCAN:
			return PM_ALT_EXIT_SCAN;
			break;
	}
	return BaseState;
}

static uint8_t GetBeginMenuState(uint8_t BaseState)
{
	return BaseState;
}

/* Correct Menu State for cases where the Menu shouldn't move */
static uint8_t FixMenuState(uint8_t BadState)
{
	switch (BadState)
	{
		case PM_SOUND_ON:
			if (uqm::globalFlags() & SoundDisabled)
			{
				return PM_SOUND_OFF;
			}
			else
			{
				return PM_SOUND_ON;
			}
		case PM_MUSIC_ON:
			if (uqm::globalFlags() & MusicDisabled)
			{
				return PM_MUSIC_OFF;
			}
			else
			{
				return PM_MUSIC_ON;
			}
		case PM_CYBORG_OFF:
			return (PM_CYBORG_OFF + ((uint8_t)(uqm::globalFlags() & CombatSpeedMask) >> CombatSpeedShift));
		case PM_READ_VERY_SLOW:
			return (PM_READ_VERY_SLOW + (uint8_t)(uqm::globalFlags() & ReadSpeedMask));
	}
	return BadState;
}

/* Choose the next menu to hilight in the 'forward' direction */
static uint8_t NextMenuState(uint8_t BaseState, uint8_t CurState)
{
	uint8_t NextState {};
	uint8_t AdjBase = BaseState;

	if (BaseState == PM_STARMAP)
	{
		AdjBase--;
	}

	switch (AdjBase + CurState)
	{
		case PM_SOUND_ON:
		case PM_SOUND_OFF:
			NextState = PM_MUSIC_ON;
			break;
		case PM_MUSIC_ON:
		case PM_MUSIC_OFF:
			NextState = PM_CYBORG_OFF;
			break;
		case PM_CYBORG_OFF:
		case PM_CYBORG_NORMAL:
		case PM_CYBORG_DOUBLE:
		case PM_CYBORG_SUPER:
			if ((isPC(uqm::UQMOptions::read().smoothScroll) && !usingSpeech) || isPC(uqm::UQMOptions::read().whichMenu))
			{
				NextState = PM_READ_VERY_SLOW;
			}
			else
			{
				NextState = PM_CHANGE_CAPTAIN;
			}
			break;
		case PM_READ_VERY_SLOW:
		case PM_READ_SLOW:
		case PM_READ_MODERATE:
		case PM_READ_FAST:
		case PM_READ_VERY_FAST:
			NextState = PM_CHANGE_CAPTAIN;
			break;
		default:
			NextState = AdjBase + CurState + 1;
	}
	if (NextState > GetEndMenuState(BaseState))
	{
		NextState = GetBeginMenuState(BaseState);
	}
	return (FixMenuState(NextState) - AdjBase);
}

/* Choose the next menu to hilight in the 'back' direction */
uint8_t PreviousMenuState(uint8_t BaseState, uint8_t CurState)
{
	int16_t NextState {};
	uint8_t AdjBase = BaseState;

	if (BaseState == PM_STARMAP)
	{
		AdjBase--;
	}

	switch (AdjBase + CurState)
	{
		case PM_SOUND_OFF:
			NextState = PM_EXIT_SETTINGS;
			break;
		case PM_MUSIC_ON:
		case PM_MUSIC_OFF:
			NextState = PM_SOUND_ON;
			break;
		case PM_CYBORG_OFF:
		case PM_CYBORG_NORMAL:
		case PM_CYBORG_DOUBLE:
		case PM_CYBORG_SUPER:
			NextState = PM_MUSIC_ON;
			break;
		case PM_READ_VERY_SLOW:
		case PM_READ_SLOW:
		case PM_READ_MODERATE:
		case PM_READ_FAST:
		case PM_READ_VERY_FAST:
			NextState = PM_CYBORG_OFF;
			break;
		case PM_CHANGE_CAPTAIN:
			if ((isPC(uqm::UQMOptions::read().smoothScroll) && !usingSpeech) || isPC(uqm::UQMOptions::read().whichMenu))
			{
				NextState = PM_READ_VERY_SLOW;
			}
			else
			{
				NextState = PM_CYBORG_OFF;
			}
			break;
		default:
			NextState = AdjBase + CurState - 1;
	}
	if (NextState < GetBeginMenuState(BaseState))
	{
		NextState = GetEndMenuState(BaseState);
	}
	return (FixMenuState((uint8_t)NextState) - AdjBase);
}


/* When using PC hierarchy, convert 3do->PC */
static bool GetAlternateMenu(uint8_t* BaseState, uint8_t* CurState)
{
	uint8_t AdjBase = *BaseState;
	uint8_t adj = 0;
	if (*BaseState == PM_STARMAP)
	{
		AdjBase--;
		adj = 1;
	}
	if (*CurState & 0x80)
	{
		switch (*CurState)
		{
			case AltManifest:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_MANIFEST - PM_ALT_SCAN - adj;
				return true;
			case AltExitManifest:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_EXIT_MANIFEST - PM_ALT_CARGO;
				return true;
		}
		uqm::log::error("Unknown state combination: {}, {}",
						*BaseState, *CurState);
		return false;
	}
	else
	{
		switch (AdjBase + *CurState)
		{
			case PM_SCAN:
				*BaseState = PM_ALT_SCAN;
				*CurState = PM_ALT_SCAN - PM_ALT_SCAN;
				return true;
			case PM_STARMAP:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_STARMAP - PM_ALT_SCAN - adj;
				return true;
			case PM_DEVICES:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_DEVICES - PM_ALT_CARGO;
				return true;
			case PM_CARGO:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_CARGO - PM_ALT_CARGO;
				return true;
			case PM_ROSTER:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_ROSTER - PM_ALT_CARGO;
				return true;
			case PM_GAME_MENU:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_GAME_MENU - PM_ALT_SCAN - adj;
				return true;
			case PM_NAVIGATE:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_NAVIGATE - PM_ALT_SCAN - adj;
				return true;
			case PM_MIN_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_MSCAN - PM_ALT_MSCAN;
				return true;
			case PM_ENE_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_ESCAN - PM_ALT_MSCAN;
				return true;
			case PM_BIO_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_BSCAN - PM_ALT_MSCAN;
				return true;
			case PM_EXIT_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_EXIT_SCAN - PM_ALT_MSCAN;
				return true;
			case PM_AUTO_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_ASCAN - PM_ALT_MSCAN;
				return true;
			case PM_LAUNCH_LANDER:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_DISPATCH - PM_ALT_MSCAN;
				return true;
		}
		return false;
	}
}

/* When using PC hierarchy, convert PC->3DO */
static uint8_t ConvertAlternateMenu(uint8_t BaseState, uint8_t NewState)
{
	switch (BaseState + NewState)
	{
		case PM_ALT_SCAN:
			return (PM_SCAN - PM_SCAN);
		case PM_ALT_STARMAP:
			return (PM_STARMAP - PM_SCAN);
		case PM_ALT_MANIFEST:
			return (AltManifest);
		case PM_ALT_GAME_MENU:
			return (PM_GAME_MENU - PM_SCAN);
		case PM_ALT_NAVIGATE:
			return (PM_NAVIGATE - PM_SCAN);
		case PM_ALT_CARGO:
			return (PM_CARGO - PM_SCAN);
		case PM_ALT_DEVICES:
			return (PM_DEVICES - PM_SCAN);
		case PM_ALT_ROSTER:
			return (PM_ROSTER - PM_SCAN);
		case PM_ALT_EXIT_MANIFEST:
			return (AltExitManifest);
		case PM_ALT_MSCAN:
			return (PM_MIN_SCAN - PM_MIN_SCAN);
		case PM_ALT_ESCAN:
			return (PM_ENE_SCAN - PM_MIN_SCAN);
		case PM_ALT_BSCAN:
			return (PM_BIO_SCAN - PM_MIN_SCAN);
		case PM_ALT_ASCAN:
			return (PM_AUTO_SCAN - PM_MIN_SCAN);
		case PM_ALT_DISPATCH:
			return (PM_LAUNCH_LANDER - PM_MIN_SCAN);
		case PM_ALT_EXIT_SCAN:
			return (PM_EXIT_SCAN - PM_MIN_SCAN);
	}
	return (NewState);
}

bool DoMenuChooser(MENU_STATE* pMS, uint8_t BaseState)
{
	uint8_t NewState = pMS->CurState;
	const uint8_t OrigBase = BaseState;
	bool useAltMenu = false;
	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		useAltMenu = GetAlternateMenu(&BaseState, &NewState);
	}
	if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_UP])
	{
		NewState = PreviousMenuState(BaseState, NewState);
	}
	else if (PulsedInputState.menu[KEY_MENU_RIGHT] || PulsedInputState.menu[KEY_MENU_DOWN])
	{
		NewState = NextMenuState(BaseState, NewState);
	}
	else if (useAltMenu && PulsedInputState.menu[KEY_MENU_SELECT])
	{
		NewState = ConvertAlternateMenu(BaseState, NewState);
		if (NewState == AltManifest)
		{
			DrawMenuStateStrings(PM_ALT_CARGO, 0);
			pMS->CurState = PM_CARGO - PM_SCAN;
			return true;
		}
		if (NewState == AltExitManifest)
		{
			if (OrigBase == PM_SCAN)
			{
				DrawMenuStateStrings(PM_ALT_SCAN,
									 PM_ALT_MANIFEST - PM_ALT_SCAN);
			}
			else
			{
				DrawMenuStateStrings(PM_ALT_STARMAP,
									 PM_ALT_MANIFEST - PM_ALT_STARMAP);
			}
			pMS->CurState = AltManifest;
			return true;
		}
		return false;
	}
	else if ((uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC) && PulsedInputState.menu[KEY_MENU_CANCEL] && (BaseState == PM_ALT_CARGO))
	{
		if (OrigBase == PM_SCAN)
		{
			DrawMenuStateStrings(PM_ALT_SCAN,
								 PM_ALT_MANIFEST - PM_ALT_SCAN);
		}
		else
		{
			DrawMenuStateStrings(PM_ALT_STARMAP,
								 PM_ALT_MANIFEST - PM_ALT_STARMAP);
		}
		pMS->CurState = AltManifest;
		return true;
	}
	else
	{
		return false;
	}

	DrawMenuStateStrings(BaseState, NewState);
	if (useAltMenu)
	{
		NewState = ConvertAlternateMenu(BaseState, NewState);
	}
	pMS->CurState = NewState;

	return true;
}

const char* IndexToText(int Index)
{
	int i = -1;

	if (Index < PM_EXIT_GAME_MENU)
	{
		i = Index;
	}

	switch (Index)
	{
		case PM_ENCOUNTER_GAME_MENU:
		case PM_OUTFIT_GAME_MENU:
		case PM_SHIPYARD_GAME_MENU:
			i = GAMESTR_GAME_MENU;
			break;
		case PM_EXIT_GAME_MENU:
		case PM_EXIT_OUTFIT:
		case PM_EXIT_SHIPYARD:
		case PM_EXIT_SETTINGS:
			i = GAMESTR_EXIT_MENU;
			break;
		case PM_CONVERSE:
			i = GAMESTR_CONVERSE;
			break;
		case PM_ATTACK:
			i = GAMESTR_ATTACK;
			break;
		case PM_FUEL:
			i = GAMESTR_FUEL;
			break;
		case PM_MODULE:
			i = GAMESTR_MODULE;
			break;
		case PM_CREW:
			i = GAMESTR_CREW;
			break;
		case PM_SOUND_ON:
			i = GAMESTR_SND_ON;
			break;
		case PM_SOUND_OFF:
			i = GAMESTR_SND_OFF;
			break;
		case PM_MUSIC_ON:
			i = GAMESTR_MUS_ON;
			break;
		case PM_MUSIC_OFF:
			i = GAMESTR_MUS_OFF;
			break;
		case PM_CYBORG_OFF:
		case PM_CYBORG_NORMAL:
		case PM_CYBORG_DOUBLE:
		case PM_CYBORG_SUPER:
			i = GAMESTR_COMBAT;
			break;
		case PM_READ_VERY_SLOW:
		case PM_READ_SLOW:
		case PM_READ_MODERATE:
		case PM_READ_FAST:
		case PM_READ_VERY_FAST:
			i = GAMESTR_READING;
			break;
		case PM_CHANGE_CAPTAIN:
			i = GAMESTR_CHANGE_CAP;
			break;
		case PM_CHANGE_SHIP:
			i = GAMESTR_CHANGE_SIS;
			break;
		default:
			break;
	}

	if (i == -1 || !strlen(GameString(PLAYMENU_STRING_BASE + i)))
	{
		return nullptr;
	}

	return GameString(PLAYMENU_STRING_BASE + i);
}

static void Draw3DOMenuText(GFXRECT* r, int Index)
{
	TEXT text;
	int16_t leading {};
	GFXRECT block;
	Color OldColor;

	if (IndexToText(Index) == nullptr)
	{
		return;
	}

	FONT OldFont {SetContextFont(PlayMenuFont)};
	OldColor = SetContextForeGroundColor(PmRectColor);

	GetContextFontLeading(&leading);

	if (!uqm::UQMOptions::read().customBorder)
	{
		block = *r;
		block.corner.y += DosNumScl(2);
		block.extent.height = leading;
		DrawFilledRectangle(&block); // with PmRectColor
	}
	else
	{
		DrawBorder(TEXT_LABEL_FRAME);
	}

	text.align = ALIGN_CENTER;
	text.baseline.x = r->corner.x + (r->extent.width >> 1);
	text.baseline.y = r->corner.y + leading - NdosNumScl(2);
	text.pStr = AlignText(IndexToText(Index),
						  &text.baseline.x);
	text.CharCount = (uint16_t)~0;

	font_DrawShadowedText(&text, NORTH_WEST_SHADOW, PmTextColor,
						  PmShadowColor);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

void DrawMenuStateStrings(uint8_t beg_index, int16_t NewState)
{
	GFXRECT r;
	STAMP s;
	uint8_t hilite = 1;
	extern FRAME PlayFrame;

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		return;
	}

	if (NewState < 0)
	{
		NewState = -NewState;
		hilite = 0;
	}

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		uint8_t tmpState = (uint8_t)NewState;
		GetAlternateMenu(&beg_index, &tmpState);
		NewState = tmpState;
	}

	if (beg_index == PM_STARMAP)
	{
		NewState--;
	}
	uint8_t end_index {GetEndMenuState(beg_index)};

	s.frame = 0;
	if (NewState <= end_index - beg_index)
	{
		s.frame = SetAbsFrameIndex(PlayFrame, beg_index + NewState);
	}

	PreUpdateFlashRect();
	GFXCONTEXT OldContext {SetContext(StatusContext)};
	GetContextClipRect(&r);
	s.origin.x = RadarX() - r.corner.x;
	s.origin.y = RadarY() - r.corner.y;
	r.corner.x = s.origin.x - ResScale(1);
	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		r.corner.y = s.origin.y - PcMenuHeight() - IfHd(2);
	}
	else
	{
		r.corner.y = s.origin.y - ResScale(11) + DosNumScl(3);
	}
	r.extent.width = RadarWidth() + ResScale(3);
	BatchGraphics();
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08));
	if (s.frame && uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		if (beg_index == PM_CREW)
		{
			fmt::format_to_sz_n(pm_crew_str, "{}({})",
								GameString(MAINMENU_STRING_BASE + static_cast<int>(PM_CREW)),
								uqm::gameState().game().CrewCost);
		}
		if (beg_index == PM_FUEL)
		{
			fmt::format_to_sz_n(pm_fuel_str, "{}({})",
								GameString(MAINMENU_STRING_BASE + static_cast<int>(PM_FUEL)),
								uqm::gameState().game().FuelCost);
		}
		if (beg_index == PM_SOUND_ON)
		{
			end_index = beg_index + 6;

			switch (beg_index + NewState)
			{
				case PM_SOUND_ON:
				case PM_SOUND_OFF:
					NewState = 0;
					break;
				case PM_MUSIC_ON:
				case PM_MUSIC_OFF:
					NewState = 1;
					break;
				case PM_CYBORG_OFF:
				case PM_CYBORG_NORMAL:
				case PM_CYBORG_DOUBLE:
				case PM_CYBORG_SUPER:
					NewState = 2;
					break;
				case PM_READ_VERY_SLOW:
				case PM_READ_SLOW:
				case PM_READ_MODERATE:
				case PM_READ_FAST:
				case PM_READ_VERY_FAST:
					NewState = 3;
					break;
				case PM_CHANGE_CAPTAIN:
					NewState = 4;
					break;
				case PM_CHANGE_SHIP:
					NewState = 5;
					break;
				case PM_EXIT_SETTINGS:
					NewState = 6;
					break;
			}
		}
		r.extent.height = RadarHeight() + ResScale(12);

		DrawPCMenu(beg_index, end_index, (uint8_t)NewState, hilite, &r);
		s.frame = 0;

		if (uqm::UQMOptions::read().submenu)
		{
			FunkyMenu(beg_index + (uint8_t)NewState, s);
		}
	}
	else
	{
		// Gray rectangle behind Lander and HyperSpace radar
		r.corner.x -= ResScale(1);
		r.corner.y += DosNumScl(5);
		r.extent.width += ResScale(1);
		r.extent.height = RadarHeight()
						+ ResScale(isPC(uqm::UQMOptions::read().whichMenu) ? 9 : 12);
		r.extent.height -= DosNumScl(6);

		if (!uqm::UQMOptions::read().customBorder)
		{
			DrawFilledRectangle(&r);
		}
		else
		{
			DrawBorder(SIS_RADAR_FRAME);
		}
	}
	if (s.frame)
	{
		if (uqm::UQMOptions::read().submenu)
		{
			FunkyMenu(beg_index + (uint8_t)NewState, s);
		}
		else
		{
			DrawStamp(&s);
		}

		Draw3DOMenuText(&r, beg_index + NewState);

		TEXT t;
		char buf[20];

		switch (beg_index + NewState)
		{
			case PM_CREW:
				t.baseline.x = s.origin.x + RadarWidth() - ResScale(2);
				t.baseline.y = s.origin.y + RadarHeight() - ResScale(2);
				t.align = ALIGN_RIGHT;
				t.CharCount = (uint16_t)~0;
				t.pStr = buf;
				fmt::format_to_sz_n(buf, "{}", uqm::gameState().game().CrewCost);
				if (isPC(uqm::UQMOptions::read().whichFonts))
				{
					SetContextFont(TinyFont);
				}
				else
				{
					SetContextFont(TinyFontBold);
				}
				SetContextForeGroundColor(ThreedomenuTextColor);
				font_DrawText(&t);
				break;
			case PM_FUEL:
				t.baseline.x = s.origin.x + RadarWidth() - ResScale(2);
				t.baseline.y = s.origin.y + RadarHeight() - ResScale(2);
				t.align = ALIGN_RIGHT;
				t.CharCount = (uint16_t)~0;
				t.pStr = buf;
				fmt::format_to_sz_n(buf, "{}", uqm::gameState().game().FuelCost);
				if (uqm::UQMOptions::read().whichFonts == uqm::EmulationMode::Console3DO && !uqm::UQMOptions::read().wholeFuel)
				{
					SetContextFont(TinyFontBold);
				}
				else
				{
					SetContextFont(TinyFont);
				}
				SetContextForeGroundColor(ThreedomenuTextColor);
				font_DrawText(&t);
				break;
		}
	}
	UnbatchGraphics();
	SetContext(OldContext);
	PostUpdateFlashRect();
}

void DrawMineralHelpers(void)
{
	STAMP s;
	TEXT t;
	char buf[40];
	GFXRECT r;
	int16_t leading {};

	if (!uqm::UQMOptions::read().submenu)
	{
		return;
	}

	GFXCONTEXT OldContext {SetContext(StatusContext)};

	BatchGraphics();

	DrawFlagStatDisplay(""); // (GameString (STATUS_STRING_BASE + 6));

	SetContextFont(TinyFont);

	GetContextFontLeading(&leading);
	leading -= ResScale(1);

	const auto ElementOrgY = [&] {
		return ResScale(24);
	};
	const auto ElementSpacingY = [&] {
		return ResScale(9);
	};

	const auto ElementCol0 = [&] {
		return ResScale(10);
	};
	const auto ElementCol1 = [&] {
		return ResScale(32);
	};
	const auto ElementCol2 = [&] {
		return ResScale(58);
	};

	// setup element icons
	s.frame = SetAbsFrameIndex(MiscDataFrame,
							   (NumScandotTransitions << 1) + 3);
	s.origin.x = ElementCol0();
	s.origin.y = ElementOrgY();
	// setup element worths
	t.baseline.x = ElementCol1();
	t.baseline.y = ElementOrgY() + ResScale(3);
	t.align = ALIGN_RIGHT;
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;

	r.extent = GetFrameBounds(s.frame);
	r.corner.x = ElementCol0() - ResScale(3);

	// draw element icons and worths
	for (uint16_t i {0}; i < NUM_ELEMENT_CATEGORIES; ++i)
	{
		// draw element icon
		DrawStamp(&s);
		s.frame = SetRelFrameIndex(s.frame, 5);
		s.origin.y += ElementSpacingY();

		// print element worth
		SetContextForeGroundColor(ModulePriceColor);
		fmt::format_to_sz_n(buf, "{}", uqm::gameState().game().ElementWorth[i]);
		font_DrawText(&t);

		// print x'es
		fmt::format_to_sz_n(buf, "x");
		t.baseline.x -= ResScale(11);
		font_DrawText(&t);

		// print the element amount
		SetContextForeGroundColor(ModuleNameColor);
		t.baseline.x = ElementCol2();
		fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().ElementAmounts[i]);
		font_DrawText(&t);

		t.baseline.x = ElementCol1();
		t.baseline.y += ElementSpacingY();
	}

	SetContextForeGroundColor(BlackColor);
	r.corner.y = s.origin.y - ResScale(3);
	DrawFilledRectangle(&r);

	s.frame = SetAbsFrameIndex(s.frame, 68);
	s.origin.x = r.corner.x + ResRecenter(r.extent.height);
	s.origin.y = r.corner.y + ResRecenter(r.extent.height);
	DrawStamp(&s);

	// print the bio amount
	SetContextForeGroundColor(ModuleNameColor);
	t.baseline.x = ElementCol2();
	fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().TotalBioMass);
	font_DrawText(&t);
	t.baseline.y += ElementSpacingY();

	r.corner.x = ResScale(4);
	r.corner.y = t.baseline.y - ResScale(7);
	r.extent.width = FieldWidth() - ResScale(3);
	r.extent.height = ResScale(1);
	SetContextForeGroundColor(CargoSelectedBackColor);
	DrawFilledRectangle(&r);

	SetContextForeGroundColor(ModuleNameColor);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + ResScale(2);
	t.baseline.y = r.corner.y + leading + ResScale(1);
	t.pStr = GameString(STATUS_STRING_BASE + 8); // FUEL:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	t.pStr = GameString(STATUS_STRING_BASE + 9); // CREW:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	t.pStr = GameString(STATUS_STRING_BASE + 10); // LAND.:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	t.pStr = GameString(STATUS_STRING_BASE + 11); // CARGO:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	SetContextForeGroundColor(ModulePriceColor);
	t.align = ALIGN_RIGHT;
	t.baseline.x = r.extent.width + ResScale(2);
	t.baseline.y = r.corner.y + leading + ResScale(1);
	t.pStr = WholeFuelValue();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().CrewEnlisted);
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().NumLanders);
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;

	if (GetStorageBayCapacity())
	{
		float totalCapacity = (float)uqm::gameState().sis().TotalElementMass
							/ (float)GetStorageBayCapacity() * 100;

		if (totalCapacity == 100)
		{
			fmt::format_to_sz_n(buf, "{:.0}%", totalCapacity);
		}
		else if (totalCapacity > 9)
		{
			fmt::format_to_sz_n(buf, "{:.1}%", totalCapacity);
		}
		else
		{
			fmt::format_to_sz_n(buf, "{:.2}%", totalCapacity);
		}
	}
	else
	{
		fmt::format_to_sz_n(buf, "---");
	}

	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	UnbatchGraphics();

	SetContext(OldContext);
}

void DrawBorder(uint8_t Visible)
{
	STAMP s;

	GFXCONTEXT OldContext {SetContext(ScreenContext)};

	s.origin.x = 0;
	s.origin.y = 0;

	if (uqm::UQMOptions::read().customBorder)
	{
		s.frame = SetAbsFrameIndex(BorderFrame, Visible);
		DrawStamp(&s);
	}
	else if (IsHd())
	{
		s.frame = SetAbsFrameIndex(HDBorderFrame, Visible);
		DrawStamp(&s);
	}

	SetContext(OldContext);
}

void FunkyMenu(uint8_t m, STAMP stmp)
{
	static bool subMenuFlag;

	if ((!stmp.frame && m >= PM_ALT_MSCAN
		 && m <= PM_ALT_EXIT_SCAN)
		|| m == PM_LAUNCH_LANDER)
	{
		if (stmp.frame)
		{
			DrawStamp(&stmp);
		}

		DrawMineralHelpers();

		subMenuFlag = true;

		if (stmp.frame)
		{
			return;
		}
	}
	else if (subMenuFlag == true)
	{
		DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);
		subMenuFlag = false;
	}

	if (stmp.frame)
	{
		DrawStamp(&stmp);
	}
}
