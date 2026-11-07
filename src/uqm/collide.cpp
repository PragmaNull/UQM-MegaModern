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

#include "collide.h"
#include "element.h"
#include "races.h"
#include "units.h"
#include "libs/mathlib.h"
#include "core/log/log.h"


namespace uqm
{
//#define DEBUG_COLLIDE

void collide(Element* ElementPtr0, Element* ElementPtr1)
{
	int16_t speed {};
	int16_t dx0 {};
	int16_t dy0 {};
	int16_t dx1 {};
	int16_t dy1 {};
	int16_t dx_rel {};
	int16_t dy_rel {};
	int16_t TravelAngle0 {};
	int16_t TravelAngle1 {};
	int16_t ImpactAngle0 {};
	int16_t ImpactAngle1 {};
	int16_t RelTravelAngle {};
	int16_t Directness {};

	dx_rel = ElementPtr0->next.location.x
		   - ElementPtr1->next.location.x;
	dy_rel = ElementPtr0->next.location.y
		   - ElementPtr1->next.location.y;
	ImpactAngle0 = ARCTAN(dx_rel, dy_rel);
	ImpactAngle1 = NormalizeAngle(ImpactAngle0 + HalfCircle);

	GetCurrentVelocityComponents(&ElementPtr0->velocity, &dx0, &dy0);
	TravelAngle0 = GetVelocityTravelAngle(&ElementPtr0->velocity);
	GetCurrentVelocityComponents(&ElementPtr1->velocity, &dx1, &dy1);
	TravelAngle1 = GetVelocityTravelAngle(&ElementPtr1->velocity);
	dx_rel = dx0 - dx1;
	dy_rel = dy0 - dy1;
	RelTravelAngle = ARCTAN(dx_rel, dy_rel);
	speed = square_root((long)dx_rel * dx_rel + (long)dy_rel * dy_rel);

	Directness = NormalizeAngle(RelTravelAngle - ImpactAngle0);
	if (Directness <= Quadrant || Directness >= HalfCircle + Quadrant)
	/* shapes just scraped each other but still collided,
			 * they will collide again unless we fudge it.
			 */
	{
		Directness = HalfCircle;
		ImpactAngle0 = TravelAngle0 + HalfCircle;
		ImpactAngle1 = TravelAngle1 + HalfCircle;
	}

#ifdef DEBUG_COLLIDE
	uqm::log::debug("Centers: <{}, {}> <{}, {}>",
					ElementPtr0->next.location.x, ElementPtr0->next.location.y,
					ElementPtr1->next.location.x, ElementPtr1->next.location.y);
	uqm::log::debug("RelTravelAngle : {}, ImpactAngles <{}, {}>",
					RelTravelAngle, ImpactAngle0, ImpactAngle1);
#endif /* DEBUG_COLLIDE */

	if (ElementPtr0->next.location.x == ElementPtr0->current.location.x
		&& ElementPtr0->next.location.y == ElementPtr0->current.location.y
		&& ElementPtr1->next.location.x == ElementPtr1->current.location.x
		&& ElementPtr1->next.location.y == ElementPtr1->current.location.y)
	{
		if (ElementPtr0->state_flags & ElementPtr1->state_flags & DefyPhysics)
		{
			ImpactAngle0 = TravelAngle0 + (HalfCircle - Octant);
			ImpactAngle1 = TravelAngle1 + (HalfCircle - Octant);
			ZeroVelocityComponents(&ElementPtr0->velocity);
			ZeroVelocityComponents(&ElementPtr1->velocity);
		}
		ElementPtr0->state_flags |= (DefyPhysics | Collision);
		ElementPtr1->state_flags |= (DefyPhysics | Collision);
#ifdef DEBUG_COLLIDE
		uqm::log::debug("No movement before collision -- "
						"<({}, {}) = {}, ({}, {}) = {}>",
						dx0, dy0, ImpactAngle0 - Octant, dx1, dy1,
						ImpactAngle1 - Octant);
#endif /* DEBUG_COLLIDE */
	}

	{
		int16_t mass0 {};
		int16_t mass1 {};
		long scalar {};

		mass0 = ElementPtr0->mass_points /* << 2 */;
		mass1 = ElementPtr1->mass_points /* << 2 */;
		scalar = (long)Sine(Directness, speed << 1) * (mass0 * mass1);

		if (!GravityMass(ElementPtr0->mass_points + 1))
		{
			if (ElementPtr0->state_flags & PlayerShip)
			{
				Starship* StarShipPtr {};

				GetElementStarShip(ElementPtr0, &StarShipPtr);
				StarShipPtr->cur_status_flags &=
					~(ShipAtMaxSpeed | ShipBeyondMaxSpeed);
				if (!(ElementPtr0->state_flags & DefyPhysics))
				{
					if (ElementPtr0->turn_wait < CollisionTurnWait)
					{
						ElementPtr0->turn_wait += CollisionTurnWait;
					}
					if (ElementPtr0->thrust_wait < CollisionThrustWait)
					{
						ElementPtr0->thrust_wait += CollisionThrustWait;
					}
				}
			}

			speed = (int16_t)(scalar / ((long)mass0 * (mass0 + mass1)));
			DeltaVelocityComponents(&ElementPtr0->velocity,
									Cosine(ImpactAngle0, speed),
									Sine(ImpactAngle0, speed));

			GetCurrentVelocityComponents(&ElementPtr0->velocity, &dx0, &dy0);
			if (dx0 < 0)
			{
				dx0 = -dx0;
			}
			if (dy0 < 0)
			{
				dy0 = -dy0;
			}

			if (VelocityToWorld(dx0 + dy0) < ScaledOne)
			{
				SetVelocityComponents(&ElementPtr0->velocity,
									  Cosine(ImpactAngle0,
											 WorldToVelocity(ScaledOne) - 1),
									  Sine(ImpactAngle0,
										   WorldToVelocity(ScaledOne) - 1));
			}
		}

		if (!GravityMass(ElementPtr1->mass_points + 1))
		{
			if (ElementPtr1->state_flags & PlayerShip)
			{
				Starship* StarShipPtr {};

				GetElementStarShip(ElementPtr1, &StarShipPtr);
				StarShipPtr->cur_status_flags &=
					~(ShipAtMaxSpeed | ShipBeyondMaxSpeed);
				if (!(ElementPtr1->state_flags & DefyPhysics))
				{
					if (ElementPtr1->turn_wait < CollisionTurnWait)
					{
						ElementPtr1->turn_wait += CollisionTurnWait;
					}
					if (ElementPtr1->thrust_wait < CollisionThrustWait)
					{
						ElementPtr1->thrust_wait += CollisionThrustWait;
					}
				}
			}

			speed = (int16_t)(scalar / ((long)mass1 * (mass0 + mass1)));
			DeltaVelocityComponents(&ElementPtr1->velocity,
									Cosine(ImpactAngle1, speed),
									Sine(ImpactAngle1, speed));

			GetCurrentVelocityComponents(&ElementPtr1->velocity, &dx1, &dy1);
			if (dx1 < 0)
			{
				dx1 = -dx1;
			}
			if (dy1 < 0)
			{
				dy1 = -dy1;
			}

			if (VelocityToWorld(dx1 + dy1) < ScaledOne)
			{
				SetVelocityComponents(&ElementPtr1->velocity,
									  Cosine(ImpactAngle1,
											 WorldToVelocity(ScaledOne) - 1),
									  Sine(ImpactAngle1,
										   WorldToVelocity(ScaledOne) - 1));
			}
		}
#ifdef DEBUG_COLLIDE
		GetCurrentVelocityComponents(&ElementPtr0->velocity, &dx0, &dy0);
		GetCurrentVelocityComponents(&ElementPtr1->velocity, &dx1, &dy1);
		uqm::log::debug("After: <{}, {}> <{}, {}>\n",
						dx0, dy0, dx1, dy1);
#endif /* DEBUG_COLLIDE */
	}
}

} // namespace uqm
