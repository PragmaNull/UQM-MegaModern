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

#pragma once
#ifndef UQM_LIBS_GRAPHICS_TFB_DRAW_H
#define UQM_LIBS_GRAPHICS_TFB_DRAW_H

#include "core/types/enum.h"
#include "libs/time/timelib.h"
#include "core/thread/Mutex.h"
#include "core/stl/array.h"
#include "core/stl/optional.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/cmap.h"


namespace tfb
{
using Canvas = void*;
} // namespace tfb

enum class ScreenBuffer
{
	Main,
	Extra,
	Transition,

	Count
};
consteval bool isArithmeticEnum(ScreenBuffer) { return true; }


namespace tfb
{
struct Image
{
	Canvas NormalImg;
	Canvas ScaledImg;
	Canvas MipmapImg;
	Canvas FilledImg;
	int colormap_index;
	int colormap_version;
	HotSpot NormalHs;
	HotSpot MipmapHs;
	HotSpot last_scale_hs;
	int last_scale;
	ScaleMode last_scale_type;
	Color last_fill;
	GfxExtent extent;
	uqm::RecursiveMutex mutex;
	bool dirty;
};

struct Char
{
	GfxExtent extent;
	GfxExtent disp;
	// Display extent
	HotSpot hotSpot;
	uint8_t* data;
	uint32_t pitch;
	// Pitch is for storing all chars of a page
	// in one rectangular pixel matrix
};
} // namespace tfb

// we do not support paletted format for now
namespace tfb
{
struct PixelFormat
{
	int BitsPerPixel;
	int BytesPerPixel;
	uint32_t Rmask, Gmask, Bmask, Amask;
	uint32_t Rshift, Gshift, Bshift, Ashift;
	uint32_t Rloss, Gloss, Bloss, Aloss;
};
} // namespace tfb

// Drawing commands

void TFB_DrawScreen_Line(int x1, int y1, int x2, int y2, Color color,
						 DrawMode, ScreenBuffer dest, uint8_t thickness);
void TFB_DrawScreen_Rect(GfxRect* rect, Color, DrawMode, ScreenBuffer dest);
void TFB_DrawScreen_Image(tfb::Image* img, int x, int y, int scale,
						  tfb::ScaleMode scaleMode, tfb::ColorMap*, DrawMode, ScreenBuffer dest);
void TFB_DrawScreen_Copy(const GfxRect* r, ScreenBuffer src, ScreenBuffer dest);
void TFB_DrawScreen_FilledImage(tfb::Image* img, int x, int y, int scale,
								tfb::ScaleMode scaleMode, Color, DrawMode, ScreenBuffer dest);
void TFB_DrawScreen_FontChar(tfb::Char*, tfb::Image* backing, int x, int y,
							 DrawMode, ScreenBuffer dest);

void TFB_DrawScreen_CopyToImage(tfb::Image* img, const GfxRect* r, ScreenBuffer src);
void TFB_DrawScreen_SetMipmap(tfb::Image* img, tfb::Image* mmimg, int hotx,
							  int hoty);
void TFB_DrawScreen_DeleteImage(tfb::Image* img);
void TFB_DrawScreen_DeleteData(uint8_t* data);
void TFB_DrawScreen_WaitForSignal(void);
void TFB_DrawScreen_ReinitVideo(uqm::GfxDriver driver, uqm::GfxFlags flags, int width, int height);
void TFB_DrawScreen_Callback(void (*callback)(void* arg), void* arg);

tfb::Image* TFB_DrawImage_New(tfb::Canvas canvas);
tfb::Image* TFB_DrawImage_CreateForScreen(int w, int h, bool withalpha);
tfb::Image* TFB_DrawImage_New_Rotated(tfb::Image* img, int angle);
void TFB_DrawImage_SetMipmap(tfb::Image* img, tfb::Image* mmimg, int hotx,
							 int hoty);
void TFB_DrawImage_Delete(tfb::Image* image);
void TFB_DrawImage_FixScaling(tfb::Image* image, int scaleTarget, tfb::ScaleMode type);
bool TFB_DrawImage_Intersect(tfb::Image* img1, GfxPoint img1org,
							 tfb::Image* img2, GfxPoint img2org, const GfxRect* interRect);
void TFB_DrawImage_CopyRect(tfb::Image* source, const GfxRect* srcRect,
							tfb::Image* target, GfxPoint dstPt);

void TFB_DrawImage_Line(int x1, int y1, int x2, int y2, Color color,
						DrawMode, tfb::Image* target, uint8_t thickness);
void TFB_DrawImage_Rect(GfxRect* rect, Color, DrawMode, tfb::Image* target);
void TFB_DrawImage_Image(tfb::Image* img, int x, int y, int scale,
						 tfb::ScaleMode scaleMode, tfb::ColorMap*, DrawMode, tfb::Image* target);
void TFB_DrawImage_FilledImage(tfb::Image* img, int x, int y, int scale,
							   tfb::ScaleMode scaleMode, Color, DrawMode, tfb::Image* target);
void TFB_DrawImage_FontChar(tfb::Char*, tfb::Image* backing, int x, int y,
							DrawMode, tfb::Image* target);
void TFB_DrawImage_MaskImage(tfb::Image* img, DrawMode mode, tfb::Image* target, Color* fill);

tfb::Canvas TFB_DrawCanvas_LoadFromFile(const uqm::io::Directory& dir, const char* fileName);
tfb::Canvas TFB_DrawCanvas_New_TrueColor(int w, int h, bool hasalpha);
tfb::Canvas TFB_DrawCanvas_New_ForScreen(int w, int h, bool withalpha);
tfb::Canvas TFB_DrawCanvas_New_Paletted(int w, int h, Color palette[256],
										int transparent_index);
