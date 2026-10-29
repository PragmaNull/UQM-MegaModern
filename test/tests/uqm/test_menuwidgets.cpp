// Tests for setup menu widgets that are bound to settings.
#include <gtest/gtest.h>

#include "options/options.h"
#include "uqm/menuwidgets.h"

#include <fstream>
#include <sstream>

// Widget callbacks from the graphics library. These tests do not draw or handle input.
int Widget_HandleEventChoice(WIDGET*, int)
{
	return 0;
}
int Widget_ReceiveFocusChoice(WIDGET*, int)
{
	return 0;
}
void Widget_DrawChoice(WIDGET*, int, int) {}
int Widget_HeightChoice(WIDGET*)
{
	return 0;
}
int Widget_WidthFullScreen(WIDGET*)
{
	return 0;
}
int Widget_HandleEventSlider(WIDGET*, int)
{
	return 0;
}
int Widget_ReceiveFocusSimple(WIDGET*, int)
{
	return 0;
}
void Widget_DrawSlider(WIDGET*, int, int) {}
int Widget_HeightOneLine(WIDGET*)
{
	return 0;
}
void Widget_Slider_DrawValue(WIDGET_SLIDER*, int, int) {}
void Widget_Slider_DrawPercentageValue(WIDGET_SLIDER*, int, int) {}
int Widget_HandleEventIgnoreAll(WIDGET*, int)
{
	return 0;
}
int Widget_ReceiveFocusRefuseFocus(WIDGET*, int)
{
	return 0;
}
void Widget_DrawLabel(WIDGET*, int, int) {}
int Widget_HeightLabel(WIDGET*)
{
	return 0;
}

namespace uqm
{

namespace
{

constexpr const char* c_menu {R"({"menus": [{"id": "test", "subtitle": "Test", "background": 0, "items": [
	{"heading": ["Section"]},
	{"choice": "interface.skipIntro", "label": "Skip Intro", "options": [
		{"value": "false", "label": "Disabled"},
		{"value": "true", "label": "Enabled", "tooltip": ["Skips the intro."]}
	]},
	{"choice": "audio.music3do", "label": "3DO Music", "options": [
		{"value": "false", "label": "Disabled"},
		{"value": "true", "label": "Enabled", "requiresAddon": ["3domusic"]}
	]},
	{"choice": "visuals.sphereColors", "label": "Sphere Colors", "options": [
		{"value": "Default", "label": "Default"},
		{"value": "1", "label": "Custom"}
	]},
	{"slider": "audio.musicVolume", "label": "Music", "min": 0, "max": 100, "step": 5, "scale": 100},
	{"slider": "visuals.nebulaeVolume", "label": "Nebulae", "min": 0, "max": 50, "step": 1},
	{"choice": "no.such.setting", "label": "Missing", "options": [{"value": "x", "label": "x"}]},
	{"spacer": true},
	{"back": true}
]}]})"};

class MenuWidgetsTest : public testing::Test
{
protected:
	void SetUp() override
	{
		const auto parsed {menuDefinitionFromJson(c_menu)};
		ASSERT_TRUE(parsed.has_value());
		definition = *parsed;
	}

	WIDGET** build()
	{
		MenuWidgets::Hooks hooks {};
		hooks.isAddonAvailable = [this](uqstl::string_view addon) {
			return uqstl::find(availableAddons.begin(), availableAddons.end(), addon) != availableAddons.end();
		};
		hooks.addonUnavailable = [this] {
			++unavailableCount;
		};
		hooks.settingChanged = [this](const SettingBase& setting) {
			changed.emplace_back(setting.key());
		};
		widgets = uqstl::make_unique<MenuWidgets>(options, MenuWidgets::SharedWidgets {&spacer, &back}, uqstl::move(hooks));
		return widgets->build(definition.menus[0]);
	}

	static WIDGET_CHOICE& choice(WIDGET** list, size_t index)
	{
		EXPECT_EQ(list[index]->tag, WIDGET_TYPE_CHOICE);
		return *reinterpret_cast<WIDGET_CHOICE*>(list[index]);
	}

	static WIDGET_SLIDER& slider(WIDGET** list, size_t index)
	{
		EXPECT_EQ(list[index]->tag, WIDGET_TYPE_SLIDER);
		return *reinterpret_cast<WIDGET_SLIDER*>(list[index]);
	}

	// Does what the widget system does when the player picks an option.
	static void select(WIDGET_CHOICE& widget, int option)
	{
		const int oldValue {widget.selected};
		widget.selected = option;
		widget.onChange(&widget, oldValue);
	}

	static void move(WIDGET_SLIDER& widget, int value)
	{
		widget.value = value;
		widget.onChange(&widget);
	}

	OptionsStruct options {};
	MenuDefinition definition {};
	WIDGET spacer {};
	WIDGET back {};
	uqstl::vector<uqstl::string> availableAddons {};
	int unavailableCount {};
	uqstl::vector<uqstl::string> changed {};
	uqstl::unique_ptr<MenuWidgets> widgets {};
};

// Widget indices in c_menu. The item with an unknown setting is left out.
constexpr size_t c_heading {0};
constexpr size_t c_skipIntro {1};
constexpr size_t c_music3do {2};
constexpr size_t c_sphereColors {3};
constexpr size_t c_musicVolume {4};
constexpr size_t c_nebulae {5};
constexpr size_t c_spacer {6};
constexpr size_t c_back {7};
constexpr size_t c_end {8};

} // namespace

