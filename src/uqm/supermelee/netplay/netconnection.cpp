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
#include "netconnection.h"

#include "netrcv.h"

#if defined(DEBUG) || defined(NETPLAY_DEBUG)
#include "core/log/log.h"
#endif
#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
#include "options.h"
// for configDir
#endif

#include <assert.h>
#include <stdlib.h>
#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
#include <errno.h>
#include <time.h>
#endif


static void closeCallback(NetDescriptor* nd);
static void NetConnection_doClose(NetConnection* conn);

#include "core/platform/platform.h"
#include "core/string/StringUtils.h"
#include "nc_connect.ci"

#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
uqm::io::File* netplayDebugFile;
#endif

// Used as initial value for Agreement structures, by structure assignment.
const Agreement Agreement_nothingAgreed {};


// The NetConnection keeps a pointer to the passed NetplayPeerOptions;
// do not free it as long as the NetConnection exists.
NetConnection*
NetConnection_open(int player, const uqm::NetplayPeerOptions* options,
				   NetConnection_ConnectCallback connectCallback,
				   NetConnection_CloseCallback closeCallback,
				   NetConnection_ErrorCallback errorCallback,
				   NetConnection_DeleteCallback deleteCallback, void* extra)
{
	NetConnection* conn;

	conn = (NetConnection*)malloc(sizeof(NetConnection));

#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
	{
		char dumpFileName[PATH_MAX];
		time_t now;
		struct tm* nowTm;
		size_t strftimeResult;

		now = time(nullptr);
		if (now == (time_t)-1)
		{
			uqm::log::critical("time() failed: {}.", uqm::strerror(errno));
			abort();
		}

		nowTm = localtime(&now);
		// XXX: I would like to use localtime_r(), but it isn't very
		// portable (yet), and adding a check for it to the build.sh script
		// is not worth the effort for a debugging function right now.

		strftimeResult = strftime(dumpFileName, sizeof dumpFileName, "debug/netlog-%Y%m%e%H%M%S", nowTm);
		if (strftimeResult == 0)
		{
			uqm::log::critical("strftime() failed: {}.", uqm::strerror(errno));
			abort();
		}

		// The user needs to create the debug/ dir manually. If there
		// is no debug/ dir, no log will be created.
		uqstl::error_code error {};
		uqstl::optional<uqm::io::File> debugFile {configDir->open(dumpFileName, uqm::io::OpenMode::Write | uqm::io::OpenMode::Create | uqm::io::OpenMode::Truncate | uqm::io::OpenMode::Text, error)};
		conn->debugFile = debugFile ? new uqm::io::File {uqstl::move(*debugFile)} : nullptr;
		if (conn->debugFile == nullptr)
		{
			uqm::log::debug("Not creating a netplay debug log for "
							"player {}.",
							player);
		}
		else
		{
			uqm::log::debug("Creating netplay debug log '{}' for "
							"player {}.",
							dumpFileName, player);
			if (netplayDebugFile == nullptr)
			{
				// Debug info relating to no specific network connection
				// is sent to the first opened one.
				netplayDebugFile = conn->debugFile;
			}
		}
	}
#endif

	conn->nd = nullptr;
	conn->player = player;
	conn->state = NetState_unconnected;
	conn->options = options;
	conn->extra = extra;
	PacketQueue_init(&conn->queue);

	conn->connectCallback = connectCallback;
	conn->closeCallback = closeCallback;
	conn->errorCallback = errorCallback;
	conn->deleteCallback = deleteCallback;
	conn->readyCallback = nullptr;
	conn->readyCallbackArg = nullptr;
	conn->resetCallback = nullptr;
	conn->resetCallbackArg = nullptr;

	conn->readBuf = (uint8_t*)malloc(NETPLAY_READBUFSIZE);
	conn->readEnd = conn->readBuf;

	conn->stateData = nullptr;
	conn->stateFlags.connected = false;
	conn->stateFlags.disconnected = false;
	conn->stateFlags.discriminant = false;
	conn->stateFlags.handshake.localOk = false;
	conn->stateFlags.handshake.remoteOk = false;
	conn->stateFlags.handshake.canceling = false;
	conn->stateFlags.ready.localReady = false;
	conn->stateFlags.ready.remoteReady = false;
	conn->stateFlags.reset.localReset = false;
	conn->stateFlags.reset.remoteReset = false;
	conn->stateFlags.agreement = Agreement_nothingAgreed;
	conn->stateFlags.inputDelay = 0;
#ifdef NETPLAY_CHECKSUM
	conn->stateFlags.checksumInterval = NETPLAY_CHECKSUM_INTERVAL;
#endif

#ifdef NETPLAY_STATISTICS
	{
		size_t i;

		conn->statistics.packetsReceived = 0;
		conn->statistics.packetsSent = 0;
		for (i = 0; i < PACKET_NUM; i++)
		{
			conn->statistics.packetTypeReceived[i] = 0;
			conn->statistics.packetTypeSent[i] = 0;
		}
	}
#endif

	NetConnection_go(conn);

	return conn;
}

static void
NetConnection_doDeleteCallback(NetConnection* conn)
{
	if (conn->deleteCallback != nullptr)
	{
		//NetConnection_incRef(conn);
		conn->deleteCallback(conn);
		//NetConnection_decRef(conn);
	}
}

