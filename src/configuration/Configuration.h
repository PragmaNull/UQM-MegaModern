#pragma once
#ifndef UQM_CONFIGURATION_CONFIG_H_
#define UQM_CONFIGURATION_CONFIG_H_

namespace uqm
{
struct OptionsStruct;

// Reads settings.json from the config directory. Returns false if the file is missing or invalid.
bool loadUserSettings(OptionsStruct& options);

bool saveUserSettings(const OptionsStruct& options);

// Updates game state that is derived from settings, such as the player control templates.
void applyUserSettings(OptionsStruct& options);

} // namespace uqm
#endif /* UQM_CONFIGURATION_CONFIG_H_ */
