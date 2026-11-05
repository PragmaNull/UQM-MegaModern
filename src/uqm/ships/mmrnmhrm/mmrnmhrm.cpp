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
#include "mmrnmhrm.h"
#include "resinst.h"

// Core characteristics
static constexpr auto MaxCrew {20};
static constexpr auto MaxEnergy {10};
static constexpr auto ShipMass {3};

// X-Wing characteristics
static constexpr auto EnergyRegeneration {2};
static constexpr auto EnergyWait {6};
static constexpr auto MaxThrust {20};
static constexpr auto ThrustIncrement {5};
static constexpr auto ThrustWait {1};
static constexpr auto TurnWait {2};

// Y-Wing characteristics
static constexpr auto YwingEnergyRegeneration {1};
static constexpr auto YwingSpecialEnergyCost {MaxEnergy};
static constexpr auto YwingEnergyWait {6};
static auto YwingMaxThrust() { return ResScale(50); }
static auto YwingThrustIncrement() { return ResScale(10); }
static constexpr auto YwingThrustWait {0};
static constexpr auto YwingTurnWait {14};

// X-Wing Lasers
static auto MmrnmhrmOffset() { return ResScale(16); }
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {0};
static auto CenterOffs() { return DisplayToWorld(ResScale(4)); }
static auto WingOffs() { return DisplayToWorld(chooseIfHd(10, 48)); }
static auto LaserRange() { return DisplayToWorld(ResScale(125) + MmrnmhrmOffset()); }

// Y-Wing Missiles
static constexpr auto YwingWeaponEnergyCost {1};
static constexpr auto YwingWeaponWait {20};
static auto LaunchOffs() { return DisplayToWorld(ResScale(4)); }
static constexpr auto MissileOffset {0};
static auto MissileSpeed() { return DisplayToWorld(ResScale(20)); }
static constexpr auto MissileLife {40};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};
static constexpr auto TrackWait {5};

// Transform
static constexpr auto SpecialEnergyCost {MaxEnergy};
static constexpr auto SpecialWait {0};
static constexpr auto YwingSpecialWait {0};

