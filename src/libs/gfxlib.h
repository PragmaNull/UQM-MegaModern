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

// JMS 2011: Added DPOINT type - a coordinate point with larger values to avoid overflows in hires modes.

#ifndef LIBS_GFXLIB_H_
#define LIBS_GFXLIB_H_

#include <fmt/base.h>
#include "port.h"
#include "core/misc/bittricks.h"
#include "libs/time/timelib.h"
#include <math.h>

typedef struct Color Color;
struct Color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

#include "libs/reslib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

typedef struct context_desc CONTEXT_DESC;
typedef struct frame_desc FRAME_DESC;
typedef struct font_desc FONT_DESC;
typedef struct drawable_desc DRAWABLE_DESC;

typedef CONTEXT_DESC* GFXCONTEXT;
typedef FRAME_DESC* FRAME;
typedef FONT_DESC* FONT;
typedef DRAWABLE_DESC* DRAWABLE;

typedef uint16_t TIME_VALUE;

inline constexpr auto TimeShift {8};
inline constexpr auto MaxTimeValue {((1 << TimeShift) + 1)};


static inline bool
sameColor(Color c1, Color c2)
{
	return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a;
}

static inline bool
sameColor24(Color c1, Color c2)
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
constexpr Color
colorFromRgb15(uint8_t r, uint8_t g, uint8_t b)
{
	return {static_cast<uint8_t>(Cc5to8(r)), static_cast<uint8_t>(Cc5to8(g)), static_cast<uint8_t>(Cc5to8(b)), 0xff};
}
constexpr Color MakeRgb15(uint8_t r, uint8_t g, uint8_t b) { return colorFromRgb15(r, g, b); }

constexpr Color
buildColorRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return {r, g, b, a};
}
constexpr Color BuildColorRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return buildColorRgba(r, g, b, a); }

constexpr auto BuildShadeRgba(auto s) { return buildColorRgba((s), (s), (s), 0xFF); }

constexpr auto BuildColorRgb(auto r, auto g, auto b) { return buildColorRgba((r), (g), (b), 0xFF); }

static inline void
IncreaseBrightness(uint8_t* ch, uint8_t value)
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

static inline bool
AreTheyShades(Color first_color, Color second_color)
{
	return (bool)((first_color.r == first_color.g
				   && first_color.g == first_color.b)
				  && (second_color.r == second_color.g
					  && second_color.g == second_color.b));
}

static inline Color
CreateAvgShade(Color first_color, Color second_color)
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


typedef uint8_t CREATE_FLAGS;
// WantMask is deprecated (and non-functional). It used to generate a bitmap
// of changed pixels for a target DRAWABLE, so that DRAW_SUBTRACTIVE could
// paint background pixels over them, i.e. a revert draw. The backgrounds
// are fully erased now instead.
inline constexpr auto WantMask {(CREATE_FLAGS)(1 << 0)};
inline constexpr auto WantPixmap {(CREATE_FLAGS)(1 << 1)};
// MappedToDisplay is deprecated but still checked by LoadDisplayPixmap().
// Its former use was to indicate a pre-scaled graphic for the display.
inline constexpr auto MappedToDisplay {(CREATE_FLAGS)(1 << 2)};
inline constexpr auto WantAlpha {(CREATE_FLAGS)(1 << 3)};

struct EXTENT
{
	int16_t width, height;
};

// JMS: Extent with larger values to avoid overflows in hires modes.
typedef struct dextent
{
	int32_t width, height;
} DEXTENT;

typedef struct point
{
	constexpr point() = default;
	constexpr point(int16_t _x, int16_t _y)
		: x {_x}
		, y {_y}
	{
	}
	constexpr point(int _x, int _y)
		: x {(int16_t)_x}
		, y {(int16_t)_y}
	{
	}
	constexpr point(float _x, float _y)
		: x {(int16_t)_x}
		, y {(int16_t)_y}
	{
	}
	constexpr point(double _x, double _y)
		: x {(int16_t)_x}
		, y {(int16_t)_y}
	{
	}

	int16_t x {};
	int16_t y {};
} GFXPOINT;

inline GFXPOINT operator+(const GFXPOINT& lhs, const GFXPOINT& rhs)
{
	return {static_cast<int16_t>(lhs.x + rhs.x), static_cast<int16_t>(lhs.y + rhs.y)};
}


// JMS: coordinate point with larger values to avoid overflows in hires modes.
typedef struct dpoint
{
	int32_t x, y;
} DPOINT;

typedef struct stamp
{
	GFXPOINT origin;
	FRAME frame;
} STAMP;

