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

#include "colors.h"
#include "collide.h"
#include "element.h"
#include "ship.h"
#include "globdata.h"
#include "intel.h"
#include "setup.h"
#include "units.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
#include "uqm/random/UQMRandom.h"

//#define DEBUG_CYBORG

uint16_t
PlotIntercept(ELEMENT* ElementPtr0, ELEMENT* ElementPtr1,
			  uint16_t max_turns, uint16_t margin_of_error)
{
	int32_t time_y_0 {};
	int32_t time_y_1 {};
	DPOINT dst[2];
	GFXRECT r0 = {
		{0, 0},
		{0, 0}
	  };
	GFXRECT r1 = {
		{0, 0},
		{0, 0}
	  };
	int32_t dx_0 {};
	int32_t dy_0 {};
	int32_t dx_1 {};
	int32_t dy_1 {}; // JMS:These were int16_t. No overflows now.

	if ((ElementPtr0->state_flags | ElementPtr1->state_flags) & FiniteLife)
	{
		//log_add (log_Debug, "E0:{}, E1:{}, max:{}",ElementPtr0->life_span,ElementPtr1->life_span, max_turns);

		if (!(ElementPtr0->state_flags & FiniteLife))
		{
			if (ElementPtr1->life_span < max_turns)
			{
				max_turns = ElementPtr1->life_span;
			}
		}
		else if (!(ElementPtr1->state_flags & FiniteLife))
		{
			if (ElementPtr0->life_span < max_turns)
			{
				max_turns = ElementPtr0->life_span;
			}
		}
		else
		{
			if (ElementPtr0->life_span < max_turns)
			{
				max_turns = ElementPtr0->life_span;
			}
			if (ElementPtr1->life_span < max_turns)
			{
				max_turns = ElementPtr1->life_span;
			}
		}
	}

	dst[0].x = (int32_t)ElementPtr0->current.location.x;
	dst[0].y = (int32_t)ElementPtr0->current.location.y;
	GetCurrentVelocityComponentsSdword(&ElementPtr0->velocity, &dx_0, &dy_0);
	dx_0 = (int32_t)VelocityToWorld((long)dx_0 * (long)max_turns);
	dy_0 = (int32_t)VelocityToWorld((long)dy_0 * (long)max_turns);

	dst[1].x = (int32_t)ElementPtr1->current.location.x;
	dst[1].y = (int32_t)ElementPtr1->current.location.y;
	GetCurrentVelocityComponentsSdword(&ElementPtr1->velocity, &dx_1, &dy_1);
	dx_1 = (int32_t)VelocityToWorld((long)dx_1 * (long)max_turns);
	dy_1 = (int32_t)VelocityToWorld((long)dy_1 * (long)max_turns);

	if (margin_of_error)
	{
		dst[1].y -= margin_of_error;
		time_y_0 = 1;
		time_y_1 = margin_of_error << 1;
	}
	else
	{
		GetFrameRect(ElementPtr0->IntersectControl.IntersectStamp.frame, &r0);
		GetFrameRect(ElementPtr1->IntersectControl.IntersectStamp.frame, &r1);

		dst[0].y += DisplayToWorld(r0.corner.y);
		dst[1].y += DisplayToWorld(r1.corner.y);
		time_y_0 = DisplayToWorld(r0.extent.height);
		time_y_1 = DisplayToWorld(r1.extent.height);
	}

	int32_t dy {dst[1].y - dst[0].y};
	time_y_0 = dy - time_y_0 + 1;
	time_y_1 = dy + time_y_1 - 1;
	dy = dy_0 - dy_1;

	if ((time_y_0 <= 0 && time_y_1 >= 0)
		|| (time_y_0 > 0 && dy >= time_y_0)
		|| (time_y_1 < 0 && dy <= time_y_1))
	{
		int32_t time_x_0 {};
		int32_t time_x_1 {};

		if (margin_of_error)
		{
			dst[1].x -= margin_of_error;
			time_x_0 = 1;
			time_x_1 = margin_of_error << 1;
		}
		else
		{
			dst[0].x += DisplayToWorld(r0.corner.x);
			dst[1].x += DisplayToWorld(r1.corner.x);
			time_x_0 = DisplayToWorld(r0.extent.width);
			time_x_1 = DisplayToWorld(r1.extent.width);
		}

		int32_t dx {dst[1].x - dst[0].x};
		time_x_0 = dx - time_x_0 + 1;
		time_x_1 = dx + time_x_1 - 1;
		dx = dx_0 - dx_1;

		if ((time_x_0 <= 0 && time_x_1 >= 0)
			|| (time_x_0 > 0 && dx >= time_x_0)
			|| (time_x_1 < 0 && dx <= time_x_1))
		{
			if (dx == 0 && dy == 0)
			{
				time_y_0 = time_y_1 = 0;
			}
			else
			{
				int32_t t {};
				long time_beg {};
				long time_end {};
				long fract {};

				if (time_y_1 < 0)
				{
					t = time_y_0;
					time_y_0 = -time_y_1;
					time_y_1 = -t;
				}
				else if (time_y_0 <= 0)
				{
					if (dy < 0)
					{
						time_y_1 = -time_y_0;
					}
					time_y_0 = 0;
				}
				if (dy < 0)
				{
					dy = -dy;
				}
				if (dy < time_y_1)
				{
					time_y_1 = dy;
				}

				if (time_x_1 < 0)
				{
					t = time_x_0;
					time_x_0 = -time_x_1;
					time_x_1 = -t;
				}
				else if (time_x_0 <= 0)
				{
					if (dx < 0)
					{
						time_x_1 = -time_x_0;
					}
					time_x_0 = 0;
				}
				if (dx < 0)
				{
					dx = -dx;
				}
				if (dx < time_x_1)
				{
					time_x_1 = dx;
				}

				if (dx == 0)
				{
					time_beg = time_y_0;
					time_end = time_y_1;
					fract = dy;
				}
				else if (dy == 0)
				{
					time_beg = time_x_0;
					time_end = time_x_1;
					fract = dx;
				}
				else
				{
					long time_x {(long)time_x_0 * (long)dy};
					long time_y {(long)time_y_0 * (long)dx};
					time_beg = time_x < time_y ? time_y : time_x;

					time_x = (long)time_x_1 * (long)dy;
					time_y = (long)time_y_1 * (long)dx;
					time_end = time_x > time_y ? time_y : time_x;

					fract = (long)dx * (long)dy;
				}

				if ((time_beg *= max_turns) < fract)
				{
					time_y_0 = 0;
				}
				else
				{
					time_y_0 = (int32_t)(time_beg / fract);
				}

				if (time_end >= fract) /* just in case of overflow */
				{
					time_y_1 = max_turns - 1;
				}
				else
				{
					time_y_1 = (int32_t)((time_end * max_turns) / fract);
				}
			}

			if (time_y_0 <= time_y_1)
			{
				if (margin_of_error != 0)
				{
					return ((uint16_t)time_y_0 + 1);
				}
				else
				{
					DPOINT Pt0;
					DPOINT Pt1;
					VELOCITY_DESC Velocity0;
					VELOCITY_DESC Velocity1;
					INTERSECT_CONTROL Control0;
					INTERSECT_CONTROL Control1;

					Pt0.x = (int32_t)ElementPtr0->current.location.x;
					Pt0.y = (int32_t)ElementPtr0->current.location.y;
					Velocity0 = ElementPtr0->velocity;
					Control0 = ElementPtr0->IntersectControl;

					Pt1.x = (int32_t)ElementPtr1->current.location.x;
					Pt1.y = (int32_t)ElementPtr1->current.location.y;
					Velocity1 = ElementPtr1->velocity;
					Control1 = ElementPtr1->IntersectControl;

					if (time_y_0)
					{
						GetNextVelocityComponentsSdword(&Velocity0, &dx_0, &dy_0, time_y_0);
						Pt0.x += dx_0;
						Pt0.y += dy_0;
						Control0.EndPoint.x = WorldToDisplay(Pt0.x);
						Control0.EndPoint.y = WorldToDisplay(Pt0.y);

						GetNextVelocityComponentsSdword(&Velocity1, &dx_1, &dy_1, time_y_0);
						Pt1.x += dx_1;
						Pt1.y += dy_1;
						Control1.EndPoint.x = WorldToDisplay(Pt1.x);
						Control1.EndPoint.y = WorldToDisplay(Pt1.y);
					}

					do
					{
						TIME_VALUE when {};

						++time_y_0;

						GetNextVelocityComponentsSdword(&Velocity0, &dx_0, &dy_0, 1);
						Pt0.x += dx_0;
						Pt0.y += dy_0;

						GetNextVelocityComponentsSdword(&Velocity1, &dx_1, &dy_1, 1);
						Pt1.x += dx_1;
						Pt1.y += dy_1;

						Control0.IntersectStamp.origin = Control0.EndPoint;
						Control0.EndPoint.x = WorldToDisplay(Pt0.x);
						Control0.EndPoint.y = WorldToDisplay(Pt0.y);

						Control1.IntersectStamp.origin = Control1.EndPoint;
						Control1.EndPoint.x = WorldToDisplay(Pt1.x);
						Control1.EndPoint.y = WorldToDisplay(Pt1.y);
						when = DrawablesIntersect(&Control0,
												  &Control1, MaxTimeValue);
						if (when)
						{
							if (when == 1
								&& time_y_0 == 1
								&& ((ElementPtr0->state_flags
									 | ElementPtr1->state_flags)
									& Appearing))
							{
								when = 0;
								Control0.EndPoint.x = WorldToDisplay(Pt0.x);
								Control0.EndPoint.y = WorldToDisplay(Pt0.y);

								Control1.EndPoint.x = WorldToDisplay(Pt1.x);
								Control1.EndPoint.y = WorldToDisplay(Pt1.y);
							}

							if (when)
							{
								return ((uint16_t)time_y_0);
							}
						}
					} while (time_y_0 < time_y_1);
				}
			}
		}
	}

	return (0);
}

