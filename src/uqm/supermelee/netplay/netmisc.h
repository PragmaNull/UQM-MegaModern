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

#ifndef UQM_SUPERMELEE_NETPLAY_NETMISC_H_
#define UQM_SUPERMELEE_NETPLAY_NETMISC_H_

typedef struct BattleStateData BattleStateData;

#include "netconnection.h"
#include "netstate.h"
#include <cstdint>

#include "../../battle.h"
// for BattleFrameCounter, BattleFrameRateTicks

#if 0 //defined(__cplusplus)
extern "C" {
#endif

struct BattleStateData : NetConnectionStateData
{
	struct MeleeState* meleeState;
	struct BattleState* battleState;
	struct PickMeleeState* getMeleeState;
	BattleFrameCounter endFrameCount;
};


void NetMelee_connectCallback(NetConnection* conn);
void NetMelee_closeCallback(NetConnection* conn);
void NetMelee_errorCallback(NetConnection* conn,
							const NetConnectionError* error);

void NetMelee_reenterState_inSetup(NetConnection* conn);


// Returns true iff the connection is in a state where the confirmation
// handshake is meaningful. Right now this is only when we're in the
// pre-game setup menu.
static inline bool handshakeMeaningful(NetState state)
{
	return state == NetState::InSetup;
}

static inline bool readyFlagsMeaningful(NetState state)
{
	return state == NetState::Init || state == NetState::PreBattle || state == NetState::SelectShip || state == NetState::InterBattle || state == NetState::InBattle || state == NetState::EndingBattle || state == NetState::EndingBattle2;
}


#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_SUPERMELEE_NETPLAY_NETMISC_H_ */
