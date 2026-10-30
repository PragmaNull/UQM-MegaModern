//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "UQMGame.h"

#include "core/log/log.h"
#include "options/options.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "configuration/Configuration.h"

//////////////////////////////////////////////////
// legacy includes below


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt/getopt.h"
#endif

#include <stdarg.h>
#include <errno.h>
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/cmap.h"
#include "libs/sound/sound.h"
#include "libs/input/input_common.h"
#include "libs/inplib.h"
#include "core/thread/Thread.h"
#include "libs/threadlib.h"
#include "libs/scriptlib.h"
#include "uqm/controls.h"
#include "uqm/battle.h"
// For BattleFrameRateTicks
#include <cstdint>
#include "port.h"
#include "libs/memlib.h"
#include "core/log/log.h"
#include "options.h"
#include "uqmversion.h"
#include "uqm/comm.h"
#ifdef NETPLAY
#include "libs/callback/callback.h"
#include "libs/callback/alarm.h"
#include "libs/net.h"
#include "uqm/supermelee/netplay/netplay.h"
#endif
#include "uqm/setup.h"
#include "uqm/starcon.h"
#include "libs/math/random.h"

bool restartGame;

#if defined(GFXMODULE_SDL)
#include <SDL.h>
// Including this is actually necessary on OSX.
#endif

// Version headers for library startup logging
#include <CLI/Version.hpp>
#include <fmt/base.h>
#include <gsl-lite/gsl-lite.hpp>
#include <lua.hpp>
#include <magic_enum/magic_enum.hpp>
#include <mikmod.h>
#include <nlohmann/json_fwd.hpp>
#include <png.h>
#include <scn/fwd.h>
#include <spdlog/version.h>
#include <vorbis/codec.h>
#include <zlib.h>


