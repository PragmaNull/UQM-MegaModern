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
#include "supox.h"
#include "resinst.h"

#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {12};
static constexpr auto MaxEnergy {16};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {40};
static constexpr auto ThrustIncrement {8};
static constexpr auto ThrustWait {0};
static constexpr auto TurnWait {1};
static constexpr auto ShipMass {4};

// Gob launcher
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {2};
static auto SupoxOffset() { return ResScale(23); }
static constexpr auto MissileOffset {2};
static constexpr auto MissileSpeed {DisplayToWorld(30)};
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};

// Lateral/reverse thrust
static constexpr auto SpecialEnergyCost {1};
/* Unused - uncomment below to enable. */
static constexpr auto SpecialWait {0};
/* Unused except to initialize supox_desc.special_wait */

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }

static RaceDesc supox_desc =
	{
		{/* ShipInfoDesc */
		 "blade",
		 FiresFore,
		 16, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 SupoxRaceStrings,
		 SupoxIconMaskPmapAnim,
		 SupoxMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			333 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				7468,
				9246,
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
			 SupoxBigMaskPmapAnim,
			 SupoxMedMaskPmapAnim,
			 SupoxSmlMaskPmapAnim,
		 },
		 {
			 GobBigMaskPmapAnim,
			 GobMedMaskPmapAnim,
			 GobSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {SupoxCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 SupoxVictorySong,
		 SupoxShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			(MissileSpeed * MissileLife) >> 1,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static void supox_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							   uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (StarShipPtr->special_counter || lpEvalDesc->ObjectPtr == 0)
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
	}
	else
	{
		bool LinedUp {};
		int32_t delta_x {};
		int32_t delta_y {};

		delta_x = lpEvalDesc->ObjectPtr->next.location.x
				- ShipPtr->next.location.x;
		delta_y = lpEvalDesc->ObjectPtr->next.location.y
				- ShipPtr->next.location.y;
		const uint16_t direction_angle {ARCTAN(delta_x, delta_y)};

		LinedUp = (bool)(NormalizeAngle(NormalizeAngle(direction_angle
													   - FacingToAngle(StarShipPtr->ShipFacing))
										+ Quadrant)
						 <= HalfCircle);

		if (!LinedUp
			|| lpEvalDesc->which_turn > 20
			|| NormalizeAngle(
				   lpEvalDesc->facing
				   - (FacingToAngle(StarShipPtr->ShipFacing)
					  + HalfCircle)
				   + Octant)
				   > Quadrant)
		{
			StarShipPtr->ship_input_state &= ~SpecialFlag;
		}
		else if (LinedUp && lpEvalDesc->which_turn <= 12)
		{
			StarShipPtr->ship_input_state |= SpecialFlag;
		}

		if (StarShipPtr->ship_input_state & SpecialFlag)
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
	}

	ship_intelligence(ShipPtr,
					  ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->ship_input_state & SpecialFlag)
	{
		StarShipPtr->ship_input_state |= ThrustFlag | WeaponFlag;
	}

	lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon];
	if (StarShipPtr->special_counter == 0
		&& lpEvalDesc->ObjectPtr
		&& lpEvalDesc->MoveState == MovementState::Avoid
		&& ShipPtr->turn_wait == 0)
	{
		StarShipPtr->ship_input_state &= ~ThrustFlag;
		StarShipPtr->ship_input_state |= SpecialFlag;
		if (!(StarShipPtr->cur_status_flags & (LeftFlag | RightFlag)))
		{
			StarShipPtr->ship_input_state |= 1 << ((uint8_t)uqm::random().next() & 1);
		}
		else
		{
			StarShipPtr->ship_input_state |=
				StarShipPtr->cur_status_flags & (LeftFlag | RightFlag);
		}
	}
}

static uint16_t initialize_horn(Element* ShipPtr, HELEMENT HornArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = SupoxOffset();
	MissileBlock.speed = MissileSpeedHd();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset;
	HornArray[0] = initialize_missile(&MissileBlock);
	return (1);
}

static void supox_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SpecialFlag)
		/*
			&& DeltaEnergy (ElementPtr, -SpecialEnergyCost)
*/
	)
	{
		int16_t add_facing {0};
		if (StarShipPtr->cur_status_flags & ThrustFlag)
		{
			if (ElementPtr->thrust_wait == 0)
			{
				++ElementPtr->thrust_wait;
			}

			add_facing = AngleToFacing(HalfCircle);
		}
		if (StarShipPtr->cur_status_flags & LeftFlag)
		{
			if (ElementPtr->turn_wait == 0)
			{
				++ElementPtr->turn_wait;
			}

			if (add_facing)
			{
				add_facing += AngleToFacing(Octant);
			}
			else
			{
				add_facing = -AngleToFacing(Quadrant);
			}
		}
		else if (StarShipPtr->cur_status_flags & RightFlag)
		{
			if (ElementPtr->turn_wait == 0)
			{
				++ElementPtr->turn_wait;
			}

			if (add_facing)
			{
				add_facing -= AngleToFacing(Octant);
			}
			else
			{
				add_facing = AngleToFacing(Quadrant);
			}
		}

		if (add_facing)
		{
			const uint16_t facing {StarShipPtr->ShipFacing};
			StarShipPtr->ShipFacing = NormalizeFacing(
				facing + add_facing);
			const STATUS_FLAGS thrust_status {inertial_thrust(ElementPtr)};
			StarShipPtr->cur_status_flags &=
				~(ShipAtMaxSpeed
				  | ShipBeyondMaxSpeed
				  | ShipInGravityWell);
			StarShipPtr->cur_status_flags |= thrust_status;
			StarShipPtr->ShipFacing = facing;
		}
	}
}

RaceDesc* init_supox(void)
{
	if (IsHd())
	{
		supox_desc.characteristics.max_thrust = ResScale(MaxThrust);
		supox_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		supox_desc.cyborg_control.WeaponRange = (MissileSpeedHd() * MissileLife) >> 1;
	}
	else
	{
		supox_desc.characteristics.max_thrust = MaxThrust;
		supox_desc.characteristics.thrust_increment = ThrustIncrement;
		supox_desc.cyborg_control.WeaponRange =
			(MissileSpeed * MissileLife) >> 1;
	}

	supox_desc.preprocess_func = supox_preprocess;
	supox_desc.init_weapon_func = initialize_horn;
	supox_desc.cyborg_control.intelligence_func = supox_intelligence;

	RaceDesc* RaceDescPtr {&supox_desc};

	return (RaceDescPtr);
}

} // namespace uqm
