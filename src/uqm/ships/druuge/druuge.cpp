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
#include "druuge.h"
#include "resinst.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {14};
static constexpr auto MaxEnergy {32};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {50};
static constexpr auto MaxThrust {20};
static constexpr auto ThrustIncrement {2};
static constexpr auto ThrustWait {1};
static constexpr auto TurnWait {4};
static constexpr auto ShipMass {5};

// Mass Driver
static constexpr auto WeaponEnergyCost {4};
static constexpr auto WeaponWait {10};
static auto DruugeOffset() { return ResScale(24); }
static auto MissileOffset() { return ResScale(6); }
static constexpr auto MissileSpeed {DisplayToWorld(30)};
static constexpr auto MissileLife {20};
static constexpr auto MissileRange {MissileSpeed * MissileLife};
static constexpr auto MissileHits {4};
static constexpr auto MissileDamage {6};
static auto RecoilVelocity() { return WorldToVelocity(DisplayToWorld(ResScale(6))); }
static auto MaxRecoilVelocity() { return (RecoilVelocity() * 4); }

// Furnace
static constexpr auto SpecialEnergyCost {16};
static constexpr auto SpecialWait {30};

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }
static auto MissileRangeHd() { return (MissileSpeedHd() * MissileLife); }

static RaceDesc druuge_desc =
	{
		{/* ShipInfoDesc */
		 "mauler",
		 FiresFore,
		 17, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 DruugeRaceStrings,
		 DruugeIconMaskPmapAnim,
		 DruugeMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			1400 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				9500,
				2792,
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
			 DruugeBigMaskPmapAnim,
			 DruugeMedMaskPmapAnim,
			 DruugeSmlMaskPmapAnim,
		 },
		 {
			 DruugeCannonBigMaskPmapAnim,
			 DruugeCannonMedMaskPmapAnim,
			 DruugeCannonSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {DruugeCaptMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 DruugeVictorySong,
		 DruugeShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			MissileRange,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static void cannon_collision(Element* ElementPtr0, GfxPoint* pPt0,
							 Element* ElementPtr1, GfxPoint* pPt1)
{
	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);

	if ((ElementPtr1->state_flags & PlayerShip)
		&& ElementPtr1->crew_level
		&& !GravityMass(ElementPtr1->mass_points + 1))
	{
		uint16_t angle {};
		int16_t cur_delta_x {};
		int16_t cur_delta_y {};
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr1, &StarShipPtr);
		StarShipPtr->cur_status_flags &=
			~(ShipAtMaxSpeed | ShipBeyondMaxSpeed);

		angle = FacingToAngle(
			GetFrameIndex(ElementPtr0->next.image.frame));
		DeltaVelocityComponents(&ElementPtr1->velocity,
								Cosine(angle, RecoilVelocity()),
								Sine(angle, RecoilVelocity()));
		GetCurrentVelocityComponents(&ElementPtr1->velocity,
									 &cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
			> (long)MaxRecoilVelocity() * (long)MaxRecoilVelocity())
		{
			angle = ARCTAN(cur_delta_x, cur_delta_y);
			SetVelocityComponents(&ElementPtr1->velocity,
								  Cosine(angle, MaxRecoilVelocity()),
								  Sine(angle, MaxRecoilVelocity()));
		}
	}
}

static uint16_t initialize_cannon(Element* ShipPtr, ElementHandle CannonArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = MissileBlock.face;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = DruugeOffset();
	MissileBlock.speed = chooseIfHd(MissileSpeed, MissileSpeedHd());
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();
	CannonArray[0] = initialize_missile(&MissileBlock);

	if (CannonArray[0])
	{
		Element* CannonPtr {};

		LockElement(CannonArray[0], &CannonPtr);
		CannonPtr->collision_func = cannon_collision;
		UnlockElement(CannonArray[0]);
	}

	return (1);
}

static void druuge_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								uint16_t ConcernCounter)
{
	uint16_t ship_flags = 0;
	Starship* StarShipPtr {};
	Starship* EnemyStarShipPtr = nullptr;

	GetElementStarShip(ShipPtr, &StarShipPtr);

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (StarShipPtr->cur_status_flags & ShipBeyondMaxSpeed)
	{
		lpEvalDesc->MoveState = MovementState::Entice;
	}
	else if (lpEvalDesc->ObjectPtr
			 && lpEvalDesc->which_turn <= ResDescale(WorldToTurn(ResScale(MissileRange) * 3 / 4)))
	{
		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		ship_flags = EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags;
		EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags &=
			~(FiresFore | FiresRight | FiresAft | FiresLeft);

		lpEvalDesc->MoveState = MovementState::Pursue;
		if (ShipPtr->thrust_wait == 0)
		{
			++ShipPtr->thrust_wait;
		}
	}
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
	if (EnemyStarShipPtr)
	{
		EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags = ship_flags;
	}

	if (!(StarShipPtr->cur_status_flags & ShipBeyondMaxSpeed)
		&& (lpEvalDesc->which_turn <= 12
			|| (ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr
				&& ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].which_turn <= 6)))
	{
		StarShipPtr->ship_input_state |= WeaponFlag;
		if (ShipPtr->thrust_wait < WeaponWait + 1)
		{
			ShipPtr->thrust_wait = WeaponWait + 1;
		}
	}


	if ((StarShipPtr->ship_input_state & WeaponFlag)
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level < WeaponEnergyCost
		&& ShipPtr->crew_level > 1)
	{
		StarShipPtr->ship_input_state |= SpecialFlag;
	}
	else
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
	}
}

