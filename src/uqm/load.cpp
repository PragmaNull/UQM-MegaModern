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

#include <assert.h>

#include "build.h"
#include "encount.h"
#include "starmap.h"
#include "libs/file.h"
#include "globdata.h"
#include "options.h"
#include "save.h"
#include "setup.h"
#include "state.h"
#include "grpintrn.h"
#include "uqmdebug.h"
#include "gameev.h"
#include "libs/tasklib.h"
#include "core/log/log.h"
#include "core/stl/stl.h"
#include "libs/misc.h"
#include "master.h"

//#define DEBUG_LOAD

ACTIVITY NextActivity;
uint8_t IndependantResFactor;

template <size_t TypeSize>
struct ReadValueTypes
{
	using temp_type = uint64_t; // default to something absurd.
};
template <>
struct ReadValueTypes<1>
{
	using temp_type = uint8_t;
};
template <>
struct ReadValueTypes<2>
{
	using temp_type = uint16_t;
};
template <>
struct ReadValueTypes<4>
{
	using temp_type = uint32_t;
};


template <typename T>
static inline bool readValue(uqm::io::File& fp, T* v)
{
	using temp_type = typename ReadValueTypes<sizeof(T)>::temp_type;
	temp_type t {};
	for (uint8_t n {}, shift {}; n < sizeof(T); ++n, shift += 8)
	{
		uint8_t b {};
		if (!fp.readValue(b)) [[unlikely]]
		{
			return false;
		}

		t |= static_cast<temp_type>(b) << shift;
	}

	if (v) [[likely]]
	{
		*v = static_cast<T>(t);
	}
	return true;
}

template <typename T>
static inline bool readValueArray(uqm::io::File& fp, uqstl::span<T> ar)
{
	assert(!ar.empty());
	if constexpr (sizeof(T) == 1)
	{
		return fp.readExact(uqstl::as_writable_bytes(ar));
	}
	else
	{
		for (auto& v : ar)
		{
			if (!readValue<T>(fp, &v)) [[unlikely]]
			{
				return false;
			}
		}
		return true;
	}
}

static inline bool read_str(uqm::io::File& fp, uqstl::span<char> str)
{
	// no type conversion needed for strings
	return readValueArray(fp, str);
}

template <typename T>
static inline bool skipN(uqm::io::File& fp, uint16_t count)
{
	for (int i = 0; i < count; ++i)
	{
		if (!readValue<T>(fp, nullptr))
		{
			return false;
		}
	}
	return true;
}


static void
LoadShipQueue(uqm::io::File& fh, QUEUE* pQueue, uint32_t size)
{
	uint16_t num_links = size / 11;

	while (num_links--)
	{
		HSHIPFRAG hStarShip;
		SHIP_FRAGMENT* FragPtr;
		uint16_t Index;

		readValue(fh, &Index);

		hStarShip = CloneShipFragment((RACE_ID)Index, pQueue, 0);
		FragPtr = LockShipFrag(pQueue, hStarShip);

		// Read SHIP_FRAGMENT elements
		readValue(fh, &FragPtr->captains_name_index);
		readValue(fh, &FragPtr->race_id);
		readValue(fh, &FragPtr->index);
		readValue(fh, &FragPtr->crew_level);
		readValue(fh, &FragPtr->max_crew);
		readValue(fh, &FragPtr->energy_level);
		readValue(fh, &FragPtr->max_energy);

		UnlockShipFrag(pQueue, hStarShip);
	}
}

static void
LoadRaceQueue(uqm::io::File& fh, QUEUE* pQueue, uint32_t size)
{
	uint16_t num_links = size / 30;

	while (num_links--)
	{
		HFLEETINFO hStarShip;
		FLEET_INFO* FleetPtr;
		uint16_t Index;

		readValue(fh, &Index);

		hStarShip = GetStarShipFromIndex(pQueue, Index);
		FleetPtr = LockFleetInfo(pQueue, hStarShip);

		// Read FLEET_INFO elements
		readValue(fh, &FleetPtr->allied_state);

		if (FleetPtr->allied_state > 2)
		{
			FleetPtr->allied_state = BAD_GUY;
		}

		readValue(fh, &FleetPtr->days_left);
		readValue(fh, &FleetPtr->growth_fract);
		readValue(fh, &FleetPtr->crew_level);
		readValue(fh, &FleetPtr->max_crew);
		readValue(fh, &FleetPtr->growth);
		readValue(fh, &FleetPtr->max_energy);
		readValue(fh, &FleetPtr->loc.x);
		readValue(fh, &FleetPtr->loc.y);

		readValue(fh, &FleetPtr->actual_strength);
		readValue(fh, &FleetPtr->known_strength);
		readValue(fh, &FleetPtr->known_loc.x);
		readValue(fh, &FleetPtr->known_loc.y);
		readValue(fh, &FleetPtr->growth_err_term);
		readValue(fh, &FleetPtr->func_index);
		readValue(fh, &FleetPtr->dest_loc.x);
		readValue(fh, &FleetPtr->dest_loc.y);

		UnlockFleetInfo(pQueue, hStarShip);
	}
}

