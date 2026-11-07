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

#include <stdlib.h>

#include "core/misc/bittricks.h"
#include "core/string/StringUtils.h"
#include "gendef.h"
#include "libs/mathlib.h"
#include "master.h"
#include "options.h"
#include "planets/planets.h"
#include "races.h"
#include "save.h"
#include "setup.h"
#include "sis.h"
#include "starbase.h"
#include "starmap.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
// Allocate a new Starship or ShipFragment and put it in the queue
LinkHandle Build(Queue* pQueue, SpeciesId SpeciesID)
{
	assert(GetLinkSize(pQueue) == sizeof(Starship) || GetLinkSize(pQueue) == sizeof(ShipFragment));

	LinkHandle hNewShip {AllocLink(pQueue)};
	if (!hNewShip)
	{
		return 0;
	}

	ShipBase* ShipPtr {(ShipBase*)LockLink(pQueue, hNewShip)};
	memset(ShipPtr, 0, GetLinkSize(pQueue));
	ShipPtr->SpeciesID = SpeciesID;

	UnlockLink(pQueue, hNewShip);
	PutQueue(pQueue, hNewShip);

	return hNewShip;
}

LinkHandle GetStarShipFromIndex(Queue* pShipQ, uint16_t Index)
{
	LinkHandle hStarShip {};
	LinkHandle hNextShip {};

	for (hStarShip = GetHeadLink(pShipQ);
		 Index > 0 && hStarShip; hStarShip = hNextShip, --Index)
	{
		const QueueLink* StarShipPtr {LockLink(pShipQ, hStarShip)};
		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockLink(pShipQ, hStarShip);
	}

	return (hStarShip);
}

// Gives the first fleet in avail_race_q that builds specified ship.
LinkHandle GetFleetFromSpecies(SpeciesId id)
{
	LinkHandle hFleet {};
	LinkHandle hNextFleet {};

	for (hFleet = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hFleet; hFleet = hNextFleet)
	{
		const FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};
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
FleetInfoHandle GetSeededFleetFromIndex(uint16_t Index)
{
	const FleetInfo* TemplatePtr = nullptr;
	SpeciesId ship;
	const bool loadWindow = ((uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed == 0) || (!uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed != 0) || (uqm::currentSeed().value != uqm::gameState().sis().Seed));

	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, Index)};
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

