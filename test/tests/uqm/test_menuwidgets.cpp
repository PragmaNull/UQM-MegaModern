// Tests for setup menu widgets that are bound to settings.
#include <gtest/gtest.h>

#include "options/options.h"
#include "uqm/menuwidgets.h"

#include <cstring>
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
int Widget_HandleEventTextEntry(WIDGET*, int)
{
	return 0;
}
void Widget_DrawTextEntry(WIDGET*, int, int) {}
void Widget_DrawButton(WIDGET*, int, int) {}
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

TEST_F(MenuWidgetsTest, TextShowsAndSetsTheSetting)
{
	const auto textMenu {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"text": "seed.value", "label": "Seed", "tooltip": ["Type a seed."]}
	]}]})")};
	ASSERT_TRUE(textMenu.has_value());
	options.customSeed = 12345;
	MenuWidgets::Hooks hooks {};
	hooks.settingChanged = [this](const SettingBase& setting) {
		changed.emplace_back(setting.key());
	};
	MenuWidgets builder {
		options, MenuWidgets::SharedWidgets {&spacer, &back},
		uqstl::move(hooks)
	  };
	WIDGET** list {builder.build(textMenu->menus[0])};
	ASSERT_EQ(list[0]->tag, WIDGET_TYPE_TEXTENTRY);
	auto& widget {*reinterpret_cast<WIDGET_TEXTENTRY*>(list[0])};
	EXPECT_STREQ(widget.category, "Seed");
	EXPECT_STREQ(widget.tooltip[0], "Type a seed.");
	EXPECT_STREQ(widget.value, "12345");

	std::strcpy(widget.value, "777");
	widget.onChange(&widget);
	EXPECT_EQ(*options.customSeed, 777);
	EXPECT_EQ(changed, (uqstl::vector<uqstl::string> {"seed.value"}));

	std::strcpy(widget.value, "abc");
	widget.onChange(&widget);
	EXPECT_EQ(*options.customSeed, 777);
	EXPECT_STREQ(widget.value, "777");
}

TEST_F(MenuWidgetsTest, ChoiceUsesSuppliedLabels)
{
	const auto labelMenu {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"choice": "controls.player1Template", "label": "Player", "labels": "templates", "options": [
			{"value": "0", "label": "Template 1"}, {"value": "1", "label": "Template 2"}
		]}
	]}]})")};
	ASSERT_TRUE(labelMenu.has_value());
	EXPECT_EQ(labelMenu->menus[0].items[0].labels, "templates");

	MenuWidgets::Hooks hooks {};
	hooks.optionLabel = [](uqstl::string_view list, size_t index) -> const char* {
		return list == "templates" && index == 0 ? "Arrows" : nullptr;
	};
	MenuWidgets builder {
		options, MenuWidgets::SharedWidgets {&spacer, &back},
		uqstl::move(hooks)
	  };
	WIDGET** list {builder.build(labelMenu->menus[0])};
	const WIDGET_CHOICE& widget {choice(list, 0)};
	EXPECT_STREQ(widget.options[0].optname, "Arrows");
	EXPECT_STREQ(widget.options[1].optname, "Template 2");
}

TEST_F(MenuWidgetsTest, MenuButtonOpensTheTarget)
{
	const auto buttonMenu {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"menu": "audio", "label": "Sound", "tooltip": ["Opens the sound menu."]},
		{"menu": "missing", "label": "Missing"},
		{"exit": true}
	]}]})")};
	ASSERT_TRUE(buttonMenu.has_value());

	uqstl::vector<uqstl::string> opened {};
	MenuWidgets::Hooks hooks {};
	hooks.openMenu = [&opened](uqstl::string_view id) {
		opened.emplace_back(id);
		return id == "audio";
	};
	WIDGET exit {};
	MenuWidgets::SharedWidgets shared {&spacer, &back, &exit};
	MenuWidgets builder {options, shared, uqstl::move(hooks)};
	WIDGET** list {builder.build(buttonMenu->menus[0])};
	ASSERT_EQ(list[0]->tag, WIDGET_TYPE_BUTTON);
	const auto& button {*reinterpret_cast<WIDGET_BUTTON*>(list[0])};
	EXPECT_STREQ(button.name, "Sound");
	EXPECT_STREQ(button.tooltip[0], "Opens the sound menu.");
	EXPECT_EQ(list[2], &exit);

	EXPECT_FALSE(list[0]->handleEvent(list[0], WIDGET_EVENT_DOWN));
	EXPECT_TRUE(opened.empty());
	EXPECT_TRUE(list[0]->handleEvent(list[0], WIDGET_EVENT_SELECT));
	EXPECT_FALSE(list[1]->handleEvent(list[1], WIDGET_EVENT_SELECT));
	EXPECT_EQ(opened, (uqstl::vector<uqstl::string> {"audio", "missing"}));
}

TEST_F(MenuWidgetsTest, RefreshShowsChangedSettings)
{
	WIDGET** list {build()};
	options.skipIntro = true;
	options.nebulaevol = 12;
	widgets->refresh();
	EXPECT_EQ(choice(list, c_skipIntro).selected, 1);
	EXPECT_EQ(slider(list, c_nebulae).value, 12);
}

