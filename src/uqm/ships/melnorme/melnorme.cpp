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

#include "melnorme.h"

#include "../../setup.h"
#include "../../setupmenu.h"
#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/battle.h"
#include "uqm/globdata.h"
#include "uqm/random/UQMRandom.h"
#include "uqm/tactrans.h" // Kruzen: for sounds on April Fools

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {20};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {36};
static constexpr auto ThrustIncrement {6};
static constexpr auto ThrustWait {4};
static constexpr auto TurnWait {4};
static constexpr auto ShipMass {7};

// Blaster Pulse
static constexpr auto WeaponEnergyCost {5};
static constexpr auto WeaponWait {1};
static auto MelnormeOffset() { return ResScale(24); }
static constexpr auto LevelCounter {72};
static constexpr auto MaxPump {4};
static constexpr auto PumpupSpeed {DisplayToWorld(45)};
static constexpr auto PumpupLife {10};
static constexpr auto PumpupDamage {2};
static constexpr auto MinPumpitudeAnims {3};
static constexpr auto NumPumpAnims {5};
static constexpr auto ReverseDir {(uint8_t)(1 << 7)};

// Confusion Pulse
static constexpr auto SpecialEnergyCost {20};
static constexpr auto SpecialWait {20};
static auto CmissileSpeed() { return DisplayToWorld(ResScale(30)); }
static constexpr auto CmissileLife {20};
static constexpr auto CmissileHits {200};
static constexpr auto CmissileDamage {0};
static auto CmissileOffset() { return ResScale(4); }

// HdResolutionFactor
static auto PumpupSpeedHd() { return ResScale(PumpupSpeed); }

