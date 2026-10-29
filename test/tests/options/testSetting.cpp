#include <gtest/gtest.h>

#include "options/Setting.h"
#include "options/options.h"

namespace uqm
{

namespace
{

enum class Color
{
	Red,
	Green,
	Blue,
};

enum class Flags
{
	None = 0x0,
	A = 0x1,
	B = 0x2,
};
ENUM_BITWISE_OPS(Flags);

bool isPercent(const int& value)
{
	return value >= 0 && value <= 100;
}

struct TestSettings : SettingsRegistry
{
	Setting<bool> enabled {*this, "general.enabled", false, "A bool."};
	Setting<int> percent {*this, "general.percent", 50, "An int with a validator.", isPercent};
	Setting<float> scale {*this, "audio.scale", 1.0f, "A float."};
	Setting<uqstl::string> name {*this, "name", "default", "A top-level string."};
	Setting<Vec2u> size {
		*this, "graphics.window.size", Vec2u {640, 480},
		"A nested Vec2u."
	 };
	Setting<Color> color {*this, "graphics.color", Color::Red, "An enum."};
	Setting<Flags> flags {*this, "graphics.flags", Flags::None, "A flag enum."};
};

} // namespace

TEST(SettingTest, RegistersInDeclarationOrder)
{
	TestSettings settings {};
	ASSERT_EQ(settings.settings().size(), 7u);
	EXPECT_EQ(settings.settings()[0]->key(), "general.enabled");
	EXPECT_EQ(settings.settings()[6]->key(), "graphics.flags");
	EXPECT_EQ(settings.find("audio.scale"), &settings.scale);
	EXPECT_EQ(settings.find("missing"), nullptr);
}

TEST(SettingTest, BehavesLikeOptionT)
{
	TestSettings settings {};
	EXPECT_FALSE(settings.enabled.set);
	settings.enabled = true;
	EXPECT_TRUE(settings.enabled.set);
	EXPECT_TRUE(*settings.enabled);
	EXPECT_EQ(settings.percent.defaultValue(), 50);
}

TEST(SettingTest, RoundTripsEveryType)
{
	TestSettings source {};
	source.enabled = true;
	source.percent = 75;
	source.scale = 0.25f;
	source.name = uqstl::string {"custom"};
	source.size = Vec2u {1920, 1080};
	source.color = Color::Blue;
	source.flags = Flags::A | Flags::B;

	TestSettings loaded {};
	ASSERT_TRUE(loaded.fromJson(source.toJson()));
	EXPECT_TRUE(*loaded.enabled);
	EXPECT_EQ(*loaded.percent, 75);
	EXPECT_FLOAT_EQ(*loaded.scale, 0.25f);
	EXPECT_EQ(*loaded.name, "custom");
	EXPECT_EQ(*loaded.size, (Vec2u {1920, 1080}));
	EXPECT_EQ(*loaded.color, Color::Blue);
	EXPECT_EQ(*loaded.flags, Flags::A | Flags::B);
}

TEST(SettingTest, WritesNestedObjectsAndVersion)
{
	TestSettings settings {};
	settings.color = Color::Green;
	const uqstl::string json {settings.toJson()};
	EXPECT_NE(json.find("\"version\": 1"), uqstl::string::npos);
	EXPECT_NE(json.find("\"window\": {"), uqstl::string::npos);
	EXPECT_NE(json.find("\"color\": \"Green\""), uqstl::string::npos);
}

TEST(SettingTest, WritesFloatsAtSinglePrecision)
{
	TestSettings settings {};
	settings.scale = 0.85f;
	const uqstl::string json {settings.toJson()};
	EXPECT_NE(json.find("\"scale\": 0.85"), uqstl::string::npos) << json;
	EXPECT_EQ(json.find("0.8500000"), uqstl::string::npos) << json;
}

TEST(SettingTest, ReadsEnumsByNameOrNumber)
{
	TestSettings settings {};
	ASSERT_TRUE(settings.fromJson(R"({"graphics": {"color": "blue"}})"));
	EXPECT_EQ(*settings.color, Color::Blue);
	ASSERT_TRUE(settings.fromJson(R"({"graphics": {"color": 1}})"));
	EXPECT_EQ(*settings.color, Color::Green);
}

TEST(SettingTest, RejectsInvalidValuesAndKeepsCurrent)
{
	TestSettings settings {};
	ASSERT_TRUE(settings.fromJson(R"({
		"general": {"enabled": "yes", "percent": 150},
		"graphics": {"color": "Purple", "window": {"size": {"x": -1, "y": 2}}},
		"audio": {"scale": "loud"}
	})"));
	EXPECT_FALSE(*settings.enabled);
	EXPECT_EQ(*settings.percent, 50);
	EXPECT_EQ(*settings.color, Color::Red);
	EXPECT_EQ(*settings.size, (Vec2u {640, 480}));
	EXPECT_FLOAT_EQ(*settings.scale, 1.0f);
}

TEST(SettingTest, RejectsOutOfRangeEnumNumber)
{
	TestSettings settings {};
	ASSERT_TRUE(settings.fromJson(R"({"graphics": {"color": 7}})"));
	EXPECT_EQ(*settings.color, Color::Red);
}

TEST(SettingTest, FlagEnumNumberMustUseDeclaredBits)
{
	TestSettings settings {};
	ASSERT_TRUE(settings.fromJson(R"({"graphics": {"flags": 3}})"));
	EXPECT_EQ(*settings.flags, Flags::A | Flags::B);

	ASSERT_TRUE(settings.fromJson(R"({"graphics": {"flags": 4}})"));
	EXPECT_EQ(*settings.flags, Flags::A | Flags::B);

	ASSERT_TRUE(settings.fromJson(R"({"graphics": {"flags": -1}})"));
	EXPECT_EQ(*settings.flags, Flags::A | Flags::B);
}

TEST(SettingTest, IgnoresUnknownKeys)
{
	TestSettings settings {};
	ASSERT_TRUE(settings.fromJson(R"({"unknown": 1, "general": {"other": true, "enabled": true}})"));
	EXPECT_TRUE(*settings.enabled);
}

TEST(SettingTest, RejectsMalformedJson)
{
	TestSettings settings {};
	EXPECT_FALSE(settings.fromJson("{ not json"));
	EXPECT_FALSE(settings.fromJson("[1, 2]"));
}

TEST(SettingTest, CommandLineOverrideWinsOverFile)
{
	TestSettings settings {};
	settings.percent = 10;
	settings.captureCommandLine();

	ASSERT_TRUE(settings.fromJson(R"({"general": {"percent": 80}})"));
	EXPECT_EQ(*settings.percent, 10);
}

TEST(SettingTest, CommandLineOverrideIsNotSaved)
{
	TestSettings settings {};
	settings.percent = 10;
	settings.captureCommandLine();
	ASSERT_TRUE(settings.fromJson(R"({"general": {"percent": 80}})"));

	TestSettings saved {};
	ASSERT_TRUE(saved.fromJson(settings.toJson()));
	EXPECT_EQ(*saved.percent, 80);
}

TEST(SettingTest, ChangeAfterCommandLineOverrideIsSaved)
{
	TestSettings settings {};
	settings.percent = 10;
	settings.captureCommandLine();
	settings.percent = 30;

	TestSettings saved {};
	ASSERT_TRUE(saved.fromJson(settings.toJson()));
	EXPECT_EQ(*saved.percent, 30);
}

TEST(SettingTest, ResetRestoresDefaults)
{
	TestSettings settings {};
	settings.percent = 10;
	settings.captureCommandLine();
	settings.resetAll();
	EXPECT_EQ(*settings.percent, 50);
	EXPECT_FALSE(settings.percent.set);

	ASSERT_TRUE(settings.fromJson(R"({"general": {"percent": 80}})"));
	EXPECT_EQ(*settings.percent, 80);
}

TEST(OptionsSettingsTest, KeysAreUniqueAndDescribed)
{
	const OptionsStruct options {};
	const auto all {options.settings()};
	ASSERT_FALSE(all.empty());

	for (size_t i = 0; i < all.size(); ++i)
	{
		const uqstl::string_view key {all[i]->key()};
		EXPECT_FALSE(key.empty());
		EXPECT_FALSE(all[i]->description().empty()) << key;

		for (size_t j = i + 1; j < all.size(); ++j)
		{
			const uqstl::string_view other {all[j]->key()};
			EXPECT_NE(key, other);
			EXPECT_FALSE(other.starts_with(uqstl::string {key} + ".")) << key << " is a prefix of " << other;
			EXPECT_FALSE(key.starts_with(uqstl::string {other} + ".")) << other << " is a prefix of " << key;
		}
	}
}

TEST(OptionsSettingsTest, DefaultsRoundTrip)
{
	const OptionsStruct source {};
	const uqstl::string json {source.toJson()};

	OptionsStruct loaded {};
	ASSERT_TRUE(loaded.fromJson(json));
	EXPECT_EQ(loaded.toJson(), json);
}

TEST(OptionsSettingsTest, ChangedValuesRoundTrip)
{
	OptionsStruct source {};
	source.scanlines = true;
	source.resolution = Vec2u {1280, 960};
	source.musicVolumeScale = 0.5f;
	source.whichFonts = EmulationMode::Console3DO;
	source.optDiffChooser = Difficulty::Hard;
	source.player2Control = 1;

	OptionsStruct loaded {};
	ASSERT_TRUE(loaded.fromJson(source.toJson()));
	EXPECT_TRUE(*loaded.scanlines);
	EXPECT_EQ(*loaded.resolution, (Vec2u {1280, 960}));
	EXPECT_FLOAT_EQ(*loaded.musicVolumeScale, 0.5f);
	EXPECT_EQ(*loaded.whichFonts, EmulationMode::Console3DO);
	EXPECT_EQ(*loaded.optDiffChooser, Difficulty::Hard);
	EXPECT_EQ(*loaded.player2Control, 1);
}

} // namespace uqm
