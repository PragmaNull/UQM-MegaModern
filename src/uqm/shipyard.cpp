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
#include "build.h"
#include "colors.h"
#include "controls.h"
#include "menustat.h"
#include "fmv.h"
#include "gameopt.h"
#include "gamestr.h"
#include "supermelee/melee.h"
#include "master.h"
#include "options.h"
#include "races.h"
#include "nameref.h"
#include "resinst.h"
#include "settings.h"
#include "starbase.h"
#include "setup.h"
#include "sis.h"
#include "units.h"
#include "sounds.h"
#include "core/string/StringUtils.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "uqmdebug.h"
#include "libs/graphics/drawable.h"
#include "util.h"
#include "shipcont.h"

static auto DosMenu() { return (uqm::UQMOptions::read().dosMenus || IsDos()); } // The DOS Menu window

// 3DO 4x3 hangar layout
static constexpr auto HangarShipsRow3do {4};
static constexpr auto HangarY3do {64};
static constexpr auto HangarDy3do {44};

const int16_t hangar_x_coords_3do[HangarShipsRow3do] =
	{
		19, 60, 116, 157};

// modified PC 6x2 hangar layout
static constexpr auto HangarShipsRowDos {6};
// The Y position of the upper line of hangar bay doors.
static auto HangarY() { return ResScale(SafeBool(DosBool(88, 71), HangarY3do)); }
// The Y position of the lower line of hangar bay doors.
static auto HangarDy() { return ResScale(SafeBool(DosBool(84, 76), HangarDy3do)); }

// The X positions of the hangar bay doors for each resolution mode.
// Calculated from the right edge of the left grey border bar on the
// screen.
const int16_t hangar_x_coords_uqm[HangarShipsRowDos] =
	{
		0, 38, 76, 131, 169, 207};

const int16_t hangar_x_coords_dos[HangarShipsRowDos] =
	{
		0, 38, 76, 133, 171, 209};

int16_t hangar_x_coords[HangarShipsRowDos];

static constexpr auto HangarShips {12};
static auto HangarShipsRow() { return SafeBool(HangarShipsRowDos, HangarShipsRow3do); }

static auto HangarRows() { return (HangarShips / HangarShipsRow()); }

static auto HangarAnimRate() { return ResScale(15); } // fps

enum class ShipyardMenuItem
{
	Crew,
	SaveLoad,
	Exit
};
consteval bool isArithmeticEnum(ShipyardMenuItem) { return true; }

// Editing mode for DoModifyShips()
enum class DmsMode
{
	Navigate,  // Navigating the ship slots.
	AddEscort, // Selecting a ship to add to an empty slot.
	EditCrew,  // Hiring or dismissing crew.
	Exit,	   // Leaving DoModifyShips() mode.
};

static bool DoShipSpins;

static bool stowed_q;				// Build queue or storage queue
static uint8_t currentStowShip = 0; // Saves location in storage queue
static uint16_t stowCount = 0;		// Number of ships stowed
static uint16_t availableCount = 0; // Number of ships available to purchase

// This is all for drawing the DOS version modules menu
static auto ShipsOrgY() { return ResScale(36); }
static auto ShipsSpacingY() { return ResScale(16); }

static auto ShipsCol0() { return ResScale(5); }
static auto ShipsCol1() { return ResScale(61); }

static auto ShipsSelOrgX() { return (ShipsCol0() - ResScale(1)); }
static auto ShipsSelWidth() { return (FieldWidth() - ResScale(3)); }

static auto NameOfsY() { return ResScale(2); }
static auto TextBaseline() { return ResScale(6); }
static auto TextSpacingY() { return ResScale(7); }

static constexpr auto MaxVisModules {5};

struct ShipStatsEntry
{
	char ShipName[30];
	uint16_t ShipCost;
	uint8_t CrewLevel, MaxCrew;
};

struct ShipyardShipsState
{
	uint16_t count;	   // Number of buildable ships in the list
	uint16_t topIndex; // Index of the top ship displayed
	ShipStatsEntry ShipStats[*RaceId::NumBuildableShips + MaxStowedShips];
};

ShipyardShipsState ShipState;

// Count the ships in the stowed queue
static uint16_t GetStowedShipCount(void)
{
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};

	uint16_t Index {0};
	for (hStarShip = GetHeadLink(&uqm::gameState().game().stowed_ship_q);
		 hStarShip; hStarShip = hNextShip)
	{
		const ShipFragment* StarShipPtr {LockShipFrag(&uqm::gameState().game().stowed_ship_q, hStarShip)};
		++Index;
		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStarShip);
	}

	return Index;
}

uint16_t ShipPoints(HFLEETINFO hFleet);

static void DrawShipsStatus(uint16_t index, uint16_t pos, bool selected)
{
	GfxRect r;
	GfxText t;
	char buf[10];

	if (stowed_q)
	{
		index += *RaceId::NumBuildableShips;
	}
	t.align = TextAlign::Left;
	t.baseline.x = ShipsCol0();

	r.extent.width = ShipsSelWidth();
	r.extent.height = TextSpacingY() * 2;
	r.corner.x = ShipsSelOrgX();

	// draw line background
	r.corner.y = ShipsOrgY() + pos * ShipsSpacingY() + NameOfsY();
	SetContextForeGroundColor(selected ?
								  ModuleSelectedBackColor :
								  ModuleBackColor);
	DrawFilledRectangle(&r);
	SetContextFont(TinyFont);

	// print ship name or captain name
	SetContextForeGroundColor(selected ? ModuleSelectedColor :
										 (stowed_q ? CaptainNameTextColor : ModuleNameColor));
	t.baseline.y = r.corner.y + TextBaseline();
	t.pStr = ShipState.ShipStats[index].ShipName;
	t.CharCount = (stowed_q ? (uint16_t)~0 : uqm::utf8StringPos(t.pStr, ' '));
	font_DrawText(&t);
	if (!stowed_q)
	{
		// print second half of ship name
		t.baseline.y += TextSpacingY();
		t.pStr = uqm::skipUTF8Chars(t.pStr, t.CharCount + 1);
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
	// print ship cost or fleet points
	SetContextForeGroundColor(selected ?
								  ModuleSelectedColor :
								  ModulePriceColor);
	t.align = TextAlign::Right;
	t.baseline.x = ShipsCol1() - ResScale(2);
	if (!stowed_q)
	{
		t.baseline.y -= ResScale(3);
	}
	fmt::format_to_sz_n(buf, "{}", ShipState.ShipStats[index].ShipCost);
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
	if (stowed_q)
	{
		// print crew totals
		t.baseline.y += TextSpacingY();
		fmt::format_to_sz_n(buf, "{}/{}",
							ShipState.ShipStats[index].CrewLevel,
							ShipState.ShipStats[index].MaxCrew);
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
}

static void DrawShipsDisplay(ShipyardShipsState* shipState)
{
	GfxText t;
	GfxRect r;

	r.corner.x = ResScale(2);
	r.corner.y = ResScale(20);
	r.extent.width = FieldWidth() + ResScale(1);
	r.extent.height = (ResScale(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, ResScale(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, ModuleBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(BorderFrameIndex::DeviceCargo);
	}

	// print the "SHIPS" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - ResScale(1);
	t.baseline.y = r.corner.y + ResScale(7);
	t.align = TextAlign::Center;
	t.pStr = GameString(GameStringBase::StarbaseString + 10);
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(ModuleSelectedColor);
	font_DrawText(&t);

	// Indicate mode
	t.baseline.y += ResScale(7);
	if (stowed_q)
	{
		t.pStr = "stored";
	}
	else
	{
		t.pStr = "new";
	}
	font_DrawText(&t);

	// print names and costs
	const uint16_t count = (stowed_q ? stowCount : shipState->count);
	for (uint16_t i {0}; i < MaxVisModules; ++i)
	{
		const uint16_t modIndex = shipState->topIndex + i;

		if (modIndex >= count)
		{
			break;
		}

		DrawShipsStatus(modIndex, i, false);
	}
}

static void DrawShips(ShipyardShipsState* shipState, uint16_t NewItem)
{
	GFXCONTEXT OldContext = SetContext(StatusContext);
	const uint16_t pos = NewItem - shipState->topIndex;

	BatchGraphics();

	DrawShipsDisplay(shipState);
	DrawShipsStatus(NewItem, pos, true);

	UnbatchGraphics();

	SetContext(OldContext);
}

static void ManipulateShips(int16_t NewState)
{
	int16_t NewTop {};

	if (!DosMenu())
	{
		return;
	}

	ShipyardShipsState* shipState {&ShipState};
	NewTop = shipState->topIndex;

	if (NewState > RaceId::NumBuildableShips + MaxStowedShips)
	{
		DrawShips(&ShipState, NewState);
		return;
	}

	if (NewState < NewTop || NewState >= NewTop + MaxVisModules)
	{
		shipState->topIndex = NewState - NewState % MaxVisModules;
	}

	DrawShips(shipState, NewState);
}

static void GetShipStats(ShipStatsEntry* ship_stats, SpeciesId species_id)
{
	HMASTERSHIP hMasterShip = FindMasterShip(species_id);
	const MasterShipInfo* ShipPtr = LockMasterShip(&master_q, hMasterShip);

	fmt::format_to_sz_n(ship_stats->ShipName, "{} {}",
						ShipPtr->ShipInfo.race_strings->text(2),
						ShipPtr->ShipInfo.race_strings->text(4));

	ship_stats->ShipCost = ShipPtr->ShipInfo.ship_cost * 100;

	UnlockMasterShip(&master_q, hMasterShip);
}

static void GetStowedShipStats(ShipStatsEntry* ship_stats, HSHIPFRAG hStowShip)
{
	const ShipFragment* StowShipPtr = LockShipFrag(
		&uqm::gameState().game().stowed_ship_q, hStowShip);

	HMASTERSHIP hMasterShip = FindMasterShip(StowShipPtr->SpeciesID);
	const MasterShipInfo* ShipPtr = LockMasterShip(&master_q, hMasterShip);
	ship_stats->ShipCost = ShipPtr->ShipInfo.ship_cost;
	UnlockMasterShip(&master_q, hMasterShip);

	fmt::format_to_sz_n(ship_stats->ShipName, "{}",
						StowShipPtr->race_strings->text(StowShipPtr->captains_name_index));

	ship_stats->CrewLevel = StowShipPtr->crew_level;
	ship_stats->MaxCrew = StowShipPtr->max_crew;

	UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
}

static void InventoryShips(ShipyardShipsState* ship_state)
{
	uint16_t i {};
	int16_t ShipsOnBoard = 0;

	for (i = 0; i < RaceId::NumBuildableShips; ++i)
	{
		HFLEETINFO hStarShip =
			GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, i);
		const FleetInfo* FleetPtr =
			LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);

		if (FleetPtr->allied_state == RaceAllegiance::Good || FleetPtr->can_build)
		{
			GetShipStats(&ship_state->ShipStats[ShipsOnBoard],
						 FleetPtr->SpeciesID);

			++ShipsOnBoard;
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}

	ship_state->count = ShipsOnBoard;
	stowCount = GetStowedShipCount();

	HSHIPFRAG hStowShip = GetStarShipFromIndex(&uqm::gameState().game().stowed_ship_q, 0);
	while (i < stowCount + RaceId::NumBuildableShips && hStowShip)
	{
		const ShipFragment* StowShipPtr =
			LockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
		HSHIPFRAG hNext = StowShipPtr->succ;

		GetStowedShipStats(&(ship_state->ShipStats[i]), hStowShip);

		UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
		hStowShip = hNext;
		i++;
	}
}

static void FillHangarX(void)
{
	for (uint8_t i {0}; i < HangarShipsRow(); i++)
	{
		if (IsPad())
		{
			hangar_x_coords[i] = hangar_x_coords_3do[i];
		}
		else
		{
			if (!IsDos())
			{
				hangar_x_coords[i] = hangar_x_coords_uqm[i];
			}
			else
			{
				hangar_x_coords[i] = hangar_x_coords_dos[i];
			}
		}

		if (IsHd())
		{
			hangar_x_coords[i] <<= resolutionFactor;
		}
	}
}

static auto WanderingX() { return (optWindowType == uqm::WindowStyle::UQM ? 2 : (optWindowType == uqm::WindowStyle::Console3DO ? 6 : 19)); }

static constexpr auto InitialCrew {2000};

static void showRemainingCrew(void)
{
	GfxRect r;
	GfxText t;
	char buf[30] {};
	int16_t remaining_crew {};

	if (!isDifficulty(uqm::Difficulty::Hard))
	{
		return;
	}

	BatchGraphics();

	remaining_crew = InitialCrew - (int16_t)uqm::makeWord(uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED0), uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED1));

	r.extent = MAKE_EXTENT(ResScale(122), ResScale(7));
	r.corner = MAKE_POINT(ResScale(WanderingX()),
						  ResScale(74) - (r.extent.height + ResScale(2)));

	if (optWindowType != uqm::WindowStyle::UQM)
	{
		r.corner.y = ResScale(1);
	}

	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	// Print remaining crew
	SetContextFont(TinyFont);
	SetContextForeGroundColor(BlueprintColor);
	t.baseline.x = r.corner.x;
	t.baseline.y = r.corner.y + r.extent.height - ResScale(1);
	t.align = TextAlign::Left;
	t.CharCount = (uint16_t)~0;
	t.pStr = buf;
	uqm::utf8StringCopy(
		buf, sizeof(buf), GameString(GameStringBase::StarbaseString + 6));
	// Remaining Crew:

	font_DrawText(&t);

	t.baseline.x += ResScale(71);
	SetContextForeGroundColor(
		remaining_crew > 1000 ? FullCrewColor :
								(remaining_crew < 250 ? LowCrewColor :
														HalfCrewColor));

	if (CheckAlliance(RaceId::Shofixti) == RaceAllegiance::Good)
	{
		const auto fmtResult = fmt::format_to_n(buf, sizeof(buf) - 1, "{}", StrInfinitySign);
		*fmtResult.out = '\0';
	}
	else if (remaining_crew == 0)
	{
		uqm::utf8StringCopy(
			buf, sizeof(buf), GameString(GameStringBase::StarbaseString + 7));
		// Cdr. Hayes
	}
	else
	{
		const auto fmtResult = fmt::format_to_n(buf, sizeof(buf) - 1, "{}", remaining_crew);
		*fmtResult.out = '\0';
	}

	font_DrawText(&t);

	UnbatchGraphics();
}

uint16_t ShipPoints(HFLEETINFO hFleet)
{
	if (!hFleet)
	{
		return 0;
	}
	const FleetInfo* FleetPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);

	const uint16_t shipCost = GetShipCostFromIndex(FindMasterShipIndex(FleetPtr->SpeciesID));

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	return shipCost;
}