static RaceDesc melnorme_desc =
	{
		{/* ShipInfoDesc */
		 "trader",
		 ShipFlags::FiresFore,
		 18, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergySize, MaxEnergySize,
		 MelnormeRaceStrings,
		 MelnormeIconMaskPmapAnim,
		 MelnormeMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			InfiniteRadius, /* Initial sphere of influence radius */
			{
				/* Known location (center of SoI) */
				MaxXUniverse >> 1,
				MaxYUniverse >> 1,
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
			 MelnormeBigMaskPmapAnim,
			 MelnormeMedMaskPmapAnim,
			 MelnormeSmlMaskPmapAnim,
		 },
		 {
			 PumpupBigMaskPmapAnim,
			 PumpupMedMaskPmapAnim,
			 PumpupSmlMaskPmapAnim,
		 },
		 {
			 ConfuseBigMaskPmapAnim,
			 ConfuseMedMaskPmapAnim,
			 ConfuseSmlMaskPmapAnim,
		 },
		 {MelnormeCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 MelnormeVictorySong,
		 MelnormeShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			PumpupSpeed * PumpupLife,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static void pump_up_preprocess(Element* ElementPtr)
{
	if (--ElementPtr->thrust_wait & 1)
	{
		uint16_t frame_index {GetFrameIndex(ElementPtr->current.image.frame)};
		if (((ElementPtr->turn_wait & ReverseDir)
			 && (frame_index % NumPumpAnims) != 0)
			|| (!(ElementPtr->turn_wait & ReverseDir)
				&& ((frame_index + 1) % NumPumpAnims) == 0))
		{
			--frame_index;
			ElementPtr->turn_wait |= ReverseDir;
		}
		else
		{
			++frame_index;
			ElementPtr->turn_wait &= ~ReverseDir;
		}

		ElementPtr->next.image.frame = SetAbsFrameIndex(
			ElementPtr->current.image.frame, frame_index);

		ElementPtr->state_flags |= ElementFlags::Changing;
	}
}

static uint16_t initialize_pump_up(Element* ShipPtr, ElementHandle PumpUpArray[]);

static void pump_up_postprocess(Element* ElementPtr)
{
	if (testFlag(ElementPtr->state_flags, ElementFlags::Appearing))
	{
		ZeroVelocityComponents(&ElementPtr->velocity);
	}
	else
	{
		ElementHandle hPumpUp {};
		Element* EPtr {};
		Element* ShipPtr {};
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		LockElement(StarShipPtr->hShip, &ShipPtr);
		initialize_pump_up(ShipPtr, &hPumpUp);
		DeltaEnergy(ShipPtr, 0);
		UnlockElement(StarShipPtr->hShip);

		LockElement(hPumpUp, &EPtr);

		EPtr->current.image.frame = ElementPtr->current.image.frame;
		EPtr->turn_wait = ElementPtr->turn_wait;
		EPtr->thrust_wait = ElementPtr->thrust_wait;
		if (--EPtr->thrust_wait == 0)
		{
			if ((EPtr->turn_wait & ~ReverseDir) < MaxPump - 1)
			{
				++EPtr->turn_wait;
				EPtr->current.image.frame = SetRelFrameIndex(
					EPtr->current.image.frame, NumPumpAnims);
				ProcessSound(GetSound(
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 0),
							 EPtr);
			}
			if (antiCheat(ElementPtr, false, uqm::GodModeFlags::NoDamage))
			{
				EPtr->thrust_wait = 5;
			}
			else
			{
				EPtr->thrust_wait = LevelCounter;
			}
		}

		EPtr->mass_points = EPtr->hit_points =
			(PumpupDamage << (ElementPtr->turn_wait & ~ReverseDir));
		SetElementStarShip(EPtr, StarShipPtr);

		if (EPtr->thrust_wait & 1)
		{
			uint16_t frame_index {GetFrameIndex(EPtr->current.image.frame)};
			if (((EPtr->turn_wait & ReverseDir)
				 && (frame_index % NumPumpAnims) != 0)
				|| (!(EPtr->turn_wait & ReverseDir)
					&& ((frame_index + 1) % NumPumpAnims) == 0))
			{
				--frame_index;
				EPtr->turn_wait |= ReverseDir;
			}
			else
			{
				++frame_index;
				EPtr->turn_wait &= ~ReverseDir;
			}

			EPtr->current.image.frame = SetAbsFrameIndex(
				EPtr->current.image.frame, frame_index);
		}

		if (testFlag(StarShipPtr->cur_status_flags & StarShipPtr->old_status_flags, StatusFlags::Weapon))
		{
			StarShipPtr->weapon_counter = WeaponWait;
		}
		else
		{
			uint16_t angle {};

			EPtr->life_span = PumpupLife;
			EPtr->preprocess_func = pump_up_preprocess;
			EPtr->postprocess_func = 0;

			angle = FacingToAngle(StarShipPtr->ShipFacing);
			SetVelocityComponents(&EPtr->velocity,
								  Cosine(angle, WorldToVelocity(PumpupSpeedHd())),
								  Sine(angle, WorldToVelocity(PumpupSpeedHd())));

			ProcessSound(GetSound(
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						 EPtr);
		}

		UnlockElement(hPumpUp);
		PutElement(hPumpUp);

		SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
					NoPrim);
		ElementPtr->state_flags |= ElementFlags::Nonsolid;
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

static void pump_up_collision(Element* ElementPtr0, GfxPoint* pPt0,
							  Element* ElementPtr1, GfxPoint* pPt1)
{
	GfxRect r;

	GetFrameRect(ElementPtr0->next.image.frame, &r);

	const uint8_t old_thrust_wait {ElementPtr0->thrust_wait};
	ElementPtr0->blast_offset = r.extent.width >> 1;
	ElementHandle hBlastElement {weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1)};
	ElementPtr0->thrust_wait = old_thrust_wait;

	if (hBlastElement)
	{
		Element* BlastElementPtr {};

		LockElement(hBlastElement, &BlastElementPtr);

		BlastElementPtr->life_span =
			MinPumpitudeAnims
			+ (ElementPtr0->turn_wait & ~ReverseDir);
		BlastElementPtr->turn_wait = BlastElementPtr->next_turn = 0;
		{
			BlastElementPtr->preprocess_func = animate;
		}

		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame =
			SetAbsFrameIndex(BlastElementPtr->current.image.farray[0],
							 MaxPump * NumPumpAnims);

		UnlockElement(hBlastElement);
	}
}

static uint16_t initialize_pump_up(Element* ShipPtr, ElementHandle PumpUpArray[])
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
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
	MissileBlock.pixoffs = MelnormeOffset();
	MissileBlock.speed = DisplayToWorld(MelnormeOffset());
	MissileBlock.hit_points = PumpupDamage;
	MissileBlock.damage = PumpupDamage;
	MissileBlock.life = 2;
	MissileBlock.preprocess_func = 0;
	MissileBlock.blast_offs = 0;
	PumpUpArray[0] = initialize_missile(&MissileBlock);

	if (PumpUpArray[0])
	{
		Element* PumpUpPtr {};

		LockElement(PumpUpArray[0], &PumpUpPtr);
		PumpUpPtr->postprocess_func = pump_up_postprocess;
		PumpUpPtr->collision_func = pump_up_collision;

		if (antiCheat(ShipPtr, false, uqm::GodModeFlags::NoDamage))
		{
			PumpUpPtr->thrust_wait = 5;
		}
		else
		{
			PumpUpPtr->thrust_wait = LevelCounter;
		}

		UnlockElement(PumpUpArray[0]);
	}

	return (1);
}

static void confuse_preprocess_april(Element* ElementPtr)
{
	const static Color colorTable[] =
		{
			BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x7a),
			BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7a),
			BuildColor(MakeRgb15(0x1F, 0x1F, 0x00), 0x7a),
			BuildColor(MakeRgb15(0x00, 0x1F, 0x00), 0x7a),
			BuildColor(MakeRgb15(0x00, 0x1F, 0x15), 0x7a),
			BuildColor(MakeRgb15(0x00, 0x15, 0x1F), 0x7a),
			BuildColor(MakeRgb15(0x00, 0x00, 0x1F), 0x7a),
			BuildColor(MakeRgb15(0x15, 0x00, 0x1F), 0x7a),
		};
	const size_t colorTabCount = std::size(colorTable);

	if (!(testFlag(ElementPtr->state_flags, ElementFlags::Nonsolid)))
	{
		ElementPtr->next.image.frame = SetAbsFrameIndex(
			ElementPtr->current.image.frame,
			(GetFrameIndex(ElementPtr->current.image.frame) + 1) & 7);
		ElementPtr->state_flags |= ElementFlags::Changing;
	}
	else if (ElementPtr->hTarget == 0)
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= ElementFlags::Disappearing;
	}
	else
	{
		Element* eptr {};

		LockElement(ElementPtr->hTarget, &eptr);

		ElementPtr->next.location = eptr->next.location;

		if (ElementPtr->turn_wait)
		{
			Starship* StarShipPtr {};

			if (GetFrameIndex(ElementPtr->next.image.frame =
								  IncFrameIndex(ElementPtr->current.image.frame))
				== 0)
			{
				ElementPtr->next.image.frame =
					SetRelFrameIndex(ElementPtr->next.image.frame, -8);
			}

			GetElementStarShip(eptr, &StarShipPtr);
			StarShipPtr->ship_input_state =
				(StarShipPtr->ship_input_state
				 & ~(StatusFlags::Left | StatusFlags::Right | StatusFlags::Special))
				| static_cast<StatusFlags>(ElementPtr->turn_wait);

			if (ElementPtr->life_span > 1)
			{
				SetPrimType(&DisplayArray[eptr->PrimIndex],
							PrimitiveType::StampFill);
				SetPrimColor(&DisplayArray[eptr->PrimIndex],
							 colorTable[ElementPtr->colorCycleIndex]);
				ElementPtr->colorCycleIndex++;

				if (ElementPtr->colorCycleIndex == colorTabCount)
				{
					ElementPtr->colorCycleIndex = 0;
				}
			}
			else
			{
				SetPrimType(&DisplayArray[eptr->PrimIndex],
							PrimitiveType::Stamp);
				BattleSong(true);
			}

			ElementHandle hEffect {AllocElement()};
			if (hEffect)
			{
				LockElement(hEffect, &eptr);
				eptr->playerNr = ElementPtr->playerNr;
				eptr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid | ElementFlags::Changing;
				eptr->life_span = 1;
				eptr->current = eptr->next = ElementPtr->next;
				eptr->preprocess_func = confuse_preprocess_april;

				GetElementStarShip(ElementPtr, &StarShipPtr);
				SetElementStarShip(eptr, StarShipPtr);
				eptr->hTarget = ElementPtr->hTarget;

				UnlockElement(hEffect);
				PutElement(hEffect);
			}
		}

		UnlockElement(ElementPtr->hTarget);
	}
}

