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

#include "vux.h"

#include "../../setup.h"
#include "../../setupmenu.h"
#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/globdata.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// Core characteristics
static constexpr auto MaxCrew {20};
static constexpr auto MaxEnergy {40};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {8};
static constexpr auto MaxThrust {21};
static constexpr auto ThrustIncrement {7};
static constexpr auto ThrustWait {4};
static constexpr auto TurnWait {6};
static constexpr auto ShipMass {6};

// Laser
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {0};
static auto VuxOffset() { return chooseIfHd(12, 38); }
static auto LaserBase() { return ResScale(150); }
static auto LaserRange() { return DisplayToWorld(LaserBase() + VuxOffset()); }

// Limpet
static constexpr auto SpecialEnergyCost {2};
static constexpr auto SpecialWait {7};
static auto LimpetSpeed() { return ResScale(25); }
static auto LimpetOffset() { return ResScale(8); }
static constexpr auto LimpetLife {80};
static constexpr auto LimpetHits {1};
static constexpr auto LimpetDamage {0};
static auto MinThrustIncrement() { return DisplayToWorld(ResScale(1)); }

// Aggressive Entry
static auto WarpOffset() { return ResScale(46); }
/* How far outside of the laser range can the ship warp in. */
static auto MaxxEntryDist() { return DisplayToWorld((LaserBase() + VuxOffset() + WarpOffset()) << 1); }
static auto MaxyEntryDist() { return DisplayToWorld((LaserBase() + VuxOffset() + WarpOffset()) << 1); }
/* Originally, the warp distance was:
		 * DisplayToWorld (SpaceHeight() << 1)
		 * where SpaceHeight() = CanvasHeight
		 * But in reality this should be relative to the laser-range. */