static RACE_DESC mmrnmhrm_desc =
	{
		{/* SHIP_INFO */
		 "xform",
		 FiresFore | ImmediateWeapon,
		 19, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 MmrnmhrmRaceStrings,
		 MmrnmhrmIconMaskPmapAnim,
		 MmrnmhrmMiconMaskPmapAnim,
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
			 MmrnmhrmBigMaskPmapAnim,
			 MmrnmhrmMedMaskPmapAnim,
			 MmrnmhrmSmlMaskPmapAnim,
		 },
		 {
			 TorpBigMaskPmapAnim,
			 TorpMedMaskPmapAnim,
			 TorpSmlMaskPmapAnim,
		 },
		 {
			 YwingBigMaskPmapAnim,
			 YwingMedMaskPmapAnim,
			 YwingSmlMaskPmapAnim,
		 },
		 {MmrnmhrmCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 MmrnmhrmVictorySong,
		 MmrnmhrmShipSounds,
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

// Private per-instance ship data
typedef CHARACTERISTIC_STUFF MMRNMHRM_DATA;

// Local typedef
typedef MMRNMHRM_DATA CustomShipData_t;

// Retrieve race-specific ship data from a race desc
static CustomShipData_t*
GetCustomShipData(RACE_DESC* pRaceDesc)
{
	return (CustomShipData_t*)pRaceDesc->data;
}

// Set the race-specific data in a race desc
// (Re)Allocates its own storage for the data.
static void
SetCustomShipData(RACE_DESC* pRaceDesc, const CustomShipData_t* data)
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

static void
missile_preprocess(ELEMENT* ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		uint16_t facing {GetFrameIndex(ElementPtr->next.image.frame)};
		if (TrackShip(ElementPtr, &facing) > 0)
		{
			ElementPtr->next.image.frame =
				SetAbsFrameIndex(ElementPtr->next.image.frame,
								 facing);
			ElementPtr->state_flags |= Changing;

			SetVelocityVector(&ElementPtr->velocity,
							  MissileSpeed(), facing);
		}

		ElementPtr->turn_wait = TrackWait;
	}
}

static void
mmrnmhrm_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
					  uint16_t ConcernCounter)
{
	bool CanTransform {};
	STARSHIP* StarShipPtr {};
	STARSHIP* EnemyStarShipPtr = nullptr;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	CanTransform = (bool)(StarShipPtr->special_counter == 0
						  && StarShipPtr->RaceDescPtr->ship_info.energy_level >= StarShipPtr->RaceDescPtr->characteristics.special_energy_cost);

	const EVALUATE_DESC* lpEvalDesc {&ObjectsOfConcern[ENEMY_SHIP_INDEX]};
	if (lpEvalDesc->ObjectPtr)
	{
		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
	}

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	StarShipPtr->ship_input_state &= ~SpecialFlag;
	if (CanTransform
		&& lpEvalDesc->ObjectPtr
		&& !(StarShipPtr->ship_input_state & WeaponFlag))
	{
		int32_t delta_x {};
		int32_t delta_y {};
		uint16_t travel_angle {};
		uint16_t direction_angle {};

		GetCurrentVelocityComponentsSdword(&lpEvalDesc->ObjectPtr->velocity,
										   &delta_x, &delta_y);
		if (delta_x == 0 && delta_y == 0)
		{
			direction_angle = travel_angle = 0;
		}
		else
		{
			delta_x = lpEvalDesc->ObjectPtr->current.location.x
					- ShipPtr->current.location.x;
			delta_y = lpEvalDesc->ObjectPtr->current.location.y
					- ShipPtr->current.location.y;
			direction_angle = ARCTAN(-delta_x, -delta_y);
			travel_angle = GetVelocityTravelAngle(
				&lpEvalDesc->ObjectPtr->velocity);
		}

		if (ShipPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
		{
			if (lpEvalDesc->which_turn > 8)
			{
				if (Maneuverability(&EnemyStarShipPtr->RaceDescPtr->cyborg_control) <= ResolutionCompensated(SlowShip)
					|| NormalizeAngle(
						   direction_angle - travel_angle + Quadrant)
						   > HalfCircle)
				{
					StarShipPtr->ship_input_state |= SpecialFlag;
				}
			}
		}
		else
		{
			int32_t ship_delta_x {};
			int32_t ship_delta_y {};

			GetCurrentVelocityComponentsSdword(&ShipPtr->velocity,
											   &ship_delta_x, &ship_delta_y);
			delta_x -= ship_delta_x;
			delta_y -= ship_delta_y;
			travel_angle = ARCTAN(delta_x, delta_y);
			if (lpEvalDesc->which_turn < 16)
			{
				if (lpEvalDesc->which_turn <= 8
					|| NormalizeAngle(
						   direction_angle - travel_angle + Octant)
						   <= Quadrant)
				{
					StarShipPtr->ship_input_state |= SpecialFlag;
				}
			}
			else if (lpEvalDesc->which_turn > 32
					 && NormalizeAngle(
							direction_angle - travel_angle + Quadrant)
							> HalfCircle)
			{
				StarShipPtr->ship_input_state |= SpecialFlag;
			}
		}
	}

	if (ShipPtr->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		if (!(StarShipPtr->ship_input_state & SpecialFlag)
			&& lpEvalDesc->ObjectPtr)
		{
			StarShipPtr->ship_input_state |= WeaponFlag;
		}
		else
		{
			StarShipPtr->ship_input_state &= ~WeaponFlag;
		}
	}
}

static void
twin_laser_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
					 ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	if (!(ElementPtr1->state_flags & PlayerShip)
		|| !elementsOfSamePlayer(ElementPtr0, ElementPtr1))
	{
		weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	}
}

