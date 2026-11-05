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

#include "../ship.h"
#include "arilou.h"
#include "resinst.h"
#include "core/log/log.h"
#include "libs/mathlib.h"
#include "uqm/globdata.h"
#include <math.h>
#include "uqm/random/UQMRandom.h"

// Core characteristics
#define MAX_CREW 6
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 6
#define MAX_THRUST /* DISPLAY_TO_WORLD (10) */ 40
#define THRUST_INCREMENT MAX_THRUST
#define THRUST_WAIT 0
#define TURN_WAIT 0
#define SHIP_MASS 1

// Tracking Laser
#define WEAPON_ENERGY_COST 2
#define WEAPON_WAIT 1
#define ARILOU_OFFSET 9
#define LASER_RANGE DISPLAY_TO_WORLD(100 + ARILOU_OFFSET)

// Teleporter
#define SPECIAL_ENERGY_COST 3
#define SPECIAL_WAIT 2
#define HYPER_LIFE 5

// HdResolutionFactor Values
#define LASER_RANGE_HD RES_SCALE(LASER_RANGE)

static RACE_DESC arilou_desc =
	{
		{/* SHIP_INFO */
		 "skiff",
		 /* FiresFore | */ ImmediateWeapon,
		 16, /* Super Melee cost */
		 MAX_CREW, MAX_CREW,
		 MAX_ENERGY, MAX_ENERGY,
		 ARILOU_RACE_STRINGS,
		 ARILOU_ICON_MASK_PMAP_ANIM,
		 ARILOU_MICON_MASK_PMAP_ANIM,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
			250 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				438,
				6372,
			},
		 },
		{
			MAX_THRUST,
			THRUST_INCREMENT,
			ENERGY_REGENERATION,
			WEAPON_ENERGY_COST,
			SPECIAL_ENERGY_COST,
			ENERGY_WAIT,
			TURN_WAIT,
			THRUST_WAIT,
			WEAPON_WAIT,
			SPECIAL_WAIT,
			SHIP_MASS,
		 },
		{{
			 ARILOU_BIG_MASK_PMAP_ANIM,
			 ARILOU_MED_MASK_PMAP_ANIM,
			 ARILOU_SML_MASK_PMAP_ANIM,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {
			 WARP_BIG_MASK_PMAP_ANIM,
			 WARP_MED_MASK_PMAP_ANIM,
			 WARP_SML_MASK_PMAP_ANIM,
		 },
		 {ARILOU_CAPTAIN_MASK_PMAP_ANIM,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 ARILOU_VICTORY_SONG,
		 ARILOU_SHIP_SOUNDS,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			LASER_RANGE >> 1,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static uint16_t
initialize_autoaim_laser(ELEMENT* ShipPtr, HELEMENT LaserArray[])
{
	uint16_t orig_facing {};
	int16_t delta_facing {};
	STARSHIP* StarShipPtr {};
	LASER_BLOCK LaserBlock;
	const uint16_t LaserRange = chooseIfHd(LASER_RANGE, LASER_RANGE_HD);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	LaserBlock.face = orig_facing = StarShipPtr->ShipFacing;
	if ((delta_facing = TrackShip(ShipPtr, &LaserBlock.face)) > 0)
	{
		LaserBlock.face = NORMALIZE_FACING(orig_facing + delta_facing);
	}
	ShipPtr->hTarget = 0;

	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = COSINE(FACING_TO_ANGLE(LaserBlock.face), LaserRange);
	LaserBlock.ey = SINE(FACING_TO_ANGLE(LaserBlock.face), LaserRange);
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = IgnoreSimilar;
	LaserBlock.pixoffs = RES_SCALE(ARILOU_OFFSET);
	LaserBlock.color = BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E);
	LaserArray[0] = initialize_laser(&LaserBlock);

	return (1);
}

static void
arilou_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
					uint16_t ConcernCounter)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state |= ThrustFlag;

	ObjectsOfConcern[ENEMY_SHIP_INDEX].MoveState = ENTICE;
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;

		const EVALUATE_DESC* lpEvalDesc {&ObjectsOfConcern[ENEMY_WEAPON_INDEX]};
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 6)
		{
			bool IsTrackingWeapon {};
			STARSHIP* EnemyStarShipPtr {};

			GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
			if (((EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
				  & SeekingWeapon)
				 && lpEvalDesc->ObjectPtr->next.image.farray == EnemyStarShipPtr->RaceDescPtr->ship_data.weapon)
				|| ((EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
					 & SeekingSpecial)
					&& lpEvalDesc->ObjectPtr->next.image.farray == EnemyStarShipPtr->RaceDescPtr->ship_data.special))
			{
				IsTrackingWeapon = true;
			}
			else
			{
				IsTrackingWeapon = false;
			}

			if (((lpEvalDesc->ObjectPtr->state_flags & PlayerShip)															 /* means IMMEDIATE WeaponFlag */
				 || (IsTrackingWeapon && (lpEvalDesc->which_turn == 1 || (lpEvalDesc->ObjectPtr->state_flags & CrewObject))) /* FIGHTERS!!! */
				 || PlotIntercept(lpEvalDesc->ObjectPtr, ShipPtr, 3, 0))
				&& !(uqm::random().next() & 3))
			{
				StarShipPtr->ship_input_state &= ~(LeftFlag | RightFlag | ThrustFlag | WeaponFlag);
				StarShipPtr->ship_input_state |= SpecialFlag;
			}
		}
	}
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <= SPECIAL_ENERGY_COST << 1)
	{
		StarShipPtr->ship_input_state &= ~WeaponFlag;
	}
}

