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
#include "core/log/log.h"

DRAWABLE
GetFrameParentDrawable(FRAME f)
{
	if (f != nullptr)
	{
		return f->parent;
	}
	return nullptr;
}

FRAME
CaptureDrawable(DRAWABLE DrawablePtr)
{
	if (DrawablePtr)
	{
		return &DrawablePtr->Frame[0];
	}

	return nullptr;
}

DRAWABLE
ReleaseDrawable(FRAME FramePtr)
{
	if (FramePtr != 0)
	{
		DRAWABLE Drawable;

		Drawable = GetFrameParentDrawable(FramePtr);

		return (Drawable);
	}

	return nullptr;
}

uint16_t
GetFrameCount(FRAME FramePtr)
{
	DRAWABLE_DESC* DrawablePtr;

	if (FramePtr == 0)
	{
		return (0);
	}

	DrawablePtr = GetFrameParentDrawable(FramePtr);
	return DrawablePtr->MaxIndex + 1;
}

uint16_t
GetFrameIndex(FRAME FramePtr)
{
	if (FramePtr == 0)
	{
		return (0);
	}

	return FramePtr->Index;
}

FRAME
SetAbsFrameIndex(FRAME FramePtr, uint16_t FrameIndex)
{
	if (FramePtr != 0)
	{
		DRAWABLE_DESC* DrawablePtr;

		DrawablePtr = GetFrameParentDrawable(FramePtr);

		FrameIndex = FrameIndex % (DrawablePtr->MaxIndex + 1);
		FramePtr = &DrawablePtr->Frame[FrameIndex];
	}
	return FramePtr;
}

FRAME
SetRelFrameIndex(FRAME FramePtr, int16_t FrameOffs)
{
	if (FramePtr != 0)
	{
		uint16_t num_frames;
		DRAWABLE_DESC* DrawablePtr;

		DrawablePtr = GetFrameParentDrawable(FramePtr);
		num_frames = DrawablePtr->MaxIndex + 1;
		if (FrameOffs < 0)
		{
			while ((FrameOffs += num_frames) < 0)
				;
		}

		FrameOffs = ((int16_t)FramePtr->Index + FrameOffs) % num_frames;
		FramePtr = &DrawablePtr->Frame[FrameOffs];
	}

	return FramePtr;
}

FRAME
SetEquFrameIndex(FRAME DstFramePtr, FRAME SrcFramePtr)
{
	uint16_t Index;

	if (!DstFramePtr || !SrcFramePtr)
	{
		return 0;
	}

	Index = GetFrameIndex(SrcFramePtr);
#ifdef DEBUG
	{
		DRAWABLE_DESC* DrawablePtr = GetFrameParentDrawable(DstFramePtr);
		if (Index > DrawablePtr->MaxIndex)
		{
			uqm::log::debug("SetEquFrameIndex: source index ({}) beyond "
							"destination range ({})",
							(int)Index,
							(int)DrawablePtr->MaxIndex);
		}
	}
#endif

	return SetAbsFrameIndex(DstFramePtr, Index);
}

FRAME
IncFrameIndex(FRAME FramePtr)
{
	DRAWABLE_DESC* DrawablePtr;

	if (FramePtr == 0)
	{
		return (0);
	}

	DrawablePtr = GetFrameParentDrawable(FramePtr);
	if (FramePtr->Index < DrawablePtr->MaxIndex)
	{
		return ++FramePtr;
	}
	else
	{
		return DrawablePtr->Frame.data();
	}
}

FRAME
DecFrameIndex(FRAME FramePtr)
{
	if (FramePtr == 0)
	{
		return (0);
	}

	if (FramePtr->Index > 0)
	{
		return --FramePtr;
	}
	else
	{
		DRAWABLE_DESC* DrawablePtr;

		DrawablePtr = GetFrameParentDrawable(FramePtr);
		return &DrawablePtr->Frame[DrawablePtr->MaxIndex];
	}
}

bool IsFrameIndexed(FRAME FramePtr)
{
	return TFB_DrawCanvas_IsPaletted(FramePtr->image->NormalImg);
}
