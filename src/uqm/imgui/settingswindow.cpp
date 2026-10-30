// ImGui windows that edit the settings and the cheats.

#include "settingswindow.h"

#include <imgui.h>
#include "options.h"
#include "core/log/log.h"
#include "core/stl/functional.h"
#include "core/stl/memory.h"
#include "core/stl/set.h"
#include "core/string/StringUtils.h"
#include "core/thread/Mutex.h"
#include "configuration/Configuration.h"
#include "libs/callback/callback.h"
#include "libs/graphics/imgui/toolwindows.h"
#include "uqm/menudef.h"
#include "uqm/setupsettings.h"

namespace uqm
{

namespace
{

constexpr int c_maxMenuDepth {4};

// The menu definition, which the game thread reads.
struct LoadedDefinition
{
	Mutex lock {};
	uqstl::optional<MenuDefinition> definition UQM_GUARDED_BY(lock) {};
	bool done UQM_GUARDED_BY(lock) {};
};

LoadedDefinition s_loaded {};
uqstl::optional<MenuDefinition> s_definition {};
bool s_loadRequested {};
bool s_loadFailed {};

// Runs the task on the game thread the next time that it processes callbacks.
void runOnGameThread(uqstl::function<void()> task)
{
	auto owned {uqstl::make_unique<uqstl::function<void()>>(uqstl::move(task))};
	Callback_add(
		[](CallbackArg arg) {
			const uqstl::unique_ptr<uqstl::function<void()>> task {static_cast<uqstl::function<void()>*>(arg)};
			(*task)();
		},
		owned.release());
}

void setSetting(uqstl::string key, uqstl::string text)
{
	runOnGameThread([key {uqstl::move(key)}, text {uqstl::move(text)}] {
		SettingBase* const setting {UQMOptions::edit().find(key)};
		if (setting == nullptr || setting->hasValueText(text))
		{
			return;
		}
		if (!setting->parseText(text))
		{
			log::warn("'{}' is not a valid value for the setting '{}'.", text, key);
			return;
		}
		applySettingChange(*setting);
	});
}

void setCheatState(uqstl::string key, int value)
{
	runOnGameThread([key {uqstl::move(key)}, value] {
		if (CheatAction* const cheat {findCheatState(key)})
		{
			*cheat = static_cast<CheatAction>(value);
		}
	});
}

void saveSettings()
{
	runOnGameThread([] {
		if (saveUserSettings(UQMOptions::read()))
		{
			log::info("Settings saved.");
		}
	});
}

// Returns the menu definition, or nullptr until the game thread has read it.
const MenuDefinition* menuDefinition()
{
	if (s_definition)
	{
		return &*s_definition;
	}
	if (!s_loadRequested)
	{
		s_loadRequested = true;
		runOnGameThread([] {
			auto definition {readSetupMenuDefinition()};
			LockGuard guard {s_loaded.lock};
			s_loaded.definition = uqstl::move(definition);
			s_loaded.done = true;
		});
	}
	LockGuard guard {s_loaded.lock};
	if (s_loaded.done)
	{
		s_definition = uqstl::move(s_loaded.definition);
		s_loadFailed = !s_definition;
	}
	return s_definition ? &*s_definition : nullptr;
}

// Joins tooltip lines. Removes the "&!" marks that the setup menu uses to color a line.
uqstl::string joinLines(uqstl::span<const uqstl::string> lines)
{
	uqstl::string result {};
	for (const uqstl::string& line : lines)
	{
		if (!result.empty())
		{
			result += '\n';
		}
		result += line.starts_with("&!") ? uqstl::string_view {line}.substr(2) : uqstl::string_view {line};
	}
	return result;
}

void tooltip(uqstl::span<const uqstl::string> lines, uqstl::string_view extra = {})
{
	uqstl::string text {joinLines(lines)};
	if (!extra.empty())
	{
		if (!text.empty())
		{
			text += "\n\n";
		}
		text += extra;
	}
	if (!text.empty())
	{
		ImGui::SetItemTooltip("%s", text.c_str());
	}
}

void applyMarker(const SettingBase& setting)
{
	if (setting.requiresRestart())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(restart)");
		ImGui::SetItemTooltip("The change takes effect after the game restarts.");
	}
	else if (setting.requiresReload())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(reload)");
		ImGui::SetItemTooltip("The change takes effect after the setup menu reloads the game, or after a restart.");
	}
}

[[nodiscard]] bool anyAddonAvailable(uqstl::span<const uqstl::string> addons)
{
	return addons.empty() || uqstl::any_of(addons.begin(), addons.end(), [](const uqstl::string& addon) {
			   return isMenuAddonAvailable(addon);
		   });
}

[[nodiscard]] const char* optionLabel(const MenuItemDef& item, size_t index)
{
	if (item.labels == "controlTemplates" && index < uqstl::size(input_templates))
	{
		return input_templates[index].name;
	}
	return item.options[index].label.c_str();
}

