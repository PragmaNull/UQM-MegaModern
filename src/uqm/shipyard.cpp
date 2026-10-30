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

#define DOS_MENU (uqm::UQMOptions::read().dosMenus || IS_DOS) // The DOS Menu window

// 3DO 4x3 hangar layout
#define HANGAR_SHIPS_ROW_3DO 4
#define HANGAR_Y_3DO 64
#define HANGAR_DY_3DO 44

const int16_t hangar_x_coords_3do[HANGAR_SHIPS_ROW_3DO] =
	{
		19, 60, 116, 157};

// modified PC 6x2 hangar layout
#define HANGAR_SHIPS_ROW_DOS 6
// The Y position of the upper line of hangar bay doors.
#define HANGAR_Y RES_SCALE(SAFE_BOOL(DOS_BOOL(88, 71), HANGAR_Y_3DO))
// The Y position of the lower line of hangar bay doors.
#define HANGAR_DY RES_SCALE(SAFE_BOOL(DOS_BOOL(84, 76), HANGAR_DY_3DO))

// The X positions of the hangar bay doors for each resolution mode.
// Calculated from the right edge of the left grey border bar on the
// screen.
const int16_t hangar_x_coords_uqm[HANGAR_SHIPS_ROW_DOS] =
	{
		0, 38, 76, 131, 169, 207};

const int16_t hangar_x_coords_dos[HANGAR_SHIPS_ROW_DOS] =
	{
		0, 38, 76, 133, 171, 209};

int16_t hangar_x_coords[HANGAR_SHIPS_ROW_DOS];

#define HANGAR_SHIPS 12
#define HANGAR_SHIPS_ROW SAFE_BOOL(HANGAR_SHIPS_ROW_DOS, \
								   HANGAR_SHIPS_ROW_3DO)

#define HANGAR_ROWS (HANGAR_SHIPS / HANGAR_SHIPS_ROW)

#define HANGAR_ANIM_RATE RES_SCALE(15) // fps

enum
{
	SHIPYARD_CREW,
	SHIPYARD_SAVELOAD,
	SHIPYARD_EXIT
};

// Editing mode for DoModifyShips()
typedef enum
{
	DMS_Mode_navigate,	// Navigating the ship slots.
	DMS_Mode_addEscort, // Selecting a ship to add to an empty slot.
	DMS_Mode_editCrew,	// Hiring or dismissing crew.
	DMS_Mode_exit,		// Leaving DoModifyShips() mode.
} DMS_Mode;

static bool DoShipSpins;

static bool stowed_q;				// Build queue or storage queue
static uint8_t currentStowShip = 0; // Saves location in storage queue
static uint16_t stowCount = 0;		// Number of ships stowed
static uint16_t availableCount = 0; // Number of ships available to purchase

// This is all for drawing the DOS version modules menu
#define SHIPS_ORG_Y RES_SCALE(36)
#define SHIPS_SPACING_Y RES_SCALE(16)

#define SHIPS_COL_0 RES_SCALE(5)
#define SHIPS_COL_1 RES_SCALE(61)

#define SHIPS_SEL_ORG_X (SHIPS_COL_0 - RES_SCALE(1))
#define SHIPS_SEL_WIDTH (FIELD_WIDTH - RES_SCALE(3))

#define NAME_OFS_Y RES_SCALE(2)
#define TEXT_BASELINE RES_SCALE(6)
#define TEXT_SPACING_Y RES_SCALE(7)

#define MAX_VIS_MODULES 5

typedef struct
{
	char ShipName[30];
	uint16_t ShipCost;
	uint8_t CrewLevel, MaxCrew;
} SHIP_STATS;

typedef struct
{
	uint16_t count;	   // Number of buildable ships in the list
	uint16_t topIndex; // Index of the top ship displayed
	SHIP_STATS ShipStats[NUM_BUILDABLE_SHIPS + MAX_STOWED_SHIPS];
} SHIPS_STATE;

SHIPS_STATE ShipState;

// Count the ships in the stowed queue
static uint16_t
GetStowedShipCount(void)
{
	uint16_t Index;
	HSHIPFRAG hStarShip, hNextShip;

	Index = 0;
	for (hStarShip = GetHeadLink(&GLOBAL(stowed_ship_q));
		 hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT* StarShipPtr;

		StarShipPtr = LockShipFrag(&GLOBAL(stowed_ship_q), hStarShip);
		++Index;
		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockShipFrag(&GLOBAL(stowed_ship_q), hStarShip);
	}

	return Index;
}

uint16_t ShipPoints(HFLEETINFO hFleet);