static RaceDesc vux_desc =
	{
		{/* ShipInfoDesc */
		 "intruder",
		 ShipFlags::FiresFore | ShipFlags::SeekingSpecial | ShipFlags::ImmediateWeapon,
		 12, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 VuxRaceStrings,
		 VuxIconMaskPmapAnim,
		 VuxMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FleetStatus */
			900 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				4412,
				1558,
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
			 VuxBigMaskPmapAnim,
			 VuxMedMaskPmapAnim,
			 VuxSmlMaskPmapAnim,
		 },
		 {
			 SlimeMaskPmapAnim,
			 NullResource,
			 NullResource,
		 },
		 {
			 LimpetsBigMaskPmapAnim,
			 LimpetsMedMaskPmapAnim,
			 LimpetsSmlMaskPmapAnim,
		 },
		 {VuxCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 VuxVictorySong,
		 VuxShipSounds,
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


static void limpet_preprocess(Element* ElementPtr)
{
	uint16_t facing {};
	uint16_t orig_facing {};
	int16_t delta_facing {};

	facing = orig_facing = NormalizeFacing(AngleToFacing(
		GetVelocityTravelAngle(&ElementPtr->velocity)));
	if ((delta_facing = TrackShip(ElementPtr, &facing)) > 0)
	{
		facing = orig_facing + delta_facing;
		SetVelocityVector(&ElementPtr->velocity, LimpetSpeed(), facing);
	}
	ElementPtr->next.image.frame =
		IncFrameIndex(ElementPtr->next.image.frame);

	ElementPtr->state_flags |= ElementFlags::Changing;
}

static void limpet_collision(Element* ElementPtr0, GfxPoint* pPt0,
							 Element* ElementPtr1, GfxPoint* pPt1)
{
	if (testFlag(ElementPtr1->state_flags, ElementFlags::PlayerShip))
	{
		GfxStamp s;
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr1, &StarShipPtr);
		RaceDesc* RDPtr {StarShipPtr->RaceDescPtr};
		if (!antiCheat(ElementPtr1, false, uqm::GodModeFlags::NoDamage))
		{
			if (++RDPtr->characteristics.turn_wait == 0)
			{
				--RDPtr->characteristics.turn_wait;
			}
			if (++RDPtr->characteristics.thrust_wait == 0)
			{
				--RDPtr->characteristics.thrust_wait;
			}
			if (RDPtr->characteristics.thrust_increment <= MinThrustIncrement())
			{
				RDPtr->characteristics.max_thrust =
					RDPtr->characteristics.thrust_increment << 1;
			}
			else
			{
				uint16_t num_thrusts {};

				num_thrusts = RDPtr->characteristics.max_thrust / RDPtr->characteristics.thrust_increment;
				RDPtr->characteristics.thrust_increment -= ResScale(1);
				RDPtr->characteristics.max_thrust =
					RDPtr->characteristics.thrust_increment * num_thrusts;
			}
			RDPtr->cyborg_control.ManeuverabilityIndex = 0;

			GetElementStarShip(ElementPtr0, &StarShipPtr);
			ProcessSound(GetSound(
							 /* LIMPET_AFFIXES */
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
						 ElementPtr1);
			s.frame = SetAbsFrameIndex(
				StarShipPtr->RaceDescPtr->ship_data.weapon[0], (uint16_t)uqm::random().next());
			ModifySilhouette(ElementPtr1, &s, ModifyFlags::Image);
		}
		else
		{
			GetElementStarShip(ElementPtr0, &StarShipPtr);
			ProcessSound(GetSound(
							 /* LIMPET_AFFIXES */
							 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
						 ElementPtr1);
		}
	}

	ElementPtr0->hit_points = 0;
	ElementPtr0->life_span = 0;
	ElementPtr0->state_flags |= ElementFlags::Collision | ElementFlags::Disappearing;

	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static void spawn_limpets(Element* ElementPtr)
{
	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ElementPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special.data();
	MissileBlock.face = StarShipPtr->ShipFacing + HalfCircle;
	MissileBlock.index = 0;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
	MissileBlock.pixoffs = LimpetOffset();
	MissileBlock.speed = LimpetSpeed();
	MissileBlock.hit_points = LimpetHits;
	MissileBlock.damage = LimpetDamage;
	MissileBlock.life = LimpetLife;
	MissileBlock.preprocess_func = limpet_preprocess;
	MissileBlock.blast_offs = 0;

	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	ElementHandle Limpet {initialize_missile(&MissileBlock)};
	if (Limpet)
	{
		Element* LimpetPtr {};

		LockElement(Limpet, &LimpetPtr);
		LimpetPtr->collision_func = limpet_collision;
		SetElementStarShip(LimpetPtr, StarShipPtr);
		UnlockElement(Limpet);

		PutElement(Limpet);
	}
}

static uint16_t initialize_horrific_laser(Element* ShipPtr, ElementHandle LaserArray[])
{
	Starship* StarShipPtr {};
	LaserDesc LaserBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	LaserBlock.face = StarShipPtr->ShipFacing;
	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = Cosine(FacingToAngle(LaserBlock.face), LaserRange());
	LaserBlock.ey = Sine(FacingToAngle(LaserBlock.face), LaserRange());
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = ElementFlags::IgnoreSimilar;
	LaserBlock.pixoffs = VuxOffset();
	LaserBlock.color = BuildColor(MakeRgb15(0x0A, 0x1F, 0x0A), 0x0A);
	LaserArray[0] = initialize_laser(&LaserBlock);

	return (1);
}

static void vux_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							 uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
	lpEvalDesc->MoveState = MovementState::Pursue;
	if (ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr != 0
		&& ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].MoveState == MovementState::Entice)
	{
		if ((testFlag(ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr->state_flags, ElementFlags::FiniteLife))
			&& !(testFlag(ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].ObjectPtr->state_flags, ElementFlags::CrewObject)))
		{
			ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].MoveState = MovementState::Avoid;
		}
		else
		{
			ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon].MoveState = MovementState::Pursue;
		}
	}

	ship_intelligence(ShipPtr,
					  ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
		&& lpEvalDesc->ObjectPtr != 0
		&& lpEvalDesc->which_turn <= 12
		&& (testFlag(StarShipPtr->ship_input_state, (StatusFlags::Left | StatusFlags::Right)))
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= (uint8_t)(StarShipPtr->RaceDescPtr->ship_info.max_energy >> 1))
	{
		StarShipPtr->ship_input_state |= StatusFlags::Special;
	}
	else
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Special;
	}
}