namespace uqm
{

UQMGame* UQMGame::s_instance {nullptr};

UQMGame::UQMGame()
{
	assert(s_instance == nullptr);
	s_instance = this;
}

UQMGame::~UQMGame()
{
	assert(s_instance == this);
	s_instance = nullptr;
}

UQMGame& UQMGame::getInstance()
{
	assert(s_instance != nullptr);
	return *s_instance;
}

uqstl::pair<int, bool> UQMGame::setup(uqstl::span<uqgsl::zstring> args)
{

	// NOTE: we cannot use the logging facility yet because we may have to
	//   log to a file, and we'll only get the log file name after parsing
	//   the options.
	const auto [parseResult, needExit] = m_options.parseArgs(args);
	if (parseResult != 0 || needExit)
	{
		return {parseResult, needExit};
	}

	//log_init(15);

	const auto& options {m_options.get()};

	if (options.logFile.empty())
	{
		uqstl::string logDir {resolveConfigDir(options.configDir)};
		if (!logDir.empty() && logDir.back() != '/' && logDir.back() != '\\')
		{
			logDir += '/';
		}
		m_options.edit().logFile = logDir + "logs/uqm.log";
	}

	m_logger.init(options.logFile);

	for (size_t i = 0; i < args.size(); ++i)
	{
		uqm::log::debug("argv[{}] = \"{}\"", i, args[i]);
	}

	if (options.runMode == RunMode::Version)
	{
		uqm::log::info("{}.{}.{} {}\n", UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
					   UQM_PATCH_VERSION,
					   (resolutionFactor ? "HD " UQM_EXTRA_VERSION : UQM_EXTRA_VERSION));
		//log_showBox(false, false);
		return {EXIT_SUCCESS, true};
	}

	uqm::log::info("!! The Ur-Quan Masters v{}.{}.{} {} (compiled {} {})", UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION, (resolutionFactor ? "HD " UQM_EXTRA_VERSION : UQM_EXTRA_VERSION), __DATE__, __TIME__);
	uqm::log::info("!! This software comes with ABSOLUTELY NO WARRANTY;");
	uqm::log::info("!! for details see the included 'COPYING' file.");

#ifdef NETPLAY
	uqm::log::info("Netplay protocol version {}.{}. Netplay opponent "
				   "must have UQM {}.{}.{} or later.",
				   NETPLAY_PROTOCOL_VERSION_MAJOR, NETPLAY_PROTOCOL_VERSION_MINOR,
				   NETPLAY_MIN_UQM_VERSION_MAJOR, NETPLAY_MIN_UQM_VERSION_MINOR,
				   NETPLAY_MIN_UQM_VERSION_PATCH);
#endif

	// Compiler info to help with future debugging.
#ifdef _MSC_VER
	uqm::log::info("MSC_VER: {}", _MSC_VER);
	uqm::log::info("MSC_FULL_VER: {}", _MSC_FULL_VER);
	uqm::log::info("MSC_BUILD: {}", _MSC_BUILD);
#endif // _MSC_VER

#ifdef __GNUC__
	uqm::log::info("GCC_VERSION: {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif // __GNUC__

#ifdef __clang__
	uqm::log::info("CLANG_VERSION: {}.{}.{}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#endif // __clang__

#ifdef __MINGW32__
	uqm::log::info("MINGW32_VERSION: {}.{}", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
#endif // __MINGW32__

#ifdef __MINGW64__
	uqm::log::info("MINGW64_VERSION: {}.{}", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
#endif // __MINGW64__

	// Library versions.
	uqm::log::info("{:-^40}", "Library versions");
	uqm::log::info("\tSDL (compiled): {}.{}.{}",
				   SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
	{
		SDL_version sdlRuntime {};
		SDL_GetVersion(&sdlRuntime);
		uqm::log::info("\tSDL (runtime):  {}.{}.{}",
					   sdlRuntime.major, sdlRuntime.minor, sdlRuntime.patch);
	}
	uqm::log::info("\tfmt:           {}.{}.{}",
				   FMT_VERSION / 10000, (FMT_VERSION % 10000) / 100, FMT_VERSION % 100);
	uqm::log::info("\tspdlog:        {}.{}.{}",
				   SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
	uqm::log::info("\tLua:           {}", LUA_RELEASE);
	uqm::log::info("\tzlib:          {}", zlibVersion());
	uqm::log::info("\tlibpng:        {}", PNG_LIBPNG_VER_STRING);
	uqm::log::info("\tlibmikmod:     {}.{}.{}",
				   LIBMIKMOD_VERSION_MAJOR, LIBMIKMOD_VERSION_MINOR, LIBMIKMOD_REVISION);
	uqm::log::info("\tnlohmann_json: {}.{}.{}",
				   NLOHMANN_JSON_VERSION_MAJOR, NLOHMANN_JSON_VERSION_MINOR,
				   NLOHMANN_JSON_VERSION_PATCH);
	uqm::log::info("\tCLI11:         {}", CLI11_VERSION);
	uqm::log::info("\tmagic_enum:    {}.{}.{}",
				   MAGIC_ENUM_VERSION_MAJOR, MAGIC_ENUM_VERSION_MINOR, MAGIC_ENUM_VERSION_PATCH);
	uqm::log::info("\tscnlib:        {}.{}.{}",
				   SCN_VERSION / 10000000, (SCN_VERSION % 10000000) / 10000,
				   SCN_VERSION % 10000);
	uqm::log::info("\tgsl-lite:      {}", gsl_lite_VERSION);
	uqm::log::info("\tvorbis:        {}", vorbis_version_string());
	uqm::log::info("{:-^40}", "");

	if (!options.inputScript.empty() && !LoadInputScript(options.inputScript.c_str()))
	{
		return {EXIT_FAILURE, true};
	}

	TFB_PreInit();
	mem_init();
	//log_initThreads();
	initIO();
	prepareConfigDir(options.configDir);

	if (!options.safeMode.value)
	{
		loadUserSettings(m_options.edit());
	}
	applyUserSettings(m_options.edit());


	/* TODO: These must be global so that initAudio (in StarCon2Main)
	   can see them. */
	snddriver = options.soundDriver.value;
	// clear existing quality flag before applying the option value.
	soundflags &= ~AudioQualityFlagMask;
	soundflags |= toAudioFlags(*options.soundQuality);

	musicVolumeScale = options.musicVolumeScale.value;
	sfxVolumeScale = options.sfxVolumeScale.value;
	speechVolumeScale = options.speechVolumeScale.value;

	g_seedType = options.seedType.value;
	optRequiresReload = false;
	optRequiresRestart = false;

	prepareContentDir(c_str(options.contentDir), c_str(options.addonDir), c_str(args.front()));

	resolutionFactor = isAddonAvailable(HD_MODE) ?
						   (unsigned int)options.resolutionFactor.value :
						   0;

	optWindowType = WindowStyle::UQM;
	const WindowStyle windowStyle {*options.windowEmulationMode};
	if (windowStyle != WindowStyle::UQM && isAddonAvailable(WINDOW_MODE(resolutionFactor, static_cast<int>(windowStyle))))
	{
		optWindowType = windowStyle;
	}

	Vec2u windowSize {*options.resolution};
	{
		int w = 320;
		int h = (DOS_BOOL(240, 200));

		int loresBlowup = options.loresBlowupScale.value;
		int scaleFactor = loresBlowup + 1; // stored value starts with 0

		CanvasWidth = w << resolutionFactor;
		CanvasHeight = h << resolutionFactor;

		if (loresBlowup < 6)
		{
			SavedWidth = loresBlowup ? (w * scaleFactor) : CanvasWidth;
			SavedHeight = loresBlowup ? (h * scaleFactor) : CanvasHeight;
		}
		else
		{
			SavedWidth = inBounds(options.resolution.value.x, 320, 1920);
			SavedHeight = inBounds(options.resolution.value.y, 200, 1440);
		}

		if (options.keepAspectRatio)
		{
			float threshold = 0.75f;
			float ratio = (float)SavedHeight / (float)SavedWidth;

			if (ratio > threshold) // screen is narrower than 4:3
			{
				windowSize.y = SavedHeight / threshold;
			}
			else if (ratio < threshold) // screen is wider than 4:3
			{
				windowSize.x = SavedWidth * threshold;
			}
		}
		else
		{
			windowSize.x = SavedWidth;
			windowSize.y = SavedHeight;
		}
	}


	prepareMeleeDir();
	prepareSaveDir();
	prepareScrShotDir();
	prepareShadowAddons(options.addons);
#if 0
	initTempDir ();
#endif

	InitTimeSystem();

	luaUqm_init();

	Alarm_init();
	Callback_init();

#ifdef NETPLAY
	Network_init();
	NetManager_init();
#endif

	const uqm::GfxDriver gfxDriver = uqm::GfxDriver::SDL_Pure;
	const uqm::GfxFlags gfxFlags {uqm::displayFlags(options)};
	unsigned int windowType {static_cast<unsigned int>(optWindowType)};
	TFB_InitGraphics(gfxDriver, gfxFlags, options.graphicsBackend.c_str(),
					 windowSize.x, windowSize.y,
					 &resolutionFactor, &windowType);
	if (options.gamma.set && setGammaCorrection(options.gamma.value))
	{
		// gamma.value already holds the correct value in the singleton
	}
	else
	{
		m_options.edit().gamma = 1.0f; // failed or default
	}

	InitColorMaps();
	init_communication();
	// initAudio (snddriver, soundflags);
	TFB_SetInputVectors(ImmediateInputState.menu, NUM_MENU_KEYS,
						&ImmediateInputState.key[0][0], NUM_KEYS,
						static_cast<int>(ControlTemplate::NUM));
	TFB_InitInput(TFB_INPUTDRIVER_SDL, 0);

	return {EXIT_SUCCESS, false};
}

int UQMGame::run()
{
	m_gameThread = makeThread("Starcon2Main", [](uqstl::stop_token) {
		(void)Starcon2Main(nullptr);
	});

	for (int i = 0; i < 2000 && !MainExited;)
	{
		if (QuitPosted)
		{ /* Try to stop the main thread, but limited number of times */
			SignalStopMainThread();
			++i;
		}
		else if (!GameActive)
		{ // Throttle down the main loop when game is inactive
			HibernateThread(getTicksForFramerate(4));
		}

		TFB_ProcessEvents();
		ProcessUtilityKeys();
		TFB_FlushGraphics();
	}

	// A game thread that did not stop ends with the process.
	if (MainExited)
	{
		m_gameThread.join();
	}
	else
	{
		m_gameThread.detach();
	}

	return EXIT_SUCCESS;
}

void UQMGame::teardown()
{
	/* Currently, we use atexit() callbacks everywhere, so we
	 *   cannot simply call unInitAudio() and the like, because other
	 *   tasks might still be using it */
	if (MainExited)
	{
		TFB_UninitInput();
		unInitAudio();
		uninit_communication();

		TFB_PurgeDanglingGraphics();
		// Purge above refers to colormaps which have to be still up
		UninitColorMaps();
		TFB_UninitGraphics();

#ifdef NETPLAY
		NetManager_uninit();
		Network_uninit();
#endif

		Callback_uninit();
		Alarm_uninit();

		luaUqm_uninit();

		UnInitTimeSystem();
#if 0
		unInitTempDir ();
#endif
		uninitIO();
		mem_uninit();
	}
}

} // namespace uqm