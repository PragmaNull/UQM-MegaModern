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

#include "chmmr.h"

#include "../../setup.h"
#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/colors.h"
#include "uqm/globdata.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {1};
static constexpr auto MaxThrust {35};
static constexpr auto ThrustIncrement {7};
static constexpr auto ThrustWait {5};
static constexpr auto TurnWait {3};
static constexpr auto ShipMass {10};

// Laser
static constexpr auto WeaponEnergyCost {2};
static constexpr auto WeaponWait {0};
static auto ChmmrOffset() { return ResScale(18); }
static auto LaserRange() { return DisplayToWorld(ResScale(150)); }
static constexpr auto NumCycles {4};

// Tractor Beam
static constexpr auto SpecialEnergyCost {1};
static constexpr auto SpecialWait {0};
static constexpr auto NumShadows {5};

// Satellites
static constexpr auto NumSatellites {3};
static auto SatelliteOffset() { return DisplayToWorld(ResScale(64)); }
static constexpr auto SatelliteHitpoints {10};
static constexpr auto SatelliteMass {10};
static auto DefenseRange() { return (uint16_t)ResScale(64); }
static constexpr auto DefenseWait {2};

static RaceDesc chmmr_desc =
	{
		{/* ShipInfoDesc */
		 "avatar",
		 ShipFlags::FiresFore | ShipFlags::ImmediateWeapon | ShipFlags::SeekingSpecial | ShipFlags::PointDefense,
		 30, /* Super Melee cost */
		 MaxCrewSize, MaxCrewSize,
		 MaxEnergySize, MaxEnergySize,
		 ChmmrRaceStrings,
		 ChmmrIconMaskPmapAnim,
		 ChmmrMiconMaskPmapAnim,
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
			 ChmmrBigMaskPmapAnim,
			 ChmmrMedMaskPmapAnim,
			 ChmmrSmlMaskPmapAnim,
		 },
		 {
			 MuzzleBigMaskPmapAnim,
			 MuzzleMedMaskPmapAnim,
			 MuzzleSmlMaskPmapAnim,
		 },
		 {
			 SatelliteBigMaskPmapAnim,
			 SatelliteMedMaskPmapAnim,
			 SatelliteSmlMaskPmapAnim,
		 },
		 {ChmmrCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 ChmmrVictorySong,
		 ChmmrShipSounds,
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

static void laser_death(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	StarShipPtr->special_counter = ElementPtr->turn_wait;

	if (StarShipPtr->hShip)
	{
		int32_t dx {};
		int32_t dy {};
		long dist {};
		ElementHandle hIonSpots {};
		Element* ShipPtr {};

		LockElement(StarShipPtr->hShip, &ShipPtr);

		dx = ElementPtr->current.location.x
		   - ShipPtr->current.location.x;
		dy = ElementPtr->current.location.y
		   - ShipPtr->current.location.y;
		if (((uint8_t)uqm::random().next() & 0x07)
			&& (dist = (long)dx * dx + (long)dy * dy) >= (long)DisplayToWorld(ChmmrOffset() + ResScale(10))
															 * DisplayToWorld(ChmmrOffset() + ResScale(10))
			&& (hIonSpots = AllocElement()))
		{
			uint16_t magnitude {};
			Element* IonSpotsPtr {};

			LockElement(hIonSpots, &IonSpotsPtr);
			IonSpotsPtr->playerNr = ElementPtr->playerNr;
			IonSpotsPtr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid
									 | ElementFlags::IgnoreSimilar | ElementFlags::Appearing;
			IonSpotsPtr->turn_wait = IonSpotsPtr->next_turn = 0;
			IonSpotsPtr->life_span = 9;

			const uint16_t angle {ARCTAN(dx, dy)};
			magnitude = ((uint16_t)uqm::random().next()
						 % ((square_root(dist) + 1)
							- DisplayToWorld(ChmmrOffset() + ResScale(10))))
					  + DisplayToWorld(ChmmrOffset() + ResScale(10));
			IonSpotsPtr->current.location.x =
				ShipPtr->current.location.x
				+ Cosine(angle, magnitude);
			IonSpotsPtr->current.location.y =
				ShipPtr->current.location.y
				+ Sine(angle, magnitude);
			IonSpotsPtr->current.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.weapon;
			IonSpotsPtr->current.image.frame = SetAbsFrameIndex(
				StarShipPtr->RaceDescPtr->ship_data.weapon[0],
				AngleToFacing(FullCircle) << 1);

			IonSpotsPtr->preprocess_func = animate;

			SetElementStarShip(IonSpotsPtr, StarShipPtr);

			SetPrimType(&DisplayArray[IonSpotsPtr->PrimIndex], PrimitiveType::Stamp);

			UnlockElement(hIonSpots);
			PutElement(hIonSpots);
		}

		UnlockElement(StarShipPtr->hShip);
	}
}

static uint16_t initialize_megawatt_laser(Element* ShipPtr, ElementHandle LaserArray[])
{
	GfxRect r;
	Starship* StarShipPtr {};
	LaserDesc LaserBlock;
	const static Color cycle_array[] =
		{
			BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
			BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7F),
			BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7B),
			BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7F),
		};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	LaserBlock.face = StarShipPtr->ShipFacing;
	GetFrameRect(SetAbsFrameIndex(
					 StarShipPtr->RaceDescPtr->ship_data.weapon[0], LaserBlock.face),
				 &r);
	LaserBlock.cx = DisplayAlign(ShipPtr->next.location.x)
				  + DisplayToWorld(r.corner.x);
	LaserBlock.cy = DisplayAlign(ShipPtr->next.location.y)
				  + DisplayToWorld(r.corner.y);
	LaserBlock.ex = Cosine(FacingToAngle(LaserBlock.face), LaserRange());
	LaserBlock.ey = Sine(FacingToAngle(LaserBlock.face), LaserRange());
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = ElementFlags::IgnoreSimilar;
	LaserBlock.pixoffs = 0;
	LaserBlock.color = cycle_array[StarShipPtr->special_counter];
	LaserArray[0] = initialize_laser(&LaserBlock);

	if (LaserArray[0])
	{
		Element* LaserPtr {};

		LockElement(LaserArray[0], &LaserPtr);

		LaserPtr->mass_points = 2;
		LaserPtr->death_func = laser_death;
		LaserPtr->turn_wait = (uint8_t)((StarShipPtr->special_counter + 1)
										% NumCycles);

		UnlockElement(LaserArray[0]);
	}

	return (1);
}

