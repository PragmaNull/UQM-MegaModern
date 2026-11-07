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

#include "lastbat.h"

#include "../ship.h"
#include "resinst.h"
#include "uqm/battle.h"
#include "uqm/colors.h"
#include "uqm/globdata.h"
// For BattleFrameRateTicks
#include "libs/mathlib.h"
#include "libs/time/timelib.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {1};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {6};
static constexpr auto MaxThrust {0};
static constexpr auto ThrustIncrement {0};
static constexpr auto TurnWait {0};
static constexpr auto ThrustWait {0};
static constexpr auto ShipMass {MaxShipMass * 10};
static constexpr auto TurretWait {0}; /* Controls animation of the Sa-Matra's central
                       * 'furnace', a new frame is displayed once every
                       * TurretWait frames. */

// Yellow comet
static constexpr auto WeaponWait {(c_gameSecond / BattleFrameRateTicks) * 10};
static constexpr auto CometDamage {2};
static constexpr auto CometOffset {0};
static auto CometHits() { return difficultyCase(10, 12, 15); }
static auto CometSpeed() { return ResScale(DisplayToWorld(ifEasyDifficulty(12, 10))); } // Kryzen: 17
static constexpr auto CometLife {2};
static constexpr auto CometTurnWait {3}; // Kruzen: 2 \  // compensate high speed with lesser maneuverability
static auto MaxComets() { return difficultyCase(2, 3, 4); }
static constexpr auto WeaponEnergyCost {2};
/* Used for samatra_desc.weapon_energy_cost, but the value isn't
		 * actually used. */

// Green sentinel
static constexpr auto SpecialWait {(c_gameSecond / BattleFrameRateTicks) * 3};
static auto SentinelSpeed() { return DisplayToWorld(ResScale(8)); }
static constexpr auto SentinelLife {2};
static constexpr auto SentinelOffset {0};
static auto SentinelHits() { return difficultyCase(9, 10, 12); }
static auto SentinelDamage() { return difficultyCase(1, 1, 2); }
static constexpr auto TrackWait {1};
static constexpr auto AnimationWait {1};
static auto RecoilVelocity() { return WorldToVelocity(ResScale(DisplayToWorld(ifEasyDifficulty(10, 8)))); }
static auto MaxRecoilVelocity() { return (RecoilVelocity() * 4); }
static auto MaxSentinels() { return difficultyCase(4, 4, 5); }
static constexpr auto SpecialEnergyCost {3};
/* Used for samatra_desc.special_energy_cost, but the value isn't
		 * actually used. */
static constexpr auto EnergyDrain {8}; // for HARD mode

// Blue force field
static constexpr auto GateDamage {1};
static constexpr auto GateHits {100};

// Red generators
static auto GeneratorHits() { return difficultyCase(12, 15, 18); }
static constexpr auto MaxGenerators {8};

