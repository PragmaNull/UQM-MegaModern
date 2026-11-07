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

#include "libs/graphics/drawable.h"

#include <math.h>

#include "core/log/log.h"
#include "libs/gfxlib.h"
#include "libs/graphics/context.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/sdl/sdl_common.h"
#include "libs/graphics/tfb_draw.h"
#include "tfb_draw.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FrameRef _CurFramePtr;

FrameRef SetContextFGFrame(FrameRef Frame)
{
	FrameRef LastFrame {};

	if (Frame != (LastFrame = (FrameRef)_CurFramePtr))
	{
		if (LastFrame)
		{
			DeactivateDrawable();
		}

		_CurFramePtr = Frame;
		if (_CurFramePtr)
		{
			ActivateDrawable();
		}

		if (ContextActive())
		{
			SwitchContextFGFrame(Frame);
		}
	}

	return (LastFrame);
}

FrameRef GetContextFGFrame(void)
{
	return _CurFramePtr;
}

static DrawableRef request_drawable(uint16_t NumFrames, DrawableType drawableType,
									CreateFlags flags, int16_t width, int16_t height)
{
	DrawableRef Drawable {AllocDrawable(NumFrames)};
	if (!Drawable)
	{
		return nullptr;
	}

	Drawable->Flags = flags;
	Drawable->MaxIndex = NumFrames - 1;

	for (uint16_t i {0}; i < NumFrames; ++i)
	{
		FrameRef FramePtr = &Drawable->Frame[i];

		if (drawableType == RamDrawable && width > 0 && height > 0)
		{
			FramePtr->image = TFB_DrawImage_New(TFB_DrawCanvas_New_TrueColor(
				width, height, testFlag(flags, CreateFlags::WantAlpha)));
		}

		FramePtr->Type = drawableType;
		FramePtr->Index = i;
		SetFrameBounds(FramePtr, width, height);
	}

	return Drawable;
}

// Kruzen: New construct to create paletted drawable
// Uses previously unused TFB_DrawCanvas_New_Paletted()
static DrawableRef request_indexed_drawable(Color* palette, int16_t width, int16_t height)
{
	DrawableRef Drawable {AllocDrawable(1)};
	if (!Drawable)
	{
		return nullptr;
	}

	Drawable->Flags = CreateFlags::WantPixmap;
	Drawable->MaxIndex = 0;

	{
		FrameRef FramePtr = &Drawable->Frame[0];

		if (width > 0 && height > 0)
		{
			FramePtr->image = TFB_DrawImage_New(
				TFB_DrawCanvas_New_Paletted(
					width, height, palette, -1));
		}
		else
		{
			return nullptr;
		}

		FramePtr->Type = RamDrawable;
		FramePtr->Index = 0;
		SetFrameBounds(FramePtr, width, height);
	}

	return Drawable;
}

DrawableRef CreateDisplay(CreateFlags createFlags, int16_t* pwidth, int16_t* pheight)
{
	DrawableRef Drawable {};

	// TODO: CanvasWidth and CanvasHeight should be passed in
	//   instead of returned.
	Drawable = request_drawable(1, ScreenDrawable,
								(createFlags & (CreateFlags::WantPixmap | CreateFlags::WantMask)),
								CanvasWidth, CanvasHeight);
	if (Drawable)
	{
		FrameRef F {CaptureDrawable(Drawable)};
		if (F == 0)
		{
			DestroyDrawable(Drawable);
		}
		else
		{
			*pwidth = GetFrameWidth(F);
			*pheight = GetFrameHeight(F);

			ReleaseDrawable(F);
			return (Drawable);
		}
	}

	*pwidth = *pheight = 0;
	return (0);
}

DrawableRef AllocDrawable(uint16_t n)
{
	auto* Drawable {new DrawableDesc {}};
	Drawable->Frame.assign(n, FrameDesc {});
	for (FrameDesc& F : Drawable->Frame)
	{
		F.parent = Drawable;
	}
	return Drawable;
}

