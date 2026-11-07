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
#include "orz.h"
#include "resinst.h"
#include "../../setup.h"
#include "../../setupmenu.h"
#include "uqm/colors.h"
#include "uqm/globdata.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {16};
static constexpr auto MaxEnergy {20};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {6};
static constexpr auto MaxThrust {35};
static constexpr auto ThrustIncrement {5};
static constexpr auto ThrustWait {0};
static constexpr auto TurnWait {1};
static constexpr auto ShipMass {4};

// Howitzer
static constexpr auto WeaponEnergyCost {MaxEnergy / 3};
static constexpr auto WeaponWait {4};

static constexpr auto MissileSpeed {DisplayToWorld(30)};
static constexpr auto MissileLife {12};
static constexpr auto MissileHits {2};
static constexpr auto MissileDamage {3};
static constexpr auto MissileOffset {1};

// Marine
static constexpr auto SpecialEnergyCost {0};
static constexpr auto SpecialWait {12};
static auto MarineMaxThrust() { return ResScale(32); }
static auto MarineThrustIncrement() { return ResScale(8); }
static constexpr auto MarineHitPoints {3};
static constexpr auto MarineMassPoints {1};
static constexpr auto MaxMarines {8};
static constexpr auto MarineWait {12};
static constexpr auto IonLife {1};
static constexpr auto StartIonColor {BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7A)};

// Rotating Turret
static auto TurretOffset() { return ResScale(14); }
static constexpr auto TurretWait {3};

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }

