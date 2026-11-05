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
#include "blackurq.h"
#include "resinst.h"

#include "uqm/globdata.h"

// Core characteristics
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {30};
static constexpr auto ThrustIncrement {6};
static constexpr auto TurnWait {4};
static constexpr auto ThrustWait {6};
static constexpr auto ShipMass {10};

// Buzzsaw
static constexpr auto WeaponEnergyCost {6};
static constexpr auto WeaponWait {6};
static auto MissileOffset() { return ResScale(9); }
static auto KohrAhOffset() { return ResScale(28); }
static auto MissileSpeed() { return ResScale(64); }
static constexpr auto MissileLife {64};
/* actually, it's as long as you hold the button down.*/
static constexpr auto MissileHits {10};
static constexpr auto MissileDamage {4};
static constexpr auto SawRate {0};
static constexpr auto MaxSaws {8};
static auto ActivateRange() { return ResScale(224); }
/* Originally SpaceWidth() - the distance within which
		 * stationary sawblades will home */
static constexpr auto TrackWait {4};
static auto FragmentSpeed() { return MissileSpeed(); }
static constexpr auto FragmentLife {10};
static auto FragmentRange() { return (FragmentLife * FragmentSpeed()); }

// F.R.I.E.D.
static constexpr auto SpecialEnergyCost {(MaxEnergySize / 2)};
static constexpr auto SpecialWait {9};
static auto GasOffset() { return ResScale(2); }
static auto GasSpeed() { return ResScale(16); }
static constexpr auto GasRate {2}; /* Controls animation of the gas cloud decay - the decay
                    * animation advances one frame every GasRate frames. */
static constexpr auto GasHits {100};
static constexpr auto GasDamage {3};
static constexpr auto GasAltDamage {50};
static constexpr auto NumGasClouds {16};

