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
#include "human.h"
#include "resinst.h"

#include "uqm/colors.h"
#include "uqm/globdata.h"

// Core characteristics
static constexpr auto MaxCrew {18};
static constexpr auto MaxEnergy {18};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {8};
static constexpr auto MaxThrust {24};
static auto MaxThrustHd() { return ResScale(MaxThrust); }
static constexpr auto ThrustIncrement {3};
static constexpr auto ThrustWait {4};
static constexpr auto TurnWait {1};
static constexpr auto ShipMass {6};

// Nuke
static constexpr auto WeaponEnergyCost {9};
static constexpr auto WeaponWait {10};
static auto HumanOffset() { return ResScale(42); }
static auto NukeOffset() { return ResScale(8); }
static auto MinMissileSpeed() { return DisplayToWorld(ResScale(10)); }
static auto MaxMissileSpeed() { return DisplayToWorld(ResScale(20)); }
static auto MissileSpeed() { return (MaxThrustHd() >= MinMissileSpeed() ? MaxThrustHd() : MinMissileSpeed()); }
static auto ThrustScale() { return DisplayToWorld(ResScale(1)); }
static constexpr auto MissileLife {60};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {4};
static constexpr auto TrackWait {3};

// Point-Defense Laser
static constexpr auto SpecialEnergyCost {4};
static constexpr auto SpecialWait {9};
#define LASER_RANGE (uint16_t)ResScale(100)