static void
DrawShipsStatus(uint16_t index, uint16_t pos, bool selected)
{
	GFXRECT r;
	TEXT t;
	char buf[10];

	if (stowed_q)
	{
		index += NUM_BUILDABLE_SHIPS;
	}
	t.align = ALIGN_LEFT;
	t.baseline.x = SHIPS_COL_0;

	r.extent.width = SHIPS_SEL_WIDTH;
	r.extent.height = TEXT_SPACING_Y * 2;
	r.corner.x = SHIPS_SEL_ORG_X;

	// draw line background
	r.corner.y = SHIPS_ORG_Y + pos * SHIPS_SPACING_Y + NAME_OFS_Y;
	SetContextForeGroundColor(selected ?
								  MODULE_SELECTED_BACK_COLOR :
								  MODULE_BACK_COLOR);
	DrawFilledRectangle(&r);
	SetContextFont(TinyFont);

	// print ship name or captain name
	SetContextForeGroundColor(selected ? MODULE_SELECTED_COLOR :
										 (stowed_q ? CAPTAIN_NAME_TEXT_COLOR : MODULE_NAME_COLOR));
	t.baseline.y = r.corner.y + TEXT_BASELINE;
	t.pStr = ShipState.ShipStats[index].ShipName;
	t.CharCount = (stowed_q ? (uint16_t)~0 : uqm::utf8StringPos(t.pStr, ' '));
	font_DrawText(&t);
	if (!stowed_q)
	{
		// print second half of ship name
		t.baseline.y += TEXT_SPACING_Y;
		t.pStr = uqm::skipUTF8Chars(t.pStr, t.CharCount + 1);
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
	// print ship cost or fleet points
	SetContextForeGroundColor(selected ?
								  MODULE_SELECTED_COLOR :
								  MODULE_PRICE_COLOR);
	t.align = ALIGN_RIGHT;
	t.baseline.x = SHIPS_COL_1 - RES_SCALE(2);
	if (!stowed_q)
	{
		t.baseline.y -= RES_SCALE(3);
	}
	fmt::format_to_sz_n(buf, "{}", ShipState.ShipStats[index].ShipCost);
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
	if (stowed_q)
	{
		// print crew totals
		t.baseline.y += TEXT_SPACING_Y;
		fmt::format_to_sz_n(buf, "{}/{}",
							ShipState.ShipStats[index].CrewLevel,
							ShipState.ShipStats[index].MaxCrew);
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
}

static void
DrawShipsDisplay(SHIPS_STATE* shipState)
{
	TEXT t;
	GFXRECT r;
	uint16_t i;

	r.corner.x = RES_SCALE(2);
	r.corner.y = RES_SCALE(20);
	r.extent.width = FIELD_WIDTH + RES_SCALE(1);
	r.extent.height = (RES_SCALE(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IS_HD)
	{
		DrawStarConBox(&r, RES_SCALE(1),
					   SHADOWBOX_MEDIUM_COLOR, SHADOWBOX_DARK_COLOR,
					   true, MODULE_BACK_COLOR, false, TRANSPARENT);
	}
	else
	{
		DrawBorder(DEVICE_CARGO_FRAME);
	}

	// print the "SHIPS" title
	SetContextFont(StarConFont);
	t.baseline.x = (STATUS_WIDTH >> 1) - RES_SCALE(1);
	t.baseline.y = r.corner.y + RES_SCALE(7);
	t.align = ALIGN_CENTER;
	t.pStr = GAME_STRING(STARBASE_STRING_BASE + 10);
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(MODULE_SELECTED_COLOR);
	font_DrawText(&t);

	// Indicate mode
	t.baseline.y += RES_SCALE(7);
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
	uint16_t count = (stowed_q ? stowCount : shipState->count);
	for (i = 0; i < MAX_VIS_MODULES; ++i)
	{
		uint16_t modIndex = shipState->topIndex + i;

		if (modIndex >= count)
		{
			break;
		}

		DrawShipsStatus(modIndex, i, false);
	}
}

static void
DrawShips(SHIPS_STATE* shipState, uint16_t NewItem)
{
	GFXCONTEXT OldContext = SetContext(StatusContext);
	uint16_t pos = NewItem - shipState->topIndex;

	BatchGraphics();

	DrawShipsDisplay(shipState);
	DrawShipsStatus(NewItem, pos, true);

	UnbatchGraphics();

	SetContext(OldContext);
}

static void
ManipulateShips(int16_t NewState)
{
	SHIPS_STATE* shipState;
	int16_t NewTop;

	if (!DOS_MENU)
	{
		return;
	}

	shipState = &ShipState;
	NewTop = shipState->topIndex;

	if (NewState > NUM_BUILDABLE_SHIPS + MAX_STOWED_SHIPS)
	{
		DrawShips(&ShipState, NewState);
		return;
	}

	if (NewState < NewTop || NewState >= NewTop + MAX_VIS_MODULES)
	{
		shipState->topIndex = NewState - NewState % MAX_VIS_MODULES;
	}

	DrawShips(shipState, NewState);
}

static void
GetShipStats(SHIP_STATS* ship_stats, SPECIES_ID species_id)
{
	HMASTERSHIP hMasterShip = FindMasterShip(species_id);
	MASTER_SHIP_INFO* ShipPtr = LockMasterShip(&master_q, hMasterShip);

	fmt::format_to_sz_n(ship_stats->ShipName, "{} {}",
						(char*)GetStringAddress(SetAbsStringTableIndex(
							ShipPtr->ShipInfo.race_strings, 2)),
						(char*)GetStringAddress(SetAbsStringTableIndex(
							ShipPtr->ShipInfo.race_strings, 4)));

	ship_stats->ShipCost = ShipPtr->ShipInfo.ship_cost * 100;

	UnlockMasterShip(&master_q, hMasterShip);
}

static void
GetStowedShipStats(SHIP_STATS* ship_stats, HSHIPFRAG hStowShip)
{
	SHIP_FRAGMENT* StowShipPtr = LockShipFrag(
		&GLOBAL(stowed_ship_q), hStowShip);

	HMASTERSHIP hMasterShip = FindMasterShip(StowShipPtr->SpeciesID);
	MASTER_SHIP_INFO* ShipPtr = LockMasterShip(&master_q, hMasterShip);
	ship_stats->ShipCost = ShipPtr->ShipInfo.ship_cost;
	UnlockMasterShip(&master_q, hMasterShip);

	fmt::format_to_sz_n(ship_stats->ShipName, "{}",
						(char*)GetStringAddress(SetAbsStringTableIndex(StowShipPtr->race_strings, StowShipPtr->captains_name_index)));

	ship_stats->CrewLevel = StowShipPtr->crew_level;
	ship_stats->MaxCrew = StowShipPtr->max_crew;

	UnlockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
}

static void
InventoryShips(SHIPS_STATE* ship_state)
{
	uint16_t i;
	int16_t ShipsOnBoard = 0;

	for (i = 0; i < NUM_BUILDABLE_SHIPS; ++i)
	{
		HFLEETINFO hStarShip =
			GetStarShipFromIndex(&GLOBAL(avail_race_q), i);
		FLEET_INFO* FleetPtr =
			LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);

		if (FleetPtr->allied_state == GOOD_GUY || FleetPtr->can_build)
		{
			GetShipStats(&ship_state->ShipStats[ShipsOnBoard],
						 FleetPtr->SpeciesID);

			++ShipsOnBoard;
		}

		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	}

	ship_state->count = ShipsOnBoard;
	stowCount = GetStowedShipCount();

	HSHIPFRAG hStowShip = GetStarShipFromIndex(&GLOBAL(stowed_ship_q), 0);
	while (i < stowCount + NUM_BUILDABLE_SHIPS && hStowShip)
	{
		SHIP_FRAGMENT* StowShipPtr =
			LockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
		HSHIPFRAG hNext = StowShipPtr->succ;

		GetStowedShipStats(&(ship_state->ShipStats[i]), hStowShip);

		UnlockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
		hStowShip = hNext;
		i++;
	}
}

static void
FillHangarX(void)
{
	uint8_t i;

	for (i = 0; i < HANGAR_SHIPS_ROW; i++)
	{
		if (IS_PAD)
		{
			hangar_x_coords[i] = hangar_x_coords_3do[i];
		}
		else
		{
			if (!IS_DOS)
			{
				hangar_x_coords[i] = hangar_x_coords_uqm[i];
			}
			else
			{
				hangar_x_coords[i] = hangar_x_coords_dos[i];
			}
		}

		if (IS_HD)
		{
			hangar_x_coords[i] <<= RESOLUTION_FACTOR;
		}
	}
}

#define WANDERING_X (optWindowType == uqm::WindowStyle::UQM ? 2 : (optWindowType == uqm::WindowStyle::Console3DO ? 6 : 19))

static void
showRemainingCrew(void)
{
	GFXRECT r;
	TEXT t;
	char buf[30] {};
	int16_t remaining_crew;
#define INITIAL_CREW 2000

	if (!isDifficulty(uqm::Difficulty::Hard))
	{
		return;
	}

	BatchGraphics();

	remaining_crew = INITIAL_CREW - (int16_t)uqm::makeWord(GET_GAME_STATE(CREW_PURCHASED0), GET_GAME_STATE(CREW_PURCHASED1));

	r.extent = MAKE_EXTENT(RES_SCALE(122), RES_SCALE(7));
	r.corner = MAKE_POINT(RES_SCALE(WANDERING_X),
						  RES_SCALE(74) - (r.extent.height + RES_SCALE(2)));

	if (optWindowType != uqm::WindowStyle::UQM)
	{
		r.corner.y = RES_SCALE(1);
	}

	SetContextForeGroundColor(BLACK_COLOR);
	DrawFilledRectangle(&r);

	// Print remaining crew
	SetContextFont(TinyFont);
	SetContextForeGroundColor(BLUEPRINT_COLOR);
	t.baseline.x = r.corner.x;
	t.baseline.y = r.corner.y + r.extent.height - RES_SCALE(1);
	t.align = ALIGN_LEFT;
	t.CharCount = (uint16_t)~0;
	t.pStr = buf;
	uqm::utf8StringCopy(
		buf, sizeof(buf), GAME_STRING(STARBASE_STRING_BASE + 6));
	// Remaining Crew:

	font_DrawText(&t);

	t.baseline.x += RES_SCALE(71);
	SetContextForeGroundColor(
		remaining_crew > 1000 ? FULL_CREW_COLOR :
								(remaining_crew < 250 ? LOW_CREW_COLOR :
														HALF_CREW_COLOR));

	if (CheckAlliance(SHOFIXTI_SHIP) == GOOD_GUY)
	{
		const auto fmtResult = fmt::format_to_n(buf, sizeof(buf) - 1, "{}", STR_INFINITY_SIGN);
		*fmtResult.out = '\0';
	}
	else if (remaining_crew == 0)
	{
		uqm::utf8StringCopy(
			buf, sizeof(buf), GAME_STRING(STARBASE_STRING_BASE + 7));
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

uint16_t
ShipPoints(HFLEETINFO hFleet)
{
	if (!hFleet)
	{
		return 0;
	}
	FLEET_INFO* FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);

	uint16_t shipCost = GetShipCostFromIndex(FindMasterShipIndex(FleetPtr->SpeciesID));

	UnlockFleetInfo(&GLOBAL(avail_race_q), hFleet);
	return shipCost;
}

uint16_t
ShipCost(uint8_t race_id)
{
	return ShipPoints(GetStarShipFromIndex(&GLOBAL(avail_race_q), race_id)) * 100;
}

static int16_t
CalculateAllyPoints()
{
	uint8_t i;
	HFLEETINFO hFleet;
	FLEET_INFO* FleetPtr;
	int16_t MaxPoints = difficultyCase(90, 60, 30);

	for (i = ARILOU_SHIP; i <= LAST_MELEE_ID; i++)
	{
		hFleet = GetStarShipFromIndex(&GLOBAL(avail_race_q), i);
		if (!hFleet)
		{
			continue;
		}

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);
		if (!FleetPtr)
		{
			continue;
		}

		if (FleetPtr->allied_state == GOOD_GUY)
		{
			MaxPoints += ShipPoints(hFleet) * difficultyCase(3, 2, 1);
		}

		UnlockFleetInfo(&GLOBAL(avail_race_q), hFleet);
	}

	return MaxPoints;
}

/*
 * Returns the total point value of all the ships escorting the SIS.
 */
uint16_t
CalculateEscortsPoints(void)
{
	uint16_t total = 0;
	HSHIPFRAG hStarShip, hNextShip;

	for (hStarShip = GetHeadLink(&GLOBAL(built_ship_q));
		 hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT* StarShipPtr = LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
		hNextShip = _GetSuccLink(StarShipPtr);
		total += ShipPoints(GetStarShipFromIndex(&GLOBAL(avail_race_q), StarShipPtr->race_id));
		UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
	}
	return total;
}

#define REMAINING_FP (CalculateAllyPoints() - CalculateEscortsPoints())

bool CanBuyPoints(HFLEETINFO hFleet)
{
	if ((!uqm::UQMOptions::read().fleetPointSys && !isDifficulty(uqm::Difficulty::Hard))
		|| GET_GAME_STATE(CHMMR_BOMB_STATE) == 3)
	{
		return true;
	}

	if (ShipPoints(hFleet) > REMAINING_FP)
	{
		return false;
	}

	return true;
}

static void
showRemainingPoints(int delta)
{
	GFXRECT r;
	TEXT t;
	int8_t percentage_left;
	int16_t FP = REMAINING_FP + delta;

	if ((!uqm::UQMOptions::read().fleetPointSys && !isDifficulty(uqm::Difficulty::Hard))
		|| GET_GAME_STATE(CHMMR_BOMB_STATE) == 3)
	{
		return;
	}

	BatchGraphics();

	// Draw black rectangle before drawing text
	r.extent = MAKE_EXTENT(RES_SCALE(100), RES_SCALE(7));
	r.corner.x = RES_SCALE(((optWindowType != uqm::WindowStyle::Console3DO) ?
								DOS_SIS_SCREEN_WIDTH :
								THREEDO_SIS_SCREEN_WIDTH)
						   - 100);
	r.corner.y = RES_SCALE(74) - (r.extent.height + RES_SCALE(2));

	if (optWindowType != uqm::WindowStyle::UQM)
	{
		r.corner.y = RES_SCALE(1);
	}

	SetContextForeGroundColor(BLACK_COLOR);
	DrawFilledRectangle(&r);

	// Draw remaining points
	SetContextFont(TinyFont);
	t.align = ALIGN_RIGHT;
	t.CharCount = (uint16_t)~0;
	char buf[30] {};
	fmt::format_to_n(buf, sizeof(buf) - 1, "{}", FP);
	t.pStr = buf;

	r.corner.x = RES_SCALE(((optWindowType != uqm::WindowStyle::Console3DO) ?
								DOS_SIS_SCREEN_WIDTH :
								THREEDO_SIS_SCREEN_WIDTH)
						   - WANDERING_X);

	t.baseline.x = r.corner.x;
	t.baseline.y = r.corner.y + r.extent.height - RES_SCALE(1);

	percentage_left = (FP > 0 ?
						   (float)FP / (float)CalculateAllyPoints() * 100 :
						   0);
	SetContextForeGroundColor(
		percentage_left > 50 ? FULL_CREW_COLOR :
							   (percentage_left < 25 ? LOW_CREW_COLOR :
													   HALF_CREW_COLOR));

	font_DrawText(&t);

	// Draw the "Fleet Points: " text
	r = font_GetTextRect(&t);
	t.baseline.x -= r.extent.width;
	uqm::utf8StringCopy(
		buf, sizeof(buf), GAME_STRING(STARBASE_STRING_BASE + 8));
	// Fleet Points:

	SetContextForeGroundColor(BLUEPRINT_COLOR);
	font_DrawText(&t);

	UnbatchGraphics();
}

static void
animatePowerLines(MENU_STATE* pMS)
{
	static STAMP s;
	static COLORMAP ColorMap;
	static TimeCount NextTime = 0;
	TimeCount Now = GetTimeCounter();

	if (IS_PAD || GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return;
	}

	if (pMS)
	{ // Init animation
		s.origin.x = 0;
		s.origin.y = 0;
		s.frame = SetAbsFrameIndex(pMS->ModuleFrame, 28);
		ColorMap = SetAbsColorMapIndex(pMS->CurString, 0);
	}

	if (Now >= NextTime || pMS)
	{
		NextTime = Now + (GameTicksPerSecond / HANGAR_ANIM_RATE);

		SetColorMap(GetColorMapAddress(ColorMap));
		DrawStamp(&s);
		// Advance colomap cycle
		ColorMap = SetRelColorMapIndex(ColorMap, 1);
	}
}

static void
SpinStarShip(MENU_STATE* pMS, HFLEETINFO hStarShip)
{
	int Index;
	FLEET_INFO* FleetPtr;

	FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	Index = FindMasterShipIndex(FleetPtr->SpeciesID);
	UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);

	if (Index >= 0 && Index < NUM_MELEE_SHIPS)
	{
		if (isPC(uqm::UQMOptions::read().whichIntro))
		{
			PlayMenuSound(MENU_SOUND_SUCCESS);
		}
		DoShipSpins = true;
		DoShipSpin(Index, pMS->hMusic);
	}
	DoShipSpins = false;
}

static void
on_input_frame(void)
{
	GFXCONTEXT oldContext;

	oldContext = SetContext(SpaceContext);
	if (!DoShipSpins)
	{
		animatePowerLines(nullptr);
	}
	SetContext(oldContext);
}

// Count the ships which can be built by the player.
static uint16_t
GetAvailableRaceCount(void)
{
	uint16_t Index;
	HFLEETINFO hStarShip, hNextShip;

	Index = 0;
	for (hStarShip = GetHeadLink(&GLOBAL(avail_race_q));
		 hStarShip; hStarShip = hNextShip)
	{
		FLEET_INFO* FleetPtr;

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		if (FleetPtr->allied_state == GOOD_GUY
			|| FleetPtr->can_build)
		{
			++Index;
		}

		hNextShip = _GetSuccLink(FleetPtr);
		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	}

	return Index;
}

static HFLEETINFO
GetAvailableRaceFromIndex(uint8_t Index)
{
	HFLEETINFO hStarShip, hNextShip;

	for (hStarShip = GetHeadLink(&GLOBAL(avail_race_q));
		 hStarShip; hStarShip = hNextShip)
	{
		FLEET_INFO* FleetPtr;

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		if ((FleetPtr->allied_state == GOOD_GUY
			 || FleetPtr->can_build)
			&& Index-- == 0)
		{
			UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
			return hStarShip;
		}

		hNextShip = _GetSuccLink(FleetPtr);
		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	}

	return 0;
}

static void
DrawShipyardShipText(GFXRECT* r, int Index)
{
	char race_name[64];
	char ship_name[64];
	FONT OldFont;
	Color OldColor;
	int16_t leading;
	GFXRECT block;
	TEXT text;
	int16_t strip_align;

	if (IS_DOS)
	{
		return;
	}

	HFLEETINFO hStarShip =
		GetStarShipFromIndex(&GLOBAL(avail_race_q), Index);
	FLEET_INFO* FleetPtr =
		LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	HMASTERSHIP hMasterShip = FindMasterShip(FleetPtr->SpeciesID);
	MASTER_SHIP_INFO* ShipPtr = LockMasterShip(&master_q, hMasterShip);
	uqm::utf8StringCopy((char*)&race_name, sizeof(race_name),
						(char*)GetStringAddress(SetAbsStringTableIndex(
							ShipPtr->ShipInfo.race_strings,
							GetStringTableCount(ShipPtr->ShipInfo.race_strings) - 3)));
	uqm::utf8StringCopy((char*)&ship_name, sizeof(ship_name),
						(char*)GetStringAddress(SetAbsStringTableIndex(
							ShipPtr->ShipInfo.race_strings,
							GetStringTableCount(ShipPtr->ShipInfo.race_strings) - 2)));
	UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	UnlockMasterShip(&master_q, hMasterShip);

	if (!strlen((char*)&race_name) || !strlen((char*)&ship_name))
	{
		return;
	}

	OldFont = SetContextFont(ModuleFont);
	OldColor = SetContextForeGroundColor(SHP_RECT_COLOR);

	GetContextFontLeading(&leading);

	if (!uqm::UQMOptions::read().customBorder)
	{
		block = *r;
		block.extent.height = (leading << 1);
		DrawFilledRectangle(&block);
	}
	else
	{
		DrawBorder(TEXT_LABEL_FRAME);
	}

	text.align = ALIGN_CENTER;
	text.baseline.x = r->corner.x + (r->extent.width >> 1);
	text.baseline.y = r->corner.y + leading;
	strip_align = text.baseline.x;

	// Actually we don't want to align, we just want to strip the alignment
	text.pStr = AlignText((const char*)&race_name, &strip_align);
	text.CharCount = (uint16_t)~0;
	font_DrawShadowedText(&text, WEST_SHADOW, SHP_TEXT_COLOR,
						  SHP_SHADOW_COLOR);

	text.baseline.y += leading;
	text.pStr = AlignText((const char*)&ship_name, &strip_align);
	text.CharCount = (uint16_t)~0;
	font_DrawShadowedText(&text, WEST_SHADOW, SHP_TEXT_COLOR,
						  SHP_SHADOW_COLOR);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static void
DrawRaceStrings(uint8_t NewRaceItem)
{
	GFXRECT r, textRect;
	STAMP s;
	GFXCONTEXT OldContext;


	OldContext = SetContext(StatusContext);
	GetContextClipRect(&r);
	s.origin.x = RADAR_X - r.corner.x;
	s.origin.y = RADAR_Y - r.corner.y;
	r.corner.x = s.origin.x - RES_SCALE(2);
	r.corner.y = s.origin.y - RES_SCALE(12);
	r.extent.width = RADAR_WIDTH + RES_SCALE(4);
	r.extent.height = RES_SCALE(12);
	BatchGraphics();
	ClearSISRect(CLEAR_SIS_RADAR);
	SetContextForeGroundColor(DKGRAY_COLOR);

	if (!IS_DOS)
	{
		if (!uqm::UQMOptions::read().customBorder)
		{
			DrawFilledRectangle(&r);
		}

		textRect = r;
	}

	DrawBorder(SIS_RADAR_FRAME);

	if (!IS_DOS)
	{
		r.corner = s.origin;
		r.extent.width = RADAR_WIDTH;
		r.extent.height = RADAR_HEIGHT;
		SetContextForeGroundColor(BLACK_COLOR);
		DrawFilledRectangle(&r);
	}
	else
	{
		GFXRECT dosRect;

		dosRect.corner.x = RES_SCALE(3);
		dosRect.corner.y = RADAR_Y + RES_SCALE(1);
		dosRect.extent.width = RADAR_WIDTH + RES_SCALE(2);
		dosRect.extent.height = RADAR_HEIGHT - RES_SCALE(2);

		if (!IS_HD)
		{
			DrawStarConBox(&dosRect, RES_SCALE(1),
						   PCMENU_TOP_LEFT_BORDER_COLOR,
						   PCMENU_BOTTOM_RIGHT_BORDER_COLOR, true, BLACK_COLOR,
						   false, TRANSPARENT);
		}
		else
		{
			DrawRenderedBox(&dosRect, true, BLACK_COLOR,
							THIN_INNER_BEVEL, uqm::UQMOptions::read().customBorder);
		}
	}

	if (NewRaceItem != (uint8_t)~0)
	{
		TEXT t;
		HFLEETINFO hStarShip;
		FLEET_INFO* FleetPtr;
		uint16_t shipCost, shipPoints, shipCrew, maxCrew;
		GFXRECT r;
		STRING captain;

		ManipulateShips(NewRaceItem);

		if (stowed_q)
		{
			HSHIPFRAG hStowShip = GetStarShipFromIndex(
				&GLOBAL(stowed_ship_q), NewRaceItem);
			SHIP_FRAGMENT* StowShipPtr = LockShipFrag(
				&GLOBAL(stowed_ship_q), hStowShip);
			NewRaceItem = StowShipPtr->race_id;
			shipCrew = StowShipPtr->crew_level;
			captain = SetAbsStringTableIndex(StowShipPtr->race_strings,
											 StowShipPtr->captains_name_index);
			UnlockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);

			hStarShip = GetStarShipFromIndex(
				&GLOBAL(avail_race_q), NewRaceItem);
			FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
			maxCrew = FleetPtr->crew_level;
			UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		}
		else
		{
			hStarShip = GetAvailableRaceFromIndex(NewRaceItem);
			NewRaceItem = GetIndexFromStarShip(&GLOBAL(avail_race_q),
											   hStarShip);
		}
		shipPoints = ShipPoints(hStarShip);
		shipCost = ShipCost(NewRaceItem);

		// Draw the ship name or captain name above the ship image.
		if (!IS_DOS)
		{
			if (stowed_q)
			{
				t.baseline.x = RES_SCALE(4) + RADAR_WIDTH / 2;
				t.baseline.y = RADAR_Y - RES_SCALE(2)
							 - DOS_NUM_SCL(2) - SAFE_Y;
				t.align = ALIGN_CENTER;
				t.pStr = (char*)GetStringAddress(captain);
				t.CharCount = GetStringLength(captain);
				font_DrawShadowedText(&t, WEST_SHADOW, CAPTAIN_NAME_TEXT_COLOR,
									  SHP_SHADOW_COLOR);
			}
			else
			{
				DrawShipyardShipText(&textRect, NewRaceItem);
			}
		}

		// Draw the ship image.
		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		s.frame = FleetPtr->melee_icon;
		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		s.origin.x += (RADAR_WIDTH >> 1);
		s.origin.y += (RADAR_HEIGHT >> 1);
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

		t.baseline.x = RES_SCALE(4) + RADAR_WIDTH - RES_SCALE(2);
		t.baseline.y = RADAR_Y + RADAR_HEIGHT - RES_SCALE(2)
					 - DOS_NUM_SCL(2) - SAFE_Y;
		t.align = ALIGN_RIGHT;
		t.CharCount = (uint16_t)~0;
		char buf[30] {};
		t.pStr = buf;
		if (!stowed_q)
		{
			// Print the ship cost
			fmt::format_to_n(buf, sizeof(buf) - 1, "{}", shipCost);

			if (shipCost <= (GLOBAL_SIS(ResUnits)))
			{
				SetContextForeGroundColor(BRIGHT_GREEN_COLOR);
			}
			else if (shipCost > (GLOBAL_SIS(ResUnits)))
			{
				SetContextForeGroundColor(BRIGHT_RED_COLOR);
			}
		}
		else
		{
			// Print the ship's crew
			fmt::format_to_n(buf, sizeof(buf) - 1, "{}/{}", shipCrew, maxCrew);

			if (shipCrew == maxCrew)
			{
				SetContextForeGroundColor(BRIGHT_GREEN_COLOR);
			}
			else if (shipCrew > maxCrew / 2)
			{
				SetContextForeGroundColor(BRIGHT_YELLOW_COLOR);
			}
			else
			{
				SetContextForeGroundColor(BRIGHT_RED_COLOR);
			}
		}

		font_DrawText(&t);

		// Print the fleet points
		if ((uqm::UQMOptions::read().fleetPointSys || isDifficulty(uqm::Difficulty::Hard))
			&& GET_GAME_STATE(CHMMR_BOMB_STATE) < 3)
		{
			t.baseline.y = RADAR_Y + RES_SCALE(7)
						 + DOS_NUM_SCL(2) - SAFE_Y;
			const auto fmtResult = fmt::format_to_n(buf, sizeof(buf) - 1, "{}", shipPoints);
			*fmtResult.out = '\0';

			if (shipPoints < REMAINING_FP / 2)
			{
				SetContextForeGroundColor(BRIGHT_GREEN_COLOR);
			}
			else if (shipPoints <= REMAINING_FP)
			{
				SetContextForeGroundColor(BRIGHT_YELLOW_COLOR);
			}
			else
			{
				SetContextForeGroundColor(BRIGHT_RED_COLOR);
			}

			font_DrawText(&t);

			r = font_GetTextRect(&t);
			t.baseline.x -= r.extent.width;

			uqm::utf8StringCopy(
				buf, sizeof(buf),
				GAME_STRING(STARBASE_STRING_BASE + 9)); // FP:
			SetContextForeGroundColor(BRIGHT_BLUE_COLOR);
			font_DrawText(&t);
		}
	}
	UnbatchGraphics();
	SetContext(OldContext);
}

// Width of an escort ship window.
#define SHIP_WIN_WIDTH RES_SCALE(34)

// Height of an escort ship window.
#define SHIP_WIN_HEIGHT (SHIP_WIN_WIDTH + RES_SCALE(6))

// For how many animation frames' time the escort ship bay doors
// are slid left and right when opening them. If this number is not large
// enough, part of the doors are left visible upon opening.
#define SHIP_WIN_FRAMES ((SHIP_WIN_WIDTH >> 1) + 1)

// Print the crew count of an escort ship on top of its (already drawn)
// image, either as '30' (full), '28/30' (partially full), or 'SCRAP'
// (empty).
// pRect is the rectangle of the ship image.
static void
ShowShipCrew(SHIP_FRAGMENT* StarShipPtr, const GFXRECT* pRect)
{
	GFXRECT r;
	TEXT t;
	HFLEETINFO hTemplate;
	FLEET_INFO* TemplatePtr;
	uint16_t maxCrewLevel;
	STRING captain;

	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	hTemplate = GetStarShipFromIndex(&GLOBAL(avail_race_q),
									 StarShipPtr->race_id);
	TemplatePtr = LockFleetInfo(&GLOBAL(avail_race_q), hTemplate);
	maxCrewLevel =
		EXTENDED ? TemplatePtr->max_crew : TemplatePtr->crew_level;
	UnlockFleetInfo(&GLOBAL(avail_race_q), hTemplate);

	captain = SetAbsStringTableIndex(StarShipPtr->race_strings,
									 StarShipPtr->captains_name_index);

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
							GAME_STRING(STARBASE_STRING_BASE + 5));
	}
	else
	{
		fmt::format_to_n(buf, sizeof(buf) - 1, "{}/{}", StarShipPtr->crew_level, maxCrewLevel);
	}

	r = *pRect;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + r.extent.height - RES_SCALE(1);
	t.align = ALIGN_CENTER;
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	if (r.corner.y)
	{
		// Black out the crew before printing it if this is a redraw
		r.corner.y = t.baseline.y - RES_SCALE(6);
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = RES_SCALE(6);
		SetContextForeGroundColor(BLACK_COLOR);
		DrawFilledRectangle(&r);
	}
	SetContextForeGroundColor((StarShipPtr->crew_level != 0) ?
								  (BUILD_COLOR(MAKE_RGB15(0x00, 0x14, 0x00), 0x02)) :
								  (BUILD_COLOR(MAKE_RGB15(0x12, 0x00, 0x00), 0x2B)));
	font_DrawText(&t);

	if (!uqm::UQMOptions::read().captainNames)
	{
		return;
	}

	r = *pRect;
	t.baseline.y = r.corner.y + RES_SCALE(7);
	t.pStr = (char*)GetStringAddress(captain);
	t.CharCount = GetStringLength(captain);
	if (!r.corner.y)
	{
		// Dim the top of the ship the first time you draw it
		r.corner.y = t.baseline.y - RES_SCALE(6);
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = RES_SCALE(6);
		Color shady = BUILD_COLOR_RGBA(0x00, 0x00, 0x00, 0x80);
		SetContextForeGroundColor(shady);
		DrawFilledRectangle(&r);

		SetContextFont(TinyFont);
		SetContextForeGroundColor(CAPTAIN_NAME_TEXT_COLOR);
		font_DrawText(&t);
	}
}

