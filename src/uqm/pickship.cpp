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

#include "pickship.h"

#include "battle.h"
#include "build.h"
#include "colors.h"
#include "controls.h"
#include "encount.h"
#include "libs/mathlib.h"
#include "menustat.h"
#include "nameref.h"
#include "races.h"
#include "resinst.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "supermelee/pickmele.h"
#include "util.h"

namespace uqm
{
static constexpr auto NumPickShipRows {2};
static constexpr auto NumPickShipColumns {6};

static auto FlagshipXOffs() { return ResScale(65); }
static auto FlagshipYOffs() { return ResScale(4); }
static auto FlagshipWidth() { return ResScale(22); }
static auto FlagshipHeight() { return ResScale(48); }

FrameRef PickFrame;
GfxPoint frameOrigin;

void InitPickFrame(void)
{
	PickFrame = CaptureDrawable(LoadGraphic(Sc2PickPmapAnimRes));
	PickFrame = SetAbsFrameIndex(PickFrame, isPC(uqm::UQMOptions::read().flagshipColor) ? 0 : 2);
}

void DestroyPickFrame(void)
{
	DestroyDrawable(ReleaseDrawable(PickFrame));
	PickFrame = 0;
}

void RepairPickFrame(GfxRect* pRect, uint16_t frame)
{
	GfxRect OldRect;
	GfxStamp s;
	GfxRect r;

	GetContextClipRect(&OldRect);

	r.corner.x = pRect->corner.x + OldRect.corner.x;
	r.corner.y = pRect->corner.y + OldRect.corner.y;
	r.extent = pRect->extent;

	SetContextClipRect(&r);

	s.origin.x = frameOrigin.x - pRect->corner.x;
	s.origin.y = frameOrigin.y - pRect->corner.y;
	s.frame = SetAbsFrameIndex(PickFrame, frame);
	DrawStamp(&s);

	SetContextClipRect(&OldRect);
}

static bool DoPickBattleShip(MenuState* pMS)
{
	GfxRect r {};
	GfxText t {};
	int dx = 0;
	int dy = 0;

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		pMS->CurFrame = 0;
		return (false);
	}

	SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);

	if (!pMS->Initialized)
	{
		pMS->Initialized = true;
		pMS->InputFunc = DoPickBattleShip;

		goto ChangeSelection;
	}
	else if (PulsedInputState.menu[*MenuKey::MenuSelect])
	{
		if ((StarshipHandle)pMS->CurFrame)
		{
			PlayMenuSound(MenuSoundEffect::Success);
			return (false);
		}
	}
	else
	{
		int16_t new_row; // NOLINT(cppcoreguidelines-init-variables)
		int16_t new_col; // NOLINT(cppcoreguidelines-init-variables)
		if (PulsedInputState.menu[*MenuKey::MenuRight])
		{
			dx = 1;
		}
		if (PulsedInputState.menu[*MenuKey::MenuLeft])
		{
			dx = -1;
		}
		if (PulsedInputState.menu[*MenuKey::MenuUp])
		{
			dy = -1;
		}
		if (PulsedInputState.menu[*MenuKey::MenuDown])
		{
			dy = 1;
		}

		new_col = pMS->first_item.x + dx;
		new_row = pMS->first_item.y + dy;
		if (new_row != pMS->first_item.y
			|| new_col != pMS->first_item.x)
		{
			uint16_t crew_level;		 // NOLINT(cppcoreguidelines-init-variables)
			uint16_t max_crew;			 // NOLINT(cppcoreguidelines-init-variables)
			uint16_t ship_index;		 // NOLINT(cppcoreguidelines-init-variables)
			StarshipHandle hBattleShip;	 // NOLINT(cppcoreguidelines-init-variables)
			StarshipHandle hNextShip;	 // NOLINT(cppcoreguidelines-init-variables)
			const Starship* StarShipPtr; // NOLINT(cppcoreguidelines-init-variables)

			if (new_col < 0)
			{
				new_col = NumPickShipColumns;
			}
			else if (new_col > NumPickShipColumns)
			{
				new_col = 0;
			}

			if (new_row < 0)
			{
				new_row = NumPickShipRows - 1;
			}
			else if (new_row == NumPickShipRows)
			{
				new_row = 0;
			}

			PlayMenuSound(MenuSoundEffect::Move);


#ifdef NEVER
			SetContextForeGroundColor(
				BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x1D));
			DrawRectangle(&pMS->flash_rect0, false);
#endif /* NEVER */
			pMS->first_item.y = new_row;
			pMS->first_item.x = new_col;

ChangeSelection:
			if (pMS->first_item.x == (NumPickShipColumns >> 1))
			{
				pMS->flash_rect0.corner.x =
					pMS->flash_rect1.corner.x - ResScale(2) + FlagshipXOffs();
				pMS->flash_rect0.corner.y =
					pMS->flash_rect1.corner.y - ResScale(2) + FlagshipYOffs();
				pMS->flash_rect0.extent.width = FlagshipWidth() + ResScale(4);
				pMS->flash_rect0.extent.height = FlagshipHeight() + ResScale(4);

				hBattleShip = GetTailLink(&race_q[0]); /* Flagship */
			}
			else
			{
				new_col = pMS->first_item.x;
				pMS->flash_rect0.corner.x = ResScale(5) + pMS->flash_rect1.corner.x
										  - ResScale(2) + ((IconWidth() + ResScale(4)) * new_col);
				if (new_col > (NumPickShipColumns >> 1))
				{
					--new_col;
					pMS->flash_rect0.corner.x += FlagshipWidth() - IconWidth();
				}
				pMS->flash_rect0.corner.y = ResScale(16) + pMS->flash_rect1.corner.y
										  - ResScale(2) + ((IconHeight() + ResScale(4)) * pMS->first_item.y);
				pMS->flash_rect0.extent.width = IconWidth() + ResScale(4);
				pMS->flash_rect0.extent.height = IconHeight() + ResScale(4);

				ship_index = (pMS->first_item.y * NumPickShipColumns)
						   + new_col;

				for (hBattleShip = GetHeadLink(&race_q[0]);
					 hBattleShip != GetTailLink(&race_q[0]);
					 hBattleShip = hNextShip)
				{
					StarShipPtr = LockStarShip(&race_q[0], hBattleShip);
					if (StarShipPtr->index == ship_index
						&& (StarShipPtr->SpeciesID != SpeciesId::None))
					{
						UnlockStarShip(&race_q[0], hBattleShip);
						break;
					}

					hNextShip = _GetSuccLink(StarShipPtr);
					UnlockStarShip(&race_q[0], hBattleShip);
				}

				if (hBattleShip == GetTailLink(&race_q[0]))
				{
					hBattleShip = 0;
				}
			}

			pMS->CurFrame = (FrameRef)hBattleShip;

			SetContextForeGroundColor(BlackColor);
			r.corner.x = pMS->flash_rect1.corner.x + ResScale(6) - ResScale(1);
			r.corner.y = pMS->flash_rect1.corner.y + ResScale(5) - ResScale(1);
			r.extent.width = ((IconWidth() + ResScale(4)) * 3) - ResScale(4);
			r.extent.height = ResScale(7);

			if (IsHd())
			{
				RepairPickFrame(&r, 0);
			}
			else
			{
				DrawFilledRectangle(&r);
			}

			if (hBattleShip == 0)
			{
				crew_level = 0;
				max_crew = 0;
				// Satisfy compiler.
			}
			else
			{
				SetContextFont(isPC(uqm::UQMOptions::read().whichFonts) ? TinyFont : TinyFontBold);

				t.baseline.x = r.corner.x + (r.extent.width >> 1);
				t.baseline.y = r.corner.y + (r.extent.height - ResScale(1));
				t.align = TextAlign::Center;

				StarShipPtr = LockStarShip(&race_q[0], hBattleShip);
				if (StarShipPtr->captains_name_index == 0)
				{
					t.pStr = uqm::gameState().sis().CommanderName;
					t.CharCount = (uint16_t)~0;
					crew_level = uqm::gameState().sis().CrewEnlisted;
					max_crew = GetCrewPodCapacity();
				}
				else
				{
					t.pStr = StarShipPtr->race_strings->text(StarShipPtr->captains_name_index);
					t.CharCount = (uint16_t)~0;
					crew_level = StarShipPtr->crew_level;
					max_crew = StarShipPtr->max_crew;
				}
				UnlockStarShip(&race_q[0], hBattleShip);

				// Code to make use of the PC version's font gradient
				/*if (isPC (uqm::UQMOptions::read().whichFonts))
					SetContextForeGroundColor (WhiteColor);
				else
					SetContextForeGroundColor (
						BuildColor (MakeRgb15 (0x14, 0x0A, 0x00), 0x0C));*/

				SetContextForeGroundColor(
					BuildColor(MakeRgb15(0x14, 0x0A, 0x00), 0x0C));

				font_DrawText(&t);
				SetContextForeGroundColor(BlackColor);
			}

			r.corner.x += (IconWidth() + ResScale(4))
							* ((NumPickShipColumns >> 1) + 1)
						+ FlagshipWidth() - IconWidth();

			if (IsHd())
			{
				RepairPickFrame(&r, 0);
			}
			else
			{
				DrawFilledRectangle(&r);
			}

			if (crew_level)
			{
				char buf[80] {};

				t.baseline.x = r.corner.x + (r.extent.width >> 1);
				t.pStr = buf;
				t.CharCount = (uint16_t)~0;
				if (crew_level >= max_crew)
				{
					fmt::format_to_n(buf, sizeof(buf) - 1, "{}", crew_level);
				}
				else
				{
					fmt::format_to_n(buf, sizeof(buf) - 1, "{}/{}", crew_level, max_crew);
				}

				// Code to make use of the PC version's font gradient
				/*if (isPC (uqm::UQMOptions::read().whichFonts))
					SetContextForeGroundColor (WhiteColor);
				else
					SetContextForeGroundColor (
							BuildColor (MakeRgb15 (0x00, 0x14, 0x00), 0x02));*/

				SetContextForeGroundColor(
					BuildColor(MakeRgb15(0x00, 0x14, 0x00), 0x02));
				font_DrawText(&t);
			}

			SetFlashRect(nullptr, false);
			SetFlashRect(&pMS->flash_rect0, uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
		}
	}

	return (true);
}

