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

#include "options.h"
#include "collide.h"
#include "races.h"
#include "units.h"
#include "core/log/log.h"

//#define DEBUG_GRAVITY

bool CalculateGravity(ELEMENT* ElementPtr)
{
	bool HasGravity {};
	HELEMENT hTestElement {};
	HELEMENT hSuccElement {};

	bool retval {false};
	HasGravity = (bool)(CollidingElement(ElementPtr)
						&& GRAVITY_MASS(ElementPtr->mass_points + 1));
	for (hTestElement = GetHeadElement();
		 hTestElement != 0; hTestElement = hSuccElement)
	{
		bool TestHasGravity {};
		ELEMENT* TestElementPtr {};

		LockElement(hTestElement, &TestElementPtr);
		if (TestElementPtr != ElementPtr
			&& CollidingElement(TestElementPtr)
			&& (TestHasGravity =
					GRAVITY_MASS(TestElementPtr->mass_points + 1))
				   != HasGravity)
		{
			uint16_t abs_dx {};
			uint16_t abs_dy {};
			int16_t dx {};
			int16_t dy {};

			if (!(ElementPtr->state_flags & PreProcessFlag))
			{
				dx = ElementPtr->current.location.x
				   - TestElementPtr->current.location.x;
				dy = ElementPtr->current.location.y
				   - TestElementPtr->current.location.y;
			}
			else
			{
				dx = ElementPtr->next.location.x
				   - TestElementPtr->next.location.x;
				dy = ElementPtr->next.location.y
				   - TestElementPtr->next.location.y;
			}
#ifdef DEBUG_GRAVITY
			if (TestElementPtr->state_flags & PlayerShip)
			{
				uqm::log::debug("CalculateGravity:");
				uqm::log::debug("\tdx = {}, dy = {}", dx, dy);
			}
#endif /* DEBUG_GRAVITY */
			dx = WRAP_DELTA_X(dx);
			dy = WRAP_DELTA_Y(dy);
#ifdef DEBUG_GRAVITY
			if (TestElementPtr->state_flags & PlayerShip)
			{
				uqm::log::debug("\twrap_dx = {}, wrap_dy = {}", dx, dy);
			}
#endif /* DEBUG_GRAVITY */
			abs_dx = dx >= 0 ? dx : -dx;
			abs_dy = dy >= 0 ? dy : -dy;
			abs_dx = WORLD_TO_DISPLAY(abs_dx);
			abs_dy = WORLD_TO_DISPLAY(abs_dy);
#ifdef DEBUG_GRAVITY
			if (TestElementPtr->state_flags & PlayerShip)
			{
				uqm::log::debug("\tdisplay_dx = {}, display_dy = {}",
								abs_dx, abs_dy);
			}
#endif /* DEBUG_GRAVITY */
			if (abs_dx <= GravityThreshold()
				&& abs_dy <= GravityThreshold())
			{
				uint32_t dist_squared {};

				dist_squared = (uint32_t)(abs_dx * abs_dx)
							 + (uint32_t)(abs_dy * abs_dy);
				if (dist_squared <= (uint32_t)(GravityThreshold()
											   * GravityThreshold()))
				{
#ifdef NEVER
					uint16_t magnitude;

#define DIFUSE_GRAVITY RES_SCALE(175) // JMS_GFX: Because of the ifdef NEVER this is actually never run. Well, changed it for consistency
					dist_squared += (uint32_t)abs_dx * (DIFUSE_GRAVITY << 1)
								  + (uint32_t)abs_dy * (DIFUSE_GRAVITY << 1)
								  + ((uint32_t)(DIFUSE_GRAVITY * DIFUSE_GRAVITY) << 1);
					if ((magnitude = (uint16_t)((uint32_t)(GravityThreshold()
														   * GravityThreshold())
												/ dist_squared))
						== 0)
					{
						magnitude = 1;
					}

#define MAX_MAGNITUDE RES_SCALE(6) // JMS_GFX: Because of the ifdef NEVER this is actually never run. Well, changed it for consistency
					else if (magnitude > MAX_MAGNITUDE)
					{
						magnitude = MAX_MAGNITUDE;
					}
					uqm::log::debug("magnitude = {}", magnitude);
#endif /* NEVER */

#ifdef DEBUG_GRAVITY
					if (TestElementPtr->state_flags & PlayerShip)
					{
						uqm::log::debug("dist_squared = {}", dist_squared);
					}
#endif /* DEBUG_GRAVITY */
					if (TestHasGravity)
					{
						retval = true;
						UnlockElement(hTestElement);
						break;
					}
					else
					{
						const uint16_t angle {ARCTAN(dx, dy)};
						DeltaVelocityComponents(&TestElementPtr->velocity,
												COSINE(angle, WORLD_TO_VELOCITY(RES_SCALE(1))),
												SINE(angle, WORLD_TO_VELOCITY(RES_SCALE(1))));
						if (TestElementPtr->state_flags & PlayerShip)
						{
							STARSHIP* StarShipPtr {};

							GetElementStarShip(TestElementPtr, &StarShipPtr);
							StarShipPtr->cur_status_flags &= ~ShipAtMaxSpeed;
							StarShipPtr->cur_status_flags |= ShipInGravityWell;
						}
					}
				}
			}
		}

		hSuccElement = GetSuccElement(TestElementPtr);
		UnlockElement(hTestElement);
	}

	return (retval);
}

bool TimeSpaceMatterConflict(ELEMENT* ElementPtr)
{
	HELEMENT hTestElement {};
	HELEMENT hSuccElement {};
	INTERSECT_CONTROL ElementControl;

	ElementControl.IntersectStamp.origin.x =
		WORLD_TO_DISPLAY(ElementPtr->current.location.x);
	ElementControl.IntersectStamp.origin.y =
		WORLD_TO_DISPLAY(ElementPtr->current.location.y);
	ElementControl.IntersectStamp.frame =
		SetEquFrameIndex(ElementPtr->current.image.farray[0],
						 ElementPtr->current.image.frame);
	ElementControl.EndPoint = ElementControl.IntersectStamp.origin;
	for (hTestElement = GetHeadElement();
		 hTestElement != 0; hTestElement = hSuccElement)
	{
		ELEMENT* TestElementPtr {};

		LockElement(hTestElement, &TestElementPtr);
		hSuccElement = GetSuccElement(TestElementPtr);
		if (TestElementPtr != ElementPtr
			&& (CollidingElement(TestElementPtr)
				/* ship in transition */
				|| (TestElementPtr->state_flags & PlayerShip)))
		{
			INTERSECT_CONTROL TestElementControl;

			TestElementControl.IntersectStamp.origin.x =
				WORLD_TO_DISPLAY(TestElementPtr->current.location.x);
			TestElementControl.IntersectStamp.origin.y =
				WORLD_TO_DISPLAY(TestElementPtr->current.location.y);
			TestElementControl.IntersectStamp.frame =
				SetEquFrameIndex(TestElementPtr->current.image.farray[0],
								 TestElementPtr->current.image.frame);
			TestElementControl.EndPoint = TestElementControl.IntersectStamp.origin;
			if (DrawablesIntersect(&ElementControl,
								   &TestElementControl, MaxTimeValue))
			{
				UnlockElement(hTestElement);

				break;
			}
		}
		UnlockElement(hTestElement);
	}

	return (hTestElement != 0 ? true : false);
}
