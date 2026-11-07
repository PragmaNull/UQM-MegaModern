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
#include "slylandr.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {12};
static constexpr auto MaxEnergy {20};
static constexpr auto EnergyRegeneration {0};
static constexpr auto EnergyWait {10};
static constexpr auto MaxThrust {60};
static constexpr auto ThrustIncrement {MaxThrust};
static constexpr auto ThrustWait {0};
static constexpr auto TurnWait {0};
static constexpr auto ShipMass {1};

// Lightning weapon
static constexpr auto WeaponEnergyCost {2};
static constexpr auto WeaponWait {17};

static auto LaserLength() { return ResScale(32); }
/* Total length of lighting bolts. Actual range is usually less than
		 * this, since the lightning rarely is straight. */

// Harvester
static constexpr auto SpecialEnergyCost {0};
static constexpr auto SpecialWait {20};
static auto HarvestRange() { return ResScale(208 * 3 / 8); }
/* Was originally (SpaceHeight() * 3 / 8) */

static RaceDesc slylandro_desc =
	{
		{/* ShipInfoDesc */
		 "probe",
		 SeekingWeapon | CrewImmune,
		 17, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 SlylandroRaceStrings,
		 SlylandroIconMaskPmapAnim,
		 SlylandroMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			InfiniteRadius, /* Initial sphere of influence radius */
			{
				/* Known location (center of SoI) */
				333,
				9812,
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
			 SlylandroBigMaskPmapAnim,
			 SlylandroMedMaskPmapAnim,
			 SlylandroSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {SlylandroCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 SlylandroVictorySong,
		 SlylandroShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			CloseRangeWeapon << 1,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static uint16_t initialize_lightning(Element* ElementPtr,
									 ElementHandle LaserArray[]);

static void lightning_postprocess(Element* ElementPtr)
{
	if (ElementPtr->turn_wait
		&& !(ElementPtr->state_flags & Collision))
	{
		ElementHandle Lightning {};

		initialize_lightning(ElementPtr, &Lightning);
		if (Lightning)
		{
			PutElement(Lightning);
		}
	}
}

static void lightning_collision(Element* ElementPtr0, GfxPoint* pPt0,
								Element* ElementPtr1, GfxPoint* pPt1)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr0, &StarShipPtr);
	if (StarShipPtr->weapon_counter > WeaponWait >> 1)
	{
		StarShipPtr->weapon_counter =
			WeaponWait - StarShipPtr->weapon_counter;
	}
	StarShipPtr->weapon_counter -= ElementPtr0->turn_wait;
	ElementPtr0->turn_wait = 0;

	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static uint16_t initialize_lightning(Element* ElementPtr, ElementHandle LaserArray[])
{
	LaserDesc LaserBlock;

	LaserBlock.cx = ElementPtr->next.location.x;
	LaserBlock.cy = ElementPtr->next.location.y;
	LaserBlock.ex = 0;
	LaserBlock.ey = 0;

	LaserBlock.sender = ElementPtr->playerNr;
	LaserBlock.flags = IgnoreSimilar;
	LaserBlock.face = 0;
	LaserBlock.pixoffs = 0;
	LaserArray[0] = initialize_laser(&LaserBlock);

	if (LaserArray[0])
	{
		int16_t delta {};
		uint16_t angle {};
		uint16_t facing {};
		Element* LaserPtr {};
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);

		LockElement(LaserArray[0], &LaserPtr);
		LaserPtr->postprocess_func = lightning_postprocess;
		LaserPtr->collision_func = lightning_collision;

		const uint32_t rand_val {uqm::random().next()};

		if (!(ElementPtr->state_flags & PlayerShip))
		{
			angle = GetVelocityTravelAngle(&ElementPtr->velocity);
			facing = NormalizeFacing(AngleToFacing(angle));
			delta = TrackShip(ElementPtr, &facing);

			LaserPtr->turn_wait = ElementPtr->turn_wait - 1;

			SetPrimColor(&DisplayArray[LaserPtr->PrimIndex],
						 GetPrimColor(&DisplayArray[ElementPtr->PrimIndex]));
		}
		else
		{
			facing = StarShipPtr->ShipFacing;
			ElementPtr->hTarget = 0;
			delta = TrackShip(ElementPtr, &facing);
			ElementPtr->hTarget = 0;
			angle = FacingToAngle(facing);

			if ((LaserPtr->turn_wait = StarShipPtr->weapon_counter) == 0)
			{
				LaserPtr->turn_wait = WeaponWait;
			}

			if (LaserPtr->turn_wait > WeaponWait >> 1)
			{
				LaserPtr->turn_wait = WeaponWait - LaserPtr->turn_wait;
			}

			switch (uqm::highByte(uqm::lowWord(rand_val)) & 3)
			{
				case 0:
					SetPrimColor(
						&DisplayArray[LaserPtr->PrimIndex],
						BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F));
					break;
				case 1:
					SetPrimColor(
						&DisplayArray[LaserPtr->PrimIndex],
						BuildColor(MakeRgb15(0x16, 0x17, 0x1F), 0x42));
					break;
				case 2:
					SetPrimColor(
						&DisplayArray[LaserPtr->PrimIndex],
						BuildColor(MakeRgb15(0x06, 0x07, 0x1F), 0x4A));
					break;
				case 3:
					SetPrimColor(
						&DisplayArray[LaserPtr->PrimIndex],
						BuildColor(MakeRgb15(0x00, 0x00, 0x18), 0x50));
					break;
			}
		}

		if (delta == -1 || delta == AngleToFacing(HalfCircle))
		{
			angle += uqm::lowWord(rand_val);
		}
		else if (delta == 0)
		{
			angle += uqm::lowWord(rand_val) & 1 ? -1 : 1;
		}
		else if (delta < AngleToFacing(HalfCircle))
		{
			angle += uqm::lowWord(rand_val) & (Quadrant - 1);
		}
		else
		{
			angle -= uqm::lowWord(rand_val) & (Quadrant - 1);
		}
		delta = WorldToVelocity(
			DisplayToWorld((uqm::highWord(rand_val) & (LaserLength() - 1)) + 4));
		SetVelocityComponents(&LaserPtr->velocity,
							  Cosine(angle, delta), Sine(angle, delta));

		SetElementStarShip(LaserPtr, StarShipPtr);
		UnlockElement(LaserArray[0]);
	}

	return (1);
}

static void slylandro_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								   uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	// no dodging in role playing game, unless you haven't
	// visited the starbase yet or difficulty is set to Hard
	if ((uqm::activityKind(uqm::currentActivity()) == ActivityKind::InEncounter) && uqm::gameState().flag(uqm::GameFlag::STARBASE_AVAILABLE) && !isDifficulty(uqm::Difficulty::Hard))
	{
		ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr = 0;
	}

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level == 0
		&& ObjectsOfConcern[*IntelObjectIndex::GravityMass].ObjectPtr == 0)
	{
		ConcernCounter = *IntelObjectIndex::FirstEmpty + 1;
	}
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == MovementState::Pursue
		&& lpEvalDesc->which_turn <= 6)
	{
		lpEvalDesc->MoveState = MovementState::Entice;
	}

	++ShipPtr->thrust_wait;
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
	--ShipPtr->thrust_wait;

	if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)
	{
		StarShipPtr->ship_input_state |= WeaponFlag;
	}
	else
	{
		StarShipPtr->ship_input_state &= ~WeaponFlag;
	}

	StarShipPtr->ship_input_state &= ~SpecialFlag;
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level < StarShipPtr->RaceDescPtr->ship_info.max_energy)
	{
		lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::FirstEmpty];
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)
		{
			StarShipPtr->ship_input_state |= SpecialFlag;
		}
	}
}

