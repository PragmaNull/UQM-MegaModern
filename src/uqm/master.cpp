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

#include "master.h"

#include "build.h"
#include "resinst.h"
#include "displist.h"
#include "fmv.h"
#include "supermelee/melee.h"


QUEUE master_q;
static int master_ship_seed = 0;

void LoadMasterShipList(void (*YieldProcessing)(void))
{
	SPECIES_ID s_id = ARILOU_ID;
	uint16_t num_entries {LAST_MELEE_ID - ARILOU_ID + 1};
	InitQueue(&master_q, num_entries, sizeof(MASTER_SHIP_INFO));
	while (num_entries--)
	{
		HMASTERSHIP hStarShip {};
		HMASTERSHIP hNextShip {};

		HMASTERSHIP hBuiltShip {AllocLink(&master_q)};
		if (!hBuiltShip)
		{
			continue;
		}

		// Allow other things to run
		//  supposedly, loading ship packages and data takes some time
		if (YieldProcessing)
		{
			YieldProcessing();
		}

		MASTER_SHIP_INFO* BuiltPtr {LockMasterShip(&master_q, hBuiltShip)};
		BuiltPtr->SpeciesID = postIncSpeciesId_HACK(s_id);
		RACE_DESC* RDPtr {load_ship(BuiltPtr->SpeciesID, false)};
		if (!RDPtr)
		{
			UnlockMasterShip(&master_q, hBuiltShip);
			continue;
		}

		// Grab a copy of loaded icons, strings and info
		// XXX: SHIP_INFO implicitly referenced here
		BuiltPtr->ShipInfo = RDPtr->ship_info;
		BuiltPtr->Fleet = RDPtr->fleet;
		free_ship(RDPtr, false, false);

		const char* builtName {BuiltPtr->ShipInfo.race_strings->text(2)};
		UnlockMasterShip(&master_q, hBuiltShip);

		// Insert the ship in the master queue in the right location
		// to keep the list sorted on the name of the race.
		for (hStarShip = GetHeadLink(&master_q);
			 hStarShip; hStarShip = hNextShip)
		{
			const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hStarShip)};
			hNextShip = _GetSuccLink(MasterPtr);
			const char* curName {MasterPtr->ShipInfo.race_strings->text(2)};
			UnlockMasterShip(&master_q, hStarShip);

			if (strcmp(builtName, curName) < 0)
			{
				break;
			}
		}
		InsertQueue(&master_q, hBuiltShip, hStarShip);
		AdvanceLoadProgress();
	}
	master_ship_seed = uqm::currentSeed().ships ? uqm::currentSeed().value : -1;
}

void FreeMasterShipList(void)
{
	HMASTERSHIP hStarShip {};
	HMASTERSHIP hNextShip {};

	for (hStarShip = GetHeadLink(&master_q);
		 hStarShip != 0; hStarShip = hNextShip)
	{
		const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hStarShip)};
		hNextShip = _GetSuccLink(MasterPtr);

		DestroyDrawable(ReleaseDrawable(MasterPtr->ShipInfo.melee_icon));
		DestroyDrawable(ReleaseDrawable(MasterPtr->ShipInfo.icons));
		delete MasterPtr->ShipInfo.race_strings;

		UnlockMasterShip(&master_q, hStarShip);
	}

	UninitQueue(&master_q);
}

void ReloadMasterShipList(void (*YieldProcessing)(void))
{
	if ((uqm::currentSeed().ships && master_ship_seed != uqm::currentSeed().value)
		|| (!uqm::currentSeed().ships && master_ship_seed != -1))
	{
		FreeMasterShipList();
		LoadMasterShipList(YieldProcessing);
		master_ship_seed = uqm::currentSeed().ships ? uqm::currentSeed().value : -1;
	}
}

HMASTERSHIP
FindMasterShip(SPECIES_ID ship_ref)
{
	HMASTERSHIP hStarShip {};
	HMASTERSHIP hNextShip {};

	for (hStarShip = GetHeadLink(&master_q); hStarShip; hStarShip = hNextShip)
	{
		SPECIES_ID ref;

		const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hStarShip)};
		hNextShip = _GetSuccLink(MasterPtr);
		ref = MasterPtr->SpeciesID;
		UnlockMasterShip(&master_q, hStarShip);

		if (ref == ship_ref)
		{
			break;
		}
	}

	return (hStarShip);
}

int FindMasterShipIndex(SPECIES_ID ship_ref)
{
	HMASTERSHIP hStarShip {};
	HMASTERSHIP hNextShip {};
	int index {};

	for (index = 0, hStarShip = GetHeadLink(&master_q); hStarShip;
		 ++index, hStarShip = hNextShip)
	{
		SPECIES_ID ref;

		const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hStarShip)};
		hNextShip = _GetSuccLink(MasterPtr);
		ref = MasterPtr->SpeciesID;
		UnlockMasterShip(&master_q, hStarShip);

		if (ref == ship_ref)
		{
			break;
		}
	}

	return hStarShip ? index : -1;
}

uint16_t
GetShipCostFromIndex(unsigned Index)
{
	HMASTERSHIP hMasterShip {GetStarShipFromIndex(&master_q, Index)};
	if (!hMasterShip)
	{
		return 0;
	}

	const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hMasterShip)};
	const uint16_t val {MasterPtr->ShipInfo.ship_cost};
	UnlockMasterShip(&master_q, hMasterShip);

	return val;
}

FRAME
GetShipIconsFromIndex(unsigned Index)
{
	HMASTERSHIP hMasterShip {GetStarShipFromIndex(&master_q, Index)};
	if (!hMasterShip)
	{
		return 0;
	}

	const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hMasterShip)};
	FRAME val {MasterPtr->ShipInfo.icons};
	UnlockMasterShip(&master_q, hMasterShip);

	return val;
}

FRAME
GetShipMeleeIconsFromIndex(unsigned Index)
{
	HMASTERSHIP hMasterShip {GetStarShipFromIndex(&master_q, Index)};
	if (!hMasterShip)
	{
		return 0;
	}

	const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hMasterShip)};
	FRAME val {MasterPtr->ShipInfo.melee_icon};
	UnlockMasterShip(&master_q, hMasterShip);

	return val;
}
