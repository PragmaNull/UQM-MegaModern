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
static constexpr auto MaxCrew {20};
static constexpr auto MaxEnergy {20};
static constexpr auto EnergyRegeneration {0};
static constexpr auto EnergyWait {255};
static constexpr auto MaxThrust {36};
static constexpr auto ThrustIncrement {6};
static constexpr auto ThrustWait {6};
static constexpr auto TurnWait {1};
static constexpr auto ShipMass {8};

// Weapon
static constexpr auto WeaponEnergyCost {0};
static constexpr auto WeaponWait {7};

static auto MissileSpeed() { return DISPLAY_TO_WORLD(RES_SCALE(30)); }
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};
static auto MissileOffset() { return RES_SCALE(1); }
static auto LaunchXoffs0() { return DISPLAY_TO_WORLD(RES_SCALE(5)); }
static auto LaunchYoffs0() { return -DISPLAY_TO_WORLD(chooseIfHd(18, 63)); }
static auto LaunchXoffs1() { return DISPLAY_TO_WORLD(chooseIfHd(13, 48)); }
static auto LaunchYoffs1() { return -DISPLAY_TO_WORLD(chooseIfHd(9, 25)); }
static auto LaunchXoffs2() { return DISPLAY_TO_WORLD(chooseIfHd(17, 64)); }
static auto LaunchYoffs2() { return -DISPLAY_TO_WORLD(chooseIfHd(4, 11)); }

// Shield
static constexpr auto SpecialEnergyCost {1};
static constexpr auto SpecialWait {12};

static RACE_DESC utwig_desc =
	{
		{/* SHIP_INFO */
		 "jugger",
		 FiresFore | PointDefense | ShieldDefense,
		 22, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy >> 1, MaxEnergy,
		 UtwigRaceStrings,
		 UtwigIconMaskPmapAnim,
		 UtwigMiconMaskPmapAnim,
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
			MaxThrust,
			ThrustIncrement,
			EnergyRegeneration,
			WeaponEnergyCost,
			SpecialEnergyCost,
			EnergyWait,
			TurnWait,
			ThrustWait,
			WeaponWait,
			SpecialWait,
			ShipMass,
		 },
		{{
			 UtwigBigMaskPmapAnim,
			 UtwigMedMaskPmapAnim,
			 UtwigSmlMaskPmapAnim,
		 },
		 {
			 LanceBigMaskPmapAnim,
			 LanceMedMaskPmapAnim,
			 LanceSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {UtwigCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 UtwigVictorySong,
		 UtwigShipSounds,
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
	MissileBlock.speed = MissileSpeed();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();
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
				cos0 = LaunchXoffs0();
				sin0 = LaunchYoffs0();
				break;
			case 1:
				cos0 = LaunchXoffs1();
				sin0 = LaunchYoffs1();
				break;
			case 2:
				cos0 = LaunchXoffs2();
				sin0 = LaunchYoffs2();
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
	else if (StarShipPtr->special_counter % (SpecialWait >> 1) == 0)
	{
		if (!DeltaEnergy(ElementPtr, -SpecialEnergyCost))
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
		utwig_desc.characteristics.max_thrust = RES_SCALE(MaxThrust);
		utwig_desc.characteristics.thrust_increment = RES_SCALE(ThrustIncrement);
		utwig_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd;
	}
	else
	{
		utwig_desc.characteristics.max_thrust = MaxThrust;
		utwig_desc.characteristics.thrust_increment = ThrustIncrement;
		utwig_desc.cyborg_control.WeaponRange = CloseRangeWeapon;
	}

	utwig_desc.preprocess_func = utwig_preprocess;
	utwig_desc.init_weapon_func = initialize_lance;
	utwig_desc.cyborg_control.intelligence_func = utwig_intelligence;

	RACE_DESC* RaceDescPtr {&utwig_desc};

	return (RaceDescPtr);
}
