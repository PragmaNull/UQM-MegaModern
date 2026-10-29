#include <gtest/gtest.h>

#include "options/SettingsCli.h"
#include "options/options.h"

#include <CLI/CLI.hpp>

#include <map>

namespace uqm
{

namespace
{

struct CliName
{
	uqstl::string key {};
	uqstl::string name {};
	bool negated {};
};

// Every CLI name of every setting.
uqstl::vector<CliName> allCliNames()
{
	const OptionsStruct options {};
	uqstl::vector<CliName> result {};
	for (const SettingBase* setting : options.settings())
	{
		uqstl::string_view names {setting->cliNames()};
		while (!names.empty())
		{
			const size_t comma {names.find(',')};
			uqstl::string name {names.substr(0, comma)};
			const size_t brace {name.find('{')};
			const bool negated {brace != uqstl::string::npos && name.substr(brace) == "{false}"};
			result.push_back({uqstl::string {setting->key()}, name.substr(0, brace), negated});
			names = comma == uqstl::string_view::npos ? uqstl::string_view {} : names.substr(comma + 1);
		}
	}
	return result;
}

// Sample value and expected value text for settings that are not flags or enums.
const uqstl::map<uqstl::string, uqstl::pair<uqstl::string, uqstl::string>> c_sampleValues {
	{"graphics.gamma",		   {"2.5", "2.5"}	 },
	{"audio.musicVolume",	  {"50", "0.5"}	   },
	{"audio.sfxVolume",		{"25", "0.25"}	  },
	{"audio.speechVolume",	   {"75", "0.75"}	 },
	{"cheats.timeDilation",	{"200", "200"}	  },
	{"seed.value",			   {"12345", "12345"}},
	{"visuals.nebulaeVolume", {"25", "25"}	  },
};

// Arguments some options need to be accepted.
const uqstl::map<uqstl::string, uqstl::string> c_requiredArguments {
	{"audio.positionalSfx", "--sounddriver OpenAL"},
};

uqstl::string withRequiredArguments(const CliName& cli, const uqstl::string& commandLine)
{
	const auto required {c_requiredArguments.find(cli.key)};
	return required == c_requiredArguments.end() ? commandLine : commandLine + " " + required->second;
}

struct ParsedOptions
{
	OptionsStruct options {};

	void parse(const uqstl::string& commandLine)
	{
		CLI::App app {};
		SettingsCli cli {app, options};
		app.parse(commandLine, false);
		cli.apply();
	}

