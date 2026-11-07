// Copyright Michael Martin, 2003

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

#pragma once
#ifndef UQM_LIBS_GRAPHICS_TFB_PRIM_H
#define UQM_LIBS_GRAPHICS_TFB_PRIM_H

#include "libs/gfxlib.h"
#include "tfb_draw.h"


void TFB_Prim_Line(GfxLine*, Color, DrawMode, GfxPoint ctxOrigin, uint8_t thickness);
void TFB_Prim_Point(GfxPoint*, Color, DrawMode, GfxPoint ctxOrigin, bool scaled);
void TFB_Prim_Rect(GfxRect*, Color, DrawMode, GfxPoint ctxOrigin, bool scaled);
void TFB_Prim_FillRect(GfxRect*, Color, DrawMode, GfxPoint ctxOrigin);
void TFB_Prim_Stamp(GfxStamp*, DrawMode, GfxPoint ctxOrigin, bool unscaled);
void TFB_Prim_StampFill(GfxStamp*, Color, DrawMode, GfxPoint ctxOrigin, bool unscaled);
void TFB_Prim_FontChar(GfxPoint charOrigin, TFB_Char* fontChar,
					   TFB_Image* backing, DrawMode, GfxPoint ctxOrigin);
void TFB_Prim_MaskFrame(FRAME layer, FRAME base, DrawMode mode, Color* fill);

#endif // UQM_LIBS_GRAPHICS_TFB_PRIM_H
