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
#include "options.h"
#include "races.h"
#include "master.h"
#include "sis.h"
#include "setup.h"
#include "core/string/StringUtils.h"
#include "core/misc/bittricks.h"
#include "libs/mathlib.h"
#include "planets/planets.h"
#include "starbase.h"
#include "starmap.h"
#include "gendef.h"
#include "save.h"
#include <stdlib.h>
#include "uqm/random/UQMRandom.h"


// Allocate a new STARSHIP or SHIP_FRAGMENT and put it in the queue
HLINK Build(QUEUE* pQueue, SPECIES_ID SpeciesID)
{
	assert(GetLinkSize(pQueue) == sizeof(STARSHIP) || GetLinkSize(pQueue) == sizeof(SHIP_FRAGMENT));

	HLINK hNewShip {AllocLink(pQueue)};
	if (!hNewShip)
	{
		return 0;
	}

	SHIP_BASE* ShipPtr {(SHIP_BASE*)LockLink(pQueue, hNewShip)};
	memset(ShipPtr, 0, GetLinkSize(pQueue));
	ShipPtr->SpeciesID = SpeciesID;

	UnlockLink(pQueue, hNewShip);
	PutQueue(pQueue, hNewShip);

	return hNewShip;
}

HLINK GetStarShipFromIndex(QUEUE* pShipQ, uint16_t Index)
{
	HLINK hStarShip {};
	HLINK hNextShip {};

	for (hStarShip = GetHeadLink(pShipQ);
		 Index > 0 && hStarShip; hStarShip = hNextShip, --Index)
	{
		const LINK* StarShipPtr {LockLink(pShipQ, hStarShip)};
		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockLink(pShipQ, hStarShip);
	}

	return (hStarShip);
}

// Gives the first fleet in avail_race_q that builds specified ship.
HLINK GetFleetFromSpecies(SPECIES_ID id)
{
	HLINK hFleet {};
	HLINK hNextFleet {};

	for (hFleet = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hFleet; hFleet = hNextFleet)
	{
		const FLEET_INFO* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};
		if (FleetPtr->SpeciesID == id)
		{
			UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
			return hFleet;
		}
		hNextFleet = _GetSuccLink(FleetPtr);
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	}

	return hFleet;
}

// Returns the handle for the first fleet that normally builds the ships
// the fleet in Index builds after seeding.
// e.g. if Index is a fleet building Cruisers, returns the Earthling fleet.
// This is for every part of the game that assumes a RACE_ID is a SPECIES_ID
// If shipseed is not in use, it will do GetStarShipFromIndex on avail_race_q
// If this seems like overkill just remember the Yehat rebels.
HFLEETINFO GetSeededFleetFromIndex(uint16_t Index)
{
	const FLEET_INFO* TemplatePtr = nullptr;
	SPECIES_ID ship;
	const bool loadWindow = ((uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed == 0) || (!uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed != 0) || (uqm::currentSeed().value != uqm::gameState().sis().Seed));

	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, Index)};
	if (!hFleet)
	{
		return hFleet;
	}
	TemplatePtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	if (!TemplatePtr)
	{
		return nullptr;
	}
	ship = SeedShip(TemplatePtr->SpeciesID, loadWindow);
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	hFleet = GetFleetFromSpecies(ship);
	return hFleet;
}

HSHIPFRAG GetEscortByStarShipIndex(uint16_t index)
{
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};
	const SHIP_FRAGMENT* StarShipPtr {};

	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q);
		 hStarShip; hStarShip = hNextShip)
	{
		StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

		if (StarShipPtr->index == index)
		{
			UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
			break;
		}

		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}

	return hStarShip;
}

SPECIES_ID ShipIdStrToIndex(const char* shipIdStr)
{
	HMASTERSHIP hStarShip {};
	HMASTERSHIP hNextShip {};
	SPECIES_ID result = NO_ID;

	for (hStarShip = GetHeadLink(&master_q);
		 hStarShip != 0; hStarShip = hNextShip)
	{
		const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hStarShip)};
		hNextShip = _GetSuccLink(MasterPtr);

		if (strcmp(shipIdStr, MasterPtr->ShipInfo.idStr) == 0)
		{
			result = MasterPtr->SpeciesID;
			UnlockMasterShip(&master_q, hStarShip);
			break;
		}

		UnlockMasterShip(&master_q, hStarShip);
	}

	return result;
}

typedef struct
{
	const char* idStr;
	RACE_ID id;
} RaceIdMap;

// We would eventually want to unhardcode this.
static RaceIdMap raceIdMap[] = {
	// Sorted on the name, for the binary search.
	{/* .idStr = */ "androsynth",  /* .id = */ ANDROSYNTH_SHIP	 },
	{/* .idStr = */ "arilou",	  /* .id = */ ARILOU_SHIP		 },
	{/* .idStr = */ "chenjesu",	/* .id = */ CHENJESU_SHIP	 },
	{/* .idStr = */ "chmmr",		 /* .id = */ CHMMR_SHIP	   },
	{/* .idStr = */ "druuge",	  /* .id = */ DRUUGE_SHIP		 },
	{/* .idStr = */ "human",		 /* .id = */ HUMAN_SHIP	   },
	{/* .idStr = */ "ilwrath",	   /* .id = */ ILWRATH_SHIP	   },
	{/* .idStr = */ "kohrah",	  /* .id = */ BLACK_URQUAN_SHIP},
	{/* .idStr = */ "melnorme",	/* .id = */ MELNORME_SHIP	 },
	{/* .idStr = */ "mmrnmhrm",	/* .id = */ MMRNMHRM_SHIP	 },
	{/* .idStr = */ "mycon",		 /* .id = */ MYCON_SHIP	   },
	{/* .idStr = */ "orz",		   /* .id = */ ORZ_SHIP		   },
	{/* .idStr = */ "pkunk",		 /* .id = */ PKUNK_SHIP	   },
	{/* .idStr = */ "samatra",	   /* .id = */ SAMATRA_SHIP	   },
	{/* .idStr = */ "shofixti",	/* .id = */ SHOFIXTI_SHIP	 },
	{/* .idStr = */ "slylandro",	 /* .id = */ SLYLANDRO_SHIP   },
	{/* .idStr = */ "spathi",	  /* .id = */ SPATHI_SHIP		 },
	{/* .idStr = */ "supox",		 /* .id = */ SUPOX_SHIP	   },
	{/* .idStr = */ "syreen",	  /* .id = */ SYREEN_SHIP		 },
	{/* .idStr = */ "thraddash",	 /* .id = */ THRADDASH_SHIP   },
	{/* .idStr = */ "umgah",		 /* .id = */ UMGAH_SHIP	   },
	{/* .idStr = */ "urquandrone", /* .id = */ URQUAN_DRONE_SHIP},
	{/* .idStr = */ "urquan",	  /* .id = */ URQUAN_SHIP		 },
	{/* .idStr = */ "utwig",		 /* .id = */ UTWIG_SHIP	   },
	{/* .idStr = */ "vux",		   /* .id = */ VUX_SHIP		   },
	{/* .idStr = */ "yehat",		 /* .id = */ YEHAT_SHIP	   },
	{/* .idStr = */ "yehatrebel",  /* .id = */ YEHAT_REBEL_SHIP },
	{/* .idStr = */ "zoqfotpik",	 /* .id = */ ZOQFOTPIK_SHIP   },
	// Same as URQUAN_DRONE_SHIP
};