static RaceDesc samatra_desc =
	{
		{/* ShipInfoDesc */
		 "samatra",
		 /* FiresFore | */ ImmediateWeapon | CrewImmune,
		 16, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergySize, MaxEnergySize,
		 NullResource,
		 NullResource,
		 NullResource,
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
			 SamatraBigMaskAnim,
			 SamatraMedMaskPmapAnim,
			 SamatraSmlMaskPmapAnim,
		 },
		 {
			 SentinelBigMaskAnim,
			 SentinelMedMaskPmapAnim,
			 SentinelSmlMaskPmapAnim,
		 },
		 {
			 GeneratorBigMaskAnim,
			 GeneratorMedMaskPmapAnim,
			 GeneratorSmlMaskPmapAnim,
		 },
		 {SamatraCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 NullResource,
		 SamatraShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			0,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static ElementHandle spawn_comet(Element* ElementPtr);

static void comet_preprocess(Element* ElementPtr)
{
	uint16_t frame_index {};

	frame_index = GetFrameIndex(ElementPtr->current.image.frame) + 1;
	if (frame_index < 29)
	{
		if (frame_index == 25)
		{
			int16_t cur_delta_x {};
			int16_t cur_delta_y {};
			Starship* StarShipPtr {};

			GetElementStarShip(ElementPtr, &StarShipPtr);
			++StarShipPtr->RaceDescPtr->characteristics.weapon_wait;
			spawn_comet(ElementPtr);
			ElementPtr->state_flags |= Nonsolid;

			GetCurrentVelocityComponents(&ElementPtr->velocity,
										 &cur_delta_x, &cur_delta_y);
			SetVelocityComponents(&ElementPtr->velocity,
								  cur_delta_x / 2, cur_delta_y / 2);
		}
		++ElementPtr->life_span;
	}

	ElementPtr->next.image.frame =
		SetAbsFrameIndex(
			ElementPtr->current.image.frame, frame_index);
	ElementPtr->state_flags |= Changing;
}

static void comet_collision(Element* ElementPtr0, GfxPoint* pPt0,
							Element* ElementPtr1, GfxPoint* pPt1)
{
	if (ElementPtr1->playerNr == RpgPlayerNum)
	{
		uint8_t old_hits {};

		if (ElementPtr1->state_flags & PlayerShip)
		{
			ElementPtr0->mass_points = CometDamage;
		}
		else
		{
			ElementPtr0->mass_points = 50;
		}

		old_hits = ElementPtr0->hit_points;
		const uint16_t old_life {ElementPtr0->life_span};
		ElementHandle hBlastElement {weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1)};
		if (ElementPtr1->state_flags & PlayerShip)
		{
			ElementPtr0->hit_points = old_hits;
			ElementPtr0->life_span = old_life;
			ElementPtr0->state_flags &= ~(Disappearing | Nonsolid | Collision);

			if (hBlastElement)
			{
				RemoveElement(hBlastElement);
				FreeElement(hBlastElement);
			}
		}

		if (ElementPtr0->state_flags & Disappearing)
		{
			Starship* StarShipPtr {};

			GetElementStarShip(ElementPtr0, &StarShipPtr);
			--StarShipPtr->RaceDescPtr->characteristics.weapon_wait;
		}
	}
}

static ElementHandle spawn_comet(Element* ElementPtr)
{
	MissileDesc MissileBlock;
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = 0;
	MissileBlock.index = 24;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = 0;
	MissileBlock.speed = 0;
	MissileBlock.hit_points = CometHits();
	MissileBlock.damage = CometDamage;
	MissileBlock.life = CometLife;
	MissileBlock.preprocess_func = comet_preprocess;
	MissileBlock.blast_offs = CometOffset;
	ElementHandle hComet {initialize_missile(&MissileBlock)};

	if (hComet)
	{
		Element* CometPtr {};

		PutElement(hComet);

		LockElement(hComet, &CometPtr);
		CometPtr->collision_func = comet_collision;
		SetElementStarShip(CometPtr, StarShipPtr);
		{
			uint16_t facing {};

			CometPtr->turn_wait = ElementPtr->turn_wait;
			CometPtr->hTarget = ElementPtr->hTarget;
			if (ElementPtr->state_flags & PlayerShip)
			{
				CometPtr->turn_wait = 0;
				facing = (uint16_t)uqm::random().next();
				SetVelocityVector(&CometPtr->velocity,
								  CometSpeed(), facing);
			}
			else
			{
				CometPtr->velocity = ElementPtr->velocity;
				CometPtr->hit_points = ElementPtr->hit_points;
				facing = AngleToFacing(
					GetVelocityTravelAngle(&CometPtr->velocity));
			}

			if (CometPtr->turn_wait)
			{
				--CometPtr->turn_wait;
			}
			else
			{
				facing = NormalizeFacing(facing);
				if (TrackShip(CometPtr, &facing) > 0)
				{
					SetVelocityVector(&CometPtr->velocity,
									  CometSpeed(), facing);
				}
				CometPtr->turn_wait = CometTurnWait;
			}
		}
		UnlockElement(hComet);
	}

	return (hComet);
}

static void turret_preprocess(Element* ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame,
							 (GetFrameIndex(ElementPtr->current.image.frame) % 10) + 1);
		ElementPtr->state_flags |= Changing;

		ElementPtr->turn_wait = TurretWait;
	}
}

