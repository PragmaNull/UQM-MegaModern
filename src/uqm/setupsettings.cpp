// Setting changes and menu data that the setup menu and the ImGui settings windows share.

#include "setupsettings.h"

#include "controls.h"
#include "options.h"
#include "setup.h"
#include "units.h"
#include "core/log/log.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/file/textfile.h"
#include "libs/math/random.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "libs/sound/sound.h"

#if defined(DIRECTIONAL_JOY)
#define HAVE_DIRECTIONAL_JOYSTICK true
#else
#define HAVE_DIRECTIONAL_JOYSTICK false
#endif

namespace uqm
{

namespace
{

constexpr const char* c_menuDefinitionResource {"ui.setupmenu"};

} // namespace

uqstl::optional<MenuDefinition> readSetupMenuDefinition()
{
	const char* path {res_GetString(c_menuDefinitionResource)};
	if (path == nullptr)
	{
		log::error("The resource '{}' is not defined.", c_menuDefinitionResource);
		return uqstl::nullopt;
	}
	const auto json {readTextFile(contentDir(), path)};
	if (!json)
	{
		log::error("Could not read the setup menu definition '{}'.", path);
		return uqstl::nullopt;
	}

	auto definition {menuDefinitionFromJson(*json)};
	if (!definition)
	{
		log::error("The setup menu definition '{}' is not valid.", path);
		return uqstl::nullopt;
	}
	for (const uqstl::string& error : validateMenuDefinition(*definition, UQMOptions::read()))
	{
		log::error("{}", error);
	}
	if (definition->find("main") == nullptr)
	{
		log::error("The setup menu definition has no 'main' menu.");
		return uqstl::nullopt;
	}
	return definition;
}

bool isMenuAddonAvailable(uqstl::string_view addon)
{
	// The window type addons depend on the selected graphics mode.
	const bool hd {*UQMOptions::read().resolutionFactor != 0};
	if (addon == "dos-mode")
	{
		return isAddonAvailable(DOS_MODE(hd));
	}
	if (addon == "3do-mode")
	{
		return isAddonAvailable(THREEDO_MODE(hd));
	}
	if (addon == "directional-joystick")
	{
		return HAVE_DIRECTIONAL_JOYSTICK;
	}
	return isAddonAvailable(uqstl::string {addon}.c_str());
}

CheatAction* findCheatState(uqstl::string_view key)
{
	const auto indexed {[key](uqstl::string_view prefix, size_t count) -> uqstl::optional<size_t> {
		size_t index {};
		if (!key.starts_with(prefix) || parseStr(key.substr(prefix.size()), index) != uqstl::errc {} || index >= count)
		{
			return uqstl::nullopt;
		}
		return index;
	}};
	if (const auto device {indexed("device.", uqstl::size(optDeviceArray))})
	{
		return &optDeviceArray[*device];
	}
	if (const auto upgrade {indexed("upgrade.", uqstl::size(optUpgradeArray))})
	{
		return &optUpgradeArray[*upgrade];
	}
	return nullptr;
}

// Sets the window size and the display flags from the settings.
static void applyGraphicsSettings()
{
	const OptionsStruct& options {UQMOptions::read()};

	const GfxFlags newFlags {(g_gfxFlags & ~c_displayFlagsMask) | displayFlags(options)};

	const GfxDriver newDriver {GraphicsDriver};

	const int blowup {*options.loresBlowupScale};
	if (blowup == 0)
	{
		SavedWidth = RES_SCALE(320);
		SavedHeight = RES_SCALE(DOS_BOOL(240, 200));
	}
	else if (blowup < 6)
	{
		SavedWidth = 320 * (1 + blowup);
		SavedHeight = DOS_BOOL(240, 200) * (1 + blowup);
	}
	else
	{
		SavedWidth = inBounds(static_cast<int>(options.resolution.value.x), 320, 1920);
		SavedHeight = inBounds(static_cast<int>(options.resolution.value.y), 200, 1440);
	}

	int newWidth {SavedWidth};
	int newHeight {SavedHeight};
	if (*options.keepAspectRatio)
	{
		const float threshold {0.75f};
		const float ratio {static_cast<float>(newHeight) / static_cast<float>(newWidth)};
		if (ratio > threshold) // screen is narrower than 4:3
		{
			newWidth = static_cast<int>(newHeight / threshold);
		}
		else if (ratio < threshold) // screen is wider than 4:3
		{
			newHeight = static_cast<int>(newWidth * threshold);
		}
	}

	if (newWidth != WindowWidth || newHeight != WindowHeight || newFlags != g_gfxFlags || newDriver != GraphicsDriver)
	{
		// Exclusive fullscreen needs a second reinit after a size change.
		const bool exclusive {testFlag(newFlags, GfxFlags::ExclusiveFullscreen)};
		const bool resized {newWidth != WindowWidth || newHeight != WindowHeight};
		if (exclusive && resized)
		{
			TFB_DrawScreen_ReinitVideo(newDriver, newFlags & ~GfxFlags::ExclusiveFullscreen, newWidth, newHeight);
			FlushInput();
		}
		TFB_DrawScreen_ReinitVideo(newDriver, newFlags, newWidth, newHeight);
	}
	FlushInput();
}

bool applySettingChange(const SettingBase& setting)
{
	optRequiresReload = optRequiresReload || setting.requiresReload();
	optRequiresRestart = optRequiresRestart || setting.requiresRestart();

	const uqstl::string_view key {setting.key()};
	const OptionsStruct& options {UQMOptions::read()};
	if (key == "audio.musicVolume")
	{
		musicVolumeScale = *options.musicVolumeScale;
		SetMusicVolume(musicVolume);
	}
	else if (key == "audio.sfxVolume")
	{
		sfxVolumeScale = *options.sfxVolumeScale;
	}
	else if (key == "audio.speechVolume")
	{
		speechVolumeScale = *options.speechVolumeScale;
		SetSpeechVolume(speechVolumeScale);
	}
	else if (key == "graphics.gamma")
	{
		TFB_SetGamma(*options.gamma);
	}
	else if (key == "seed.type" || key == "seed.value")
	{
		g_seedType = *options.seedType;
		const bool primeSeed {*options.seedType == SeedType::None || *options.seedType == SeedType::Prime};
		if (primeSeed && *options.customSeed != PrimeA)
		{
			UQMOptions::edit().customSeed = PrimeA;
			return true;
		}
	}
	else if (key == "graphics.loresBlowupScale" || key == "graphics.resolution")
	{
		auto& edit {UQMOptions::edit()};
		if (key == "graphics.resolution")
		{
			edit.loresBlowupScale = 6;
		}
		applyGraphicsSettings();
		edit.resolution = Vec2u {static_cast<uint32_t>(SavedWidth), static_cast<uint32_t>(SavedHeight)};
		return true;
	}
	else if (key == "graphics.windowMode" || key == "graphics.scaler" || key == "graphics.scanlines" || key == "graphics.showFps"
			 || key == "graphics.keepAspectRatio")
	{
		applyGraphicsSettings();
	}
	else if (key == "controls.player1Template")
	{
		PlayerControlTemplates[0] = static_cast<ControlTemplate>(*options.player1Control);
	}
	else if (key == "controls.player2Template")
	{
		PlayerControlTemplates[1] = static_cast<ControlTemplate>(*options.player2Control);
	}
	return false;
}

} // namespace uqm
