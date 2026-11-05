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

#ifndef UQM_GRPINFO_H_
#define UQM_GRPINFO_H_

#include "port.h"
#include "core/misc/bittricks.h"
#include "displist.h"
#include "libs/gfxlib.h"
// for GFXPOINT
#include <assert.h>

#if 0 //defined(__cplusplus)
extern "C" {
#endif

// XXX: Needed to maintain savegame compatibility
inline constexpr auto NumSavedBattleGroups {64};

typedef HLINK HIPGROUP;

typedef uint8_t IP_FLAGS;

inline constexpr auto CanTurn {(1 << 0)};
inline constexpr auto Rotates {(1 << 1)};
inline constexpr auto IsProbe {(1 << 2)};
inline constexpr auto IsXform {(1 << 3)};

typedef struct
{
	// LINK elements; must be first
	HIPGROUP pred;
	HIPGROUP succ;

	uint16_t group_counter;
	uint8_t race_id;
	uint8_t sys_loc;
	uint8_t task; // AKA mission
	uint8_t in_system;
	// a simple != 0 flag
	// In older savegames this will be >1, because
	//   CloneShipFragment was used to spawn groups,
	//   and it set this to crew_level values

	uint8_t dest_loc;
	uint8_t orbit_pos;
	/* Also: saved prev dest_loc before intercept call,
			 *   restored to dest_loc on all-clear */
	uint8_t group_id;
	GFXPOINT loc;

	FRAME melee_icon;

	IP_FLAGS flags;
} IP_GROUP;

enum
{
	IN_ORBIT = 0,
	EXPLORE,
	FLEE,
	ON_STATION,

	IGNORE_FLAGSHIP = 1 << 2,
	REFORM_GROUP = 1 << 3
};
inline constexpr auto MaxRevolutions {5};

inline constexpr auto StationRadius {1600};
inline constexpr auto OrbitRadius {2400};

static inline IP_GROUP*
LockIpGroup(const QUEUE* pq, HIPGROUP h)
{
	assert(GetLinkSize(pq) == sizeof(IP_GROUP));
	return (IP_GROUP*)LockLink(pq, h);
}

constexpr auto UnlockIpGroup(const auto& pq, const auto& h) { return UnlockLink(pq, h); }
constexpr auto FreeIpGroup(const auto& pq, const auto& h) { return FreeLink(pq, h); }

extern HIPGROUP BuildGroup(QUEUE* pDstQueue, uint8_t race_id);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_GRPINFO_H_ */
