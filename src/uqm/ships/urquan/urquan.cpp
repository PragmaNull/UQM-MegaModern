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
#include "urquan.h"
#include "resinst.h"
#include "../../intel.h"
#include "uqm/globdata.h"
#include "../../setup.h"
#include "../../setupmenu.h"
#include "uqm/colors.h"
#include <stdlib.h>

namespace uqm
{
// Core characteristics
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {30};
static constexpr auto ThrustIncrement {6};
static constexpr auto ThrustWait {6};
static constexpr auto TurnWait {4};
static constexpr auto ShipMass {10};

// Fusion blast
static constexpr auto WeaponEnergyCost {6};
static constexpr auto WeaponWait {6};
static constexpr auto MissileSpeed {DisplayToWorld(20)};
static constexpr auto MissileLife {20};
static constexpr auto MissileHits {10};
static constexpr auto MissileDamage {6};
static auto MissileOffset() { return chooseIfHd(8, 21); }
static auto UrquanOffset() { return ResScale(32); }

// Fighters
static constexpr auto SpecialEnergyCost {8};
static constexpr auto SpecialWait {9};
static auto FighterOffset() { return ResScale(4); }
static auto FighterSpeed() { return DisplayToWorld(ResScale(8)); }
static constexpr auto OneWayFlight {125};
static constexpr auto TrackThreshold {6};
static constexpr auto FighterLife {OneWayFlight + OneWayFlight + 150};
static constexpr auto FighterHits {1};
static constexpr auto FighterMass {0};
static constexpr auto FighterWeaponWait {8};
static auto FighterLaserRange() { return DisplayToWorld(ResScale(40) + FighterOffset()); }
static constexpr auto IonLife {1};
static constexpr auto StartIonColor {BuildColorRgba(0xfc, 0xb8, 0xfc, 0xff)};

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }

static RaceDesc urquan_desc =
	{
		{/* ShipInfoDesc */
		 "dreadnought",
		 FiresFore | SeekingSpecial,
		 30, /* Super Melee cost */
		 MaxCrewSize, MaxCrewSize,
		 MaxEnergySize, MaxEnergySize,
		 UrquanRaceStrings,
		 UrquanIconMaskPmapAnim,
		 UrquanMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			2666 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				5750,
				6000,
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
			 UrquanBigMaskPmapAnim,
			 UrquanMedMaskPmapAnim,
			 UrquanSmlMaskPmapAnim,
		 },
		 {
			 FusionBigMaskPmapAnim,
			 FusionMedMaskPmapAnim,
			 FusionSmlMaskPmapAnim,
		 },
		 {
			 FighterBigMaskPmapAnim,
			 FighterMedMaskPmapAnim,
			 FighterSmlMaskPmapAnim,
		 },
		 {UrquanCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 UrquanVictorySong,
		 UrquanShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			MissileSpeed * MissileLife,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static uint16_t initialize_fusion(Element* ShipPtr, ElementHandle FusionArray[])
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
	MissileBlock.pixoffs = UrquanOffset();
	MissileBlock.speed = MissileSpeedHd();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();
	FusionArray[0] = initialize_missile(&MissileBlock);

	return (1);
}

static void fighter_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};
	LaserDesc LaserBlock;

	GetElementStarShip(ElementPtr, &StarShipPtr);
	LaserBlock.cx = ElementPtr->next.location.x;
	LaserBlock.cy = ElementPtr->next.location.y;
	LaserBlock.face = ElementPtr->thrust_wait;
	LaserBlock.ex = Cosine(FacingToAngle(LaserBlock.face), FighterLaserRange());
	LaserBlock.ey = Sine(FacingToAngle(LaserBlock.face), FighterLaserRange());
	LaserBlock.sender = ElementPtr->playerNr;
	LaserBlock.flags = IgnoreSimilar;
	LaserBlock.pixoffs = FighterOffset();
	LaserBlock.color = BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E);
	ElementHandle Laser {initialize_laser(&LaserBlock)};
	if (Laser)
	{
		Element* LaserPtr {};

		LockElement(Laser, &LaserPtr);
		SetElementStarShip(LaserPtr, StarShipPtr);

		ProcessSound(GetSound(
						 /* FIGHTER_ZAP */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
					 LaserPtr);

		UnlockElement(Laser);
		PutElement(Laser);
	}

	ElementPtr->postprocess_func = 0;
	ElementPtr->thrust_wait = FighterWeaponWait;
}