static void
LoadGroupQueue(uqm::io::File& fh, QUEUE* pQueue, uint32_t size)
{
	uint16_t num_links = size / 13;

	while (num_links--)
	{
		HIPGROUP hGroup;
		IP_GROUP* GroupPtr;

		hGroup = BuildGroup(pQueue, 0);
		GroupPtr = LockIpGroup(pQueue, hGroup);

		readValue(fh, &GroupPtr->group_counter);
		readValue(fh, &GroupPtr->race_id);
		readValue(fh, &GroupPtr->sys_loc);
		readValue(fh, &GroupPtr->task);
		readValue(fh, &GroupPtr->in_system); /* was crew_level */
		readValue(fh, &GroupPtr->dest_loc);
		readValue(fh, &GroupPtr->orbit_pos);
		readValue(fh, &GroupPtr->group_id); /* was max_energy */
		readValue(fh, &GroupPtr->loc.x);
		readValue(fh, &GroupPtr->loc.y);

		UnlockIpGroup(pQueue, hGroup);
	}
}

static void
LoadEncounter(ENCOUNTER* EncounterPtr, uqm::io::File& fh, bool try_core)
{
	uint16_t i;

	EncounterPtr->pred = 0;
	EncounterPtr->succ = 0;
	EncounterPtr->hElement = 0;
	readValue(fh, &EncounterPtr->transition_state);
	readValue(fh, &EncounterPtr->origin.x);
	readValue(fh, &EncounterPtr->origin.y);
	readValue(fh, &EncounterPtr->radius);
	// former STAR_DESC fields
	readValue(fh, &EncounterPtr->loc_pt.x);
	readValue(fh, &EncounterPtr->loc_pt.y);
	readValue(fh, &EncounterPtr->race_id);
	readValue(fh, &EncounterPtr->num_ships);
	readValue(fh, &EncounterPtr->flags);

	// Load each entry in the BRIEF_SHIP_INFO array
	for (i = 0; i < MAX_HYPER_SHIPS; i++)
	{
		BRIEF_SHIP_INFO* ShipInfo = &EncounterPtr->ShipList[i];

		readValue(fh, &ShipInfo->race_id);
		readValue(fh, &ShipInfo->crew_level);
		readValue(fh, &ShipInfo->max_crew);
		readValue(fh, &ShipInfo->max_energy);
	}

	// Load the stuff after the BRIEF_SHIP_INFO array
	readValue(fh, &EncounterPtr->log_x);
	readValue(fh, &EncounterPtr->log_y);

	if (try_core)
	{ // Use old Log X to Universe code to get proper coordinates from core saves
		EncounterPtr->log_x = RES_SCALE(UNIVERSE_TO_LOGX(oldLogxToUniverse(EncounterPtr->log_x)));
		EncounterPtr->log_y = RES_SCALE(UNIVERSE_TO_LOGY(oldLogyToUniverse(EncounterPtr->log_y)));
	}
	else
	{ // JMS: Let's make savegames work even between different resolution modes.
		EncounterPtr->log_x <<= RESOLUTION_FACTOR;
		EncounterPtr->log_y <<= RESOLUTION_FACTOR;
	}
}

static void
LoadEvent(EVENT* EventPtr, uqm::io::File& fh)
{
	EventPtr->pred = 0;
	EventPtr->succ = 0;
	readValue(fh, &EventPtr->day_index);
	readValue(fh, &EventPtr->month_index);
	readValue(fh, &EventPtr->year_index);
	readValue(fh, &EventPtr->func_index);
}

static void
LoadClockState(CLOCK_STATE* ClockPtr, uqm::io::File& fh)
{
	readValue(fh, &ClockPtr->day_index);
	readValue(fh, &ClockPtr->month_index);
	readValue(fh, &ClockPtr->year_index);
	readValue(fh, &ClockPtr->tick_count);
	readValue(fh, &ClockPtr->day_in_ticks);
}

