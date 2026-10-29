#pragma once
#ifndef OPTIONS_OPTIONDEFS_H_
#define OPTIONS_OPTIONDEFS_H_

#include "libs/graphics/gfx_defs.h"	  // for GfxFlags
#include "libs/sound/audiocoredefs.h" // for AudioFlags.
namespace uqm
{

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class SeedType
{
	None = 0,
	Planet = 1,
	MRQ = 2,
	Prime = 3,
	StarSeed = 4
};

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class EmulationMode : int
{
	None = 0x0,
	Console3DO = 0x1, // would just call it "3d0" but enum names can't start with a number.
	PC = 0x2,
	All = 0xFF
};
ENUM_BITWISE_OPS(EmulationMode);

static constexpr const char* toStringImpl(const EmulationMode e)
{
	switch (e)
	{
		case EmulationMode::None:
			return "None";
		case EmulationMode::Console3DO:
			return "3DO";
		case EmulationMode::PC:
			return "PC";
		case EmulationMode::All:
			return "All";
	}
	return "???";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class WindowMode
{
	Windowed,
	WindowedFullscreen,
	Fullscreen,
};

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class ScalingMode : int
{
	None,
	Bilinear = static_cast<int>(GfxFlags::Scale_Bilinear),
	BiAdapt = static_cast<int>(GfxFlags::Scale_BiAdapt),
	BiAdaptAdv = static_cast<int>(GfxFlags::Scale_BiAdaptAdv),
	Triscan = static_cast<int>(GfxFlags::Scale_Triscan),
	HQ = static_cast<int>(GfxFlags::Scale_HQXX),
};
static constexpr GfxFlags toGfxFlags(const ScalingMode mode)
{
	return static_cast<GfxFlags>(mode);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class MeleeScaleMode : int
{
	Step,
	PC,
	Nearest,
	Bilinear,
	Console3DO,
	Smooth,
};
static constexpr TFBScaleMode toTFBScaleMode(const MeleeScaleMode mode)
{
	switch (mode)
	{
		case MeleeScaleMode::Step:
			return TFBScaleMode::Step;
		case MeleeScaleMode::PC:
			return TFBScaleMode::Step;
		case MeleeScaleMode::Nearest:
			return TFBScaleMode::Nearest;
		case MeleeScaleMode::Bilinear:
			return TFBScaleMode::Bilinear;
		case MeleeScaleMode::Console3DO:
			return TFBScaleMode::Trilinear;
		case MeleeScaleMode::Smooth:
			return TFBScaleMode::Trilinear;
	}
	return TFBScaleMode::Step;
}
static constexpr MeleeScaleMode toMeleeScaleMode(const TFBScaleMode mode)
{
	switch (mode)
	{
		case TFBScaleMode::None:
			break;
		case TFBScaleMode::Step:
			return MeleeScaleMode::PC; // also MeleeScaleMode::Step
		case TFBScaleMode::Nearest:
			return MeleeScaleMode::Nearest;
		case TFBScaleMode::Bilinear:
			return MeleeScaleMode::Bilinear;
		case TFBScaleMode::Trilinear:
			return MeleeScaleMode::Console3DO; // also MeleeScaleMode::Smooth
	}
	return MeleeScaleMode::Step;
}


static constexpr const char* toStringImpl(const MeleeScaleMode mode)
{
	switch (mode)
	{
		case MeleeScaleMode::Step: // = TFB_SCALE_STEP,
			return "Step";
		case MeleeScaleMode::PC: // = TFB_SCALE_STEP,
			return "PC";
		case MeleeScaleMode::Nearest: // = TFB_SCALE_NEAREST,
			return "Nearest";
		case MeleeScaleMode::Bilinear: // = TFB_SCALE_BILINEAR,
			return "Bilinear";
		case MeleeScaleMode::Console3DO: // = TFB_SCALE_TRILINEAR,
			return "3DO";
		case MeleeScaleMode::Smooth: // = TFB_SCALE_TRILINEAR,
			return "Smooth";
	}
	return "???";
}


///////////////////////////////////////////////////////////////////////////////////////////////////

enum class AudioQuality
{
	Low,
	Medium,
	High,
};
static constexpr AudioFlags toAudioFlags(AudioQuality quality)
{
	switch (quality)
	{
		case AudioQuality::Low:
			return AudioFlags::QualityLow;
		case AudioQuality::Medium:
			return AudioFlags::QualityMedium;
		case AudioQuality::High:
			return AudioFlags::QualityHigh;
	}
	return AudioFlags::None;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class GodModeFlags
{
	None = 0,
	InfiniteBattery = 1 << 0,
	NoDamage = 1 << 1,

	All = InfiniteBattery | NoDamage
};
ENUM_BITWISE_OPS(GodModeFlags);

////////////////////////////////////////////////////////////////////////////////////////////////////

// seems restrictive to have only these four options, but when one considers space constraints,
// it makes sense, I usppose.
enum class DateFormat
{
	MMM_dd_yyyy, //MMM dd.yyyy ie: Mar 01.2500
	MM_dd_yyyy,	 //MM.dd.yyyy  ie: 03.01.2500
	dd_MMM_yyyy, //dd MMM yyyy ie: 01 Mar 2500
	dd_MM_yyyy	 //dd.MM.yyyy  ie: 01.03.2500
};
static constexpr const char* toStringImpl(const DateFormat fmt)
{
	switch (fmt)
	{
		case DateFormat::MMM_dd_yyyy:
			return "MMM dd.yyyy";
		case DateFormat::MM_dd_yyyy:
			return "MM.dd.yyyy";
		case DateFormat::dd_MMM_yyyy:
			return "dd MMM yyyy";
		case DateFormat::dd_MM_yyyy:
			return "dd.MM.yyyy";
	}
	return "???";
}
////////////////////////////////////////////////////////////////////////////////////////////////////

enum class Difficulty : uint8_t
{
	Normal,
	Easy,
	Hard,
	ChooseYourOwn, // shows difficulty selection screen at the start of a new game, like in the original 3DO version. Note that this doesn't actually change any game parameters, it's just for show.
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class SphereOfInfluenceColors
{
	Default,
	Starseed,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class SphereOfInfluenceMusic
{
	None,
	RaceAfterDiscovery,
	RaceBeforeDiscovery,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class FuelRangeDisplay
{
	Normal,
	FuelAtDestination,
	FuelToSol,
	All = FuelAtDestination | FuelToSol
};
ENUM_BITWISE_OPS(FuelRangeDisplay);

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class NomadMode : uint8_t
{
	Off,
	Easy,
	Normal
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class CoarseScanStyle
{
	Text,
	Pictograms,
	P6014Text,
	P6014Pictograms,
};

[[nodiscard]] constexpr bool usesPictograms(CoarseScanStyle style) noexcept
{
	return style == CoarseScanStyle::Pictograms || style == CoarseScanStyle::P6014Pictograms;
}

// All styles except Text use the alternative scan title colors.
[[nodiscard]] constexpr bool usesAltScanColors(CoarseScanStyle style) noexcept
{
	return style != CoarseScanStyle::Text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class ControllerType
{
	KeyboardMouse,
	XBox,
	Playstation,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace uqm

#endif /* OPTIONS_OPTIONDEFS_H_ */