static void
ShowCombatShip(MENU_STATE* pMS, uint16_t which_window,
			   SHIP_FRAGMENT* YankedStarShipPtr)
{
	uint16_t i;
	uint16_t num_ships;
	HSHIPFRAG hStarShip, hNextShip;
	SHIP_FRAGMENT* StarShipPtr;
	struct
	{
		SHIP_FRAGMENT* StarShipPtr;
		GFXPOINT finished_s;
		STAMP ship_s;
		STAMP lfdoor_s;
		STAMP rtdoor_s;
	} ship_win_info[MAX_BUILT_SHIPS], *pship_win_info;

	num_ships = 1;
	pship_win_info = &ship_win_info[0];
	if (YankedStarShipPtr)
	{
		pship_win_info->StarShipPtr = YankedStarShipPtr;

		pship_win_info->lfdoor_s.origin.x = -(SHIP_WIN_WIDTH >> 1);
		pship_win_info->rtdoor_s.origin.x = (SHIP_WIN_WIDTH >> 1);
		pship_win_info->lfdoor_s.origin.y = 0;
		pship_win_info->rtdoor_s.origin.y = 0;
		pship_win_info->lfdoor_s.frame = IncFrameIndex(pMS->ModuleFrame);
		pship_win_info->rtdoor_s.frame =
			IncFrameIndex(pship_win_info->lfdoor_s.frame);

		pship_win_info->ship_s.origin.x = (SHIP_WIN_WIDTH >> 1) + 1;
		pship_win_info->ship_s.origin.y = (SHIP_WIN_WIDTH >> 1);
		pship_win_info->ship_s.frame = YankedStarShipPtr->melee_icon;

		pship_win_info->finished_s.x = hangar_x_coords[which_window % HANGAR_SHIPS_ROW];
		pship_win_info->finished_s.y = HANGAR_Y + (HANGAR_DY * (which_window / HANGAR_SHIPS_ROW));
	}
	else
	{
		if (which_window == (uint16_t)~0)
		{
			hStarShip = GetHeadLink(&GLOBAL(built_ship_q));
			num_ships = CountLinks(&GLOBAL(built_ship_q));
		}
		else
		{
			HSHIPFRAG hTailShip;

			hTailShip = GetTailLink(&GLOBAL(built_ship_q));
			RemoveQueue(&GLOBAL(built_ship_q), hTailShip);

			hStarShip = GetHeadLink(&GLOBAL(built_ship_q));
			while (hStarShip)
			{
				StarShipPtr =
					LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
				if (StarShipPtr->index > which_window)
				{
					UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
					break;
				}
				hNextShip = _GetSuccLink(StarShipPtr);
				UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);

				hStarShip = hNextShip;
			}
			InsertQueue(&GLOBAL(built_ship_q), hTailShip, hStarShip);

			hStarShip = hTailShip;
			StarShipPtr = LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
			StarShipPtr->index = which_window;
			UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
		}

		for (i = 0; i < num_ships; ++i)
		{
			StarShipPtr = LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
			hNextShip = _GetSuccLink(StarShipPtr);

			pship_win_info->StarShipPtr = StarShipPtr;
			// XXX BUG: this looks wrong according to the original
			// semantics of LockShipFrag(): StarShipPtr is not
			// valid anymore after UnlockShipFrag() is called,
			// but it is used thereafter.

			pship_win_info->lfdoor_s.origin.x = -RES_SCALE(1);
			pship_win_info->rtdoor_s.origin.x = RES_SCALE(1);
			pship_win_info->lfdoor_s.origin.y = 0;
			pship_win_info->rtdoor_s.origin.y = 0;
			pship_win_info->lfdoor_s.frame =
				IncFrameIndex(pMS->ModuleFrame);
			pship_win_info->rtdoor_s.frame =
				IncFrameIndex(pship_win_info->lfdoor_s.frame);

			pship_win_info->ship_s.origin.x = (SHIP_WIN_WIDTH >> 1) + 1;
			pship_win_info->ship_s.origin.y = (SHIP_WIN_WIDTH >> 1);
			pship_win_info->ship_s.frame = StarShipPtr->melee_icon;

			which_window = StarShipPtr->index;
			pship_win_info->finished_s.x = hangar_x_coords[which_window % HANGAR_SHIPS_ROW];
			pship_win_info->finished_s.y = HANGAR_Y + (HANGAR_DY * (which_window / HANGAR_SHIPS_ROW));
			++pship_win_info;

			UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
			hStarShip = hNextShip;
		}
	}

	if (num_ships)
	{
		bool AllDoorsFinished;
		uint32_t TimeIn;
		GFXRECT r;
		GFXCONTEXT OldContext;
		GFXRECT OldClipRect;
		int j;

		AllDoorsFinished = false;
		r.corner.x = r.corner.y = 0;
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = SHIP_WIN_HEIGHT;
		FlushInput();
		TimeIn = GetTimeCounter();

		for (j = 0; (j < SHIP_WIN_FRAMES) && !AllDoorsFinished; j++)
		{
			SleepThreadUntil(TimeIn + GameTicksPerSecond / RES_SCALE(24));
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
			SetContextBackGroundColor(BLACK_COLOR);

			BatchGraphics();
			pship_win_info = &ship_win_info[0];
			for (i = 0; i < num_ships; ++i)
			{
				{
					GFXRECT ClipRect;

					ClipRect.corner.x = SIS_ORG_X + pship_win_info->finished_s.x;
					ClipRect.corner.y = SIS_ORG_Y + pship_win_info->finished_s.y;
					ClipRect.extent.width = SHIP_WIN_WIDTH;
					ClipRect.extent.height = SHIP_WIN_HEIGHT;
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

static void
CrewTransaction(int16_t crew_delta)
{
	if (crew_delta)
	{
		int16_t crew_bought;

		crew_bought = (int16_t)uqm::makeWord(
						  GET_GAME_STATE(CREW_PURCHASED0),
						  GET_GAME_STATE(CREW_PURCHASED1))
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
			if (crew_bought >= CREW_EXPENSE_THRESHOLD
				&& crew_bought - crew_delta < CREW_EXPENSE_THRESHOLD)
			{
				GLOBAL(CrewCost) += 2;
				DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
			}
		}
		else
		{
			if (crew_bought < CREW_EXPENSE_THRESHOLD
				&& crew_bought - crew_delta >= CREW_EXPENSE_THRESHOLD)
			{
				GLOBAL(CrewCost) -= 2;
				DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
			}
		}
		if (CheckAlliance(SHOFIXTI_SHIP) != GOOD_GUY)
		{
			SET_GAME_STATE(CREW_PURCHASED0, uqm::lowByte(crew_bought));
			SET_GAME_STATE(CREW_PURCHASED1, uqm::highByte(crew_bought));

			showRemainingCrew();
		}
	}
}

static void
DMS_FlashFlagShip(void)
{
	GFXRECT r;
	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = SIS_SCREEN_WIDTH;
	if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
	{
		r.extent.height = RES_SCALE(63) - SAFE_NUM_SCL(2);
	}
	else
	{
		r.extent.height = RES_SCALE(74) - DOS_NUM_SCL(9) - SAFE_NEG(3);
	}
	SetFlashRect(&r, uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
}

static void
DMS_GetEscortShipRect(GFXRECT* rOut, uint8_t slotNr)
{
	uint8_t row = slotNr / HANGAR_SHIPS_ROW;
	uint8_t col = slotNr % HANGAR_SHIPS_ROW;

	rOut->corner.x = hangar_x_coords[col];
	rOut->corner.y = HANGAR_Y + (HANGAR_DY * row);
	rOut->extent.width = SHIP_WIN_WIDTH;
	rOut->extent.height = SHIP_WIN_HEIGHT;
}

static void
DMS_FlashEscortShip(uint8_t slotNr)
{
	GFXRECT r;
	DMS_GetEscortShipRect(&r, slotNr);
	SetFlashRect(&r, uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
}

static void
DMS_FlashFlagShipCrewCount(void)
{
	GFXRECT r;
	SetContext(StatusContext);
	GetGaugeRect(&r, true);
	SetFlashRect(&r, false);
	SetContext(SpaceContext);
}

static void
DMS_FlashEscortShipCrewCount(uint8_t slotNr)
{
	GFXRECT r;
	uint8_t row = slotNr / HANGAR_SHIPS_ROW;
	uint8_t col = slotNr % HANGAR_SHIPS_ROW;

	r.corner.x = hangar_x_coords[col];
	r.corner.y = (HANGAR_Y + (HANGAR_DY * row))
			   + (SHIP_WIN_HEIGHT - RES_SCALE(6));
	r.extent.width = SHIP_WIN_WIDTH;
	r.extent.height = RES_SCALE(5);

	SetContext(SpaceContext);
	SetFlashRect(&r, false);
}

// Helper function for DoModifyShips(). Called to change the flash
// rectangle to the currently selected ship (flagship or escort ship).
static void
DMS_FlashActiveShip(MENU_STATE* pMS)
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
static void
DMS_SetMode(MENU_STATE* pMS, DMS_Mode mode)
{
	switch (mode)
	{
		case DMS_Mode_navigate:
			SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			DMS_FlashActiveShip(pMS);
			break;
		case DMS_Mode_addEscort:
			SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			SetFlashRect(DOS_BOOL(SFR_MENU_3DO, SFR_MENU_NON), false);
			break;
		case DMS_Mode_editCrew:
			SetMenuSounds(MENU_SOUND_ARROWS,
						  MENU_SOUND_PAGE | MENU_SOUND_ACTION);
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
		case DMS_Mode_exit:
			SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			SetFlashRect(SFR_MENU_3DO, false);
			break;
	}
}

#define MODIFY_CREW_FLAG (1 << 8)
// Helper function for DoModifyShips(), called when the player presses the
// special button.
// It works both when the cursor is over an escort ship, while not editing
// the crew, and when a new ship is added.
// hStarShip is the ship in the slot under the cursor
// (or 0 if no such ship).
static bool
DMS_SpinShip(MENU_STATE* pMS, HSHIPFRAG hStarShip)
{
	HFLEETINFO hSpinShip = 0;
	GFXCONTEXT OldContext;
	GFXRECT OldClipRect;

	// No spinning the flagship.
	if (uqm::highNibble(pMS->CurState) != 0)
	{
		return false;
	}

	// We must either be hovering over a used ship slot, or adding a new
	// ship to the fleet.
	if ((hStarShip == 0) == !(pMS->delta_item & MODIFY_CREW_FLAG))
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
			&GLOBAL(stowed_ship_q), uqm::lowByte(pMS->delta_item));
		SHIP_FRAGMENT* FragPtr = LockShipFrag(
			&GLOBAL(stowed_ship_q), hStowedShip);
		hSpinShip = GetStarShipFromIndex(
			&GLOBAL(avail_race_q), FragPtr->race_id);
		UnlockShipFrag(&GLOBAL(stowed_ship_q), hStowedShip);
	}
	else
	{
		// Hovering over an escort ship.
		SHIP_FRAGMENT* FragPtr = LockShipFrag(
			&GLOBAL(built_ship_q), hStarShip);
		hSpinShip = GetStarShipFromIndex(
			&GLOBAL(avail_race_q), FragPtr->race_id);
		UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
	}

	SetFlashRect(nullptr, false);

	OldContext = SetContext(ScreenContext);
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
static int16_t
DMS_HireFlagShipCrew(void)
{
	GFXRECT r;
	int16_t crew_bought;

	crew_bought = (int16_t)uqm::makeWord(
		GET_GAME_STATE(CREW_PURCHASED0),
		GET_GAME_STATE(CREW_PURCHASED1));

	if (GetCPodCapacity(&r.corner) <= GetCrewCount())
	{
		// At capacity.
		return 0;
	}

	if (GLOBAL_SIS(ResUnits) < (uint32_t)GLOBAL(CrewCost))
	{
		// Not enough RUs.
		return 0;
	}

	if (isDifficulty(uqm::Difficulty::Hard) && crew_bought >= 2000
		&& CheckAlliance(SHOFIXTI_SHIP) != GOOD_GUY)
	{
		// Ran out of StarBase crew
		return 0;
	}

	// Draw a crew member.
	// Crew dots/rectangles for Original and HD graphics.
	r.extent.width = RES_SCALE(1);
	r.extent.height = r.extent.width;
	DrawFilledRectangle(&r);

	// Update the crew counter and RU. Note that the crew counter is
	// flashing.
	PreUpdateFlashRect();
	DeltaSISGauges(1, 0, -GLOBAL(CrewCost));
	PostUpdateFlashRect();

	return 1;
}

// Helper function for DoModifyShips(), called when the player presses the
// down button when modifying the crew of the flagship.
// Dismiss crew from the flagship.
// Returns the change in crew (-1 on success, 0 on failure).
static int16_t
DMS_DismissFlagShipCrew(void)
{
	int16_t crew_bought;
	GFXRECT r;

	if (GetCrewCount() == 0)
	{
		// No crew to dismiss.
		return 0;
	}

	crew_bought = (int16_t)uqm::makeWord(
		GET_GAME_STATE(CREW_PURCHASED0),
		GET_GAME_STATE(CREW_PURCHASED1));

	// Update the crew counter and RU. Note that the crew counter is
	// flashing.
	PreUpdateFlashRect();
	DeltaSISGauges(-1, 0, GLOBAL(CrewCost) - (crew_bought == CREW_EXPENSE_THRESHOLD ? 2 : 0));
	PostUpdateFlashRect();

	// Remove the pixel representing the crew member.
	GetCPodCapacity(&r.corner);
	r.extent.width = RES_SCALE(1);
	r.extent.height = r.extent.width;
	SetContextForeGroundColor(BLACK_COLOR);
	DrawFilledRectangle(&r);

	return -1;
}

// Helper function for DoModifyShips(), called when the player presses the
// up button when modifying the crew of an escort ship.
// Buy crew for an escort ship
// Returns the change in crew (1 on success, 0 on failure).
static int16_t
DMS_HireEscortShipCrew(SHIP_FRAGMENT* StarShipPtr)
{
	uint16_t templateMaxCrew;
	GFXRECT r;
	int16_t crew_bought;
	uint16_t crew_level = (StarShipPtr->crew_level > StarShipPtr->max_crew ? StarShipPtr->crew_level - StarShipPtr->max_crew : StarShipPtr->crew_level);

	crew_bought = (int16_t)uqm::makeWord(
		GET_GAME_STATE(CREW_PURCHASED0),
		GET_GAME_STATE(CREW_PURCHASED1));

	{
		// XXX Split this off into a separate function?
		HFLEETINFO hTemplate = GetStarShipFromIndex(
			&GLOBAL(avail_race_q),
			StarShipPtr->race_id);
		FLEET_INFO* TemplatePtr =
			LockFleetInfo(&GLOBAL(avail_race_q), hTemplate);
		templateMaxCrew = (EXTENDED && !isDifficulty(uqm::Difficulty::Hard)) ?
							  TemplatePtr->max_crew :
							  TemplatePtr->crew_level;
		UnlockFleetInfo(&GLOBAL(avail_race_q), hTemplate);
	}

	if (GLOBAL_SIS(ResUnits) < (uint32_t)GLOBAL(CrewCost))
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
		&& CheckAlliance(SHOFIXTI_SHIP) != GOOD_GUY)
	{
		// Ran out of StarBase crew
		return 0;
	}

	if (crew_level > 0)
	{
		DeltaSISGauges(0, 0, -GLOBAL(CrewCost));
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
static int16_t
DMS_DismissEscortShipCrew(SHIP_FRAGMENT* StarShipPtr)
{
	int16_t crew_delta = 0;
	GFXRECT r;
	uint16_t crew_level = (StarShipPtr->crew_level > StarShipPtr->max_crew ? StarShipPtr->crew_level - StarShipPtr->max_crew : StarShipPtr->crew_level);

	if (crew_level > 0)
	{
		if (crew_level > 1)
		{
			// The ship was not at 'scrap'.
			// Give one crew member worth of RU.
			int16_t crew_bought = (int16_t)uqm::makeWord(
				GET_GAME_STATE(CREW_PURCHASED0),
				GET_GAME_STATE(CREW_PURCHASED1));

			DeltaSISGauges(0, 0, GLOBAL(CrewCost) - (crew_bought == CREW_EXPENSE_THRESHOLD ? 2 : 0));
		}
		else
		{
			// With the last crew member, the ship will be scrapped.
			// Give RU for the ship.
			HFLEETINFO hTemplate = GetStarShipFromIndex(
				&GLOBAL(avail_race_q), StarShipPtr->race_id);
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
		PlayMenuSound(MENU_SOUND_FAILURE);
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
static void
DMS_ModifyCrew(MENU_STATE* pMS, HSHIPFRAG hStarShip, int8_t dy)
{
	int16_t crew_delta = 0;
	SHIP_FRAGMENT* StarShipPtr = nullptr;
	uint16_t loop;
	uint16_t DoLoop = 1;
	GFXRECT r;
	//int16_t remaining_crew = INITIAL_CREW - (int16_t)MAKE_WORD (
	//		GET_GAME_STATE (CREW_PURCHASED0),
	//		GET_GAME_STATE (CREW_PURCHASED1)); Unused

	DoLoop = abs(dy);

	if (abs(dy) == 50)
	{
		PlayMenuSound(MENU_SOUND_INVOKED);
	}

	if (hStarShip)
	{
		StarShipPtr = LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
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
		uint16_t crew_level = (StarShipPtr->crew_level > StarShipPtr->max_crew ? StarShipPtr->crew_level - StarShipPtr->max_crew : StarShipPtr->crew_level);
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
		PlayMenuSound(MENU_SOUND_FAILURE);
	}

	if (hStarShip)
	{
		UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);

		// Clear out the bought ship index so that flash rects work
		// correctly.
		pMS->delta_item &= MODIFY_CREW_FLAG;
	}
}

// Helper function for DoModifyShips(), called when the player presses the
// select button when the cursor is over an empty escort ship slot.
// Try to add the currently selected ship as an escort ship.
static void
DMS_TryAddEscortShip(MENU_STATE* pMS)
{
	HFLEETINFO hFleet = GetAvailableRaceFromIndex(
		uqm::lowByte(pMS->delta_item));

	uint16_t Index = GetIndexFromStarShip(&GLOBAL(avail_race_q), hFleet);
	uint16_t shipCost = ShipCost(Index);

	if (CanBuyPoints(hFleet) && GLOBAL_SIS(ResUnits) >= (uint32_t)shipCost
		&& CloneShipFragment((RACE_ID)Index, &GLOBAL(built_ship_q), 1))
	{
		ShowCombatShip(pMS, pMS->CurState, nullptr);
		// Reset flash rectangle
		DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
		DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
		// twice to reset menu selection

		DeltaSISGauges(UNDEFINED_DELTA, UNDEFINED_DELTA, -(int)shipCost);
		DMS_SetMode(pMS, DMS_Mode_editCrew);

		showRemainingPoints(0);
	}
	else
	{
		// not enough RUs to build, cloning the ship failed,
		// or fleet point limit reached
		PlayMenuSound(MENU_SOUND_FAILURE);
	}
}

// Helper function for DoModifyShips(), called when the player presses the
// select button when the cursor is over an empty escort ship slot.
// Try to moving the currently selected ship from storage as an escort ship.
static void
DMS_TryUnstowEscortShip(MENU_STATE* pMS)
{
	HSHIPFRAG hStarShip;
	HSHIPFRAG hStowShip = GetStarShipFromIndex(&GLOBAL(stowed_ship_q),
											   uqm::lowByte(pMS->delta_item));
	SHIP_FRAGMENT* StowShipPtr =
		LockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
	uint16_t Index = StowShipPtr->race_id;
	HFLEETINFO hFleet = GetStarShipFromIndex(&GLOBAL(avail_race_q), Index);

	if (CanBuyPoints(hFleet) && (hStarShip = CloneShipFragment((RACE_ID)Index, &GLOBAL(built_ship_q), StowShipPtr->crew_level)))
	{
		SHIP_FRAGMENT* StarShipPtr =
			LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
		StarShipPtr->captains_name_index = StowShipPtr->captains_name_index;
		UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);

		UnlockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
		RemoveQueue(&GLOBAL(stowed_ship_q), hStowShip);
		FreeShipFrag(&GLOBAL(stowed_ship_q), hStowShip);

		ShowCombatShip(pMS, pMS->CurState, nullptr);
		// Reset flash rectangle
		DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
		DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
		// twice to reset menu selection

		DeltaSISGauges(UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);

		if (DOS_MENU)
		{
			// Regenerate the ship list
			memset(&ShipState, 0, sizeof ShipState);
			InventoryShips(&ShipState);
		}

		DMS_SetMode(pMS, DMS_Mode_editCrew);

		showRemainingPoints(0);
		stowCount = GetStowedShipCount();
		currentStowShip = uqm::lowByte(pMS->delta_item);
		stowed_q &= (stowCount > 0);
	}
	else
	{
		// Cloning the ship failed or fleet point limit reached
		PlayMenuSound(MENU_SOUND_FAILURE);
		UnlockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
	}
}

// Helper function for DoModifyShips(), called when the player is in the
// mode to add a new escort ship to the fleet (after pressing select on an
// empty slot).
// lowByte (pMS->delta_item) is used to store the currently highlighted
// ship. Returns false if the flash rectangle needs to be updated.
static void
DMS_AddEscortShip(MENU_STATE* pMS, bool special, bool select,
				  bool cancel, int8_t dx, int8_t dy)
{
	assert(pMS->delta_item & MODIFY_CREW_FLAG);

	uint8_t currentShip = uqm::lowByte(pMS->delta_item);

	if (special)
	{ // JSD: I don't think we can use delta_item, but SpinShip should be ok
		//if (DMS_SpinShip (pMS, GetEscortByStarShipIndex (pMS->delta_item)))
		if (DMS_SpinShip(pMS, nullptr))
		{
			DMS_SetMode(pMS, DMS_Mode_addEscort);
		}
		return;
	}

	if (cancel)
	{
		// Cancel selecting an escort ship.
		pMS->delta_item &= ~MODIFY_CREW_FLAG;
		SetFlashRect(nullptr, false);

		DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
		DrawMenuStateStrings(PM_CREW, SHIPYARD_CREW);
		// twice to reset menu selection
		DMS_SetMode(pMS, DMS_Mode_navigate);

		if (DOS_MENU)
		{
			DeltaSISGauges(UNDEFINED_DELTA, UNDEFINED_DELTA,
						   UNDEFINED_DELTA);
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
	else if (dy && STORAGE_Q)
	{
		// "dy" switches between stowed ships and buying.
		if (stowCount == 0)
		{
			PlayMenuSound(MENU_SOUND_FAILURE);
		}
		else
		{
			stowed_q = !stowed_q;
			currentShip = 0;
			PreUpdateFlashRect();
			DrawRaceStrings(currentShip);
			PostUpdateFlashRect();
			pMS->delta_item = currentShip | MODIFY_CREW_FLAG;
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
				if (DOS_MENU && dx != -1)
				{
					currentShip = currentShip - currentShip % MAX_VIS_MODULES;
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
			pMS->delta_item = currentShip | MODIFY_CREW_FLAG;
		}
	}
}

// Helper function for DoModifyShips(), called when the player presses
// 'select' or 'cancel' after selling all the crew.
static void
DMS_ScrapEscortShip(MENU_STATE* pMS, HSHIPFRAG hStarShip)
{
	SHIP_FRAGMENT* StarShipPtr =
		LockShipFrag(&GLOBAL(built_ship_q), hStarShip);

	SetFlashRect(nullptr, false);
	ShowCombatShip(pMS, pMS->CurState, StarShipPtr);

	UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);

	RemoveQueue(&GLOBAL(built_ship_q), hStarShip);
	FreeShipFrag(&GLOBAL(built_ship_q), hStarShip);
	// refresh SIS display
	DeltaSISGauges(UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);

	showRemainingPoints(0);

	SetContext(SpaceContext);
	DMS_SetMode(pMS, DMS_Mode_navigate);
}

// Helper function for DoModifyShips(), called when the player presses
// 'select' or 'cancel' after selling all the crew.
static void
DMS_StowEscortShip(MENU_STATE* pMS, HSHIPFRAG hStarShip)
{
	HSHIPFRAG hStowShip;
	SHIP_FRAGMENT* StarShipPtr =
		LockShipFrag(&GLOBAL(built_ship_q), hStarShip);

	SetFlashRect(nullptr, false);
	ShowCombatShip(pMS, pMS->CurState, StarShipPtr);

	uint16_t Index = StarShipPtr->race_id;
	if ((hStowShip = CloneShipFragment((RACE_ID)Index, &GLOBAL(stowed_ship_q),
									   StarShipPtr->crew_level)))
	{
		SHIP_FRAGMENT* StowShipPtr =
			LockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
		StowShipPtr->captains_name_index = StarShipPtr->captains_name_index;
		UnlockShipFrag(&GLOBAL(stowed_ship_q), hStowShip);
	}
	UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);

	RemoveQueue(&GLOBAL(built_ship_q), hStarShip);
	FreeShipFrag(&GLOBAL(built_ship_q), hStarShip);

	// refresh SIS display
	DeltaSISGauges(UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);

	showRemainingPoints(0);
	stowCount = GetStowedShipCount();

	SetContext(SpaceContext);
	DMS_SetMode(pMS, DMS_Mode_navigate);
}

// Helper function for DoModifyShips(), called when the player presses
// one of the motion keys when not in crew modification mode.
static uint8_t
DMS_MoveCursor(uint8_t curState, int8_t dx, int8_t dy)
{
	uint8_t row = uqm::lowNibble(curState) / HANGAR_SHIPS_ROW;
	uint8_t col = uqm::lowNibble(curState) % HANGAR_SHIPS_ROW;
	bool isFlagShipSelected = (uqm::highNibble(curState) != 0);

	if (dy)
	{
		// Vertical motion.

		// We consider the flagship an extra row (on the bottom),
		// to ease operations.
		if (isFlagShipSelected)
		{
			row = HANGAR_ROWS;
		}

		// Move up/down, wrapping around:
		row = (row + (HANGAR_ROWS + 1) + dy) % (HANGAR_ROWS + 1);

		// If we moved to the 'extra row', this means the flag ship.
		isFlagShipSelected = (row == HANGAR_ROWS);
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
			col = (col + HANGAR_SHIPS_ROW + dx) % HANGAR_SHIPS_ROW;
		}
	}

	return uqm::makeByte(row * HANGAR_SHIPS_ROW + col,
						 isFlagShipSelected ? 0xf : 0);
}

// Helper function for DoModifyShips(), called every time DoModifyShip() is
// called when we are in crew editing mode.
static void
DMS_EditCrewMode(MENU_STATE* pMS, HSHIPFRAG hStarShip,
				 bool select, bool cancel, int8_t dy)
{
	if (cancel)
	{
		// Leave crew editing mode.
		if (hStarShip != 0)
		{
			// Exiting crew editing mode for an escort ship.
			SHIP_FRAGMENT* StarShipPtr = LockShipFrag(
				&GLOBAL(built_ship_q), hStarShip);
			uint16_t crew_level = StarShipPtr->crew_level;
			uint16_t max_crew = StarShipPtr->max_crew;
			if (stowCount == MAX_STOWED_SHIPS && crew_level > max_crew)
			{
				PlayMenuSound(MENU_SOUND_FAILURE);
				UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
				return;
			}
			if (crew_level > max_crew)
			{
				// This happens when it's been forced by stowing
				// set crew back to proper level before it happens
				StarShipPtr->crew_level -= StarShipPtr->max_crew;
			}

			UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);

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
				if (DOS_MENU)
				{
					// Regenerate the ship list
					memset(&ShipState, 0, sizeof ShipState);
					InventoryShips(&ShipState);
				}
			}
		}

		pMS->delta_item &= ~MODIFY_CREW_FLAG;
		DMS_SetMode(pMS, DMS_Mode_navigate);
	}
	else if (dy)
	{
		// Hire or dismiss crew for the flagship or an escort
		// ship.
		DMS_ModifyCrew(pMS, hStarShip, dy);
	}
	else if (select && hStarShip)
	{
		SHIP_FRAGMENT* StarShipPtr = LockShipFrag(
			&GLOBAL(built_ship_q), hStarShip);
		HFLEETINFO hTemplate = GetStarShipFromIndex(
			&GLOBAL(avail_race_q), StarShipPtr->race_id);

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

		GFXRECT r;
		PreUpdateFlashRect();
		DMS_GetEscortShipRect(&r, StarShipPtr->index);
		ShowShipCrew(StarShipPtr, &r);
		PostUpdateFlashRect();

		UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
	}
}

// Helper function for DoModifyShips(), called every time DoModifyShip() is
// called when we are in the mode where you can select a ship or empty
// slot.
static void
DMS_NavigateShipSlots(MENU_STATE* pMS, bool special, bool select,
					  bool cancel, int8_t dx, int8_t dy)
{
	HSHIPFRAG hStarShip = GetEscortByStarShipIndex(pMS->CurState);

	if (dx || dy)
	{
		// Moving through the ship slots.
		uint8_t NewState = DMS_MoveCursor(pMS->CurState, dx, dy);
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
			DMS_SetMode(pMS, DMS_Mode_navigate);
		}
	}
	else if (select)
	{
		if (hStarShip == 0 && uqm::highNibble(pMS->CurState) == 0)
		{
			// Select button was pressed over an empty escort
			// ship slot. Switch to 'add escort ship' mode.
			if (!STORAGE_Q || !stowCount)
			{
				stowed_q = false;
			}
			else if (currentStowShip >= stowCount)
			{
				currentStowShip = stowCount - 1;
			}
			pMS->delta_item = MODIFY_CREW_FLAG | (stowed_q ? currentStowShip : 0);
			DrawRaceStrings(uqm::lowByte(pMS->delta_item));
			DMS_SetMode(pMS, DMS_Mode_addEscort);
		}
		else
		{
			// Select button was pressed over an escort ship or
			// the flagship. Entering crew editing mode
			pMS->delta_item |= MODIFY_CREW_FLAG;
			DMS_SetMode(pMS, DMS_Mode_editCrew);
		}
	}
	else if (cancel)
	{
		// Leave escort ship editor.
		pMS->InputFunc = DoShipyard;
		pMS->CurState = SHIPYARD_CREW;
		DrawMenuStateStrings(PM_CREW, pMS->CurState);
		DMS_SetMode(pMS, DMS_Mode_exit);
	}
}

/* In this routine, the least significant byte of pMS->CurState is used
 * to store the current selected ship index
 * a special case for the row is hi-nibble == -1 (0xf), which specifies
 * SIS as the selected ship
 * some bitwise math is still done to scroll through ships, for it to work
 * ships per row number must divide 0xf0 without remainder
 */
static bool
DoModifyShips(MENU_STATE* pMS)
{
	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
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
		DMS_SetMode(pMS, DMS_Mode_navigate);
		stowCount = GetStowedShipCount();
		availableCount = GetAvailableRaceCount();
	}
	else
	{
		bool special = (PulsedInputState.menu[KEY_MENU_SPECIAL] != 0);
		bool select = (PulsedInputState.menu[KEY_MENU_SELECT] != 0);
		bool cancel = (PulsedInputState.menu[KEY_MENU_CANCEL] != 0);
		int8_t dx = 0;
		int8_t dy = 0;


		if (!(pMS->delta_item & MODIFY_CREW_FLAG))
		{
			// Navigating through the ship slots.
			if (PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				dx = 1;
			}
			if (PulsedInputState.menu[KEY_MENU_LEFT])
			{
				dx = -1;
			}
			if (PulsedInputState.menu[KEY_MENU_UP])
			{
				dy = -1;
			}
			if (PulsedInputState.menu[KEY_MENU_DOWN])
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
				if (PulsedInputState.menu[KEY_MENU_RIGHT])
				{
					dx = 1;
				}
				if (PulsedInputState.menu[KEY_MENU_LEFT])
				{
					dx = -1;
				}
				if (PulsedInputState.menu[KEY_MENU_UP])
				{
					dx = -1;
				}
				if (PulsedInputState.menu[KEY_MENU_DOWN])
				{
					dx = 1;
				}
				if (PulsedInputState.menu[KEY_MENU_ZOOM_IN])
				{
					dx = 5;
				}
				if (PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
				{
					dx = -5;
				}
				if (PulsedInputState.menu[KEY_MENU_NEXT])
				{
					dy = 1;
				}

				DMS_AddEscortShip(pMS, special, select, cancel, dx, dy);
			}
			else
			{
				// Crew editing mode.
				if (PulsedInputState.menu[KEY_MENU_UP])
				{
					dy = -1;
				}
				if (PulsedInputState.menu[KEY_MENU_DOWN])
				{
					dy = 1;
				}
				if (PulsedInputState.menu[KEY_MENU_RIGHT])
				{
					dy = -10;
				}
				if (PulsedInputState.menu[KEY_MENU_LEFT])
				{
					dy = 10;
				}
				if (PulsedInputState.menu[KEY_MENU_ZOOM_IN])
				{
					dy = -50;
				}
				if (PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
				{
					dy = 50;
				}
				special = false;
				if (PulsedInputState.menu[KEY_MENU_NEXT])
				{
					special = true;
				}

				// Special prepares / unprepares stowing, select = cancel
				DMS_EditCrewMode(pMS, hStarShip, special && STORAGE_Q,
								 select || cancel, dy);
			}
		}
	}

	SleepThread(GameTicksPerSecond / 60);
	// Kruzen: was 30, upped to 60 to fit new HD
	// powerline animation. No issues detected so far

	return true;
}

static void
DrawBluePrint(MENU_STATE* pMS)
{
	uint16_t num_frames;
	STAMP s;
	FRAME ModuleFrame;

	ModuleFrame = CaptureDrawable(LoadGraphic(SISBLU_MASK_ANIM));

	s.origin.x = -SAFE_PAD;
	s.origin.y = 0;
	s.frame = DecFrameIndex(ModuleFrame);
	SetContextForeGroundColor(BLUEPRINT_COLOR);
	DrawFilledStamp(&s);

	for (num_frames = 0; num_frames < NUM_DRIVE_SLOTS; ++num_frames)
	{
		DrawShipPiece(ModuleFrame, GLOBAL_SIS(DriveSlots[num_frames]),
					  num_frames, true);
	}
	for (num_frames = 0; num_frames < NUM_JET_SLOTS; ++num_frames)
	{
		DrawShipPiece(ModuleFrame, GLOBAL_SIS(JetSlots[num_frames]),
					  num_frames, true);
	}
	for (num_frames = 0; num_frames < NUM_MODULE_SLOTS; ++num_frames)
	{
		uint8_t which_piece;

		which_piece = GLOBAL_SIS(ModuleSlots[num_frames]);

		if (!(pMS->CurState == SHIPYARD
			  && (which_piece == CREW_POD || which_piece == STORAGE_BAY)))
		{
			DrawShipPiece(ModuleFrame, which_piece, num_frames, true);
		}
	}

	SetContextForeGroundColor(
		BUILD_COLOR(MAKE_RGB15(0x0A, 0x0A, 0x1F), 0x09));
	for (num_frames = 0; num_frames < NUM_MODULE_SLOTS; ++num_frames)
	{
		uint8_t which_piece;

		which_piece = GLOBAL_SIS(ModuleSlots[num_frames]);
		if (pMS->CurState == SHIPYARD
			&& (which_piece == CREW_POD || which_piece == STORAGE_BAY))
		{
			DrawShipPiece(ModuleFrame, which_piece, num_frames, true);
		}
	}

	{
		num_frames = GLOBAL_SIS(CrewEnlisted);
		GLOBAL_SIS(CrewEnlisted) = 0;

		while (num_frames--)
		{
			GFXRECT r;
			// Crew dots
			r.extent.width = RES_SCALE(1);
			r.extent.height = r.extent.width;

			GetCPodCapacity(&r.corner);
			DrawFilledRectangle(&r);

			++GLOBAL_SIS(CrewEnlisted);
		}
	}
	{
		GFXRECT r;

		num_frames = GLOBAL_SIS(TotalElementMass);
		GLOBAL_SIS(TotalElementMass) = 0;

		r.extent.width = RES_SCALE(9);
		r.extent.height = RES_SCALE(1);
		while (num_frames)
		{
			uint16_t m;

			m = num_frames < SBAY_MASS_PER_ROW ?
					num_frames :
					SBAY_MASS_PER_ROW;
			GLOBAL_SIS(TotalElementMass) += m;
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

bool DoShipyard(MENU_STATE* pMS)
{
	bool select, cancel;
	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		goto ExitShipyard;
	}

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];

	OutfitOrShipyard = 3;

	FillHangarX();

	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoShipyard;
		stowCount = GetStowedShipCount();
		availableCount = GetAvailableRaceCount();

		if (DOS_MENU)
		{
			memset(&ShipState, 0, sizeof ShipState);
			InventoryShips(&ShipState);
		}

		if (!IS_DOS)
		{
			ModuleFont = LoadFont(MODULE_FONT);
		}

		{
			STAMP s;
			GFXRECT r, old_r;

			pMS->ModuleFrame = CaptureDrawable(
				LoadGraphic(SHIPYARD_PMAP_ANIM));

			pMS->CurString = CaptureColorMap(
				LoadColorMap(HANGAR_COLOR_TAB));

			pMS->hMusic = LoadMusic(SHIPYARD_MUSIC);

			SetTransitionSource(nullptr);
			BatchGraphics();
			DrawSISFrame();
			DrawSISMessage(GAME_STRING(STARBASE_STRING_BASE + 3));
			DrawSISTitle(GAME_STRING(STARBASE_STRING_BASE));
			SetContext(SpaceContext);
			DrawBluePrint(pMS);

			pMS->CurState = SHIPYARD_CREW;
			DrawMenuStateStrings(PM_CREW, pMS->CurState);

			SetContext(SpaceContext);
			s.origin.x = 0;
			s.origin.y = 0;
			s.frame = SetAbsFrameIndex(pMS->ModuleFrame, 0);

			if (!(IS_PAD || classicPackPresent))
			{
				// PC hangar
				// the PC ship dock needs to overwrite the border
				// expand the clipping rect by 1 pixel
				GetContextClipRect(&old_r);
				r = old_r;
				r.corner.x -= RES_SCALE(1);
				r.extent.width += RES_SCALE(2);
				r.extent.height += RES_SCALE(1);

				SetContextClipRect(&r);
				DrawStamp(&s);

				if (uqm::UQMOptions::read().customBorder)
				{
					DrawBorder(SIS_REPAIR_FRAME);
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

			PlayMusicResume(pMS->hMusic, NORMAL_VOLUME);

			ShowCombatShip(pMS, (uint16_t)~0, nullptr);

			SetInputCallback(on_input_frame);

			SetFlashRect(SFR_MENU_3DO, false);
		}

		pMS->Initialized = true;
	}
	else if (cancel || (select && pMS->CurState == SHIPYARD_EXIT))
	{
ExitShipyard:
		SetInputCallback(nullptr);


		if (pMS->CurState < SHIPYARD_EXIT)
		{
			DrawMenuStateStrings(PM_CREW, SHIPYARD_EXIT);
		}

		DestroyDrawable(ReleaseDrawable(pMS->ModuleFrame));
		pMS->ModuleFrame = 0;
		DestroyColorMap(ReleaseColorMap(pMS->CurString));
		pMS->CurString = 0;

		// Release Fonts
		if (!IS_DOS)
		{
			DestroyFont(ModuleFont);
		}

		SetMusicPosition();

		return false;
	}
	else if (select)
	{
		if (pMS->CurState != SHIPYARD_SAVELOAD)
		{
			pMS->Initialized = false;
			DrawMenuStateStrings(PM_CREW, pMS->CurState);
			DoModifyShips(pMS);
		}
		else
		{
			// Clearing FlashRect is not necessary
			if (!GameOptions())
			{
				goto ExitShipyard;
			}
			DrawMenuStateStrings(PM_CREW, pMS->CurState);
			SetFlashRect(SFR_MENU_3DO, false);
		}
	}
	else
	{
		DoMenuChooser(pMS, PM_CREW);
	}

	if (uqm::UQMOptions::read().infiniteRU)
	{
		GLOBAL_SIS(ResUnits) = 1000000L;
	}

	return true;
}