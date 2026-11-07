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
#include "libs/gfxlib.h"
#include "libs/graphics/drawable.h"


// Reads a piece of screen into a passed FrameRef or a newly created one
DrawableRef LoadDisplayPixmap(const GfxRect* area, FrameRef frame)
{
	// TODO: This should just return a FrameRef instead of DrawableRef
	DrawableRef buffer = GetFrameParentDrawable(frame);
	uint16_t index {};

	if (!buffer)
	{ // asked to create a new DrawableRef instead
		buffer = CreateDrawable(CreateFlags::WantPixmap | CreateFlags::MappedToDisplay,
								area->extent.width, area->extent.height, 1);
		if (!buffer)
		{
			return nullptr;
		}

		index = 0;
	}
	else
	{
		index = GetFrameIndex(frame);
	}

	frame = SetAbsFrameIndex(CaptureDrawable(buffer), index);

	if (_CurFramePtr->Type != ScreenDrawable
		|| frame->Type == ScreenDrawable
		|| !testFlag(GetFrameParentDrawable(frame)->Flags, CreateFlags::MappedToDisplay))
	{
		uqm::log::warn("Unimplemented function activated: "
					   "LoadDisplayPixmap()");
	}
	else
	{
		tfb::Image* img = frame->image;
		TFB_DrawScreen_CopyToImage(img, area, ScreenBuffer::Main);
	}

	ReleaseDrawable(frame);

	return buffer;
}