void drawChoice(const MenuItemDef& item)
{
	const SettingBase* const setting {item.setting.empty() ? nullptr : UQMOptions::read().find(item.setting)};
	int current {-1};
	if (setting != nullptr)
	{
		for (size_t index {0}; index < item.options.size(); ++index)
		{
			if (setting->hasValueText(item.options[index].value))
			{
				current = static_cast<int>(index);
				break;
			}
		}
	}
	else if (const CheatAction* const cheat {findCheatState(item.state)})
	{
		current = static_cast<int>(*cheat);
	}
	else
	{
		return;
	}

	const uqstl::string preview {current >= 0 && current < static_cast<int>(item.options.size()) ? uqstl::string {optionLabel(item, current)} : setting != nullptr ? setting->valueText() :
																																									 uqstl::string {}};
	if (ImGui::BeginCombo(item.label.c_str(), preview.c_str()))
	{
		for (size_t index {0}; index < item.options.size(); ++index)
		{
			const MenuOptionDef& option {item.options[index]};
			ImGui::BeginDisabled(!anyAddonAvailable(option.requiresAddon));
			if (ImGui::Selectable(optionLabel(item, index), static_cast<int>(index) == current))
			{
				if (setting != nullptr)
				{
					setSetting(item.setting, option.value);
				}
				else
				{
					setCheatState(item.state, static_cast<int>(index));
				}
			}
			ImGui::EndDisabled();
			tooltip(option.tooltip);
		}
		ImGui::EndCombo();
	}
	tooltip(item.tooltip);
	if (setting != nullptr)
	{
		applyMarker(*setting);
	}
}

void drawSlider(const MenuItemDef& item)
{
	const SettingBase* const setting {UQMOptions::read().find(item.setting)};
	if (setting == nullptr)
	{
		return;
	}
	int value {sliderDisplayedValue(item, setting->valueText()).value_or(item.min)};
	if (ImGui::SliderInt(item.label.c_str(), &value, item.min, item.max, item.percent ? "%d%%" : "%d"))
	{
		const int step {uqstl::max(item.step, 1)};
		value = uqstl::clamp(item.min + (value - item.min + step / 2) / step * step, item.min, item.max);
		setSetting(item.setting, sliderSettingText(item, value));
	}
	tooltip(item.tooltip);
	applyMarker(*setting);
}

