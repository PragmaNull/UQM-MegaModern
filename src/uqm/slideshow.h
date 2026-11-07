// Reads slideshow files: the JSON files with the commands of the cutscenes.

#pragma once
#ifndef UQM_SLIDESHOW_H
#define UQM_SLIDESHOW_H

#include <cstdint>

#include "core/stl/array.h"
#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"


namespace uqm
{
inline constexpr int c_slideshowVersion {1};
inline constexpr int32_t c_slideshowMaxFonts {5};
inline constexpr size_t c_slideshowMaxFadeInLines {15};

enum class SlideOp : uint8_t
{
	Dims,
	Font,
	Ani,
	Music,
	Ditty,
	Wait,
	WaitDitty,
	SpinWait,
	Sync,
	Resync,
	DeltaSync,
	BackgroundColor,
	TextColor,
	TextBackColor,
	TextFadeColor,
	TextAlign,
	TextEffect,
	Text,
	TextSpin,
	SpinStat,
	TextFadeIn,
	TextFadeOut,
	SaveBackground,
	RestoreBackground,
	Draw,
	DrawSis,
	Batch,
	Unbatch,
	FadeToColor,
	FadeToBlack,
	FadeToWhite,
	ClearScreen,
	Call,
	Line,
	LineSpin,
	GetRect,
	StatBox,
	Movie,
	Animate,
};

// The resolution that a command is for. A command for the other resolution does nothing.
enum class SlideResolution : uint8_t
{
	Any,
	Sd,
	Hd,
};

enum class SlideTextAlign : uint8_t
{
	Top,
	Middle,
	Bottom,
};

// A command and its values. Each command uses only some of the fields.
struct SlideCommand
{
	SlideOp op {};
	SlideResolution resolution {SlideResolution::Any};
	// Font, frame, or first movie or animation frame.
	int32_t index {};
	int32_t lastIndex {};
	int32_t x {};
	int32_t y {};
	int32_t x2 {};
	int32_t y2 {};
	int32_t width {};
	int32_t height {};
	int32_t ms {};
	int32_t fps {};
	int32_t loops {};
	int32_t filled {};
	int32_t empty {};
	uqstl::optional<int32_t> scale {};
	int32_t angle {};
	uqstl::array<uint8_t, 3> color {};
	SlideTextAlign align {SlideTextAlign::Bottom};
	bool traced {};
	// A file, a ship name for a ditty, or the text to draw.
	uqstl::string text {};
	uqstl::vector<uqstl::string> lines {};
};

// Returns nullopt if 'json' is not a valid slideshow file.
[[nodiscard]] uqstl::optional<uqstl::vector<SlideCommand>> slideshowFromJson(uqstl::string_view json);

} // namespace uqm


#endif // UQM_SLIDESHOW_H
