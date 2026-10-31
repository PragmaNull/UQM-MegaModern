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

#include <stdlib.h>
#include "gameev.h"
#include "globdata.h"
#include "sis.h"
// for DrawStatusMessage()
#include "setup.h"
#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"
#include "core/thread/Mutex.h"
#include "core/log/log.h"
#include "options.h"

// WARNING: Most of clock functions are only meant to be called by the
//   Starcon2Main thread! If you need access from other threads, examine
//   the locking system!
// XXX: This mutex is only necessary because debugging functions
//   may access the clock and event data from a different thread
static uqm::RecursiveMutex clock_mutex {};
static bool s_clockInited {};

static uqm::GameClock&
gameClock(void)
{
	return uqm::gameState().clock();
}

static void
nextClockDay(void)
{
	gameClock().nextDay();

	// update the date on screen
	DrawStatusMessage(nullptr);
}

// Computes how many days have passed since the game has begun
float daysElapsed(void)
{
	return gameClock().daysElapsed();
}

static void
processClockDayEvents(void)
{
	uint8_t funcIndex {};
	while (gameClock().takeDueEvent(funcIndex))
	{
		EventHandler(funcIndex);
	}
}

bool InitGameClock(void)
{
	if (!gameClock().start(NUM_EVENTS))
	{
		return (false);
	}
	s_clockInited = true;
	return (true);
}

bool UninitGameClock(void)
{
	s_clockInited = false;
	gameClock().stop();
	return (true);
}

// For debugging use only
void LockGameClock(void) UQM_NO_THREAD_SAFETY_ANALYSIS
{
	// Block the GameClockTick() for executing
	if (s_clockInited)
	{
		clock_mutex.lock();
	}
}

// For debugging use only
void UnlockGameClock(void) UQM_NO_THREAD_SAFETY_ANALYSIS
{
	if (s_clockInited)
	{
		clock_mutex.unlock();
	}
}

// For debugging use only
bool GameClockRunning(void)
{
	if (!s_clockInited)
	{
		return false;
	}

	const uqm::LockGuard lock {clock_mutex};
	return gameClock().dayInTicks() != 0;
}

void SetGameClockRate(uint16_t seconds_per_day)
{
	int16_t new_day_in_ticks = (int16_t)(seconds_per_day * CLOCK_BASE_FRAMERATE);
	if (uqm::UQMOptions::read().timeDilationPct != 100) [[unlikely]]
	{
		new_day_in_ticks = static_cast<int16_t>(new_day_in_ticks * (uqm::UQMOptions::read().timeDilationPct / 100.f));
		uqm::log::debug("Setting time dilation scale to {}%, with a day tick length of {}", uqm::UQMOptions::read().timeDilationPct.value, new_day_in_ticks);
	}
	gameClock().setDayInTicks(new_day_in_ticks);
}

bool ValidateEvent(EVENT_TYPE type, uint16_t* pmonth_index, uint16_t* pday_index,
				   uint16_t* pyear_index)
{
	return gameClock().resolveDate(type, *pmonth_index, *pday_index, *pyear_index);
}

HEVENT
AddEvent(EVENT_TYPE type, uint16_t month_index, uint16_t day_index, uint16_t year_index, uint8_t func_index)
{
	if (type == RELATIVE_EVENT
		&& month_index == 0
		&& day_index == 0
		&& year_index == 0)
	{
		EventHandler(func_index);
	}
	else if (ValidateEvent(type, &month_index, &day_index, &year_index))
	{
		return gameClock().addEvent(month_index, day_index, year_index, func_index);
	}

	return (0);
}

void GameClockTick(void)
{
	// XXX: This mutex is only necessary because debugging functions
	//   may access the clock and event data from a different thread
	const uqm::LockGuard lock {clock_mutex};

	if (gameClock().tick())
	{ // next day -- move the calendar
		nextClockDay();
		processClockDayEvents();
	}
}

void MoveGameClockToNextEvent(bool skipHyperspaceEncounters)
{
	if (!GameClockRunning())
	{
		return;
	}

	const uqm::LockGuard lock {clock_mutex};
	while (const auto event {gameClock().firstEvent()})
	{
		while (gameClock().isBefore(event->year_index, event->month_index, event->day_index))
		{
			MoveGameClockDays(1);
		}
		// An event for today that was added after the events of today ran.
		processClockDayEvents();
		if (!skipHyperspaceEncounters || event->func_index != HYPERSPACE_ENCOUNTER_EVENT)
		{
			return;
		}
	}
}

void MoveGameClockDays(uint16_t days)
{
	// XXX: This should theoretically hold the clock_mutex, but if
	//   someone manages to hit the debug button while this function
	//   runs, it's their own fault :-P

	for (; days > 0; --days)
	{
		nextClockDay();
		processClockDayEvents();
	}
	gameClock().restartDay();
}