//static void
//ion_preprocess (Element* ElementPtr)
//{
//	static const Color colorTable[] =
//	{
//		BuildColorRgba(0xfc, 0x7c, 0xfc, 0xff),
//		BuildColorRgba(0xfc, 0x40, 0xfc, 0xff),
//		BuildColorRgba(0xd0, 0x14, 0xd0, 0xff),
//		BuildColorRgba(0x90, 0x08, 0x94, 0xff),
//	};
//	const size_t colorTabCount = ARRAY_SIZE (colorTable);
//
//	ElementPtr->colorCycleIndex++;
//	if (ElementPtr->colorCycleIndex != colorTabCount)
//	{
//		ElementPtr->life_span = ElementPtr->thrust_wait;
//
//		SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
//			colorTable[ElementPtr->colorCycleIndex]);
//
//		ElementPtr->state_flags &= ~Disappearing;
//		ElementPtr->state_flags |= Changing;
//	}
//}

//static void
//spawn_fighter_ion_trail (Element *ElementPtr, Starship *StarShipPtr,
//		uint16_t facing)
//{
//	ElementHandle hIonElement;
//
//	hIonElement = AllocElement ();
//	if (hIonElement)
//	{
//		uint16_t angle;
//		Element *IonElementPtr;
//
//		angle = FacingToAngle (facing) + HalfCircle;
//
//		InsertElement (hIonElement, GetHeadElement ());
//		LockElement (hIonElement, &IonElementPtr);
//		IonElementPtr->playerNr = NeutralPlayerNum;
//		IonElementPtr->state_flags = Appearing | FiniteLife | Nonsolid;
//		IonElementPtr->thrust_wait = IonLife;
//		IonElementPtr->life_span = IonElementPtr->thrust_wait;
//		SetPrimType (&DisplayArray[IonElementPtr->PrimIndex], POINT_PRIM);
//		SetPrimColor (&DisplayArray[IonElementPtr->PrimIndex], StartIonColor);
//		IonElementPtr->colorCycleIndex = 0;
//		IonElementPtr->current.location = ElementPtr->current.location;
//		IonElementPtr->current.location.x +=
//				(int16_t)Cosine (angle, DisplayToWorld (ResScale (2)));
//		IonElementPtr->current.location.y +=
//				(int16_t)Sine (angle, DisplayToWorld (ResScale (2)));
//		IonElementPtr->death_func = ion_preprocess;
//
//		SetElementStarShip (IonElementPtr, StarShipPtr);
//
//		{
//			IonElementPtr->next = IonElementPtr->current;
//			--IonElementPtr->life_span;
//			IonElementPtr->state_flags |= PreProcessFlag;
//		}
//
//		UnlockElement (hIonElement);
//	}
//}

