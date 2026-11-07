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

#include "core/log/log.h"
#include "gfxintrn.h"

DrawableRef GetFrameParentDrawable(FrameRef f)
{
	if (f != nullptr)
	{
		return f->parent;
	}
	return nullptr;
}

FrameRef CaptureDrawable(DrawableRef DrawablePtr)
{
	if (DrawablePtr)
	{
		return &DrawablePtr->Frame[0];
	}

	return nullptr;
}

DrawableRef ReleaseDrawable(FrameRef FramePtr)
{
	if (FramePtr != 0)
	{
		DrawableRef Drawable {GetFrameParentDrawable(FramePtr)};

		return (Drawable);
	}

	return nullptr;
}

uint16_t GetFrameCount(FrameRef FramePtr)
{
	if (FramePtr == 0)
	{
		return (0);
	}

	const DrawableDesc* DrawablePtr {GetFrameParentDrawable(FramePtr)};
	return DrawablePtr->MaxIndex + 1;
}

uint16_t GetFrameIndex(FrameRef FramePtr)
{
	if (FramePtr == 0)
	{
		return (0);
	}

	return FramePtr->Index;
}

FrameRef SetAbsFrameIndex(FrameRef FramePtr, uint16_t FrameIndex)
{
	if (FramePtr != 0)
	{
		DrawableDesc* DrawablePtr {GetFrameParentDrawable(FramePtr)};

		FrameIndex = FrameIndex % (DrawablePtr->MaxIndex + 1);
		FramePtr = &DrawablePtr->Frame[FrameIndex];
	}
	return FramePtr;
}

FrameRef SetRelFrameIndex(FrameRef FramePtr, int16_t FrameOffs)
{
	if (FramePtr != 0)
	{
		uint16_t num_frames {};

		DrawableDesc* DrawablePtr {GetFrameParentDrawable(FramePtr)};
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

FrameRef SetEquFrameIndex(FrameRef DstFramePtr, FrameRef SrcFramePtr)
{
	if (!DstFramePtr || !SrcFramePtr)
	{
		return 0;
	}

	const uint16_t Index {GetFrameIndex(SrcFramePtr)};
#ifdef DEBUG
	{
		DrawableDesc* DrawablePtr = GetFrameParentDrawable(DstFramePtr);
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

FrameRef IncFrameIndex(FrameRef FramePtr)
{
	if (FramePtr == 0)
	{
		return (0);
	}

	DrawableDesc* DrawablePtr {GetFrameParentDrawable(FramePtr)};
	if (FramePtr->Index < DrawablePtr->MaxIndex)
	{
		return ++FramePtr;
	}
	else
	{
		return DrawablePtr->Frame.data();
	}
}

FrameRef DecFrameIndex(FrameRef FramePtr)
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
		DrawableDesc* DrawablePtr {GetFrameParentDrawable(FramePtr)};
		return &DrawablePtr->Frame[DrawablePtr->MaxIndex];
	}
}

bool IsFrameIndexed(FrameRef FramePtr)
{
	return TFB_DrawCanvas_IsPaletted(FramePtr->image->NormalImg);
}
