#pragma once

#include "core/stl/stl.h"
#include "core/string/StringUtils.h"
#include "options/netoptions.h"
#include "options/OptionConstants.h"
#include "options/OptionTypes.h"
#include "options/OptionDefs.h"
#include "options/Setting.h"

#include "lib/Math2D/Math2D.h"
#include "libs/math/randomdefs.h" // TODO: only used for the PrimeA constant and SeedType. Find a better way to share that perhaps?
#include "libs/platform.h"

namespace uqm
{

enum class RunMode
{
	Normal,
	Usage,
	Version,
};


inline constexpr int c_controlTemplateCount {6};

namespace settings
{
inline bool isUnitInterval(const float& value)
{
	return value >= 0.0f && value <= 1.0f;
}

inline bool isValidGamma(const float& value)
{
	return value >= 0.03f && value <= 9.9f;
}

inline bool isNonNegative(const int& value)
{
	return value >= 0;
}

inline bool isPositive(const int& value)
{
	return value > 0;
}

inline bool isValidResolutionFactor(const int& value)
{
	return value >= 0 && value <= 2;
}

inline bool isValidWindowStyle(const int& value)
{
	return value >= 0 && value <= 2;
}

inline bool isValidNebulaeVolume(const int& value)
{
	return value >= 0 && value <= 50;
}

inline bool isSaneSeed(const int& value)
{
	return SANE_SEED(value);
}

inline bool isValidControlTemplate(const int& value)
{
	return value >= 0 && value < c_controlTemplateCount;
}
} // namespace settings

struct OptionsStruct : SettingsRegistry
{
	// Commandline-only options
	RunMode runMode {RunMode::Normal};		// Controls whether the game runs normally or just prints version/usage.
	uqstl::string logFile {};				// Path to the log output file.
	uqstl::string configDir {};				// Directory where settings.json and other config files are read/written.
	uqstl::string contentDir {};			// Root directory for game content packages (.uqm files).
	uqstl::string addonDir {};				// Directory scanned for optional add-on packages.
	uqstl::vector<uqstl::string> addons {}; // Ordered list of add-on package names to load at startup.

	uqstl::string graphicsBackend {}; // SDL renderer backend hint passed to TFB_InitGraphics.
	uqstl::string inputScript {};	  // File of scripted menu key presses, for testing.

