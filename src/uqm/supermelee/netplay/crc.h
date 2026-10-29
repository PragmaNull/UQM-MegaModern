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

#ifndef UQM_SUPERMELEE_NETPLAY_CRC_H_
#define UQM_SUPERMELEE_NETPLAY_CRC_H_

typedef struct crc_State crc_State;

#include <cstdint>

#include <stddef.h>

#if 0 //defined(__cplusplus)
extern "C" {
#endif

struct crc_State
{
	uint32_t crc;
};

void crc_init(crc_State* state);
void crc_processBytes(crc_State* state, uint8_t* buf, size_t bufLen);
void crc_processUint8(crc_State* state, uint8_t val);
void crc_processUint16(crc_State* state, uint16_t val);
void crc_processUint32(crc_State* state, uint32_t val);
uint32_t crc_finish(const crc_State* state);

#if 0 //defined(__cplusplus)
}
#endif

#ifdef DUMP_CRC_OPS
#include "netconnection.h"
// for netplayDebugFile
//#define crc_log(...)  log_add (logDebug, __VA_ARGS__)
#define crc_log(...)                                      \
	do                                                    \
	{                                                     \
		netplayDebugPrint(netplayDebugFile, __VA_ARGS__); \
		netplayDebugPrint(netplayDebugFile, "\n");        \
	} while (false)
#endif

#endif /* UQM_SUPERMELEE_NETPLAY_CRC_H_ */
