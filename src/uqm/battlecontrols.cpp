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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "battlecontrols.h"

#include "intel.h"
// For computer_intelligence()
#include "tactrans.h"
// For battleEndReady*
#include "init.h"
// For NUM_PLAYERS
#ifdef NETPLAY
#include "supermelee/netplay/netmelee.h"
#endif /* NETPLAY */

InputContext* PlayerInput[NUM_PLAYERS];

template <typename Context>
static void
InputContext_delete(InputContext* context)
{
	delete static_cast<Context*>(context);
}

BattleInputHandlers ComputerInputHandlers = {
	/* .frameInput     = */ (BattleFrameInputFunction)computer_intelligence,
	/* .selectShip     = */ (SelectShipFunction)selectShipComputer,
	/* .battleEndReady = */ (BattleEndReadyFunction)battleEndReadyComputer,
	/* .deleteContext  = */ InputContext_delete<ComputerInputContext>,
};

BattleInputHandlers HumanInputHandlers = {
	/* .frameInput     = */ (BattleFrameInputFunction)frameInputHuman,
	/* .selectShip     = */ (SelectShipFunction)selectShipHuman,
	/* .battleEndReady = */ (BattleEndReadyFunction)battleEndReadyHuman,
	/* .deleteContext  = */ InputContext_delete<HumanInputContext>,
};

#ifdef NETPLAY
BattleInputHandlers NetworkInputHandlers = {
	/* .frameInput     = */ (BattleFrameInputFunction)networkBattleInput,
	/* .selectShip     = */ (SelectShipFunction)selectShipNetwork,
	/* .battleEndReady = */ (BattleEndReadyFunction)battleEndReadyNetwork,
	/* .deleteContext  = */ InputContext_delete<NetworkInputContext>,
};
#endif


void InputContext_init(InputContext* context, BattleInputHandlers* handlers,
					   uint16_t playerNr)
{
	context->handlers = handlers;
	context->playerNr = playerNr;
}


ComputerInputContext*
ComputerInputContext_new(uint16_t playerNr)
{
	auto* result {new ComputerInputContext {}};
	InputContext_init(result,
					  &ComputerInputHandlers, playerNr);
	return result;
}

HumanInputContext*
HumanInputContext_new(uint16_t playerNr)
{
	auto* result {new HumanInputContext {}};
	InputContext_init(result,
					  &HumanInputHandlers, playerNr);
	return result;
}

#ifdef NETPLAY
NetworkInputContext*
NetworkInputContext_new(uint16_t playerNr)
{
	auto* result {new NetworkInputContext {}};
	InputContext_init(result,
					  &NetworkInputHandlers, playerNr);
	return result;
}
#endif
