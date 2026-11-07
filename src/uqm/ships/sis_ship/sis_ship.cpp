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

#include "sis_ship.h"

#include "../../setup.h"
#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/colors.h"
#include "uqm/controls.h"
#include "uqm/globdata.h"
#include "uqm/hyper.h"

namespace uqm
{
/* Core characteristics.
 * All of these are changed at init time by some module, except for
 * MaxEnergySize, ThrustIncrement, and MaxShipMass. */

/* This value gets thrown out - actual max crew is determined by the
		 * number of crew pods. The minimum value is 1 (just the Captain). */

static constexpr auto EnergyRegeneration {1};
/* Shiva furnaces increase this by 1 each. */
static constexpr auto ShivaEnergyRegenInc {1};

static constexpr auto EnergyWait {10};
/* Dynamos decrease this by 2 each, to a minimum of 4. */
static constexpr auto MinEnergyWait {4};
static constexpr auto DynamoUnitEnergyWaitDec {2};

static constexpr auto MaxThrust {10};
/* Thrusters increase this and decrease ThrustWait based on
		 * ThrustIncrement, see InitDriveSlots near the bottom of this file
		 * for details. */
static constexpr auto ThrustIncrement {4};
static constexpr auto ThrustWait {6};
static constexpr auto TurnWait {17};
/* Turning jets decrease by 2 each */


/* Primary weapon - energy cost and damage change at init time based on
 * the number and type of weapon modules installed. */

static constexpr auto BlasterDamage {2};
/* This is the damage value for the basic ion bolt guns. Fusion
		 * blasters and hellbore cannons end up doing (BlasterDamage * 2)
		 * and (BlasterDamage * 3) damage, respectively, but this depends
		 * on enum values. */

static constexpr auto BlasterHits {2}; /* Hitpoints for ion bolt guns, see BLASTER_DAMAGE */

static constexpr auto WeaponEnergyCost {1};
/* This value gets thrown out and reset in an ugly manner based on
		 * the enum that is used for module IDs. Bigger gun = higher value.
		 */
static constexpr auto WeaponWait {6};
static constexpr auto BlasterSpeed {DisplayToWorld(24)};
static constexpr auto BlasterLife {12};
/* This value is greatly increased, based in part on the enum used
		 * for module IDs (bigger gun == longer life). See the first half of
		 * InitWeaponSlots */
static constexpr auto MaxTracking {3};
static auto TrackerEnergyCost() { return ifHardDifficulty(6, 3); }
static auto BlasterOffset() { return ResScale(8); }
static auto SisVertOffset() { return ResScale(28); }
/* Used for foward, spread, and rear slots */
static auto SisHorzOffset() { return ResScale(20); }
/* Used for side slot */

/* Secondary weapon */
static constexpr auto SpecialEnergyCost {0};
/* Increased by 1 for each point defense module */
static constexpr auto AntimissileEnergyInc {1};
static constexpr auto SpecialWait {9};
static auto LaserRange() { return (uint16_t)ResScale(100); }
static constexpr auto MaxDefense {8};

// HdResolutionFactor
static auto BlasterSpeedHd() { return ResScale(BlasterSpeed); }
static auto BlasterSpeedBool() { return chooseIfHd(BlasterSpeed, BlasterSpeedHd()); }


static RaceDesc sis_desc =
	{
		{/* ShipInfoDesc */
		 "flagship",
		 ShipFlags::None,
		 16, /* Super Melee cost */
		 MaxCrewSize, MaxCrewSize,
		 MaxEnergySize, MaxEnergySize,
		 NullResource,
		 SisIconMaskPmapAnim,
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
			MaxShipMass,
		 },
		{{
			 SisBigMaskPmapAnim,
			 SisMedMaskPmapAnim,
			 SisSmlMaskPmapAnim,
		 },
		 {
			 BlasterBigMaskPmapAnim,
			 BlasterMedMaskPmapAnim,
			 BlasterSmlMaskPmapAnim,
		 },
		 {
			 NullResource,
			 NullResource,
			 NullResource,
		 },
		 {SisCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 SisVictorySong,
		 SisShipSounds,
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 {nullptr, nullptr, nullptr},
		 nullptr,
		 nullptr},
		{
			0,
			BlasterSpeed * BlasterLife,
			nullptr,
		 },
		(UNINIT_FUNC*)nullptr,
		(PREPROCESS_FUNC*)nullptr,
		(POSTPROCESS_FUNC*)nullptr,
		(INIT_WEAPON_FUNC*)nullptr,
		0,
		0, /* CodeRef */
};

// Private per-instance SIS data
struct FlagshipBattleData
{
	uint16_t num_trackers;
	uint16_t num_blasters;
	MissileDesc MissileBlock[6];
};

static void InitWeaponSlots(RaceDesc* RaceDescPtr,
							const ModuleType* ModuleSlots);
static void InitModuleSlots(RaceDesc* RaceDescPtr,
							const ModuleType* ModuleSlots);
static void InitDriveSlots(RaceDesc* RaceDescPtr,
						   const ModuleType* DriveSlots);
static void InitJetSlots(RaceDesc* RaceDescPtr,
						 const ModuleType* JetSlots);
static void uninit_sis(RaceDesc* pRaceDesc);


// Local typedef
typedef FlagshipBattleData CustomShipData_t;

// Retrieve race-specific ship data from a race desc
static CustomShipData_t* GetCustomShipData(RaceDesc* pRaceDesc)
{
	return (CustomShipData_t*)pRaceDesc->data;
}

// Set the race-specific data in a race desc
// (Re)Allocates its own storage for the data.
static void SetCustomShipData(RaceDesc* pRaceDesc, const CustomShipData_t* data)
{
	if (pRaceDesc->data == data)
	{
		return; // no-op
	}

	if (pRaceDesc->data) // Out with the old
	{
		delete static_cast<CustomShipData_t*>(pRaceDesc->data);
		pRaceDesc->data = nullptr;
	}

	if (data) // In with the new
	{
		pRaceDesc->data = new CustomShipData_t {*data};
	}
}

static void sis_hyper_preprocess(Element* ElementPtr)
{
	int32_t udx = 0;
	int32_t udy = 0;
	int16_t dx = 0;
	int16_t dy = 0;
	Starship* StarShipPtr {};

	if (testFlag(ElementPtr->state_flags, ElementFlags::Appearing))
	{
		ElementPtr->velocity = uqm::flagship().velocity;
	}

	int16_t AccelerateDirection {0};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	++StarShipPtr->weapon_counter; /* no shooting in hyperspace! */

	if (!uqm::flagship().autopilot.hasTarget()
		|| (testFlag(StarShipPtr->cur_status_flags, (StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust))))
	{
LeaveAutoPilot:

		uqm::flagship().autopilot.clear();

		if ((testFlag(StarShipPtr->cur_status_flags, (StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust)))
			&& (ValidPoint(LoadAdvancedAutoPilot())
				|| ValidPoint(LoadAdvancedQuasiPilot())))
		{
			ZeroAdvancedAutoPilot();
			ZeroAdvancedQuasiPilot();
		}

		if (!(testFlag(StarShipPtr->cur_status_flags, StatusFlags::Thrust))
			|| (uqm::gameState().sis().FuelOnBoard == 0
				&& uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1))
		{
			AccelerateDirection = -1;
			GetCurrentVelocityComponents(&ElementPtr->velocity,
										 &dx, &dy);

			udx = chooseIfHd(dx << 4, (int32_t)dx);
			udy = chooseIfHd(dy << 4, (int32_t)dy);

			StarShipPtr->cur_status_flags &= ~StatusFlags::Thrust;
		}
	}
	else
	{
		int16_t facing {};
		GfxPoint universe;

		universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
		universe.y = LogyToUniverse(uqm::gameState().sis().log_y);
		udx = uqm::flagship().autopilot.target().x - universe.x;
		udy = -(uqm::flagship().autopilot.target().y - universe.y);
		if ((dx = (int16_t)udx) < 0)
		{
			dx = -dx;
		}
		if ((dy = (int16_t)udy) < 0)
		{
			dy = -dy;
		}
		if (dx <= ResScale(1) && dy <= ResScale(1))
		{
			goto LeaveAutoPilot;
		}

		facing = NormalizeFacing(AngleToFacing(ARCTAN(udx, udy)));

		/* This prevents ship from flying backwards on auto-pilot.
		 * It could also theoretically abort autopilot in a bad savegame */
		if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::ShipAtMaxSpeed))
			/*|| (ElementPtr->state_flags & Appearing)*/)
		{
			if (NormalizeFacing(StarShipPtr->ShipFacing
								+ AngleToFacing(Quadrant)
								- facing)
				> AngleToFacing(HalfCircle))
			{
				goto LeaveAutoPilot;
			}

			facing = StarShipPtr->ShipFacing;
		}
		else if ((int)facing != (int)StarShipPtr->ShipFacing
				 && ElementPtr->turn_wait == 0)
		{
			if (NormalizeFacing(
					StarShipPtr->ShipFacing - facing)
				>= AngleToFacing(HalfCircle))
			{
				facing = NormalizeFacing(facing - 1);
				StarShipPtr->cur_status_flags |= StatusFlags::Right;
			}
			else if ((int)StarShipPtr->ShipFacing != (int)facing)
			{
				facing = NormalizeFacing(facing + 1);
				StarShipPtr->cur_status_flags |= StatusFlags::Left;
			}

			if ((int)facing == (int)StarShipPtr->ShipFacing)
			{
				ZeroVelocityComponents(&ElementPtr->velocity);
			}
		}

