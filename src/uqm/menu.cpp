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

#include "colors.h"
#include "controls.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "gamestr.h"
#include "ifontres.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/gfx_common.h"
#include "libs/time/timelib.h"
#include "menustat.h"
#include "nameref.h"
#include "options.h"
#include "planets/planets.h"
#include "setup.h"
#include "shipcont.h"
#include "units.h"
#include "util.h"

namespace uqm
{
static uint8_t GetEndMenuState(uint8_t BaseState);
static uint8_t GetBeginMenuState(uint8_t BaseState);
static uint8_t FixMenuState(uint8_t BadState);
static uint8_t NextMenuState(uint8_t BaseState, uint8_t CurState);
static uint8_t PreviousMenuState(uint8_t BaseState, uint8_t CurState);
static bool GetAlternateMenu(MenuString* BaseState, uint8_t* CurState);
static uint8_t ConvertAlternateMenu(uint8_t BaseState, uint8_t NewState);
void FunkyMenu(uint8_t m, GfxStamp stmp);

/* Draw the blue background for PC Menu Text, with a border around it.
 * The specified rectangle includes the border. */
static void DrawPCMenuFrame(GfxRect* r)
{

	if (IsHd() || uqm::UQMOptions::read().customBorder)
	{
		DrawRenderedBox(r, true, PcmenuBackgroundColor,
						uqm::UQMOptions::read().customBorder ? BevelLevel::SpecialBevel : BevelLevel::ThinInnerBevel,
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

static uqstl::array<char, 128> pm_crew_str;
static uqstl::array<char, 128> pm_fuel_str;

/* Actually display the menu text */
static void DrawPCMenu(uint8_t beg_index, uint8_t end_index, uint8_t NewState, uint8_t hilite, GfxRect* r)
{
	uint8_t pos {};
	uint16_t j {};
	static uint16_t rd = 0;
	GfxText t;
	uqstl::array<char, 256> buf;
	GfxRect rt;

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

	DrawBorder(BorderFrameIndex::SisRadar);

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

	FontRef OldFont {SetContextFont(StarConFont)};
	t.align = TextAlign::Left;
	t.baseline.x = r->corner.x + ResScale(2);
	t.baseline.y = r->corner.y + PcMenuHeight() - ResScale(1);
	t.pStr = buf.data();
	t.CharCount = (uint16_t)~0;
	r->corner.x += ResScale(1);
	r->extent.width -= ResScale(2);
	for (uint16_t i {beg_index}; i <= end_index; i++)
	{
		// To compensate for the read speed menu entries
		j = i > MenuString::ExitSettings ?
				(i - (MenuString::ChangeCaptain - MenuString::ReadVerySlow)) :
				i;

		uqm::utf8StringCopy(buf.data(), sizeof buf,
							(i == MenuString::Fuel) ? pm_fuel_str.data() :
							(i == MenuString::Crew) ? pm_crew_str.data() :
													  GameString(GameStringBase::MainmenuString + j));
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
					GfxStamp s;

					s.origin = r->corner;

					s.frame = SetAbsFrameIndex(CustBevelFrame,
											   BevelLevel::DosMenuHilite);
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
		case *MenuString::Scan:
		case *MenuString::Starmap:
			return *MenuString::Navigate;
			break;
		case *MenuString::MinScan:
			return *MenuString::LaunchLander;
			break;
		case *MenuString::SaveGame:
			return *MenuString::ExitGameMenu;
			break;
		case *MenuString::Converse:
			return *MenuString::EncounterGameMenu;
			break;
		case *MenuString::Fuel:
			return *MenuString::ExitOutfit;
			break;
		case *MenuString::Crew:
			return *MenuString::ExitShipyard;
			break;
		case *MenuString::SoundOn:
			return *MenuString::ExitSettings;
			break;
		case *MenuString::AltScan:
		case *MenuString::AltStarmap:
			return *MenuString::AltNavigate;
			break;
		case *MenuString::AltCargo:
			return *MenuString::AltExitManifest;
			break;
		case *MenuString::AltMscan:
			return *MenuString::AltExitScan;
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
		case *MenuString::SoundOn:
			if (uqm::globalFlags() & SoundDisabled)
			{
				return *MenuString::SoundOff;
			}
			else
			{
				return *MenuString::SoundOn;
			}
		case *MenuString::MusicOn:
			if (uqm::globalFlags() & MusicDisabled)
			{
				return *MenuString::MusicOff;
			}
			else
			{
				return *MenuString::MusicOn;
			}
		case *MenuString::CyborgOff:
			return *(MenuString::CyborgOff + ((uint8_t)(uqm::globalFlags() & CombatSpeedMask) >> CombatSpeedShift));
		case *MenuString::ReadVerySlow:
			return *(MenuString::ReadVerySlow + (uint8_t)(uqm::globalFlags() & ReadSpeedMask));
	}
	return BadState;
}

/* Choose the next menu to hilight in the 'forward' direction */
static uint8_t NextMenuState(uint8_t BaseState, uint8_t CurState)
{
	uint8_t NextState {};
	uint8_t AdjBase = BaseState;

	if (BaseState == MenuString::Starmap)
	{
		AdjBase--;
	}

	switch (AdjBase + CurState)
	{
		case *MenuString::SoundOn:
		case *MenuString::SoundOff:
			NextState = *MenuString::MusicOn;
			break;
		case *MenuString::MusicOn:
		case *MenuString::MusicOff:
			NextState = *MenuString::CyborgOff;
			break;
		case *MenuString::CyborgOff:
		case *MenuString::CyborgNormal:
		case *MenuString::CyborgDouble:
		case *MenuString::CyborgSuper:
			if ((isPC(uqm::UQMOptions::read().smoothScroll) && !usingSpeech) || isPC(uqm::UQMOptions::read().whichMenu))
			{
				NextState = *MenuString::ReadVerySlow;
			}
			else
			{
				NextState = *MenuString::ChangeCaptain;
			}
			break;
		case *MenuString::ReadVerySlow:
		case *MenuString::ReadSlow:
		case *MenuString::ReadModerate:
		case *MenuString::ReadFast:
		case *MenuString::ReadVeryFast:
			NextState = *MenuString::ChangeCaptain;
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

	if (BaseState == MenuString::Starmap)
	{
		AdjBase--;
	}

	switch (AdjBase + CurState)
	{
		case *MenuString::SoundOff:
			NextState = *MenuString::ExitSettings;
			break;
		case *MenuString::MusicOn:
		case *MenuString::MusicOff:
			NextState = *MenuString::SoundOn;
			break;
		case *MenuString::CyborgOff:
		case *MenuString::CyborgNormal:
		case *MenuString::CyborgDouble:
		case *MenuString::CyborgSuper:
			NextState = *MenuString::MusicOn;
			break;
		case *MenuString::ReadVerySlow:
		case *MenuString::ReadSlow:
		case *MenuString::ReadModerate:
		case *MenuString::ReadFast:
		case *MenuString::ReadVeryFast:
			NextState = *MenuString::CyborgOff;
			break;
		case *MenuString::ChangeCaptain:
			if ((isPC(uqm::UQMOptions::read().smoothScroll) && !usingSpeech) || isPC(uqm::UQMOptions::read().whichMenu))
			{
				NextState = *MenuString::ReadVerySlow;
			}
			else
			{
				NextState = *MenuString::CyborgOff;
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
static bool GetAlternateMenu(MenuString* BaseState, uint8_t* CurState)
{
	uint8_t AdjBase = *(*BaseState);
	uint8_t adj = 0;
	if (*BaseState == MenuString::Starmap)
	{
		AdjBase--;
		adj = 1;
	}
	if (*CurState & 0x80)
	{
		switch (*CurState)
		{
			case AltManifest:
				*BaseState = MenuString::AltScan + adj;
				*CurState = MenuString::AltManifest - MenuString::AltScan - adj;
				return true;
			case AltExitManifest:
				*BaseState = MenuString::AltCargo;
				*CurState = MenuString::AltExitManifest - MenuString::AltCargo;
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
			case *MenuString::Scan:
				*BaseState = MenuString::AltScan;
				*CurState = MenuString::AltScan - MenuString::AltScan;
				return true;
			case *MenuString::Starmap:
				*BaseState = MenuString::AltScan + adj;
				*CurState = MenuString::AltStarmap - MenuString::AltScan - adj;
				return true;
			case *MenuString::Devices:
				*BaseState = MenuString::AltCargo;
				*CurState = MenuString::AltDevices - MenuString::AltCargo;
				return true;
			case *MenuString::Cargo:
				*BaseState = MenuString::AltCargo;
				*CurState = MenuString::AltCargo - MenuString::AltCargo;
				return true;
			case *MenuString::Roster:
				*BaseState = MenuString::AltCargo;
				*CurState = MenuString::AltRoster - MenuString::AltCargo;
				return true;
			case *MenuString::GameMenu:
				*BaseState = MenuString::AltScan + adj;
				*CurState = MenuString::AltGameMenu - MenuString::AltScan - adj;
				return true;
			case *MenuString::Navigate:
				*BaseState = MenuString::AltScan + adj;
				*CurState = MenuString::AltNavigate - MenuString::AltScan - adj;
				return true;
			case *MenuString::MinScan:
				*BaseState = MenuString::AltMscan;
				*CurState = MenuString::AltMscan - MenuString::AltMscan;
				return true;
			case *MenuString::EneScan:
				*BaseState = MenuString::AltMscan;
				*CurState = MenuString::AltEscan - MenuString::AltMscan;
				return true;
			case *MenuString::BioScan:
				*BaseState = MenuString::AltMscan;
				*CurState = MenuString::AltBscan - MenuString::AltMscan;
				return true;
			case *MenuString::ExitScan:
				*BaseState = MenuString::AltMscan;
				*CurState = MenuString::AltExitScan - MenuString::AltMscan;
				return true;
			case *MenuString::AutoScan:
				*BaseState = MenuString::AltMscan;
				*CurState = MenuString::AltAscan - MenuString::AltMscan;
				return true;
			case *MenuString::LaunchLander:
				*BaseState = MenuString::AltMscan;
				*CurState = MenuString::AltDispatch - MenuString::AltMscan;
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
		case *MenuString::AltScan:
			return (MenuString::Scan - MenuString::Scan);
		case *MenuString::AltStarmap:
			return (MenuString::Starmap - MenuString::Scan);
		case *MenuString::AltManifest:
			return (AltManifest);
		case *MenuString::AltGameMenu:
			return (MenuString::GameMenu - MenuString::Scan);
		case *MenuString::AltNavigate:
			return (MenuString::Navigate - MenuString::Scan);
		case *MenuString::AltCargo:
			return (MenuString::Cargo - MenuString::Scan);
		case *MenuString::AltDevices:
			return (MenuString::Devices - MenuString::Scan);
		case *MenuString::AltRoster:
			return (MenuString::Roster - MenuString::Scan);
		case *MenuString::AltExitManifest:
			return (AltExitManifest);
		case *MenuString::AltMscan:
			return (MenuString::MinScan - MenuString::MinScan);
		case *MenuString::AltEscan:
			return (MenuString::EneScan - MenuString::MinScan);
		case *MenuString::AltBscan:
			return (MenuString::BioScan - MenuString::MinScan);
		case *MenuString::AltAscan:
			return (MenuString::AutoScan - MenuString::MinScan);
		case *MenuString::AltDispatch:
			return (MenuString::LaunchLander - MenuString::MinScan);
		case *MenuString::AltExitScan:
			return (MenuString::ExitScan - MenuString::MinScan);
	}
	return (NewState);
}

bool DoMenuChooser(MenuState* pMS, MenuString BaseState)
{
	uint8_t NewState = pMS->CurState;
	const MenuString OrigBase {BaseState};
	bool useAltMenu = false;
	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		useAltMenu = GetAlternateMenu(&BaseState, &NewState);
	}
	if (PulsedInputState.menu[*MenuKey::MenuLeft] || PulsedInputState.menu[*MenuKey::MenuUp])
	{
		NewState = PreviousMenuState(*BaseState, NewState);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuRight] || PulsedInputState.menu[*MenuKey::MenuDown])
	{
		NewState = NextMenuState(*BaseState, NewState);
	}
	else if (useAltMenu && PulsedInputState.menu[*MenuKey::MenuSelect])
	{
		NewState = ConvertAlternateMenu(*BaseState, NewState);
		if (NewState == AltManifest)
		{
			DrawMenuStateStrings(MenuString::AltCargo, 0);
			pMS->CurState = MenuString::Cargo - MenuString::Scan;
			return true;
		}
		if (NewState == AltExitManifest)
		{
			if (OrigBase == MenuString::Scan)
			{
				DrawMenuStateStrings(MenuString::AltScan,
									 MenuString::AltManifest - MenuString::AltScan);
			}
			else
			{
				DrawMenuStateStrings(MenuString::AltStarmap,
									 MenuString::AltManifest - MenuString::AltStarmap);
			}
			pMS->CurState = AltManifest;
			return true;
		}
		return false;
	}
	else if ((uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC) && PulsedInputState.menu[*MenuKey::MenuCancel] && (BaseState == MenuString::AltCargo))
	{
		if (OrigBase == MenuString::Scan)
		{
			DrawMenuStateStrings(MenuString::AltScan,
								 MenuString::AltManifest - MenuString::AltScan);
		}
		else
		{
			DrawMenuStateStrings(MenuString::AltStarmap,
								 MenuString::AltManifest - MenuString::AltStarmap);
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
		NewState = ConvertAlternateMenu(*BaseState, NewState);
	}
	pMS->CurState = NewState;

	return true;
}

const char* IndexToText(int Index)
{
	int i = -1;

	if (Index < MenuString::ExitGameMenu)
	{
		i = Index;
	}

	switch (Index)
	{
		case *MenuString::EncounterGameMenu:
		case *MenuString::OutfitGameMenu:
		case *MenuString::ShipyardGameMenu:
			i = *GameMenuString::GameMenu;
			break;
		case *MenuString::ExitGameMenu:
		case *MenuString::ExitOutfit:
		case *MenuString::ExitShipyard:
		case *MenuString::ExitSettings:
			i = *GameMenuString::ExitMenu;
			break;
		case *MenuString::Converse:
			i = *GameMenuString::Converse;
			break;
		case *MenuString::Attack:
			i = *GameMenuString::Attack;
			break;
		case *MenuString::Fuel:
			i = *GameMenuString::Fuel;
			break;
		case *MenuString::Module:
			i = *GameMenuString::Module;
			break;
		case *MenuString::Crew:
			i = *GameMenuString::Crew;
			break;
		case *MenuString::SoundOn:
			i = *GameMenuString::SndOn;
			break;
		case *MenuString::SoundOff:
			i = *GameMenuString::SndOff;
			break;
		case *MenuString::MusicOn:
			i = *GameMenuString::MusOn;
			break;
		case *MenuString::MusicOff:
			i = *GameMenuString::MusOff;
			break;
		case *MenuString::CyborgOff:
		case *MenuString::CyborgNormal:
		case *MenuString::CyborgDouble:
		case *MenuString::CyborgSuper:
			i = *GameMenuString::Combat;
			break;
		case *MenuString::ReadVerySlow:
		case *MenuString::ReadSlow:
		case *MenuString::ReadModerate:
		case *MenuString::ReadFast:
		case *MenuString::ReadVeryFast:
			i = *GameMenuString::Reading;
			break;
		case *MenuString::ChangeCaptain:
			i = *GameMenuString::ChangeCap;
			break;
		case *MenuString::ChangeShip:
			i = *GameMenuString::ChangeSis;
			break;
		default:
			break;
	}

	if (i == -1 || !strlen(GameString(GameStringBase::PlaymenuString + i)))
	{
		return nullptr;
	}

	return GameString(GameStringBase::PlaymenuString + i);
}

static void Draw3DOMenuText(GfxRect* r, int Index)
{
	GfxText text;
	int16_t leading {};
	GfxRect block;
	Color OldColor;

	if (IndexToText(Index) == nullptr)
	{
		return;
	}

	FontRef OldFont {SetContextFont(PlayMenuFont)};
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
		DrawBorder(BorderFrameIndex::TextLabel);
	}

	text.align = TextAlign::Center;
	text.baseline.x = r->corner.x + (r->extent.width >> 1);
	text.baseline.y = r->corner.y + leading - NdosNumScl(2);
	text.pStr = AlignText(IndexToText(Index),
						  &text.baseline.x);
	text.CharCount = (uint16_t)~0;

	font_DrawShadowedText(&text, ShadowAngle::NorthWest, PmTextColor,
						  PmShadowColor);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

void DrawMenuStateStrings(MenuString beg_index, int16_t NewState)
{
	GfxRect r;
	GfxStamp s;
	uint8_t hilite = 1;
	extern FrameRef PlayFrame;

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
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

	if (beg_index == MenuString::Starmap)
	{
		NewState--;
	}
	uint8_t end_index {GetEndMenuState(*beg_index)};

	s.frame = 0;
	if (NewState <= end_index - *beg_index)
	{
		s.frame = SetAbsFrameIndex(PlayFrame, beg_index + NewState);
	}

	PreUpdateFlashRect();
	GfxContextRef OldContext {SetContext(StatusContext)};
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
		if (beg_index == MenuString::Crew)
		{
			fmt::format_to_sz_n(pm_crew_str, "{}({})",
								GameString(GameStringBase::MainmenuString + static_cast<int>(MenuString::Crew)),
								uqm::gameState().game().CrewCost);
		}
		if (beg_index == MenuString::Fuel)
		{
			fmt::format_to_sz_n(pm_fuel_str, "{}({})",
								GameString(GameStringBase::MainmenuString + static_cast<int>(MenuString::Fuel)),
								uqm::gameState().game().FuelCost);
		}
		if (beg_index == MenuString::SoundOn)
		{
			end_index = *beg_index + 6;

			switch (beg_index + NewState)
			{
				case MenuString::SoundOn:
				case MenuString::SoundOff:
					NewState = 0;
					break;
				case MenuString::MusicOn:
				case MenuString::MusicOff:
					NewState = 1;
					break;
				case MenuString::CyborgOff:
				case MenuString::CyborgNormal:
				case MenuString::CyborgDouble:
				case MenuString::CyborgSuper:
					NewState = 2;
					break;
				case MenuString::ReadVerySlow:
				case MenuString::ReadSlow:
				case MenuString::ReadModerate:
				case MenuString::ReadFast:
				case MenuString::ReadVeryFast:
					NewState = 3;
					break;
				case MenuString::ChangeCaptain:
					NewState = 4;
					break;
				case MenuString::ChangeShip:
					NewState = 5;
					break;
				case MenuString::ExitSettings:
					NewState = 6;
					break;
				default:
					break;
			}
		}
		r.extent.height = RadarHeight() + ResScale(12);

		DrawPCMenu(*beg_index, end_index, (uint8_t)NewState, hilite, &r);
		s.frame = 0;

		if (uqm::UQMOptions::read().submenu)
		{
			FunkyMenu(*beg_index + (uint8_t)NewState, s);
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
			DrawBorder(BorderFrameIndex::SisRadar);
		}
	}
	if (s.frame)
	{
		if (uqm::UQMOptions::read().submenu)
		{
			FunkyMenu(*beg_index + (uint8_t)NewState, s);
		}
		else
		{
			DrawStamp(&s);
		}

		Draw3DOMenuText(&r, *beg_index + NewState);

		GfxText t;
		uqstl::array<char, 20> buf;

		switch (beg_index + NewState)
		{
			case MenuString::Crew:
				t.baseline.x = s.origin.x + RadarWidth() - ResScale(2);
				t.baseline.y = s.origin.y + RadarHeight() - ResScale(2);
				t.align = TextAlign::Right;
				t.CharCount = (uint16_t)~0;
				t.pStr = buf.data();
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
			case MenuString::Fuel:
				t.baseline.x = s.origin.x + RadarWidth() - ResScale(2);
				t.baseline.y = s.origin.y + RadarHeight() - ResScale(2);
				t.align = TextAlign::Right;
				t.CharCount = (uint16_t)~0;
				t.pStr = buf.data();
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
			default:
				break;
		}
	}
	UnbatchGraphics();
	SetContext(OldContext);
	PostUpdateFlashRect();
}

void DrawMineralHelpers(void)
{
	GfxStamp s;
	GfxText t;
	uqstl::array<char, 40> buf;
	GfxRect r;
	int16_t leading {};

	if (!uqm::UQMOptions::read().submenu)
	{
		return;
	}

	GfxContextRef OldContext {SetContext(StatusContext)};

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

	// setup Element icons
	s.frame = SetAbsFrameIndex(MiscDataFrame,
							   (NumScandotTransitions << 1) + 3);
	s.origin.x = ElementCol0();
	s.origin.y = ElementOrgY();
	// setup Element worths
	t.baseline.x = ElementCol1();
	t.baseline.y = ElementOrgY() + ResScale(3);
	t.align = TextAlign::Right;
	t.pStr = buf.data();
	t.CharCount = (uint16_t)~0;

	r.extent = GetFrameBounds(s.frame);
	r.corner.x = ElementCol0() - ResScale(3);

	// draw Element icons and worths
	for (uint16_t i {0}; i < MineralCategory::NumElementCategories; ++i)
	{
		// draw Element icon
		DrawStamp(&s);
		s.frame = SetRelFrameIndex(s.frame, 5);
		s.origin.y += ElementSpacingY();

		// print Element worth
		SetContextForeGroundColor(ModulePriceColor);
		fmt::format_to_sz_n(buf, "{}", uqm::gameState().game().ElementWorth[i]);
		font_DrawText(&t);

		// print x'es
		fmt::format_to_sz_n(buf, "x");
		t.baseline.x -= ResScale(11);
		font_DrawText(&t);

		// print the Element amount
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
	t.align = TextAlign::Left;
	t.baseline.x = r.corner.x + ResScale(2);
	t.baseline.y = r.corner.y + leading + ResScale(1);
	t.pStr = GameString(GameStringBase::StatusString + 8); // FUEL:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	t.pStr = GameString(GameStringBase::StatusString + 9); // CREW:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	t.pStr = GameString(GameStringBase::StatusString + 10); // LAND.:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	t.pStr = GameString(GameStringBase::StatusString + 11); // CARGO:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	SetContextForeGroundColor(ModulePriceColor);
	t.align = TextAlign::Right;
	t.baseline.x = r.extent.width + ResScale(2);
	t.baseline.y = r.corner.y + leading + ResScale(1);
	t.pStr = WholeFuelValue();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().CrewEnlisted);
	t.pStr = buf.data();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	t.baseline.y += leading;
	fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().NumLanders);
	t.pStr = buf.data();
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

	t.pStr = buf.data();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	UnbatchGraphics();

	SetContext(OldContext);
}

void DrawBorder(BorderFrameIndex Visible)
{
	GfxStamp s;

	GfxContextRef OldContext {SetContext(ScreenContext)};

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

void FunkyMenu(uint8_t m, GfxStamp stmp)
{
	static bool subMenuFlag;

	if ((!stmp.frame && m >= MenuString::AltMscan
		 && m <= MenuString::AltExitScan)
		|| m == MenuString::LaunchLander)
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

} // namespace uqm