static void confuse_preprocess(Element* ElementPtr)
{
	if (!(testFlag(ElementPtr->state_flags, ElementFlags::Nonsolid)))
	{
		ElementPtr->next.image.frame = SetAbsFrameIndex(
			ElementPtr->current.image.frame,
			(GetFrameIndex(ElementPtr->current.image.frame) + 1) & 7);
		ElementPtr->state_flags |= ElementFlags::Changing;
	}
	else if (ElementPtr->hTarget == 0)
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= ElementFlags::Disappearing;
	}
	else
	{
		Element* eptr {};

		LockElement(ElementPtr->hTarget, &eptr);

		ElementPtr->next.location = eptr->next.location;

		if (ElementPtr->turn_wait)
		{
			Starship* StarShipPtr {};

			if (GetFrameIndex(ElementPtr->next.image.frame =
								  IncFrameIndex(ElementPtr->current.image.frame))
				== 0)
			{
				ElementPtr->next.image.frame =
					SetRelFrameIndex(ElementPtr->next.image.frame, -8);
			}

			GetElementStarShip(eptr, &StarShipPtr);
			StarShipPtr->ship_input_state =
				(StarShipPtr->ship_input_state
				 & ~(StatusFlags::Left | StatusFlags::Right | StatusFlags::Special))
				| static_cast<StatusFlags>(ElementPtr->turn_wait);

			ElementHandle hEffect {AllocElement()};
			if (hEffect)
			{
				LockElement(hEffect, &eptr);
				eptr->playerNr = ElementPtr->playerNr;
				eptr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid | ElementFlags::Changing;
				eptr->life_span = 1;
				eptr->current = eptr->next = ElementPtr->next;
				eptr->preprocess_func = confuse_preprocess;
				SetPrimType(&DisplayArray[eptr->PrimIndex],
							PrimitiveType::Stamp);

				GetElementStarShip(ElementPtr, &StarShipPtr);
				SetElementStarShip(eptr, StarShipPtr);
				eptr->hTarget = ElementPtr->hTarget;

				UnlockElement(hEffect);
				PutElement(hEffect);
			}
		}

		UnlockElement(ElementPtr->hTarget);
	}
}