uint16_t ShipCost(uint8_t race_id)
{
	return ShipPoints(GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race_id)) * 100;
}

static int16_t CalculateAllyPoints()
{
	HFLEETINFO hFleet {};
	const FleetInfo* FleetPtr {};
	int16_t MaxPoints = difficultyCase(90, 60, 30);

	for (uint8_t i {*RaceId::Arilou}; i <= SpeciesId::LastMelee; i++)
	{
		hFleet = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, i);
		if (!hFleet)
		{
			continue;
		}

		FleetPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
		if (!FleetPtr)
		{
			continue;
		}

		if (FleetPtr->allied_state == RaceAllegiance::Good)
		{
			MaxPoints += ShipPoints(hFleet) * difficultyCase(3, 2, 1);
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	}

	return MaxPoints;
}

/*
 * Returns the total point value of all the ships escorting the SIS.
 */
uint16_t CalculateEscortsPoints(void)
{
	uint16_t total = 0;
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};

	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q);
		 hStarShip; hStarShip = hNextShip)
	{
		const ShipFragment* StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
		hNextShip = _GetSuccLink(StarShipPtr);
		total += ShipPoints(GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, StarShipPtr->race_id));
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}
	return total;
}

static auto RemainingFp() { return (CalculateAllyPoints() - CalculateEscortsPoints()); }

bool CanBuyPoints(HFLEETINFO hFleet)
{
	if ((!uqm::UQMOptions::read().fleetPointSys && !isDifficulty(uqm::Difficulty::Hard))
		|| uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 3)
	{
		return true;
	}

	if (ShipPoints(hFleet) > RemainingFp())
	{
		return false;
	}

	return true;
}

static void showRemainingPoints(int delta)
{
	GfxRect r;
	GfxText t;
	int8_t percentage_left {};
	int16_t FP = RemainingFp() + delta;

	if ((!uqm::UQMOptions::read().fleetPointSys && !isDifficulty(uqm::Difficulty::Hard))
		|| uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 3)
	{
		return;
	}

	BatchGraphics();

	// Draw black rectangle before drawing text
	r.extent = MAKE_EXTENT(ResScale(100), ResScale(7));
	r.corner.x = ResScale(((optWindowType != uqm::WindowStyle::Console3DO) ?
							   DosSisScreenWidth :
							   ThreedoSisScreenWidth)
						  - 100);
	r.corner.y = ResScale(74) - (r.extent.height + ResScale(2));

	if (optWindowType != uqm::WindowStyle::UQM)
	{
		r.corner.y = ResScale(1);
	}

	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	// Draw remaining points
	SetContextFont(TinyFont);
	t.align = TextAlign::Right;
	t.CharCount = (uint16_t)~0;
	char buf[30] {};
	fmt::format_to_n(buf, sizeof(buf) - 1, "{}", FP);
	t.pStr = buf;

	r.corner.x = ResScale(((optWindowType != uqm::WindowStyle::Console3DO) ?
							   DosSisScreenWidth :
							   ThreedoSisScreenWidth)
						  - WanderingX());

	t.baseline.x = r.corner.x;
	t.baseline.y = r.corner.y + r.extent.height - ResScale(1);

	percentage_left = (FP > 0 ?
						   (float)FP / (float)CalculateAllyPoints() * 100 :
						   0);
	SetContextForeGroundColor(
		percentage_left > 50 ? FullCrewColor :
							   (percentage_left < 25 ? LowCrewColor :
													   HalfCrewColor));

	font_DrawText(&t);

	// Draw the "Fleet Points: " text
	r = font_GetTextRect(&t);
	t.baseline.x -= r.extent.width;
	uqm::utf8StringCopy(
		buf, sizeof(buf), GameString(GameStringBase::StarbaseString + 8));
	// Fleet Points:

	SetContextForeGroundColor(BlueprintColor);
	font_DrawText(&t);

	UnbatchGraphics();
}

static void animatePowerLines(MenuState* pMS)
{
	static GfxStamp s;
	static COLORMAP ColorMap {};
	static size_t ColorMapIndex {};
	static TimeCount NextTime {};
	const TimeCount Now = GetTimeCounter();

	if (IsPad() || uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return;
	}

	if (pMS)
	{ // Init animation
		s.origin.x = 0;
		s.origin.y = 0;
		s.frame = SetAbsFrameIndex(pMS->ModuleFrame, 28);
		ColorMap = pMS->HangarColorMap;
		ColorMapIndex = 0;
	}

	if (Now >= NextTime || pMS)
	{
		NextTime = Now + (c_gameSecond / HangarAnimRate());

		SetColorMap(GetColorMapAddress(ColorMap, ColorMapIndex));
		DrawStamp(&s);
		// Advance colomap cycle
		ColorMapIndex = (ColorMapIndex + 1) % ColorMap->size();
	}
}

static void SpinStarShip(MenuState* pMS, HFLEETINFO hStarShip)
{
	const FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
	const int Index {FindMasterShipIndex(FleetPtr->SpeciesID)};
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);

	if (Index >= 0 && Index < NumMeleeShips)
	{
		if (isPC(uqm::UQMOptions::read().whichIntro))
		{
			PlayMenuSound(MenuSoundEffect::Success);
		}
		DoShipSpins = true;
		DoShipSpin(Index, pMS->hMusic);
	}
	DoShipSpins = false;
}

static void on_input_frame(void)
{
	GFXCONTEXT oldContext {SetContext(SpaceContext)};
	if (!DoShipSpins)
	{
		animatePowerLines(nullptr);
	}
	SetContext(oldContext);
}

// Count the ships which can be built by the player.
static uint16_t GetAvailableRaceCount(void)
{
	HFLEETINFO hStarShip {};
	HFLEETINFO hNextShip {};

	uint16_t Index {0};
	for (hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hStarShip; hStarShip = hNextShip)
	{
		const FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
		if (FleetPtr->allied_state == RaceAllegiance::Good
			|| FleetPtr->can_build)
		{
			++Index;
		}

		hNextShip = _GetSuccLink(FleetPtr);
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}

	return Index;
}