static bool
LoadGameState(GAME_STATE* GSPtr, uqm::io::File& fh, bool try_core)
{
	uint32_t magic;
	legacySave = try_core;
	readValue(fh, &magic);
	if (magic != GLOBAL_STATE_TAG)
	{
		return false;
	}
	readValue(fh, &magic);
	if (magic != 75)
	{
		/* Chunk is the wrong size. */
		return false;
	}
	readValue(fh, &GSPtr->glob_flags);
	readValue(fh, &GSPtr->CrewCost);
	readValue(fh, &GSPtr->FuelCost);

	readValueArray<uint8_t>(fh, GSPtr->ModuleCost);
	readValueArray<uint8_t>(fh, GSPtr->ElementWorth);
	readValue(fh, &GSPtr->CurrentActivity);

	LoadClockState(&GSPtr->GameClock, fh);

	readValue(fh, &GSPtr->autopilot.x);
	readValue(fh, &GSPtr->autopilot.y);
	readValue(fh, &GSPtr->ip_location.x);
	readValue(fh, &GSPtr->ip_location.y);
	/* STAMP ShipStamp */
	readValue(fh, &GSPtr->ShipStamp.origin.x);
	readValue(fh, &GSPtr->ShipStamp.origin.y);
	readValue(fh, &GSPtr->ShipFacing);
	readValue(fh, &GSPtr->ip_planet);
	readValue(fh, &GSPtr->in_orbit);

	// Let's make savegames work even between different resolution modes.
	GSPtr->ShipStamp.origin.x <<= RESOLUTION_FACTOR;
	GSPtr->ShipStamp.origin.y <<= RESOLUTION_FACTOR;

	/* VELOCITY_DESC velocity */
	readValue(fh, &GSPtr->velocity.TravelAngle);
	readValue(fh, &GSPtr->velocity.vector.width);
	readValue(fh, &GSPtr->velocity.vector.height);
	readValue(fh, &GSPtr->velocity.fract.width);
	readValue(fh, &GSPtr->velocity.fract.height);
	readValue(fh, &GSPtr->velocity.error.width);
	readValue(fh, &GSPtr->velocity.error.height);
	readValue(fh, &GSPtr->velocity.incr.width);
	readValue(fh, &GSPtr->velocity.incr.height);

	if (lowByte(GSPtr->CurrentActivity) != IN_INTERPLANETARY)
	{ // Let's make savegames work even between different resolution modes.
		GSPtr->velocity.vector.width <<= RESOLUTION_FACTOR;
		GSPtr->velocity.vector.height <<= RESOLUTION_FACTOR;
		GSPtr->velocity.fract.width <<= RESOLUTION_FACTOR;
		GSPtr->velocity.fract.height <<= RESOLUTION_FACTOR;
		GSPtr->velocity.error.width <<= RESOLUTION_FACTOR;
		GSPtr->velocity.error.height <<= RESOLUTION_FACTOR;
		GSPtr->velocity.incr.width <<= RESOLUTION_FACTOR;
		GSPtr->velocity.incr.height <<= RESOLUTION_FACTOR;
	}

	readValue(fh, &magic);
	if (magic != GAME_STATE_TAG)
	{
		return false;
	}
	{
		uint8_t* buf;
		bool result;
		int rev;
		size_t gameStateByteCount;

		readValue(fh, &magic);
		rev = (try_core ? 0 : getGameStateRevByBytes(gameStateBitMap, magic));
		gameStateByteCount = (totalBitsForGameState(gameStateBitMap, rev) + 7) >> 3;

		if (rev < 0 || magic < gameStateByteCount)
		{
			uqm::log::error("Warning: Savegame is corrupt: saved game "
							"state is too small.");
			return false;
		}

		uqm::log::debug("Detected save game state rev {}: {}",
						rev, gameStateBitMapRevTag[rev]);

		buf = (uint8_t*)HMalloc(gameStateByteCount);
		if (buf == nullptr)
		{
			uqm::log::error("Warning: Cannot allocate enough bytes for "
							"the saved game state ({} bytes).",
							(unsigned long)gameStateByteCount);
			return false;
		}

		readValueArray<uint8_t>(fh, {buf, (uint16_t)gameStateByteCount});
		result = deserialiseGameState(gameStateBitMap, buf, gameStateByteCount, rev);
		HFree(buf);
		if (result == false)
		{
			// An error message is already printed.
			return false;
		}

		if (rev < 2)
		{
			GSPtr->glob_flags = NUM_READ_SPEEDS >> 1;
		}

		if (rev < 3)
		{
			ZeroLastLoc();
		}

		if (rev < 4)
		{
			ZeroAdvancedAutoPilot();
			ZeroAdvancedQuasiPilot();
		}

		if (magic > gameStateByteCount)
		{
			skipN<uint8_t>(fh, (uint16_t)(magic - gameStateByteCount));
		}
	}
	return true;
}

