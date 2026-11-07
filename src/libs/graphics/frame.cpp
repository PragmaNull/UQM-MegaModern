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

#include "gfxintrn.h"
#include "gfx_common.h"
#include "tfb_draw.h"
#include "tfb_prim.h"
#include "uqm/units.h"

HOT_SPOT MAKE_HOT_SPOT(int16_t x, int16_t y)
{
	HOT_SPOT hs;
	hs.x = x;
	hs.y = y;
	return hs;
}

// XXX: InternalPrimitive and INTERNAL_PRIM_DESC are not used
typedef union
{
	GfxPoint Point;
	GfxStamp Stamp;
	BresenhamLine Line;
	GfxText Text;
	GfxRect Rect;
} INTERNAL_PRIM_DESC;

struct InternalPrimitive
{
	PRIM_LINKS Links;
	GRAPHICS_PRIM Type;
	Color Color;
	INTERNAL_PRIM_DESC Object;
};


// pValidRect or origin may be nullptr
bool GetContextValidRect(GfxRect* pValidRect, GfxPoint* origin)
{
	GfxRect tempRect;
	GfxPoint tempPt;

	if (!pValidRect)
	{
		pValidRect = &tempRect;
	}
	if (!origin)
	{
		origin = &tempPt;
	}

	// Start with a rect the size of foreground frame
	pValidRect->corner.x = 0;
	pValidRect->corner.y = 0;
	pValidRect->extent = GetFrameBounds(_CurFramePtr);
	*origin = _CurFramePtr->HotSpot;

	if (_pCurContext->ClipRect.extent.width)
	{
		// If the cliprect is completely outside of the valid frame
		// bounds we have nothing to draw
		if (!BoxIntersect(&_pCurContext->ClipRect,
						  pValidRect, pValidRect))
		{
			return (false);
		}

		// Foreground frame hotspot defines a drawing position offset
		// WRT the context cliprect
		origin->x += _pCurContext->ClipRect.corner.x;
		origin->y += _pCurContext->ClipRect.corner.y;
	}

	return (true);
}

static void ClearBackGround(GfxRect* pClipRect)
{
	GfxRect clearRect;
	const Color color = _get_context_bg_color();
	clearRect.corner.x = 0;
	clearRect.corner.y = 0;
	clearRect.extent = pClipRect->extent;
	TFB_Prim_FillRect(&clearRect, color, DrawReplaceMode,
					  pClipRect->corner);
}

void DrawBatch(Primitive* lpBasePrim, PRIM_LINKS PrimLinks,
			   BATCH_FLAGS BatchFlags)
{
	GfxRect ValidRect;
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(&ValidRect, &origin))
	{
		Primitive* lpPrim {};
		const DrawMode mode = _get_context_draw_mode();

		BatchGraphics();

		if (BatchFlags & BatchBuildPage)
		{
			ClearBackGround(&ValidRect);
		}

		uint16_t CurIndex {GetPredLink(PrimLinks)};

		for (; CurIndex != EndOfList;
			 CurIndex = GetSuccLink(GetPrimLinks(lpPrim)))
		{
			GfxRect ClipRect;
			Color color;

			lpPrim = &lpBasePrim[CurIndex];
			const GRAPHICS_PRIM PrimType {GetPrimType(lpPrim)};
			if (!ValidPrimType(PrimType))
			{
				continue;
			}
			const uint8_t flags {GetPrimFlags(lpPrim)};

			Primitive* lpWorkPrim {lpPrim};

			switch (PrimType)
			{
				case PrimitiveType::Point:
					color = GetPrimColor(lpWorkPrim);
					TFB_Prim_Point(&lpWorkPrim->Object.Point, color,
								   mode, origin, false);
					break;
				case PrimitiveType::Stamp:
					if (flags & HyperToQuasiColor)
					{
						TFB_Prim_Stamp(&lpWorkPrim->Object.Stamp,
									   MAKE_DRAW_MODE(DrawKind::Hyptoquas, TransferAlpha),
									   origin, (bool)(flags & UnscaledStamp));
					}
					else
					{
						TFB_Prim_Stamp(&lpWorkPrim->Object.Stamp, mode,
									   origin, (bool)(flags & UnscaledStamp));
					}
					break;
				case PrimitiveType::StampFill:
					color = GetPrimColor(lpWorkPrim);
					if (flags & HsStarmask)
					{
						TFB_Prim_StampFill(&lpWorkPrim->Object.Stamp, color,
										   MAKE_DRAW_MODE(DrawKind::Overlay, TransferAlpha),
										   origin, (bool)(flags & UnscaledStamp));
					}
					else
					{
						TFB_Prim_StampFill(&lpWorkPrim->Object.Stamp, color,
										   mode, origin, (bool)(flags & UnscaledStamp));
					}
					break;
				case PrimitiveType::Line:
					color = GetPrimColor(lpWorkPrim);
					TFB_Prim_Line(&lpWorkPrim->Object.Line, color,
								  mode, origin, uqm::chooseIfHd(1, 3));
					break;
				case PrimitiveType::Text:
					if (!TextRect(&lpWorkPrim->Object.Text, &ClipRect, nullptr))
					{
						continue;
					}
					// ClipRect is relative to origin
					_text_blt(&ClipRect, &lpWorkPrim->Object.Text, origin);
					break;
				case PrimitiveType::Rect:
					color = GetPrimColor(lpWorkPrim);
					TFB_Prim_Rect(&lpWorkPrim->Object.Rect, color,
								  mode, origin, false);
					break;
				case PrimitiveType::RectFill:
					color = GetPrimColor(lpWorkPrim);
					TFB_Prim_FillRect(&lpWorkPrim->Object.Rect, color,
									  mode, origin);
					break;
				case PrimitiveType::PointHd:
					color = GetPrimColor(lpWorkPrim);
					TFB_Prim_Point(&lpWorkPrim->Object.Point, color,
								   mode, origin, true);
					break;
				default:
					break;
			}
		}

		UnbatchGraphics();
	}
}

