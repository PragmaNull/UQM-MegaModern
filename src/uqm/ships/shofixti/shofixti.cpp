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
#include "shofixti.h"
#include "resinst.h"
#include "../../setup.h"
#include "../../setupmenu.h"
#include "uqm/globdata.h"
#include "uqm/tactrans.h"
#include "libs/mathlib.h"
#include "../../starmap.h"

// Core characteristics
static constexpr auto MaxCrew {6};
static constexpr auto MaxEnergy {4};
static constexpr auto EnergyRegeneration {1};
static constexpr auto EnergyWait {9};
static constexpr auto MaxThrust {35};
static constexpr auto ThrustIncrement {5};
static constexpr auto TurnWait {1};
static constexpr auto ThrustWait {0};
static constexpr auto WeaponWait {3};
static constexpr auto SpecialWait {0};
static constexpr auto ShipMass {1};

// Dart Gun
static constexpr auto WeaponEnergyCost {1};
static auto ShofixtiOffset() { return chooseIfHd(15, 51); }
static auto MissileOffset() { return RES_SCALE(1); }
static constexpr auto MissileSpeed {DISPLAY_TO_WORLD(24)};
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};

// Glory Device
static constexpr auto SpecialEnergyCost {0};
static constexpr auto SpecialSauce {180};
static auto DestructRange() { return RES_SCALE(SpecialSauce); }
static constexpr auto MaxDestruction {(SpecialSauce / 10)};

// Full game: Tanaka/Katana's damaged ships
static constexpr auto NumLimpets {3};

// HdResolutionFactor
static auto MissileSpeedHd() { return RES_SCALE(MissileSpeed); }

static RACE_DESC shofixti_desc =
	{
		{/* SHIP_INFO */
		 "scout",
		 FiresFore,
		 5, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 ShofixtiRaceStrings,
		 ShofixtiIconMaskPmapAnim,
		 ShofixtiMiconMaskPmapAnim,
		 nullptr, nullptr, nullptr},
		{
			/* FLEET_STUFF */
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
			 ShofixtiBigMaskPmapAnim,
			 ShofixtiMedMaskPmapAnim,
			 ShofixtiSmlMaskPmapAnim,
		 },
		 {
			 DartBigMaskPmapAnim,
			 DartMedMaskPmapAnim,
			 DartSmlMaskPmapAnim,
		 },
		 {
			 DestructBigMaskAnim,
			 DestructMedMaskAnim,
			 DestructSmlMaskAnim,
		 },
		 {ShofixtiCaptainMaskPmapAnim,
		  nullptr, nullptr, nullptr, nullptr, nullptr,
		  0, 0, 0, 0, 0},
		 ShofixtiVictorySong,
		 ShofixtiShipSounds,
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

static uint16_t
initialize_standard_missile(ELEMENT* ShipPtr, HELEMENT MissileArray[])
{

	STARSHIP* StarShipPtr {};
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IgnoreSimilar;
	MissileBlock.pixoffs = ShofixtiOffset();
	MissileBlock.speed = MissileSpeedHd();
	MissileBlock.hit_points = MissileHits;
	MissileBlock.damage = MissileDamage;
	MissileBlock.life = MissileLife;
	MissileBlock.preprocess_func = nullptr;
	MissileBlock.blast_offs = MissileOffset();
	MissileArray[0] = initialize_missile(&MissileBlock);

	return (1);
}

static constexpr auto DestructSwitch {((NumExplosionFrames * 3) - 3)};

static void
destruct_preprocess(ELEMENT* ElementPtr)
{

	// ship_death() set the ship element's life_span to
	// (NumExplosionFrames * 3)
	PRIMITIVE* lpPrim {&DisplayArray[ElementPtr->PrimIndex]};
	ElementPtr->state_flags |= Changing;
	if (ElementPtr->life_span > DestructSwitch)
	{
		// First, stamp-fill the ship's own element with changing colors
		// for 3 frames. No explosion element yet.
		SetPrimType(lpPrim, STAMPFILL_PRIM);
		if (ElementPtr->life_span == DestructSwitch + 2)
		{
			SetPrimColor(lpPrim,
						 BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E));
		}
		else
		{
			SetPrimColor(lpPrim,
						 BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F));
		}
	}
	else if (ElementPtr->life_span < DestructSwitch)
	{
		// Stamp-fill the explosion element with cycling colors for the
		// remainder of the glory explosion frames.
		const Color color = GetPrimColor(lpPrim);

		ElementPtr->next.image.frame =
			IncFrameIndex(ElementPtr->current.image.frame);
		if (sameColor(color,
					  BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F)))
		{
			SetPrimColor(lpPrim,
						 BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E));
		}
		else if (sameColor(color,
						   BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E)))
		{
			SetPrimColor(lpPrim,
						 BuildColor(MakeRgb15(0x1F, 0x0A, 0x0A), 0x0C));
		}
		else if (sameColor(color,
						   BuildColor(MakeRgb15(0x1F, 0x0A, 0x0A), 0x0C)))
		{
			SetPrimColor(lpPrim,
						 BuildColor(MakeRgb15(0x14, 0x0A, 0x00), 0x06));
		}
		else if (sameColor(color,
						   BuildColor(MakeRgb15(0x14, 0x0A, 0x00), 0x06)))
		{
			SetPrimColor(lpPrim,
						 BuildColor(MakeRgb15(0x14, 0x00, 0x00), 0x04));
		}
	}
	else
	{
		SetPrimType(lpPrim, NoPrim);
		// The ship's own element will not be drawn anymore but will remain
		// alive all through the glory explosion.
		ElementPtr->preprocess_func = nullptr;

		// Spawn a separate glory explosion element.
		// XXX: Why? Why not keep using the ship's element?
		//   Is it because of conflicting state_flags, hit_points or
		//   mass_points?
		HELEMENT hDestruct {AllocElement()};
		if (hDestruct)
		{
			ELEMENT* DestructPtr {};
			STARSHIP* StarShipPtr {};

			GetElementStarShip(ElementPtr, &StarShipPtr);

			PutElement(hDestruct);
			LockElement(hDestruct, &DestructPtr);
			SetElementStarShip(DestructPtr, StarShipPtr);
			DestructPtr->hit_points = 0;
			DestructPtr->mass_points = 0;
			DestructPtr->playerNr = NeutralPlayerNum;
			DestructPtr->state_flags = Appearing | FiniteLife | Nonsolid;
			SetPrimType(&DisplayArray[DestructPtr->PrimIndex],
						STAMPFILL_PRIM);
			SetPrimColor(&DisplayArray[DestructPtr->PrimIndex],
						 BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F));
			DestructPtr->current.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.special;
			DestructPtr->current.image.frame =
				StarShipPtr->RaceDescPtr->ship_data.special[0];
			DestructPtr->life_span = GetFrameCount(
				DestructPtr->current.image.frame);
			DestructPtr->current.location = ElementPtr->current.location;
			DestructPtr->preprocess_func = destruct_preprocess;
			DestructPtr->postprocess_func = nullptr;
			DestructPtr->death_func = nullptr;
			ZeroVelocityComponents(&DestructPtr->velocity);
			UnlockElement(hDestruct);
		}
	}
}

