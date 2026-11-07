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
#include "yehat.h"
#include "resinst.h"

#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {20};
static constexpr auto MaxEnergy {10};
static constexpr auto EnergyRegeneration {2};
static constexpr auto EnergyWait {6};
static constexpr auto MaxThrust {30};
static constexpr auto ThrustIncrement {6};
static constexpr auto ThrustWait {2};
static constexpr auto TurnWait {2};
static constexpr auto ShipMass {3};

// Twin Pulse Cannon
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {0};
static auto YehatOffset() { return ResScale(16); }
static auto LaunchOffs() { return DisplayToWorld(ResScale(8)); }
static constexpr auto MissileSpeed {DisplayToWorld(20)};
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};
static auto MissileOffset() { return ResScale(1); }

// Force Shield
static constexpr auto SpecialEnergyCost {3};
static constexpr auto SpecialWait {2};
static constexpr auto ShieldLife {10};

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }

static RaceDesc yehat_desc =
	{
		{/* ShipInfoDesc */
		 "terminator",
		 FiresFore | ShieldDefense,
		 23, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 YehatRaceStrings,
		 YehatIconMaskPmapAnim,
		 YehatMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			750 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				4970,
				40,
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
			 YehatBigMaskPmapAnim,
			 YehatMedMaskPmapAnim,
			 YehatSmlMaskPmapAnim,
		 },
		 {
			 YehatCannonBigMaskPmapAnim,
			 YehatCannonMedMaskPmapAnim,
			 YehatCannonSmlMaskPmapAnim,
		 },
		 {
			 ShieldBigMaskAnim,
			 ShieldMedMaskAnim,
			 ShieldSmlMaskAnim,
		 },
		 {YehatCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 YehatVictorySong,
		 YehatShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			MissileSpeed * MissileLife / 3,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static uint16_t initialize_standard_missiles(Element* ShipPtr, HELEMENT MissileArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = YehatOffset();
	MissileBlock.speed = MissileSpeedHd();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();

	const int32_t offs_x {-Sine(FacingToAngle(MissileBlock.face), LaunchOffs())};
	const int32_t offs_y {Cosine(FacingToAngle(MissileBlock.face), LaunchOffs())};

	MissileBlock.cx = ShipPtr->next.location.x + offs_x;
	MissileBlock.cy = ShipPtr->next.location.y + offs_y;
	MissileArray[0] = initialize_missile(&MissileBlock);

	MissileBlock.cx = ShipPtr->next.location.x - offs_x;
	MissileBlock.cy = ShipPtr->next.location.y - offs_y;
	MissileArray[1] = initialize_missile(&MissileBlock);

	return (2);
}

static void yehat_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							   uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	int16_t ShieldStatus {-1};
	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == MovementState::Entice)
	{
		ShieldStatus = 0;
		if (!(lpEvalDesc->ObjectPtr->state_flags & (FiniteLife | CrewObject)))
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
		else if (lpEvalDesc->ObjectPtr->mass_points
				 || (lpEvalDesc->ObjectPtr->state_flags & CrewObject))
		{
			if (!(lpEvalDesc->ObjectPtr->state_flags & FiniteLife))
			{
				lpEvalDesc->which_turn <<= 1;
			}
			else
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
			}
			ShieldStatus = 1;
		}
	}

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
		if (ShieldStatus)
		{
			if (ShipPtr->life_span <= NormalLife + 1
				&& (ShieldStatus > 0 || lpEvalDesc->ObjectPtr)
				&& lpEvalDesc->which_turn <= 2
				&& (ShieldStatus > 0
					|| (lpEvalDesc->ObjectPtr->state_flags
						& PlayerShip) /* means IMMEDIATE WeaponFlag */
					|| PlotIntercept(lpEvalDesc->ObjectPtr,
									 ShipPtr, 2, 0))
				&& (uqm::random().next() & 3))
			{
				StarShipPtr->ship_input_state |= SpecialFlag;
			}

			if (lpEvalDesc->ObjectPtr
				&& !(lpEvalDesc->ObjectPtr->state_flags & CrewObject))
			{
				lpEvalDesc->ObjectPtr = 0;
			}
		}
	}

	if ((lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyShip])->ObjectPtr)
	{
		Starship* EnemyStarShipPtr {};

		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
			  & ImmediateWeapon))
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
	}
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
	/*
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <= SpecialEnergyCost)
		StarShipPtr->ship_input_state &= ~WeaponFlag;
*/
}

