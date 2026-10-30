/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
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

#include "alarm.h"

#include "libs/time/timelib.h"

#include "core/stl/memory.h"

#include <map>

// The alarms in time order. Alarms with the same time stay in the order they were added.
static uqstl::multimap<AlarmTime, uqstl::unique_ptr<Alarm>> s_alarms {};

void Alarm_init(void)
{
	s_alarms.clear();
}

void Alarm_uninit(void)
{
	s_alarms.clear();
}

static inline AlarmTime
AlarmTime_nowMs(void)
{
	return GetTimeMs();
}

static Alarm*
Alarm_add(AlarmTime time, AlarmCallback callback, AlarmCallbackArg arg)
{
	auto alarm {uqstl::make_unique<Alarm>(Alarm {time, callback, arg})};
	Alarm* result {alarm.get()};
	s_alarms.emplace(time, uqstl::move(alarm));
	return result;
}

Alarm*
Alarm_addAbsoluteMs(uint32_t ms, AlarmCallback callback,
					AlarmCallbackArg arg)
{
	return Alarm_add(ms, callback, arg);
}

Alarm*
Alarm_addRelativeMs(uint32_t ms, AlarmCallback callback,
					AlarmCallbackArg arg)
{
	return Alarm_add(AlarmTime_nowMs() + ms, callback, arg);
}

void Alarm_remove(Alarm* alarm)
{
	auto [first, last] {s_alarms.equal_range(alarm->time)};
	for (auto it {first}; it != last; ++it)
	{
		if (it->second.get() == alarm)
		{
			s_alarms.erase(it);
			return;
		}
	}
}

// Process at most one alarm, if its time has come.
// It is safe to call this function again from inside a callback function
// that it called. It should not be called from multiple threads at once.
bool Alarm_processOne(void)
{
	if (s_alarms.empty() || AlarmTime_nowMs() < s_alarms.begin()->first)
	{
		return false;
	}

	// Take the alarm out first, because the callback can add or remove alarms.
	const uqstl::unique_ptr<Alarm> alarm {uqstl::move(s_alarms.extract(s_alarms.begin()).mapped())};
	alarm->callback(alarm->arg);
	return true;
}

uint32_t
Alarm_timeBeforeNextMs(void)
{
	if (s_alarms.empty())
	{
		return UINT32_MAX;
	}

	return alarmTimeToMsUint32(s_alarms.begin()->first);
}
