// Tests for the virtual game clock.
#include "libs/time/VirtualClock.h"

#include <gtest/gtest.h>

using uqm::VirtualClock;

TEST(VirtualClock, ReadAdvancesByOneTick)
{
	VirtualClock clock {100, 5000};
	EXPECT_EQ(clock.read(), 100u);
	EXPECT_EQ(clock.read(), 101u);
	EXPECT_EQ(clock.now(), 102u);
}

TEST(VirtualClock, SleepAdvancesToTheWakeTime)
{
	VirtualClock clock {100, 5000};
	EXPECT_EQ(clock.sleepUntil(150, 5000), 5050u);
	EXPECT_EQ(clock.now(), 150u);
}

TEST(VirtualClock, SleepToAnEarlierTimeDoesNotGoBack)
{
	VirtualClock clock {100, 5000};
	(void)clock.sleepUntil(150, 5000);
	(void)clock.sleepUntil(120, 5050);
	EXPECT_EQ(clock.now(), 150u);
}

TEST(VirtualClock, LongStallUsesTheRealTimeAgain)
{
	VirtualClock clock {100, 5000};
	// The real time is 10 seconds later than the virtual time.
	EXPECT_EQ(clock.sleepUntil(110, 5000 + 8400), 5000u + 8400u);
	// Pacing continues from the new reference.
	EXPECT_EQ(clock.sleepUntil(130, 5000 + 8400), 5000u + 8400u + 20u);
}

TEST(VirtualClock, CounterWrapsAround)
{
	VirtualClock clock {0xfffffff0u, 0};
	EXPECT_EQ(clock.sleepUntil(0x10u, 0), 0x20u);
	EXPECT_EQ(clock.now(), 0x10u);
}