static void yehat_postprocess(Element* ElementPtr)
{
	if (!(ElementPtr->state_flags & Nonsolid))
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		/* take care of shield effect */
		if (StarShipPtr->special_counter > 0)
		{
			if (ElementPtr->life_span == NormalLife)
			{
				StarShipPtr->special_counter = 0;
			}
			else
			{
#ifdef OLD
				SetPrimColor(
					&DisplayArray[ElementPtr->PrimIndex],
					BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F));
				SetPrimType(
					&DisplayArray[ElementPtr->PrimIndex],
					PrimitiveType::StampFill);
#endif /* OLD */

				ProcessSound(GetSound(
								 /* YEHAT_SHIELD_ON */
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
							 ElementPtr);
				DeltaEnergy(ElementPtr, -SpecialEnergyCost);
			}
		}

#ifdef OLD
		if (ElementPtr->life_span > NormalLife)
		{
			HELEMENT hShipElement;

			if (hShipElement = AllocElement())
			{
				Element* ShipElementPtr;

				InsertElement(hShipElement, GetSuccElement(ElementPtr));
				LockElement(hShipElement, &ShipElementPtr);
				ShipElementPtr->playerNr = ElementPtr->playerNr;
				ShipElementPtr->state_flags =
					/* in place of Appearing */
					(Changing | PreProcessFlag | PostProcessFlag)
					| FiniteLife | Nonsolid;
				SetPrimType(
					&DisplayArray[ShipElementPtr->PrimIndex],
					PrimitiveType::Stamp);

				ShipElementPtr->life_span = 0; /* because preprocessing
													 * will not be done
													 */
				ShipElementPtr->current.location = ElementPtr->next.location;
				ShipElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
				ShipElementPtr->current.image.frame =
					SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.ship[0],
									 StarShipPtr->ShipFacing);
				ShipElementPtr->next = ShipElementPtr->current;
				ShipElementPtr->preprocess_func =
					ShipElementPtr->postprocess_func =
						ShipElementPtr->death_func = nullptr;
				ZeroVelocityComponents(&ShipElementPtr->velocity);

				UnlockElement(hShipElement);
			}
		}
#endif /* OLD */
	}
}

static void yehat_preprocess(Element* ElementPtr)
{
	if (!(ElementPtr->state_flags & Appearing))
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		if ((ElementPtr->life_span > NormalLife
			 /* take care of shield effect */
			 && --ElementPtr->life_span == NormalLife)
			|| (ElementPtr->life_span == NormalLife
				&& ElementPtr->next.image.farray
					   == StarShipPtr->RaceDescPtr->ship_data.special))
		{
#ifdef NEVER
			SetPrimType(
				&DisplayArray[ElementPtr->PrimIndex],
				PrimitiveType::Stamp);
#endif /* NEVER */

			ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
			ElementPtr->next.image.frame =
				SetEquFrameIndex(StarShipPtr->RaceDescPtr->ship_data.ship[0],
								 ElementPtr->next.image.frame);
			ElementPtr->state_flags |= Changing;
		}

		if ((StarShipPtr->cur_status_flags & SpecialFlag)
			&& StarShipPtr->special_counter == 0)
		{
			if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SpecialEnergyCost)
			{
				DeltaEnergy(ElementPtr, -SpecialEnergyCost); /* so text will flash */
			}
			else
			{
				ElementPtr->life_span = ShieldLife + NormalLife;

				ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
				ElementPtr->next.image.frame =
					SetEquFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0],
									 ElementPtr->next.image.frame);
				ElementPtr->state_flags |= Changing;

				StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
			}
		}
	}
}

RaceDesc* init_yehat(void)
{
	if (IsHd())
	{
		yehat_desc.characteristics.max_thrust = ResScale(MaxThrust);
		yehat_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		yehat_desc.cyborg_control.WeaponRange = MissileSpeedHd() * MissileLife / 3;
	}
	else
	{
		yehat_desc.characteristics.max_thrust = MaxThrust;
		yehat_desc.characteristics.thrust_increment = ThrustIncrement;
		yehat_desc.cyborg_control.WeaponRange =
			MissileSpeed * MissileLife / 3;
	}

	yehat_desc.preprocess_func = yehat_preprocess;
	yehat_desc.postprocess_func = yehat_postprocess;
	yehat_desc.init_weapon_func = initialize_standard_missiles;
	yehat_desc.cyborg_control.intelligence_func = yehat_intelligence;

	RaceDesc* RaceDescPtr {&yehat_desc};

	return (RaceDescPtr);
}

} // namespace uqm