	SettingBase& setting(const uqstl::string& key)
	{
		SettingBase* found {options.find(key)};
		EXPECT_NE(found, nullptr) << key;
		return *found;
	}
};

void PrintTo(const CliName& cli, std::ostream* os)
{
	*os << cli.key << " " << cli.name;
}

uqstl::string testName(const testing::TestParamInfo<CliName>& info)
{
	uqstl::string name {info.param.key + "_" + info.param.name};
	for (char& c : name)
	{
		if (!uqstl::isalnum(static_cast<unsigned char>(c)))
		{
			c = '_';
		}
	}
	return name;
}

} // namespace

class SettingsCliOptionTest : public testing::TestWithParam<CliName>
{
};

TEST_P(SettingsCliOptionTest, SetsValueAndMarksOverride)
{
	const CliName& cli {GetParam()};
	const OptionsStruct reference {};
	const SettingBase& referenceSetting {*reference.find(cli.key)};

	if (referenceSetting.isFlag())
	{
		ParsedOptions parsed {};
		parsed.parse(withRequiredArguments(cli, cli.name));
		EXPECT_EQ(parsed.setting(cli.key).valueText(), cli.negated ? "false" : "true");
		EXPECT_TRUE(parsed.setting(cli.key).hasCommandLineOverride());
		return;
	}

	uqstl::vector<uqstl::pair<uqstl::string, uqstl::string>> cases {};
	for (const uqstl::string& name : referenceSetting.valueNames())
	{
		cases.emplace_back(name, name);
	}
	if (cases.empty())
	{
		const auto sample {c_sampleValues.find(cli.key)};
		ASSERT_NE(sample, c_sampleValues.end()) << "Add a sample value for " << cli.key;
		cases.push_back(sample->second);
	}

	for (const auto& [input, expected] : cases)
	{
		ParsedOptions parsed {};
		parsed.parse(withRequiredArguments(cli, cli.name + " " + input));
		EXPECT_EQ(parsed.setting(cli.key).valueText(), expected) << cli.name << " " << input;
		EXPECT_TRUE(parsed.setting(cli.key).hasCommandLineOverride()) << cli.name << " " << input;
	}
}

TEST_P(SettingsCliOptionTest, RejectsInvalidValue)
{
	const CliName& cli {GetParam()};
	const OptionsStruct reference {};
	if (reference.find(cli.key)->isFlag())
	{
		GTEST_SKIP() << "Flags take no value.";
	}

	ParsedOptions parsed {};
	EXPECT_THROW(parsed.parse(cli.name + " bogus"), CLI::ParseError);
}

INSTANTIATE_TEST_SUITE_P(AllSettings, SettingsCliOptionTest, testing::ValuesIn(allCliNames()), testName);

TEST(SettingsCliTest, EverySettingWithCliNamesIsCovered)
{
	const OptionsStruct options {};
	size_t withCli {};
	for (const SettingBase* setting : options.settings())
	{
		withCli += setting->cliNames().empty() ? 0 : 1;
	}
	EXPECT_GT(withCli, 0u);
	EXPECT_GE(allCliNames().size(), withCli);
}

TEST(SettingsCliTest, NoArgumentsLeavesDefaults)
{
	ParsedOptions parsed {};
	parsed.parse("");
	const OptionsStruct reference {};
	for (const SettingBase* setting : parsed.options.settings())
	{
		EXPECT_FALSE(setting->hasCommandLineOverride()) << setting->key();
		EXPECT_EQ(setting->valueText(), reference.find(setting->key())->valueText()) << setting->key();
	}
}

TEST(SettingsCliTest, ValueEqualToDefaultIsAnOverride)
{
	ParsedOptions parsed {};
	parsed.parse("--fullscreen Fullscreen");
	ASSERT_TRUE(parsed.options.fromJson(R"({"graphics": {"windowMode": "Windowed"}})"));
	EXPECT_EQ(*parsed.options.windowMode, WindowMode::Fullscreen);
}

TEST(SettingsCliTest, EnumsAcceptNumbersAndAnyCase)
{
	ParsedOptions parsed {};
	parsed.parse("--difficulty hard --fullscreen 0");
	EXPECT_EQ(*parsed.options.optDiffChooser, Difficulty::Hard);
	EXPECT_EQ(*parsed.options.windowMode, WindowMode::Windowed);
}

TEST(SettingsCliTest, VolumesRejectOutOfRange)
{
	ParsedOptions parsed {};
	EXPECT_THROW(parsed.parse("--musicvol 150"), CLI::ParseError);
	EXPECT_THROW(parsed.parse("--sfxvol -1"), CLI::ParseError);
}

TEST(SettingsCliTest, RangesAreChecked)
{
	EXPECT_THROW(ParsedOptions {}.parse("--gamma 20"), CLI::ParseError);
	EXPECT_THROW(ParsedOptions {}.parse("--timedilation 2000"), CLI::ParseError);
	EXPECT_THROW(ParsedOptions {}.parse("--nebulaevol 60"), CLI::ParseError);
	EXPECT_THROW(ParsedOptions {}.parse("--customseed 1"), CLI::ParseError);
}

TEST(SettingsCliTest, PcAnd3doOnly)
{
	EXPECT_THROW(ParsedOptions {}.parse("--intro None"), CLI::ParseError);
}

TEST(SettingsCliTest, StereoSfxRequiresOpenAL)
{
	EXPECT_THROW(ParsedOptions {}.parse("--stereosfx"), CLI::ParseError);

	ParsedOptions parsed {};
	parsed.parse("--stereosfx --sounddriver OpenAL");
	EXPECT_TRUE(*parsed.options.stereoSFX);
}

TEST(SettingsCliTest, GodModeFlagsCombine)
{
	ParsedOptions parsed {};
	parsed.parse("--godmode InfiniteBattery NoDamage");
	EXPECT_EQ(*parsed.options.optGodModes, GodModeFlags::InfiniteBattery | GodModeFlags::NoDamage);
	EXPECT_TRUE(parsed.options.optGodModes.hasCommandLineOverride());
}

TEST(SettingsCliTest, FuelRangeFlagsCombine)
{
	ParsedOptions parsed {};
	parsed.parse("--fuelrange FuelAtDestination FuelToSol");
	EXPECT_EQ(*parsed.options.optFuelRange, FuelRangeDisplay::FuelAtDestination | FuelRangeDisplay::FuelToSol);
	EXPECT_TRUE(parsed.options.optFuelRange.hasCommandLineOverride());
}

TEST(SettingsCliTest, UnknownOptionIsRejected)
{
	EXPECT_THROW(ParsedOptions {}.parse("--no-such-option"), CLI::ParseError);
}

} // namespace uqm
