// The game clock in game ticks and milliseconds, and the sleep functions that use it.
#include "timelib.h"

#include "core/thread/Thread.h"
#include "libs/callback/async.h"


namespace
{

using Clock = uqstl::chrono::steady_clock;

Clock::time_point startTime()
{
	static const Clock::time_point start {Clock::now()};
	return start;
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
		uqstl::this_thread::sleep_for(wakeTime - now);
	}
}

} // namespace

GameTimeClock::time_point GameTimeClock::now()
{
	return time_point {uqstl::chrono::duration_cast<GameTicks>(Clock::now() - startTime())};
}

uqstl::chrono::milliseconds GetTimeMs(void)
{
	return uqstl::chrono::duration_cast<uqstl::chrono::milliseconds>(Clock::now() - startTime());
}

TimeCount GetTimeCounter(void)
{
	return GameTimeClock::now();
}

void HibernateThread(TimePeriod timePeriod)
{
	uqstl::this_thread::sleep_for(timePeriod);
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

		const uqstl::chrono::milliseconds next {Async_nextTime()};
		const TimeCount nextTime {next == uqstl::chrono::milliseconds::max() ? wakeTime : TimeCount {uqstl::chrono::duration_cast<GameTicks>(next)}};
		sleepUntilTicks(uqstl::min(wakeTime, nextTime));
	}
}

void TaskSwitch(void)
{
	uqm::taskSwitch();
}
