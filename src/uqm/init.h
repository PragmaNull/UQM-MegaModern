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

#pragma once
#ifndef UQM_INIT_H
#define UQM_INIT_H

#include "libs/gfxlib.h"
#include "libs/reslib.h"
#include "units.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

inline constexpr auto NumPlayers {2};
inline constexpr auto NumSides {2};

extern FRAME stars_in_space;
extern FRAME stars_misc[NumViews];
extern FRAME scenery;

extern bool InitSpace(void);
extern void UninitSpace(void);

extern int16_t InitShips(void);
extern void UninitShips(void);
extern void ReloadGameContent(void);

extern bool load_animation(FRAME* pixarray, RESOURCE big_res,
						   RESOURCE med_res, RESOURCE sml_res);
extern bool free_image(FRAME* pixarray);

#if 0 //defined(__cplusplus)
}
#endif

#endif // UQM_INIT_H