static HFLEETINFO GetAvailableRaceFromIndex(uint8_t Index)
{
	HFLEETINFO hStarShip {};
	HFLEETINFO hNextShip {};

	for (hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hStarShip; hStarShip = hNextShip)
	{
		const FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
		if ((FleetPtr->allied_state == RaceAllegiance::Good
			 || FleetPtr->can_build)
			&& Index-- == 0)
		{
			UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
			return hStarShip;
		}

		hNextShip = _GetSuccLink(FleetPtr);
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}

	return 0;
}

static void DrawShipyardShipText(GfxRect* r, int Index)
{
	char race_name[64];
	char ship_name[64];
	Color OldColor;
	int16_t leading {};
	GfxRect block;
	GfxText text;

	if (IsDos())
	{
		return;
	}

	HFLEETINFO hStarShip =
		GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, Index);
	const FleetInfo* FleetPtr =
		LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	HMASTERSHIP hMasterShip = FindMasterShip(FleetPtr->SpeciesID);
	const MasterShipInfo* ShipPtr = LockMasterShip(&master_q, hMasterShip);
	uqm::utf8StringCopy((char*)&race_name, sizeof(race_name),
						ShipPtr->ShipInfo.race_strings->text(ShipPtr->ShipInfo.race_strings->size() - 3));
	uqm::utf8StringCopy((char*)&ship_name, sizeof(ship_name),
						ShipPtr->ShipInfo.race_strings->text(ShipPtr->ShipInfo.race_strings->size() - 2));
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	UnlockMasterShip(&master_q, hMasterShip);

	if (!strlen((char*)&race_name) || !strlen((char*)&ship_name))
	{
		return;
	}

	FONT OldFont {SetContextFont(ModuleFont)};
	OldColor = SetContextForeGroundColor(ShpRectColor);

	GetContextFontLeading(&leading);

	if (!uqm::UQMOptions::read().customBorder)
	{
		block = *r;
		block.extent.height = (leading << 1);
		DrawFilledRectangle(&block);
	}
	else
	{
		DrawBorder(BorderFrameIndex::TextLabel);
	}

	text.align = TextAlign::Center;
	text.baseline.x = r->corner.x + (r->extent.width >> 1);
	text.baseline.y = r->corner.y + leading;
	int16_t strip_align {text.baseline.x};

	// Actually we don't want to align, we just want to strip the alignment
	text.pStr = AlignText((const char*)&race_name, &strip_align);
	text.CharCount = (uint16_t)~0;
	font_DrawShadowedText(&text, ShadowAngle::West, ShpTextColor,
						  ShpShadowColor);

	text.baseline.y += leading;
	text.pStr = AlignText((const char*)&ship_name, &strip_align);
	text.CharCount = (uint16_t)~0;
	font_DrawShadowedText(&text, ShadowAngle::West, ShpTextColor,
						  ShpShadowColor);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static void DrawRaceStrings(uint8_t NewRaceItem)
{
	GfxRect r;
	GfxRect textRect;
	GfxStamp s;


	GFXCONTEXT OldContext {SetContext(StatusContext)};
	GetContextClipRect(&r);
	s.origin.x = RadarX() - r.corner.x;
	s.origin.y = RadarY() - r.corner.y;
	r.corner.x = s.origin.x - ResScale(2);
	r.corner.y = s.origin.y - ResScale(12);
	r.extent.width = RadarWidth() + ResScale(4);
	r.extent.height = ResScale(12);
	BatchGraphics();
	ClearSISRect(ClearSisRadar);
	SetContextForeGroundColor(DkgrayColor);

	if (!IsDos())
	{
		if (!uqm::UQMOptions::read().customBorder)
		{
			DrawFilledRectangle(&r);
		}

		textRect = r;
	}

	DrawBorder(BorderFrameIndex::SisRadar);

	if (!IsDos())
	{
		r.corner = s.origin;
		r.extent.width = RadarWidth();
		r.extent.height = RadarHeight();
		SetContextForeGroundColor(BlackColor);
		DrawFilledRectangle(&r);
	}
	else
	{
		GfxRect dosRect;

		dosRect.corner.x = ResScale(3);
		dosRect.corner.y = RadarY() + ResScale(1);
		dosRect.extent.width = RadarWidth() + ResScale(2);
		dosRect.extent.height = RadarHeight() - ResScale(2);

		if (!IsHd())
		{
			DrawStarConBox(&dosRect, ResScale(1),
						   PcmenuTopLeftBorderColor,
						   PcmenuBottomRightBorderColor, true, BlackColor,
						   false, Transparent);
		}
		else
		{
			DrawRenderedBox(&dosRect, true, BlackColor,
							BevelLevel::ThinInnerBevel, uqm::UQMOptions::read().customBorder);
		}
	}

	if (NewRaceItem != (uint8_t)~0)
	{
		GfxText t;
		HFLEETINFO hStarShip {};
		const FleetInfo* FleetPtr {};
		uint16_t shipCrew {};
		uint16_t maxCrew {};
		GfxRect r;
		const char* captain {};

		ManipulateShips(NewRaceItem);

		if (stowed_q)
		{
			HSHIPFRAG hStowShip = GetStarShipFromIndex(
				&uqm::gameState().game().stowed_ship_q, NewRaceItem);
			const ShipFragment* StowShipPtr = LockShipFrag(
				&uqm::gameState().game().stowed_ship_q, hStowShip);
			NewRaceItem = StowShipPtr->race_id;
			shipCrew = StowShipPtr->crew_level;
			captain = StowShipPtr->race_strings->text(StowShipPtr->captains_name_index);
			UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);

			hStarShip = GetStarShipFromIndex(
				&uqm::gameState().game().avail_race_q, NewRaceItem);
			FleetPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
			maxCrew = FleetPtr->crew_level;
			UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
		}
		else
		{
			hStarShip = GetAvailableRaceFromIndex(NewRaceItem);
			NewRaceItem = GetIndexFromStarShip(&uqm::gameState().game().avail_race_q,
											   hStarShip);
		}
		uint16_t shipPoints {ShipPoints(hStarShip)};
		uint16_t shipCost {ShipCost(NewRaceItem)};

		// Draw the ship name or captain name above the ship image.
		if (!IsDos())
		{
			if (stowed_q)
			{
				t.baseline.x = ResScale(4) + RadarWidth() / 2;
				t.baseline.y = RadarY() - ResScale(2)
							 - DosNumScl(2) - SafeY();
				t.align = TextAlign::Center;
				t.pStr = captain;
				t.CharCount = (uint16_t)~0;
				font_DrawShadowedText(&t, ShadowAngle::West, CaptainNameTextColor,
									  ShpShadowColor);
			}
			else
			{
				DrawShipyardShipText(&textRect, NewRaceItem);
			}
		}

		// Draw the ship image.
		FleetPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
		s.frame = FleetPtr->melee_icon;
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
		s.origin.x += (RadarWidth() >> 1);
		s.origin.y += (RadarHeight() >> 1);
		DrawStamp(&s);


		// Draw additional information
		if (isPC(uqm::UQMOptions::read().whichFonts))
		{
			SetContextFont(TinyFont);
		}
		else
		{
			SetContextFont(TinyFontBold);
		}

		t.baseline.x = ResScale(4) + RadarWidth() - ResScale(2);
		t.baseline.y = RadarY() + RadarHeight() - ResScale(2)
					 - DosNumScl(2) - SafeY();
		t.align = TextAlign::Right;
		t.CharCount = (uint16_t)~0;
		char buf[30] {};
		t.pStr = buf;
		if (!stowed_q)
		{
			// Print the ship cost
			fmt::format_to_n(buf, sizeof(buf) - 1, "{}", shipCost);

			if (shipCost <= (uqm::gameState().sis().ResUnits))
			{
				SetContextForeGroundColor(BrightGreenColor);
			}
			else if (shipCost > (uqm::gameState().sis().ResUnits))
			{
				SetContextForeGroundColor(BrightRedColor);
			}
		}
		else
		{
			// Print the ship's crew
			fmt::format_to_n(buf, sizeof(buf) - 1, "{}/{}", shipCrew, maxCrew);

			if (shipCrew == maxCrew)
			{
				SetContextForeGroundColor(BrightGreenColor);
			}
			else if (shipCrew > maxCrew / 2)
			{
				SetContextForeGroundColor(BrightYellowColor);
			}
			else
			{
				SetContextForeGroundColor(BrightRedColor);
			}
		}

		font_DrawText(&t);

		// Print the fleet points
		if ((uqm::UQMOptions::read().fleetPointSys || isDifficulty(uqm::Difficulty::Hard))
			&& uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 3)
		{
			t.baseline.y = RadarY() + ResScale(7)
						 + DosNumScl(2) - SafeY();
			const auto fmtResult = fmt::format_to_n(buf, sizeof(buf) - 1, "{}", shipPoints);
			*fmtResult.out = '\0';

			if (shipPoints < RemainingFp() / 2)
			{
				SetContextForeGroundColor(BrightGreenColor);
			}
			else if (shipPoints <= RemainingFp())
			{
				SetContextForeGroundColor(BrightYellowColor);
			}
			else
			{
				SetContextForeGroundColor(BrightRedColor);
			}

			font_DrawText(&t);

			r = font_GetTextRect(&t);
			t.baseline.x -= r.extent.width;

			uqm::utf8StringCopy(
				buf, sizeof(buf),
				GameString(GameStringBase::StarbaseString + 9)); // FP:
			SetContextForeGroundColor(BrightBlueColor);
			font_DrawText(&t);
		}
	}
	UnbatchGraphics();
	SetContext(OldContext);
}

// Width of an escort ship window.
static auto ShipWinWidth() { return ResScale(34); }

// Height of an escort ship window.
static auto ShipWinHeight() { return (ShipWinWidth() + ResScale(6)); }

// For how many animation frames' time the escort ship bay doors
// are slid left and right when opening them. If this number is not large
// enough, part of the doors are left visible upon opening.
static auto ShipWinFrames() { return ((ShipWinWidth() >> 1) + 1); }

