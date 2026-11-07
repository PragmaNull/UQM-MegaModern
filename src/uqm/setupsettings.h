// Setting changes and menu data that the setup menu and the ImGui settings windows share.

#pragma once
#ifndef UQM_SETUPSETTINGS_H
#define UQM_SETUPSETTINGS_H

#include <cstdint>

#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "uqm/menudef.h"


namespace uqm
{
class SettingBase;
class SettingsRegistry;
enum class CheatAction : uint8_t;

// Reads the menu definition of the STRING resource 'resource' and checks it against 'settings'.
// Returns nullopt and logs the error if it cannot be read.
[[nodiscard]] uqstl::optional<MenuDefinition> readMenuDefinition(uqgsl::czstring resource, const SettingsRegistry& settings);

// Reads the setup menu definition. Returns nullopt and logs the error if it cannot be read.
[[nodiscard]] uqstl::optional<MenuDefinition> readSetupMenuDefinition();

// True if the addon is available. Also accepts the names "dos-mode", "3do-mode" and "directional-joystick".
[[nodiscard]] bool isMenuAddonAvailable(uqstl::string_view addon);

// The cheat action of a state such as "device.3" or "upgrade.0", or nullptr if there is no such state.
[[nodiscard]] CheatAction* findCheatState(uqstl::string_view key);

// Applies a changed setting to the running game. Returns true if it also changed other settings.
bool applySettingChange(const SettingBase& setting);

} // namespace uqm


#endif // UQM_SETUPSETTINGS_H
