// Tests for reading sound bank files.
#include <gtest/gtest.h>

#include "libs/sound/soundbank.h"

using namespace uqm;

TEST(SoundBankTest, ReadsSoundsInOrder)
{
	const auto sounds {soundBankFromJson(R"({"version": 1, "sounds": ["getcrew.wav", "shipdies.wav"]})")};
	ASSERT_TRUE(sounds);
	ASSERT_EQ(sounds->size(), 2u);
	EXPECT_EQ((*sounds)[0], "getcrew.wav");
	EXPECT_EQ((*sounds)[1], "shipdies.wav");
}

TEST(SoundBankTest, RejectsInvalidFiles)
{
	EXPECT_FALSE(soundBankFromJson("not json"));
	EXPECT_FALSE(soundBankFromJson(R"({"sounds": []})"));
	EXPECT_FALSE(soundBankFromJson(R"({"version": 1, "sounds": "a.wav"})"));
	EXPECT_FALSE(soundBankFromJson(R"({"version": 1, "sounds": ["a.wav", 2]})"));
}