static void
NetConnection_delete(NetConnection* conn)
{
	NetConnection_doDeleteCallback(conn);
	if (conn->stateData != nullptr)
	{
		NetConnectionStateData_release(conn->stateData);
		conn->stateData = nullptr;
	}
	free(conn->readBuf);
	PacketQueue_uninit(&conn->queue);

#ifdef NETPLAY_DEBUG_FILE
	if (conn->debugFile != nullptr)
	{
		if (netplayDebugFile == conn->debugFile)
		{
			// There may be other network connections, with an open
			// debug file, but we don't know about that.
			// The debugging person just has to work around that.
			netplayDebugFile = nullptr;
		}
		delete conn->debugFile;
	}
#endif

	free(conn);
}

static void
Netplay_doCloseCallback(NetConnection* conn)
{
	if (conn->closeCallback != nullptr)
	{
		//NetConnection_incRef(conn);
		conn->closeCallback(conn);
		//NetConnection_decRef(conn);
	}
}

// Auxiliary function for closing, used by both closeCallback() and
// NetConnection_close()
static void
NetConnection_doClose(NetConnection* conn)
{
	conn->stateFlags.connected = false;
	conn->stateFlags.disconnected = true;

	// First the callback, so that it can still use the information
	// of what is the current state, and the stateData:
	Netplay_doCloseCallback(conn);

	NetConnection_setState(conn, NetState_unconnected);
}

// Called when the NetDescriptor is shut down.
static void
closeCallback(NetDescriptor* nd)
{
	NetConnection* conn = (NetConnection*)NetDescriptor_getExtra(nd);
	if (conn == nullptr)
	{
		return;
	}
	conn->nd = nullptr;
	NetConnection_doClose(conn);
}

// Close and release a NetConnection.
void NetConnection_close(NetConnection* conn)
{
	if (conn->nd != nullptr)
	{
		NetDescriptor_setCloseCallback(conn->nd, nullptr);
		// We're not interested in the close callback of the
		// NetDescriptor anymore.
		NetDescriptor_close(conn->nd);
		// This would queue the close callback.
		conn->nd = nullptr;
	}
	if (!conn->stateFlags.disconnected)
	{
		NetConnection_doClose(conn);
	}
	NetConnection_delete(conn);
}

void NetConnection_doErrorCallback(NetConnection* nd, int err)
{
	NetConnectionError error;

	if (nd->errorCallback != nullptr)
	{
		error.state = nd->state;
		error.err = err;
	}
	(*nd->errorCallback)(nd, &error);
}

void NetConnection_setStateData(NetConnection* conn,
								NetConnectionStateData* stateData)
{
	conn->stateData = stateData;
}

NetConnectionStateData*
NetConnection_getStateData(const NetConnection* conn)
{
	return conn->stateData;
}

void NetConnection_setExtra(NetConnection* conn, void* extra)
{
	conn->extra = extra;
}

void* NetConnection_getExtra(const NetConnection* conn)
{
	return conn->extra;
}

void NetConnection_setReadyCallback(NetConnection* conn,
									NetConnection_ReadyCallback callback, void* arg)
{
	conn->readyCallback = callback;
	conn->readyCallbackArg = arg;
}

NetConnection_ReadyCallback
NetConnection_getReadyCallback(const NetConnection* conn)
{
	return conn->readyCallback;
}

void* NetConnection_getReadyCallbackArg(const NetConnection* conn)
{
	return conn->readyCallbackArg;
}

void NetConnection_setResetCallback(NetConnection* conn,
									NetConnection_ResetCallback callback, void* arg)
{
	conn->resetCallback = callback;
	conn->resetCallbackArg = arg;
}

NetConnection_ResetCallback
NetConnection_getResetCallback(const NetConnection* conn)
{
	return conn->resetCallback;
}

void* NetConnection_getResetCallbackArg(const NetConnection* conn)
{
	return conn->resetCallbackArg;
}

void NetConnection_setState(NetConnection* conn, NetState state)
{
#ifdef NETPLAY_DEBUG
	uqm::log::debug("NETPLAY: [{}] +/- Connection state changed to: "
					"{}.\n",
					conn->player, netStateData[state].name);
#endif
#ifdef DEBUG
	if (state == conn->state)
	{
		uqm::log::warn("NETPLAY: [{}]     Connection state set to {} "
					   "while already in that state.\n",
					   conn->player, netStateData[state].name);
	}
#endif
	conn->state = state;
}

NetState
NetConnection_getState(const NetConnection* conn)
{
	return conn->state;
}

bool NetConnection_getDiscriminant(const NetConnection* conn)
{
	return conn->stateFlags.discriminant;
}

const uqm::NetplayPeerOptions*
NetConnection_getPeerOptions(const NetConnection* conn)
{
	return conn->options;
}

bool NetConnection_isConnected(const NetConnection* conn)
{
	return conn->stateFlags.connected;
}

int NetConnection_getPlayerNr(const NetConnection* conn)
{
	return conn->player;
}

size_t
NetConnection_getInputDelay(const NetConnection* conn)
{
	return conn->stateFlags.inputDelay;
}

#ifdef NETPLAY_CHECKSUM
ChecksumBuffer*
NetConnection_getChecksumBuffer(NetConnection* conn)
{
	return &conn->checksumBuffer;
}

size_t
NetConnection_getChecksumInterval(const NetConnection* conn)
{
	return conn->stateFlags.checksumInterval;
}
#endif /* NETPLAY_CHECKSUM */

#ifdef NETPLAY_STATISTICS
NetStatistics*
NetConnection_getStatistics(NetConnection* conn)
{
	return &conn->statistics;
}
#endif
