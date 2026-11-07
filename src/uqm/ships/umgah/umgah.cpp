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
#include "umgah.h"
#include "resinst.h"

#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {10};
static constexpr auto MaxEnergy {30};
static constexpr auto EnergyRegeneration {MaxEnergy};
static constexpr auto EnergyWait {150};
static constexpr auto MaxThrust {18};
static constexpr auto ThrustIncrement {6};
static constexpr auto ThrustWait {3};
static constexpr auto TurnWait {4};
static constexpr auto ShipMass {1};

// Antimatter cone
static constexpr auto WeaponEnergyCost {0};
static constexpr auto WeaponWait {0};
static constexpr auto UmgahOffset {0};
static constexpr auto ConeOffset {0};
static constexpr auto ConeSpeed {0};
static constexpr auto ConeHits {100};
static constexpr auto ConeDamage {1};
static constexpr auto ConeLife {1};

// Retropropulsion
static constexpr auto SpecialEnergyCost {1};
static constexpr auto SpecialWait {2};
static auto JumpDist() { return DisplayToWorld(ResScale(40)); }

static RaceDesc umgah_desc =
	{
		{/* ShipInfoDesc */
		 "drone",
		 FiresFore | ImmediateWeapon,
		 7, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 UmgahRaceStrings,
		 UmgahIconMaskPmapAnim,
		 UmgahMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			833 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				1798,
				6000,
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
			 UmgahBigMaskPmapAnim,
			 UmgahMedMaskPmapAnim,
			 UmgahSmlMaskPmapAnim,
		 },
		 {
			 SpritzMaskPmapAnim,
			 NullResource,
			 NullResource,
		 },
		 {
			 ConeBigMaskAnim,
			 ConeMedMaskAnim,
			 ConeSmlMaskAnim,
		 },
		 {UmgahCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 UmgahVictorySong,
		 UmgahShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			(LongRangeWeapon << 2),
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};


// Private per-instance ship data
struct UmgahData
{
	uint16_t prevFacing;
};

// Local typedef
typedef UmgahData CustomShipData_t;

// Retrieve race-specific ship data from a race desc
static CustomShipData_t* GetCustomShipData(RaceDesc* pRaceDesc)
{
	return (CustomShipData_t*)pRaceDesc->data;
}

// Set the race-specific data in a race desc
// (Re)Allocates its own storage for the data.
static void SetCustomShipData(RaceDesc* pRaceDesc, const CustomShipData_t* data)
{
	if (pRaceDesc->data == data)
	{
		return; // no-op
	}

	if (pRaceDesc->data) // Out with the old
	{
		delete static_cast<CustomShipData_t*>(pRaceDesc->data);
		pRaceDesc->data = nullptr;
	}

	if (data) // In with the new
	{
		pRaceDesc->data = new CustomShipData_t {*data};
	}
}


static void cone_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	StarShipPtr->RaceDescPtr->ship_data.special[0] =
		SetRelFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0],
						 AngleToFacing(FullCircle));

	ElementPtr->state_flags |= Appearing;
}

static void cone_collision(Element* ElementPtr0, GfxPoint* pPt0,
						   Element* ElementPtr1, GfxPoint* pPt1)
{
	ElementHandle hBlastElement {weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1)};
	if (hBlastElement)
	{
		RemoveElement(hBlastElement);
		FreeElement(hBlastElement);

		ElementPtr0->state_flags &= ~Disappearing;
	}
}