// Print the crew count of an escort ship on top of its (already drawn)
// image, either as '30' (full), '28/30' (partially full), or 'SCRAP'
// (empty).
// pRect is the rectangle of the ship image.
static void ShowShipCrew(ShipFragment* StarShipPtr, const GfxRect* pRect)
{
	GfxRect r;
	GfxText t;
	HFLEETINFO hTemplate {};
	uint16_t maxCrewLevel {};

	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	hTemplate = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
									 StarShipPtr->race_id);
	const FleetInfo* TemplatePtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hTemplate)};
	maxCrewLevel =
		IsExtended() ? TemplatePtr->max_crew : TemplatePtr->crew_level;
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hTemplate);

	const char* captain {StarShipPtr->race_strings->text(StarShipPtr->captains_name_index)};

	char buf[80] {};
	if (StarShipPtr->crew_level == maxCrewLevel)
	{
		fmt::format_to_n(buf, sizeof(buf) - 1, "{}", StarShipPtr->crew_level);
	}
	else if (StarShipPtr->crew_level > maxCrewLevel)
	{
		fmt::format_to_n(buf, sizeof(buf) - 1, "[{}/{}]", StarShipPtr->crew_level - maxCrewLevel, maxCrewLevel);
	}
	else if (StarShipPtr->crew_level == 0)
	{
		uqm::utf8StringCopy(buf, sizeof(buf),
							GameString(GameStringBase::StarbaseString + 5));
	}
	else
	{
		fmt::format_to_n(buf, sizeof(buf) - 1, "{}/{}", StarShipPtr->crew_level, maxCrewLevel);
	}

	r = *pRect;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + r.extent.height - ResScale(1);
	t.align = TextAlign::Center;
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	if (r.corner.y)
	{
		// Black out the crew before printing it if this is a redraw
		r.corner.y = t.baseline.y - ResScale(6);
		r.extent.width = ShipWinWidth();
		r.extent.height = ResScale(6);
		SetContextForeGroundColor(BlackColor);
		DrawFilledRectangle(&r);
	}
	SetContextForeGroundColor((StarShipPtr->crew_level != 0) ?
								  (BuildColor(MakeRgb15(0x00, 0x14, 0x00), 0x02)) :
								  (BuildColor(MakeRgb15(0x12, 0x00, 0x00), 0x2B)));
	font_DrawText(&t);

	if (!uqm::UQMOptions::read().captainNames)
	{
		return;
	}

	r = *pRect;
	t.baseline.y = r.corner.y + ResScale(7);
	t.pStr = captain;
	t.CharCount = (uint16_t)~0;
	if (!r.corner.y)
	{
		// Dim the top of the ship the first time you draw it
		r.corner.y = t.baseline.y - ResScale(6);
		r.extent.width = ShipWinWidth();
		r.extent.height = ResScale(6);
		const Color shady = BuildColorRgba(0x00, 0x00, 0x00, 0x80);
		SetContextForeGroundColor(shady);
		DrawFilledRectangle(&r);

		SetContextFont(TinyFont);
		SetContextForeGroundColor(CaptainNameTextColor);
		font_DrawText(&t);
	}
}

static void ShowCombatShip(MenuState* pMS, uint16_t which_window,
						   ShipFragment* YankedStarShipPtr)
{
	uint16_t i {};
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};
	ShipFragment* StarShipPtr {};
	struct
	{
		ShipFragment* StarShipPtr;
		GfxPoint finished_s;
		GfxStamp ship_s;
		GfxStamp lfdoor_s;
		GfxStamp rtdoor_s;
	} ship_win_info[MaxBuiltShips], *pship_win_info {};

	uint16_t num_ships {1};
	pship_win_info = &ship_win_info[0];
	if (YankedStarShipPtr)
	{
		pship_win_info->StarShipPtr = YankedStarShipPtr;

		pship_win_info->lfdoor_s.origin.x = -(ShipWinWidth() >> 1);
		pship_win_info->rtdoor_s.origin.x = (ShipWinWidth() >> 1);
		pship_win_info->lfdoor_s.origin.y = 0;
		pship_win_info->rtdoor_s.origin.y = 0;
		pship_win_info->lfdoor_s.frame = IncFrameIndex(pMS->ModuleFrame);
		pship_win_info->rtdoor_s.frame =
			IncFrameIndex(pship_win_info->lfdoor_s.frame);

		pship_win_info->ship_s.origin.x = (ShipWinWidth() >> 1) + 1;
		pship_win_info->ship_s.origin.y = (ShipWinWidth() >> 1);
		pship_win_info->ship_s.frame = YankedStarShipPtr->melee_icon;

		pship_win_info->finished_s.x = hangar_x_coords[which_window % HangarShipsRow()];
		pship_win_info->finished_s.y = HangarY() + (HangarDy() * (which_window / HangarShipsRow()));
	}
	else
	{
		if (which_window == (uint16_t)~0)
		{
			hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q);
			num_ships = CountLinks(&uqm::gameState().game().built_ship_q);
		}
		else
		{
			HSHIPFRAG hTailShip {GetTailLink(&uqm::gameState().game().built_ship_q)};
			RemoveQueue(&uqm::gameState().game().built_ship_q, hTailShip);

			hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q);
			while (hStarShip)
			{
				StarShipPtr =
					LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
				if (StarShipPtr->index > which_window)
				{
					UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
					break;
				}
				hNextShip = _GetSuccLink(StarShipPtr);
				UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

				hStarShip = hNextShip;
			}
			InsertQueue(&uqm::gameState().game().built_ship_q, hTailShip, hStarShip);

			hStarShip = hTailShip;
			StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
			StarShipPtr->index = which_window;
			UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
		}

		for (i = 0; i < num_ships; ++i)
		{
			StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
			hNextShip = _GetSuccLink(StarShipPtr);

			pship_win_info->StarShipPtr = StarShipPtr;
			// XXX BUG: this looks wrong according to the original
			// semantics of LockShipFrag(): StarShipPtr is not
			// valid anymore after UnlockShipFrag() is called,
			// but it is used thereafter.

			pship_win_info->lfdoor_s.origin.x = -ResScale(1);
			pship_win_info->rtdoor_s.origin.x = ResScale(1);
			pship_win_info->lfdoor_s.origin.y = 0;
			pship_win_info->rtdoor_s.origin.y = 0;
			pship_win_info->lfdoor_s.frame =
				IncFrameIndex(pMS->ModuleFrame);
			pship_win_info->rtdoor_s.frame =
				IncFrameIndex(pship_win_info->lfdoor_s.frame);

			pship_win_info->ship_s.origin.x = (ShipWinWidth() >> 1) + 1;
			pship_win_info->ship_s.origin.y = (ShipWinWidth() >> 1);
			pship_win_info->ship_s.frame = StarShipPtr->melee_icon;

			which_window = StarShipPtr->index;
			pship_win_info->finished_s.x = hangar_x_coords[which_window % HangarShipsRow()];
			pship_win_info->finished_s.y = HangarY() + (HangarDy() * (which_window / HangarShipsRow()));
			++pship_win_info;

			UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
			hStarShip = hNextShip;
		}
	}

	if (num_ships)
	{
		TimeCount TimeIn;
		GfxRect r;
		GFXCONTEXT OldContext {};
		GfxRect OldClipRect;

		bool AllDoorsFinished {false};
		r.corner.x = r.corner.y = 0;
		r.extent.width = ShipWinWidth();
		r.extent.height = ShipWinHeight();
		FlushInput();
		TimeIn = GetTimeCounter();

		for (int j {0}; (j < ShipWinFrames()) && !AllDoorsFinished; j++)
		{
			SleepThreadUntil(TimeIn + c_gameSecond / ResScale(24));
			TimeIn = GetTimeCounter();
			if (AnyButtonPress(false))
			{
				if (YankedStarShipPtr != 0)
				{ // Fully close the doors
					ship_win_info[0].lfdoor_s.origin.x = 0;
					ship_win_info[0].rtdoor_s.origin.x = 0;
				}
				AllDoorsFinished = true;
			}

			OldContext = SetContext(SpaceContext);
			GetContextClipRect(&OldClipRect);
			SetContextBackGroundColor(BlackColor);

			BatchGraphics();
			pship_win_info = &ship_win_info[0];
			for (i = 0; i < num_ships; ++i)
			{
				{
					GfxRect ClipRect;

					ClipRect.corner.x = SisOrgX() + pship_win_info->finished_s.x;
					ClipRect.corner.y = SisOrgY() + pship_win_info->finished_s.y;
					ClipRect.extent.width = ShipWinWidth();
					ClipRect.extent.height = ShipWinHeight();
					SetContextClipRect(&ClipRect);

					ClearDrawable();
					DrawStamp(&pship_win_info->ship_s);
					ShowShipCrew(pship_win_info->StarShipPtr, &r);
					if (!AllDoorsFinished || YankedStarShipPtr)
					{
						DrawStamp(&pship_win_info->lfdoor_s);
						DrawStamp(&pship_win_info->rtdoor_s);
						if (YankedStarShipPtr)
						{ // Close the doors
							++pship_win_info->lfdoor_s.origin.x;
							--pship_win_info->rtdoor_s.origin.x;
						}
						else
						{ // Open the doors
							--pship_win_info->lfdoor_s.origin.x;
							++pship_win_info->rtdoor_s.origin.x;
						}
					}
				}
				++pship_win_info;
			}

			SetContextClipRect(&OldClipRect);
#ifndef USE_3DO_HANGAR
			animatePowerLines(nullptr);
#endif
			UnbatchGraphics();
			SetContext(OldContext);
		}
	}
}

static void CrewTransaction(int16_t crew_delta)
{
	if (crew_delta)
	{
		int16_t crew_bought {};

		crew_bought = (int16_t)uqm::makeWord(
						  uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED0),
						  uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED1))
					+ crew_delta;
		if (crew_bought < 0)
		{
			if (crew_delta < 0)
			{
				crew_bought = 0;
			}
			else
			{
				crew_bought = 0x7FFF;
			}
		}
		else if (crew_delta > 0)
		{
			if (crew_bought >= CrewExpenseThreshold
				&& crew_bought - crew_delta < CrewExpenseThreshold)
			{
				uqm::gameState().game().CrewCost += 2;
				DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
			}
		}
		else
		{
			if (crew_bought < CrewExpenseThreshold
				&& crew_bought - crew_delta >= CrewExpenseThreshold)
			{
				uqm::gameState().game().CrewCost -= 2;
				DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
			}
		}
		if (CheckAlliance(RaceId::Shofixti) != RaceAllegiance::Good)
		{
			uqm::gameState().setFlag(uqm::GameFlag::CREW_PURCHASED0, static_cast<uint32_t>(uqm::lowByte(crew_bought)));
			uqm::gameState().setFlag(uqm::GameFlag::CREW_PURCHASED1, static_cast<uint32_t>(uqm::highByte(crew_bought)));

			showRemainingCrew();
		}
	}
}

