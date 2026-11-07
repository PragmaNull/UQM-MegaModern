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
#include "packet.h"

#include "uqmversion.h"

#include "packethandlers.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


namespace uqm
{
#define DEFINE_PACKETDATA(name)                               \
	{                                                         \
		/* .len = */ sizeof(Packet_##name),                   \
		/* .handler = */ (PacketHandler)PacketHandler_##name, \
		/* .name = */ #name,                                  \
	}
PacketTypeData packetTypeData[*PacketType::Count] = {
	DEFINE_PACKETDATA(Init),
	DEFINE_PACKETDATA(Ping),
	DEFINE_PACKETDATA(Ack),
	DEFINE_PACKETDATA(Ready),
	DEFINE_PACKETDATA(Fleet),
	DEFINE_PACKETDATA(TeamName),
	DEFINE_PACKETDATA(Handshake0),
	DEFINE_PACKETDATA(Handshake1),
	DEFINE_PACKETDATA(HandshakeCancel),
	DEFINE_PACKETDATA(HandshakeCancelAck),
	DEFINE_PACKETDATA(SeedRandom),
	DEFINE_PACKETDATA(InputDelay),
	DEFINE_PACKETDATA(SelectShip),
	DEFINE_PACKETDATA(BattleInput),
	DEFINE_PACKETDATA(FrameCount),
	DEFINE_PACKETDATA(Checksum),
	DEFINE_PACKETDATA(Abort),
	DEFINE_PACKETDATA(Reset),
};

static inline void* Packet_alloc(size_t size)
{
	return malloc(size);
}

static Packet* Packet_create(PacketType type, size_t extraSize)
{
	size_t len {};

	// Alignment requirement.
	assert(extraSize % 4 == 0);

	len = packetTypeData[*type].len + extraSize;
	Packet* result {(Packet*)Packet_alloc(len)};
	result->header.len = uqm::convertBigEndian<uint16_t>((uint16_t)len);
	result->header.type = uqm::convertBigEndian<uint16_t>((uint16_t)type);
	return result;
}

void Packet_delete(Packet* packet)
{
	free(packet);
}

Packet_Init* Packet_Init_create(void)
{
	Packet_Init* packet = (Packet_Init*)Packet_create(PacketType::Init, 0);

	packet->protoVersion.major = NetplayProtocolVersionMajor;
	packet->protoVersion.minor = NetplayProtocolVersionMinor;
	packet->padding0 = 0;
	packet->uqmVersion.major = UqmMajorVersion;
	packet->uqmVersion.minor = UqmMinorVersion;
	packet->uqmVersion.patch = UqmPatchVersion;
	packet->padding1 = 0;
	return packet;
}

Packet_Ping* Packet_Ping_create(uint32_t id)
{
	Packet_Ping* packet = (Packet_Ping*)Packet_create(PacketType::Ping, 0);

	packet->id = uqm::convertBigEndian<uint32_t>(id);
	return packet;
}

Packet_Ack* Packet_Ack_create(uint32_t id)
{
	Packet_Ack* packet = (Packet_Ack*)Packet_create(PacketType::Ack, 0);

	packet->id = uqm::convertBigEndian<uint32_t>(id);
	return packet;
}

Packet_Ready* Packet_Ready_create(void)
{
	Packet_Ready* packet = (Packet_Ready*)Packet_create(PacketType::Ready, 0);

	return packet;
}

// The fleet itself still needs to be filled by the caller.
// This function takes care of the necessary padding; it is allocated,
// and filled with zero chars.
Packet_Fleet* Packet_Fleet_create(NetplaySide side, size_t numShips)
{
	const size_t fleetSize {numShips * sizeof(FleetEntry)};
	const size_t extraSize {(fleetSize + 3) & ~0x03};
	Packet_Fleet* packet {(Packet_Fleet*)Packet_create(PacketType::Fleet, extraSize)};
	packet->side = (uint8_t)side;
	packet->padding = 0;
	packet->numShips = uqm::convertBigEndian<uint16_t>((uint16_t)numShips);
	memset((char*)packet + sizeof(Packet_Fleet) + fleetSize,
		   '\0', extraSize - fleetSize);

	return packet;
}

// 'size' is the number of bytes (not characters) in 'name', excluding
// a possible terminating '\0'. A '\0' will be included in the packet though.
// This function takes care of the required padding.
Packet_TeamName* Packet_TeamName_create(NetplaySide side, const char* name, size_t size)
{
	const size_t extraSize {((size + 1) + 3) & ~0x03};
	// The +1 is for the '\0'.
	Packet_TeamName* packet {(Packet_TeamName*)Packet_create(PacketType::TeamName, extraSize)};
	packet->side = (uint8_t)side;
	packet->padding = 0;
	memcpy(teamName(packet), name, size);
	memset((char*)packet + sizeof(Packet_TeamName) + size, '\0',
		   extraSize - size);
	// This takes care of the terminating '\0', as well as the
	// padding.

	return packet;
}

Packet_Handshake0* Packet_Handshake0_create(void)
{
	Packet_Handshake0* packet =
		(Packet_Handshake0*)Packet_create(PacketType::Handshake0, 0);
	return packet;
}

Packet_Handshake1* Packet_Handshake1_create(void)
{
	Packet_Handshake1* packet =
		(Packet_Handshake1*)Packet_create(PacketType::Handshake1, 0);
	return packet;
}

Packet_HandshakeCancel* Packet_HandshakeCancel_create(void)
{
	Packet_HandshakeCancel* packet =
		(Packet_HandshakeCancel*)Packet_create(
			PacketType::HandshakeCancel, 0);
	return packet;
}

Packet_HandshakeCancelAck* Packet_HandshakeCancelAck_create(void)
{
	Packet_HandshakeCancelAck* packet =
		(Packet_HandshakeCancelAck*)Packet_create(
			PacketType::HandshakeCancelAck, 0);
	return packet;
}

Packet_SeedRandom* Packet_SeedRandom_create(uint32_t seed)
{
	Packet_SeedRandom* packet =
		(Packet_SeedRandom*)Packet_create(PacketType::SeedRandom, 0);

	packet->seed = uqm::convertBigEndian<uint32_t>(seed);
	return packet;
}

Packet_InputDelay* Packet_InputDelay_create(uint32_t delay)
{
	Packet_InputDelay* packet =
		(Packet_InputDelay*)Packet_create(PacketType::InputDelay, 0);

	packet->delay = uqm::convertBigEndian<uint32_t>(delay);
	return packet;
}

Packet_SelectShip* Packet_SelectShip_create(uint16_t ship)
{
	Packet_SelectShip* packet =
		(Packet_SelectShip*)Packet_create(PacketType::SelectShip, 0);
	packet->ship = uqm::convertBigEndian<uint16_t>(ship);
	packet->padding = 0;
	return packet;
}

Packet_BattleInput* Packet_BattleInput_create(uint8_t state)
{
	Packet_BattleInput* packet =
		(Packet_BattleInput*)Packet_create(PacketType::BattleInput, 0);
	packet->state = (uint8_t)state;
	packet->padding0 = 0;
	packet->padding1 = 0;
	return packet;
}

Packet_FrameCount* Packet_FrameCount_create(uint32_t frameCount)
{
	Packet_FrameCount* packet =
		(Packet_FrameCount*)Packet_create(PacketType::FrameCount, 0);
	packet->frameCount = uqm::convertBigEndian<uint32_t>(frameCount);
	return packet;
}

Packet_Checksum* Packet_Checksum_create(uint32_t frameNr, uint32_t checksum)
{
	Packet_Checksum* packet =
		(Packet_Checksum*)Packet_create(PacketType::Checksum, 0);
	packet->frameNr = uqm::convertBigEndian<uint32_t>(frameNr);
	packet->checksum = uqm::convertBigEndian<uint32_t>(checksum);
	return packet;
}

Packet_Abort* Packet_Abort_create(uint16_t reason)
{
	Packet_Abort* packet = (Packet_Abort*)Packet_create(PacketType::Abort, 0);
	packet->reason = uqm::convertBigEndian<uint16_t>(reason);
	return packet;
}

Packet_Reset* Packet_Reset_create(uint16_t reason)
{
	Packet_Reset* packet = (Packet_Reset*)Packet_create(PacketType::Reset, 0);
	packet->reason = uqm::convertBigEndian<uint16_t>(reason);
	return packet;
}

} // namespace uqm
