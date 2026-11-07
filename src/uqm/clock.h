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
#ifndef UQM_CLOCK_H
#define UQM_CLOCK_H

#include "libs/time/timelib.h"
#include "displist.h"

namespace uqm
{
inline constexpr auto StartYear {2155};

// the running of the game-clock is based on game framerates
// *not* on the system (or translated) timer
// and is hard-coded to the original 24 fps
inline constexpr auto ClockBaseFramerate {24};
inline constexpr auto ClockFrameRate {c_gameSecond / ClockBaseFramerate};

using EventHandle = LinkHandle;

enum class GameEventId : uint8_t;

struct GameEvent
{
	// QueueLink elements; must be first
	EventHandle pred, succ;

	uint8_t day_index, month_index;
	uint16_t year_index;
	GameEventId func_index;
};

enum class EventType
{
	Absolute = 0,
	Relative
};

EventHandle AllocEvent();
void PutEvent(EventHandle h);
void UnlockEvent(EventHandle h);
void FreeEvent(EventHandle h);
void ForAllEvents(void (*callback)(QueueLink*, void*), void* arg);
void LockEvent(EventHandle h, GameEvent** ppe);
constexpr auto GetSuccEvent(const auto& l) { return _GetSuccLink(l); }

// Rates are in seconds per game day
inline constexpr auto HyperspaceClockRate {5};
// XXX: the IP rate is based on 24 ticks/second (see SetGameClockRate),
//   however, IP runs at 30 fps right now. So in reality, the IP clock
//   rate is closer to 23 seconds per game day. The clock is faster, but
//   the flagship also moves faster.
inline constexpr auto InterplanetaryClockRate {30};

extern bool InitGameClock(void);
extern bool UninitGameClock(void);

extern float daysElapsed(void);

extern void SetGameClockRate(uint16_t seconds_per_day);
extern bool ValidateEvent(EventType type, uint16_t* pmonth_index,
						  uint16_t* pday_index, uint16_t* pyear_index);
extern EventHandle AddEvent(EventType type, uint16_t month_index, uint16_t day_index, uint16_t year_index, GameEventId func_index);
extern void EventHandler(GameEventId selector);
extern void GameClockTick(void);
extern void MoveGameClockDays(uint16_t days);
// Moves the date to the first event, and runs the events on the way. Can go past the hyperspace Encounter events.
void MoveGameClockToNextEvent(bool skipHyperspaceEncounters);

// The lock/unlock/running functions are for debugging use only
// Locking will block the GameClockTick() function and thus
// the thread moving the clock.
extern void LockGameClock(void);
extern void UnlockGameClock(void);
// A weak indicator of the clock moving. Suitable for debugging,
// but not much else
extern bool GameClockRunning(void);

} // namespace uqm

#endif // UQM_CLOCK_H