static RACE_DESC black_urquan_desc =
	{
		{/* SHIP_INFO */
		 "marauder",
		 FiresFore,
		 30, /* Super Melee cost */
		 MaxCrewSize, MaxCrewSize,
		 MaxEnergySize, MaxEnergySize,
		 KohrAhRaceStrings,
		 KohrAhIconMaskPmapAnim,
		 KohrAhMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
			2666 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				6000,
				6250,
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
			 KohrAhBigMaskPmapAnim,
			 KohrAhMedMaskPmapAnim,
			 KohrAhSmlMaskPmapAnim,
		 },
		 {
			 BuzzsawBigMaskPmapAnim,
			 BuzzsawMedMaskPmapAnim,
			 BuzzsawSmlMaskPmapAnim,
		 },
		 {
			 GasBigMaskPmapAnim,
			 GasMedMaskPmapAnim,
			 GasSmlMaskPmapAnim,
		 },
		 {KohrAhCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 KohrAhVictorySong,
		 KohrAhShipSounds,
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

static constexpr auto LastSpinIndex {1};

static void
spin_preprocess(ELEMENT* ElementPtr)
{
	ELEMENT* ShipPtr {};
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	LockElement(StarShipPtr->hShip, &ShipPtr);
	if (ShipPtr->crew_level
		&& ++StarShipPtr->RaceDescPtr->characteristics.special_wait > MaxSaws)
	{
		ElementPtr->life_span = 1;
		ElementPtr->state_flags |= Disappearing;
	}
	else
	{
		++ElementPtr->life_span;
		if (ElementPtr->turn_wait)
		{
			--ElementPtr->turn_wait;
		}
		else
		{
			if (GetFrameIndex(
					ElementPtr->current.image.frame)
				< LastSpinIndex)
			{
				ElementPtr->next.image.frame =
					IncFrameIndex(ElementPtr->current.image.frame);
			}
			else
			{
				ElementPtr->next.image.frame =
					SetAbsFrameIndex(ElementPtr->current.image.frame, 0);
			}
			ElementPtr->state_flags |= Changing;

			ElementPtr->turn_wait = SawRate;
		}
	}
	UnlockElement(StarShipPtr->hShip);
}

static void
buzztrack_preprocess(ELEMENT* ElementPtr)
{
	if (ElementPtr->thrust_wait)
	{
		--ElementPtr->thrust_wait;
	}
	else
	{
		uint16_t facing = 0;

		if (ElementPtr->hTarget == 0
			&& TrackShip(ElementPtr, &facing) < 0)
		{
			ZeroVelocityComponents(&ElementPtr->velocity);
		}
		else
		{
			int16_t delta_x {};
			int16_t delta_y {};
			ELEMENT* eptr {};

			LockElement(ElementPtr->hTarget, &eptr);
			delta_x = eptr->current.location.x
					- ElementPtr->current.location.x;
			delta_y = eptr->current.location.y
					- ElementPtr->current.location.y;
			UnlockElement(ElementPtr->hTarget);
			delta_x = WrapDeltaX(delta_x);
			delta_y = WrapDeltaY(delta_y);
			facing = NormalizeFacing(
				AngleToFacing(ARCTAN(delta_x, delta_y)));

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
			if (delta_x >= ActivateRange()
				|| delta_y >= ActivateRange()
				|| (uint32_t)((uint16_t)delta_x * delta_x)
						   + (uint32_t)((uint16_t)delta_y * delta_y)
					   >= (uint32_t)ActivateRange() * ActivateRange())
			{
				ZeroVelocityComponents(&ElementPtr->velocity);
			}
			else
			{
				ElementPtr->thrust_wait = TrackWait;
				SetVelocityVector(&ElementPtr->velocity,
								  DisplayToWorld(ResScale(2)), facing);
			}
		}
	}

	spin_preprocess(ElementPtr);
}

static void
decelerate_preprocess(ELEMENT* ElementPtr)
{
	int32_t dx {};
	int32_t dy {};

	GetCurrentVelocityComponentsSdword(&ElementPtr->velocity, &dx, &dy);
	dx /= 2;
	dy /= 2;
	SetVelocityComponents(&ElementPtr->velocity, dx, dy);
	if (dx == 0 && dy == 0)
	{
		ElementPtr->preprocess_func = buzztrack_preprocess;
	}

	spin_preprocess(ElementPtr);
}

static void
splinter_preprocess(ELEMENT* ElementPtr)
{
	ElementPtr->next.image.frame =
		IncFrameIndex(ElementPtr->current.image.frame);
	ElementPtr->state_flags |= Changing;
}

static void
buzzsaw_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
				  ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);

	if (ElementPtr0->state_flags & Disappearing)
	{
		ElementPtr0->state_flags &= ~Disappearing;
		ElementPtr0->state_flags |= Nonsolid | Changing;
		ElementPtr0->life_span = 5;
		ElementPtr0->next.image.frame =
			SetAbsFrameIndex(ElementPtr0->current.image.frame, 2);

		ElementPtr0->preprocess_func = splinter_preprocess;
	}
}

static void
buzzsaw_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (!(StarShipPtr->cur_status_flags & WeaponFlag))
	{
		ElementPtr->life_span >>= 1;
		ElementPtr->preprocess_func = decelerate_preprocess;
	}

	spin_preprocess(ElementPtr);
}

