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

#include "chenjesu.h"

#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/globdata.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {36};
static constexpr auto MaxEnergy {30};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {27};
static constexpr auto ThrustIncrement {3};
static constexpr auto ThrustWait {4};
static constexpr auto TurnWait {6};
static constexpr auto ShipMass {10};

// Photon Shard
static constexpr auto WeaponEnergyCost {5};
static constexpr auto WeaponWait {0};
static auto ChenjesuOffset() { return ResScale(16); }
static constexpr auto MissileOffset {0};
static auto MissileSpeed() { return DisplayToWorld(ResScale(16)); }
static constexpr auto MissileLife {90};
/* actually, it's as long as you hold the button down. */
static constexpr auto MissileHits {10};
static constexpr auto MissileDamage {6};
static constexpr auto NumSparkles {8};

// Shrapnel
static auto FragmentOffset() { return ResScale(2); }
static constexpr auto NumFragments {8};
static constexpr auto FragmentLife {10};
static auto FragmentSpeed() { return MissileSpeed(); }
static auto FragmentRange() { return (FragmentLife * FragmentSpeed()); }
/* This bit is for the cyborg only. */
static constexpr auto FragmentHits {1};
static constexpr auto FragmentDamage {2};

// DOGI
static constexpr auto SpecialEnergyCost {MaxEnergy};
static constexpr auto SpecialWait {0};
static auto DoggyOffset() { return ResScale(18); }
static auto DoggySpeed() { return DisplayToWorld(ResScale(8)); }
static constexpr auto EnergyDrain {10};
static constexpr auto MaxDoggies {4};
static constexpr auto DoggyHits {3};
static constexpr auto DoggyMass {4};

static RaceDesc chenjesu_desc =
	{
		{/* ShipInfoDesc */
		 "broodhome",
		 FiresFore | SeekingSpecial | SeekingWeapon,
		 28, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 ChenjesuRaceStrings,
		 ChenjesuIconMaskPmapAnim,
		 ChenjesuMiconMaskPmapAnim,
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
			 ChenjesuBigMaskPmapAnim,
			 ChenjesuMedMaskPmapAnim,
			 ChenjesuSmlMaskPmapAnim,
		 },
		 {
			 SparkBigMaskPmapAnim,
			 SparkMedMaskPmapAnim,
			 SparkSmlMaskPmapAnim,
		 },
		 {
			 DoggyBigMaskPmapAnim,
			 DoggyMedMaskPmapAnim,
			 DoggySmlMaskPmapAnim,
		 },
		 {ChenjesuCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 ChenjesuVictorySong,
		 ChenjesuShipSounds,
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

static void crystal_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ElementPtr, &StarShipPtr);
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.index = 1;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = 0;
	MissileBlock.speed = FragmentSpeed();
	MissileBlock.hit_points = FragmentHits;
	MissileBlock.damage = FragmentDamage;
	MissileBlock.life = FragmentLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = FragmentOffset();

	for (MissileBlock.face = 0;
		 MissileBlock.face < AngleToFacing(FullCircle);
		 MissileBlock.face +=
		 (AngleToFacing(FullCircle) / NumFragments))
	{
		ElementHandle hFragment {initialize_missile(&MissileBlock)};
		if (hFragment)
		{
			Element* FragPtr {};

			LockElement(hFragment, &FragPtr);
			SetElementStarShip(FragPtr, StarShipPtr);
			UnlockElement(hFragment);
			PutElement(hFragment);
		}
	}

	ProcessSound(GetSound(
					 /* CRYSTAL_FRAGMENTS */
					 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
				 ElementPtr);
}

static void crystal_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->cur_status_flags & WeaponFlag)
	{
		++ElementPtr->life_span; /* keep it going while key pressed */
	}
	else
	{
		ElementPtr->life_span = 1;

		ElementPtr->postprocess_func = crystal_postprocess;
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
		ElementPtr->state_flags |= Changing;

		ElementPtr->turn_wait = ElementPtr->next_turn;
	}
}

static void crystal_collision(Element* ElementPtr0, GfxPoint* pPt0,
							  Element* ElementPtr1, GfxPoint* pPt1)
{
	ElementHandle hBlastElement {};

	hBlastElement =
		weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	if (hBlastElement)
	{
		Element* BlastElementPtr {};

		LockElement(hBlastElement, &BlastElementPtr);
		BlastElementPtr->current.location = ElementPtr1->current.location;

		BlastElementPtr->life_span = NumSparkles;
		BlastElementPtr->turn_wait = BlastElementPtr->next_turn = 0;
		{
			BlastElementPtr->preprocess_func = animate;
		}

		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame =
			SetAbsFrameIndex(BlastElementPtr->current.image.farray[0],
							 2); /* skip stones */

		UnlockElement(hBlastElement);
	}
}