static void chmmr_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							   uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state &= ~StatusFlags::Special;
	const ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	if (StarShipPtr->special_counter == 0
		&& lpEvalDesc->ObjectPtr
		&& !(testFlag(StarShipPtr->ship_input_state, StatusFlags::Weapon))
		&& lpEvalDesc->which_turn > 12
		&& NormalizeAngle(
			   GetVelocityTravelAngle(&ShipPtr->velocity)
			   - (GetVelocityTravelAngle(&lpEvalDesc->ObjectPtr->velocity)
				  + HalfCircle)
			   + Quadrant)
			   > HalfCircle)
	{
		StarShipPtr->ship_input_state |= StatusFlags::Special;
	}
}

static void chmmr_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);

	if (testFlag(StarShipPtr->cur_status_flags, StatusFlags::Weapon))
	{
		ElementHandle hMuzzleFlash {};
		Element* MuzzleFlashPtr {};

		LockElement(GetTailElement(), &MuzzleFlashPtr);
		if (MuzzleFlashPtr != ElementPtr
			&& elementsOfSamePlayer(MuzzleFlashPtr, ElementPtr)
			&& (testFlag(MuzzleFlashPtr->state_flags, ElementFlags::Appearing))
			&& GetPrimType(&DisplayArray[MuzzleFlashPtr->PrimIndex]) == PrimitiveType::Line
			&& !(StarShipPtr->special_counter & 1)
			&& (hMuzzleFlash = AllocElement()))
		{
			LockElement(hMuzzleFlash, &MuzzleFlashPtr);
			MuzzleFlashPtr->playerNr = ElementPtr->playerNr;
			MuzzleFlashPtr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid
										| ElementFlags::IgnoreSimilar | ElementFlags::Appearing;
			MuzzleFlashPtr->life_span = 1;

			MuzzleFlashPtr->current = ElementPtr->next;
			MuzzleFlashPtr->current.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.weapon;
			MuzzleFlashPtr->current.image.frame = SetAbsFrameIndex(
				StarShipPtr->RaceDescPtr->ship_data.weapon[0],
				StarShipPtr->ShipFacing + AngleToFacing(FullCircle));

			SetElementStarShip(MuzzleFlashPtr, StarShipPtr);

			SetPrimType(&DisplayArray[MuzzleFlashPtr->PrimIndex], PrimitiveType::Stamp);

			UnlockElement(hMuzzleFlash);
			PutElement(hMuzzleFlash);
		}
		UnlockElement(GetTailElement());
	}

	if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		Element* ShipElementPtr {};

		LockElement(ElementPtr->hTarget, &ShipElementPtr);

		ProcessSound(GetSound(
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ShipElementPtr);

		UnlockElement(ElementPtr->hTarget);

		uint16_t facing {0};
		if (TrackShip(ElementPtr, &facing) >= 0)
		{
			Element* ShipElementPtr {};

			LockElement(ElementPtr->hTarget, &ShipElementPtr);
			if (!GravityMass(ShipElementPtr->mass_points + 1))
			{
				int16_t dx {};
				int16_t dy {};
				uint16_t angle {};
				uint16_t magnitude {};
				Starship* EnemyStarShipPtr {};
				const static int16_t shadow_offs[] =
					{
						DisplayToWorld(8),
						DisplayToWorld(8 + 9),
						DisplayToWorld(8 + 9 + 11),
						DisplayToWorld(8 + 9 + 11 + 14),
						DisplayToWorld(8 + 9 + 11 + 14 + 18),
					};
				const static Color color_tab[] =
					{
						BuildColor(MakeRgb15(0x00, 0x00, 0x10), 0x53),
						BuildColor(MakeRgb15(0x00, 0x00, 0x0E), 0x54),
						BuildColor(MakeRgb15(0x00, 0x00, 0x0C), 0x55),
						BuildColor(MakeRgb15(0x00, 0x00, 0x09), 0x56),
						BuildColor(MakeRgb15(0x00, 0x00, 0x07), 0x57),
					};
				uint32_t max_speed {};

				// calculate tractor beam effect
				angle = FacingToAngle(StarShipPtr->ShipFacing);
				dx = (ElementPtr->next.location.x
					  + Cosine(angle, (LaserRange() / 3)
										  + DisplayToWorld(ChmmrOffset())))
				   - ShipElementPtr->next.location.x;
				dy = (ElementPtr->next.location.y
					  + Sine(angle, (LaserRange() / 3)
										+ DisplayToWorld(ChmmrOffset())))
				   - ShipElementPtr->next.location.y;
				angle = ARCTAN(dx, dy);
				magnitude = WorldToVelocity(ResScale(12)) / ShipElementPtr->mass_points;
				DeltaVelocityComponents(&ShipElementPtr->velocity,
										Cosine(angle, magnitude), Sine(angle, magnitude));

				GetCurrentVelocityComponents(&ShipElementPtr->velocity,
											 &dx, &dy);
				GetElementStarShip(ShipElementPtr, &EnemyStarShipPtr);

				// set the effected ship's speed flags
				const uint32_t current_speed {VelocitySquared(dx, dy)};
				max_speed = VelocitySquared(WorldToVelocity(
												EnemyStarShipPtr->RaceDescPtr->characteristics.max_thrust),
											0);
				EnemyStarShipPtr->cur_status_flags &= ~(StatusFlags::ShipAtMaxSpeed
														| StatusFlags::ShipBeyondMaxSpeed);
				if (current_speed > max_speed)
				{
					EnemyStarShipPtr->cur_status_flags |= (StatusFlags::ShipAtMaxSpeed
														   | StatusFlags::ShipBeyondMaxSpeed);
				}
				else if (current_speed == max_speed)
				{
					EnemyStarShipPtr->cur_status_flags |= StatusFlags::ShipAtMaxSpeed;
				}

				// add tractor beam graphical effects
				for (int16_t i {0}; i < NumShadows; ++i)
				{
					ElementHandle hShadow {AllocElement()};
					if (hShadow)
					{
						Element* ShadowElementPtr {};

						LockElement(hShadow, &ShadowElementPtr);
						ShadowElementPtr->playerNr = ShipElementPtr->playerNr;
						ShadowElementPtr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid
													  | ElementFlags::IgnoreSimilar | ElementFlags::PostProcess;
						ShadowElementPtr->life_span = 1;

						ShadowElementPtr->current = ShipElementPtr->next;
						ShadowElementPtr->current.location.x +=
							Cosine(angle, ResScale(shadow_offs[i]));
						ShadowElementPtr->current.location.y +=
							Sine(angle, ResScale(shadow_offs[i]));
						ShadowElementPtr->next = ShadowElementPtr->current;

						SetElementStarShip(ShadowElementPtr, EnemyStarShipPtr);
						SetVelocityComponents(&ShadowElementPtr->velocity,
											  dx, dy);

						SetPrimType(&DisplayArray[ShadowElementPtr->PrimIndex], PrimitiveType::StampFill);
						SetPrimColor(&DisplayArray[ShadowElementPtr->PrimIndex], color_tab[i]);

						UnlockElement(hShadow);
						InsertElement(hShadow, GetHeadElement());
					}
				}
			}
			UnlockElement(ElementPtr->hTarget);
		}
	}

	StarShipPtr->special_counter = 0;
}