ShipFragmentHandle GetEscortByStarShipIndex(uint16_t index)
{
	ShipFragmentHandle hStarShip {};
	ShipFragmentHandle hNextShip {};
	const ShipFragment* StarShipPtr {};

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

SpeciesId ShipIdStrToIndex(const char* shipIdStr)
{
	MasterShipHandle hStarShip {};
	MasterShipHandle hNextShip {};
	SpeciesId result = SpeciesId::None;

	for (hStarShip = GetHeadLink(&master_q);
		 hStarShip != 0; hStarShip = hNextShip)
	{
		const MasterShipInfo* MasterPtr {LockMasterShip(&master_q, hStarShip)};
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

struct RaceIdMap
{
	const char* idStr;
	RaceId id;
};

// We would eventually want to unhardcode this.
static uqstl::array<RaceIdMap, 28> raceIdMap {
	{
		// Sorted on the name, for the binary search.
		{/* .idStr = */ "androsynth", /* .id = */ RaceId::Androsynth},
		{/* .idStr = */ "arilou", /* .id = */ RaceId::Arilou},
		{/* .idStr = */ "chenjesu", /* .id = */ RaceId::Chenjesu},
		{/* .idStr = */ "chmmr", /* .id = */ RaceId::Chmmr},
		{/* .idStr = */ "druuge", /* .id = */ RaceId::Druuge},
		{/* .idStr = */ "human", /* .id = */ RaceId::Human},
		{/* .idStr = */ "ilwrath", /* .id = */ RaceId::Ilwrath},
		{/* .idStr = */ "kohrah", /* .id = */ RaceId::BlackUrquan},
		{/* .idStr = */ "melnorme", /* .id = */ RaceId::Melnorme},
		{/* .idStr = */ "mmrnmhrm", /* .id = */ RaceId::Mmrnmhrm},
		{/* .idStr = */ "mycon", /* .id = */ RaceId::Mycon},
		{/* .idStr = */ "orz", /* .id = */ RaceId::Orz},
		{/* .idStr = */ "pkunk", /* .id = */ RaceId::Pkunk},
		{/* .idStr = */ "samatra", /* .id = */ RaceId::Samatra},
		{/* .idStr = */ "shofixti", /* .id = */ RaceId::Shofixti},
		{/* .idStr = */ "slylandro", /* .id = */ RaceId::Slylandro},
		{/* .idStr = */ "spathi", /* .id = */ RaceId::Spathi},
		{/* .idStr = */ "supox", /* .id = */ RaceId::Supox},
		{/* .idStr = */ "syreen", /* .id = */ RaceId::Syreen},
		{/* .idStr = */ "thraddash", /* .id = */ RaceId::Thraddash},
		{/* .idStr = */ "umgah", /* .id = */ RaceId::Umgah},
		{/* .idStr = */ "urquandrone", /* .id = */ RaceId::UrquanDrone},
		{/* .idStr = */ "urquan", /* .id = */ RaceId::Urquan},
		{/* .idStr = */ "utwig", /* .id = */ RaceId::Utwig},
		{/* .idStr = */ "vux", /* .id = */ RaceId::Vux},
		{/* .idStr = */ "yehat", /* .id = */ RaceId::Yehat},
		{/* .idStr = */ "yehatrebel", /* .id = */ RaceId::YehatRebel},
		{/* .idStr = */ "zoqfotpik", /* .id = */ RaceId::Zoqfotpik},
		// Same as URQUAN_DRONE_SHIP
	}
};

static int RaceIdCompare(const void* id1, const void* id2)
{
	return strcmp(((RaceIdMap*)id1)->idStr, ((RaceIdMap*)id2)->idStr);
}

RaceId RaceIdStrToIndex(const char* raceIdStr)
{
	RaceIdMap key = {/* .idStr = */ raceIdStr, /* .id = */ (RaceId)-1};
	const RaceIdMap* found = (RaceIdMap*)bsearch(&key, raceIdMap.data(),
												 std::size(raceIdMap),
												 sizeof raceIdMap[0], RaceIdCompare);

	if (found == nullptr)
	{
		return (RaceId)-1;
	}

	return found->id;
}

/*
 * Give the player 'count' ships of the specified race,
 * limited by the number of free slots.
 * Returns the number of ships added.
 */
uint16_t AddEscortShips(RaceId race, int16_t count)
{
	uint16_t i {};

	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
	if (!hFleet || count <= 0)
	{
		return 0;
	}

	uint8_t which_window {0};
	for (i = 0; i < (uint16_t)count; i++)
	{
		ShipFragmentHandle hStarShip {};
		ShipFragmentHandle hOldShip {};
		ShipFragment* StarShipPtr {};

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
	ShipFragmentHandle hStarShip {};
	ShipFragmentHandle hNextShip {};

	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q);
		 hStarShip; hStarShip = hNextShip)
	{
		const ShipFragment* StarShipPtr {LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip)};
		hNextShip = _GetSuccLink(StarShipPtr);
		total += ShipCost(StarShipPtr->race_id);
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}
	return total;
}

bool ShipsReady(RaceId race)
{
	int16_t i {};
	uint16_t year {};
	uint16_t month {};
	uint16_t day = 0;
	switch (race)
	{
		case RaceId::Pkunk:
			year = uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_DAY);
			break;
		case RaceId::Supox:
			year = uqm::gameState().flag(uqm::GameFlag::SUPOX_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::SUPOX_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::SUPOX_SHIP_DAY);
			break;
		case RaceId::Utwig:
			year = uqm::gameState().flag(uqm::GameFlag::UTWIG_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::UTWIG_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::UTWIG_SHIP_DAY);
			break;
		case RaceId::Yehat:
		case RaceId::YehatRebel:
			year = uqm::gameState().flag(uqm::GameFlag::YEHAT_SHIP_YEAR);
			month = uqm::gameState().flag(uqm::GameFlag::YEHAT_SHIP_MONTH);
			day = uqm::gameState().flag(uqm::GameFlag::YEHAT_SHIP_DAY);
			break;
		default:
			return false;
	}
	return ((i = (uqm::gameState().clock().year() - StartYear) - year) > 0) || (i == 0 && (((i = uqm::gameState().clock().month() - month) > 0) || (i == 0 && uqm::gameState().clock().day() > day)));
}

