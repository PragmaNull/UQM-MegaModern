// Reads the credits file: the blocks of text that the credits show.

#pragma once
#ifndef UQM_CREDITSFILE_H
#define UQM_CREDITSFILE_H

#include "core/stl/stl.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

#include <cstdint>

namespace uqm
{

inline constexpr int c_creditsFileVersion {1};
inline constexpr size_t c_creditsMaxColumns {5};
inline constexpr size_t c_creditsMaxLines {50};

enum class CreditsAlign : uint8_t
{
	Left,
	Center,
	Right,
};

struct CreditsColumn
{
	CreditsAlign align {CreditsAlign::Left};
	// The point to align the text to, in SD pixels. Without it, the column uses a default point.
	uqstl::optional<int32_t> x {};
};

struct CreditsBlock
{
	// The font size in points.
	int32_t size {};
	uqstl::vector<CreditsColumn> columns {};
	// The cells of each line, one for each column.
	uqstl::vector<uqstl::vector<uqstl::string>> lines {};
};

// Returns nullopt if 'json' is not a valid credits file.
[[nodiscard]] uqstl::optional<uqstl::vector<CreditsBlock>> creditsFromJson(uqstl::string_view json);

} // namespace uqm

#endif // UQM_CREDITSFILE_H