DrawableRef CreateDrawable(CreateFlags createFlags, int16_t width, int16_t height, uint16_t num_frames)
{
	DrawableRef Drawable {};

	Drawable = request_drawable(num_frames, RamDrawable,
								(createFlags & (CreateFlags::WantMask | CreateFlags::WantPixmap | CreateFlags::WantAlpha | CreateFlags::MappedToDisplay)),
								width, height);
	if (Drawable)
	{
		FrameRef F {CaptureDrawable(Drawable)};
		if (F)
		{
			ReleaseDrawable(F);

			return (Drawable);
		}
	}

	return (0);
}

// Kruzen: New construct to create paletted drawable
DrawableRef CreateIndexedDrawable(Color* palette, int16_t width, int16_t height)
{
	DrawableRef Drawable {request_indexed_drawable(palette, width, height)};

	if (Drawable)
	{
		FrameRef F {CaptureDrawable(Drawable)};
		if (F)
		{
			ReleaseDrawable(F);

			return (Drawable);
		}
	}

	return (0);
}

bool DestroyDrawable(DrawableRef Drawable)
{
	if (_CurFramePtr && (Drawable == _CurFramePtr->parent))
	{
		SetContextFGFrame((FrameRef) nullptr);
	}

	if (Drawable)
	{
		FreeDrawable(Drawable);

		return (true);
	}

	return (false);
}

bool GetFrameRect(FrameRef FramePtr, GfxRect* pRect)
{
	if (FramePtr)
	{
		pRect->corner.x = -FramePtr->hotSpot.x;
		pRect->corner.y = -FramePtr->hotSpot.y;
		pRect->extent = GetFrameBounds(FramePtr);

		return (true);
	}

	return (false);
}

HotSpot SetFrameHot(FrameRef FramePtr, HotSpot hotSpot)
{
	if (FramePtr)
	{
		HotSpot OldHot;

		OldHot = FramePtr->hotSpot;
		FramePtr->hotSpot = hotSpot;

		return (OldHot);
	}

	return (MAKE_HOT_SPOT(0, 0));
}

HotSpot GetFrameHot(FrameRef FramePtr)
{
	if (FramePtr)
	{
		return FramePtr->hotSpot;
	}

	return (MAKE_HOT_SPOT(0, 0));
}

DrawableRef RotateFrame(FrameRef Frame, int angle_deg)
{
	double dx {NAN};
	double dy {NAN};
	double d {NAN};
	const double angle = angle_deg * M_PI / 180;

	if (!Frame)
	{
		return nullptr;
	}

	assert(Frame->Type != ScreenDrawable);

	DrawableRef Drawable {request_drawable(1, RamDrawable, CreateFlags::WantPixmap, 0, 0)};
	if (!Drawable)
	{
		return 0;
	}
	FrameRef RotFramePtr {CaptureDrawable(Drawable)};
	if (!RotFramePtr)
	{
		FreeDrawable(Drawable);
		return 0;
	}

	RotFramePtr->image = TFB_DrawImage_New_Rotated(
		Frame->image, angle_deg);
	SetFrameBounds(RotFramePtr, RotFramePtr->image->extent.width,
				   RotFramePtr->image->extent.height);

	/* now we need to rotate the hot-spot, eww */
	dx = Frame->hotSpot.x - (GetFrameWidth(Frame) / 2);
	dy = Frame->hotSpot.y - (GetFrameHeight(Frame) / 2);
	d = sqrt((double)dx * dx + (double)dy * dy);
	if ((int)d != 0)
	{
		const double organg = atan2(-dy, dx);
		dx = cos(organg + angle) * d;
		dy = -sin(organg + angle) * d;
	}
	RotFramePtr->hotSpot.x = (GetFrameWidth(RotFramePtr) / 2) + (int)dx;
	RotFramePtr->hotSpot.y = (GetFrameHeight(RotFramePtr) / 2) + (int)dy;

	ReleaseDrawable(RotFramePtr);

	return Drawable;
}

// color.a is ignored
void SetFrameTransparentColor(FrameRef frame, Color color)
{
	if (!frame)
	{
		return;
	}

	assert(frame->Type != ScreenDrawable);

	tfb::Image* img {frame->image};
	const uqm::LockGuard imageLock {img->mutex};

	// TODO: This should defer to TFB_DrawImage instead
	TFB_DrawCanvas_SetTransparentColor(img->NormalImg, color, false);
}