typedef struct rect
{
	GFXPOINT corner;
	EXTENT extent;
} GFXRECT;

// Kruzen: Thanks to JMS, using this to draw ovals
// Overflows happen in HD with Starmap and max zoom (Fuel circles)
typedef struct drect
{
	DPOINT corner;
	DEXTENT extent;
} DRECT;

typedef struct line
{
	GFXPOINT first, second;
} LINE;

static inline GFXPOINT
MAKE_POINT(int16_t x, int16_t y)
{
	GFXPOINT pt = {x, y};
	return pt;
}

static inline DPOINT
MAKE_DPOINT(int32_t x, int32_t y)
{
	DPOINT pt = {x, y};
	return pt;
}

static inline EXTENT
MAKE_EXTENT(int16_t width, int16_t height)
{
	EXTENT ext = {width, height};
	return ext;
}

static inline DEXTENT
MAKE_DEXTENT(int32_t width, int32_t height)
{
	DEXTENT ext = {width, height};
	return ext;
}

// Kruzen: Some DrawOval() calls still use standard rect where overflow is impossible as it's 2 figures away from that
// Used to draw SOI and planet orbits
// To avoid any typedef conflicts - transform standard GFXRECT to DRECT
static inline DRECT
RECT_TO_DRECT(GFXRECT r)
{
	DRECT dr = {
		{r.corner.x,	 r.corner.y	   },
		{r.extent.width, r.extent.height}
	 };

	return dr;
}

//static inline void
//MAKE_LINE (LINE *line, int x1, int y1, int x2, int y2)
//{
//	line->first.x = x1;
//	line->first.y = y1;
//	line->second.x = x2;
//	line->second.y = y2;
//}

static inline void
MAKE_LINE(LINE* line, GFXPOINT first, GFXPOINT second)
{
	line->first = first;
	line->second = second;
}

static inline bool
pointsEqual(GFXPOINT p1, GFXPOINT p2)
{
	return p1.x == p2.x && p1.y == p2.y;
}

static inline bool
extentsEqual(EXTENT e1, EXTENT e2)
{
	return e1.width == e2.width && e1.height == e2.height;
}

static inline bool
rectsEqual(GFXRECT r1, GFXRECT r2)
{
	return pointsEqual(r1.corner, r2.corner)
		&& extentsEqual(r1.extent, r2.extent);
}

static inline bool
pointWithinRect(GFXRECT r, GFXPOINT p)
{
	return p.x >= r.corner.x && p.y >= r.corner.y
		&& p.x < r.corner.x + r.extent.width
		&& p.y < r.corner.y + r.extent.height;
}

static inline double
ptDistance(GFXPOINT p1, GFXPOINT p2)
{
	return (sqrt(pow((double)p2.x - (double)p1.x, 2)
				 + pow((double)p2.y - (double)p1.y, 2)));
}

static inline double
calcDistance(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	const double dx = (double)x2 - (double)x1;
	const double dy = (double)y2 - (double)y1;

	return (sqrt(pow(dx, 2) + pow(dy, 2)));
}