static RaceDesc orz_desc =
	{
		{/* ShipInfoDesc */
		 "nemesis",
		 FiresFore | SeekingSpecial,
		 23, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 OrzRaceStrings,
		 OrzIconMaskPmapAnim,
		 OrzMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			333 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				3608,
				2637,
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
			 OrzBigMaskPmapAnim,
			 OrzMedMaskPmapAnim,
			 OrzSmlMaskPmapAnim,
		 },
		 {
			 HowitzerBigMaskPmapAnim,
			 HowitzerMedMaskPmapAnim,
			 HowitzerSmlMaskPmapAnim,
		 },
		 {
			 TurretBigMaskPmapAnim,
			 TurretMedMaskPmapAnim,
			 TurretSmlMaskPmapAnim,
		 },
		 {OrzCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 OrzVictorySong,
		 OrzShipSounds,
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

static void howitzer_collision(Element* ElementPtr0, GfxPoint* pPt0,
							   Element* ElementPtr1, GfxPoint* pPt1)
{
	if (!elementsOfSamePlayer(ElementPtr0, ElementPtr1))
	{
		weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	}
}

static uint16_t initialize_turret_missile(Element* ShipPtr, ElementHandle MissileArray[])
{
	Element* TurretPtr {};
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;

	LockElement(GetSuccElement(ShipPtr), &TurretPtr);
	if (TurretPtr->turn_wait == 0
		&& (StarShipPtr->cur_status_flags & SpecialFlag)
		&& (StarShipPtr->cur_status_flags & (LeftFlag | RightFlag)))
	{
		if (StarShipPtr->cur_status_flags & RightFlag)
		{
			++TurretPtr->thrust_wait;
		}
		else
		{
			--TurretPtr->thrust_wait;
		}

		TurretPtr->turn_wait = TurretWait + 1;
	}
	MissileBlock.face = MissileBlock.index =
		NormalizeFacing(StarShipPtr->ShipFacing
						+ TurretPtr->thrust_wait);
	UnlockElement(GetSuccElement(ShipPtr));

	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = TurretOffset();
	MissileBlock.speed = MissileSpeedHd();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset;
	MissileArray[0] = initialize_missile(&MissileBlock);

	if (MissileArray[0])
	{
		Element* HowitzerPtr {};

		LockElement(MissileArray[0], &HowitzerPtr);
		HowitzerPtr->collision_func = howitzer_collision;
		UnlockElement(MissileArray[0]);
	}

	return (1);
}

static uint8_t count_marines(Starship* StarShipPtr, bool FindSpot)
{
	uint8_t id_use[MaxMarines];
	ElementHandle hElement {};
	ElementHandle hNextElement {};

	uint8_t num_marines {MaxMarines};
	while (num_marines--)
	{
		id_use[num_marines] = 0;
	}

	num_marines = 0;
	for (hElement = GetTailElement(); hElement; hElement = hNextElement)
	{
		Element* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		hNextElement = GetPredElement(ElementPtr);
		if (ElementPtr->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.special
			&& ElementPtr->life_span
			&& !(ElementPtr->state_flags & (FiniteLife | Disappearing)))
		{
			if (ElementPtr->state_flags & Nonsolid)
			{
				id_use[ElementPtr->turn_wait] = 1;
			}

			if (++num_marines == MaxMarines)
			{
				UnlockElement(hElement);
				hNextElement = 0;
			}
		}
		UnlockElement(hElement);
	}

	if (FindSpot)
	{
		num_marines = 0;
		while (id_use[num_marines])
		{
			++num_marines;
		}
	}

	return (num_marines);
}

static void orz_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							 uint16_t ConcernCounter)
{
	Element* TurretPtr {};
	Starship* StarShipPtr {};

	LockElement(GetSuccElement(ShipPtr), &TurretPtr);

	++TurretPtr->turn_wait;
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);
	--TurretPtr->turn_wait;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	const ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (lpEvalDesc->ObjectPtr == 0)
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
	}
	else if (StarShipPtr->special_counter != 1)
	{
		Starship* EnemyStarShipPtr {};

		if (ShipPtr->turn_wait == 0
			&& lpEvalDesc->MoveState == MovementState::Entice
			&& lpEvalDesc->which_turn < 24
			&& (StarShipPtr->cur_status_flags
				& (ShipAtMaxSpeed | ShipBeyondMaxSpeed))
			&& !(StarShipPtr->ship_input_state & ThrustFlag)
			&& NormalizeAngle(
				   GetVelocityTravelAngle(&ShipPtr->velocity)
				   - ARCTAN(
					   lpEvalDesc->ObjectPtr->next.location.x
						   - ShipPtr->next.location.x,
					   lpEvalDesc->ObjectPtr->next.location.y
						   - ShipPtr->next.location.y)
				   + (Quadrant - (Octant >> 1)))
				   >= ((Quadrant - (Octant >> 1)) << 1))
		{
			StarShipPtr->ship_input_state &= ~(LeftFlag | RightFlag);
		}

		StarShipPtr->ship_input_state &= ~SpecialFlag;
		if (ShipPtr->turn_wait == 0
			&& !(StarShipPtr->ship_input_state & (LeftFlag | RightFlag | WeaponFlag))
			&& TurretPtr->turn_wait == 0)
		{
			int16_t delta_facing {};
			uint16_t facing {}; //, orig_facing;

			facing = NormalizeFacing(StarShipPtr->ShipFacing
									 + TurretPtr->thrust_wait);
			if ((delta_facing = TrackShip(TurretPtr, &facing)) > 0)
			{
				StarShipPtr->ship_input_state |= SpecialFlag;
				if (delta_facing == AngleToFacing(HalfCircle))
				{
					delta_facing += (((uint8_t)uqm::random().next() & 1) << 1) - 1;
				}

				if (delta_facing < AngleToFacing(HalfCircle))
				{
					StarShipPtr->ship_input_state |= RightFlag;
				}
				else
				{
					StarShipPtr->ship_input_state |= LeftFlag;
				}
			}
		}

		GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (StarShipPtr->special_counter == 0
			&& !(StarShipPtr->ship_input_state & WeaponFlag)
			&& StarShipPtr->RaceDescPtr->ship_info.crew_level > (uint8_t)(StarShipPtr->RaceDescPtr->ship_info.max_crew >> 2)
			&& !(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
				 & PointDefense)
			&& (Maneuverability(
					&EnemyStarShipPtr->RaceDescPtr->cyborg_control)
					< ResolutionCompensated(SlowShip)
				|| lpEvalDesc->which_turn <= 12
				|| count_marines(StarShipPtr, false) < 2))
		{
			StarShipPtr->ship_input_state |= WeaponFlag | SpecialFlag;
		}
	}

	UnlockElement(GetSuccElement(ShipPtr));
}

static void ion_preprocess(Element* ElementPtr)
{
	/* Originally, this table also contained the now commented out
	 * entries. It then used some if statements to skip over these.
	 * The current behaviour is the same as the old behavior.
	 */
	const static Color colorTable[] =
		{
			BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7a),
			//BuildColor (MakeRgb15 (0x1F, 0x11, 0x00), 0x7b),
			BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7c),
			//BuildColor (MakeRgb15 (0x1F, 0x0A, 0x00), 0x7d),
			//BuildColor (MakeRgb15 (0x1F, 0x07, 0x00), 0x7e),
			BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7f),

			//BuildColor (MakeRgb15 (0x1F, 0x00, 0x00), 0x2a),
			BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2b),
			//BuildColor (MakeRgb15 (0x17, 0x00, 0x00), 0x2c),
			BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2d),
			//BuildColor (MakeRgb15 (0x0F, 0x00, 0x00), 0x2e),
			//BuildColor (MakeRgb15 (0x0B, 0x00, 0x00), 0x2f),
		};
	const size_t colorTabCount = std::size(colorTable);

	ElementPtr->colorCycleIndex++;
	if (ElementPtr->colorCycleIndex != colorTabCount)
	{
		ElementPtr->life_span = ElementPtr->thrust_wait;

		SetPrimColor(&DisplayArray[ElementPtr->PrimIndex],
					 colorTable[ElementPtr->colorCycleIndex]);

		ElementPtr->state_flags &= ~Disappearing;
		ElementPtr->state_flags |= Changing;
	}
}

