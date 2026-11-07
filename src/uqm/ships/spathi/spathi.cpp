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
#include "spathi.h"
#include "resinst.h"

// Core characteristics
static constexpr auto MaxCrew {30};
static constexpr auto MaxEnergy {10};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {10};
static constexpr auto MaxThrust {48};
static constexpr auto ThrustIncrement {12};
static constexpr auto ThrustWait {1};
static constexpr auto TurnWait {1};
static constexpr auto ShipMass {5};

// Forward gun
static constexpr auto WeaponEnergyCost {2};
static constexpr auto WeaponWait {0};
static auto SpathiForwardOffset() { return chooseIfHd(16, 59); }
static constexpr auto MissileSpeed {DisplayToWorld(30)};
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};
static auto MissileOffset() { return ResScale(1); }
static constexpr auto MissileRange {(MissileSpeed * MissileLife)};
/* This is for the cyborg only. */

// B.U.T.T.
static constexpr auto SpecialEnergyCost {3};
static constexpr auto SpecialWait {7};
static auto SpathiRearOffset() { return chooseIfHd(20, 71); }
static auto DiscriminatorSpeed() { return DisplayToWorld(ResScale(8)); }
static constexpr auto DiscriminatorLife {30};
static constexpr auto DiscriminatorHits {1};
static constexpr auto DiscriminatorDamage {2};
static auto DiscriminatorOffset() { return ResScale(4); }
static constexpr auto TrackWait {1};

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }
static auto MissileRangeHd() { return (MissileSpeedHd() * MissileLife); }

static RaceDesc spathi_desc =
	{
		{/* ShipInfoDesc */
		 "eluder",
		 FiresFore | FiresAft | SeekingSpecial | DontChase,
		 18, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 SpathiRaceStrings,
		 SpathiIconMaskPmapAnim,
		 SpathiMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			1000 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				2549,
				3600,
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
			 SpathiBigMaskPmapAnim,
			 SpathiMedMaskPmapAnim,
			 SpathiSmlMaskPmapAnim,
		 },
		 {
			 MissileBigMaskPmapAnim,
			 MissileMedMaskPmapAnim,
			 MissileSmlMaskPmapAnim,
		 },
		 {
			 DiscrimBigMaskPmapAnim,
			 DiscrimMedMaskPmapAnim,
			 DiscrimSmlMaskPmapAnim,
		 },
		 {SpathiCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 SpathiVictorySong,
		 SpathiShipSounds,
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

static void butt_missile_preprocess(Element* ElementPtr)
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
							  DiscriminatorSpeed(), facing);
		}

		ElementPtr->turn_wait = TrackWait;
	}
}

static void spawn_butt_missile(Element* ShipPtr)
{
	Starship* StarShipPtr {};
	MissileDesc ButtMissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	ButtMissileBlock.cx = ShipPtr->next.location.x;
	ButtMissileBlock.cy = ShipPtr->next.location.y;
	ButtMissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	ButtMissileBlock.face = ButtMissileBlock.index =
		NormalizeFacing(StarShipPtr->ShipFacing
						+ AngleToFacing(HalfCircle));
	ButtMissileBlock.sender = ShipPtr->playerNr;
	ButtMissileBlock.flags = 0;
	ButtMissileBlock.pixoffs = SpathiRearOffset();
	ButtMissileBlock.speed = DiscriminatorSpeed();
	ButtMissileBlock.hit_points = DiscriminatorHits;
	ButtMissileBlock.damage = DiscriminatorDamage;
	ButtMissileBlock.life = DiscriminatorLife;
	ButtMissileBlock.preprocess_func = butt_missile_preprocess;
	ButtMissileBlock.blast_offs = DiscriminatorOffset();
	HELEMENT ButtMissile {initialize_missile(&ButtMissileBlock)};
	if (ButtMissile)
	{
		Element* ButtPtr {};

		LockElement(ButtMissile, &ButtPtr);
		ButtPtr->turn_wait = TrackWait;
		SetElementStarShip(ButtPtr, StarShipPtr);

		ProcessSound(GetSound(
						 /* LAUNCH_BUTT_MISSILE */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ButtPtr);

		UnlockElement(ButtMissile);
		PutElement(ButtMissile);
	}
}

static void spathi_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state &= ~SpecialFlag;

	const ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (StarShipPtr->special_counter == 0
		&& lpEvalDesc->ObjectPtr
		&& lpEvalDesc->which_turn <= 24)
	{
		uint16_t travel_facing {};
		uint16_t direction_facing {};
		int32_t delta_x {};
		int32_t delta_y {};

		travel_facing = NormalizeFacing(
			AngleToFacing(GetVelocityTravelAngle(&ShipPtr->velocity)
						  + HalfCircle));
		delta_x = lpEvalDesc->ObjectPtr->current.location.x
				- ShipPtr->current.location.x;
		delta_y = lpEvalDesc->ObjectPtr->current.location.y
				- ShipPtr->current.location.y;
		direction_facing = NormalizeFacing(
			AngleToFacing(ARCTAN(delta_x, delta_y)));

		if (NormalizeFacing(direction_facing
							- (StarShipPtr->ShipFacing + AngleToFacing(HalfCircle))
							+ AngleToFacing(Quadrant))
				<= AngleToFacing(HalfCircle)
			&& (lpEvalDesc->which_turn <= 8
				|| NormalizeFacing(direction_facing
								   + AngleToFacing(HalfCircle)
								   - AngleToFacing(GetVelocityTravelAngle(
									   &lpEvalDesc->ObjectPtr->velocity))
								   + AngleToFacing(Quadrant))
					   <= AngleToFacing(HalfCircle))
			&& (!(StarShipPtr->cur_status_flags & (ShipBeyondMaxSpeed | ShipInGravityWell))
				|| NormalizeFacing(direction_facing
								   - travel_facing + AngleToFacing(Quadrant))
					   <= AngleToFacing(HalfCircle)))
		{
			StarShipPtr->ship_input_state |= SpecialFlag;
		}
	}
}

static uint16_t initialize_standard_missile(Element* ShipPtr, HELEMENT MissileArray[])
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
	MissileBlock.pixoffs = SpathiForwardOffset();
	MissileBlock.speed = MissileSpeedHd();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();
	MissileArray[0] = initialize_missile(&MissileBlock);

	return (1);
}

static void spathi_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SpecialFlag)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		spawn_butt_missile(ElementPtr);

		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

RaceDesc* init_spathi(void)
{
	if (IsHd())
	{
		spathi_desc.characteristics.max_thrust = ResScale(MaxThrust);
		spathi_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		spathi_desc.cyborg_control.WeaponRange = MissileRangeHd();
	}
	else
	{
		spathi_desc.characteristics.max_thrust = MaxThrust;
		spathi_desc.characteristics.thrust_increment = ThrustIncrement;
		spathi_desc.cyborg_control.WeaponRange = MissileRange;
	}

	spathi_desc.postprocess_func = spathi_postprocess;
	spathi_desc.init_weapon_func = initialize_standard_missile;
	spathi_desc.cyborg_control.intelligence_func = spathi_intelligence;

	RaceDesc* RaceDescPtr {&spathi_desc};

	return (RaceDescPtr);
}