		GetCurrentVelocityComponents(&ElementPtr->velocity, &dx, &dy);
		if ((uqm::gameState().sis().FuelOnBoard
			 || uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) > 1)
			&& (int)facing == (int)StarShipPtr->ShipFacing)
		{
			StarShipPtr->cur_status_flags |= StatusFlags::ShipAtMaxSpeed;
			AccelerateDirection = 1;
		}
		else
		{
			AccelerateDirection = -1;
			udx = dx << IfHd(4);
			udy = dy << IfHd(4);
		}
	}

	if (ElementPtr->thrust_wait == 0 && AccelerateDirection)
	{
		uint16_t dist {};
		int16_t speed {};
		int16_t velocity_increment {};

		velocity_increment = WorldToVelocity(
			StarShipPtr->RaceDescPtr->characteristics.thrust_increment);

		if ((dist = square_root((long)udx * udx + (long)udy * udy)) == 0)
		{
			dist = 1; /* prevent divide by zero */
		}

		speed = square_root((long)dx * dx + (long)dy * dy);
		if (AccelerateDirection < 0)
		{
			dy = (speed / velocity_increment - 1) * velocity_increment;
			if (dy < speed - velocity_increment)
			{
				dy = speed - velocity_increment;
			}
			if ((speed = dy) < 0)
			{
				speed = 0;
			}

			StarShipPtr->cur_status_flags &= ~StatusFlags::ShipAtMaxSpeed;
		}
		else
		{
			int16_t max_velocity {};

			AccelerateDirection = 0;

			max_velocity = WorldToVelocity(
				StarShipPtr->RaceDescPtr->characteristics.max_thrust);

			dy = (speed / velocity_increment + 1)
			   * velocity_increment;
			if (dy < speed + velocity_increment)
			{
				dy = speed + velocity_increment;
			}
			if ((speed = dy) > max_velocity)
			{
				speed = max_velocity;
				StarShipPtr->cur_status_flags |= StatusFlags::ShipAtMaxSpeed;
			}
		}

		dx = (int32_t)((long)udx * speed / (long)dist);
		dy = (int32_t)((long)udy * speed / (long)dist);
		SetVelocityComponents(&ElementPtr->velocity, dx, dy);

		ElementPtr->thrust_wait =
			StarShipPtr->RaceDescPtr->characteristics.thrust_wait;
	}
}

