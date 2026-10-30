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
#include "globdata.h"
#include "options.h"
#include "save.h"
#include "setup.h"
#include "state.h"
#include "grpintrn.h"
#include "uqmdebug.h"
#include "gameev.h"
#include "libs/threadlib.h"
#include "core/log/log.h"
#include "core/stl/stl.h"
#include "master.h"
#include "libs/file/textfile.h"
#include "core/string/StringUtils.h"

#include <yyjson.h>

#include <utility>

//#define DEBUG_LOAD

ACTIVITY NextActivity;
uint8_t IndependantResFactor;

namespace
{

template <typename T>
bool toInt(yyjson_val* val, T& out)
{
	if constexpr (uqstl::is_enum_v<T>)
	{
		uqstl::underlying_type_t<T> raw {};
		if (!toInt(val, raw))
		{
			return false;
		}
		out = static_cast<T>(raw);
		return true;
	}
	else if (yyjson_is_uint(val))
	{
		const uint64_t value {yyjson_get_uint(val)};
		if (!uqstl::in_range<T>(value))
		{
			return false;
		}
		out = static_cast<T>(value);
		return true;
	}
	else if (yyjson_is_sint(val))
	{
		const int64_t value {yyjson_get_sint(val)};
		if (!uqstl::in_range<T>(value))
		{
			return false;
		}
		out = static_cast<T>(value);
		return true;
	}
	return false;
}

// Reads values from a save game document. Logs the first error only.
class SaveReader
{
public:
	bool ok() const
	{
		return m_ok;
	}

	void fail(const char* key)
	{
		if (m_ok)
		{
			uqm::log::error("Save game: value '{}' is missing or not valid.", key);
		}
		m_ok = false;
	}

	template <typename T>
	void get(yyjson_val* obj, const char* key, T& out)
	{
		if (!toInt(yyjson_obj_get(obj, key), out))
		{
			fail(key);
		}
	}

	bool getBool(yyjson_val* obj, const char* key)
	{
		yyjson_val* val {yyjson_obj_get(obj, key)};
		if (val && !yyjson_is_bool(val))
		{
			fail(key);
		}
		return yyjson_get_bool(val);
	}

	template <typename X, typename Y>
	void getPoint(yyjson_val* obj, const char* key, X& x, Y& y)
	{
		yyjson_val* point {object(obj, key)};
		get(point, "x", x);
		get(point, "y", y);
	}

	// Returns the number of values read.
	template <typename T>
	size_t getArray(yyjson_val* obj, const char* key, uqstl::span<T> out)
	{
		yyjson_val* list {yyjson_obj_get(obj, key)};
		if (!yyjson_is_arr(list) || yyjson_arr_size(list) > out.size())
		{
			fail(key);
			return 0;
		}

		size_t index {};
		size_t count {};
		yyjson_val* val {};
		yyjson_arr_foreach(list, index, count, val)
		{
			if (!toInt(val, out[index]))
			{
				fail(key);
				return 0;
			}
		}
		return count;
	}

	template <typename T>
	void getFixedArray(yyjson_val* obj, const char* key, uqstl::span<T> out)
	{
		if (getArray(obj, key, out) != out.size())
		{
			fail(key);
		}
	}

	void getString(yyjson_val* obj, const char* key, uqstl::span<char> out)
	{
		yyjson_val* val {yyjson_obj_get(obj, key)};
		if (!yyjson_is_str(val))
		{
			fail(key);
			return;
		}
		uqm::strncpy_safe(out, {yyjson_get_str(val), yyjson_get_len(val)});
	}

	yyjson_val* object(yyjson_val* obj, const char* key)
	{
		yyjson_val* val {yyjson_obj_get(obj, key)};
		if (!yyjson_is_obj(val))
		{
			fail(key);
			return nullptr;
		}
		return val;
	}