static void DMS_FlashFlagShip(void)
{
	GfxRect r;
	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = SisScreenWidth();
	if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
	{
		r.extent.height = ResScale(63) - SafeNumScl(2);
	}
	else
	{
		r.extent.height = ResScale(74) - DosNumScl(9) - SafeNeg(3);
	}
	SetFlashRect(&r, uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
}

static void DMS_GetEscortShipRect(GfxRect* rOut, uint8_t slotNr)
{
	const uint8_t row = slotNr / HangarShipsRow();
	const uint8_t col = slotNr % HangarShipsRow();

	rOut->corner.x = hangar_x_coords[col];
	rOut->corner.y = HangarY() + (HangarDy() * row);
	rOut->extent.width = ShipWinWidth();
	rOut->extent.height = ShipWinHeight();
}

static void DMS_FlashEscortShip(uint8_t slotNr)
{
	GfxRect r;
	DMS_GetEscortShipRect(&r, slotNr);
	SetFlashRect(&r, uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
}

static void DMS_FlashFlagShipCrewCount(void)
{
	GfxRect r;
	SetContext(StatusContext);
	GetGaugeRect(&r, true);
	SetFlashRect(&r, false);
	SetContext(SpaceContext);
}

static void DMS_FlashEscortShipCrewCount(uint8_t slotNr)
{
	GfxRect r;
	const uint8_t row = slotNr / HangarShipsRow();
	const uint8_t col = slotNr % HangarShipsRow();

	r.corner.x = hangar_x_coords[col];
	r.corner.y = (HangarY() + (HangarDy() * row))
			   + (ShipWinHeight() - ResScale(6));
	r.extent.width = ShipWinWidth();
	r.extent.height = ResScale(5);

	SetContext(SpaceContext);
	SetFlashRect(&r, false);
}

// Helper function for DoModifyShips(). Called to change the flash
// rectangle to the currently selected ship (flagship or escort ship).
static void DMS_FlashActiveShip(MenuState* pMS)
{
	if (uqm::highNibble(pMS->CurState))
	{
		// Flash the flag ship.
		DMS_FlashFlagShip();
	}
	else
	{
		// Flash the current escort ship slot.
		DMS_FlashEscortShip(pMS->CurState);
	}
}

// Helper function for DoModifyShips(). Called to switch between
// the various edit modes.
// XXX: right now, this only switches the sound and flash rectangle.
// Perhaps we should move more of the code to modify other aspects
// here too.
static void DMS_SetMode(MenuState* pMS, DmsMode mode)
{
	switch (mode)
	{
		case DmsMode::Navigate:
			SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
			DMS_FlashActiveShip(pMS);
			break;
		case DmsMode::AddEscort:
			SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
			SetFlashRect(DosBool(SfrMenu3do(), SfrMenuNon()), false);
			break;
		case DmsMode::EditCrew:
			SetMenuSounds(MenuSoundArrows,
						  MenuSoundPage | MenuSoundAction);
			if (uqm::highNibble(pMS->CurState))
			{
				// Enter crew editing mode for the flagship.
				DMS_FlashFlagShipCrewCount();
			}
			else
			{
				// Enter crew editing mode for an escort ship.
				DMS_FlashEscortShipCrewCount(pMS->CurState);
			}
			break;
		case DmsMode::Exit:
			SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
			SetFlashRect(SfrMenu3do(), false);
			break;
	}
}

static constexpr auto ModifyCrewFlag {1 << 8};
// Helper function for DoModifyShips(), called when the player presses the
// special button.
// It works both when the cursor is over an escort ship, while not editing
// the crew, and when a new ship is added.
// hStarShip is the ship in the slot under the cursor
// (or 0 if no such ship).
static bool DMS_SpinShip(MenuState* pMS, HSHIPFRAG hStarShip)
{
	HFLEETINFO hSpinShip = 0;
	GfxRect OldClipRect;

	// No spinning the flagship.
	if (uqm::highNibble(pMS->CurState) != 0)
	{
		return false;
	}

	// We must either be hovering over a used ship slot, or adding a new
	// ship to the fleet.
	if ((hStarShip == 0) == !(pMS->delta_item & ModifyCrewFlag))
	{
		return false;
	}

	if (!hStarShip && !stowed_q)
	{
		// Selecting a ship to build.
		hSpinShip = GetAvailableRaceFromIndex(uqm::lowByte(pMS->delta_item));
		if (!hSpinShip)
		{
			return false;
		}
	}
	else if (!hStarShip && stowed_q)
	{
		// Selecting a stowed ship
		HSHIPFRAG hStowedShip = GetStarShipFromIndex(
			&uqm::gameState().game().stowed_ship_q, uqm::lowByte(pMS->delta_item));
		const ShipFragment* FragPtr = LockShipFrag(
			&uqm::gameState().game().stowed_ship_q, hStowedShip);
		hSpinShip = GetStarShipFromIndex(
			&uqm::gameState().game().avail_race_q, FragPtr->race_id);
		UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowedShip);
	}
	else
	{
		// Hovering over an escort ship.
		const ShipFragment* FragPtr = LockShipFrag(
			&uqm::gameState().game().built_ship_q, hStarShip);
		hSpinShip = GetStarShipFromIndex(
			&uqm::gameState().game().avail_race_q, FragPtr->race_id);
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}

	SetFlashRect(nullptr, false);

	GFXCONTEXT OldContext {SetContext(ScreenContext)};
	GetContextClipRect(&OldClipRect);

	SpinStarShip(pMS, hSpinShip);

	SetContextClipRect(&OldClipRect);
	SetContext(OldContext);

	return true;
}

// Helper function for DoModifyShips(), called when the player presses the
// up button when modifying the crew of the flagship.
// Buy crew for the flagship.
// Returns the change in crew (1 on success, 0 on failure).
static int16_t DMS_HireFlagShipCrew(void)
{
	GfxRect r;
	int16_t crew_bought {};

	crew_bought = (int16_t)uqm::makeWord(
		uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED0),
		uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED1));

	if (GetCPodCapacity(&r.corner) <= GetCrewCount())
	{
		// At capacity.
		return 0;
	}

	if (uqm::gameState().sis().ResUnits < (uint32_t)uqm::gameState().game().CrewCost)
	{
		// Not enough RUs.
		return 0;
	}

	if (isDifficulty(uqm::Difficulty::Hard) && crew_bought >= 2000
		&& CheckAlliance(RaceId::Shofixti) != RaceAllegiance::Good)
	{
		// Ran out of StarBase crew
		return 0;
	}

	// Draw a crew member.
	// Crew dots/rectangles for Original and HdResolutionFactor graphics.
	r.extent.width = ResScale(1);
	r.extent.height = r.extent.width;
	DrawFilledRectangle(&r);

	// Update the crew counter and RU. Note that the crew counter is
	// flashing.
	PreUpdateFlashRect();
	DeltaSISGauges(1, 0, -uqm::gameState().game().CrewCost);
	PostUpdateFlashRect();

	return 1;
}

// Helper function for DoModifyShips(), called when the player presses the
// down button when modifying the crew of the flagship.
// Dismiss crew from the flagship.
// Returns the change in crew (-1 on success, 0 on failure).
static int16_t DMS_DismissFlagShipCrew(void)
{
	int16_t crew_bought {};
	GfxRect r;

	if (GetCrewCount() == 0)
	{
		// No crew to dismiss.
		return 0;
	}

	crew_bought = (int16_t)uqm::makeWord(
		uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED0),
		uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED1));

	// Update the crew counter and RU. Note that the crew counter is
	// flashing.
	PreUpdateFlashRect();
	DeltaSISGauges(-1, 0, uqm::gameState().game().CrewCost - (crew_bought == CrewExpenseThreshold ? 2 : 0));
	PostUpdateFlashRect();

	// Remove the pixel representing the crew member.
	GetCPodCapacity(&r.corner);
	r.extent.width = ResScale(1);
	r.extent.height = r.extent.width;
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	return -1;
}

// Helper function for DoModifyShips(), called when the player presses the
// up button when modifying the crew of an escort ship.
// Buy crew for an escort ship
// Returns the change in crew (1 on success, 0 on failure).
static int16_t DMS_HireEscortShipCrew(ShipFragment* StarShipPtr)
{
	uint16_t templateMaxCrew {};
	GfxRect r;
	int16_t crew_bought {};
	const uint16_t crew_level = (StarShipPtr->crew_level > StarShipPtr->max_crew ? StarShipPtr->crew_level - StarShipPtr->max_crew : StarShipPtr->crew_level);

	crew_bought = (int16_t)uqm::makeWord(
		uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED0),
		uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED1));

	{
		// XXX Split this off into a separate function?
		HFLEETINFO hTemplate = GetStarShipFromIndex(
			&uqm::gameState().game().avail_race_q,
			StarShipPtr->race_id);
		const FleetInfo* TemplatePtr =
			LockFleetInfo(&uqm::gameState().game().avail_race_q, hTemplate);
		templateMaxCrew = (IsExtended() && !isDifficulty(uqm::Difficulty::Hard)) ?
							  TemplatePtr->max_crew :
							  TemplatePtr->crew_level;
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hTemplate);
	}

	if (uqm::gameState().sis().ResUnits < (uint32_t)uqm::gameState().game().CrewCost)
	{
		// Not enough money to hire a crew member.
		return 0;
	}

	if (crew_level >= StarShipPtr->max_crew)
	{
		// This ship cannot handle more crew.
		return 0;
	}

	if (crew_level >= templateMaxCrew)
	{
		// A ship of this type cannot handle more crew.
		return 0;
	}

	if (isDifficulty(uqm::Difficulty::Hard) && crew_bought >= 2000
		&& CheckAlliance(RaceId::Shofixti) != RaceAllegiance::Good)
	{
		// Ran out of StarBase crew
		return 0;
	}

	if (crew_level > 0)
	{
		DeltaSISGauges(0, 0, -uqm::gameState().game().CrewCost);
	}
	else
	{
		// Buy a ship.
		DeltaSISGauges(0, 0, -(int)ShipCost(StarShipPtr->race_id));
		showRemainingPoints(0);
	}

	++StarShipPtr->crew_level;

	PreUpdateFlashRect();
	DMS_GetEscortShipRect(&r, StarShipPtr->index);
	ShowShipCrew(StarShipPtr, &r);
	PostUpdateFlashRect();

	return 1;
}