static void umgah_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							   uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};
	Starship* EnemyStarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == MovementState::Entice)
	{
		if (lpEvalDesc->which_turn > 3
			|| (StarShipPtr->old_status_flags & SpecialFlag))
		{
			lpEvalDesc->ObjectPtr = 0;
		}
		else if ((lpEvalDesc->ObjectPtr->state_flags & FiniteLife)
				 && !(lpEvalDesc->ObjectPtr->state_flags & CrewObject))
		{
			lpEvalDesc->MoveState = MovementState::Avoid;
		}
		else
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
	}

	lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyShip];
	if (StarShipPtr->special_counter
		|| ObjectsOfConcern[*IntelObjectIndex::GravityMass].ObjectPtr
		|| lpEvalDesc->ObjectPtr == 0)
	{
		StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = CloseRangeWeapon;
		ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (lpEvalDesc->which_turn < 16)
		{
			StarShipPtr->ship_input_state |= WeaponFlag;
		}
		StarShipPtr->ship_input_state &= ~SpecialFlag;
	}
	else
	{
		uint8_t this_turn {};
		bool EnemyBehind {};
		bool EnoughJuice {};

		if (lpEvalDesc->which_turn >= 0xFF + 1)
		{
			this_turn = 0xFF;
		}
		else
		{
			this_turn = (uint8_t)lpEvalDesc->which_turn;
		}

		EnoughJuice = (bool)((WorldToTurn(ResDescale(
								 JumpDist() * StarShipPtr->RaceDescPtr->ship_info.energy_level
								 / SpecialEnergyCost)))
							 > this_turn);
		const int32_t delta_x {lpEvalDesc->ObjectPtr->next.location.x - ShipPtr->next.location.x};
		const int32_t delta_y {lpEvalDesc->ObjectPtr->next.location.y - ShipPtr->next.location.y};
		EnemyBehind = (bool)(NormalizeAngle(
								 ARCTAN(delta_x, delta_y)
								 - (FacingToAngle(StarShipPtr->ShipFacing)
									+ HalfCircle)
								 + (Octant + (Octant >> 2)))
							 <= ((Octant + (Octant >> 2)) << 1));

		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (EnoughJuice
			&& ((StarShipPtr->old_status_flags & SpecialFlag)
				|| EnemyBehind
				|| (this_turn > 6
					&& Maneuverability(
						   &EnemyStarShipPtr->RaceDescPtr->cyborg_control)
						   <= ResolutionCompensated(SlowShip))
				|| (this_turn >= 16 && this_turn <= 24)))
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = (LongRangeWeapon << 3);
		}
		else
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = CloseRangeWeapon;
		}

		ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange == CloseRangeWeapon)
		{
			StarShipPtr->ship_input_state &= ~SpecialFlag;
		}
		else
		{
			bool LinedUp {};

			StarShipPtr->ship_input_state &= ~ThrustFlag;
			LinedUp = (bool)(ShipPtr->turn_wait == 0
							 && !(StarShipPtr->old_status_flags & (LeftFlag | RightFlag)));
			if (((StarShipPtr->old_status_flags & SpecialFlag)
				 && this_turn <= StarShipPtr->RaceDescPtr->characteristics.special_wait)
				|| (!(StarShipPtr->old_status_flags & SpecialFlag)
					&& EnemyBehind && (LinedUp || this_turn < 16)))
			{
				StarShipPtr->ship_input_state |= SpecialFlag;
				StarShipPtr->RaceDescPtr->characteristics.special_wait = this_turn;

				/* don't want him backing straight into ship */
				if (this_turn <= 8 && LinedUp)
				{
					if (uqm::random().next() & 1)
					{
						StarShipPtr->ship_input_state |= LeftFlag;
					}
					else
					{
						StarShipPtr->ship_input_state |= RightFlag;
					}
				}
			}
			else if (StarShipPtr->old_status_flags & SpecialFlag)
			{
				StarShipPtr->ship_input_state &= ~(SpecialFlag | LeftFlag | RightFlag);
				StarShipPtr->ship_input_state |= ThrustFlag;
			}
		}

		if (this_turn < 16 && !EnemyBehind)
		{
			StarShipPtr->ship_input_state |= WeaponFlag;
		}
	}

	if (!(StarShipPtr->ship_input_state & SpecialFlag))
	{
		StarShipPtr->RaceDescPtr->characteristics.special_wait = 0xFF;
	}
}

