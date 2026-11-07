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

#include "utwig.h"

#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/globdata.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
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

static auto MissileSpeed() { return DisplayToWorld(ResScale(30)); }
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};
static auto MissileOffset() { return ResScale(1); }
static auto LaunchXoffs0() { return DisplayToWorld(ResScale(5)); }
static auto LaunchYoffs0() { return -DisplayToWorld(chooseIfHd(18, 63)); }
static auto LaunchXoffs1() { return DisplayToWorld(chooseIfHd(13, 48)); }
static auto LaunchYoffs1() { return -DisplayToWorld(chooseIfHd(9, 25)); }
static auto LaunchXoffs2() { return DisplayToWorld(chooseIfHd(17, 64)); }
static auto LaunchYoffs2() { return -DisplayToWorld(chooseIfHd(4, 11)); }

// Shield
static constexpr auto SpecialEnergyCost {1};
static constexpr auto SpecialWait {12};

static RaceDesc utwig_desc =
	{
		{/* ShipInfoDesc */
		 "jugger",
		 ShipFlags::FiresFore | ShipFlags::PointDefense | ShipFlags::ShieldDefense,
		 22, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy >> 1, MaxEnergy,
		 UtwigRaceStrings,
		 UtwigIconMaskPmapAnim,
		 UtwigMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
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

static uint16_t initialize_lance(Element* ShipPtr, ElementHandle WeaponArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon.data();
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
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
		angle = FacingToAngle(MissileBlock.face) + Quadrant;
		cos1cos0 = Cosine(angle, cos0);
		sin1sin0 = Sine(angle, sin0);
		sin1cos0 = Sine(angle, cos0);
		cos1sin0 = Cosine(angle, sin0);

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

static void utwig_intelligence(Element* ShipPtr, uqstl::span<ObjectEvaluation> ObjectsOfConcern)
{
	int16_t ShieldStatus {};
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
	{
		ShieldStatus = 0;
	}
	else
	{
		ShieldStatus = -1;
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == MovementState::Entice)
		{
			ShieldStatus = 0;
			if (!(testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::FiniteLife)))
			{
				lpEvalDesc->MoveState = MovementState::Pursue;
			}
			else if (lpEvalDesc->ObjectPtr->mass_points
					 || (testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::CrewObject)))
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
					lpEvalDesc->MoveState = MovementState::Pursue;
				}
				ShieldStatus = 1;
			}
		}
	}

	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Special;
		if (ShieldStatus)
		{
			if ((ShieldStatus > 0 || lpEvalDesc->ObjectPtr)
				&& lpEvalDesc->which_turn <= 2
				&& (ShieldStatus > 0
					|| (testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::PlayerShip)) /* means IMMEDIATE WeaponFlag */
					|| PlotIntercept(lpEvalDesc->ObjectPtr,
									 ShipPtr, 2, 0))
				&& (uqm::random().next() & 3))
			{
				StarShipPtr->ship_input_state |= StatusFlags::Special;
				StarShipPtr->ship_input_state &= ~StatusFlags::Weapon;
			}

			lpEvalDesc->ObjectPtr = 0;
		}
	}

	if (StarShipPtr->RaceDescPtr->ship_info.energy_level
		&& (lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyShip])->ObjectPtr)
	{
		Starship* EnemyStarShipPtr {};

		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(testFlag(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags, ShipFlags::ImmediateWeapon)))
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
	}
	ship_intelligence(ShipPtr, ObjectsOfConcern);
}

static void utwig_collision(Element* ElementPtr0, GfxPoint* pPt0,
							Element* ElementPtr1, GfxPoint* pPt1)
{
	if (ElementPtr0->life_span > NormalLife
		&& (testFlag(ElementPtr1->state_flags, ElementFlags::FiniteLife))
		&& ElementPtr1->mass_points)
	{
		ElementPtr0->life_span += ElementPtr1->mass_points;
	}

	collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void utwig_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	if (testFlag(ElementPtr->state_flags, ElementFlags::Appearing))
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

	if (!(testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special)))
	{
		StarShipPtr->special_counter = 0;
	}
	else if (StarShipPtr->special_counter % (SpecialWait >> 1) == 0)
	{
		if (!DeltaEnergy(ElementPtr, -SpecialEnergyCost))
		{
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &=
				~(ShipFlags::PointDefense | ShipFlags::ShieldDefense);
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

	Primitive* lpPrim {&DisplayArray[ElementPtr->PrimIndex]};
	if (StarShipPtr->special_counter == 0)
	{
		// The shield is off.
		SetPrimColor(lpPrim,
					 BuildColor(MakeRgb15(0x1F, 0x1C, 0x00), 0x78));
		ElementPtr->colorCycleIndex = 0;
		ElementPtr->life_span = NormalLife;
		SetPrimType(lpPrim, PrimitiveType::Stamp);
	}
	else
	{
		// The shield is on.

		/* Originally, this table also contained the now commented out
		 * entries. It then used some if statements to skip over these.
		 * The current behaviour is the same as the old behavior,
		 * but I am not sure that the old behavior was intended. - SvdB
		 */
		const static uqstl::array<Color, 7> colorTable {
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
		SetPrimType(lpPrim, PrimitiveType::StampFill);
	}
}

RaceDesc* init_utwig(void)
{
	if (IsHd())
	{
		utwig_desc.characteristics.max_thrust = ResScale(MaxThrust);
		utwig_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
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

	RaceDesc* RaceDescPtr {&utwig_desc};

	return (RaceDescPtr);
}

} // namespace uqm
