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

#pragma once
#ifndef UQM_WEAPON_H
#define UQM_WEAPON_H

#include "core/types/enum.h"
#include "element.h"
#include "libs/gfxlib.h"

namespace uqm
{
struct LaserDesc
{
	int16_t cx, cy, ex, ey;
	ElementFlags flags;
	int16_t sender; // player number
	int16_t pixoffs;
	uint16_t face;
	Color color;
};

struct MissileDesc
{
	int16_t cx, cy;
	ElementFlags flags;
	int16_t sender; // player number
	int16_t pixoffs, speed, hit_points, damage;
	uint16_t face, index, life;
	FrameRef* farray;
	void (*preprocess_func)(Element* ElementPtr);
	int16_t blast_offs;
};

extern ElementHandle initialize_laser(LaserDesc* pLaserBlock);
extern ElementHandle initialize_missile(MissileDesc* pMissileBlock);
extern ElementHandle weapon_collision(Element* ElementPtr0, GfxPoint* pPt0,
									  Element* ElementPtr1, GfxPoint* pPt1);
extern int16_t TrackShip(Element* Tracker, uint16_t* pfacing);
extern void Untarget(Element* ElementPtr);

enum class ModifyFlags : uint8_t
{
	None = 0,
	Image = 1 << 0,
	Swap = 1 << 1,
};
ENUM_BITWISE_OPS(ModifyFlags)

extern FrameRef ModifySilhouette(Element* ElementPtr, GfxStamp* modify_stamp,
								 ModifyFlags modify_flags);

} // namespace uqm

#endif // UQM_WEAPON_H
