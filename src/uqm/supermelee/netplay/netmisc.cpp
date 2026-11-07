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

#include "netplay.h"
#include "netmisc.h"

#include "netmelee.h"
#include "notifyall.h"
#include "packetsenders.h"
#include "proto/ready.h"

#include "../melee.h"
// For feedback functions.

#include <stdlib.h>


static BattleStateData* BattleStateData_alloc(void);
static void BattleStateData_free(BattleStateData* battleStateData);
static inline BattleStateData* BattleStateData_new(
	struct MeleeState* meleeState,
	struct BattleState* battleState,
	struct PickMeleeState* getMeleeState);
static void BattleStateData_delete(BattleStateData* battleStateData);


static BattleStateData* BattleStateData_alloc(void)
{
	return (BattleStateData*)malloc(sizeof(BattleStateData));
}

static void BattleStateData_free(BattleStateData* battleStateData)
{
	free(battleStateData);
}

static inline BattleStateData* BattleStateData_new(struct MeleeState* meleeState,
												   struct BattleState* battleState,
												   struct PickMeleeState* getMeleeState)
{
	BattleStateData* battleStateData = BattleStateData_alloc();
	battleStateData->releaseFunction =
		(NetConnectionStateData_ReleaseFunction)BattleStateData_delete;
	battleStateData->meleeState = meleeState;
	battleStateData->battleState = battleState;
	battleStateData->getMeleeState = getMeleeState;
	return battleStateData;
}

static void BattleStateData_delete(BattleStateData* battleStateData)
{
	BattleStateData_free(battleStateData);
}


////////////////////////////////////////////////////////////////////////////


static void NetMelee_enterState_inSetup(NetConnection* conn, void* arg);

// Called when a connection has been established.
void NetMelee_connectCallback(NetConnection* conn)
{
	struct MeleeState* meleeState {(struct MeleeState*)NetConnection_getExtra(conn)};
	BattleStateData* battleStateData {BattleStateData_new(meleeState, nullptr, nullptr)};
	NetConnection_setStateData(conn, (NetConnectionStateData*)battleStateData);
	NetConnection_setExtra(conn, nullptr);

	// We have sent no teams yet. Initialize the state accordingly.
	MeleeSetup_resetSentTeams(meleeState->meleeSetup);

	sendInit(conn);
	Netplay_localReady(conn, NetMelee_enterState_inSetup, nullptr, false);
}

// Called when a connection is closed.
void NetMelee_closeCallback(NetConnection* conn)
{
	closeFeedback(conn);
}

// Called when a network error occurs during connect.
void NetMelee_errorCallback(NetConnection* conn,
							const NetConnectionError* error)
{
	errorFeedback(conn);
	(void)error;
}

// Callback function for when both sides have finished initialisation after
// initial connect.
static void NetMelee_enterState_inSetup(NetConnection* conn, void* arg)
{
	struct MeleeState* meleeState {};

	NetConnection_setState(conn, NetState::InSetup);

	const BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};
	meleeState = battleStateData->meleeState;

	const int player {NetConnection_getPlayerNr(conn)};

	connectedFeedback(conn);

	// Send our team to the remote side.
	// XXX This only works with 2 players atm.
	assert(NumPlayers == 2);
	Melee_bootstrapSyncTeam(meleeState, player);

	flushPacketQueues();

	(void)arg;
}
