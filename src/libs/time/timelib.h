// The game clock in game ticks and milliseconds, and the sleep functions that use it.
#ifndef LIBS_TIME_TIMELIB_H_
#define LIBS_TIME_TIMELIB_H_

#include <cstdint>

typedef uint32_t TimeCount;
typedef uint32_t TimePeriod;

/* GameTicksPerSecond is the LCM of all the fractions of a second the game uses.
 * Battle is 24 FPS, Landers are 35 FPS, most UI-level things are 15 FPS,
 * The Interplanetary flight is 30 FPS, Comm ambient animation is 40 FPS,
 * (also Comm Oscilloscope is 32 FPS, but it does not require a stable
 * timer and currently runs within the Comm ambient anim paradigm anyway)
 * Thus, the minimum value for GameTicksPerSecond is 840. */
constexpr uint32_t GameTicksPerSecond {840};

static constexpr uint32_t getTicksForFramerate(const uint32_t desiredFps)
{
	return GameTicksPerSecond / desiredFps;
}

// Milliseconds since the game clock started. Wraps after about 49 days.
uint32_t GetTimeMs(void);
// Game ticks since the game clock started.
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
