/*
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "notifyall.h"

#include "netmelee.h"
#include "notify.h"

// Notify the network connections of a team name change.
void Netplay_NotifyAll_setTeamName(MeleeState* pMS, size_t playerNr)
{
	const char* name {MeleeSetup_getTeamName(pMS->meleeSetup, playerNr)};
	const size_t len {strlen(name)};
	for (size_t playerI {0}; playerI < NumPlayers; playerI++)
	{
		NetConnection* conn = netConnections[playerI];

		if (conn == nullptr)
		{
			continue;
		}

		if (!NetConnection_isConnected(conn))
		{
			continue;
		}

		if (NetConnection_getState(conn) != NetState_inSetup)
		{
			continue;
		}

		Netplay_Notify_setTeamName(conn, playerNr, name, len);
	}
}

// Notify the network connections of the configuration of a fleet.
void Netplay_NotifyAll_setFleet(MeleeState* pMS, size_t playerNr)
{
	const MeleeSetup* setup = pMS->meleeSetup;
	const MeleeShip* ships = MeleeSetup_getFleet(setup, playerNr);

	for (size_t playerI {0}; playerI < NumPlayers; playerI++)
	{
		NetConnection* conn = netConnections[playerI];

		if (conn == nullptr)
		{
			continue;
		}

		if (!NetConnection_isConnected(conn))
		{
			continue;
		}

		if (NetConnection_getState(conn) != NetState_inSetup)
		{
			continue;
		}

		Netplay_Notify_setFleet(conn, playerNr, ships, MeleeFleetSize);
	}
}

// Notify the network of a change in the configuration of a fleet.
void Netplay_NotifyAll_setShip(MeleeState* pMS, size_t playerNr, size_t index)
{
	const MeleeSetup* setup = pMS->meleeSetup;
	const MeleeShip ship = MeleeSetup_getShip(
		setup, playerNr, (FleetShipIndex)index);

	for (size_t playerI {0}; playerI < NumPlayers; playerI++)
	{
		NetConnection* conn = netConnections[playerI];

		if (conn == nullptr)
		{
			continue;
		}

		if (!NetConnection_isConnected(conn))
		{
			continue;
		}

		if (NetConnection_getState(conn) != NetState_inSetup)
		{
			continue;
		}

		Netplay_Notify_setShip(
			conn, playerNr, (FleetShipIndex)index, ship);
	}
}

static bool Netplay_NotifyAll_inputDelayCallback(NetConnection* conn, void* arg)
{
	const size_t* delay = (size_t*)arg;
	Netplay_Notify_inputDelay(conn, *delay);
	return true;
}

bool Netplay_NotifyAll_inputDelay(size_t delay)
{
	return forEachConnectedPlayer(Netplay_NotifyAll_inputDelayCallback,
								  &delay);
}

#ifdef NETPLAY_CHECKSUM
void Netplay_NotifyAll_checksum(BattleFrameCounter frameNr, Checksum checksum)
{
	for (uint16_t player {0}; player < NumPlayers; player++)
	{
		NetConnection* conn = netConnections[player];
		if (conn == nullptr)
		{
			continue;
		}

		if (!NetConnection_isConnected(conn))
		{
			continue;
		}

		Netplay_Notify_checksum(conn, frameNr, checksum);
	}
}
#endif /* NETPLAY_CHECKSUM */

void Netplay_NotifyAll_battleInput(BATTLE_INPUT_STATE input)
{
	for (uint16_t player {0}; player < NumPlayers; player++)
	{
		NetConnection* conn = netConnections[player];
		if (conn == nullptr)
		{
			continue;
		}

		if (!NetConnection_isConnected(conn))
		{
			continue;
		}

		Netplay_Notify_battleInput(conn, input);
	}
}