static void gate_collision(Element* ElementPtr0, GfxPoint* pPt0,
						   Element* ElementPtr1, GfxPoint* pPt1)
{
	if (ElementPtr1->playerNr == RpgPlayerNum)
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr0, &StarShipPtr);
		if (StarShipPtr->RaceDescPtr->characteristics.max_thrust == 0)
		{
			if (!(ElementPtr1->state_flags & FiniteLife))
			{
				ElementPtr0->state_flags |= Collision;
			}

			if ((ElementPtr1->state_flags & PlayerShip)
				&& GetPrimType(
					   &DisplayArray[ElementPtr0->PrimIndex])
					   == PrimitiveType::StampFill
				&& uqm::gameState().flag(uqm::GameFlag::BOMB_CARRIER))
			{
				uqm::currentActivity() &= ~ActivityFlag::InBattle;
			}
		}
		else
		{
			if (ElementPtr1->state_flags & PlayerShip)
			{
				ElementPtr0->mass_points = GateDamage;
			}
			else
			{
				ElementPtr0->mass_points = 50;
			}

			ElementPtr0->hit_points = GateHits;
			ElementHandle hBlastElement {weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1)};
			ElementPtr0->state_flags &= ~(Disappearing | Nonsolid | Collision);
			ElementPtr0->life_span = NormalLife;

			if (hBlastElement)
			{
				RemoveElement(hBlastElement);
				FreeElement(hBlastElement);
			}
		}
	}
}

static void gate_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->characteristics.max_thrust == 0)
	{
		ElementPtr->mass_points = ShipMass;
		ElementPtr->state_flags &= ~FiniteLife;
		ElementPtr->life_span = NormalLife + 1;
		ElementPtr->preprocess_func = 0;
		SetPrimColor(
			&DisplayArray[ElementPtr->PrimIndex],
			BlackColor);
		SetPrimType(
			&DisplayArray[ElementPtr->PrimIndex],
			PrimitiveType::StampFill);
	}
	else
	{
		++ElementPtr->life_span;
		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->current.image.frame);
		if (GetFrameIndex(ElementPtr->next.image.frame) == 0)
		{
			ElementPtr->next.image.frame =
				SetAbsFrameIndex(
					ElementPtr->next.image.frame, 11);
		}

		ElementPtr->state_flags |= Changing;
	}
}

static void generator_death(Element* ElementPtr)
{
	if (!(ElementPtr->state_flags & FiniteLife))
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		--StarShipPtr->RaceDescPtr->characteristics.max_thrust;
		ElementPtr->state_flags |= FiniteLife | Nonsolid;
		ElementPtr->preprocess_func = 0;
		ElementPtr->turn_wait = 12;
		ElementPtr->thrust_wait = 0;

		ElementPtr->current.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame, 10 - 1);
	}

	if (ElementPtr->thrust_wait)
	{
		--ElementPtr->thrust_wait;
		ElementPtr->state_flags &= ~Disappearing;
		ElementPtr->state_flags |= Changing;
		++ElementPtr->life_span;
	}
	else if (ElementPtr->turn_wait--)
	{
		ElementPtr->state_flags &= ~Disappearing;
		ElementPtr->state_flags |= Changing;
		++ElementPtr->life_span;

		ElementPtr->next.image.frame = IncFrameIndex(
			ElementPtr->current.image.frame);

		ElementPtr->thrust_wait = 1;
	}
}

static void generator_preprocess(Element* ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else if ((ElementPtr->turn_wait =
				  (uint8_t)((GeneratorHits()
							 - ElementPtr->hit_points)
							/ 5))
			 < 3)
	{
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame,
							 (GetFrameIndex(ElementPtr->current.image.frame) + 1) % 10);
		ElementPtr->state_flags |= Changing;
	}
}

