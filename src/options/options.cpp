#include "options.h"

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <regex>
//#include <fmt/ranges.h>

#include "../options.h"
#include "core/log/log.h"
#include "lib/Math2D/Math2DStringUtils.h"
#include "libs/graphics/sdl/pure.h" // for enumerating renderer backends.
#include "uqm/init.h"				// for NUM_PLAYERS, which we need to know to set up netplay options. Maybe we should move that somewhere else?

//#include "getopt/getopt.h"

#include "options/SettingsCli.h"
#ifdef NETPLAY
#include "options/netoptions.h" // for NETPLAY options
#endif
/*
static int InvalidArgument(const char* supplied, const char* opt_name)
{
	error::saveError("Invalid argument '{}' to option {}.", supplied, opt_name);
	return EXIT_FAILURE;
}

enum
{
	CSCAN_OPT = 1000,
	MENU_OPT,
	FONT_OPT,
	SHIELD_OPT,
	SCROLL_OPT,
	SOUND_OPT,
	STEREOSFX_OPT,
	ADDON_OPT,
	ADDONDIR_OPT,
	ACCEL_OPT,
	SAFEMODE_OPT,
	RENDERER_OPT,
	CHEATMODE_OPT,
	GODMODE_OPT,
	TDM_OPT,
	BWARP_OPT,
	UNLOCKSHIPS_OPT,
	HEADSTART_OPT,
	UPGRADES_OPT,
	INFINITERU_OPT,
	SKIPINTRO_OPT,
	MENUMUS_OPT,
	NEBU_OPT,
	ORBITS_OPT,
	TEXTPLAN_OPT,
	DATE_OPT,
	INFFUEL_OPT,
	PICKUP_OPT,
	SUBMENU_OPT,
	INFCRED_OPT,
	CUSTBORD_OPT,
	SEEDTYPE_OPT,
	EXSEED_OPT,
	SHIPSEED_OPT,
	SPHERECOLORS_OPT,
	SPACEMUSIC_OPT,
	WHOLEFUEL_OPT,
	DIRJOY_OPT,
	LANDHOLD_OPT,
	SCRTRANS_OPT,
	DIFFICULTY_OPT,
	FUELRANGE_OPT,
	EXTENDED_OPT,
	NOMAD_OPT,
	GAMEOVER_OPT,
	SHIPDIRIP_OPT,
	HAZCOLORS_OPT,
	ORZFONT_OPT,
	CONTYPE_OPT,
	SISFACEHS_OPT,
	COLORPLAN_OPT,
	PLANSTYLE_OPT,
	STARBACK_OPT,
	SCANSTYLE_OPT,
	OSCILLO_OPT,
	OSCSTYLE_OPT,
	HYPERSTARS_OPT,
	LANDSTYLE_OPT,
	PLANTEX_OPT,
	SISENGINE_OPT,
	NOHSENC_OPT,
	DECLEANSE_OPT,
	NOMELEEOBJ_OPT,
	SHOWSTARS_OPT,
	UNSCALEDSS_OPT,
	SCANSPH_OPT,
	SLAUGHTER_OPT,
	SISADVAP_OPT,
	MELEETIPS_OPT,
	MUSICRESUME_OPT,
	WINDTYPE_OPT,
	SCATTERELEMS_OPT,
	SHOWUPG_OPT,
	FLTPTSYS_OPT,
	SHIPSTORE_OPT,
	CAPTNAMES_OPT,
	DOSMENUS_OPT,
	MELEE_OPT,
	LOADGAME_OPT,
	NEBUVOL_OPT,
	CLAPAK_OPT,
#ifdef NETPLAY
	NETHOST1_OPT,
	NETPORT1_OPT,
	NETHOST2_OPT,
	NETPORT2_OPT,
	NETDELAY_OPT,
#endif
};

constexpr const char* optString = "+r:f:oc:b:spC:n:?hM:S:T:q:ug:l:i:vwxk";
struct option longOptions[] =
	{
		{"res",				1, nullptr, 'r'			   },
		{"fullscreen",		   1, nullptr, 'f'			  },
		{"opengl",			   0, nullptr, 'o'			  },
		{"scale",			  1, nullptr, 'c'			 },
		{"meleezoom",		  1, nullptr, 'b'			 },
		{"scanlines",		  0, nullptr, 's'			 },
		{"fps",				0, nullptr, 'p'			   },
		{"configdir",		  1, nullptr, 'C'			 },
		{"contentdir",		   1, nullptr, 'n'			  },
		{"help",				 0, nullptr, 'h'				},
		{"musicvol",			 1, nullptr, 'M'				},
		{"sfxvol",			   1, nullptr, 'S'			  },
		{"speechvol",		  1, nullptr, 'T'			 },
		{"audioquality",		 1, nullptr, 'q'				},
		{"nosubtitles",		0, nullptr, 'u'			   },
		{"gamma",			  1, nullptr, 'g'			 },
		{"logfile",			1, nullptr, 'l'			   },
		{"intro",			  1, nullptr, 'i'			 },
		{"version",			0, nullptr, 'v'			   },
		{"windowed",			 0, nullptr, 'w'				},
		{"nogl",				 0, nullptr, 'x'				},
		{"keepaspectratio",	0, nullptr, 'k'			   },

		//  options with no short equivalent:
		{"cscan",			  1, nullptr, CSCAN_OPT	   },
		{"menu",				 1, nullptr, MENU_OPT		 },
		{"font",				 1, nullptr, FONT_OPT		 },
		{"shield",			   1, nullptr, SHIELD_OPT		 },
		{"scroll",			   1, nullptr, SCROLL_OPT		 },
		{"sound",			  1, nullptr, SOUND_OPT	   },
		{"stereosfx",		  0, nullptr, STEREOSFX_OPT   },
		{"addon",			  1, nullptr, ADDON_OPT	   },
		{"addondir",			 1, nullptr, ADDONDIR_OPT	 },
		{"accel",			  1, nullptr, ACCEL_OPT	   },
		{"safe",				 0, nullptr, SAFEMODE_OPT	 },
		{"renderer",			 1, nullptr, RENDERER_OPT	 },
		{"kohrstahp",		  0, nullptr, CHEATMODE_OPT   },
		{"precursormode",	  1, nullptr, GODMODE_OPT	 },
		{"timedilation",		 1, nullptr, TDM_OPT			},
		{"bubblewarp",		   0, nullptr, BWARP_OPT		},
		{"unlockships",		0, nullptr, UNLOCKSHIPS_OPT },
		{"headstart",		  0, nullptr, HEADSTART_OPT   },
		{"unlockupgrades",	   0, nullptr, UPGRADES_OPT	   },
		{"infiniteru",		   0, nullptr, INFINITERU_OPT	 },
		{"skipintro",		  0, nullptr, SKIPINTRO_OPT   },
		{"mainmenumusic",	  0, nullptr, MENUMUS_OPT	 },
		{"nebulae",			0, nullptr, NEBU_OPT		},
		{"orbitingplanets",	0, nullptr, ORBITS_OPT	  },
		{"texturedplanets",	0, nullptr, TEXTPLAN_OPT	},
		{"dateformat",		   1, nullptr, DATE_OPT		   },
		{"infinitefuel",		 0, nullptr, INFFUEL_OPT		},
		{"partialpickup",	  0, nullptr, PICKUP_OPT		},
		{"submenu",			0, nullptr, SUBMENU_OPT	   },
		{"infinitecredits",	0, nullptr, INFCRED_OPT	   },
		{"customborder",		 0, nullptr, CUSTBORD_OPT	 },
		{"seedtype",			 0, nullptr, SEEDTYPE_OPT	 },
		{"customseed",		   1, nullptr, EXSEED_OPT		 },
		{"shipseed",			 0, nullptr, SHIPSEED_OPT	 },
		{"spherecolors",		 0, nullptr, SPHERECOLORS_OPT},
		{"spacemusic",		   1, nullptr, SPACEMUSIC_OPT	 },
		{"wholefuel",		  0, nullptr, WHOLEFUEL_OPT   },
		{"dirjoystick",		0, nullptr, DIRJOY_OPT	  },
		{"landerhold",		   0, nullptr, LANDHOLD_OPT	   },
		{"scrtrans",			 1, nullptr, SCRTRANS_OPT	 },
		{"melee",			  0, nullptr, MELEE_OPT	   },
		{"loadgame",			 0, nullptr, LOADGAME_OPT	 },
		{"difficulty",		   1, nullptr, DIFFICULTY_OPT	 },
		{"fuelrange",		  1, nullptr, FUELRANGE_OPT   },
		{"extended",			 0, nullptr, EXTENDED_OPT	 },
		{"nomad",			  1, nullptr, NOMAD_OPT	   },
		{"gameover",			 0, nullptr, GAMEOVER_OPT	 },
		{"shipdirectionip",	0, nullptr, SHIPDIRIP_OPT	 },
		{"hazardcolors",		 0, nullptr, HAZCOLORS_OPT	  },
		{"orzcompfont",		0, nullptr, ORZFONT_OPT	   },
		{"smartautopilot",	   0, nullptr, SISFACEHS_OPT	},
		{"tintplansphere",	   1, nullptr, COLORPLAN_OPT	},
		{"planetstyle",		1, nullptr, PLANSTYLE_OPT	 },
		{"starbackground",	   1, nullptr, STARBACK_OPT	   },
		{"scanstyle",		  1, nullptr, SCANSTYLE_OPT   },
		{"nonstoposcill",	  0, nullptr, OSCILLO_OPT	 },
		{"scopestyle",		   1, nullptr, OSCSTYLE_OPT	   },
		{"animhyperstars",	   0, nullptr, HYPERSTARS_OPT	 },
		{"landerview",		   1, nullptr, LANDSTYLE_OPT	},
		{"planettexture",	  1, nullptr, PLANTEX_OPT	 },
		{"sisenginecolor",	   1, nullptr, SISENGINE_OPT	},
		{"nohqencounters",	   0, nullptr, NOHSENC_OPT	  },
		{"decleanse",		  0, nullptr, DECLEANSE_OPT   },
		{"nomeleeobstacles",	 0, nullptr, NOMELEEOBJ_OPT  },
		{"showvisitstars",	   0, nullptr, SHOWSTARS_OPT	},
		{"unscaledstarsystem", 0, nullptr, UNSCALEDSS_OPT	 },
		{"spheretype",		   1, nullptr, SCANSPH_OPT	  },
		{"nebulaevol",		   1, nullptr, NEBUVOL_OPT	  },
		{"slaughtermode",	  0, nullptr, SLAUGHTER_OPT   },
		{"advancedautopilot",  0, nullptr, SISADVAP_OPT	  },
		{"meleetooltips",	  0, nullptr, MELEETIPS_OPT   },
		{"musicresume",		1, nullptr, MUSICRESUME_OPT },
		{"windowtype",		   1, nullptr, WINDTYPE_OPT	   },
		{"noclassic",		  0, nullptr, CLAPAK_OPT		},
		{"scatterelements",	0, nullptr, SCATTERELEMS_OPT},
		{"showupgrades",		 0, nullptr, SHOWUPG_OPT		},
		{"fleetpointsys",	  0, nullptr, FLTPTSYS_OPT	  },
		{"shipstore",		  0, nullptr, SHIPSTORE_OPT   },
		{"captainnames",		 0, nullptr, CAPTNAMES_OPT	  },
		{"dosmenus",			 0, nullptr, DOSMENUS_OPT	 },
#ifdef NETPLAY
		{"nethost1",			 1, nullptr, NETHOST1_OPT	 },
		{"netport1",			 1, nullptr, NETPORT1_OPT	 },
		{"nethost2",			 1, nullptr, NETHOST2_OPT	 },
		{"netport2",			 1, nullptr, NETPORT2_OPT	 },
		{"netdelay",			 1, nullptr, NETDELAY_OPT	 },
#endif
		{0,					0, 0,		  0			   }
};
*/
namespace uqm
{

template <typename OptionT, typename OptionValueT = typename OptionT::value_type>
inline void setOption(OptionT& option, OptionValueT value)
{
	option = value;
}

template <typename OptionT>
bool setOption(OptionT& option, uqgsl::czstring strval, uqgsl::czstring optionName)
{
	if (const auto temp {parseOptionValue<typename OptionT::value_type>(strval, optionName)}; temp.has_value())
	{
		option = *temp;
		return true;
	}
	return false;
}

// returns true is value was found and set successfully
template <typename OptionT, typename OptionListT>
bool setListOption(OptionT& option, uqgsl::czstring strval, const OptionListT& list)
{
	if (const auto val {getOptionListValue(list, strval)}; val.has_value())
	{
		option = *val;
		return true;
	}

	return false;
}

template <typename OptionT>
inline bool setChoiceOption(OptionT& option, uqgsl::czstring strval)
{
	return setListOption(option, strval, EmulationChoiceList);
}

bool setVolumeOption(FloatOption& option, uqgsl::czstring strval, uqgsl::czstring optName)
{
	if (const auto optionIntValue {parseOptionValue<int>(strval, optName)}; optionIntValue.has_value())
	{
		option.value = normalizeValue(*optionIntValue, 100);
		option.set = true;
		return true;
	}
	return false;
}

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
	pathGroup.add_option("-l,--logfile", m_options.logFile, "Path to the log file to create. If not specified, no log will be generated.")
		->capture_default_str();
	pathGroup.add_option("--addon", m_options.addons, "Addons to load from the addons directory. Any number may be specified")
		->expected(-1)
		->capture_default_str();

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

//	uqm::log::info("  --unlockupgrades : Unlocks every upgrade for your flagship and landers.. Default={}", defaults.unlockUpgrades.toString());
//	uqm::log::info("  --infiniteru : Gives you infinite R.U. as long as the cheat is on. Default={}", defaults.infiniteRU.toString());
//	uqm::log::info("  --skipintro : Skips the intro and Logo fmv. Default={}", defaults.skipIntro.toString());
//	uqm::log::info("  --mainmenumusic : Switches the main menu music on/off. Default={}", defaults.mainMenuMusic.toString());
//	uqm::log::info("  --nebulae : Enables/Disables nebulae in star systems. Default={}", defaults.nebulae.toString());
//	uqm::log::info("  --orbitingplanets : Enables/Disables orbiting planets in star systems. Default={}", defaults.orbitingPlanets.toString());
//	uqm::log::info("  --texturedplanets : Enables/Disables textured planets in star systems. Default={}", defaults.texturedPlanets.toString());
//	uqm::log::info("  --infinitefuel : Infinite fuel in the main game. Default={}", defaults.infiniteFuel.toString());
//	uqm::log::info("  --partialpickup : Enables/Disables partial mineral pickup . Default={}", defaults.partialPickup.toString());
//	uqm::log::info("  --submenu : Enables/Disables mineral and star map keys submenu . Default={}", defaults.submenu.toString());
//	uqm::log::info("  --dateformat : 0: MMM DD.YYYY | 1: MM.DD.YYYY | 2: DD MMM.YYYY | 3: DD.MM.YYYY (default: 0)");
//	uqm::log::info("  --infinitecredits: Gives you infinite Melnorme Credits . Default={}", defaults.infiniteCredits.toString());
//	uqm::log::info("  --melee : Takes you straight to Super Melee after the splash screen.");
//	uqm::log::info("  --loadgame : Takes you straight to the Load Game sceen after the splash screen.");
//	uqm::log::info("  --customborder : Enables the custom border frame.. Default={}", defaults.customBorder.toString());
//	uqm::log::info("  --seedtype: 0: Default seed | 1: Seed planets  | 2: Seed Melnorme/Rainbow/Quasispace  | 3: Seed Starmap (default: 0)");
//	uqm::log::info("  --customseed=# : Allows you to customize the internal seed used to generate the solar systems in-game. (default: 16807)");
//	uqm::log::info("  --shipseed: Seed the ships assigned to each race. Uses --customseed value (default {})", defaults.shipSeed.toString());
//	uqm::log::info("  --spherecolors: 0: Default colors | 1: StarSeed colors (default: 0)");
//	uqm::log::info("  --spacemusic #: Enables localized music for aliens when you are in their sphere of influence\n0: Default (OFF) | 1: No Spoilers | 2: Spoilers");
//	uqm::log::info("  --wholefuel : Enables the display of the whole fuel value in the ship status. Default={}", defaults.wholeFuel.toString());
//	uqm::log::info("  --dirjoystick : Enables the use of directional joystick controls for Android. Default={}", defaults.directionalJoystick.toString());
//	uqm::log::info("  --landerhold : Switch between PC/3DO max lander hold, pc=64, 3do=50. Default={}", defaults.landerHold.toString());
//	uqm::log::info("  --scrtrans : Screen transitions, pc=instantaneous, 3do=crossfade. Default={}", defaults.scrTrans.toString());
//	uqm::log::info("  --difficulty : 0: Normal | 1: Easy | 2: Hard | 3: Choose at Start (default: 0)");
//	uqm::log::info("  --fuelrange : Enables extra fuel range indicators : 0: No indicators | 1: Fuel range at destination | 2: Remaining fuel range to Sol | 3: Both option 1 and 2  enabled simultaneously (default: 0)");
//	uqm::log::info("  --extended : Enables Extended Edition features. Default={}", defaults.extended.toString());
//	uqm::log::info("  --nomad : Enables 'Nomad Mode' (No Starbase) : 0: Off | 1: Easy | 2: Normal (default: 0)");
//	uqm::log::info("  --gameover : Enables Game Over cutscenes. Default={}", defaults.gameOver.toString());
//	uqm::log::info("  --shipdirectionip : Enable NPC ships in IP to face their direction of travel. Default={}", defaults.shipDirectionIP.toString());
//	uqm::log::info("  --hazardcolors : Enable colored text based on hazard severity when viewing planetary scans. Default={}", defaults.hazardColors.toString());
//	uqm::log::info("  --orzcompfont : Enable alternate font for untranslatable Orz speech. Default={}", defaults.orzCompFont.toString());
//	uqm::log::info("  --smartautopilot : Activating Auto-Pilot within Solar System pilots the Flagship out via the shortest route.. Default={}", defaults.smartAutoPilot.toString());
//	uqm::log::info("  --controllertype : 0: Keyboard | 1: Xbox | 2: PlayStation 4 (default: 0)");
//	uqm::log::info("  --tintplansphere : Tint the planet sphere with scan color during scan. Default={}", defaults.tintPlanSphere.toString());
//	uqm::log::info("  --planetstyle : Choose between PC or 3DO planet color and shading. Default={}", defaults.planetStyle.toString());
//	uqm::log::info("  --starbackground : Set the background stars in solar system between PC, 3DO, UQM, or HD-mod patterns (default: pc)");
//	uqm::log::info("  --scanstyle : Choose between PC or 3DO scanning types. Default={}", defaults.scanStyle.toString());
//	uqm::log::info("  --nonstoposcill : Oscilloscope uses both voice and music data. Default={}", defaults.nonStopOscill.toString());
//	uqm::log::info("  --scopestyle : Choose between either the PC or 3DO oscilloscope type. Default={}", defaults.scopeStyle.toString());
//	uqm::log::info("  --animhyperstars : HD only - Use old HD-mod animated HyperSpace stars. Default={}", defaults.hyperStars.toString());
//	uqm::log::info("  --landerview : Choose between either the PC or 3DO lander view. Default={}", defaults.landerStyle.toString());
//	uqm::log::info("  --planettexture : Choose between either 3DO or UQM planet map texture [when not using custom seed] (default: 3do)");
//	uqm::log::info("  --sisenginecolor : Choose between either the PC or 3DO Flagship engine color. Default={}", defaults.flagshipColor.toString());
//	uqm::log::info("  --nohqencounters : Disables HyperSpace encounters. Default={}", defaults.noHQEncounters.toString());
//	uqm::log::info("  --decleanse : Moves the Death March 100 years ahead from its actual start date [does not work once the Death March has started]. Default={}", defaults.deCleansing.toString());
//	uqm::log::info("  --nomeleeobstacles : Removes the planet and asteroids from Super Melee. Default={}", defaults.meleeObstacles.toString());
//	uqm::log::info("  --showvisitstars : Dim visited stars on the StarMap and encase the star name in parenthesis. Default={}", defaults.showVisitedStars.toString());
//	uqm::log::info("  --unscaledstarsystem : Show the classic HD-mod Beta Star System view. Default={}", defaults.unscaledStarSystem.toString());
//	uqm::log::info("  --spheretype : Choose between PC, 3DO, or UQM scan sphere styles. Default={}", defaults.sphereType.toString());
//	uqm::log::info("  --nebulaevol=VOLUME (0-50, default 11)");
//	uqm::log::info("  --slaughtermode : Affect a race's SOI by destroying their ships in battle. Default={}", defaults.slaughterMode.toString());
//	uqm::log::info("  --advancedautopilot : Finds the route that uses the least amount of fuel through HyperSpace or QuasiSpace and Auto-Pilots the Flagship on the best route. Default={}", defaults.advancedAutoPilot.toString());
//	uqm::log::info("  --meleetooltips : Show SC1-style ship description tooltips at the bottom of the Super-Melee screen when picking a ship for your fleet. Default={}", defaults.meleeToolTips.toString());
//	uqm::log::info("  --musicresume : Resumes the music in UQM where it last left off : 0: Off | 1: 5 Minutes | 2: Indefinite (default: 0)");
//	uqm::log::info("  --windowtype : Choose between DOS, 3DO or UQM window types : 0: DOS | 1: 3DO | 2: UQM (default: 0)");
//	uqm::log::info("  --scatterelements : Scatter a percentage of the elements in the lander's cargo hold onto the planet's surface when the lander explodes. Default={}", defaults.scatterElements.toString());
//	uqm::log::info("  --showupgrades : Show lander upgrade graphics when exploring planets. Default={}", defaults.showUpgrades.toString());
//	uqm::log::info("  --fleetpointsys : Restrict the amount of ships that can be purchased via their melee points. Default={}", defaults.fleetPointSys.toString());
//	uqm::log::info("  --shipstore : Enable a storage queue accessed at the shipyard. Default={}", defaults.shipStore.toString());
//	uqm::log::info("  --captainnames : Display captain names at shipyard. Default={}", defaults.captainNames.toString());
//	uqm::log::info("  --dosmenus : Display DOS style menu in shipyard in place of SIS window. Default={}", defaults.dosMenus.toString());
//
//
//	int optionIndex;
//	bool badArg = false;
//
//	opterr = 0;
//
//	if (args.empty())
//	{
//		error::saveError("Error: Bad command line.");
//		return EXIT_FAILURE;
//	}
//
//#ifdef __APPLE__
//	// If we are launched by double-clicking an application bundle, Finder
//	// sticks a "-psn_<some_number>" argument into the list, which makes
//	// getopt extremely unhappy. Check for this case and wipe out the
//	// entire command line if it looks like it happened.
//	if ((argc >= 2) && (strncmp(argv[1], "-psn_", 5) == 0))
//	{
//		return EXIT_SUCCESS;
//	}
//#endif
//
//	while (!badArg)
//	{
//		int c;
//		optionIndex = -1;
//		c = getopt_long(args.size(), args.data(), optString, longOptions, &optionIndex);
//		if (c == -1)
//		{
//			break;
//		}
//
//
//		switch (c)
//		{
//			case '?':
//				if (optopt != '?')
//				{
//					error::saveError("Invalid option or its argument \"%c\"", c);
//					badArg = true;
//					break;
//				}
//				// fall through
//			case 'h':
//				options.runMode = RunMode::Usage;
//				return EXIT_SUCCESS;
//			case 'v':
//				options.runMode = RunMode::Version;
//				return EXIT_SUCCESS;
//			case 'r':
//				{
//					int width {};
//					int height {};
//					if (sscanf(optarg, "%dx%d", &width, &height) != 2)
//					{
//						error::saveError("Error: invalid argument \"{}\" specified as resolution.", optarg);
//						badArg = true;
//						break;
//					}
//					options.resolution.value.width = width;
//					options.resolution.value.height = height;
//					options.resolution.set = true;
//					break;
//				}
//			case 'f':
//				{
//					if (const auto fsValue {parseOptionValue<int>(optarg, "Fullscreen")}; fsValue.has_value())
//					{
//						if (*fsValue < 0 || *fsValue > 2)
//						{
//							error::saveError("\nFullscreen has to be 0, 1, or 2. Not {}\n", *fsValue);
//							badArg = true;
//						}
//						else
//						{
//							options.fullscreen = *fsValue;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case 'w':
//				options.fullscreen = 0;
//				break;
//			case 'o':
//				options.opengl = true;
//				break;
//			case 'x':
//				options.opengl = false;
//				break;
//			case 'k':
//				options.keepAspectRatio = true;
//				break;
//			case 'c':
//				if (!setListOption(options.scaler, optarg, ScalerList))
//				{
//					InvalidArgument(optarg, "--scale or -c");
//					badArg = true;
//				}
//				break;
//			case 'b':
//				if (!setListOption(options.meleeScale, optarg, MeleeScaleList))
//				{
//					InvalidArgument(optarg, "--meleezoom or -b");
//					badArg = true;
//				}
//				break;
//			case 's':
//				options.scanlines = true;
//				break;
//			case 'p':
//				options.showFps = true;
//				break;
//			case 'n':
//				options.contentDir = optarg;
//				break;
//			case 'M':
//				if (!setVolumeOption(options.musicVolumeScale, optarg, "music volume"))
//				{
//					badArg = true;
//				}
//				break;
//			case 'S':
//				if (!setVolumeOption(options.sfxVolumeScale, optarg, "sfx volume"))
//				{
//					badArg = true;
//				}
//				break;
//			case 'T':
//				if (!setVolumeOption(options.speechVolumeScale, optarg, "speech volume"))
//				{
//					badArg = true;
//				}
//				break;
//			case 'q':
//				if (!setListOption(options.soundQuality, optarg, AudioQualityList))
//				{
//					InvalidArgument(optarg, "--audioquality or -q");
//					badArg = true;
//				}
//				break;
//			case 'u':
//				options.subtitles = false;
//				break;
//			case 'g':
//				if (!setOption(options.gamma, optarg, "gamma correction"))
//				{
//					badArg = true;
//				}
//				break;
//			case 'l':
//				options.logFile = optarg;
//				break;
//			case 'C':
//				options.configDir = optarg;
//				break;
//			case 'i':
//				if (!setOption(options.whichIntro, optarg, "introduction"))
//				{
//					InvalidArgument(optarg, "--intro or -i");
//					badArg = true;
//				}
//				break;
//			case CSCAN_OPT:
//				{
//					int temp;
//					if (const auto scanVal {parseOptionValue<int>(optarg, "Coarse Scans")}; scanVal.has_value())
//					{
//						if (temp < 0 || temp > 2)
//						{
//							error::saveError("\nCoarse Scan has to be 0, 1, 2 or 3.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.whichCoarseScan = temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case MENU_OPT:
//				if (!parseOption(options.whichMenu, optarg, "Menu Emulation"))
//				{
//					InvalidArgument(optarg, "--menu");
//					badArg = true;
//				}
//				break;
//			case FONT_OPT:
//				if (!parseOption(options.whichFonts, optarg, "Font Emulation"))
//				{
//					InvalidArgument(optarg, "--font");
//					badArg = true;
//				}
//				break;
//			case SHIELD_OPT:
//				if (!parseOption(options.whichShield, optarg, "Shield Emulation"))
//				{
//					InvalidArgument(optarg, "--shield");
//					badArg = true;
//				}
//				break;
//			case SCROLL_OPT:
//				if (!parseOption(options.smoothScroll, optarg, "Scrolling emulation"))
//				{
//					InvalidArgument(optarg, "--scroll");
//					badArg = true;
//				}
//				break;
//			case SOUND_OPT:
//				if (!setListOption(options.soundDriver, optarg, AudioDriverList))
//				{
//					InvalidArgument(optarg, "--sound");
//					badArg = true;
//				}
//				break;
//			case STEREOSFX_OPT:
//				options.stereoSFX = true;
//				break;
//			case ADDON_OPT:
//				options.addons.push_back(optarg);
//				break;
//			case ADDONDIR_OPT:
//				options.addonDir = optarg;
//				break;
//			case ACCEL_OPT:
//				{
//					if (const auto listValue {getOptionListValue(AccelList, optarg)}; listValue.has_value())
//					{
//						force_platform = static_cast<PLATFORM_TYPE>(*listValue);
//					}
//					else
//					{
//						InvalidArgument(optarg, "--accel");
//						badArg = true;
//					}
//					break;
//				}
//			case SAFEMODE_OPT:
//				options.safeMode = true;
//				break;
//			case RENDERER_OPT:
//				options.graphicsBackend = optarg;
//				break;
//			case CHEATMODE_OPT:
//				setOption(options.cheatMode, true);
//				break;
//			case GODMODE_OPT:
//				{
//					int temp;
//					if (const auto godModeVal {parseOptionValue<int>(optarg, "God Modes")}; godModeVal.has_value())
//					{
//						if (temp < 0 || temp > 2)
//						{
//							error::saveError("God Mode has to be 0, 1, or 2. Got {}", *godModeVal);
//							badArg = true;
//						}
//						else
//						{
//							options.optGodModes = *godModeVal;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case TDM_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Time Dilation scale")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 2)
//						{
//							error::saveError("\nTime Dilation scale has to be "
//											 "0, 1, or 2.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.timeDilationScale = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case BWARP_OPT:
//				setOption(options.bubbleWarp, true);
//				break;
//			case UNLOCKSHIPS_OPT:
//				setOption(options.unlockShips, true);
//				break;
//			case HEADSTART_OPT:
//				setOption(options.headStart, true);
//				break;
//			case UPGRADES_OPT:
//				//setBoolOption (&options.unlockUpgrades, true);
//				break;
//			case INFINITERU_OPT:
//				setOption(options.infiniteRU, true);
//				break;
//			case SKIPINTRO_OPT:
//				setOption(options.skipIntro, true);
//				break;
//			case MENUMUS_OPT:
//				setOption(options.mainMenuMusic, true);
//				break;
//			case NEBU_OPT:
//				setOption(options.nebulae, true);
//				break;
//			case ORBITS_OPT:
//				setOption(options.orbitingPlanets, true);
//				break;
//			case TEXTPLAN_OPT:
//				setOption(options.texturedPlanets, true);
//				break;
//			case DATE_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Date Format")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 3)
//						{
//							error::saveError("\nDate Format has to be 0, 1, 2, or 3.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.optDateFormat = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//						break;
//					}
//					break;
//				}
//			case INFFUEL_OPT:
//				setOption(options.infiniteFuel, true);
//				break;
//			case PICKUP_OPT:
//				setOption(options.partialPickup, true);
//				break;
//			case SUBMENU_OPT:
//				setOption(options.submenu, true);
//				break;
//			case INFCRED_OPT:
//				setOption(options.infiniteCredits, true);
//				break;
//			case CUSTBORD_OPT:
//				setOption(options.customBorder, true);
//				break;
//			case SEEDTYPE_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Seed Type")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 3)
//						{
//							error::saveError("\nSeed Type has to be 0, 1, 2, or 3.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.seedType = static_cast<SeedType>(*temp);
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case EXSEED_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Custom Seed")}; temp.has_value())
//					{
//						if (!SANE_SEED(*temp))
//						{
//							error::saveError("Custom Seed can not be less than {} or greater than {}.", MIN_SEED, MAX_SEED);
//							badArg = true;
//						}
//						else
//						{
//							options.customSeed = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case SHIPSEED_OPT:
//				setOption(options.shipSeed, true);
//				break;
//			case SPHERECOLORS_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Sphere Colors")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 1)
//						{
//							error::saveError("\nSphere Colors has to 0 or 1.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.sphereColors = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case SPACEMUSIC_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Ambient Space Music")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 2)
//						{
//							error::saveError("\nAmbient Space Music has to be "
//											 "0, 1, or 2.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.spaceMusic = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//						break;
//					}
//					break;
//				}
//			case WHOLEFUEL_OPT:
//				setOption(options.wholeFuel, true);
//				break;
//			case DIRJOY_OPT:
//				setOption(options.directionalJoystick, true);
//				break;
//			case LANDHOLD_OPT:
//				if (!setChoiceOption(options.landerHold, optarg))
//				{
//					InvalidArgument(optarg, "--landerhold");
//					badArg = true;
//				}
//				break;
//			case SCRTRANS_OPT:
//				if (!setChoiceOption(options.scrTrans, optarg))
//				{
//					InvalidArgument(optarg, "--scrtrans");
//					badArg = true;
//				}
//				break;
//			case DIFFICULTY_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Difficulty")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 3)
//						{
//							error::saveError("\nDifficulty has to be 0, 1, 2, or 3.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.optDiffChooser = options.optDifficulty = *temp;
//							if (temp > 2)
//							{
//								options.optDifficulty = 0;
//							}
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case FUELRANGE_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Fuel range indicator")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 3)
//						{
//							error::saveError("\nFuel range indicator has to be 0, 1, 2,"
//											 " or 3.\n");
//							badArg = true;
//						}
//						else
//						{
//							options.optFuelRange = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case EXTENDED_OPT:
//				setOption(options.extended, true);
//				break;
//			case NOMAD_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Nomad Mode type")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 2)
//						{
//							error::saveError("\nNomad Mode has to be 0, 1, or 2.");
//							badArg = true;
//						}
//						else
//						{
//							options.nomad = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case GAMEOVER_OPT:
//				setOption(options.gameOver, true);
//				break;
//			case SHIPDIRIP_OPT:
//				setOption(options.shipDirectionIP, true);
//				break;
//			case HAZCOLORS_OPT:
//				setOption(options.hazardColors, true);
//				break;
//			case ORZFONT_OPT:
//				setOption(options.orzCompFont, true);
//				break;
//			case CONTYPE_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Controller Type")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 2)
//						{
//							error::saveError("Controller type has to be 0, 1, or 2.");
//							badArg = true;
//						}
//						else
//						{
//							options.optControllerType = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case SISFACEHS_OPT:
//				setOption(options.smartAutoPilot, true);
//				break;
//			case COLORPLAN_OPT:
//				if (!setOption(options.tintPlanSphere, optarg, "Planet sphere tint"))
//				{
//					InvalidArgument(optarg, "--tintplansphere");
//					badArg = true;
//				}
//				break;
//			case PLANSTYLE_OPT:
//				if (!setOption(options.planetStyle, optarg, "Planet style"))
//				{
//					InvalidArgument(optarg, "--planetstyle");
//					badArg = true;
//				}
//				break;
//			case STARBACK_OPT:
//				{
//					if (const auto temp {parseOptionValue<int>(optarg, "Star Background")}; temp.has_value())
//					{
//						if (*temp < 0 || *temp > 3)
//						{
//							error::saveError("Star background has to be between 0-3.");
//							badArg = true;
//						}
//						else
//						{
//							options.starBackground = *temp;
//						}
//					}
//					else
//					{
//						badArg = true;
//					}
//					break;
//				}
//			case SCANSTYLE_OPT:
//				if (!setOption(options.scanStyle, optarg, "Scan style"))
//				{
//					InvalidArgument(optarg, "--scanstyle");
//					badArg = true;
//				}
//				break;
//			case OSCILLO_OPT:
//				setOption(options.nonStopOscill, true);
//				break;
//			case OSCSTYLE_OPT:
//				if (!setOption(options.scopeStyle, optarg, "Oscilloscope style"))
//				{
//					InvalidArgument(optarg, "--scopestyle");
//					badArg = true;
//				}
//				break;
//			case HYPERSTARS_OPT:
//				setOption(options.hyperStars, true);
//				break;
//			case LANDSTYLE_OPT:
//				if (!setOption(options.landerStyle, optarg, "Lander view"))
//				{
//					InvalidArgument(optarg, "--landerview");
//					badArg = true;
//				}
//				break;
//			case PLANTEX_OPT:
//				if (strcmp(optarg, "uqm") == 0)
//				{
//					setOption(options.planetTexture, true);
//				}
//				else if (strcmp(optarg, "3do") == 0)
//				{
//					setOption(options.planetTexture, false);
//				}
//				else
//				{
//					InvalidArgument(optarg, "--planettexture");
//					error::saveError("Planet Texture can only be set to '3do' or 'uqm'");
//					badArg = true;
//				}
//				break;
//			case SISENGINE_OPT:
//				if (!setChoiceOption(options.flagshipColor, optarg))
//				{
//					InvalidArgument(optarg, "--sisenginecolor");
//					badArg = true;
//				}
//				break;
//			case NOHSENC_OPT:
//				setOption(options.noHQEncounters, true);
//				break;
//			case DECLEANSE_OPT:
//				setOption(options.deCleansing, true);
//				break;
//			case NOMELEEOBJ_OPT:
//				setOption(options.meleeObstacles, true);
//				break;
//			case SHOWSTARS_OPT:
//				setOption(options.showVisitedStars, true);
//				break;
//			case UNSCALEDSS_OPT:
//				setOption(options.unscaledStarSystem, true);
//				break;
//			case SCANSPH_OPT:
//				if (const auto temp {parseOptionValue<int>(optarg, "Sphere Type")}; temp.has_value())
//				{
//					if (*temp < 0 || *temp > 2)
//					{
//						error::saveError("Sphere Type has to be between 0-2");
//						badArg = true;
//					}
//					else
//					{
//						options.sphereType = *temp;
//					}
//				}
//				else
//				{
//					badArg = true;
//				}
//				break;
//SLAUGHTER_OPT:
//				setOption(options.slaughterMode, true);
//				break;
//			case SISADVAP_OPT:
//				setOption(options.advancedAutoPilot, true);
//				break;
//			case MELEETIPS_OPT:
//				setOption(options.meleeToolTips, true);
//				break;
//			case MUSICRESUME_OPT:
//				if (const auto temp {parseOptionValue<int>(optarg, "Music Resume")}; temp.has_value())
//				{
//					if (*temp < 0 || *temp > 2)
//					{
//						error::saveError("Music Resume has to be between 0-2");
//						badArg = true;
//					}
//					else
//					{
//						options.musicResume = *temp;
//					}
//				}
//				else
//				{
//					badArg = true;
//				}
//				break;
//			case WINDTYPE_OPT:
//				if (const auto temp {parseOptionValue<int>(optarg, "Window Type")}; temp.has_value())
//				{
//					if (*temp < 0 || *temp > 2)
//					{
//						error::saveError("Window type has to be between 0-2");
//						badArg = true;
//					}
//					else
//					{
//						options.windowType = *temp;
//					}
//				}
//				else
//				{
//					badArg = true;
//				}
//				break;
//			case SCATTERELEMS_OPT:
//				setOption(options.scatterElements, true);
//				break;
//			case SHOWUPG_OPT:
//				setOption(options.showUpgrades, true);
//				break;
//			case FLTPTSYS_OPT:
//				setOption(options.fleetPointSys, true);
//				break;
//			case SHIPSTORE_OPT:
//				setOption(options.shipStore, true);
//				break;
//			case CAPTNAMES_OPT:
//				setOption(options.captainNames, true);
//				break;
//			case DOSMENUS_OPT:
//				setOption(options.dosMenus, true);
//				break;
//			case MELEE_OPT:
//				optSuperMelee = true;
//				break;
//			case LOADGAME_OPT:
//				optLoadGame = true;
//				break;
//			case NEBUVOL_OPT:
//				if (const auto temp {parseOptionValue<int>(optarg, "Nebulae Volume")}; temp.has_value())
//				{
//					if (*temp < 0 || *temp > 50)
//					{
//						error::saveError("Nebulae volume has to be between 0-50");
//						badArg = true;
//					}
//					else
//					{
//						options.nebulaevol = *temp;
//					}
//				}
//				else
//				{
//					badArg = true;
//				}
//				break;
//			case CLAPAK_OPT:
//				optNoClassic = true;
//				break;
//#ifdef NETPLAY
//			case NETHOST1_OPT:
//				g_netplayOptions.peer[0].isServer = false;
//				g_netplayOptions.peer[0].host = optarg;
//				break;
//			case NETPORT1_OPT:
//				g_netplayOptions.peer[0].port = optarg;
//				break;
//			case NETHOST2_OPT:
//				g_netplayOptions.peer[1].isServer = false;
//				g_netplayOptions.peer[1].host = optarg;
//				break;
//			case NETPORT2_OPT:
//				g_netplayOptions.peer[1].port = optarg;
//				break;
//			case NETDELAY_OPT:
//				if (const auto temp {parseOptionValue<int>(optarg, "network input delay")}; temp.has_value())
//				{
//
//					if (g_netplayOptions.inputDelay > BattleFrameRateTicks)
//					{
//						error::saveError("Network input delay is absurdly large.");
//						badArg = true;
//					}
//					else
//					{
//						g_netplayOptions.inputDelay = *temp;
//					}
//				}
//				else
//				{
//					badArg = true;
//				}
//				break;
//
//#endif
//			default:
//				const char* optionName {optionIndex < 0 ? "<unknown>" : longOptions[optionIndex].name};
//				error::saveError("Error: Unknown option '{}'", optionName);
//				badArg = true;
//				break;
//		}
//	}
//
//	if (!badArg && optind != args.size())
//	{
//		error::saveError("\nError: Extra arguments found on the command line.");
//		badArg = true;
//	}
//
//	return badArg ? EXIT_FAILURE : EXIT_SUCCESS;
//	return {EXIT_SUCCESS, false};
//}

//
//void printUsage(FILE* out, const OptionsStruct& defaults)
//{
//	//FILE* old = log_setOutput(out);
//	//log_captureLines(LOG_CAPTURE_ALL);
//
//	uqm::log::info("Options:");
//	uqm::log::info("  -r, --res=WIDTHxHEIGHT (default: 640x480, bigger "
//				   "works only with --opengl)");
//	uqm::log::info("  -f, --fullscreen (default: 0)");
//	uqm::log::info("  -w, --windowed (default: true)");
//	uqm::log::info("  -o, --opengl. Default={}", defaults.opengl.toString());
//	uqm::log::info("  -x, --nogl. Default={}", defaults.opengl.value ? OptionFalseText : OptionTrueText);
//	uqm::log::info("  -k, --keepaspectratio. Default={}", defaults.keepAspectRatio.toString());
//	uqm::log::info("  -c, --scale=MODE (bilinear, biadapt, biadv, triscan, hq or none (default) )");
//	uqm::log::info("  -b, --meleezoom=MODE (step, aka pc, or smooth, aka 3do; default is 3do)");
//	uqm::log::info("  -s, --scanlines. Default={}", defaults.scanlines.toString());
//	uqm::log::info("  -p, --fps. Default={}", defaults.showFps.toString());
//	uqm::log::info("  -g, --gamma=CORRECTIONVALUE (default: 1.0, which causes no change)");
//	uqm::log::info("  -C, --configdir=CONFIGDIR");
//	uqm::log::info("  -n, --contentdir=CONTENTDIR");
//	uqm::log::info("  -M, --musicvol=VOLUME (0-100, default 100)");
//	uqm::log::info("  -S, --sfxvol=VOLUME (0-100, default 100)");
//	uqm::log::info("  -T, --speechvol=VOLUME (0-100, default 100)");
//	uqm::log::info("  -q, --audioquality=QUALITY (high, medium or low, default medium)");
//	uqm::log::info("  -u, --nosubtitles");
//	uqm::log::info("  -l, --logfile=FILE (sends console output to logfile FILE)");
//	uqm::log::info("  --addon ADDON (using a specific addon; may be specified multiple times)");
//	uqm::log::info("  --addondir=ADDONDIR (directory where addons reside)");
//	uqm::log::info("  --renderer=name (Select named rendering engine if possible)");
//	uqm::log::info("  --sound=DRIVER (openal, mixsdl, none; default mixsdl)");
//	uqm::log::info("  --stereosfx (enables positional sound effects, currently only for openal)");
//	uqm::log::info("  --safe (start in safe mode)");
//#ifdef NETPLAY
//	uqm::log::info("  --nethostN=HOSTNAME (server to connect to for player N (1=bottom, 2=top)");
//	uqm::log::info("  --netportN=PORT (port to connect to/listen on for player N (1=bottom, 2=top)");
//	uqm::log::info("  --netdelay=FRAMES (number of frames to buffer/delay network input for");
//#endif
//	uqm::log::info("The following options can take either '3do' or 'pc' as an option:");
//	uqm::log::info("  -i, --intro : Intro/ending version. Default={}", defaults.whichIntro.toString());
//	uqm::log::info("  --cscan     : coarse-scan display, pc=text, 3do=hieroglyphs. Default={}", defaults.whichCoarseScan.toString());
//	uqm::log::info("  --menu      : menu type, pc=text, 3do=graphical. Default={}", defaults.whichMenu.toString());
//	uqm::log::info("  --font      : font types and colors. Default={}", defaults.whichFonts.toString());
//	uqm::log::info("  --shield    : slave shield type; pc=static, 3do=throbbing. Default={}", defaults.whichShield.toString());
//	uqm::log::info("  --scroll    : ff/frev during comm.  pc=per-page, 3do=smooth. Default={}", defaults.smoothScroll.toString());
//
//	uqm::log::info("\nThe following options are MegaMod specific\n");
//
//	uqm::log::info("  --kohrstahp : Stops Kohr-Ah advancing.. Default={}", defaults.cheatMode.toString());
//	uqm::log::info("  --precursormode : =1 Infinite ship battery. =2 No damage, =3 Infinite ship battery and no damage (default: 0)");
//	uqm::log::info("  --timedilation : =1 Time is slowed down times 6. =2 Time is sped up times 5 (default: 0)");
//	uqm::log::info("  --bubblewarp : Instantaneous travel to any point on the Starmap.. Default={}", defaults.bubbleWarp.toString());
//	uqm::log::info("  --unlockships : Allows you to purchase ships that you can't normally acquire in the main game.. Default={}", defaults.unlockShips.toString());
//	uqm::log::info("  --headstart : Gives you an extra storage bay full of minerals, Fwiffo, and the Moonbase during a new game. Default={}", defaults.headStart.toString());
//	uqm::log::info("  --unlockupgrades : Unlocks every upgrade for your flagship and landers.. Default={}", defaults.unlockUpgrades.toString());
//	uqm::log::info("  --infiniteru : Gives you infinite R.U. as long as the cheat is on. Default={}", defaults.infiniteRU.toString());
//	uqm::log::info("  --skipintro : Skips the intro and Logo fmv. Default={}", defaults.skipIntro.toString());
//	uqm::log::info("  --mainmenumusic : Switches the main menu music on/off. Default={}", defaults.mainMenuMusic.toString());
//	uqm::log::info("  --nebulae : Enables/Disables nebulae in star systems. Default={}", defaults.nebulae.toString());
//	uqm::log::info("  --orbitingplanets : Enables/Disables orbiting planets in star systems. Default={}", defaults.orbitingPlanets.toString());
//	uqm::log::info("  --texturedplanets : Enables/Disables textured planets in star systems. Default={}", defaults.texturedPlanets.toString());
//	uqm::log::info("  --infinitefuel : Infinite fuel in the main game. Default={}", defaults.infiniteFuel.toString());
//	uqm::log::info("  --partialpickup : Enables/Disables partial mineral pickup . Default={}", defaults.partialPickup.toString());
//	uqm::log::info("  --submenu : Enables/Disables mineral and star map keys submenu . Default={}", defaults.submenu.toString());
//	uqm::log::info("  --dateformat : 0: MMM DD.YYYY | 1: MM.DD.YYYY | 2: DD MMM.YYYY | 3: DD.MM.YYYY (default: 0)");
//	uqm::log::info("  --infinitecredits: Gives you infinite Melnorme Credits . Default={}", defaults.infiniteCredits.toString());
//	uqm::log::info("  --melee : Takes you straight to Super Melee after the splash screen.");
//	uqm::log::info("  --loadgame : Takes you straight to the Load Game sceen after the splash screen.");
//	uqm::log::info("  --customborder : Enables the custom border frame.. Default={}", defaults.customBorder.toString());
//	uqm::log::info("  --seedtype: 0: Default seed | 1: Seed planets  | 2: Seed Melnorme/Rainbow/Quasispace  | 3: Seed Starmap (default: 0)");
//	uqm::log::info("  --customseed=# : Allows you to customize the internal seed used to generate the solar systems in-game. (default: 16807)");
//	uqm::log::info("  --shipseed: Seed the ships assigned to each race. Uses --customseed value (default {})", defaults.shipSeed.toString());
//	uqm::log::info("  --spherecolors: 0: Default colors | 1: StarSeed colors (default: 0)");
//	uqm::log::info("  --spacemusic #: Enables localized music for aliens when you are in their sphere of influence\n0: Default (OFF) | 1: No Spoilers | 2: Spoilers");
//	uqm::log::info("  --wholefuel : Enables the display of the whole fuel value in the ship status. Default={}", defaults.wholeFuel.toString());
//	uqm::log::info("  --dirjoystick : Enables the use of directional joystick controls for Android. Default={}", defaults.directionalJoystick.toString());
//	uqm::log::info("  --landerhold : Switch between PC/3DO max lander hold, pc=64, 3do=50. Default={}", defaults.landerHold.toString());
//	uqm::log::info("  --scrtrans : Screen transitions, pc=instantaneous, 3do=crossfade. Default={}", defaults.scrTrans.toString());
//	uqm::log::info("  --difficulty : 0: Normal | 1: Easy | 2: Hard | 3: Choose at Start (default: 0)");
//	uqm::log::info("  --fuelrange : Enables extra fuel range indicators : 0: No indicators | 1: Fuel range at destination | 2: Remaining fuel range to Sol | 3: Both option 1 and 2  enabled simultaneously (default: 0)");
//	uqm::log::info("  --extended : Enables Extended Edition features. Default={}", defaults.extended.toString());
//	uqm::log::info("  --nomad : Enables 'Nomad Mode' (No Starbase) : 0: Off | 1: Easy | 2: Normal (default: 0)");
//	uqm::log::info("  --gameover : Enables Game Over cutscenes. Default={}", defaults.gameOver.toString());
//	uqm::log::info("  --shipdirectionip : Enable NPC ships in IP to face their direction of travel. Default={}", defaults.shipDirectionIP.toString());
//	uqm::log::info("  --hazardcolors : Enable colored text based on hazard severity when viewing planetary scans. Default={}", defaults.hazardColors.toString());
//	uqm::log::info("  --orzcompfont : Enable alternate font for untranslatable Orz speech. Default={}", defaults.orzCompFont.toString());
//	uqm::log::info("  --smartautopilot : Activating Auto-Pilot within Solar System pilots the Flagship out via the shortest route.. Default={}", defaults.smartAutoPilot.toString());
//	uqm::log::info("  --controllertype : 0: Keyboard | 1: Xbox | 2: PlayStation 4 (default: 0)");
//	uqm::log::info("  --tintplansphere : Tint the planet sphere with scan color during scan. Default={}", defaults.tintPlanSphere.toString());
//	uqm::log::info("  --planetstyle : Choose between PC or 3DO planet color and shading. Default={}", defaults.planetStyle.toString());
//	uqm::log::info("  --starbackground : Set the background stars in solar system between PC, 3DO, UQM, or HD-mod patterns (default: pc)");
//	uqm::log::info("  --scanstyle : Choose between PC or 3DO scanning types. Default={}", defaults.scanStyle.toString());
//	uqm::log::info("  --nonstoposcill : Oscilloscope uses both voice and music data. Default={}", defaults.nonStopOscill.toString());
//	uqm::log::info("  --scopestyle : Choose between either the PC or 3DO oscilloscope type. Default={}", defaults.scopeStyle.toString());
//	uqm::log::info("  --animhyperstars : HD only - Use old HD-mod animated HyperSpace stars. Default={}", defaults.hyperStars.toString());
//	uqm::log::info("  --landerview : Choose between either the PC or 3DO lander view. Default={}", defaults.landerStyle.toString());
//	uqm::log::info("  --planettexture : Choose between either 3DO or UQM planet map texture [when not using custom seed] (default: 3do)");
//	uqm::log::info("  --sisenginecolor : Choose between either the PC or 3DO Flagship engine color. Default={}", defaults.flagshipColor.toString());
//	uqm::log::info("  --nohqencounters : Disables HyperSpace encounters. Default={}", defaults.noHQEncounters.toString());
//	uqm::log::info("  --decleanse : Moves the Death March 100 years ahead from its actual start date [does not work once the Death March has started]. Default={}", defaults.deCleansing.toString());
//	uqm::log::info("  --nomeleeobstacles : Removes the planet and asteroids from Super Melee. Default={}", defaults.meleeObstacles.toString());
//	uqm::log::info("  --showvisitstars : Dim visited stars on the StarMap and encase the star name in parenthesis. Default={}", defaults.showVisitedStars.toString());
//	uqm::log::info("  --unscaledstarsystem : Show the classic HD-mod Beta Star System view. Default={}", defaults.unscaledStarSystem.toString());
//	uqm::log::info("  --spheretype : Choose between PC, 3DO, or UQM scan sphere styles. Default={}", defaults.sphereType.toString());
//	uqm::log::info("  --nebulaevol=VOLUME (0-50, default 11)");
//	uqm::log::info("  --slaughtermode : Affect a race's SOI by destroying their ships in battle. Default={}", defaults.slaughterMode.toString());
//	uqm::log::info("  --advancedautopilot : Finds the route that uses the least amount of fuel through HyperSpace or QuasiSpace and Auto-Pilots the Flagship on the best route. Default={}", defaults.advancedAutoPilot.toString());
//	uqm::log::info("  --meleetooltips : Show SC1-style ship description tooltips at the bottom of the Super-Melee screen when picking a ship for your fleet. Default={}", defaults.meleeToolTips.toString());
//	uqm::log::info("  --musicresume : Resumes the music in UQM where it last left off : 0: Off | 1: 5 Minutes | 2: Indefinite (default: 0)");
//	uqm::log::info("  --windowtype : Choose between DOS, 3DO or UQM window types : 0: DOS | 1: 3DO | 2: UQM (default: 0)");
//	uqm::log::info("  --scatterelements : Scatter a percentage of the elements in the lander's cargo hold onto the planet's surface when the lander explodes. Default={}", defaults.scatterElements.toString());
//	uqm::log::info("  --showupgrades : Show lander upgrade graphics when exploring planets. Default={}", defaults.showUpgrades.toString());
//	uqm::log::info("  --fleetpointsys : Restrict the amount of ships that can be purchased via their melee points. Default={}", defaults.fleetPointSys.toString());
//	uqm::log::info("  --shipstore : Enable a storage queue accessed at the shipyard. Default={}", defaults.shipStore.toString());
//	uqm::log::info("  --captainnames : Display captain names at shipyard. Default={}", defaults.captainNames.toString());
//	uqm::log::info("  --dosmenus : Display DOS style menu in shipyard in place of SIS window. Default={}", defaults.dosMenus.toString());
//
//	//log_setOutput(old);
//}
} // namespace uqm