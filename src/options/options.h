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

	// Commandline and user config options
	Setting<bool> opengl {*this, "graphics.useOpenGL", false, "Force OpenGL renderer on SDL1 builds; ignored on SDL2."};
	Setting<bool> alwaysOpenGL {*this, "graphics.alwaysOpenGL", false, "Use OpenGL even at native resolutions."};
	Setting<Vec2u> resolution {
		*this, "graphics.resolution", Vec2u {640, 480},
			"Window/framebuffer pixel dimensions when not using a blowup scaler."
	 };
	Setting<WindowMode> windowMode {*this, "graphics.windowMode", WindowMode::Fullscreen, "Windowed, windowed-fullscreen, or exclusive fullscreen."};
	Setting<bool> scanlines {*this, "graphics.scanlines", false, "Overlay a scanline effect over the rendered frame."};
	Setting<ScalingMode> scaler {*this, "graphics.scaler", ScalingMode::None, "Post-process pixel scaler applied when blitting to the display."};
	Setting<bool> showFps {*this, "graphics.showFps", false, "Draw an on-screen frames-per-second counter."};
	Setting<bool> keepAspectRatio {*this, "graphics.keepAspectRatio", false, "Letterbox/pillarbox to preserve the native 4:3 aspect ratio."};
	Setting<float> gamma {*this, "graphics.gamma", 1.0f, "Display gamma correction value applied via the GPU or SDL.", settings::isValidGamma};
	Setting<AudioDriverType> soundDriver {*this, "audio.driver", AudioDriverType::MixSDL, "Backend used for audio output (MixSDL, OpenAL, or silence)."};
	Setting<AudioQuality> soundQuality {*this, "audio.quality", AudioQuality::High, "Resampling quality level for the audio mixer."};
	Setting<bool> use3doMusic {*this, "audio.music3do", true, "Play the 3DO remixed music tracks instead of the PC MIDI/MOD originals."};
	Setting<bool> useRemixMusic {*this, "audio.musicRemix", false, "Play the Precursors fan-remix music pack when available."};
	Setting<bool> useSpeech {*this, "audio.speech", true, "Enable voiced dialogue during alien conversations."};
	Setting<EmulationMode> whichCoarseScan {*this, "visuals.coarseScanStyle", EmulationMode::PC, "Planetary coarse-scan display: iconic (PC) or full image (3DO)."};
	Setting<EmulationMode> whichMenu {*this, "interface.menuStyle", uqm::EmulationMode::PC, "Main menu layout: text list (PC) or animated orb (3DO)."};
	Setting<EmulationMode> whichFonts {*this, "interface.fontStyle", uqm::EmulationMode::PC, "Text rendering style: flat PC fonts or gradient 3DO fonts."};
	Setting<EmulationMode> whichIntro {*this, "interface.introStyle", uqm::EmulationMode::PC, "Introduction sequence: slides (PC) or FMV movies (3DO)."};
	Setting<EmulationMode> whichShield {*this, "visuals.shieldStyle", uqm::EmulationMode::PC, "Flagship shield appearance: static (PC) or animated pulse (3DO)."};
	Setting<EmulationMode> smoothScroll {*this, "visuals.smoothScroll", uqm::EmulationMode::PC, "Starmap/comm scrolling: stepped (PC) or smooth (3DO)."};
	Setting<MeleeScaleMode> meleeScale {*this, "graphics.meleeScale", MeleeScaleMode::Smooth, "Scaling algorithm used when zooming the melee battle view."};
	Setting<bool> subtitles {*this, "interface.subtitles", true, "Show text subtitles alongside voiced alien dialogue."};
	Setting<bool> stereoSFX {*this, "audio.positionalSfx", false, "Pan sound effects in stereo based on screen position."};
	Setting<float> musicVolumeScale {*this, "audio.musicVolume", 1.0f, "Master volume multiplier for background music (0–1).", settings::isUnitInterval};
	Setting<float> sfxVolumeScale {*this, "audio.sfxVolume", 1.0f, "Master volume multiplier for sound effects (0–1).", settings::isUnitInterval};
	Setting<float> speechVolumeScale {*this, "audio.speechVolume", 1.0f, "Master volume multiplier for speech audio (0–1).", settings::isUnitInterval};
	BoolOption safeMode {false}; // Skip loading settings.json so the game starts with all defaults.

	// megamod options
	Setting<int> resolutionFactor {*this, "graphics.resolutionFactor", 0, "HD scale factor: 0 = SD (320×240), 2 = HD (640×480 canvas).", settings::isValidResolutionFactor};
	Setting<int> loresBlowupScale {*this, "graphics.loresBlowupScale", 1, "Integer upscale applied to the SD canvas for windowed HD displays.", settings::isNonNegative};
	Setting<bool> cheatMode {*this, "cheats.kohrAhStop", false, "Freeze the Kohr-Ah death march timer so their fleet never advances."};
	Setting<GodModeFlags> optGodModes {*this, "cheats.godModes", GodModeFlags::None, "Bitfield enabling infinite battery and/or no-damage in combat."};
	Setting<int> timeDilationPct {*this, "cheats.timeDilation", 100, "Game clock speed as a percentage of normal (100 = real-time).", settings::isPositive};
	Setting<bool> bubbleWarp {*this, "cheats.bubbleWarp", false, "Allow warping to any star system regardless of fuel range."};
	Setting<bool> unlockShips {*this, "cheats.unlockShips", false, "Make all ships available in the Full-Game Melee roster from the start."};
	Setting<bool> headStart {*this, "cheats.headStart", false, "Begin the campaign with extra fuel, RU, and crew."};
	BoolOption unlockUpgrades {false}; // Make all ship upgrades purchasable immediately at the starbase.
	Setting<bool> infiniteRU {*this, "cheats.infiniteRU", false, "Grant unlimited Resource Units so the player never runs out of money."};
	Setting<bool> skipIntro {*this, "interface.skipIntro", false, "Jump past the opening intro sequence directly to the main menu."};
	Setting<bool> mainMenuMusic {*this, "audio.mainMenuMusic", true, "Play background music on the main menu screen."};
	Setting<bool> nebulae {*this, "visuals.nebulae", false, "Render nebula cloud artwork in the Hyperspace star map."};
	Setting<bool> orbitingPlanets {*this, "visuals.orbitingPlanets", false, "Animate planets orbiting their star in the solar system view."};
	Setting<bool> texturedPlanets {*this, "visuals.texturedPlanets", false, "Apply surface texture maps to planets in the solar system view."};
	Setting<DateFormat> optDateFormat {*this, "interface.dateFormat", DateFormat::MMM_dd_yyyy, "Date display format shown on the starmap HUD clock."};
	Setting<bool> infiniteFuel {*this, "cheats.infiniteFuel", false, "Flagship fuel never depletes during hyperspace travel."};
	Setting<bool> partialPickup {*this, "gameplay.partialPickup", false, "Allow picking up a partial mineral load when the lander is nearly full."};
	Setting<bool> submenu {*this, "interface.submenu", false, "Show an extra submenu layer in the in-flight status/outfit screens."};
	Setting<bool> infiniteCredits {*this, "cheats.infiniteCredits", false, "Crew cost at the starbase is waived; credits never decrease."};
	Setting<bool> customBorder {*this, "graphics.customBorder", false, "Draw a decorative border frame around the game viewport."};
	Setting<SeedType> seedType {*this, "seed.type", SeedType::None, "Algorithm used to generate the galaxy map seed (None, Planet, MRQ, etc.)."};
	Setting<int> customSeed {*this, "seed.value", PrimeA, "Explicit seed value fed to the star/planet generation RNG.", settings::isSaneSeed};
	Setting<bool> shipSeed {*this, "seed.ships", false, "Derive the starting ship layout from the custom seed instead of a fixed value."};
	Setting<SphereOfInfluenceColors> sphereColors {*this, "visuals.sphereColors", SphereOfInfluenceColors::Default, "Color scheme for alien sphere-of-influence overlays on the starmap."};
	Setting<SphereOfInfluenceMusic> spaceMusic {*this, "audio.spaceMusic", SphereOfInfluenceMusic::None, "Switch hyperspace music to the race whose SOI the flagship is inside."};
	Setting<bool> volasMusic {*this, "audio.musicVolasaurus", false, "Enable the Volasaurus fan-remix music pack when available."};
	Setting<bool> wholeFuel {*this, "interface.wholeFuel", false, "Snap fuel pickup amounts to whole units, avoiding fractional values."};
	Setting<bool> directionalJoystick {*this, "interface.directionalJoystick", false, "Treat joystick input as a D-pad (Android/touch-device support)."};
	Setting<EmulationMode> landerHold {*this, "gameplay.landerHold", EmulationMode::Console3DO, "Lander cargo-hold display: text list (PC) or graphical icons (3DO)."};
	Setting<EmulationMode> scrTrans {*this, "interface.screenTransition", EmulationMode::Console3DO, "Screen-transition style: cut (PC) or crossfade (3DO)."};
	OptionT<Difficulty> optDifficulty {Difficulty::Normal}; // Active difficulty level applied to game mechanics (Normal/Easy/Hard).
	Setting<Difficulty> optDiffChooser {*this, "gameplay.difficulty", Difficulty::ChooseYourOwn, "Difficulty shown in the setup menu (may be ChooseYourOwn)."};
	Setting<FuelRangeDisplay> optFuelRange {*this, "interface.fuelRange", FuelRangeDisplay::Normal, "Starmap fuel-range indicator style (circle, destination, Sol distance)."};
	Setting<bool> extended {*this, "gameplay.extended", false, "Enable extended game content (extra star systems, dialogue, events)."};
	Setting<NomadMode> nomad {*this, "gameplay.nomad", NomadMode::Off, "Nomad variant: starts the player without a homeworld or starbase."};
	Setting<bool> gameOver {*this, "gameplay.gameOver", false, "End the game when the flagship is destroyed instead of retrying."};
	Setting<bool> shipDirectionIP {*this, "visuals.shipDirectionIP", false, "Rotate the flagship icon to face its travel direction in interplanetary space."};
	Setting<bool> hazardColors {*this, "visuals.hazardColors", false, "Color-code planetary hazard indicators (bio, weather, tectonics) distinctly."};
	Setting<bool> orzCompFont {*this, "visuals.orzCompFont", false, "Use a compact font variant for Orz dialogue to fit their unusual phrasing."};

	Setting<ControllerType> optControllerType {*this, "interface.controllerType", ControllerType::KeyboardMouse, "Button-prompt artwork set: keyboard/mouse, Xbox, or PlayStation layout."};
	Setting<bool> smartAutoPilot {*this, "autopilot.smart", false, "Auto-pilot avoids obstacles and optimises the hyperspace route to the destination."};
	Setting<EmulationMode> tintPlanSphere {*this, "visuals.tintPlanetSphere", uqm::EmulationMode::Console3DO, "Tint the planet sphere with its dominant surface color (3DO style)."};
	Setting<EmulationMode> planetStyle {*this, "visuals.planetStyle", uqm::EmulationMode::Console3DO, "Planet rendering pipeline: PC flat-color or 3DO shaded sphere."};
	Setting<int> starBackground {*this, "visuals.starBackground", 0, "Starfield density/style index used in the Hyperspace and combat backgrounds.", settings::isNonNegative};
	Setting<EmulationMode> scanStyle {*this, "visuals.scanStyle", uqm::EmulationMode::Console3DO, "Planetary scan display layout: PC text readout or 3DO graphical panels."};
	Setting<bool> nonStopOscill {*this, "visuals.nonStopOscilloscope", false, "Keep the comm oscilloscope animating continuously instead of only during speech."};
	Setting<EmulationMode> scopeStyle {*this, "visuals.scopeStyle", uqm::EmulationMode::Console3DO, "Oscilloscope visual style: simple PC waveform or stylized 3DO display."};
	Setting<bool> hyperStars {*this, "visuals.hyperStars", false, "Draw animated star-streak effect during hyperspace travel."};
	Setting<EmulationMode> landerStyle {*this, "visuals.landerStyle", uqm::EmulationMode::Console3DO, "Planetary lander mini-game rendering: PC sprites or 3DO visuals."};
	Setting<bool> planetTexture {*this, "visuals.planetTexture", true, "Apply surface detail textures to the rotating planet sphere."};
	Setting<EmulationMode> flagshipColor {*this, "visuals.flagshipColor", uqm::EmulationMode::PC, "Flagship hull color palette: original PC blue or 3DO gold scheme."};
	Setting<bool> noHQEncounters {*this, "cheats.noHQEncounters", false, "Suppress random Hierarchy ship encounters in Ur-Quan-controlled space."};
	Setting<bool> deCleansing {*this, "cheats.deCleansing", false, "Remove the Kohr-Ah Cleansing event so the death march never triggers a loss."};
	Setting<bool> meleeObstacles {*this, "cheats.meleeObstacles", false, "Spawn asteroid/obstacle fields in the melee combat arena."};
	Setting<bool> showVisitedStars {*this, "visuals.showVisitedStars", false, "Mark previously visited star systems with a distinct icon on the starmap."};
	Setting<bool> unscaledStarSystem {*this, "visuals.unscaledStarSystem", false, "Display the solar system view at native canvas resolution without HD scaling."};
	Setting<int> sphereType {*this, "visuals.sphereType", 2, "Visual style index for the sphere-of-influence overlay (filled, outline, etc.).", settings::isNonNegative};
	Setting<int> nebulaevol {*this, "visuals.nebulaeVolume", 16, "Opacity/density of the hyperspace nebula cloud overlay (0–50).", settings::isValidNebulaeVolume};
	Setting<bool> slaughterMode {*this, "gameplay.slaughterMode", false, "Hostile alien ships no longer flee combat, always fighting to the death."};
	Setting<bool> advancedAutoPilot {*this, "autopilot.advanced", false, "Auto-pilot also handles refuelling stops and quasispace portal navigation."};
	Setting<bool> meleeToolTips {*this, "interface.meleeToolTips", false, "Show contextual control hints during melee battles."};
	Setting<int> musicResume {*this, "audio.musicResume", 0, "Music resume mode: 0 = restart track, 1 = resume from last position.", settings::isNonNegative};
	Setting<int> windowEmulationMode {*this, "graphics.windowStyle", 2, "Window chrome/border style: 0 = DOS, 1 = 3DO, 2 = UQM native.", settings::isValidWindowStyle};
	Setting<bool> scatterElements {*this, "gameplay.scatterElements", false, "Scatter mineral element deposits randomly across the planet surface."};
	Setting<bool> showUpgrades {*this, "visuals.showUpgrades", false, "Display purchased ship-upgrade icons in the flagship status screen."};
	Setting<bool> fleetPointSys {*this, "gameplay.fleetPointSystem", false, "Use a fleet-point budget system to limit ships chosen in Full-Game Melee."};
	Setting<bool> shipStore {*this, "gameplay.shipStore", false, "Enable a ship store at the starbase where ally vessels can be purchased."};
	Setting<bool> captainNames {*this, "interface.captainNames", false, "Show individual captain names on ships in the melee roster."};
	Setting<bool> dosMenus {*this, "interface.dosMenus", false, "Use the DOS-style text menus for outfit, crew, and fuel purchasing screens."};

	Setting<int> player1Control {*this, "controls.player1Template", 0, "Control template index used by player one.", settings::isValidControlTemplate};
	Setting<int> player2Control {*this, "controls.player2Template", 3, "Control template index used by player two.", settings::isValidControlTemplate};
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