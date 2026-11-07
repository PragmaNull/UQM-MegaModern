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

#include "syreen.h"

#include "../../setup.h"
#include "../../setupmenu.h"
#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"

namespace uqm
{
// Core characteristics

static constexpr auto MaxCrew {12};
static constexpr auto MaxEnergy {16};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {6};
static constexpr auto MaxThrust {36};
static constexpr auto ThrustIncrement {9};
static constexpr auto ThrustWait {1};
static constexpr auto TurnWait {1};
static constexpr auto ShipMass {2};

// Particle Beam Stiletto
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {8};
static auto SyreenOffset() { return ResScale(30); }
static constexpr auto MissileSpeed {DisplayToWorld(30)};
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {2};
static auto MissileOffset() { return ResScale(3); }

// Syreen song
static constexpr auto SpecialEnergyCost {5};
static constexpr auto SpecialWait {20};
static auto AbandonerRange() { return ResScale(208); } /* originally SpaceHeight() */
static constexpr auto MaxAbandoners {8};

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }

static RaceDesc syreen_desc =
	{
		{/* ShipInfoDesc */
		 "penetrator",
		 ShipFlags::FiresFore,
		 13, /* Super Melee cost */
		 MaxCrew, MaxCrewSize,
		 MaxEnergy, MaxEnergy,
		 SyreenRaceStrings,
		 SyreenIconMaskPmapAnim,
		 SyreenMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
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
			 SyreenBigMaskPmapAnim,
			 SyreenMedMaskPmapAnim,
			 SyreenSmlMaskPmapAnim,
		 },
		 {
			 DaggerBigMaskPmapAnim,
			 DaggerMedMaskPmapAnim,
			 DaggerSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {SyreenCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 SyreenVictorySong,
		 SyreenShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			(MissileSpeed * MissileLife * 2 / 3),
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static uint16_t initialize_dagger(Element* ShipPtr, ElementHandle DaggerArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
	MissileBlock.pixoffs = SyreenOffset();
	MissileBlock.speed = MissileSpeedHd();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();
	DaggerArray[0] = initialize_missile(&MissileBlock);

	return (1);
}

static void spawn_crew(Element* ElementPtr)
{
	if (testFlag(ElementPtr->state_flags, ElementFlags::PlayerShip))
	{
		ElementHandle hCrew {AllocElement()};
		if (hCrew != 0)
		{
			Element* CrewPtr {};

			LockElement(hCrew, &CrewPtr);
			CrewPtr->next.location = ElementPtr->next.location;
			CrewPtr->playerNr = ElementPtr->playerNr;
			CrewPtr->state_flags = ElementFlags::Appearing | ElementFlags::Nonsolid | ElementFlags::FiniteLife;
			CrewPtr->life_span = 0;
			CrewPtr->death_func = spawn_crew;
			CrewPtr->pParent = ElementPtr->pParent;
			CrewPtr->hTarget = 0;
			UnlockElement(hCrew);

			PutElement(hCrew);
		}
	}
	else
	{
		ElementHandle hElement {};
		ElementHandle hNextElement {};

		for (hElement = GetHeadElement();
			 hElement != 0; hElement = hNextElement)
		{
			Element* ObjPtr {};

			LockElement(hElement, &ObjPtr);
			hNextElement = GetSuccElement(ObjPtr);

			if ((testFlag(ObjPtr->state_flags, ElementFlags::PlayerShip))
				&& !elementsOfSamePlayer(ObjPtr, ElementPtr)
				&& ObjPtr->crew_level > 1)
			{
				uint32_t d_squared {};

				int32_t dx {ObjPtr->next.location.x - ElementPtr->next.location.x};
				if (dx < 0)
				{
					dx = -dx;
				}
				int32_t dy {ObjPtr->next.location.y - ElementPtr->next.location.y};
				if (dy < 0)
				{
					dy = -dy;
				}

				dx = WorldToDisplay(dx);
				dy = WorldToDisplay(dy);
				if (dx <= AbandonerRange() && dy <= AbandonerRange()
					&& (d_squared = (uint32_t)((uint16_t)dx * (uint16_t)dx)
								  + (uint32_t)((uint16_t)dy * (uint16_t)dy))
						   <= (uint32_t)((uint16_t)AbandonerRange() * (uint16_t)AbandonerRange()))
				{
					uint16_t crew_loss {};

					if (!antiCheat(ElementPtr, true, uqm::GodModeFlags::NoDamage))
					{
						crew_loss = ((MaxAbandoners * (AbandonerRange() - square_root(d_squared))) / AbandonerRange()) + 1;
					}
					else
					{
						crew_loss = 0;
					}

					if (crew_loss >= ObjPtr->crew_level)
					{
						crew_loss = ObjPtr->crew_level - 1;
					}

					AbandonShip(ObjPtr, ElementPtr, crew_loss);
				}
			}

			UnlockElement(hElement);
		}
	}
}

static void syreen_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								uint16_t ConcernCounter)
{
	ship_intelligence(ShipPtr,
					  ObjectsOfConcern, ConcernCounter);

	const ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (lpEvalDesc->ObjectPtr != nullptr)
	{
		Starship* StarShipPtr {};
		Starship* EnemyStarShipPtr {};

		GetElementStarShip(ShipPtr, &StarShipPtr);
		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(testFlag(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags, ShipFlags::CrewImmune))
			&& StarShipPtr->special_counter == 0
			&& lpEvalDesc->ObjectPtr->crew_level > 1
			&& lpEvalDesc->which_turn <= 14)
		{
			StarShipPtr->ship_input_state |= StatusFlags::Special;
		}
		else
		{
			StarShipPtr->ship_input_state &= ~StatusFlags::Special;
		}
	}
}

static void syreen_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		ProcessSound(GetSound(
						 /* SYREEN_SONG */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);
		spawn_crew(ElementPtr);

		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

RaceDesc* init_syreen(void)
{
	if (IsHd())
	{
		syreen_desc.characteristics.max_thrust = ResScale(MaxThrust);
		syreen_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		syreen_desc.cyborg_control.WeaponRange = (MissileSpeedHd() * MissileLife * 2 / 3);
	}
	else
	{
		syreen_desc.characteristics.max_thrust = MaxThrust;
		syreen_desc.characteristics.thrust_increment = ThrustIncrement;
		syreen_desc.cyborg_control.WeaponRange =
			(MissileSpeed * MissileLife * 2 / 3);
	}

	syreen_desc.postprocess_func = syreen_postprocess;
	syreen_desc.init_weapon_func = initialize_dagger;
	syreen_desc.cyborg_control.intelligence_func = syreen_intelligence;

	RaceDesc* RaceDescPtr {&syreen_desc};

	return (RaceDescPtr);
}

} // namespace uqm