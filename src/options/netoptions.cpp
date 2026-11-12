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

#include "netoptions.h"

#ifdef NETPLAY

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include "libs/time/timelib.h"


namespace uqm
{

static constexpr uint32_t BattleFPS {24}; // TODO: this is also defined in battle.h as BattleFrameRateTicks. Maybe we should unify these?
static constexpr size_t NetworkInputBufferMaxTicks {static_cast<size_t>(getTicksForFramerate(BattleFPS).count())};

NetplayOptions::NetplayOptions(size_t playerCount)
	: m_playerCount {playerCount}
	, m_peers(playerCount)
{
}

void NetplayOptions::configureCommands(CLI::App& app)
{
	// Netplay
	auto netplayGroup {app.add_option_group("Netplay")};
	m_peers.resize(m_playerCount);

	for (size_t n {0}; n < m_playerCount; ++n)
	{
		const size_t playerNum {n + 1};
		netplayGroup->add_option(fmt::format("--nethost{}", playerNum), m_peers[n].address.host, fmt::format("Server to connect to for player {} (1=bottom, 2=top)", playerNum));
		netplayGroup->add_option(fmt::format("--netport{}", playerNum), m_peers[n].address.port, fmt::format("Port to connect to/listen on for player {} (1=bottom, 2=top)", playerNum))
			->check(CLI::Range(1, 65535));
	}
	netplayGroup->add_option("--netdelay", m_inputDelayFrames, fmt::format("Number of frames to buffer/delay network input for. Default={}.", m_inputDelayFrames))
		->check(CLI::Range(size_t {0}, getMaxFrameInputDelay()));
}

size_t NetplayOptions::getMaxFrameInputDelay() const
{
	return NetworkInputBufferMaxTicks;
}

void NetplayOptions::setFrameInputDelay(size_t newDelay)
{
	m_inputDelayFrames = std::min(m_inputDelayFrames, getMaxFrameInputDelay());
}

} // namespace uqm

#endif /* NETPLAY */