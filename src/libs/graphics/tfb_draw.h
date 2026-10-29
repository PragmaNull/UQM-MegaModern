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

#ifndef TFB_DRAW_H
#define TFB_DRAW_H

#include "libs/threadlib.h"


typedef void* TFB_Canvas;

typedef enum
{
	TFB_SCREEN_MAIN,
	TFB_SCREEN_EXTRA,
	TFB_SCREEN_TRANSITION,

	TFB_GFX_NUMSCREENS
} SCREEN;

#include "libs/graphics/gfx_common.h"
#include "libs/graphics/cmap.h"

typedef struct tfb_image
{
	TFB_Canvas NormalImg;
	TFB_Canvas ScaledImg;
	TFB_Canvas MipmapImg;
	TFB_Canvas FilledImg;
	int colormap_index;
	int colormap_version;
	HOT_SPOT NormalHs;
	HOT_SPOT MipmapHs;
	HOT_SPOT last_scale_hs;
	int last_scale;
	uqm::TFBScaleMode last_scale_type;
	Color last_fill;
	EXTENT extent;
	Mutex mutex;
	bool dirty;
} TFB_Image;

typedef struct tfb_char
{
	EXTENT extent;
	EXTENT disp;
	// Display extent
	HOT_SPOT HotSpot;
	uint8_t* data;
	uint32_t pitch;
	// Pitch is for storing all chars of a page
	// in one rectangular pixel matrix
} TFB_Char;

// we do not support paletted format for now
typedef struct tfb_pixelformat
{
	int BitsPerPixel;
	int BytesPerPixel;
	uint32_t Rmask, Gmask, Bmask, Amask;
	uint32_t Rshift, Gshift, Bshift, Ashift;
	uint32_t Rloss, Gloss, Bloss, Aloss;
} TFB_PixelFormat;

// Drawing commands

void TFB_DrawScreen_Line(int x1, int y1, int x2, int y2, Color color,
						 DrawMode, SCREEN dest, uint8_t thickness);
void TFB_DrawScreen_Rect(GFXRECT* rect, Color, DrawMode, SCREEN dest);
void TFB_DrawScreen_Image(TFB_Image* img, int x, int y, int scale,
						  uqm::TFBScaleMode scaleMode, TFB_ColorMap*, DrawMode, SCREEN dest);
void TFB_DrawScreen_Copy(const GFXRECT* r, SCREEN src, SCREEN dest);
void TFB_DrawScreen_FilledImage(TFB_Image* img, int x, int y, int scale,
								uqm::TFBScaleMode scaleMode, Color, DrawMode, SCREEN dest);
void TFB_DrawScreen_FontChar(TFB_Char*, TFB_Image* backing, int x, int y,
							 DrawMode, SCREEN dest);

void TFB_DrawScreen_CopyToImage(TFB_Image* img, const GFXRECT* r, SCREEN src);
void TFB_DrawScreen_SetMipmap(TFB_Image* img, TFB_Image* mmimg, int hotx,
							  int hoty);
void TFB_DrawScreen_DeleteImage(TFB_Image* img);
void TFB_DrawScreen_DeleteData(void*);
void TFB_DrawScreen_WaitForSignal(void);
void TFB_DrawScreen_ReinitVideo(uqm::GfxDriver driver, uqm::GfxFlags flags, int width, int height);
void TFB_DrawScreen_Callback(void (*callback)(void* arg), void* arg);

TFB_Image* TFB_DrawImage_New(TFB_Canvas canvas);
TFB_Image* TFB_DrawImage_CreateForScreen(int w, int h, bool withalpha);
TFB_Image* TFB_DrawImage_New_Rotated(TFB_Image* img, int angle);
void TFB_DrawImage_SetMipmap(TFB_Image* img, TFB_Image* mmimg, int hotx,
							 int hoty);
void TFB_DrawImage_Delete(TFB_Image* image);
void TFB_DrawImage_FixScaling(TFB_Image* image, int scaleTarget, uqm::TFBScaleMode type);
bool TFB_DrawImage_Intersect(TFB_Image* img1, GFXPOINT img1org,
							 TFB_Image* img2, GFXPOINT img2org, const GFXRECT* interRect);
void TFB_DrawImage_CopyRect(TFB_Image* source, const GFXRECT* srcRect,
							TFB_Image* target, GFXPOINT dstPt);

void TFB_DrawImage_Line(int x1, int y1, int x2, int y2, Color color,
						DrawMode, TFB_Image* target, uint8_t thickness);
void TFB_DrawImage_Rect(GFXRECT* rect, Color, DrawMode, TFB_Image* target);
void TFB_DrawImage_Image(TFB_Image* img, int x, int y, int scale,
						 uqm::TFBScaleMode scaleMode, TFB_ColorMap*, DrawMode, TFB_Image* target);
void TFB_DrawImage_FilledImage(TFB_Image* img, int x, int y, int scale,
							   uqm::TFBScaleMode scaleMode, Color, DrawMode, TFB_Image* target);
void TFB_DrawImage_FontChar(TFB_Char*, TFB_Image* backing, int x, int y,
							DrawMode, TFB_Image* target);
void TFB_DrawImage_MaskImage(TFB_Image* img, DrawMode mode, TFB_Image* target, Color* fill);

TFB_Canvas TFB_DrawCanvas_LoadFromFile(const uqm::io::Directory& dir, const char* fileName);
TFB_Canvas TFB_DrawCanvas_New_TrueColor(int w, int h, bool hasalpha);
TFB_Canvas TFB_DrawCanvas_New_ForScreen(int w, int h, bool withalpha);
TFB_Canvas TFB_DrawCanvas_New_Paletted(int w, int h, Color palette[256],
									   int transparent_index);