static void doggy_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	++StarShipPtr->special_counter;
	if (ElementPtr->thrust_wait > 0) /* could be non-zero after a collision */
	{
		--ElementPtr->thrust_wait;
	}
	else
	{
		uint16_t facing {};
		uint16_t orig_facing {};
		int16_t delta_facing {};

		facing = orig_facing =
			NormalizeFacing(AngleToFacing(
				GetVelocityTravelAngle(&ElementPtr->velocity)));
		if ((delta_facing = TrackShip(ElementPtr, &facing)) < 0)
		{
			facing = NormalizeFacing(uqm::random().next());
		}
		else
		{
			Element* ShipPtr {};

			LockElement(ElementPtr->hTarget, &ShipPtr);
			facing = NormalizeFacing(AngleToFacing(
				ARCTAN(ShipPtr->current.location.x - ElementPtr->current.location.x,
					   ShipPtr->current.location.y - ElementPtr->current.location.y)));
			delta_facing = NormalizeFacing(facing - GetFrameIndex(ShipPtr->current.image.frame));
			UnlockElement(ElementPtr->hTarget);

			if (delta_facing > AngleToFacing(HalfCircle - Octant) && delta_facing < AngleToFacing(HalfCircle + Octant))
			{
				if (delta_facing >= AngleToFacing(HalfCircle))
				{
					facing -= AngleToFacing(Quadrant);
				}
				else
				{
					facing += AngleToFacing(Quadrant);
				}
			}

			facing = NormalizeFacing(facing);
		}

		if (facing != orig_facing)
		{
			SetVelocityVector(&ElementPtr->velocity,
							  DoggySpeed(), facing);
		}
	}
}

static void doggy_death(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	ProcessSound(GetSound(
					 /* DOGGY_DIES */
					 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 3),
				 ElementPtr);

	ElementPtr->state_flags &= ~Disappearing;
	ElementPtr->state_flags |= Nonsolid | FiniteLife;
	ElementPtr->life_span = 6;
	{
		ElementPtr->preprocess_func = animate;
	}
	ElementPtr->death_func = nullptr;
	ElementPtr->collision_func = nullptr;
	ZeroVelocityComponents(&ElementPtr->velocity);

	ElementPtr->turn_wait = ElementPtr->next_turn = 0;
}

static void doggy_collision(Element* ElementPtr0, GfxPoint* pPt0,
							Element* ElementPtr1, GfxPoint* pPt1)
{
	collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	if ((ElementPtr1->state_flags & PlayerShip)
		&& !elementsOfSamePlayer(ElementPtr0, ElementPtr1))
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr0, &StarShipPtr);
		ProcessSound(GetSound(
						 /* DOGGY_STEALS_ENERGY */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
					 ElementPtr0);
		GetElementStarShip(ElementPtr1, &StarShipPtr);
		if (StarShipPtr->RaceDescPtr->ship_info.energy_level < EnergyDrain)
		{
			DeltaEnergy(ElementPtr1, -StarShipPtr->RaceDescPtr->ship_info.energy_level);
		}
		else
		{
			DeltaEnergy(ElementPtr1, -EnergyDrain);
		}
	}
	if (ElementPtr0->thrust_wait <= CollisionThrustWait)
	{
		ElementPtr0->thrust_wait += CollisionThrustWait << 1;
	}
}

static void spawn_doggy(Element* ElementPtr)
{
	ElementHandle hDoggyElement {};

	if ((hDoggyElement = AllocElement()) != 0)
	{
		uint16_t angle {};
		Element* DoggyElementPtr {};
		Starship* StarShipPtr {};

		ElementPtr->state_flags |= DefyPhysics;

		PutElement(hDoggyElement);
		LockElement(hDoggyElement, &DoggyElementPtr);
		DoggyElementPtr->hit_points = DoggyHits;
		DoggyElementPtr->mass_points = DoggyMass;
		DoggyElementPtr->thrust_wait = 0;
		DoggyElementPtr->playerNr = ElementPtr->playerNr;
		DoggyElementPtr->state_flags = Appearing;
		DoggyElementPtr->life_span = NormalLife;
		SetPrimType(&DisplayArray[DoggyElementPtr->PrimIndex],
					PrimitiveType::Stamp);
		{
			DoggyElementPtr->preprocess_func = doggy_preprocess;
			DoggyElementPtr->postprocess_func = nullptr;
			DoggyElementPtr->collision_func = doggy_collision;
			DoggyElementPtr->death_func = doggy_death;
		}

		GetElementStarShip(ElementPtr, &StarShipPtr);
		angle = FacingToAngle(StarShipPtr->ShipFacing) + HalfCircle;
		DoggyElementPtr->current.location.x = ElementPtr->next.location.x
											+ Cosine(angle, DisplayToWorld(ChenjesuOffset() + DoggyOffset()));
		DoggyElementPtr->current.location.y = ElementPtr->next.location.y
											+ Sine(angle, DisplayToWorld(ChenjesuOffset() + DoggyOffset()));
		DoggyElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
		DoggyElementPtr->current.image.frame = StarShipPtr->RaceDescPtr->ship_data.special[0];

		SetVelocityVector(&DoggyElementPtr->velocity,
						  DoggySpeed(), NormalizeFacing(AngleToFacing(angle)));

		SetElementStarShip(DoggyElementPtr, StarShipPtr);

		ProcessSound(GetSound(
						 /* RELEASE_DOGGY */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 4),
					 DoggyElementPtr);

		UnlockElement(hDoggyElement);
	}
}