// A text field that sets the setting when the player presses Enter.
void drawTextField(const SettingBase& setting, uqgsl::czstring label)
{
	char buffer[64] {};
	fmt::format_to_sz_n(buffer, sizeof(buffer), "{}", setting.valueText());
	if (ImGui::InputText(label, buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		setSetting(uqstl::string {setting.key()}, buffer);
	}
}

void drawText(const MenuItemDef& item)
{
	const SettingBase* const setting {UQMOptions::read().find(item.setting)};
	if (setting == nullptr)
	{
		return;
	}
	drawTextField(*setting, item.label.c_str());
	tooltip(item.tooltip, "Press Enter to apply.");
	applyMarker(*setting);
}

void drawGamma(const MenuItemDef& item)
{
	float gamma {*UQMOptions::read().gamma};
	if (ImGui::SliderFloat(item.label.c_str(), &gamma, 0.4f, 2.5f, "%.2f"))
	{
		setSetting("graphics.gamma", fmt::format("{}", gamma));
	}
	tooltip(item.tooltip);
}

// A widget for a setting that no menu shows, chosen by the type of the setting.
void drawSetting(const SettingBase& setting)
{
	const uqstl::string key {setting.key()};
	if (setting.isFlag())
	{
		bool value {setting.hasValueText("true")};
		if (ImGui::Checkbox(key.c_str(), &value))
		{
			setSetting(key, value ? "true" : "false");
		}
	}
	else if (const uqstl::vector<uqstl::string> names {setting.valueNames()}; !names.empty())
	{
		const uqstl::string current {setting.valueText()};
		if (ImGui::BeginCombo(key.c_str(), current.c_str()))
		{
			for (const uqstl::string& name : names)
			{
				if (ImGui::Selectable(name.c_str(), setting.hasValueText(name)))
				{
					setSetting(key, name);
				}
			}
			ImGui::EndCombo();
		}
	}
	else
	{
		drawTextField(setting, key.c_str());
	}
	const uqstl::string description {setting.description()};
	ImGui::SetItemTooltip("%s\n\nDefault: %s", description.c_str(), setting.defaultText().c_str());
	applyMarker(setting);
}

void drawMenu(const MenuDefinition& definition, const MenuDef& menu, int depth);

void drawItem(const MenuDefinition& definition, const MenuItemDef& item, int depth)
{
	switch (item.kind)
	{
		case MenuItemKind::Choice:
			drawChoice(item);
			break;
		case MenuItemKind::Slider:
			drawSlider(item);
			break;
		case MenuItemKind::Text:
			drawText(item);
			break;
		case MenuItemKind::Heading:
			ImGui::SeparatorText(joinLines(item.lines).c_str());
			break;
		case MenuItemKind::Legacy:
			if (item.widget == "gamma")
			{
				drawGamma(item);
			}
			break;
		case MenuItemKind::Menu:
			if (const MenuDef* const submenu {definition.find(item.target)}; submenu != nullptr && depth < c_maxMenuDepth)
			{
				if (ImGui::TreeNode(item.label.c_str()))
				{
					drawMenu(definition, *submenu, depth + 1);
					ImGui::TreePop();
				}
			}
			break;
		default:
			break;
	}
}

void drawMenu(const MenuDefinition& definition, const MenuDef& menu, int depth)
{
	for (const MenuItemDef& item : menu.items)
	{
		if (!anyAddonAvailable(item.requiresAddon))
		{
			continue;
		}
		ImGui::PushID(&item);
		drawItem(definition, item, depth);
		ImGui::PopID();
	}
}

// Adds the settings that the items of the menu and its submenus show.
void collectShownSettings(const MenuDefinition& definition, const MenuDef& menu, uqstl::set<uqstl::string>& keys, int depth)
{
	for (const MenuItemDef& item : menu.items)
	{
		if (!item.setting.empty())
		{
			keys.insert(item.setting);
		}
		else if (item.kind == MenuItemKind::Legacy && item.widget == "gamma")
		{
			keys.insert("graphics.gamma");
		}
		else if (item.kind == MenuItemKind::Menu && depth < c_maxMenuDepth)
		{
			if (const MenuDef* const submenu {definition.find(item.target)})
			{
				collectShownSettings(definition, *submenu, keys, depth + 1);
			}
		}
	}
}

void drawOtherSettings(const MenuDefinition& definition)
{
	uqstl::set<uqstl::string> shown {};
	collectShownSettings(definition, *definition.find("main"), shown, 0);

	ImGui::TextWrapped("Settings that the setup menu does not show.");
	uqstl::string_view group {};
	for (const SettingBase* setting : UQMOptions::read().settings())
	{
		const uqstl::string_view key {setting->key()};
		if (shown.contains(uqstl::string {key}))
		{
			continue;
		}
		const uqstl::string_view prefix {key.substr(0, key.find('.'))};
		if (prefix != group)
		{
			group = prefix;
			ImGui::SeparatorText(uqstl::string {group}.c_str());
		}
		ImGui::PushID(setting);
		drawSetting(*setting);
		ImGui::PopID();
	}
}

// Draws the menu as a window. The submenus of the top menu are tabs.
void drawMenuWindow(uqgsl::czstring title, uqstl::string_view menuId, bool withOther, bool& open)
{
	ImGui::SetNextWindowSize(ImVec2 {640.0f, 600.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(title, &open))
	{
		ImGui::End();
		return;
	}

	const MenuDefinition* const definition {menuDefinition()};
	const MenuDef* const menu {definition != nullptr ? definition->find(menuId) : nullptr};
	if (menu == nullptr)
	{
		ImGui::TextUnformatted(s_loadFailed ? "The setup menu definition could not be read." : "Loading...");
		ImGui::End();
		return;
	}

	if (ImGui::Button("Save settings"))
	{
		saveSettings();
	}
	ImGui::SetItemTooltip("Write the settings to settings.json. Changes apply at once, but are saved only with this button or the setup menu.");

	ImGui::PushItemWidth((ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 14.0f) * 0.5f);
	if (ImGui::BeginTabBar("menus"))
	{
		const auto isValueItem {[](const MenuItemDef& item) {
			return item.kind == MenuItemKind::Choice || item.kind == MenuItemKind::Slider || item.kind == MenuItemKind::Text
				|| item.kind == MenuItemKind::Legacy;
		}};
		if (uqstl::any_of(menu->items.begin(), menu->items.end(), isValueItem) && ImGui::BeginTabItem("General"))
		{
			for (const MenuItemDef& item : menu->items)
			{
				if (item.kind != MenuItemKind::Menu && anyAddonAvailable(item.requiresAddon))
				{
					ImGui::PushID(&item);
					drawItem(*definition, item, 0);
					ImGui::PopID();
				}
			}
			ImGui::EndTabItem();
		}
		for (const MenuItemDef& item : menu->items)
		{
			const MenuDef* const submenu {item.kind == MenuItemKind::Menu ? definition->find(item.target) : nullptr};
			if (submenu != nullptr && anyAddonAvailable(item.requiresAddon) && ImGui::BeginTabItem(item.label.c_str()))
			{
				drawMenu(*definition, *submenu, 1);
				ImGui::EndTabItem();
			}
		}
		if (withOther && ImGui::BeginTabItem("Other"))
		{
			drawOtherSettings(*definition);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::PopItemWidth();
	ImGui::End();
}

} // namespace

void addSettingsWindows()
{
	addToolWindow("Settings", [](bool& open) {
		drawMenuWindow("Settings", "main", true, open);
	},
				  true);
	addToolWindow("Cheats", [](bool& open) {
		drawMenuWindow("Cheats", "cheats", false, open);
	});
}

} // namespace uqm
