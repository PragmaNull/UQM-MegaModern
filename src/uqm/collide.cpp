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


//#define DEBUG_COLLIDE

void collide(ELEMENT* ElementPtr0, ELEMENT* ElementPtr1)
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
	ImpactAngle1 = NORMALIZE_ANGLE(ImpactAngle0 + HALF_CIRCLE);

	GetCurrentVelocityComponents(&ElementPtr0->velocity, &dx0, &dy0);
	TravelAngle0 = GetVelocityTravelAngle(&ElementPtr0->velocity);
	GetCurrentVelocityComponents(&ElementPtr1->velocity, &dx1, &dy1);
	TravelAngle1 = GetVelocityTravelAngle(&ElementPtr1->velocity);
	dx_rel = dx0 - dx1;
	dy_rel = dy0 - dy1;
	RelTravelAngle = ARCTAN(dx_rel, dy_rel);
	speed = square_root((long)dx_rel * dx_rel + (long)dy_rel * dy_rel);

	Directness = NORMALIZE_ANGLE(RelTravelAngle - ImpactAngle0);
	if (Directness <= QUADRANT || Directness >= HALF_CIRCLE + QUADRANT)
	/* shapes just scraped each other but still collided,
			 * they will collide again unless we fudge it.
			 */
	{
		Directness = HALF_CIRCLE;
		ImpactAngle0 = TravelAngle0 + HALF_CIRCLE;
		ImpactAngle1 = TravelAngle1 + HALF_CIRCLE;
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
		if (ElementPtr0->state_flags & ElementPtr1->state_flags & DEFY_PHYSICS)
		{
			ImpactAngle0 = TravelAngle0 + (HALF_CIRCLE - OCTANT);
			ImpactAngle1 = TravelAngle1 + (HALF_CIRCLE - OCTANT);
			ZeroVelocityComponents(&ElementPtr0->velocity);
			ZeroVelocityComponents(&ElementPtr1->velocity);
		}
		ElementPtr0->state_flags |= (DEFY_PHYSICS | COLLISION);
		ElementPtr1->state_flags |= (DEFY_PHYSICS | COLLISION);
#ifdef DEBUG_COLLIDE
		uqm::log::debug("No movement before collision -- "
						"<({}, {}) = {}, ({}, {}) = {}>",
						dx0, dy0, ImpactAngle0 - OCTANT, dx1, dy1,
						ImpactAngle1 - OCTANT);
#endif /* DEBUG_COLLIDE */
	}

	{
		int16_t mass0 {};
		int16_t mass1 {};
		long scalar {};

		mass0 = ElementPtr0->mass_points /* << 2 */;
		mass1 = ElementPtr1->mass_points /* << 2 */;
		scalar = (long)SINE(Directness, speed << 1) * (mass0 * mass1);

		if (!GRAVITY_MASS(ElementPtr0->mass_points + 1))
		{
			if (ElementPtr0->state_flags & PLAYER_SHIP)
			{
				STARSHIP* StarShipPtr {};

				GetElementStarShip(ElementPtr0, &StarShipPtr);
				StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
				if (!(ElementPtr0->state_flags & DEFY_PHYSICS))
				{
					if (ElementPtr0->turn_wait < COLLISION_TURN_WAIT)
					{
						ElementPtr0->turn_wait += COLLISION_TURN_WAIT;
					}
					if (ElementPtr0->thrust_wait < COLLISION_THRUST_WAIT)
					{
						ElementPtr0->thrust_wait += COLLISION_THRUST_WAIT;
					}
				}
			}

			speed = (int16_t)(scalar / ((long)mass0 * (mass0 + mass1)));
			DeltaVelocityComponents(&ElementPtr0->velocity,
									COSINE(ImpactAngle0, speed),
									SINE(ImpactAngle0, speed));

			GetCurrentVelocityComponents(&ElementPtr0->velocity, &dx0, &dy0);
			if (dx0 < 0)
			{
				dx0 = -dx0;
			}
			if (dy0 < 0)
			{
				dy0 = -dy0;
			}

			if (VELOCITY_TO_WORLD(dx0 + dy0) < SCALED_ONE)
			{
				SetVelocityComponents(&ElementPtr0->velocity,
									  COSINE(ImpactAngle0,
											 WORLD_TO_VELOCITY(SCALED_ONE) - 1),
									  SINE(ImpactAngle0,
										   WORLD_TO_VELOCITY(SCALED_ONE) - 1));
			}
		}

		if (!GRAVITY_MASS(ElementPtr1->mass_points + 1))
		{
			if (ElementPtr1->state_flags & PLAYER_SHIP)
			{
				STARSHIP* StarShipPtr {};

				GetElementStarShip(ElementPtr1, &StarShipPtr);
				StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
				if (!(ElementPtr1->state_flags & DEFY_PHYSICS))
				{
					if (ElementPtr1->turn_wait < COLLISION_TURN_WAIT)
					{
						ElementPtr1->turn_wait += COLLISION_TURN_WAIT;
					}
					if (ElementPtr1->thrust_wait < COLLISION_THRUST_WAIT)
					{
						ElementPtr1->thrust_wait += COLLISION_THRUST_WAIT;
					}
				}
			}

			speed = (int16_t)(scalar / ((long)mass1 * (mass0 + mass1)));
			DeltaVelocityComponents(&ElementPtr1->velocity,
									COSINE(ImpactAngle1, speed),
									SINE(ImpactAngle1, speed));

			GetCurrentVelocityComponents(&ElementPtr1->velocity, &dx1, &dy1);
			if (dx1 < 0)
			{
				dx1 = -dx1;
			}
			if (dy1 < 0)
			{
				dy1 = -dy1;
			}

			if (VELOCITY_TO_WORLD(dx1 + dy1) < SCALED_ONE)
			{
				SetVelocityComponents(&ElementPtr1->velocity,
									  COSINE(ImpactAngle1,
											 WORLD_TO_VELOCITY(SCALED_ONE) - 1),
									  SINE(ImpactAngle1,
										   WORLD_TO_VELOCITY(SCALED_ONE) - 1));
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
