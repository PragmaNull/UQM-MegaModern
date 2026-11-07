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

#include "pkunk.h"

#include "../../settings.h"
#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/build.h" // for fwiffo
#include "uqm/globdata.h"
#include "uqm/nameref.h" // for fwiffo
#include "uqm/random/UQMRandom.h"
#include "uqm/tactrans.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {8};
static constexpr auto MaxEnergy {12};
static constexpr auto EnergyRegeneration {0};
static constexpr auto EnergyWait {0};
static constexpr auto MaxThrust {64};
static constexpr auto ThrustIncrement {16};
static constexpr auto ThrustWait {0};
static constexpr auto TurnWait {0};
static constexpr auto ShipMass {1};

// Triple Miniguns
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {0};
static auto PkunkOffset() { return ResScale(15); }
static auto MissileOffset() { return ResScale(1); }
static constexpr auto MissileSpeed {DisplayToWorld(24)};
static constexpr auto MissileLife {5};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};

// Taunt
static constexpr auto SpecialEnergyCost {2};
static constexpr auto SpecialWait {16};

// Respawn
static constexpr auto PhoenixLife {12};
static constexpr auto StartPhoenixColor {BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7A)};
static constexpr auto TransitionLife {1};
static auto TransitionSpeed() { return DisplayToWorld(ResScale(20)); }

static RaceDesc pkunk_desc =
	{
		{/* ShipInfoDesc */
		 "fury",
		 ShipFlags::FiresFore | ShipFlags::FiresLeft | ShipFlags::FiresRight,
		 20, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 PkunkRaceStrings,
		 PkunkIconMaskPmapAnim,
		 PkunkMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			666 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				502,
				401,
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
			0, /* SpecialWait */
			ShipMass,
		 },
		{{
			 PkunkBigMaskPmapAnim,
			 PkunkMedMaskPmapAnim,
			 PkunkSmlMaskPmapAnim,
		 },
		 {
			 BugBigMaskPmapAnim,
			 BugMedMaskPmapAnim,
			 BugSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {PkunkCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 PkunkVictorySong,
		 PkunkShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			CloseRangeWeapon + 1,
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
struct PkunkData
{
	ElementHandle hPhoenix;
	ElementProcessFunc* saved_preprocess_func;
	ElementProcessFunc* saved_postprocess_func;
	ElementProcessFunc* saved_death_func;
};

// Local typedef
typedef PkunkData CustomShipData_t;

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

static void animate(Element* ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->current.image.frame);
		ElementPtr->state_flags |= ElementFlags::Changing;

		ElementPtr->turn_wait = ElementPtr->next_turn;
	}
}

static uint16_t initialize_bug_missile(Element* ShipPtr, ElementHandle MissileArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
	MissileBlock.pixoffs = PkunkOffset();
	MissileBlock.speed = ResScale(MissileSpeed);
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();

	for (uint16_t i {0}; i < 3; ++i)
	{
		MissileBlock.face =
			StarShipPtr->ShipFacing
			+ (AngleToFacing(Quadrant) * i);
		if (i == 2)
		{
			MissileBlock.face += AngleToFacing(Quadrant);
		}
		MissileBlock.face = NormalizeFacing(MissileBlock.face);

		if ((MissileArray[i] = initialize_missile(&MissileBlock)))
		{
			int32_t dx {};
			int32_t dy {};
			Element* MissilePtr {};

			LockElement(MissileArray[i], &MissilePtr);
			GetCurrentVelocityComponentsSdword(&ShipPtr->velocity, &dx, &dy);
			DeltaVelocityComponents(&MissilePtr->velocity, dx, dy);
			MissilePtr->current.location.x -= VelocityToWorld(dx);
			MissilePtr->current.location.y -= VelocityToWorld(dy);

			MissilePtr->preprocess_func = animate;
			UnlockElement(MissileArray[i]);
		}
	}

	return (3);
}

static void pkunk_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							   uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	PkunkData* pkunkData {GetCustomShipData(StarShipPtr->RaceDescPtr)};
	if (pkunkData->hPhoenix && testFlag(StarShipPtr->control, PlayerControlFlags::DifficultyStandard))
	{
		RemoveElement(pkunkData->hPhoenix);
		FreeElement(pkunkData->hPhoenix);
		pkunkData->hPhoenix = 0;
	}

	if (StarShipPtr->RaceDescPtr->ship_info.energy_level < StarShipPtr->RaceDescPtr->ship_info.max_energy
		&& (StarShipPtr->special_counter == 0
			|| (uint8_t)uqm::random().next() < 20))
	{
		StarShipPtr->ship_input_state |= StatusFlags::Special;
	}
	else
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Special;
	}
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
}

