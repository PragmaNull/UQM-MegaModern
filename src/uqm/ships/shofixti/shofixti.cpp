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

#include "shofixti.h"

#include "../../setup.h"
#include "../../setupmenu.h"
#include "../../starmap.h"
#include "../orz/orz.h"
#include "../ship.h"
#include "libs/mathlib.h"
#include "resinst.h"
#include "uqm/globdata.h"
#include "uqm/random/UQMRandom.h"
#include "uqm/tactrans.h"

namespace uqm
{
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
static auto MissileOffset() { return ResScale(1); }
static constexpr auto MissileSpeed {DisplayToWorld(24)};
static constexpr auto MissileLife {10};
static constexpr auto MissileHits {1};
static constexpr auto MissileDamage {1};

// Glory Device
static constexpr auto SpecialEnergyCost {0};
static constexpr auto SpecialSauce {180};
static auto DestructRange() { return ResScale(SpecialSauce); }
static constexpr auto MaxDestruction {SpecialSauce / 10};

// Full game: Tanaka/Katana's damaged ships
static constexpr auto NumLimpets {3};

// HdResolutionFactor
static auto MissileSpeedHd() { return ResScale(MissileSpeed); }

static RaceDesc shofixti_desc =
	{
		{/* ShipInfoDesc */
		 "scout",
		 ShipFlags::FiresFore,
		 5, /* Super Melee cost */
		 MaxCrew, MaxCrew,
		 MaxEnergy, MaxEnergy,
		 ShofixtiRaceStrings,
		 ShofixtiIconMaskPmapAnim,
		 ShofixtiMiconMaskPmapAnim,
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

static uint16_t initialize_standard_missile(Element* ShipPtr, ElementHandle MissileArray[])
{

	Starship* StarShipPtr {};
	MissileDesc MissileBlock;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = ElementFlags::IgnoreSimilar;
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

static constexpr auto DestructSwitch {(NumExplosionFrames * 3) - 3};

static void destruct_preprocess(Element* ElementPtr)
{

	// ship_death() set the ship Element's life_span to
	// (NumExplosionFrames * 3)
	Primitive* lpPrim {&DisplayArray[ElementPtr->PrimIndex]};
	ElementPtr->state_flags |= ElementFlags::Changing;
	if (ElementPtr->life_span > DestructSwitch)
	{
		// First, stamp-fill the ship's own Element with changing colors
		// for 3 frames. No explosion Element yet.
		SetPrimType(lpPrim, PrimitiveType::StampFill);
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
		// Stamp-fill the explosion Element with cycling colors for the
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
		// The ship's own Element will not be drawn anymore but will remain
		// alive all through the glory explosion.
		ElementPtr->preprocess_func = nullptr;

		// Spawn a separate glory explosion Element.
		// XXX: Why? Why not keep using the ship's Element?
		//   Is it because of conflicting state_flags, hit_points or
		//   mass_points?
		ElementHandle hDestruct {AllocElement()};
		if (hDestruct)
		{
			Element* DestructPtr {};
			Starship* StarShipPtr {};

			GetElementStarShip(ElementPtr, &StarShipPtr);

			PutElement(hDestruct);
			LockElement(hDestruct, &DestructPtr);
			SetElementStarShip(DestructPtr, StarShipPtr);
			DestructPtr->hit_points = 0;
			DestructPtr->mass_points = 0;
			DestructPtr->playerNr = NeutralPlayerNum;
			DestructPtr->state_flags = ElementFlags::Appearing | ElementFlags::FiniteLife | ElementFlags::Nonsolid;
			SetPrimType(&DisplayArray[DestructPtr->PrimIndex],
						PrimitiveType::StampFill);
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

constexpr auto OrzMarine(const auto& ptr) { return (ptr->preprocess_func == intruder_preprocess && ptr->collision_func == marine_collision); }

static void self_destruct_kill_objects(Element* ElementPtr)
{
	// This is called during PostProcessQueue(), close to or at the end,
	// for the temporary destruct Element to apply the effects of glory
	// explosion. The effects are not seen until the next frame.
	ElementHandle hNextElement {};

	for (ElementHandle hElement {GetHeadElement()}; hElement != 0; hElement = hNextElement)
	{
		Element* ObjPtr {};
		uint32_t dist {};

		LockElement(hElement, &ObjPtr);
		hNextElement = GetSuccElement(ObjPtr);

		if (!CollidingElement(ObjPtr) && !OrzMarine(ObjPtr))
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
		delta_x = WorldToDisplay(delta_x);
		delta_y = WorldToDisplay(delta_y);
		dist = delta_x * delta_x + delta_y * delta_y;
		if (delta_x <= DestructRange() && delta_y <= DestructRange()
			&& (int)dist <= DestructRange() * DestructRange())
		{
			const int destruction = 1 + MaxDestruction * (DestructRange() - square_root(dist)) / DestructRange();

			// XXX: Why not simply call do_damage()?
			if (testFlag(ObjPtr->state_flags, ElementFlags::PlayerShip))
			{
				if (!antiCheat(ElementPtr, true, uqm::GodModeFlags::NoDamage))
				{
					if (!DeltaCrew(ObjPtr, -destruction))
					{
						ObjPtr->life_span = 0;
					}
				}
			}
			else if (!GravityMass(ObjPtr->mass_points))
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
static void shofixti_destruct_death(Element* ShipPtr)
{
	Starship* StarShip {};

	GetElementStarShip(ShipPtr, &StarShip);

	StopAllBattleMusic();

	StartShipExplosion(ShipPtr, false);
	// We process the explosion ourselves because it is different
	ShipPtr->preprocess_func = destruct_preprocess;

	PlaySound(GetSound(StarShip->RaceDescPtr->ship_data.ship_sounds,
					   1),
			  CalcSoundPosition(ShipPtr), ShipPtr, GameSoundPriority + 1);

	Starship* winner {GetWinnerStarShip()};
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
		StarShip->cur_status_flags &= ~StatusFlags::PlayVictoryDitty;
	}
	RecordShipDeath(ShipPtr);
}

static void self_destruct(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);

	// Spawn a temporary Element, which dies in this same frame, in order
	// to defer the effects of the glory explosion.
	// It will be the last Element (or one of the last) for which the
	// death_func() will be called from PostProcessQueue() in this frame.
	// XXX: Why at the end? Why not just do it now?
	ElementHandle hDestruct {AllocElement()};
	if (hDestruct)
	{
		Element* DestructPtr {};

		LockElement(hDestruct, &DestructPtr);
		DestructPtr->playerNr = ElementPtr->playerNr;
		DestructPtr->state_flags = ElementFlags::Appearing | ElementFlags::Nonsolid | ElementFlags::FiniteLife;
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

	ElementPtr->state_flags |= ElementFlags::Nonsolid;
	ElementPtr->life_span = 0;
	// The ship is now dead. It's death_func, i.e. shofixti_destruct_death(),
	// will be called the next frame.
	ElementPtr->death_func = shofixti_destruct_death;
}

static void shofixti_intelligence(Element* ShipPtr, ObjectEvaluation* ObjectsOfConcern,
								  uint16_t ConcernCounter)
{
	Starship* StarShipPtr {};

	ship_intelligence(ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter != 0)
	{
		return;
	}

	if (testFlag(StarShipPtr->ship_input_state, StatusFlags::Special))
	{
		StarShipPtr->ship_input_state &= ~StatusFlags::Special;
	}
	else
	{
		const ObjectEvaluation* lpWeaponEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyWeapon]};
		const ObjectEvaluation* lpShipEvalDesc {&ObjectsOfConcern[*IntelObjectIndex::EnemyShip]};
		if (StarShipPtr->RaceDescPtr->ship_data.special[0]
			&& (StarShipPtr->RaceDescPtr->ship_data.captain_control.special_offset < 4
				|| (lpShipEvalDesc->ObjectPtr != nullptr
					&& lpShipEvalDesc->which_turn <= 4)
				|| (lpWeaponEvalDesc->ObjectPtr != nullptr
					/* means IMMEDIATE WeaponFlag */
					&& (((testFlag(lpWeaponEvalDesc->ObjectPtr->state_flags, ElementFlags::PlayerShip))
						 && ShipPtr->crew_level == 1)
						|| (PlotIntercept(lpWeaponEvalDesc->ObjectPtr, ShipPtr, 2, 0)
							&& lpWeaponEvalDesc->ObjectPtr->mass_points >= ShipPtr->crew_level
							&& (uqm::random().next() & 1))))))
		{
			StarShipPtr->ship_input_state |= StatusFlags::Special;
		}
	}
}

static void shofixti_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (testFlag(StarShipPtr->cur_status_flags ^ StarShipPtr->old_status_flags, StatusFlags::Special)
		&& !testFlag(StarShipPtr->cur_status_flags, StatusFlags::Special))
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

static void shofixti_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->ship_data.captain_control.special_offset == 5)
	{
		self_destruct(ElementPtr);
		StarShipPtr->cur_status_flags |= StatusFlags::ShofixtiExplosion;
	}
}

RaceDesc* init_shofixti(void)
{
	RaceDesc* RaceDescPtr {};
	// The caller of this func will copy the struct
	static RaceDesc new_shofixti_desc;

	if (IsHd())
	{
		shofixti_desc.characteristics.max_thrust = ResScale(MaxThrust);
		shofixti_desc.characteristics.thrust_increment = ResScale(ThrustIncrement);
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
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InEncounter
		&& CurStarDescPtr() && CurStarDescPtr()->Index == PlotId::Shofixti
		&& !uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_RECRUITED))
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
					return DisplayToWorld(ResScale(1));
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
					new_shofixti_desc.characteristics.thrust_increment -= ResScale(1);
					new_shofixti_desc.characteristics.max_thrust =
						new_shofixti_desc.characteristics.thrust_increment * num_thrusts;
				}
			}
		}
	}

	RaceDescPtr = &new_shofixti_desc;

	return (RaceDescPtr);
}

} // namespace uqm