static StarshipHandle GetArmadaStarShip(void)
{
	GfxRect pick_r;
	StarshipHandle hBattleShip {};

	if (battle_counter[1] == 0)
	{
		// No opponents left.
		return 0;
	}

	//    MenuSounds = CaptureSound (LoadSound (MenuSoundsRes));

	InitPickFrame();
	GfxContextRef OldContext {SetContext(SpaceContext)};
	DrawArmadaPickShip(false, &pick_r);

	{
		MenuState menuState;

		menuState.InputFunc = DoPickBattleShip;
		menuState.Initialized = false;
		menuState.first_item.x = NumPickShipColumns >> 1;
		menuState.first_item.y = 0;
		menuState.CurFrame = 0;
		menuState.flash_rect1.corner = pick_r.corner;
		menuState.flash_rect1.extent.width = 0;

		SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
		DoInput(&menuState, false);

		SetFlashRect(nullptr, false);

		hBattleShip = (StarshipHandle)menuState.CurFrame;
	}

	if (hBattleShip)
	{
		if (hBattleShip == GetTailLink(&race_q[0]))
		{ // Player chose SIS. There will be no more choices.
			battle_counter[RpgPlayerNum] = 1;
		}

		WaitForSoundEnd(0);
	}

	//    DestroySound (ReleaseSound (MenuSounds));

	SetContext(OldContext);
	DestroyPickFrame();

	return (hBattleShip);
}

static constexpr auto RunAwayFuelCost {5 * FuelTankScale};

// Get the next ship to use.
StarshipHandle GetEncounterStarShip(Starship* LastStarShipPtr, uint16_t which_player)
{
	if (inHQSpace())
	{
		assert(which_player == RpgPlayerNum);
		// SIS for the Hyperspace flight
		return GetHeadLink(&race_q[which_player]);
	}
	else if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::SuperMelee)
	{
		// Let the player chose their own ship. (May be a computer player).
		StarshipHandle hBattleShip {};

		if (battle_counter[0] == 0 || battle_counter[1] == 0)
		{ // One side is out of ships. Game over.
			return 0;
		}

		if (!GetNextMeleeStarShip(which_player, &hBattleShip))
		{
			return 0;
		}

		return hBattleShip;
	}
	else
	{
		// Full game.
		if (which_player == RpgPlayerNum)
		{ // Human player in a full game.
			if (LastStarShipPtr == 0 && battle_counter[which_player] == 1)
			{ // First time picking a ship and player has no escorts
				// SIS is the last ship in queue (though there is only one)
				return GetTailLink(&race_q[which_player]);
			}
			else if (battle_counter[which_player])
			{ // Player still has ships left
				return GetArmadaStarShip();
			}
			else if (LastStarShipPtr != 0)
			{ // last ship was the flagship
				if (LastStarShipPtr->crew_level == 0)
				{ // Died in the line of duty
					uqm::gameState().sis().CrewEnlisted = (uint16_t)~0;
					DeathByMelee = true;
				}
				else
				{ // Player ran away
					if (uqm::gameState().sis().FuelOnBoard > RunAwayFuelCost && !uqm::UQMOptions::read().infiniteFuel)
					{
						uqm::gameState().sis().FuelOnBoard -= RunAwayFuelCost;
					}
					else
					{
						uqm::gameState().sis().FuelOnBoard = (uqm::UQMOptions::read().infiniteFuel ? uqm::gameState().sis().FuelOnBoard : 0);
					}
				}
			}
			return 0;
		}
		else
		{ // NPC player in a full game
			if (FleetIsInfinite(which_player))
			{
				if (LastStarShipPtr != 0)
				{ // The current Starship is reused for the next one;
					// update with new info
					// XXX: Note that if Syreen had a homeworld you could
					//   fight, all Syreen ships there would be crewed to
					//   the maximum, instead of the normal level
					LastStarShipPtr->crew_level = LastStarShipPtr->max_crew;
					LastStarShipPtr->playerNr = which_player;
					LastStarShipPtr->captains_name_index = PickCaptainName();
				}
				battle_counter[which_player]++;

				return GetHeadLink(&race_q[which_player]);
			}

			// Get the next ship for the computer
			if (LastStarShipPtr != 0)
			{
				return _GetSuccLink(LastStarShipPtr);
			}

			// Get the very first ship for the computer
			return GetHeadLink(&race_q[which_player]);
		}
	}
}

