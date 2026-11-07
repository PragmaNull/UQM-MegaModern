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
#ifndef UQM_TACTRANS_H
#define UQM_TACTRANS_H

#include "core/misc/bittricks.h"
#include "races.h"
#include "element.h"
#include "battlecontrols.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

bool battleEndReadyHuman(HumanInputContext* context);
bool battleEndReadyComputer(ComputerInputContext* context);
#ifdef NETPLAY
bool battleEndReadyNetwork(NetworkInputContext* context);
#endif

extern void ship_transition(Element* ElementPtr);
extern bool OpponentAlive(Starship* TestStarShipPtr);
extern void new_ship(Element* ElementPtr);
extern void ship_death(Element* ShipPtr);
extern void spawn_ion_trail(Element* ElementPtr, int16_t x, int16_t y);
extern void flee_preprocess(Element* ElementPtr);

extern void StopDitty(void);
extern void ResetWinnerStarShip(void);
extern void StopAllBattleMusic(void);
extern Starship* FindAliveStarShip(Element* deadShip);
extern Starship* GetWinnerStarShip(void);
extern void SetWinnerStarShip(Starship* winner);
extern void RecordShipDeath(Element* deadShip);
extern void StartShipExplosion(Element* ShipPtr, bool playSound);

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_TACTRANS_H