static void generator_collision(Element* ElementPtr0, GfxPoint* pPt0,
								Element* ElementPtr1, GfxPoint* pPt1)
{
	if (!(ElementPtr1->state_flags & FiniteLife))
	{
		ElementPtr0->state_flags |= Collision;
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void sentinel_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	++StarShipPtr->RaceDescPtr->characteristics.special_wait;
	++ElementPtr->life_span;

	if (ElementPtr->thrust_wait)
	{
		--ElementPtr->thrust_wait;
	}
	else
	{
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->current.image.frame,
							 (GetFrameIndex(ElementPtr->current.image.frame) + 1) % 6);
		ElementPtr->state_flags |= Changing;

		ElementPtr->thrust_wait = AnimationWait;
	}

	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		uint16_t facing {};
		ElementHandle hTarget {};

		if (!(ElementPtr->state_flags & Nonsolid))
		{
			facing = AngleToFacing(
				GetVelocityTravelAngle(&ElementPtr->velocity));
		}
		else
		{
			ElementPtr->state_flags &= ~Nonsolid;
			facing = (uint16_t)uqm::random().next();
			SetVelocityVector(&ElementPtr->velocity,
							  SentinelSpeed(), facing);
		}
		facing = NormalizeFacing(facing);
		if (ElementPtr->hTarget == 0)
		{
			uint16_t f {facing};
			TrackShip(ElementPtr, &f);
		}

		if (ElementPtr->hTarget == 0)
		{
			hTarget = StarShipPtr->hShip;
		}
		else if (StarShipPtr->hShip == 0)
		{
			hTarget = ElementPtr->hTarget;
		}
		else
		{
			int32_t delta_x0 {};
			int32_t delta_y0 {};
			int32_t delta_x1 {};
			int32_t delta_y1 {};
			Element* ShipPtr {};
			Element* EnemyShipPtr {};

			LockElement(ElementPtr->hTarget, &EnemyShipPtr);

			LockElement(StarShipPtr->hShip, &ShipPtr);
			delta_x0 = (int32_t)ShipPtr->current.location.x
					 - (int32_t)ElementPtr->current.location.x;
			delta_y0 = (int32_t)ShipPtr->current.location.y
					 - (int32_t)ElementPtr->current.location.y;

			delta_x1 = (int32_t)ShipPtr->current.location.x
					 - (int32_t)EnemyShipPtr->current.location.x;
			delta_y1 = (int32_t)ShipPtr->current.location.y
					 - (int32_t)EnemyShipPtr->current.location.y;
			UnlockElement(StarShipPtr->hShip);

			if ((long)delta_x0 * delta_x0
					+ (long)delta_y0 * delta_y0
				> (long)delta_x1 * delta_x1
					  + (long)delta_y1 * delta_y1)
			{
				hTarget = StarShipPtr->hShip;
			}
			else
			{
				hTarget = ElementPtr->hTarget;
			}

			UnlockElement(ElementPtr->hTarget);
		}

		if (hTarget)
		{
			uint16_t num_frames {};
			int32_t delta_x {};
			int32_t delta_y {};
			Element* TargetPtr {};
			VelocityDesc TargetVelocity;

			LockElement(hTarget, &TargetPtr);

			delta_x = (int32_t)TargetPtr->current.location.x
					- (int32_t)ElementPtr->current.location.x;
			delta_x = WrapDeltaX(delta_x);
			delta_y = (int32_t)TargetPtr->current.location.y
					- (int32_t)ElementPtr->current.location.y;
			delta_y = WrapDeltaY(delta_y);

			if ((num_frames = ResDescale(WorldToTurn(
					 square_root((long)delta_x * delta_x
								 + (long)delta_y * delta_y))))
				== 0)
			{
				num_frames = 1;
			}

			TargetVelocity = TargetPtr->velocity;
			GetNextVelocityComponentsSdword(&TargetVelocity,
											&delta_x, &delta_y, num_frames);

			delta_x = ((int32_t)TargetPtr->current.location.x + (int32_t)delta_x)
					- (int32_t)ElementPtr->current.location.x;
			delta_x = WrapDeltaX(delta_x);
			delta_y = ((int32_t)TargetPtr->current.location.y + (int32_t)delta_y)
					- (int32_t)ElementPtr->current.location.y;
			delta_y = WrapDeltaY(delta_y);

			UnlockElement(hTarget);

			delta_x = NormalizeFacing(
				AngleToFacing(ARCTAN(delta_x, delta_y)) - facing);

			if (delta_x > 0)
			{
				if (delta_x <= AngleToFacing(HalfCircle))
				{
					++facing;
				}
				else
				{
					--facing;
				}
			}

			SetVelocityVector(&ElementPtr->velocity,
							  SentinelSpeed(), facing);
		}

		ElementPtr->turn_wait = TrackWait;
	}
}