static void fighter_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);

	++StarShipPtr->RaceDescPtr->characteristics.special_wait;
	if (FighterLife - ElementPtr->life_span > TrackThreshold
		&& !(ElementPtr->state_flags & Changing))
	{
		uint16_t orig_facing {};
		uint16_t facing {};
		Element* eptr {};

		bool Enroute {true};

		int32_t delta_x {StarShipPtr->RaceDescPtr->ship_info.crew_level};
		int32_t delta_y {ElementPtr->life_span};

		orig_facing = facing =
			GetFrameIndex(ElementPtr->current.image.frame);
		if (((delta_y & 1) || ElementPtr->hTarget
			 || TrackShip(ElementPtr, &facing) >= 0)
			&& (delta_x == 0 || delta_y >= OneWayFlight))
		{
			ElementPtr->state_flags |= IgnoreSimilar;
		}
		else if (delta_x)
		{
			LockElement(StarShipPtr->hShip, &eptr);
			delta_x = eptr->current.location.x
					- ElementPtr->current.location.x;
			delta_y = eptr->current.location.y
					- ElementPtr->current.location.y;
			UnlockElement(StarShipPtr->hShip);
			delta_x = WrapDeltaX(delta_x);
			delta_y = WrapDeltaY(delta_y);
			facing = NormalizeFacing(
				AngleToFacing(ARCTAN(delta_x, delta_y)));

#ifdef NEVER
			if (delta_x < 0)
			{
				delta_x = -delta_x;
			}
			if (delta_y < 0)
			{
				delta_y = -delta_y;
			}
			if (delta_x <= LASER_RANGE && delta_y <= LASER_RANGE)
#endif /* NEVER */
				ElementPtr->state_flags &= ~IgnoreSimilar;

			Enroute = false;
		}

		if (ElementPtr->thrust_wait > 0)
		{
			--ElementPtr->thrust_wait;
		}

		if (ElementPtr->hTarget)
		{
			LockElement(ElementPtr->hTarget, &eptr);
			delta_x = eptr->current.location.x
					- ElementPtr->current.location.x;
			delta_y = eptr->current.location.y
					- ElementPtr->current.location.y;
			UnlockElement(ElementPtr->hTarget);
			delta_x = WrapDeltaX(delta_x);
			delta_y = WrapDeltaY(delta_y);

			if (ElementPtr->thrust_wait == 0
				&& abs(delta_x) < FighterLaserRange() * 3 / 4
				&& abs(delta_y) < FighterLaserRange() * 3 / 4
				&& delta_x * delta_x + delta_y * delta_y < (FighterLaserRange() * 3 / 4) * (FighterLaserRange() * 3 / 4))
			{
				ElementPtr->thrust_wait =
					(uint8_t)NormalizeFacing(
						AngleToFacing(ARCTAN(delta_x, delta_y)));
				ElementPtr->postprocess_func = fighter_postprocess;
			}

			if (Enroute)
			{
				facing = GetFrameIndex(eptr->current.image.frame);
				if (ElementPtr->turn_wait & LeftFlag)
				{
					delta_x += Cosine(FacingToAngle(facing - 4),
									  DisplayToWorld(ResScale(30)));
					delta_y += Sine(FacingToAngle(facing - 4),
									DisplayToWorld(ResScale(30)));
				}
				else
				{
					delta_x += Cosine(FacingToAngle(facing + 4),
									  DisplayToWorld(ResScale(30)));
					delta_y += Sine(FacingToAngle(facing + 4),
									DisplayToWorld(ResScale(30)));
				}
				facing = NormalizeFacing(
					AngleToFacing(ARCTAN(delta_x, delta_y)));
			}
		}
		ElementPtr->state_flags |= Changing;

		if (facing != orig_facing)
		{
			ElementPtr->next.image.frame = SetAbsFrameIndex(
				ElementPtr->next.image.frame, facing);
		}
		SetVelocityVector(
			&ElementPtr->velocity, FighterSpeed(), facing);
		//spawn_fighter_ion_trail (ElementPtr, StarShipPtr, facing);
	}
}