	// Commandline and user config options
	Setting<bool> opengl {
		*this, {.key = "graphics.useOpenGL", .description = "Force OpenGL renderer on SDL1 builds; ignored on SDL2.", .cli = "-o,-x{false},--opengl,--nogl{false}"},
		 false
	  };
	Setting<bool> alwaysOpenGL {
		*this, {.key = "graphics.alwaysOpenGL", .description = "Use OpenGL even at native resolutions."},
		  false
	   };
	Setting<Vec2u> resolution {
		*this, {.key = "graphics.resolution", .description = "Window/framebuffer pixel dimensions when not using a blowup scaler."},
		 Vec2u {640,							 480																					}
	 };
	Setting<WindowMode> windowMode {
		*this, {.key = "graphics.windowMode", .description = "Windowed, windowed-fullscreen, or exclusive fullscreen.", .cli = "-f,--fullscreen"},
		   WindowMode::Fullscreen
	 };
	Setting<bool> scanlines {
		*this, {.key = "graphics.scanlines", .description = "Render simulated scanlines, like an old CRT.", .cli = "-s,--scanlines"},
		  false
	   };
	Setting<ScalingMode> scaler {
		*this, {.key = "graphics.scaler", .description = "Post-process pixel scaler applied when blitting to the display.", .cli = "-c,--scale"},
		  ScalingMode::None
	   };
	Setting<bool> showFps {
		*this, {.key = "graphics.showFps", .description = "Draw an on-screen frames-per-second counter.", .cli = "-p,--fps"},
		  false
	   };
	Setting<bool> keepAspectRatio {
		*this, {.key = "graphics.keepAspectRatio", .description = "Letterbox/pillarbox to preserve the native 4:3 aspect ratio.", .cli = "-k,--keepaspectratio"},
		  false
	   };
	Setting<float> gamma {
		*this, {.key = "graphics.gamma", .description = "Gamma correction value, a value of 1.0 means no change.", .cli = "-g,--gamma"},
		 1.0f, settings::isValidGamma
	 };
	Setting<AudioDriverType> soundDriver {
		*this, {.key = "audio.driver", .description = "Backend used for audio output (MixSDL, OpenAL, or silence).", .cli = "--sound,--sounddriver", .requiresRestart = true},
		   AudioDriverType::MixSDL
	  };
	Setting<AudioQuality> soundQuality {
		*this, {.key = "audio.quality", .description = "Resampling quality level for the audio mixer.", .cli = "-q,--audioquality", .requiresRestart = true},
		  AudioQuality::High
	};
	Setting<bool> use3doMusic {
		*this, {.key = "audio.music3do", .description = "Play the 3DO remixed music tracks instead of the PC MIDI/MOD originals.", .requiresReload = true},
			true
	};
	Setting<bool> useRemixMusic {
		*this, {.key = "audio.musicRemix", .description = "Play the Precursors fan-remix music pack when available.", .requiresReload = true},
		   false
	};
	Setting<bool> useSpeech {
		*this, {.key = "audio.speech", .description = "Enable voiced dialogue during alien conversations.", .requiresReload = true},
		 true
	 };
	Setting<CoarseScanStyle> whichCoarseScan {
		*this, {.key = "visuals.coarseScanStyle", .description = "Coarse-scan display: text or pictograms, in the original or the P6014 style.", .cli = "--cscan"},
			CoarseScanStyle::Text
	 };
	Setting<EmulationMode> whichMenu {
		*this, {.key = "interface.menuStyle", .description = "Menu type, PC=text, 3DO=graphical.", .cli = "--menu"},
		 uqm::EmulationMode::PC
	   };
	Setting<EmulationMode> whichFonts {
		*this, {.key = "interface.fontStyle", .description = "Font types and colors.", .cli = "--font"},
		 uqm::EmulationMode::PC
	   };
	Setting<EmulationMode> whichIntro {
		*this, {.key = "interface.introStyle", .description = "Which intro/ending version to use.", .cli = "-i,--intro", .requiresReload = true},
		  uqm::EmulationMode::PC
	};
	Setting<EmulationMode> whichShield {
		*this, {.key = "visuals.shieldStyle", .description = "Slave shield type; PC=static, 3DO=throbbing.", .cli = "--shield"},
		 uqm::EmulationMode::PC
	   };
	Setting<EmulationMode> smoothScroll {
		*this, {.key = "visuals.smoothScroll", .description = "Scrolling animation during dialogue. PC=page-flip, 3DO=smooth scroll.", .cli = "--scroll"},
		   uqm::EmulationMode::PC
	 };
	Setting<MeleeScaleMode> meleeScale {
		*this, {.key = "graphics.meleeScale", .description = "Scaling algorithm used when zooming the melee battle view.", .cli = "-b,--meleezoom"},
		 MeleeScaleMode::Smooth
	   };
	Setting<bool> subtitles {
		*this, {.key = "interface.subtitles", .description = "Show text subtitles alongside voiced alien dialogue.", .cli = "-u,--subtitles,--nosubtitles{false}"},
			true
	};
	Setting<bool> stereoSFX {
		*this, {.key = "audio.positionalSfx", .description = "Enable stereo sfx. Requires --sounddriver to be \"OpenAL\".", .cli = "--stereosfx", .requiresReload = true},
		   false
	};
	Setting<float> musicVolumeScale {
		*this, {.key = "audio.musicVolume", .description = "Master volume multiplier for background music (0–1).", .cli = "-M,--musicvol"},
			1.0f, settings::isUnitInterval
	  };
	Setting<float> sfxVolumeScale {
		*this, {.key = "audio.sfxVolume", .description = "Master volume multiplier for sound effects (0–1).", .cli = "-S,--sfxvol"},
		 1.0f, settings::isUnitInterval
	   };
	Setting<float> speechVolumeScale {
		*this, {.key = "audio.speechVolume", .description = "Master volume multiplier for speech audio (0–1).", .cli = "-T,--speechvol"},
		  1.0f, settings::isUnitInterval
	};
	BoolOption safeMode {false}; // Skip loading settings.json so the game starts with all defaults.

