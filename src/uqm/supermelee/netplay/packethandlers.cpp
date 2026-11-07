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

#include "port.h"

#define NETCONNECTION_INTERNAL
#include "netplay.h"
#include "packethandlers.h"

#include "netinput.h"
#include "netmisc.h"
#include "packetsenders.h"
#include "proto/npconfirm.h"
#include "proto/ready.h"
#include "proto/reset.h"
#include "core/log/log.h"

#include "../../controls.h"
// for BATTLE_INPUT_STATE
#include "../../init.h"
// for NumPlayers
#include "../../globdata.h"
// for GLOBAL
#include "../melee.h"
// for various update functions.
#include "../meleeship.h"
// for MeleeShip
#include "../pickmele.h"
// for various update functions.
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


static bool testNetState(bool condition, PacketType type)
{
	if (!condition)
	{
		uqm::log::error("Packet of type '{}' received from wrong "
						"state.\n",
						packetTypeData[*type].name);
	}
	return condition;
}

static int versionCompare(int major1, int minor1, float patch1,
						  int major2, int minor2, float patch2)
{
	if (major1 < major2)
	{
		return -1;
	}
	if (major1 > major2)
	{
		return 1;
	}

	if (minor1 < minor2)
	{
		return -1;
	}
	if (minor1 > minor2)
	{
		return 1;
	}

	if (patch1 < patch2)
	{
		return -1;
	}
	if (patch1 > patch2)
	{
		return 1;
	}

	return 0;
}

NetResult PacketHandler_Init(NetConnection* conn, const Packet_Init* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::Init && !conn->stateFlags.ready.remoteReady, PacketType::Init))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (packet->protoVersion.major != NetplayProtocolVersionMajor || packet->protoVersion.minor != NetplayProtocolVersionMinor)
	{
		sendAbort(conn, NetplayAbortReason::VersionMismatch);
		abortFeedback(conn, NetplayAbortReason::VersionMismatch);
		uqm::log::error("Protocol version {}.{} not supported.\n",
						packet->protoVersion.major, packet->protoVersion.minor);
		return uqm::failure(uqstl::errc::function_not_supported);
	}

	if (versionCompare(packet->uqmVersion.major, packet->uqmVersion.minor,
					   packet->uqmVersion.patch, NetplayMinUqmVersionMajor,
					   NetplayMinUqmVersionMinor, NetplayMinUqmVersionPatch)
		< 0)
	{
		sendAbort(conn, NetplayAbortReason::VersionMismatch);
		abortFeedback(conn, NetplayAbortReason::VersionMismatch);
		uqm::log::error("Remote side is running a version of UQM that "
						"is too old ({}.{}.{}; {}.{}.{} is required).\n",
						packet->uqmVersion.major, packet->uqmVersion.minor,
						packet->uqmVersion.patch, NetplayMinUqmVersionMajor,
						NetplayMinUqmVersionMinor, NetplayMinUqmVersionPatch);
		return uqm::failure(uqstl::errc::function_not_supported);
	}

	Netplay_remoteReady(conn);

	return {};
}

NetResult PacketHandler_Ping(NetConnection* conn, const Packet_Ping* packet)
{
	if (!testNetState(conn->state > NetState::Init, PacketType::Ping))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	sendAck(conn, packet->id);
	return {};
}

NetResult PacketHandler_Ack(NetConnection* conn, const Packet_Ack* packet)
{
	if (!testNetState(conn->state > NetState::Init, PacketType::Ack))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	(void)conn;
	(void)packet;
	return {};
}

// Convert the side indication relative to a remote party to
// a local player number.
static inline int localSide(NetConnection* conn, NetplaySide side)
{
	if (side == NetplaySide::Local)
	{
		// "local" relative to the remote party.
		return conn->player;
	}

	return 1 - conn->player;
}

NetResult PacketHandler_Ready(NetConnection* conn, const Packet_Ready* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(readyFlagsMeaningful(conn->state) && !conn->stateFlags.ready.remoteReady, PacketType::Ready))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	Netplay_remoteReady(conn);

	(void)packet;
	// Its contents is not interesting.

	return {};
}

