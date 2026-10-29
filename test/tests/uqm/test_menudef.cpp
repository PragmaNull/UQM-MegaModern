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
	EXPECT_NE(definition->find("qol"), nullptr);
}

} // namespace uqm