static bool harvest_space_junk(Element* ElementPtr)
{
	ElementHandle hElement {};
	ElementHandle hNextElement {};

	bool retval {false};
	for (hElement = GetHeadElement();
		 hElement; hElement = hNextElement)
	{
		Element* ObjPtr {};

		LockElement(hElement, &ObjPtr);
		hNextElement = GetSuccElement(ObjPtr);

		if (!(ObjPtr->state_flags & (Appearing | PlayerShip | FiniteLife))
			&& ObjPtr->playerNr == NeutralPlayerNum
			&& !GravityMass(ObjPtr->mass_points)
			&& CollisionPossible(ObjPtr, ElementPtr))
		{
			int32_t dx {};
			int32_t dy {};

			if ((dx = ObjPtr->next.location.x
					- ElementPtr->next.location.x)
				< 0)
			{
				dx = -dx;
			}
			if ((dy = ObjPtr->next.location.y
					- ElementPtr->next.location.y)
				< 0)
			{
				dy = -dy;
			}
			dx = WorldToDisplay(dx);
			dy = WorldToDisplay(dy);
			if (dx <= HarvestRange() && dy <= HarvestRange()
				&& dx * dx + dy * dy <= HarvestRange() * HarvestRange())
			{
				ObjPtr->life_span = 0;
				ObjPtr->state_flags |= Nonsolid;

				if (!retval)
				{
					Starship* StarShipPtr {};

					retval = true;

					GetElementStarShip(ElementPtr, &StarShipPtr);
					ProcessSound(GetSound(
									 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
								 ElementPtr);
					DeltaEnergy(ElementPtr, MaxEnergy);
				}
			}
		}

		UnlockElement(hElement);
	}

	return (retval);
}

static void slylandro_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->weapon_counter
		&& StarShipPtr->weapon_counter < WeaponWait)
	{
		ElementHandle Lightning {};

		initialize_lightning(ElementPtr, &Lightning);
		if (Lightning)
		{
			PutElement(Lightning);
		}
	}

	if (StarShipPtr->special_counter == 0
		&& (StarShipPtr->cur_status_flags & SpecialFlag)
		&& harvest_space_junk(ElementPtr))
	{
		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

static void slylandro_preprocess(Element* ElementPtr)
{
	if (!(ElementPtr->state_flags & (Appearing | Nonsolid)))
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		if ((StarShipPtr->cur_status_flags & ThrustFlag)
			&& !(StarShipPtr->old_status_flags & ThrustFlag))
		{
			StarShipPtr->ShipFacing += AngleToFacing(HalfCircle);
		}

		if (ElementPtr->turn_wait == 0)
		{
			ElementPtr->turn_wait +=
				StarShipPtr->RaceDescPtr->characteristics.turn_wait + 1;
			if (StarShipPtr->cur_status_flags & LeftFlag)
			{
				--StarShipPtr->ShipFacing;
			}
			else if (StarShipPtr->cur_status_flags & RightFlag)
			{
				++StarShipPtr->ShipFacing;
			}
		}

		StarShipPtr->ShipFacing = NormalizeFacing(StarShipPtr->ShipFacing);

		if (ElementPtr->thrust_wait == 0)
		{
			ElementPtr->thrust_wait +=
				StarShipPtr->RaceDescPtr->characteristics.thrust_wait + 1;

			SetVelocityVector(&ElementPtr->velocity,
							  StarShipPtr->RaceDescPtr->characteristics.max_thrust,
							  StarShipPtr->ShipFacing);
			StarShipPtr->cur_status_flags |= ShipAtMaxSpeed;
			StarShipPtr->cur_status_flags &= ~ShipInGravityWell;
		}

		ElementPtr->next.image.frame = IncFrameIndex(ElementPtr->next.image.frame);
		ElementPtr->state_flags |= Changing;
	}
}

RaceDesc* init_slylandro(void)
{
	if (IsHd())
	{
		slylandro_desc.characteristics.max_thrust = ResScale(MaxThrust);
		slylandro_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		slylandro_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd << 1;
	}
	else
	{
		slylandro_desc.characteristics.max_thrust = MaxThrust;
		slylandro_desc.characteristics.thrust_increment = ThrustIncrement;
		slylandro_desc.cyborg_control.WeaponRange =
			CloseRangeWeapon << 1;
	}

	slylandro_desc.preprocess_func = slylandro_preprocess;
	slylandro_desc.postprocess_func = slylandro_postprocess;
	slylandro_desc.init_weapon_func = initialize_lightning;
	slylandro_desc.cyborg_control.intelligence_func = slylandro_intelligence;

	RaceDesc* RaceDescPtr {&slylandro_desc};

	return (RaceDescPtr);
}

} // namespace uqm