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

#define NETCONNECTION_INTERNAL
#include "netplay.h"
#include "port.h"

#include "netsend.h"
#include "netconnection.h"
#include "packet.h"
#include "core/log/log.h"
#include "core/platform/platform.h"

#include <assert.h>
#include <string.h>


NetResult sendPacket(NetConnection* conn, Packet* packet)
{
	assert(NetConnection_isConnected(conn));

#ifdef NETPLAY_DEBUG
	//if (packetType(packet) != PACKET_BATTLEINPUT &&
	//		packetType(packet) != PACKET_CHECKSUM) {
	//	// Reporting BattleInput or Checksum would get so spammy that it
	//	// would slow down the battle.
	//	uqm::log::debug( "NETPLAY: [{}] ==> Sending packet of type {}.\n",
	//			conn->player, packetTypeData[packetType(packet)].name);
	//}
#ifdef NETPLAY_DEBUG_FILE
	if (conn->debugFile != nullptr)
	{
		netplayDebugPrint(conn->debugFile,
						  "NETPLAY: [{}] ==> Sending packet of type {}.\n",
						  conn->player, packetTypeData[packetType(packet)].name);
	}
#endif /* NETPLAY_DEBUG_FILE */
#endif /* NETPLAY_DEBUG */

	const auto* const data {reinterpret_cast<const std::byte*>(packet)};
	const std::error_code error {conn->connection->send(uqstl::span<const std::byte> {data, packetLength(packet)})};
	if (error)
	{
		if (error != std::errc::connection_reset)
		{
			uqm::log::error("send() failed: {}.", error.message());
		}
		return uqstl::unexpected {error};
	}

#ifdef NETPLAY_STATISTICS
	NetConnection_getStatistics(conn)->packetsSent++;
	NetConnection_getStatistics(conn)->packetTypeSent[packetType(packet)]++;
#endif

	return {};
}
