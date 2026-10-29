// Setup menu layout and text, read from the menu definition file.
#pragma once
#ifndef UQM_MENUDEF_H_
#define UQM_MENUDEF_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

#include <optional>

namespace uqm
{

class SettingsRegistry;

struct MenuOptionDef
{
	// Setting value in command-line text form, e.g. "true" or "3DO".
	uqstl::string value {};
	uqstl::string label {};
	// At most three lines.
	uqstl::vector<uqstl::string> tooltip {};
	// The option needs at least one of these addons. Empty if it needs none.
	uqstl::vector<uqstl::string> requiresAddon {};
};

enum class MenuItemKind
{
	Choice,
	Slider,
	Text,
	Heading,
	Spacer,
	Back,
	// An existing widget that the setup menu builds itself.
	Legacy,
};

struct MenuItemDef
{
	MenuItemKind kind {MenuItemKind::Spacer};
	// Setting key for a Choice, Slider or Text.
	uqstl::string setting {};
	uqstl::string label {};
	uqstl::vector<MenuOptionDef> options {};
	// Name of a list that the game supplies at run time. Its entries replace the option labels of a Choice.
	uqstl::string labels {};

	// Slider range in displayed units. The setting value is the displayed value divided by 'scale'.
	int min {0};
	int max {100};
	int step {5};
	double scale {1.0};
	bool percent {};
	// Tooltip of a Slider or Text. At most three lines.
	uqstl::vector<uqstl::string> tooltip {};

	// Text of a Heading.
	uqstl::vector<uqstl::string> lines {};

	// Name of the widget for a Legacy item.
	uqstl::string widget {};
	// A Legacy item whose widget is missing in this build is left out without an error.
	bool optional {};
};

struct MenuDef
{
	uqstl::string id {};
	uqstl::string subtitle {};
	int background {};
	uqstl::vector<MenuItemDef> items {};
};

struct MenuDefinition
{
	uqstl::vector<MenuDef> menus {};

	[[nodiscard]] const MenuDef* find(uqstl::string_view id) const noexcept;
};

// Returns nullopt if 'json' is not a valid menu definition.
[[nodiscard]] uqstl::optional<MenuDefinition> menuDefinitionFromJson(uqstl::string_view json);

// Converts a slider position to the setting value in text form.
[[nodiscard]] uqstl::string sliderSettingText(const MenuItemDef& slider, int displayed);

// Converts a setting value in text form to a slider position.
[[nodiscard]] uqstl::optional<int> sliderDisplayedValue(const MenuItemDef& slider, uqstl::string_view settingText);

// Returns one message for each item that refers to an unknown setting or has an invalid value.
[[nodiscard]] uqstl::vector<uqstl::string> validateMenuDefinition(const MenuDefinition& definition, const SettingsRegistry& settings);

} // namespace uqm

#endif // UQM_MENUDEF_H_