static int RaceIdCompare(const void* id1, const void* id2)
{
	return strcmp(((RaceIdMap*)id1)->idStr, ((RaceIdMap*)id2)->idStr);
}

RACE_ID RaceIdStrToIndex(const char* raceIdStr)
{
	RaceIdMap key = {/* .idStr = */ raceIdStr, /* .id = */ (RACE_ID)-1};
	const RaceIdMap* found = (RaceIdMap*)bsearch(&key, raceIdMap,
												 std::size(raceIdMap),
												 sizeof raceIdMap[0], RaceIdCompare);

	if (found == nullptr)
	{
		return (RACE_ID)-1;
	}

	return found->id;
}

/*
 * Give the player 'count' ships of the specified race,
 * limited by the number of free slots.
 * Returns the number of ships added.
 */
uint16_t AddEscortShips(RACE_ID race, int16_t count)
{
	uint16_t i {};

	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet || count <= 0)
	{
		return 0;
	}

	uint8_t which_window {0};
	for (i = 0; i < (uint16_t)count; i++)
	{
		HSHIPFRAG hStarShip {};
		HSHIPFRAG hOldShip {};
		SHIP_FRAGMENT* StarShipPtr {};

		if (!StorageQ())
		{
			hStarShip = CloneShipFragment(race, &uqm::gameState().game().built_ship_q, 0);
		}
		else if (!CanBuyPoints(hFleet) || !(hStarShip = CloneShipFragment(race, &uqm::gameState().game().built_ship_q, 0)))
		{ // If we don't have room or failed to create in built queue, stow it
			if ((hStarShip =
					 CloneShipFragment(race, &uqm::gameState().game().stowed_ship_q, 0)))
			{
				continue;
			}
		}
		if (!hStarShip)
		{
			break;
		}

		RemoveQueue(&uqm::gameState().game().built_ship_q, hStarShip);

		/* Find first available escort window */
		while ((hOldShip = GetStarShipFromIndex(
					&uqm::gameState().game().built_ship_q, which_window++)))
		{
			StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hOldShip);
			const uint8_t win_loc {StarShipPtr->index};
			UnlockShipFrag(&uqm::gameState().game().built_ship_q, hOldShip);
			if (which_window <= win_loc)
			{
				break;
			}
		}

		StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
		StarShipPtr->index = which_window - 1;
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

		InsertQueue(&uqm::gameState().game().built_ship_q, hStarShip, hOldShip);
	}

	DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);
	return i;
}

/*
 * Returns the total value of all the ships escorting the SIS.
 */
uint16_t CalculateEscortsWorth(void)
{
	uint16_t total = 0;
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};

	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q);
		 hStarShip; hStarShip = hNextShip)
	{
		const SHIP_FRAGMENT* StarShipPtr {LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip)};
		hNextShip = _GetSuccLink(StarShipPtr);
		total += ShipCost(StarShipPtr->race_id);
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}
	return total;
}

bool ShipsReady(RACE_ID race)
{
	int16_t i {};
	uint16_t year {};
	uint16_t month {};
	uint16_t day = 0;
	switch (race)
	{
		case PKUNK_SHIP:
			year = uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_DAY);
			break;
		case SUPOX_SHIP:
			year = uqm::gameState().flag(uqm::GameFlag::SUPOX_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::SUPOX_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::SUPOX_SHIP_DAY);
			break;
		case UTWIG_SHIP:
			year = uqm::gameState().flag(uqm::GameFlag::UTWIG_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::UTWIG_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::UTWIG_SHIP_DAY);
			break;
		case YEHAT_SHIP:
		case YEHAT_REBEL_SHIP:
			year = uqm::gameState().flag(uqm::GameFlag::YEHAT_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::YEHAT_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::YEHAT_SHIP_DAY);
			break;
		default:
			return false;
	}
	return ((i = (uqm::gameState().clock().year() - StartYear) - year) > 0) || (i == 0 && (((i = uqm::gameState().clock().month() - month) > 0) || (i == 0 && uqm::gameState().clock().day() > day)));
}

void PrepareShip(RACE_ID race)
{
	uint8_t di {};
	uint8_t yi {};

	const uint8_t mi {uqm::gameState().clock().month()};
	if ((di = uqm::gameState().clock().day()) > 28)
	{
		di = 28;
	}
	yi = (uint8_t)(uqm::gameState().clock().year() - StartYear) + 1;
	switch (race)
	{
		case PKUNK_SHIP:
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SHIP_YEAR, static_cast<uint32_t>(yi));
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SHIP_MONTH, static_cast<uint32_t>(mi));
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SHIP_DAY, static_cast<uint32_t>(di));
			break;
		case SUPOX_SHIP:
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_SHIP_YEAR, static_cast<uint32_t>(yi));
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_SHIP_MONTH, static_cast<uint32_t>(mi));
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_SHIP_DAY, static_cast<uint32_t>(di));
			break;
		case UTWIG_SHIP:
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_SHIP_YEAR, static_cast<uint32_t>(yi));
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_SHIP_MONTH, static_cast<uint32_t>(mi));
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_SHIP_DAY, static_cast<uint32_t>(di));
			break;
		case YEHAT_SHIP:
		case YEHAT_REBEL_SHIP:
			uqm::gameState().setFlag(uqm::GameFlag::YEHAT_SHIP_YEAR, static_cast<uint32_t>(yi));
			uqm::gameState().setFlag(uqm::GameFlag::YEHAT_SHIP_MONTH, static_cast<uint32_t>(mi));
			uqm::gameState().setFlag(uqm::GameFlag::YEHAT_SHIP_DAY, static_cast<uint32_t>(di));
			break;
		default:
			break;
	}
}

#if 0
/*
 * Returns the size of the fleet of the specified race when the starmap was
 * last checked. If the race has no SoI, 0 is returned.
 */
uint16_t GetRaceKnownSize (RACE_ID race)
{
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;
	uint16_t result;

	hFleet = GetStarShipFromIndex (&uqm::gameState().game().avail_race_q, race);
	if (!hFleet)
		return 0;

	FleetPtr = LockFleetInfo (&uqm::gameState().game().avail_race_q, hFleet);

	result = FleetPtr->known_strength;

	UnlockFleetInfo (&uqm::gameState().game().avail_race_q, hFleet);
	return result;
}
#endif

/*
 * Start or end an alliance with the specified race.
 * Being in an alliance with a race makes their ships available for building
 * in the shipyard.
 * flag == true: start an alliance
 * flag == false: end an alliance
 */
bool SetRaceAllied(RACE_ID race, bool flag)
{
	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet)
	{
		return false;
	}

	FLEET_INFO* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	if (FleetPtr->allied_state == DEAD_GUY)
	{
		/* Strange request, silently ignore it */
	}
	else
	{
		FleetPtr->allied_state = (flag ? GOOD_GUY : BAD_GUY);
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	return true;
}