static void
arilou_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (!(ElementPtr->state_flags & Nonsolid))
	{
		if (ElementPtr->thrust_wait == 0)
		{
			ZeroVelocityComponents(&ElementPtr->velocity);
			StarShipPtr->cur_status_flags &= ~ShipAtMaxSpeed;
		}

		if ((StarShipPtr->cur_status_flags & SpecialFlag)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy(ElementPtr, -SPECIAL_ENERGY_COST))
		{
			/* Special key is pressed; start teleport */
			ZeroVelocityComponents(&ElementPtr->velocity);
			StarShipPtr->cur_status_flags &=
				~(ShipAtMaxSpeed | LeftFlag | RightFlag | ThrustFlag | WeaponFlag);

			ElementPtr->state_flags |= Nonsolid | FiniteLife | Changing;
			ElementPtr->life_span = HYPER_LIFE;

			ElementPtr->next.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.special;
			ElementPtr->next.image.frame =
				StarShipPtr->RaceDescPtr->ship_data.special[0];

			ProcessSound(GetSound(
							 /* HYPERJUMP */
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						 ElementPtr);
			StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
	else if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		uint16_t life_span {};

		StarShipPtr->cur_status_flags =
			(StarShipPtr->cur_status_flags
			 & ~(LeftFlag | RightFlag | ThrustFlag | WeaponFlag | SpecialFlag))
			| (StarShipPtr->old_status_flags
			   & (LeftFlag | RightFlag | ThrustFlag | WeaponFlag | SpecialFlag));
		++StarShipPtr->weapon_counter;
		++StarShipPtr->special_counter;
		++StarShipPtr->energy_counter;
		++ElementPtr->turn_wait;
		++ElementPtr->thrust_wait;

		if ((life_span = ElementPtr->life_span) == NormalLife)
		{ /* Ending teleport */
			ElementPtr->state_flags &= ~(Nonsolid | FiniteLife);
			ElementPtr->state_flags |= Appearing;
			ElementPtr->current.image.farray =
				ElementPtr->next.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.ship;
			ElementPtr->current.image.frame =
				ElementPtr->next.image.frame =
					SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.ship[0],
									 StarShipPtr->ShipFacing);
			InitIntersectStartPoint(ElementPtr);
		}
		else
		{ /* Teleporting in progress */
			--life_span;
			if (life_span != 2)
			{
				if (life_span < 2)
				{
					ElementPtr->next.image.frame =
						DecFrameIndex(ElementPtr->next.image.frame);
				}
				else
				{
					ElementPtr->next.image.frame =
						IncFrameIndex(ElementPtr->next.image.frame);
				}
			}
			else
			{ // JMS: Reduce the odds of teleporting into Sa-Matra.
				if (uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE)
				{
					int32_t dist = 0;
					int32_t dx {};
					int32_t dy {};

					do
					{
						ElementPtr->next.location.x =
							WRAP_X(DISPLAY_ALIGN_X(uqm::random().next()));
						ElementPtr->next.location.y =
							WRAP_Y(DISPLAY_ALIGN_Y(uqm::random().next()));

						dx = ((int32_t)ElementPtr->next.location.x - (LogSpaceWidth() >> 1));
						dy = ((int32_t)ElementPtr->next.location.y - (LogSpaceHeight() >> 1));

						dist = sqrt(dx * dx + dy * dy);
					} while (dist < (RES_SCALE(2800)));
				}
				else
				{
					ElementPtr->next.location.x =
						WRAP_X(DISPLAY_ALIGN_X(uqm::random().next()));
					ElementPtr->next.location.y =
						WRAP_Y(DISPLAY_ALIGN_Y(uqm::random().next()));
				}
			}
		}

		ElementPtr->state_flags |= Changing;
	}
}

RACE_DESC*
init_arilou(void)
{
	if (IsHd())
	{
		arilou_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		arilou_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		arilou_desc.cyborg_control.WeaponRange = LASER_RANGE_HD >> 1;
	}
	else
	{
		arilou_desc.characteristics.max_thrust = MAX_THRUST;
		arilou_desc.characteristics.thrust_increment = THRUST_INCREMENT;
		arilou_desc.cyborg_control.WeaponRange = LASER_RANGE >> 1;
	}

	arilou_desc.preprocess_func = arilou_preprocess;
	arilou_desc.init_weapon_func = initialize_autoaim_laser;
	arilou_desc.cyborg_control.intelligence_func = arilou_intelligence;

	RACE_DESC* RaceDescPtr {&arilou_desc};

	return (RaceDescPtr);
}