static bool
LoadSisState(SIS_STATE* SSPtr, uqm::io::File& fp, bool try_core,
			 int legacyMM)
{
	uint16_t SisNameSize = (legacyMM == 1 || try_core) ?
							   LEGACY_SIS_NAME_SIZE :
							   SIS_NAME_SIZE;

	if (readValue(fp, &SSPtr->log_x) == false
		|| readValue(fp, &SSPtr->log_y) == false
		|| readValue(fp, &SSPtr->ResUnits) == false
		|| readValue(fp, &SSPtr->FuelOnBoard) == false
		|| readValue(fp, &SSPtr->CrewEnlisted) == false
		|| readValue(fp, &SSPtr->TotalElementMass) == false
		|| readValue(fp, &SSPtr->TotalBioMass) == false
		|| readValueArray<uint8_t>(fp, SSPtr->ModuleSlots) == false
		|| readValueArray<uint8_t>(fp, SSPtr->DriveSlots) == false
		|| readValueArray<uint8_t>(fp, SSPtr->JetSlots) == false
		|| readValue(fp, &SSPtr->NumLanders) == false
		|| readValueArray<uint16_t>(fp, SSPtr->ElementAmounts) == false
		|| read_str(fp, SSPtr->ShipName) == false
		|| read_str(fp, SSPtr->CommanderName) == false
		|| read_str(fp, SSPtr->PlanetName) == false
		|| (!try_core && (readValue(fp, &SSPtr->Difficulty) == false))
		|| (!try_core && (readValue(fp, &SSPtr->Extended) == false))
		|| (!try_core && (readValue(fp, &SSPtr->Nomad) == false))
		|| (!try_core && (readValue(fp, &SSPtr->Seed) == false))
		|| (!try_core && !(legacyMM > 0) && (readValue(fp, &SSPtr->ShipSeed) == false)))
	{
		return false;
	}
	else
	{
		if (try_core)
		{ // Use old Log X to Universe code to get proper coordinates from core saves
			SSPtr->log_x = UNIVERSE_TO_LOGX(oldLogxToUniverse(SSPtr->log_x));
			SSPtr->log_y = UNIVERSE_TO_LOGY(oldLogyToUniverse(SSPtr->log_y));
		}
		else
		{ // JMS: Let's make savegames work even between different resolution modes.
			SSPtr->log_x <<= RESOLUTION_FACTOR;
			SSPtr->log_y <<= RESOLUTION_FACTOR;
		}
		return true;
	}
}

static bool
LoadSummary(SUMMARY_DESC* SummPtr, uqm::io::File& fp, bool try_core)
{
	uint32_t magic;
	uint32_t magicTag = try_core ? SAVEFILE_TAG : MMV4_TAG;
	uint32_t nameSize = 0;
	int legacyMM = false;
	if (!readValue(fp, &magic))
	{
		return false;
	}
	if (magic == magicTag || magic == MEGA_TAG || magic == MMV3_TAG)
	{
		legacyMM = magic == MEGA_TAG ? 1 : magic == MMV3_TAG ? 2 :
															   0;

		if (!readValue(fp, &magic) || magic != SUMMARY_TAG)
		{
			return false;
		}
		if (!readValue(fp, &magic) || magic < 160)
		{
			return false;
		}
		nameSize = magic - 160;
	}
	else
	{
		return false;
	}

	if (!LoadSisState(&SummPtr->SS, fp, try_core, legacyMM))
	{
		return false;
	}

	SummPtr->SS.SaveVersion = 0;

	if (try_core)
	{ // Sanitize seed, difficulty, extended, and nomad variables
		SummPtr->SS.Difficulty = uqm::Difficulty::Normal;
		SummPtr->SS.Seed = 0;
		SummPtr->SS.Extended = 0;
		SummPtr->SS.Nomad = uqm::NomadMode::Off;
		SummPtr->SS.SaveVersion = 1;
	}
	if (try_core || legacyMM > 0)
	{
		SummPtr->SS.ShipSeed = 0;
	}

	if (readValue(fp, &SummPtr->Activity) == false
		|| readValue(fp, &SummPtr->Flags) == false
		|| readValue(fp, &SummPtr->day_index) == false
		|| readValue(fp, &SummPtr->month_index) == false
		|| readValue(fp, &SummPtr->year_index) == false
		|| readValue(fp, &SummPtr->MCreditLo) == false
		|| readValue(fp, &SummPtr->MCreditHi) == false
		|| readValue(fp, &SummPtr->NumShips) == false
		|| readValue(fp, &SummPtr->NumDevices) == false
		|| readValueArray<uint8_t>(fp, SummPtr->ShipList) == false
		|| readValueArray<uint8_t>(fp, SummPtr->DeviceList) == false
		|| (!try_core && (readValue(fp, &SummPtr->res_factor) == false)))
	{
		return false;
	}

	IndependantResFactor = !try_core ? SummPtr->res_factor : 0;

	if (nameSize < SAVE_NAME_SIZE)
	{
		if (!readValueArray<char>(fp, {SummPtr->SaveName, nameSize}))
		{
			return false;
		}
		SummPtr->SaveName[nameSize] = 0;
	}
	else
	{
		uint32_t remaining = nameSize - SAVE_NAME_SIZE + 1;
		if (!readValueArray<char>(fp, {SummPtr->SaveName, SAVE_NAME_SIZE - 1}))
		{
			return false;
		}
		SummPtr->SaveName[SAVE_NAME_SIZE - 1] = 0;
		if (!skipN<char>(fp, remaining))
		{
			return false;
		}
	}
	return true;
}