NetResult PacketHandler_Fleet(NetConnection* conn, const Packet_Fleet* packet)
{
	uint16_t numShips = uqm::convertBigEndian<uint16_t>(packet->numShips);

	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::InSetup, PacketType::Fleet))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	const int player {localSide(conn, (NetplaySide)packet->side)};

	const size_t len {packetLength((const Packet*)packet)};
	if (sizeof *packet + numShips * sizeof(FleetEntry) > len)
	{
		// There is not enough room in the packet to contain all
		// the ships it says it contains.
		uqm::log::warn("Invalid fleet size. Specified size is {}, "
					   "actual size = {}\n",
					   numShips, (int)((len - sizeof *packet) / sizeof(FleetEntry)));
		return uqm::failure(uqstl::errc::bad_message);
	}

	const BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};

	if (conn->stateFlags.handshake.localOk)
	{
		static_cast<void>(Netplay_cancelConfirmation(conn));
		confirmationCancelled(battleStateData->meleeState, conn->player);
	}

	for (size_t i {0}; i < numShips; i++)
	{
		MeleeShip ship = (MeleeShip)fleetEntries(packet)[i].ship;
		FleetShipIndex index = (FleetShipIndex)fleetEntries(packet)[i].index;

		if (!MeleeShip_valid(ship))
		{
			uqm::log::warn("Invalid ship type number {} (max = {}).", static_cast<int>(ship), NumMeleeShips - 1);
			return uqm::failure(uqstl::errc::bad_message);
		}

		if (index >= MeleeFleetSize)
		{
			uqm::log::warn("Invalid ship position number {} "
						   "(max = {}).\n",
						   index, MeleeFleetSize - 1);
			return uqm::failure(uqstl::errc::bad_message);
		}

		Melee_RemoteChange_ship(battleStateData->meleeState, conn,
								player, index, ship);
	}

	// Padding data may follow; it is ignored.
	return {};
}

NetResult PacketHandler_TeamName(NetConnection* conn, const Packet_TeamName* packet)
{
	size_t nameLen {};
	int side {};

	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::InSetup, PacketType::Fleet))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	const BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};

	if (conn->stateFlags.handshake.localOk)
	{
		static_cast<void>(Netplay_cancelConfirmation(conn));
		confirmationCancelled(battleStateData->meleeState, conn->player);
	}

	side = localSide(conn, (NetplaySide)packet->side);
	nameLen = packetLength((const Packet*)packet)
			- sizeof(Packet_TeamName) - 1;
	// The -1 is for not counting the terminating '\0'.

	{
		char buf[MaxTeamChars + 1];

		if (nameLen > MaxTeamChars)
		{
			nameLen = MaxTeamChars;
		}
		memcpy(buf, teamName(packet), nameLen);
		buf[nameLen] = '\0';

		Melee_RemoteChange_teamName(battleStateData->meleeState, conn,
									side, buf);
	}

	// Padding data may follow; it is ignored.
	return {};
}

static void handshakeComplete(NetConnection* conn)
{
	assert(!conn->stateFlags.handshake.localOk);
	assert(!conn->stateFlags.handshake.remoteOk);
	assert(!conn->stateFlags.handshake.canceling);

	assert(conn->state == NetState::InSetup);
	NetConnection_setState(conn, NetState::PreBattle);
}

NetResult PacketHandler_Handshake0(NetConnection* conn,
								   const Packet_Handshake0* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(handshakeMeaningful(conn->state)
						  && !conn->stateFlags.handshake.remoteOk,
					  PacketType::Handshake0))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	conn->stateFlags.handshake.remoteOk = true;
	if (conn->stateFlags.handshake.localOk && !conn->stateFlags.handshake.canceling)
	{
		sendHandshake1(conn);
	}

	(void)packet;
	// Its contents is not interesting.

	return {};
}