// Helper function for DoModifyShips(), called when the player presses the
// down button when modifying the crew of an escort ship.
// Dismiss crew from an escort ship
// Returns the change in crew (-1 on success, 0 on failure).
static int16_t DMS_DismissEscortShipCrew(ShipFragment* StarShipPtr)
{
	int16_t crew_delta = 0;
	GfxRect r;
	const uint16_t crew_level = (StarShipPtr->crew_level > StarShipPtr->max_crew ? StarShipPtr->crew_level - StarShipPtr->max_crew : StarShipPtr->crew_level);

	if (crew_level > 0)
	{
		if (crew_level > 1)
		{
			// The ship was not at 'scrap'.
			// Give one crew member worth of RU.
			const int16_t crew_bought = (int16_t)uqm::makeWord(
				uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED0),
				uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED1));

			DeltaSISGauges(0, 0, uqm::gameState().game().CrewCost - (crew_bought == CrewExpenseThreshold ? 2 : 0));
		}
		else
		{
			// With the last crew member, the ship will be scrapped.
			// Give RU for the ship.
			HFLEETINFO hTemplate = GetStarShipFromIndex(
				&uqm::gameState().game().avail_race_q, StarShipPtr->race_id);
			DeltaSISGauges(0, 0, (int)ShipCost(StarShipPtr->race_id));
			showRemainingPoints(ShipPoints(hTemplate));
		}
		crew_delta = -1;
		--StarShipPtr->crew_level;
		if (StarShipPtr->crew_level == StarShipPtr->max_crew)
		{
			StarShipPtr->crew_level = 0;
		}
	}
	else
	{ // no crew to dismiss
		PlayMenuSound(MenuSoundEffect::Failure);
	}

	PreUpdateFlashRect();
	DMS_GetEscortShipRect(&r, StarShipPtr->index);
	ShowShipCrew(StarShipPtr, &r);
	PostUpdateFlashRect();

	return crew_delta;
}

// Helper function for DoModifyShips(), called when the player presses the
// up or down button when modifying the crew of the flagship or of an
// escort ship.
// 'hStarShip' is the currently escort ship, or 0 if no ship is selected.
// 'dy' is -1 if the 'up' button was pressed, or '1' if the down button was
// pressed.
static void DMS_ModifyCrew(MenuState* pMS, HSHIPFRAG hStarShip, int8_t dy)
{
	int16_t crew_delta = 0;
	ShipFragment* StarShipPtr = nullptr;
	uint16_t loop {};
	uint16_t DoLoop = 1;
	GfxRect r;
	//int16_t remaining_crew = InitialCrew - (int16_t)MAKE_WORD (
	//		GET_GAME_STATE (CREW_PURCHASED0),
	//		GET_GAME_STATE (CREW_PURCHASED1)); Unused

	DoLoop = abs(dy);

	if (abs(dy) == 50)
	{
		PlayMenuSound(MenuSoundEffect::Invoked);
	}

	if (hStarShip)
	{
		StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}

	if (hStarShip == 0)
	{
		if (dy == -50)
		{
			DoLoop = GetCPodCapacity(&r.corner) - GetCrewCount();
		}
		else if (dy == 50)
		{
			DoLoop = GetCrewCount();
		}

		// Add/Dismiss crew for the flagship.
		for (loop = 0; loop < DoLoop; loop++)
		{
			if (dy < 0)
			{
				// Add crew for the flagship.
				crew_delta = DMS_HireFlagShipCrew();
			}
			else
			{
				// Dismiss crew from the flagship.
				crew_delta = DMS_DismissFlagShipCrew();
			}

			if (crew_delta != 0)
			{
				DMS_FlashFlagShipCrewCount();
			}

			if (crew_delta == 0)
			{
				break;
			}

			CrewTransaction(crew_delta);
			animatePowerLines(nullptr);
		}
	}
	else
	{
		const uint16_t crew_level = (StarShipPtr->crew_level > StarShipPtr->max_crew ? StarShipPtr->crew_level - StarShipPtr->max_crew : StarShipPtr->crew_level);
		if (dy == -50)
		{
			DoLoop = StarShipPtr->max_crew - crew_level;
		}
		else if (dy == 50)
		{
			DoLoop = crew_level;
		}

		// Add/Dismiss crew for an escort ship.
		for (loop = 0; loop < DoLoop; loop++)
		{
			if (dy < 0)
			{
				// Add crew for an escort ship.
				crew_delta = DMS_HireEscortShipCrew(StarShipPtr);
			}
			else
			{
				// Dismiss crew from an escort ship.
				crew_delta = DMS_DismissEscortShipCrew(StarShipPtr);
			}

			if (crew_delta != 0)
			{
				DMS_FlashEscortShipCrewCount(StarShipPtr->index);
			}

			if (crew_delta == 0)
			{
				break;
			}

			if (!isDifficulty(uqm::Difficulty::Hard) || ((dy > 0 && StarShipPtr->crew_level >= 1) || (dy < 0 && StarShipPtr->crew_level != 1)))
			{
				CrewTransaction(crew_delta);
				animatePowerLines(nullptr);
			}
		}
	}

	if (crew_delta == 0)
	{
		PlayMenuSound(MenuSoundEffect::Failure);
	}

	if (hStarShip)
	{
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

		// Clear out the bought ship index so that flash rects work
		// correctly.
		pMS->delta_item &= ModifyCrewFlag;
	}
}

// Helper function for DoModifyShips(), called when the player presses the
// select button when the cursor is over an empty escort ship slot.
// Try to add the currently selected ship as an escort ship.
static void DMS_TryAddEscortShip(MenuState* pMS)
{
	HFLEETINFO hFleet = GetAvailableRaceFromIndex(
		uqm::lowByte(pMS->delta_item));

	const uint16_t Index = GetIndexFromStarShip(&uqm::gameState().game().avail_race_q, hFleet);
	const uint16_t shipCost = ShipCost(Index);

	if (CanBuyPoints(hFleet) && uqm::gameState().sis().ResUnits >= (uint32_t)shipCost
		&& CloneShipFragment((RaceId)Index, &uqm::gameState().game().built_ship_q, 1))
	{
		ShowCombatShip(pMS, pMS->CurState, nullptr);
		// Reset flash rectangle
		DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
		DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
		// twice to reset menu selection

		DeltaSISGauges(UndefinedDelta, UndefinedDelta, -(int)shipCost);
		DMS_SetMode(pMS, DmsMode::EditCrew);

		showRemainingPoints(0);
	}
	else
	{
		// not enough RUs to build, cloning the ship failed,
		// or fleet point limit reached
		PlayMenuSound(MenuSoundEffect::Failure);
	}
}

// Helper function for DoModifyShips(), called when the player presses the
// select button when the cursor is over an empty escort ship slot.
// Try to moving the currently selected ship from storage as an escort ship.
static void DMS_TryUnstowEscortShip(MenuState* pMS)
{
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hStowShip = GetStarShipFromIndex(&uqm::gameState().game().stowed_ship_q,
											   uqm::lowByte(pMS->delta_item));
	const ShipFragment* StowShipPtr =
		LockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
	const uint16_t Index = StowShipPtr->race_id;
	HFLEETINFO hFleet = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, Index);

	if (CanBuyPoints(hFleet) && (hStarShip = CloneShipFragment((RaceId)Index, &uqm::gameState().game().built_ship_q, StowShipPtr->crew_level)))
	{
		ShipFragment* StarShipPtr =
			LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
		StarShipPtr->captains_name_index = StowShipPtr->captains_name_index;
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

		UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
		RemoveQueue(&uqm::gameState().game().stowed_ship_q, hStowShip);
		FreeShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);

		ShowCombatShip(pMS, pMS->CurState, nullptr);
		// Reset flash rectangle
		DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
		DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
		// twice to reset menu selection

		DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);

		if (DosMenu())
		{
			// Regenerate the ship list
			memset(&ShipState, 0, sizeof ShipState);
			InventoryShips(&ShipState);
		}

		DMS_SetMode(pMS, DmsMode::EditCrew);

		showRemainingPoints(0);
		stowCount = GetStowedShipCount();
		currentStowShip = uqm::lowByte(pMS->delta_item);
		stowed_q &= (stowCount > 0);
	}
	else
	{
		// Cloning the ship failed or fleet point limit reached
		PlayMenuSound(MenuSoundEffect::Failure);
		UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
	}
}

