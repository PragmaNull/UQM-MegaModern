// Tests for reading the credits file.
#include <gtest/gtest.h>

#include "uqm/creditsfile.h"

using namespace uqm;

TEST(CreditsFileTest, ReadsBlocks)
{
	const auto blocks {creditsFromJson(R"({"version": 1, "blocks": [
		{"id": "Art caption", "size": 17, "columns": [{"align": "center"}], "lines": ["ART"]},
		{"size": 13, "columns": [{"align": "right", "x": 146}, {"align": "left", "x": 165}],
		 "lines": [["Fred Ford", "Programming"], "Paul Reiche III"]}
	]})")};
	ASSERT_TRUE(blocks);
	ASSERT_EQ(blocks->size(), 2u);

	const CreditsBlock& caption {(*blocks)[0]};
	EXPECT_EQ(caption.size, 17);
	ASSERT_EQ(caption.columns.size(), 1u);
	EXPECT_EQ(caption.columns[0].align, CreditsAlign::Center);
	EXPECT_FALSE(caption.columns[0].x);
	ASSERT_EQ(caption.lines.size(), 1u);
	EXPECT_EQ(caption.lines[0], (uqstl::vector<uqstl::string> {"ART"}));

	const CreditsBlock& table {(*blocks)[1]};
	ASSERT_EQ(table.columns.size(), 2u);
	EXPECT_EQ(table.columns[0].align, CreditsAlign::Right);
	EXPECT_EQ(table.columns[0].x, 146);
	EXPECT_EQ(table.columns[1].align, CreditsAlign::Left);
	EXPECT_EQ(table.columns[1].x, 165);
	ASSERT_EQ(table.lines.size(), 2u);
	EXPECT_EQ(table.lines[0], (uqstl::vector<uqstl::string> {"Fred Ford", "Programming"}));
	EXPECT_EQ(table.lines[1], (uqstl::vector<uqstl::string> {"Paul Reiche III"}));
}

TEST(CreditsFileTest, RejectsInvalidFiles)
{
	EXPECT_FALSE(creditsFromJson("not json"));
	EXPECT_FALSE(creditsFromJson(R"({"blocks": []})"));
	EXPECT_FALSE(creditsFromJson(R"({"version": 1, "blocks": [{"size": 0, "columns": [{"align": "left"}], "lines": ["a"]}]})"));
	EXPECT_FALSE(creditsFromJson(R"({"version": 1, "blocks": [{"size": 13, "columns": [], "lines": ["a"]}]})"));
	EXPECT_FALSE(creditsFromJson(R"({"version": 1, "blocks": [{"size": 13, "columns": [{"align": "up"}], "lines": ["a"]}]})"));
	EXPECT_FALSE(creditsFromJson(R"({"version": 1, "blocks": [{"size": 13, "columns": [{"align": "left"}], "lines": []}]})"));
	EXPECT_FALSE(creditsFromJson(R"({"version": 1, "blocks": [{"size": 13, "columns": [{"align": "left"}], "lines": [[]]}]})"));
	EXPECT_FALSE(creditsFromJson(R"({"version": 1, "blocks": [{"size": 13, "columns": [{"align": "left"}], "lines": [["a", 1]]}]})"));
}
