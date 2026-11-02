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

#ifndef UQM_SUPERMELEE_NETPLAY_PACKET_H_
#define UQM_SUPERMELEE_NETPLAY_PACKET_H_

#if 0 //defined(__cplusplus)
extern "C" {
#endif

typedef struct Packet Packet;

typedef enum PacketType
{
	PACKET_INIT,
	PACKET_PING,
	PACKET_ACK,
	PACKET_READY,
	PACKET_FLEET,
	PACKET_TEAMNAME,
	PACKET_HANDSHAKE0,
	PACKET_HANDSHAKE1,
	PACKET_HANDSHAKECANCEL,
	PACKET_HANDSHAKECANCELACK,
	PACKET_SEEDRANDOM,
	PACKET_INPUTDELAY,
	PACKET_SELECTSHIP,
	PACKET_BATTLEINPUT,
	PACKET_FRAMECOUNT,
	PACKET_CHECKSUM,
	PACKET_ABORT,
	PACKET_RESET,

	PACKET_NUM, /* Number of packet types */
} PacketType;

// Sent before aborting the connection.
typedef enum NetplayAbortReason
{
	AbortReason_unspecified,
	AbortReason_versionMismatch,
	AbortReason_invalidHash,
	AbortReason_protocolError,
	// Network is in an inconsistent state.
} NetplayAbortReason;

// Sent before resetting the connection. A game in progress is terminated.
typedef enum NetplayResetReason
{
	ResetReason_unspecified,
	ResetReason_syncLoss,
	ResetReason_manualReset,
} NetplayResetReason;

#if 0 //defined(__cplusplus)
}
#endif

#ifndef PACKET_H_STANDALONE
#include "netconnection.h"

#include <cstdint>
#include "libs/network/bytesex.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

/* NB: These handlers are expected not to modify the state if an
 *     error occurs.
 * When a handler is called, it has already been validated that the
 * a complete packet has arrived.
 */
typedef int (*PacketHandler)(NetConnection* conn, const void* packet);

typedef struct
{
	size_t len; /* Minimal length of a packet of this type */
	PacketHandler handler;
	const char* name;
} PacketTypeData;

extern PacketTypeData packetTypeData[];

#if 0 //defined(__cplusplus)
}
#endif

#endif