static void sentinel_collision(Element* ElementPtr0, GfxPoint* pPt0,
							   Element* ElementPtr1, GfxPoint* pPt1)
{
	uint16_t angle {};
	Starship* StarShipPtr {};

	if (ElementPtr1->playerNr == NpcPlayerNum)
	{
		if (ElementPtr0->preprocess_func == ElementPtr1->preprocess_func
			&& !(ElementPtr0->state_flags & DefyPhysics)
			&& (pPt0->x != ElementPtr0->IntersectControl.IntersectStamp.origin.x
				|| pPt0->y != ElementPtr0->IntersectControl.IntersectStamp.origin.y))
		{
			angle = ARCTAN(pPt0->x - pPt1->x, pPt0->y - pPt1->y);

			SetVelocityComponents(&ElementPtr0->velocity,
								  Cosine(angle, WorldToVelocity(SentinelSpeed())),
								  Sine(angle, WorldToVelocity(SentinelSpeed())));
			ElementPtr0->turn_wait = TrackWait;
			ElementPtr0->state_flags |= Collision | DefyPhysics;
		}
	}
	else
	{
		uint8_t old_hits {};

		old_hits = ElementPtr0->hit_points;
		const uint16_t old_life {ElementPtr0->life_span};
		ElementPtr0->blast_offset = 0;
		ElementHandle hBlastElement {weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1)};
		ElementPtr0->thrust_wait = 0;

		if ((ElementPtr1->state_flags & PlayerShip)
			&& ElementPtr1->crew_level
			&& !GravityMass(ElementPtr1->mass_points + 1))
		{
			int32_t cur_delta_x {};
			int32_t cur_delta_y {};

			ElementPtr0->life_span = old_life;
			ElementPtr0->hit_points = old_hits;
			ElementPtr0->state_flags &= ~Disappearing;
			ElementPtr0->state_flags |= DefyPhysics;
			ElementPtr0->turn_wait = (c_gameSecond / BattleFrameRateTicks) >> 1;

			GetElementStarShip(ElementPtr1, &StarShipPtr);
			StarShipPtr->cur_status_flags &=
				~(ShipAtMaxSpeed | ShipBeyondMaxSpeed);
			if (ElementPtr1->turn_wait < CollisionTurnWait)
			{
				ElementPtr1->turn_wait += CollisionTurnWait;
			}
			if (ElementPtr1->thrust_wait < CollisionThrustWait)
			{
				ElementPtr1->thrust_wait += CollisionThrustWait;
			}

			angle = GetVelocityTravelAngle(&ElementPtr0->velocity);
			const auto recoilVelocity {RecoilVelocity()};
			DeltaVelocityComponents(&ElementPtr1->velocity,
									Cosine(angle, recoilVelocity),
									Sine(angle, recoilVelocity));
			GetCurrentVelocityComponentsSdword(&ElementPtr1->velocity,
											   &cur_delta_x, &cur_delta_y);
			if ((long)cur_delta_x * (long)cur_delta_x
					+ (long)cur_delta_y * (long)cur_delta_y
				> (long)MaxRecoilVelocity() * (long)MaxRecoilVelocity())
			{
				angle = ARCTAN(cur_delta_x, cur_delta_y);
				SetVelocityComponents(&ElementPtr1->velocity,
									  Cosine(angle, MaxRecoilVelocity()),
									  Sine(angle, MaxRecoilVelocity()));
			}
			if (isDifficulty(uqm::Difficulty::Hard))
			{
				if (StarShipPtr->RaceDescPtr->ship_info.energy_level < EnergyDrain)
				{
					DeltaEnergy(ElementPtr1, -StarShipPtr->RaceDescPtr->ship_info.energy_level);
				}
				else
				{
					DeltaEnergy(ElementPtr1, -EnergyDrain);
				}
				if (IsExtended())
				{
					GetElementStarShip(ElementPtr0, &StarShipPtr);
					ProcessSound(GetSound(
									 /* ENERGY DRAIN */
									 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
								 ElementPtr0);
				}
			}

			ZeroVelocityComponents(&ElementPtr0->velocity);
		}

		if (ElementPtr0->state_flags & Disappearing)
		{
			GetElementStarShip(ElementPtr0, &StarShipPtr);
			--StarShipPtr->RaceDescPtr->characteristics.special_wait;
			if (hBlastElement)
			{
				Element* BlastElementPtr {};

				LockElement(hBlastElement, &BlastElementPtr);
				BlastElementPtr->life_span = 7;
				BlastElementPtr->current.image.frame =
					SetAbsFrameIndex(
						BlastElementPtr->current.image.farray[0], 6);
				UnlockElement(hBlastElement);
			}
		}
	}
}