/* In order to detect any Orz Marines that have boarded the ship
   when it self-destructs, we'll need to see some Orz functions */
#include "../orz/orz.h"
#include "uqm/random/UQMRandom.h"
#define ORZ_MARINE(ptr) (ptr->preprocess_func == intruder_preprocess && ptr->collision_func == marine_collision)

static void
self_destruct_kill_objects(ELEMENT* ElementPtr)
{
	// This is called during PostProcessQueue(), close to or at the end,
	// for the temporary destruct element to apply the effects of glory
	// explosion. The effects are not seen until the next frame.
	HELEMENT hNextElement {};

	for (HELEMENT hElement {GetHeadElement()}; hElement != 0; hElement = hNextElement)
	{
		ELEMENT* ObjPtr {};
		uint32_t dist {};

		LockElement(hElement, &ObjPtr);
		hNextElement = GetSuccElement(ObjPtr);

		if (!CollidingElement(ObjPtr) && !ORZ_MARINE(ObjPtr))
		{
			UnlockElement(hElement);
			continue;
		}

		int32_t delta_x {ObjPtr->next.location.x - ElementPtr->next.location.x};
		if (delta_x < 0)
		{
			delta_x = -delta_x;
		}
		int32_t delta_y {ObjPtr->next.location.y - ElementPtr->next.location.y};
		if (delta_y < 0)
		{
			delta_y = -delta_y;
		}
		delta_x = WORLD_TO_DISPLAY(delta_x);
		delta_y = WORLD_TO_DISPLAY(delta_y);
		dist = delta_x * delta_x + delta_y * delta_y;
		if (delta_x <= DestructRange() && delta_y <= DestructRange()
			&& (int)dist <= DestructRange() * DestructRange())
		{
			const int destruction = 1 + MaxDestruction * (DestructRange() - square_root(dist)) / DestructRange();

			// XXX: Why not simply call do_damage()?
			if (ObjPtr->state_flags & PlayerShip)
			{
				if (!antiCheat(ElementPtr, true, uqm::GodModeFlags::NoDamage))
				{
					if (!DeltaCrew(ObjPtr, -destruction))
					{
						ObjPtr->life_span = 0;
					}
				}
			}
			else if (!GRAVITY_MASS(ObjPtr->mass_points))
			{
				if (destruction < ObjPtr->hit_points)
				{
					ObjPtr->hit_points -= destruction;
				}
				else
				{
					ObjPtr->hit_points = 0;
					ObjPtr->life_span = 0;
				}
			}
		}

		UnlockElement(hElement);
	}
}

