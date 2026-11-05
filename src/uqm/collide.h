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

#ifndef UQM_COLLIDE_H_
#define UQM_COLLIDE_H_

#include "element.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

inline constexpr auto CollisionTurnWait {1};
inline constexpr auto CollisionThrustWait {3};

inline constexpr auto SkipCollision {(Nonsolid | Disappearing)};
constexpr auto CollidingElement(const auto& e) { return (!((e)->state_flags & SkipCollision)); }
constexpr auto CollisionPossible(const auto& e0, const auto& e1) { return (CollidingElement(e0) && (!(((e1)->state_flags & (e0)->state_flags) & Collision) && ((!(((e1)->state_flags & (e0)->state_flags) & IgnoreSimilar) || (e1)->pParent != (e0)->pParent)) && ((e1)->mass_points || (e0)->mass_points))); }

inline void InitIntersectStartPoint(ELEMENT* eptr)
{
	eptr->IntersectControl.IntersectStamp.origin.x = WorldToDisplay(eptr->current.location.x);
	eptr->IntersectControl.IntersectStamp.origin.y = WorldToDisplay(eptr->current.location.y);
}

inline void InitIntersectEndPoint(ELEMENT* eptr)
{
	eptr->IntersectControl.EndPoint.x = WorldToDisplay(eptr->next.location.x);
	eptr->IntersectControl.EndPoint.y = WorldToDisplay(eptr->next.location.y);
}

inline void InitIntersectFrame(ELEMENT* eptr)
{
	eptr->IntersectControl.IntersectStamp.frame = SetEquFrameIndex(eptr->next.image.farray[0], eptr->next.image.frame);
}

extern void collide(ELEMENT* ElementPtr0, ELEMENT* ElementPtr1);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_COLLIDE_H_ */
