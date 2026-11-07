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

#include "mycon.h"

#include "../ship.h"
#include "resinst.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {20};
static constexpr auto MaxEnergy {40};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {27};
static constexpr auto ThrustIncrement {9};
static constexpr auto ThrustWait {6};
static constexpr auto TurnWait {6};
static constexpr auto ShipMass {7};

// Plasmoid
static constexpr auto WeaponEnergyCost {20};
static constexpr auto WeaponWait {5};
static auto MyconOffset() { return ResScale(24); }
static constexpr auto MissileOffset {0};
static constexpr auto NumPlasmas {11};
static constexpr auto NumGloballs {8};
static constexpr auto PlasmaDuration {13};
static constexpr auto MissileLife {NumPlasmas * PlasmaDuration};
static auto MissileSpeed() { return DisplayToWorld(ResScale(8)); }
static constexpr auto MissileDamage {10};
static constexpr auto TrackWait {1};

// Regenerate
static constexpr auto SpecialEnergyCost {MaxEnergy};
static constexpr auto SpecialWait {0};
static constexpr auto RegenerationAmount {4};

static RaceDesc mycon_desc =
	{
		{/* ShipInfoDesc */
		 "podship",
		 ShipFlags::FiresFore | ShipFlags::SeekingWeapon,
		 21, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 MyconRaceStrings,
		 MyconIconMaskPmapAnim,
		 MyconMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			1070 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				6392,
				2200,
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
			 MyconBigMaskPmapAnim,
			 MyconMedMaskPmapAnim,
			 MyconSmlMaskPmapAnim,
		 },
		 {
			 PlasmaBigMaskPmapAnim,
			 PlasmaMedMaskPmapAnim,
			 PlasmaSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {MyconCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 MyconVictorySong,
		 MyconShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			DisplayToWorld(800),
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static void plasma_preprocess(Element* ElementPtr)
{
	uint16_t plasma_index {};

	if (ElementPtr->mass_points > ElementPtr->hit_points)
	{
		ElementPtr->life_span = ElementPtr->hit_points * PlasmaDuration;
	}
	else
	{
		ElementPtr->hit_points = (uint8_t)((ElementPtr->life_span * MissileDamage + (MissileLife - 1)) / MissileLife);
	}
	ElementPtr->mass_points = ElementPtr->hit_points;
	plasma_index = NumPlasmas - ((ElementPtr->life_span + (PlasmaDuration - 1)) / PlasmaDuration);
	if (plasma_index != GetFrameIndex(ElementPtr->next.image.frame))
	{
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->next.image.frame,
							 plasma_index);
		ElementPtr->state_flags |= ElementFlags::Changing;
	}

	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		uint16_t facing {};

		facing = NormalizeFacing(AngleToFacing(
			GetVelocityTravelAngle(&ElementPtr->velocity)));
		if (TrackShip(ElementPtr, &facing) > 0)
		{
			SetVelocityVector(&ElementPtr->velocity,
							  MissileSpeed(), facing);
		}

		ElementPtr->turn_wait = TrackWait;
	}
}

static void plasma_blast_preprocess(Element* ElementPtr)
{
	if (ElementPtr->life_span >= ElementPtr->thrust_wait)
	{
		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->next.image.frame);
	}
	else
	{
		ElementPtr->next.image.frame =
			DecFrameIndex(ElementPtr->next.image.frame);
	}
	if (ElementPtr->hTarget)
	{
		Element* ShipPtr {};

		LockElement(ElementPtr->hTarget, &ShipPtr);
		ElementPtr->next.location = ShipPtr->next.location;
		UnlockElement(ElementPtr->hTarget);
	}

	ElementPtr->state_flags |= ElementFlags::Changing;
}

static void plasma_collision(Element* ElementPtr0, GfxPoint* pPt0,
							 Element* ElementPtr1, GfxPoint* pPt1)
{
	ElementHandle hBlastElement {};

	const int16_t old_mass {(int16_t)ElementPtr0->mass_points};
	if ((ElementPtr0->pParent != ElementPtr1->pParent
		 || (testFlag(ElementPtr1->state_flags, ElementFlags::PlayerShip)))
		&& (hBlastElement =
				weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1)))
	{
		int16_t num_animations {};
		Element* BlastElementPtr {};

		LockElement(hBlastElement, &BlastElementPtr);
		BlastElementPtr->pParent = ElementPtr0->pParent;
		if (!(testFlag(ElementPtr1->state_flags, ElementFlags::PlayerShip)))
		{
			BlastElementPtr->hTarget = 0;
		}
		else
		{
			Starship* StarShipPtr {};

			GetElementStarShip(ElementPtr1, &StarShipPtr);
			BlastElementPtr->hTarget = StarShipPtr->hShip;
		}

		BlastElementPtr->current.location = ElementPtr1->current.location;

		if ((num_animations =
				 (old_mass * NumGloballs + (MissileDamage - 1)) / MissileDamage)
			== 0)
		{
			num_animations = 1;
		}

		BlastElementPtr->thrust_wait = (uint8_t)num_animations;
		BlastElementPtr->life_span = (num_animations << 1) - 1;
		{
			BlastElementPtr->preprocess_func = plasma_blast_preprocess;
		}
		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame =
			SetAbsFrameIndex(BlastElementPtr->current.image.farray[0],
							 NumPlasmas);

		UnlockElement(hBlastElement);
	}
}

