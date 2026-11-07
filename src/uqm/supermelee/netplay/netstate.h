/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
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
#ifndef UQM_SUPERMELEE_NETPLAY_NETSTATE_H
#define UQM_SUPERMELEE_NETPLAY_NETSTATE_H

#include "core/types/enum.h"
#include "port.h"

namespace uqm
{
typedef struct NetConnectionStateData NetConnectionStateData;

// State of a NetConnection.
enum class NetState
{
	Unconnected,   /* No connection initiated */
	Connecting,	   /* Connection being setup */
	Init,		   /* Initialising the connection */
	InSetup,	   /* In the network game setup */
	PreBattle,	   /* Pre-battle initialisations */
	InterBattle,   /* Negotiations between battles. */
	SelectShip,	   /* Selecting a ship in battle */
	InBattle,	   /* Battle has started */
	EndingBattle,  /* Both sides are prepared to end */
	EndingBattle2, /* Waiting for the final synchronisation */
};
consteval bool isArithmeticEnum(NetState) { return true; }

} // namespace uqm

#include <cstdint>

namespace uqm
{
struct NetStateData
{
	const char* name;
};
extern NetStateData netStateData[];

typedef void (*NetConnectionStateData_ReleaseFunction)(
	NetConnectionStateData* stateData);

struct
	NetConnectionStateData
{
	NetConnectionStateData_ReleaseFunction releaseFunction;
};

void NetConnectionStateData_release(NetConnectionStateData* stateData);

static inline bool NetState_battleActive(NetState state)
{
	return state == NetState::InBattle || state == NetState::EndingBattle || state == NetState::EndingBattle2;
}

} // namespace uqm

#endif // UQM_SUPERMELEE_NETPLAY_NETSTATE_H