TEST_F(MenuWidgetsTest, BuildsWidgetsInOrder)
{
	WIDGET** list {build()};
	EXPECT_EQ(list[c_heading]->tag, WIDGET_TYPE_LABEL);
	EXPECT_EQ(list[c_spacer], &spacer);
	EXPECT_EQ(list[c_back], &back);
	EXPECT_EQ(list[c_end], nullptr);

	const auto& heading {*reinterpret_cast<WIDGET_LABEL*>(list[c_heading])};
	ASSERT_EQ(heading.line_count, 1);
	EXPECT_STREQ(heading.lines[0], "Section");
}

TEST_F(MenuWidgetsTest, ChoiceShowsLabelsAndTooltips)
{
	WIDGET** list {build()};
	const WIDGET_CHOICE& widget {choice(list, c_skipIntro)};
	EXPECT_STREQ(widget.category, "Skip Intro");
	ASSERT_EQ(widget.numopts, 2);
	EXPECT_STREQ(widget.options[1].optname, "Enabled");
	EXPECT_STREQ(widget.options[1].tooltip[0], "Skips the intro.");
	EXPECT_STREQ(widget.options[1].tooltip[1], "");
}

TEST_F(MenuWidgetsTest, ChoiceSelectsTheCurrentValue)
{
	options.skipIntro = true;
	options.sphereColors = SphereOfInfluenceColors::Starseed;
	WIDGET** list {build()};
	EXPECT_EQ(choice(list, c_skipIntro).selected, 1);
	EXPECT_EQ(choice(list, c_sphereColors).selected, 1);
}

TEST_F(MenuWidgetsTest, ChoiceChangeSetsTheSetting)
{
	WIDGET** list {build()};
	select(choice(list, c_skipIntro), 1);
	EXPECT_TRUE(*options.skipIntro);
	EXPECT_EQ(changed, (uqstl::vector<uqstl::string> {"interface.skipIntro"}));

	select(choice(list, c_sphereColors), 1);
	EXPECT_EQ(*options.sphereColors, SphereOfInfluenceColors::Starseed);
}

TEST_F(MenuWidgetsTest, NoChangeHookWhenTheValueStaysTheSame)
{
	WIDGET** list {build()};
	select(choice(list, c_skipIntro), 0);
	EXPECT_TRUE(changed.empty());
}

TEST_F(MenuWidgetsTest, MissingAddonRefusesTheOption)
{
	options.use3doMusic = false;
	WIDGET** list {build()};
	WIDGET_CHOICE& widget {choice(list, c_music3do)};
	select(widget, 1);
	EXPECT_EQ(widget.selected, 0);
	EXPECT_FALSE(*options.use3doMusic);
	EXPECT_EQ(unavailableCount, 1);
	EXPECT_TRUE(changed.empty());
}

TEST_F(MenuWidgetsTest, AvailableAddonAllowsTheOption)
{
	options.use3doMusic = false;
	availableAddons.emplace_back("3domusic");
	WIDGET** list {build()};
	select(choice(list, c_music3do), 1);
	EXPECT_TRUE(*options.use3doMusic);
	EXPECT_EQ(unavailableCount, 0);
	EXPECT_EQ(changed, (uqstl::vector<uqstl::string> {"audio.music3do"}));
}

TEST_F(MenuWidgetsTest, SliderShowsTheScaledValue)
{
	options.musicVolumeScale = 0.4f;
	options.nebulaevol = 20;
	WIDGET** list {build()};
	EXPECT_EQ(slider(list, c_musicVolume).value, 40);
	EXPECT_EQ(slider(list, c_musicVolume).max, 100);
	EXPECT_EQ(slider(list, c_nebulae).value, 20);
	EXPECT_EQ(slider(list, c_nebulae).step, 1);
}

TEST_F(MenuWidgetsTest, SliderChangeSetsTheSetting)
{
	WIDGET** list {build()};
	move(slider(list, c_musicVolume), 55);
	EXPECT_FLOAT_EQ(*options.musicVolumeScale, 0.55f);
	move(slider(list, c_nebulae), 33);
	EXPECT_EQ(*options.nebulaevol, 33);
	EXPECT_EQ(changed, (uqstl::vector<uqstl::string> {"audio.musicVolume", "visuals.nebulaeVolume"}));
}

TEST_F(MenuWidgetsTest, ContentMenusBuild)
{
	std::ifstream file {UQM_TEST_CONTENT_DIR "/base/ui/setupmenu.json", std::ios::binary};
	if (!file)
	{
		GTEST_SKIP() << "Content is not present.";
	}
	std::stringstream text {};
	text << file.rdbuf();
	const auto content {menuDefinitionFromJson(text.str())};
	ASSERT_TRUE(content.has_value());

	MenuWidgets builder {
		options, {&spacer, &back},
		   {}
	 };
	for (const MenuDef& menu : content->menus)
	{
		WIDGET** list {builder.build(menu)};
		size_t count {};
		while (list[count] != nullptr)
		{
			++count;
		}
		EXPECT_EQ(count, menu.items.size()) << menu.id;
	}
}

} // namespace uqm