static void mycon_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							   uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == MovementState::Entice)
	{
		if ((testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::FiniteLife))
			&& !(testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::CrewObject)))
		{
			lpEvalDesc->MoveState = MovementState::Avoid;
		}
		else
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
	}

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].MoveState == MovementState::Pursue)
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Thrust; /* don't pursue seekers */
	}

	lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyShip];
	if (StarShipPtr->weapon_counter == 0
		&& lpEvalDesc->ObjectPtr
		&& (lpEvalDesc->which_turn <= 16
			|| ShipPtr->crew_level == StarShipPtr->RaceDescPtr->ship_info.max_crew))
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
							- StarShipPtr->ShipFacing
							+ AngleToFacing(Quadrant))
				<= AngleToFacing(HalfCircle)
			&& (!(testFlag(StarShipPtr->cur_status_flags, (StatusFlags::ShipBeyondMaxSpeed | StatusFlags::ShipInGravityWell)))
				|| NormalizeFacing(direction_facing
								   - travel_facing + AngleToFacing(Octant))
					   <= AngleToFacing(Quadrant)))
		{
			StarShipPtr->ship_input_state |= StatusFlags::Weapon;
		}
	}

	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Special;
		StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = DisplayToWorld(ResScale(800));
		if (ShipPtr->crew_level < StarShipPtr->RaceDescPtr->ship_info.max_crew)
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = MissileSpeed() * MissileLife;
			if (StarShipPtr->RaceDescPtr->ship_info.energy_level >= SpecialEnergyCost
				&& !(testFlag(StarShipPtr->ship_input_state, StatusFlags::Weapon)))
			{
				StarShipPtr->ship_input_state |= StatusFlags::Special;
			}
		}
	}
}

static uint16_t initialize_plasma(Element* ShipPtr, ElementHandle PlasmaArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = ElementFlags::None;
	MissileBlock.pixoffs = MyconOffset();
	MissileBlock.speed = MissileSpeed();
	MissileBlock.hit_points = MissileDamage;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = plasma_preprocess;
	MissileBlock.blast_offs = MissileOffset;
	PlasmaArray[0] = initialize_missile(&MissileBlock);

	if (PlasmaArray[0])
	{
		Element* PlasmaPtr {};

		LockElement(PlasmaArray[0], &PlasmaPtr);
		PlasmaPtr->collision_func = plasma_collision;
		PlasmaPtr->turn_wait = TrackWait + 2;
		UnlockElement(PlasmaArray[0]);
	}

	return (1);
}

static void mycon_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
		&& StarShipPtr->special_counter == 0
		&& ElementPtr->crew_level != StarShipPtr->RaceDescPtr->ship_info.max_crew
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		int16_t add_crew {};

		ProcessSound(GetSound(
						 /* GROW_NEW_CREW */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);
		if ((add_crew = RegenerationAmount) > StarShipPtr->RaceDescPtr->ship_info.max_crew - ElementPtr->crew_level)
		{
			add_crew = StarShipPtr->RaceDescPtr->ship_info.max_crew - ElementPtr->crew_level;
		}
		DeltaCrew(ElementPtr, add_crew);

		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

RaceDesc* init_mycon(void)
{
	if (IsHd())
	{
		mycon_desc.characteristics.max_thrust = ResScale(MaxThrust);
		mycon_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		mycon_desc.cyborg_control.WeaponRange = DisplayToWorld(3200);
	}
	else
	{
		mycon_desc.characteristics.max_thrust = MaxThrust;
		mycon_desc.characteristics.thrust_increment = ThrustIncrement;
		mycon_desc.cyborg_control.WeaponRange = DisplayToWorld(800);
	}

	mycon_desc.postprocess_func = mycon_postprocess;
	mycon_desc.init_weapon_func = initialize_plasma;
	mycon_desc.cyborg_control.intelligence_func = mycon_intelligence;

	RaceDesc* RaceDescPtr {&mycon_desc};

	return (RaceDescPtr);
}

} // namespace uqm