static void druuge_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	/* if just fired cannon */
	if ((StarShipPtr->cur_status_flags & WeaponFlag)
		&& StarShipPtr->weapon_counter == StarShipPtr->RaceDescPtr->characteristics.weapon_wait)
	{
		uint16_t angle {};
		int32_t cur_delta_x {};
		int32_t cur_delta_y {};

		StarShipPtr->cur_status_flags &= ~ShipAtMaxSpeed;

		angle = FacingToAngle(StarShipPtr->ShipFacing) + HalfCircle;
		DeltaVelocityComponents(&ElementPtr->velocity,
								Cosine(angle, RecoilVelocity()),
								Sine(angle, RecoilVelocity()));
		GetCurrentVelocityComponentsSdword(&ElementPtr->velocity,
										   &cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
			> (long)MaxRecoilVelocity() * (long)MaxRecoilVelocity())
		{
			angle = ARCTAN(cur_delta_x, cur_delta_y);
			SetVelocityComponents(&ElementPtr->velocity,
								  Cosine(angle, MaxRecoilVelocity()),
								  Sine(angle, MaxRecoilVelocity()));
		}
	}
}

static void druuge_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->cur_status_flags & SpecialFlag)
	{
		if (StarShipPtr->special_counter
			|| ElementPtr->crew_level == 1
			|| StarShipPtr->RaceDescPtr->ship_info.energy_level
				   == StarShipPtr->RaceDescPtr->ship_info.max_energy)
		{
			StarShipPtr->cur_status_flags &= ~SpecialFlag;
		}
		else
		{
			ProcessSound(GetSound(
							 /* BURN UP CREW */
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						 ElementPtr);

			DeltaCrew(ElementPtr, -1);
			DeltaEnergy(ElementPtr, SpecialEnergyCost);

			StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
}

RaceDesc* init_druuge(void)
{
	if (IsHd())
	{
		druuge_desc.characteristics.max_thrust = ResScale(MaxThrust);
		druuge_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		druuge_desc.cyborg_control.WeaponRange = MissileRangeHd();
	}
	else
	{
		druuge_desc.characteristics.max_thrust = MaxThrust;
		druuge_desc.characteristics.thrust_increment = ThrustIncrement;
		druuge_desc.cyborg_control.WeaponRange = MissileRange;
	}

	druuge_desc.preprocess_func = druuge_preprocess;
	druuge_desc.postprocess_func = druuge_postprocess;
	druuge_desc.init_weapon_func = initialize_cannon;
	druuge_desc.cyborg_control.intelligence_func = druuge_intelligence;

	RaceDesc* RaceDescPtr {&druuge_desc};

	return (RaceDescPtr);
}

} // namespace uqm
