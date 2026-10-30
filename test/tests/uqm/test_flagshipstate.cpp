// Tests the autopilot target of uqm::FlagshipState.

#include <gtest/gtest.h>
#include "uqm/session/FlagshipState.h"

namespace
{

TEST(AutopilotTest, StartsWithNoTarget)
{
	const uqm::Autopilot autopilot {};
	EXPECT_FALSE(autopilot.hasTarget());
	EXPECT_EQ(autopilot.target().x, -1);
	EXPECT_EQ(autopilot.target().y, -1);
}

TEST(AutopilotTest, SetTargetAndClear)
{
	uqm::Autopilot autopilot {};
	autopilot.setTarget({1752, 1450});
	EXPECT_TRUE(autopilot.hasTarget());
	EXPECT_EQ(autopilot.target().x, 1752);
	EXPECT_EQ(autopilot.target().y, 1450);

	autopilot.clear();
	EXPECT_FALSE(autopilot.hasTarget());
}

TEST(AutopilotTest, OneMissingCoordinateIsNoTarget)
{
	uqm::Autopilot autopilot {};
	autopilot.setTarget({100, -1});
	EXPECT_FALSE(autopilot.hasTarget());
	autopilot.setTarget({-1, 100});
	EXPECT_FALSE(autopilot.hasTarget());
}

} // namespace
