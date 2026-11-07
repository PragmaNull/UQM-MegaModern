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

#pragma once
#ifndef UQM_SUPERMELEE_PICKMELE_H
#define UQM_SUPERMELEE_PICKMELE_H

namespace uqm
{
struct PickMeleeState;

} // namespace uqm

#include "core/misc/bittricks.h"
#include "meleesetup.h"
#include "uqm/battlecontrols.h"
#include "uqm/races.h"

namespace uqm
{
void MeleeShipDeath(Starship*);
void BuildPickMeleeFrame(void);
void DestroyPickMeleeFrame(void);
void FillPickMeleeFrame(MeleeSetup* setup);
void MeleeGameOver(void);
bool GetInitialMeleeStarShips(StarshipHandle* result);
bool GetNextMeleeStarShip(uint16_t which_player, StarshipHandle* result);

bool updateMeleeSelection(PickMeleeState* gms, uint16_t player, uint16_t ship);

bool selectShipHuman(HumanInputContext* context, PickMeleeState* gms);
bool selectShipComputer(ComputerInputContext* context,
						PickMeleeState* gms);
#ifdef NETPLAY
bool selectShipNetwork(NetworkInputContext* context, PickMeleeState* gms);
#endif /* NETPLAY */

} // namespace uqm

#ifdef PICKMELE_INTERNAL

#include "../flash.h"
#include "../init.h"
#include "libs/time/timelib.h"


namespace uqm
{
struct PickMeleeState
{
	bool (*InputFunc)(struct PickMeleeState* pInputState);

	bool Initialized;

	struct
	{
		TimeCount timeIn;
		StarshipHandle hBattleShip;
		// Chosen ship.
		uint16_t choice;
		// Index of chosen ship, or (uint16_t) ~0 for random choice.

		uint16_t row;
		uint16_t col;
		uint16_t ships_left;
		// Number of ships still available.
		uint16_t randomIndex;
		// Pre-generated random number.
		bool selecting;
		// Is this player selecting a ship?
		bool done;
		// Has a selection been made for this player?
		FlashContext* flashContext;
		// Context for controlling the flash rectangle.
#ifdef NETPLAY
		bool remoteSelected;
#endif
	} player[NumPlayers];
};

bool setShipSelected(PickMeleeState* gms, uint16_t playerI, uint16_t choice,
					 bool reportNetwork);

} // namespace uqm


#endif /* PICKMELE_INTERNAL */

#endif // UQM_SUPERMELEE_PICKMELE_H