RACE_DESC human_desc =
	{
		{/* SHIP_INFO */
		 "cruiser",
		 FiresFore | SeekingWeapon | PointDefense,
		 11, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 HumanRaceStrings,
		 HumanIconMaskPmapAnim,
		 HumanMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
			0, /* Initial sphere of influence radius */
			{
				/* Known location (center of SoI) */
				1752,
				1450,
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
			 HumanBigMaskPmapAnim,
			 HumanMedMaskPmapAnim,
			 HumanSmlMaskPmapAnim,
		 },
		 {
			 SaturnBigMaskPmapAnim,
			 SaturnMedMaskPmapAnim,
			 SaturnSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {HumanCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 HumanVictorySong,
		 HumanShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			LongRangeWeapon,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static void
nuke_preprocess(ELEMENT* ElementPtr)
{
	uint16_t facing {GetFrameIndex(ElementPtr->next.image.frame)};
	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		if (TrackShip(ElementPtr, &facing) > 0)
		{
			ElementPtr->next.image.frame =
				SetAbsFrameIndex(ElementPtr->next.image.frame,
								 facing);
			ElementPtr->state_flags |= Changing;
		}

		ElementPtr->turn_wait = TrackWait;
	}

	{
		int32_t speed {};

		if ((speed = MissileSpeed() + ((MissileLife - ElementPtr->life_span) * ThrustScale())) > MaxMissileSpeed())
		{
			speed = MaxMissileSpeed();
		}
		SetVelocityVector(&ElementPtr->velocity,
						  speed, facing);
	}
}

static void
spawn_point_defense(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (ElementPtr->state_flags & PlayerShip)
	{
		HELEMENT hDefense {AllocElement()};
		if (hDefense)
		{
			ELEMENT* DefensePtr {};

			LockElement(hDefense, &DefensePtr);
			DefensePtr->playerNr = ElementPtr->playerNr;
			DefensePtr->state_flags = Appearing | Nonsolid | FiniteLife;
			DefensePtr->death_func = spawn_point_defense;

			GetElementStarShip(ElementPtr, &StarShipPtr);
			SetElementStarShip(DefensePtr, StarShipPtr);
			UnlockElement(hDefense);

			PutElement(hDefense);
		}
	}
	else
	{
		HELEMENT hNextObject {};
		ELEMENT* ShipPtr {};

		bool PaidFor {false};

		LockElement(StarShipPtr->hShip, &ShipPtr);
		for (HELEMENT hObject {GetTailElement()}; hObject; hObject = hNextObject)
		{
			ELEMENT* ObjectPtr {};

			LockElement(hObject, &ObjectPtr);
			hNextObject = GetPredElement(ObjectPtr);
			if (ObjectPtr != ShipPtr && CollidingElement(ObjectPtr) && !ObjectCloaked(ObjectPtr))
			{
				int16_t delta_x {};
				int16_t delta_y {};

				delta_x = ObjectPtr->next.location.x - ShipPtr->next.location.x;
				delta_y = ObjectPtr->next.location.y - ShipPtr->next.location.y;
				if (delta_x < 0)
				{
					delta_x = -delta_x;
				}
				if (delta_y < 0)
				{
					delta_y = -delta_y;
				}
				delta_x = WorldToDisplay(delta_x);
				delta_y = WorldToDisplay(delta_y);
				if ((uint16_t)delta_x <= LASER_RANGE && (uint16_t)delta_y <= LASER_RANGE && (uint16_t)delta_x * (uint16_t)delta_x + (uint16_t)delta_y * (uint16_t)delta_y <= LASER_RANGE * LASER_RANGE)
				{
					HELEMENT hPointDefense {};
					LASER_BLOCK LaserBlock;

					if (!PaidFor)
					{
						if (!DeltaEnergy(ShipPtr, -SpecialEnergyCost))
						{
							break;
						}

						ProcessSound(GetSound(
										 /* POINT_DEFENSE_LASER */
										 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
									 ElementPtr);
						StarShipPtr->special_counter =
							StarShipPtr->RaceDescPtr->characteristics.special_wait;
						PaidFor = true;
					}

					LaserBlock.cx = ShipPtr->next.location.x;
					LaserBlock.cy = ShipPtr->next.location.y;
					LaserBlock.face = 0;
					LaserBlock.ex = ObjectPtr->next.location.x
								  - ShipPtr->next.location.x;
					LaserBlock.ey = ObjectPtr->next.location.y
								  - ShipPtr->next.location.y;
					LaserBlock.sender = ShipPtr->playerNr;
					LaserBlock.flags = IgnoreSimilar;
					LaserBlock.pixoffs = 0;
					LaserBlock.color = BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F);
					hPointDefense = initialize_laser(&LaserBlock);
					if (hPointDefense)
					{
						ELEMENT* PDPtr {};

						LockElement(hPointDefense, &PDPtr);
						SetElementStarShip(PDPtr, StarShipPtr);
						PDPtr->hTarget = 0;
						UnlockElement(hPointDefense);

						PutElement(hPointDefense);
					}
				}
			}
			UnlockElement(hObject);
		}
		UnlockElement(StarShipPtr->hShip);
	}
}

static uint16_t
initialize_nuke(ELEMENT* ShipPtr, HELEMENT NukeArray[])
{
	STARSHIP* StarShipPtr {};
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = 0;
	MissileBlock.pixoffs = HumanOffset();
	MissileBlock.speed = ResScale(MissileSpeed());
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nuke_preprocess;
	MissileBlock.blast_offs = NukeOffset();
	NukeArray[0] = initialize_missile(&MissileBlock);

	if (NukeArray[0])
	{
		ELEMENT* NukePtr {};

		LockElement(NukeArray[0], &NukePtr);
		NukePtr->turn_wait = TrackWait;
		UnlockElement(NukeArray[0]);
	}

	return (1);
}

static void
human_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
				   uint16_t ConcernCounter)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
		&& ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr != nullptr
			 && ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 2)
			|| (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr != nullptr
				&& ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn <= 4)))
	{
		StarShipPtr->ship_input_state |= SpecialFlag;
	}
	else
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
	}
	ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = nullptr;

	ship_intelligence(ShipPtr,
					  ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->weapon_counter == 0)
	{
		if (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr
			&& (!(StarShipPtr->ship_input_state & (LeftFlag | RightFlag /* | ThrustFlag */))
				|| ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn <= 12))
		{
			StarShipPtr->ship_input_state |= WeaponFlag;
		}
	}
}

static void
human_postprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SpecialFlag)
		&& StarShipPtr->special_counter == 0)
	{
		spawn_point_defense(ElementPtr);
	}
}

RACE_DESC*
init_human(void)
{
	if (IsHd())
	{
		human_desc.characteristics.max_thrust = MaxThrustHd();
		human_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		human_desc.cyborg_control.WeaponRange = LongRangeWeaponHd;
	}
	else
	{
		human_desc.characteristics.max_thrust = MaxThrust;
		human_desc.characteristics.thrust_increment = ThrustIncrement;
		human_desc.cyborg_control.WeaponRange = LongRangeWeapon;
	}

	human_desc.postprocess_func = human_postprocess;
	human_desc.init_weapon_func = initialize_nuke;
	human_desc.cyborg_control.intelligence_func = human_intelligence;

	RACE_DESC* RaceDescPtr {&human_desc};

	return (RaceDescPtr);
}
