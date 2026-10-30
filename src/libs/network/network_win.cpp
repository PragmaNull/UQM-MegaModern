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

#include "netport.h"

#include "network.h"

#include "core/misc/bittricks.h"
#include "core/misc/FatalExit.h"
#include "core/log/log.h"

#include <errno.h>
#include <winsock2.h>

void Network_init(void)
{
	WSADATA data;
	int startupResult;
	WORD requestVersion = MAKEWORD(2, 2);

	startupResult = WSAStartup(requestVersion, &data);
	if (startupResult != 0)
	{
		int savedErrno = winsockErrorToErrno(startupResult);
		uqm::log::critical("WSAStartup failed.");
		errno = savedErrno;
		uqm::fatalExit();
	}

#ifdef DEBUG
	uqm::log::debug("Winsock version {}.{} found: \"{}\".",
					uqm::lowByte(data.wHighVersion), uqm::highByte(data.wHighVersion),
					data.szDescription);
	uqm::log::debug("Requesting to use Winsock version {}.{}, got "
					"version {}.{}.",
					uqm::lowByte(requestVersion), uqm::highByte(requestVersion),
					uqm::lowByte(data.wVersion), uqm::highByte(data.wVersion));
#endif
	if (data.wVersion != requestVersion)
	{
		uqm::log::critical("Winsock version {}.{} presented, requested "
						   "{}.{}.",
						   uqm::lowByte(data.wVersion), uqm::highByte(data.wVersion),
						   uqm::lowByte(requestVersion), uqm::highByte(requestVersion));
		(void)WSACleanup();
		// Ignoring errors; we're going to abort anyhow.
		uqm::fatalExit();
	}
}

void Network_uninit(void)
{
	int cleanupResult;

	cleanupResult = WSACleanup();
	if (cleanupResult == SOCKET_ERROR)
	{
		int savedErrno = getWinsockErrno();
		uqm::log::critical("WSACleanup failed.");
		errno = savedErrno;
		uqm::fatalExit();
	}
}
