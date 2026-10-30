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

#ifndef UQM_UTIL_H_
#define UQM_UTIL_H_

#include "core/stl/stl.h"
#include "core/types/bytes.h"
#include "libs/gfxlib.h"
#include "setupmenu.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

extern void DrawStarConBox(GFXRECT* pRect, int16_t BorderWidth,
						   Color TopLeftColor, Color BottomRightColor, bool FillInterior,
						   Color InteriorColor, bool CreateCorners, Color CornerColor);
extern void DrawBorderPadding(uint32_t videoWidth);
extern void DrawRadarBorder(void);
extern uint32_t SeedRandomNumbers(void);

extern void DrawRenderedBox(GFXRECT* r, bool filled, Color fill_color,
							int type, int custom);

// saveRect can be nullptr to save the entire context frame
extern STAMP SaveContextFrame(const GFXRECT* saveRect);

extern uint32_t get_fuel_to_sol(void);
extern void DrawFlagStatDisplay(const char* str);
extern char* WholeFuelValue(void);

void formatFuelValue(uint32_t coarseFuel, uqstl::span<char> buf);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_UTIL_H_ */
