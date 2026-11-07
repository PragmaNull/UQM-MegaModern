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
#ifndef UQM_GAMEEV_H
#define UQM_GAMEEV_H


#include "core/types/enum.h"
#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif


enum class GameEventId : uint8_t
{
	ArilouEntranceEvent = 0,
	ArilouExitEvent,
	HyperspaceEncounterEvent,
	KohrAhVictoriousEvent,
	AdvancePkunkMission,
	AdvanceThraddMission,
	ZoqfotDistressEvent,
	ZoqfotDeathEvent,
	ShofixtiReturnEvent,
	AdvanceUtwigSupoxMission,
	KohrAhGenocideEvent,
	SpathiShieldEvent,
	AdvanceIlwrathMission,
	AdvanceMyconMission,
	ArilouUmgahCheck,
	YehatRebelEvent,
	SlylandroRampUp,
	SlylandroRampDown,

	NumEvents
};
consteval bool isArithmeticEnum(GameEventId) { return true; }

enum class ArilouGateState
{
	Closing = 0,
	Opening
};
consteval bool isArithmeticEnum(ArilouGateState) { return true; }

extern int eventIdStrToNum(const char* eventIdStr);
const extern char* eventIdNumToStr(int eventNum);

extern void initEventSystem(void);
extern void uninitEventSystem(void);

extern void AddInitialGameEvents(void);
extern void EventHandler(GameEventId selector);
extern void SetRaceDest(uint8_t which_race, int16_t x, int16_t y, uint8_t days_left,
						uint8_t func_index);


#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_GAMEEV_H
