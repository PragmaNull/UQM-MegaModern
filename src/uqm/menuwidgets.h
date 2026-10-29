// Setup menu widgets built from a menu definition and bound to settings.
#pragma once
#ifndef UQM_MENUWIDGETS_H_
#define UQM_MENUWIDGETS_H_

#include "core/stl/stl.h"
#include "core/stl/vector.h"
#include "libs/graphics/widgets.h"
#include "uqm/menudef.h"

#include <memory>

namespace uqm
{

class SettingsRegistry;

class MenuWidgets
{
public:
	// Widgets that many menus share.
	struct SharedWidgets
	{
		WIDGET* spacer {};
		WIDGET* back {};
	};

	MenuWidgets(SettingsRegistry& settings, SharedWidgets shared);
	~MenuWidgets();

	MenuWidgets(const MenuWidgets&) = delete;
	MenuWidgets& operator=(const MenuWidgets&) = delete;

	// Returns the null-terminated widget list for 'menu'. It stays valid until this object is destroyed.
	// A choice selects the option that matches its setting, and sets the setting when it changes.
	WIDGET** build(const MenuDef& menu);

private:
	struct BoundChoice;

	static void onChoiceChanged(WIDGET_CHOICE* self, int oldValue);

	SettingsRegistry& m_settings;
	SharedWidgets m_shared {};
	uqstl::vector<uqstl::unique_ptr<BoundChoice>> m_choices {};
	uqstl::vector<uqstl::unique_ptr<uqstl::vector<WIDGET*>>> m_lists {};
};

} // namespace uqm

#endif // UQM_MENUWIDGETS_H_
