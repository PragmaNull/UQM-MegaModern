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
#include "starmap.h"
#include "gendef.h"
#include "globdata.h"
#include "intel.h"
#include "state.h"
#include "grpintrn.h"

#include "libs/mathlib.h"
#include "core/log/log.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static uint8_t LastEncGroup;
// Last encountered group, saved into state files

void ReadGroupHeader(GAME_STATE_FILE* fp, GROUP_HEADER* pGH)
{
	sread_8(fp, &pGH->NumGroups);
	sread_8(fp, &pGH->day_index);
	sread_8(fp, &pGH->month_index);
	sread_8(fp, nullptr); /* padding */
	sread_16(fp, &pGH->star_index);
	sread_16(fp, &pGH->year_index);
	sread_a32(fp, pGH->GroupOffset, NUM_SAVED_BATTLE_GROUPS + 1);
}

void WriteGroupHeader(GAME_STATE_FILE* fp, const GROUP_HEADER* pGH)
{
	swrite_8(fp, pGH->NumGroups);
	swrite_8(fp, pGH->day_index);
	swrite_8(fp, pGH->month_index);
	swrite_8(fp, 0); /* padding */
	swrite_16(fp, pGH->star_index);
	swrite_16(fp, pGH->year_index);
	swrite_a32(fp, pGH->GroupOffset, NUM_SAVED_BATTLE_GROUPS + 1);
}

void ReadShipFragment(GAME_STATE_FILE* fp, SHIP_FRAGMENT* FragPtr)
{
	uint8_t tmpb;

	sread_16(fp, nullptr); /* unused: was which_side */
	sread_8(fp, &FragPtr->captains_name_index);
	sread_8(fp, nullptr);  /* padding; for savegame compat */
	sread_16(fp, nullptr); /* unused: was ship_flags */
	sread_8(fp, &FragPtr->race_id);
	sread_8(fp, &FragPtr->index);
	// XXX: reading crew as uint8_t to maintain savegame compatibility
	sread_8(fp, &tmpb);
	FragPtr->crew_level = tmpb;
	sread_8(fp, &tmpb);
	FragPtr->max_crew = tmpb;
	sread_8(fp, &FragPtr->energy_level);
	sread_8(fp, &FragPtr->max_energy);
	sread_16(fp, nullptr); /* unused; was loc.x */
	sread_16(fp, nullptr); /* unused; was loc.y */
}

void WriteShipFragment(GAME_STATE_FILE* fp, const SHIP_FRAGMENT* FragPtr)
{
	swrite_16(fp, 0); /* unused: was which_side */
	swrite_8(fp, FragPtr->captains_name_index);
	swrite_8(fp, 0);  /* padding; for savegame compat */
	swrite_16(fp, 0); /* unused: was ship_flags */
	swrite_8(fp, FragPtr->race_id);
	swrite_8(fp, FragPtr->index);
	// XXX: writing crew as uint8_t to maintain savegame compatibility
	swrite_8(fp, FragPtr->crew_level);
	swrite_8(fp, FragPtr->max_crew);
	swrite_8(fp, FragPtr->energy_level);
	swrite_8(fp, FragPtr->max_energy);
	swrite_16(fp, 0); /* unused; was loc.x */
	swrite_16(fp, 0); /* unused; was loc.y */
}

void ReadIpGroup(GAME_STATE_FILE* fp, IP_GROUP* GroupPtr)
{
	uint8_t tmpb;

	sread_16(fp, nullptr); /* unused; was which_side */
	sread_8(fp, nullptr);  /* unused; was captains_name_index */
	sread_8(fp, nullptr);  /* padding; for savegame compat */
	sread_16(fp, &GroupPtr->group_counter);
	sread_8(fp, &GroupPtr->race_id);
	sread_8(fp, &tmpb); /* was var2 */
	GroupPtr->sys_loc = LONIBBLE(tmpb);
	GroupPtr->task = HINIBBLE(tmpb);
	sread_8(fp, &GroupPtr->in_system); /* was crew_level */
	sread_8(fp, nullptr);			   /* unused; was max_crew */
	sread_8(fp, &tmpb);				   /* was energy_level */
	GroupPtr->dest_loc = LONIBBLE(tmpb);
	GroupPtr->orbit_pos = HINIBBLE(tmpb);
	sread_8(fp, &GroupPtr->group_id); /* was max_energy */
	sread_16s(fp, &GroupPtr->loc.x);
	sread_16s(fp, &GroupPtr->loc.y);
}