/*
 * 	Make the sphere of influence for the specified race shown on the starmap
 * 	in the future.
 * 	Does nothing for races without a SoI, or for races which have an
 * 	infinite SoI.
 * 	The value returned is 'race', unless the type of ship is only available
 * 	in SuperMelee, in which case 0 is returned.
 */
uint16_t StartSphereTracking(RACE_ID race)
{
	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet)
	{
		return 0;
	}

	FLEET_INFO* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	if (FleetPtr->actual_strength == 0)
	{
		// Race has no Sphere of Influence.
		if (FleetPtr->allied_state == DEAD_GUY)
		{
			// Race is extinct.
			UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
			return 0;
		}
	}
	else if (FleetPtr->known_strength == 0
			 && FleetPtr->actual_strength != InfiniteRadius)
	{
		FleetPtr->known_strength = 1;
		FleetPtr->known_loc = FleetPtr->loc;
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	return race;
}

/*
 * 	Check whether we are tracking the SoI of a race.
 * 	If a race has no SoI, this function will always return false.
 */
bool CheckSphereTracking(RACE_ID race)
{
	uint16_t result {};

	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet)
	{
		return false;
	}

	const FLEET_INFO* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	if (FleetPtr->actual_strength == 0)
	{
		// Race has no Sphere of Influence.
		// Maybe it never had one, or maybe the race is extinct.
		result = false;
	}
	else
	{
		result = (FleetPtr->known_strength > 0);
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	return result;
}

bool KillRace(RACE_ID race)
{
	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet)
	{
		return false;
	}

	FLEET_INFO* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	FleetPtr->allied_state = DEAD_GUY;
	FleetPtr->actual_strength = 0;

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	return true;
}

/*
 * Returns the number of ships of the specified race among the
 * escort ships.
 */
uint16_t CountEscortShips(RACE_ID race)
{
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};
	uint16_t result = 0;

	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet)
	{
		return 0;
	}

	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q); hStarShip;
		 hStarShip = hNextShip)
	{
		const SHIP_FRAGMENT* StarShipPtr {LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip)};
		hNextShip = _GetSuccLink(StarShipPtr);
		const uint8_t ship_type {StarShipPtr->race_id};
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

		if (ship_type == race)
		{
			result++;
		}
	}
	return result;
}

/*
 * Returns true if and only if a ship of the specified race is among the
 * escort ships.
 */
bool HaveEscortShip(RACE_ID race)
{
	return (CountEscortShips(race) > 0);
}

/*
 * Test if the SIS can have an escort of the specified race.
 * Returns 0 if 'race' is not available.
 * Otherwise, returns the number of ships that can be added.
 */
uint16_t EscortFeasibilityStudy(RACE_ID race)
{
	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet)
	{
		return 0;
	}

	return (MaxBuiltShips - CountLinks(&uqm::gameState().game().built_ship_q) + (StorageQ() ? MaxStowedShips - CountLinks(&uqm::gameState().game().stowed_ship_q) : 0));
}

/*
 * Test the alliance status of the specified race.
 * Either DEAD_GUY (extinct), GOOD_GUY (allied), or BAD_GUY (not allied) is
 * returned.
 */
uint16_t CheckAlliance(RACE_ID race)
{
	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, race)};
	if (!hFleet)
	{
		return 0;
	}

	const FLEET_INFO* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};
	const uint16_t flags {FleetPtr->allied_state};
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);

	return flags;
}

bool RaceDead(RACE_ID race)
{
	return CheckAlliance(race) == DEAD_GUY;
}

/*
 * Remove a number of escort ships of the specified race (if present).
 * Returns the number of escort ships removed.
 */
uint16_t RemoveSomeEscortShips(RACE_ID race, uint16_t count)
{
	QUEUE* ship_q = &uqm::gameState().game().built_ship_q;
	HSHIPFRAG hStarShip = GetHeadLink(ship_q);
	HSHIPFRAG hNextShip {};

	if (count == 0)
	{
		return 0;
	}
	if (!hStarShip && count > MaxBuiltShips)
	{
		ship_q = &uqm::gameState().game().stowed_ship_q;
		hStarShip = GetHeadLink(ship_q);
	}

	for (hStarShip = GetHeadLink(ship_q); hStarShip;
		 hStarShip = hNextShip)
	{
		const SHIP_FRAGMENT* StarShipPtr {LockShipFrag(ship_q, hStarShip)};
		hNextShip = _GetSuccLink(StarShipPtr);
		const bool RemoveShip {(StarShipPtr->race_id == race)};
		UnlockShipFrag(ship_q, hStarShip);

		if (RemoveShip)
		{
			RemoveQueue(ship_q, hStarShip);
			FreeShipFrag(ship_q, hStarShip);
			count--;
			if (count == 0)
			{
				break;
			}
		}
		if (count > MaxBuiltShips && !hNextShip && ship_q == &uqm::gameState().game().built_ship_q)
		{
			ship_q = &uqm::gameState().game().stowed_ship_q;
			hNextShip = GetHeadLink(ship_q);
		}
	}

	// Update the display.
	DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);

	return count;
}

/*
 * Remove all escort ships of the specified race.
 */
uint16_t RemoveEscortShips(RACE_ID race)
{
	return RemoveSomeEscortShips(race, (uint16_t)-1);
}

uint16_t GetIndexFromStarShip(QUEUE* pShipQ, HLINK hStarShip)
{
	uint16_t Index {0};
	while (hStarShip != GetHeadLink(pShipQ))
	{
		const LINK* StarShipPtr {LockLink(pShipQ, hStarShip)};
		HLINK hNextShip {_GetPredLink(StarShipPtr)};
		UnlockLink(pShipQ, hStarShip);

		hStarShip = hNextShip;
		++Index;
	}

	return Index;
}

uint8_t NameCaptain(QUEUE* pQueue, SPECIES_ID SpeciesID)
{
	uint8_t name_index {};
	HLINK hStarShip {};

	assert(GetLinkSize(pQueue) == sizeof(STARSHIP) || GetLinkSize(pQueue) == sizeof(SHIP_FRAGMENT));

	do
	{
		HLINK hNextShip {};

		name_index = PickCaptainName();
		for (hStarShip = GetHeadLink(pQueue); hStarShip;
			 hStarShip = hNextShip)
		{
			uint8_t test_name_index = -1;

			const SHIP_BASE* ShipPtr {(SHIP_BASE*)LockLink(pQueue, hStarShip)};
			hNextShip = _GetSuccLink(ShipPtr);
			if (ShipPtr->SpeciesID == SpeciesID)
			{
				test_name_index = ShipPtr->captains_name_index;
			}
			UnlockLink(pQueue, hStarShip);

			if (name_index == test_name_index)
			{
				break;
			}
		}
	} while (hStarShip /* name matched another ship */);

	return name_index;
}