// This function is called when the ship dies via Glory Device.
// The generic ship_death() function is not called for the ship in this case.
static void
shofixti_destruct_death(ELEMENT* ShipPtr)
{
	STARSHIP* StarShip {};

	GetElementStarShip(ShipPtr, &StarShip);

	StopAllBattleMusic();

	StartShipExplosion(ShipPtr, false);
	// We process the explosion ourselves because it is different
	ShipPtr->preprocess_func = destruct_preprocess;

	PlaySound(GetSound(StarShip->RaceDescPtr->ship_data.ship_sounds,
					   1),
			  CalcSoundPosition(ShipPtr), ShipPtr, GameSoundPriority + 1);

	STARSHIP* winner {GetWinnerStarShip()};
	if (winner == nullptr)
	{ // No winner determined yet
		winner = FindAliveStarShip(ShipPtr);
		if (winner == nullptr)
		{ // No ships left alive after the Glory Device thus Shofixti wins
			winner = StarShip;
		}
		SetWinnerStarShip(winner);
	}
	else if (winner == StarShip)
	{ // This ship is the winner
		// It may have self-destructed before the ditty started playing,
		// and in that case, there should be no ditty
		StarShip->cur_status_flags &= ~PlayVictoryDitty;
	}
	RecordShipDeath(ShipPtr);
}

static void
self_destruct(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);

	// Spawn a temporary element, which dies in this same frame, in order
	// to defer the effects of the glory explosion.
	// It will be the last element (or one of the last) for which the
	// death_func() will be called from PostProcessQueue() in this frame.
	// XXX: Why at the end? Why not just do it now?
	HELEMENT hDestruct {AllocElement()};
	if (hDestruct)
	{
		ELEMENT* DestructPtr {};

		LockElement(hDestruct, &DestructPtr);
		DestructPtr->playerNr = ElementPtr->playerNr;
		DestructPtr->state_flags = Appearing | Nonsolid | FiniteLife;
		DestructPtr->next.location = ElementPtr->next.location;
		DestructPtr->life_span = 0;
		DestructPtr->pParent = ElementPtr->pParent;
		DestructPtr->hTarget = 0;

		DestructPtr->death_func = self_destruct_kill_objects;

		UnlockElement(hDestruct);

		PutElement(hDestruct);
	}

	// Must kill off the remaining crew ourselves
	DeltaCrew(ElementPtr, -(int)ElementPtr->crew_level);
	ZeroVelocityComponents(&ElementPtr->velocity);

	ElementPtr->state_flags |= Nonsolid;
	ElementPtr->life_span = 0;
	// The ship is now dead. It's death_func, i.e. shofixti_destruct_death(),
	// will be called the next frame.
	ElementPtr->death_func = shofixti_destruct_death;
}

static void
shofixti_intelligence(ELEMENT* ShipPtr, EVALUATE_DESC* ObjectsOfConcern,
					  uint16_t ConcernCounter)
{
	STARSHIP* StarShipPtr {};

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter != 0)
	{
		return;
	}

	if (StarShipPtr->ship_input_state & SpecialFlag)
	{
		StarShipPtr->ship_input_state &= ~SpecialFlag;
	}
	else
	{
		const EVALUATE_DESC* lpWeaponEvalDesc {&ObjectsOfConcern[ENEMY_WEAPON_INDEX]};
		const EVALUATE_DESC* lpShipEvalDesc {&ObjectsOfConcern[ENEMY_SHIP_INDEX]};
		if (StarShipPtr->RaceDescPtr->ship_data.special[0]
			&& (StarShipPtr->RaceDescPtr->ship_data.captain_control.special_offset < 4
				|| (lpShipEvalDesc->ObjectPtr != nullptr
					&& lpShipEvalDesc->which_turn <= 4)
				|| (lpWeaponEvalDesc->ObjectPtr != nullptr
					/* means IMMEDIATE WeaponFlag */
					&& (((lpWeaponEvalDesc->ObjectPtr->state_flags & PlayerShip)
						 && ShipPtr->crew_level == 1)
						|| (PlotIntercept(lpWeaponEvalDesc->ObjectPtr, ShipPtr, 2, 0)
							&& lpWeaponEvalDesc->ObjectPtr->mass_points >= ShipPtr->crew_level
							&& (uqm::random().next() & 1))))))
		{
			StarShipPtr->ship_input_state |= SpecialFlag;
		}
	}
}

