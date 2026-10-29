// Tests for the SuperMelee configuration JSON format.
#include <gtest/gtest.h>

#include "uqm/supermelee/meleeconfig.h"

namespace uqm
{

TEST(MeleeConfigTest, RoundTripsSides)
{
	const uqstl::vector<MeleeSideConfig> sides {
		{PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard, "Ur-Quan Rush", {MELEE_URQUAN, MELEE_NONE, MELEE_KOHR_AH}},
		{PlayerControlFlags::Cyborg | PlayerControlFlags::DifficultyAwesome, "Spathi Swarm", {MELEE_SPATHI, MELEE_SPATHI}			 },
	};

	const auto loaded {meleeConfigFromJson(meleeConfigToJson(sides))};
	ASSERT_TRUE(loaded.has_value());
	ASSERT_EQ(loaded->size(), 2u);
	EXPECT_EQ((*loaded)[0].control, PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard);
	EXPECT_EQ((*loaded)[0].teamName, "Ur-Quan Rush");
	EXPECT_EQ((*loaded)[0].ships, (uqstl::vector<MeleeShip> {MELEE_URQUAN, MELEE_NONE, MELEE_KOHR_AH}));
	EXPECT_EQ((*loaded)[1].control, PlayerControlFlags::Cyborg | PlayerControlFlags::DifficultyAwesome);
	EXPECT_EQ((*loaded)[1].ships, (uqstl::vector<MeleeShip> {MELEE_SPATHI, MELEE_SPATHI}));
}

TEST(MeleeConfigTest, WritesReadableNames)
{
	const uqstl::vector<MeleeSideConfig> sides {
		{PlayerControlFlags::Human, "Team", {MELEE_ZOQFOTPIK, MELEE_NONE}}
	  };
	const uqstl::string json {meleeConfigToJson(sides)};
	EXPECT_NE(json.find("\"ZOQFOTPIK\""), uqstl::string::npos) << json;
	EXPECT_NE(json.find("null"), uqstl::string::npos) << json;
	EXPECT_NE(json.find("\"Human\""), uqstl::string::npos) << json;
}

TEST(MeleeConfigTest, ReadsNamesInAnyCase)
{
	const auto loaded {meleeConfigFromJson(R"({"sides": [{"control": ["human"], "team": {"name": "T", "ships": ["androsynth", "Kohr_Ah"]}}]})")};
	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ((*loaded)[0].control, PlayerControlFlags::Human);
	EXPECT_EQ((*loaded)[0].ships, (uqstl::vector<MeleeShip> {MELEE_ANDROSYNTH, MELEE_KOHR_AH}));
}

TEST(MeleeConfigTest, RejectsInvalidStructure)
{
	EXPECT_FALSE(meleeConfigFromJson("{ not json"));
	EXPECT_FALSE(meleeConfigFromJson(R"({"sides": {}})"));
	EXPECT_FALSE(meleeConfigFromJson(R"({"sides": [{"control": ["Human"], "team": {"ships": []}}]})"));
	EXPECT_FALSE(meleeConfigFromJson(R"({"sides": [{"control": ["Pilot"], "team": {"name": "T", "ships": []}}]})"));
	EXPECT_FALSE(meleeConfigFromJson(R"({"sides": [{"control": ["Human"], "team": {"name": "T", "ships": ["Starfury"]}}]})"));
	EXPECT_FALSE(meleeConfigFromJson(R"({"sides": [{"control": ["Human"], "team": {"name": "T", "ships": [3]}}]})"));
}

} // namespace uqm
