// Setup menu widgets built from a menu definition and bound to settings.
#pragma once
#ifndef UQM_MENUWIDGETS_H_
#define UQM_MENUWIDGETS_H_

#include "core/stl/stl.h"
#include "core/stl/vector.h"
#include "libs/graphics/widgets.h"
#include "uqm/menudef.h"

#include <functional>
#include <map>
#include <memory>

namespace uqm
{

class SettingBase;
class SettingsRegistry;

class MenuWidgets
{
public:
	// Widgets that many menus share.
	struct SharedWidgets
	{
		WIDGET* spacer {};
		WIDGET* back {};
		WIDGET* exit {};
		// Widgets for Legacy items, by name.
		uqstl::map<uqstl::string, WIDGET*> legacy {};
	};

	struct Hooks
	{
		std::function<bool(uqstl::string_view addon)> isAddonAvailable {};
		// Called after a choice is refused because its addon is missing.
		std::function<void()> addonUnavailable {};
		// Called after a widget changes a setting.
		std::function<void(const SettingBase& setting)> settingChanged {};
		// Gives the label of an option from a list that the game supplies. Returns nullptr if there is no label.
		std::function<const char*(uqstl::string_view list, size_t index)> optionLabel {};
		// Opens the menu with this id. Returns false if there is no such menu.
		std::function<bool(uqstl::string_view id)> openMenu {};
		// Edits the text of a Text item when the player selects it.
		int (*textEntrySelect)(WIDGET_TEXTENTRY* self) {};
	};

	MenuWidgets(SettingsRegistry& settings, SharedWidgets shared, Hooks hooks);
	~MenuWidgets();

	MenuWidgets(const MenuWidgets&) = delete;
	MenuWidgets& operator=(const MenuWidgets&) = delete;

	// Returns the null-terminated widget list for 'menu'. It stays valid until this object is destroyed.
	// Choices and sliders show the value of their setting, and set the setting when they change.
	WIDGET** build(const MenuDef& menu);

	// Shows the current setting values again, for example after a hook changed a setting.
	void refresh();

private:
	struct BoundChoice;
	struct BoundSlider;
	struct BoundText;
	struct MenuButton;
	struct Heading;

	static void onChoiceChanged(WIDGET_CHOICE* self, int oldValue);
	static void onSliderChanged(WIDGET_SLIDER* self);
	static void onTextChanged(WIDGET_TEXTENTRY* self);
	static int onMenuButtonEvent(WIDGET* self, int event);

	WIDGET* buildChoice(const MenuDef& menu, const MenuItemDef& item);
	WIDGET* buildSlider(const MenuDef& menu, const MenuItemDef& item);
	WIDGET* buildText(const MenuDef& menu, const MenuItemDef& item);
	WIDGET* buildMenuButton(const MenuItemDef& item);
	WIDGET* buildHeading(const MenuItemDef& item);
	void applyChange(SettingBase& setting, uqstl::string_view text);

	SettingsRegistry& m_settings;
	SharedWidgets m_shared {};
	Hooks m_hooks {};
	uqstl::vector<uqstl::unique_ptr<BoundChoice>> m_choices {};
	uqstl::vector<uqstl::unique_ptr<BoundSlider>> m_sliders {};
	uqstl::vector<uqstl::unique_ptr<BoundText>> m_texts {};
	uqstl::vector<uqstl::unique_ptr<MenuButton>> m_menuButtons {};
	uqstl::vector<uqstl::unique_ptr<Heading>> m_headings {};
	uqstl::vector<uqstl::unique_ptr<uqstl::vector<WIDGET*>>> m_lists {};
};

} // namespace uqm

#endif // UQM_MENUWIDGETS_H_