static void
shofixti_preprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags
		 ^ StarShipPtr->old_status_flags)
			& SpecialFlag
		&& !(StarShipPtr->cur_status_flags & SpecialFlag))
	{
		if (!(StarShipPtr->RaceDescPtr->ship_data.captain_control.special_offset & 1))
		{
			StarShipPtr->RaceDescPtr->ship_data.captain_control.special_offset++;
		}
		else
		{
			StarShipPtr->RaceDescPtr->ship_data.captain_control.special_offset += 2;
		}
	}
}

static void
shofixti_postprocess(ELEMENT* ElementPtr)
{
	STARSHIP* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->ship_data.captain_control.special_offset == 5)
	{
		self_destruct(ElementPtr);
		StarShipPtr->cur_status_flags |= ShofixtiExplosion;
	}
}

RACE_DESC*
init_shofixti(void)
{
	RACE_DESC* RaceDescPtr {};
	// The caller of this func will copy the struct
	static RACE_DESC new_shofixti_desc;

	if (IsHd())
	{
		shofixti_desc.characteristics.max_thrust = RES_SCALE(MaxThrust);
		shofixti_desc.characteristics.thrust_increment = RES_SCALE(ThrustIncrement);
		shofixti_desc.cyborg_control.WeaponRange = MissileSpeedHd() * MissileLife;
	}
	else
	{
		shofixti_desc.characteristics.max_thrust = MaxThrust;
		shofixti_desc.characteristics.thrust_increment = ThrustIncrement;
		shofixti_desc.cyborg_control.WeaponRange =
			MissileSpeed * MissileLife;
	}

	shofixti_desc.preprocess_func = shofixti_preprocess;
	shofixti_desc.postprocess_func = shofixti_postprocess;
	shofixti_desc.init_weapon_func = initialize_standard_missile;
	shofixti_desc.cyborg_control.intelligence_func = shofixti_intelligence;

	new_shofixti_desc = shofixti_desc;
	if (uqm::lowByte(uqm::currentActivity()) == IN_ENCOUNTER
		&& CurStarDescPtr() && CurStarDescPtr()->Index == SHOFIXTI_DEFINED
		&& !GET_GAME_STATE(SHOFIXTI_RECRUITED))
	{
		// Tanaka/Katana flies in a damaged ship.
		// This is determined by being at the homeworld & not allied
		uint16_t i {};

		new_shofixti_desc.ship_data.ship_rsc[0] = OldshofBigMaskPmapAnim;
		new_shofixti_desc.ship_data.ship_rsc[1] = OldshofMedMaskPmapAnim;
		new_shofixti_desc.ship_data.ship_rsc[2] = OldshofSmlMaskPmapAnim;
		new_shofixti_desc.ship_data.special_rsc[0] = NullResource;
		new_shofixti_desc.ship_data.special_rsc[1] = NullResource;
		new_shofixti_desc.ship_data.special_rsc[2] = NullResource;
		new_shofixti_desc.ship_data.captain_control.captain_rsc =
			OldshofCaptainMaskPmapAnim;
		// JMS: Tanaka also has a corresponding limpeted ship status icon in melee.
		new_shofixti_desc.ship_info.icons_rsc = OldshofIconMaskPmapAnim;

		/* Weapon doesn't work as well */
		if (!isDifficulty(uqm::Difficulty::Hard))
		{
			new_shofixti_desc.characteristics.weapon_wait = 10;
		}

		/* Simulate VUX limpets */
		for (i = 0; i < NumLimpets; ++i)
		{
			if (!isDifficulty(uqm::Difficulty::Hard))
			{
				if (++new_shofixti_desc.characteristics.turn_wait == 0)
				{
					--new_shofixti_desc.characteristics.turn_wait;
				}
				if (++new_shofixti_desc.characteristics.thrust_wait == 0)
				{
					--new_shofixti_desc.characteristics.thrust_wait;
				}

				/* This should be the same as MinThrustIncrement() in vux.c */
				const auto MinThrustIncrement = [&] {
					return DISPLAY_TO_WORLD(RES_SCALE(1));
				};

				if (new_shofixti_desc.characteristics.thrust_increment <= MinThrustIncrement())
				{
					new_shofixti_desc.characteristics.max_thrust =
						new_shofixti_desc.characteristics.thrust_increment << 1;
				}
				else
				{
					uint16_t num_thrusts {};

					num_thrusts = new_shofixti_desc.characteristics.max_thrust / new_shofixti_desc.characteristics.thrust_increment;
					new_shofixti_desc.characteristics.thrust_increment -= RES_SCALE(1);
					new_shofixti_desc.characteristics.max_thrust =
						new_shofixti_desc.characteristics.thrust_increment * num_thrusts;
				}
			}
		}
	}

	RaceDescPtr = &new_shofixti_desc;

	return (RaceDescPtr);
}