static void pkunk_preprocess(Element* ElementPtr);

static void new_pkunk(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	const PkunkData* pkunkData {GetCustomShipData(StarShipPtr->RaceDescPtr)};

	ElementPtr->state_flags = ElementFlags::Appearing | ElementFlags::PlayerShip | ElementFlags::IgnoreSimilar;
	ElementPtr->mass_points = ShipMass;
	// Restore the Element processing callbacks after the explosion.
	// The callbacks were changed for the explosion sequence
	ElementPtr->preprocess_func = pkunkData->saved_preprocess_func;
	ElementPtr->postprocess_func = pkunkData->saved_postprocess_func;
	ElementPtr->death_func = pkunkData->saved_death_func;
	// preprocess_func() is called during the phoenix transition and
	// then cleared, so we need to restore it
	StarShipPtr->RaceDescPtr->preprocess_func = pkunk_preprocess;
	StarShipPtr->RaceDescPtr->ship_info.crew_level = MaxCrew;
	StarShipPtr->RaceDescPtr->ship_info.energy_level = MaxEnergy;
	/* fix vux impairment */
	StarShipPtr->RaceDescPtr->characteristics.max_thrust = ResScale(MaxThrust);
	StarShipPtr->RaceDescPtr->characteristics.thrust_increment = ResScale(ThrustIncrement);
	StarShipPtr->RaceDescPtr->characteristics.turn_wait = TurnWait;
	StarShipPtr->RaceDescPtr->characteristics.thrust_wait = ThrustWait;
	StarShipPtr->RaceDescPtr->characteristics.special_wait = 0;

	StarShipPtr->ship_input_state = StatusFlags::None;
	// Pkunk wins in a simultaneous destruction if it reincarnates
	StarShipPtr->cur_status_flags &= StatusFlags::PlayVictoryDitty;
	StarShipPtr->old_status_flags = StatusFlags::None;
	StarShipPtr->energy_counter = 0;
	StarShipPtr->weapon_counter = 0;
	StarShipPtr->special_counter = 0;
	ElementPtr->crew_level = 0;
	ElementPtr->turn_wait = 0;
	ElementPtr->thrust_wait = 0;
	ElementPtr->life_span = NormalLife;

	StarShipPtr->ShipFacing = NormalizeFacing(uqm::random().next());
	ElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
	ElementPtr->current.image.frame = SetAbsFrameIndex(
		StarShipPtr->RaceDescPtr->ship_data.ship[0],
		StarShipPtr->ShipFacing);
	SetPrimType(&DisplayArray[ElementPtr->PrimIndex], PrimitiveType::Stamp);

	do
	{
		ElementPtr->current.location.x =
			WrapX(DisplayAlignX(uqm::random().next()));
		ElementPtr->current.location.y =
			WrapY(DisplayAlignY(uqm::random().next()));
	} while (CalculateGravity(ElementPtr)
			 || TimeSpaceMatterConflict(ElementPtr));

	ZeroVelocityComponents(&ElementPtr->velocity);

	// XXX: Hack: Set hTarget!=0 so that ship_preprocess() does not
	//   call ship_transition() for us.
	ElementPtr->hTarget = StarShipPtr->hShip;
}