static uint16_t
initialize_dual_weapons(ELEMENT* ShipPtr, HELEMENT WeaponArray[])
{
	int16_t cx {};
	int16_t cy {};
	uint16_t angle {};
	int32_t offs_x {};
	int32_t offs_y {};
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	const uint16_t facing {StarShipPtr->ShipFacing};
	angle = FacingToAngle(facing);
	cx = ShipPtr->next.location.x + Cosine(angle, CenterOffs());
	cy = ShipPtr->next.location.y + Sine(angle, CenterOffs());

	if (ShipPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
	{
		int16_t ex {};
		int16_t ey {};
		LASER_BLOCK LaserBlock;
		ELEMENT* LaserPtr {};

		LaserBlock.sender = ShipPtr->playerNr;
		LaserBlock.flags = 0;
		LaserBlock.pixoffs = 0;
		LaserBlock.color = BuildColor(MakeRgb15(0x1F, 0x0A, 0x0A), 0x0C);
		LaserBlock.face = facing;

		ex = cx + Cosine(angle, LaserRange());
		ey = cy + Sine(angle, LaserRange());
		offs_x = -Sine(angle, WingOffs());
		offs_y = Cosine(angle, WingOffs());

		LaserBlock.cx = cx + offs_x;
		LaserBlock.cy = cy + offs_y;
		LaserBlock.ex = ex - LaserBlock.cx;
		LaserBlock.ey = ey - LaserBlock.cy;
		if ((WeaponArray[0] = initialize_laser(&LaserBlock)))
		{
			LockElement(WeaponArray[0], &LaserPtr);
			LaserPtr->collision_func = twin_laser_collision;
			UnlockElement(WeaponArray[0]);
		}

		LaserBlock.cx = cx - offs_x;
		LaserBlock.cy = cy - offs_y;
		LaserBlock.ex = ex - LaserBlock.cx;
		LaserBlock.ey = ey - LaserBlock.cy;
		if ((WeaponArray[1] = initialize_laser(&LaserBlock)))
		{
			LockElement(WeaponArray[1], &LaserPtr);
			LaserPtr->collision_func = twin_laser_collision;
			UnlockElement(WeaponArray[1]);
		}
	}
	else
	{
		MISSILE_BLOCK TorpBlock;
		ELEMENT* TorpPtr {};

		TorpBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
		TorpBlock.sender = ShipPtr->playerNr;
		TorpBlock.flags = IgnoreSimilar;
		TorpBlock.pixoffs = 0;
		TorpBlock.speed = MissileSpeed();
		TorpBlock.hit_points = MissileHits;
		TorpBlock.damage = MissileDamage;
		TorpBlock.life = MissileLife;
		TorpBlock.preprocess_func = missile_preprocess;
		TorpBlock.blast_offs = MissileOffset;

		TorpBlock.face = TorpBlock.index = NormalizeFacing(facing - 1);
		offs_x = -Sine(FacingToAngle(TorpBlock.face), LaunchOffs());
		offs_y = Cosine(FacingToAngle(TorpBlock.face), LaunchOffs());

		TorpBlock.cx = cx + offs_x;
		TorpBlock.cy = cy + offs_y;
		if ((WeaponArray[0] = initialize_missile(&TorpBlock)))
		{
			LockElement(WeaponArray[0], &TorpPtr);
			TorpPtr->turn_wait = TrackWait;
			UnlockElement(WeaponArray[0]);
		}

		TorpBlock.face = TorpBlock.index = NormalizeFacing(facing + 1);

		TorpBlock.cx = cx - offs_x;
		TorpBlock.cy = cy - offs_y;
		if ((WeaponArray[1] = initialize_missile(&TorpBlock)))
		{
			LockElement(WeaponArray[1], &TorpPtr);
			TorpPtr->turn_wait = TrackWait;
			UnlockElement(WeaponArray[1]);
		}
	}

	return (2);
}

static void
mmrnmhrm_postprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	/* take care of transform effect */
	if (ElementPtr->next.image.farray != ElementPtr->current.image.farray)
	{
		MMRNMHRM_DATA tempShipData;
		const MMRNMHRM_DATA* otherwing_desc {};

		ProcessSound(GetSound(
						 /* TRANSFORM */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);

		StarShipPtr->weapon_counter = 0;

		/* Swap characteristics descriptors around */
		otherwing_desc = GetCustomShipData(StarShipPtr->RaceDescPtr);
		if (!otherwing_desc)
		{
			return; // No ship data (?!)
		}

		tempShipData = *otherwing_desc;
		SetCustomShipData(StarShipPtr->RaceDescPtr, &StarShipPtr->RaceDescPtr->characteristics);
		StarShipPtr->RaceDescPtr->characteristics = tempShipData;
		StarShipPtr->RaceDescPtr->cyborg_control.ManeuverabilityIndex = 0;

		if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = ResScale(LongRangeWeapon - 1);
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~ImmediateWeapon;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags |= SeekingWeapon;
			StarShipPtr->RaceDescPtr->ship_data.weapon_sound_index = 2;

			StarShipPtr->cur_status_flags &=
				~(ShipAtMaxSpeed | ShipBeyondMaxSpeed);
		}
		else
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = ResScale(CloseRangeWeapon);
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~SeekingWeapon;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags |= ImmediateWeapon;
			StarShipPtr->RaceDescPtr->ship_data.weapon_sound_index = 0;

			if (StarShipPtr->cur_status_flags
				& (ShipAtMaxSpeed | ShipBeyondMaxSpeed))
			{
				StarShipPtr->cur_status_flags |=
					ShipAtMaxSpeed | ShipBeyondMaxSpeed;
			}
		}
	}
}

