#include "options.h"

#include <CLI/CLI.hpp>

#include "../options.h"
#include "libs/graphics/sdl/pure.h" // for enumerating renderer backends.
#include "uqm/init.h"				// for NUM_PLAYERS, which we need to know to set up netplay options. Maybe we should move that somewhere else?

#include "options/SettingsCli.h"
#ifdef NETPLAY
#include "options/netoptions.h" // for NETPLAY options
#endif
namespace uqm
{

UQMOptions* UQMOptions::s_instance {nullptr};

UQMOptions::UQMOptions()
	: m_options {}
#ifdef NETPLAY
	, m_netplayOptions {NUM_PLAYERS}
#endif
{
	assert(s_instance == nullptr);
	s_instance = this;
}

UQMOptions::~UQMOptions()
{
	assert(s_instance == this);
	s_instance = nullptr;
}

UQMOptions& UQMOptions::getInstance()
{
	assert(s_instance != nullptr);
	return *s_instance;
}


uqstl::pair<int, bool> UQMOptions::parseArgs(uqstl::span<uqgsl::zstring> args)
{
	CLI::App app("A port of the UQM MegaMod depot to modern C++", "Ur-Quan Masters MegaModERN");
	auto normalizedArgs {app.ensure_utf8(args.data())};

	const OptionsStruct defaults {};
	SettingsCli settingsCli {app, m_options};

	// Options that are not saved settings.
	CLI::App& gameplayGroup {settingsCli.group("Gameplay")};
	gameplayGroup.add_flag("--safe,--safemode", m_options.safeMode.edit(), "Start the game in safe-mode. No configurations or other options will be loaded.")
		->default_str(defaults.safeMode.toString());
	gameplayGroup.add_flag("--melee", optSuperMelee, "Takes you straight to Super Melee after the splash screen.");
	gameplayGroup.add_flag("--loadgame", optLoadGame, "Takes you straight to the Load Game sceen after the splash screen.");

	uqstl::vector<uqstl::string> rendererBackends {};
	TFB_Pure_GetRendererBackends(rendererBackends);
	uqstl::sort(rendererBackends.begin(), rendererBackends.end());
	settingsCli.group("Graphics")
		.add_option("--renderer", m_options.graphicsBackend, "Rendering backend to use. If not specified, SDL will choose a default for us.")
		->check(CLI::IsMember(rendererBackends, CLI::ignore_case));

	CLI::App& pathGroup {settingsCli.group("Paths")};
	pathGroup.add_option("-C,--configdir", m_options.configDir, "Path to the directory containing configuration files.")
		->check(CLI::ExistingPath)
		->capture_default_str();
	pathGroup.add_option("-n,--contentdir", m_options.contentDir, "Path to the directory containing game content.")
		->check(CLI::ExistingPath)
		->capture_default_str();
	pathGroup.add_option("--addondir", m_options.addonDir, "Path to the directory containing game addons.")
		->check(CLI::ExistingPath)
		->capture_default_str();
	pathGroup.add_option("-l,--logfile", m_options.logFile, "Path to the log file to create. The default is logs/uqm.log in the config dir.")
		->capture_default_str();
	pathGroup.add_option("--addon", m_options.addons, "Addons to load from the addons directory. Any number may be specified")
		->expected(-1)
		->capture_default_str();

	settingsCli.group("Testing")
		.add_option("--input-script", m_options.inputScript, "File of menu key presses to run, for testing. See uqm/inputscript.h.")
		->check(CLI::ExistingFile);

	settingsCli.group("Cheats")
		.add_flag("--unlockupgrades", m_options.unlockUpgrades.edit(), "Unlocks every upgrade for your flagship and landers)")
		->default_str(defaults.unlockUpgrades.toString());

#ifdef NETPLAY
	m_netplayOptions.configureCommands(app);
#endif

	try
	{
		app.parse(args.size(), args.data());
		settingsCli.apply();
	}
	catch (const CLI::CallForHelp& e)
	{
		std::cout << app.help();
		return {0, true};
	}
	catch (const CLI::CallForAllHelp& e)
	{
		std::cout << app.help("", CLI::AppFormatMode::All);
		return {0, true};
	}
	catch (const CLI::ParseError& e)
	{
		return {app.exit(e), true};
	}

	return {EXIT_SUCCESS, false};
}

} // namespace uqm