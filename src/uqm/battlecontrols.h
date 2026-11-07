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
#ifndef UQM_BATTLECONTROLS_H
#define UQM_BATTLECONTROLS_H

namespace uqm
{
struct BattleInputHandlers;
struct InputContext;
struct ComputerInputContext;
struct HumanInputContext;
#ifdef NETPLAY
struct NetworkInputContext;
#endif /* NETPLAY */

} // namespace uqm

#include "controls.h"
#include "core/misc/bittricks.h"
#include "races.h"
#include "supermelee/pickmele.h"

namespace uqm
{
typedef BattleInputState (*BattleFrameInputFunction)(
	InputContext* context, Starship* StarShipPtr);
typedef bool (*SelectShipFunction)(InputContext* context,
								   PickMeleeState* gms);
typedef bool (*BattleEndReadyFunction)(InputContext* context);
typedef void (*DeleteInputContextFunction)(InputContext* context);


struct BattleInputHandlers
{
	BattleFrameInputFunction frameInput;
	SelectShipFunction selectShip;
	BattleEndReadyFunction battleEndReady;
	DeleteInputContextFunction deleteContext;
};

// Base "class" for all ...InputContext structures
struct InputContext
{
	BattleInputHandlers* handlers;
	uint16_t playerNr;
};

struct ComputerInputContext : InputContext
{
	// TODO: Put RNG Context used for the AI here.
};

struct HumanInputContext : InputContext
{
};

#ifdef NETPLAY
struct NetworkInputContext : InputContext
{
	// TODO: put NetworkConnection for this player here.
};
#endif /* NETPLAY */

ComputerInputContext* ComputerInputContext_new(uint16_t playerNr);
HumanInputContext* HumanInputContext_new(uint16_t playerNr);
#ifdef NETPLAY
NetworkInputContext* NetworkInputContext_new(uint16_t playerNr);
#endif /* NETPLAY */

extern uqstl::array<InputContext*, NumPlayers> PlayerInput;


BattleInputState frameInputHuman(HumanInputContext* context,
								 Starship* StarShipPtr);
void InputContext_init(InputContext* context, BattleInputHandlers* handlers,
					   uint16_t playerNr);
// Do not call directly, only from the FreeInputContextFunction.
// Call InputContext->handlers->freeContext() to release an
// InputContext.

} // namespace uqm

#endif // UQM_BATTLECONTROLS_H