void DrawArmadaPickShip(bool draw_salvage_frame, GfxRect* pPickRect)
{
	StarshipHandle hBattleShip {};
	StarshipHandle hNextShip {};
	const Starship* StarShipPtr {};
	GfxRect r;
	GfxRect pick_r;
	GfxStamp s;
	GfxText t;

	GfxContextRef OldContext {SetContext(SpaceContext)};

	BatchGraphics();

	s.frame = PickFrame;
	GetFrameRect(s.frame, &pick_r);
	GetContextClipRect(&r);
	pick_r.corner.x = (r.extent.width >> 1) - (pick_r.extent.width >> 1);
	pick_r.corner.y = (r.extent.height >> 1) - (pick_r.extent.height >> 1);

	if (!draw_salvage_frame)
	{
		*pPickRect = pick_r;
		frameOrigin = pick_r.corner;
	}
	else
	{
		s.origin.x = r.extent.width >> 1;
		s.frame = SetAbsFrameIndex(s.frame, 1);
		GetFrameRect(s.frame, &r);
		s.origin.x -= r.extent.width >> 1;
		s.origin.y = pick_r.corner.y - (r.extent.height >> 1);
		DrawStamp(&s);

		frameOrigin = s.origin;

		s.frame = SetAbsFrameIndex(s.frame, isPC(uqm::UQMOptions::read().flagshipColor) ? 0 : 2);
		pick_r.corner.y = s.origin.y + r.extent.height;

		r.corner.x = pick_r.corner.x;
		r.corner.y = s.origin.y;
		*pPickRect = r;
	}
	s.origin = pick_r.corner;
	DrawStamp(&s);

	FrameRef OldFontEffect {SetContextFontEffect(nullptr)};

	t.baseline.x = pick_r.corner.x + (pick_r.extent.width >> 1);
	t.baseline.y = pick_r.corner.y + pick_r.extent.height - ResScale(5);
	t.align = TextAlign::Center;
	t.pStr = uqm::gameState().sis().ShipName;
	t.CharCount = (uint16_t)~0;

	// Code to make use of the PC version's font gradient
	// Along with the FrameRef "OldFontEffect"
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFontEffect(SetAbsFrameIndex(FontGradFrame, 6));
	}
	else
	{
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x12, 0x12, 0x12), 0x17));
	}

	SetContextFont(StarConFont);
	font_DrawText(&t);

	SetContextFontEffect(OldFontEffect);

	r.extent.width = IconWidth();
	r.extent.height = IconHeight();
	for (hBattleShip = GetHeadLink(&race_q[0]);
		 hBattleShip != 0; hBattleShip = hNextShip)
	{
		StarShipPtr = LockStarShip(&race_q[0], hBattleShip);

		if (StarShipPtr->captains_name_index)
		{ // Escort ship, not SIS

			const uint16_t ship_index {StarShipPtr->index};

			s.origin.x = pick_r.corner.x
					   + (ResScale(5) + ((IconWidth() + ResScale(4)) * (ship_index % NumPickShipColumns)));
			if ((ship_index % NumPickShipColumns) >= (NumPickShipColumns >> 1))
			{
				s.origin.x += FlagshipWidth() + ResScale(4);
			}
			s.origin.y = pick_r.corner.y
					   + (ResScale(16) + ((IconHeight() + ResScale(4)) * (ship_index / NumPickShipColumns)));
			s.frame = StarShipPtr->icons;
			r.corner = s.origin;

			SetContextForeGroundColor(BlackColor);
			DrawFilledRectangle(&r);

			if ((StarShipPtr->SpeciesID != SpeciesId::None) || (StarShipPtr->crew_level == 0))
			{
				DrawStamp(&s);
				if (StarShipPtr->SpeciesID == SpeciesId::None)
				{
					/* Dead ship - mark with an X. */
					s.origin.x -= ResScale(1);
					s.frame = SetAbsFrameIndex(StatusFrame, 3);
					DrawStamp(&s);
				}
			}
			else
			{
				/* Ship ran away */
				SetContextForeGroundColor(
					BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01));
				DrawFilledStamp(&s);
			}
		}

		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockStarShip(&race_q[0], hBattleShip);
	}

	UnbatchGraphics();

	SetContext(OldContext);
}

} // namespace uqm