NetResult PacketHandler_Handshake1(NetConnection* conn,
								   const Packet_Handshake1* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(handshakeMeaningful(conn->state) && (conn->stateFlags.handshake.localOk || conn->stateFlags.handshake.canceling), PacketType::Handshake1))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (conn->stateFlags.handshake.canceling)
	{
		conn->stateFlags.handshake.remoteOk = true;
	}
	else
	{
		const bool remoteWasOk = conn->stateFlags.handshake.remoteOk;

		conn->stateFlags.handshake.localOk = false;
		conn->stateFlags.handshake.remoteOk = false;

		if (!remoteWasOk)
		{
			// Received Handshake1 without prior Handshake0.
			// A Handshake0 is implied, but we still need to confirm
			// it with a Handshake1.
			sendHandshake1(conn);
		}

		handshakeComplete(conn);
	}

	(void)packet;
	// Its contents is not interesting.

	return {};
}

NetResult PacketHandler_HandshakeCancel(NetConnection* conn,
										const Packet_HandshakeCancel* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(handshakeMeaningful(conn->state)
						  && conn->stateFlags.handshake.remoteOk,
					  PacketType::HandshakeCancel))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	conn->stateFlags.handshake.remoteOk = false;
	sendHandshakeCancelAck(conn);

	(void)packet;
	// Its contents is not interesting.

	return {};
}

NetResult PacketHandler_HandshakeCancelAck(NetConnection* conn,
										   const Packet_HandshakeCancelAck* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(handshakeMeaningful(conn->state)
						  && conn->stateFlags.handshake.canceling,
					  PacketType::HandshakeCancelAck))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	conn->stateFlags.handshake.canceling = false;
	if (conn->stateFlags.handshake.localOk)
	{
		if (conn->stateFlags.handshake.remoteOk)
		{
			sendHandshake1(conn);
		}
		else
		{
			sendHandshake0(conn);
		}
	}

	(void)packet;
	// Its contents is not interesting.

	return {};
}

NetResult PacketHandler_SeedRandom(NetConnection* conn,
								   const Packet_SeedRandom* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::PreBattle && !conn->stateFlags.discriminant, PacketType::SeedRandom))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	const BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};
	updateRandomSeed(battleStateData->meleeState, conn->player,
					 uqm::convertBigEndian<uint32_t>(packet->seed));

	conn->stateFlags.agreement.randomSeed = true;
	return {};
}

NetResult PacketHandler_InputDelay(NetConnection* conn,
								   const Packet_InputDelay* packet)
{
	//BattleStateData *battleStateData;
	uint32_t delay {};

	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::PreBattle, PacketType::InputDelay))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	// battleStateData = (BattleStateData *) NetConnection_getStateData(conn);
	delay = uqm::convertBigEndian<uint32_t>(packet->delay);
	if (delay > BattleFrameRateTicks.count())
	{
		uqm::log::error("NETPLAY: [{}]     Received absurdly large "
						"input delay value ({}).\n",
						conn->player, delay);
		return uqm::failure(uqstl::errc::bad_message);
	}
	conn->stateFlags.inputDelay = delay;

	return {};
}

NetResult PacketHandler_SelectShip(NetConnection* conn,
								   const Packet_SelectShip* packet)
{
	bool updateResult {};

	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::SelectShip, PacketType::SelectShip))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	const BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};
	updateResult = updateMeleeSelection(battleStateData->getMeleeState,
										conn->player, uqm::convertBigEndian<uint16_t>(packet->ship));
	if (!updateResult)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	return {};
}

NetResult PacketHandler_BattleInput(NetConnection* conn,
									const Packet_BattleInput* packet)
{
	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::InBattle || conn->state == NetState::EndingBattle || conn->state == NetState::EndingBattle2, PacketType::BattleInput))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	const BATTLE_INPUT_STATE input {(BATTLE_INPUT_STATE)packet->state};
	BattleInputBuffer* bib {getBattleInputBuffer(conn->player)};
	if (!BattleInputBuffer_push(bib, input))
	{
		return uqm::failure(uqstl::errc::no_buffer_space);
	}

	return {};
}

NetResult PacketHandler_FrameCount(NetConnection* conn,
								   const Packet_FrameCount* packet)
{
	BattleFrameCounter frameCount {};

	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(conn->state == NetState::EndingBattle,
					  PacketType::FrameCount))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	frameCount = (BattleFrameCounter)uqm::convertBigEndian<uint32_t>(packet->frameCount);