static void satellite_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	++ElementPtr->life_span;

	ElementPtr->next.image.frame =
		SetAbsFrameIndex(ElementPtr->current.image.frame,
						 (GetFrameIndex(ElementPtr->current.image.frame) + 1) & 7);
	ElementPtr->state_flags |= ElementFlags::Changing;

	ElementPtr->turn_wait = (uint8_t)NormalizeAngle(
		ElementPtr->turn_wait + 1);

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->hShip)
	{
		int32_t dx {};
		int32_t dy {};
		Element* ShipPtr {};

		StarShipPtr->RaceDescPtr->ship_info.ship_flags |= ShipFlags::PointDefense;

		LockElement(StarShipPtr->hShip, &ShipPtr);

		dx = (ShipPtr->next.location.x
			  + Cosine(ElementPtr->turn_wait, SatelliteOffset()))
		   - ElementPtr->current.location.x;
		dy = (ShipPtr->next.location.y
			  + Sine(ElementPtr->turn_wait, SatelliteOffset()))
		   - ElementPtr->current.location.y;
		dx = WrapDeltaX(dx);
		dy = WrapDeltaY(dy);
		if ((long)dx * dx + (long)dy * dy
			<= DisplayToWorld(ResScale(20L)) * DisplayToWorld(ResScale(20L)))
		{
			SetVelocityComponents(&ElementPtr->velocity,
								  WorldToVelocity(dx),
								  WorldToVelocity(dy));
		}
		else
		{
			const uint16_t angle {ARCTAN(dx, dy)};
			SetVelocityComponents(&ElementPtr->velocity,
								  Cosine(angle, WorldToVelocity(DisplayToWorld(ResScale(20)))),
								  Sine(angle, WorldToVelocity(DisplayToWorld(ResScale(20)))));
		}

		UnlockElement(StarShipPtr->hShip);
	}
}

