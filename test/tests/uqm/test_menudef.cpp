// Tests for the setup menu definition format.
#include <gtest/gtest.h>

#include "options/options.h"
#include "uqm/menudef.h"

#include <fstream>
#include <sstream>

namespace uqm
{

namespace
{

constexpr const char* c_sample {R"({
	"version": 1,
	"menus": [
		{
			"id": "qol",
			"subtitle": "Quality of Life Options",
			"background": 10,
			"items": [
				{"choice": "interface.skipIntro", "label": "Skip Intro", "options": [
					{"value": "false", "label": "Disabled", "tooltip": ["Line one.", "Line two."]},
					{"value": "true", "label": "Enabled"}
				]},
				{"spacer": true},
				{"back": true}
			]
		}
	]
})"};

} // namespace

TEST(MenuDefTest, ReadsMenusAndItems)
{
	const auto definition {menuDefinitionFromJson(c_sample)};
	ASSERT_TRUE(definition.has_value());
	ASSERT_EQ(definition->menus.size(), 1u);

	const MenuDef* menu {definition->find("qol")};
	ASSERT_NE(menu, nullptr);
	EXPECT_EQ(menu->subtitle, "Quality of Life Options");
	EXPECT_EQ(menu->background, 10);
	ASSERT_EQ(menu->items.size(), 3u);

	const MenuItemDef& choice {menu->items[0]};
	EXPECT_EQ(choice.kind, MenuItemKind::Choice);
	EXPECT_EQ(choice.setting, "interface.skipIntro");
	EXPECT_EQ(choice.label, "Skip Intro");
	ASSERT_EQ(choice.options.size(), 2u);
	EXPECT_EQ(choice.options[0].value, "false");
	EXPECT_EQ(choice.options[0].tooltip, (uqstl::vector<uqstl::string> {"Line one.", "Line two."}));
	EXPECT_TRUE(choice.options[1].tooltip.empty());

	EXPECT_EQ(menu->items[1].kind, MenuItemKind::Spacer);
	EXPECT_EQ(menu->items[2].kind, MenuItemKind::Back);
	EXPECT_EQ(definition->find("missing"), nullptr);
}

TEST(MenuDefTest, RejectsInvalidStructure)
{
	EXPECT_FALSE(menuDefinitionFromJson("{ not json"));
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": {}})"));
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"label": "x"}]}]})"));
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"choice": "k", "label": "x", "options": []}]}]})"));
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"choice": "k", "label": "x", "options": [{"value": "1", "label": "a", "tooltip": ["1", "2", "3", "4"]}]}]}]})"));
}

TEST(MenuDefTest, ValidationReportsUnknownSettingsAndValues)
{
	const auto definition {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"choice": "no.such.setting", "label": "x", "options": [{"value": "true", "label": "On"}]},
		{"choice": "interface.skipIntro", "label": "y", "options": [{"value": "maybe", "label": "?"}, {"value": "true", "label": "On"}]}
	]}]})")};
	ASSERT_TRUE(definition.has_value());

	const OptionsStruct options {};
	const auto errors {validateMenuDefinition(*definition, options)};
	ASSERT_EQ(errors.size(), 2u);
	EXPECT_NE(errors[0].find("no.such.setting"), uqstl::string::npos);
	EXPECT_NE(errors[1].find("maybe"), uqstl::string::npos);
}

TEST(MenuDefTest, ReadsHeadingsSlidersAndAddons)
{
	const auto definition {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"heading": ["Line one", "Line two"]},
		{"slider": "audio.musicVolume", "label": "Music", "min": 0, "max": 100, "step": 5, "scale": 100, "percent": true, "tooltip": ["Tip."]},
		{"choice": "audio.music3do", "label": "3DO", "options": [
			{"value": "false", "label": "Off"},
			{"value": "true", "label": "On", "requiresAddon": ["3domusic"]}
		]}
	]}]})")};
	ASSERT_TRUE(definition.has_value());
	const auto& items {definition->menus[0].items};
	ASSERT_EQ(items.size(), 3u);

	EXPECT_EQ(items[0].kind, MenuItemKind::Heading);
	EXPECT_EQ(items[0].lines, (uqstl::vector<uqstl::string> {"Line one", "Line two"}));

	EXPECT_EQ(items[1].kind, MenuItemKind::Slider);
	EXPECT_EQ(items[1].setting, "audio.musicVolume");
	EXPECT_EQ(items[1].max, 100);
	EXPECT_DOUBLE_EQ(items[1].scale, 100.0);
	EXPECT_TRUE(items[1].percent);
	EXPECT_EQ(items[1].tooltip, (uqstl::vector<uqstl::string> {"Tip."}));

	EXPECT_TRUE(items[2].options[0].requiresAddon.empty());
	EXPECT_EQ(items[2].options[1].requiresAddon, (uqstl::vector<uqstl::string> {"3domusic"}));
}

TEST(MenuDefTest, ReadsLegacyItems)
{
	const auto definition {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"legacy": "windowType"}]}]})")};
	ASSERT_TRUE(definition.has_value());
	EXPECT_EQ(definition->menus[0].items[0].kind, MenuItemKind::Legacy);
	EXPECT_EQ(definition->menus[0].items[0].widget, "windowType");
}

TEST(MenuDefTest, RejectsInvalidSliders)
{
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"slider": "k", "label": "x", "min": 10, "max": 5}]}]})"));
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"slider": "k", "label": "x", "step": 0}]}]})"));
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"slider": "k", "label": "x", "scale": 0}]}]})"));
	EXPECT_FALSE(menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [{"heading": []}]}]})"));
}

TEST(MenuDefTest, SliderValuesConvertWithScale)
{
	MenuItemDef volume {.kind = MenuItemKind::Slider, .scale = 100.0};
	EXPECT_EQ(sliderSettingText(volume, 50), "0.5");
	EXPECT_EQ(sliderDisplayedValue(volume, "0.75"), 75);

	MenuItemDef nebulae {.kind = MenuItemKind::Slider};
	EXPECT_EQ(sliderSettingText(nebulae, 16), "16");
	EXPECT_EQ(sliderDisplayedValue(nebulae, "16"), 16);
	EXPECT_FALSE(sliderDisplayedValue(nebulae, "abc"));
}

TEST(MenuDefTest, ValidationChecksSliderRange)
{
	const auto definition {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"slider": "visuals.nebulaeVolume", "label": "x", "min": 0, "max": 80}
	]}]})")};
	ASSERT_TRUE(definition.has_value());
	const OptionsStruct options {};
	const auto errors {validateMenuDefinition(*definition, options)};
	ASSERT_EQ(errors.size(), 1u);
	EXPECT_NE(errors[0].find("80"), uqstl::string::npos);
}

TEST(MenuDefTest, ContentMenuDefinitionIsValid)
{
	std::ifstream file {UQM_TEST_CONTENT_DIR "/base/ui/setupmenu.json", std::ios::binary};
	if (!file)
	{
		GTEST_SKIP() << "Content is not present.";
	}
	std::stringstream text {};
	text << file.rdbuf();

	const auto definition {menuDefinitionFromJson(text.str())};
	ASSERT_TRUE(definition.has_value());
	const OptionsStruct options {};
	EXPECT_EQ(validateMenuDefinition(*definition, options), uqstl::vector<uqstl::string> {});
	for (const char* id : {"engine", "advanced", "cheats", "music", "visual", "qol"})
	{
		EXPECT_NE(definition->find(id), nullptr) << id;
	}
}

} // namespace uqm