static void marine_preprocess(Element* ElementPtr);

void intruder_preprocess(Element* ElementPtr)
{
	ElementHandle hElement {};
	ElementHandle hNextElement {};
	Element* ShipPtr {};
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	LockElement(StarShipPtr->hShip, &ShipPtr);
	if (ShipPtr->crew_level == 0
		&& ShipPtr->life_span == 1
		&& (ShipPtr->state_flags & (FiniteLife | Nonsolid)) == (FiniteLife | Nonsolid))
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= Disappearing;
	}
	UnlockElement(StarShipPtr->hShip);

	if (ElementPtr->thrust_wait)
	{
		--ElementPtr->thrust_wait;
	}

	for (hElement = GetHeadElement(); hElement; hElement = hNextElement)
	{
		LockElement(hElement, &ShipPtr);
		if ((ShipPtr->state_flags & PlayerShip)
			&& !elementsOfSamePlayer(ShipPtr, ElementPtr))
		{
			GfxStamp s;

			if (ElementPtr->thrust_wait == MarineWait)
			{
				--ElementPtr->thrust_wait;

				s.origin.x = ResScale(16 + (ElementPtr->turn_wait & 3) * 9);
				s.origin.y = ResScale(14 + (ElementPtr->turn_wait >> 2) * 11);
				s.frame = SetAbsFrameIndex(ElementPtr->next.image.farray[0],
										   GetFrameCount(ElementPtr->next.image.farray[0]) - 2);
				if (IsHd()) // Clear background
				{
					ModifySilhouette(ShipPtr, &s, ModifySwap);
				}
				ModifySilhouette(ShipPtr, &s, 0);
			}

			ElementPtr->next.location = ShipPtr->next.location;

			if (ShipPtr->crew_level == 0
				|| ElementPtr->life_span == 0)
			{
				UnlockElement(hElement);
				hElement = 0;
LeftShip:
				s.origin.x = ResScale(16 + (ElementPtr->turn_wait & 3) * 9);
				s.origin.y = ResScale(14 + (ElementPtr->turn_wait >> 2) * 11);
				s.frame = ElementPtr->next.image.frame;
				ModifySilhouette(ShipPtr, &s, ModifySwap);
			}
			else if (ElementPtr->thrust_wait == 0)
			{
				ElementPtr->thrust_wait = MarineWait;

				const uint8_t randval {(uint8_t)uqm::random().next()};
				if (randval < (0x0100 / 16))
				{
					ElementPtr->life_span = 0;
					ElementPtr->state_flags |= Disappearing;

					ProcessSound(GetSound(
									 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 4),
								 ElementPtr);
					goto LeftShip;
				}
				else if (randval < (0x0100 / 2 + 0x0100 / 16))
				{
					if (!antiCheat(ElementPtr, true, uqm::GodModeFlags::NoDamage))
					{
						if (!DeltaCrew(ShipPtr, -1))
						{
							ShipPtr->life_span = 0;
						}
					}

					++ElementPtr->thrust_wait;
					s.origin.x = ResScale(16 + (ElementPtr->turn_wait & 3) * 9);
					s.origin.y = ResScale(14 + (ElementPtr->turn_wait >> 2) * 11);
					s.frame = SetAbsFrameIndex(ElementPtr->next.image.farray[0],
											   GetFrameCount(ElementPtr->next.image.farray[0]) - 1);
					ModifySilhouette(ShipPtr, &s, 0);
					ProcessSound(GetSound(
									 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 3),
								 ElementPtr);
				}
			}

			UnlockElement(hElement);
			break;
		}
		hNextElement = GetSuccElement(ShipPtr);
		UnlockElement(hElement);
	}

	if (hElement == 0 && ElementPtr->life_span)
	{
		ElementPtr->state_flags &= ~Nonsolid;
		ElementPtr->state_flags |= Changing | CrewObject;
		SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
					PrimitiveType::Stamp);

		ElementPtr->current.image.frame =
			ElementPtr->next.image.frame =
				SetAbsFrameIndex(
					StarShipPtr->RaceDescPtr->ship_data.special[0], 21);
		ElementPtr->thrust_wait = 0;
		ElementPtr->turn_wait =
			uqm::makeByte(0, NormalizeFacing((uint8_t)uqm::random().next()));
		ElementPtr->preprocess_func = marine_preprocess;
	}
}

