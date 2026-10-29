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
	Setting<bool> enabled {
		*this, {.key = "general.enabled", .description = "A bool.", .cli = "--enabled"},
		 false
	  };
	Setting<int> percent {
		*this, {.key = "general.percent", .description = "An int with a validator.", .cli = "--percent"},
		  50, isPercent
	   };
	Setting<float> scale {
		*this, {.key = "audio.scale", .description = "A float."},
		  1.0f
	  };
	Setting<uqstl::string> name {
		*this, {.key = "name", .description = "A top-level string."},
		  "default"
	   };
	Setting<Vec2u> size {
		*this, {.key = "graphics.window.size", .description = "A nested Vec2u."},
		  Vec2u {640,							  480							 }
	  };
	Setting<Color> color {
		*this, {.key = "graphics.color", .description = "An enum."},
		 Color::Red
	   };
	Setting<Flags> flags {
		*this, {.key = "graphics.flags", .description = "A flag enum."},
		 Flags::None
	};
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
	settings.percent.markCommandLine();

	ASSERT_TRUE(settings.fromJson(R"({"general": {"percent": 80}})"));
	EXPECT_EQ(*settings.percent, 10);
	EXPECT_TRUE(settings.percent.hasCommandLineOverride());
}

TEST(SettingTest, CommandLineValueEqualToDefaultWinsOverFile)
{
	TestSettings settings {};
	settings.percent.markCommandLine();

	ASSERT_TRUE(settings.fromJson(R"({"general": {"percent": 80}})"));
	EXPECT_EQ(*settings.percent, 50);
}

TEST(SettingTest, CommandLineOverrideIsNotSaved)
{
	TestSettings settings {};
	settings.percent = 10;
	settings.percent.markCommandLine();
	ASSERT_TRUE(settings.fromJson(R"({"general": {"percent": 80}})"));

	TestSettings saved {};
	ASSERT_TRUE(saved.fromJson(settings.toJson()));
	EXPECT_EQ(*saved.percent, 80);
}

TEST(SettingTest, ChangeAfterCommandLineOverrideIsSaved)
{
	TestSettings settings {};
	settings.percent = 10;
	settings.percent.markCommandLine();
	settings.percent = 30;
	EXPECT_FALSE(settings.percent.hasCommandLineOverride());

	TestSettings saved {};
	ASSERT_TRUE(saved.fromJson(settings.toJson()));
	EXPECT_EQ(*saved.percent, 30);
}

TEST(SettingTest, ResetRestoresDefaults)
{
	TestSettings settings {};
	settings.percent = 10;
	settings.percent.markCommandLine();
	settings.resetAll();
	EXPECT_EQ(*settings.percent, 50);
	EXPECT_FALSE(settings.percent.set);

	ASSERT_TRUE(settings.fromJson(R"({"general": {"percent": 80}})"));
	EXPECT_EQ(*settings.percent, 80);
}

TEST(SettingTest, ParsesTextForEachType)
{
	TestSettings settings {};
	EXPECT_TRUE(settings.enabled.parseText("true"));
	EXPECT_TRUE(*settings.enabled);
	EXPECT_TRUE(settings.percent.parseText("75"));
	EXPECT_EQ(*settings.percent, 75);
	EXPECT_TRUE(settings.scale.parseText("0.5"));
	EXPECT_FLOAT_EQ(*settings.scale, 0.5f);
	EXPECT_TRUE(settings.name.parseText("text"));
	EXPECT_EQ(*settings.name, "text");
	EXPECT_TRUE(settings.size.parseText("800x600"));
	EXPECT_EQ(*settings.size, (Vec2u {800, 600}));
	EXPECT_TRUE(settings.color.parseText("blue"));
	EXPECT_EQ(*settings.color, Color::Blue);
	EXPECT_TRUE(settings.color.parseText("1"));
	EXPECT_EQ(*settings.color, Color::Green);
	EXPECT_TRUE(settings.flags.parseText("3"));
	EXPECT_EQ(*settings.flags, Flags::A | Flags::B);
}

TEST(SettingTest, RejectsInvalidText)
{
	TestSettings settings {};
	EXPECT_FALSE(settings.percent.parseText("abc"));
	EXPECT_FALSE(settings.percent.parseText("150"));
	EXPECT_FALSE(settings.size.parseText("800"));
	EXPECT_FALSE(settings.color.parseText("Purple"));
	EXPECT_FALSE(settings.color.parseText("7"));
	EXPECT_FALSE(settings.flags.parseText("4"));
	EXPECT_EQ(*settings.percent, 50);
	EXPECT_EQ(*settings.color, Color::Red);
}

TEST(SettingTest, DescribesCommandLineValues)
{
	TestSettings settings {};
	EXPECT_TRUE(settings.enabled.isFlag());
	EXPECT_FALSE(settings.percent.isFlag());
	EXPECT_EQ(settings.percent.cliNames(), "--percent");
	EXPECT_TRUE(settings.scale.cliNames().empty());
	EXPECT_EQ(settings.percent.defaultText(), "50");
	EXPECT_EQ(settings.size.defaultText(), "640x480");
	EXPECT_EQ(settings.color.defaultText(), "Red");
	EXPECT_EQ(settings.color.valueNames(), (uqstl::vector<uqstl::string> {"Red", "Green", "Blue"}));
	EXPECT_TRUE(settings.percent.valueNames().empty());
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

TEST(OptionsSettingsTest, CommandLineNamesAreUnique)
{
	const OptionsStruct options {};
	uqstl::vector<uqstl::string> seen {};
	for (const SettingBase* setting : options.settings())
	{
		uqstl::string_view names {setting->cliNames()};
		while (!names.empty())
		{
			const size_t comma {names.find(',')};
			uqstl::string name {names.substr(0, comma)};
			name = name.substr(0, name.find('{'));
			EXPECT_EQ(uqstl::count(seen.begin(), seen.end(), name), 0) << name;
			seen.push_back(name);
			names = comma == uqstl::string_view::npos ? uqstl::string_view {} : names.substr(comma + 1);
		}
	}
	EXPECT_FALSE(seen.empty());
}

TEST(OptionsSettingsTest, EmulationModeAcceptsOnlyPcAnd3do)
{
	OptionsStruct options {};
	EXPECT_EQ(options.whichFonts.valueNames(), (uqstl::vector<uqstl::string> {"PC", "3DO"}));
	EXPECT_TRUE(options.whichFonts.parseText("3do"));
	EXPECT_EQ(*options.whichFonts, EmulationMode::Console3DO);
	EXPECT_FALSE(options.whichFonts.parseText("None"));

	ASSERT_TRUE(options.fromJson(R"({"interface": {"fontStyle": "None"}})"));
	EXPECT_EQ(*options.whichFonts, EmulationMode::Console3DO);
	ASSERT_TRUE(options.fromJson(R"({"interface": {"fontStyle": "PC"}})"));
	EXPECT_EQ(*options.whichFonts, EmulationMode::PC);
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
