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
#ifndef UQM_BATTLE_H_
#define UQM_BATTLE_H_

#include "options.h"
#include "core/misc/bittricks.h"
#include "libs/time/timelib.h"

#if defined(NETPLAY)
typedef uint32_t BattleFrameCounter;
#endif

#include "init.h"
// For NUM_SIDES

#if 0 //defined(__cplusplus)
extern "C" {
#endif

// The callback function is called on every battle frame
// just before the display queue is drawn
typedef void(BattleFrameCallback)(void);

typedef struct battlestate_struct
{
	bool (*InputFunc)(struct battlestate_struct* pInputState);
	bool first_time;
	uint32_t NextTime;
	BattleFrameCallback* frame_cb;
} BATTLE_STATE;

extern uint8_t battle_counter[NUM_SIDES];
extern bool instantVictory;
#if defined(NETPLAY)
extern BattleFrameCounter battleFrameCount;
#endif
#ifdef NETPLAY
uint16_t GetPlayerOrder(uint16_t i);
#else
#define GetPlayerOrder(i) (i)
#endif

bool Battle(BattleFrameCallback*);

static constexpr uint32_t BattleFPS {24}; // TODO: this is also defined in netoptions.cpp as BattleFPS. Maybe we should unify these?
static constexpr uint32_t BattleFrameRateTicks {getTicksForFramerate(BattleFPS)};

extern void BattleSong(bool DoPlay);
extern void FreeBattleSong(void);
extern bool RunAwayAllowed(void);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_BATTLE_H_ */
