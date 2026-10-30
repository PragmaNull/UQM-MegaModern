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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gfx_common.h"
#include "tfb_draw.h"
#include "drawcmd.h"
#include "core/log/log.h"
#include "libs/memlib.h"


static const HOT_SPOT NullHs = {0, 0};

void TFB_DrawScreen_Line(int x1, int y1, int x2, int y2, Color color,
						 DrawMode mode, SCREEN dest, uint8_t thickness)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_LINE;
	DC.data.line.x1 = x1;
	DC.data.line.y1 = y1;
	DC.data.line.x2 = x2;
	DC.data.line.y2 = y2;
	DC.data.line.color = color;
	DC.data.line.drawMode = mode;
	DC.data.line.destBuffer = dest;
	DC.data.line.thickness = thickness;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_Rect(GFXRECT* rect, Color color, DrawMode mode, SCREEN dest)
{
	GFXRECT locRect;
	TFB_DrawCommand DC;

	if (!rect)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = CanvasWidth;
		locRect.extent.height = CanvasHeight;
		rect = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_RECTANGLE;
	DC.data.rect.rect = *rect;
	DC.data.rect.color = color;
	DC.data.rect.drawMode = mode;
	DC.data.rect.destBuffer = dest;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_Image(TFB_Image* img, int x, int y, int scale,
						  uqm::TFBScaleMode scaleMode, TFB_ColorMap* cmap, DrawMode mode, SCREEN dest)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_IMAGE;
	DC.data.image.image = img;
	DC.data.image.colormap = cmap;
	DC.data.image.x = x;
	DC.data.image.y = y;
	DC.data.image.scale = (scale == GSCALE_IDENTITY) ? 0 : scale;
	DC.data.image.scaleMode = scaleMode;
	DC.data.image.drawMode = mode;
	DC.data.image.destBuffer = dest;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_FilledImage(TFB_Image* img, int x, int y, int scale,
								uqm::TFBScaleMode scaleMode, Color color, DrawMode mode, SCREEN dest)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_FILLEDIMAGE;
	DC.data.filledimage.image = img;
	DC.data.filledimage.x = x;
	DC.data.filledimage.y = y;
	DC.data.filledimage.scale = (scale == GSCALE_IDENTITY) ? 0 : scale;
	DC.data.filledimage.scaleMode = scaleMode;
	DC.data.filledimage.color = color;
	DC.data.filledimage.drawMode = mode;
	DC.data.filledimage.destBuffer = dest;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_FontChar(TFB_Char* fontChar, TFB_Image* backing,
							 int x, int y, DrawMode mode, SCREEN dest)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_FONTCHAR;
	DC.data.fontchar.fontchar = fontChar;
	DC.data.fontchar.backing = backing;
	DC.data.fontchar.x = x;
	DC.data.fontchar.y = y;
	DC.data.fontchar.drawMode = mode;
	DC.data.fontchar.destBuffer = dest;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_CopyToImage(TFB_Image* img, const GFXRECT* r, SCREEN src)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_COPYTOIMAGE;
	DC.data.copytoimage.rect = *r;
	DC.data.copytoimage.image = img;
	DC.data.copytoimage.srcBuffer = src;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_Copy(const GFXRECT* r, SCREEN src, SCREEN dest)
{
	GFXRECT locRect;
	TFB_DrawCommand DC;

	if (!r)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = CanvasWidth;
		locRect.extent.height = CanvasHeight;
		r = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_COPY;
	DC.data.copy.rect = *r;
	DC.data.copy.srcBuffer = src;
	DC.data.copy.destBuffer = dest;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_SetMipmap(TFB_Image* img, TFB_Image* mmimg, int hotx, int hoty)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_SETMIPMAP;
	DC.data.setmipmap.image = img;
	DC.data.setmipmap.mipmap = mmimg;
	DC.data.setmipmap.hotx = hotx;
	DC.data.setmipmap.hoty = hoty;

	TFB_EnqueueDrawCommand(&DC);
}

void TFB_DrawScreen_DeleteImage(TFB_Image* img)
{
	if (img)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEIMAGE;
		DC.data.deleteimage.image = img;

		TFB_EnqueueDrawCommand(&DC);
	}
}

void TFB_DrawScreen_DeleteData(void* data)
// data must be a result of HXalloc() call
{
	if (data)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEDATA;
		DC.data.deletedata.data = data;

		TFB_EnqueueDrawCommand(&DC);
	}
}

void TFB_DrawScreen_WaitForSignal(void)
{
	// The renderer releases this when it reaches the command.
	thread_local uqstl::binary_semaphore flushed {0};
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_SENDSIGNAL;
	DrawCommand.data.sendsignal.sem = &flushed;
	Lock_DCQ(1);
	TFB_BatchReset();
	TFB_EnqueueDrawCommand(&DrawCommand);
	Unlock_DCQ();
	flushed.acquire();
}

