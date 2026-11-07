#pragma once
#ifndef UQM_CONFIGURATION_CONFIGURATION_H
#define UQM_CONFIGURATION_CONFIGURATION_H

namespace uqm
{
struct OptionsStruct;

// Reads settings.json from the config directory. Returns false if the file is missing or invalid.
bool loadUserSettings(OptionsStruct& options);

bool saveUserSettings(const OptionsStruct& options);

// Updates game state that is derived from settings, such as the player control templates.
void applyUserSettings(OptionsStruct& options);

} // namespace uqm
#endif // UQM_CONFIGURATION_CONFIGURATION_H
