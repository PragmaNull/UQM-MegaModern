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
#include "ilwrath.h"
#include "resinst.h"

#include "uqm/colors.h"
#include "uqm/globdata.h"


// Core characteristics
static constexpr auto MaxCrew {22};
static constexpr auto MaxEnergy {16};
static constexpr auto EnergyRegeneration {4};
static constexpr auto EnergyWait {4};
static constexpr auto MaxThrust {25};
static constexpr auto ThrustIncrement {5};
static constexpr auto ThrustWait {0};
static constexpr auto TurnWait {2};
static constexpr auto ShipMass {7};
static constexpr auto LookAhead {4};
/* Controls how much the auto-turn will attempt to "lead"
		 * its target. */

// Hellfire Spout
static constexpr auto WeaponEnergyCost {1};
static constexpr auto WeaponWait {0};
static constexpr auto MissileLife {8};
static auto IlwrathOffset() { return ResScale(29); }
static auto MissileSpeed() { return ResScale(MaxThrust); }
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};
static constexpr auto MissileOffset {0};

// Cloaking Device
static constexpr auto SpecialEnergyCost {3};
static constexpr auto SpecialWait {13};

static RACE_DESC ilwrath_desc =
	{
		{/* SHIP_INFO */
		 "avenger",
		 FiresFore,
		 10, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 IlwrathRaceStrings,
		 IlwrathIconMaskPmapAnim,
		 IlwrathMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
			1410 / SphereRadiusIncrement * 2, /* Initial SoI radius */
			{
				/* Known location (center of SoI) */
				48,
				1700,
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
			 IlwrathBigMaskPmapAnim,
			 IlwrathMedMaskPmapAnim,
			 IlwrathSmlMaskPmapAnim,
		 },
		 {
			 FireBigMaskPmapAnim,
			 FireMedMaskPmapAnim,
			 FireSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {IlwrathCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 IlwrathVictorySong,
		 IlwrathShipSounds,
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

static void
flame_preprocess(ELEMENT* ElementPtr)
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

static void
flame_collision(ELEMENT* ElementPtr0, GFXPOINT* pPt0,
				ELEMENT* ElementPtr1, GFXPOINT* pPt1)
{
	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	ElementPtr0->state_flags &= ~Disappearing;
	ElementPtr0->state_flags |= Nonsolid;
}

static void
ilwrath_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
					 uint16_t ConcernCounter)
{
	STARSHIP* StarShipPtr {};

	EVALUATE_DESC* lpEvalDesc {&ObjectsOfConcern[ENEMY_SHIP_INDEX]};
	lpEvalDesc->MoveState = PURSUE;
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 10)
	{
		/* don't want to dodge when you could be flaming */
		ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = 0;
	}

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (lpEvalDesc->ObjectPtr
		&& (lpEvalDesc->which_turn <= 6
			|| (lpEvalDesc->which_turn <= 10
				&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 10)))
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
		if (ObjectCloaked(ShipPtr))
		{
			StarShipPtr->ship_input_state &= ~LeftFlag | RightFlag;
			StarShipPtr->ship_input_state |= ThrustFlag;
		}
		StarShipPtr->ship_input_state |= WeaponFlag;
	}
	else if (StarShipPtr->special_counter == 0
			 && (uqm::lowByte(uqm::currentActivity()) != IN_ENCOUNTER
				 || !uqm::gameState().flag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER)))
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
		if (!ObjectCloaked(ShipPtr)
			&& !(StarShipPtr->ship_input_state & WeaponFlag))
		{
			StarShipPtr->ship_input_state |= SpecialFlag;
		}
	}
}

static uint16_t
initialize_flame(ELEMENT* ShipPtr, HELEMENT FlameArray[])
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
	MissileBlock.pixoffs = IlwrathOffset();
	MissileBlock.speed = MissileSpeed();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = flame_preprocess;
	MissileBlock.blast_offs = MissileOffset;
	FlameArray[0] = initialize_missile(&MissileBlock);

	if (FlameArray[0])
	{
		int32_t dx {};
		int32_t dy {};
		ELEMENT* FlamePtr {};

		LockElement(FlameArray[0], &FlamePtr);
		GetCurrentVelocityComponentsSdword(&ShipPtr->velocity, &dx, &dy);
		DeltaVelocityComponents(&FlamePtr->velocity, dx, dy);
		FlamePtr->current.location.x -= VelocityToWorld(dx);
		FlamePtr->current.location.y -= VelocityToWorld(dy);

		FlamePtr->collision_func = flame_collision;
		FlamePtr->turn_wait = 0;
		UnlockElement(FlameArray[0]);
	}

	return (1);
}

