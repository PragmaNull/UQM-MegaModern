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
};

enum class MenuItemKind
{
	Choice,
	Spacer,
	Back,
};

struct MenuItemDef
{
	MenuItemKind kind {MenuItemKind::Spacer};
	// Setting key for a Choice.
	uqstl::string setting {};
	uqstl::string label {};
	uqstl::vector<MenuOptionDef> options {};
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

// Returns one message for each choice that refers to an unknown setting or has an invalid option value.
[[nodiscard]] uqstl::vector<uqstl::string> validateMenuDefinition(const MenuDefinition& definition, const SettingsRegistry& settings);

} // namespace uqm

#endif // UQM_MENUDEF_H_
