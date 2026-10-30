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

#ifndef UQM_SHIP_H_INCL_
#define UQM_SHIP_H_INCL_

#include "core/types/bytes.h"
#include "races.h"
#include "element.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

extern bool GetNextStarShip(STARSHIP* LastStarShipPtr, uint16_t which_side);
extern bool GetInitialStarShips(void);

extern void animation_preprocess(ELEMENT* ElementPtr);
extern void ship_preprocess(ELEMENT* ElementPtr);
extern void ship_postprocess(ELEMENT* ElementPtr);
extern void collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
					  ELEMENT* ElementPtr1, GFXPOINT* pPt1);

extern STATUS_FLAGS inertial_thrust(ELEMENT* ElementPtr);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_SHIP_H_INCL_ */