// This function is called when the ship dies but reincarnates.
// The generic ship_death() function is not called for the ship in this case.
static void pkunk_reincarnation_death(Element* ShipPtr)
{
	// Simulate ship death
	StopAllBattleMusic();
	StartShipExplosion(ShipPtr, true);
	// Once the explosion ends, we will get a brand new ship
	ShipPtr->death_func = new_pkunk;
}

static void intercept_pkunk_death(Element* ElementPtr)
{
	Starship* StarShipPtr {};
	Element* ShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	PkunkData* pkunkData {GetCustomShipData(StarShipPtr->RaceDescPtr)};

	if (StarShipPtr->RaceDescPtr->ship_info.crew_level != 0)
	{ // Ship not dead yet.
		// Keep the Phoenix Element alive.
		ElementPtr->state_flags &= ~ElementFlags::Disappearing;
		ElementPtr->life_span = 1;
		return;
	}

	LockElement(StarShipPtr->hShip, &ShipPtr);
	// GravityMass() indicates a warp-out here. If Pkunk dies while warping
	// out, there is no reincarnation.
	if (!GravityMass(ShipPtr->mass_points + 1))
	{
		// XXX: Hack: Set mass_points to indicate a reincarnation to
		//   FindAliveStarShip()
		ShipPtr->mass_points = MaxShipMass + 1;
		// Save the various Element processing callbacks before the
		// explosion happens, because we were not the ones who set
		// these callbacks and they are about to be changed.
		pkunkData->saved_preprocess_func = ShipPtr->preprocess_func;
		pkunkData->saved_postprocess_func = ShipPtr->postprocess_func;
		pkunkData->saved_death_func = ShipPtr->death_func;

		ShipPtr->death_func = pkunk_reincarnation_death;
	}
	UnlockElement(StarShipPtr->hShip);
}

static void spawn_phoenix_trail(Element* ElementPtr)
{
	const static Color colorTable[] =
		{
			BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7a),
			BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7b),
			BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7c),
			BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7d),
			BuildColor(MakeRgb15(0x1F, 0x07, 0x00), 0x7e),
			BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7f),
			BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x2a),
			BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2b),
			BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2c),
			BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2d),
			BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2e),
			BuildColor(MakeRgb15(0x0B, 0x00, 0x00), 0x2f),
		};
	const size_t colorTableCount = std::size(colorTable);

	ElementPtr->colorCycleIndex++;
	if (ElementPtr->colorCycleIndex != colorTableCount)
	{
		ElementPtr->life_span = TransitionLife;

		SetPrimColor(&DisplayArray[ElementPtr->PrimIndex],
					 colorTable[ElementPtr->colorCycleIndex]);

		ElementPtr->state_flags &= ~ElementFlags::Disappearing;
		ElementPtr->state_flags |= ElementFlags::Changing;
	} // else, the Element disappears.
}

