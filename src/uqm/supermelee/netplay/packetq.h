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
#ifndef UQM_SUPERMELEE_NETPLAY_PACKETQ_H
#define UQM_SUPERMELEE_NETPLAY_PACKETQ_H

#include <cstdint>
#include <sys/types.h>

#include "netplay.h"

namespace uqm
{
struct NetConnection;
struct Packet;

struct PacketQueueLink
{
	PacketQueueLink* next;
	Packet* packet;
};

struct PacketQueue
{
	size_t size;
	PacketQueueLink* first;
	PacketQueueLink** end;

	// first points to the first entry in the queue
	// end points to the location where the next message should be inserted.
};

void PacketQueue_init(PacketQueue* queue);
void PacketQueue_uninit(PacketQueue* queue);
void queuePacket(NetConnection* conn, Packet* packet);
NetResult flushPacketQueue(NetConnection* conn);

} // namespace uqm

#endif // UQM_SUPERMELEE_NETPLAY_PACKETQ_H
