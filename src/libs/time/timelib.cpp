// The game clock in game ticks and milliseconds, and the sleep functions that use it.
#include "timelib.h"

#include "core/thread/Thread.h"
#include "libs/callback/async.h"

#include <chrono>
#include <thread>

namespace
{

using Clock = uqstl::chrono::steady_clock;

Clock::time_point startTime()
{
	static const Clock::time_point start {Clock::now()};
	return start;
}

TimeCount msToTicks(uint32_t ms)
{
	// Overflow-safe conversion.
	return (ms / 1000) * GameTicksPerSecond + ((ms % 1000) * GameTicksPerSecond / 1000);
}

void sleepTicks(TimePeriod ticks)
{
	uqstl::this_thread::sleep_for(uqstl::chrono::milliseconds {static_cast<int64_t>(ticks) * 1000 / GameTicksPerSecond});
}

void sleepUntilTicks(TimeCount wakeTime)
{
	const TimeCount now {GetTimeCounter()};
	if (wakeTime <= now)
	{
		uqm::taskSwitch();
	}
	else
	{
		sleepTicks(wakeTime - now);
	}
}

} // namespace

uint32_t GetTimeMs(void)
{
	const auto elapsed {uqstl::chrono::duration_cast<uqstl::chrono::milliseconds>(Clock::now() - startTime())};
	return static_cast<uint32_t>(elapsed.count());
}

TimeCount GetTimeCounter(void)
{
	return msToTicks(GetTimeMs());
}

void HibernateThread(TimePeriod timePeriod)
{
	sleepTicks(timePeriod);
}

void HibernateThreadUntil(TimeCount wakeTime)
{
	sleepUntilTicks(wakeTime);
}

void SleepThread(TimePeriod timePeriod)
{
	SleepThreadUntil(GetTimeCounter() + timePeriod);
}

void SleepThreadUntil(TimeCount wakeTime)
{
	for (;;)
	{
		Async_process();

		const TimeCount now {GetTimeCounter()};
		if (wakeTime <= now)
		{
			return;
		}

		TimeCount nextTime {msToTicks(Async_timeBeforeNextMs())};
		if (wakeTime < nextTime)
		{
			nextTime = wakeTime;
		}

		sleepUntilTicks(nextTime);
	}
}

void TaskSwitch(void)
{
	uqm::taskSwitch();
}
