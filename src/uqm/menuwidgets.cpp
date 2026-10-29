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
	MenuWidgets* owner {};
	SettingBase* setting {};
	MenuItemDef item {};
	uqstl::vector<CHOICE_OPTION> options {};
};

struct MenuWidgets::BoundSlider
{
	WIDGET_SLIDER widget {};
	MenuWidgets* owner {};
	SettingBase* setting {};
	MenuItemDef item {};
};

struct MenuWidgets::Heading
{
	WIDGET_LABEL widget {};
	MenuItemDef item {};
	uqstl::vector<const char*> lines {};
};

namespace
{

// The widget callbacks get only the widget, so the bindings are found by widget address.
uqstl::map<const void*, void*> s_bindings {};

void fillTooltip(const char* (&tooltip)[3], const uqstl::vector<uqstl::string>& lines)
{
	for (size_t line = 0; line < 3; ++line)
	{
		tooltip[line] = line < lines.size() ? lines[line].c_str() : "";
	}
}

} // namespace

MenuWidgets::MenuWidgets(SettingsRegistry& settings, SharedWidgets shared, Hooks hooks)
	: m_settings {settings}
	, m_shared {shared}
	, m_hooks {uqstl::move(hooks)}
{
}

MenuWidgets::~MenuWidgets()
{
	for (const auto& choice : m_choices)
	{
		s_bindings.erase(&choice->widget);
	}
	for (const auto& slider : m_sliders)
	{
		s_bindings.erase(&slider->widget);
	}
}

WIDGET** MenuWidgets::build(const MenuDef& menu)
{
	auto& list {*m_lists.emplace_back(uqstl::make_unique<uqstl::vector<WIDGET*>>())};
	for (const MenuItemDef& item : menu.items)
	{
		WIDGET* widget {};
		switch (item.kind)
		{
			case MenuItemKind::Spacer:
				widget = m_shared.spacer;
				break;
			case MenuItemKind::Back:
				widget = m_shared.back;
				break;
			case MenuItemKind::Legacy:
				if (const auto found {m_shared.legacy.find(item.widget)}; found != m_shared.legacy.end())
				{
					widget = found->second;
				}
				else if (!item.optional)
				{
					uqm::log::error("Menu '{}': unknown widget '{}'.", menu.id, item.widget);
				}
				break;
			case MenuItemKind::Heading:
				widget = buildHeading(item);
				break;
			case MenuItemKind::Choice:
				widget = buildChoice(menu, item);
				break;
			case MenuItemKind::Slider:
				widget = buildSlider(menu, item);
				break;
		}
		if (widget != nullptr)
		{
			list.push_back(widget);
		}
	}
	list.push_back(nullptr);
	return list.data();
}

WIDGET* MenuWidgets::buildChoice(const MenuDef& menu, const MenuItemDef& item)
{
	SettingBase* setting {m_settings.find(item.setting)};
	if (setting == nullptr)
	{
		uqm::log::error("Menu '{}': unknown setting '{}'.", menu.id, item.setting);
		return nullptr;
	}

	auto& choice {*m_choices.emplace_back(uqstl::make_unique<BoundChoice>())};
	choice.owner = this;
	choice.setting = setting;
	choice.item = item;
	for (const MenuOptionDef& option : choice.item.options)
	{
		CHOICE_OPTION widgetOption {option.label.c_str(), {}};
		fillTooltip(widgetOption.tooltip, option.tooltip);
		choice.options.push_back(widgetOption);
	}

	int selected {};
	for (size_t index = 0; index < choice.item.options.size(); ++index)
	{
		if (setting->hasValueText(choice.item.options[index].value))
		{
			selected = static_cast<int>(index);
			break;
		}
	}

	WIDGET_CHOICE& widget {choice.widget};
	widget.tag = WIDGET_TYPE_CHOICE;
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

	s_bindings[&widget] = &choice;
	return reinterpret_cast<WIDGET*>(&widget);
}