void PrepareShip(RaceId race)
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
		case RaceId::Pkunk:
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SHIP_YEAR, static_cast<uint32_t>(yi));
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SHIP_MONTH, static_cast<uint32_t>(mi));
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SHIP_DAY, static_cast<uint32_t>(di));
			break;
		case RaceId::Supox:
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_SHIP_YEAR, static_cast<uint32_t>(yi));
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_SHIP_MONTH, static_cast<uint32_t>(mi));
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_SHIP_DAY, static_cast<uint32_t>(di));
			break;
		case RaceId::Utwig:
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_SHIP_YEAR, static_cast<uint32_t>(yi));
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_SHIP_MONTH, static_cast<uint32_t>(mi));
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_SHIP_DAY, static_cast<uint32_t>(di));
			break;
		case RaceId::Yehat:
		case RaceId::YehatRebel:
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
uint16_t GetRaceKnownSize (RaceId race)
{
	FleetInfoHandle hFleet;
	FleetInfo *FleetPtr;
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
bool SetRaceAllied(RaceId race, bool flag)
{
	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
	if (!hFleet)
	{
		return false;
	}

	FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	if (FleetPtr->allied_state == RaceAllegiance::Dead)
	{
		/* Strange request, silently ignore it */
	}
	else
	{
		FleetPtr->allied_state = *(flag ? RaceAllegiance::Good : RaceAllegiance::Bad);
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
uint16_t StartSphereTracking(RaceId race)
{
	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
	if (!hFleet)
	{
		return 0;
	}

	FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	if (FleetPtr->actual_strength == 0)
	{
		// Race has no Sphere of Influence.
		if (FleetPtr->allied_state == RaceAllegiance::Dead)
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
	return *race;
}

/*
 * 	Check whether we are tracking the SoI of a race.
 * 	If a race has no SoI, this function will always return false.
 */
bool CheckSphereTracking(RaceId race)
{
	uint16_t result {};

	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
	if (!hFleet)
	{
		return false;
	}

	const FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

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

bool KillRace(RaceId race)
{
	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
	if (!hFleet)
	{
		return false;
	}

	FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	FleetPtr->allied_state = *RaceAllegiance::Dead;
	FleetPtr->actual_strength = 0;

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
	return true;
}

/*
 * Returns the number of ships of the specified race among the
 * escort ships.
 */
uint16_t CountEscortShips(RaceId race)
{
	ShipFragmentHandle hStarShip {};
	ShipFragmentHandle hNextShip {};
	uint16_t result = 0;

	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
	if (!hFleet)
	{
		return 0;
	}

	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q); hStarShip;
		 hStarShip = hNextShip)
	{
		const ShipFragment* StarShipPtr {LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip)};
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
bool HaveEscortShip(RaceId race)
{
	return (CountEscortShips(race) > 0);
}

/*
 * Test if the SIS can have an escort of the specified race.
 * Returns 0 if 'race' is not available.
 * Otherwise, returns the number of ships that can be added.
 */
uint16_t EscortFeasibilityStudy(RaceId race)
{
	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
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
uint16_t CheckAlliance(RaceId race)
{
	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *race)};
	if (!hFleet)
	{
		return 0;
	}

	const FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};
	const uint16_t flags {FleetPtr->allied_state};
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);

	return flags;
}

bool RaceDead(RaceId race)
{
	return CheckAlliance(race) == RaceAllegiance::Dead;
}

/*
 * Remove a number of escort ships of the specified race (if present).
 * Returns the number of escort ships removed.
 */
uint16_t RemoveSomeEscortShips(RaceId race, uint16_t count)
{
	Queue* ship_q = &uqm::gameState().game().built_ship_q;
	ShipFragmentHandle hStarShip = GetHeadLink(ship_q);
	ShipFragmentHandle hNextShip {};

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
		const ShipFragment* StarShipPtr {LockShipFrag(ship_q, hStarShip)};
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
uint16_t RemoveEscortShips(RaceId race)
{
	return RemoveSomeEscortShips(race, (uint16_t)-1);
}

uint16_t GetIndexFromStarShip(Queue* pShipQ, LinkHandle hStarShip)
{
	uint16_t Index {0};
	while (hStarShip != GetHeadLink(pShipQ))
	{
		const QueueLink* StarShipPtr {LockLink(pShipQ, hStarShip)};
		LinkHandle hNextShip {_GetPredLink(StarShipPtr)};
		UnlockLink(pShipQ, hStarShip);

		hStarShip = hNextShip;
		++Index;
	}

	return Index;
}

uint8_t NameCaptain(Queue* pQueue, SpeciesId SpeciesID)
{
	uint8_t name_index {};
	LinkHandle hStarShip {};

	assert(GetLinkSize(pQueue) == sizeof(Starship) || GetLinkSize(pQueue) == sizeof(ShipFragment));

	do
	{
		LinkHandle hNextShip {};

		name_index = PickCaptainName();
		for (hStarShip = GetHeadLink(pQueue); hStarShip;
			 hStarShip = hNextShip)
		{
			uint8_t test_name_index = -1;

			const ShipBase* ShipPtr {(ShipBase*)LockLink(pQueue, hStarShip)};
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
ShipFragmentHandle CloneShipFragment(RaceId shipIndex, Queue* pDstQueue, uint16_t crew_level)
{
	FleetInfoHandle hFleet {};
	uint8_t captains_name_index {};

	assert(GetLinkSize(pDstQueue) == sizeof(ShipFragment));

	// If options mismatch with SIS, it means we're in a load window.
	// In that case we want to find the correct fleet ID for that ship.
	if ((uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed == 0)
		|| (!uqm::currentSeed().ships && uqm::gameState().sis().ShipSeed != 0)
		|| (uqm::currentSeed().value != uqm::gameState().sis().Seed))
	{
		hFleet = GetSeededFleetFromIndex(*shipIndex);
	}
	else
	{
		hFleet = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *shipIndex);
	}

	if (!hFleet)
	{
		return 0;
	}

	const FleetInfo* TemplatePtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};
	if (shipIndex == RaceId::Samatra)
	{
		captains_name_index = 0;
	}
	else
	{
		captains_name_index = NameCaptain(pDstQueue,
										  TemplatePtr->SpeciesID);
	}
	ShipFragmentHandle hBuiltShip {Build(pDstQueue, TemplatePtr->SpeciesID)};
	if (hBuiltShip)
	{
		ShipFragment* ShipFragPtr {LockShipFrag(pDstQueue, hBuiltShip)};
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
int SetEscortCrewComplement(RaceId which_ship, uint16_t crew_level, uint8_t captain)
{
	ShipFragmentHandle hStarShip {};
	ShipFragmentHandle hNextShip {};
	ShipFragment* StarShipPtr = 0;
	int Index {};

	FleetInfoHandle hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *which_ship)};
	if (!hFleet)
	{
		return -1;
	}
	const FleetInfo* TemplatePtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

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
				LanderShields |= (1 << *LanderEvent::BiologicalDisaster);
			}
			else
			{
				LanderShields &= ~(1 << *LanderEvent::BiologicalDisaster);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 4:
			if (var)
			{
				LanderShields |= (1 << *LanderEvent::EarthquakeDisaster);
			}
			else
			{
				LanderShields &= ~(1 << *LanderEvent::EarthquakeDisaster);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 5:
			if (var)
			{
				LanderShields |= (1 << *LanderEvent::LightningDisaster);
			}
			else
			{
				LanderShields &= ~(1 << *LanderEvent::LightningDisaster);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 6:
			if (var)
			{
				LanderShields |= (1 << *LanderEvent::LavaspotDisaster);
			}
			else
			{
				LanderShields &= ~(1 << *LanderEvent::LavaspotDisaster);
			}
			uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>(LanderShields));
			break;
		case 7:
			ModuleCost = var ? 4000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[*ModuleType::AntimissileDefense] = ModuleCost;
			break;
		case 8:
			ModuleCost = var ? 4000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[*ModuleType::BlasterWeapon] = ModuleCost;
			break;
		case 9:
			ModuleCost = var ? 1000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[*ModuleType::HigheffFuelsys] = ModuleCost;
			break;
		case 10:
			ModuleCost = var ? 5000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[*ModuleType::TrackingSystem] = ModuleCost;
			break;
		case 11:
			ModuleCost = var ? 6000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[*ModuleType::CannonWeapon] = ModuleCost;
			break;
		case 12:
			ModuleCost = var ? 4000 / ModuleCostScale : 0;
			uqm::gameState().game().ModuleCost[*ModuleType::ShivaFurnace] = ModuleCost;
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
		plot_map[*PlotId::Melnorme0].star->Type =
			MakeStar(StellarSize::SuperGiant, StarBodyColor::Orange, -1);
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

void JitDebug(FleetInfo* FleetPtr, uint16_t rand_val_x, uint16_t rand_val_y)
{
	static uqstl::array<const char* const, 26> fleet_name {"ERROR", "Arilou", "CHMMR", "Earthling", "Orz", "Pkunk",
														   "Shofixti", "Spathi", "Supox", "Thraddash", "Utwig", "VUX",
														   "Yehat", "Melnorme", "Druuge", "Ilwrath", "Mycon", "Slylandro",
														   "Umgah", "Ur Quan", "ZoqFotPik", "Syreen", "Kohr Ah",
														   "Androsynth", "Chenjesu", "Mmrnmhrm"};
	const uint16_t str = (FleetPtr->actual_strength > 0 ?
							  FleetPtr->actual_strength :
							  WarEraStrength(FleetPtr->SpeciesID));
	fmt::print(stderr, "{} Fleet {}: Actual Str {}; WarEra Str {}; ",
			   fleet_name[FleetPtr->SpeciesID > 25 ? 0 : *FleetPtr->SpeciesID],
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
GfxPoint DefaultFleetLocation(SpeciesId SpeciesID, uint16_t visit)
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
		case SpeciesId::Chmmr:
			return GfxPoint {577, 2509}; // Homeworld
		case SpeciesId::Earthling:
			return GfxPoint {1806, 1476}; // War era map
		case SpeciesId::Pkunk:
			if (visit == PlotId::Yehat)
			{
				return GfxPoint {4970, 400}; // Rejoin Yehat
			}
			else if (visit == PlotId::WarEra)
			{
				return GfxPoint {577, 463}; // War Era 'Unknown'
			}
			else
			{
				return GfxPoint {502, 401}; // Return homeworld
			}
		case SpeciesId::Shofixti:
			return GfxPoint {2852, 242}; // War era map
		case SpeciesId::Spathi:
			return GfxPoint {2416, 3687}; // War era map
		case SpeciesId::Supox:
			if (visit == PlotId::Samatra)
			{
				return GfxPoint {6479, 7541}; // Attack Kohr-Ah
			}
			else
			{
				return GfxPoint {7468, 9246}; // Return homeworld
			}
		case SpeciesId::Thraddash:
			if (visit == PlotId::Samatra)
			{
				return GfxPoint {4879, 7201}; // Attack Kohr-Ah
			}
			else if (visit == PlotId::WarEra)
			{
				return GfxPoint {2808, 8522}; // War Era 'Unknown'
			}
			else
			{
				return GfxPoint {2535, 8358}; // Return homeworld
			}
		case SpeciesId::Utwig:
			if (visit == PlotId::Samatra)
			{
				return GfxPoint {7208, 7000}; // Attack Kohr-Ah
			}
			else
			{
				return GfxPoint {8534, 8797}; // Return homeworld
			}
		case SpeciesId::Vux:
			return GfxPoint {4333, 1520}; // War era map
		case SpeciesId::Yehat:
			if (visit == PlotId::Yehat)
			{
				return GfxPoint {5150, 0}; // Here there be rebels
			}
			else if (visit == PlotId::WarEra)
			{
				return GfxPoint {4969, 75}; // War era map
			}
			else
			{
				return GfxPoint {4970, 40}; // Filthy Royalists
			}
		case SpeciesId::Druuge:
			return GfxPoint {9421, 2754}; // War Era 'Unknown'
		case SpeciesId::Ilwrath:
			if (visit == PlotId::Thradd)
			{
				return GfxPoint {2500, 8070}; // Thraddash/Ilwrath zone
			}
			else if (visit == PlotId::Pkunk)
			{
				return GfxPoint {48, 1700}; // Attacking Pkunk (unused?)
			}
			else if (visit == PlotId::WarEra)
			{
				return GfxPoint {0, 3589}; // War era map
			}
			else
			{
				return GfxPoint {215, 3630}; // Homeworld (extended lore)
			}
		case SpeciesId::Mycon:
			if (visit == PlotId::MyconTrap)
			{
				return GfxPoint {6858, 577}; // Goin to the party
			}
			else if (visit == PlotId::WarEra)
			{
				return GfxPoint {6278, 2399}; // War era map
			}
			else
			{
				return GfxPoint {6392, 2200}; // Going back home
			}
		case SpeciesId::Umgah:
			if (visit == PlotId::Samatra)
			{
				return GfxPoint {5288, 4892}; // Compelled to seek death
			}
			else
			{
				return GfxPoint {1860, 6099}; // War era map
			}
		case SpeciesId::Syreen:
			if (visit == PlotId::MyconTrap)
			{
				return GfxPoint {6858, 577}; // Goin to the party
			}
			else
			{
				return GfxPoint {4125, 3770}; // Going back home
			}
		case SpeciesId::Androsynth:
			return GfxPoint {3676, 2619}; // War era map
		case SpeciesId::Chenjesu:
			return GfxPoint {701, 2137}; // War era map
		case SpeciesId::Mmrnmhrm:
			return GfxPoint {672, 2930}; // War era map
		default:
			fmt::print(stderr, "{} {} / plot {} combo.\n",
					   "DefaultFleetLocation called with bad fleet",
					   (int)SpeciesID, visit);
			return GfxPoint {0, 0};
	}
}

// Provide the war era strength for the given species ID (from fleet).
uint16_t WarEraStrength(SpeciesId SpeciesID)
{
	switch (SpeciesID)
	{
		case SpeciesId::Earthling:
			return 460 / SphereRadiusIncrement * 2;
		case SpeciesId::Pkunk:
			return 350 / SphereRadiusIncrement * 2;
		case SpeciesId::Shofixti:
			return 329 / SphereRadiusIncrement * 2;
		case SpeciesId::Spathi:
			return 1007 / SphereRadiusIncrement * 2;
		case SpeciesId::Thraddash:
			return 723 / SphereRadiusIncrement * 2;
		case SpeciesId::Vux:
			return 572 / SphereRadiusIncrement * 2;
		case SpeciesId::Yehat:
			return 1029 / SphereRadiusIncrement * 2;
		case SpeciesId::Druuge:
			return 745 / SphereRadiusIncrement * 2;
		case SpeciesId::Ilwrath:
			return 470 / SphereRadiusIncrement * 2;
		case SpeciesId::Mycon:
			return 591 / SphereRadiusIncrement * 2;
		case SpeciesId::Umgah:
			return 506 / SphereRadiusIncrement * 2;
		case SpeciesId::Androsynth:
			return 504 / SphereRadiusIncrement * 2;
		case SpeciesId::Chenjesu:
			return 657 / SphereRadiusIncrement * 2;
		case SpeciesId::Mmrnmhrm:
			return 482 / SphereRadiusIncrement * 2;
		default:
			return 0;
	}
	return 0;
}

// Provide the homeworld plot ID for the given species ID (from fleet).
uint16_t Homeworld(SpeciesId SpeciesID)
{
	switch (SpeciesID)
	{
		case SpeciesId::Arilou:
			return *PlotId::Arilou;
		case SpeciesId::Chmmr:
			return *PlotId::Chmmr;
		case SpeciesId::Earthling:
			return *PlotId::Sol;
		case SpeciesId::Orz:
			return *PlotId::Orz;
		case SpeciesId::Pkunk:
			return *PlotId::Pkunk;
		case SpeciesId::Shofixti:
			return *PlotId::Shofixti;
		case SpeciesId::Spathi:
			return *PlotId::Spathi;
		case SpeciesId::Supox:
			return *PlotId::Supox;
		case SpeciesId::Thraddash:
			return *PlotId::Thradd;
		case SpeciesId::Utwig:
			return *PlotId::Utwig;
		case SpeciesId::Vux:
			return *PlotId::Vux;
		case SpeciesId::Yehat:
			return *PlotId::Yehat;
		case SpeciesId::Melnorme:
			return *PlotId::Melnorme1; // I have my reasons
		case SpeciesId::Druuge:
			return *PlotId::Druuge;
		case SpeciesId::Ilwrath:
			return *PlotId::Ilwrath;
		case SpeciesId::Mycon:
			return *PlotId::Mycon;
		case SpeciesId::Slylandro:
			return *PlotId::Slylandro;
		case SpeciesId::Umgah:
			return *PlotId::TalkingPet;
		case SpeciesId::UrQuan:
			return *PlotId::Samatra;
		case SpeciesId::Zoqfotpik:
			return *PlotId::Zoqfot;
		case SpeciesId::Syreen:
			return *PlotId::Syreen;
		case SpeciesId::KohrAh:
			return *PlotId::Samatra;
		case SpeciesId::Androsynth:
			return *PlotId::Androsynth;
		case SpeciesId::Chenjesu:
			return *PlotId::Chmmr;
		case SpeciesId::Mmrnmhrm:
			return *PlotId::MotherArk;
		default:
			fmt::print(stderr, "Homeworld {} {}.\n",
					   "(Fleet ID) called with bad fleet ID", (int)SpeciesID);
			return *PlotId::NumPlots;
	}
}

// SeedFleet is called during initial setup of a map.  Sets the fleet
// referenced in the FleetInfo pointer passed to the coordinates of that
// fleet's plot on the PlotLocation array passed.
// For Prime seed games this just returns, as the fleet does not relocate.
void SeedFleet(FleetInfo* FleetPtr, PlotLocation* plotmap)
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
											(FleetPtr->SpeciesID == SpeciesId::Ilwrath) ? *PlotId::Pkunk : *PlotId::Home);
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
GfxPoint SeedFleetLocation(FleetInfo* FleetPtr, PlotLocation* plotmap, uint16_t visit)
{
	uint16_t rand_val_x {};
	uint16_t rand_val_y {};
	uint16_t home {};				// Plot ID of the homeworld for the fleet
	uint16_t strength {};			// Strength of the fleet
	GfxPoint warpoint;				// location being visited, or offset for samatra
	GfxPoint location = {0, 0};		// The results of the seeding
	bool myRNG = false;				// If you create RNG, clean up RNG
	uqstl::array<char, 256> buf {}; // For debug string

	if (!FleetPtr || !plotmap)
	{
		fmt::print(stderr, "SeedFleetLocation called with nullptr PTR(s).\n");
		return GfxPoint {~0, ~0};
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
	if (home >= PlotId::NumPlots)
	{
		return GfxPoint {~0, ~0}; // Error already messaged in Homeworld ().
	}
	if (visit >= PlotId::NumPlots && visit != PlotId::WarEra && visit != PlotId::Home)
	{
		fmt::print(stderr, "{} {}\n",
				   "SeedFleet called with invalid away plot ID", visit);
		return GfxPoint {~0, ~0};
	}
	if (visit < PlotId::NumPlots && !plotmap[visit].star)
	{
		fmt::print(stderr, "{} {} has a nullptr star pointer.\n",
				   "SeedFleet called, but away plot ID", visit);
		return GfxPoint {~0, ~0};
	}
	if (!plotmap[home].star)
	{
		fmt::print(stderr, "{} {} has a nullptr star pointer.\n",
				   "SeedFleet called, but home plot ID", home);
		return GfxPoint {~0, ~0};
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
	if (visit > 0 && visit != PlotId::Samatra && visit < PlotId::NumPlots)
	{
		StarGenRNG->reseed(GetRandomSeedForStar(plotmap[visit].star));
		rand_val_x += StarGenRNG->next() % sizeof(uint16_t);
		rand_val_y += StarGenRNG->next() % sizeof(uint16_t);
		// If there's a conflict, this will assist in WARPATH
		warpoint = plotmap[visit].star_pt;
	}
	else if (visit == PlotId::Samatra)
	{
		// Contrary to other fleet movements, I don't want to jitter from
		// the SAMATRA center.  Pick a spot nearby and then jitter from there.
		// First, imagine a coord near SAMATRA that Utwig/Supox/Thraddash can
		// sort of share (it will bias in the general direction of home).
		StarGenRNG->reseed(GetRandomSeedForStar(plotmap[visit].star));
		const uint16_t rand_val_s = StarGenRNG->next();
		warpoint = GfxPoint {
			plotmap[visit].star_pt.x + uqm::lowByte(rand_val_s) * 2 * AwayX(visit),
			plotmap[visit].star_pt.y + uqm::highByte(rand_val_s) * 2 * AwayY(visit)};

		fmt::format_to_sz_n(buf, "'Sa-Matra' center {:05.1} : {:05.1}\n",
							(float)warpoint.x / 10, (float)warpoint.y / 10);
	}
	else
	{
		warpoint = plotmap[home].star_pt; // just in case
	}

	strength = (visit == PlotId::WarEra ? WarEraStrength(FleetPtr->SpeciesID) :
										  FleetPtr->actual_strength);

	switch (FleetPtr->SpeciesID)
	{
		case SpeciesId::Arilou: // Arilou don't jitter
			location = plotmap[home].star_pt;
			break;
		case SpeciesId::Chmmr: // Jitter towards Mmrnmhrm home
			location = GfxPoint {
				plotmap[home].star_pt.x + TowardX(*PlotId::MotherArk) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(*PlotId::MotherArk) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Earthling:
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Orz: // Jitter towards the playground
			location = GfxPoint {
				plotmap[home].star_pt.x + TowardX(*PlotId::TaaloProtector) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(*PlotId::TaaloProtector) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Pkunk: // Jitter away from Ilwrath / home from Yehat
			if (visit == PlotId::Yehat)
			{
				location = GfxPoint {
					plotmap[visit].star_pt.x + AwayX(visit) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
					plotmap[visit].star_pt.y + AwayY(visit) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + AwayX(*PlotId::Ilwrath) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + AwayY(*PlotId::Ilwrath) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Shofixti:
			location = GfxPoint {
				plotmap[home].star_pt.x + TowardX(*PlotId::Yehat) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(*PlotId::Yehat) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Spathi:
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Supox: // Jitter towards Utwig
			if (visit == PlotId::Samatra)
			{
				// 1200 distance times warpath
				// Then jitter fleet away from Utwig, for reasons
				location = GfxPoint {
					WarpathX(warpoint, 1200) + AwayX(*PlotId::Utwig) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1200) + AwayY(*PlotId::Utwig) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + TowardX(*PlotId::Utwig) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(*PlotId::Utwig) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Thraddash:
			if (visit == PlotId::Samatra)
			{
				// 1200 distance times warpath
				// Bias away from the aqua helix because thraddash.
				location = GfxPoint {
					WarpathX(warpoint, 1200) + AwayX(*PlotId::AquaHelix) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1200) + AwayY(*PlotId::AquaHelix) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + TowardX(*PlotId::AquaHelix) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(*PlotId::AquaHelix) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Utwig: // Jitter towards Supox
			if (visit == PlotId::Samatra)
			{
				// 1200 distance times warpath
				// Then jitter fleet away from Supox, for reasons
				location = GfxPoint {
					WarpathX(warpoint, 1200) + AwayX(*PlotId::Supox) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1200) + AwayY(*PlotId::Supox) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + TowardX(*PlotId::Supox) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(*PlotId::Supox) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Vux:
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Yehat: // Jitter towards Shofixti.  Except rebels.
			if (visit == PlotId::Yehat)
			{
				location = GfxPoint {
					plotmap[home].star_pt.x + AwayX(*PlotId::Shofixti) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
					plotmap[home].star_pt.y + AwayY(*PlotId::Shofixti) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + TowardX(*PlotId::Shofixti) * FifthX(rand_val_x, rand_val_y) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + TowardY(*PlotId::Shofixti) * FifthY(rand_val_x, rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Melnorme:
			break;
		case SpeciesId::Druuge:
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Ilwrath:		 // Jitter towards victims
			if (visit == PlotId::Thradd) // default 290 distance
			{
				// Set the X and Y coords based on being 500 units away
				// from the target along the warpath.
				location = GfxPoint {
					WarpathX(warpoint, 500) + TowardX(visit) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 500) + TowardY(visit) * Jitter(strength, rand_val_y)};
				fmt::format_to_sz_n(buf, "{} {:05.1} : {:05.1}\n",
									"ILWRATH x THRADDASH",
									(float)location.x / 10, (float)location.y / 10);
				break;
			}
			if (visit == PlotId::Pkunk) // default 1270 distance
			{
				warpoint = plotmap[visit].star_pt;
				location = GfxPoint {
					WarpathX(warpoint, 1400) + TowardX(visit) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 1400) + TowardY(visit) * Jitter(strength, rand_val_y)};
				fmt::format_to_sz_n(buf, "{} {:05.1} : {:05.1}\n",
									"ILWRATH x PKUNK",
									(float)location.x / 10, (float)location.y / 10);
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Mycon:
			if (visit == PlotId::MyconTrap)
			{
				location = GfxPoint {
					plotmap[visit].star_pt.x + AwayX(visit) * Jitter(strength, rand_val_x),
					plotmap[visit].star_pt.y + AwayY(visit) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {// Jitter towards sun device and eggs
								 plotmap[home].star_pt.x + (plotmap[home].star_pt.x * 2 < plotmap[*PlotId::SunDevice].star_pt.x + plotmap[*PlotId::EggCase2].star_pt.x / 2 + plotmap[*PlotId::EggCase1].star_pt.x / 3 + plotmap[*PlotId::EggCase0].star_pt.x / 6 ? 1 : -1) * Jitter(strength, rand_val_x),
								 plotmap[home].star_pt.y + (plotmap[home].star_pt.y * 2 < plotmap[*PlotId::SunDevice].star_pt.y + plotmap[*PlotId::EggCase2].star_pt.y / 2 + plotmap[*PlotId::EggCase1].star_pt.y / 3 + plotmap[*PlotId::EggCase0].star_pt.y / 6 ? 1 : -1) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Slylandro:
			break;
		case SpeciesId::Umgah:
			if (visit == PlotId::Samatra)
			{
				// 2000 distance times warpath
				// Bias away from the umgah
				location = GfxPoint {
					WarpathX(warpoint, 2000) + AwayX(*PlotId::TalkingPet) * Jitter(strength, rand_val_x),
					WarpathY(warpoint, 2000) + AwayY(*PlotId::TalkingPet) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::UrQuan: // Halved jitter due to size
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfY(rand_val_y) * Jitter(strength, rand_val_x) / 2,
				plotmap[home].star_pt.y + HalfX(rand_val_x) + Jitter(strength, rand_val_y) / 2};
			break;
		case SpeciesId::KohrAh:	 // Halved jitter, swap x and y from UQ (same seed)
			location = GfxPoint {// and ONE sign for a 90 degree turn
								 plotmap[home].star_pt.x - HalfX(rand_val_x) * Jitter(strength, rand_val_y) / 2,
								 plotmap[home].star_pt.y + HalfY(rand_val_y) + Jitter(strength, rand_val_x) / 2};
			break;
		case SpeciesId::Zoqfotpik: // ZoqFot jitter is inverted (str - jit) away from
			location = GfxPoint {  // KA/UQ zone, then /2, gives 16% - 50% jit
								 plotmap[home].star_pt.x + AwayX(*PlotId::Samatra) * (FleetPtr->actual_strength * SphereRadiusIncrement / 2 - Jitter(strength, rand_val_x)) / 2,
								 plotmap[home].star_pt.y + AwayY(*PlotId::Samatra) * (FleetPtr->actual_strength * SphereRadiusIncrement / 2 - Jitter(strength, rand_val_y)) / 2};
			break;
		case SpeciesId::Syreen:
			if (visit == PlotId::MyconTrap)
			{
				location = GfxPoint {
					plotmap[visit].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
					plotmap[visit].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
				break;
			}
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Androsynth:
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_y),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_x)};
			break;
		case SpeciesId::Chenjesu:
			location = GfxPoint {
				plotmap[home].star_pt.x + HalfX(rand_val_x) * Jitter(strength, rand_val_x),
				plotmap[home].star_pt.y + HalfY(rand_val_y) * Jitter(strength, rand_val_y)};
			break;
		case SpeciesId::Mmrnmhrm:
			location = GfxPoint {
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

} // namespace uqm