static void confusion_collision(Element* ElementPtr0, GfxPoint* pPt0,
								Element* ElementPtr1, GfxPoint* pPt1)
{
	if (testFlag(ElementPtr1->state_flags, ElementFlags::PlayerShip))
	{
		ElementHandle hConfusionElement {};
		ElementHandle hNextElement {};
		Element* ConfusionPtr {};
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr0, &StarShipPtr);
		for (hConfusionElement = GetHeadElement();
			 hConfusionElement; hConfusionElement = hNextElement)
		{
			LockElement(hConfusionElement, &ConfusionPtr);
			if (elementsOfSamePlayer(ConfusionPtr, ElementPtr0)
				&& ConfusionPtr->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.special
				&& (testFlag(ConfusionPtr->state_flags, ElementFlags::Nonsolid)))
			{
				UnlockElement(hConfusionElement);
				break;
			}
			hNextElement = GetSuccElement(ConfusionPtr);
			UnlockElement(hConfusionElement);
		}

		if (hConfusionElement || (hConfusionElement = AllocElement()))
		{
			LockElement(hConfusionElement, &ConfusionPtr);

			if (ConfusionPtr->state_flags == ElementFlags::None) /* not allocated before */
			{
				InsertElement(hConfusionElement, GetHeadElement());

				ConfusionPtr->current = ElementPtr0->next;
				ConfusionPtr->current.image.frame = SetAbsFrameIndex(
					ConfusionPtr->current.image.frame, 8);
				ConfusionPtr->next = ConfusionPtr->current;
				ConfusionPtr->playerNr = ElementPtr0->playerNr;
				ConfusionPtr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid | ElementFlags::Changing;
				ConfusionPtr->preprocess_func = confuse_preprocess;
				SetPrimType(
					&DisplayArray[ConfusionPtr->PrimIndex],
					NoPrim);

				/*if (AprilFools)
				{
					ConfusionPtr->preprocess_func = confuse_preprocess_april;
					ConfusionPtr->colorCycleIndex = 0;
					StopAllBattleMusic();
					ProcessSound(GetSound(
						StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr0);
				}*/

				SetElementStarShip(ConfusionPtr, StarShipPtr);
				GetElementStarShip(ElementPtr1, &StarShipPtr);
				ConfusionPtr->hTarget = StarShipPtr->hShip;
			}

			if (!antiCheat(ElementPtr1, false, uqm::GodModeFlags::NoDamage))
			{
				ConfusionPtr->life_span = 400;
			}
			ConfusionPtr->turn_wait =
				(uint8_t)(1 << ((uint8_t)uqm::random().next() & 1)); /* LeftFlag or RightFlag */

			UnlockElement(hConfusionElement);
		}

		ElementPtr0->hit_points = 0;
		ElementPtr0->life_span = 0;
		ElementPtr0->state_flags |= ElementFlags::Disappearing | ElementFlags::Collision | ElementFlags::Nonsolid;
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static uint16_t initialize_confusion(Element* ShipPtr, ElementHandle ConfusionArray[])
{
	Starship* StarShipPtr {};
	MissileDesc ConfusionBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	ConfusionBlock.cx = ShipPtr->next.location.x;
	ConfusionBlock.cy = ShipPtr->next.location.y;
	ConfusionBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	ConfusionBlock.index = 0;
	ConfusionBlock.face = StarShipPtr->ShipFacing;
	ConfusionBlock.sender = ShipPtr->playerNr;
	ConfusionBlock.flags = ElementFlags::IgnoreSimilar;
	ConfusionBlock.pixoffs = MelnormeOffset();
	ConfusionBlock.speed = CmissileSpeed();
	ConfusionBlock.hit_points = CmissileHits;
	ConfusionBlock.damage = CmissileDamage;
	ConfusionBlock.life = CmissileLife;
	ConfusionBlock.preprocess_func = confuse_preprocess;
	ConfusionBlock.blast_offs = CmissileOffset();
	ConfusionArray[0] = initialize_missile(&ConfusionBlock);

	if (ConfusionArray[0])
	{
		Element* CMissilePtr {};

		LockElement(ConfusionArray[0], &CMissilePtr);
		CMissilePtr->collision_func = confusion_collision;
		SetElementStarShip(CMissilePtr, StarShipPtr);
		UnlockElement(ConfusionArray[0]);
	}
	return (1);
}

static uint16_t initialize_test_pump_up(Element* ShipPtr, ElementHandle PumpUpArray[])
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;
	//Element *PumpUpPtr;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
	MissileBlock.pixoffs = MelnormeOffset();
	MissileBlock.speed = PumpupSpeedHd();
	MissileBlock.hit_points = PumpupDamage;
	MissileBlock.damage = PumpupDamage;
	MissileBlock.life = PumpupLife;
	MissileBlock.preprocess_func = 0;
	MissileBlock.blast_offs = 0;
	PumpUpArray[0] = initialize_missile(&MissileBlock);

	return (1);
}

