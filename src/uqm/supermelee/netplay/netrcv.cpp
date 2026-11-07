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
#include "netrcv.h"

#include <cstdint>
#include <string.h>

#include "core/log/log.h"
#include "core/platform/platform.h"
#include "netconnection.h"
#include "netplay.h"
#include "packet.h"
#include "port.h"


namespace uqm
{
// Try to get a single packet from a stream of data.
// Returns the length of the packet that was processed, 0 if the packet is
// not complete, or an error, in which case the state is unchanged.
static uqstl::expected<size_t, uqstl::error_code> dataReceivedSingle(NetConnection* conn, uqstl::span<const uint8_t> data)
{
	uint32_t packetLen {};
	PacketType type;

	if (data.size() < sizeof(PacketHeader))
	{
		// Incomplete packet. We'll have to wait for the rest.
		return 0;
	}

	packetLen = packetLength(reinterpret_cast<const Packet*>(data.data()));
	type = packetType(reinterpret_cast<const Packet*>(data.data()));

	if (!validPacketType(type))
	{
		uqm::log::warn("Packet with invalid type {} received.\n", static_cast<int>(type));
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (packetLen < packetTypeData[*type].len)
	{
		// Bad len field of packet.
		uqm::log::warn("Packet with bad length field received (type="
					   "{}, lenfield={}.\n",
					   packetTypeData[*type].name,
					   packetLen);
		return uqm::failure(uqstl::errc::bad_message);
	}

	if (data.size() < packetLen)
	{
		// Incomplete packet. We'll have to wait for the rest.
		return 0;
	}

#ifdef NETPLAY_STATISTICS
	NetConnection_getStatistics(conn)->packetsReceived++;
	NetConnection_getStatistics(conn)->packetTypeReceived[*type]++;
#endif

#ifdef NETPLAY_DEBUG
	if (type != PacketType::BattleInput && type != PacketType::Checksum)
	{
		// Reporting BattleInput and Checksum would get so spammy that it
		// would slow down the battle.
		uqm::log::debug("NETPLAY: [{}] <== Received packet of type {}.\n",
						NetConnection_getPlayerNr(conn), packetTypeData[type].name);
	}
#ifdef NETPLAY_DEBUG_FILE
	if (conn->debugFile != nullptr)
	{
		netplayDebugPrint(conn->debugFile,
						  "NETPLAY: [{}] <== Received packet of type {}.\n",
						  NetConnection_getPlayerNr(conn), packetTypeData[type].name);
	}
#endif /* NETPLAY_DEBUG_FILE */
#endif /* NETPLAY_DEBUG */

	if (NetResult result {packetTypeData[*type].handler(conn, data.data())}; !result)
	{
		return uqstl::unexpected {result.error()};
	}

	return packetLen;
}

// Try to get all the packets from a stream of data.
// Returns the number of bytes processed.
static uqstl::expected<size_t, uqstl::error_code> dataReceivedMulti(NetConnection* conn, uqstl::span<const uint8_t> data)
{
	size_t processed {0};
	while (data.size() > 0)
	{
		const uqstl::expected<size_t, uqstl::error_code> single {dataReceivedSingle(conn, data)};
		if (!single)
		{
			return single;
		}
		const size_t packetLen {*single};

		if (packetLen == 0)
		{
			// No packet was processed. This means that no complete
			// packet arrived.
			break;
		}

		processed += packetLen;
		data = data.subspan(packetLen);
	}

	return processed;
}

uqstl::optional<size_t> dataReceived(NetConnection* conn, uqstl::span<const std::byte> data)
{
	const uqstl::expected<size_t, uqstl::error_code> processed {dataReceivedMulti(conn, {reinterpret_cast<const uint8_t*>(data.data()), data.size()})};
	if (!processed)
	{
		return uqstl::nullopt;
	}
	return *processed;
}

} // namespace uqm