static void
InitCyborg(STARSHIP* StarShipPtr)
{
	uint16_t Index {};
	uint16_t Divisor {};

	Index = StarShipPtr->RaceDescPtr->characteristics.max_thrust
		  * StarShipPtr->RaceDescPtr->characteristics.thrust_increment;
	if ((Divisor = StarShipPtr->RaceDescPtr->characteristics.turn_wait
				 + StarShipPtr->RaceDescPtr->characteristics.thrust_wait)
		> 0)
	{
		Index /= Divisor;
	}
	else
	{
		Index >>= 1;
	}
#ifdef PRINT_MI
	{
		const char* shipName {StarShipPtr->RaceDescPtr->ship_data.race_strings->text(0)};
		uqm::log::debug("MI({}) -- <{}:{}> = {}", shipName,
						StarShipPtr->RaceDescPtr->characteristics.max_thrust * StarShipPtr->RaceDescPtr->characteristics.thrust_increment,
						Divisor, Index);
	}
#endif /* PRINT_MI */
	StarShipPtr->RaceDescPtr->cyborg_control.ManeuverabilityIndex = Index;
}

static void
ship_movement(ELEMENT* ShipPtr, EVALUATE_DESC* EvalDescPtr)
{
	if (EvalDescPtr->which_turn == 0)
	{
		EvalDescPtr->which_turn = 1;
	}

	switch (EvalDescPtr->MoveState)
	{
		case PURSUE:
			Pursue(ShipPtr, EvalDescPtr);
			break;
		case AVOID:
#ifdef NOTYET
			Avoid(ShipPtr, EvalDescPtr);
			break;
#endif /* NOTYET */
		case ENTICE:
			Entice(ShipPtr, EvalDescPtr);
			break;
		case NO_MOVEMENT:
			break;
	}
}