WIDGET* MenuWidgets::buildSlider(const MenuDef& menu, const MenuItemDef& item)
{
	SettingBase* setting {m_settings.find(item.setting)};
	if (setting == nullptr)
	{
		uqm::log::error("Menu '{}': unknown setting '{}'.", menu.id, item.setting);
		return nullptr;
	}

	auto& slider {*m_sliders.emplace_back(uqstl::make_unique<BoundSlider>())};
	slider.owner = this;
	slider.setting = setting;
	slider.item = item;

	WIDGET_SLIDER& widget {slider.widget};
	widget.tag = WIDGET_TYPE_SLIDER;
	widget.handleEvent = Widget_HandleEventSlider;
	widget.receiveFocus = Widget_ReceiveFocusSimple;
	widget.draw = Widget_DrawSlider;
	widget.height = Widget_HeightOneLine;
	widget.width = Widget_WidthFullScreen;
	widget.draw_value = slider.item.percent ? Widget_Slider_DrawPercentageValue : Widget_Slider_DrawValue;
	widget.min = slider.item.min;
	widget.max = slider.item.max;
	widget.step = slider.item.step;
	widget.value = uqstl::clamp(sliderDisplayedValue(slider.item, setting->valueText()).value_or(slider.item.min), slider.item.min, slider.item.max);
	widget.category = slider.item.label.c_str();
	fillTooltip(widget.tooltip, slider.item.tooltip);
	widget.onChange = onSliderChanged;

	s_bindings[&widget] = &slider;
	return reinterpret_cast<WIDGET*>(&widget);
}

WIDGET* MenuWidgets::buildHeading(const MenuItemDef& item)
{
	auto& heading {*m_headings.emplace_back(uqstl::make_unique<Heading>())};
	heading.item = item;
	for (const uqstl::string& line : heading.item.lines)
	{
		heading.lines.push_back(line.c_str());
	}

	WIDGET_LABEL& widget {heading.widget};
	widget.tag = WIDGET_TYPE_LABEL;
	widget.handleEvent = Widget_HandleEventIgnoreAll;
	widget.receiveFocus = Widget_ReceiveFocusRefuseFocus;
	widget.draw = Widget_DrawLabel;
	widget.height = Widget_HeightLabel;
	widget.width = Widget_WidthFullScreen;
	widget.line_count = static_cast<int>(heading.lines.size());
	widget.lines = heading.lines.data();
	return reinterpret_cast<WIDGET*>(&widget);
}

void MenuWidgets::applyChange(SettingBase& setting, uqstl::string_view text)
{
	const uqstl::string before {setting.valueText()};
	if (!setting.parseText(text))
	{
		uqm::log::error("Menu: '{}' is not a valid value for '{}'.", text, setting.key());
		return;
	}
	if (setting.valueText() != before && m_hooks.settingChanged)
	{
		m_hooks.settingChanged(setting);
	}
}

void MenuWidgets::onChoiceChanged(WIDGET_CHOICE* self, int oldValue)
{
	const auto found {s_bindings.find(self)};
	if (found == s_bindings.end())
	{
		return;
	}
	BoundChoice& choice {*static_cast<BoundChoice*>(found->second)};
	if (self->selected < 0 || self->selected >= static_cast<int>(choice.item.options.size()))
	{
		return;
	}

	const MenuOptionDef& option {choice.item.options[static_cast<size_t>(self->selected)]};
	const Hooks& hooks {choice.owner->m_hooks};
	if (!option.requiresAddon.empty() && hooks.isAddonAvailable)
	{
		const bool available {uqstl::any_of(option.requiresAddon.begin(), option.requiresAddon.end(), [&](const uqstl::string& addon) {
			return hooks.isAddonAvailable(addon);
		})};
		if (!available)
		{
			self->selected = oldValue;
			if (hooks.addonUnavailable)
			{
				hooks.addonUnavailable();
			}
			return;
		}
	}

	choice.owner->applyChange(*choice.setting, option.value);
}

void MenuWidgets::onSliderChanged(WIDGET_SLIDER* self)
{
	const auto found {s_bindings.find(self)};
	if (found == s_bindings.end())
	{
		return;
	}
	BoundSlider& slider {*static_cast<BoundSlider*>(found->second)};
	slider.owner->applyChange(*slider.setting, sliderSettingText(slider.item, self->value));
}

} // namespace uqm