static void chenjesu_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								  uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	static uint32_t old_dist[NumSides] = {(uint32_t)~0, (uint32_t)~0};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state &= ~SpecialFlag;

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (lpEvalDesc->ObjectPtr)
	{
		Starship* EnemyStarShipPtr {};

		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if ((lpEvalDesc->which_turn <= 16
			 && Maneuverability(
					&EnemyStarShipPtr->RaceDescPtr->cyborg_control)
					>= ResolutionCompensated(MediumShip))
			|| (Maneuverability(
					&EnemyStarShipPtr->RaceDescPtr->cyborg_control)
					<= ResolutionCompensated(SlowShip)
				&& GetWeaponRange(
					   &EnemyStarShipPtr->RaceDescPtr->cyborg_control)
					   >= chooseIfHd(LongRangeWeapon, LongRangeWeaponHd) * 3 / 4
				&& (EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & SeekingWeapon)))
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
	}

	if (StarShipPtr->special_counter == 1
		&& ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr
		&& ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].MoveState == MovementState::Entice
		&& ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].which_turn <= 8)
	{
		lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon];
	}

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (lpEvalDesc->ObjectPtr)
	{
		ElementHandle h {};
		ElementHandle hNext {};
		Element* CrystalPtr {};

		h = (StarShipPtr->old_status_flags & WeaponFlag) ?
				GetTailElement() :
				(ElementHandle)0;
		for (; h; h = hNext)
		{
			LockElement(h, &CrystalPtr);
			hNext = GetPredElement(CrystalPtr);
			if (!(CrystalPtr->state_flags & Nonsolid)
				&& CrystalPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.weapon
				&& CrystalPtr->preprocess_func
				&& CrystalPtr->life_span > 0
				&& elementsOfSamePlayer(CrystalPtr, ShipPtr))
			{
				if (ObjectsOfConcern[*IntelObjectIndex::EnemyShip].ObjectPtr)
				{
					uint16_t which_turn {};
					bool crystal_would_miss = false;

					if (!IsHd() || lpEvalDesc != &ObjectsOfConcern[*IntelObjectIndex::EnemyShip])
					{
						crystal_would_miss = ((which_turn = PlotIntercept(CrystalPtr,
																		  ObjectsOfConcern[*IntelObjectIndex::EnemyShip].ObjectPtr,
																		  CrystalPtr->life_span, FragmentRange() / 2))
												  == 0
											  || (which_turn == 1
												  && PlotIntercept(CrystalPtr,
																   ObjectsOfConcern[*IntelObjectIndex::EnemyShip].ObjectPtr,
																   CrystalPtr->life_span, 0)
														 == 0));
					}
					else
					{
						uint32_t curr_dist = 0;

						const int32_t dx {CrystalPtr->next.location.x - ObjectsOfConcern[*IntelObjectIndex::EnemyShip].ObjectPtr->next.location.x};
						const int32_t dy {CrystalPtr->next.location.y - ObjectsOfConcern[*IntelObjectIndex::EnemyShip].ObjectPtr->next.location.y};

						curr_dist = square_root((long)dx * dx + (long)dy * dy);

						/*which_turn = PlotIntercept (CrystalPtr,
							ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr,
							CrystalPtr->life_span, FragmentRange() / 3);*/
						crystal_would_miss = ((PlotIntercept(CrystalPtr,
															 ObjectsOfConcern[*IntelObjectIndex::EnemyShip].ObjectPtr,
															 CrystalPtr->life_span, FragmentRange() * 8)
											   == 0)
											  || curr_dist > old_dist[ShipPtr->playerNr]);
						old_dist[ShipPtr->playerNr] = curr_dist;
					}
					if (crystal_would_miss)
					{
						StarShipPtr->ship_input_state &= ~WeaponFlag;
						old_dist[ShipPtr->playerNr] = (uint32_t)~0;

						// JMS: Let's try to stop Chenjesu's stupid over-rapid firing behavior in hires modes...
						if (IsHd()
							&& ObjectsOfConcern[*IntelObjectIndex::EnemyShip].which_turn > 16
							&& ((ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr
								 && ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].which_turn > 8)
								|| !(ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr)))
						{
							StarShipPtr->weapon_counter = 10;
						}
					}
					else if (StarShipPtr->weapon_counter == 0)
					{
						StarShipPtr->ship_input_state |= WeaponFlag;
						lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyShip];
					}

					UnlockElement(h);
					break;
				}
				hNext = 0;
			}
			UnlockElement(h);
		}

		if (h == 0)
		{
			if (StarShipPtr->old_status_flags & WeaponFlag)
			{
				StarShipPtr->ship_input_state &= ~WeaponFlag;
				old_dist[ShipPtr->playerNr] = (uint32_t)~0;
				if (lpEvalDesc == &ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon])
				{
					StarShipPtr->weapon_counter = 3;
				}
			}
			else if (StarShipPtr->weapon_counter == 0
					 && ship_weapons(ShipPtr, lpEvalDesc->ObjectPtr, FragmentRange() / 2))
			{
				if (IsHd())
				{
					Element Ship;
					ElementHandle Weapon[6];
					Element* EPtr {};
					Starship* StarShipPtr2 {};

					Ship = *ShipPtr;
					GetElementStarShip(&Ship, &StarShipPtr2);
					const uint16_t num_weapons {(*StarShipPtr->RaceDescPtr->init_weapon_func)(ShipPtr, Weapon)};
					(void)num_weapons;
					ElementHandle* WeaponPtr {&Weapon[0]};

					ElementHandle w {*WeaponPtr};
					if (w)
					{
						LockElement(w, &EPtr);
						if (EPtr->state_flags & Appearing)
						{
							EPtr->next = EPtr->current;
							InitIntersectStartPoint(EPtr);
							InitIntersectEndPoint(EPtr);
							InitIntersectFrame(EPtr);
						}

						if (PlotIntercept(EPtr, lpEvalDesc->ObjectPtr,
										  EPtr->life_span, FragmentRange() / 2)
							< 80)
						{
							StarShipPtr->ship_input_state |= WeaponFlag;
						}

						UnlockElement(w);
						FreeElement(w);
					}
				}
				else
				{
					StarShipPtr->ship_input_state |= WeaponFlag;
				}
			}
		}
	}

	if (StarShipPtr->special_counter < MaxDoggies)
	{
		if (lpEvalDesc->ObjectPtr
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level <= SpecialEnergyCost
			&& !(StarShipPtr->ship_input_state & WeaponFlag))
		{
			StarShipPtr->ship_input_state |= SpecialFlag;
		}
	}
}

