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

#include "zoqfot.h"

#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {10};
static constexpr auto MaxEnergy {10};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {40};
static constexpr auto ThrustIncrement {10};
static constexpr auto ThrustWait {0};
static constexpr auto TurnWait {1};
static constexpr auto ShipMass {5};

// Main weapon
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {0};
static auto ZoqfotpikOffset() { return ResScale(13); }
static constexpr auto MissileOffset {0};
static constexpr auto MissileSpeed {DisplayToWorld(10)};
/* Used by the cyborg only. */
static constexpr auto MissileLife {10};
static constexpr auto MissileRange {MissileSpeed * MissileLife};
static constexpr auto MissileDamage {1};
static constexpr auto MissileHits {1};
static constexpr auto SpitWait {2};
/* Controls the main weapon color change animation's speed.
		 * The animation advances one frame every SpitWait frames. */

// Tongue
static constexpr auto SpecialEnergyCost {MaxEnergy * 3 / 4};
static constexpr auto SpecialWait {6};
static constexpr auto TongueSpeed {0};
static constexpr auto TongueHits {1};
static constexpr auto TongueDamage {12};
static auto TongueOffset() { return ResScale(4); }

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }
static auto MissileRangeHd() { return (MissileSpeedHd() * MissileLife); }

static RaceDesc zoqfotpik_desc =
	{
		{/* ShipInfoDesc */
		 "stinger",
		 ShipFlags::FiresFore,
		 6, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 ZoqfotpikRaceStrings,
		 ZoqfotpikIconMaskPmapAnim,
		 ZoqfotpikMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			320 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				3761,
				5333,
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
			 ZoqfotpikBigMaskPmapAnim,
			 ZoqfotpikMedMaskPmapAnim,
			 ZoqfotpikSmlMaskPmapAnim,
		 },
		 {
			 SpitBigMaskPmapAnim,
			 SpitMedMaskPmapAnim,
			 SpitSmlMaskPmapAnim,
		 },
		 {
			 StingerBigMaskPmapAnim,
			 StingerMedMaskPmapAnim,
			 StingerSmlMaskPmapAnim,
		 },
		 {ZoqfotpikCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 ZoqfotpikVictorySong,
		 ZoqfotpikShipSounds,
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

static void spit_preprocess(Element* ElementPtr)
{
	ElementPtr->next.image.frame =
		IncFrameIndex(ElementPtr->next.image.frame);
	/* turn_wait is abused here to control the animation speed. */
	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		uint16_t index {};
		uint16_t speed {};

		uint16_t angle {GetVelocityTravelAngle(&ElementPtr->velocity)};
		if ((index = GetFrameIndex(ElementPtr->next.image.frame)) == 1)
		{
			angle = angle + (((uint16_t)uqm::random().next() % 3) - 1);
		}
		else
		{
			index >>= 1;
		}

		speed = WorldToVelocity(DisplayToWorld(
									ResScale(GetFrameCount(ElementPtr->next.image.frame)) - index)
								<< 1);
		SetVelocityComponents(&ElementPtr->velocity,
							  (int16_t)Cosine(angle, speed),
							  (int16_t)Sine(angle, speed));

		/* turn_wait is abused here to control the animation speed. */
		ElementPtr->turn_wait = SpitWait;
		ElementPtr->state_flags |= ElementFlags::Changing;
	}
}

static uint16_t initialize_spit(Element* ShipPtr, ElementHandle SpitArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon.data();
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
	MissileBlock.pixoffs = ZoqfotpikOffset();
	MissileBlock.speed = DisplayToWorld(
							 ResScale(GetFrameCount(StarShipPtr->RaceDescPtr->ship_data.weapon[0])))
					  << 1;
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = spit_preprocess;
	MissileBlock.blast_offs = MissileOffset;
	SpitArray[0] = initialize_missile(&MissileBlock);

	if (IsHd() && SpitArray[0])
	{
		Element* SpitPtr {};

		LockElement(SpitArray[0], &SpitPtr);
		SpitPtr->IntersectControl.IntersectStamp.frame =
			DecFrameIndex(StarShipPtr->RaceDescPtr->ship_data.weapon[0]);
		UnlockElement(SpitArray[0]);
	}

	return (1);
}

static void spawn_tongue(Element* ElementPtr);

static void tongue_postprocess(Element* ElementPtr)
{
	if (ElementPtr->turn_wait)
	{
		spawn_tongue(ElementPtr);
	}
}

static void tongue_collision(Element* ElementPtr0, GfxPoint* pPt0,
							 Element* ElementPtr1, GfxPoint* pPt1)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr0, &StarShipPtr);
	if (StarShipPtr->special_counter == StarShipPtr->RaceDescPtr->characteristics.special_wait)
	{
		weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	}

	StarShipPtr->special_counter -= ElementPtr0->turn_wait;
	ElementPtr0->turn_wait = 0;
	ElementPtr0->state_flags |= ElementFlags::Nonsolid;
}

