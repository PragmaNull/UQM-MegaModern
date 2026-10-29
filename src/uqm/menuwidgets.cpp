// Setup menu widgets built from a menu definition and bound to settings.
#include "menuwidgets.h"

#include "core/log/log.h"
#include "options/Setting.h"

#include <map>

namespace uqm
{

struct MenuWidgets::BoundChoice
{
	WIDGET_CHOICE widget {};
	SettingBase* setting {};
	MenuItemDef item {};
	uqstl::vector<CHOICE_OPTION> options {};
};

namespace
{

uqstl::map<const WIDGET_CHOICE*, SettingBase*> s_settingsByWidget {};
uqstl::map<const WIDGET_CHOICE*, const MenuItemDef*> s_itemsByWidget {};

} // namespace

MenuWidgets::MenuWidgets(SettingsRegistry& settings, SharedWidgets shared)
	: m_settings {settings}
	, m_shared {shared}
{
}

MenuWidgets::~MenuWidgets()
{
	for (const auto& choice : m_choices)
	{
		s_settingsByWidget.erase(&choice->widget);
		s_itemsByWidget.erase(&choice->widget);
	}
}

WIDGET** MenuWidgets::build(const MenuDef& menu)
{
	auto& list {*m_lists.emplace_back(uqstl::make_unique<uqstl::vector<WIDGET*>>())};
	for (const MenuItemDef& item : menu.items)
	{
		switch (item.kind)
		{
			case MenuItemKind::Spacer:
				list.push_back(m_shared.spacer);
				continue;
			case MenuItemKind::Back:
				list.push_back(m_shared.back);
				continue;
			case MenuItemKind::Choice:
				break;
		}

		SettingBase* setting {m_settings.find(item.setting)};
		if (setting == nullptr)
		{
			uqm::log::error("Menu '{}': unknown setting '{}'.", menu.id, item.setting);
			continue;
		}

		auto& choice {*m_choices.emplace_back(uqstl::make_unique<BoundChoice>())};
		choice.setting = setting;
		choice.item = item;
		for (const MenuOptionDef& option : choice.item.options)
		{
			CHOICE_OPTION widgetOption {
				option.label.c_str(), {"", "", ""}
			  };
			for (size_t line = 0; line < option.tooltip.size() && line < 3; ++line)
			{
				widgetOption.tooltip[line] = option.tooltip[line].c_str();
			}
			choice.options.push_back(widgetOption);
		}

		const uqstl::string current {setting->valueText()};
		int selected {};
		for (size_t index = 0; index < choice.item.options.size(); ++index)
		{
			if (choice.item.options[index].value == current)
			{
				selected = static_cast<int>(index);
			}
		}

		WIDGET_CHOICE& widget {choice.widget};
		widget.tag = WIDGET_TYPE_CHOICE;
		widget.parent = nullptr;
		widget.handleEvent = Widget_HandleEventChoice;
		widget.receiveFocus = Widget_ReceiveFocusChoice;
		widget.draw = Widget_DrawChoice;
		widget.height = Widget_HeightChoice;
		widget.width = Widget_WidthFullScreen;
		widget.category = choice.item.label.c_str();
		widget.numopts = static_cast<int>(choice.options.size());
		widget.options = choice.options.data();
		widget.selected = selected;
		widget.highlighted = selected;
		widget.choice_num = -1;
		widget.onChange = onChoiceChanged;

		s_settingsByWidget[&widget] = setting;
		s_itemsByWidget[&widget] = &choice.item;
		list.push_back(reinterpret_cast<WIDGET*>(&widget));
	}
	list.push_back(nullptr);
	return list.data();
}

void MenuWidgets::onChoiceChanged(WIDGET_CHOICE* self, int oldValue)
{
	const auto setting {s_settingsByWidget.find(self)};
	const auto item {s_itemsByWidget.find(self)};
	if (setting == s_settingsByWidget.end() || item == s_itemsByWidget.end() || self->selected < 0
		|| self->selected >= static_cast<int>(item->second->options.size()))
	{
		return;
	}

	const uqstl::string& value {item->second->options[static_cast<size_t>(self->selected)].value};
	if (!setting->second->parseText(value))
	{
		uqm::log::error("Menu: '{}' is not a valid value for '{}'.", value, setting->second->key());
		self->selected = oldValue;
	}
}

} // namespace uqm
