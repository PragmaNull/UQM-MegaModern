#pragma once
#ifndef LIBS_GRAPHICS_GFX_DEFS_H_
#define LIBS_GRAPHICS_GFX_DEFS_H_

#include "core/types/enum.h"

namespace uqm
{

// driver for TFB_InitGraphics
enum class GfxDriver
{
	SDL_Pure,
};

// forced redraw
enum class TFBRedraw
{
	No,
	Fading,
	Expose,
	Yes,
};

enum TFBScaleMode : int
{
	None,
	Step,
	Nearest,
	Bilinear,
	Trilinear,
};

// flags for TFB_InitGraphics
enum class GfxFlags
{
	None = 0,
	Fullscreen = 1,
	ExclusiveFullscreen = 1 << 1,
	ShowFPS = 1 << 2,
	Scanlines = 1 << 3,
	Scale_Bilinear = 1 << 4,
	Scale_BiAdapt = 1 << 5,
	Scale_BiAdaptAdv = 1 << 6,
	Scale_Triscan = 1 << 7,
	Scale_HQXX = 1 << 8,
};
ENUM_BITWISE_OPS(GfxFlags);

static inline constexpr GfxFlags GfxFlagsFullscreen {GfxFlags::Fullscreen | GfxFlags::ExclusiveFullscreen};
static inline constexpr GfxFlags GfxFlagsScaleAny {GfxFlags::Scale_Bilinear | GfxFlags::Scale_BiAdapt | GfxFlags::Scale_BiAdaptAdv | GfxFlags::Scale_Triscan | GfxFlags::Scale_HQXX};
static inline constexpr GfxFlags GfxFlagsScaleSoftwareOnly {GfxFlagsScaleAny & ~GfxFlags::Scale_Bilinear};


} // namespace uqm

#endif /* LIBS_GRAPHICS_GFX_DEFS_H_ */