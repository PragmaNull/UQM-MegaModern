// Tests for reading slideshow files.
#include <gtest/gtest.h>

#include "uqm/slideshow.h"

#include <fmt/format.h>

using namespace uqm;

TEST(SlideshowTest, ReadsCommands)
{
	const auto commands {slideshowFromJson(R"({"version": 1, "commands": [
		{"comment": "Set size", "op": "dims", "width": 320, "height": 240},
		{"op": "font", "resolution": "hd", "index": 1, "file": "fonts/slides.fon"},
		{"op": "textColor", "color": [85, 255, 85]},
		{"op": "textAlign", "align": "middle"},
		{"op": "textEffect", "effect": "traced"},
		{"op": "textFadeIn", "lines": ["When I awoke,", "there was an angel"]},
		{"op": "textSpin", "x": 68, "y": 53, "text": " Accessing"},
		{"op": "spinStat", "filled": 4, "empty": 2, "text": "CREW"},
		{"op": "lineSpin", "x1": 1, "y1": 2, "x2": 3, "y2": 4},
		{"op": "movie", "fps": 10, "from": 2, "to": 5},
		{"op": "sync", "ms": 400}
	]})")};
	ASSERT_TRUE(commands);
	ASSERT_EQ(commands->size(), 11u);

	EXPECT_EQ((*commands)[0].op, SlideOp::Dims);
	EXPECT_EQ((*commands)[0].width, 320);
	EXPECT_EQ((*commands)[0].height, 240);
	EXPECT_EQ((*commands)[0].resolution, SlideResolution::Any);

	EXPECT_EQ((*commands)[1].op, SlideOp::Font);
	EXPECT_EQ((*commands)[1].resolution, SlideResolution::Hd);
	EXPECT_EQ((*commands)[1].index, 1);
	EXPECT_EQ((*commands)[1].text, "fonts/slides.fon");

	EXPECT_EQ((*commands)[2].color, (uqstl::array<uint8_t, 3> {85, 255, 85}));
	EXPECT_EQ((*commands)[3].align, SlideTextAlign::Middle);
	EXPECT_TRUE((*commands)[4].traced);
	EXPECT_EQ((*commands)[5].lines, (uqstl::vector<uqstl::string> {"When I awoke,", "there was an angel"}));

	EXPECT_EQ((*commands)[6].x, 68);
	EXPECT_EQ((*commands)[6].y, 53);
	EXPECT_EQ((*commands)[6].text, " Accessing");

	EXPECT_EQ((*commands)[7].filled, 4);
	EXPECT_EQ((*commands)[7].empty, 2);

	EXPECT_EQ((*commands)[8].x2, 3);
	EXPECT_EQ((*commands)[8].y2, 4);

	EXPECT_EQ((*commands)[9].fps, 10);
	EXPECT_EQ((*commands)[9].index, 2);
	EXPECT_EQ((*commands)[9].lastIndex, 5);

	EXPECT_EQ((*commands)[10].ms, 400);
}

TEST(SlideshowTest, DrawPlacementIsOptional)
{
	const auto commands {slideshowFromJson(R"({"version": 1, "commands": [
		{"op": "draw", "frame": 3},
		{"op": "draw", "frame": 4, "x": 10, "y": 20, "scale": 512, "angle": 90},
		{"op": "drawSis", "x": 202, "y": 310}
	]})")};
	ASSERT_TRUE(commands);
	ASSERT_EQ(commands->size(), 3u);

	EXPECT_EQ((*commands)[0].index, 3);
	EXPECT_EQ((*commands)[0].x, 0);
	EXPECT_EQ((*commands)[0].y, 0);
	EXPECT_FALSE((*commands)[0].scale);

	EXPECT_EQ((*commands)[1].x, 10);
	EXPECT_EQ((*commands)[1].scale, 512);
	EXPECT_EQ((*commands)[1].angle, 90);

	EXPECT_EQ((*commands)[2].op, SlideOp::DrawSis);
	EXPECT_EQ((*commands)[2].x, 202);
	EXPECT_EQ((*commands)[2].y, 310);
}

TEST(SlideshowTest, RejectsInvalidCommands)
{
	const auto invalid {[](const char* command) {
		return !slideshowFromJson(fmt::format(R"({{"version": 1, "commands": [{}]}})", command));
	}};
	EXPECT_FALSE(slideshowFromJson("not json"));
	EXPECT_FALSE(slideshowFromJson(R"({"commands": []})"));
	EXPECT_TRUE(invalid(R"({"op": "jump"})"));
	EXPECT_TRUE(invalid(R"({"op": "dims", "width": 320})"));
	EXPECT_TRUE(invalid(R"({"op": "font", "index": 5})"));
	EXPECT_TRUE(invalid(R"({"op": "font", "index": 0, "resolution": "4k"})"));
	EXPECT_TRUE(invalid(R"({"op": "textColor", "color": [1, 2]})"));
	EXPECT_TRUE(invalid(R"({"op": "textColor", "color": [1, 2, 256]})"));
	EXPECT_TRUE(invalid(R"({"op": "textAlign", "align": "left"})"));
	EXPECT_TRUE(invalid(R"({"op": "textFadeIn", "lines": []})"));
	EXPECT_TRUE(invalid(R"({"op": "draw", "frame": 1, "scale": 256})"));
	EXPECT_TRUE(invalid(R"({"op": "draw", "frame": 1, "x": 1, "y": 2, "angle": 90})"));
	EXPECT_TRUE(invalid(R"({"op": "movie", "fps": 0, "from": 0, "to": 1})"));
	EXPECT_TRUE(invalid(R"({"op": "movie", "fps": 10, "from": 5, "to": 1})"));
	EXPECT_TRUE(invalid(R"({"op": "animate", "first": 0, "last": 1, "loops": 1, "ms": 0, "fps": 0})"));
}
