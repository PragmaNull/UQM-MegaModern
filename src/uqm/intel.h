//Copyright Paul Reiche, Fred Ford. 1992-2002

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
#pragma once
#ifndef UQM_INTEL_H_
#define UQM_INTEL_H_


#include "uqm/inteldefs.h"
#include "options/OptionDefs.h"
#include "battlecontrols.h"
#include "controls.h"
#include "element.h"
#include "races.h"
#include "setup.h"

constexpr auto Maneuverability(const auto& pi) { return ((pi)->ManeuverabilityIndex); }
constexpr auto GetWeaponRange(const auto& pi) { return ((pi)->WeaponRange); }

constexpr auto WorldToTurn(const auto& d) { return ((d) >> 6); }
constexpr auto ResolutionCompensated(const auto& speed) { return (ResScale(ResScale(speed))); }

extern BATTLE_INPUT_STATE computer_intelligence(
	ComputerInputContext* context, Starship* StarShipPtr);
extern BATTLE_INPUT_STATE tactical_intelligence(
	ComputerInputContext* context, Starship* StarShipPtr);
extern void ship_intelligence(Element* ShipPtr,
							  ObjectEvaluation* ObjectsOfConcern, uint16_t ConcernCounter);
extern bool ship_weapons(Element* ShipPtr, Element* OtherPtr,
						 uint16_t margin_of_error);

extern void Pursue(Element* ShipPtr, ObjectEvaluation* EvalDescPtr);
extern void Entice(Element* ShipPtr, ObjectEvaluation* EvalDescPtr);
extern void Avoid(Element* ShipPtr, ObjectEvaluation* EvalDescPtr);
extern bool TurnShip(Element* ShipPtr, uint16_t angle);
extern bool ThrustShip(Element* ShipPtr, uint16_t angle);

extern PlayerControlFlags PlayerControl[];

static inline bool antiCheatImpl(const Element* Element, const bool swapPlayers, const uqm::GodModeFlags testFlags, uqstl::span<PlayerControlFlags> playerControl, const uqm::GodModeFlags godModeData)
{
	// both players are AI-controlled
	if (testFlag(playerControl[0] & playerControl[1], ComputerControlFlags))
	{
		return false;
	}

	if (!testFlag(godModeData, testFlags))
	{
		return false;
	}

	if (Element == nullptr || Element->playerNr < 0 || Element->playerNr >= playerControl.size()) [[unlikely]]
	{
		return false;
	}

	assert(Element->playerNr == 0 || Element->playerNr == 1);
	const int elementOwner = Element->playerNr;
	const int opponent = elementOwner ^ 1;

	const int playerIndex = swapPlayers ? elementOwner : opponent;

	return testFlag(playerControl[playerIndex], ComputerControlFlags);
}

static inline bool antiCheat(Element* ElementPtr, bool SwapBool, uqm::GodModeFlags godModeType)
{
	return antiCheatImpl(ElementPtr, SwapBool, godModeType, {PlayerControl, NumPlayers}, uqm::UQMOptions::read().optGodModes);
}

static inline bool antiCheatAltImpl(const uqm::GodModeFlags testFlags, const uqstl::span<PlayerControlFlags> playerControl, const uqm::GodModeFlags godModeData)
{
	const bool player0Computer = testFlag(playerControl[0], ComputerControlFlags);
	const bool player1Computer = testFlag(playerControl[1], ComputerControlFlags);

	if (player0Computer && player1Computer)
	{
		return false;
	}

	if (!testFlag(godModeData, testFlags))
	{
		return false;
	}

	return player0Computer != player1Computer;
}

static inline bool antiCheatAlt(uqm::GodModeFlags godModeFlags)
{
	return antiCheatAltImpl(godModeFlags, {PlayerControl, NumPlayers}, uqm::UQMOptions::read().optGodModes);
}

static inline bool isNetwork(void)
{
	return testFlag(PlayerControl[0] | PlayerControl[1], PlayerControlFlags::Network);
}

#endif /* UQM_INTEL_H_ */