// XXX: merge this with spawn_ion_trail from tactrans.c?
static void
spawn_marine_ion_trail(Element* ElementPtr, Starship* StarShipPtr,
					   uint16_t facing)
{
	ElementHandle hIonElement {AllocElement()};
	if (hIonElement)
	{
		uint16_t angle {};
		Element* IonElementPtr {};

		angle = FacingToAngle(facing) + HalfCircle;

		InsertElement(hIonElement, GetHeadElement());
		LockElement(hIonElement, &IonElementPtr);
		IonElementPtr->playerNr = NeutralPlayerNum;
		IonElementPtr->state_flags = Appearing | FiniteLife | Nonsolid;
		IonElementPtr->thrust_wait = IonLife;
		IonElementPtr->life_span = IonElementPtr->thrust_wait;
		// When the Element "dies", in the death_func
		// 'cycle_ion_trail', it is given new life a number of
		// times, by setting life_span to thrust_wait.
		SetPrimType(&DisplayArray[IonElementPtr->PrimIndex],
					PrimitiveType::Point); // Actual marine ion trail
		SetPrimColor(&DisplayArray[IonElementPtr->PrimIndex],
					 StartIonColor);
		IonElementPtr->colorCycleIndex = 0;
		IonElementPtr->current.location = ElementPtr->current.location;
		IonElementPtr->current.location.x +=
			(int16_t)Cosine(angle, DisplayToWorld(ResScale(2)));
		IonElementPtr->current.location.y +=
			(int16_t)Sine(angle, DisplayToWorld(ResScale(2)));
		IonElementPtr->death_func = ion_preprocess;

		SetElementStarShip(IonElementPtr, StarShipPtr);

		{
			/* normally done during preprocess, but because
			 * object is being inserted at head rather than
			 * appended after tail it may never get preprocessed.
			 */
			IonElementPtr->next = IonElementPtr->current;
			--IonElementPtr->life_span;
			IonElementPtr->state_flags |= PreProcessFlag;
		}

		UnlockElement(hIonElement);
	}
}

