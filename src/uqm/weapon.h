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

#ifndef UQM_WEAPON_H_
#define UQM_WEAPON_H_

#include "element.h"
#include "libs/gfxlib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

struct LASER_BLOCK
{
	int16_t cx, cy, ex, ey;
	ELEMENT_FLAGS flags;
	int16_t sender; // player number
	int16_t pixoffs;
	uint16_t face;
	Color color;
};

struct MISSILE_BLOCK
{
	int16_t cx, cy;
	ELEMENT_FLAGS flags;
	int16_t sender; // player number
	int16_t pixoffs, speed, hit_points, damage;
	uint16_t face, index, life;
	FRAME* farray;
	void (*preprocess_func)(ELEMENT* ElementPtr);
	int16_t blast_offs;
};

extern HELEMENT initialize_laser(LASER_BLOCK* pLaserBlock);
extern HELEMENT initialize_missile(MISSILE_BLOCK* pMissileBlock);
extern HELEMENT weapon_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
								 ELEMENT* ElementPtr1, GFXPOINT* pPt1);
extern int16_t TrackShip(ELEMENT* Tracker, uint16_t* pfacing);
extern void Untarget(ELEMENT* ElementPtr);

inline constexpr auto ModifyImage {(1 << 0)};
inline constexpr auto ModifySwap {(1 << 1)};

extern FRAME ModifySilhouette(ELEMENT* ElementPtr, STAMP* modify_stamp,
							  uint8_t modify_flags);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_WEAPON_H_ */
