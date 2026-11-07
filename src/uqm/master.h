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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once
#ifndef UQM_MASTER_H
#define UQM_MASTER_H

#include "races.h"
#include "core/misc/bittricks.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

using MasterShipHandle = LinkHandle;

struct MasterShipInfo
{
	// QueueLink elements; must be first
	MasterShipHandle pred;
	MasterShipHandle succ;

	SpeciesId SpeciesID;

	ShipInfoDesc ShipInfo;
	FleetStatus Fleet;
	// FleetStatus is only necessary here because avail_race_q
	// is initialized in part from master_q (kinda hacky)
};

extern Queue master_q;
/* List of ships available in SuperMelee;
		 * queue Element is MasterShipInfo */

static inline MasterShipInfo* LockMasterShip(const Queue* pq, MasterShipHandle h)
{
	assert(GetLinkSize(pq) == sizeof(MasterShipInfo));
	return (MasterShipInfo*)LockLink(pq, h);
}

constexpr auto UnlockMasterShip(const auto& pq, const auto& h) { return UnlockLink(pq, h); }


extern void LoadMasterShipList(void (*YieldProcessing)(void));
extern void FreeMasterShipList(void);
extern void ReloadMasterShipList(void (*YieldProcessing)(void));
extern MasterShipHandle FindMasterShip(SpeciesId ship_ref);
extern int FindMasterShipIndex(SpeciesId ship_ref);
uint16_t GetShipCostFromIndex(unsigned Index);
FrameRef GetShipIconsFromIndex(unsigned Index);
FrameRef GetShipMeleeIconsFromIndex(unsigned Index);

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_MASTER_H
