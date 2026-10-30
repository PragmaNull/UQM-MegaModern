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

#define ValidPrimType(pt) ((pt) < NUM_PRIMS)

typedef struct bresenham_line
{
	GFXPOINT first, second;
	int16_t abs_delta_x, abs_delta_y;
	int16_t error_term;
	bool end_points_exchanged;
	INTERSECT_CODE intersect_code;
} BRESENHAM_LINE;

typedef uint16_t DRAWABLE_TYPE;
#define ROM_DRAWABLE 0
#define RAM_DRAWABLE 1
#define SCREEN_DRAWABLE 2

struct frame_desc
{
	DRAWABLE_TYPE Type;
	uint16_t Index;
	HOT_SPOT HotSpot;
	EXTENT Bounds;
	TFB_Image* image;
	struct drawable_desc* parent;
};

struct drawable_desc
{
	CREATE_FLAGS Flags;
	uint16_t MaxIndex;
	uqstl::vector<FRAME_DESC> Frame;
};

#define GetFrameWidth(f) ((f)->Bounds.width)
#define GetFrameHeight(f) ((f)->Bounds.height)
#define GetFrameBounds(f) ((f)->Bounds)
#define SetFrameBounds(f, w, h) \
	((f)->Bounds.width = (w),   \
	 ((f))->Bounds.height = (h))

#define DRAWABLE_PRIORITY DEFAULT_MEM_PRIORITY

extern DRAWABLE AllocDrawable(uint16_t num_frames);
#define FreeDrawable(D) _ReleaseCelData(D)

typedef struct
{
	GFXRECT Box;
	FRAME FramePtr;
} IMAGE_BOX;

extern INTERSECT_CODE _clip_line(const DRECT* pClipRect,
								 BRESENHAM_LINE* pLine);

extern void* _GetCelData(const ResourceFile& resource);
extern bool _ReleaseCelData(void* handle);

extern FRAME _CurFramePtr;

// ClipRect is relative to ctxOrigin
extern void _text_blt(GFXRECT* pClipRect, TEXT* TextPtr, GFXPOINT ctxOrigin);
extern void _text_blt_fade(GFXRECT* pClipRect, TEXT* TextPtr, GFXPOINT ctxOrigin, FRAME repair, bool* skip);
extern uint8_t _text_blt_alt(GFXRECT* pClipRect, TEXT* TextPtr, GFXPOINT ctxOrigin, uint8_t swap, FONT AltFontPtr, uqm::UniChar key);

#endif /* LIBS_GRAPHICS_DRAWABLE_H_ */