	// megamod options
	Setting<int> resolutionFactor {
		*this, {.key = "graphics.resolutionFactor", .description = "HD scale factor: 0 = SD (320×240), 2 = HD (640×480 canvas).", .requiresReload = true},
		   0, settings::isValidResolutionFactor
	   };
	Setting<int> loresBlowupScale {
		*this, {.key = "graphics.loresBlowupScale", .description = "Integer upscale applied to the SD canvas for windowed HD displays."},
		  1, settings::isNonNegative
	};
	Setting<bool> cheatMode {
		*this, {.key = "cheats.kohrAhStop", .description = "Stops Kohr-Ah advancing.", .cli = "--kohrstahp"},
		  false
	   };
	Setting<GodModeFlags> optGodModes {
		*this, {.key = "cheats.godModes", .description = "Bitfield enabling infinite battery and/or no-damage in combat."},
			GodModeFlags::None
	  };
	Setting<int> timeDilationPct {
		*this, {.key = "cheats.timeDilation", .description = "Time dilation scale percentage. 100 = regular speed.", .cli = "--timedilation"},
		   100, settings::isPositive
	};
	Setting<bool> bubbleWarp {
		*this, {.key = "cheats.bubbleWarp", .description = "Instantaneous travel to any point on the Starmap.", .cli = "--bubblewarp"},
			false
	 };
	Setting<bool> unlockShips {
		*this, {.key = "cheats.unlockShips", .description = "Allows you to purchase ships that you can't normally acquire in the main game.", .cli = "--unlockships"},
		   false
	};
	Setting<bool> headStart {
		*this, {.key = "cheats.headStart", .description = "Gives you an extra storage bay full of minerals, Fwiffo, and the Moonbase during a new game.", .cli = "--headstart"},
		 false
	  };
	BoolOption unlockUpgrades {false}; // Make all ship upgrades purchasable immediately at the starbase.
	Setting<bool> infiniteRU {
		*this, {.key = "cheats.infiniteRU", .description = "Gives you infinite R.U. as long as the cheat is on.", .cli = "--infiniteru"},
		  false
	   };
	Setting<bool> skipIntro {
		*this, {.key = "interface.skipIntro", .description = "Skips the intro and Logo fmv.", .cli = "--skipintro"},
		 false
	  };
	Setting<bool> mainMenuMusic {
		*this, {.key = "audio.mainMenuMusic", .description = "Switches the main menu music on/off.", .cli = "--mainmenumusic"},
			true
	};
	Setting<bool> nebulae {
		*this, {.key = "visuals.nebulae", .description = "Enables/Disables nebulae in star systems.", .cli = "--nebulae"},
		   false
	};
	Setting<bool> orbitingPlanets {
		*this, {.key = "visuals.orbitingPlanets", .description = "Enables/Disables orbiting planets in star systems.", .cli = "--orbitingplanets"},
			false
	 };
	Setting<bool> texturedPlanets {
		*this, {.key = "visuals.texturedPlanets", .description = "Enables/Disables textured planets in star systems.", .cli = "--texturedplanets"},
			false
	 };
	Setting<DateFormat> optDateFormat {
		*this, {.key = "interface.dateFormat", .description = "Date format to use in-game.", .cli = "--dateformat"},
		 DateFormat::MMM_dd_yyyy
	};
	Setting<bool> infiniteFuel {
		*this, {.key = "cheats.infiniteFuel", .description = "Infinite fuel in the main game.", .cli = "--infinitefuel"},
		  false
	   };
	Setting<bool> partialPickup {
		*this, {.key = "gameplay.partialPickup", .description = "Enables/Disables partial mineral pickup.", .cli = "--partialpickup"},
		   false
	};
	Setting<bool> submenu {
		*this, {.key = "interface.submenu", .description = "Enables/Disables mineral and star map keys submenu.", .cli = "--submenu"},
		   false
	};
	Setting<bool> infiniteCredits {
		*this, {.key = "cheats.infiniteCredits", .description = "Gives you infinite Melnorme Credits.", .cli = "--infinitecredits"},
		 false
	  };
	Setting<bool> customBorder {
		*this, {.key = "graphics.customBorder", .description = "Enables the custom border frame.", .cli = "--customborder"},
		 false
	  };
	Setting<SeedType> seedType {
		*this, {.key = "seed.type", .description = "Seed type for solar system generation. Default is \"None\", which is the same seed used in the original game. Changing this will change the layout of the entire star map.", .cli = "--seedtype"},
		   SeedType::None
	 };
	Setting<int> customSeed {
		*this, {.key = "seed.value", .description = "Allows you to customize the internal seed used to generate the solar systems in-game.", .cli = "--customseed"},
		 PrimeA, settings::isSaneSeed
	 };
	Setting<bool> shipSeed {
		*this, {.key = "seed.ships", .description = "Seeds the ships assigned to each race. Uses --customseed value.", .cli = "--shipseed"},
		 false
	  };
	Setting<SphereOfInfluenceColors> sphereColors {
		*this, {.key = "visuals.sphereColors", .description = "Color scheme for race spheres of influence in star map.", .cli = "--spherecolors"},
		   SphereOfInfluenceColors::Default
	   };
	Setting<SphereOfInfluenceMusic> spaceMusic {
		*this, {.key = "audio.spaceMusic", .description = "Enables localized music for aliens when you are in their sphere of influence.", .cli = "--spacemusic", .requiresReload = true},
		   SphereOfInfluenceMusic::None
	   };
	Setting<bool> volasMusic {
		*this, {.key = "audio.musicVolasaurus", .description = "Enable the Volasaurus fan-remix music pack when available.", .requiresReload = true},
		  false
	   };
	Setting<bool> wholeFuel {
		*this, {.key = "interface.wholeFuel", .description = "Enables the display of the whole fuel value in the ship status.", .cli = "--wholefuel"},
		   false
	};
	Setting<bool> directionalJoystick {
		*this, {.key = "interface.directionalJoystick", .description = "Enables the use of directional joystick controls for Android.", .cli = "--dirjoystick"},
		 false
	  };
	Setting<EmulationMode> landerHold {
		*this, {.key = "gameplay.landerHold", .description = "Switch between PC/3DO max lander hold, pc=64, 3do=50.", .cli = "--landerhold"},
		  EmulationMode::Console3DO
	   };
	Setting<EmulationMode> scrTrans {
		*this, {.key = "interface.screenTransition", .description = "Screen transitions, pc=instantaneous, 3do=crossfade.", .cli = "--scrtrans"},
		  EmulationMode::Console3DO
	   };
	OptionT<Difficulty> optDifficulty {Difficulty::Normal}; // Active difficulty level applied to game mechanics (Normal/Easy/Hard).
	Setting<Difficulty> optDiffChooser {
		*this, {.key = "gameplay.difficulty", .description = "Difficulty shown in the setup menu (may be ChooseYourOwn).", .cli = "--difficulty"},
		   Difficulty::ChooseYourOwn
	};
	Setting<FuelRangeDisplay> optFuelRange {
		*this, {.key = "interface.fuelRange", .description = "Starmap fuel-range indicator style (circle, destination, Sol distance)."},
		 FuelRangeDisplay::Normal
	 };
	Setting<bool> extended {
		*this, {.key = "gameplay.extended", .description = "Enables Extended Edition features.", .cli = "--extended"},
		   false
	};
	Setting<NomadMode> nomad {
		*this, {.key = "gameplay.nomad", .description = "Enables 'Nomad Mode' (No Starbase).", .cli = "--nomad"},
		  NomadMode::Off
	};
	Setting<bool> gameOver {
		*this, {.key = "gameplay.gameOver", .description = "Enables Game Over cutscenes.", .cli = "--gameover"},
		 false
	  };
	Setting<bool> shipDirectionIP {
		*this, {.key = "visuals.shipDirectionIP", .description = "Enable NPC ships in IP to face their direction of travel.", .cli = "--shipdirectionip"},
		   false
	};
	Setting<bool> hazardColors {
		*this, {.key = "visuals.hazardColors", .description = "Enable colored text based on hazard severity when viewing planetary scans.", .cli = "--hazardcolors"},
		  false
	   };
	Setting<bool> orzCompFont {
		*this, {.key = "visuals.orzCompFont", .description = "Enable alternate font for untranslatable Orz speech.", .cli = "--orzcompfont"},
		  false
	   };