static void samatra_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								 uint16_t ConcernCounter)
{
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
}

static void samatra_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->characteristics.max_thrust)
	{
		if (StarShipPtr->weapon_counter == 0
			&& StarShipPtr->RaceDescPtr->characteristics.weapon_wait < MaxComets()
			&& spawn_comet(ElementPtr))
		{
			StarShipPtr->weapon_counter = WeaponWait;
			if (IsExtended())
			{
				ProcessSound(GetSound(
								 /* COMET SPAWN */
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 0),
							 ElementPtr);
			}
		}

		if (StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->characteristics.special_wait < MaxSentinels())
		{
			MissileDesc MissileBlock;

			MissileBlock.cx = ElementPtr->next.location.x;
			MissileBlock.cy = ElementPtr->next.location.y;
			MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
			MissileBlock.face = 0;
			MissileBlock.index = 0;
			MissileBlock.sender = ElementPtr->playerNr;
			MissileBlock.flags = 0;
			MissileBlock.pixoffs = 0;
			MissileBlock.speed = SentinelSpeed();
			MissileBlock.hit_points = SentinelHits();
			MissileBlock.damage = SentinelDamage();
			MissileBlock.life = SentinelLife;
			MissileBlock.preprocess_func = sentinel_preprocess;
			MissileBlock.blast_offs = SentinelOffset;
			ElementHandle hSentinel {initialize_missile(&MissileBlock)};

			if (hSentinel)
			{
				Element* SentinelPtr {};

				LockElement(hSentinel, &SentinelPtr);
				SentinelPtr->collision_func = sentinel_collision;
				SentinelPtr->turn_wait = TrackWait + 2;
				SetElementStarShip(SentinelPtr, StarShipPtr);
				UnlockElement(hSentinel);

				StarShipPtr->special_counter = SpecialWait;

				PutElement(hSentinel);
			}
		}
	}
}