void TFB_DrawScreen_ReinitVideo(uqm::GfxDriver driver, uqm::GfxFlags flags, int width, int height)
{
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_REINITVIDEO;
	DrawCommand.data.reinitvideo.driver = driver;
	DrawCommand.data.reinitvideo.flags = flags;
	DrawCommand.data.reinitvideo.width = width;
	DrawCommand.data.reinitvideo.height = height;
	TFB_EnqueueDrawCommand(&DrawCommand);
}

void TFB_DrawScreen_Callback(void (*callback)(void* arg), void* arg)
{
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_CALLBACK;
	DrawCommand.data.callback.callback = callback;
	DrawCommand.data.callback.arg = arg;
	TFB_EnqueueDrawCommand(&DrawCommand);
}

void TFB_DrawImage_Line(int x1, int y1, int x2, int y2, Color color,
						DrawMode mode, TFB_Image* target, uint8_t thickness)
{
	const uqm::LockGuard lock {target->mutex};
	TFB_DrawCanvas_Line(x1, y1, x2, y2, color, mode, target->NormalImg, thickness);
	target->dirty = true;
}

void TFB_DrawImage_Rect(GFXRECT* rect, Color color, DrawMode mode, TFB_Image* target)
{
	const uqm::LockGuard lock {target->mutex};
	TFB_DrawCanvas_Rect(rect, color, mode, target->NormalImg);
	target->dirty = true;
}

void TFB_DrawImage_Image(TFB_Image* img, int x, int y, int scale,
						 uqm::TFBScaleMode scaleMode, TFB_ColorMap* cmap, DrawMode mode, TFB_Image* target)
{
	const uqm::LockGuard lock {target->mutex};
	TFB_DrawCanvas_Image(img, x, y, scale, scaleMode, cmap,
						 mode, target->NormalImg);
	target->dirty = true;
}

void TFB_DrawImage_FilledImage(TFB_Image* img, int x, int y, int scale,
							   uqm::TFBScaleMode scaleMode, Color color, DrawMode mode, TFB_Image* target)
{
	const uqm::LockGuard lock {target->mutex};
	TFB_DrawCanvas_FilledImage(img, x, y, scale, scaleMode, color,
							   mode, target->NormalImg);
	target->dirty = true;
}

void TFB_DrawImage_FontChar(TFB_Char* fontChar, TFB_Image* backing,
							int x, int y, DrawMode mode, TFB_Image* target)
{
	const uqm::LockGuard lock {target->mutex};
	TFB_DrawCanvas_FontChar(fontChar, backing, x, y, mode, target->NormalImg);
	target->dirty = true;
}

void TFB_DrawImage_MaskImage(TFB_Image* img, DrawMode mode, TFB_Image* target, Color* fill)
{
	const uqm::LockGuard lock {target->mutex};
	TFB_DrawCanvas_MaskImage(img, mode, target->NormalImg, fill);
	target->dirty = true;
}


TFB_Image*
TFB_DrawImage_New(TFB_Canvas canvas)
{
	auto* img {new TFB_Image {}};
	img->ScaledImg = nullptr;
	img->MipmapImg = nullptr;
	img->FilledImg = nullptr;
	img->colormap_index = -1;
	img->colormap_version = 0;
	img->NormalHs = NullHs;
	img->MipmapHs = NullHs;
	img->last_scale_hs = NullHs;
	img->last_scale_type = uqm::TFBScaleMode::None;
	img->last_scale = 0;
	img->dirty = false;
	TFB_DrawCanvas_GetExtent(canvas, &img->extent);

	if (TFB_DrawCanvas_IsPaletted(canvas))
	{
		img->NormalImg = canvas;
	}
	else
	{
		img->NormalImg = TFB_DrawCanvas_ToScreenFormat(canvas);
	}

	return img;
}

TFB_Image*
TFB_DrawImage_CreateForScreen(int w, int h, bool withalpha)
{
	auto* img {new TFB_Image {}};
	img->ScaledImg = nullptr;
	img->MipmapImg = nullptr;
	img->FilledImg = nullptr;
	img->colormap_index = -1;
	img->colormap_version = 0;
	img->NormalHs = NullHs;
	img->MipmapHs = NullHs;
	img->last_scale_hs = NullHs;
	img->last_scale_type = uqm::TFBScaleMode::None;
	img->last_scale = 0;
	img->extent.width = w;
	img->extent.height = h;

	img->NormalImg = TFB_DrawCanvas_New_ForScreen(w, h, withalpha);

	return img;
}

