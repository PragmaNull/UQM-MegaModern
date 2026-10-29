#include "Configuration.h"

#include "options.h"
#include "options/options.h"
#include "core/log/log.h"
#include "libs/reslib.h"
#include "uqm/controls.h"

#include <cstring>

namespace uqm
{

static constexpr const char* c_settingsFile {"settings.json"};
static constexpr const char* c_badSettingsFile {"settings.json.bad"};

static_assert(c_controlTemplateCount == static_cast<int>(ControlTemplate::NUM));

bool loadUserSettings(OptionsStruct& options)
{
	uio_Stream* stream {uio_fopen(configDir, c_settingsFile, "rb")};
	if (stream == nullptr)
	{
		uqm::log::info("No {} found; using default settings.", c_settingsFile);
		return false;
	}

	uqstl::string json {};
	char buffer[4096] {};
	size_t count {};
	while ((count = uio_fread(buffer, 1, sizeof buffer, stream)) > 0)
	{
		json.append(buffer, count);
	}
	uio_fclose(stream);

	if (!options.fromJson(json))
	{
		uio_unlink(configDir, c_badSettingsFile);
		if (uio_rename(configDir, c_settingsFile, configDir, c_badSettingsFile) == 0)
		{
			uqm::log::warn("Moved unreadable {} to {}; using default settings.", c_settingsFile, c_badSettingsFile);
		}
		else
		{
			uqm::log::error("Could not move unreadable {} to {}.", c_settingsFile, c_badSettingsFile);
		}
		return false;
	}
	return true;
}

bool saveUserSettings(const OptionsStruct& options)
{
	uio_Stream* stream {res_OpenResFile(configDir, c_settingsFile, "wb")};
	if (stream == nullptr)
	{
		uqm::log::error("Could not open {} for writing.", c_settingsFile);
		return false;
	}

	const uqstl::string json {options.toJson()};
	const bool written {WriteResFile(json.data(), 1, json.size(), stream) == json.size()};
	res_CloseResFile(stream);

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
