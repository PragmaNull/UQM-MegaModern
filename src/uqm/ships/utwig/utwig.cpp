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
#include "utwig.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 0
#define ENERGY_WAIT 255
#define MAX_THRUST 36
#define THRUST_INCREMENT 6
#define THRUST_WAIT 6
#define TURN_WAIT 1
#define SHIP_MASS 8

// Weapon
#define WEAPON_ENERGY_COST 0
#define WEAPON_WAIT 7
#define UTWIG_OFFSET 9
#define MISSILE_SPEED DISPLAY_TO_WORLD(RES_SCALE(30))
#define MISSILE_LIFE 10
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define MISSILE_OFFSET RES_SCALE(1)
#define LAUNCH_XOFFS0 DISPLAY_TO_WORLD(RES_SCALE(5))
#define LAUNCH_YOFFS0 -DISPLAY_TO_WORLD(chooseIfHd(18, 63))
#define LAUNCH_XOFFS1 DISPLAY_TO_WORLD(chooseIfHd(13, 48))
#define LAUNCH_YOFFS1 -DISPLAY_TO_WORLD(chooseIfHd(9, 25))
#define LAUNCH_XOFFS2 DISPLAY_TO_WORLD(chooseIfHd(17, 64))
#define LAUNCH_YOFFS2 -DISPLAY_TO_WORLD(chooseIfHd(4, 11))

// Shield
#define SPECIAL_ENERGY_COST 1
#define SPECIAL_WAIT 12

static RACE_DESC utwig_desc =
	{
		{/* SHIP_INFO */
		 "jugger",
		 FiresFore | PointDefense | ShieldDefense,
		 22, /* Super Melee cost */
		 MAX_CREW, MAX_CREW,
		 MAX_ENERGY >> 1, MAX_ENERGY,
		 UTWIG_RACE_STRINGS,
		 UTWIG_ICON_MASK_PMAP_ANIM,
		 UTWIG_MICON_MASK_PMAP_ANIM,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
			666 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				8534,
				8797,
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
			 UTWIG_BIG_MASK_PMAP_ANIM,
			 UTWIG_MED_MASK_PMAP_ANIM,
			 UTWIG_SML_MASK_PMAP_ANIM,
		 },
		 {
			 LANCE_BIG_MASK_PMAP_ANIM,
			 LANCE_MED_MASK_PMAP_ANIM,
			 LANCE_SML_MASK_PMAP_ANIM,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {UTWIG_CAPTAIN_MASK_PMAP_ANIM,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 UTWIG_VICTORY_SONG,
		 UTWIG_SHIP_SOUNDS,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			CloseRangeWeapon,
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
initialize_lance(ELEMENT* ShipPtr, HELEMENT WeaponArray[])
{
	STARSHIP* StarShipPtr {};
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	MissileBlock.pixoffs = 0;

	for (uint16_t i {0}; i < 3; ++i)
	{
		uint16_t angle {};
		int16_t sin0 = 0;
		int16_t cos0 = 0;
		int16_t sin1sin0 {};
		int16_t cos1sin0 {};
		int16_t cos1cos0 {};
		int16_t sin1cos0 {};

		switch (i)
		{
			case 0:
				cos0 = LAUNCH_XOFFS0;
				sin0 = LAUNCH_YOFFS0;
				break;
			case 1:
				cos0 = LAUNCH_XOFFS1;
				sin0 = LAUNCH_YOFFS1;
				break;
			case 2:
				cos0 = LAUNCH_XOFFS2;
				sin0 = LAUNCH_YOFFS2;
				break;
		}
		angle = FACING_TO_ANGLE(MissileBlock.face) + Quadrant;
		cos1cos0 = COSINE(angle, cos0);
		sin1sin0 = SINE(angle, sin0);
		sin1cos0 = SINE(angle, cos0);
		cos1sin0 = COSINE(angle, sin0);

		cos0 = cos1cos0 - sin1sin0;
		sin0 = sin1cos0 + cos1sin0;
		MissileBlock.cx = ShipPtr->next.location.x + cos0;
		MissileBlock.cy = ShipPtr->next.location.y + sin0;
		WeaponArray[(i << 1)] = initialize_missile(&MissileBlock);

		cos0 = -cos1cos0 - sin1sin0;
		sin0 = -sin1cos0 + cos1sin0;
		MissileBlock.cx = ShipPtr->next.location.x + cos0;
		MissileBlock.cy = ShipPtr->next.location.y + sin0;
		WeaponArray[(i << 1) + 1] = initialize_missile(&MissileBlock);
	}

	return (6);
}

static void
utwig_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
				   uint16_t ConcernCounter)
{
	int16_t ShieldStatus {};
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	EVALUATE_DESC* lpEvalDesc {&ObjectsOfConcern[ENEMY_WEAPON_INDEX]};
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
	{
		ShieldStatus = 0;
	}
	else
	{
		ShieldStatus = -1;
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
		{
			ShieldStatus = 0;
			if (!(lpEvalDesc->ObjectPtr->state_flags & FiniteLife))
			{
				lpEvalDesc->MoveState = PURSUE;
			}
			else if (lpEvalDesc->ObjectPtr->mass_points
					 || (lpEvalDesc->ObjectPtr->state_flags & CrewObject))
			{
				if ((lpEvalDesc->which_turn >>= 1) == 0)
				{
					lpEvalDesc->which_turn = 1;
				}

				if (lpEvalDesc->ObjectPtr->mass_points)
				{
					lpEvalDesc->ObjectPtr = 0;
				}
				else
				{
					lpEvalDesc->MoveState = PURSUE;
				}
				ShieldStatus = 1;
			}
		}
	}

	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
		if (ShieldStatus)
		{
			if ((ShieldStatus > 0 || lpEvalDesc->ObjectPtr)
				&& lpEvalDesc->which_turn <= 2
				&& (ShieldStatus > 0
					|| (lpEvalDesc->ObjectPtr->state_flags
						& PlayerShip) /* means IMMEDIATE WeaponFlag */
					|| PlotIntercept(lpEvalDesc->ObjectPtr,
									 ShipPtr, 2, 0))
				&& (uqm::random().next() & 3))
			{
				StarShipPtr->ship_input_state |= SpecialFlag;
				StarShipPtr->ship_input_state &= ~WeaponFlag;
			}

			lpEvalDesc->ObjectPtr = 0;
		}
	}

	if (StarShipPtr->RaceDescPtr->ship_info.energy_level
		&& (lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX])->ObjectPtr)
	{
		STARSHIP* EnemyStarShipPtr {};

		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
			  & ImmediateWeapon))
		{
			lpEvalDesc->MoveState = PURSUE;
		}
	}
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
}

