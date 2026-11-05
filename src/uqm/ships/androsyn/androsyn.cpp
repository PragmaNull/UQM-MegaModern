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
#include "androsyn.h"
#include "resinst.h"
#include "../../units.h"

#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

// Core characteristics
static constexpr auto MaxCrew {20};
static constexpr auto MaxEnergy {24};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {8};
static constexpr auto MaxThrust {24};
static constexpr auto ThrustIncrement {3};
static constexpr auto TurnWait {4};
static constexpr auto ThrustWait {0};
static constexpr auto ShipMass {6};

// Bubbles
static constexpr auto WeaponEnergyCost {3};
static constexpr auto WeaponWait {0};
static auto AndrosynthOffset() { return ResScale(14); }
static auto MissileOffset() { return ResScale(3); }
static auto MissileSpeed() { return DisplayToWorld(ResScale(8)); }
static constexpr auto MissileLife {200};
static constexpr auto MissileHits {3};
static constexpr auto MissileDamage {2};
static constexpr auto TrackWait {2};

// Blazer
static constexpr auto SpecialEnergyCost {2};
static constexpr auto BlazerDegeneration {(-1)};
static constexpr auto SpecialWait {0};
static auto BlazerOffset() { return ResScale(10); }
static auto BlazerThrust() { return ResScale(60); }
static constexpr auto BlazerTurnWait {1};
static constexpr auto BlazerDamage {3};
static constexpr auto BlazerMass {1};