static void marine_preprocess(Element* ElementPtr)
{
	Element* ShipPtr {};
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	LockElement(StarShipPtr->hShip, &ShipPtr);
	if (ShipPtr->crew_level == 0
		&& ShipPtr->life_span == 1
		&& (ShipPtr->state_flags & (FiniteLife | Nonsolid)) == (FiniteLife | Nonsolid))
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= Disappearing | Nonsolid;
		ElementPtr->turn_wait = 1;
	}
	UnlockElement(StarShipPtr->hShip);

	if (uqm::lowNibble(ElementPtr->turn_wait))
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		uint16_t pfacing = 0;
		int32_t delta_x {};
		int32_t delta_y {};
		int32_t delta_facing {};
		ElementHandle hObject {};
		ElementHandle hNextObject {};
		Element* ObjectPtr {};

		// XXX: thrust_wait is abused to store marine speed and
		//   gravity well flags
		ElementPtr->thrust_wait &= ~(ShipInGravityWell >> 6);

		ElementHandle hTarget {0};
		for (hObject = GetHeadElement();
			 hObject; hObject = hNextObject)
		{
			LockElement(hObject, &ObjectPtr);
			hNextObject = GetSuccElement(ObjectPtr);
			if (GravityMass(ObjectPtr->mass_points))
			{
				delta_x = ObjectPtr->current.location.x
						- ElementPtr->current.location.x;
				delta_x = WrapDeltaX(delta_x);

				delta_y = ObjectPtr->current.location.y
						- ElementPtr->current.location.y;
				delta_y = WrapDeltaY(delta_y);
				if ((long)delta_x * delta_x + (long)delta_y * delta_y <= (long)(DisplayToWorld(GravityThreshold())
																				* DisplayToWorld(GravityThreshold())))
				{
					pfacing = AngleToFacing(ARCTAN(delta_x, delta_y));
					delta_facing = NormalizeFacing(
						pfacing - AngleToFacing(GetVelocityTravelAngle(&ElementPtr->velocity))
						+ AngleToFacing(Octant));
					if (delta_facing <= AngleToFacing(Quadrant))
					{
						hTarget = hObject;
						hNextObject = 0;
					}

					ElementPtr->thrust_wait |= (ShipInGravityWell >> 6);
				}
			}
			else if ((ObjectPtr->state_flags & PlayerShip)
					 && ObjectPtr->crew_level
					 && !ObjectCloaked(ObjectPtr))
			{
				if (!elementsOfSamePlayer(ObjectPtr, ElementPtr))
				{
					if (ElementPtr->state_flags & IgnoreSimilar)
					{
						hTarget = hObject;
					}
				}
				else if (hTarget == 0)
				{
					hTarget = hObject;
				}
			}
			UnlockElement(hObject);
		}

		uint16_t facing {uqm::highNibble(ElementPtr->turn_wait)};
		if (hTarget == 0)
		{
			delta_facing = -1;
		}
		else
		{
			LockElement(hTarget, &ObjectPtr);
			delta_x = ObjectPtr->current.location.x
					- ElementPtr->current.location.x;
			delta_x = WrapDeltaX(delta_x);
			delta_y = ObjectPtr->current.location.y
					- ElementPtr->current.location.y;
			delta_y = WrapDeltaY(delta_y);
			if (GravityMass(ObjectPtr->mass_points))
			{
				delta_facing = NormalizeFacing(pfacing - facing
											   + AngleToFacing(Octant));

				if (delta_facing > AngleToFacing(Quadrant))
				{
					delta_facing = 0;
				}
				else
				{
					if (delta_facing == AngleToFacing(Octant))
					{
						facing += (((int16_t)uqm::random().next() & 1) << 1) - 1;
					}
					else if (delta_facing < AngleToFacing(Octant))
					{
						++facing;
					}
					else
					{
						--facing;
					}
				}
			}
			else
			{
				uint32_t num_frames {};
				VelocityDesc ShipVelocity;

				if (elementsOfSamePlayer(ObjectPtr, ElementPtr)
					&& (ElementPtr->state_flags & IgnoreSimilar))
				{
					ElementPtr->next.image.frame = SetAbsFrameIndex(
						StarShipPtr->RaceDescPtr->ship_data.special[0],
						21);
					ElementPtr->state_flags &= ~IgnoreSimilar;
					ElementPtr->state_flags |= Changing;
				}

				num_frames = ResDescale(WorldToTurn(
					square_root((long)delta_x * delta_x
								+ (long)delta_y * delta_y)));

				if (num_frames == 0)
				{
					num_frames = 1;
				}

				ShipVelocity = ObjectPtr->velocity;
				GetNextVelocityComponentsSdword(&ShipVelocity,
												&delta_x, &delta_y, num_frames);

				delta_x = ((int32_t)ObjectPtr->current.location.x + delta_x)
						- (int32_t)ElementPtr->current.location.x;
				delta_y = ((int32_t)ObjectPtr->current.location.y + delta_y)
						- (int32_t)ElementPtr->current.location.y;

				delta_facing = NormalizeFacing(
					AngleToFacing(ARCTAN(delta_x, delta_y)) - facing);

				if (delta_facing > 0)
				{
					if (delta_facing == AngleToFacing(HalfCircle))
					{
						facing += (((uint8_t)uqm::random().next() & 1) << 1) - 1;
					}
					else if (delta_facing < AngleToFacing(HalfCircle))
					{
						++facing;
					}
					else
					{
						--facing;
					}
				}
			}
			UnlockElement(hTarget);
		}

		ElementPtr->turn_wait = uqm::makeByte(0, NormalizeFacing(facing));
		if (delta_facing == 0
			|| ((ElementPtr->thrust_wait & (ShipBeyondMaxSpeed >> 6))
				&& !(ElementPtr->thrust_wait & (ShipInGravityWell >> 6))))
		{
			Starship* StarShipPtr {};

			GetElementStarShip(ElementPtr, &StarShipPtr);

			// XXX: Hack: abusing the primary Starship struct in order
			//   to call inertial_thrust() for a marine
			const uint16_t OldFacing {StarShipPtr->ShipFacing};
			const StatusFlags OldStatus {StarShipPtr->cur_status_flags};
			const uint16_t OldIncrement {StarShipPtr->RaceDescPtr->characteristics.thrust_increment};
			const uint16_t OldThrust {StarShipPtr->RaceDescPtr->characteristics.max_thrust};

			StarShipPtr->ShipFacing = facing;
			// XXX: thrust_wait is abused to store marine speed and
			//   gravity well flags
			StarShipPtr->cur_status_flags = ElementPtr->thrust_wait << 6;
			StarShipPtr->RaceDescPtr->characteristics.thrust_increment =
				MarineThrustIncrement();
			StarShipPtr->RaceDescPtr->characteristics.max_thrust =
				MarineMaxThrust();

			const StatusFlags thrust_status {inertial_thrust(ElementPtr)};

			StarShipPtr->RaceDescPtr->characteristics.max_thrust = OldThrust;
			StarShipPtr->RaceDescPtr->characteristics.thrust_increment =
				OldIncrement;
			StarShipPtr->cur_status_flags = OldStatus;
			StarShipPtr->ShipFacing = OldFacing;

			if ((ElementPtr->thrust_wait & (ShipInGravityWell >> 6))
				|| delta_facing
				|| !(thrust_status
					 & (ShipAtMaxSpeed | ShipBeyondMaxSpeed)))
			{
				spawn_marine_ion_trail(ElementPtr, StarShipPtr, facing);
			}

			// XXX: thrust_wait is abused to store marine speed and
			//   gravity well flags
			ElementPtr->thrust_wait = (uint8_t)(thrust_status >> 6);
		}
	}
}