static void spawn_point_defense(Element* ElementPtr)
{
	uint16_t best_dist {};
	Starship* StarShipPtr {};
	ElementHandle hObject {};
	ElementHandle hNextObject {};
	Element* ShipPtr {};
	Element* SattPtr {};
	Element* ObjectPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	ElementHandle hBestObject {0};
	best_dist = DefenseRange() + ResScale(1);
	uint8_t weakest {255};
	LockElement(StarShipPtr->hShip, &ShipPtr);
	LockElement(ElementPtr->hTarget, &SattPtr);
	for (hObject = GetPredElement(ElementPtr);
		 hObject; hObject = hNextObject)
	{
		LockElement(hObject, &ObjectPtr);
		hNextObject = GetPredElement(ObjectPtr);
		if (!elementsOfSamePlayer(ObjectPtr, ShipPtr)
			&& ObjectPtr->playerNr != NeutralPlayerNum
			&& CollisionPossible(ObjectPtr, ShipPtr)
			&& !ObjectCloaked(ObjectPtr))
		{
			int32_t delta_x {};
			int32_t delta_y {};
			uint16_t dist {};

			delta_x = ObjectPtr->next.location.x
					- SattPtr->next.location.x;
			delta_y = ObjectPtr->next.location.y
					- SattPtr->next.location.y;
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
			if ((uint16_t)delta_x <= DefenseRange() && (uint16_t)delta_y <= DefenseRange() && (dist = (uint16_t)delta_x * (uint16_t)delta_x + (uint16_t)delta_y * (uint16_t)delta_y) <= DefenseRange() * DefenseRange()
				&& (ObjectPtr->hit_points < weakest
					|| (ObjectPtr->hit_points == weakest
						&& dist < best_dist)))
			{
				hBestObject = hObject;
				best_dist = dist;
				weakest = ObjectPtr->hit_points;
			}
		}
		UnlockElement(hObject);
	}

	if (hBestObject)
	{
		LaserDesc LaserBlock;

		LockElement(hBestObject, &ObjectPtr);

		LaserBlock.cx = SattPtr->next.location.x;
		LaserBlock.cy = SattPtr->next.location.y;
		LaserBlock.face = 0;
		LaserBlock.ex = ObjectPtr->next.location.x
					  - SattPtr->next.location.x;
		LaserBlock.ey = ObjectPtr->next.location.y
					  - SattPtr->next.location.y;
		LaserBlock.sender = SattPtr->playerNr;
		LaserBlock.flags = ElementFlags::IgnoreSimilar;
		LaserBlock.pixoffs = 0;
		LaserBlock.color = BuildColor(MakeRgb15(0x00, 0x01, 0x1F), 0x4D);
		ElementHandle hPointDefense {initialize_laser(&LaserBlock)};
		if (hPointDefense)
		{
			Element* PDPtr {};

			LockElement(hPointDefense, &PDPtr);
			SetElementStarShip(PDPtr, StarShipPtr);
			PDPtr->hTarget = 0;
			UnlockElement(hPointDefense);

			PutElement(hPointDefense);

			SattPtr->thrust_wait = DefenseWait;
		}

		UnlockElement(hBestObject);
	}

	UnlockElement(ElementPtr->hTarget);
	UnlockElement(StarShipPtr->hShip);
}