static void
mmrnmhrm_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);

	if (!(ElementPtr->state_flags & Appearing))
	{
		if ((StarShipPtr->cur_status_flags & SpecialFlag)
			&& StarShipPtr->special_counter == 0)
		{
			/* Either we transform or text will flash */
			if (DeltaEnergy(ElementPtr,
							-StarShipPtr->RaceDescPtr->characteristics.special_energy_cost))
			{
				if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
				{
					ElementPtr->next.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.special;
				}
				else
				{
					ElementPtr->next.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.ship;
				}
				ElementPtr->next.image.frame =
					SetEquFrameIndex(ElementPtr->next.image.farray[0],
									 ElementPtr->next.image.frame);
				ElementPtr->state_flags |= Changing;

				StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
			}
		}
	}
}

static void
uninit_mmrnmhrm(RACE_DESC* pRaceDesc)
{
	SetCustomShipData(pRaceDesc, nullptr);
}

RACE_DESC*
init_mmrnmhrm(void)
{
	// The caller of this func will copy the struct
	static RACE_DESC new_mmrnmhrm_desc;
	MMRNMHRM_DATA otherwing_desc;

	if (IsHd())
	{
		mmrnmhrm_desc.characteristics.max_thrust = ResScale(MaxThrust);
		mmrnmhrm_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		mmrnmhrm_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd;
	}
	else
	{
		mmrnmhrm_desc.characteristics.max_thrust = MaxThrust;
		mmrnmhrm_desc.characteristics.thrust_increment = ThrustIncrement;
		mmrnmhrm_desc.cyborg_control.WeaponRange = CloseRangeWeapon;
	}

	mmrnmhrm_desc.uninit_func = uninit_mmrnmhrm;
	mmrnmhrm_desc.preprocess_func = mmrnmhrm_preprocess;
	mmrnmhrm_desc.postprocess_func = mmrnmhrm_postprocess;
	mmrnmhrm_desc.init_weapon_func = initialize_dual_weapons;
	mmrnmhrm_desc.cyborg_control.intelligence_func = mmrnmhrm_intelligence;

	new_mmrnmhrm_desc = mmrnmhrm_desc;

	otherwing_desc.max_thrust = YwingMaxThrust();
	otherwing_desc.thrust_increment = YwingThrustIncrement();
	otherwing_desc.energy_regeneration = YwingEnergyRegeneration;
	otherwing_desc.weapon_energy_cost = YwingWeaponEnergyCost;
	otherwing_desc.special_energy_cost = YwingSpecialEnergyCost;
	otherwing_desc.energy_wait = YwingEnergyWait;
	otherwing_desc.turn_wait = YwingTurnWait;
	otherwing_desc.thrust_wait = YwingThrustWait;
	otherwing_desc.weapon_wait = YwingWeaponWait;
	otherwing_desc.special_wait = YwingSpecialWait;
	otherwing_desc.ship_mass = ShipMass;

	SetCustomShipData(&new_mmrnmhrm_desc, &otherwing_desc);

	RACE_DESC* RaceDescPtr {&new_mmrnmhrm_desc};

	return (RaceDescPtr);
}