static void
utwig_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
				ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	if (ElementPtr0->life_span > NormalLife
		&& (ElementPtr1->state_flags & FiniteLife)
		&& ElementPtr1->mass_points)
	{
		ElementPtr0->life_span += ElementPtr1->mass_points;
	}

	collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void
utwig_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	if (ElementPtr->state_flags & Appearing)
	{
		ElementPtr->collision_func = utwig_collision;
	}

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (ElementPtr->life_span > (NormalLife + 1))
	{
		DeltaEnergy(ElementPtr,
					ElementPtr->life_span - (NormalLife + 1));

		ProcessSound(GetSound(
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);
	}

	if (!(StarShipPtr->cur_status_flags & SpecialFlag))
	{
		StarShipPtr->special_counter = 0;
	}
	else if (StarShipPtr->special_counter % (SPECIAL_WAIT >> 1) == 0)
	{
		if (!DeltaEnergy(ElementPtr, -SPECIAL_ENERGY_COST))
		{
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &=
				~(PointDefense | ShieldDefense);
		}
		else if (StarShipPtr->special_counter == 0)
		{
			StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
			ProcessSound(GetSound(
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
						 ElementPtr);
		}
	}

	PRIMITIVE* lpPrim {&DisplayArray[ElementPtr->PrimIndex]};
	if (StarShipPtr->special_counter == 0)
	{
		// The shield is off.
		SetPrimColor(lpPrim,
					 BuildColor(MakeRgb15(0x1F, 0x1C, 0x00), 0x78));
		ElementPtr->colorCycleIndex = 0;
		ElementPtr->life_span = NormalLife;
		SetPrimType(lpPrim, STAMP_PRIM);
	}
	else
	{
		// The shield is on.

		/* Originally, this table also contained the now commented out
		 * entries. It then used some if statements to skip over these.
		 * The current behaviour is the same as the old behavior,
		 * but I am not sure that the old behavior was intended. - SvdB
		 */
		const static Color colorTable[] =
			{
				BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7a),
				//BuildColor (MakeRgb15 (0x1F, 0x11, 0x00), 0x7b),
				BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7c),
				//BuildColor (MakeRgb15 (0x1F, 0x0A, 0x00), 0x7d),
				BuildColor(MakeRgb15(0x1F, 0x07, 0x00), 0x7e),
				//BuildColor (MakeRgb15 (0x1F, 0x03, 0x00), 0x7f),
				BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x2a),
				//BuildColor (MakeRgb15 (0x1B, 0x00, 0x00), 0x2b),
				BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2c),
				BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2d),
				BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2e),
				//BuildColor (MakeRgb15 (0x0B, 0x00, 0x00), 0x2f),
			};
		const size_t colorTableCount = std::size(colorTable);

		if (StarShipPtr->weapon_counter == 0)
		{
			++StarShipPtr->weapon_counter;
		}

		// colorCycleIndex is actually 1 higher than the entry in colorTable
		// which is currently used, as it is 0 when the shield is off,
		// and we don't want to skip over the first entry of the table.
		ElementPtr->colorCycleIndex++;
		if (ElementPtr->colorCycleIndex == colorTableCount + 1)
		{
			ElementPtr->colorCycleIndex = 1;
		}

		SetPrimColor(lpPrim, colorTable[ElementPtr->colorCycleIndex - 1]);

		ElementPtr->life_span = NormalLife + 1;
		SetPrimType(lpPrim, STAMPFILL_PRIM);
	}
}

RACE_DESC*
init_utwig(void)
{
	if (IsHd())
	{
		utwig_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		utwig_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		utwig_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd;
	}
	else
	{
		utwig_desc.characteristics.max_thrust = MAX_THRUST;
		utwig_desc.characteristics.thrust_increment = THRUST_INCREMENT;
		utwig_desc.cyborg_control.WeaponRange = CloseRangeWeapon;
	}

	utwig_desc.preprocess_func = utwig_preprocess;
	utwig_desc.init_weapon_func = initialize_lance;
	utwig_desc.cyborg_control.intelligence_func = utwig_intelligence;

	RACE_DESC* RaceDescPtr {&utwig_desc};

	return (RaceDescPtr);
}
