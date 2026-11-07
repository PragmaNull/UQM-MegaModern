// Reads animation files: the JSON files that list the images of the frames of a drawable.

#pragma once
#ifndef UQM_LIBS_GRAPHICS_ANIFILE_H
#define UQM_LIBS_GRAPHICS_ANIFILE_H

#include <cstdint>

#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

namespace uqm
{

// Use the transparency of the PNG file.
inline constexpr int32_t c_aniTransparencyPng {-2};
inline constexpr int32_t c_aniTransparencyNone {-1};
inline constexpr int32_t c_aniNoColormap {-1};

inline constexpr int c_aniFileVersion {1};

struct AniFrame
{
	// Relative to the directory of the animation file.
	uqstl::string image {};
	// c_aniTransparencyPng, c_aniTransparencyNone, or the index of the transparent color.
	int32_t transparentColor {c_aniTransparencyPng};
	int32_t colormapIndex {c_aniNoColormap};
	int32_t hotspotX {};
	int32_t hotspotY {};
};

// Returns nullopt if 'json' is not a valid animation file.
[[nodiscard]] uqstl::optional<uqstl::vector<AniFrame>> aniFromJson(uqstl::string_view json);

} // namespace uqm

#endif // UQM_LIBS_GRAPHICS_ANIFILE_H
