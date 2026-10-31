// The game clock in game ticks and milliseconds, and the sleep functions that use it.
#ifndef LIBS_TIME_TIMELIB_H_
#define LIBS_TIME_TIMELIB_H_

#include "core/stl/chrono.h"

#include <cstdint>

/* GameTicksPerSecond is the LCM of all the fractions of a second the game uses.
 * Battle is 24 FPS, Landers are 35 FPS, most UI-level things are 15 FPS,
 * The Interplanetary flight is 30 FPS, Comm ambient animation is 40 FPS,
 * (also Comm Oscilloscope is 32 FPS, but it does not require a stable
 * timer and currently runs within the Comm ambient anim paradigm anyway)
 * Thus, the minimum value for GameTicksPerSecond is 840. */
constexpr uint32_t GameTicksPerSecond {840};

// A duration in game ticks.
using GameTicks = uqstl::chrono::duration<int64_t, uqstl::ratio<1, GameTicksPerSecond>>;
inline constexpr GameTicks c_gameSecond {GameTicksPerSecond};

static constexpr GameTicks getTicksForFramerate(const uint32_t desiredFps)
{
	return c_gameSecond / desiredFps;
}

// Game ticks since the game clock started.
struct GameTimeClock
{
	using duration = GameTicks;
	using rep = duration::rep;
	using period = duration::period;
	using time_point = uqstl::chrono::time_point<GameTimeClock>;
	static constexpr bool is_steady {true};

	static time_point now();
};

using TimeCount = GameTimeClock::time_point;
using TimePeriod = GameTicks;

// Time since the game clock started.
uqstl::chrono::milliseconds GetTimeMs(void);
TimeCount GetTimeCounter(void);

// Sleep without processing asynchronous operations.
void HibernateThread(TimePeriod timePeriod);
void HibernateThreadUntil(TimeCount wakeTime);

// Sleep, and process asynchronous operations until the wake time.
void SleepThread(TimePeriod timePeriod);
void SleepThreadUntil(TimeCount wakeTime);

// Sleeps for a short time, so that busy-wait loops do not use all of a CPU core.
void TaskSwitch(void);

#endif /* LIBS_TIME_TIMELIB_H_ */
