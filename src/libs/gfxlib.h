//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// JMS 2011: Added LargePoint type - a coordinate point with larger values to avoid overflows in hires modes.

#pragma once
#ifndef UQM_LIBS_GFXLIB_H
#define UQM_LIBS_GFXLIB_H

#include "core/types/enum.h"
#include <fmt/base.h>
#include "port.h"
#include "core/misc/bittricks.h"
#include "libs/time/timelib.h"
#include <math.h>

struct Color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

#include "libs/reslib.h"

struct GraphicsContextDesc;
struct FrameDesc;
struct FontDesc;
struct DrawableDesc;

using GfxContextRef = GraphicsContextDesc*;
using FrameRef = FrameDesc*;
using FontRef = FontDesc*;
using DrawableRef = DrawableDesc*;

using TimeValue = uint16_t;

inline constexpr auto TimeShift {8};
inline constexpr auto MaxTimeValue {(1 << TimeShift) + 1};


static inline bool sameColor(Color c1, Color c2)
{
	return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a;
}

static inline bool sameColor24(Color c1, Color c2)
{
	return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

// Transform a 5-bits color component to an 8-bits color component.
// Form 1, calculates '(r5 / 31.0) * 255.0, highest value is 0xff:
constexpr auto Cc5to8(auto c) { return (((c) << 3) | ((c) >> 2)); }
// Form 2, calculates '(r5 / 32.0) * 256.0, highest value is 0xf8:
//#define Cc5to8(c) ((c) << 3)

constexpr Color BuildColor(Color col, [[maybe_unused]] int c256) { return col; }
// BuildColor used to combine a 15-bit RGB color tripple with a
// destination VGA palette index into a 32-bit value.
// Now, it is an empty wrapper which returns the first argument,
// which is of type Color, and ignores the second argument,
// the palette index.
//
// It is a remnant of 8bpp hardware paletted display (VGA).
// The palette index would be overwritten with the RGB value
// and the drawing op would use this index on screen.
// The palette indices 0-15, as used in DOS SC2, are unchanged
// from the standard VGA palette and are identical to 16-color EGA.
// Various frames, borders, menus, etc. frequently refer to these
// first 16 colors and normally do not change the RGB values from
// the standard ones (see colors.h; most likely unchanged from SC1)
// The palette index is meaningless in UQM for the most part.
// New code should just use index 0.

// Turn a 15 bits color into a 24-bits color.
// r, g, and b are each 5-bits color components.
constexpr Color colorFromRgb15(uint8_t r, uint8_t g, uint8_t b)
{
	return {static_cast<uint8_t>(Cc5to8(r)), static_cast<uint8_t>(Cc5to8(g)), static_cast<uint8_t>(Cc5to8(b)), 0xff};
}
constexpr Color MakeRgb15(uint8_t r, uint8_t g, uint8_t b) { return colorFromRgb15(r, g, b); }

constexpr Color buildColorRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return {r, g, b, a};
}
constexpr Color BuildColorRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return buildColorRgba(r, g, b, a); }

constexpr auto BuildShadeRgba(auto s) { return buildColorRgba((s), (s), (s), 0xFF); }

constexpr auto BuildColorRgb(auto r, auto g, auto b) { return buildColorRgba((r), (g), (b), 0xFF); }

static inline void IncreaseBrightness(uint8_t* ch, uint8_t value)
{
	int c {};
	if (*ch < 128)
	{
		c = ((*ch * 255) >> 7);
	}
	else
	{
		c = (((*ch + 255) << 1) - 255 - ((255 * *ch) >> 7));
	}

	if (c > 0xFF)
	{
		c = 0xFF;
	}

	if (value == 0xFF)
	{
		*ch = (uint8_t)c;
	}
	else
	{
		*ch = (uint8_t)(((c - *ch) * value) >> 8) + *ch;
	}
}

static inline bool AreTheyShades(Color first_color, Color second_color)
{
	return (bool)((first_color.r == first_color.g
				   && first_color.g == first_color.b)
				  && (second_color.r == second_color.g
					  && second_color.g == second_color.b));
}