	Setting<ControllerType> optControllerType {
		*this, {.key = "interface.controllerType", .description = "Controller type to use for input.", .cli = "--controllertype"},
		   ControllerType::KeyboardMouse
	};
	Setting<bool> smartAutoPilot {
		*this, {.key = "autopilot.smart", .description = "Activating Auto-Pilot within Solar System pilots the Flagship out via the shortest route.", .cli = "--smartautopilot"},
		  false
	   };
	Setting<EmulationMode> tintPlanSphere {
		*this, {.key = "visuals.tintPlanetSphere", .description = "Tint the planet sphere with scan color during scan.", .cli = "--tintplansphere"},
		 uqm::EmulationMode::Console3DO
	   };
	Setting<EmulationMode> planetStyle {
		*this, {.key = "visuals.planetStyle", .description = "Choose between PC or 3DO planet color and shading.", .cli = "--planetstyle"},
			uqm::EmulationMode::Console3DO
	  };
	Setting<int> starBackground {
		*this, {.key = "visuals.starBackground", .description = "Starfield density/style index used in the Hyperspace and combat backgrounds."},
		 0, settings::isNonNegative
	   };
	Setting<EmulationMode> scanStyle {
		*this, {.key = "visuals.scanStyle", .description = "Choose between PC or 3DO scanning types.", .cli = "--scanstyle"},
		  uqm::EmulationMode::Console3DO
	};
	Setting<bool> nonStopOscill {
		*this, {.key = "visuals.nonStopOscilloscope", .description = "Oscilloscope uses both voice and music data.", .cli = "--nonstoposcill"},
			false
	 };
	Setting<EmulationMode> scopeStyle {
		*this, {.key = "visuals.scopeStyle", .description = "Choose between either the PC or 3DO oscilloscope type.", .cli = "--scopestyle"},
		  uqm::EmulationMode::Console3DO
	};
	Setting<bool> hyperStars {
		*this, {.key = "visuals.hyperStars", .description = "HD only - Use old HD-mod animated HyperSpace stars.", .cli = "--animhyperstars"},
		   false
	};
	Setting<EmulationMode> landerStyle {
		*this, {.key = "visuals.landerStyle", .description = "Choose between either the PC or 3DO lander view.", .cli = "--landerview"},
		 uqm::EmulationMode::Console3DO
	   };
	Setting<bool> planetTexture {
		*this, {.key = "visuals.planetTexture", .description = "Apply surface detail textures to the rotating planet sphere."},
			true
	};
	Setting<EmulationMode> flagshipColor {
		*this, {.key = "visuals.flagshipColor", .description = "Choose between either the PC or 3DO Flagship engine color.", .cli = "--sisenginecolor"},
		 uqm::EmulationMode::PC
	   };
	Setting<bool> noHQEncounters {
		*this, {.key = "cheats.noHQEncounters", .description = "Disables HyperSpace encounters.", .cli = "--nohqencounters"},
		  false
	   };
	Setting<bool> deCleansing {
		*this, {.key = "cheats.deCleansing", .description = "Moves the Death March 100 years ahead from its actual start date [does not work once the Death March has started].", .cli = "--decleanse"},
		 false
	  };
	Setting<bool> meleeObstacles {
		*this, {.key = "cheats.meleeObstacles", .description = "Removes the planet and asteroids from Super Melee.", .cli = "--nomeleeobstacles"},
		   false
	};
	Setting<bool> showVisitedStars {
		*this, {.key = "visuals.showVisitedStars", .description = "Dim visited stars on the StarMap and encase the star name in parenthesis.", .cli = "--showvisitstars"},
		   false
	};
	Setting<bool> unscaledStarSystem {
		*this, {.key = "visuals.unscaledStarSystem", .description = "Show the classic HD-mod Beta Star System view.", .cli = "--unscaledstarsystem"},
		  false
	   };
	Setting<int> sphereType {
		*this, {.key = "visuals.sphereType", .description = "Visual style index for the sphere-of-influence overlay (filled, outline, etc.)."},
			2, settings::isNonNegative
	  };
	Setting<int> nebulaevol {
		*this, {.key = "visuals.nebulaeVolume", .description = "Opacity/density of the hyperspace nebula cloud overlay (0–50).", .cli = "--nebulaevol"},
		 16, settings::isValidNebulaeVolume
	   };
	Setting<bool> slaughterMode {
		*this, {.key = "gameplay.slaughterMode", .description = "Affect a race's SOI by destroying their ships in battle.", .cli = "--slaughtermode"},
		   false
	};
	Setting<bool> advancedAutoPilot {
		*this, {.key = "autopilot.advanced", .description = "Finds the route that uses the least amount of fuel through HyperSpace or QuasiSpace and Auto-Pilots the Flagship on the best route.", .cli = "--advancedautopilot"},
		  false
	   };
	Setting<bool> meleeToolTips {
		*this, {.key = "interface.meleeToolTips", .description = "Show SC1-style ship description tooltips at the bottom of the Super-Melee screen when picking a ship for your fleet.", .cli = "--meleetooltips"},
			false
	 };
	Setting<int> musicResume {
		*this, {.key = "audio.musicResume", .description = "Music resume mode: 0 = restart track, 1 = resume from last position."},
			0, settings::isNonNegative
	  };
	Setting<int> windowEmulationMode {
		*this, {.key = "graphics.windowStyle", .description = "Window chrome/border style: 0 = DOS, 1 = 3DO, 2 = UQM native.", .requiresReload = true},
			2, settings::isValidWindowStyle
	   };
	Setting<bool> scatterElements {
		*this, {.key = "gameplay.scatterElements", .description = "Scatter a percentage of the elements in the lander's cargo hold onto the planet's surface when the lander explodes.", .cli = "--scatterelements"},
		  false
	   };
	Setting<bool> showUpgrades {
		*this, {.key = "visuals.showUpgrades", .description = "Show lander upgrade graphics when exploring planets.", .cli = "--showupgrades"},
			false
	 };
	Setting<bool> fleetPointSys {
		*this, {.key = "gameplay.fleetPointSystem", .description = "Restrict the amount of ships that can be purchased via their melee points.", .cli = "--fleetpointsys"},
			false
	 };
	Setting<bool> shipStore {
		*this, {.key = "gameplay.shipStore", .description = "Enable a storage queue accessed at the shipyard.", .cli = "--shipstore"},
		   false
	};
	Setting<bool> captainNames {
		*this, {.key = "interface.captainNames", .description = "Display captain names at shipyard.", .cli = "--captainnames"},
			false
	 };
	Setting<bool> dosMenus {
		*this, {.key = "interface.dosMenus", .description = "Display DOS style menu in shipyard in place of SIS window.", .cli = "--dosmenus"},
			false
	 };