static void satellite_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	if (ElementPtr->thrust_wait || ElementPtr->life_span == 0)
	{
		--ElementPtr->thrust_wait;
	}
	else
	{
		ElementHandle hDefense {AllocElement()};
		if (hDefense)
		{
			Element* DefensePtr {};

			PutElement(hDefense);

			LockElement(hDefense, &DefensePtr);
			DefensePtr->playerNr = ElementPtr->playerNr;
			DefensePtr->state_flags = ElementFlags::Appearing | ElementFlags::Nonsolid | ElementFlags::FiniteLife;

			{
				Element* SuccPtr {};

				LockElement(GetSuccElement(ElementPtr), &SuccPtr);
				DefensePtr->hTarget = GetPredElement(SuccPtr);
				UnlockElement(GetSuccElement(ElementPtr));

				DefensePtr->death_func = spawn_point_defense;
			}

			GetElementStarShip(ElementPtr, &StarShipPtr);
			SetElementStarShip(DefensePtr, StarShipPtr);

			UnlockElement(hDefense);
		}
	}
}

static void satellite_collision(Element* ElementPtr0, GfxPoint* pPt0,
								Element* ElementPtr1, GfxPoint* pPt1)
{
	(void)ElementPtr0; /* Satisfying compiler (unused parameter) */
	(void)pPt0;		   /* Satisfying compiler (unused parameter) */
	(void)ElementPtr1; /* Satisfying compiler (unused parameter) */
	(void)pPt1;		   /* Satisfying compiler (unused parameter) */
}