#ifdef NETPLAY_DEBUG
	uqm::log::debug("NETPLAY: [{}] <== Received battleFrameCount {}.\n",
					conn->player, (unsigned int)frameCount);
#endif

	BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};
	if (frameCount > battleStateData->endFrameCount)
	{
		battleStateData->endFrameCount = frameCount;
	}
	Netplay_remoteReady(conn);

	return {};
}

NetResult PacketHandler_Checksum(NetConnection* conn, const Packet_Checksum* packet)
{
#ifdef NETPLAY_CHECKSUM
	uint32_t frameNr {};
	uint32_t checksum {};
	size_t delay {};
	size_t interval {};
#endif

	if (conn->stateFlags.reset.localReset)
	{
		return {};
	}
	if (conn->stateFlags.reset.remoteReset)
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (!testNetState(NetState_battleActive(conn->state), PacketType::Checksum))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

#ifdef NETPLAY_CHECKSUM
	frameNr = uqm::convertBigEndian<uint32_t>(packet->frameNr);
	checksum = uqm::convertBigEndian<uint32_t>(packet->checksum);
	interval = NetConnection_getChecksumInterval(conn);
	delay = getBattleInputDelay();

	if (frameNr % interval != 0)
	{
		uqm::log::warn("NETPLAY: [{}] <== Received checksum "
					   "for frame {}, while we only expect checksums on frames "
					   "divisable by {} -- discarding.\n",
					   conn->player,
					   (unsigned int)frameNr, (unsigned int)interval);
		return {};
		// No need to close the connection; checksums are not
		// essential.
	}

	// The checksum is sent at the beginning of a frame.
	// If we're in frame n and have sent our input already,
	// the remote side has got enough input to progress delay + 1 frames from
	// frame n. The next frame is then n + delay + 1, for which we can
	// receive a checksum.
	if (frameNr > battleFrameCount + delay + 1)
	{
		uqm::log::warn("NETPLAY: [{}] <== Received checksum "
					   "for a frame too far in the future (frame {}, current "
					   "is {}, input delay is {}) -- discarding.\n",
					   conn->player,
					   (unsigned int)frameNr, (unsigned int)battleFrameCount, (unsigned int)delay);
		return {};
		// No need to close the connection; checksums are not
		// essential.
	}

	// We can progress delay more frames after the last frame for which we
	// received input. If we call that frame n, we can complete frames
	// n through n + delay - 1. While we are waiting for the next input,
	// in frame n + delay,  we will first receive the checksum that the
	// remote side sent at the start of frame n + 1.
	// In this situation frameNr is n + 1, and battleFrameCount is
	// n + delay.
	if (frameNr + delay < battleFrameCount)
	{
		uqm::log::warn("NETPLAY: [{}] <== Received checksum "
					   "for a frame too far in the past (frame {}, current "
					   "is {}, input delay is {}) -- discarding.",
					   conn->player,
					   (unsigned int)frameNr, (unsigned int)battleFrameCount, (unsigned int)delay);
		return {};
		// No need to close the connection; checksums are not
		// essential.
	}

	addRemoteChecksum(conn, frameNr, checksum);
#endif

#ifndef NETPLAY_CHECKSUM
	(void)packet;
#endif
	return {};
}

NetResult PacketHandler_Abort(NetConnection* conn, const Packet_Abort* packet)
{
	abortFeedback(conn, (NetplayAbortReason)packet->reason);

	return uqm::failure(uqstl::errc::bad_message);
	// Close connection.
}

NetResult PacketHandler_Reset(NetConnection* conn, const Packet_Reset* packet)
{
	NetplayResetReason reason;

	if (!testNetState(!conn->stateFlags.reset.remoteReset, PacketType::Reset))
	{
		return uqm::failure(uqstl::errc::bad_message);
	}

	reason = (NetplayResetReason)uqm::convertBigEndian<uint16_t>(packet->reason);

	Netplay_remoteReset(conn, reason);
	return {};
}
