#include "Configuration.h"

#include "options.h"
#include "options/options.h"
#include "core/log/log.h"
#include "libs/file/textfile.h"
#include "libs/reslib.h"
#include "uqm/controls.h"

#include <cstring>

namespace uqm
{

static constexpr const char* c_settingsFile {"settings.json"};

static_assert(c_controlTemplateCount == static_cast<int>(ControlTemplate::NUM));

bool loadUserSettings(OptionsStruct& options)
{
	const auto json {readTextFile(configDir, c_settingsFile)};
	if (!json)
	{
		uqm::log::info("No {} found; using default settings.", c_settingsFile);
		return false;
	}

	if (!options.fromJson(*json))
	{
		moveToBadFile(configDir, c_settingsFile);
		return false;
	}
	return true;
}

bool saveUserSettings(const OptionsStruct& options)
{
	const bool written {writeTextFile(configDir, c_settingsFile, options.toJson())};
	if (!written)
	{
		uqm::log::error("Could not write {}.", c_settingsFile);
	}
	return written;
}

void applyUserSettings(OptionsStruct& options)
{
	if (options.alwaysOpenGL && !options.opengl.hasCommandLineOverride())
	{
		options.opengl.value = true;
	}

	options.optDifficulty.value = *options.optDiffChooser;
	if (options.optDifficulty.value == Difficulty::ChooseYourOwn)
	{
		options.optDifficulty.value = Difficulty::Normal;
	}

	PlayerControlTemplates[0] = static_cast<ControlTemplate>(*options.player1Control);
	PlayerControlTemplates[1] = static_cast<ControlTemplate>(*options.player2Control);

	memset(&optDeviceArray, 0, sizeof(optDeviceArray));
	memset(&optUpgradeArray, 0, sizeof(optUpgradeArray));
}

} // namespace uqm