// Helper function for DoModifyShips(), called when the player is in the
// mode to add a new escort ship to the fleet (after pressing select on an
// empty slot).
// lowByte (pMS->delta_item) is used to store the currently highlighted
// ship. Returns false if the flash rectangle needs to be updated.
static void DMS_AddEscortShip(MenuState* pMS, bool special, bool select,
							  bool cancel, int8_t dx, int8_t dy)
{
	assert(pMS->delta_item & ModifyCrewFlag);

	uint8_t currentShip = uqm::lowByte(pMS->delta_item);

	if (special)
	{ // JSD: I don't think we can use delta_item, but SpinShip should be ok
		//if (DMS_SpinShip (pMS, GetEscortByStarShipIndex (pMS->delta_item)))
		if (DMS_SpinShip(pMS, nullptr))
		{
			DMS_SetMode(pMS, DmsMode::AddEscort);
		}
		return;
	}

	if (cancel)
	{
		// Cancel selecting an escort ship.
		pMS->delta_item &= ~ModifyCrewFlag;
		SetFlashRect(nullptr, false);

		DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
		DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Crew);
		// twice to reset menu selection
		DMS_SetMode(pMS, DmsMode::Navigate);

		if (DosMenu())
		{
			DeltaSISGauges(UndefinedDelta, UndefinedDelta,
						   UndefinedDelta);
		}
		currentStowShip = currentShip;
	}
	else if (select)
	{
		// Selected a ship to be inserted in an empty escort
		// ship slot.
		if (!stowed_q)
		{
			DMS_TryAddEscortShip(pMS);
		}
		else
		{
			DMS_TryUnstowEscortShip(pMS);
		}
	}
	else if (dy && StorageQ())
	{
		// "dy" switches between stowed ships and buying.
		if (stowCount == 0)
		{
			PlayMenuSound(MenuSoundEffect::Failure);
		}
		else
		{
			stowed_q = !stowed_q;
			currentShip = 0;
			PreUpdateFlashRect();
			DrawRaceStrings(currentShip);
			PostUpdateFlashRect();
			pMS->delta_item = currentShip | ModifyCrewFlag;
		}
	}
	else if (dx)
	{
		// Motion key (up/down/left/right) pressed while selecting a ship
		// to be inserted in an empty escort ship slot.
		if (dx < 0)
		{
			if (currentShip < -dx)
			{
				currentShip = (stowed_q ? stowCount : availableCount) - 1;
				if (DosMenu() && dx != -1)
				{
					currentShip = currentShip - currentShip % MaxVisModules;
				}
			}
			else
			{
				currentShip += dx;
			}
		}
		else if (dx > 0)
		{
			if (currentShip + dx >= (stowed_q ? stowCount : availableCount))
			{
				currentShip = 0;
			}
			else
			{
				currentShip += dx;
			}
		}

		if (currentShip != uqm::lowByte(pMS->delta_item))
		{
			PreUpdateFlashRect();
			DrawRaceStrings(currentShip);
			PostUpdateFlashRect();
			pMS->delta_item = currentShip | ModifyCrewFlag;
		}
	}
}

// Helper function for DoModifyShips(), called when the player presses
// 'select' or 'cancel' after selling all the crew.
static void DMS_ScrapEscortShip(MenuState* pMS, HSHIPFRAG hStarShip)
{
	ShipFragment* StarShipPtr =
		LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

	SetFlashRect(nullptr, false);
	ShowCombatShip(pMS, pMS->CurState, StarShipPtr);

	UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

	RemoveQueue(&uqm::gameState().game().built_ship_q, hStarShip);
	FreeShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	// refresh SIS display
	DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);

	showRemainingPoints(0);

	SetContext(SpaceContext);
	DMS_SetMode(pMS, DmsMode::Navigate);
}

// Helper function for DoModifyShips(), called when the player presses
// 'select' or 'cancel' after selling all the crew.
static void DMS_StowEscortShip(MenuState* pMS, HSHIPFRAG hStarShip)
{
	HSHIPFRAG hStowShip {};
	ShipFragment* StarShipPtr =
		LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

	SetFlashRect(nullptr, false);
	ShowCombatShip(pMS, pMS->CurState, StarShipPtr);

	const uint16_t Index = StarShipPtr->race_id;
	if ((hStowShip = CloneShipFragment((RaceId)Index, &uqm::gameState().game().stowed_ship_q,
									   StarShipPtr->crew_level)))
	{
		ShipFragment* StowShipPtr =
			LockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
		StowShipPtr->captains_name_index = StarShipPtr->captains_name_index;
		UnlockShipFrag(&uqm::gameState().game().stowed_ship_q, hStowShip);
	}
	UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

	RemoveQueue(&uqm::gameState().game().built_ship_q, hStarShip);
	FreeShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

	// refresh SIS display
	DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);

	showRemainingPoints(0);
	stowCount = GetStowedShipCount();

	SetContext(SpaceContext);
	DMS_SetMode(pMS, DmsMode::Navigate);
}

// Helper function for DoModifyShips(), called when the player presses
// one of the motion keys when not in crew modification mode.
static uint8_t DMS_MoveCursor(uint8_t curState, int8_t dx, int8_t dy)
{
	uint8_t row = uqm::lowNibble(curState) / HangarShipsRow();
	uint8_t col = uqm::lowNibble(curState) % HangarShipsRow();
	bool isFlagShipSelected = (uqm::highNibble(curState) != 0);

	if (dy)
	{
		// Vertical motion.

		// We consider the flagship an extra row (on the bottom),
		// to ease operations.
		if (isFlagShipSelected)
		{
			row = HangarRows();
		}

		// Move up/down, wrapping around:
		row = (row + (HangarRows() + 1) + dy) % (HangarRows() + 1);

		// If we moved to the 'extra row', this means the flag ship.
		isFlagShipSelected = (row == HangarRows());
		if (isFlagShipSelected)
		{
			row = 0;
		}
	}
	else if (dx)
	{
		// Horizontal motion.
		if (!isFlagShipSelected)
		{
			// Moving horizontally through the escort ship slots,
			// wrapping around if necessary.
			col = (col + HangarShipsRow() + dx) % HangarShipsRow();
		}
	}

	return uqm::makeByte(row * HangarShipsRow() + col,
						 isFlagShipSelected ? 0xf : 0);
}

// Helper function for DoModifyShips(), called every time DoModifyShip() is
// called when we are in crew editing mode.
static void DMS_EditCrewMode(MenuState* pMS, HSHIPFRAG hStarShip,
							 bool select, bool cancel, int8_t dy)
{
	if (cancel)
	{
		// Leave crew editing mode.
		if (hStarShip != 0)
		{
			// Exiting crew editing mode for an escort ship.
			ShipFragment* StarShipPtr = LockShipFrag(
				&uqm::gameState().game().built_ship_q, hStarShip);
			const uint16_t crew_level = StarShipPtr->crew_level;
			const uint16_t max_crew = StarShipPtr->max_crew;
			if (stowCount == MaxStowedShips && crew_level > max_crew)
			{
				PlayMenuSound(MenuSoundEffect::Failure);
				UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
				return;
			}
			if (crew_level > max_crew)
			{
				// This happens when it's been forced by stowing
				// set crew back to proper level before it happens
				StarShipPtr->crew_level -= StarShipPtr->max_crew;
			}

			UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

			if (crew_level == 0)
			{
				// Scrapping the escort ship before exiting crew edit
				// mode.
				DMS_ScrapEscortShip(pMS, hStarShip);
			}
			else if (crew_level > max_crew)
			{
				// Stowing the ship along with its crew
				DMS_StowEscortShip(pMS, hStarShip);
				if (DosMenu())
				{
					// Regenerate the ship list
					memset(&ShipState, 0, sizeof ShipState);
					InventoryShips(&ShipState);
				}
			}
		}

		pMS->delta_item &= ~ModifyCrewFlag;
		DMS_SetMode(pMS, DmsMode::Navigate);
	}
	else if (dy)
	{
		// Hire or dismiss crew for the flagship or an escort
		// ship.
		DMS_ModifyCrew(pMS, hStarShip, dy);
	}
	else if (select && hStarShip)
	{
		ShipFragment* StarShipPtr = LockShipFrag(
			&uqm::gameState().game().built_ship_q, hStarShip);
		HFLEETINFO hTemplate = GetStarShipFromIndex(
			&uqm::gameState().game().avail_race_q, StarShipPtr->race_id);

		if (StarShipPtr->crew_level > StarShipPtr->max_crew)
		{
			StarShipPtr->crew_level -= StarShipPtr->max_crew;
			showRemainingPoints(0);
		}
		else if (StarShipPtr->crew_level > 0)
		{
			StarShipPtr->crew_level += StarShipPtr->max_crew;
			showRemainingPoints(ShipPoints(hTemplate));
		}

		GfxRect r;
		PreUpdateFlashRect();
		DMS_GetEscortShipRect(&r, StarShipPtr->index);
		ShowShipCrew(StarShipPtr, &r);
		PostUpdateFlashRect();

		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}
}

// Helper function for DoModifyShips(), called every time DoModifyShip() is
// called when we are in the mode where you can select a ship or empty
// slot.
static void DMS_NavigateShipSlots(MenuState* pMS, bool special, bool select,
								  bool cancel, int8_t dx, int8_t dy)
{
	HSHIPFRAG hStarShip = GetEscortByStarShipIndex(pMS->CurState);

	if (dx || dy)
	{
		// Moving through the ship slots.
		const uint8_t NewState = DMS_MoveCursor(pMS->CurState, dx, dy);
		if (NewState != pMS->CurState)
		{
			pMS->CurState = NewState;
			DMS_FlashActiveShip(pMS);
		}
	}

	if (special)
	{
		if (DMS_SpinShip(pMS, hStarShip))
		{
			DMS_SetMode(pMS, DmsMode::Navigate);
		}
	}
	else if (select)
	{
		if (hStarShip == 0 && uqm::highNibble(pMS->CurState) == 0)
		{
			// Select button was pressed over an empty escort
			// ship slot. Switch to 'add escort ship' mode.
			if (!StorageQ() || !stowCount)
			{
				stowed_q = false;
			}
			else if (currentStowShip >= stowCount)
			{
				currentStowShip = stowCount - 1;
			}
			pMS->delta_item = ModifyCrewFlag | (stowed_q ? currentStowShip : 0);
			DrawRaceStrings(uqm::lowByte(pMS->delta_item));
			DMS_SetMode(pMS, DmsMode::AddEscort);
		}
		else
		{
			// Select button was pressed over an escort ship or
			// the flagship. Entering crew editing mode
			pMS->delta_item |= ModifyCrewFlag;
			DMS_SetMode(pMS, DmsMode::EditCrew);
		}
	}
	else if (cancel)
	{
		// Leave escort ship editor.
		pMS->InputFunc = DoShipyard;
		pMS->CurState = *ShipyardMenuItem::Crew;
		DrawMenuStateStrings(MenuString::Crew, pMS->CurState);
		DMS_SetMode(pMS, DmsMode::Exit);
	}
}

/* In this routine, the least significant byte of pMS->CurState is used
 * to store the current selected ship index
 * a special case for the row is hi-nibble == -1 (0xf), which specifies
 * SIS as the selected ship
 * some bitwise math is still done to scroll through ships, for it to work
 * ships per row number must divide 0xf0 without remainder
 */
