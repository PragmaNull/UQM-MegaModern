// Implements the sleep functions that count in game ticks.
#include "libs/threadlib.h"

#include "core/thread/Thread.h"
#include "libs/async.h"

#include <chrono>
#include <thread>

namespace
{

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

		const uint32_t nextTimeMs {Async_timeBeforeNextMs()};
		// Overflow-safe conversion.
		TimeCount nextTime {(nextTimeMs / 1000) * GameTicksPerSecond + ((nextTimeMs % 1000) * GameTicksPerSecond / 1000)};
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