static void satellite_death(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~ShipFlags::PointDefense;

	ElementPtr->state_flags &= ~ElementFlags::Disappearing;
	ElementPtr->state_flags |= ElementFlags::Nonsolid | ElementFlags::FiniteLife | ElementFlags::Changing;
	ElementPtr->life_span = 4;
	ElementPtr->turn_wait = 1;
	ElementPtr->next_turn = 0;
	ElementPtr->next.image.frame =
		SetAbsFrameIndex(ElementPtr->current.image.frame, 8);

	ElementPtr->preprocess_func = animate;
	ElementPtr->death_func = nullptr;
	ElementPtr->postprocess_func = nullptr;
	ElementPtr->collision_func = nullptr;
}

static void spawn_satellites(Element* ElementPtr)
{
	uint16_t i {};
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->hShip)
	{
		LockElement(StarShipPtr->hShip, &ElementPtr);
		for (i = 0; i < NumSatellites; ++i)
		{
			ElementHandle hSatellite {AllocElement()};
			if (hSatellite)
			{
				uint16_t angle {};
				Element* SattPtr {};

				LockElement(hSatellite, &SattPtr);
				SattPtr->playerNr = ElementPtr->playerNr;
				SattPtr->state_flags = ElementFlags::IgnoreSimilar | ElementFlags::Appearing
									 | ElementFlags::FiniteLife;
				SattPtr->life_span = NormalLife + 1;
				SattPtr->hit_points = SatelliteHitpoints;
				SattPtr->mass_points = SatelliteMass;

				angle = (i * FullCircle + (NumSatellites >> 1))
					  / NumSatellites;
				SattPtr->turn_wait = (uint8_t)angle;
				SattPtr->current.location.x = ElementPtr->next.location.x
											+ Cosine(angle, SatelliteOffset());
				SattPtr->current.location.y = ElementPtr->next.location.y
											+ Sine(angle, SatelliteOffset());
				SattPtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
				SattPtr->current.image.frame = SetAbsFrameIndex(
					StarShipPtr->RaceDescPtr->ship_data.special[0],
					(uint16_t)uqm::random().next() & 0x07);

				SattPtr->preprocess_func = satellite_preprocess;
				SattPtr->postprocess_func = satellite_postprocess;
				SattPtr->death_func = satellite_death;
				SattPtr->collision_func = satellite_collision;

				SetElementStarShip(SattPtr, StarShipPtr);

				SetPrimType(&DisplayArray[SattPtr->PrimIndex], PrimitiveType::Stamp);

				UnlockElement(hSatellite);
				PutElement(hSatellite);
			}
		}
		UnlockElement(StarShipPtr->hShip);
	}
}

static void chmmr_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	ElementHandle hSatellite {AllocElement()};
	if (hSatellite)
	{
		Element* SattPtr {};
		Starship* StarShipPtr {};

		LockElement(hSatellite, &SattPtr);
		SattPtr->playerNr = ElementPtr->playerNr;
		SattPtr->state_flags = ElementFlags::FiniteLife | ElementFlags::Nonsolid | ElementFlags::IgnoreSimilar
							 | ElementFlags::Appearing;
		SattPtr->life_span = HyperjumpLife + 1;

		SattPtr->death_func = spawn_satellites;

		GetElementStarShip(ElementPtr, &StarShipPtr);
		SetElementStarShip(SattPtr, StarShipPtr);

		SetPrimType(&DisplayArray[SattPtr->PrimIndex], NoPrim);

		UnlockElement(hSatellite);
		PutElement(hSatellite);
	}

	StarShipPtr->RaceDescPtr->preprocess_func = 0;
}

RaceDesc* init_chmmr(void)
{
	if (IsHd())
	{
		chmmr_desc.characteristics.max_thrust = ResScale(MaxThrust);
		chmmr_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		chmmr_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd;
	}
	else
	{
		chmmr_desc.characteristics.max_thrust = MaxThrust;
		chmmr_desc.characteristics.thrust_increment = ThrustIncrement;
		chmmr_desc.cyborg_control.WeaponRange = CloseRangeWeapon;
	}

	chmmr_desc.preprocess_func = chmmr_preprocess;
	chmmr_desc.postprocess_func = chmmr_postprocess;
	chmmr_desc.init_weapon_func = initialize_megawatt_laser;
	chmmr_desc.cyborg_control.intelligence_func = chmmr_intelligence;

	RaceDesc* RaceDescPtr {&chmmr_desc};

	return (RaceDescPtr);
}

} // namespace uqm
