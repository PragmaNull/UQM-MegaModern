// Tests for reading resource index files.
#include <gtest/gtest.h>

#include "libs/resource/resindex.h"

using namespace uqm;

TEST(ResourceIndexTest, ReadsEntriesInFileOrder)
{
	const auto entries {resourceIndexFromJson(R"({"version": 1, "resources": {
		"graphics.earthmask": {"type": "GFXRES", "data": "base/earthmask.ani"},
		"ship.arilou.code": {"type": "SHIP", "data": 1},
		"config.scale": {"type": "FLT32", "data": 1.5},
		"config.on": {"type": "bool", "data": true},
		"comm.arilou.dialogue": {"type": "CONVERSATION", "data": {"text": "arilou.json", "clips": "arilou/"}}
	}})",
											  "")};
	ASSERT_TRUE(entries);
	ASSERT_EQ(entries->size(), 5u);

	EXPECT_EQ((*entries)[0].key, "graphics.earthmask");
	EXPECT_EQ((*entries)[0].type, "GFXRES");
	EXPECT_STREQ(resourceString((*entries)[0].arg), "base/earthmask.ani");

	EXPECT_EQ(uqstl::get<int64_t>((*entries)[1].arg), 1);
	EXPECT_EQ(uqstl::get<double>((*entries)[2].arg), 1.5);
	EXPECT_TRUE(uqstl::get<bool>((*entries)[3].arg));

	const ResourceArg& conversation {(*entries)[4].arg};
	EXPECT_EQ(resourceString(conversation), nullptr);
	EXPECT_STREQ(resourceStringField(conversation, "text"), "arilou.json");
	EXPECT_STREQ(resourceStringField(conversation, "clips"), "arilou/");
	EXPECT_EQ(resourceStringField(conversation, "timing"), nullptr);
}

TEST(ResourceIndexTest, PutsThePrefixBeforeEachKey)
{
	const auto entries {resourceIndexFromJson(R"({"version": 1, "resources": {"up.1": {"type": "STRING", "data": "key Up"}}})", "menu.")};
	ASSERT_TRUE(entries);
	ASSERT_EQ(entries->size(), 1u);
	EXPECT_EQ((*entries)[0].key, "menu.up.1");
}

TEST(ResourceIndexTest, ReadsIntegerFields)
{
	const auto entries {resourceIndexFromJson(R"({"version": 1, "resources": {
		"slides.intro": {"type": "3DOVID", "data": {"video": "intro.duk", "loop": 30}}
	}})",
											  "")};
	ASSERT_TRUE(entries);
	ASSERT_EQ(entries->size(), 1u);
	EXPECT_EQ(resourceIntField((*entries)[0].arg, "loop"), 30);
	EXPECT_EQ(resourceIntField((*entries)[0].arg, "video"), uqstl::nullopt);
	EXPECT_EQ(resourceIntField((*entries)[0].arg, "audio"), uqstl::nullopt);
}

TEST(ResourceIndexTest, SkipsInvalidEntries)
{
	const auto entries {resourceIndexFromJson(R"({"version": 1, "resources": {
		"no.type": {"data": "x"},
		"no.data": {"type": "STRING"},
		"nested": {"type": "STRING", "data": {"a": {"b": 1}}},
		"array": {"type": "STRING", "data": [1, 2]},
		"good": {"type": "STRING", "data": "x"}
	}})",
											  "")};
	ASSERT_TRUE(entries);
	ASSERT_EQ(entries->size(), 1u);
	EXPECT_EQ((*entries)[0].key, "good");
}

TEST(ResourceIndexTest, RejectsInvalidFiles)
{
	EXPECT_FALSE(resourceIndexFromJson("not json", ""));
	EXPECT_FALSE(resourceIndexFromJson(R"({"resources": {}})", ""));
	EXPECT_FALSE(resourceIndexFromJson(R"({"version": 2, "resources": {}})", ""));
	EXPECT_FALSE(resourceIndexFromJson(R"({"version": 1, "resources": []})", ""));
}
