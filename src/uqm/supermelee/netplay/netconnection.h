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
#ifndef UQM_SUPERMELEE_NETPLAY_NETCONNECTION_H
#define UQM_SUPERMELEE_NETPLAY_NETCONNECTION_H

#include "netplay.h"

namespace uqm
{
// for NETPLAY_STATISTICS

struct NetConnection;
struct NetConnectionError;
#ifdef NETPLAY_STATISTICS
struct NetStatistics;
#endif

typedef void (*NetConnection_ConnectCallback)(NetConnection* nd);
typedef void (*NetConnection_CloseCallback)(NetConnection* nd);
typedef void (*NetConnection_ErrorCallback)(NetConnection* nd,
											const NetConnectionError* error);
typedef void (*NetConnection_DeleteCallback)(NetConnection* nd);

typedef void (*NetConnection_ReadyCallback)(NetConnection* conn, void* arg);
typedef void (*NetConnection_ResetCallback)(NetConnection* conn, void* arg);

} // namespace uqm

#include "netstate.h"
#include "options/netoptions.h"
#ifdef NETPLAY_CHECKSUM
#include "checkbuf.h"
#endif
#if defined(NETPLAY_STATISTICS) || defined(NETCONNECTION_INTERNAL)
#include "packet.h"
#endif
#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
#include "libs/io/FileSystem.h"
#endif

namespace uqm
{
struct NetConnectionError
{
	NetState state;
	int err;
};

#ifdef NETPLAY_STATISTICS
struct NetStatistics
{
	size_t packetsReceived;
	uqstl::array<size_t, *PacketType::Count> packetTypeReceived;
	size_t packetsSent;
	uqstl::array<size_t, *PacketType::Count> packetTypeSent;
};
#endif

} // namespace uqm

#ifdef NETCONNECTION_INTERNAL
#include "core/stl/memory.h"
#include "libs/network/Connection.h"
#include "libs/network/Connector.h"
#include "libs/network/Listener.h"
#include "packetq.h"


namespace uqm
{
struct HandShakeFlags
{
	// For actions that require agreement by both parties.
	bool localOk   :1; /* Action confirmed by us */
	bool remoteOk  :1; /* Action confirmed by the remote party */
	bool canceling :1; /* Awaiting cancel confirmation */
};

struct ReadyFlags
{
	// For actions that do not require agreement, but for which it
	// only is relevant that both sides are ready.
	bool localReady	 :1;
	bool remoteReady :1;
};

struct ResetFlags
{
	bool localReset	 :1;
	bool remoteReset :1;
};

// Which parameters have we both sides of a connection reached agreement on?
struct Agreement
{
	bool randomSeed :1;
};

struct NetStateFlags
{
	bool connected;
	/* This NetConnection is connected. */
	bool disconnected;
	/* This NetConnection has been disconnected. This implies
			 * !connected. It is only set if the NetConnection was once
			 * connected, but is no longer. */
	bool discriminant;
	/* If it is true here, it is false on the remote side
			 * of the same connection. It may be used to break ties.
			 * It is guaranteed not to change during a connection. Undefined
			 * while not connected. */
	HandShakeFlags handshake;
	ReadyFlags ready;
	ResetFlags reset;
	Agreement agreement;
	size_t inputDelay;
	/* Used during negotiation of the actual inputDelay. This
			 * field does NOT necessarilly contain the actual input delay,
			 * which is a property of the game, not of any specific
			 * connection. Use getBattleInputDelay() to get at it. */
#ifdef NETPLAY_CHECKSUM
	size_t checksumInterval;
#endif
};

struct NetConnection
{
	uqstl::unique_ptr<uqm::net::Connection> connection;
	// The listener or the connector while the connection is not established.
	uqstl::unique_ptr<uqm::net::Listener> listener;
	uqstl::unique_ptr<uqm::net::Connector> connector;
	int player;
	// Number of the player for this connection, as it is
	// known locally. For the other player, it may be
	// differently.
	NetState state;
	NetStateFlags stateFlags;