static constexpr auto MenuDelay {10};

static void sis_hyper_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	uqm::flagship().velocity = ElementPtr->velocity;

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Weapon)) || PulsedInputState.menu[*MenuKey::MenuCancel])
		&& StarShipPtr->special_counter == 0)
	{
		HyperspaceMenu();
		StarShipPtr->cur_status_flags &= ~StatusFlags::ShipAtMaxSpeed;
		StarShipPtr->special_counter = MenuDelay;
	}
}

static void spawn_point_defense(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (testFlag(ElementPtr->state_flags, ElementFlags::PlayerShip))
	{
		ElementHandle hDefense {AllocElement()};
		if (hDefense)
		{
			Element* DefensePtr {};

			LockElement(hDefense, &DefensePtr);
			DefensePtr->playerNr = ElementPtr->playerNr;
			DefensePtr->state_flags = ElementFlags::Appearing | ElementFlags::Nonsolid | ElementFlags::FiniteLife;
			DefensePtr->death_func = spawn_point_defense;

			GetElementStarShip(ElementPtr, &StarShipPtr);
			SetElementStarShip(DefensePtr, StarShipPtr);
			UnlockElement(hDefense);

			PutElement(hDefense);
		}
	}
	else
	{
		ElementHandle hNextObject {};
		Element* ShipPtr {};
		Color LaserColor;
		const static Color ColorRange[] =
			{
				BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7F),
				BuildColor(MakeRgb15(0x1F, 0x07, 0x00), 0x7E),
				BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7D),
				BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7C),
				BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7B),
				BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7A),
				BuildColor(MakeRgb15(0x1F, 0x18, 0x00), 0x79),
				BuildColor(MakeRgb15(0x1F, 0x1C, 0x00), 0x78),
			};

		bool PaidFor {false};

		LaserColor = ColorRange[StarShipPtr->RaceDescPtr->characteristics.special_energy_cost];
		LockElement(StarShipPtr->hShip, &ShipPtr);
		for (ElementHandle hObject {GetTailElement()}; hObject; hObject = hNextObject)
		{
			Element* ObjectPtr {};

			LockElement(hObject, &ObjectPtr);
			hNextObject = GetPredElement(ObjectPtr);
			if (ObjectPtr != ShipPtr && CollidingElement(ObjectPtr) && !ObjectCloaked(ObjectPtr))
			{
				int32_t delta_x {ObjectPtr->next.location.x - ShipPtr->next.location.x};
				int32_t delta_y {ObjectPtr->next.location.y - ShipPtr->next.location.y};
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
				if ((uint16_t)delta_x <= LaserRange() && (uint16_t)delta_y <= LaserRange() && (uint16_t)delta_x * (uint16_t)delta_x + (uint16_t)delta_y * (uint16_t)delta_y <= LaserRange() * LaserRange())
				{
					ElementHandle hPointDefense {};
					LaserDesc LaserBlock;

					if (!PaidFor)
					{
						if (!DeltaEnergy(ShipPtr,
										 -(StarShipPtr->RaceDescPtr->characteristics.special_energy_cost
										   << 2)))
						{
							break;
						}

						ProcessSound(GetSound(
										 /* POINT_DEFENSE_LASER */
										 StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
									 ElementPtr);
						StarShipPtr->special_counter =
							StarShipPtr->RaceDescPtr->characteristics.special_wait;
						PaidFor = true;
					}

					LaserBlock.cx = ShipPtr->next.location.x;
					LaserBlock.cy = ShipPtr->next.location.y;
					LaserBlock.face = 0;
					LaserBlock.ex = ObjectPtr->next.location.x
								  - ShipPtr->next.location.x;
					LaserBlock.ey = ObjectPtr->next.location.y
								  - ShipPtr->next.location.y;
					LaserBlock.sender = ShipPtr->playerNr;
					LaserBlock.flags = ElementFlags::IgnoreSimilar;
					LaserBlock.pixoffs = 0;
					LaserBlock.color = LaserColor;
					hPointDefense = initialize_laser(&LaserBlock);
					if (hPointDefense)
					{
						Element* PDPtr {};

						LockElement(hPointDefense, &PDPtr);
						PDPtr->mass_points =
							StarShipPtr->RaceDescPtr->characteristics.special_energy_cost;
						SetElementStarShip(PDPtr, StarShipPtr);
						PDPtr->hTarget = 0;
						UnlockElement(hPointDefense);

						PutElement(hPointDefense);
					}
				}
			}
			UnlockElement(hObject);
		}
		UnlockElement(StarShipPtr->hShip);
	}
}

