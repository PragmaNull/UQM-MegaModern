#include "Configuration.h"

#include <cstring>

#include "core/log/log.h"
#include "libs/file/json.h"
#include "libs/file/textfile.h"
#include "libs/reslib.h"
#include "options.h"
#include "options/options.h"
#include "uqm/controls.h"

namespace uqm
{

static constexpr const char* c_settingsFile {"settings.json"};

static_assert(c_controlTemplateCount == uqstl::to_underlying(ControlTemplate::NUM));

bool loadUserSettings(OptionsStruct& options)
{
	auto json {readJsonText(uqm::configDir(), c_settingsFile)};
	if (!json)
	{
		uqm::log::info("No {} found; using default settings.", c_settingsFile);
		return false;
	}

	if (!options.fromJson(uqstl::move(*json)))
	{
		moveToBadFile(uqm::configDir(), c_settingsFile);
		return false;
	}
	return true;
}

bool saveUserSettings(const OptionsStruct& options)
{
	const bool written {writeTextFile(uqm::configDir(), c_settingsFile, options.toJson())};
	if (!written)
	{
		uqm::log::error("Could not write {}.", c_settingsFile);
	}
	return written;
}

void applyUserSettings(OptionsStruct& options)
{
	options.optDifficulty.value = *options.optDiffChooser;
	if (options.optDifficulty.value == Difficulty::ChooseYourOwn)
	{
		options.optDifficulty.value = Difficulty::Normal;
	}

	PlayerControlTemplates[0] = static_cast<ControlTemplate>(*options.player1Control);
	PlayerControlTemplates[1] = static_cast<ControlTemplate>(*options.player2Control);

	uqstl::fill(uqstl::begin(optDeviceArray), uqstl::end(optDeviceArray), CheatAction::None);
	uqstl::fill(uqstl::begin(optUpgradeArray), uqstl::end(optUpgradeArray), CheatAction::None);
}

} // namespace uqm