static void melnorme_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								  uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_test_pump_up;
	const uint8_t old_count {StarShipPtr->weapon_counter};

	if (StarShipPtr->weapon_counter == WeaponWait)
	{
		StarShipPtr->weapon_counter = 0;
	}

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (lpEvalDesc->ObjectPtr)
	{
		if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SpecialEnergyCost
																   + WeaponEnergyCost
			&& !(testFlag(StarShipPtr->old_status_flags, StatusFlags::Weapon)))
		{
			lpEvalDesc->MoveState = MovementState::Entice;
		}
		else
		{
			Starship* EnemyStarShipPtr {};

			GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
			if (!(testFlag(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags, ShipFlags::ImmediateWeapon)))
			{
				lpEvalDesc->MoveState = MovementState::Pursue;
			}
		}
	}
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->weapon_counter == 0
		&& (old_count != 0
			|| ((StarShipPtr->special_counter
				 || StarShipPtr->RaceDescPtr->ship_info.energy_level >= SpecialEnergyCost
																			+ WeaponEnergyCost)
				&& !(testFlag(StarShipPtr->ship_input_state, StatusFlags::Weapon)))))
	{
		StarShipPtr->ship_input_state ^= StatusFlags::Weapon;
	}

	StarShipPtr->ship_input_state &= ~StatusFlags::Special;
	if (StarShipPtr->special_counter == 0
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SpecialEnergyCost)
	{
		const StatusFlags old_input_state {StarShipPtr->ship_input_state};

		StarShipPtr->RaceDescPtr->init_weapon_func = initialize_confusion;

		++ShipPtr->turn_wait;
		++ShipPtr->thrust_wait;
		ship_intelligence(ShipPtr, ObjectsOfConcern, *IntelObjectIndex::EnemyShip + 1);
		--ShipPtr->thrust_wait;
		--ShipPtr->turn_wait;

		if (testFlag(StarShipPtr->ship_input_state, StatusFlags::Weapon))
		{
			StarShipPtr->ship_input_state &= ~StatusFlags::Weapon;
			StarShipPtr->ship_input_state |= StatusFlags::Special;
		}

		StarShipPtr->ship_input_state = old_input_state | (StarShipPtr->ship_input_state & StatusFlags::Special);
	}

	StarShipPtr->weapon_counter = old_count;

	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_pump_up;
}