static void
buzzsaw_postprocess(ELEMENT* ElementPtr)
{
	ElementPtr->postprocess_func = 0;
	HELEMENT hElement {AllocElement()};
	if (hElement)
	{
		ELEMENT* ListElementPtr {};
		STARSHIP* StarShipPtr {};

		LockElement(hElement, &ListElementPtr);
		const uint16_t primIndex {ListElementPtr->PrimIndex};
		*ListElementPtr = *ElementPtr;
		ListElementPtr->PrimIndex = primIndex;
		DisplayArray[primIndex] =
			DisplayArray[ElementPtr->PrimIndex];
		ListElementPtr->current = ListElementPtr->next;
		InitIntersectStartPoint(ListElementPtr);
		InitIntersectEndPoint(ListElementPtr);
		ListElementPtr->state_flags = (ListElementPtr->state_flags
									   & ~(PreProcessFlag | Changing | Appearing))
									| PostProcessFlag;
		UnlockElement(hElement);

		GetElementStarShip(ElementPtr, &StarShipPtr);
		LockElement(StarShipPtr->hShip, &ListElementPtr);
		InsertElement(hElement, GetSuccElement(ListElementPtr));
		UnlockElement(StarShipPtr->hShip);

		ElementPtr->life_span = 0;
	}
}

static uint16_t
initialize_buzzsaw(ELEMENT* ShipPtr, HELEMENT SawArray[])
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
	MissileBlock.pixoffs = KohrAhOffset();
	MissileBlock.speed = MissileSpeed();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = buzzsaw_preprocess;
	MissileBlock.blast_offs = MissileOffset();
	SawArray[0] = initialize_missile(&MissileBlock);

	if (SawArray[0])
	{
		ELEMENT* SawPtr {};

		LockElement(SawArray[0], &SawPtr);
		SawPtr->turn_wait = SawRate;
		SawPtr->thrust_wait = 0;
		SawPtr->postprocess_func = buzzsaw_postprocess;
		SawPtr->collision_func = buzzsaw_collision;
		UnlockElement(SawArray[0]);
	}

	return (1);
}

static void
black_urquan_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
						  uint16_t ConcernCounter)
{
	STARSHIP* StarShipPtr {};

	EVALUATE_DESC* lpEvalDesc {&ObjectsOfConcern[ENEMY_WEAPON_INDEX]};
	if (lpEvalDesc->ObjectPtr
		&& lpEvalDesc->MoveState == ENTICE
		&& (lpEvalDesc->ObjectPtr->state_flags & CrewObject)
		&& lpEvalDesc->which_turn <= 8)
	{
		lpEvalDesc->MoveState = PURSUE;
	}

	ship_intelligence(ShipPtr,
					  ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state &= ~SpecialFlag;

	if (StarShipPtr->special_counter == 0
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SpecialEnergyCost
		&& lpEvalDesc->ObjectPtr
		&& lpEvalDesc->which_turn <= 8)
	{
		StarShipPtr->ship_input_state |= SpecialFlag;
	}

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		HELEMENT h {};
		HELEMENT hNext {};
		ELEMENT* BuzzSawPtr {};

		h = (StarShipPtr->old_status_flags & WeaponFlag) ?
				GetSuccElement(ShipPtr) :
				(HELEMENT)0;
		for (; h; h = hNext)
		{
			LockElement(h, &BuzzSawPtr);
			hNext = GetSuccElement(BuzzSawPtr);
			if (!(BuzzSawPtr->state_flags & Nonsolid)
				&& BuzzSawPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.weapon
				&& BuzzSawPtr->life_span > MissileLife * 3 / 4
				&& elementsOfSamePlayer(BuzzSawPtr, ShipPtr))
			{
				{
					//uint16_t which_turn;

					if (!PlotIntercept(BuzzSawPtr,
									   lpEvalDesc->ObjectPtr, BuzzSawPtr->life_span,
									   FragmentRange() / 2))
					{
						StarShipPtr->ship_input_state &= ~WeaponFlag;
					}
					else if (StarShipPtr->weapon_counter == 0)
					{
						StarShipPtr->ship_input_state |= WeaponFlag;
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
			}
			else if (StarShipPtr->weapon_counter == 0
					 && ship_weapons(ShipPtr, lpEvalDesc->ObjectPtr, FragmentRange() / 2))
			{
				StarShipPtr->ship_input_state |= WeaponFlag;
			}

			if (StarShipPtr->special_counter == 0
				&& !(StarShipPtr->ship_input_state & WeaponFlag)
				&& lpEvalDesc->which_turn <= 8
				&& (StarShipPtr->ship_input_state & (LeftFlag | RightFlag))
				&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SpecialEnergyCost)
			{
				StarShipPtr->ship_input_state |= SpecialFlag;
			}
		}
	}
}

static void
gas_cloud_preprocess(ELEMENT* ElementPtr)
{
	if (ElementPtr->turn_wait)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->current.image.frame);
		ElementPtr->state_flags |= Changing;

		ElementPtr->turn_wait = GasRate;
	}
}

