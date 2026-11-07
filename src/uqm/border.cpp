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


#include "colors.h"
#include "libs/gfxlib.h"
#include "menustat.h"
#include "setup.h"
#include "sis.h"
#include "units.h"
#include "util.h"


namespace uqm
{
void InitSISContexts(void)
{
	GfxRect r;

	SetContext(StatusContext);

	SetContext(SpaceContext);
	SetContextFGFrame(Screen);

	r.corner.x = SisOrgX();
	r.corner.y = SisOrgY();
	r.extent.width = SisScreenWidth();
	r.extent.height = SisScreenHeight();
	SetContextClipRect(&r);
}

void DrawSISFrame(void)
{
	GfxRect r;

	SetContext(ScreenContext);

	BatchGraphics();

	if (!IsHd())
	{
		SetContextForeGroundColor(MenuForegroundColor);

		// Top horizontal border behind SIS Title & Message (Small block in
		// between as well)
		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = SisOrgX() + SisScreenWidth() + 1;
		r.extent.height = SisOrgY() - 1;
		DrawFilledRectangle(&r);

		// Left side vertical border
		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = SisOrgX() - 1;
		r.extent.height = SisOrgY() + SisScreenHeight() + 1;
		DrawFilledRectangle(&r);

		// Bottom horizontal border
		r.corner.x = 0;
		r.corner.y = r.extent.height;
		r.extent.width = SisOrgX() + SisScreenWidth() + 1;
		r.extent.height = CanvasHeight - SisOrgY() + SisScreenHeight();
		DrawFilledRectangle(&r);

		// Right side vertical border and Stat/Menu
		r.corner.x = SisOrgX() + SisScreenWidth() + 1;
		r.corner.y = 0;
		r.extent.width = CanvasWidth - r.corner.x;
		r.extent.height = CanvasHeight;
		DrawFilledRectangle(&r);

		// Light and dark grey edges of the inner space window.
		r.corner.x = SisOrgX() - 1;
		r.corner.y = SisOrgY() - 1;
		r.extent.width = SisScreenWidth() + 2;
		r.extent.height = SisScreenHeight() + 2;
		DrawStarConBox(&r, 1,
					   SisLeftBorderColor,
					   SisBottomRightBorderColor,
					   true, BlackColor, false, Transparent);

		// The big Blue box in the upper edge of screen (SIS Message)
		r.corner.y = 0;
		r.extent.height = SisOrgY();
		r.corner.x = SisOrgX();
		r.extent.width = SisMessageBoxWidth();
		DrawStarConBox(&r, 1,
					   SisMessageTopLeftColor,
					   SisMessageBottomRightColor,
					   true, SisMessageBackgroundColor, false, Transparent);

		// The smaller blue box (SIS Title).
		r.extent.width = SisTitleBoxWidth();
		r.corner.x = SisOrgX() + SisScreenWidth() - SisTitleBoxWidth();
		DrawStarConBox(&r, 1,
					   SisTitleTopLeftColor,
					   SisTitleBottomRightColor,
					   true, SisTitleBackgroundColor, false, Transparent);

		// Black border between menu area and space window area
		SetContextForeGroundColor(BlackColor);
		r.corner.x = SafeX() + SpaceWidth() - 1;
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = CanvasHeight;
		DrawFilledRectangle(&r);

		// Bottom corners of the SIS gauges
		r.corner.x = SafeX() + SpaceWidth();
		r.corner.y = SafeY() + 139;
		DrawPoint(&r.corner);
		r.corner.x = CanvasWidth - 1;
		DrawPoint(&r.corner);

		// Light grey border on the left side of SIS Message
		SetContextForeGroundColor(SisLeftBorderColor);
		r.corner.y = 1;
		r.extent.width = 1;
		r.extent.height = SafeY() + SisTitleHeight();
		r.corner.x = SisOrgX() - 1;
		DrawFilledRectangle(&r);

		// Light grey border on the left side of SIS Title
		r.corner.x = SisOrgX() + SisScreenWidth() - SisTitleBoxWidth() - 1;
		DrawFilledRectangle(&r);

		// Light grey horizontal line at the bottom of the screen, space
		// window side
		r.corner.x = 0;
		r.corner.y = CanvasHeight - 1;
		r.extent.width = SafeX() + SpaceWidth() - 1;
		r.extent.height = 1;
		DrawFilledRectangle(&r);

		// Light grey vertical line at the right side of space window
		r.corner.x = SafeX() + SpaceWidth() - 2;
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = CanvasHeight - 1;
		DrawFilledRectangle(&r);

		// Vertical line at the right side of the Flagstats
		r.corner.x = CanvasWidth - 1;
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = SafeY() + 139;
		DrawFilledRectangle(&r);

		// Horizontal line below the in-game menu
		r.corner.x = SpaceWidth() + SafeX();
		r.corner.y = CanvasHeight - 1;
		r.extent.width = StatusWidth() + SafeX();
		r.extent.height = 1;
		DrawFilledRectangle(&r);

		// Vertical line at the right side in-game menu
		r.corner.x = CanvasWidth - 1;
		r.corner.y = SafeY() + 140;
		r.extent.width = 1;
		r.extent.height = (CanvasHeight - 1) - r.corner.y;
		DrawFilledRectangle(&r);

		// Dark grey border around blue boxes.
		SetContextForeGroundColor(SisBottomRightBorderColor);
		// Vertical line on the right side of SIS Message
		r.corner.y = 1;
		r.extent.width = 1;
		r.extent.height = SafeY() + SisMessageHeight() - DosNum(1);
		r.corner.x = SisOrgX() + SisMessageBoxWidth();
		DrawFilledRectangle(&r);

		// Vertical line on the right side of SIS Title
		r.corner.x = SisOrgX() + SisScreenWidth();
		++r.extent.height;
		DrawFilledRectangle(&r);

		//
		r.corner.y = 0;
		r.extent.width = (SafeX() + SpaceWidth() - 2) - r.corner.x;
		r.extent.height = 1;
		DrawFilledRectangle(&r);

		//
		r.corner.x = 0;
		r.extent.width = SisOrgX() - r.corner.x;
		DrawFilledRectangle(&r);

		// Horizontal line between boxes
		r.corner.x = SisOrgX() + SisMessageBoxWidth();
		r.extent.width = SisSpacerBoxWidth();
		DrawFilledRectangle(&r);

		//
		r.corner.x = 0;
		r.corner.y = 1;
		r.extent.width = 1;
		r.extent.height = (CanvasHeight - 1) - r.corner.y;
		DrawFilledRectangle(&r);

		// Dark verticle line accent for the top left of the right panel
		r.corner.x = SafeX() + SpaceWidth();
		r.corner.y = 0;
		r.extent.width = 1;
		r.extent.height = SafeY() + 139;
		DrawFilledRectangle(&r);

		// Horizontal line of the separator below the SIS gauges
		r.corner.x = SafeX() + SpaceWidth() + 1;
		r.corner.y = SafeY() + 139;
		r.extent.width = StatusWidth() - 2;
		r.extent.height = 1;
		DrawFilledRectangle(&r);

		// Dark verticle line to the left of in-game menu
		r.corner.x = SafeX() + SpaceWidth();
		r.corner.y = SafeY() + 140;
		r.extent.width = 1;
		r.extent.height = CanvasHeight - r.corner.y;
		DrawFilledRectangle(&r);
	}
	else
	{
		SetContextForeGroundColor(BlackColor);
		r.corner.x = SisOrgX() - ResScale(1);
		r.corner.y = SisOrgY() - ResScale(1);
		r.extent.width = SisScreenWidth() + ResScale(2);
		r.extent.height = SisScreenHeight() + ResScale(2);
		DrawFilledRectangle(&r);
	}

	DrawBorder(BorderFrameIndex::SisMain);

	InitSISContexts();
	ClearSISRect(ClearSisFlags::DrawDisplay);

	UnbatchGraphics();
}

} // namespace uqm