static void sis_battle_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->characteristics.special_energy_cost == 0)
	{
		StarShipPtr->cur_status_flags &= ~StatusFlags::Special;
		StarShipPtr->special_counter = 2;
	}
	if (!(testFlag(StarShipPtr->RaceDescPtr->ship_info.ship_flags, (ShipFlags::FiresFore | ShipFlags::FiresRight | ShipFlags::FiresAft | ShipFlags::FiresLeft))))
	{
		StarShipPtr->cur_status_flags &= ~StatusFlags::Weapon;
		StarShipPtr->weapon_counter = 2;
	}
}

static void sis_battle_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
		&& StarShipPtr->special_counter == 0
		&& StarShipPtr->RaceDescPtr->characteristics.special_energy_cost)
	{
		spawn_point_defense(ElementPtr);
	}
}

static void blaster_collision(Element* ElementPtr0, GfxPoint* pPt0,
							  Element* ElementPtr1, GfxPoint* pPt1)
{
	ElementHandle hBlastElement {weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1)};
	if (hBlastElement)
	{
		Element* BlastElementPtr {};

		LockElement(hBlastElement, &BlastElementPtr);
		switch (ElementPtr0->mass_points)
		{
			case BlasterDamage * 1:
				BlastElementPtr->life_span = 2;
				BlastElementPtr->current.image.frame =
					SetAbsFrameIndex(ElementPtr0->current.image.frame, 0);
				BlastElementPtr->preprocess_func = nullptr;
				break;
			case BlasterDamage * 2:
				BlastElementPtr->life_span = 6;
				BlastElementPtr->current.image.frame =
					IncFrameIndex(ElementPtr0->current.image.frame);
				break;
			case BlasterDamage * 3:
				BlastElementPtr->life_span = 7;
				BlastElementPtr->current.image.frame =
					SetAbsFrameIndex(ElementPtr0->current.image.frame, 20);
				break;
		}
		UnlockElement(hBlastElement);
	}
}