static void spawn_tongue(Element* ElementPtr)
{
	Starship* StarShipPtr {};
	MissileDesc TongueBlock;

	GetElementStarShip(ElementPtr, &StarShipPtr);
	TongueBlock.cx = ElementPtr->next.location.x;
	TongueBlock.cy = ElementPtr->next.location.y;
	TongueBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special.data();
	TongueBlock.face = TongueBlock.index = StarShipPtr->ShipFacing;
	TongueBlock.sender = ElementPtr->playerNr;
	TongueBlock.flags = ElementFlags::IgnoreSimilar;
	TongueBlock.pixoffs = 0;
	TongueBlock.speed = TongueSpeed;
	TongueBlock.hit_points = TongueHits;
	TongueBlock.damage = TongueDamage;
	TongueBlock.life = 1;
	TongueBlock.preprocess_func = 0;
	TongueBlock.blast_offs = TongueOffset();
	ElementHandle Tongue {initialize_missile(&TongueBlock)};
	if (Tongue)
	{
		Element* TonguePtr {};

		LockElement(Tongue, &TonguePtr);
		TonguePtr->postprocess_func = tongue_postprocess;
		TonguePtr->collision_func = tongue_collision;
		if (testFlag(ElementPtr->state_flags, ElementFlags::PlayerShip))
		{
			TonguePtr->turn_wait = StarShipPtr->special_counter;
		}
		else
		{
			uint16_t angle {};
			GfxRect r;

			TonguePtr->turn_wait = ElementPtr->turn_wait - 1;

			GetFrameRect(TonguePtr->current.image.frame, &r);
			const int32_t x_offs {DisplayToWorld(r.extent.width >> 1)};
			const int32_t y_offs {DisplayToWorld(r.extent.height >> 1)};

			angle = FacingToAngle(StarShipPtr->ShipFacing);
			if (angle > HalfCircle && angle < FullCircle)
			{
				TonguePtr->current.location.x -= x_offs;
			}
			else if (angle > 0 && angle < HalfCircle)
			{
				TonguePtr->current.location.x += x_offs;
			}
			if (angle < Quadrant || angle > FullCircle - Quadrant)
			{
				TonguePtr->current.location.y -= y_offs;
			}
			else if (angle > Quadrant && angle < FullCircle - Quadrant)
			{
				TonguePtr->current.location.y += y_offs;
			}
		}

		SetElementStarShip(TonguePtr, StarShipPtr);
		UnlockElement(Tongue);
		PutElement(Tongue);
	}
}

static void zoqfotpik_intelligence(Element* ShipPtr, uqstl::span<ObjectEvaluation> ObjectsOfConcern)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	bool GiveTongueJob {false};
	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Special;

		ObjectEvaluation* lpEnemyEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
		if (lpEnemyEvalDesc->ObjectPtr
			&& lpEnemyEvalDesc->which_turn <= 4
#ifdef NEVER
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SpecialEnergyCost
#endif /* NEVER */
		)
		{
			int32_t delta_x {};
			int32_t delta_y {};

			GiveTongueJob = true;

			lpEnemyEvalDesc->MoveState = MovementState::Pursue;
			delta_x = lpEnemyEvalDesc->ObjectPtr->next.location.x
					- ShipPtr->next.location.x;
			delta_y = lpEnemyEvalDesc->ObjectPtr->next.location.y
					- ShipPtr->next.location.y;
			if (StarShipPtr->ShipFacing == NormalizeFacing(AngleToFacing(ARCTAN(delta_x, delta_y))))
			{
				StarShipPtr->ship_input_state |= StatusFlags::Special;
			}
		}
	}

	++StarShipPtr->weapon_counter;
	ship_intelligence(ShipPtr, ObjectsOfConcern);
	--StarShipPtr->weapon_counter;

	if (StarShipPtr->weapon_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Weapon;
		if (!GiveTongueJob)
		{
			for (size_t ConcernCounter {ObjectsOfConcern.size()}; ConcernCounter-- > 0;)
			{
				ObjectEvaluation* const pConcern {&ObjectsOfConcern[ConcernCounter]};
				if (pConcern->ObjectPtr
					&& (ConcernCounter == IntelObjectIndex::EnemyShip
						|| (ConcernCounter == IntelObjectIndex::EnemyWeapon
							&& pConcern->MoveState != MovementState::Avoid
#ifdef NEVER
							&& !(StarShipPtr->control & STANDARD_RATING)
#endif /* NEVER */
								))
					&& ship_weapons(ShipPtr,
									pConcern->ObjectPtr, DisplayToWorld(ResScale(20))))
				{
					StarShipPtr->ship_input_state |= StatusFlags::Weapon;
					break;
				}
			}
		}
	}
}

static void zoqfotpik_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		ProcessSound(GetSound(
						 /* STICK_OUT_TONGUE */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);

		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}

	if (StarShipPtr->special_counter)
	{
		spawn_tongue(ElementPtr);
	}
}

RaceDesc* init_zoqfotpik(void)
{
	if (IsHd())
	{
		zoqfotpik_desc.characteristics.max_thrust = ResScale(MaxThrust);
		zoqfotpik_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		zoqfotpik_desc.cyborg_control.WeaponRange = MissileRangeHd();
	}
	else
	{
		zoqfotpik_desc.characteristics.max_thrust = MaxThrust;
		zoqfotpik_desc.characteristics.thrust_increment = ThrustIncrement;
		zoqfotpik_desc.cyborg_control.WeaponRange = MissileRange;
	}

	zoqfotpik_desc.postprocess_func = zoqfotpik_postprocess;
	zoqfotpik_desc.init_weapon_func = initialize_spit;
	zoqfotpik_desc.cyborg_control.intelligence_func = zoqfotpik_intelligence;

	RaceDesc* RaceDescPtr {&zoqfotpik_desc};

	return (RaceDescPtr);
}

} // namespace uqm