// JMS:GFX Made SIZEs SDWORDs and changed the GetNextVelocityComponents to GetNextVelocityComponentsSdword
bool ship_weapons(ELEMENT* ShipPtr, ELEMENT* OtherPtr, uint16_t margin_of_error)
{
	int32_t delta_x {};
	int32_t delta_y {};
	uint16_t n {};
	uint16_t num_weapons {};
	ELEMENT Ship;
	HELEMENT Weapon[6];
	STARSHIP* StarShipPtr {};

	if (ObjectCloaked(OtherPtr))
	{
		margin_of_error += DisplayToWorld(ResScale(40));
	}

	Ship = *ShipPtr;
	GetNextVelocityComponentsSdword(&Ship.velocity,
									&delta_x, &delta_y, 1);
	Ship.next.location.x =
		Ship.current.location.x + delta_x;
	Ship.next.location.y =
		Ship.current.location.y + delta_y;
	Ship.current.location = Ship.next.location;

	GetElementStarShip(&Ship, &StarShipPtr);
	num_weapons =
		(*StarShipPtr->RaceDescPtr->init_weapon_func)(&Ship, Weapon);

	if ((n = num_weapons))
	{
		HELEMENT w {};
		//STARSHIP *StarShipPtr;
		ELEMENT* EPtr {};

		HELEMENT* WeaponPtr {&Weapon[0]};
		do
		{
			w = *WeaponPtr;
			if (w)
			{
				LockElement(w, &EPtr);
				if (EPtr->state_flags & Appearing)
				{
					EPtr->next = EPtr->current;
					InitIntersectStartPoint(EPtr);
					InitIntersectEndPoint(EPtr);
					InitIntersectFrame(EPtr);
				}

				if (PlotIntercept(EPtr, OtherPtr,
								  EPtr->life_span, margin_of_error))
				{
					UnlockElement(w);
					break;
				}

				UnlockElement(w);
				FreeElement(w);
			}
			++WeaponPtr;
		} while (--n);

		if ((num_weapons = n))
		{
			do
			{
				w = *WeaponPtr++;
				if (w)
				{
					FreeElement(w);
				}
			} while (--n);
		}
	}

	//if (num_weapons > 0)
	//	log_add (log_Debug, "dx:{}, dy:{}, currx:{}, curry:{}, nextx:{}, nexty:{}", delta_x, delta_y, Ship.current.location.x, Ship.current.location.y, Ship.next.location.x, Ship.next.location.y);

	return (num_weapons > 0);
}

void ship_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
					   uint16_t ConcernCounter)
{
	uint16_t margin_of_error {};
	STARSHIP* StarShipPtr {};
	// EVALUATE_DESC *ObjectsOfConcernEWeapon; unused

	GetElementStarShip(ShipPtr, &StarShipPtr);

	bool ShipMoved {true};
	if (ShipPtr->turn_wait == 0)
	{
		ShipMoved = false;
	}
	if (ShipPtr->thrust_wait == 0)
	{
		ShipMoved = false;
	}

	bool ShipFired {true};
	if (StarShipPtr->weapon_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~WeaponFlag;
		if (!(StarShipPtr->RaceDescPtr->ship_info.ship_flags & SeekingWeapon))
		{
			ShipFired = false;
		}
	}

	if (testFlag(StarShipPtr->control, PlayerControlFlags::DifficultyAwesome))
	{
		margin_of_error = 0;
	}
	else if (testFlag(StarShipPtr->control, PlayerControlFlags::DifficultyGood))
	{
		margin_of_error = DisplayToWorld(ResScale(20));
	}
	else /* if (StarShipPtr->control & STANDARD_RATING) */
	{
		margin_of_error = DisplayToWorld(ResScale(40));
	}

	ObjectsOfConcern += ConcernCounter;

	// ObjectsOfConcernEWeapon = ObjectsOfConcern - ConcernCounter + ENEMY_WEAPON_INDEX;

	while (ConcernCounter--)
	{
		--ObjectsOfConcern;
		if (ObjectsOfConcern->ObjectPtr)
		{
			if (!ShipMoved
				&& (ConcernCounter != ENEMY_WEAPON_INDEX
					|| ObjectsOfConcern->MoveState == PURSUE
					|| (ObjectsOfConcern->ObjectPtr->state_flags & CrewObject)
					|| Maneuverability(
						   &StarShipPtr->RaceDescPtr->cyborg_control)
						   >= ResolutionCompensated(MediumShip)))
			{
				ship_movement(ShipPtr, ObjectsOfConcern);
				ShipMoved = true;
			}
			if (!ShipFired
				&& (ConcernCounter == ENEMY_SHIP_INDEX
					|| (ConcernCounter == ENEMY_WEAPON_INDEX
						&& ObjectsOfConcern->MoveState != AVOID
#ifdef NEVER
						&& !(StarShipPtr->control & STANDARD_RATING)
#endif /* NEVER */
							)))
			{
				ShipFired = ship_weapons(ShipPtr,
										 ObjectsOfConcern->ObjectPtr, margin_of_error);
				if (ShipFired)
				{
					StarShipPtr->ship_input_state |= WeaponFlag;
				}
			}
		}
	}
}

bool TurnShip(ELEMENT* ShipPtr, uint16_t angle)
{
	uint16_t ship_delta_facing {};
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	uint16_t f {StarShipPtr->ShipFacing};
	ship_delta_facing = NormalizeFacing(AngleToFacing(angle) - f);
	if (ship_delta_facing)
	{
		if (ship_delta_facing == AngleToFacing(HalfCircle))
		{
			ship_delta_facing =
				NormalizeFacing(ship_delta_facing + (uqm::random().next() & 1 ? AngleToFacing(Octant >> 1) : -AngleToFacing(Octant >> 1)));
		}

		if (ship_delta_facing < AngleToFacing(HalfCircle))
		{
			StarShipPtr->ship_input_state |= RightFlag;
			++f;
			ShipPtr->next.image.frame =
				IncFrameIndex(ShipPtr->current.image.frame);
		}
		else
		{
			StarShipPtr->ship_input_state |= LeftFlag;
			--f;
			ShipPtr->next.image.frame =
				DecFrameIndex(ShipPtr->current.image.frame);
		}

#ifdef NOTYET
		if (((StarShipPtr->ship_input_state & (LeftFlag | RightFlag))
			 ^ (StarShipPtr->cur_status_flags & (LeftFlag | RightFlag)))
			== (LeftFlag | RightFlag))
		{
			StarShipPtr->ship_input_state &= ~(LeftFlag | RightFlag);
		}
		else
#endif /* NOTYET */
		{
			StarShipPtr->ShipFacing = NormalizeFacing(f);

			return (true);
		}
	}

	return (false);
}

bool ThrustShip(ELEMENT* ShipPtr, uint16_t angle)
{
	bool ShouldThrust {};
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->ship_input_state & ThrustFlag)
	{
		ShouldThrust = true;
	}
	else if (NormalizeFacing(AngleToFacing(angle)
							 - AngleToFacing(GetVelocityTravelAngle(&ShipPtr->velocity)))
				 == 0
			 && (StarShipPtr->cur_status_flags
				 & (ShipAtMaxSpeed | ShipBeyondMaxSpeed))
			 && !(StarShipPtr->cur_status_flags & ShipInGravityWell))
	{
		ShouldThrust = false;
	}
	else
	{
		int16_t ship_delta_facing {};

		ship_delta_facing =
			NormalizeFacing(AngleToFacing(angle)
							- StarShipPtr->ShipFacing + AngleToFacing(Quadrant));
		if (ship_delta_facing == AngleToFacing(Quadrant)
			|| ((StarShipPtr->cur_status_flags & ShipBeyondMaxSpeed)
				&& ship_delta_facing <= AngleToFacing(HalfCircle)))
		{
			ShouldThrust = true;
		}
		else
		{
			ShouldThrust = false;
		}
	}

	if (ShouldThrust)
	{
		inertial_thrust(ShipPtr);

		StarShipPtr->ship_input_state |= ThrustFlag;
	}

	return (ShouldThrust);
}