static void blaster_preprocess(Element* ElementPtr)
{
	uint8_t wait {};

	switch (ElementPtr->mass_points)
	{
		case BlasterDamage * 1:
			if (GetFrameIndex(ElementPtr->current.image.frame) < 8)
			{
				ElementPtr->next.image.frame =
					IncFrameIndex(ElementPtr->current.image.frame);
				ElementPtr->state_flags |= ElementFlags::Changing;
			}
			break;
		case BlasterDamage * 3:
			if (GetFrameIndex(ElementPtr->current.image.frame) < 19)
			{
				ElementPtr->next.image.frame =
					IncFrameIndex(ElementPtr->current.image.frame);
			}
			else
			{
				ElementPtr->next.image.frame =
					SetAbsFrameIndex(ElementPtr->current.image.frame, 16);
			}
			ElementPtr->state_flags |= ElementFlags::Changing;
			break;
	}

	if (uqm::lowNibble(ElementPtr->turn_wait))
	{
		--ElementPtr->turn_wait;
	}
	else if ((wait = uqm::highNibble(ElementPtr->turn_wait)))
	{
		uint16_t facing {};

		facing = NormalizeFacing(AngleToFacing(
			GetVelocityTravelAngle(&ElementPtr->velocity)));
		if (TrackShip(ElementPtr, &facing) > 0)
		{
			SetVelocityVector(&ElementPtr->velocity, BlasterSpeedBool(), facing);
		}

		ElementPtr->turn_wait = uqm::makeByte(wait, wait);
	}
}

static uint16_t initialize_blasters(Element* ShipPtr, ElementHandle BlasterArray[])
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	const FlagshipBattleData* SisData {GetCustomShipData(StarShipPtr->RaceDescPtr)};

	const uint8_t nt {(uint8_t)((4 - SisData->num_trackers) & 3)};

	for (uint16_t i {0}; i < SisData->num_blasters; ++i)
	{
		MissileDesc MissileBlock = SisData->MissileBlock[i];

		MissileBlock.cx = ShipPtr->next.location.x;
		MissileBlock.cy = ShipPtr->next.location.y;
		MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
		MissileBlock.sender = ShipPtr->playerNr;
		MissileBlock.face = NormalizeFacing(StarShipPtr->ShipFacing
											+ MissileBlock.face);

		BlasterArray[i] = initialize_missile(&MissileBlock);
		if (BlasterArray[i])
		{
			Element* BlasterPtr {};

			LockElement(BlasterArray[i], &BlasterPtr);
			BlasterPtr->collision_func = blaster_collision;
			BlasterPtr->turn_wait = uqm::makeByte(nt, nt);
			UnlockElement(BlasterArray[i]);
		}
	}

	return SisData->num_blasters;
}