static void fighter_collision(Element* ElementPtr0, GfxPoint* pPt0,
							  Element* ElementPtr1, GfxPoint* pPt1)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr0, &StarShipPtr);
	if (GravityMass(ElementPtr1->mass_points))
	{
		ElementHandle hFighterElement {AllocElement()};
		if (hFighterElement)
		{
			uint16_t travel_facing {};
			int16_t delta_facing {};
			Element* FighterElementPtr {};

			LockElement(hFighterElement, &FighterElementPtr);
			const uint16_t primIndex {FighterElementPtr->PrimIndex};
			*FighterElementPtr = *ElementPtr0;
			FighterElementPtr->PrimIndex = primIndex;
			DisplayArray[primIndex] =
				DisplayArray[ElementPtr0->PrimIndex];
			FighterElementPtr->state_flags &= ~PreProcessFlag;
			FighterElementPtr->state_flags |= Changing;
			FighterElementPtr->next = FighterElementPtr->current;
			travel_facing = GetVelocityTravelAngle(
				&FighterElementPtr->velocity);
			delta_facing = NormalizeAngle(
				ARCTAN(pPt1->x - pPt0->x, pPt1->y - pPt0->y)
				- travel_facing);
			if (delta_facing == 0)
			{
				if (FighterElementPtr->turn_wait & LeftFlag)
				{
					travel_facing -= Quadrant;
				}
				else
				{
					travel_facing += Quadrant;
				}
			}
			else if (delta_facing <= HalfCircle)
			{
				travel_facing -= Quadrant;
			}
			else
			{
				travel_facing += Quadrant;
			}

			travel_facing = NormalizeFacing(AngleToFacing(
				NormalizeAngle(travel_facing)));
			FighterElementPtr->next.image.frame =
				SetAbsFrameIndex(FighterElementPtr->next.image.frame,
								 travel_facing);
			SetVelocityVector(&FighterElementPtr->velocity,
							  FighterSpeed(), travel_facing);
			UnlockElement(hFighterElement);

			PutElement(hFighterElement);
		}

		ElementPtr0->state_flags |= Disappearing | Collision;
	}
	else if (ElementPtr0->pParent != ElementPtr1->pParent)
	{
		ElementPtr0->blast_offset = 0;
		weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
		ElementPtr0->state_flags |= Disappearing | Collision;
	}
	else if (ElementPtr1->state_flags & PlayerShip)
	{
		ProcessSound(GetSound(
						 /* FIGHTERS_RETURN */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 3),
					 ElementPtr1);
		DeltaCrew(ElementPtr1, 1);
		ElementPtr0->state_flags |= Disappearing | Collision;
	}

	if (ElementPtr0->state_flags & Disappearing)
	{
		ElementPtr0->state_flags &= ~Disappearing;

		ElementPtr0->hit_points = 0;
		ElementPtr0->life_span = 0;
		ElementPtr0->state_flags |= Nonsolid;

		--StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

static void spawn_fighters(Element* ElementPtr)
{
	int16_t i {};
	uint16_t facing {};
	ElementHandle hFighterElement {};
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	facing = StarShipPtr->ShipFacing + AngleToFacing(HalfCircle);
	const int32_t delta_x {Cosine(FacingToAngle(facing), DisplayToWorld(ResScale(14)))};
	const int32_t delta_y {Sine(FacingToAngle(facing), DisplayToWorld(ResScale(14)))};

	i = ElementPtr->crew_level > 2 ? 2 : 1;
	while (i-- && (hFighterElement = AllocElement()))
	{
		int32_t sx {};
		int32_t sy {};
		uint16_t fighter_facing {};
		Element* FighterElementPtr {};

		if (!antiCheat(ElementPtr, false, uqm::GodModeFlags::NoDamage))
		{
			DeltaCrew(ElementPtr, -1);
		}

		PutElement(hFighterElement);
		LockElement(hFighterElement, &FighterElementPtr);
		FighterElementPtr->hit_points = FighterHits;
		FighterElementPtr->mass_points = FighterMass;
		FighterElementPtr->thrust_wait = TrackThreshold + 1;
		FighterElementPtr->playerNr = ElementPtr->playerNr;
		FighterElementPtr->state_flags = Appearing | FiniteLife
									   | CrewObject | IgnoreSimilar;
		FighterElementPtr->life_span = FighterLife;
		SetPrimType(&DisplayArray[FighterElementPtr->PrimIndex],
					PrimitiveType::Stamp);
		{
			FighterElementPtr->preprocess_func = fighter_preprocess;
			FighterElementPtr->postprocess_func = 0;
			FighterElementPtr->collision_func = fighter_collision;
			FighterElementPtr->death_func = nullptr;
		}

		FighterElementPtr->current.location = ElementPtr->next.location;
		if (i == 1)
		{
			FighterElementPtr->turn_wait = LeftFlag;
			fighter_facing = NormalizeFacing(facing + 2);
			FighterElementPtr->current.location.x += delta_x - delta_y;
			FighterElementPtr->current.location.y += delta_y + delta_x;
		}
		else
		{
			FighterElementPtr->turn_wait = RightFlag;
			fighter_facing = NormalizeFacing(facing - 2);
			FighterElementPtr->current.location.x += delta_x + delta_y;
			FighterElementPtr->current.location.y += delta_y - delta_x;
		}
		sx = Cosine(FacingToAngle(fighter_facing),
					WorldToVelocity(FighterSpeed()));
		sy = Sine(FacingToAngle(fighter_facing),
				  WorldToVelocity(FighterSpeed()));
		SetVelocityComponents(&FighterElementPtr->velocity, sx, sy);
		FighterElementPtr->current.location.x -= VelocityToWorld(sx);
		FighterElementPtr->current.location.y -= VelocityToWorld(sy);

		FighterElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
		FighterElementPtr->current.image.frame =
			SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0],
							 fighter_facing);
		SetElementStarShip(FighterElementPtr, StarShipPtr);
		UnlockElement(hFighterElement);
	}
}