// crew_level can be set to InfiniteFleet for a ship which is to
// represent an infinite number of ships.
HSHIPFRAG CloneShipFragment(RACE_ID shipIndex, QUEUE* pDstQueue, uint16_t crew_level)
{
	HFLEETINFO hFleet {};
	uint8_t captains_name_index {};

	assert(GetLinkSize(pDstQueue) == sizeof(SHIP_FRAGMENT));

	// If options mismatch with SIS, it means we're in a load window.
	// In that case we want to find the correct fleet ID for that ship.
	if ((uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed == 0)
		|| (!uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed != 0)
		|| (uqm::currentSeed().value != uqm::gameState().sis().Seed))
	{
		hFleet = GetSeededFleetFromIndex(shipIndex);
	}
	else
	{
		hFleet = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, shipIndex);
	}

	if (!hFleet)
	{
		return 0;
	}

	const FLEET_INFO* TemplatePtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};
	if (shipIndex == SAMATRA_SHIP)
	{
		captains_name_index = 0;
	}
	else
	{
		captains_name_index = NameCaptain(pDstQueue,
										  TemplatePtr->SpeciesID);
	}
	HSHIPFRAG hBuiltShip {Build(pDstQueue, TemplatePtr->SpeciesID)};
	if (hBuiltShip)
	{
		SHIP_FRAGMENT* ShipFragPtr {LockShipFrag(pDstQueue, hBuiltShip)};
		ShipFragPtr->captains_name_index = captains_name_index;
		ShipFragPtr->race_strings = TemplatePtr->race_strings;
		ShipFragPtr->icons = TemplatePtr->icons;
		ShipFragPtr->melee_icon = TemplatePtr->melee_icon;
		if (crew_level)
		{
			ShipFragPtr->crew_level = crew_level;
		}
		else
		{
			ShipFragPtr->crew_level = TemplatePtr->crew_level;
		}
		ShipFragPtr->max_crew = TemplatePtr->max_crew;
		ShipFragPtr->energy_level = 0;
		ShipFragPtr->max_energy = TemplatePtr->max_energy;
		ShipFragPtr->race_id = (uint8_t)shipIndex;
		ShipFragPtr->index = 0;
		UnlockShipFrag(pDstQueue, hBuiltShip);
	}
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);

	return hBuiltShip;
}

/* Set the crew and captain's name on the first fully-crewed escort
 * ship of race 'which_ship' */
int SetEscortCrewComplement(RACE_ID which_ship, uint16_t crew_level, uint8_t captain)
{
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};
	SHIP_FRAGMENT* StarShipPtr = 0;
	int Index {};

	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, which_ship)};
	if (!hFleet)
	{
		return -1;
	}
	const FLEET_INFO* TemplatePtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	/* Find first ship of which_ship race */
	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q), Index = 0;
		 hStarShip; hStarShip = hNextShip, ++Index)
	{
		StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
		hNextShip = _GetSuccLink(StarShipPtr);
		if (which_ship == StarShipPtr->race_id && StarShipPtr->crew_level == TemplatePtr->crew_level)
		{
			break; /* found one */
		}
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}
	if (hStarShip)
	{
		StarShipPtr->crew_level = (crew_level > StarShipPtr->max_crew ?
									   StarShipPtr->max_crew :
									   crew_level);
		StarShipPtr->captains_name_index = captain;
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}
	else
	{
		Index = -1;
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	return Index;
}

static void deviceSwitch(int device, int val)
{
	int var = 0;

	if (val)
	{
		var = val - 1;
	}

	switch (device)
	{
		case 0:
			uqm::gameState().setFlag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 1:
			uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 2:
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 3:
			uqm::gameState().setFlag(uqm::GameFlag::SUN_DEVICE_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 4:
			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 5:
			uqm::gameState().setFlag(uqm::GameFlag::AQUA_HELIX_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 6:
			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 7:
			uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(var));
			break;
		case 8:
			uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(var ? 2 : 0));
			break;
		case 9:
			uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(var ? 3 : 0));
			break;
		case 10:
			uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(var ? 4 : 0));
			break;
		case 11:
			uqm::gameState().setFlag(uqm::GameFlag::MAIDENS_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 12:
			uqm::gameState().setFlag(uqm::GameFlag::UMGAH_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 13:
			uqm::gameState().setFlag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 14:
			uqm::gameState().setFlag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 15:
		case 16:
		case 17:
			if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2 && var)
			{
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
			}
			uqm::gameState().setFlag(uqm::GameFlag::KNOW_SYREEN_WORLD_SHATTERED, static_cast<uint32_t>(var));

			if (device == 15)
			{
				uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE0_ON_SHIP, static_cast<uint32_t>(var));
			}
			else if (device == 16)
			{
				uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE1_ON_SHIP, static_cast<uint32_t>(var));
			}
			else
			{
				uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE2_ON_SHIP, static_cast<uint32_t>(var));
			}
			break;
		case 18:
			uqm::gameState().setFlag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 19:
			uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 20:
			uqm::gameState().setFlag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 21:
			uqm::gameState().setFlag(uqm::GameFlag::PORTAL_KEY_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 22:
			uqm::gameState().setFlag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 23:
			uqm::gameState().setFlag(uqm::GameFlag::GLOWING_ROD_ON_SHIP, static_cast<uint32_t>(var));
			break;
		case 24:
			uqm::gameState().setFlag(uqm::GameFlag::MOONBASE_ON_SHIP, static_cast<uint32_t>(var));
			break;
		default: // Shouldn't happen, do nothing
			break;
	}

	//SET_GAME_STATE (SHIP_VAULT_UNLOCKED, 1);
	//SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 0);
	//SET_GAME_STATE (SYREEN_HOME_VISITS, 0);
}