static void sis_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
							 uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	const FlagshipBattleData* SisData {GetCustomShipData(StarShipPtr->RaceDescPtr)};

	ObjectEvaluation* lpEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
	if (lpEvalDesc->ObjectPtr)
	{
		if (StarShipPtr->RaceDescPtr->characteristics.special_energy_cost)
		{
			if (StarShipPtr->special_counter == 0
				&& ((lpEvalDesc->ObjectPtr
					 && lpEvalDesc->which_turn <= 2)
					|| (ObjectsOfConcern[*IntelObjectIndex::EnemyShip].ObjectPtr != nullptr
						&& ObjectsOfConcern[*IntelObjectIndex::EnemyShip].which_turn <= 4)))
			{
				StarShipPtr->ship_input_state |= StatusFlags::Special;
			}
			else
			{
				StarShipPtr->ship_input_state &= ~StatusFlags::Special;
			}
			lpEvalDesc->ObjectPtr = nullptr;
		}
		else if (Maneuverability(&StarShipPtr->RaceDescPtr->cyborg_control)
					 < ResolutionCompensated(MediumShip)
				 && lpEvalDesc->MoveState == MovementState::Entice
				 && (!(testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::CrewObject))
					 || lpEvalDesc->which_turn <= 8)
				 && (!(testFlag(lpEvalDesc->ObjectPtr->state_flags, ElementFlags::FiniteLife))
					 || (lpEvalDesc->ObjectPtr->mass_points >= 4
						 && lpEvalDesc->which_turn == 2
						 && ObjectsOfConcern[*IntelObjectIndex::EnemyShip].which_turn > 16)))
		{
			lpEvalDesc->MoveState = MovementState::Pursue;
		}
	}

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	lpEvalDesc = &ObjectsOfConcern[*IntelObjectIndex::EnemyShip];
	if (SisData->num_trackers
		&& StarShipPtr->weapon_counter == 0
		&& !(testFlag(StarShipPtr->ship_input_state, StatusFlags::Weapon))
		&& lpEvalDesc->ObjectPtr
		&& lpEvalDesc->which_turn <= 16)
	{
		uint16_t direction_facing {};
		int32_t delta_x {};
		int32_t delta_y {};
		ShipFlags ship_flags {};
		uint16_t facing {};

		delta_x = lpEvalDesc->ObjectPtr->current.location.x
				- ShipPtr->current.location.x;
		delta_y = lpEvalDesc->ObjectPtr->current.location.y
				- ShipPtr->current.location.y;
		direction_facing = NormalizeFacing(
			AngleToFacing(ARCTAN(delta_x, delta_y)));

		ship_flags = StarShipPtr->RaceDescPtr->ship_info.ship_flags;
		facing = StarShipPtr->ShipFacing;
		for (size_t dir {0}; dir < FireDirections.size(); ++dir, facing += Quadrant)
		{
			if (testFlag(ship_flags, FireDirections[dir]) && NormalizeFacing(direction_facing - facing + AngleToFacing(Octant)) <= AngleToFacing(Quadrant))
			{
				StarShipPtr->ship_input_state |= StatusFlags::Weapon;
				break;
			}
		}
	}
}

static void InitWeaponSlots(RaceDesc* RaceDescPtr, const ModuleType* ModuleSlots)
{
	FlagshipBattleData* SisData = GetCustomShipData(RaceDescPtr);
	MissileDesc* lpMB = SisData->MissileBlock;

	SisData->num_blasters = 0;
	for (uint16_t i {0}; i < NumModuleSlots; ++i)
	{
		const bool IfHard = isDifficulty(uqm::Difficulty::Hard) && (i == 1 || i == 2) ? true : false;

		if (i == 3)
		{
			i = NumModuleSlots - 1;
		}

		uint16_t which_gun {*ModuleSlots[(NumModuleSlots - 1) - i]};

		if (which_gun < ModuleType::GunWeapon || which_gun > ModuleType::CannonWeapon)
		{
			continue; /* not a gun */
		}

		which_gun -= *ModuleType::GunWeapon - 1;
		RaceDescPtr->characteristics.weapon_energy_cost +=
			which_gun * (IfHard ? 4 : 2);

		lpMB->flags = ElementFlags::IgnoreSimilar;
		lpMB->blast_offs = BlasterOffset();
		lpMB->speed = BlasterSpeedBool();
		lpMB->preprocess_func = blaster_preprocess;
		lpMB->hit_points = BlasterHits * which_gun;
		lpMB->damage = BlasterDamage * which_gun;
		lpMB->life = BlasterLife + ((BlasterLife >> 2) * (which_gun - 1));

		if (which_gun == 1)
		{
			lpMB->index = 0;
		}
		else if (which_gun == 2)
		{
			lpMB->index = 9;
		}
		else
		{
			lpMB->index = 16;
		}

		switch (i)
		{
			case 0: /* NOSE WeaponFlag */
				RaceDescPtr->ship_info.ship_flags |= ShipFlags::FiresFore;
				lpMB->pixoffs = SisVertOffset();
				lpMB->face = 0;
				break;
			case 1: /* SPREAD WeaponFlag */
				RaceDescPtr->ship_info.ship_flags |= ShipFlags::FiresFore;
				lpMB->pixoffs = SisVertOffset();
				lpMB->face = +1;
				/* copy it because there are two */
				lpMB[1] = lpMB[0];
				++lpMB;
				++SisData->num_blasters;
				lpMB->face = NormalizeFacing(-1);
				break;
			case 2: /* SIDE WeaponFlag */
				RaceDescPtr->ship_info.ship_flags |=
					ShipFlags::FiresLeft | ShipFlags::FiresRight;
				lpMB->pixoffs = SisHorzOffset();
				lpMB->face = AngleToFacing(Quadrant);
				/* copy it because there are two */
				lpMB[1] = lpMB[0];
				++lpMB;
				++SisData->num_blasters;
				lpMB->face = NormalizeFacing(-AngleToFacing(Quadrant));
				break;
			case NumModuleSlots - 1: /* TAIL WeaponFlag */
				RaceDescPtr->ship_info.ship_flags |= ShipFlags::FiresAft;
				lpMB->pixoffs = SisVertOffset();
				lpMB->face = AngleToFacing(HalfCircle);
				break;
		}

		++lpMB;
		++SisData->num_blasters;
	}
}