	Setting<int> player1Control {
		*this, {.key = "controls.player1Template", .description = "Control template index used by player one."},
		 0, settings::isValidControlTemplate
	};
	Setting<int> player2Control {
		*this, {.key = "controls.player2Template", .description = "Control template index used by player two."},
		 3, settings::isValidControlTemplate
	};
};

template <typename ListT>
[[nodiscard]] inline uqstl::optional<typename ListT::value_type::value_type> getOptionListValue(const ListT& list, uqstl::string_view name) noexcept
{
	for (const auto& item : list)
	{
		if (name.compare(item.name) == 0)
		{
			return {item.value};
		}
	}
	return uqstl::nullopt;
}

template <typename OptionValueT>
[[nodiscard]] inline uqstl::optional<OptionValueT> parseOptionValue(uqstl::string_view valueStr, uqstl::string_view optionName)
{
	OptionValueT destValue {};
	const uqstl::errc errCode {parseStr(valueStr, destValue)};

	if (errCode == std::errc {})
	{
		return {destValue};
	}
	else if (errCode == std::errc::invalid_argument)
	{
		throw std::invalid_argument(fmt::format("Invalid string for {} option value '{}'.", optionName, valueStr));
	}
	else if (errCode == std::errc::result_out_of_range)
	{
		throw std::out_of_range(fmt::format("The value '{}' held in the option string for {} is too large to be held by the option.", valueStr, optionName));
	}
	return {};
}

template <typename OptionT, typename OptionValueT = typename OptionT::value_type>
inline bool parseOption(OptionT& option, uqstl::string_view valueStr, uqstl::string_view optionName)
{
	if (const auto optionValue {parseOptionValue<OptionValueT>(valueStr, optionName)}; optionValue.has_value())
	{
		option = optionValue.value();
		return true;
	}

	return false;
}

template <>
inline bool parseOption(EmulationOption& option, uqstl::string_view valueStr, uqstl::string_view optionName)
{
	if (const auto listValue {getOptionListValue(EmulationChoiceList, valueStr)}; listValue.has_value())
	{
		option = *listValue;
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////


template <typename InT>
[[nodiscard]] inline float normalizeValue(const InT inVol, const InT max) noexcept
{
	if (inVol < InT {})
	{
		return 0.0f;
	}

	if (inVol > max)
	{
		return 1.0f;
	}

	return inVol / static_cast<float>(max);
}

class UQMOptions
{
public:
	UQMOptions();
	~UQMOptions();

	[[nodiscard]] static UQMOptions& getInstance();

	// Returns an [exit code, needs exit] result.
	uqstl::pair<int, bool> parseArgs(uqstl::span<uqgsl::zstring> args);
	//void printUsage(FILE* out, const OptionsStruct& defaultOptions);

	const OptionsStruct& get() const
	{
		return m_options;
	}

	static const OptionsStruct& read()
	{
		return getInstance().m_options;
	}

	static OptionsStruct& edit()
	{
		return getInstance().m_options;
	}
#ifdef NETPLAY
	NetplayOptions& getNetplayOptions()
	{
		return m_netplayOptions;
	}
#endif


private:
	OptionsStruct m_options;
#ifdef NETPLAY
	NetplayOptions m_netplayOptions;
#endif

	static UQMOptions* s_instance;
};

} // namespace uqm