static void upgradeSwitch(int upgrade, int val)
{
	int var = 0;
	uint8_t LanderShields {};
	int ModuleCost {};

	if (val)
	{
		var = val - 1;
	}

	if (upgrade > 2 || upgrade < 7)
	{
		LanderShields = uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS);
	}

	switch (upgrade)
	{
		case 0:
			uqm::gameState().setFlag(uqm::GameFlag::IMPROVED_LANDER_SPEED, static_cast<uint32_t>(var));
			break;
		case 1:
			uqm::gameState().setFlag(uqm::GameFlag::IMPROVED_LANDER_CARGO, static_cast<uint32_t>(var));
			break;
		case 2:
			uqm::gameState().setFlag(uqm::GameFlag::IMPROVED_LANDER_SHOT, static_cast<uint32_t>(var));
			break;
		case 3:
			if (var)
			{
				LanderShields |= (1 << BIOLOGICAL_DISASTER);
			}
			else
			{
				LanderShields &= ~(1 << BIOLOGICAL_DISASTER);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 4:
			if (var)
			{
				LanderShields |= (1 << EARTHQUAKE_DISASTER);
			}
			else
			{
				LanderShields &= ~(1 << EARTHQUAKE_DISASTER);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 5:
			if (var)
			{
				LanderShields |= (1 << LIGHTNING_DISASTER);
			}
			else
			{
				LanderShields &= ~(1 << LIGHTNING_DISASTER);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 6:
			if (var)
			{
				LanderShields |= (1 << LAVASPOT_DISASTER);
			}
			else
			{
				LanderShields &= ~(1 << LAVASPOT_DISASTER);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 7:
			ModuleCost = var ? 4000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[ANTIMISSILE_DEFENSE] = ModuleCost;
			break;
		case 8:
			ModuleCost = var ? 4000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[BLASTER_WEAPON] = ModuleCost;
			break;
		case 9:
			ModuleCost = var ? 1000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[HIGHEFF_FUELSYS] = ModuleCost;
			break;
		case 10:
			ModuleCost = var ? 5000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[TRACKING_SYSTEM] = ModuleCost;
			break;
		case 11:
			ModuleCost = var ? 6000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[CANNON_WEAPON] = ModuleCost;
			break;
		case 12:
			ModuleCost = var ? 4000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[SHIVA_FURNACE] = ModuleCost;
			break;
		default: // Shouldn't happen, do nothing
			break;
	}
}

static void cheatAddRemoveDevices(void)
{
	for (uint8_t i {0}; i < std::size(optDeviceArray); i++)
	{
		if (optDeviceArray[i] != uqm::CheatAction::None)
		{
			deviceSwitch(i, static_cast<int>(optDeviceArray[i]));
		}
	}
}

static void cheatAddRemoveUpgrades(void)
{
	for (uint8_t i {0}; i < NumUpgrades; i++)
	{
		if (optUpgradeArray[i] != uqm::CheatAction::None)
		{
			upgradeSwitch(i, static_cast<int>(optUpgradeArray[i]));
		}
	}
}

void loadGameCheats(void)
{
	if (IsExtended() && !uqm::isStarSeed())
	{
		plot_map[MELNORME0_DEFINED].star->Type =
			MakeStar(SUPER_GIANT_STAR, ORANGE_BODY, -1);
	}

	if (uqm::UQMOptions::read().infiniteRU)
	{
		oldRU = uqm::gameState().sis().ResUnits;
	}
	else
	{
		oldRU = 0;
	}

	//for (uint8_t i = ARILOU_SHIP; i <= MMRNMHRM_SHIP; ++i)
	//{
	//	StartSphereTracking (i);
	//	KillRace (i);
	//}

	// SET_GAME_STATE (CHMMR_UNLEASHED, 1);

	// SET_GAME_STATE (KNOW_HOMEWORLD, ~0);

	if (uqm::UQMOptions::read().infiniteFuel)
	{
		loadFuel = uqm::gameState().sis().FuelOnBoard;
		uqm::gameState().sis().FuelOnBoard = GetFuelTankCapacity();
	}
	else
	{
		loadFuel = 0;
	}

	cheatAddRemoveDevices();
	cheatAddRemoveUpgrades();
}

// Jitter returns a distance between 0..66.6% of the fleet's actual strength,
// weighted towards 20% median value.  We can adjust the jitter by changing
// the fraction at the end.
// (uint16_t) sqrt (rand_val) gives a value 0..255 which leans heavy towards 255
// so subtract from 255 to receive a weighted towards zero jitter.
uint16_t Jitter(uint16_t str, uint16_t rand_val)
{
	return (str * (SphereRadiusIncrement / 2) * (255 - (uint16_t)sqrt(rand_val)) / 256) * 2 / 3;
}

void JitDebug(FLEET_INFO* FleetPtr, uint16_t rand_val_x, uint16_t rand_val_y)
{
	const static char* const fleet_name[] =
		{"ERROR", "Arilou", "CHMMR", "Earthling", "Orz", "Pkunk",
		 "Shofixti", "Spathi", "Supox", "Thraddash", "Utwig", "VUX",
		 "Yehat", "Melnorme", "Druuge", "Ilwrath", "Mycon", "Slylandro",
		 "Umgah", "Ur Quan", "ZoqFotPik", "Syreen", "Kohr Ah",
		 "Androsynth", "Chenjesu", "Mmrnmhrm"};
	const uint16_t str = (FleetPtr->actual_strength > 0 ?
							  FleetPtr->actual_strength :
							  WarEraStrength(FleetPtr->SpeciesID));
	fmt::print(stderr, "{} Fleet {}: Actual Str {}; WarEra Str {}; ",
			   fleet_name[FleetPtr->SpeciesID > 25 ? 0 : FleetPtr->SpeciesID],
			   (int)FleetPtr->SpeciesID,
			   FleetPtr->actual_strength * SphereRadiusIncrement / 2,
			   WarEraStrength(FleetPtr->SpeciesID) * SphereRadiusIncrement / 2);
	fmt::print(stderr, "Rand X {} ({}); Rand Y {} ({})\n",
			   rand_val_x, (255 - (uint16_t)sqrt(rand_val_x)),
			   rand_val_y, (255 - (uint16_t)sqrt(rand_val_y)));
	fmt::print(stderr, "		Jit X {}% Jit Y {}%; (Jit X {} Jit Y {})  ",
			   (255 - (uint16_t)sqrt(rand_val_x)) * 67 / 256,
			   (255 - (uint16_t)sqrt(rand_val_y)) * 67 / 256,
			   Jitter(str, rand_val_x),
			   Jitter(str, rand_val_y));
	fmt::print(stderr, "{:05.1} : {:05.1}  units.\n",
			   (float)Jitter(str, rand_val_x) / 10,
			   (float)Jitter(str, rand_val_y) / 10);
}

// Provide the default fleet movement or war era position for the
// given fleet and plot ID being visited
GFXPOINT DefaultFleetLocation(SPECIES_ID SpeciesID, uint16_t visit)
{
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "Seeding hard coded location for fleet {} (plot {}).\n",
			   SpeciesID, visit);
#endif
	// We need to return plot specific coordinates here
	// We also assume we only care about fleet movements and war map,
	// as initial fleet positions are loaded from the ship files.
	switch (SpeciesID)
	{
		case CHMMR_ID:
			return GFXPOINT {577, 2509}; // Homeworld
		case EARTHLING_ID:
			return GFXPOINT {1806, 1476}; // War era map
		case PKUNK_ID:
			if (visit == YEHAT_DEFINED)
			{
				return GFXPOINT {4970, 400}; // Rejoin Yehat
			}
			else if (visit == WAR_ERA)
			{
				return GFXPOINT {577, 463}; // War Era 'Unknown'
			}
			else
			{
				return GFXPOINT {502, 401}; // Return homeworld
			}
		case SHOFIXTI_ID:
			return GFXPOINT {2852, 242}; // War era map
		case SPATHI_ID:
			return GFXPOINT {2416, 3687}; // War era map
		case SUPOX_ID:
			if (visit == SAMATRA_DEFINED)
			{
				return GFXPOINT {6479, 7541}; // Attack Kohr-Ah
			}
			else
			{
				return GFXPOINT {7468, 9246}; // Return homeworld
			}
		case THRADDASH_ID:
			if (visit == SAMATRA_DEFINED)
			{
				return GFXPOINT {4879, 7201}; // Attack Kohr-Ah
			}
			else if (visit == WAR_ERA)
			{
				return GFXPOINT {2808, 8522}; // War Era 'Unknown'
			}
			else
			{
				return GFXPOINT {2535, 8358}; // Return homeworld
			}
		case UTWIG_ID:
			if (visit == SAMATRA_DEFINED)
			{
				return GFXPOINT {7208, 7000}; // Attack Kohr-Ah
			}
			else
			{
				return GFXPOINT {8534, 8797}; // Return homeworld
			}
		case VUX_ID:
			return GFXPOINT {4333, 1520}; // War era map
		case YEHAT_ID:
			if (visit == YEHAT_DEFINED)
			{
				return GFXPOINT {5150, 0}; // Here there be rebels
			}
			else if (visit == WAR_ERA)
			{
				return GFXPOINT {4969, 75}; // War era map
			}
			else
			{
				return GFXPOINT {4970, 40}; // Filthy Royalists
			}
		case DRUUGE_ID:
			return GFXPOINT {9421, 2754}; // War Era 'Unknown'
		case ILWRATH_ID:
			if (visit == THRADD_DEFINED)
			{
				return GFXPOINT {2500, 8070}; // Thraddash/Ilwrath zone
			}
			else if (visit == PKUNK_DEFINED)
			{
				return GFXPOINT {48, 1700}; // Attacking Pkunk (unused?)
			}
			else if (visit == WAR_ERA)
			{
				return GFXPOINT {0, 3589}; // War era map
			}
			else
			{
				return GFXPOINT {215, 3630}; // Homeworld (extended lore)
			}
		case MYCON_ID:
			if (visit == MYCON_TRAP_DEFINED)
			{
				return GFXPOINT {6858, 577}; // Goin to the party
			}
			else if (visit == WAR_ERA)
			{
				return GFXPOINT {6278, 2399}; // War era map
			}
			else
			{
				return GFXPOINT {6392, 2200}; // Going back home
			}
		case UMGAH_ID:
			if (visit == SAMATRA_DEFINED)
			{
				return GFXPOINT {5288, 4892}; // Compelled to seek death
			}
			else
			{
				return GFXPOINT {1860, 6099}; // War era map
			}
		case SYREEN_ID:
			if (visit == MYCON_TRAP_DEFINED)
			{
				return GFXPOINT {6858, 577}; // Goin to the party
			}
			else
			{
				return GFXPOINT {4125, 3770}; // Going back home
			}
		case ANDROSYNTH_ID:
			return GFXPOINT {3676, 2619}; // War era map
		case CHENJESU_ID:
			return GFXPOINT {701, 2137}; // War era map
		case MMRNMHRM_ID:
			return GFXPOINT {672, 2930}; // War era map
		default:
			fmt::print(stderr, "{} {} / plot {} combo.\n",
					   "DefaultFleetLocation called with bad fleet",
					   (int)SpeciesID, visit);
			return GFXPOINT {0, 0};
	}
}

// Provide the war era strength for the given species ID (from fleet).
uint16_t WarEraStrength(SPECIES_ID SpeciesID)
{
	switch (SpeciesID)
	{
		case EARTHLING_ID:
			return 460 / SphereRadiusIncrement * 2;
		case PKUNK_ID:
			return 350 / SphereRadiusIncrement * 2;
		case SHOFIXTI_ID:
			return 329 / SphereRadiusIncrement * 2;
		case SPATHI_ID:
			return 1007 / SphereRadiusIncrement * 2;
		case THRADDASH_ID:
			return 723 / SphereRadiusIncrement * 2;
		case VUX_ID:
			return 572 / SphereRadiusIncrement * 2;
		case YEHAT_ID:
			return 1029 / SphereRadiusIncrement * 2;
		case DRUUGE_ID:
			return 745 / SphereRadiusIncrement * 2;
		case ILWRATH_ID:
			return 470 / SphereRadiusIncrement * 2;
		case MYCON_ID:
			return 591 / SphereRadiusIncrement * 2;
		case UMGAH_ID:
			return 506 / SphereRadiusIncrement * 2;
		case ANDROSYNTH_ID:
			return 504 / SphereRadiusIncrement * 2;
		case CHENJESU_ID:
			return 657 / SphereRadiusIncrement * 2;
		case MMRNMHRM_ID:
			return 482 / SphereRadiusIncrement * 2;
		default:
			return 0;
	}
	return 0;
}

// Provide the homeworld plot ID for the given species ID (from fleet).
uint16_t Homeworld(SPECIES_ID SpeciesID)
{
	switch (SpeciesID)
	{
		case ARILOU_ID:
			return ARILOU_DEFINED;
		case CHMMR_ID:
			return CHMMR_DEFINED;
		case EARTHLING_ID:
			return SOL_DEFINED;
		case ORZ_ID:
			return ORZ_DEFINED;
		case PKUNK_ID:
			return PKUNK_DEFINED;
		case SHOFIXTI_ID:
			return SHOFIXTI_DEFINED;
		case SPATHI_ID:
			return SPATHI_DEFINED;
		case SUPOX_ID:
			return SUPOX_DEFINED;
		case THRADDASH_ID:
			return THRADD_DEFINED;
		case UTWIG_ID:
			return UTWIG_DEFINED;
		case VUX_ID:
			return VUX_DEFINED;
		case YEHAT_ID:
			return YEHAT_DEFINED;
		case MELNORME_ID:
			return MELNORME1_DEFINED; // I have my reasons
		case DRUUGE_ID:
			return DRUUGE_DEFINED;
		case ILWRATH_ID:
			return ILWRATH_DEFINED;
		case MYCON_ID:
			return MYCON_DEFINED;
		case SLYLANDRO_ID:
			return SLYLANDRO_DEFINED;
		case UMGAH_ID:
			return TALKING_PET_DEFINED;
		case UR_QUAN_ID:
			return SAMATRA_DEFINED;
		case ZOQFOTPIK_ID:
			return ZOQFOT_DEFINED;
		case SYREEN_ID:
			return SYREEN_DEFINED;
		case KOHR_AH_ID:
			return SAMATRA_DEFINED;
		case ANDROSYNTH_ID:
			return ANDROSYNTH_DEFINED;
		case CHENJESU_ID:
			return CHMMR_DEFINED;
		case MMRNMHRM_ID:
			return MOTHER_ARK_DEFINED;
		default:
			fmt::print(stderr, "Homeworld {} {}.\n",
					   "(Fleet ID) called with bad fleet ID", (int)SpeciesID);
			return NUM_PLOTS;
	}
}

// SeedFleet is called during initial setup of a map.  Sets the fleet
// referenced in the FLEET_INFO pointer passed to the coordinates of that
// fleet's plot on the PLOT_LOCATION array passed.
// For Prime seed games this just returns, as the fleet does not relocate.
void SeedFleet(FLEET_INFO* FleetPtr, PLOT_LOCATION* plotmap)
{
	if ((!uqm::isStarSeed()) || (uqm::currentSeed().type == uqm::SeedType::MRQ))
	{
		return;
	}
	if (!FleetPtr || !plotmap)
	{
		fmt::print(stderr, "SeedFleet called with nullptr PTR(s).\n");
		return;
	}
	FleetPtr->known_loc = SeedFleetLocation(FleetPtr, plotmap,
											(FleetPtr->SpeciesID == ILWRATH_ID) ? PKUNK_DEFINED : HOME);
	return;
}

// Helper define functions for SeedFleetLocation
//
// WARPATH gives an offset to the "warpoint" destination, to be multiplied
// by dist, in the direction of the warpather's home star.  This will scale
// the arguments based on total distance to provide a new center.
// {WarpathX (visit, dist) + jitter, WarpathY (visit, dist) + jitter}

// This points your jitter towards or away from "a" in X, Y directions

// For those with no toward/away you can randomly jitter +/-
constexpr int HalfX(auto rand_val_x) { return (rand_val_x % 2 ? 1 : -1); }
constexpr int HalfY(auto rand_val_y) { return (rand_val_y % 2 ? 1 : -1); }

// 1/5 of the time we flip X, or Y, or both for some races.  Why?  Why not!
constexpr int FifthX(auto rand_val_x, auto rand_val_y) { return ((rand_val_x * rand_val_y) % 10 ? 1 : -1); }
constexpr int FifthY(auto rand_val_x, auto rand_val_y) { return ((rand_val_x + rand_val_y) % 5 ? 1 : -1); }

// SeedFleetLocation is called when a fleet needs to be located or relocated.
// It will return the center for the SOI of the race listed, or if visit
// plot ID != HOME, another value as dictated by the interaction of the two.
// Then the location is "jittered".  Essentially this function maps between a
// fleet/race ID and the plot IDs.
// For PrimeSeed games this will need to look up the hard coded fleet movements
// or resets which were part of gameev.c, or the war era map.
GFXPOINT SeedFleetLocation(FLEET_INFO* FleetPtr, PLOT_LOCATION* plotmap, uint16_t visit)
{
	uint16_t rand_val_x {};
	uint16_t rand_val_y {};
	uint16_t home {};			// Plot ID of the homeworld for the fleet
	uint16_t strength {};		// Strength of the fleet
	GFXPOINT warpoint;			// location being visited, or offset for samatra
	GFXPOINT location = {0, 0}; // The results of the seeding
	bool myRNG = false;			// If you create RNG, clean up RNG
	char buf[256] = "";			// For debug string

	if (!FleetPtr || !plotmap)
	{
		fmt::print(stderr, "SeedFleetLocation called with nullptr PTR(s).\n");
		return GFXPOINT {~0, ~0};
	}

	// Firstly, handle hard coded data from prime seed
	if (!uqm::isStarSeed() || uqm::currentSeed().type == uqm::SeedType::MRQ)
	{
		return DefaultFleetLocation(FleetPtr->SpeciesID, visit);
	}

	home = Homeworld(FleetPtr->SpeciesID);
	const auto TowardX = [&](auto a) {
		return ((plotmap[home].star_pt.x < plotmap[(a)].star_pt.x) ? 1 : -1);
	};
	const auto TowardY = [&](auto a) {
		return ((plotmap[home].star_pt.y < plotmap[(a)].star_pt.y) ? 1 : -1);
	};
	const auto AwayX = [&](auto a) {
		return ((plotmap[home].star_pt.x > plotmap[(a)].star_pt.x) ? 1 : -1);
	};
	const auto AwayY = [&](auto a) {
		return ((plotmap[home].star_pt.y > plotmap[(a)].star_pt.y) ? 1 : -1);
	};
	const auto WarpathX = [&](const auto& a, const auto& d) {
		return (warpoint.x + (d) * (plotmap[home].star_pt.x - (a).x) / sqrt((plotmap[home].star_pt.x - (a).x) * (plotmap[home].star_pt.x - (a).x) + (plotmap[home].star_pt.y - (a).y) * (plotmap[home].star_pt.y - (a).y)));
	};
	const auto WarpathY = [&](const auto& a, const auto& d) {
		return (warpoint.y + (d) * (plotmap[home].star_pt.y - (a).y) / sqrt((plotmap[home].star_pt.x - (a).x) * (plotmap[home].star_pt.x - (a).x) + (plotmap[home].star_pt.y - (a).y) * (plotmap[home].star_pt.y - (a).y)));
	};
	if (home >= NUM_PLOTS)
	{
		return GFXPOINT {~0, ~0}; // Error already messaged in Homeworld ().
	}
	if (visit >= NUM_PLOTS && visit != WAR_ERA && visit != HOME)
	{
		fmt::print(stderr, "{} {}\n",
				   "SeedFleet called with invalid away plot ID", visit);
		return GFXPOINT {~0, ~0};
	}
	if (visit < NUM_PLOTS && !plotmap[visit].star)
	{
		fmt::print(stderr, "{} {} has a nullptr star pointer.\n",
				   "SeedFleet called, but away plot ID", visit);
		return GFXPOINT {~0, ~0};
	}
	if (!plotmap[home].star)
	{
		fmt::print(stderr, "{} {} has a nullptr star pointer.\n",
				   "SeedFleet called, but home plot ID", home);
		return GFXPOINT {~0, ~0};
	}

	if (!StarGenRNG)
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "SeedFleet creating a STAR GEN RNG\n");
#endif
		StarGenRNG.emplace();
		myRNG = true;
	}
	StarGenRNG->reseed(GetRandomSeedForStar(plotmap[home].star));
	rand_val_x = StarGenRNG->next();
	rand_val_y = StarGenRNG->next();
	if (visit > 0 && visit != SAMATRA_DEFINED && visit < NUM_PLOTS)
	{
		StarGenRNG->reseed(GetRandomSeedForStar(plotmap[visit].star));
		rand_val_x += StarGenRNG->next() % sizeof(uint16_t);
		rand_val_y += StarGenRNG->next() % sizeof(uint16_t);
		// If there's a conflict, this will assist in WARPATH
		warpoint = plotmap[visit].star_pt;
	}
	else if (visit == SAMATRA_DEFINED)
	{
		// Contrary to other fleet movements, I don't want to jitter from
		// the SAMATRA center.  Pick a spot nearby and then jitter from there.
		// First, imagine a coord near SAMATRA that Utwig/Supox/Thraddash can
		// sort of share (it will bias in the general direction of home).
		StarGenRNG->reseed(GetRandomSeedForStar(plotmap[visit].star));
		const uint16_t rand_val_s = StarGenRNG->next();
		warpoint = GFXPOINT {
			plotmap[visit].star_pt.x + uqm::lowByte(rand_val_s) * 2 * AwayX(visit),
			plotmap[visit].star_pt.y + uqm::highByte(rand_val_s) * 2 * AwayY(visit)};

		fmt::format_to_sz_n(buf, "'Sa-Matra' center {:05.1} : {:05.1}\n",
							(float)warpoint.x / 10, (float)warpoint.y / 10);
	}
	else
	{
		warpoint = plotmap[home].star_pt; // just in case
	}

	strength = (visit == WAR_ERA ? WarEraStrength(FleetPtr->SpeciesID) :
								   FleetPtr->actual_strength);

	switch (FleetPtr->SpeciesID)
	{
		case ARILOU_ID: // Arilou don't jitter
			location = plotmap[home].star_pt;
			break;
		case CHMMR_ID: // Jitter towards Mmrnmhrm home
			location = GFXPOINT {
				plotmap[home].star_pt.x + TowardX(MOTHER_ARK_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(MOTHER_ARK_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case EARTHLING_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case ORZ_ID: // Jitter towards the playground
			location = GFXPOINT {
				plotmap[home].star_pt.x + TowardX(TAALO_PROTECTOR_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(TAALO_PROTECTOR_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case PKUNK_ID: // Jitter away from Ilwrath / home from Yehat
			if (visit == YEHAT_DEFINED)
			{
				location = GFXPOINT {
					plotmap[visit].star_pt.x + AwayX(visit) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
					plotmap[visit].star_pt.y + AwayY(visit) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + AwayX(ILWRATH_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + AwayY(ILWRATH_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SHOFIXTI_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + TowardX(YEHAT_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(YEHAT_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SPATHI_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SUPOX_ID: // Jitter towards Utwig
			if (visit == SAMATRA_DEFINED)
			{
				// 1200 distance times warpath
				// Then jitter fleet away from Utwig, for reasons
				location = GFXPOINT {
					WarpathX(warpoint, 1200) + AwayX(UTWIG_DEFINED) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1200) + AwayY(UTWIG_DEFINED) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + TowardX(UTWIG_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(UTWIG_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case THRADDASH_ID:
			if (visit == SAMATRA_DEFINED)
			{
				// 1200 distance times warpath
				// Bias away from the aqua helix because thraddash.
				location = GFXPOINT {
					WarpathX(warpoint, 1200) + AwayX(AQUA_HELIX_DEFINED) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1200) + AwayY(AQUA_HELIX_DEFINED) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + TowardX(AQUA_HELIX_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(AQUA_HELIX_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case UTWIG_ID: // Jitter towards Supox
			if (visit == SAMATRA_DEFINED)
			{
				// 1200 distance times warpath
				// Then jitter fleet away from Supox, for reasons
				location = GFXPOINT {
					WarpathX(warpoint, 1200) + AwayX(SUPOX_DEFINED) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1200) + AwayY(SUPOX_DEFINED) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + TowardX(SUPOX_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(SUPOX_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case VUX_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case YEHAT_ID: // Jitter towards Shofixti.  Except rebels.
			if (visit == YEHAT_DEFINED)
			{
				location = GFXPOINT {
					plotmap[home].star_pt.x + AwayX(SHOFIXTI_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
					plotmap[home].star_pt.y + AwayY(SHOFIXTI_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + TowardX(SHOFIXTI_DEFINED) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(SHOFIXTI_DEFINED) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case MELNORME_ID:
			break;
		case DRUUGE_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case ILWRATH_ID:				 // Jitter towards victims
			if (visit == THRADD_DEFINED) // default 290 distance
			{
				// Set the X and Y coords based on being 500 units away
				// from the target along the warpath.
				location = GFXPOINT {
					WarpathX(warpoint, 500) + TowardX(visit) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 500) + TowardY(visit) * Jitter(strength, rand_val_y)};
				fmt::format_to_sz_n(buf, "{} {:05.1} : {:05.1}\n",
									"ILWRATH x THRADDASH",
									(float)location.x / 10, (float)location.y / 10);
				break;
			}
			if (visit == PKUNK_DEFINED) // default 1270 distance
			{
				warpoint = plotmap[visit].star_pt;
				location = GFXPOINT {
					WarpathX(warpoint, 1400) + TowardX(visit) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1400) + TowardY(visit) * Jitter(strength, rand_val_y)};
				fmt::format_to_sz_n(buf, "{} {:05.1} : {:05.1}\n",
									"ILWRATH x PKUNK",
									(float)location.x / 10, (float)location.y / 10);
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case MYCON_ID:
			if (visit == MYCON_TRAP_DEFINED)
			{
				location = GFXPOINT {
					plotmap[visit].star_pt.x + AwayX(visit) * Jitter(strength, rand_val_x),
					plotmap[visit].star_pt.y + AwayY(visit) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {// Jitter towards sun device and eggs
								 plotmap[home].star_pt.x + (plotmap[home].star_pt.x * 2 < plotmap[SUN_DEVICE_DEFINED].star_pt.x + plotmap[EGG_CASE2_DEFINED].star_pt.x / 2 + plotmap[EGG_CASE1_DEFINED].star_pt.x / 3 + plotmap[EGG_CASE0_DEFINED].star_pt.x / 6 ? 1 : -1) * Jitter(strength, rand_val_x),
								 plotmap[home].star_pt.y + (plotmap[home].star_pt.y * 2 < plotmap[SUN_DEVICE_DEFINED].star_pt.y + plotmap[EGG_CASE2_DEFINED].star_pt.y / 2 + plotmap[EGG_CASE1_DEFINED].star_pt.y / 3 + plotmap[EGG_CASE0_DEFINED].star_pt.y / 6 ? 1 : -1) * Jitter(strength, rand_val_y)};
			break;
		case SLYLANDRO_ID:
			break;
		case UMGAH_ID:
			if (visit == SAMATRA_DEFINED)
			{
				// 2000 distance times warpath
				// Bias away from the umgah
				location = GFXPOINT {
					WarpathX(warpoint, 2000) + AwayX(TALKING_PET_DEFINED) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 2000) + AwayY(TALKING_PET_DEFINED) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case UR_QUAN_ID: // Halved jitter due to size
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfY(rand_val_y) * Jitter(strength, rand_val_x) / 2,
				plotmap[home].star_pt.y + HalfX(rand_val_x) + Jitter(strength, rand_val_y) / 2};
			break;
		case KOHR_AH_ID:		 // Halved jitter, swap x and y from UQ (same seed)
			location = GFXPOINT {// and ONE sign for a 90 degree turn
								 plotmap[home].star_pt.x - HalfX(rand_val_x) * Jitter(strength, rand_val_y) / 2,
								 plotmap[home].star_pt.y + HalfY(rand_val_y) + Jitter(strength, rand_val_x) / 2};
			break;
		case ZOQFOTPIK_ID:		 // ZoqFot jitter is inverted (str - jit) away from
			location = GFXPOINT {// KA/UQ zone, then /2, gives 16% - 50% jit
								 plotmap[home].star_pt.x + AwayX(SAMATRA_DEFINED) * (FleetPtr->actual_strength * SphereRadiusIncrement / 2 - Jitter(strength, rand_val_x)) / 2,
								 plotmap[home].star_pt.y + AwayY(SAMATRA_DEFINED) * (FleetPtr->actual_strength * SphereRadiusIncrement / 2 - Jitter(strength, rand_val_y)) / 2};
			break;
		case SYREEN_ID:
			if (visit == MYCON_TRAP_DEFINED)
			{
				location = GFXPOINT {
					plotmap[visit].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
					plotmap[visit].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case ANDROSYNTH_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_y),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_x)};
			break;
		case CHENJESU_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case MMRNMHRM_ID:
			location = GFXPOINT {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_y),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_x)};
			break;
		default:
			break;
	}
	if (location.x < 0)
	{
		location.x = 0;
	}
	if (location.x >= MaxXUniverse)
	{
		location.x = MaxXUniverse - 1;
	}
	if (location.y < 0)
	{
		location.y = 0;
	}
	if (location.y >= MaxYUniverse)
	{
		location.y = MaxYUniverse - 1;
	}

#ifdef DEBUG_STARSEED
	JitDebug(FleetPtr, rand_val_x, rand_val_y);
	if (buf[0])
	{
		fmt::print(stderr, "{}", buf);
	}
#endif
	if (StarGenRNG && myRNG)
	{
		StarGenRNG.reset();
	}
	return (location);
}