static inline Color CreateAvgShade(Color first_color, Color second_color)
{
	Color temp;

	temp = buildColorRgba(0, 0, 0, 0);

	if (first_color.r > second_color.r)
	{
		temp.r = first_color.r - second_color.r;
		temp.g = temp.r;
		temp.b = temp.r;
	}

	if (first_color.r < second_color.r)
	{
		temp.r = second_color.r - first_color.r;
		temp.g = temp.r;
		temp.b = temp.r;
	}

	if (sameColor(first_color, second_color))
	{
		return first_color;
	}

	if (temp.r == first_color.r || temp.r == second_color.r)
	{
		temp.r = (first_color.r + second_color.r) >> 1;
		temp.g = temp.r;
		temp.b = temp.r;
	}

	if (temp.r > 0)
	{
		temp.a = 255;
	}

	return temp;
}


using CreateFlags = uint8_t;
// WantMask is deprecated (and non-functional). It used to generate a bitmap
// of changed pixels for a target DrawableRef, so that DRAW_SUBTRACTIVE could
// paint background pixels over them, i.e. a revert draw. The backgrounds
// are fully erased now instead.
inline constexpr auto WantMask {(CreateFlags)(1 << 0)};
inline constexpr auto WantPixmap {(CreateFlags)(1 << 1)};
// MappedToDisplay is deprecated but still checked by LoadDisplayPixmap().
// Its former use was to indicate a pre-scaled graphic for the display.
inline constexpr auto MappedToDisplay {(CreateFlags)(1 << 2)};
inline constexpr auto WantAlpha {(CreateFlags)(1 << 3)};

struct GfxExtent
{
	int16_t width, height;
};

// JMS: Extent with larger values to avoid overflows in hires modes.
struct LargeExtent
{
	int32_t width, height;
};

struct GfxPoint
{
	constexpr GfxPoint() = default;
	constexpr GfxPoint(int16_t _x, int16_t _y)
		: x {_x}
		, y {_y}
	{
	}
	constexpr GfxPoint(int _x, int _y)
		: x {(int16_t)_x}
		, y {(int16_t)_y}
	{
	}
	constexpr GfxPoint(float _x, float _y)
		: x {(int16_t)_x}
		, y {(int16_t)_y}
	{
	}
	constexpr GfxPoint(double _x, double _y)
		: x {(int16_t)_x}
		, y {(int16_t)_y}
	{
	}

	int16_t x {};
	int16_t y {};
};

inline GfxPoint operator+(const GfxPoint& lhs, const GfxPoint& rhs)
{
	return {static_cast<int16_t>(lhs.x + rhs.x), static_cast<int16_t>(lhs.y + rhs.y)};
}


// JMS: coordinate point with larger values to avoid overflows in hires modes.
struct LargePoint
{
	int32_t x, y;
};

struct GfxStamp
{
	GfxPoint origin;
	FrameRef frame;
};

struct GfxRect
{
	GfxPoint corner;
	GfxExtent extent;
};

// Kruzen: Thanks to JMS, using this to draw ovals
// Overflows happen in HdResolutionFactor with Starmap and max zoom (Fuel circles)
struct LargeRect
{
	LargePoint corner;
	LargeExtent extent;
};

struct GfxLine
{
	GfxPoint first, second;
};

static inline GfxPoint MAKE_POINT(int16_t x, int16_t y)
{
	GfxPoint pt = {x, y};
	return pt;
}

static inline LargePoint MAKE_DPOINT(int32_t x, int32_t y)
{
	LargePoint pt = {x, y};
	return pt;
}

static inline GfxExtent MAKE_EXTENT(int16_t width, int16_t height)
{
	GfxExtent ext = {width, height};
	return ext;
}

static inline LargeExtent MAKE_DEXTENT(int32_t width, int32_t height)
{
	LargeExtent ext = {width, height};
	return ext;
}

// Kruzen: Some DrawOval() calls still use standard rect where overflow is impossible as it's 2 figures away from that
// Used to draw SOI and planet orbits
// To avoid any typedef conflicts - transform standard GfxRect to LargeRect
static inline LargeRect RECT_TO_DRECT(GfxRect r)
{
	LargeRect dr = {
		{r.corner.x,	 r.corner.y	   },
		{r.extent.width, r.extent.height}
	 };

	return dr;
}