static void InitModuleSlots(RaceDesc* RaceDescPtr, const ModuleType* ModuleSlots)
{
	FlagshipBattleData* SisData = GetCustomShipData(RaceDescPtr);

	RaceDescPtr->ship_info.max_crew = 0;
	uint16_t num_trackers {0};
	for (uint16_t i {0}; i < NumModuleSlots; ++i)
	{
		const ModuleType which_mod {ModuleSlots[(NumModuleSlots - 1) - i]};
		switch (which_mod)
		{
			case ModuleType::CrewPod:
				RaceDescPtr->ship_info.max_crew += CrewPodCapacity;
				break;
			case ModuleType::TrackingSystem:
				++num_trackers;
				break;
			case ModuleType::AntimissileDefense:
				RaceDescPtr->characteristics.special_energy_cost +=
					AntimissileEnergyInc;
				break;
			case ModuleType::ShivaFurnace:
				RaceDescPtr->characteristics.energy_regeneration +=
					ShivaEnergyRegenInc;
				break;
			case ModuleType::DynamoUnit:
				RaceDescPtr->characteristics.energy_wait -=
					DynamoUnitEnergyWaitDec;
				if (RaceDescPtr->characteristics.energy_wait < MinEnergyWait)
				{
					RaceDescPtr->characteristics.energy_wait = MinEnergyWait;
				}
				break;
			default:
				break;
		}
	}

	if (num_trackers > MaxTracking)
	{
		num_trackers = MaxTracking;
	}
	RaceDescPtr->characteristics.weapon_energy_cost +=
		num_trackers * TrackerEnergyCost();
	SisData->num_trackers = num_trackers;
	if (RaceDescPtr->characteristics.special_energy_cost)
	{
		RaceDescPtr->ship_info.ship_flags |= ShipFlags::PointDefense;
		if (RaceDescPtr->characteristics.special_energy_cost > MaxDefense)
		{
			RaceDescPtr->characteristics.special_energy_cost = MaxDefense;
		}
	}
}

static void InitDriveSlots(RaceDesc* RaceDescPtr, const ModuleType* DriveSlots)
{
	// NB. RaceDescPtr->characteristics.max_thrust is already initialised.
	RaceDescPtr->characteristics.thrust_wait = 0;
	for (uint16_t i {0}; i < NumDriveSlots; ++i)
	{
		switch (DriveSlots[i])
		{
			case ModuleType::FusionThruster:
				RaceDescPtr->characteristics.max_thrust += ResScale(2);
				++RaceDescPtr->characteristics.thrust_wait;
				break;
			default:
				break;
		}
	}
	RaceDescPtr->characteristics.thrust_wait = (uint8_t)(ThrustWait - (RaceDescPtr->characteristics.thrust_wait >> 1));
	RaceDescPtr->characteristics.max_thrust =
		((RaceDescPtr->characteristics.max_thrust / RaceDescPtr->characteristics.thrust_increment) + 1)
		* RaceDescPtr->characteristics.thrust_increment;
}

static void InitJetSlots(RaceDesc* RaceDescPtr, const ModuleType* JetSlots)
{
	for (uint16_t i {0}; i < NumJetSlots; ++i)
	{
		switch (JetSlots[i])
		{
			case ModuleType::TurningJets:
				RaceDescPtr->characteristics.turn_wait -= 2;
				break;
			default:
				break;
		}
	}
}