void WriteIpGroup(GAME_STATE_FILE* fp, const IP_GROUP* GroupPtr)
{
	swrite_16(fp, 0); /* unused; was which_side */
	swrite_8(fp, 0);  /* unused; was captains_name_index */
	swrite_8(fp, 0);  /* padding; for savegame compat */
	swrite_16(fp, GroupPtr->group_counter);
	swrite_8(fp, GroupPtr->race_id);
	assert(GroupPtr->sys_loc < 0x10 && GroupPtr->task < 0x10);
	swrite_8(fp, MAKE_BYTE(GroupPtr->sys_loc, GroupPtr->task));
	/* was var2 */
	swrite_8(fp, GroupPtr->in_system); /* was crew_level */
	swrite_8(fp, 0);				   /* unused; was max_crew */
	assert(GroupPtr->dest_loc < 0x10 && GroupPtr->orbit_pos < 0x10);
	swrite_8(fp, MAKE_BYTE(GroupPtr->dest_loc, GroupPtr->orbit_pos));
	/* was energy_level */
	swrite_8(fp, GroupPtr->group_id); /* was max_energy */
	swrite_16(fp, GroupPtr->loc.x);
	swrite_16(fp, GroupPtr->loc.y);
}

void InitGroupInfo(bool FirstTime)
{
	GAME_STATE_FILE* fp;

	assert(NUM_SAVED_BATTLE_GROUPS >= MAX_BATTLE_GROUPS);

	fp = OpenStateFile(RANDGRPINFO_FILE, "wb");
	if (fp)
	{
		GROUP_HEADER GH;

		memset(&GH, 0, sizeof(GH));
		GH.star_index = (uint16_t)~0;
		WriteGroupHeader(fp, &GH);
		CloseStateFile(fp);
	}

	if (FirstTime && (fp = OpenStateFile(DEFGRPINFO_FILE, "wb")))
	{
		// Group headers cannot start with offset 0 in 'defined' group
		// info file, so bump it (because offset 0 is reserved to
		// indicate the 'random' group info file).
		swrite_8(fp, 0);
		CloseStateFile(fp);
	}
}

void UninitGroupInfo(void)
{
	DeleteStateFile(DEFGRPINFO_FILE);
	DeleteStateFile(RANDGRPINFO_FILE);
}

HIPGROUP
BuildGroup(QUEUE* pDstQueue, uint8_t race_id)
{
	HFLEETINFO hFleet;
	FLEET_INFO* TemplatePtr;
	HLINK hGroup;
	IP_GROUP* GroupPtr;

	assert(GetLinkSize(pDstQueue) == sizeof(IP_GROUP));

	hFleet = GetStarShipFromIndex(&GLOBAL(avail_race_q), race_id);
	if (!hFleet)
	{
		return 0;
	}

	hGroup = AllocLink(pDstQueue);
	if (!hGroup)
	{
		return 0;
	}

	TemplatePtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);
	GroupPtr = LockIpGroup(pDstQueue, hGroup);
	memset(GroupPtr, 0, GetLinkSize(pDstQueue));
	GroupPtr->race_id = race_id;
	GroupPtr->melee_icon = TemplatePtr->melee_icon;
	UnlockFleetInfo(&GLOBAL(avail_race_q), hFleet);
	UnlockIpGroup(pDstQueue, hGroup);
	PutQueue(pDstQueue, hGroup);

	return hGroup;
}