static RACE_DESC androsynth_desc =
	{
		{/* SHIP_INFO */
		 "guardian",
		 FiresFore | SeekingWeapon,
		 15, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 AndrosynthRaceStrings,
		 AndrosynthIconMaskPmapAnim,
		 AndrosynthMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
			0, /* Initial sphere of influence radius */
			{
				/* Known location (center of SoI) */
				0,
				0,
			},
		 },
		{
			/* CHARACTERISTIC_STUFF */
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
		{/* DATA_STUFF */
		 {
			 AndrosynthBigMaskPmapAnim,
			 AndrosynthMedMaskPmapAnim,
			 AndrosynthSmlMaskPmapAnim,
		 },
		 {
			 BubbleBigMaskPmapAnim,
			 BubbleMedMaskPmapAnim,
			 BubbleSmlMaskPmapAnim,
		 },
		 {
			 BlazerBigMaskPmapAnim,
			 BlazerMedMaskPmapAnim,
			 BlazerSmlMaskPmapAnim,
		 },
		 {AndrosynthCaptMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 AndrosynthVictorySong,
		 AndrosynthShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			/* INTEL_STUFF */
			0,
			LongRangeWeapon >> 2,
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
typedef struct
{
	ElementCollisionFunc* collision_func;
} ANDROSYNTH_DATA;

// Local typedef
typedef ANDROSYNTH_DATA CustomShipData_t;

// Retrieve race-specific ship data from a race desc
static CustomShipData_t* GetCustomShipData(RACE_DESC* pRaceDesc)
{
	return (CustomShipData_t*)pRaceDesc->data;
}

// Set the race-specific data in a race desc
// (Re)Allocates its own storage for the data.
static void SetCustomShipData(RACE_DESC* pRaceDesc, const CustomShipData_t* data)
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

static void blazer_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
							 ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	const uint16_t old_crew_level {ElementPtr0->crew_level};
	const uint16_t old_life {ElementPtr0->life_span};
	const uint8_t old_offs {ElementPtr0->blast_offset};
	ElementPtr0->blast_offset = BlazerOffset();
	ElementPtr0->mass_points = BlazerDamage;
	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	ElementPtr0->mass_points = BlazerMass;
	ElementPtr0->blast_offset = old_offs;
	ElementPtr0->life_span = old_life;
	ElementPtr0->crew_level = old_crew_level;

	ElementPtr0->state_flags &= ~(Disappearing | Nonsolid);
	collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void bubble_preprocess(ELEMENT* ElementPtr)
{
	uint8_t turn_wait {};

	uint8_t thrust_wait {uqm::highNibble(ElementPtr->turn_wait)};
	turn_wait = uqm::lowNibble(ElementPtr->turn_wait);
	if (thrust_wait > 0)
	{
		--thrust_wait;
	}
	else
	{
		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->current.image.frame);
		ElementPtr->state_flags |= Changing;

		thrust_wait = (uint8_t)((uint16_t)uqm::random().next() & 3);
	}

	if (turn_wait > 0)
	{
		--turn_wait;
	}
	else
	{
		uint16_t facing {};
		int16_t delta_facing {};

		facing = NormalizeFacing(AngleToFacing(
			GetVelocityTravelAngle(&ElementPtr->velocity)));
		if ((delta_facing = TrackShip(ElementPtr, &facing)) == -1)
		{
			facing = (uint16_t)uqm::random().next();
		}
		else if (delta_facing <= AngleToFacing(HalfCircle))
		{
			facing += (uint16_t)uqm::random().next() & (AngleToFacing(HalfCircle) - 1);
		}
		else
		{
			facing -= (uint16_t)uqm::random().next() & (AngleToFacing(HalfCircle) - 1);
		}
		SetVelocityVector(&ElementPtr->velocity,
						  MissileSpeed(), facing);
		turn_wait = TrackWait;
	}

	ElementPtr->turn_wait = uqm::makeByte(turn_wait, thrust_wait);
}

static uint16_t initialize_bubble(ELEMENT* ShipPtr, HELEMENT BubbleArray[])
{
	STARSHIP* StarShipPtr {};
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = AndrosynthOffset();
	MissileBlock.speed = MissileSpeed();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = bubble_preprocess;
	MissileBlock.blast_offs = MissileOffset();
	BubbleArray[0] = initialize_missile(&MissileBlock);

	if (BubbleArray[0])
	{
		ELEMENT* BubblePtr {};

		LockElement(BubbleArray[0], &BubblePtr);
		BubblePtr->turn_wait = 0;
		UnlockElement(BubbleArray[0]);
	}

	return (1);
}

static void androsynth_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
									uint16_t ConcernCounter)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	EVALUATE_DESC* lpEvalDesc {&ObjectsOfConcern[ENEMY_WEAPON_INDEX]};
	/* in blazer form */
	if (ShipPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		ObjectsOfConcern[CREW_OBJECT_INDEX].ObjectPtr = 0;
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
		{
			if ((lpEvalDesc->ObjectPtr->state_flags & FiniteLife)
				&& !(lpEvalDesc->ObjectPtr->state_flags & CrewObject))
			{
				lpEvalDesc->MoveState = AVOID;
			}
			else
			{
				lpEvalDesc->ObjectPtr = 0;
			}
		}

		ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
	}
	else
	{
		STARSHIP* pEnemyStarShip = nullptr;

		lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
		if (lpEvalDesc->ObjectPtr)
		{
			GetElementStarShip(lpEvalDesc->ObjectPtr, &pEnemyStarShip);
			if (lpEvalDesc->which_turn <= 16
				&& (StarShipPtr->special_counter > 0
					|| StarShipPtr->RaceDescPtr->ship_info.energy_level < MaxEnergy / 3
					|| ((GetWeaponRange(&pEnemyStarShip->RaceDescPtr->cyborg_control) <= ResScale(CloseRangeWeapon)
						 && lpEvalDesc->ObjectPtr->crew_level > BlazerDamage)
						|| (lpEvalDesc->ObjectPtr->crew_level > (BlazerDamage * 3)
							&& Maneuverability(&pEnemyStarShip->RaceDescPtr->cyborg_control) > ResolutionCompensated(SlowShip)))))
			{
				lpEvalDesc->MoveState = ENTICE;
			}
		}

		ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (StarShipPtr->special_counter == 0)
		{
			StarShipPtr->ship_input_state &= ~SpecialFlag;
			if ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr
				 && ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 4)
				|| (lpEvalDesc->ObjectPtr
					&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= MaxEnergy / 3
					&& (GetWeaponRange(&pEnemyStarShip->RaceDescPtr->cyborg_control) >= GetWeaponRange(&StarShipPtr->RaceDescPtr->cyborg_control) << 1
						|| (lpEvalDesc->which_turn < 16
							&& (GetWeaponRange(&pEnemyStarShip->RaceDescPtr->cyborg_control) > ResScale(CloseRangeWeapon)
								|| lpEvalDesc->ObjectPtr->crew_level <= BlazerDamage)
							&& (lpEvalDesc->ObjectPtr->crew_level <= (BlazerDamage * 3)
								|| Maneuverability(&pEnemyStarShip->RaceDescPtr->cyborg_control) <= ResolutionCompensated(SlowShip))))))
			{
				StarShipPtr->ship_input_state |= SpecialFlag;
			}
		}

		if (!(StarShipPtr->ship_input_state & SpecialFlag)
			&& StarShipPtr->weapon_counter == 0
			&& lpEvalDesc->ObjectPtr)
		{
			if (lpEvalDesc->which_turn <= 4)
			{
				StarShipPtr->ship_input_state |= WeaponFlag;
			}
			else if (lpEvalDesc->MoveState != PURSUE
					 && lpEvalDesc->which_turn <= 12)
			{
				int32_t ship_delta_x {};
				int32_t ship_delta_y {};
				int32_t other_delta_x {};
				int32_t other_delta_y {};

				GetCurrentVelocityComponentsSdword(&ShipPtr->velocity,
												   &ship_delta_x, &ship_delta_y);
				GetCurrentVelocityComponentsSdword(&lpEvalDesc->ObjectPtr->velocity,
												   &other_delta_x, &other_delta_y);
				int32_t delta_x {ship_delta_x - other_delta_x};
				int32_t delta_y {ship_delta_y - other_delta_y};
				const uint16_t travel_facing {ARCTAN(delta_x, delta_y)};

				delta_x =
					lpEvalDesc->ObjectPtr->next.location.x - ShipPtr->next.location.x;
				delta_y =
					lpEvalDesc->ObjectPtr->next.location.y - ShipPtr->next.location.y;
				const uint16_t direction_facing {ARCTAN(delta_x, delta_y)};

				if (NormalizeAngle(travel_facing
								   - direction_facing + Octant)
					<= Quadrant)
				{
					StarShipPtr->ship_input_state |= WeaponFlag;
				}
			}
		}
	}
}

