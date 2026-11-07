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
#ifndef UQM_SHIP_H
#define UQM_SHIP_H

#include "core/misc/bittricks.h"
#include "races.h"
#include "element.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

extern bool GetNextStarShip(Starship* LastStarShipPtr, uint16_t which_side);
extern bool GetInitialStarShips(void);

extern void animation_preprocess(Element* ElementPtr);
extern void ship_preprocess(Element* ElementPtr);
extern void ship_postprocess(Element* ElementPtr);
extern void collision(Element* ElementPtr0, GfxPoint* pPt0,
					  Element* ElementPtr1, GfxPoint* pPt1);

extern StatusFlags inertial_thrust(Element* ElementPtr);

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_SHIP_H