void BuildGroups(void)
{
	uint8_t Index;
	uint8_t BestIndex = 0;
	uint16_t BestPercent = 0;
	GFXPOINT universe;
	HFLEETINFO hFleet, hNextFleet;

	uint8_t HomeWorld[] = {HOMEWORLD_LOC};
	uint8_t EncounterPercent[] = {RACE_INTERPLANETARY_PERCENT};

	EncounterPercent[SLYLANDRO_SHIP] *= GET_GAME_STATE(SLYLANDRO_MULTIPLIER);

	/* make Ur-Quan encounters impossible at the ZFP homeworld,
	 * like their dialog claims */
	if (EXTENDED && CurStarDescPtr->Index == ZOQFOT_DEFINED)
	{
		EncounterPercent[URQUAN_SHIP] = EncounterPercent[BLACK_URQUAN_SHIP] = 0;
	}

	Index = GET_GAME_STATE(UTWIG_SUPOX_MISSION);
	if (Index > 1 && Index < 5)
	{
		// When the Utwig and Supox are on their mission, there won't be
		// new battle groups generated for the system.
		// Note that old groups may still exist (in which case this function
		// would not even be called), but those expire after spending a week
		// outside of the star system, or when a different star system is
		// entered.
		HomeWorld[UTWIG_SHIP] = 0;
		HomeWorld[SUPOX_SHIP] = 0;
	}

	universe = CurStarDescPtr->star_pt;
	for (hFleet = GetHeadLink(&GLOBAL(avail_race_q)), Index = 0;
		 hFleet; hFleet = hNextFleet, ++Index)
	{
		uint16_t i, encounter_radius;
		FLEET_INFO* FleetPtr;

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);
		hNextFleet = _GetSuccLink(FleetPtr);

		if ((encounter_radius = FleetPtr->actual_strength)
			&& (i = EncounterPercent[Index]))
		{
			int16_t dx, dy;
			uint32_t d_squared;
			uint8_t race_enc;

			race_enc = HomeWorld[Index];
			if (race_enc && CurStarDescPtr->Index == race_enc)
			{ // In general, there are always ships at the Homeworld for
				// the races specified in HomeWorld[] array.
				BestIndex = Index;
				BestPercent = 70;
				if (race_enc == SPATHI_DEFINED || race_enc == SUPOX_DEFINED)
				{
					BestPercent = 2;
				}
				// Terminate the loop!
				hNextFleet = 0;

				goto FoundHome;
			}

			if (encounter_radius == INFINITE_RADIUS)
			{
				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
			}
			else
			{
				encounter_radius =
					(encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;
			}
			dx = universe.x - FleetPtr->loc.x;
			if (dx < 0)
			{
				dx = -dx;
			}
			dy = universe.y - FleetPtr->loc.y;
			if (dy < 0)
			{
				dy = -dy;
			}
			if ((uint16_t)dx < encounter_radius
				&& (uint16_t)dy < encounter_radius
				&& (d_squared = (uint32_t)dx * dx + (uint32_t)dy * dy) < (uint32_t)encounter_radius * encounter_radius)
			{
				uint32_t rand_val;

				// EncounterPercent is only used in practice for the Slylandro
				// Probes, for the rest of races the chance of encounter is
				// calced directly below from the distance to the Homeworld
				if (FleetPtr->actual_strength != INFINITE_RADIUS)
				{
					i = 70 - (uint16_t)((uint32_t)square_root(d_squared) * 60L / encounter_radius);
				}

				rand_val = TFB_Random();
				if ((int)(LOWORD(rand_val) % 100) < (int)i
					&& (BestPercent == 0
						|| (HIWORD(rand_val) % (i + BestPercent)) < i))
				{
					if (FleetPtr->actual_strength == INFINITE_RADIUS)
					{ // The prevailing encounter chance is hereby limitted
						// to 4% for races with infinite SoI (currently, it
						// is only the Slylandro Probes)
						i = 4;
					}

					BestPercent = i;
					BestIndex = Index;
				}
			}
		}

FoundHome:
		UnlockFleetInfo(&GLOBAL(avail_race_q), hFleet);
	}

	if (BestPercent)
	{
		uint8_t which_group, num_groups;
		uint8_t EncounterMakeup[] =
			{
				RACE_ENCOUNTER_MAKEUP};

		which_group = 0;
		num_groups = ((uint16_t)TFB_Random() % (BestPercent >> 1)) + 1;
		if (num_groups > MAX_BATTLE_GROUPS)
		{
			num_groups = MAX_BATTLE_GROUPS;
		}
		else if (num_groups < 5
				 && (Index = HomeWorld[BestIndex])
				 && CurStarDescPtr->Index == Index)
		{
			num_groups = 5;
		}
		do
		{
			for (Index = HINIBBLE(EncounterMakeup[BestIndex]); Index;
				 --Index)
			{
				if (Index <= LONIBBLE(EncounterMakeup[BestIndex])
					|| (uint16_t)TFB_Random() % 100 < 50)
				{
					CloneShipFragment((RACE_ID)BestIndex,
									  &GLOBAL(npc_built_ship_q), 0);
				}
			}

			PutGroupInfo(GROUPS_RANDOM, ++which_group);
			ReinitQueue(&GLOBAL(npc_built_ship_q));
		} while (--num_groups);
	}

	GetGroupInfo(GROUPS_RANDOM, GROUP_INIT_IP);
}

