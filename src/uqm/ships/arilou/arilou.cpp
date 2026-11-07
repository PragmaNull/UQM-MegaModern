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

#include "arilou.h"

#include <math.h>

#include "../ship.h"
#include "core/log/log.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/globdata.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {6};
static constexpr auto MaxEnergy {20};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {6};
static constexpr auto MaxThrust {40};
static constexpr auto ThrustIncrement {MaxThrust};
static constexpr auto ThrustWait {0};
static constexpr auto TurnWait {0};
static constexpr auto ShipMass {1};

// Tracking Laser
static constexpr auto WeaponEnergyCost {2};
static constexpr auto WeaponWait {1};
static constexpr auto ArilouOffset {9};
static constexpr auto LaserRangeSd {DisplayToWorld(100 + ArilouOffset)};

// Teleporter
static constexpr auto SpecialEnergyCost {3};
static constexpr auto SpecialWait {2};
static constexpr auto HyperLife {5};

// HdResolutionFactor Values
static auto LaserRangeHd() { return ResScale(LaserRangeSd); }

static RaceDesc arilou_desc =
	{
		{/* ShipInfoDesc */
		 "skiff",
		 /* FiresFore | */ ShipFlags::ImmediateWeapon,
		 16, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 ArilouRaceStrings,
		 ArilouIconMaskPmapAnim,
		 ArilouMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			250 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				438,
				6372,
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
			 ArilouBigMaskPmapAnim,
			 ArilouMedMaskPmapAnim,
			 ArilouSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {
			 WarpBigMaskPmapAnim,
			 WarpMedMaskPmapAnim,
			 WarpSmlMaskPmapAnim,
		 },
		 {ArilouCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 ArilouVictorySong,
		 ArilouShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			LaserRangeSd >> 1,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

static uint16_t initialize_autoaim_laser(Element* ShipPtr, ElementHandle LaserArray[])
{
	uint16_t orig_facing {};
	int16_t delta_facing {};
	Starship* StarShipPtr {};
	LaserDesc LaserBlock;
	const uint16_t LaserRange = chooseIfHd(LaserRangeSd, LaserRangeHd());

	GetElementStarShip(ShipPtr, &StarShipPtr);
	LaserBlock.face = orig_facing = StarShipPtr->ShipFacing;
	if ((delta_facing = TrackShip(ShipPtr, &LaserBlock.face)) > 0)
	{
		LaserBlock.face = NormalizeFacing(orig_facing + delta_facing);
	}
	ShipPtr->hTarget = 0;

	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = Cosine(FacingToAngle(LaserBlock.face), LaserRange);
	LaserBlock.ey = Sine(FacingToAngle(LaserBlock.face), LaserRange);
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = ElementFlags::IgnoreSimilar;
	LaserBlock.pixoffs = ResScale(ArilouOffset);
	LaserBlock.color = BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E);
	LaserArray[0] = initialize_laser(&LaserBlock);

	return (1);
}

static void arilou_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state |= StatusFlags::Thrust;

	ObjectsOfConcern[*IntelObjectIndex::EnemyShip].MoveState = MovementState::Entice;
	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Special;

		const ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 6)
		{
			bool IsTrackingWeapon {};
			Starship* EnemyStarShipPtr {};

			GetElementStarShip(lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
			if (((testFlag(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags, ShipFlags::SeekingWeapon))
				 && lpEvalDesc->ObjectPtr->next.image.farray == EnemyStarShipPtr->RaceDescPtr->ship_data.weapon.data())
				|| ((testFlag(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags, ShipFlags::SeekingSpecial))
					&& lpEvalDesc->ObjectPtr->next.image.farray == EnemyStarShipPtr->RaceDescPtr->ship_data.special.data()))
			{
				IsTrackingWeapon = true;
			}
			else
			{
				IsTrackingWeapon = false;
			}

			if (((testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::PlayerShip))															/* means IMMEDIATE WeaponFlag */
				 || (IsTrackingWeapon && (lpEvalDesc->which_turn == 1 || (testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::CrewObject)))) /* FIGHTERS!!! */
				 || PlotIntercept(lpEvalDesc->ObjectPtr, ShipPtr, 3, 0))
				&& !(uqm::random().next() & 3))
			{
				StarShipPtr->ship_input_state &= ~(StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon);
				StarShipPtr->ship_input_state |= StatusFlags::Special;
			}
		}
	}
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <= SpecialEnergyCost << 1)
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Weapon;
	}
}