static void vux_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		ProcessSound(GetSound(
						 /* LAUNCH_LIMPET */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);
		spawn_limpets(ElementPtr);

		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

static void vux_preprocess(Element* ElementPtr)
{
	if (testFlag(ElementPtr->state_flags, ElementFlags::Appearing))
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		uint16_t facing {StarShipPtr->ShipFacing};
		if ((uqm::activityKind(uqm::currentActivity()) != ActivityKind::InEncounter
			 || isDifficulty(uqm::Difficulty::Hard))
			&& TrackShip(ElementPtr, &facing) >= 0)
		{
			Element* OtherShipPtr {};
			int16_t SA_MATRA_EXTRA_DIST = 0;
			LockElement(ElementPtr->hTarget, &OtherShipPtr);

			// JMS: Not REALLY necessary as VUX can ordinarily never be played against Sa-Matra.
			// But handy in debugging as a single VUX limpet incapacitates Sa-Matra completely.
			if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
			{
				SA_MATRA_EXTRA_DIST += ResScale(1000);
			}

			do
			{
				// JMS_GFX: Circumventing overflows by using temp variables
				// instead of subtracting straight from the GfxPoint sized
				// ShipImagePtr->current.location.
				int32_t temp_x =
					((int32_t)OtherShipPtr->current.location.x - (MaxxEntryDist() >> 1)) + ((uint16_t)uqm::random().next() % MaxxEntryDist());
				int32_t temp_y =
					((int32_t)OtherShipPtr->current.location.y - (MaxyEntryDist() >> 1)) + ((uint16_t)uqm::random().next() % MaxyEntryDist());

				temp_x += temp_x > 0 ? SA_MATRA_EXTRA_DIST : -SA_MATRA_EXTRA_DIST;
				temp_y += temp_y > 0 ? SA_MATRA_EXTRA_DIST : -SA_MATRA_EXTRA_DIST;

				const int32_t dx {OtherShipPtr->current.location.x - temp_x};
				const int32_t dy {OtherShipPtr->current.location.y - temp_y};
				facing = NormalizeFacing(
					AngleToFacing(ARCTAN(dx, dy)));
				ElementPtr->current.image.frame =
					SetAbsFrameIndex(ElementPtr->current.image.frame,
									 facing);

				ElementPtr->current.location.x =
					WrapX(DisplayAlign(temp_x));
				ElementPtr->current.location.y =
					WrapY(DisplayAlign(temp_y));
			} while (CalculateGravity(ElementPtr)
					 || TimeSpaceMatterConflict(ElementPtr));

			UnlockElement(ElementPtr->hTarget);
			ElementPtr->hTarget = 0;

			ElementPtr->next = ElementPtr->current;
			InitIntersectStartPoint(ElementPtr);
			InitIntersectEndPoint(ElementPtr);
			InitIntersectFrame(ElementPtr);

			StarShipPtr->ShipFacing = facing;
		}

		StarShipPtr->RaceDescPtr->preprocess_func = 0;
	}
}

RaceDesc* init_vux(void)
{
	if (IsHd())
	{
		vux_desc.characteristics.max_thrust = ResScale(MaxThrust);
		vux_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		vux_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd;
	}
	else
	{
		vux_desc.characteristics.max_thrust = MaxThrust;
		vux_desc.characteristics.thrust_increment = ThrustIncrement;
		vux_desc.cyborg_control.WeaponRange = CloseRangeWeapon;
	}

	vux_desc.preprocess_func = vux_preprocess;
	vux_desc.postprocess_func = vux_postprocess;
	vux_desc.init_weapon_func = initialize_horrific_laser;
	vux_desc.cyborg_control.intelligence_func = vux_intelligence;

	RaceDesc* RaceDescPtr {&vux_desc};

	return (RaceDescPtr);
}

} // namespace uqm
