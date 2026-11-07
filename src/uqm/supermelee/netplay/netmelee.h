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
#ifndef UQM_SUPERMELEE_NETPLAY_NETMELEE_H
#define UQM_SUPERMELEE_NETPLAY_NETMELEE_H

#if defined(NETPLAY)

#include "../../battlecontrols.h"
#include "netconnection.h"
#include "netinput.h"
#include "netplay.h"
#include "packetsenders.h"
// for NetworkInputContext
#include "../../controls.h"
// for BattleInputState
#include "../../races.h"

namespace uqm
{
// for Starship

extern struct NetConnection* netConnections[];


void addNetConnection(NetConnection* conn, int playerNr);
void removeNetConnection(int playerNr);
void closeAllConnections(void);
void closeDisconnectedConnections(void);
size_t getNumNetConnections(void);
typedef bool (*ForEachConnectionCallback)(NetConnection* conn, void* arg);
bool forEachConnectedPlayer(ForEachConnectionCallback callback, void* arg);

struct MeleeState* NetMelee_getMeleeState(NetConnection* conn);
struct BattleState* NetMelee_getBattleState(NetConnection* conn);

void netInput(void);
void netInputBlocking(uint32_t timeoutMs);
void flushPacketQueues(void);

void confirmConnections(void);
void cancelConfirmations(void);
void connectionsLocalReady(NetConnection_ReadyCallback callback, void* arg);

bool allConnected(void);

void initBattleStateDataConnections(void);
void setBattleStateConnections(struct BattleState* bs);

BattleInputState networkBattleInput(NetworkInputContext* context,
									Starship* StarShipPtr);

NetConnection* openPlayerNetworkConnection(uint16_t player, void* extra);
void closePlayerNetworkConnection(uint16_t player);

bool setupInputDelay(size_t localInputDelay);
bool setStateConnections(NetState state);
bool sendAbortConnections(NetplayAbortReason reason);
bool resetConnections(NetplayResetReason reason);
bool localReadyConnections(NetConnection_ReadyCallback readyCallback,
						   void* arg, bool notifyRemote);

bool negotiateReady(NetConnection* conn, bool notifyRemote,
					NetState nextState);
bool negotiateReadyConnections(bool notifyRemote, NetState nextState);
bool waitReady(NetConnection* conn);

bool waitReset(NetConnection* conn, NetState nextState);
bool waitResetConnections(NetState nextState);

} // namespace uqm

#endif

#endif // UQM_SUPERMELEE_NETPLAY_NETMELEE_H