static void
ilwrath_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	STATUS_FLAGS status_flags {StarShipPtr->cur_status_flags};
	PRIMITIVE* lpPrim {&DisplayArray[ElementPtr->PrimIndex]};
	if (GetPrimType(lpPrim) == STAMPFILL_PRIM)
	{
		Color color;
		bool weapon_discharge {};

		color = GetPrimColor(lpPrim);
		weapon_discharge = ((status_flags & WeaponFlag)
							&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= WeaponEnergyCost);
		if (weapon_discharge
			|| (StarShipPtr->special_counter == 0
				&& ((status_flags & SpecialFlag) || !sameColor(color, BlackColor))))
		{
			if (sameColor(color,
						  BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F)))
			{
				SetPrimType(lpPrim, STAMP_PRIM);
			}
			else if (sameColor(color,
							   BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B)))
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F));
			}
			else if (sameColor(color,
							   BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03)))
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B));
			}
			else if (sameColor(color,
							   BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)))
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03));
			}
			else if (sameColor(color,
							   BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)))
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09));
			}
			else
			{
				ProcessSound(GetSound(
								 /* CLOAKING_OFF */
								 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
							 ElementPtr);
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01));
				if (weapon_discharge)
				{
					uint16_t facing {StarShipPtr->ShipFacing};
					if (TrackShip(ElementPtr, &facing) >= 0)
					{
						ELEMENT* eptr {};
						int16_t dx0 {};
						int16_t dy0 {};
						int16_t dx1 {};
						int16_t dy1 {};
						VELOCITY_DESC v;

						LockElement(ElementPtr->hTarget, &eptr);
						v = eptr->velocity;
						GetNextVelocityComponents(&v, &dx0, &dy0, LookAhead);
						v = ElementPtr->velocity;
						GetNextVelocityComponents(&v, &dx1, &dy1, LookAhead);
						dx0 = (eptr->current.location.x + dx0)
							- (ElementPtr->current.location.x + dx1);
						dy0 = (eptr->current.location.y + dy0)
							- (ElementPtr->current.location.y + dy1);
						UnlockElement(ElementPtr->hTarget);

						StarShipPtr->ShipFacing =
							NormalizeFacing(
								AngleToFacing(ARCTAN(dx0, dy0)));
#ifdef NOTYET
						if (ElementPtr->thrust_wait == 0
							&& (StarShipPtr->cur_status_flags & ThrustFlag))
						{
							uint16_t last_facing;

							do
							{
								VELOCITY_DESC temp_v;

								last_facing = StarShipPtr->ShipFacing;
								inertial_thrust(ElementPtr);
								temp_v = ElementPtr->velocity;
								ElementPtr->velocity = v;

								dx0 += dx1;
								dy0 += dy1;
								GetNextVelocityComponents(&temp_v,
														  &dx1, &dy1, LookAhead);
								dx0 -= dx1;
								dy0 -= dy1;
								StarShipPtr->ShipFacing =
									NormalizeFacing(
										AngleToFacing(ARCTAN(dx0, dy0)));
							} while (StarShipPtr->ShipFacing != last_facing);
						}
#endif /* NOTYET */
						if (ElementPtr->turn_wait == 0)
						{
							++ElementPtr->turn_wait;
						}
						ElementPtr->next.image.frame =
							SetAbsFrameIndex(ElementPtr->next.image.frame,
											 StarShipPtr->ShipFacing);
					}
					ElementPtr->hTarget = 0;
				}
			}

			ElementPtr->state_flags |= Changing;
			status_flags &= ~SpecialFlag;
			StarShipPtr->special_counter = 0;
		}
		else if (!sameColor(color, BlackColor))
		{
			if (sameColor(color,
						  BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)))
			{
				SetPrimColor(lpPrim, BlackColor);
				Untarget(ElementPtr);
			}
			else if (sameColor(color,
							   BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)))
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01));
			}
			else if (sameColor(color,
							   BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03)))
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09));
			}
			else if (sameColor(color,
							   BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B)))
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03));
			}
			else
			{
				SetPrimColor(lpPrim,
							 BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B));
			}

			ElementPtr->state_flags |= Changing;
		}
	}

	if ((status_flags & SpecialFlag)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SpecialEnergyCost))
	{
		SetPrimColor(lpPrim,
					 BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F));
		SetPrimType(lpPrim, STAMPFILL_PRIM);

		ProcessSound(GetSound(
						 /* CLOAKING_ON */
						 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
					 ElementPtr);
		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;

		ElementPtr->state_flags |= Changing;
	}
}

RACE_DESC*
init_ilwrath(void)
{
	if (IsHd())
	{
		ilwrath_desc.characteristics.max_thrust = ResScale(MaxThrust);
		ilwrath_desc.characteristics.thrust_increment =
			ResScale(ThrustIncrement);
		ilwrath_desc.cyborg_control.WeaponRange = CloseRangeWeaponHd;
	}
	else
	{
		ilwrath_desc.characteristics.max_thrust = MaxThrust;
		ilwrath_desc.characteristics.thrust_increment = ThrustIncrement;
		ilwrath_desc.cyborg_control.WeaponRange = CloseRangeWeapon;
	}

	ilwrath_desc.preprocess_func = ilwrath_preprocess;
	ilwrath_desc.init_weapon_func = initialize_flame;
	ilwrath_desc.cyborg_control.intelligence_func = ilwrath_intelligence;

	RACE_DESC* RaceDescPtr {&ilwrath_desc};

	return (RaceDescPtr);
}