	// Returns nullptr if the array is not there.
	yyjson_val* optionalArray(yyjson_val* obj, const char* key)
	{
		yyjson_val* val {yyjson_obj_get(obj, key)};
		if (val && !yyjson_is_arr(val))
		{
			fail(key);
			return nullptr;
		}
		return val;
	}

private:
	bool m_ok {true};
};

void LoadShipFragment(SaveReader& reader, yyjson_val* obj, SHIP_FRAGMENT& frag)
{
	reader.get(obj, "captains_name_index", frag.captains_name_index);
	reader.get(obj, "race_id", frag.race_id);
	reader.get(obj, "index", frag.index);
	reader.get(obj, "crew_level", frag.crew_level);
	reader.get(obj, "max_crew", frag.max_crew);
	reader.get(obj, "energy_level", frag.energy_level);
	reader.get(obj, "max_energy", frag.max_energy);
}

void LoadIpGroup(SaveReader& reader, yyjson_val* obj, IP_GROUP& group)
{
	reader.get(obj, "group_counter", group.group_counter);
	reader.get(obj, "race_id", group.race_id);
	reader.get(obj, "sys_loc", group.sys_loc);
	reader.get(obj, "task", group.task);
	reader.get(obj, "in_system", group.in_system);
	reader.get(obj, "dest_loc", group.dest_loc);
	reader.get(obj, "orbit_pos", group.orbit_pos);
	reader.get(obj, "group_id", group.group_id);
	reader.getPoint(obj, "loc", group.loc.x, group.loc.y);
}

void LoadShipQueue(SaveReader& reader, yyjson_val* root, const char* key, QUEUE* pQueue)
{
	size_t index {};
	size_t count {};
	yyjson_val* item {};
	yyjson_arr_foreach(reader.optionalArray(root, key), index, count, item)
	{
		uint16_t raceId {};
		reader.get(item, "race_id", raceId);
		const HSHIPFRAG hStarShip {reader.ok() ? CloneShipFragment(static_cast<RACE_ID>(raceId), pQueue, 0) : HSHIPFRAG {}};
		if (!hStarShip)
		{
			reader.fail(key);
			return;
		}

		SHIP_FRAGMENT* FragPtr {LockShipFrag(pQueue, hStarShip)};
		LoadShipFragment(reader, item, *FragPtr);
		UnlockShipFrag(pQueue, hStarShip);
	}
}

void LoadRaceQueue(SaveReader& reader, yyjson_val* root, QUEUE* pQueue)
{
	size_t index {};
	size_t count {};
	yyjson_val* item {};
	yyjson_arr_foreach(reader.optionalArray(root, "raceQueue"), index, count, item)
	{
		uint16_t Index {};
		reader.get(item, "Index", Index);
		const HFLEETINFO hStarShip {reader.ok() ? GetStarShipFromIndex(pQueue, Index) : HFLEETINFO {}};
		if (!hStarShip)
		{
			reader.fail("raceQueue");
			return;
		}

		FLEET_INFO* FleetPtr {LockFleetInfo(pQueue, hStarShip)};
		reader.get(item, "allied_state", FleetPtr->allied_state);
		if (FleetPtr->allied_state > 2)
		{
			FleetPtr->allied_state = BAD_GUY;
		}
		reader.get(item, "days_left", FleetPtr->days_left);
		reader.get(item, "growth_fract", FleetPtr->growth_fract);
		reader.get(item, "crew_level", FleetPtr->crew_level);
		reader.get(item, "max_crew", FleetPtr->max_crew);
		reader.get(item, "growth", FleetPtr->growth);
		reader.get(item, "max_energy", FleetPtr->max_energy);
		reader.getPoint(item, "loc", FleetPtr->loc.x, FleetPtr->loc.y);
		reader.get(item, "actual_strength", FleetPtr->actual_strength);
		reader.get(item, "known_strength", FleetPtr->known_strength);
		reader.getPoint(item, "known_loc", FleetPtr->known_loc.x, FleetPtr->known_loc.y);
		reader.get(item, "growth_err_term", FleetPtr->growth_err_term);
		reader.get(item, "func_index", FleetPtr->func_index);
		reader.getPoint(item, "dest_loc", FleetPtr->dest_loc.x, FleetPtr->dest_loc.y);
		UnlockFleetInfo(pQueue, hStarShip);
	}
}

void LoadGroupQueue(SaveReader& reader, yyjson_val* root, QUEUE* pQueue)
{
	size_t index {};
	size_t count {};
	yyjson_val* item {};
	yyjson_arr_foreach(reader.optionalArray(root, "ipGroupQueue"), index, count, item)
	{
		const HIPGROUP hGroup {BuildGroup(pQueue, 0)};
		IP_GROUP* GroupPtr {LockIpGroup(pQueue, hGroup)};
		LoadIpGroup(reader, item, *GroupPtr);
		UnlockIpGroup(pQueue, hGroup);
	}
}

void LoadEncounter(SaveReader& reader, yyjson_val* obj, ENCOUNTER& encounter)
{
	encounter.pred = 0;
	encounter.succ = 0;
	encounter.hElement = 0;
	reader.get(obj, "transition_state", encounter.transition_state);
	reader.getPoint(obj, "origin", encounter.origin.x, encounter.origin.y);
	reader.get(obj, "radius", encounter.radius);
	reader.getPoint(obj, "loc_pt", encounter.loc_pt.x, encounter.loc_pt.y);
	reader.get(obj, "race_id", encounter.race_id);
	reader.get(obj, "flags", encounter.flags);

	yyjson_val* ships {yyjson_obj_get(obj, "ships")};
	if (!yyjson_is_arr(ships) || yyjson_arr_size(ships) > MAX_HYPER_SHIPS)
	{
		reader.fail("ships");
		return;
	}
	encounter.num_ships = static_cast<uint8_t>(yyjson_arr_size(ships));
	size_t index {};
	size_t count {};
	yyjson_val* ship {};
	yyjson_arr_foreach(ships, index, count, ship)
	{
		BRIEF_SHIP_INFO& ShipInfo {encounter.ShipList[index]};
		reader.get(ship, "race_id", ShipInfo.race_id);
		reader.get(ship, "crew_level", ShipInfo.crew_level);
		reader.get(ship, "max_crew", ShipInfo.max_crew);
		reader.get(ship, "max_energy", ShipInfo.max_energy);
	}

	reader.getPoint(obj, "log", encounter.log_x, encounter.log_y);
	// Let's make savegames work even between different resolution modes.
	encounter.log_x <<= RESOLUTION_FACTOR;
	encounter.log_y <<= RESOLUTION_FACTOR;
}

void LoadEncounters(SaveReader& reader, yyjson_val* root)
{
	size_t index {};
	size_t count {};
	yyjson_val* item {};
	yyjson_arr_foreach(reader.optionalArray(root, "encounters"), index, count, item)
	{
		const HENCOUNTER hEncounter {AllocEncounter()};
		ENCOUNTER* EncounterPtr {};
		LockEncounter(hEncounter, &EncounterPtr);
		*EncounterPtr = ENCOUNTER {};
		LoadEncounter(reader, item, *EncounterPtr);
		UnlockEncounter(hEncounter);
		PutEncounter(hEncounter);
	}
}

void LoadEvents(SaveReader& reader, yyjson_val* root)
{
	size_t index {};
	size_t count {};
	yyjson_val* item {};
	yyjson_arr_foreach(reader.optionalArray(root, "events"), index, count, item)
	{
		const HEVENT hEvent {AllocEvent()};
		EVENT* EventPtr {};
		bool DeCleanse {false};
		LockEvent(hEvent, &EventPtr);

		EventPtr->pred = 0;
		EventPtr->succ = 0;
		reader.get(item, "day_index", EventPtr->day_index);
		reader.get(item, "month_index", EventPtr->month_index);
		reader.get(item, "year_index", EventPtr->year_index);
		reader.get(item, "func_index", EventPtr->func_index);

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
}

void LoadClockState(SaveReader& reader, yyjson_val* obj, CLOCK_STATE& clock)
{
	yyjson_val* clockObj {reader.object(obj, "GameClock")};
	reader.get(clockObj, "day_index", clock.day_index);
	reader.get(clockObj, "month_index", clock.month_index);
	reader.get(clockObj, "year_index", clock.year_index);
	reader.get(clockObj, "tick_count", clock.tick_count);
	reader.get(clockObj, "day_in_ticks", clock.day_in_ticks);
}

// A game state that is not in the file is set to 0.
void LoadGameStateFlags(SaveReader& reader, yyjson_val* root)
{
	yyjson_val* flags {reader.object(root, "gameState")};
	for (const GameStateBitMap* bm {gameStateBitMap}; bm->name || bm->numBits; ++bm)
	{
		if (!bm->name)
		{
			continue;
		}

		uint32_t value {};
		yyjson_val* val {yyjson_obj_get(flags, bm->name)};
		if (val && !toInt(val, value))
		{
			reader.fail(bm->name);
		}
		setGameStateUint(bm->name, value);
	}
}

void LoadGameState(SaveReader& reader, yyjson_val* root, GAME_STATE& GS)
{
	yyjson_val* state {reader.object(root, "globalState")};
	reader.get(state, "glob_flags", GS.glob_flags);
	reader.get(state, "CrewCost", GS.CrewCost);
	reader.get(state, "FuelCost", GS.FuelCost);
	reader.getFixedArray<uint8_t>(state, "ModuleCost", GS.ModuleCost);
	reader.getFixedArray<uint8_t>(state, "ElementWorth", GS.ElementWorth);
	reader.get(state, "CurrentActivity", GS.CurrentActivity);
	LoadClockState(reader, state, GS.GameClock);
	reader.getPoint(state, "autopilot", GS.autopilot.x, GS.autopilot.y);
	reader.getPoint(state, "ip_location", GS.ip_location.x, GS.ip_location.y);
	reader.getPoint(state, "ShipOrigin", GS.ShipStamp.origin.x, GS.ShipStamp.origin.y);
	reader.get(state, "ShipFacing", GS.ShipFacing);
	reader.get(state, "ip_planet", GS.ip_planet);
	reader.get(state, "in_orbit", GS.in_orbit);

	// Let's make savegames work even between different resolution modes.
	GS.ShipStamp.origin.x <<= RESOLUTION_FACTOR;
	GS.ShipStamp.origin.y <<= RESOLUTION_FACTOR;

	VELOCITY_DESC& velocity {GS.velocity};
	yyjson_val* velocityObj {reader.object(state, "velocity")};
	reader.get(velocityObj, "TravelAngle", velocity.TravelAngle);
	reader.getPoint(velocityObj, "vector", velocity.vector.width, velocity.vector.height);
	reader.getPoint(velocityObj, "fract", velocity.fract.width, velocity.fract.height);
	reader.getPoint(velocityObj, "error", velocity.error.width, velocity.error.height);
	reader.getPoint(velocityObj, "incr", velocity.incr.width, velocity.incr.height);

	if (lowByte(GS.CurrentActivity) != IN_INTERPLANETARY)
	{ // Let's make savegames work even between different resolution modes.
		velocity.vector.width <<= RESOLUTION_FACTOR;
		velocity.vector.height <<= RESOLUTION_FACTOR;
		velocity.fract.width <<= RESOLUTION_FACTOR;
		velocity.fract.height <<= RESOLUTION_FACTOR;
		velocity.error.width <<= RESOLUTION_FACTOR;
		velocity.error.height <<= RESOLUTION_FACTOR;
		velocity.incr.width <<= RESOLUTION_FACTOR;
		velocity.incr.height <<= RESOLUTION_FACTOR;
	}

	LoadGameStateFlags(reader, root);
}

void LoadSisState(SaveReader& reader, yyjson_val* obj, SIS_STATE& SS)
{
	yyjson_val* sis {reader.object(obj, "sis")};
	reader.getPoint(sis, "log", SS.log_x, SS.log_y);
	reader.get(sis, "ResUnits", SS.ResUnits);
	reader.get(sis, "FuelOnBoard", SS.FuelOnBoard);
	reader.get(sis, "CrewEnlisted", SS.CrewEnlisted);
	reader.get(sis, "TotalElementMass", SS.TotalElementMass);
	reader.get(sis, "TotalBioMass", SS.TotalBioMass);
	reader.getFixedArray<uint8_t>(sis, "ModuleSlots", SS.ModuleSlots);
	reader.getFixedArray<uint8_t>(sis, "DriveSlots", SS.DriveSlots);
	reader.getFixedArray<uint8_t>(sis, "JetSlots", SS.JetSlots);
	reader.get(sis, "NumLanders", SS.NumLanders);
	reader.getFixedArray<uint16_t>(sis, "ElementAmounts", SS.ElementAmounts);
	reader.getString(sis, "ShipName", SS.ShipName);
	reader.getString(sis, "CommanderName", SS.CommanderName);
	reader.getString(sis, "PlanetName", SS.PlanetName);
	reader.get(sis, "Difficulty", SS.Difficulty);
	reader.get(sis, "Extended", SS.Extended);
	reader.get(sis, "Nomad", SS.Nomad);
	reader.get(sis, "Seed", SS.Seed);
	reader.get(sis, "ShipSeed", SS.ShipSeed);

	// JMS: Let's make savegames work even between different resolution modes.
	SS.log_x <<= RESOLUTION_FACTOR;
	SS.log_y <<= RESOLUTION_FACTOR;
}

void LoadSummary(SaveReader& reader, yyjson_val* root, SUMMARY_DESC& Summ)
{
	yyjson_val* summary {reader.object(root, "summary")};
	reader.getString(summary, "SaveName", Summ.SaveName);
	reader.get(summary, "Activity", Summ.Activity);
	reader.get(summary, "Flags", Summ.Flags);
	reader.get(summary, "day_index", Summ.day_index);
	reader.get(summary, "month_index", Summ.month_index);
	reader.get(summary, "year_index", Summ.year_index);
	reader.get(summary, "MCreditLo", Summ.MCreditLo);
	reader.get(summary, "MCreditHi", Summ.MCreditHi);
	Summ.NumShips = static_cast<uint8_t>(reader.getArray<uint8_t>(summary, "ShipList", Summ.ShipList));
	Summ.NumDevices = static_cast<uint8_t>(reader.getArray<uint8_t>(summary, "DeviceList", Summ.DeviceList));
	reader.get(summary, "res_factor", Summ.res_factor);
	LoadSisState(reader, summary, Summ.SS);

	// Core UQM saves converted to this format set "coreSave".
	Summ.SS.SaveVersion = reader.getBool(summary, "coreSave") ? 1 : 0;
	IndependantResFactor = Summ.res_factor;
}

void LoadStarDesc(SaveReader& reader, yyjson_val* root, STAR_DESC& SD)
{
	yyjson_val* star {reader.object(root, "starDesc")};
	reader.getPoint(star, "star_pt", SD.star_pt.x, SD.star_pt.y);
	reader.get(star, "Type", SD.Type);
	reader.get(star, "Index", SD.Index);
	reader.get(star, "Prefix", SD.Prefix);
	reader.get(star, "Postfix", SD.Postfix);
}

void LoadScanInfo(SaveReader& reader, yyjson_val* root)
{
	yyjson_val* list {reader.optionalArray(root, "starInfo")};
	if (!list)
	{
		return;
	}

	GAME_STATE_FILE* fp {OpenStateFile(STARINFO_FILE, "wb")};
	if (fp)
	{
		size_t index {};
		size_t count {};
		yyjson_val* item {};
		yyjson_arr_foreach(list, index, count, item)
		{
			uint32_t val {};
			if (!toInt(item, val))
			{
				reader.fail("starInfo");
			}
			swrite_32(fp, val);
		}
		CloseStateFile(fp);
	}
}

void LoadGroupList(SaveReader& reader, yyjson_val* groupList)
{
	GAME_STATE_FILE* fp {OpenStateFile(RANDGRPINFO_FILE, "rb")};
	if (fp)
	{
		GROUP_HEADER h;
		uint8_t LastEnc {};
		ReadGroupHeader(fp, &h);
		/* There's only supposed to be one of these, so group 0 should be
		 * zero here whenever we're here. We add the group list to the
		 * end here. */
		h.GroupOffset[0] = LengthStateFile(fp);
		SeekStateFile(fp, 0, SEEK_SET);
		WriteGroupHeader(fp, &h);
		SeekStateFile(fp, h.GroupOffset[0], SEEK_SET);

		reader.get(groupList, "LastEnc", LastEnc);
		yyjson_val* groups {yyjson_obj_get(groupList, "groups")};
		if (!yyjson_is_arr(groups) || yyjson_arr_size(groups) > UINT8_MAX)
		{
			reader.fail("groups");
			CloseStateFile(fp);
			return;
		}
		swrite_8(fp, LastEnc);
		swrite_8(fp, static_cast<uint8_t>(yyjson_arr_size(groups)));

		size_t index {};
		size_t count {};
		yyjson_val* item {};
		yyjson_arr_foreach(groups, index, count, item)
		{
			uint8_t race_outer {};
			IP_GROUP ip {};
			reader.get(item, "race_outer", race_outer);
			LoadIpGroup(reader, item, ip);

			swrite_8(fp, race_outer);
			WriteIpGroup(fp, &ip);
		}
		CloseStateFile(fp);
	}
}

void SetBattleGroupOffset(int encounterIndex, uint32_t offset)
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

void LoadBattleGroup(SaveReader& reader, yyjson_val* battleGroup)
{
	GAME_STATE_FILE* fp {};
	GROUP_HEADER h {};
	uint32_t encounter {};
	uint32_t offset {};
	bool current {reader.getBool(battleGroup, "current")};

	reader.get(battleGroup, "encounter", encounter);
	if (encounter)
	{
		/* This is a defined group, so it's new */
		fp = OpenStateFile(DEFGRPINFO_FILE, "rb");
		offset = fp ? LengthStateFile(fp) : 0;
	}
	else
	{
		/* This is the random group. Load in what was there,
		 * as we might have already seen the Group List. */
		fp = OpenStateFile(RANDGRPINFO_FILE, "rb");
		current = false;
		if (fp)
		{
			ReadGroupHeader(fp, &h);
		}
	}
	if (!fp)
	{
		return;
	}

	yyjson_val* groups {yyjson_obj_get(battleGroup, "groups")};
	if (!yyjson_is_arr(groups) || yyjson_arr_size(groups) > NUM_SAVED_BATTLE_GROUPS)
	{
		reader.fail("groups");
		CloseStateFile(fp);
		return;
	}
	reader.get(battleGroup, "star_index", h.star_index);
	reader.get(battleGroup, "day_index", h.day_index);
	reader.get(battleGroup, "month_index", h.month_index);
	reader.get(battleGroup, "year_index", h.year_index);
	h.NumGroups = static_cast<uint8_t>(yyjson_arr_size(groups));

	/* Write out the half-finished state file so that we can use
	 * the file size to compute group offsets */
	SeekStateFile(fp, offset, SEEK_SET);
	WriteGroupHeader(fp, &h);

	size_t index {};
	size_t count {};
	yyjson_val* group {};
	yyjson_arr_foreach(groups, index, count, group)
	{
		uint8_t icon {};
		reader.get(group, "icon", icon);
		yyjson_val* ships {yyjson_obj_get(group, "ships")};
		if (!yyjson_is_arr(ships) || yyjson_arr_size(ships) > UINT8_MAX)
		{
			reader.fail("ships");
			break;
		}

		h.GroupOffset[index + 1] = LengthStateFile(fp);
		SeekStateFile(fp, h.GroupOffset[index + 1], SEEK_SET);
		swrite_8(fp, icon);
		swrite_8(fp, static_cast<uint8_t>(yyjson_arr_size(ships)));

		size_t shipIndex {};
		size_t shipCount {};
		yyjson_val* ship {};
		yyjson_arr_foreach(ships, shipIndex, shipCount, ship)
		{
			uint8_t race_outer {};
			SHIP_FRAGMENT sf {};
			reader.get(ship, "race_outer", race_outer);
			LoadShipFragment(reader, ship, sf);

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
}

void LoadGroups(SaveReader& reader, yyjson_val* root)
{
	yyjson_val* groupList {yyjson_obj_get(root, "groupList")};
	yyjson_val* battleGroups {reader.optionalArray(root, "battleGroups")};
	if (!groupList && !battleGroups)
	{
		return;
	}

	InitGroupInfo(true);
	GLOBAL(BattleGroupRef) = 0;
	if (groupList)
	{
		if (!yyjson_is_obj(groupList))
		{
			reader.fail("groupList");
			return;
		}
		LoadGroupList(reader, groupList);
	}

	size_t index {};
	size_t count {};
	yyjson_val* battleGroup {};
	yyjson_arr_foreach(battleGroups, index, count, battleGroup)
	{
		LoadBattleGroup(reader, battleGroup);
	}
}

} // namespace

bool LoadGame(uint16_t which_game, SUMMARY_DESC* SummPtr)
{
	const uqstl::string file {saveFileName(which_game)};
	const auto json {uqm::readTextFile(uqm::saveDir(), file.c_str())};
	if (!json)
	{
		return false;
	}

	yyjson_read_err err {};
	const uqstl::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)> doc {
		yyjson_read_opts(const_cast<char*>(json->data()), json->size(), YYJSON_READ_ALLOW_INVALID_UNICODE, nullptr, &err),
		&yyjson_doc_free};
	if (!doc)
	{
		uqm::log::error("{}: JSON parse error at byte {}: {}", file, err.pos, err.msg);
		return false;
	}

	yyjson_val* root {yyjson_doc_get_root(doc.get())};
	SaveReader reader {};
	uint32_t version {};
	reader.get(root, "version", version);
	if (!reader.ok() || version > c_saveFormatVersion)
	{
		uqm::log::error("{}: save game version {} is not supported.", file, version);
		return false;
	}

	SUMMARY_DESC loc_sd {};
	LoadSummary(reader, root, loc_sd);
	if (!reader.ok())
	{
		uqm::log::error("{}: the save game summary is not valid.", file);
		return false;
	}
	if (SummPtr)
	{ // only need summary for displaying to user
		*SummPtr = loc_sd;
		return true;
	}

	GlobData.SIS_state = loc_sd.SS;
	legacySave = loc_sd.SS.SaveVersion > 0;

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

	const ACTIVITY Activity {GLOBAL(CurrentActivity)};
	LoadGameState(reader, root, GlobData.Game_state);
	NextActivity = GLOBAL(CurrentActivity);
	GLOBAL(CurrentActivity) = Activity;

	LoadRaceQueue(reader, root, &GLOBAL(avail_race_q));
	LoadShipQueue(reader, root, "npcShipQueue", &GLOBAL(npc_built_ship_q));
	LoadGroupQueue(reader, root, &GLOBAL(ip_group_q));
	LoadShipQueue(reader, root, "shipQueue", &GLOBAL(built_ship_q));
	LoadShipQueue(reader, root, "stowedShipQueue", &GLOBAL(stowed_ship_q));
	LoadEvents(reader, root);
	LoadEncounters(reader, root);
	LoadScanInfo(reader, root);
	LoadGroups(reader, root);

	STAR_DESC SD {};
	LoadStarDesc(reader, root, SD);
	loadGameCheats();
	if (!reader.ok())
	{
		uqm::log::error("{}: the save game is not valid.", file);
		return false;
	}

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