tfb::Canvas TFB_DrawCanvas_New_ScaleTarget(tfb::Canvas canvas,
										   tfb::Canvas oldcanvas, tfb::ScaleMode type, tfb::ScaleMode last_type);
tfb::Canvas TFB_DrawCanvas_New_RotationTarget(tfb::Canvas src, int angle);
tfb::Canvas TFB_DrawCanvas_ToScreenFormat(tfb::Canvas canvas);
bool TFB_DrawCanvas_IsPaletted(tfb::Canvas canvas);
void TFB_DrawCanvas_Rescale_Nearest(tfb::Canvas src, tfb::Canvas dst,
									int scale, HotSpot* src_hs, GfxExtent* size, HotSpot* dst_hs);
void TFB_DrawCanvas_Rescale_Bilinear(tfb::Canvas src, tfb::Canvas dst,
									 int scale, HotSpot* src_hs, GfxExtent* size, HotSpot* dst_hs);
void TFB_DrawCanvas_Rescale_Trilinear(tfb::Canvas src, tfb::Canvas mipmap,
									  tfb::Canvas dst, int scale, HotSpot* src_hs, HotSpot* mm_hs,
									  GfxExtent* size, HotSpot* dst_hs);
void TFB_DrawCanvas_GetScaledExtent(tfb::Canvas src_canvas, HotSpot* src_hs,
									tfb::Canvas src_mipmap, HotSpot* mm_hs,
									int scale, tfb::ScaleMode type, GfxExtent* size, HotSpot* hs);
void TFB_DrawCanvas_Rotate(tfb::Canvas src, tfb::Canvas dst, int angle,
						   GfxExtent size);
void TFB_DrawCanvas_GetRotatedExtent(tfb::Canvas src, int angle, GfxExtent* size);
void TFB_DrawCanvas_GetExtent(tfb::Canvas canvas, GfxExtent* size);
void TFB_DrawCanvas_SetClipRect(tfb::Canvas canvas, const GfxRect* clipRect);

void TFB_DrawCanvas_Delete(tfb::Canvas canvas);

void TFB_DrawCanvas_Line(int x1, int y1, int x2, int y2, Color color,
						 DrawMode, tfb::Canvas target, uint8_t thickness);
void TFB_DrawCanvas_Rect(GfxRect* rect, Color, DrawMode, tfb::Canvas target);
void TFB_DrawCanvas_Image(tfb::Image* img, int x, int y, int scale,
						  tfb::ScaleMode scaleMode, tfb::ColorMap*, DrawMode, tfb::Canvas target);
void TFB_DrawCanvas_FilledImage(tfb::Image* img, int x, int y, int scale,
								tfb::ScaleMode scaleMode, Color, DrawMode, tfb::Canvas target);
void TFB_DrawCanvas_FontChar(tfb::Char*, tfb::Image* backing, int x, int y,
							 DrawMode, tfb::Canvas target);
void TFB_DrawCanvas_CopyRect(tfb::Canvas source, const GfxRect* srcRect,
							 tfb::Canvas target, GfxPoint dstPt);
void TFB_DrawCanvas_MaskImage(tfb::Image* img, DrawMode mode, tfb::Canvas target, Color* fill);

bool TFB_DrawCanvas_GetFontCharData(tfb::Canvas canvas, uint8_t* outData,
									unsigned dataPitch);
uqstl::optional<uqstl::array<Color, 256>> TFB_DrawCanvas_ExtractPalette(tfb::Canvas canvas);
void TFB_DrawCanvas_SetPalette(tfb::Canvas target, Color palette[256]);
int TFB_DrawCanvas_GetTransparentIndex(tfb::Canvas canvas);
void TFB_DrawCanvas_SetTransparentIndex(tfb::Canvas canvas, int i,
										bool rleaccel);
bool TFB_DrawCanvas_GetTransparentColor(tfb::Canvas canvas,
										Color* color);
void TFB_DrawCanvas_SetTransparentColor(tfb::Canvas canvas,
										Color color, bool rleaccel);
void TFB_DrawCanvas_CopyTransparencyInfo(tfb::Canvas src, tfb::Canvas dst);
void TFB_DrawCanvas_Initialize(void);
void TFB_DrawCanvas_Lock(tfb::Canvas canvas);
void TFB_DrawCanvas_Unlock(tfb::Canvas canvas);
void TFB_DrawCanvas_GetScreenFormat(tfb::PixelFormat* fmt);
int TFB_DrawCanvas_GetStride(tfb::Canvas canvas);
void* TFB_DrawCanvas_GetLine(tfb::Canvas canvas, int line);
Color TFB_DrawCanvas_GetPixel(tfb::Canvas canvas, int x, int y);
bool TFB_DrawCanvas_Intersect(tfb::Canvas canvas1, GfxPoint c1org,
							  tfb::Canvas canvas2, GfxPoint c2org, const GfxRect* interRect);

bool TFB_DrawCanvas_GetPixelColors(tfb::Canvas, Color* pixels,
								   int width, int height);
bool TFB_DrawCanvas_SetPixelColors(tfb::Canvas, const Color* pixels,
								   int width, int height);
bool TFB_DrawCanvas_GetPixelIndexes(tfb::Canvas, uint8_t* data,
									int width, int height);
bool TFB_DrawCanvas_SetPixelIndexes(tfb::Canvas, const uint8_t* data,
									int width, int height);

const char* TFB_DrawCanvas_GetError(void);

tfb::Canvas TFB_GetScreenCanvas(ScreenBuffer screen);
tfb::Canvas TFB_GetFPSCanvas(void);

#endif // UQM_LIBS_GRAPHICS_TFB_DRAW_H