void findRaceSOI(void)
{
	GFXPOINT universe = CurStarDescPtr->star_pt;
	HFLEETINFO hFleet, hNextFleet;
	uint8_t Index;
	uint8_t i = 0, j;
	uint8_t RaceHasMusic[] = {RACE_MUSIC_BOOL};
	uint8_t HomeWorld[] = {HOMEWORLD_LOC};
	uint8_t WhichHomeWorld = NO_ID;
#define MAX_OVERLAP 10
	uint8_t SpeciesArr[MAX_OVERLAP] = {NO_ID};
	uint16_t RadiusArr[MAX_OVERLAP] = {0};

	if (!SpaceMusicOK)
	{
		spaceMusicBySOI = NO_ID;
		return;
	}

	if (GET_GAME_STATE(KOHR_AH_FRENZY))
	{
		spaceMusicBySOI = SA_MATRA_ID;
		return;
	}

	for (hFleet = GetHeadLink(&GLOBAL(avail_race_q)), Index = 0;
		 hFleet; hFleet = hNextFleet, ++Index)
	{
		FLEET_INFO* FleetPtr;
		SPECIES_ID SpeciesID;
		RACE_ID RaceID;

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);
		hNextFleet = _GetSuccLink(FleetPtr);
		SpeciesID = FleetPtr->SpeciesID;
		RaceID = (RACE_ID)(SpeciesID - 1);

		if (HomeWorld[Index] && CurStarDescPtr->Index == HomeWorld[Index]
			&& !RaceDead(RaceID)
			&& (uqm::UQMOptions::read().spaceMusic == uqm::SphereOfInfluenceMusic::RaceBeforeDiscovery
				|| (uqm::UQMOptions::read().spaceMusic == uqm::SphereOfInfluenceMusic::RaceAfterDiscovery && (CheckSphereTracking(RaceID) || IsHomeworldKnown(SpeciesToHomeID(SpeciesID))))))
		{ // Found a HomeWorld, set the species ID accordingly
			// If the No Spoilers option is enabled. Only add the
			// species ID if their SOI is shown on the map or if
			// their HomeWorld is known
			WhichHomeWorld = SpeciesID;
		}

		if (FleetPtr->actual_strength && RaceHasMusic[Index])
		{
			int16_t dx, dy;
			uint16_t encounter_radius;
			uint32_t d_squared;

			if (FleetPtr->actual_strength == INFINITE_RADIUS)
			{
				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
			}
			else
			{
				encounter_radius = (FleetPtr->actual_strength
									* SPHERE_RADIUS_INCREMENT)
								>> 1;
			}

			dx = universe.x - FleetPtr->loc.x;
			if (dx < 0)
			{
				dx = -dx;
			}

			dy = universe.y - FleetPtr->loc.y;
			if (dy < 0)
			{
				dy = -dy;
			}

			if ((uint16_t)dx < encounter_radius
				&& (uint16_t)dy < encounter_radius
				&& (d_squared = (uint32_t)dx * dx + (uint32_t)dy * dy)
					   < (uint32_t)encounter_radius * encounter_radius)
			{ // Finds the race SOI
				if (uqm::UQMOptions::read().spaceMusic == uqm::SphereOfInfluenceMusic::RaceBeforeDiscovery || (uqm::UQMOptions::read().spaceMusic == uqm::SphereOfInfluenceMusic::RaceAfterDiscovery && CheckSphereTracking(RaceID)))
				{ // If the No Spoilers option is enabled. Only add the
					// species ID if their SOI is shown on the map.
					SpeciesArr[i] = SpeciesID;
					RadiusArr[i] = encounter_radius;
					++i;
				}
			}
		}
	}

	if (WhichHomeWorld != NO_ID)
	{ // HomeWorlds override all SOI findings
		spaceMusicBySOI = WhichHomeWorld;
		return;
	}

	Index = 0;
	for (j = 1; j < i; j++)
	{ // Finding the smallest SOI we're currently occupying
		if (RadiusArr[j] < RadiusArr[Index])
		{
			Index = j;
		}
	}
	spaceMusicBySOI = SpeciesArr[Index];
}

