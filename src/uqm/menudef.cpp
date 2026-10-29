// Setup menu layout and text, read from the menu definition file.
#include "menudef.h"

#include "core/log/log.h"
#include "options/Setting.h"

#include <fmt/format.h>
#include <yyjson.h>

namespace uqm
{

namespace
{

constexpr size_t c_maxTooltipLines {3};

uqstl::optional<uqstl::string> getString(yyjson_val* obj, const char* name)
{
	yyjson_val* val {yyjson_obj_get(obj, name)};
	if (!yyjson_is_str(val))
	{
		return uqstl::nullopt;
	}
	return uqstl::string {yyjson_get_str(val), yyjson_get_len(val)};
}

uqstl::optional<MenuOptionDef> readOption(yyjson_val* optionObj)
{
	const auto value {getString(optionObj, "value")};
	const auto label {getString(optionObj, "label")};
	yyjson_val* tooltip {yyjson_obj_get(optionObj, "tooltip")};
	if (!value || !label || (tooltip != nullptr && !yyjson_is_arr(tooltip)) || yyjson_arr_size(tooltip) > c_maxTooltipLines)
	{
		return uqstl::nullopt;
	}

	MenuOptionDef option {*value, *label, {}};
	size_t index {};
	size_t count {};
	yyjson_val* line {};
	yyjson_arr_foreach(tooltip, index, count, line)
	{
		if (!yyjson_is_str(line))
		{
			return uqstl::nullopt;
		}
		option.tooltip.emplace_back(yyjson_get_str(line), yyjson_get_len(line));
	}
	return option;
}

uqstl::optional<MenuItemDef> readItem(yyjson_val* itemObj)
{
	if (!yyjson_is_obj(itemObj))
	{
		return uqstl::nullopt;
	}
	if (yyjson_is_true(yyjson_obj_get(itemObj, "spacer")))
	{
		return MenuItemDef {MenuItemKind::Spacer};
	}
	if (yyjson_is_true(yyjson_obj_get(itemObj, "back")))
	{
		return MenuItemDef {MenuItemKind::Back};
	}

	const auto setting {getString(itemObj, "choice")};
	const auto label {getString(itemObj, "label")};
	yyjson_val* options {yyjson_obj_get(itemObj, "options")};
	if (!setting || !label || !yyjson_is_arr(options) || yyjson_arr_size(options) == 0)
	{
		return uqstl::nullopt;
	}

	MenuItemDef item {MenuItemKind::Choice, *setting, *label, {}};
	size_t index {};
	size_t count {};
	yyjson_val* optionObj {};
	yyjson_arr_foreach(options, index, count, optionObj)
	{
		auto option {readOption(optionObj)};
		if (!option)
		{
			return uqstl::nullopt;
		}
		item.options.push_back(uqstl::move(*option));
	}
	return item;
}

uqstl::optional<MenuDef> readMenu(yyjson_val* menuObj)
{
	const auto id {getString(menuObj, "id")};
	const auto subtitle {getString(menuObj, "subtitle")};
	yyjson_val* background {yyjson_obj_get(menuObj, "background")};
	yyjson_val* items {yyjson_obj_get(menuObj, "items")};
	if (!id || !subtitle || !yyjson_is_uint(background) || !yyjson_is_arr(items))
	{
		return uqstl::nullopt;
	}

	MenuDef menu {*id, *subtitle, static_cast<int>(yyjson_get_uint(background)), {}};
	size_t index {};
	size_t count {};
	yyjson_val* itemObj {};
	yyjson_arr_foreach(items, index, count, itemObj)
	{
		auto item {readItem(itemObj)};
		if (!item)
		{
			return uqstl::nullopt;
		}
		menu.items.push_back(uqstl::move(*item));
	}
	return menu;
}

} // namespace

const MenuDef* MenuDefinition::find(uqstl::string_view id) const noexcept
{
	for (const MenuDef& menu : menus)
	{
		if (menu.id == id)
		{
			return &menu;
		}
	}
	return nullptr;
}

uqstl::optional<MenuDefinition> menuDefinitionFromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Menu definition: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<MenuDefinition> result {};
	yyjson_val* menus {yyjson_obj_get(yyjson_doc_get_root(doc), "menus")};
	if (yyjson_is_arr(menus))
	{
		result.emplace();
		size_t index {};
		size_t count {};
		yyjson_val* menuObj {};
		yyjson_arr_foreach(menus, index, count, menuObj)
		{
			auto menu {readMenu(menuObj)};
			if (!menu)
			{
				result.reset();
				break;
			}
			result->menus.push_back(uqstl::move(*menu));
		}
	}

	if (!result)
	{
		uqm::log::error("Menu definition: the file does not have the expected structure.");
	}
	yyjson_doc_free(doc);
	return result;
}

uqstl::vector<uqstl::string> validateMenuDefinition(const MenuDefinition& definition, const SettingsRegistry& settings)
{
	uqstl::vector<uqstl::string> errors {};
	for (const MenuDef& menu : definition.menus)
	{
		for (const MenuItemDef& item : menu.items)
		{
			if (item.kind != MenuItemKind::Choice)
			{
				continue;
			}
			const SettingBase* setting {settings.find(item.setting)};
			if (setting == nullptr)
			{
				errors.push_back(fmt::format("Menu '{}': unknown setting '{}'.", menu.id, item.setting));
				continue;
			}
			for (const MenuOptionDef& option : item.options)
			{
				if (!setting->acceptsText(option.value))
				{
					errors.push_back(fmt::format("Menu '{}': '{}' is not a valid value for '{}'.", menu.id, option.value, item.setting));
				}
			}
		}
	}
	return errors;
}

} // namespace uqm