static uint16_t initialize_cone(Element* ShipPtr, ElementHandle ConeArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = UmgahOffset;
	MissileBlock.speed = ConeSpeed;
	MissileBlock.hit_points = ConeHits;
	MissileBlock.damage = ConeDamage;
	MissileBlock.life = ConeLife;
	MissileBlock.preprocess_func = cone_preprocess;
	MissileBlock.blast_offs = ConeOffset;

	// This func is called every frame while the player is holding down WeaponFlag
	// Don't reset the cone FrameRef to the first image every time
	const UmgahData* umgahData {GetCustomShipData(StarShipPtr->RaceDescPtr)};
	if (!umgahData || StarShipPtr->ShipFacing != umgahData->prevFacing)
	{
		const UmgahData shipData = {StarShipPtr->ShipFacing};

		SetCustomShipData(StarShipPtr->RaceDescPtr, &shipData);

		StarShipPtr->RaceDescPtr->ship_data.special[0] =
			SetAbsFrameIndex(
				StarShipPtr->RaceDescPtr->ship_data.special[0],
				StarShipPtr->ShipFacing);
	}

	MissileBlock.index = GetFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0]);
	ConeArray[0] = initialize_missile(&MissileBlock);

	if (ConeArray[0])
	{
		Element* ConePtr {};

		LockElement(ConeArray[0], &ConePtr);
		ConePtr->collision_func = cone_collision;
		ConePtr->state_flags &= ~Appearing;
		ConePtr->next = ConePtr->current;
		InitIntersectStartPoint(ConePtr);
		InitIntersectEndPoint(ConePtr);
		ConePtr->IntersectControl.IntersectStamp.frame =
			StarShipPtr->RaceDescPtr->ship_data.special[0];
		UnlockElement(ConeArray[0]);
	}

	return (1);
}

static void umgah_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->special_counter > 0)
	{
		StarShipPtr->special_counter = 0;

		ZeroVelocityComponents(&ElementPtr->velocity);
	}
}

static void umgah_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);

	if (ElementPtr->state_flags & Appearing)
	{
		// Reset the value just in case
		SetCustomShipData(StarShipPtr->RaceDescPtr, nullptr);
	}
	else
	{
		if (ElementPtr->thrust_wait == 0
			&& (StarShipPtr->cur_status_flags & SpecialFlag)
			&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
		{
			uint16_t facing {};

			ProcessSound(GetSound(
							 /* ZIP_BACKWARDS */
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						 ElementPtr);
			facing = FacingToAngle(StarShipPtr->ShipFacing) + HalfCircle;
			DeltaVelocityComponents(&ElementPtr->velocity,
									Cosine(facing, WorldToVelocity(JumpDist())),
									Sine(facing, WorldToVelocity(JumpDist())));
			StarShipPtr->cur_status_flags &=
				~(ShipAtMaxSpeed | ShipBeyondMaxSpeed);

			StarShipPtr->special_counter = SpecialWait;
		}
	}
}

static void uninit_umgah(RaceDesc* pRaceDesc)
{
	SetCustomShipData(pRaceDesc, nullptr);
}

RaceDesc* init_umgah(void)
{
	if (IsHd())
	{
		umgah_desc.characteristics.max_thrust = ResScale(MaxThrust);
		umgah_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		umgah_desc.cyborg_control.WeaponRange = (LongRangeWeaponHd << 2);
	}
	else
	{
		umgah_desc.characteristics.max_thrust = MaxThrust;
		umgah_desc.characteristics.thrust_increment = ThrustIncrement;
		umgah_desc.cyborg_control.WeaponRange = (LongRangeWeapon << 2);
	}

	umgah_desc.uninit_func = uninit_umgah;
	umgah_desc.preprocess_func = umgah_preprocess;
	umgah_desc.postprocess_func = umgah_postprocess;
	umgah_desc.init_weapon_func = initialize_cone;
	umgah_desc.cyborg_control.intelligence_func = umgah_intelligence;

	RaceDesc* RaceDescPtr {&umgah_desc};

	return (RaceDescPtr);
}

} // namespace uqm
