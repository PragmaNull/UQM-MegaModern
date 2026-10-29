// Game time that advances only when the thread sleeps, so that a replay runs the same way each time.
#pragma once
#ifndef UQM_LIBS_TIME_VIRTUALCLOCK_H_
#define UQM_LIBS_TIME_VIRTUALCLOCK_H_

#include <cstdint>

namespace uqm
{

class VirtualClock
{
public:
	using Ticks = uint32_t;

	VirtualClock(Ticks start, Ticks realNow);

	// Returns the time, then advances it by one tick so that loops that only read the time still end.
	Ticks read();

	// Advances the time to 'wakeTime'. Returns the real time to wait until, so that the game keeps the pace of real time.
	// After a long stall, the real time is used again as the reference, and no wait is needed.
	Ticks sleepUntil(Ticks wakeTime, Ticks realNow);

	[[nodiscard]] Ticks now() const { return m_now; }

private:
	Ticks m_now;
	Ticks m_virtualAnchor;
	Ticks m_realAnchor;
};

} // namespace uqm

#endif // UQM_LIBS_TIME_VIRTUALCLOCK_H_