static void phoenix_transition(Element* ElementPtr)
{
	ElementHandle hShipImage {};
	Element* ShipImagePtr {};
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	LockElement(StarShipPtr->hShip, &ShipImagePtr);

	if (!(testFlag(ShipImagePtr->state_flags, ElementFlags::Nonsolid)))
	{
		ElementPtr->preprocess_func = nullptr;
	}
	else if ((hShipImage = AllocElement()))
	{
		uint16_t angle {};

		PutElement(hShipImage);

		LockElement(hShipImage, &ShipImagePtr);
		ShipImagePtr->playerNr = NeutralPlayerNum;
		ShipImagePtr->state_flags = ElementFlags::Appearing | ElementFlags::FiniteLife | ElementFlags::Nonsolid;
		ShipImagePtr->life_span = TransitionLife;
		SetPrimType(&DisplayArray[ShipImagePtr->PrimIndex],
					PrimitiveType::StampFill);
		SetPrimColor(
			&DisplayArray[ShipImagePtr->PrimIndex],
			StartPhoenixColor);
		ShipImagePtr->colorCycleIndex = 0;
		ShipImagePtr->current.image = ElementPtr->current.image;
		ShipImagePtr->current.location = ElementPtr->current.location;
		if (!(testFlag(ElementPtr->state_flags, ElementFlags::PlayerShip)))
		{
			angle = ElementPtr->mass_points;

			ShipImagePtr->current.location.x +=
				Cosine(angle, TransitionSpeed());
			ShipImagePtr->current.location.y +=
				Sine(angle, TransitionSpeed());
			ElementPtr->preprocess_func = nullptr;
		}
		else
		{
			angle = FacingToAngle(StarShipPtr->ShipFacing);

			// JMS_GFX: Circumventing overflows by using temp variables
			// instead of subtracting straight from the GfxPoint sized
			// ShipImagePtr->current.location.
			const int32_t temp_x {(int32_t)ShipImagePtr->current.location.x - Cosine(angle, TransitionSpeed()) * (ElementPtr->life_span - 1)};
			const int32_t temp_y {(int32_t)ShipImagePtr->current.location.y - Sine(angle, TransitionSpeed()) * (ElementPtr->life_span - 1)};

			ShipImagePtr->current.location.x = WrapX(temp_x);
			ShipImagePtr->current.location.y = WrapY(temp_y);
		}

		ShipImagePtr->mass_points = (uint8_t)angle;
		ShipImagePtr->preprocess_func = phoenix_transition;
		ShipImagePtr->death_func = spawn_phoenix_trail;
		SetElementStarShip(ShipImagePtr, StarShipPtr);

		UnlockElement(hShipImage);
	}

	UnlockElement(StarShipPtr->hShip);
}

SoundBankRef FwiffoSounds = 0;

static void pkunk_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	PkunkData* pkunkData {GetCustomShipData(StarShipPtr->RaceDescPtr)};

	const bool Fwiffo = (StarShipPtr->captains_name_index == NameOffset + NumCaptainsNames && StarShipPtr->SpeciesID == SpeciesId::Spathi);
	if (Fwiffo && !FwiffoSounds)
	{
		FwiffoSounds = uqm::loadSoundBank("ship.spathi.sounds").release();
	}

	if (testFlag(ElementPtr->state_flags, ElementFlags::Appearing))
	{
		ElementHandle hPhoenix = 0;

		if (uqm::random().next() & 1)
		{
			hPhoenix = AllocElement();
		}

		// The hPhoenix Element is created and placed at the head of the
		// queue so that it is preprocessed before any of the ships' elements
		// are, and so before death_func() is called for the dead Pkunk.
		// hPhoenix detects when the Pkunk ship dies and tweaks the ship,
		// starting the death + reincarnation sequence.
		if (hPhoenix)
		{
			Element* PhoenixPtr {};

			LockElement(hPhoenix, &PhoenixPtr);
			PhoenixPtr->playerNr = ElementPtr->playerNr;
			PhoenixPtr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid | ElementFlags::IgnoreSimilar;
			PhoenixPtr->life_span = 1;

			PhoenixPtr->death_func = intercept_pkunk_death;

			SetElementStarShip(PhoenixPtr, StarShipPtr);

			UnlockElement(hPhoenix);
			InsertElement(hPhoenix, GetHeadElement());
		}
		pkunkData->hPhoenix = hPhoenix;

		// XXX: Hack: new_pkunk() sets hTarget!=0 which indicates a
		//   reincarnation to us.
		if (ElementPtr->hTarget == 0)
		{
			// A brand new ship is preprocessed only once
			StarShipPtr->RaceDescPtr->preprocess_func = 0;
		}
		else
		{ // Start the reincarnation sequence

			if (Fwiffo)
			{
				ProcessSound(GetSound(FwiffoSounds, 1), ElementPtr);
			}
			else
			{
				ProcessSound(GetSound(
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
							 ElementPtr);
			}

			ElementPtr->life_span = PhoenixLife;
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
						NoPrim);
			ElementPtr->state_flags |= ElementFlags::Nonsolid | ElementFlags::FiniteLife | ElementFlags::Changing;

			const uint16_t facing {StarShipPtr->ShipFacing};
			for (uint16_t angle {Octant}; angle < FullCircle; angle += Quadrant)
			{
				StarShipPtr->ShipFacing = NormalizeFacing(
					facing + AngleToFacing(angle));
				phoenix_transition(ElementPtr);
			}
			StarShipPtr->ShipFacing = facing;
		}
	}

	if (StarShipPtr->RaceDescPtr->preprocess_func)
	{
		StarShipPtr->cur_status_flags &=
			~(StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon | StatusFlags::Special);

		if (ElementPtr->life_span == NormalLife)
		{
			ElementPtr->current.image.frame =
				ElementPtr->next.image.frame =
					SetEquFrameIndex(
						ElementPtr->current.image.farray[0],
						ElementPtr->current.image.frame);
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
						PrimitiveType::Stamp);
			InitIntersectStartPoint(ElementPtr);
			InitIntersectEndPoint(ElementPtr);
			InitIntersectFrame(ElementPtr);
			ZeroVelocityComponents(&ElementPtr->velocity);
			ElementPtr->state_flags &= ~(ElementFlags::Nonsolid | ElementFlags::FiniteLife);
			ElementPtr->state_flags |= ElementFlags::Changing;

			StarShipPtr->RaceDescPtr->preprocess_func = 0;
		}
	}
}