TFB_Image*
TFB_DrawImage_New_Rotated(TFB_Image* img, int angle)
{
	TFB_Canvas dst;
	EXTENT size;
	TFB_Image* newimg;

	/* sanity check */
	if (!img->NormalImg)
	{
		uqm::log::warn("TFB_DrawImage_New_Rotated: "
					   "source canvas is nullptr! Failing.");
		return nullptr;
	}

	TFB_DrawCanvas_GetRotatedExtent(img->NormalImg, angle, &size);
	dst = TFB_DrawCanvas_New_RotationTarget(img->NormalImg, angle);
	if (!dst)
	{
		uqm::log::warn("TFB_DrawImage_New_Rotated: "
					   "rotation target canvas not created! Failing.");
		return nullptr;
	}
	TFB_DrawCanvas_Rotate(img->NormalImg, dst, angle, size);

	newimg = TFB_DrawImage_New(dst);
	return newimg;
}

void TFB_DrawImage_SetMipmap(TFB_Image* img, TFB_Image* mmimg, int hotx, int hoty)
{
	bool imgpal;
	bool mmpal;

	if (!img || !mmimg)
	{
		return;
	}

	const uqm::LockGuard imgLock {img->mutex};
	const uqm::LockGuard mmimgLock {mmimg->mutex};

	// Either both images must be using the same colormap, or mipmap image
	// must not be paletted. This restriction is due to the current
	// implementation of fill-stamp, which replaces the palette with
	// fill color.
	imgpal = TFB_DrawCanvas_IsPaletted(img->NormalImg);
	mmpal = TFB_DrawCanvas_IsPaletted(mmimg->NormalImg);
	if (!mmpal || (mmpal && imgpal && img->colormap_index == mmimg->colormap_index))
	{
		img->MipmapImg = mmimg->NormalImg;
		img->MipmapHs.x = hotx;
		img->MipmapHs.y = hoty;
	}
	else
	{
		img->MipmapImg = nullptr;
	}
}

void TFB_DrawImage_Delete(TFB_Image* image)
{
	if (image == 0)
	{
		uqm::log::warn("INTERNAL ERROR: Tried to delete a null image!");
		/* Should we die here? */
		return;
	}
	{
		const uqm::LockGuard lock {image->mutex};

		TFB_DrawCanvas_Delete(image->NormalImg);

		if (image->ScaledImg)
		{
			TFB_DrawCanvas_Delete(image->ScaledImg);
			image->ScaledImg = 0;
		}

		if (image->FilledImg)
		{
			TFB_DrawCanvas_Delete(image->FilledImg);
			image->FilledImg = 0;
		}
	}

	delete image;
}

void TFB_DrawImage_FixScaling(TFB_Image* image, int scaleTarget, uqm::TFBScaleMode type)
{
	if (image->dirty || !image->ScaledImg || scaleTarget != image->last_scale || type != image->last_scale_type)
	{
		image->dirty = false;
		image->ScaledImg = TFB_DrawCanvas_New_ScaleTarget(image->NormalImg,
														  image->ScaledImg, type, image->last_scale_type);

		if (type == uqm::TFBScaleMode::Nearest)
		{
			TFB_DrawCanvas_Rescale_Nearest(image->NormalImg,
										   image->ScaledImg, scaleTarget, &image->NormalHs,
										   &image->extent, &image->last_scale_hs);
		}
		else if (type == uqm::TFBScaleMode::Bilinear)
		{
			TFB_DrawCanvas_Rescale_Bilinear(image->NormalImg,
											image->ScaledImg, scaleTarget, &image->NormalHs,
											&image->extent, &image->last_scale_hs);
		}
		else
		{
			TFB_DrawCanvas_Rescale_Trilinear(image->NormalImg,
											 image->MipmapImg, image->ScaledImg, scaleTarget,
											 &image->NormalHs, &image->MipmapHs,
											 &image->extent, &image->last_scale_hs);
		}

		image->last_scale_type = type;
		image->last_scale = scaleTarget;
	}
}

bool TFB_DrawImage_Intersect(TFB_Image* img1, GFXPOINT img1org,
							 TFB_Image* img2, GFXPOINT img2org, const GFXRECT* interRect)
{
	const uqm::LockGuard lock1 {img1->mutex};
	const uqm::LockGuard lock2 {img2->mutex};
	return TFB_DrawCanvas_Intersect(img1->NormalImg, img1org,
									img2->NormalImg, img2org, interRect);
}

void TFB_DrawImage_CopyRect(TFB_Image* source, const GFXRECT* srcRect,
							TFB_Image* target, GFXPOINT dstPt)
{
	const uqm::LockGuard sourceLock {source->mutex};
	const uqm::LockGuard targetLock {target->mutex};
	TFB_DrawCanvas_CopyRect(source->NormalImg, srcRect,
							target->NormalImg, dstPt);
	target->dirty = true;
}