static void melnorme_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		ElementHandle Confusion {};

		initialize_confusion(ElementPtr, &Confusion);
		if (Confusion)
		{
			Element* CMissilePtr {};
			LockElement(Confusion, &CMissilePtr);

			ProcessSound(GetSound(
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						 CMissilePtr);

			UnlockElement(Confusion);
			PutElement(Confusion);
			StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
}

RaceDesc* init_melnorme(void)
{
	if (IsHd())
	{
		melnorme_desc.characteristics.max_thrust = ResScale(MaxThrust);
		melnorme_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		melnorme_desc.cyborg_control.WeaponRange = PumpupSpeedHd() * PumpupLife;
	}
	else
	{
		melnorme_desc.characteristics.max_thrust = MaxThrust;
		melnorme_desc.characteristics.thrust_increment = ThrustIncrement;
		melnorme_desc.cyborg_control.WeaponRange =
			PumpupSpeed * PumpupLife;
	}

	melnorme_desc.postprocess_func = melnorme_postprocess;
	melnorme_desc.init_weapon_func = initialize_pump_up;
	melnorme_desc.cyborg_control.intelligence_func = melnorme_intelligence;

	RaceDesc* RaceDescPtr {&melnorme_desc};

	return (RaceDescPtr);
}

} // namespace uqm