static inline void
printPt(GFXPOINT pt, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, pt.x, pt.y);
}
#define PRINT_PT(pt) \
	printPt(pt, #pt)

static inline void
printDPt(DPOINT dPt, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, dPt.x, dPt.y);
}
#define PRINT_DPT(dpt) \
	printDPt(dpt, #dpt)

static inline void
printExt(EXTENT ext, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, ext.width, ext.height);
}
#define PRINT_EXT(ext) \
	printExt(ext, #ext)

static inline void
printDExt(DEXTENT dExt, char* Str)
{
	fmt::print("{} = {} x {}\n", Str, dExt.width, dExt.height);
}
#define PRINT_DEXT(dext) \
	printDExt(dext, #dext)

static inline void
printRect(GFXRECT r, char* Str)
{
	fmt::print("{}.corner = {} x {}\n", Str, r.corner.x, r.corner.y);
	fmt::print("{}.extent = {} x {}\n", Str, r.extent.width, r.extent.height);
}
#define PRINT_RECT(rect) \
	printRect(rect, #rect)

static inline void
printDRect(DRECT r, char* Str)
{
	fmt::print("{}.corner = {} x {}\n", Str, r.corner.x, r.corner.y);
	fmt::print("{}.extent = {} x {}\n", Str, r.extent.width, r.extent.height);
}
#define PRINT_DRECT(drect) \
	printDRect(drect, #drect)

static inline void
ZeroPoint(GFXPOINT* pt)
{
	pt->x = pt->y = ~0;
}

static inline bool
ValidPoint(GFXPOINT pt)
{
	return (bool)(pt.x != ~0 && pt.y != ~0);
}

typedef enum
{
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} TEXT_ALIGN;

typedef enum
{
	VALIGN_TOP,
	VALIGN_MIDDLE,
	VALIGN_BOTTOM
} TEXT_VALIGN;

typedef struct text
{
	GFXPOINT baseline;
	const char* pStr;
	TEXT_ALIGN align;
	uint16_t CharCount;
} TEXT;

#if 0 //defined(__cplusplus)
}
#endif

#include "libs/strlib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

typedef uqm::StringTable* COLORMAP;
// COLORMAPPTR is really a pointer to colortable entry structure
// which is documented in doc/devel/strtab, .ct files section
const typedef void* COLORMAPPTR;

#include "graphics/prim.h"


typedef uint8_t BATCH_FLAGS;
// This flag is currently unused but it might make sense to restore it
inline constexpr auto BatchBuildPage {(BATCH_FLAGS)(1 << 0)};

typedef struct
{
	TIME_VALUE last_time_val;
	GFXPOINT EndPoint;
	STAMP IntersectStamp;
} INTERSECT_CONTROL;

typedef uint8_t INTERSECT_CODE;

inline constexpr auto IntersectLeft {(INTERSECT_CODE)(1 << 0)};
inline constexpr auto IntersectTop {(INTERSECT_CODE)(1 << 1)};
inline constexpr auto IntersectRight {(INTERSECT_CODE)(1 << 2)};
inline constexpr auto IntersectBottom {(INTERSECT_CODE)(1 << 3)};
inline constexpr auto IntersectNoclip {(INTERSECT_CODE)(1 << 7)};
inline constexpr auto IntersectAllSides {(INTERSECT_CODE)(IntersectLeft | IntersectTop | IntersectRight | IntersectBottom)};

typedef GFXPOINT HOT_SPOT;

extern HOT_SPOT MAKE_HOT_SPOT(int16_t, int16_t);

extern INTERSECT_CODE BoxIntersect(GFXRECT* pr1, GFXRECT* pr2, GFXRECT* printer);
extern void BoxUnion(GFXRECT* pr1, GFXRECT* pr2, GFXRECT* punion);

typedef enum
{
	FadeAllToWhite = 250,
	FadeSomeToWhite,
	FadeAllToBlack,
	FadeAllToColor,
	FadeSomeToBlack,
	FadeSomeToColor
} ScreenFadeType;

typedef enum
{
	DRAW_REPLACE = 0,
	// Pixels in the target FRAME are replaced entirely.
	// Non-stamp primitives with Color.a < 255 to RGB targets are
	// equivalent to DRAW_ALPHA with (DrawMode.factor = Color.a),
	// except the Text primitives.
	// DrawMode.factor: ignored
	// Text: supported (except DRAW_ALPHA via Color.a)
	// RGBA sources (WantAlpha): per-pixel alpha blending performed
	// RGBA targets (WantAlpha): replace directly supported
	DRAW_ADDITIVE,
	// Pixel channels of the source FRAME or Color channels of
	// a primitive are modulated by (DrawMode.factor / 255) and added
	// to the pixel channels of the target FRAME.
	// DrawMode.factor range: -32767..32767 (negative values make
	//    draw subtractive); 255 = 1:1 ratio
	// Text: not yet supported
	// RGBA sources (WantAlpha): alpha channel ignored
	// RGBA targets (WantAlpha): not yet supported
	DRAW_ALPHA,
	// Pixel channels of the source FRAME or Color channels of
	// a primitive are modulated by (DrawMode.factor / 255) and added
	// to the pixel channels of the target FRAME, modulated by
	// (1 - DrawMode.factor / 255)
	// DrawMode.factor range: 0..255; 255 = fully opaque
	// Text: supported
	// RGBA sources (WantAlpha): alpha channel ignored
	// RGBA targets (WantAlpha): not yet supported
	DRAW_MULTIPLY,
	// Pixel channels of the source FRAME or Color channels of
	// a primitive are modulated by (channel / 255) and multiplied
	// by pixel channels of the target FRAME, modulated by
	// (channel / 255)
	// DrawMode.factor range: not yet supported
	// Text: supported
	// RGBA sources (WantAlpha): alpha channel ignored
	// RGBA targets (WantAlpha): not yet supported
	DRAW_OVERLAY,
	DRAW_SCREEN,
	DRAW_GRAYSCALE, // To get true grayscale - factor should be == 128
	DRAW_LINEARBURN,
	DRAW_HYPTOQUAS,
	DRAW_DESATURATE, // Desaturate image. Not optimized for high fps

	DRAW_DEFAULT = DRAW_REPLACE,
} DrawKind;

typedef struct
{
	uint8_t kind;
	int16_t factor;
} DrawMode;

inline constexpr auto DrawFactor1 {0xff};
inline constexpr auto TransferAlpha {0x7fff};

inline constexpr auto DesatAmount {0xBE};

constexpr DrawMode
MAKE_DRAW_MODE(DrawKind kind, int16_t factor)
{
	return {static_cast<uint8_t>(kind), factor};
}

inline constexpr DrawMode DrawReplaceMode {MAKE_DRAW_MODE(DRAW_REPLACE, 0)};

typedef enum
{
	NORTH_SHADOW,
	NORTH_EAST_SHADOW,
	EAST_SHADOW,
	SOUTH_EAST_SHADOW,
	SOUTH_SHADOW,
	SOUTH_WEST_SHADOW,
	WEST_SHADOW,
	NORTH_WEST_SHADOW,
} SHADOW_ANGLE;

extern GFXCONTEXT SetContext(GFXCONTEXT Context);
extern Color SetContextForeGroundColor(Color Color);
extern Color GetContextForeGroundColor(void);
extern Color SetContextBackGroundColor(Color Color);
extern Color GetContextBackGroundColor(void);
extern FRAME SetContextFGFrame(FRAME Frame);
extern FRAME GetContextFGFrame(void);
// Context cliprect defines the drawing bounds. Additionally, all
// drawing positions (x,y) are relative to the cliprect corner.
extern bool SetContextClipRect(GFXRECT* pRect);
// The returned rect is always filled in. If the context cliprect
// is undefined, the returned rect has foreground frame dimensions.
extern bool GetContextClipRect(GFXRECT* pRect);
// The actual origin will be orgOffset + context ClipRect.corner
extern GFXPOINT SetContextOrigin(GFXPOINT orgOffset);
extern DrawMode SetContextDrawMode(DrawMode);
extern DrawMode GetContextDrawMode(void);
// 'area' may be nullptr to copy the entire GFXCONTEXT cliprect
// 'area' is relative to the GFXCONTEXT cliprect
extern DRAWABLE CopyContextRect(const GFXRECT* area);

extern TIME_VALUE DrawablesIntersect(INTERSECT_CONTROL* pControl0,
									 INTERSECT_CONTROL* pControl1, TIME_VALUE max_time_val);
extern void DrawStamp(STAMP* pStamp);
extern void DrawFilledStamp(STAMP* pStamp);
extern void DrawPoint(GFXPOINT* pPoint);
extern void DrawRectangle(GFXRECT* pRect, bool scaled);
extern void DrawFilledRectangle(GFXRECT* pRect);
extern void DrawLine(LINE* pLine, uint8_t thickness);
extern void ApplyMask(FRAME layer, FRAME base, DrawMode mode, Color* fill);
extern void InstaPoint(int x, int y);
extern void InstaRect(int x, int y, int w, int h, bool scaled);
extern void InstaFilledRect(int x, int y, int w, int h);
extern void InstaLine(int x1, int y1, int x2, int y2);
extern GFXRECT font_GetTextRect(TEXT* pText);
extern void font_DrawText(TEXT* pText);
extern void font_DrawText_Fade(TEXT* lpText, FRAME repair, bool* skip);
extern void font_DrawTracedText(TEXT* pText, Color text, Color trace);
extern uint8_t font_DrawTextAlt(TEXT* lpText, uint8_t swap, FONT AltFontPtr, uqm::UniChar key);
extern void font_DrawTracedTextAlt(TEXT* pText, Color text, Color trace, FONT AltFontPtr,
								   uqm::UniChar key);
extern void font_DrawShadowedText(TEXT* pText, uint8_t direction,
								  Color text_color, Color shadow_color);
extern void DrawBatch(PRIMITIVE* pBasePrim, PRIM_LINKS PrimLinks,
					  BATCH_FLAGS BatchFlags);
extern void BatchGraphics(void);
extern void UnbatchGraphics(void);
extern void FlushGraphics(void);
extern void ClearDrawable(void);
extern void ClearScreen(void);
#ifdef DEBUG
extern GFXCONTEXT CreateContextAux(const char* name);
constexpr auto CreateContext(auto name) { return CreateContextAux((name)); }
#else  /* if !defined(DEBUG) */
extern GFXCONTEXT CreateContextAux(void);
constexpr auto CreateContext(auto name) { return CreateContextAux(); }
#endif /* !defined(DEBUG) */
extern bool DestroyContext(GFXCONTEXT ContextRef);
extern DRAWABLE CreateDisplay(CREATE_FLAGS CreateFlags, int16_t* pwidth,
							  int16_t* pheight);
extern DRAWABLE CreateDrawable(CREATE_FLAGS CreateFlags, int16_t width,
							   int16_t height, uint16_t num_frames);
extern bool DestroyDrawable(DRAWABLE Drawable);
extern bool GetFrameRect(FRAME Frame, GFXRECT* pRect);
#ifdef DEBUG
const extern char* GetContextName(GFXCONTEXT context);
extern GFXCONTEXT GetFirstContext(void);
extern GFXCONTEXT GetNextContext(GFXCONTEXT context);
extern size_t GetContextCount(void);
#endif /* DEBUG */

extern HOT_SPOT SetFrameHot(FRAME Frame, HOT_SPOT HotSpot);
extern HOT_SPOT GetFrameHot(FRAME Frame);
extern bool InstallGraphicResTypes(void);
extern DRAWABLE LoadGraphicFile(const char* pStr);
extern FONT LoadFontFile(const char* pStr);
extern void* LoadGraphicInstance(RESOURCE res);
extern DRAWABLE LoadDisplayPixmap(const GFXRECT* area, FRAME frame);
extern FRAME SetContextFontEffect(FRAME EffectFrame);
extern FONT SetContextFont(FONT Font);
extern bool DestroyFont(FONT FontRef);
// The returned pRect is relative to the context drawing origin
extern bool TextRect(TEXT* pText, GFXRECT* pRect, uint8_t* pdelta);
extern bool TextRectAlt(TEXT* lpText, GFXRECT* pRect, uint8_t* pdelta, uint8_t swap, uqm::UniChar key, FONT AltFontPtr);
extern bool GetContextFontLeading(int16_t* pheight);
extern bool GetContextFontDispHeight(int16_t* pheight);
extern bool GetContextFontDispWidth(int16_t* pwidth);
extern uint16_t GetFrameCount(FRAME Frame);
extern uint16_t GetFrameIndex(FRAME Frame);
extern FRAME SetAbsFrameIndex(FRAME Frame, uint16_t FrameIndex);
extern FRAME SetRelFrameIndex(FRAME Frame, int16_t FrameOffs);
extern FRAME SetEquFrameIndex(FRAME DstFrame, FRAME SrcFrame);
extern FRAME IncFrameIndex(FRAME Frame);
extern FRAME DecFrameIndex(FRAME Frame);
extern DRAWABLE CopyFrameRect(FRAME Frame, const GFXRECT* area);
extern DRAWABLE CloneFrame(FRAME Frame);
extern DRAWABLE RotateFrame(FRAME Frame, int angle_deg);
extern DRAWABLE RescaleFrame(FRAME frame, int width, int height);
extern DRAWABLE RescalePercentage(FRAME frame, float percentage);
// This pair works for both paletted and trucolor frames
extern bool ReadFramePixelColors(FRAME frame, Color* pixels,
								 int width, int height);
extern bool WriteFramePixelColors(FRAME frame, const Color* pixels,
								  int width, int height);
// This pair only works for paletted frames
extern bool ReadFramePixelIndexes(FRAME frame, uint8_t* pixels,
								  int width, int height, bool paletted);
extern bool WriteFramePixelIndexes(FRAME frame, const uint8_t* pixels,
								   int width, int height);
extern void SetFrameTransparentColor(FRAME, Color);
extern bool IsFrameIndexed(FRAME Frame);

// If the frame is an active SCREEN_DRAWABLE, this call must be
// preceeded by FlushGraphics() for draw commands to have taken effect
extern Color GetFramePixel(FRAME, GFXPOINT pixelPt);

extern FRAME CaptureDrawable(DRAWABLE Drawable);
extern DRAWABLE ReleaseDrawable(FRAME Frame);

extern DRAWABLE GetFrameParentDrawable(FRAME Frame);

extern bool SetColorMap(COLORMAPPTR ColorMapPtr);
extern TimeCount XFormColorMap(COLORMAPPTR ColorMapPtr, TimePeriod TimeInterval);
extern TimeCount FadeScreen(ScreenFadeType fadeType, TimePeriod TimeInterval);
extern void FlushColorXForms(void);
extern uint8_t GetColorMapTableIndex(COLORMAP map, size_t index);

// The color map 'index' of 'map'. The index wraps at the number of color maps.
extern COLORMAPPTR GetColorMapAddress(COLORMAP map, size_t index);

void SetSystemRect(const GFXRECT* pRect);
void ClearSystemRect(void);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* LIBS_GFXLIB_H_ */