static void
FlushGroupInfo(GROUP_HEADER* pGH, uint32_t offset, uint8_t which_group,
			   GAME_STATE_FILE* fp)
{
	if (which_group == GROUP_LIST)
	{
		HIPGROUP hGroup, hNextGroup;

		/* If the group list was never written before, add it */
		if (pGH->GroupOffset[0] == 0)
		{
			pGH->GroupOffset[0] = LengthStateFile(fp);
		}

		// XXX: npc_built_ship_q must be empty because the wipe-out
		//   procedure is actually the writing of the npc_built_ship_q
		//   out as the group in question
		assert(!GetHeadLink(&GLOBAL(npc_built_ship_q)));

		/* Weed out the groups that left the system first */
		for (hGroup = GetHeadLink(&GLOBAL(ip_group_q));
			 hGroup; hGroup = hNextGroup)
		{
			uint8_t in_system;
			uint8_t group_id;
			IP_GROUP* GroupPtr;

			GroupPtr = LockIpGroup(&GLOBAL(ip_group_q), hGroup);
			hNextGroup = _GetSuccLink(GroupPtr);
			in_system = GroupPtr->in_system;
			group_id = GroupPtr->group_id;
			UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);

			if (!in_system)
			{
				// The following 'if' is needed because GROUP_LIST is only
				// ever flushed to RANDGRPINFO_FILE, but the current group
				// may need to be updated in the DEFGRPINFO_FILE as well.
				// In that case, PutGroupInfo() will update the correct file.
				if (GLOBAL(BattleGroupRef))
				{
					PutGroupInfo(GLOBAL(BattleGroupRef), group_id);
				}
				else
				{
					FlushGroupInfo(pGH, GROUPS_RANDOM, group_id, fp);
				}
				// This will also wipe the group out in the RANDGRPINFO_FILE
				pGH->GroupOffset[group_id] = 0;
				RemoveQueue(&GLOBAL(ip_group_q), hGroup);
				FreeIpGroup(&GLOBAL(ip_group_q), hGroup);
			}
		}
	}
	else if (which_group > pGH->NumGroups)
	{ /* Group not present yet -- add it */
		pGH->NumGroups = which_group;
		pGH->GroupOffset[which_group] = LengthStateFile(fp);
	}

	SeekStateFile(fp, offset, SEEK_SET);
	WriteGroupHeader(fp, pGH);

#ifdef DEBUG_GROUPS
	uqm::log::debug("1)FlushGroupInfo({}): WG = {}({}), NG = {}, "
					"SI = {}",
					offset, which_group, pGH->GroupOffset[which_group],
					pGH->NumGroups, pGH->star_index);
#endif /* DEBUG_GROUPS */

	if (which_group == GROUP_LIST)
	{
		/* Write out ip_group_q as group 0 */
		HIPGROUP hGroup, hNextGroup;
		uint8_t NumGroups = CountLinks(&GLOBAL(ip_group_q));

		SeekStateFile(fp, pGH->GroupOffset[0], SEEK_SET);
		swrite_8(fp, LastEncGroup);
		swrite_8(fp, NumGroups);

		hGroup = GetHeadLink(&GLOBAL(ip_group_q));
		for (; NumGroups; --NumGroups, hGroup = hNextGroup)
		{
			IP_GROUP* GroupPtr;

			GroupPtr = LockIpGroup(&GLOBAL(ip_group_q), hGroup);
			hNextGroup = _GetSuccLink(GroupPtr);

			swrite_8(fp, GroupPtr->race_id);

#ifdef DEBUG_GROUPS
			uqm::log::debug("F) type {}, loc {}<{}, {}>, task 0x%02x:{}",
							GroupPtr->race_id,
							GET_GROUP_LOC(GroupPtr),
							GroupPtr->loc.x,
							GroupPtr->loc.y,
							GET_GROUP_MISSION(GroupPtr),
							GET_GROUP_DEST(GroupPtr));
#endif /* DEBUG_GROUPS */

			WriteIpGroup(fp, GroupPtr);

			UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
		}
	}
	else
	{
		/* Write out npc_built_ship_q as 'which_group' group */
		HSHIPFRAG hStarShip, hNextShip;
		uint8_t NumShips = CountLinks(&GLOBAL(npc_built_ship_q));
		uint8_t RaceType = 0;

		hStarShip = GetHeadLink(&GLOBAL(npc_built_ship_q));
		if (NumShips > 0)
		{
			SHIP_FRAGMENT* FragPtr;

			/* The first ship in a group defines the alien race */
			FragPtr = LockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
			RaceType = FragPtr->race_id;
			UnlockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
		}

		SeekStateFile(fp, pGH->GroupOffset[which_group], SEEK_SET);
		swrite_8(fp, RaceType);
		swrite_8(fp, NumShips);

		for (; NumShips; --NumShips, hStarShip = hNextShip)
		{
			SHIP_FRAGMENT* FragPtr;

			FragPtr = LockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
			hNextShip = _GetSuccLink(FragPtr);

			swrite_8(fp, FragPtr->race_id);
			WriteShipFragment(fp, FragPtr);

			UnlockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
		}
	}
}

