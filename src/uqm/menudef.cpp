// Setup menu layout and text, read from the menu definition file.
#include "menudef.h"

#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "options/Setting.h"

#include <fmt/format.h>
#include <yyjson.h>

#include <cmath>

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

// A missing array gives an empty list.
uqstl::optional<uqstl::vector<uqstl::string>> getStringArray(yyjson_val* obj, const char* name, size_t maxCount = SIZE_MAX)
{
	yyjson_val* arr {yyjson_obj_get(obj, name)};
	if (arr == nullptr)
	{
		return uqstl::vector<uqstl::string> {};
	}
	if (!yyjson_is_arr(arr) || yyjson_arr_size(arr) > maxCount)
	{
		return uqstl::nullopt;
	}

	uqstl::vector<uqstl::string> result {};
	size_t index {};
	size_t count {};
	yyjson_val* line {};
	yyjson_arr_foreach(arr, index, count, line)
	{
		if (!yyjson_is_str(line))
		{
			return uqstl::nullopt;
		}
		result.emplace_back(yyjson_get_str(line), yyjson_get_len(line));
	}
	return result;
}

bool getInt(yyjson_val* obj, const char* name, int& out)
{
	yyjson_val* val {yyjson_obj_get(obj, name)};
	if (val == nullptr)
	{
		return true;
	}
	if (!yyjson_is_int(val))
	{
		return false;
	}
	out = static_cast<int>(yyjson_get_sint(val));
	return true;
}

uqstl::optional<MenuOptionDef> readOption(yyjson_val* optionObj)
{
	const auto value {getString(optionObj, "value")};
	const auto label {getString(optionObj, "label")};
	auto tooltip {getStringArray(optionObj, "tooltip", c_maxTooltipLines)};
	auto requiresAddon {getStringArray(optionObj, "requiresAddon")};
	if (!value || !label || !tooltip || !requiresAddon)
	{
		return uqstl::nullopt;
	}
	return MenuOptionDef {*value, *label, uqstl::move(*tooltip), uqstl::move(*requiresAddon)};
}

uqstl::optional<MenuItemDef> readChoice(yyjson_val* itemObj, uqstl::string setting)
{
	const auto label {getString(itemObj, "label")};
	yyjson_val* options {yyjson_obj_get(itemObj, "options")};
	if (!label || !yyjson_is_arr(options) || yyjson_arr_size(options) == 0)
	{
		return uqstl::nullopt;
	}

	MenuItemDef item {.kind = MenuItemKind::Choice, .setting = uqstl::move(setting), .label = *label};
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

uqstl::optional<MenuItemDef> readSlider(yyjson_val* itemObj, uqstl::string setting)
{
	const auto label {getString(itemObj, "label")};
	auto tooltip {getStringArray(itemObj, "tooltip", c_maxTooltipLines)};
	if (!label || !tooltip)
	{
		return uqstl::nullopt;
	}

	MenuItemDef item {.kind = MenuItemKind::Slider, .setting = uqstl::move(setting), .label = *label, .tooltip = uqstl::move(*tooltip)};
	if (!getInt(itemObj, "min", item.min) || !getInt(itemObj, "max", item.max) || !getInt(itemObj, "step", item.step))
	{
		return uqstl::nullopt;
	}
	if (yyjson_val * scale {yyjson_obj_get(itemObj, "scale")})
	{
		if (!yyjson_is_num(scale) || yyjson_get_num(scale) <= 0.0)
		{
			return uqstl::nullopt;
		}
		item.scale = yyjson_get_num(scale);
	}
	item.percent = yyjson_is_true(yyjson_obj_get(itemObj, "percent"));
	if (item.min >= item.max || item.step <= 0)
	{
		return uqstl::nullopt;
	}
	return item;
}

uqstl::optional<MenuItemDef> readItem(yyjson_val* itemObj)
{
	if (!yyjson_is_obj(itemObj))
	{
		return uqstl::nullopt;
	}
	if (yyjson_is_true(yyjson_obj_get(itemObj, "spacer")))
	{
		return MenuItemDef {.kind = MenuItemKind::Spacer};
	}
	if (yyjson_is_true(yyjson_obj_get(itemObj, "back")))
	{
		return MenuItemDef {.kind = MenuItemKind::Back};
	}
	if (yyjson_obj_get(itemObj, "heading") != nullptr)
	{
		auto lines {getStringArray(itemObj, "heading")};
		if (!lines || lines->empty())
		{
			return uqstl::nullopt;
		}
		return MenuItemDef {.kind = MenuItemKind::Heading, .lines = uqstl::move(*lines)};
	}
	if (auto setting {getString(itemObj, "choice")})
	{
		return readChoice(itemObj, uqstl::move(*setting));
	}
	if (auto setting {getString(itemObj, "slider")})
	{
		return readSlider(itemObj, uqstl::move(*setting));
	}
	return uqstl::nullopt;
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
			uqm::log::error("Menu definition: item {} of menu '{}' is not valid.", index, *id);
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

uqstl::string sliderSettingText(const MenuItemDef& slider, int displayed)
{
	if (slider.scale == 1.0)
	{
		return fmt::format("{}", displayed);
	}
	return fmt::format("{}", displayed / slider.scale);
}

uqstl::optional<int> sliderDisplayedValue(const MenuItemDef& slider, uqstl::string_view settingText)
{
	double value {};
	if (parseStr(settingText, value) != uqstl::errc {})
	{
		return uqstl::nullopt;
	}
	return static_cast<int>(std::lround(value * slider.scale));
}

uqstl::vector<uqstl::string> validateMenuDefinition(const MenuDefinition& definition, const SettingsRegistry& settings)
{
	uqstl::vector<uqstl::string> errors {};
	for (const MenuDef& menu : definition.menus)
	{
		for (const MenuItemDef& item : menu.items)
		{
			if (item.kind != MenuItemKind::Choice && item.kind != MenuItemKind::Slider)
			{
				continue;
			}
			const SettingBase* setting {settings.find(item.setting)};
			if (setting == nullptr)
			{
				errors.push_back(fmt::format("Menu '{}': unknown setting '{}'.", menu.id, item.setting));
				continue;
			}

			if (item.kind == MenuItemKind::Slider)
			{
				for (const int displayed : {item.min, item.max})
				{
					if (!setting->acceptsText(sliderSettingText(item, displayed)))
					{
						errors.push_back(fmt::format("Menu '{}': slider value {} is not valid for '{}'.", menu.id, displayed, item.setting));
					}
				}
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