void marine_collision(Element* ElementPtr0, GfxPoint* pPt0,
					  Element* ElementPtr1, GfxPoint* pPt1)
{
	if (ElementPtr0->life_span
		&& !(ElementPtr0->state_flags & (Nonsolid | Collision))
		&& !(ElementPtr1->state_flags & FiniteLife))
	{
		if (!elementsOfSamePlayer(ElementPtr0, ElementPtr1))
		{
			ElementPtr0->turn_wait =
				uqm::makeByte(5, uqm::highNibble(ElementPtr0->turn_wait));
			ElementPtr0->thrust_wait &=
				~((ShipAtMaxSpeed | ShipBeyondMaxSpeed) >> 6);
			ElementPtr0->state_flags |= Collision;
		}

		if (GravityMass(ElementPtr1->mass_points))
		{
			ElementPtr0->state_flags |= Nonsolid | FiniteLife;
			ElementPtr0->hit_points = 0;
			ElementPtr0->life_span = 0;
		}
		else if ((ElementPtr1->state_flags & PlayerShip)
				 && ((ElementPtr1->state_flags & FiniteLife)
					 || ElementPtr1->life_span == NormalLife))
		{
			ElementPtr1->state_flags &= ~Collision;

			if (!(ElementPtr0->state_flags & Collision))
			{
				DeltaCrew(ElementPtr1, 1);

				ElementPtr0->state_flags |=
					Disappearing | Nonsolid | FiniteLife;
				ElementPtr0->hit_points = 0;
				ElementPtr0->life_span = 0;
			}
			else if ((ElementPtr0->state_flags & IgnoreSimilar)
					 && ElementPtr1->crew_level
#ifdef NEVER
					 && (uint8_t)uqm::random().next() <= (0x0100 / 3)
#endif /* NEVER */
			)
			{
				GfxStamp s;
				Starship* StarShipPtr {};

				GetElementStarShip(ElementPtr0, &StarShipPtr);

				if (!antiCheat(ElementPtr1, false, uqm::GodModeFlags::NoDamage))
				{
					if (!DeltaCrew(ElementPtr1, -1))
					{
						ElementPtr1->life_span = 0;
					}
				}

				ElementPtr0->turn_wait = count_marines(StarShipPtr, true);
				ElementPtr0->thrust_wait = MarineWait;
				ElementPtr0->next.image.frame = SetAbsFrameIndex(
					ElementPtr0->next.image.farray[0],
					22 + ElementPtr0->turn_wait);
				ElementPtr0->state_flags |= Nonsolid;
				ElementPtr0->state_flags &= ~CrewObject;
				SetPrimType(&DisplayArray[ElementPtr0->PrimIndex], NoPrim);
				ElementPtr0->preprocess_func = intruder_preprocess;
				s.origin.x = ResScale(16 + (ElementPtr0->turn_wait & 3) * 9);
				s.origin.y = ResScale(14 + (ElementPtr0->turn_wait >> 2) * 11);
				s.frame = ElementPtr0->next.image.frame;
				ModifySilhouette(ElementPtr1, &s, 0);

				ProcessSound(GetSound(
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
							 ElementPtr1);
			}

			ElementPtr0->state_flags &= ~Collision;
		}
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
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

static void turret_postprocess(Element* ElementPtr)
{
	if (ElementPtr->life_span == 0)
	{
		Starship* StarShipPtr {};

		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);

		GetElementStarShip(ElementPtr, &StarShipPtr);
		if (StarShipPtr->hShip)
		{
			uint16_t facing {};
			ElementHandle hSpaceMarine {};
			Element* ShipPtr {};

			LockElement(StarShipPtr->hShip, &ShipPtr);
			ElementHandle hTurret {AllocElement()};
			if (hTurret)
			{
				Element* TurretPtr {};

				LockElement(hTurret, &TurretPtr);
				TurretPtr->playerNr = ElementPtr->playerNr;
				TurretPtr->state_flags = FiniteLife | Nonsolid
									   | IgnoreSimilar | Changing | PreProcessFlag
									   | PostProcessFlag;
				TurretPtr->life_span = 1;
				TurretPtr->current.image = ElementPtr->current.image;
				TurretPtr->current.location = ShipPtr->next.location;
				TurretPtr->turn_wait = ElementPtr->turn_wait;
				TurretPtr->thrust_wait = ElementPtr->thrust_wait;

				if (TurretPtr->turn_wait)
				{
					--TurretPtr->turn_wait;
				}
				else if ((StarShipPtr->cur_status_flags & SpecialFlag)
						 && (StarShipPtr->cur_status_flags & (LeftFlag | RightFlag)))
				{
					if (StarShipPtr->cur_status_flags & RightFlag)
					{
						++TurretPtr->thrust_wait;
					}
					else
					{
						--TurretPtr->thrust_wait;
					}

					TurretPtr->turn_wait = TurretWait;
				}
				facing = NormalizeFacing(StarShipPtr->ShipFacing
										 + TurretPtr->thrust_wait);
				StarShipPtr->RaceDescPtr->ship_info.ship_flags &=
					~(FiresFore | FiresRight | FiresAft | FiresLeft);
				StarShipPtr->RaceDescPtr->ship_info.ship_flags |= FiresFore
															   << (NormalizeFacing(facing + AngleToFacing(Octant))
																   / AngleToFacing(Quadrant));
				TurretPtr->current.image.frame = SetAbsFrameIndex(
					TurretPtr->current.image.frame, facing);
				facing = FacingToAngle(facing);
				if (StarShipPtr->cur_status_flags & WeaponFlag)
				{
					ElementHandle hTurretEffect {};
					Element* TurretEffectPtr {};

					LockElement(GetTailElement(), &TurretEffectPtr);
					if (TurretEffectPtr != ElementPtr
						&& elementsOfSamePlayer(TurretEffectPtr, ElementPtr)
						&& (TurretEffectPtr->state_flags & Appearing)
						&& GetPrimType(&DisplayArray[TurretEffectPtr->PrimIndex]) == PrimitiveType::Stamp
						&& (hTurretEffect = AllocElement()))
					{
						TurretPtr->current.location.x -=
							Cosine(facing, DisplayToWorld(ResScale(2)));
						TurretPtr->current.location.y -=
							Sine(facing, DisplayToWorld(ResScale(2)));

						LockElement(hTurretEffect, &TurretEffectPtr);
						TurretEffectPtr->playerNr = ElementPtr->playerNr;
						TurretEffectPtr->state_flags = FiniteLife
													 | Nonsolid | IgnoreSimilar | Appearing;
						TurretEffectPtr->life_span = 4;

						TurretEffectPtr->current.location.x =
							TurretPtr->current.location.x
							+ Cosine(facing,
									 DisplayToWorld(TurretOffset()));
						TurretEffectPtr->current.location.y =
							TurretPtr->current.location.y
							+ Sine(facing,
								   DisplayToWorld(TurretOffset()));
						TurretEffectPtr->current.image.farray =
							StarShipPtr->RaceDescPtr->ship_data.special;
						TurretEffectPtr->current.image.frame =
							SetAbsFrameIndex(
								StarShipPtr->RaceDescPtr->ship_data.special[0],
								AngleToFacing(FullCircle));

						TurretEffectPtr->preprocess_func = animate;

						SetElementStarShip(TurretEffectPtr, StarShipPtr);

						SetPrimType(&DisplayArray[TurretEffectPtr->PrimIndex], PrimitiveType::Stamp);

						UnlockElement(hTurretEffect);
						PutElement(hTurretEffect);
					}
					UnlockElement(GetTailElement());
				}
				TurretPtr->next = TurretPtr->current;

				SetPrimType(&DisplayArray[TurretPtr->PrimIndex],
							GetPrimType(&DisplayArray[ShipPtr->PrimIndex]));
				SetPrimColor(&DisplayArray[TurretPtr->PrimIndex],
							 GetPrimColor(&DisplayArray[ShipPtr->PrimIndex]));

				TurretPtr->postprocess_func = ElementPtr->postprocess_func;

				SetElementStarShip(TurretPtr, StarShipPtr);

				UnlockElement(hTurret);
				InsertElement(hTurret, GetSuccElement(ElementPtr));
			}

			if (StarShipPtr->special_counter == 0
				&& (StarShipPtr->cur_status_flags & SpecialFlag)
				&& (StarShipPtr->cur_status_flags & WeaponFlag)
				&& ShipPtr->crew_level > 1
				&& count_marines(StarShipPtr, false) < MaxMarines
				&& TrackShip(ShipPtr, &facing) >= 0
				&& (hSpaceMarine = AllocElement()))
			{
				Element* SpaceMarinePtr {};

				LockElement(hSpaceMarine, &SpaceMarinePtr);
				SpaceMarinePtr->playerNr = ElementPtr->playerNr;
				SpaceMarinePtr->state_flags = IgnoreSimilar | Appearing
											| CrewObject;
				SpaceMarinePtr->life_span = NormalLife;
				SpaceMarinePtr->hit_points = MarineHitPoints;
				SpaceMarinePtr->mass_points = MarineMassPoints;

				facing = FacingToAngle(StarShipPtr->ShipFacing);
				SpaceMarinePtr->current.location.x =
					ShipPtr->current.location.x
					- Cosine(facing, DisplayToWorld(TurretOffset() << ((resolutionFactor + 1) / 2)));
				SpaceMarinePtr->current.location.y =
					ShipPtr->current.location.y
					- Sine(facing, DisplayToWorld(TurretOffset() << ((resolutionFactor + 1) / 2)));
				SpaceMarinePtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
				SpaceMarinePtr->current.image.frame = SetAbsFrameIndex(
					StarShipPtr->RaceDescPtr->ship_data.special[0], 20);

				SpaceMarinePtr->turn_wait =
					uqm::makeByte(0, NormalizeFacing(
										 AngleToFacing(facing + HalfCircle)));
				SpaceMarinePtr->preprocess_func = marine_preprocess;
				SpaceMarinePtr->collision_func = marine_collision;

				SetElementStarShip(SpaceMarinePtr, StarShipPtr);

				SetPrimType(&DisplayArray[SpaceMarinePtr->PrimIndex], PrimitiveType::Stamp);

				UnlockElement(hSpaceMarine);
				PutElement(hSpaceMarine);

				if (!antiCheat(ElementPtr, false, uqm::GodModeFlags::NoDamage))
				{
					DeltaCrew(ShipPtr, -1);
				}

				ProcessSound(GetSound(
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
							 SpaceMarinePtr);

				StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
			}

			UnlockElement(StarShipPtr->hShip);
		}
	}
}

static void orz_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (!(ElementPtr->state_flags & Appearing))
	{
		if (((StarShipPtr->cur_status_flags
			  | StarShipPtr->old_status_flags)
			 & SpecialFlag)
			&& (StarShipPtr->cur_status_flags & (LeftFlag | RightFlag))
			&& ElementPtr->turn_wait == 0)
		{
			++ElementPtr->turn_wait;
		}

		if ((StarShipPtr->cur_status_flags & SpecialFlag)
			&& (StarShipPtr->cur_status_flags & WeaponFlag)
			&& StarShipPtr->weapon_counter == 0)
		{
			++StarShipPtr->weapon_counter;
		}
	}
	else
	{
		ElementHandle hTurret {AllocElement()};
		if (hTurret)
		{
			Element* TurretPtr {};

			LockElement(hTurret, &TurretPtr);
			TurretPtr->playerNr = ElementPtr->playerNr;
			TurretPtr->state_flags = FiniteLife | Nonsolid | IgnoreSimilar;
			TurretPtr->life_span = 1;
			TurretPtr->current.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.special;
			TurretPtr->current.image.frame = SetAbsFrameIndex(
				StarShipPtr->RaceDescPtr->ship_data.special[0],
				StarShipPtr->ShipFacing);

			TurretPtr->postprocess_func = turret_postprocess;

			SetElementStarShip(TurretPtr, StarShipPtr);

			UnlockElement(hTurret);
			InsertElement(hTurret, GetSuccElement(ElementPtr));
		}
	}
}

RaceDesc* init_orz(void)
{
	if (IsHd())
	{
		orz_desc.characteristics.max_thrust = ResScale(MaxThrust);
		orz_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		orz_desc.cyborg_control.WeaponRange = MissileSpeedHd() * MissileLife;
	}
	else
	{
		orz_desc.characteristics.max_thrust = MaxThrust;
		orz_desc.characteristics.thrust_increment = ThrustIncrement;
		orz_desc.cyborg_control.WeaponRange = MissileSpeed * MissileLife;
	}

	orz_desc.preprocess_func = orz_preprocess;
	orz_desc.init_weapon_func = initialize_turret_missile;
	orz_desc.cyborg_control.intelligence_func = orz_intelligence;

	RaceDesc* RaceDescPtr {&orz_desc};

	return (RaceDescPtr);
}

} // namespace uqm