//static inline void
//MAKE_LINE (GfxLine *line, int x1, int y1, int x2, int y2)
//{
//	line->first.x = x1;
//	line->first.y = y1;
//	line->second.x = x2;
//	line->second.y = y2;
//}

static inline void MAKE_LINE(GfxLine* line, GfxPoint first, GfxPoint second)
{
	line->first = first;
	line->second = second;
}

static inline bool pointsEqual(GfxPoint p1, GfxPoint p2)
{
	return p1.x == p2.x && p1.y == p2.y;
}

static inline bool extentsEqual(GfxExtent e1, GfxExtent e2)
{
	return e1.width == e2.width && e1.height == e2.height;
}

static inline bool rectsEqual(GfxRect r1, GfxRect r2)
{
	return pointsEqual(r1.corner, r2.corner)
		&& extentsEqual(r1.extent, r2.extent);
}

static inline bool pointWithinRect(GfxRect r, GfxPoint p)
{
	return p.x >= r.corner.x && p.y >= r.corner.y
		&& p.x < r.corner.x + r.extent.width
		&& p.y < r.corner.y + r.extent.height;
}

static inline double ptDistance(GfxPoint p1, GfxPoint p2)
{
	return (sqrt(pow((double)p2.x - (double)p1.x, 2)
				 + pow((double)p2.y - (double)p1.y, 2)));
}

static inline double calcDistance(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	const double dx = (double)x2 - (double)x1;
	const double dy = (double)y2 - (double)y1;

	return (sqrt(pow(dx, 2) + pow(dy, 2)));
}