static void arilou_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (!(testFlag(ElementPtr->state_flags, ElementFlags::Nonsolid)))
	{
		if (ElementPtr->thrust_wait == 0)
		{
			ZeroVelocityComponents(&ElementPtr->velocity);
			StarShipPtr->cur_status_flags &= ~StatusFlags::ShipAtMaxSpeed;
		}

		if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
		{
			/* Special key is pressed; start teleport */
			ZeroVelocityComponents(&ElementPtr->velocity);
			StarShipPtr->cur_status_flags &=
				~(StatusFlags::ShipAtMaxSpeed | StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon);

			ElementPtr->state_flags |= ElementFlags::Nonsolid | ElementFlags::FiniteLife | ElementFlags::Changing;
			ElementPtr->life_span = HyperLife;

			ElementPtr->next.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.special.data();
			ElementPtr->next.image.frame =
				StarShipPtr->RaceDescPtr->ship_data.special[0];

			ProcessSound(GetSound(
							 /* HYPERJUMP */
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						 ElementPtr);
			StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
	else if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special.data())
	{
		uint16_t life_span {};

		StarShipPtr->cur_status_flags =
			(StarShipPtr->cur_status_flags
			 & ~(StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon | StatusFlags::Special))
			| (((StarShipPtr->old_status_flags & (StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon | StatusFlags::Special))));
		++StarShipPtr->weapon_counter;
		++StarShipPtr->special_counter;
		++StarShipPtr->energy_counter;
		++ElementPtr->turn_wait;
		++ElementPtr->thrust_wait;

		if ((life_span = ElementPtr->life_span) == NormalLife)
		{ /* Ending teleport */
			ElementPtr->state_flags &= ~(ElementFlags::Nonsolid | ElementFlags::FiniteLife);
			ElementPtr->state_flags |= ElementFlags::Appearing;
			ElementPtr->current.image.farray =
				ElementPtr->next.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.ship.data();
			ElementPtr->current.image.frame =
				ElementPtr->next.image.frame =
					SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.ship[0],
									 StarShipPtr->ShipFacing);
			InitIntersectStartPoint(ElementPtr);
		}
		else
		{ /* Teleporting in progress */
			--life_span;
			if (life_span != 2)
			{
				if (life_span < 2)
				{
					ElementPtr->next.image.frame =
						DecFrameIndex(ElementPtr->next.image.frame);
				}
				else
				{
					ElementPtr->next.image.frame =
						IncFrameIndex(ElementPtr->next.image.frame);
				}
			}
			else
			{ // JMS: Reduce the odds of teleporting into Sa-Matra.
				if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
				{
					int32_t dist = 0;
					int32_t dx {};
					int32_t dy {};

					do
					{
						ElementPtr->next.location.x =
							WrapX(DisplayAlignX(uqm::random().next()));
						ElementPtr->next.location.y =
							WrapY(DisplayAlignY(uqm::random().next()));

						dx = ((int32_t)ElementPtr->next.location.x - (LogSpaceWidth() >> 1));
						dy = ((int32_t)ElementPtr->next.location.y - (LogSpaceHeight() >> 1));

						dist = sqrt(dx * dx + dy * dy);
					} while (dist < (ResScale(2800)));
				}
				else
				{
					ElementPtr->next.location.x =
						WrapX(DisplayAlignX(uqm::random().next()));
					ElementPtr->next.location.y =
						WrapY(DisplayAlignY(uqm::random().next()));
				}
			}
		}

		ElementPtr->state_flags |= ElementFlags::Changing;
	}
}

RaceDesc* init_arilou(void)
{
	if (IsHd())
	{
		arilou_desc.characteristics.max_thrust = ResScale(MaxThrust);
		arilou_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		arilou_desc.cyborg_control.WeaponRange = LaserRangeHd() >> 1;
	}
	else
	{
		arilou_desc.characteristics.max_thrust = MaxThrust;
		arilou_desc.characteristics.thrust_increment = ThrustIncrement;
		arilou_desc.cyborg_control.WeaponRange = LaserRangeSd >> 1;
	}

	arilou_desc.preprocess_func = arilou_preprocess;
	arilou_desc.init_weapon_func = initialize_autoaim_laser;
	arilou_desc.cyborg_control.intelligence_func = arilou_intelligence;

	RaceDesc* RaceDescPtr {&arilou_desc};

	return (RaceDescPtr);
}

} // namespace uqm