static void
LoadStarDesc(STAR_DESC* SDPtr, uqm::io::File& fh)
{
	readValue(fh, &SDPtr->star_pt.x);
	readValue(fh, &SDPtr->star_pt.y);
	readValue(fh, &SDPtr->Type);
	readValue(fh, &SDPtr->Index);
	readValue(fh, &SDPtr->Prefix);
	readValue(fh, &SDPtr->Postfix);
}

static void
LoadScanInfo(uqm::io::File& fh, uint32_t flen)
{
	GAME_STATE_FILE* fp = OpenStateFile(STARINFO_FILE, "wb");
	if (fp)
	{
		while (flen)
		{
			uint32_t val;
			readValue(fh, &val);
			swrite_32(fp, val);
			flen -= 4;
		}
		CloseStateFile(fp);
	}
}

static void
LoadGroupList(uqm::io::File& fh, uint32_t chunksize)
{
	GAME_STATE_FILE* fp = OpenStateFile(RANDGRPINFO_FILE, "rb");
	if (fp)
	{
		GROUP_HEADER h;
		uint8_t LastEnc, NumGroups;
		int i;
		ReadGroupHeader(fp, &h);
		/* There's only supposed to be one of these, so group 0 should be
		 * zero here whenever we're here. We add the group list to the
		 * end here. */
		h.GroupOffset[0] = LengthStateFile(fp);
		SeekStateFile(fp, 0, SEEK_SET);
		WriteGroupHeader(fp, &h);
		SeekStateFile(fp, h.GroupOffset[0], SEEK_SET);
		readValue(fh, &LastEnc);
		NumGroups = (chunksize - 1) / 14;
		swrite_8(fp, LastEnc);
		swrite_8(fp, NumGroups);
		for (i = 0; i < NumGroups; ++i)
		{
			uint8_t race_outer;
			IP_GROUP ip;
			readValue(fh, &race_outer);
			readValue(fh, &ip.group_counter);
			readValue(fh, &ip.race_id);
			readValue(fh, &ip.sys_loc);
			readValue(fh, &ip.task);
			readValue(fh, &ip.in_system);
			readValue(fh, &ip.dest_loc);
			readValue(fh, &ip.orbit_pos);
			readValue(fh, &ip.group_id);
			readValue(fh, &ip.loc.x);
			readValue(fh, &ip.loc.y);

			swrite_8(fp, race_outer);
			WriteIpGroup(fp, &ip);
		}
		CloseStateFile(fp);
	}
}