// JMS:GFX Made SIZEs SDWORDs and changed the GetNextVelocityComponents to GetNextVelocityComponentsSdword
void Pursue(ELEMENT* ShipPtr, EVALUATE_DESC* EvalDescPtr)
{
	uint16_t desired_turn_angle {};
	int32_t delta_x {};
	int32_t delta_y {};
	int32_t ship_delta_x {};
	int32_t ship_delta_y {};
	int32_t other_delta_x {};
	int32_t other_delta_y {};
	VELOCITY_DESC ShipVelocity;
	VELOCITY_DESC OtherVelocity;
	uint16_t distance_to_give_up_and_turn {};

	ShipVelocity = ShipPtr->velocity;
	GetNextVelocityComponentsSdword(&ShipVelocity,
									&ship_delta_x, &ship_delta_y, EvalDescPtr->which_turn);
	ShipPtr->next.location.x =
		ShipPtr->current.location.x + ship_delta_x;
	ShipPtr->next.location.y =
		ShipPtr->current.location.y + ship_delta_y;

	ELEMENT* OtherObjPtr {EvalDescPtr->ObjectPtr};

	OtherVelocity = OtherObjPtr->velocity;
	GetNextVelocityComponentsSdword(&OtherVelocity,
									&other_delta_x, &other_delta_y, EvalDescPtr->which_turn);

	delta_x = (OtherObjPtr->current.location.x + other_delta_x)
			- ShipPtr->next.location.x;
	delta_y = (OtherObjPtr->current.location.y + other_delta_y)
			- ShipPtr->next.location.y;
	delta_x = WrapDeltaX(delta_x);
	delta_y = WrapDeltaY(delta_y);
	uint16_t desired_thrust_angle {ARCTAN(delta_x, delta_y)};

	uint8_t maneuver_state {0};
	if (ShipPtr->turn_wait == 0)
	{
		maneuver_state |= LeftFlag | RightFlag;
	}
	if (ShipPtr->thrust_wait == 0
		&& ((OtherObjPtr->state_flags & PlayerShip)
			|| elementsOfSamePlayer(OtherObjPtr, ShipPtr)
			|| OtherObjPtr->preprocess_func == crew_preprocess))
	{
		maneuver_state |= ThrustFlag;
	}

	desired_turn_angle = NormalizeAngle(desired_thrust_angle + HalfCircle);
	/* other player's ship */
	if ((OtherObjPtr->state_flags & PlayerShip)
		&& OtherObjPtr->mass_points <= MaxShipMass)
	{
		STARSHIP* StarShipPtr {};
		STARSHIP* EnemyStarShipPtr {};

		GetElementStarShip(ShipPtr, &StarShipPtr);
		GetElementStarShip(OtherObjPtr, &EnemyStarShipPtr);
		if ((Maneuverability(
				 &StarShipPtr->RaceDescPtr->cyborg_control)
				 >= ResolutionCompensated(FastShip)
			 && GetWeaponRange(&StarShipPtr->RaceDescPtr->cyborg_control)
					> ResScale(CloseRangeWeapon))
			|| (EvalDescPtr->which_turn >= 24
				&& (StarShipPtr->RaceDescPtr->characteristics.max_thrust * 2 / 3 < EnemyStarShipPtr->RaceDescPtr->characteristics.max_thrust
					|| (EnemyStarShipPtr->cur_status_flags & ShipBeyondMaxSpeed))))
		{
			uint16_t ship_flags {};

			ship_flags = EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags;
			/* you're maneuverable */
			if (Maneuverability(
					&StarShipPtr->RaceDescPtr->cyborg_control)
				>= ResolutionCompensated(MediumShip))
			{
				uint16_t fire_flags {};
				uint16_t facing {};

				for (fire_flags = FiresFore, facing = EvalDescPtr->facing;
					 fire_flags <= FiresLeft;
					 fire_flags <<= 1, facing += Quadrant)
				{
					if (
						/* he's dangerous in this direction */
						(ship_flags & fire_flags)
						/* he's facing direction you want to go */
						&& NormalizeAngle(
							   desired_turn_angle - facing + Octant)
							   <= Quadrant
						&& (
							/* he's moving */
							(other_delta_x != 0 || other_delta_y != 0)
							&&
							/* he's coasting backwards */
							NormalizeAngle(
								(GetVelocityTravelAngle(&OtherVelocity) + HalfCircle)
								- facing + (Octant + (Octant >> 1)))
								<= ((Octant + (Octant >> 1)) << 1)))
					{
						/* catch him on the back side */
						desired_thrust_angle = desired_turn_angle;
						break;
					}
				}
			}

			// This code prevents Kohr-Ah, Ur-Quan and ISD from turning around mid-chase while pursuing Earthling.
			if (StarShipPtr->SpeciesID == (KOHR_AH_ID | UR_QUAN_ID)
				&& EnemyStarShipPtr->SpeciesID == EARTHLING_ID
				&& !(EnemyStarShipPtr->cur_status_flags & (ShipBeyondMaxSpeed | ShipInGravityWell)))
			{
				distance_to_give_up_and_turn = 44;
			}
			else
			{
				distance_to_give_up_and_turn = 24;
			}

			if (desired_thrust_angle != desired_turn_angle
				&& (other_delta_x || other_delta_y)
				&& EvalDescPtr->which_turn >= distance_to_give_up_and_turn
				&& NormalizeAngle(desired_thrust_angle
								  - GetVelocityTravelAngle(&OtherVelocity)
								  + Octant)
					   <= Quadrant
				&& ((NormalizeAngle(
						 GetVelocityTravelAngle(&OtherVelocity)
						 - GetVelocityTravelAngle(&ShipVelocity)
						 + Octant)
						 <= Quadrant
					 && (((StarShipPtr->cur_status_flags & ShipAtMaxSpeed)
						  && !(StarShipPtr->cur_status_flags & ShipBeyondMaxSpeed))
						 || (ship_flags & DontChase)))
					|| NormalizeAngle(
						   desired_turn_angle
						   - FacingToAngle(StarShipPtr->ShipFacing)
						   + Octant)
						   <= Quadrant))
			{
				desired_thrust_angle = desired_turn_angle;
			}
		}
	}

	if (maneuver_state & (LeftFlag | RightFlag))
	{
		TurnShip(ShipPtr, desired_thrust_angle);
	}

	if (maneuver_state & ThrustFlag)
	{
		ThrustShip(ShipPtr, desired_thrust_angle);
	}
}