static inline void printPt(GfxPoint pt, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, pt.x, pt.y);
}
#define PRINT_PT(pt) \
	printPt(pt, #pt)

static inline void printDPt(LargePoint dPt, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, dPt.x, dPt.y);
}
#define PRINT_DPT(dpt) \
	printDPt(dpt, #dpt)

static inline void printExt(GfxExtent ext, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, ext.width, ext.height);
}
#define PRINT_EXT(ext) \
	printExt(ext, #ext)

static inline void printDExt(LargeExtent dExt, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, dExt.width, dExt.height);
}
#define PRINT_DEXT(dext) \
	printDExt(dext, #dext)

static inline void printRect(GfxRect r, char* Str)
{
	fmt::print("{}.corner = {} x {}\n", Str, r.corner.x, r.corner.y);
	fmt::print("{}.extent = {} x {}\n", Str, r.extent.width, r.extent.height);
}
#define PRINT_RECT(rect) \
	printRect(rect, #rect)

static inline void printDRect(LargeRect r, char* Str)
{
	fmt::print("{}.corner = {} x {}\n", Str, r.corner.x, r.corner.y);
	fmt::print("{}.extent = {} x {}\n", Str, r.extent.width, r.extent.height);
}
#define PRINT_DRECT(drect) \
	printDRect(drect, #drect)

static inline void ZeroPoint(GfxPoint* pt)
{
	pt->x = pt->y = ~0;
}

static inline bool ValidPoint(GfxPoint pt)
{
	return (bool)(pt.x != ~0 && pt.y != ~0);
}

enum class TextAlign
{
	Left,
	Center,
	Right
};

enum class TextVAlign
{
	Top,
	Middle,
	Bottom
};
consteval bool isFlagEnum(TextVAlign) { return true; }

struct GfxText
{
	GfxPoint baseline;
	const char* pStr;
	TextAlign align;
	uint16_t CharCount;
};

#include "libs/strlib.h"

using ColorMapRef = uqm::StringTable*;
// COLORMAPPTR is really a pointer to colortable entry structure
// which is documented in doc/devel/strtab, .ct files section
const typedef void* COLORMAPPTR;

#include "graphics/prim.h"


using BatchFlags = uint8_t;
// This flag is currently unused but it might make sense to restore it
inline constexpr auto BatchBuildPage {(BatchFlags)(1 << 0)};

struct IntersectInfo
{
	TimeValue last_time_val;
	GfxPoint EndPoint;
	GfxStamp IntersectStamp;
};

using IntersectCode = uint8_t;

inline constexpr auto IntersectLeft {(IntersectCode)(1 << 0)};
inline constexpr auto IntersectTop {(IntersectCode)(1 << 1)};
inline constexpr auto IntersectRight {(IntersectCode)(1 << 2)};
inline constexpr auto IntersectBottom {(IntersectCode)(1 << 3)};
inline constexpr auto IntersectNoclip {(IntersectCode)(1 << 7)};
inline constexpr auto IntersectAllSides {(IntersectCode)(IntersectLeft | IntersectTop | IntersectRight | IntersectBottom)};

using HotSpot = GfxPoint;

extern HotSpot MAKE_HOT_SPOT(int16_t, int16_t);

extern IntersectCode BoxIntersect(GfxRect* pr1, GfxRect* pr2, GfxRect* printer);
extern void BoxUnion(GfxRect* pr1, GfxRect* pr2, GfxRect* punion);

enum class ScreenFadeType
{
	FadeAllToWhite = 250,
	FadeSomeToWhite,
	FadeAllToBlack,
	FadeAllToColor,
	FadeSomeToBlack,
	FadeSomeToColor
};

enum class DrawKind : uint8_t
{
	Replace = 0,
	// Pixels in the target FrameRef are replaced entirely.
	// Non-stamp primitives with Color.a < 255 to RGB targets are
	// equivalent to Alpha with (DrawMode.factor = Color.a),
	// except the Text primitives.
	// DrawMode.factor: ignored
	// Text: supported (except Alpha via Color.a)
	// RGBA sources (WantAlpha): per-pixel alpha blending performed
	// RGBA targets (WantAlpha): replace directly supported
	Additive,
	// Pixel channels of the source FrameRef or Color channels of
	// a primitive are modulated by (DrawMode.factor / 255) and added
	// to the pixel channels of the target FrameRef.
	// DrawMode.factor range: -32767..32767 (negative values make
	//    draw subtractive); 255 = 1:1 ratio
	// Text: not yet supported
	// RGBA sources (WantAlpha): alpha channel ignored
	// RGBA targets (WantAlpha): not yet supported
	Alpha,
	// Pixel channels of the source FrameRef or Color channels of
	// a primitive are modulated by (DrawMode.factor / 255) and added
	// to the pixel channels of the target FrameRef, modulated by
	// (1 - DrawMode.factor / 255)
	// DrawMode.factor range: 0..255; 255 = fully opaque
	// Text: supported
	// RGBA sources (WantAlpha): alpha channel ignored
	// RGBA targets (WantAlpha): not yet supported
	Multiply,
	// Pixel channels of the source FrameRef or Color channels of
	// a primitive are modulated by (channel / 255) and multiplied
	// by pixel channels of the target FrameRef, modulated by
	// (channel / 255)
	// DrawMode.factor range: not yet supported
	// Text: supported
	// RGBA sources (WantAlpha): alpha channel ignored
	// RGBA targets (WantAlpha): not yet supported
	Overlay,
	Screen,
	Grayscale, // To get true grayscale - factor should be == 128
	Linearburn,
	Hyptoquas,
	Desaturate, // Desaturate image. Not optimized for high fps

	Default = Replace,
};

struct DrawMode
{
	DrawKind kind;
	int16_t factor;
};

inline constexpr auto DrawFactor1 {0xff};
inline constexpr auto TransferAlpha {0x7fff};

inline constexpr auto DesatAmount {0xBE};

constexpr DrawMode MAKE_DRAW_MODE(DrawKind kind, int16_t factor)
{
	return {kind, factor};
}

inline constexpr DrawMode DrawReplaceMode {MAKE_DRAW_MODE(DrawKind::Replace, 0)};

enum class ShadowAngle
{
	North,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest,
};

extern GfxContextRef SetContext(GfxContextRef Context);
extern Color SetContextForeGroundColor(Color Color);
extern Color GetContextForeGroundColor(void);
extern Color SetContextBackGroundColor(Color Color);
extern Color GetContextBackGroundColor(void);
extern FrameRef SetContextFGFrame(FrameRef Frame);
extern FrameRef GetContextFGFrame(void);
// Context cliprect defines the drawing bounds. Additionally, all
// drawing positions (x,y) are relative to the cliprect corner.
extern bool SetContextClipRect(GfxRect* pRect);
// The returned rect is always filled in. If the context cliprect
// is undefined, the returned rect has foreground frame dimensions.
extern bool GetContextClipRect(GfxRect* pRect);
// The actual origin will be orgOffset + context ClipRect.corner
extern GfxPoint SetContextOrigin(GfxPoint orgOffset);
extern DrawMode SetContextDrawMode(DrawMode);
extern DrawMode GetContextDrawMode(void);
// 'area' may be nullptr to copy the entire GfxContextRef cliprect
// 'area' is relative to the GfxContextRef cliprect
extern DrawableRef CopyContextRect(const GfxRect* area);

extern TimeValue DrawablesIntersect(IntersectInfo* pControl0,
									IntersectInfo* pControl1, TimeValue max_time_val);
extern void DrawStamp(GfxStamp* pStamp);
extern void DrawFilledStamp(GfxStamp* pStamp);
extern void DrawPoint(GfxPoint* pPoint);
extern void DrawRectangle(GfxRect* pRect, bool scaled);
extern void DrawFilledRectangle(GfxRect* pRect);
extern void DrawLine(GfxLine* pLine, uint8_t thickness);
extern void ApplyMask(FrameRef layer, FrameRef base, DrawMode mode, Color* fill);
extern void InstaPoint(int x, int y);
extern void InstaRect(int x, int y, int w, int h, bool scaled);
extern void InstaFilledRect(int x, int y, int w, int h);
extern void InstaLine(int x1, int y1, int x2, int y2);
extern GfxRect font_GetTextRect(GfxText* pText);
extern void font_DrawText(GfxText* pText);
extern void font_DrawText_Fade(GfxText* lpText, FrameRef repair, bool* skip);
extern void font_DrawTracedText(GfxText* pText, Color text, Color trace);
extern uint8_t font_DrawTextAlt(GfxText* lpText, uint8_t swap, FontRef AltFontPtr, uqm::UniChar key);
extern void font_DrawTracedTextAlt(GfxText* pText, Color text, Color trace, FontRef AltFontPtr,
								   uqm::UniChar key);
extern void font_DrawShadowedText(GfxText* pText, ShadowAngle direction,
								  Color text_color, Color shadow_color);
extern void DrawBatch(Primitive* pBasePrim, PrimLinks primLinks,
					  BatchFlags batchFlags);
extern void BatchGraphics(void);
extern void UnbatchGraphics(void);
extern void FlushGraphics(void);
extern void ClearDrawable(void);
extern void ClearScreen(void);
#ifdef DEBUG
extern GfxContextRef CreateContextAux(const char* name);
constexpr auto CreateContext(auto name) { return CreateContextAux((name)); }
#else  /* if !defined(DEBUG) */
extern GfxContextRef CreateContextAux(void);
constexpr auto CreateContext(auto name) { return CreateContextAux(); }
#endif /* !defined(DEBUG) */
extern bool DestroyContext(GfxContextRef ContextRef);
extern DrawableRef CreateDisplay(CreateFlags createFlags, int16_t* pwidth,
								 int16_t* pheight);
extern DrawableRef CreateDrawable(CreateFlags createFlags, int16_t width,
								  int16_t height, uint16_t num_frames);
extern bool DestroyDrawable(DrawableRef Drawable);
extern bool GetFrameRect(FrameRef Frame, GfxRect* pRect);
#ifdef DEBUG
const extern char* GetContextName(GfxContextRef context);
extern GfxContextRef GetFirstContext(void);
extern GfxContextRef GetNextContext(GfxContextRef context);
extern size_t GetContextCount(void);
#endif /* DEBUG */

extern HotSpot SetFrameHot(FrameRef Frame, HotSpot hotSpot);
extern HotSpot GetFrameHot(FrameRef Frame);
extern bool InstallGraphicResTypes(void);
extern DrawableRef LoadGraphicFile(const char* pStr);
extern FontRef LoadFontFile(const char* pStr);
extern void* LoadGraphicInstance(RESOURCE res);
extern DrawableRef LoadDisplayPixmap(const GfxRect* area, FrameRef frame);
extern FrameRef SetContextFontEffect(FrameRef EffectFrame);
extern FontRef SetContextFont(FontRef Font);
extern bool DestroyFont(FontRef fontRef);
// The returned pRect is relative to the context drawing origin
extern bool TextRect(GfxText* pText, GfxRect* pRect, uint8_t* pdelta);
extern bool TextRectAlt(GfxText* lpText, GfxRect* pRect, uint8_t* pdelta, uint8_t swap, uqm::UniChar key, FontRef AltFontPtr);
extern bool GetContextFontLeading(int16_t* pheight);
extern bool GetContextFontDispHeight(int16_t* pheight);
extern bool GetContextFontDispWidth(int16_t* pwidth);
extern uint16_t GetFrameCount(FrameRef Frame);
extern uint16_t GetFrameIndex(FrameRef Frame);
extern FrameRef SetAbsFrameIndex(FrameRef Frame, uint16_t FrameIndex);
inline FrameRef SetAbsFrameIndex(FrameRef Frame, ArithmeticEnum auto FrameIndex)
{
	return SetAbsFrameIndex(Frame, static_cast<uint16_t>(*FrameIndex));
}
extern FrameRef SetRelFrameIndex(FrameRef Frame, int16_t FrameOffs);
extern FrameRef SetEquFrameIndex(FrameRef DstFrame, FrameRef SrcFrame);
extern FrameRef IncFrameIndex(FrameRef Frame);
extern FrameRef DecFrameIndex(FrameRef Frame);
extern DrawableRef CopyFrameRect(FrameRef Frame, const GfxRect* area);
extern DrawableRef CloneFrame(FrameRef Frame);
extern DrawableRef RotateFrame(FrameRef Frame, int angle_deg);
extern DrawableRef RescaleFrame(FrameRef frame, int width, int height);
extern DrawableRef RescalePercentage(FrameRef frame, float percentage);
// This pair works for both paletted and trucolor frames
extern bool ReadFramePixelColors(FrameRef frame, Color* pixels,
								 int width, int height);
extern bool WriteFramePixelColors(FrameRef frame, const Color* pixels,
								  int width, int height);
// This pair only works for paletted frames
extern bool ReadFramePixelIndexes(FrameRef frame, uint8_t* pixels,
								  int width, int height, bool paletted);
extern bool WriteFramePixelIndexes(FrameRef frame, const uint8_t* pixels,
								   int width, int height);
extern void SetFrameTransparentColor(FrameRef, Color);
extern bool IsFrameIndexed(FrameRef Frame);

// If the frame is an active ScreenDrawable, this call must be
// preceeded by FlushGraphics() for draw commands to have taken effect
extern Color GetFramePixel(FrameRef, GfxPoint pixelPt);

extern FrameRef CaptureDrawable(DrawableRef Drawable);
extern DrawableRef ReleaseDrawable(FrameRef Frame);

extern DrawableRef GetFrameParentDrawable(FrameRef Frame);

extern bool SetColorMap(COLORMAPPTR ColorMapPtr);
extern TimeCount XFormColorMap(COLORMAPPTR ColorMapPtr, TimePeriod TimeInterval);
extern TimeCount FadeScreen(ScreenFadeType fadeType, TimePeriod TimeInterval);
extern void FlushColorXForms(void);
extern uint8_t GetColorMapTableIndex(ColorMapRef map, size_t index);

// The color map 'index' of 'map'. The index wraps at the number of color maps.
extern COLORMAPPTR GetColorMapAddress(ColorMapRef map, size_t index);

void SetSystemRect(const GfxRect* pRect);
void ClearSystemRect(void);

#endif // UQM_LIBS_GFXLIB_H