static void
gas_cloud_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
					ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	if (ElementPtr1->state_flags & PlayerShip)
	{
		ElementPtr0->mass_points = GasDamage;
	}
	else
	{
		ElementPtr0->mass_points = GasAltDamage;
	}

	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void
spawn_gas_cloud(ELEMENT* ElementPtr)
{
	int32_t dx {};
	int32_t dy {};
	STARSHIP* StarShipPtr {};
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip(ElementPtr, &StarShipPtr);
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.index = 0;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = ResScale(20);
	MissileBlock.speed = GasSpeed();
	MissileBlock.hit_points = GasHits;
	MissileBlock.damage = GasDamage;
	MissileBlock.life =
		GetFrameCount(MissileBlock.farray[0]) * (GasRate + 1) - 1;
	MissileBlock.preprocess_func = gas_cloud_preprocess;
	MissileBlock.blast_offs = GasOffset();

	GetCurrentVelocityComponentsSdword(&ElementPtr->velocity, &dx, &dy);
	for (MissileBlock.face = 0;
		 MissileBlock.face < AngleToFacing(FullCircle);
		 MissileBlock.face +=
		 (AngleToFacing(FullCircle) / NumGasClouds))
	{
		HELEMENT hGasCloud {initialize_missile(&MissileBlock)};
		if (hGasCloud)
		{
			ELEMENT* GasCloudPtr {};

			LockElement(hGasCloud, &GasCloudPtr);
			SetElementStarShip(GasCloudPtr, StarShipPtr);
			GasCloudPtr->hTarget = 0;
			GasCloudPtr->turn_wait = GasRate - 1;
			GasCloudPtr->collision_func = gas_cloud_collision;
			DeltaVelocityComponents(&GasCloudPtr->velocity, dx, dy);
			UnlockElement(hGasCloud);
			PutElement(hGasCloud);
		}
	}
}

static void
black_urquan_postprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SpecialFlag)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		spawn_gas_cloud(ElementPtr);

		ProcessSound(GetSound(
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);

		StarShipPtr->special_counter = SpecialWait;
	}
}

static void
black_urquan_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	/* no spinning disks */
	StarShipPtr->RaceDescPtr->characteristics.special_wait = 0;
	if (StarShipPtr->weapon_counter == 0
		&& (StarShipPtr->cur_status_flags
			& StarShipPtr->old_status_flags & WeaponFlag))
	{
		++StarShipPtr->weapon_counter;
	}
}

RACE_DESC*
init_black_urquan(void)
{
	if (IsHd())
	{
		black_urquan_desc.characteristics.max_thrust = ResScale(MaxThrust);
		black_urquan_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		black_urquan_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd;
	}
	else
	{
		black_urquan_desc.characteristics.max_thrust = MaxThrust;
		black_urquan_desc.characteristics.thrust_increment =
			ThrustIncrement;
		black_urquan_desc.cyborg_control.WeaponRange = CloseRangeWeapon;
	}

	black_urquan_desc.preprocess_func = black_urquan_preprocess;
	black_urquan_desc.postprocess_func = black_urquan_postprocess;
	black_urquan_desc.init_weapon_func = initialize_buzzsaw;
	black_urquan_desc.cyborg_control.intelligence_func = black_urquan_intelligence;

	RACE_DESC* RaceDescPtr {&black_urquan_desc};

	return (RaceDescPtr);
}
