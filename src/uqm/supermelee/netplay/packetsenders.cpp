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
#include "packetsenders.h"

#include "packet.h"
#include "packetq.h"
#include "netsend.h"


void sendInit(NetConnection* conn)
{
	Packet_Init* packet;

	packet = Packet_Init_create();
	queuePacket(conn, (Packet*)packet);
}

void sendPing(NetConnection* conn, uint32_t id)
{
	Packet_Ping* packet;

	packet = Packet_Ping_create(id);
	queuePacket(conn, (Packet*)packet);
}

void sendAck(NetConnection* conn, uint32_t id)
{
	Packet_Ack* packet;

	packet = Packet_Ack_create(id);
	queuePacket(conn, (Packet*)packet);
}

void sendReady(NetConnection* conn)
{
	Packet_Ready* packet;

	packet = Packet_Ready_create();
	queuePacket(conn, (Packet*)packet);
}

void sendHandshake0(NetConnection* conn)
{
	Packet_Handshake0* packet;

	packet = Packet_Handshake0_create();
	queuePacket(conn, (Packet*)packet);
}

void sendHandshake1(NetConnection* conn)
{
	Packet_Handshake1* packet;

	packet = Packet_Handshake1_create();
	queuePacket(conn, (Packet*)packet);
}

void sendHandshakeCancel(NetConnection* conn)
{
	Packet_HandshakeCancel* packet;

	packet = Packet_HandshakeCancel_create();
	queuePacket(conn, (Packet*)packet);
}

void sendHandshakeCancelAck(NetConnection* conn)
{
	Packet_HandshakeCancelAck* packet;

	packet = Packet_HandshakeCancelAck_create();
	queuePacket(conn, (Packet*)packet);
}

void sendTeamName(NetConnection* conn, NetplaySide side, const char* name,
				  size_t len)
{
	Packet_TeamName* packet;

	packet = Packet_TeamName_create(side, name, len);
	queuePacket(conn, (Packet*)packet);
}

void sendFleet(NetConnection* conn, NetplaySide side, const MeleeShip* ships,
			   size_t shipCount)
{
	size_t i;
	Packet_Fleet* packet;

	packet = Packet_Fleet_create(side, shipCount);

	for (i = 0; i < shipCount; i++)
	{
		fleetEntries(packet)[i].index = (uint8_t)i;
		fleetEntries(packet)[i].ship = (uint8_t)ships[i];
	}

	queuePacket(conn, (Packet*)packet);
}

void sendFleetShip(NetConnection* conn, NetplaySide side,
				   FleetShipIndex shipIndex, MeleeShip ship)
{
	Packet_Fleet* packet;

	packet = Packet_Fleet_create(side, 1);

	fleetEntries(packet)[0].index = (uint8_t)shipIndex;
	fleetEntries(packet)[0].ship = (uint8_t)ship;

	queuePacket(conn, (Packet*)packet);
}

void sendSeedRandom(NetConnection* conn, uint32_t seed)
{
	Packet_SeedRandom* packet;

	packet = Packet_SeedRandom_create(seed);
	queuePacket(conn, (Packet*)packet);
}

void sendInputDelay(NetConnection* conn, uint32_t delay)
{
	Packet_InputDelay* packet;

	packet = Packet_InputDelay_create(delay);
	queuePacket(conn, (Packet*)packet);
}

void sendSelectShip(NetConnection* conn, FleetShipIndex index)
{
	Packet_SelectShip* packet;

	packet = Packet_SelectShip_create((uint16_t)index);
	queuePacket(conn, (Packet*)packet);
}

void sendBattleInput(NetConnection* conn, BATTLE_INPUT_STATE input)
{
	Packet_BattleInput* packet;

	packet = Packet_BattleInput_create((uint8_t)input);
	queuePacket(conn, (Packet*)packet);
}

void sendFrameCount(NetConnection* conn, BattleFrameCounter frameCount)
{
	Packet_FrameCount* packet;

	packet = Packet_FrameCount_create((uint32_t)frameCount);
	queuePacket(conn, (Packet*)packet);
}

#ifdef NETPLAY_CHECKSUM
void sendChecksum(NetConnection* conn, BattleFrameCounter frameNr,
				  Checksum checksum)
{
	Packet_Checksum* packet;

	packet = Packet_Checksum_create((uint32_t)frameNr, (uint32_t)checksum);
	queuePacket(conn, (Packet*)packet);
}
#endif

void sendAbort(NetConnection* conn, NetplayAbortReason reason)
{
	Packet_Abort* packet;

	packet = Packet_Abort_create((uint16_t)reason);
	queuePacket(conn, (Packet*)packet);
}

void sendReset(NetConnection* conn, NetplayResetReason reason)
{
	Packet_Reset* packet;

	packet = Packet_Reset_create((uint16_t)reason);
	queuePacket(conn, (Packet*)packet);
}
