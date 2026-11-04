// Tests for reading animation files.
#include <gtest/gtest.h>

#include "libs/graphics/anifile.h"

using namespace uqm;

TEST(AniFileTest, FramesTakeTheDefaults)
{
	const auto frames {aniFromJson(R"({"version": 1,
		"defaults": {"transparency": 0, "colormap": 10, "hotspot": [11, -3]},
		"frames": [{"image": "a.png"}, {"image": "b.png", "transparency": "none", "colormap": null, "hotspot": [0, 0]}]})")};
	ASSERT_TRUE(frames);
	ASSERT_EQ(frames->size(), 2u);

	EXPECT_EQ((*frames)[0].image, "a.png");
	EXPECT_EQ((*frames)[0].transparentColor, 0);
	EXPECT_EQ((*frames)[0].colormapIndex, 10);
	EXPECT_EQ((*frames)[0].hotspotX, 11);
	EXPECT_EQ((*frames)[0].hotspotY, -3);

	EXPECT_EQ((*frames)[1].image, "b.png");
	EXPECT_EQ((*frames)[1].transparentColor, c_aniTransparencyNone);
	EXPECT_EQ((*frames)[1].colormapIndex, c_aniNoColormap);
	EXPECT_EQ((*frames)[1].hotspotX, 0);
	EXPECT_EQ((*frames)[1].hotspotY, 0);
}

TEST(AniFileTest, FieldsWithoutDefaults)
{
	const auto frames {aniFromJson(R"({"version": 1, "frames": [{"image": "a.png"}, {"image": "b.png", "transparency": "png"}]})")};
	ASSERT_TRUE(frames);
	ASSERT_EQ(frames->size(), 2u);
	for (const AniFrame& frame : *frames)
	{
		EXPECT_EQ(frame.transparentColor, c_aniTransparencyPng);
		EXPECT_EQ(frame.colormapIndex, c_aniNoColormap);
		EXPECT_EQ(frame.hotspotX, 0);
		EXPECT_EQ(frame.hotspotY, 0);
	}
}

TEST(AniFileTest, RejectsInvalidFiles)
{
	EXPECT_FALSE(aniFromJson("not json"));
	EXPECT_FALSE(aniFromJson(R"({"frames": []})"));
	EXPECT_FALSE(aniFromJson(R"({"version": 2, "frames": []})"));
	EXPECT_FALSE(aniFromJson(R"({"version": 1, "frames": [{"transparency": 0}]})"));
	EXPECT_FALSE(aniFromJson(R"({"version": 1, "frames": [{"image": "a.png", "transparency": "some"}]})"));
	EXPECT_FALSE(aniFromJson(R"({"version": 1, "frames": [{"image": "a.png", "transparency": 256}]})"));
	EXPECT_FALSE(aniFromJson(R"({"version": 1, "frames": [{"image": "a.png", "colormap": -1}]})"));
	EXPECT_FALSE(aniFromJson(R"({"version": 1, "frames": [{"image": "a.png", "hotspot": [1]}]})"));
	EXPECT_FALSE(aniFromJson(R"({"version": 1, "defaults": {"hotspot": "x"}, "frames": [{"image": "a.png"}]})"));
}
