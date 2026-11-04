// Tests for reading font metrics files.
#include <gtest/gtest.h>

#include "libs/graphics/fontfile.h"

using namespace uqm;

TEST(FontFileTest, ReadsMetricsAndKerning)
{
	const auto metrics {fontMetricsFromJson(R"({"version": 1, "leading": 36, "charSpace": 4, "kernAmount": 0,
		"vertAlign": -2, "kerning": {" ": [3, 3], "A": [1, 2], "é": [0, 3]}})")};
	ASSERT_TRUE(metrics);
	EXPECT_EQ(metrics->leading, 36);
	EXPECT_EQ(metrics->charSpace, 4);
	EXPECT_EQ(metrics->kernAmount, 0);
	EXPECT_EQ(metrics->vertAlign, -2);
	ASSERT_EQ(metrics->kerning.size(), 3u);
	EXPECT_EQ(metrics->kerning[0].ch, UniChar {' '});
	EXPECT_EQ(metrics->kerning[1].ch, UniChar {'A'});
	EXPECT_EQ(metrics->kerning[1].left, 1);
	EXPECT_EQ(metrics->kerning[1].right, 2);
	EXPECT_EQ(metrics->kerning[2].ch, UniChar {0xE9});
}

TEST(FontFileTest, KerningIsOptional)
{
	const auto metrics {fontMetricsFromJson(R"({"version": 1, "leading": 8, "charSpace": 1, "kernAmount": 1, "vertAlign": 0})")};
	ASSERT_TRUE(metrics);
	EXPECT_TRUE(metrics->kerning.empty());
}

TEST(FontFileTest, RejectsInvalidFiles)
{
	EXPECT_FALSE(fontMetricsFromJson("not json"));
	EXPECT_FALSE(fontMetricsFromJson(R"({"version": 1, "leading": 8, "charSpace": 1, "kernAmount": 1})"));
	EXPECT_FALSE(fontMetricsFromJson(R"({"version": 1, "leading": 256, "charSpace": 1, "kernAmount": 1, "vertAlign": 0})"));
	EXPECT_FALSE(fontMetricsFromJson(R"({"version": 1, "leading": 8, "charSpace": 1, "kernAmount": 1, "vertAlign": -129})"));
	EXPECT_FALSE(fontMetricsFromJson(R"({"version": 1, "leading": 8, "charSpace": 1, "kernAmount": 1, "vertAlign": 0, "kerning": {"AB": [1, 1]}})"));
	EXPECT_FALSE(fontMetricsFromJson(R"({"version": 1, "leading": 8, "charSpace": 1, "kernAmount": 1, "vertAlign": 0, "kerning": {"A": [4, 1]}})"));
	EXPECT_FALSE(fontMetricsFromJson(R"({"version": 1, "leading": 8, "charSpace": 1, "kernAmount": 1, "vertAlign": 0, "kerning": {"A": [1]}})"));
}
