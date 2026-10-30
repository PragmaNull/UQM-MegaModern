// Tests the calendar and the event queue of uqm::GameClock.

#include <gtest/gtest.h>
#include "uqm/session/GameClock.h"

namespace
{

class GameClockTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ASSERT_TRUE(m_clock.start(8));
	}

	void TearDown() override
	{
		m_clock.stop();
	}

	uqm::GameClock m_clock {};
};

TEST_F(GameClockTest, StartsOnFebruary17)
{
	EXPECT_EQ(m_clock.day(), 17);
	EXPECT_EQ(m_clock.month(), 2);
	EXPECT_EQ(m_clock.year(), START_YEAR);
	EXPECT_EQ(m_clock.dayInTicks(), 0);
}

TEST_F(GameClockTest, NextDayGoesToNextMonthAndYear)
{
	m_clock.restore(28, 2, 2155, 0, 0);
	m_clock.nextDay();
	EXPECT_EQ(m_clock.day(), 1);
	EXPECT_EQ(m_clock.month(), 3);

	m_clock.restore(31, 12, 2155, 0, 0);
	m_clock.nextDay();
	EXPECT_EQ(m_clock.day(), 1);
	EXPECT_EQ(m_clock.month(), 1);
	EXPECT_EQ(m_clock.year(), 2156);
}

TEST_F(GameClockTest, NextDayUsesLeapYears)
{
	m_clock.restore(28, 2, 2156, 0, 0);
	m_clock.nextDay();
	EXPECT_EQ(m_clock.day(), 29);
	EXPECT_EQ(m_clock.month(), 2);
}

TEST_F(GameClockTest, TickStartsANewDayOnlyWhenTheClockRuns)
{
	EXPECT_FALSE(m_clock.tick());

	m_clock.setDayInTicks(2);
	EXPECT_FALSE(m_clock.tick());
	EXPECT_TRUE(m_clock.tick());
	EXPECT_EQ(m_clock.tickCount(), 2);
}

TEST_F(GameClockTest, SetDayInTicksKeepsThePartOfTheDay)
{
	m_clock.restore(17, 2, START_YEAR, 50, 100);
	m_clock.setDayInTicks(200);
	EXPECT_EQ(m_clock.tickCount(), 100);
	EXPECT_EQ(m_clock.dayInTicks(), 200);
}

TEST_F(GameClockTest, DaysElapsedCountsFromTheStart)
{
	m_clock.restore(17, 2, START_YEAR, 100, 100);
	EXPECT_FLOAT_EQ(m_clock.daysElapsed(), 0.0f);

	m_clock.restore(17, 2, START_YEAR + 1, 50, 100);
	EXPECT_FLOAT_EQ(m_clock.daysElapsed(), 365.5f);
}

TEST_F(GameClockTest, ResolveDateMakesRelativeDatesAbsolute)
{
	uint16_t month {0};
	uint16_t day {14};
	uint16_t year {0};
	EXPECT_TRUE(m_clock.resolveDate(RELATIVE_EVENT, month, day, year));
	EXPECT_EQ(day, 3);
	EXPECT_EQ(month, 3);
	EXPECT_EQ(year, START_YEAR);
}

TEST_F(GameClockTest, ResolveDateRefusesPastDates)
{
	uint16_t month {2};
	uint16_t day {16};
	uint16_t year {START_YEAR};
	EXPECT_FALSE(m_clock.resolveDate(ABSOLUTE_EVENT, month, day, year));
	day = 17;
	EXPECT_TRUE(m_clock.resolveDate(ABSOLUTE_EVENT, month, day, year));
}

TEST_F(GameClockTest, DueEventsComeInDateOrder)
{
	EXPECT_NE(m_clock.addEvent(2, 19, START_YEAR, 3), nullptr);
	EXPECT_NE(m_clock.addEvent(2, 18, START_YEAR, 1), nullptr);
	EXPECT_NE(m_clock.addEvent(2, 18, START_YEAR, 2), nullptr);

	uint8_t funcIndex {};
	EXPECT_FALSE(m_clock.takeDueEvent(funcIndex));

	m_clock.nextDay();
	ASSERT_TRUE(m_clock.takeDueEvent(funcIndex));
	EXPECT_EQ(funcIndex, 1);
	ASSERT_TRUE(m_clock.takeDueEvent(funcIndex));
	EXPECT_EQ(funcIndex, 2);
	EXPECT_FALSE(m_clock.takeDueEvent(funcIndex));

	m_clock.nextDay();
	ASSERT_TRUE(m_clock.takeDueEvent(funcIndex));
	EXPECT_EQ(funcIndex, 3);
}

TEST_F(GameClockTest, AddEventFailsWhenTheQueueIsFull)
{
	for (int i {0}; i < 8; ++i)
	{
		EXPECT_NE(m_clock.addEvent(3, 1, START_YEAR, 0), nullptr);
	}
	EXPECT_EQ(m_clock.addEvent(3, 1, START_YEAR, 0), nullptr);
}

} // namespace
