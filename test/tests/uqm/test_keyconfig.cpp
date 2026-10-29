// Tests for the key configuration JSON format.
#include <gtest/gtest.h>

#include "libs/input/keyconfig.h"

namespace uqm
{

namespace
{

constexpr uqgsl::czstring c_controls[] {"up", "down", "weapon"};

KeyTemplate makeTemplate(uqstl::string name, uqstl::vector<uqstl::vector<uqstl::string>> bindings)
{
	return KeyTemplate {uqstl::move(name), uqstl::move(bindings)};
}

} // namespace

TEST(KeyConfigTest, RoundTripsTemplates)
{
	const uqstl::vector<KeyTemplate> templates {
		makeTemplate("Arrows", {{"key Up"}, {"key Down", "joystick 0 axis 1 positive"}, {}			  }
		   ),
		makeTemplate("WASD", {{"key w"},	 {"key s"},									{"key space"}}
		  ),
	};

	const auto loaded {keyConfigFromJson(keyConfigToJson(templates, c_controls), c_controls)};
	ASSERT_TRUE(loaded.has_value());
	ASSERT_EQ(loaded->size(), 2u);
	EXPECT_EQ((*loaded)[0].name, "Arrows");
	EXPECT_EQ((*loaded)[0].bindings[1], (uqstl::vector<uqstl::string> {"key Down", "joystick 0 axis 1 positive"}));
	EXPECT_TRUE((*loaded)[0].bindings[2].empty());
	EXPECT_EQ((*loaded)[1].bindings[2], (uqstl::vector<uqstl::string> {"key space"}));
}

TEST(KeyConfigTest, WritesControlsByName)
{
	const uqstl::vector<KeyTemplate> templates {makeTemplate("Arrows", {{"key Up"}, {}, {}})};
	const uqstl::string json {keyConfigToJson(templates, c_controls)};
	EXPECT_NE(json.find("\"version\": 1"), uqstl::string::npos);
	EXPECT_NE(json.find("\"up\": ["), uqstl::string::npos) << json;
	EXPECT_NE(json.find("\"key Up\""), uqstl::string::npos) << json;
}

TEST(KeyConfigTest, MissingControlsHaveNoBindings)
{
	const auto loaded {keyConfigFromJson(R"({"templates": [{"name": "A", "controls": {"down": ["key x"]}}]})", c_controls)};
	ASSERT_TRUE(loaded.has_value());
	EXPECT_TRUE((*loaded)[0].bindings[0].empty());
	EXPECT_EQ((*loaded)[0].bindings[1], (uqstl::vector<uqstl::string> {"key x"}));
}

TEST(KeyConfigTest, IgnoresUnknownControls)
{
	const auto loaded {keyConfigFromJson(R"({"templates": [{"name": "A", "controls": {"jump": ["key j"]}}]})", c_controls)};
	ASSERT_TRUE(loaded.has_value());
	EXPECT_TRUE((*loaded)[0].bindings[0].empty());
}

TEST(KeyConfigTest, RejectsInvalidStructure)
{
	EXPECT_FALSE(keyConfigFromJson("{ not json", c_controls));
	EXPECT_FALSE(keyConfigFromJson(R"({"templates": {}})", c_controls));
	EXPECT_FALSE(keyConfigFromJson(R"({"templates": [{"controls": {}}]})", c_controls));
	EXPECT_FALSE(keyConfigFromJson(R"({"templates": [{"name": "A", "controls": {"up": "key Up"}}]})", c_controls));
	EXPECT_FALSE(keyConfigFromJson(R"({"templates": [{"name": "A", "controls": {"up": [1]}}]})", c_controls));
}

} // namespace uqm