static constexpr auto FlankLeft {-Quadrant};
static constexpr auto ShipLeft {-Octant};

// JMS:GFX Made SIZEs SDWORDs and changed the GetNextVelocityComponents to GetNextVelocityComponentsSdword
void Entice(ELEMENT* ShipPtr, EVALUATE_DESC* EvalDescPtr)
{
	uint16_t desired_turn_angle {};
	uint16_t cone_of_fire {};
	int32_t delta_x {};
	int32_t delta_y {};
	int32_t ship_delta_x {};
	int32_t ship_delta_y {};
	int32_t other_delta_x {};
	int32_t other_delta_y {};
	VELOCITY_DESC ShipVelocity;
	VELOCITY_DESC OtherVelocity;
	STARSHIP* StarShipPtr {};

	ShipVelocity = ShipPtr->velocity;
	GetNextVelocityComponentsSdword(&ShipVelocity,
									&ship_delta_x, &ship_delta_y, EvalDescPtr->which_turn);
	ShipPtr->next.location.x =
		ShipPtr->current.location.x + ship_delta_x;
	ShipPtr->next.location.y =
		ShipPtr->current.location.y + ship_delta_y;

	ELEMENT* OtherObjPtr {EvalDescPtr->ObjectPtr};
	OtherVelocity = OtherObjPtr->velocity;
	GetNextVelocityComponentsSdword(&OtherVelocity,
									&other_delta_x, &other_delta_y, EvalDescPtr->which_turn);

	delta_x = (OtherObjPtr->current.location.x + other_delta_x)
			- ShipPtr->next.location.x;
	delta_y = (OtherObjPtr->current.location.y + other_delta_y)
			- ShipPtr->next.location.y;
	delta_x = WrapDeltaX(delta_x);
	delta_y = WrapDeltaY(delta_y);
	uint16_t desired_thrust_angle {ARCTAN(delta_x, delta_y)};

	uint8_t maneuver_state {0};
	if (ShipPtr->turn_wait == 0)
	{
		maneuver_state |= LeftFlag | RightFlag;
	}
	if (ShipPtr->thrust_wait == 0)
	{
		maneuver_state |= ThrustFlag;
	}

	delta_x = ship_delta_x - other_delta_x;
	delta_y = ship_delta_y - other_delta_y;
	const uint16_t travel_angle {ARCTAN(delta_x, delta_y)};
	desired_turn_angle = NormalizeAngle(desired_thrust_angle + HalfCircle);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	RACE_DESC* RDPtr {StarShipPtr->RaceDescPtr};
	if (EvalDescPtr->MoveState == AVOID)
	{
		desired_turn_angle =
			NormalizeAngle(desired_turn_angle - EvalDescPtr->facing);

		if (NormalizeFacing(AngleToFacing(desired_turn_angle)))
		{
			if (desired_turn_angle <= HalfCircle)
			{
				desired_thrust_angle = RightFlag;
			}
			else /* if (desired_turn_angle > HalfCircle) */
			{
				desired_thrust_angle = LeftFlag;
			}
		}
		else
		{
			desired_turn_angle = NormalizeAngle(
				FacingToAngle(StarShipPtr->ShipFacing)
				- EvalDescPtr->facing);
			if ((desired_turn_angle & (HalfCircle - 1)) == 0)
			{
				desired_thrust_angle = uqm::random().next() & 1 ? RightFlag : LeftFlag;
			}
			else
			{
				desired_thrust_angle = desired_turn_angle < HalfCircle ? RightFlag : LeftFlag;
			}
		}

		if (desired_thrust_angle == LeftFlag)
		{
			desired_thrust_angle = EvalDescPtr->facing
								 + FlankLeft - (ShipLeft >> 1);
		}
		else
		{


			desired_thrust_angle = EvalDescPtr->facing
								 + Quadrant - (Octant >> 1);
		}

		desired_thrust_angle = NormalizeAngle(desired_thrust_angle);
	}
	else if (GravityMass(OtherObjPtr->mass_points))
	{
		uint16_t planet_facing {};

		planet_facing = NormalizeFacing(AngleToFacing(desired_thrust_angle));
		cone_of_fire = NormalizeFacing(
			planet_facing
			- StarShipPtr->ShipFacing
			+ AngleToFacing(Quadrant));

		if (RDPtr->characteristics.thrust_increment != RDPtr->characteristics.max_thrust)
		{
			maneuver_state &= ~ThrustFlag;
		}

		/* if not pointing towards planet */
		if (cone_of_fire > AngleToFacing(Quadrant << 1))
		{
			desired_turn_angle = desired_thrust_angle;
		}
		/* if pointing directly at planet */
		else if (cone_of_fire == AngleToFacing(Quadrant)
				 && NormalizeFacing(AngleToFacing(travel_angle)) != planet_facing)
		{
			desired_turn_angle = travel_angle;
		}
		else if (cone_of_fire == 0
				 || cone_of_fire == AngleToFacing(Quadrant << 1)
				 || (!(maneuver_state & ThrustFlag)
					 && (cone_of_fire < AngleToFacing(Octant)
						 || cone_of_fire > AngleToFacing((Quadrant << 1) - Octant))))
		{
			desired_turn_angle = FacingToAngle(StarShipPtr->ShipFacing);
			if (NormalizeAngle(desired_turn_angle
							   - travel_angle + Quadrant)
				> HalfCircle)
			{
				desired_turn_angle = travel_angle;
			}
			if (ShipPtr->thrust_wait == 0)
			{
				maneuver_state |= ThrustFlag;
			}
		}

		desired_thrust_angle = desired_turn_angle;
	}
	else
	{
		uint16_t WRange {};

		WRange = GetWeaponRange(
			&RDPtr->cyborg_control);

		cone_of_fire = NormalizeAngle(desired_turn_angle
									  - EvalDescPtr->facing + Octant);
		if (OtherObjPtr->state_flags & PlayerShip)
		{
			uint16_t fire_flags {};
			uint16_t ship_flags {};
			uint16_t facing {};
			STARSHIP* EnemyStarShipPtr {};

			GetElementStarShip(OtherObjPtr, &EnemyStarShipPtr);
			ship_flags = EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags;
			for (fire_flags = FiresFore, facing = EvalDescPtr->facing;
				 fire_flags <= FiresLeft;
				 fire_flags <<= 1, facing += Quadrant)
			{
				if (
					/* he's dangerous in this direction */
					(ship_flags & fire_flags)
					/* he's facing direction you want to go */
					&& (cone_of_fire = NormalizeAngle(
							desired_turn_angle - facing + Octant))
						   <= Quadrant
					/* he's moving */
					&& ((other_delta_x != 0 || other_delta_y != 0)
						/* he's coasting backwards */
						&& NormalizeAngle(
							   (GetVelocityTravelAngle(&OtherVelocity) + HalfCircle)
							   - facing + Octant)
							   <= Quadrant))
				{
					/* need to be close for a kill */
					if (WRange < ResScale(LongRangeWeapon)
						&& EvalDescPtr->which_turn <= 32)
					{
						/* catch him on the back side */
						desired_thrust_angle = desired_turn_angle;
						goto DoManeuver;
					}

					break;
				}
			}

			if (EvalDescPtr->which_turn <= 8
				&& RDPtr->characteristics.max_thrust <= EnemyStarShipPtr->RaceDescPtr->characteristics.max_thrust)
			{
				goto DoManeuver;
			}
		}

		if (
#ifdef NOTYET
			WRange < ResScale(LongRangeWeapon)
			&&
#endif /* NOTYET */
			/* not at full speed */
			!(StarShipPtr->cur_status_flags
			  & (ShipAtMaxSpeed | ShipBeyondMaxSpeed))
			&& (PlotIntercept(
					ShipPtr, OtherObjPtr, 40, ResScale(CloseRangeWeapon) << 1)
#ifdef NOTYET
				|| (
					/* object's facing direction you want to go */
					cone_of_fire <= Quadrant
					/* and you're basically going in that direction */
					&& (travel_angle == FullCircle
						|| NormalizeAngle(travel_angle
										  - desired_thrust_angle + Quadrant)
							   <= HalfCircle)
					/* and object's in range */
					&& PlotIntercept(ShipPtr, OtherObjPtr, 1, WRange))
#endif /* NOTYET */
					))
		{
			if (
				/* pointed straight at him */
				NormalizeAngle(desired_thrust_angle
							   - FacingToAngle(StarShipPtr->ShipFacing) + Octant)
					<= Quadrant
				/* or not exposed to business end */
				|| cone_of_fire > Quadrant)
			{
				desired_thrust_angle = desired_turn_angle;
			}
			else
			{
#ifdef NOTYET
				if (
					travel_angle != FullCircle
					&& NormalizeAngle(travel_angle
									  - desired_turn_angle + Octant)
						   <= Quadrant)
				{
					desired_turn_angle =
						NormalizeAngle((EvalDescPtr->facing + HalfCircle)
									   + (travel_angle - desired_turn_angle));
					if (!(maneuver_state & (LeftFlag | RightFlag)))
					{
						maneuver_state &= ~ThrustFlag;
					}
				}

				if (maneuver_state & (LeftFlag | RightFlag))
				{
					TurnShip(ShipPtr, desired_turn_angle);
					maneuver_state &= ~(LeftFlag | RightFlag);
				}
#endif /* NOTYET */

				desired_thrust_angle = FacingToAngle(StarShipPtr->ShipFacing);
				desired_turn_angle = desired_thrust_angle;
			}
		}
		else if ((cone_of_fire = PlotIntercept(
					  ShipPtr, OtherObjPtr, 10, WRange
#ifdef OLD
													- (WRange >> 3)
#else  /* !OLD */
													- (WRange >> 2)
#endif /* OLD */
						  )))
		{
			if (RDPtr->characteristics.thrust_increment != RDPtr->characteristics.max_thrust
				/* and already at full speed */
				&& (StarShipPtr->cur_status_flags
					& (ShipAtMaxSpeed | ShipBeyondMaxSpeed))
				/* and facing away from enemy */
				&& (NormalizeAngle(desired_turn_angle
								   - ARCTAN(ship_delta_x, ship_delta_y)
								   + (Octant + 2))
						<= ((Octant + 2) << 1)
					/* or not on collision course */
					|| !PlotIntercept(
						ShipPtr, OtherObjPtr, 30, ResScale(CloseRangeWeapon) << 1)))
			{
				maneuver_state &= ~ThrustFlag;
			}
			/* veer off */
			else if (cone_of_fire == 1
					 || RDPtr->characteristics.thrust_increment != RDPtr->characteristics.max_thrust)
			{
				if (maneuver_state & (LeftFlag | RightFlag))
				{
					TurnShip(ShipPtr, desired_turn_angle);
					maneuver_state &= ~(LeftFlag | RightFlag);
				}

				if (NormalizeAngle(desired_thrust_angle
								   - ARCTAN(ship_delta_x, ship_delta_y)
								   + (Octant + 2))
					<= ((Octant + 2) << 1))
				{
					desired_thrust_angle = FacingToAngle(StarShipPtr->ShipFacing);
				}
				else
				{
					desired_thrust_angle = desired_turn_angle;
				}
			}
		}
	}

DoManeuver:
	if (maneuver_state & (LeftFlag | RightFlag))
	{
		TurnShip(ShipPtr, desired_thrust_angle);
	}

	if (maneuver_state & ThrustFlag)
	{
		ThrustShip(ShipPtr, desired_thrust_angle);
	}
}