bool GetGroupInfo(uint32_t offset, uint8_t which_group)
{
	GAME_STATE_FILE* fp;
	GROUP_HEADER GH;

	if (offset != GROUPS_RANDOM && which_group != GROUP_LIST)
	{
		fp = OpenStateFile(DEFGRPINFO_FILE, "r+b");
	}
	else
	{
		fp = OpenStateFile(RANDGRPINFO_FILE, "r+b");
	}

	if (!fp)
	{
		return false;
	}

	SeekStateFile(fp, offset, SEEK_SET);
	ReadGroupHeader(fp, &GH);
#ifdef DEBUG_GROUPS
	uqm::log::debug("GetGroupInfo({}): {}({}) out of {}", offset,
					which_group, GH.GroupOffset[which_group], GH.NumGroups);
#endif /* DEBUG_GROUPS */

	if (which_group == GROUP_INIT_IP)
	{
		uint16_t month_index, day_index, year_index;

		ReinitQueue(&GLOBAL(ip_group_q));
#ifdef DEBUG_GROUPS
		uqm::log::debug("{} == {}", GH.star_index,
						(uint16_t)(CurStarDescPtr - star_array));
#endif /* DEBUG_GROUPS */

		/* Check if the requested groups are valid for this star system
		 * and if they are still current (not expired) */
		day_index = GH.day_index;
		month_index = GH.month_index;
		year_index = GH.year_index;
		if (offset == GROUPS_RANDOM
			&& (GH.star_index != (uint16_t)(CurStarDescPtr - star_array)
				|| !ValidateEvent(ABSOLUTE_EVENT, &month_index, &day_index,
								  &year_index)))
		{
#ifdef DEBUG_GROUPS
			if (GH.star_index == CurStarDescPtr - star_array)
			{
				uqm::log::debug("GetGroupInfo: battle groups out of "
								"date {}/{}/{}!",
								month_index, day_index,
								year_index);
			}
#endif /* DEBUG_GROUPS */

			CloseStateFile(fp);
			/* Erase random groups (out of date) */
			fp = OpenStateFile(RANDGRPINFO_FILE, "wb");
			memset(&GH, 0, sizeof(GH));
			GH.star_index = (uint16_t)~0;
			WriteGroupHeader(fp, &GH);
			CloseStateFile(fp);

			return false;
		}

		/* Read IP groups into ip_group_q and send them on their missions */
		for (which_group = 1; which_group <= GH.NumGroups; ++which_group)
		{
			uint8_t task, group_loc;
			uint32_t rand_val;
			uint8_t RaceType;
			uint8_t NumShips;
			HIPGROUP hGroup;
			IP_GROUP* GroupPtr;

			if (GH.GroupOffset[which_group] == 0)
			{
				continue;
			}

			SeekStateFile(fp, GH.GroupOffset[which_group], SEEK_SET);
			sread_8(fp, &RaceType);
			sread_8(fp, &NumShips);
			if (!NumShips)
			{
				continue; /* group is dead */
			}

			hGroup = BuildGroup(&GLOBAL(ip_group_q), RaceType);
			GroupPtr = LockIpGroup(&GLOBAL(ip_group_q), hGroup);
			GroupPtr->group_id = which_group;
			GroupPtr->in_system = 1;

			rand_val = TFB_Random();
			task = (uint8_t)(lowByte(LOWORD(rand_val)) % ON_STATION);
			if (task == FLEE)
			{
				task = ON_STATION;
			}
			GroupPtr->orbit_pos = NORMALIZE_FACING(
				lowByte(HIWORD(rand_val)));

			group_loc = pSolarSysState->SunDesc[0].NumPlanets;
			if (group_loc == 1 && task == EXPLORE)
			{
				task = IN_ORBIT;
			}
			else
			{
				group_loc = (uint8_t)((highByte(LOWORD(rand_val)) % group_loc) + 1);
			}
			GroupPtr->dest_loc = group_loc;
			rand_val = TFB_Random();
			GroupPtr->loc.x = (LOWORD(rand_val) % 10000) - 5000;
			GroupPtr->loc.y = (HIWORD(rand_val) % 10000) - 5000;
			GroupPtr->group_counter = 0;
			if (task == EXPLORE)
			{
				GroupPtr->group_counter = ((uint16_t)TFB_Random() % MAX_REVOLUTIONS) << FACING_SHIFT;
			}
			else if (task == ON_STATION)
			{
				uint16_t angle;
				GFXPOINT org;

				org = planetOuterLocation(group_loc - 1);
				angle = FACING_TO_ANGLE(GroupPtr->orbit_pos + 1);
				GroupPtr->loc.x = org.x + COSINE(angle, STATION_RADIUS);
				GroupPtr->loc.y = org.y + SINE(angle, STATION_RADIUS);
				group_loc = 0;
			}

			GroupPtr->task = task;
			GroupPtr->sys_loc = group_loc;

#ifdef DEBUG_GROUPS
			uqm::log::debug("battle group {}(0x%04x) strength "
							"{}, type {}, loc {}<{}, {}>, task {}",
							which_group,
							hGroup,
							NumShips,
							RaceType,
							group_loc,
							GroupPtr->loc.x,
							GroupPtr->loc.y,
							task);
#endif /* DEBUG_GROUPS */

			UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
		}

		if (offset != GROUPS_RANDOM)
		{
			InitGroupInfo(false); /* Wipe out random battle groups */
		}
		else if (ValidateEvent(ABSOLUTE_EVENT, /* still fresh */
							   &month_index, &day_index, &year_index))
		{
			CloseStateFile(fp);
			return true;
		}

		CloseStateFile(fp);
		return (GetHeadLink(&GLOBAL(ip_group_q)) != 0);
	}

	if (!GH.GroupOffset[which_group])
	{
		/* Group not present */
		CloseStateFile(fp);
		return false;
	}


	if (which_group == GROUP_LIST)
	{
		uint8_t NumGroups;
		uint16_t ShipsLeftInLEG;

		// XXX: Hack: First, save the state of last encountered group, if any.
		//   The assumption here is that we read the group list immediately
		//   after an IP encounter, and npc_built_ship_q contains whatever
		//   ships are left in the encountered group (can be none).
		ShipsLeftInLEG = CountLinks(&GLOBAL(npc_built_ship_q));

		SeekStateFile(fp, GH.GroupOffset[0], SEEK_SET);
		sread_8(fp, &LastEncGroup);

		if (LastEncGroup)
		{
			// The following 'if' is needed because GROUP_LIST is only
			// ever read from RANDGRPINFO_FILE, but the LastEncGroup
			// may need to be updated in the DEFGRPINFO_FILE as well.
			// In that case, PutGroupInfo() will update the correct file.
			if (GLOBAL(BattleGroupRef))
			{
				PutGroupInfo(GLOBAL(BattleGroupRef), LastEncGroup);
			}
			else
			{
				FlushGroupInfo(&GH, offset, LastEncGroup, fp);
			}
		}
		ReinitQueue(&GLOBAL(npc_built_ship_q));

		/* Read group 0 into ip_group_q */
		ReinitQueue(&GLOBAL(ip_group_q));
		/* Need a seek because Put/Flush has moved the file ptr */
		SeekStateFile(fp, GH.GroupOffset[0] + 1, SEEK_SET);
		sread_8(fp, &NumGroups);

		while (NumGroups--)
		{
			uint8_t group_id;
			uint8_t RaceType;
			HSHIPFRAG hGroup;
			IP_GROUP* GroupPtr;

			sread_8(fp, &RaceType);

			hGroup = BuildGroup(&GLOBAL(ip_group_q), RaceType);
			GroupPtr = LockIpGroup(&GLOBAL(ip_group_q), hGroup);
			ReadIpGroup(fp, GroupPtr);
			group_id = GroupPtr->group_id;

#ifdef DEBUG_GROUPS
			uqm::log::debug("G) type {}, loc {}<{}, {}>, task 0x%02x:{}",
							RaceType,
							GroupPtr->sys_loc,
							GroupPtr->loc.x,
							GroupPtr->loc.y,
							GroupPtr->task,
							GroupPtr->dest_loc);
#endif /* DEBUG_GROUPS */

			UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);

			if (group_id == LastEncGroup && !ShipsLeftInLEG)
			{
				/* No ships left in the last encountered group, remove it */
#ifdef DEBUG_GROUPS
				uqm::log::debug(" -- REMOVING");
#endif /* DEBUG_GROUPS */
				RemoveQueue(&GLOBAL(ip_group_q), hGroup);
				FreeIpGroup(&GLOBAL(ip_group_q), hGroup);
			}
		}

		CloseStateFile(fp);
		return (GetHeadLink(&GLOBAL(ip_group_q)) != 0);
	}
	else
	{
		/* Read 'which_group' group into npc_built_ship_q */
		uint8_t NumShips;

		// XXX: Hack: The assumption here is that we only read the makeup
		//   of a particular group when initializing an encounter, which
		//   makes this group 'last encountered'. Also the state of all
		//   groups is saved here. This may make working with savegames
		//   harder in the future, as special care will have to be taken
		//   when loading a game into an encounter.
		LastEncGroup = which_group;
		// The following 'if' is needed because GROUP_LIST is only
		// ever written to RANDGRPINFO_FILE, but the group we are reading
		// may be in the DEFGRPINFO_FILE as well.
		// In that case, PutGroupInfo() will update the correct file.
		// Always calling PutGroupInfo() here would also be acceptable now.
		if (offset != GROUPS_RANDOM)
		{
			PutGroupInfo(GROUPS_RANDOM, GROUP_LIST);
		}
		else
		{
			FlushGroupInfo(&GH, GROUPS_RANDOM, GROUP_LIST, fp);
		}
		ReinitQueue(&GLOBAL(ip_group_q));

		ReinitQueue(&GLOBAL(npc_built_ship_q));
		// skip RaceType
		SeekStateFile(fp, GH.GroupOffset[which_group] + 1, SEEK_SET);
		sread_8(fp, &NumShips);

		// Kruzen: if something went wrong - try to spawn only 1 ship
		if (NumShips > 14)
		{
			NumShips = 1;
		}

		while (NumShips--)
		{
			uint8_t RaceType;
			HSHIPFRAG hStarShip;
			SHIP_FRAGMENT* FragPtr;

			sread_8(fp, &RaceType);

			hStarShip = CloneShipFragment((RACE_ID)RaceType,
										  &GLOBAL(npc_built_ship_q), 0);

			FragPtr = LockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
			ReadShipFragment(fp, FragPtr);
			UnlockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
		}

		CloseStateFile(fp);
		return (GetHeadLink(&GLOBAL(npc_built_ship_q)) != 0);
	}
}