static uint16_t LastSound = 0;

static void pkunk_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->characteristics.special_wait)
	{
		--StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
	else if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
			 && StarShipPtr->RaceDescPtr->ship_info.energy_level < StarShipPtr->RaceDescPtr->ship_info.max_energy)
	{
		uint16_t CurSound {};

		do
		{
			CurSound =
				2 + ((uint16_t)uqm::random().next() % (StarShipPtr->RaceDescPtr->ship_data.ship_sounds->size() - 2));
		} while (CurSound == LastSound);
		ProcessSound(GetSound(
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, CurSound),
					 ElementPtr);
		LastSound = CurSound;

		DeltaEnergy(ElementPtr, SpecialEnergyCost);

		StarShipPtr->RaceDescPtr->characteristics.special_wait = SpecialWait;
	}
}

static void uninit_pkunk(RaceDesc* pRaceDesc)
{
	SetCustomShipData(pRaceDesc, nullptr);
	if (FwiffoSounds)
	{
		delete FwiffoSounds;
		FwiffoSounds = 0;
	}
}

RaceDesc* init_pkunk(void)
{
	static RaceDesc new_pkunk_desc;
	RaceDesc* RaceDescPtr {};
	PkunkData empty_data;

	if (IsHd())
	{
		pkunk_desc.characteristics.max_thrust = ResScale(MaxThrust);
		pkunk_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		pkunk_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd + 4;
	}
	else
	{
		pkunk_desc.characteristics.max_thrust = MaxThrust;
		pkunk_desc.characteristics.thrust_increment = ThrustIncrement;
		pkunk_desc.cyborg_control.WeaponRange = CloseRangeWeapon + 1;
	}

	// The caller of this func will copy the struct
	memset(&empty_data, 0, sizeof(empty_data));

	pkunk_desc.uninit_func = uninit_pkunk;
	pkunk_desc.preprocess_func = pkunk_preprocess;
	pkunk_desc.postprocess_func = pkunk_postprocess;
	pkunk_desc.init_weapon_func = initialize_bug_missile;
	pkunk_desc.cyborg_control.intelligence_func = pkunk_intelligence;

	/* copy initial ship settings to the new descriptor */
	new_pkunk_desc = pkunk_desc;
	SetCustomShipData(&new_pkunk_desc, &empty_data);

	RaceDescPtr = &new_pkunk_desc;

	LastSound = 0;
	// We need to reinitialise it at least each battle, to ensure
	// that NetPlay is synchronised if one player played another
	// game before playing against a networked opponent.

	return (RaceDescPtr);
}

} // namespace uqm