static void samatra_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	StarShipPtr->RaceDescPtr->characteristics.weapon_wait = 0;
	StarShipPtr->RaceDescPtr->characteristics.special_wait = 0;
	if (!(ElementPtr->state_flags & Appearing))
	{
		++ElementPtr->turn_wait;
		++ElementPtr->thrust_wait;
	}
	else
	{
		GfxPoint offs_orig[8] =
			{
				{-127 - 9, -53 + 18},
				{-38 - 9,  -88 + 18},
				{44 - 9,	 -85 + 18},
				{127 - 9,  -60 + 18},
				{124 - 9,  28 + 18 },
				{73 - 9,	 61 + 18 },
				{-87 - 9,  58 + 18 },
				{-136 - 9, 29 + 18 },
		};

		GfxPoint offs_hd[] =
			{
				{-544 + 44, -140 - 16},
				{-188 + 38, -280 - 20},
				{140 + 34,  -268 - 16},
				{472 + 36,  -168 - 20},
				{460 + 32,  184 - 20 },
				{256 + 44,  316 - 24 },
				{-384 + 44, 304 - 12 },
				{-580 + 32, 188 - 24 },
		};

		const GfxPoint* offs {chooseIfHd<GfxPoint*>(offs_orig, offs_hd)};

		for (StarShipPtr->RaceDescPtr->characteristics.max_thrust = 0;
			 StarShipPtr->RaceDescPtr->characteristics.max_thrust < MaxGenerators;
			 ++StarShipPtr->RaceDescPtr->characteristics.max_thrust)
		{
			ElementHandle hGenerator {AllocElement()};
			if (hGenerator)
			{
				Element* GeneratorPtr {};

				LockElement(hGenerator, &GeneratorPtr);
				GeneratorPtr->hit_points = GeneratorHits();
				GeneratorPtr->mass_points = MaxShipMass * 10;
				GeneratorPtr->life_span = NormalLife;
				GeneratorPtr->playerNr = ElementPtr->playerNr;
				GeneratorPtr->state_flags = Appearing | IgnoreSimilar;
				SetPrimType(
					&DisplayArray[GeneratorPtr->PrimIndex],
					PrimitiveType::Stamp);
				GeneratorPtr->current.location.x =
					((LogSpaceWidth() >> 1)
					 + DisplayToWorld((offs[StarShipPtr->RaceDescPtr->characteristics.max_thrust].x)))
					& ~((ScaledOne << MaxVisReduction) - 1);
				GeneratorPtr->current.location.y =
					((LogSpaceHeight() >> 1)
					 + DisplayToWorld((offs[StarShipPtr->RaceDescPtr->characteristics.max_thrust].y)))
					& ~((ScaledOne << MaxVisReduction) - 1);
				GeneratorPtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
				GeneratorPtr->current.image.frame =
					SetAbsFrameIndex(
						StarShipPtr->RaceDescPtr->ship_data.special[0],
						(uint8_t)uqm::random().next() % 10);

				GeneratorPtr->preprocess_func = generator_preprocess;
				GeneratorPtr->collision_func = generator_collision;
				GeneratorPtr->death_func = generator_death;

				SetElementStarShip(GeneratorPtr, StarShipPtr);
				UnlockElement(hGenerator);

				InsertElement(hGenerator, GetHeadElement());
			}
		}

		{
			ElementHandle hTurret {AllocElement()};
			if (hTurret)
			{
				Element* TurretPtr {};

				LockElement(hTurret, &TurretPtr);
				TurretPtr->hit_points = 1;
				TurretPtr->life_span = NormalLife;
				TurretPtr->playerNr = ElementPtr->playerNr;
				TurretPtr->state_flags = Appearing | IgnoreSimilar | Nonsolid;
				SetPrimType(
					&DisplayArray[TurretPtr->PrimIndex],
					PrimitiveType::Stamp);
				TurretPtr->current.location.x = LogSpaceWidth() >> 1;
				TurretPtr->current.location.y = LogSpaceHeight() >> 1;
				TurretPtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.ship;
				TurretPtr->current.image.frame =
					SetAbsFrameIndex(
						StarShipPtr->RaceDescPtr->ship_data.ship[0], 1);

				TurretPtr->preprocess_func = turret_preprocess;

				SetElementStarShip(TurretPtr, StarShipPtr);
				UnlockElement(hTurret);

				InsertElement(hTurret, GetSuccElement(ElementPtr));
			}
		}

		{
			ElementHandle hGate {AllocElement()};
			if (hGate)
			{
				Element* GatePtr {};

				LockElement(hGate, &GatePtr);
				GatePtr->hit_points = GateHits;
				GatePtr->mass_points = GateDamage;
				GatePtr->life_span = 2;
				GatePtr->playerNr = ElementPtr->playerNr;
				GatePtr->state_flags = Appearing | FiniteLife
									 | IgnoreSimilar;
				SetPrimType(
					&DisplayArray[GatePtr->PrimIndex],
					PrimitiveType::Stamp);
				GatePtr->current.location.x = LogSpaceWidth() >> 1;
				GatePtr->current.location.y = LogSpaceHeight() >> 1;
				GatePtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.ship;
				GatePtr->current.image.frame =
					SetAbsFrameIndex(
						StarShipPtr->RaceDescPtr->ship_data.ship[0], 11);

				GatePtr->preprocess_func = gate_preprocess;
				GatePtr->collision_func = gate_collision;

				SetElementStarShip(GatePtr, StarShipPtr);
				UnlockElement(hGate);

				InsertElement(hGate, GetSuccElement(ElementPtr));
			}
		}

		StarShipPtr->weapon_counter = WeaponWait >> 1;
		StarShipPtr->special_counter = SpecialWait >> 1;
	}
}

RaceDesc* init_samatra(void)
{
	samatra_desc.preprocess_func = samatra_preprocess;
	samatra_desc.postprocess_func = samatra_postprocess;
	samatra_desc.cyborg_control.intelligence_func = samatra_intelligence;

	RaceDesc* RaceDescPtr {&samatra_desc};

	return (RaceDescPtr);
}

} // namespace uqm
