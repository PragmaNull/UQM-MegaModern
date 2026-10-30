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

#include "intel.h"

#include "battlecontrols.h"
#include "controls.h"
#include "globdata.h"
#include "setup.h"
#include "options.h"
#include "core/log/log.h"

#include <stdio.h>


BATTLE_INPUT_STATE
computer_intelligence(ComputerInputContext* context, STARSHIP* StarShipPtr)
{
	BATTLE_INPUT_STATE InputState;

	if (uqm::lowByte(GLOBAL(CurrentActivity)) == IN_LAST_BATTLE)
	{
		return 0;
	}

	if (StarShipPtr)
	{
		// Selecting the next action for in battle.
		const bool isMelee {uqm::lowByte(GLOBAL(CurrentActivity)) == SUPER_MELEE};
		if (testFlag(StarShipPtr->control, PlayerControlFlags::Cyborg) && uqm::UQMOptions::read().passiveEnemy
			&& (isMelee || StarShipPtr->playerNr != RPG_PLAYER_NUM))
		{
			InputState = 0;
		}
		else if (testFlag(StarShipPtr->control, PlayerControlFlags::Cyborg))
		{
			InputState = tactical_intelligence(context, StarShipPtr);

			// Allow a player to warp-escape in cyborg mode
			if (StarShipPtr->playerNr == RPG_PLAYER_NUM)
			{
				InputState |= CurrentInputToBattleInput(
								  context->playerNr)
							& BATTLE_ESCAPE;
			}
		}
		else
		{
			InputState = CurrentInputToBattleInput(context->playerNr);
		}
	}
	else if (!testFlag(PlayerControl[context->playerNr], PlayerControlFlags::Psytron))
	{
		InputState = 0;
	}
	else
	{
		switch (uqm::lowByte(GLOBAL(CurrentActivity)))
		{
			case SUPER_MELEE:
				{
					SleepThread(GameTicksPerSecond >> 1);
					InputState = BATTLE_WEAPON; /* pick a random ship */
					break;
				}
			default:
				// Should not happen. Satisfying compiler.
				uqm::log::warn("Warning: Unexpected state in "
							   "computer_intelligence().");
				InputState = 0;
				break;
		}
	}
	return InputState;
}