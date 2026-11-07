// Reads font metrics files: the JSON files that give the spacing and kerning of a font.

#pragma once
#ifndef UQM_LIBS_GRAPHICS_FONTFILE_H
#define UQM_LIBS_GRAPHICS_FONTFILE_H

#include "core/stl/stl.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "core/string/Utf8.h"

#include <cstdint>

namespace uqm
{

inline constexpr int c_fontMetricsVersion {1};
inline constexpr uint8_t c_fontMaxKern {3};

struct FontKern
{
	UniChar ch {};
	uint8_t left {};
	uint8_t right {};
};

struct FontMetrics
{
	uint8_t leading {};
	uint8_t charSpace {};
	uint8_t kernAmount {};
	int8_t vertAlign {};
	uqstl::vector<FontKern> kerning {};
};

// Returns nullopt if 'json' is not a valid font metrics file.
[[nodiscard]] uqstl::optional<FontMetrics> fontMetricsFromJson(uqstl::string_view json);

} // namespace uqm

#endif // UQM_LIBS_GRAPHICS_FONTFILE_H
