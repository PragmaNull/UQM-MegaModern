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

#ifndef UQM_SUPERMELEE_PICKMELE_H_
#define UQM_SUPERMELEE_PICKMELE_H_

typedef struct getmelee_struct GETMELEE_STATE;

#include "uqm/races.h"
#include "uqm/battlecontrols.h"
#include "meleesetup.h"
#include "core/misc/bittricks.h"

void MeleeShipDeath(STARSHIP*);
void BuildPickMeleeFrame(void);
void DestroyPickMeleeFrame(void);
void FillPickMeleeFrame(MeleeSetup* setup);
void MeleeGameOver(void);
bool GetInitialMeleeStarShips(HSTARSHIP* result);
bool GetNextMeleeStarShip(uint16_t which_player, HSTARSHIP* result);

bool updateMeleeSelection(GETMELEE_STATE* gms, uint16_t player, uint16_t ship);

bool selectShipHuman(HumanInputContext* context, GETMELEE_STATE* gms);
bool selectShipComputer(ComputerInputContext* context,
						GETMELEE_STATE* gms);
#ifdef NETPLAY
bool selectShipNetwork(NetworkInputContext* context, GETMELEE_STATE* gms);
#endif /* NETPLAY */

#ifdef PICKMELE_INTERNAL

#include "../flash.h"
#include "libs/time/timelib.h"
#include "../init.h"


struct getmelee_struct
{
	bool (*InputFunc)(struct getmelee_struct* pInputState);

	bool Initialized;

	struct
	{
		TimeCount timeIn;
		HSTARSHIP hBattleShip;
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
	} player[NUM_PLAYERS];
};

bool setShipSelected(GETMELEE_STATE* gms, uint16_t playerI, uint16_t choice,
					 bool reportNetwork);


#endif /* PICKMELE_INTERNAL */

#endif /* UQM_SUPERMELEE_PICKMELE_H_ */