uint32_t
PutGroupInfo(uint32_t offset, uint8_t which_group)
{
	GAME_STATE_FILE* fp;
	GROUP_HEADER GH;

	if (offset != GROUPS_RANDOM && which_group != GROUP_LIST)
	{
		fp = OpenStateFile(DEFGRPINFO_FILE, "r+b");
	}
	else
	{
		fp = OpenStateFile(RANDGRPINFO_FILE, "r+b");
	}

	if (!fp)
	{
		return offset;
	}

	if (offset == GROUPS_ADD_NEW)
	{
		offset = LengthStateFile(fp);
		SeekStateFile(fp, offset, SEEK_SET);
		memset(&GH, 0, sizeof(GH));
		GH.star_index = (uint16_t)~0;
		WriteGroupHeader(fp, &GH);
	}

	// XXX: This is a bit dangerous. The assumption here is that we are
	//   only called to write GROUP_LIST in the GROUPS_RANDOM context,
	//   which is true right now and in which case we would seek to 0 anyway.
	//   The latter also makes guarding the seek with
	//   'if (which_group != GROUP_LIST)' moot.
	if (which_group != GROUP_LIST)
	{
		SeekStateFile(fp, offset, SEEK_SET);
		if (which_group == GROUP_SAVE_IP)
		{
			LastEncGroup = 0;
			which_group = GROUP_LIST;
		}
	}
	ReadGroupHeader(fp, &GH);

#ifdef NEVER
	// XXX: this appears to be a remnant of a slightly different group info
	//   expiration mechanism. Nowadays, the 'defined' groups never expire,
	//   and the dead 'random' groups stay in the file with NumShips==0 until
	//   the entire 'random' group header expires.
	if (GetHeadLink(&GLOBAL(npc_built_ship_q)) || GH.GroupOffset[0] == 0)
#endif /* NEVER */
	{
		uint16_t month_index, day_index, year_index;

		/* The groups in this system are good for the next 7 days */
		month_index = 0;
		day_index = 7;
		year_index = 0;
		ValidateEvent(RELATIVE_EVENT, &month_index, &day_index, &year_index);
		GH.day_index = (uint8_t)day_index;
		GH.month_index = (uint8_t)month_index;
		GH.year_index = year_index;
	}
	GH.star_index = CurStarDescPtr - star_array;

#ifdef DEBUG_GROUPS
	uqm::log::debug("PutGroupInfo({}): {} out of {} -- {}/{}/{}",
					offset, which_group, GH.NumGroups,
					GH.month_index, GH.day_index, GH.year_index);
#endif /* DEBUG_GROUPS */

	FlushGroupInfo(&GH, offset, which_group, fp);

	CloseStateFile(fp);

	return (offset);
}