TFB_Canvas TFB_DrawCanvas_New_ScaleTarget(TFB_Canvas canvas,
										  TFB_Canvas oldcanvas, uqm::TFBScaleMode type, uqm::TFBScaleMode last_type);
TFB_Canvas TFB_DrawCanvas_New_RotationTarget(TFB_Canvas src, int angle);
TFB_Canvas TFB_DrawCanvas_ToScreenFormat(TFB_Canvas canvas);
bool TFB_DrawCanvas_IsPaletted(TFB_Canvas canvas);
void TFB_DrawCanvas_Rescale_Nearest(TFB_Canvas src, TFB_Canvas dst,
									int scale, HOT_SPOT* src_hs, EXTENT* size, HOT_SPOT* dst_hs);
void TFB_DrawCanvas_Rescale_Bilinear(TFB_Canvas src, TFB_Canvas dst,
									 int scale, HOT_SPOT* src_hs, EXTENT* size, HOT_SPOT* dst_hs);
void TFB_DrawCanvas_Rescale_Trilinear(TFB_Canvas src, TFB_Canvas mipmap,
									  TFB_Canvas dst, int scale, HOT_SPOT* src_hs, HOT_SPOT* mm_hs,
									  EXTENT* size, HOT_SPOT* dst_hs);
void TFB_DrawCanvas_GetScaledExtent(TFB_Canvas src_canvas, HOT_SPOT* src_hs,
									TFB_Canvas src_mipmap, HOT_SPOT* mm_hs,
									int scale, int type, EXTENT* size, HOT_SPOT* hs);
void TFB_DrawCanvas_Rotate(TFB_Canvas src, TFB_Canvas dst, int angle,
						   EXTENT size);
void TFB_DrawCanvas_GetRotatedExtent(TFB_Canvas src, int angle, EXTENT* size);
void TFB_DrawCanvas_GetExtent(TFB_Canvas canvas, EXTENT* size);
void TFB_DrawCanvas_SetClipRect(TFB_Canvas canvas, const GFXRECT* clipRect);

void TFB_DrawCanvas_Delete(TFB_Canvas canvas);

void TFB_DrawCanvas_Line(int x1, int y1, int x2, int y2, Color color,
						 DrawMode, TFB_Canvas target, uint8_t thickness);
void TFB_DrawCanvas_Rect(GFXRECT* rect, Color, DrawMode, TFB_Canvas target);
void TFB_DrawCanvas_Image(TFB_Image* img, int x, int y, int scale,
						  uqm::TFBScaleMode scaleMode, TFB_ColorMap*, DrawMode, TFB_Canvas target);
void TFB_DrawCanvas_FilledImage(TFB_Image* img, int x, int y, int scale,
								uqm::TFBScaleMode scaleMode, Color, DrawMode, TFB_Canvas target);
void TFB_DrawCanvas_FontChar(TFB_Char*, TFB_Image* backing, int x, int y,
							 DrawMode, TFB_Canvas target);
void TFB_DrawCanvas_CopyRect(TFB_Canvas source, const GFXRECT* srcRect,
							 TFB_Canvas target, GFXPOINT dstPt);
void TFB_DrawCanvas_MaskImage(TFB_Image* img, DrawMode mode, TFB_Canvas target, Color* fill);

bool TFB_DrawCanvas_GetFontCharData(TFB_Canvas canvas, uint8_t* outData,
									unsigned dataPitch);
Color* TFB_DrawCanvas_ExtractPalette(TFB_Canvas canvas);
void TFB_DrawCanvas_SetPalette(TFB_Canvas target, Color palette[256]);
int TFB_DrawCanvas_GetTransparentIndex(TFB_Canvas canvas);
void TFB_DrawCanvas_SetTransparentIndex(TFB_Canvas canvas, int i,
										bool rleaccel);
bool TFB_DrawCanvas_GetTransparentColor(TFB_Canvas canvas,
										Color* color);
void TFB_DrawCanvas_SetTransparentColor(TFB_Canvas canvas,
										Color color, bool rleaccel);
void TFB_DrawCanvas_CopyTransparencyInfo(TFB_Canvas src, TFB_Canvas dst);
void TFB_DrawCanvas_Initialize(void);
void TFB_DrawCanvas_Lock(TFB_Canvas canvas);
void TFB_DrawCanvas_Unlock(TFB_Canvas canvas);
void TFB_DrawCanvas_GetScreenFormat(TFB_PixelFormat* fmt);
int TFB_DrawCanvas_GetStride(TFB_Canvas canvas);
void* TFB_DrawCanvas_GetLine(TFB_Canvas canvas, int line);
Color TFB_DrawCanvas_GetPixel(TFB_Canvas canvas, int x, int y);
bool TFB_DrawCanvas_Intersect(TFB_Canvas canvas1, GFXPOINT c1org,
							  TFB_Canvas canvas2, GFXPOINT c2org, const GFXRECT* interRect);

bool TFB_DrawCanvas_GetPixelColors(TFB_Canvas, Color* pixels,
								   int width, int height);
bool TFB_DrawCanvas_SetPixelColors(TFB_Canvas, const Color* pixels,
								   int width, int height);
bool TFB_DrawCanvas_GetPixelIndexes(TFB_Canvas, uint8_t* data,
									int width, int height);
bool TFB_DrawCanvas_SetPixelIndexes(TFB_Canvas, const uint8_t* data,
									int width, int height);

const char* TFB_DrawCanvas_GetError(void);

TFB_Canvas TFB_GetScreenCanvas(SCREEN screen);
TFB_Canvas TFB_GetFPSCanvas(void);

#endif