void Avoid(ELEMENT* ShipPtr, EVALUATE_DESC* EvalDescPtr)
{
	(void)ShipPtr;	   /* Satisfying compiler (unused parameter) */
	(void)EvalDescPtr; /* Satisfying compiler (unused parameter) */
}

BATTLE_INPUT_STATE
tactical_intelligence(ComputerInputContext* context, STARSHIP* StarShipPtr)
{
	ELEMENT* ShipPtr {};
	ELEMENT Ship;
	uint16_t ShipFacing {};
	HELEMENT hElement {};
	HELEMENT hNextElement {};
	uint16_t ConcernCounter {};
	EVALUATE_DESC ObjectsOfConcern[10];
	bool UltraManeuverable {};
	STARSHIP* EnemyStarShipPtr {};
	RACE_DESC* EnemyRDPtr {};

	RACE_DESC* RDPtr {StarShipPtr->RaceDescPtr};

	if (RDPtr->cyborg_control.ManeuverabilityIndex == 0)
	{
		InitCyborg(StarShipPtr);
	}

	LockElement(StarShipPtr->hShip, &ShipPtr);
	if (RDPtr->ship_info.crew_level == 0
		|| GetPrimType(&DisplayArray[ShipPtr->PrimIndex]) == NoPrim)
	{
		UnlockElement(StarShipPtr->hShip);
		return (0);
	}

	bool ShipMoved {true};
	/* Disable ship's special completely for the Standard AI */
	if (testFlag(StarShipPtr->control, PlayerControlFlags::DifficultyStandard))
	{
		++StarShipPtr->special_counter;
	}

#ifdef DEBUG_CYBORG
	if (!(ShipPtr->state_flags & FiniteLife)
		&& ShipPtr->life_span == NormalLife)
	{
		ShipPtr->life_span += 2; /* make ship invulnerable */
	}
#endif /* DEBUG_CYBORG */
	Ship = *ShipPtr;
	UnlockElement(StarShipPtr->hShip);
	ShipFacing = StarShipPtr->ShipFacing;

	for (ConcernCounter = 0;
		 ConcernCounter <= FIRST_EMPTY_INDEX; ++ConcernCounter)
	{
		ObjectsOfConcern[ConcernCounter].ObjectPtr = 0;
		ObjectsOfConcern[ConcernCounter].MoveState = NO_MOVEMENT;
		ObjectsOfConcern[ConcernCounter].which_turn = (uint16_t)~0;
	}
	--ConcernCounter;

	UltraManeuverable = (bool)(RDPtr->characteristics.thrust_increment == RDPtr->characteristics.max_thrust
							   && Maneuverability(&RDPtr->cyborg_control) >= ResolutionCompensated(MediumShip));

	if (Ship.turn_wait == 0)
	{
		ShipMoved = false;
		StarShipPtr->ship_input_state &= ~(LeftFlag | RightFlag);
	}
	if (Ship.thrust_wait == 0)
	{
		ShipMoved = false;
		StarShipPtr->ship_input_state &= ~ThrustFlag;
	}

	for (hElement = GetHeadElement();
		 hElement != 0; hElement = hNextElement)
	{
		EVALUATE_DESC ed;

		ed.MoveState = NO_MOVEMENT;

		LockElement(hElement, &ed.ObjectPtr);
		hNextElement = GetSuccElement(ed.ObjectPtr);
		if (CollisionPossible(ed.ObjectPtr, &Ship))
		{
			int32_t dx {};
			int32_t dy {};

			dx = ed.ObjectPtr->next.location.x
			   - Ship.next.location.x;
			dy = ed.ObjectPtr->next.location.y
			   - Ship.next.location.y;
			dx = WrapDeltaX(dx);
			dy = WrapDeltaY(dy);
			if (GravityMass(ed.ObjectPtr->mass_points))
			{
				uint16_t maneuver_turn {};
				uint16_t ship_bounds {};
				GFXRECT ship_footprint;

				if (UltraManeuverable)
				{
					maneuver_turn = 16;
				}
				else if (Maneuverability(&RDPtr->cyborg_control) <= ResolutionCompensated(MediumShip))
				{
					maneuver_turn = 48;
				}
				else
				{
					maneuver_turn = 32;
				}

				GetFrameRect(SetAbsFrameIndex(
								 Ship.IntersectControl.IntersectStamp.frame, 0),
							 &ship_footprint);
				ship_bounds = (uint16_t)(ship_footprint.extent.width
										 + ship_footprint.extent.height);

				if (!ShipMoved && (ed.which_turn = PlotIntercept(ed.ObjectPtr, &Ship, maneuver_turn, DisplayToWorld(ResScale(30) + (ship_bounds * 3 /* << 2 */)))))
				{
					if (ed.which_turn > 1
						|| PlotIntercept(ed.ObjectPtr, &Ship, 1,
										 DisplayToWorld(ResScale(35) + ship_bounds))
						|| PlotIntercept(ed.ObjectPtr, &Ship,
										 maneuver_turn << 1,
										 DisplayToWorld(ResScale(40) + ship_bounds))
							   > 1)
					{
						ed.facing = ARCTAN(-dx, -dy);
						if (UltraManeuverable)
						{
							ed.MoveState = AVOID;
						}
						else // Try a gravity whip
						{
							ed.MoveState = ENTICE;
						}

						ObjectsOfConcern[GRAVITY_MASS_INDEX] = ed;
					}
					else if (!UltraManeuverable && !IsVelocityZero(&Ship.velocity))
					{ // Try an orbital insertion, don't thrust
						++Ship.thrust_wait;
						if (Ship.turn_wait)
						{
							ShipMoved = true;
						}
					}
				}
			}
			else if (ed.ObjectPtr->state_flags & PlayerShip)
			{
				GetElementStarShip(ed.ObjectPtr, &EnemyStarShipPtr);
				EnemyRDPtr = EnemyStarShipPtr->RaceDescPtr;
				if (EnemyRDPtr->cyborg_control.ManeuverabilityIndex == 0)
				{
					InitCyborg(EnemyStarShipPtr);
				}

				ed.which_turn = WorldToTurn(
					square_root((long)dx * dx + (long)dy * dy));
				if (ResDescale(ed.which_turn) > ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn)
				{
					UnlockElement(hElement);
					continue;
				}
				else if (ed.which_turn == 0)
				{
					ed.which_turn = 1;
				}

				ed.which_turn >>= resolutionFactor;

				ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr = ed.ObjectPtr;
				ObjectsOfConcern[ENEMY_SHIP_INDEX].facing =
#ifdef MAYBE
					ObjectCloaked(ed.ObjectPtr) ?
						GetVelocityTravelAngle(&ed.ObjectPtr->velocity) :
#endif /* MAYBE */
						FacingToAngle(EnemyStarShipPtr->ShipFacing);
				ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn = ed.which_turn;

				if (ShipMoved
					|| ed.ObjectPtr->mass_points > MaxShipMass
					|| (GetWeaponRange(&RDPtr->cyborg_control) < ResScale(LongRangeWeapon)
						&& (GetWeaponRange(&RDPtr->cyborg_control) <= ResScale(CloseRangeWeapon)
							|| (GetWeaponRange(&EnemyRDPtr->cyborg_control) >= ResScale(LongRangeWeapon)
								&& (EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & SeekingWeapon))
							|| (
#ifdef OLD
								Maneuverability(&RDPtr->cyborg_control) < Maneuverability(&EnemyRDPtr->cyborg_control)
#else  /* !OLD */
								RDPtr->characteristics.max_thrust < EnemyRDPtr->characteristics.max_thrust
#endif /* !OLD */
								&& GetWeaponRange(&RDPtr->cyborg_control) < GetWeaponRange(&EnemyRDPtr->cyborg_control)))))
					ObjectsOfConcern[ENEMY_SHIP_INDEX].MoveState = PURSUE;
				else
				{
					ObjectsOfConcern[ENEMY_SHIP_INDEX].MoveState = ENTICE;
				}

				if ((EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & ImmediateWeapon)
					&& ship_weapons(ed.ObjectPtr, &Ship, 0))
				{
					ed.which_turn = 1;
					ed.MoveState = AVOID;
					ed.facing = ObjectsOfConcern[ENEMY_SHIP_INDEX].facing;

					ObjectsOfConcern[ENEMY_WEAPON_INDEX] = ed;
				}
			}
			else if (ed.ObjectPtr->pParent == 0)
			{
				if (!(ed.ObjectPtr->state_flags & FiniteLife))
				{
					ed.which_turn = ResDescale(WorldToTurn(
						square_root((long)dx * dx + (long)dy * dy)));

					if (ed.which_turn < ObjectsOfConcern[FIRST_EMPTY_INDEX].which_turn)
					{
						ed.MoveState = PURSUE;
						ed.facing = GetVelocityTravelAngle(
							&ed.ObjectPtr->velocity);

						ObjectsOfConcern[FIRST_EMPTY_INDEX] = ed;
					}
				}
			}
			else if (!elementsOfSamePlayer(ed.ObjectPtr, &Ship)
					 && ed.ObjectPtr->preprocess_func != crew_preprocess
					 && ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn > 1
					 && ed.ObjectPtr->life_span > 0)
			{
				GetElementStarShip(ed.ObjectPtr, &EnemyStarShipPtr);
				EnemyRDPtr = EnemyStarShipPtr->RaceDescPtr;
				if (((EnemyRDPtr->ship_info.ship_flags & SeekingWeapon)
					 && ed.ObjectPtr->next.image.farray != EnemyRDPtr->ship_data.special)
					|| ((EnemyRDPtr->ship_info.ship_flags & SeekingSpecial)
						&& ed.ObjectPtr->next.image.farray == EnemyRDPtr->ship_data.special))
				{
					if ((!(ed.ObjectPtr->state_flags & (FiniteLife | CrewObject))
						 && RDPtr->characteristics.max_thrust > DisplayToWorld(ResScale(8)))
						|| NormalizeAngle(GetVelocityTravelAngle(
											  &ed.ObjectPtr->velocity)
										  - ARCTAN(-dx, -dy)
										  + Quadrant)
							   > HalfCircle)
					{
						ed.which_turn = 0;
					}
					else
					{
						ed.which_turn = ResDescale(WorldToTurn(
							square_root((long)dx * dx + (long)dy * dy)));

						ed.MoveState = ENTICE;

						if (ed.which_turn == 0)
						{
							ed.which_turn = 1;
						}
						/* Shiver: The cap on which_turn for seeking weapons raised from 16 to 20.
						 The horrible cap of 8 for above-medium speed ships has been obliterated. */
						else if (ed.which_turn > 20)
						{
							ed.which_turn = 0;
						}
					}
				}
				else if (!testFlag(StarShipPtr->control, PlayerControlFlags::DifficultyAwesome))
				{
					ed.which_turn = 0;
				}
				else
				{
					ed.which_turn =
						PlotIntercept(ed.ObjectPtr,
									  &Ship, ed.ObjectPtr->life_span,
									  DisplayToWorld(ResScale(40)));
					ed.MoveState = AVOID;
				}

				if (ed.which_turn > 0
					&& (ed.which_turn < ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn
						|| (ed.which_turn == ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn
							&& ed.MoveState == AVOID)))
				{
					ed.facing = GetVelocityTravelAngle(
						&ed.ObjectPtr->velocity);

					ObjectsOfConcern[ENEMY_WEAPON_INDEX] = ed;
				}
			}
			else if ((ed.ObjectPtr->state_flags & CrewObject)
					 && ((!(ed.ObjectPtr->state_flags & IgnoreSimilar)
						  && elementsOfSamePlayer(ed.ObjectPtr, &Ship))
						 || ed.ObjectPtr->preprocess_func == crew_preprocess)
					 && ObjectsOfConcern[CREW_OBJECT_INDEX].which_turn > 1)
			{
				ed.which_turn = ResDescale(WorldToTurn(
					square_root((long)dx * dx + (long)dy * dy)));

				if (ed.which_turn == 0)
				{
					ed.which_turn = 1;
				}

				if (ObjectsOfConcern[CREW_OBJECT_INDEX].which_turn > ed.which_turn
					&& (ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn > 32
						|| (ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn > 8
							&& StarShipPtr->hShip == ed.ObjectPtr->hTarget)))
				{
					ed.MoveState = PURSUE;
					ed.facing = 0;
					ObjectsOfConcern[CREW_OBJECT_INDEX] = ed;
				}
			}
		}
		UnlockElement(hElement);
	}

	RDPtr->cyborg_control.intelligence_func(&Ship, ObjectsOfConcern,
											ConcernCounter);
#ifdef DEBUG_CYBORG
	StarShipPtr->ship_input_state &= ~SpecialFlag;
#endif /* DEBUG_CYBORG */

	StarShipPtr->ShipFacing = ShipFacing;
	{
		BATTLE_INPUT_STATE InputState {0};
		if (StarShipPtr->ship_input_state & LeftFlag)
		{
			InputState |= BattleLeft;
		}
		else if (StarShipPtr->ship_input_state & RightFlag)
		{
			InputState |= BattleRight;
		}
		if (StarShipPtr->ship_input_state & ThrustFlag)
		{
			InputState |= BattleThrust;
		}
		if (StarShipPtr->ship_input_state & WeaponFlag)
		{
			InputState |= BattleWeapon;
		}
		if (StarShipPtr->ship_input_state & SpecialFlag)
		{
			InputState |= BattleSpecial;
		}

		(void)context;
		return (InputState);
	}
}