Color GetFramePixel(FrameRef frame, GfxPoint pixelPt)
{
	Color ret;

	if (!frame)
	{
		return BuildColorRgba(0, 0, 0, 0);
	}

	assert(frame->Type != ScreenDrawable);

	tfb::Image* img {frame->image};
	const uqm::LockGuard imageLock {img->mutex};

	// TODO: This should defer to TFB_DrawImage instead
	ret = TFB_DrawCanvas_GetPixel(img->NormalImg, pixelPt.x, pixelPt.y);

	return ret;
}

static FrameRef makeMatchingFrame(FrameRef frame, int width, int height)
{
	const bool dst_has_alpha =
		SDL_GetPixelFormatDetails(((SDL_Surface*)(frame->image->NormalImg))->format)->Amask != 0;

	const CreateFlags flags {dst_has_alpha ? CreateFlags::WantAlpha : GetFrameParentDrawable(frame)->Flags};
	DrawableRef drawable {CreateDrawable(flags, width, height, 1)};
	if (!drawable)
	{
		return nullptr;
	}
	FrameRef newFrame {CaptureDrawable(drawable)};
	if (!newFrame)
	{
		FreeDrawable(drawable);
		return nullptr;
	}

	return newFrame;
}

// Kruzen: New construct to create paletted drawable
// Copies makeMatchingFrame() call hierarchy
static FrameRef makeMatchingIndexedFrame(FrameRef frame, int width, int height)
{
	DrawableRef drawable {};
	uqstl::optional<uqstl::array<Color, 256>> palette {TFB_DrawCanvas_ExtractPalette(frame->image->NormalImg)};

	drawable =
		CreateIndexedDrawable(palette ? palette->data() : nullptr, width, height);
	if (!drawable)
	{
		return nullptr;
	}
	FrameRef newFrame {CaptureDrawable(drawable)};
	if (!newFrame)
	{
		FreeDrawable(drawable);
		return nullptr;
	}

	return newFrame;
}

// Creates an new DrawableRef containing a copy of specified FrameRef's rect
// Source FrameRef must not be a ScreenDrawable
DrawableRef CopyFrameRect(FrameRef frame, const GfxRect* area)
{
	FrameRef newFrame {};
	const GfxPoint nullPt = MAKE_POINT(0, 0);

	if (!frame)
	{
		return nullptr;
	}

	assert(frame->Type != ScreenDrawable);

	newFrame = makeMatchingFrame(frame, area->extent.width,
								 area->extent.height);
	if (!newFrame)
	{
		return nullptr;
	}

	TFB_DrawImage_CopyRect(frame->image, area, newFrame->image, nullPt);

	return ReleaseDrawable(newFrame);
}

// Creates an new DrawableRef mostly identical to specified FrameRef
// Source FrameRef must not be a ScreenDrawable
DrawableRef CloneFrame(FrameRef frame)
{
	GfxRect r;

	if (!frame)
	{
		return nullptr;
	}

	assert(frame->Type != ScreenDrawable);

	GetFrameRect(frame, &r);
	r.corner.x = 0;
	r.corner.y = 0;

	FrameRef newFrame {CaptureDrawable(CopyFrameRect(frame, &r))};
	if (!newFrame)
	{
		return nullptr;
	}

	// copy the hot-spot
	newFrame->hotSpot = frame->hotSpot;

	return ReleaseDrawable(newFrame);
}