TEST_F(MenuWidgetsTest, LegacyItemsUseTheGivenWidget)
{
	const auto legacyMenu {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"legacy": "windowType"}, {"legacy": "missing"}, {"back": true}
	]}]})")};
	ASSERT_TRUE(legacyMenu.has_value());

	WIDGET windowType {};
	MenuWidgets::SharedWidgets shared {&spacer, &back};
	shared.legacy["windowType"] = &windowType;
	MenuWidgets builder {options, shared, {}};
	WIDGET** list {builder.build(legacyMenu->menus[0])};
	EXPECT_EQ(list[0], &windowType);
	EXPECT_EQ(list[1], &back);
	EXPECT_EQ(list[2], nullptr);
}

TEST_F(MenuWidgetsTest, LegacyItemsGetTheirText)
{
	const auto legacyMenu {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"legacy": "gamma", "label": "Gamma", "tooltip": ["Sets the gamma."]}, {"legacy": "up", "label": "Up"}
	]}]})")};
	ASSERT_TRUE(legacyMenu.has_value());

	WIDGET_SLIDER gamma {};
	gamma.tag = WIDGET_TYPE_SLIDER;
	WIDGET_CONTROLENTRY up {};
	up.tag = WIDGET_TYPE_CONTROLENTRY;
	MenuWidgets::SharedWidgets shared {&spacer, &back};
	shared.legacy["gamma"] = reinterpret_cast<WIDGET*>(&gamma);
	shared.legacy["up"] = reinterpret_cast<WIDGET*>(&up);
	MenuWidgets builder {options, shared, {}};
	builder.build(legacyMenu->menus[0]);
	EXPECT_STREQ(gamma.category, "Gamma");
	EXPECT_STREQ(gamma.tooltip[0], "Sets the gamma.");
	EXPECT_STREQ(gamma.tooltip[1], "");
	EXPECT_STREQ(up.category, "Up");
}

TEST_F(MenuWidgetsTest, StateChoiceUsesTheStateHooks)
{
	const auto stateMenu {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"state": "device.0", "label": "Portal Spawner", "options": [{"label": "Do Nothing"}, {"label": "Remove"}, {"label": "Add"}]},
		{"state": "missing", "label": "Missing", "options": [{"label": "x"}]}
	]}]})")};
	ASSERT_TRUE(stateMenu.has_value());

	int device {2};
	uqstl::vector<int> stored {};
	MenuWidgets::Hooks hooks {};
	hooks.getState = [&device](uqstl::string_view key) -> uqstl::optional<int> {
		return key == "device.0" ? uqstl::optional<int> {device} : uqstl::nullopt;
	};
	hooks.setState = [&](uqstl::string_view key, int value) {
		EXPECT_EQ(key, "device.0");
		stored.push_back(value);
		device = value;
	};
	MenuWidgets builder {
		options, MenuWidgets::SharedWidgets {&spacer, &back},
		  uqstl::move(hooks)
	};
	WIDGET** list {builder.build(stateMenu->menus[0])};
	ASSERT_EQ(list[1], nullptr);
	WIDGET_CHOICE& widget {choice(list, 0)};
	EXPECT_EQ(widget.selected, 2);

	select(widget, 1);
	EXPECT_EQ(stored, (uqstl::vector<int> {1}));

	device = 0;
	builder.refresh();
	EXPECT_EQ(widget.selected, 0);
}

TEST_F(MenuWidgetsTest, ItemsNeedTheirAddons)
{
	const auto addonMenu {menuDefinitionFromJson(R"({"menus": [{"id": "a", "subtitle": "", "background": 0, "items": [
		{"spacer": true, "requiresAddon": ["missing"]}, {"back": true, "requiresAddon": ["present", "missing"]}
	]}]})")};
	ASSERT_TRUE(addonMenu.has_value());

	MenuWidgets::Hooks hooks {};
	hooks.isAddonAvailable = [](uqstl::string_view addon) {
		return addon == "present";
	};
	MenuWidgets builder {
		options, MenuWidgets::SharedWidgets {&spacer, &back},
		  uqstl::move(hooks)
	};
	WIDGET** list {builder.build(addonMenu->menus[0])};
	EXPECT_EQ(list[0], &back);
	EXPECT_EQ(list[1], nullptr);
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

	WIDGET exit {};
	MenuWidgets::SharedWidgets shared {&spacer, &back, &exit};
	uqstl::vector<uqstl::unique_ptr<WIDGET_TEXTENTRY>> legacyWidgets {};
	for (const MenuDef& menu : content->menus)
	{
		for (const MenuItemDef& item : menu.items)
		{
			if (item.kind == MenuItemKind::Legacy && !shared.legacy.contains(item.widget))
			{
				auto& widget {*legacyWidgets.emplace_back(uqstl::make_unique<WIDGET_TEXTENTRY>())};
				widget.tag = WIDGET_TYPE_TEXTENTRY;
				shared.legacy[item.widget] = reinterpret_cast<WIDGET*>(&widget);
			}
		}
	}
	MenuWidgets::Hooks hooks {};
	hooks.getState = [](uqstl::string_view) -> uqstl::optional<int> {
		return 0;
	};
	MenuWidgets builder {options, shared, uqstl::move(hooks)};
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