static bool DoModifyShips(MenuState* pMS)
{
	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		pMS->InputFunc = DoShipyard;
		return true;
	}

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoModifyShips;
		pMS->Initialized = true;
		pMS->CurState = uqm::makeByte(0, 0xF);
		pMS->delta_item = 0;
		SetContext(SpaceContext);
		DMS_SetMode(pMS, DmsMode::Navigate);
		stowCount = GetStowedShipCount();
		availableCount = GetAvailableRaceCount();
	}
	else
	{
		bool special = (PulsedInputState.menu[*MenuKey::MenuSpecial] != 0);
		const bool select = (PulsedInputState.menu[*MenuKey::MenuSelect] != 0);
		const bool cancel = (PulsedInputState.menu[*MenuKey::MenuCancel] != 0);
		int8_t dx = 0;
		int8_t dy = 0;


		if (!(pMS->delta_item & ModifyCrewFlag))
		{
			// Navigating through the ship slots.
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

			DMS_NavigateShipSlots(pMS, special, select, cancel, dx, dy);
		}
		else
		{
			// Add an escort ship or edit the crew of a ship.
			HSHIPFRAG hStarShip = GetEscortByStarShipIndex(pMS->CurState);

			if (hStarShip == 0 && uqm::highNibble(pMS->CurState) == 0)
			{
				// Cursor is over an empty escort ship slot, while we're
				// in 'add escort ship' mode.
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
					dx = -1;
				}
				if (PulsedInputState.menu[*MenuKey::MenuDown])
				{
					dx = 1;
				}
				if (PulsedInputState.menu[*MenuKey::MenuZoomIn])
				{
					dx = 5;
				}
				if (PulsedInputState.menu[*MenuKey::MenuZoomOut])
				{
					dx = -5;
				}
				if (PulsedInputState.menu[*MenuKey::MenuNext])
				{
					dy = 1;
				}

				DMS_AddEscortShip(pMS, special, select, cancel, dx, dy);
			}
			else
			{
				// Crew editing mode.
				if (PulsedInputState.menu[*MenuKey::MenuUp])
				{
					dy = -1;
				}
				if (PulsedInputState.menu[*MenuKey::MenuDown])
				{
					dy = 1;
				}
				if (PulsedInputState.menu[*MenuKey::MenuRight])
				{
					dy = -10;
				}
				if (PulsedInputState.menu[*MenuKey::MenuLeft])
				{
					dy = 10;
				}
				if (PulsedInputState.menu[*MenuKey::MenuZoomIn])
				{
					dy = -50;
				}
				if (PulsedInputState.menu[*MenuKey::MenuZoomOut])
				{
					dy = 50;
				}
				special = false;
				if (PulsedInputState.menu[*MenuKey::MenuNext])
				{
					special = true;
				}

				// Special prepares / unprepares stowing, select = cancel
				DMS_EditCrewMode(pMS, hStarShip, special && StorageQ(),
								 select || cancel, dy);
			}
		}
	}

	SleepThread(c_gameSecond / 60);
	// Kruzen: was 30, upped to 60 to fit new HdResolutionFactor
	// powerline animation. No issues detected so far

	return true;
}

static void DrawBluePrint(MenuState* pMS)
{
	uint16_t num_frames {};
	GfxStamp s;

	FRAME ModuleFrame {CaptureDrawable(LoadGraphic(SisbluMaskAnimRes))};

	s.origin.x = -SafePad();
	s.origin.y = 0;
	s.frame = DecFrameIndex(ModuleFrame);
	SetContextForeGroundColor(BlueprintColor);
	DrawFilledStamp(&s);

	for (num_frames = 0; num_frames < NumDriveSlots; ++num_frames)
	{
		DrawShipPiece(ModuleFrame, *uqm::gameState().sis().DriveSlots[num_frames],
					  num_frames, true);
	}
	for (num_frames = 0; num_frames < NumJetSlots; ++num_frames)
	{
		DrawShipPiece(ModuleFrame, *uqm::gameState().sis().JetSlots[num_frames],
					  num_frames, true);
	}
	for (num_frames = 0; num_frames < NumModuleSlots; ++num_frames)
	{
		const uint8_t which_piece {*uqm::gameState().sis().ModuleSlots[num_frames]};

		if (!(pMS->CurState == StarbaseMenuItem::Shipyard
			  && (which_piece == ModuleType::CrewPod || which_piece == ModuleType::StorageBay)))
		{
			DrawShipPiece(ModuleFrame, which_piece, num_frames, true);
		}
	}

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09));
	for (num_frames = 0; num_frames < NumModuleSlots; ++num_frames)
	{
		const uint8_t which_piece {*uqm::gameState().sis().ModuleSlots[num_frames]};
		if (pMS->CurState == StarbaseMenuItem::Shipyard
			&& (which_piece == ModuleType::CrewPod || which_piece == ModuleType::StorageBay))
		{
			DrawShipPiece(ModuleFrame, which_piece, num_frames, true);
		}
	}

	{
		num_frames = uqm::gameState().sis().CrewEnlisted;
		uqm::gameState().sis().CrewEnlisted = 0;

		while (num_frames--)
		{
			GfxRect r;
			// Crew dots
			r.extent.width = ResScale(1);
			r.extent.height = r.extent.width;

			GetCPodCapacity(&r.corner);
			DrawFilledRectangle(&r);

			++uqm::gameState().sis().CrewEnlisted;
		}
	}
	{
		GfxRect r;

		num_frames = uqm::gameState().sis().TotalElementMass;
		uqm::gameState().sis().TotalElementMass = 0;

		r.extent.width = ResScale(9);
		r.extent.height = ResScale(1);
		while (num_frames)
		{
			uint16_t m {};

			m = num_frames < SbayMassPerRow ?
					num_frames :
					SbayMassPerRow;
			uqm::gameState().sis().TotalElementMass += m;
			GetSBayCapacity(&r.corner);
			DrawFilledRectangle(&r);
			num_frames -= m;
		}
	}

	DrawFuelInFTanks(false);

	showRemainingCrew();
	showRemainingPoints(0);

	DestroyDrawable(ReleaseDrawable(ModuleFrame));
}

bool DoShipyard(MenuState* pMS)
{
	bool select {};
	bool cancel {};
	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		goto ExitShipyard;
	}

	select = PulsedInputState.menu[*MenuKey::MenuSelect];
	cancel = PulsedInputState.menu[*MenuKey::MenuCancel];

	OutfitOrShipyard = 3;

	FillHangarX();

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoShipyard;
		stowCount = GetStowedShipCount();
		availableCount = GetAvailableRaceCount();

		if (DosMenu())
		{
			memset(&ShipState, 0, sizeof ShipState);
			InventoryShips(&ShipState);
		}

		if (!IsDos())
		{
			ModuleFont = LoadFont(ModuleFontRes);
		}

		{
			GfxStamp s;
			GfxRect r;
			GfxRect old_r;

			pMS->ModuleFrame = CaptureDrawable(
				LoadGraphic(ShipyardPmapAnimRes));

			pMS->HangarColorMap = uqm::loadStringTable(HangarColorTabRes).release();

			pMS->hMusic = LoadMusicInstance(ShipyardMusicRes);

			SetTransitionSource(nullptr);
			BatchGraphics();
			DrawSISFrame();
			DrawSISMessage(GameString(GameStringBase::StarbaseString + 3));
			DrawSISTitle(GameString(GameStringBase::StarbaseString));
			SetContext(SpaceContext);
			DrawBluePrint(pMS);

			pMS->CurState = *ShipyardMenuItem::Crew;
			DrawMenuStateStrings(MenuString::Crew, pMS->CurState);

			SetContext(SpaceContext);
			s.origin.x = 0;
			s.origin.y = 0;
			s.frame = SetAbsFrameIndex(pMS->ModuleFrame, 0);

			if (!(IsPad() || classicPackPresent))
			{
				// PC hangar
				// the PC ship dock needs to overwrite the border
				// expand the clipping rect by 1 pixel
				GetContextClipRect(&old_r);
				r = old_r;
				r.corner.x -= ResScale(1);
				r.extent.width += ResScale(2);
				r.extent.height += ResScale(1);

				SetContextClipRect(&r);
				DrawStamp(&s);

				if (uqm::UQMOptions::read().customBorder)
				{
					DrawBorder(BorderFrameIndex::SisRepair);
				}

				SetContextClipRect(&old_r);

				animatePowerLines(pMS);
			}
			else
			{
				DrawStamp(&s);

				if (classicPackPresent)
				{
					animatePowerLines(pMS);
				}
			}

			if (isPC(uqm::UQMOptions::read().whichFonts))
			{
				SetContextFont(TinyFont);
			}
			else
			{
				SetContextFont(TinyFontBold);
			}

			ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);
			UnbatchGraphics();

			PlayMusicResume(pMS->hMusic, NormalVolume);

			ShowCombatShip(pMS, (uint16_t)~0, nullptr);

			SetInputCallback(on_input_frame);

			SetFlashRect(SfrMenu3do(), false);
		}

		pMS->Initialized = true;
	}
	else if (cancel || (select && pMS->CurState == ShipyardMenuItem::Exit))
	{
ExitShipyard:
		SetInputCallback(nullptr);


		if (pMS->CurState < ShipyardMenuItem::Exit)
		{
			DrawMenuStateStrings(MenuString::Crew, *ShipyardMenuItem::Exit);
		}

		DestroyDrawable(ReleaseDrawable(pMS->ModuleFrame));
		pMS->ModuleFrame = 0;
		delete pMS->HangarColorMap;
		pMS->HangarColorMap = 0;

		// Release Fonts
		if (!IsDos())
		{
			DestroyFont(ModuleFont);
		}

		SetMusicPosition();

		return false;
	}
	else if (select)
	{
		if (pMS->CurState != ShipyardMenuItem::SaveLoad)
		{
			pMS->Initialized = false;
			DrawMenuStateStrings(MenuString::Crew, pMS->CurState);
			DoModifyShips(pMS);
		}
		else
		{
			// Clearing FlashRect is not necessary
			if (!GameOptions())
			{
				goto ExitShipyard;
			}
			DrawMenuStateStrings(MenuString::Crew, pMS->CurState);
			SetFlashRect(SfrMenu3do(), false);
		}
	}
	else
	{
		DoMenuChooser(pMS, MenuString::Crew);
	}

	if (uqm::UQMOptions::read().infiniteRU)
	{
		uqm::gameState().sis().ResUnits = 1000000L;
	}

	return true;
}