// Creates a new DrawableRef of specified size and scales the passed
// frame onto it. The aspect ratio is not preserved.
DrawableRef RescaleFrame(FrameRef frame, int width, int height)
{
	FrameRef newFrame {};

	if (!frame)
	{
		return nullptr;
	}

	assert(frame->Type != ScreenDrawable);

	if (TFB_DrawCanvas_IsPaletted(frame->image->NormalImg))
	{ // request Paletted frame
		newFrame = makeMatchingIndexedFrame(frame, width, height);
	}
	else
	{ // request TrueColor frame
		newFrame = makeMatchingFrame(frame, width, height);
	}

	if (!newFrame)
	{
		return nullptr;
	}

	// scale the hot-spot
	newFrame->hotSpot.x = frame->hotSpot.x * width / frame->Bounds.width;
	newFrame->hotSpot.y = frame->hotSpot.y * height / frame->Bounds.height;

	tfb::Image* img {frame->image};
	const uqm::LockGuard imageLock {img->mutex};
	// NOTE: We do not lock the target image because nothing has a
	//   reference to it yet!
	tfb::Canvas src {img->NormalImg};
	tfb::Canvas dst {newFrame->image->NormalImg};

	TFB_DrawCanvas_Rescale_Nearest(src, dst, -1, nullptr, nullptr, nullptr);

	return ReleaseDrawable(newFrame);
}

// Creates a new DrawableRef of specified percentage and scales the passed
// frame onto it. The aspect ratio is preserved.
DrawableRef RescalePercentage(FrameRef frame, float percentage)
{
	FrameRef newFrame {};

	if (!frame || (frame->Bounds.height == 1 && frame->Bounds.width == 1))
	{
		return nullptr;
	}

	assert(frame->Type != ScreenDrawable);

	percentage = percentage / 100;

	newFrame = makeMatchingFrame(frame,
								 (int)(frame->Bounds.width * percentage),
								 (int)(frame->Bounds.height * percentage));

	if (!newFrame)
	{
		return nullptr;
	}

	// scale the hot-spot
	newFrame->hotSpot.x = (int16_t)(frame->hotSpot.x * percentage);
	newFrame->hotSpot.y = (int16_t)(frame->hotSpot.y * percentage);

	tfb::Image* img {frame->image};
	const uqm::LockGuard imageLock {img->mutex};
	// NOTE: We do not lock the target image because nothing has a
	//   reference to it yet!
	tfb::Canvas src {img->NormalImg};
	tfb::Canvas dst {newFrame->image->NormalImg};

	TFB_DrawCanvas_Rescale_Bilinear(src, dst, -1, nullptr, nullptr, nullptr);

	return ReleaseDrawable(newFrame);
}

bool ReadFramePixelColors(FrameRef frame, Color* pixels, int width, int height)
{
	if (!frame)
	{
		return false;
	}

	assert(frame->Type != ScreenDrawable);

	// TODO: Do we need to lock the img->mutex here?
	const tfb::Image* img {frame->image};
	return TFB_DrawCanvas_GetPixelColors(img->NormalImg, pixels,
										 width, height);
}

// Warning: this functions bypasses DCQ, which is why it is not a DrawXXX
bool WriteFramePixelColors(FrameRef frame, const Color* pixels, int width, int height)
{
	if (!frame)
	{
		return false;
	}

	assert(frame->Type != ScreenDrawable);

	// TODO: Do we need to lock the img->mutex here?
	const tfb::Image* img {frame->image};
	return TFB_DrawCanvas_SetPixelColors(img->NormalImg, pixels,
										 width, height);
}

bool ReadFramePixelIndexes(FrameRef frame, uint8_t* pixels, int width, int height, bool paletted)
{
	if (!frame)
	{
		return false;
	}

	assert(frame->Type != ScreenDrawable);

	// TODO: Do we need to lock the img->mutex here?
	const tfb::Image* img {frame->image};

	// JMS_GFX: Don't try to read pixel indexes for non-indexed images.
	if (paletted)
	{
		return TFB_DrawCanvas_GetPixelIndexes(img->NormalImg, pixels,
											  width, height);
	}
	else
	{
		return false;
	}
}

// Warning: this functions bypasses DCQ, which is why it is not a DrawXXX
bool WriteFramePixelIndexes(FrameRef frame, const uint8_t* pixels, int width, int height)
{
	if (!frame)
	{
		return false;
	}

	assert(frame->Type != ScreenDrawable);

	// TODO: Do we need to lock the img->mutex here?
	const tfb::Image* img {frame->image};
	return TFB_DrawCanvas_SetPixelIndexes(img->NormalImg, pixels,
										  width, height);
}