static void androsynth_postprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	/* take care of blazer effect */
	if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		if ((StarShipPtr->cur_status_flags & AndrosynCometToggle)
			|| StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
		{
			StarShipPtr->RaceDescPtr->characteristics.energy_regeneration =
				(uint8_t)BlazerDegeneration;
			StarShipPtr->energy_counter = EnergyWait;

			if (StarShipPtr->cur_status_flags & AndrosynCometToggle)
			{
				ProcessSound(GetSound(
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
							 ElementPtr); /* COMET_ON */
				ElementPtr->turn_wait = 0;
				ElementPtr->thrust_wait = 0;
				StarShipPtr->RaceDescPtr->characteristics.special_wait =
					StarShipPtr->RaceDescPtr->characteristics.turn_wait;
				ElementPtr->mass_points = BlazerMass;
				StarShipPtr->RaceDescPtr->characteristics.turn_wait = BlazerTurnWait;
				StarShipPtr->cur_status_flags &= ~AndrosynCometToggle;

				/* Save the current collision func because we were not the
				 * ones who set it */
				{
					const ANDROSYNTH_DATA shipData = {ElementPtr->collision_func};
					SetCustomShipData(StarShipPtr->RaceDescPtr, &shipData);
					ElementPtr->collision_func = blazer_collision;
				}
			}
		}

		if (StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
		/* if blazer wasn't able to change back into ship
				 * give it a little more juice to try to get out
				 * of its predicament. Kruzen: also keep track
				 * of turn speed.
				 */
		{
			StarShipPtr->RaceDescPtr->characteristics.special_wait =
				StarShipPtr->RaceDescPtr->characteristics.turn_wait;
			StarShipPtr->RaceDescPtr->characteristics.turn_wait = BlazerTurnWait;
			DeltaEnergy(ElementPtr, -BlazerDegeneration);
			StarShipPtr->energy_counter = 1;
		}
	}
	else if ((StarShipPtr->cur_status_flags & SpecialFlag) && (StarShipPtr->cur_status_flags & AndrosynCometToggle))
	/* Kruzen: If we failed to transform into
				 * comet due to collision failed in Process() -
				 * remove the special flag.
				 */
	{
		StarShipPtr->cur_status_flags &= ~AndrosynCometToggle;
	}
}

static void androsynth_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};
	STATUS_FLAGS cur_status_flags {};

	GetElementStarShip(ElementPtr, &StarShipPtr);

	cur_status_flags = StarShipPtr->cur_status_flags;
	if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
	{
		if (cur_status_flags & SpecialFlag)
		{
			if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SpecialEnergyCost)
			{
				cur_status_flags |= LowOnEnergy; /* so text will flash */
			}
			else
			{
				cur_status_flags &= ~WeaponFlag;

				ElementPtr->next.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
				ElementPtr->next.image.frame =
					SetEquFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0],
									 ElementPtr->next.image.frame);
				ElementPtr->state_flags |= Changing;
				cur_status_flags |= AndrosynCometToggle;
			}
		}
	}
	else
	{
		cur_status_flags &= ~(ThrustFlag | WeaponFlag);

		/* protection against vux */
		if (StarShipPtr->RaceDescPtr->characteristics.turn_wait > BlazerTurnWait)
		{
			StarShipPtr->RaceDescPtr->characteristics.special_wait +=
				StarShipPtr->RaceDescPtr->characteristics.turn_wait
				- BlazerTurnWait;
			StarShipPtr->RaceDescPtr->characteristics.turn_wait = BlazerTurnWait;
		}

		if (StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
		{
			ZeroVelocityComponents(&ElementPtr->velocity);
			cur_status_flags &= ~(LeftFlag | RightFlag
								  | ShipAtMaxSpeed | ShipBeyondMaxSpeed);

			StarShipPtr->RaceDescPtr->characteristics.turn_wait =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
			StarShipPtr->RaceDescPtr->characteristics.energy_regeneration = EnergyRegeneration;
			ElementPtr->mass_points = ShipMass;
			ElementPtr->collision_func =
				GetCustomShipData(StarShipPtr->RaceDescPtr)->collision_func;
			ElementPtr->next.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.ship;
			ElementPtr->next.image.frame =
				SetEquFrameIndex(StarShipPtr->RaceDescPtr->ship_data.ship[0],
								 ElementPtr->next.image.frame);
			ElementPtr->state_flags |= Changing;
		}
		else
		{
			cur_status_flags |= SpecialFlag;

			if (ElementPtr->thrust_wait)
			{
				--ElementPtr->thrust_wait;
			}
			else
			{
				uint16_t facing {StarShipPtr->ShipFacing};
				if (ElementPtr->turn_wait == 0
					&& (cur_status_flags & (LeftFlag | RightFlag)))
				{
					if (cur_status_flags & LeftFlag)
					{
						--facing;
					}
					else
					{
						++facing;
					}
				}

				SetVelocityVector(&ElementPtr->velocity,
								  BlazerThrust(), NormalizeFacing(facing));
				cur_status_flags |= ShipAtMaxSpeed | ShipBeyondMaxSpeed;
			}
		}
	}
	StarShipPtr->cur_status_flags = cur_status_flags;
}

//static void
//uninit_androsynth (RACE_DESC *pRaceDesc)
//{
//	SetCustomShipData (pRaceDesc, nullptr);
//}


RACE_DESC* init_androsynth(void)
{
	if (IsHd())
	{
		androsynth_desc.characteristics.max_thrust = ResScale(MaxThrust);
		androsynth_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		androsynth_desc.cyborg_control.WeaponRange = LongRangeWeaponHd >> 2;
	}
	else
	{
		androsynth_desc.characteristics.max_thrust = MaxThrust;
		androsynth_desc.characteristics.thrust_increment =
			ThrustIncrement;
		androsynth_desc.cyborg_control.WeaponRange =
			LongRangeWeapon >> 2;
	}

	androsynth_desc.preprocess_func = androsynth_preprocess;
	androsynth_desc.postprocess_func = androsynth_postprocess;
	androsynth_desc.init_weapon_func = initialize_bubble;
	androsynth_desc.cyborg_control.intelligence_func = androsynth_intelligence;

	RACE_DESC* RaceDescPtr {&androsynth_desc};

	return (RaceDescPtr);
}