	NetConnection_ReadyCallback readyCallback;
	// Called when both sides have indicated that they are ready.
	// Set by Netplay_localReady().
	void* readyCallbackArg;
	// Extra argument for readyCallback().
	// XXX: when is this cleaned up if a connection is broken?

	NetConnection_ResetCallback resetCallback;
	// Called when a reset has been signalled and confirmed.
	// Set by Netplay_localReset().
	void* resetCallbackArg;
	// Extra argument for resetCallback().
	// XXX: when is this cleaned up if a connection is broken?

	const uqm::NetplayPeerOptions* options;
	PacketQueue queue;
#ifdef NETPLAY_STATISTICS
	NetStatistics statistics;
#endif
#ifdef NETPLAY_CHECKSUM
	ChecksumBuffer checksumBuffer;
#endif
#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
	uqgsl::owner<uqm::io::File*> debugFile;
#endif

	NetConnection_ConnectCallback connectCallback;
	NetConnection_CloseCallback closeCallback;
	// Called when the NetConnection becomes disconnected.
	NetConnection_ErrorCallback errorCallback;
	NetConnection_DeleteCallback deleteCallback;
	// Called when the NetConnection is destroyed.
	NetConnectionStateData* stateData;
	// State dependant information.
	void* extra;
};

} // namespace uqm

#endif /* NETCONNECTION_INTERNAL */


namespace uqm
{
NetConnection* NetConnection_open(int player,
								  const uqm::NetplayPeerOptions* options,
								  NetConnection_ConnectCallback connectCallback,
								  NetConnection_CloseCallback closeCallback,
								  NetConnection_ErrorCallback errorCallback,
								  NetConnection_DeleteCallback deleteCallback, void* extra);
void NetConnection_close(NetConnection* conn);
bool NetConnection_isConnected(const NetConnection* conn);

void NetConnection_doErrorCallback(NetConnection* nd, int err);

void NetConnection_setStateData(NetConnection* conn,
								NetConnectionStateData* stateData);
NetConnectionStateData* NetConnection_getStateData(const NetConnection* conn);
void NetConnection_setExtra(NetConnection* conn, void* extra);
void* NetConnection_getExtra(const NetConnection* conn);
void NetConnection_setState(NetConnection* conn, NetState state);
NetState NetConnection_getState(const NetConnection* conn);
bool NetConnection_getDiscriminant(const NetConnection* conn);
const uqm::NetplayPeerOptions* NetConnection_getPeerOptions(const NetConnection* conn);
int NetConnection_getPlayerNr(const NetConnection* conn);
size_t NetConnection_getInputDelay(const NetConnection* conn);
#ifdef NETPLAY_CHECKSUM
ChecksumBuffer* NetConnection_getChecksumBuffer(NetConnection* conn);
size_t NetConnection_getChecksumInterval(const NetConnection* conn);
#endif
#ifdef NETPLAY_STATISTICS
NetStatistics* NetConnection_getStatistics(NetConnection* conn);
#endif

void NetConnection_setReadyCallback(NetConnection* conn,
									NetConnection_ReadyCallback callback, void* arg);
NetConnection_ReadyCallback NetConnection_getReadyCallback(
	const NetConnection* conn);
void* NetConnection_getReadyCallbackArg(const NetConnection* conn);

void NetConnection_setResetCallback(NetConnection* conn,
									NetConnection_ResetCallback callback, void* arg);
NetConnection_ResetCallback NetConnection_getResetCallback(
	const NetConnection* conn);
void* NetConnection_getResetCallbackArg(const NetConnection* conn);


#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
extern uqm::io::File* netplayDebugFile;

// Writes formatted text to a netplay debug file, if 'file' is not nullptr.
template <typename... Args>
void netplayDebugPrint(uqm::io::File* file, fmt::format_string<Args...> format, Args&&... args)
{
	if (file != nullptr)
	{
		const uqstl::string text {fmt::format(format, uqstl::forward<Args>(args)...)};
		file->write(uqstl::as_bytes(uqstl::span {text}));
	}
}
#endif

} // namespace uqm

#endif // UQM_SUPERMELEE_NETPLAY_NETCONNECTION_H
