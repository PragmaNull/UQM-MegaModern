// Game time that advances only when the thread sleeps, so that a replay runs the same way each time.
#include "libs/time/VirtualClock.h"

namespace uqm
{

namespace
{

// A stall longer than this makes the real time the reference again. It is one second of game ticks.
constexpr VirtualClock::Ticks c_maxLag {840};

// True if 'a' is later than 'b', also when the counter wraps.
bool isLater(VirtualClock::Ticks a, VirtualClock::Ticks b)
{
	return static_cast<int32_t>(a - b) > 0;
}

} // namespace

VirtualClock::VirtualClock(Ticks start, Ticks realNow)
	: m_now {start}
	, m_virtualAnchor {start}
	, m_realAnchor {realNow}
{
}

VirtualClock::Ticks VirtualClock::read()
{
	return m_now++;
}

VirtualClock::Ticks VirtualClock::sleepUntil(Ticks wakeTime, Ticks realNow)
{
	if (isLater(wakeTime, m_now))
	{
		m_now = wakeTime;
	}
	const Ticks realTarget {m_realAnchor + (m_now - m_virtualAnchor)};
	if (isLater(realNow, realTarget + c_maxLag))
	{
		m_realAnchor = realNow;
		m_virtualAnchor = m_now;
		return realNow;
	}
	return realTarget;
}

} // namespace uqm
