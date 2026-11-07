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
#ifndef UQM_OPTIONS_NETOPTIONS_H
#define UQM_OPTIONS_NETOPTIONS_H

#ifdef NETPLAY
#include <cstdint>
#include <stddef.h>

#include "core/stl/vector.h"

namespace CLI
{
class App;
}

namespace uqm
{

struct NetAddress
{
	uqstl::string host {"localhost"};
	uint16_t port {21837};
};

struct NetplayPeerOptions
{
	bool isServer {true};
	NetAddress address {};
	// May be given as a service name.
};

class NetplayOptions
{
public:
	NetplayOptions(size_t playerCount);

	void configureCommands(CLI::App& app);

	const uqstl::vector<NetplayPeerOptions>& getPeers() const { return m_peers; }
	uqstl::vector<NetplayPeerOptions>& editPeers() { return m_peers; }
	size_t getFrameInputDelay() const { return m_inputDelayFrames; }
	void setFrameInputDelay(size_t newDelay);
	size_t getMaxFrameInputDelay() const;

private:
	NetAddress m_metaServerAddress {
		{"", // was "uqm.stack.nl".. not sure what that is.. not sure what "meta server" is, frankly...
		 21836}
	   };
	size_t m_playerCount;
	size_t m_inputDelayFrames {2};
	uqstl::vector<NetplayPeerOptions> m_peers {};
};

} // namespace uqm

#endif /* NETPLAY */
#endif // UQM_OPTIONS_NETOPTIONS_H