static void
SetBattleGroupOffset(int encounterIndex, uint32_t offset)
{
	// The reason for this switch, even though the group offsets are
	// successive, is because SET_GAME_STATE is a #define, which stringizes
	// its first argument.
	switch (encounterIndex)
	{
		case 1:
			SET_GAME_STATE(SHOFIXTI_GRPOFFS, offset);
			break;
		case 2:
			SET_GAME_STATE(ZOQFOT_GRPOFFS, offset);
			break;
		case 3:
			SET_GAME_STATE(MELNORME0_GRPOFFS, offset);
			break;
		case 4:
			SET_GAME_STATE(MELNORME1_GRPOFFS, offset);
			break;
		case 5:
			SET_GAME_STATE(MELNORME2_GRPOFFS, offset);
			break;
		case 6:
			SET_GAME_STATE(MELNORME3_GRPOFFS, offset);
			break;
		case 7:
			SET_GAME_STATE(MELNORME4_GRPOFFS, offset);
			break;
		case 8:
			SET_GAME_STATE(MELNORME5_GRPOFFS, offset);
			break;
		case 9:
			SET_GAME_STATE(MELNORME6_GRPOFFS, offset);
			break;
		case 10:
			SET_GAME_STATE(MELNORME7_GRPOFFS, offset);
			break;
		case 11:
			SET_GAME_STATE(MELNORME8_GRPOFFS, offset);
			break;
		case 12:
			SET_GAME_STATE(URQUAN_PROBE_GRPOFFS, offset);
			break;
		case 13:
			SET_GAME_STATE(COLONY_GRPOFFS, offset);
			break;
		case 14:
			SET_GAME_STATE(SAMATRA_GRPOFFS, offset);
			break;
		default:
			uqm::log::warn("SetBattleGroupOffset: invalid encounter "
						   "index.\n");
			break;
	}
}

static void
LoadBattleGroup(uqm::io::File& fh, uint32_t chunksize)
{
	GAME_STATE_FILE* fp;
	GROUP_HEADER h;
	uint32_t encounter, offset;
	uint8_t current;
	int i;

	readValue(fh, &encounter);
	readValue(fh, &current);
	chunksize -= 5;
	if (encounter)
	{
		/* This is a defined group, so it's new */
		fp = OpenStateFile(DEFGRPINFO_FILE, "rb");
		offset = LengthStateFile(fp);
		memset(&h, 0, sizeof(GROUP_HEADER));
	}
	else
	{
		/* This is the random group. Load in what was there,
		 * as we might have already seen the Group List. */
		fp = OpenStateFile(RANDGRPINFO_FILE, "rb");
		current = false;
		offset = 0;
		ReadGroupHeader(fp, &h);
	}
	if (!fp)
	{
		skipN<char>(fh, chunksize);
		return;
	}
	readValue(fh, &h.star_index);
	readValue(fh, &h.day_index);
	readValue(fh, &h.month_index);
	readValue(fh, &h.year_index);
	readValue(fh, &h.NumGroups);
	chunksize -= 7;
	/* Write out the half-finished state file so that we can use
	 * the file size to compute group offsets */
	SeekStateFile(fp, offset, SEEK_SET);
	WriteGroupHeader(fp, &h);
	for (i = 1; i <= h.NumGroups; ++i)
	{
		int j;
		uint8_t icon, NumShips;
		readValue(fh, &icon);
		readValue(fh, &NumShips);
		chunksize -= 2;
		h.GroupOffset[i] = LengthStateFile(fp);
		SeekStateFile(fp, h.GroupOffset[i], SEEK_SET);
		swrite_8(fp, icon);
		swrite_8(fp, NumShips);
		for (j = 0; j < NumShips; ++j)
		{
			uint8_t race_outer;
			SHIP_FRAGMENT sf;
			readValue(fh, &race_outer);
			readValue(fh, &sf.captains_name_index);
			readValue(fh, &sf.race_id);
			readValue(fh, &sf.index);
			readValue(fh, &sf.crew_level);
			readValue(fh, &sf.max_crew);
			readValue(fh, &sf.energy_level);
			readValue(fh, &sf.max_energy);
			chunksize -= 10;

			swrite_8(fp, race_outer);
			WriteShipFragment(fp, &sf);
		}
	}
	/* Now that the GroupOffset array is properly initialized,
	 * write the header back out. */
	SeekStateFile(fp, offset, SEEK_SET);
	WriteGroupHeader(fp, &h);
	CloseStateFile(fp);
	/* And update the gamestate accordingly, if we're a defined group. */
	if (encounter)
	{
		SetBattleGroupOffset(encounter, offset);
		if (current)
		{
			GLOBAL(BattleGroupRef) = offset;
		}
	}
	/* Consistency check. */
	if (chunksize)
	{
		uqm::log::warn("BattleGroup chunk mis-sized!");
	}
}

