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

#ifndef LIBS_GRAPHICS_DRAWABLE_H_
#define LIBS_GRAPHICS_DRAWABLE_H_

#include <stdio.h>
#include "tfb_draw.h"
#include "core/stl/vector.h"

constexpr auto ValidPrimType(auto pt) { return ((pt) < PrimitiveType::Count); }

struct BresenhamLine
{
	GfxPoint first, second;
	int16_t abs_delta_x, abs_delta_y;
	int16_t error_term;
	bool end_points_exchanged;
	INTERSECT_CODE intersect_code;
};

typedef uint16_t DRAWABLE_TYPE;
inline constexpr auto RomDrawable {0};
inline constexpr auto RamDrawable {1};
inline constexpr auto ScreenDrawable {2};

struct FrameDesc
{
	DRAWABLE_TYPE Type;
	uint16_t Index;
	HOT_SPOT HotSpot;
	GfxExtent Bounds;
	TFB_Image* image;
	struct DrawableDesc* parent;
};

struct DrawableDesc
{
	CREATE_FLAGS Flags;
	uint16_t MaxIndex;
	uqstl::vector<FrameDesc> Frame;
};

constexpr auto GetFrameWidth(auto f) { return ((f)->Bounds.width); }
constexpr auto GetFrameHeight(auto f) { return ((f)->Bounds.height); }
constexpr auto GetFrameBounds(auto f) { return ((f)->Bounds); }
constexpr auto SetFrameBounds(auto f, auto w, auto h) { return ((f)->Bounds.width = (w), ((f))->Bounds.height = (h)); }


extern DRAWABLE AllocDrawable(uint16_t num_frames);
constexpr auto FreeDrawable(auto D) { return _ReleaseCelData(D); }

struct ImageBox
{
	GfxRect Box;
	FRAME FramePtr;
};

extern INTERSECT_CODE _clip_line(const LargeRect* pClipRect,
								 BresenhamLine* pLine);

extern void* _GetCelData(const ResourceFile& resource);
extern bool _ReleaseCelData(void* handle);

extern FRAME _CurFramePtr;

// ClipRect is relative to ctxOrigin
extern void _text_blt(GfxRect* pClipRect, GfxText* TextPtr, GfxPoint ctxOrigin);
extern void _text_blt_fade(GfxRect* pClipRect, GfxText* TextPtr, GfxPoint ctxOrigin, FRAME repair, bool* skip);
extern uint8_t _text_blt_alt(GfxRect* pClipRect, GfxText* TextPtr, GfxPoint ctxOrigin, uint8_t swap, FONT AltFontPtr, uqm::UniChar key);

#endif /* LIBS_GRAPHICS_DRAWABLE_H_ */
