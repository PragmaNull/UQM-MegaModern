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

namespace uqm
{
//#define DEBUG_GRAVITY

static auto DifuseGravity() { return ResScale(175); } // JMS_GFX: Because of the ifdef NEVER this is actually never run. Well, changed it for consistency
static auto MaxMagnitude() { return ResScale(6); }	  // JMS_GFX: Because of the ifdef NEVER this is actually never run. Well, changed it for consistency

bool CalculateGravity(Element* ElementPtr)
{
	bool HasGravity {};
	ElementHandle hTestElement {};
	ElementHandle hSuccElement {};

	bool retval {false};
	HasGravity = (bool)(CollidingElement(ElementPtr)
						&& GravityMass(ElementPtr->mass_points + 1));
	for (hTestElement = GetHeadElement();
		 hTestElement != 0; hTestElement = hSuccElement)
	{
		bool TestHasGravity {};
		Element* TestElementPtr {};

		LockElement(hTestElement, &TestElementPtr);
		if (TestElementPtr != ElementPtr
			&& CollidingElement(TestElementPtr)
			&& (TestHasGravity =
					GravityMass(TestElementPtr->mass_points + 1))
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
			dx = WrapDeltaX(dx);
			dy = WrapDeltaY(dy);
#ifdef DEBUG_GRAVITY
			if (TestElementPtr->state_flags & PlayerShip)
			{
				uqm::log::debug("\twrap_dx = {}, wrap_dy = {}", dx, dy);
			}
#endif /* DEBUG_GRAVITY */
			abs_dx = dx >= 0 ? dx : -dx;
			abs_dy = dy >= 0 ? dy : -dy;
			abs_dx = WorldToDisplay(abs_dx);
			abs_dy = WorldToDisplay(abs_dy);
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

					dist_squared += (uint32_t)abs_dx * (DifuseGravity() << 1)
								  + (uint32_t)abs_dy * (DifuseGravity() << 1)
								  + ((uint32_t)(DifuseGravity() * DifuseGravity()) << 1);
					if ((magnitude = (uint16_t)((uint32_t)(GravityThreshold()
														   * GravityThreshold())
												/ dist_squared))
						== 0)
					{
						magnitude = 1;
					}

					else if (magnitude > MaxMagnitude())
					{
						magnitude = MaxMagnitude();
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
												Cosine(angle, WorldToVelocity(ResScale(1))),
												Sine(angle, WorldToVelocity(ResScale(1))));
						if (TestElementPtr->state_flags & PlayerShip)
						{
							Starship* StarShipPtr {};

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

bool TimeSpaceMatterConflict(Element* ElementPtr)
{
	ElementHandle hTestElement {};
	ElementHandle hSuccElement {};
	IntersectInfo ElementControl;

	ElementControl.IntersectStamp.origin.x =
		WorldToDisplay(ElementPtr->current.location.x);
	ElementControl.IntersectStamp.origin.y =
		WorldToDisplay(ElementPtr->current.location.y);
	ElementControl.IntersectStamp.frame =
		SetEquFrameIndex(ElementPtr->current.image.farray[0],
						 ElementPtr->current.image.frame);
	ElementControl.EndPoint = ElementControl.IntersectStamp.origin;
	for (hTestElement = GetHeadElement();
		 hTestElement != 0; hTestElement = hSuccElement)
	{
		Element* TestElementPtr {};

		LockElement(hTestElement, &TestElementPtr);
		hSuccElement = GetSuccElement(TestElementPtr);
		if (TestElementPtr != ElementPtr
			&& (CollidingElement(TestElementPtr)
				/* ship in transition */
				|| (TestElementPtr->state_flags & PlayerShip)))
		{
			IntersectInfo TestElementControl;

			TestElementControl.IntersectStamp.origin.x =
				WorldToDisplay(TestElementPtr->current.location.x);
			TestElementControl.IntersectStamp.origin.y =
				WorldToDisplay(TestElementPtr->current.location.y);
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

} // namespace uqm