bool LoadCoreGame(uint16_t which_game, SUMMARY_DESC* SummPtr)
{
	uqstl::optional<uqm::io::File> in_fp {};
	char file[PATH_MAX] {};
	SUMMARY_DESC loc_sd;

	fmt::format_to_n(file, sizeof(file) - 1, "uqmsave.{:02}", which_game);
	in_fp = res_OpenResFile(uqm::saveDir(), file, uqm::io::OpenMode::Read);
	if (!in_fp)
	{
		return LoadLegacyGame(which_game, SummPtr, false);
	}

	if (!LoadSummary(&loc_sd, *in_fp, true))
	{
		in_fp.reset();
		return LoadLegacyGame(which_game, SummPtr, false);
	}

	if (!SummPtr)
	{
		SummPtr = &loc_sd;
	}
	else
	{ // only need summary for displaying to user
		memcpy(SummPtr, &loc_sd, sizeof(*SummPtr));
		in_fp.reset();
		return true;
	}

	if (!LoadGame(which_game, SummPtr, uqstl::move(in_fp), true))
	{
		in_fp.reset();
		return false;
	}

	return true;
}

bool LoadGame(uint16_t which_game, SUMMARY_DESC* SummPtr, uqstl::optional<uqm::io::File> in_fp, bool try_core)
{
	char file[PATH_MAX] {};
	SUMMARY_DESC loc_sd;
	uint16_t num_links;
	STAR_DESC SD;
	ACTIVITY Activity;
	uint32_t chunk, chunkSize;
	bool first_group_spec = true;

	if (!try_core)
	{
		fmt::format_to_n(file, sizeof(file) - 1, "uqmsave.{:02}", which_game);
		in_fp = res_OpenResFile(uqm::saveDir(), file, uqm::io::OpenMode::Read);
		if (!in_fp)
		{
			return LoadLegacyGame(which_game, SummPtr, false);
		}

		if (!LoadSummary(&loc_sd, *in_fp, false))
		{
			in_fp.reset();
			return LoadCoreGame(which_game, SummPtr);
		}

		if (!SummPtr)
		{
			SummPtr = &loc_sd;
		}
		else
		{ // only need summary for displaying to user
			memcpy(SummPtr, &loc_sd, sizeof(*SummPtr));
			in_fp.reset();
			return true;
		}
	}

	GlobData.SIS_state = SummPtr->SS;

	uqm::UQMOptions::edit().customSeed = GLOBAL_SIS(Seed);
	uqm::UQMOptions::edit().shipSeed = (GLOBAL_SIS(ShipSeed) != 0);
	ReloadMasterShipList(nullptr);
	LoadFleetInfo();

	ReinitQueue(&GLOBAL(GameClock.event_q));
	ReinitQueue(&GLOBAL(encounter_q));
	ReinitQueue(&GLOBAL(ip_group_q));
	ReinitQueue(&GLOBAL(npc_built_ship_q));
	ReinitQueue(&GLOBAL(built_ship_q));
	ReinitQueue(&GLOBAL(stowed_ship_q));

	uninitEventSystem();
	luaUqm_uninitState();
	luaUqm_initState();
	initEventSystem();

	Activity = GLOBAL(CurrentActivity);
	if (!LoadGameState(&GlobData.Game_state, *in_fp, try_core))
	{
		in_fp.reset();
		return false;
	}
	NextActivity = GLOBAL(CurrentActivity);
	GLOBAL(CurrentActivity) = Activity;

	chunk = 0;
	while (true)
	{
		if (!readValue(*in_fp, &chunk))
		{
			break;
		}
		if (!readValue(*in_fp, &chunkSize))
		{
			in_fp.reset();
			return false;
		}
		switch (chunk)
		{
			case RACE_Q_TAG:
				LoadRaceQueue(*in_fp, &GLOBAL(avail_race_q), chunkSize);
				break;
			case IP_GRP_Q_TAG:
				LoadGroupQueue(*in_fp, &GLOBAL(ip_group_q), chunkSize);
				break;
			case ENCOUNTERS_TAG:
				num_links = chunkSize / 65;
				while (num_links--)
				{
					HENCOUNTER hEncounter;
					ENCOUNTER* EncounterPtr;

					hEncounter = AllocEncounter();
					LockEncounter(hEncounter, &EncounterPtr);

					LoadEncounter(EncounterPtr, *in_fp, try_core);

					UnlockEncounter(hEncounter);
					PutEncounter(hEncounter);
				}
				break;
			case EVENTS_TAG:
				num_links = chunkSize / 5;
#ifdef DEBUG_LOAD
				uqm::log::debug("EVENTS:");
#endif /* DEBUG_LOAD */
				while (num_links--)
				{
					HEVENT hEvent;
					EVENT* EventPtr;
					bool DeCleanse = false;

					hEvent = AllocEvent();
					LockEvent(hEvent, &EventPtr);

					LoadEvent(EventPtr, *in_fp);

#ifdef DEBUG_LOAD
					uqm::log::debug("\t{}/{}/{} -- {}",
									EventPtr->month_index,
									EventPtr->day_index,
									EventPtr->year_index,
									EventPtr->func_index);
#endif /* DEBUG_LOAD */
					if (uqm::UQMOptions::read().deCleansing && EventPtr->func_index == KOHR_AH_VICTORIOUS_EVENT)
					{
						UnlockEvent(hEvent);
						uqm::log::debug("EventPtr->year_index: {}\n", EventPtr->year_index);

						if (EventPtr->year_index == 2158)
						{
							FreeEvent(hEvent);
							DeCleanse = true;
						}
						continue;
					}

					UnlockEvent(hEvent);
					PutEvent(hEvent);

					if (uqm::UQMOptions::read().deCleansing && DeCleanse)
					{
						AddEvent(ABSOLUTE_EVENT, 2, 17, START_YEAR + YEARS_TO_KOHRAH_VICTORY,
								 KOHR_AH_VICTORIOUS_EVENT);
					}
				}
				break;
			case STAR_TAG:
				LoadStarDesc(&SD, *in_fp);
				loadGameCheats();
				break;
			case NPC_SHIP_Q_TAG:
				LoadShipQueue(*in_fp, &GLOBAL(npc_built_ship_q), chunkSize);
				break;
			case SHIP_Q_TAG:
				LoadShipQueue(*in_fp, &GLOBAL(built_ship_q), chunkSize);
				break;
			case STOWED_Q_TAG:
				LoadShipQueue(*in_fp, &GLOBAL(stowed_ship_q), chunkSize);
				break;
			case SCAN_TAG:
				LoadScanInfo(*in_fp, chunkSize);
				break;
			case GROUP_LIST_TAG:
				if (first_group_spec)
				{
					InitGroupInfo(true);
					GLOBAL(BattleGroupRef) = 0;
					first_group_spec = false;
				}
				LoadGroupList(*in_fp, chunkSize);
				break;
			case BATTLE_GROUP_TAG:
				if (first_group_spec)
				{
					InitGroupInfo(true);
					GLOBAL(BattleGroupRef) = 0;
					first_group_spec = false;
				}
				LoadBattleGroup(*in_fp, chunkSize);
				break;
			default:
				uqm::log::debug("Skipping chunk of tag %08X (size {})", chunk, chunkSize);
				if (!skipN<char>(*in_fp, chunkSize))
				{
					in_fp.reset();
					return false;
				}
				break;
		}
	}
	in_fp.reset();

	EncounterGroup = 0;
	EncounterRace = -1;

	ReinitQueue(&race_q[0]);
	ReinitQueue(&race_q[1]);
	CurStarDescPtr = FindStar(nullptr, &SD.star_pt, 0, 0);
	if (!(NextActivity & START_ENCOUNTER)
		&& lowByte(NextActivity) == IN_INTERPLANETARY)
	{
		NextActivity |= START_INTERPLANETARY;
	}

	// Reset Debug Key
	DebugKeyPressed = false;
	// Set the SeedType flag and then start Starseed
	g_seedType = (uqm::SeedType)GET_GAME_STATE(SEED_TYPE);
	// Assuming load from older version, optSeedType should be 0 (none)
	// If the seed is also 0 it's a really old save file and prime seed
	// Otherwise if the seed isn't prime, optSeedType goes to 1 (planet)
	if (g_seedType == uqm::SeedType::Prime)
	{
		if (uqm::UQMOptions::read().customSeed == 0)
		{
			GLOBAL_SIS(Seed) = uqm::UQMOptions::edit().customSeed = PrimeA;
		}
		else if (uqm::UQMOptions::read().customSeed != PrimeA)
		{
			SET_GAME_STATE(SEED_TYPE, g_seedType = uqm::SeedType::Planet);
		}
	}
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "Loading game with seed type {}, {}\n",
			   optSeedType,
			   (optSeedType == 0) ? "Default Game Mode (no seeding)" :
			   (optSeedType == 1) ? "Seed Planets (SysGenRNG only)" :
			   (optSeedType == 2) ? "MRQ (Melnorme, Rainbow, and Quasispace)" :
			   (optSeedType == 3) ? "Seed Plot (Starseed)" :
									"UNKNOWN");
#endif
	// During load game we do not want to bail on a bad seed (the argument).
	// If it fails to load the seed, it will return false.  Also, we need to
	// call this on all load games to reset the starmap as needed to the
	// proper state, including Prime seed.
	return InitStarseed(false);
}
