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
#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "core/thread/Mutex.h"
#include "core/log/log.h"
#include "libs/misc.h"
#include "options.h"

// WARNING: Most of clock functions are only meant to be called by the
//   Starcon2Main thread! If you need access from other threads, examine
//   the locking system!
// XXX: This mutex is only necessary because debugging functions
//   may access the clock and event data from a different thread
static uqm::RecursiveMutex clock_mutex {};
static bool s_clockInited {};

static bool
IsLeapYear(uint16_t year)
{
	//     every 4th year      but not 100s          yet still 400s
	return (year & 3) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

/* month is 1-based: 1=Jan, 2=Feb, etc. */
static uint8_t
DaysInMonth(uint16_t month, uint16_t year)
{
	static const uint8_t days_in_month[12] =
		{
			31,
			28,
			31,
			30,
			31,
			30,
			31,
			31,
			30,
			31,
			30,
			31,
		};

	if (month == 2 && IsLeapYear(year))
	{
		return 29; /* February, leap year */
	}

	return days_in_month[month - 1];
}

static void
nextClockDay(void)
{
	++GLOBAL(GameClock.day_index);
	if (GLOBAL(GameClock.day_index) > DaysInMonth(
			GLOBAL(GameClock.month_index),
			GLOBAL(GameClock.year_index)))
	{
		GLOBAL(GameClock.day_index) = 1;
		++GLOBAL(GameClock.month_index);
		if (GLOBAL(GameClock.month_index) > 12)
		{
			GLOBAL(GameClock.month_index) = 1;
			++GLOBAL(GameClock.year_index);
		}
	}

	// update the date on screen
	DrawStatusMessage(nullptr);
}

// Computes how many days have passed since the game has begun
float daysElapsed(void)
{
	float days = 0;
	uint16_t index;

	// Years
	for (index = START_YEAR; index < GLOBAL(GameClock.year_index); index++)
	{
		days += 365;
		if (IsLeapYear(index))
		{
			days++;
		}
	}

	if (GLOBAL(GameClock.month_index) == 1)
	{
		days = days - 31;
	}

	// Months
	for (index = 2; index < GLOBAL(GameClock.month_index); index++)
	{
		days += DaysInMonth(index, GLOBAL(GameClock.year_index));
	}

	// Days
	days = days + GLOBAL(GameClock.day_index) - 17;

	// Part of a day
	days = days + (GLOBAL(GameClock.day_in_ticks) - GLOBAL(GameClock.tick_count)) / (float)GLOBAL(GameClock.day_in_ticks);

	return days;
}

static void
processClockDayEvents(void)
{
	HEVENT hEvent;

	while ((hEvent = GetHeadEvent()))
	{
		EVENT* EventPtr;

		LockEvent(hEvent, &EventPtr);

		if (GLOBAL(GameClock.day_index) != EventPtr->day_index
			|| GLOBAL(GameClock.month_index) != EventPtr->month_index
			|| GLOBAL(GameClock.year_index) != EventPtr->year_index)
		{
			UnlockEvent(hEvent);
			break;
		}
		RemoveEvent(hEvent);
		EventHandler(EventPtr->func_index);

		UnlockEvent(hEvent);
		FreeEvent(hEvent);
	}
}

bool InitGameClock(void)
{
	if (!InitQueue(&GLOBAL(GameClock.event_q), NUM_EVENTS, sizeof(EVENT)))
	{
		return (false);
	}
	s_clockInited = true;
	GLOBAL(GameClock.month_index) = 2;
	GLOBAL(GameClock.day_index) = 17;
	GLOBAL(GameClock.year_index) = START_YEAR; /* Feb 17, START_YEAR */
	GLOBAL(GameClock.tick_count) = 0;
	GLOBAL(GameClock.day_in_ticks) = 0;

	return (true);
}

bool UninitGameClock(void)
{
	s_clockInited = false;

	UninitQueue(&GLOBAL(GameClock.event_q));

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
	int16_t day_in_ticks;

	if (!s_clockInited)
	{
		return false;
	}

	{
		const uqm::LockGuard lock {clock_mutex};
		day_in_ticks = GLOBAL(GameClock.day_in_ticks);
	}

	return day_in_ticks != 0;
}

void SetGameClockRate(uint16_t seconds_per_day)
{

	int16_t new_day_in_ticks = (int16_t)(seconds_per_day * CLOCK_BASE_FRAMERATE);
	if (uqm::UQMOptions::read().timeDilationPct != 100) [[unlikely]]
	{
		new_day_in_ticks = static_cast<int16_t>(new_day_in_ticks * (uqm::UQMOptions::read().timeDilationPct / 100.f));
		uqm::log::debug("Setting time dilation scale to {}%, with a day tick length of {}", uqm::UQMOptions::read().timeDilationPct.value, new_day_in_ticks);
	}

	int16_t new_tick_count {};
	if (GLOBAL(GameClock.day_in_ticks) == 0)
	{
		new_tick_count = new_day_in_ticks;
	}
	else if (GLOBAL(GameClock.tick_count) <= 0)
	{
		new_tick_count = 0;
	}
	else if ((new_tick_count = (int16_t)((uint32_t)GLOBAL(GameClock.tick_count)
										 * new_day_in_ticks / GLOBAL(GameClock.day_in_ticks)))
			 == 0)
	{
		new_tick_count = 1;
	}
	GLOBAL(GameClock.day_in_ticks) = new_day_in_ticks;
	GLOBAL(GameClock.tick_count) = new_tick_count;
}

bool ValidateEvent(EVENT_TYPE type, uint16_t* pmonth_index, uint16_t* pday_index,
				   uint16_t* pyear_index)
{
	uint16_t month_index, day_index, year_index;

	month_index = *pmonth_index;
	day_index = *pday_index;
	year_index = *pyear_index;
	if (type == RELATIVE_EVENT)
	{
		month_index += GLOBAL(GameClock.month_index) - 1;
		year_index += GLOBAL(GameClock.year_index) + (month_index / 12);
		month_index = (month_index % 12) + 1;

		day_index += GLOBAL(GameClock.day_index);
		while (day_index > DaysInMonth(month_index, year_index))
		{
			day_index -= DaysInMonth(month_index, year_index);
			if (++month_index > 12)
			{
				month_index = 1;
				++year_index;
			}
		}

		*pmonth_index = month_index;
		*pday_index = day_index;
		*pyear_index = year_index;
	}

	// translation: return (bool) !(date < GLOBAL (Gameclock.date));
	return (bool)(!(year_index < GLOBAL(GameClock.year_index)
					|| (year_index == GLOBAL(GameClock.year_index)
						&& (month_index < GLOBAL(GameClock.month_index)
							|| (month_index == GLOBAL(GameClock.month_index)
								&& day_index < GLOBAL(GameClock.day_index))))));
}

HEVENT
AddEvent(EVENT_TYPE type, uint16_t month_index, uint16_t day_index, uint16_t year_index, uint8_t func_index)
{
	HEVENT hNewEvent;

	if (type == RELATIVE_EVENT
		&& month_index == 0
		&& day_index == 0
		&& year_index == 0)
	{
		EventHandler(func_index);
	}
	else if (ValidateEvent(type, &month_index, &day_index, &year_index)
			 && (hNewEvent = AllocEvent()))
	{
		EVENT* EventPtr;

		LockEvent(hNewEvent, &EventPtr);
		EventPtr->day_index = (uint8_t)day_index;
		EventPtr->month_index = (uint8_t)month_index;
		EventPtr->year_index = year_index;
		EventPtr->func_index = func_index;
		UnlockEvent(hNewEvent);

		{
			HEVENT hEvent, hSuccEvent;
			for (hEvent = GetHeadEvent(); hEvent != 0; hEvent = hSuccEvent)
			{
				LockEvent(hEvent, &EventPtr);
				if (year_index < EventPtr->year_index
					|| (year_index == EventPtr->year_index
						&& (month_index < EventPtr->month_index
							|| (month_index == EventPtr->month_index
								&& day_index < EventPtr->day_index))))
				{
					UnlockEvent(hEvent);
					break;
				}

				hSuccEvent = GetSuccEvent(EventPtr);
				UnlockEvent(hEvent);
			}

			InsertEvent(hNewEvent, hEvent);
		}

		return (hNewEvent);
	}

	return (0);
}

void GameClockTick(void)
{
	// XXX: This mutex is only necessary because debugging functions
	//   may access the clock and event data from a different thread
	const uqm::LockGuard lock {clock_mutex};

	--GLOBAL(GameClock.tick_count);
	if (GLOBAL(GameClock.tick_count) <= 0)
	{ // next day -- move the calendar
		GLOBAL(GameClock.tick_count) = GLOBAL(GameClock.day_in_ticks);
		// Do not do anything until the clock is inited
		if (GLOBAL(GameClock.day_in_ticks) > 0)
		{
			nextClockDay();
			processClockDayEvents();
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
	GLOBAL(GameClock.tick_count) = GLOBAL(GameClock.day_in_ticks);
}