static uint16_t initialize_crystal(Element* ShipPtr, ElementHandle CrystalArray[])
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
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = ChenjesuOffset();
	MissileBlock.speed = MissileSpeed();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = crystal_preprocess;
	MissileBlock.blast_offs = MissileOffset;
	CrystalArray[0] = initialize_missile(&MissileBlock);

	if (CrystalArray[0])
	{
		Element* CrystalPtr {};

		LockElement(CrystalArray[0], &CrystalPtr);
		CrystalPtr->collision_func = crystal_collision;
		UnlockElement(CrystalArray[0]);
	}

	return (1);
}

static void chenjesu_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SpecialFlag)
		&& StarShipPtr->special_counter < MaxDoggies
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		spawn_doggy(ElementPtr);
	}

	StarShipPtr->special_counter = 1;
	/* say there is one doggy, because ship_postprocess will
			 * decrement special_counter */
}

static void chenjesu_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->special_counter > 1) /* only when STANDARD
											 * computer opponent
											 */
	{
		StarShipPtr->special_counter += MaxDoggies;
	}
	if (StarShipPtr->cur_status_flags
		& StarShipPtr->old_status_flags
		& WeaponFlag)
	{
		++StarShipPtr->weapon_counter;
	}
}

RaceDesc* init_chenjesu(void)
{
	if (IsHd())
	{
		chenjesu_desc.characteristics.max_thrust = ResScale(MaxThrust);
		chenjesu_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		chenjesu_desc.cyborg_control.WeaponRange = LongRangeWeaponHd;
	}
	else
	{
		chenjesu_desc.characteristics.max_thrust = MaxThrust;
		chenjesu_desc.characteristics.thrust_increment = ThrustIncrement;
		chenjesu_desc.cyborg_control.WeaponRange = LongRangeWeapon;
	}

	chenjesu_desc.preprocess_func = chenjesu_preprocess;
	chenjesu_desc.postprocess_func = chenjesu_postprocess;
	chenjesu_desc.init_weapon_func = initialize_crystal;
	chenjesu_desc.cyborg_control.intelligence_func = chenjesu_intelligence;

	RaceDesc* RaceDescPtr {&chenjesu_desc};

	return (RaceDescPtr);
}

} // namespace uqm