#if 0 //defined(__cplusplus)
extern "C" {
#endif

// When adding new packets, be sure to have all the fields properly aligned,
// and that the size of a packet is a multiple of 4 bytes in length.
// Fields should be no longer than 4 bytes in themselves, as larger
// fields may require a larger alignment.

typedef struct
{
	uint16_t len;
	uint16_t type; /* Actually of type PacketType */
} PacketHeader;

// "Base class" for all packets.
struct Packet
{
	PacketHeader header;
};

static inline size_t
packetLength(const Packet* packet)
{
	return (size_t)ntoh16(packet->header.len);
}

static inline PacketType
packetType(const Packet* packet)
{
	return (PacketType)(int)ntoh16(packet->header.type);
}

static inline bool
validPacketType(PacketType type)
{
	return type < PACKET_NUM;
}

typedef struct
{
	PacketHeader header;
	struct
	{
		uint8_t major;
		uint8_t minor;
	} protoVersion;	   /* Protocol version */
	uint16_t padding0; /* Set to 0 */
	struct
	{
		uint8_t major;
		uint8_t minor;
		uint8_t patch;
	} uqmVersion;	  /* Protocol version */
	uint8_t padding1; /* Set to 0 */
} Packet_Init;

typedef struct
{
	PacketHeader header;
	uint32_t id;
} Packet_Ping;

// Acknowledgement of a Ping.
typedef struct
{
	PacketHeader header;
	uint32_t id;
} Packet_Ack;

// Used to signal that a party is ready to continue.
typedef struct
{
	PacketHeader header;
	// No contents.
} Packet_Ready;

typedef struct
{
	PacketHeader header;
	uint32_t seed;
} Packet_SeedRandom;

typedef struct
{
	PacketHeader header;
	uint32_t delay;
} Packet_InputDelay;

// This enum is used to indicate that a packet containing it relates to
// either the local or the remote player, from the perspective of the
// sender of the message;
typedef enum
{
	NetplaySide_local,
	NetplaySide_remote
} NetplaySide;

typedef struct
{
	uint8_t index; /* Position in the fleet */
	uint8_t ship;  /* Ship type index; actually MeleeShip */
} FleetEntry;
// Structure describing an update to a player's fleet.
// TODO: use strings as ship identifiers, instead of numbers,
// so that adding of new ships doesn't break this.
typedef struct
{
	PacketHeader header;
	uint8_t side;
	uint8_t padding;
	uint16_t numShips;
	// The fleet entries follow the structure. See fleetEntries().
	// Be sure to add padding to this structure to make it a multiple of
	// 4 bytes in length.
} Packet_Fleet;

static_assert(sizeof(Packet_Fleet) == 8, "The size of the packet header is part of the protocol.");

// The fleet entries that follow the structure.
inline FleetEntry* fleetEntries(Packet_Fleet* packet)
{
	return reinterpret_cast<FleetEntry*>(packet + 1);
}

inline const FleetEntry* fleetEntries(const Packet_Fleet* packet)
{
	return reinterpret_cast<const FleetEntry*>(packet + 1);
}

typedef struct
{
	PacketHeader header;
	uint8_t side;
	uint8_t padding;
	// The '\0'-terminated name follows the structure. See teamName().
	// Be sure to add padding to this structure to make it a multiple of
	// 4 bytes in length.
} Packet_TeamName;

static_assert(sizeof(Packet_TeamName) == 6, "The size of the packet header is part of the protocol.");

// The team name that follows the structure.
inline uint8_t* teamName(Packet_TeamName* packet)
{
	return reinterpret_cast<uint8_t*>(packet + 1);
}

inline const uint8_t* teamName(const Packet_TeamName* packet)
{
	return reinterpret_cast<const uint8_t*>(packet + 1);
}

typedef struct
{
	PacketHeader header;
	// No contents.
} Packet_Handshake0;

typedef struct
{
	PacketHeader header;
	// No contents.
} Packet_Handshake1;

typedef struct
{
	PacketHeader header;
	// No contents.
} Packet_HandshakeCancel;

typedef struct
{
	PacketHeader header;
	// No contents.
} Packet_HandshakeCancelAck;

typedef struct
{
	PacketHeader header;
	uint16_t ship;
	// The value '(uint16_t) ~0' indicates random selection.
	uint16_t padding;
} Packet_SelectShip;

typedef struct
{
	PacketHeader header;
	uint8_t state; /* Actually BATTLE_INPUT_STATE */
	uint8_t padding0;
	uint16_t padding1;
} Packet_BattleInput;

typedef struct
{
	PacketHeader header;
	uint32_t frameCount; /* Actually BattleFrameCounter */
} Packet_FrameCount;

typedef struct
{
	PacketHeader header;
	uint32_t frameNr;  /* Actually BattleFrameCounter */
	uint32_t checksum; /* Actually Checksum */
} Packet_Checksum;

typedef struct
{
	PacketHeader header;
	uint16_t reason; /* Actually NetplayAbortReason */
	uint16_t padding0;
} Packet_Abort;

typedef struct
{
	PacketHeader header;
	uint16_t reason; /* Actually NetplayResetReason */
	uint16_t padding0;
} Packet_Reset;


#ifndef PACKET_H_STANDALONE
void Packet_delete(Packet* packet);
Packet_Init* Packet_Init_create(void);
Packet_Ping* Packet_Ping_create(uint32_t id);
Packet_Ack* Packet_Ack_create(uint32_t id);
Packet_Ready* Packet_Ready_create(void);
Packet_Handshake0* Packet_Handshake0_create(void);
Packet_Handshake1* Packet_Handshake1_create(void);
Packet_HandshakeCancel* Packet_HandshakeCancel_create(void);
Packet_HandshakeCancelAck* Packet_HandshakeCancelAck_create(void);
Packet_SeedRandom* Packet_SeedRandom_create(uint32_t seed);
Packet_InputDelay* Packet_InputDelay_create(uint32_t delay);
Packet_Fleet* Packet_Fleet_create(NetplaySide side, size_t numShips);
Packet_TeamName* Packet_TeamName_create(NetplaySide side, const char* name,
										size_t size);
Packet_SelectShip* Packet_SelectShip_create(uint16_t ship);
Packet_BattleInput* Packet_BattleInput_create(uint8_t state);
Packet_FrameCount* Packet_FrameCount_create(uint32_t frameCount);
Packet_Checksum* Packet_Checksum_create(uint32_t frameNr, uint32_t checksum);
Packet_Abort* Packet_Abort_create(uint16_t reason);
Packet_Reset* Packet_Reset_create(uint16_t reason);
#endif

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_SUPERMELEE_NETPLAY_PACKET_H_ */