static void urquan_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	ObjectsOfConcern[*IntelObjectIndex::EnemyShip].MoveState = MovementState::Pursue;
	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
	if (lpEvalDesc->ObjectPtr
		&& lpEvalDesc->MoveState == MovementState::Entice
		&& (!(lpEvalDesc->ObjectPtr->state_flags & CrewObject)
			|| lpEvalDesc->which_turn <= 8)
		&& (!(lpEvalDesc->ObjectPtr->state_flags & FiniteLife)
			|| (lpEvalDesc->ObjectPtr->mass_points >= 4
				&& lpEvalDesc->which_turn == 2
				&& ObjectsOfConcern[*IntelObjectIndex::EnemyShip].which_turn > 16)))
	{
		lpEvalDesc->MoveState = MovementState::Pursue;
	}

	ship_intelligence(ShipPtr,
					  ObjectsOfConcern, ConcernCounter);

	lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyShip];
	{
		Starship* EnemyStarShipPtr = nullptr;

		if (lpEvalDesc->ObjectPtr)
		{
			GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		}
		if (StarShipPtr->special_counter == 0
			&& lpEvalDesc->ObjectPtr
			&& StarShipPtr->RaceDescPtr->ship_info.crew_level > (StarShipPtr->RaceDescPtr->ship_info.max_crew >> 2)
			&& !(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
				 & PointDefense)
			&& (StarShipPtr->RaceDescPtr->characteristics.special_wait < 6
				|| (Maneuverability(
						&EnemyStarShipPtr->RaceDescPtr->cyborg_control)
						<= ResolutionCompensated(SlowShip)
					&& !(EnemyStarShipPtr->cur_status_flags & ShipBeyondMaxSpeed))
				|| (lpEvalDesc->which_turn <= 12
					&& (StarShipPtr->ship_input_state & (LeftFlag | RightFlag))
					&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= (uint8_t)(StarShipPtr->RaceDescPtr->ship_info.max_energy >> 1)))
			&& !ObjectCloaked(lpEvalDesc->ObjectPtr))
		{
			StarShipPtr->ship_input_state |= SpecialFlag;
		}
		else
		{
			StarShipPtr->ship_input_state &= ~SpecialFlag;
		}
	}

	StarShipPtr->RaceDescPtr->characteristics.special_wait = 0;
}

static void urquan_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SpecialFlag)
		&& ElementPtr->crew_level > 1
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		ProcessSound(GetSound(
						 /* LAUNCH_FIGHTERS */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);
		spawn_fighters(ElementPtr);

		StarShipPtr->special_counter = SpecialWait;
	}
}

RaceDesc* init_urquan(void)
{
	if (IsHd())
	{
		urquan_desc.characteristics.max_thrust = ResScale(MaxThrust);
		urquan_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		urquan_desc.cyborg_control.WeaponRange = MissileSpeedHd() * MissileLife;
	}
	else
	{
		urquan_desc.characteristics.max_thrust = MaxThrust;
		urquan_desc.characteristics.thrust_increment = ThrustIncrement;
		urquan_desc.cyborg_control.WeaponRange =
			MissileSpeed * MissileLife;
	}

	urquan_desc.postprocess_func = urquan_postprocess;
	urquan_desc.init_weapon_func = initialize_fusion;
	urquan_desc.cyborg_control.intelligence_func = urquan_intelligence;

	RaceDesc* RaceDescPtr {&urquan_desc};

	return (RaceDescPtr);
}

} // namespace uqm
