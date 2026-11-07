// Tests the table of game state flags and the lookup by name.

#include <gtest/gtest.h>
#include "uqm/session/GameFlags.h"

namespace
{

TEST(GameFlagsTest, EveryNameFindsItsFlag)
{
	for (size_t index {0}; index < uqm::c_gameFlagCount; ++index)
	{
		const auto flag {uqm::findGameFlag(uqm::c_gameFlags[index].name)};
		ASSERT_TRUE(flag.has_value()) << uqm::c_gameFlags[index].name;
		EXPECT_EQ(static_cast<size_t>(*flag), index);
	}
}

TEST(GameFlagsTest, NamesAreUnique)
{
	uqstl::set<uqstl::string_view> names {};
	for (const uqm::GameFlagInfo& info : uqm::c_gameFlags)
	{
		EXPECT_TRUE(names.insert(info.name).second) << info.name;
	}
}

TEST(GameFlagsTest, EnumAndTableAgree)
{
	EXPECT_EQ(uqm::gameFlagInfo(uqm::GameFlag::SHOFIXTI_VISITS).name, "SHOFIXTI_VISITS");
	EXPECT_EQ(uqm::gameFlagInfo(uqm::GameFlag::SHOFIXTI_VISITS).bits, 3);
	EXPECT_EQ(uqm::gameFlagInfo(uqm::GameFlag::CHMMR_BOMB_STATE).name, "CHMMR_BOMB_STATE");
}

TEST(GameFlagsTest, UnknownNamesAreNotFound)
{
	EXPECT_FALSE(uqm::findGameFlag("KNOW_SPATHI_HOMEWORLD").has_value());
	EXPECT_FALSE(uqm::findGameFlag("").has_value());
}

} // namespace