RaceDesc* init_sis(void)
{
	RaceDesc* RaceDescPtr {};
	uint16_t i {};
	// The caller of this func will copy the struct
	static RaceDesc new_sis_desc;
	FlagshipBattleData empty_data;
	memset(&empty_data, 0, sizeof(empty_data));

	if (IsHd())
	{
		sis_desc.characteristics.max_thrust = ResScale(MaxThrust);
		sis_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
		sis_desc.cyborg_control.WeaponRange = BlasterSpeedHd() * BlasterLife;
	}
	else
	{
		sis_desc.characteristics.max_thrust = MaxThrust;
		sis_desc.characteristics.thrust_increment = ThrustIncrement;
		sis_desc.cyborg_control.WeaponRange = BlasterSpeed * BlasterLife;
	}

	/* copy initial ship settings to new_sis_desc */
	new_sis_desc = sis_desc;

	new_sis_desc.uninit_func = uninit_sis;

	if (inHQSpace())
	{
		for (i = 0; i < NumViews; ++i)
		{
			new_sis_desc.ship_data.ship_rsc[i] = NullResource;
			new_sis_desc.ship_data.weapon_rsc[i] = NullResource;
			new_sis_desc.ship_data.special_rsc[i] = NullResource;
		}
		new_sis_desc.ship_info.icons_rsc = NullResource;
		new_sis_desc.ship_data.captain_control.captain_rsc = NullResource;
		new_sis_desc.ship_data.victory_ditty_rsc = NullResource;
		new_sis_desc.ship_data.ship_sounds_rsc = NullResource;

		new_sis_desc.ship_data.ship_rsc[0] = SisHyperMaskPmapAnim;

		new_sis_desc.preprocess_func = sis_hyper_preprocess;
		new_sis_desc.postprocess_func = sis_hyper_postprocess;

		new_sis_desc.characteristics.max_thrust -= ResScale(4);
	}
	else
	{
		new_sis_desc.preprocess_func = sis_battle_preprocess;
		new_sis_desc.postprocess_func = sis_battle_postprocess;
		new_sis_desc.init_weapon_func = initialize_blasters;
		new_sis_desc.cyborg_control.intelligence_func = sis_intelligence;

		if (uqm::UQMOptions::read().flagshipColor == uqm::EmulationMode::Console3DO)
		{
			new_sis_desc.ship_info.icons_rsc = SisIconMaskPmapAnimRed;
			new_sis_desc.ship_data.ship_rsc[0] = SisBigMaskPmapAnimRed;
			new_sis_desc.ship_data.ship_rsc[1] = SisMedMaskPmapAnimRed;
			new_sis_desc.ship_data.ship_rsc[2] = SisSmlMaskPmapAnimRed;
		}
		else
		{
			new_sis_desc.ship_info.icons_rsc = SisIconMaskPmapAnim;
			new_sis_desc.ship_data.ship_rsc[0] = SisBigMaskPmapAnim;
			new_sis_desc.ship_data.ship_rsc[1] = SisMedMaskPmapAnim;
			new_sis_desc.ship_data.ship_rsc[2] = SisSmlMaskPmapAnim;
		}

		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 3)
		{
			uqm::gameState().setFlag(uqm::GameFlag::BOMB_CARRIER, static_cast<uint32_t>(1));
		}
	}

	SetCustomShipData(&new_sis_desc, &empty_data);
	InitModuleSlots(&new_sis_desc, uqm::gameState().sis().ModuleSlots);
	InitWeaponSlots(&new_sis_desc, uqm::gameState().sis().ModuleSlots);
	InitDriveSlots(&new_sis_desc, uqm::gameState().sis().DriveSlots);
	InitJetSlots(&new_sis_desc, uqm::gameState().sis().JetSlots);

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::SuperMelee)
	{
		new_sis_desc.ship_info.crew_level = new_sis_desc.ship_info.max_crew;
	}
	else
	{
		// Count the captain too.
		new_sis_desc.ship_info.max_crew++;
		new_sis_desc.ship_info.crew_level = uqm::gameState().sis().CrewEnlisted + 1;
		new_sis_desc.ship_info.ship_flags |= ShipFlags::PlayerCaptain;
	}

	new_sis_desc.ship_info.energy_level = new_sis_desc.ship_info.max_energy;

	RaceDescPtr = &new_sis_desc;

	return (RaceDescPtr);
}

static void uninit_sis(RaceDesc* pRaceDesc)
{
	if (!inHQSpace())
	{
		uqm::gameState().sis().CrewEnlisted = pRaceDesc->ship_info.crew_level;
		if (testFlag(pRaceDesc->ship_info.ship_flags, ShipFlags::PlayerCaptain))
		{
			uqm::gameState().sis().CrewEnlisted--;
		}
	}

	SetCustomShipData(pRaceDesc, nullptr);
}

} // namespace uqm