void ClearDrawable(void)
{
	GfxRect ValidRect;

	if (GraphicsSystemActive() && GetContextValidRect(&ValidRect, nullptr))
	{
		ClearBackGround(&ValidRect);
	}
}

void ClearScreen(void)
{
	GfxRect ValidRect;

	if (!GraphicsSystemActive())
	{
		return;
	}

	ValidRect.corner = MAKE_POINT(0, 0);
	ValidRect.extent.width = CanvasWidth;
	ValidRect.extent.height = CanvasHeight;

	ClearBackGround(&ValidRect);
}

void DrawPoint(GfxPoint* lpPoint)
{
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(nullptr, &origin))
	{
		const Color color = GetPrimColor(&_locPrim);
		const DrawMode mode = _get_context_draw_mode();
		TFB_Prim_Point(lpPoint, color, mode, origin, false);
	}
}

void InstaPoint(int x, int y)
{
	GfxPoint origin = {(int16_t)x, (int16_t)y};
	DrawPoint(&origin);
}

void DrawRectangle(GfxRect* lpRect, bool scaled)
{
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(nullptr, &origin))
	{
		const Color color = GetPrimColor(&_locPrim);
		const DrawMode mode = _get_context_draw_mode();
		TFB_Prim_Rect(lpRect, color, mode, origin, scaled);
	}
}

void InstaRect(int x, int y, int w, int h, bool scaled)
{
	GfxRect r = {
		{(int16_t)x, (int16_t)y},
		{(int16_t)w, (int16_t)h}
	};
	DrawRectangle(&r, scaled);
}

void DrawFilledRectangle(GfxRect* lpRect)
{
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(nullptr, &origin))
	{
		const Color color = GetPrimColor(&_locPrim);
		const DrawMode mode = _get_context_draw_mode();
		TFB_Prim_FillRect(lpRect, color, mode, origin);
	}
}

void InstaFilledRect(int x, int y, int w, int h)
{
	GfxRect r = {
		{(int16_t)x, (int16_t)y},
		{(int16_t)w, (int16_t)h}
	};
	DrawFilledRectangle(&r);
}

void DrawLine(GfxLine* lpLine, uint8_t thickness)
{
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(nullptr, &origin))
	{
		const Color color = GetPrimColor(&_locPrim);
		const DrawMode mode = _get_context_draw_mode();
		TFB_Prim_Line(lpLine, color, mode, origin, thickness);
	}
}

void InstaLine(int x1, int y1, int x2, int y2)
{
	GfxLine line = {
		{(int16_t)x1, (int16_t)y1},
		{(int16_t)x2, (int16_t)y2}
	  };
	DrawLine(&line, 1);
}

void DrawStamp(GfxStamp* stmp)
{
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(nullptr, &origin))
	{
		const DrawMode mode = _get_context_draw_mode();
		TFB_Prim_Stamp(stmp, mode, origin, false);
	}
}

void DrawFilledStamp(GfxStamp* stmp)
{
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(nullptr, &origin))
	{
		const Color color = GetPrimColor(&_locPrim);
		const DrawMode mode = _get_context_draw_mode();
		TFB_Prim_StampFill(stmp, color, mode, origin, false);
	}
}

// Kruzen: Permanently applies layer frame to base via masking
// until base frame is unloaded from memory.
// Layer frame should be the same size or larger that base frame.
// Works with paletted but only with REPLACE mode (doesn't alter the palette).
// If layer frame is nullptr and color pointer is not nullptr - blend will be
// applied to every pixel equally with alpha = 255
void ApplyMask(FRAME layer, FRAME base, DrawMode mode, Color* fill)
{
	GfxPoint origin;

	if (GraphicsSystemActive() && GetContextValidRect(nullptr, &origin))
	{
		TFB_Prim_MaskFrame(layer, base, mode, fill);
	}
}
