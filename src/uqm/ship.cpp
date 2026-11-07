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

#include "ship.h"

#include "battle.h"
#include "build.h"
#include "collide.h"
#include "colors.h"
#include "globdata.h"
#include "hyper.h"
#include "init.h"
#include "pickship.h"
#include "races.h"
#include "setup.h"
#include "sounds.h"
#include "status.h"
#include "supermelee/pickmele.h"
#include "tactrans.h"
#include "uqm/random/UQMRandom.h"
#include "util.h"


namespace uqm
{
void animation_preprocess(Element* ElementPtr)
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


StatusFlags inertial_thrust(Element* ElementPtr)
{
	const auto MaxAllowedSpeed = [&] {
		return WorldToVelocity(DisplayToWorld(ResScale(18)));
	};
	const auto MaxAllowedSpeedSqr = [&] {
		return ((uint32_t)MaxAllowedSpeed() * MaxAllowedSpeed());
	};

	uint16_t CurrentAngle {};
	uint16_t max_thrust {};
	uint16_t thrust_increment {};
	Starship* StarShipPtr {};

	VelocityDesc* VelocityPtr {&ElementPtr->velocity};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	CurrentAngle = FacingToAngle(StarShipPtr->ShipFacing);
	const uint16_t TravelAngle {GetVelocityTravelAngle(VelocityPtr)};
	thrust_increment = StarShipPtr->RaceDescPtr->characteristics.thrust_increment;
	max_thrust = StarShipPtr->RaceDescPtr->characteristics.max_thrust;
	if (thrust_increment == max_thrust)
	{ // inertialess acceleration (Skiff)
		SetVelocityVector(VelocityPtr,
						  max_thrust, StarShipPtr->ShipFacing);
		return (StatusFlags::ShipAtMaxSpeed);
	}
	else if (TravelAngle == CurrentAngle
			 && (testFlag(StarShipPtr->cur_status_flags, (StatusFlags::ShipAtMaxSpeed | StatusFlags::ShipBeyondMaxSpeed)))
			 && !(testFlag(StarShipPtr->cur_status_flags, StatusFlags::ShipInGravityWell)))
	{ // already maxed-out acceleration
		return (((StarShipPtr->cur_status_flags & (StatusFlags::ShipAtMaxSpeed | StatusFlags::ShipBeyondMaxSpeed))));
	}
	else
	{
		int16_t delta_x {};
		int16_t delta_y {};
		int16_t cur_delta_x {};
		int16_t cur_delta_y {};

		thrust_increment = WorldToVelocity(thrust_increment);
		GetCurrentVelocityComponents(VelocityPtr, &cur_delta_x, &cur_delta_y);
		const uint32_t current_speed {VelocitySquared(cur_delta_x, cur_delta_y)};
		delta_x = cur_delta_x + Cosine(CurrentAngle, thrust_increment);
		delta_y = cur_delta_y + Sine(CurrentAngle, thrust_increment);
		uint32_t desired_speed {VelocitySquared(delta_x, delta_y)};
		const uint32_t max_speed {VelocitySquared(WorldToVelocity(max_thrust), 0)};

		if (desired_speed <= max_speed)
		{ // normal acceleration
			SetVelocityComponents(VelocityPtr, delta_x, delta_y);
		}
		else if (((testFlag(StarShipPtr->cur_status_flags, StatusFlags::ShipInGravityWell))
				  && desired_speed <= MaxAllowedSpeedSqr())
				 || desired_speed < current_speed)
		{ // acceleration in a gravity well within max allowed
			// deceleration after gravity whip
			SetVelocityComponents(VelocityPtr, delta_x, delta_y);
			return (StatusFlags::ShipAtMaxSpeed | StatusFlags::ShipBeyondMaxSpeed);
		}
		else if (TravelAngle == CurrentAngle)
		{ // normal max acceleration, same vector
			if (current_speed <= max_speed)
			{
				SetVelocityVector(VelocityPtr, max_thrust,
								  StarShipPtr->ShipFacing);
			}
			return (StatusFlags::ShipAtMaxSpeed);
		}
		else
		{ // maxed-out acceleration at an angle to current travel vector
			// thrusting at an angle while at max velocity only changes
			// the travel vector, but does not really change the velocity

			VelocityDesc v = *VelocityPtr;

			DeltaVelocityComponents(&v,
									Cosine(CurrentAngle, thrust_increment >> 1)
										- Cosine(TravelAngle, thrust_increment),
									Sine(CurrentAngle, thrust_increment >> 1)
										- Sine(TravelAngle, thrust_increment));
			GetCurrentVelocityComponents(&v, &cur_delta_x, &cur_delta_y);
			desired_speed = VelocitySquared(cur_delta_x, cur_delta_y);
			if (desired_speed > max_speed)
			{
				if (desired_speed < current_speed)
				{
					*VelocityPtr = v;
				}
				return (StatusFlags::ShipAtMaxSpeed | StatusFlags::ShipBeyondMaxSpeed);
			}

			*VelocityPtr = v;
		}

		return StatusFlags::None;
	}
}

void DrawHDMeleeBorder(Starship* StarShipPtr)
{
	int16_t y = 0;
	GfxRect r;

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle
		&& StarShipPtr->playerNr != RpgPlayerNum)
	{
		return;
	}

	y = status_y_offsets[StarShipPtr->playerNr];

	r.corner.x = 0;
	r.corner.y = 4 + y;
	r.extent.width = StatusWidth();
	r.extent.height = ShipStatusHeight() - 4;

	DrawRenderedBox(&r, false, NullColor, BevelLevel::ThickOuterBevel, false);
}

void ship_preprocess(Element* ElementPtr)
{
	StatusFlags cur_status_flags {};
	Starship* StarShipPtr {};

	GetElementStarShip(ElementPtr, &StarShipPtr);
	RaceDesc* RDPtr {StarShipPtr->RaceDescPtr};

	cur_status_flags =
		StarShipPtr->cur_status_flags
		& ~(StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon | StatusFlags::Special);
	if (!(testFlag(ElementPtr->state_flags, ElementFlags::Appearing)))
	{
		cur_status_flags |= ((StarShipPtr->ship_input_state & (StatusFlags::Left | StatusFlags::Right | StatusFlags::Thrust | StatusFlags::Weapon | StatusFlags::Special)));
	}
	else
	{ // Preprocessing for the first time
		ElementPtr->crew_level = RDPtr->ship_info.crew_level;

		if (ElementPtr->playerNr == NpcPlayerNum
			&& uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
		{ // Sa-Matra
			GfxStamp s;

			const ContextScope contextScope {StatusContext};
			s.origin.x = s.origin.y = 0;
			s.frame = RDPtr->ship_data.captain_control.background;
			DrawStamp(&s);
			DestroyDrawable(ReleaseDrawable(s.frame));
			RDPtr->ship_data.captain_control.background = 0;
		}
		else if (uqm::activityKind(uqm::currentActivity()) <= ActivityKind::InEncounter)
		{
			InitShipStatus(&RDPtr->ship_info, StarShipPtr, nullptr, false);
			{
				const ContextScope contextScope {StatusContext};
				DrawCaptainsWindow(StarShipPtr);

				if (IsHd())
				{
					DrawHDMeleeBorder(StarShipPtr);
				}
			}
			if (RDPtr->preprocess_func)
			{
				(*RDPtr->preprocess_func)(ElementPtr);
			}

			// XXX: Hack: Pkunk sets hTarget!=0 when it reincarnates. In that
			//   case there is no ship_transition() but a Phoenix transition
			//   instead.
			if (ElementPtr->hTarget == 0)
			{
				ship_transition(ElementPtr);
			}
			else
			{
				ElementPtr->hTarget = 0;
				if (!PLRPlaying((MusicRef)~0) && OpponentAlive(StarShipPtr))
				{
					BattleSong(true);
				}
			}
			return;
		}
		else
		{
			ElementPtr->current.location.x = LogSpaceWidth() >> 1;
			ElementPtr->current.location.y = LogSpaceHeight() >> 1;
			ElementPtr->next.location = ElementPtr->current.location;
			InitIntersectStartPoint(ElementPtr);
			InitIntersectEndPoint(ElementPtr);

			if (hyper_transition(ElementPtr))
			{
				return;
			}
		}
	}
	StarShipPtr->cur_status_flags = cur_status_flags;

	if (StarShipPtr->energy_counter)
	{
		--StarShipPtr->energy_counter;
	}
	else if (RDPtr->ship_info.energy_level < (uint8_t)RDPtr->ship_info.max_energy
			 || (int8_t)RDPtr->characteristics.energy_regeneration < 0)
	{
		DeltaEnergy(ElementPtr,
					(int8_t)RDPtr->characteristics.energy_regeneration);
	}

	if (RDPtr->preprocess_func)
	{
		(*RDPtr->preprocess_func)(ElementPtr);
		cur_status_flags = StarShipPtr->cur_status_flags;
	}

	if (ElementPtr->turn_wait)
	{
		--ElementPtr->turn_wait;
	}
	else if (testFlag(cur_status_flags, (StatusFlags::Left | StatusFlags::Right)))
	{
		if (testFlag(cur_status_flags, StatusFlags::Left))
		{
			StarShipPtr->ShipFacing =
				NormalizeFacing(StarShipPtr->ShipFacing - 1);
		}
		else
		{
			StarShipPtr->ShipFacing =
				NormalizeFacing(StarShipPtr->ShipFacing + 1);
		}
		ElementPtr->next.image.frame =
			SetAbsFrameIndex(ElementPtr->next.image.frame,
							 StarShipPtr->ShipFacing);
		ElementPtr->state_flags |= ElementFlags::Changing;

		ElementPtr->turn_wait = RDPtr->characteristics.turn_wait;
	}

	if (ElementPtr->thrust_wait)
	{
		--ElementPtr->thrust_wait;
	}
	else if (testFlag(cur_status_flags, StatusFlags::Thrust))
	{
		const StatusFlags thrust_status {inertial_thrust(ElementPtr)};
		StarShipPtr->cur_status_flags &=
			~(StatusFlags::ShipAtMaxSpeed
			  | StatusFlags::ShipBeyondMaxSpeed
			  | StatusFlags::ShipInGravityWell);
		StarShipPtr->cur_status_flags |= thrust_status;

		ElementPtr->thrust_wait = RDPtr->characteristics.thrust_wait;

		if (!ObjectCloaked(ElementPtr)
			&& uqm::activityKind(uqm::currentActivity()) <= ActivityKind::InEncounter)
		{
			spawn_ion_trail(ElementPtr, 0, 0);
		}
	}

	if (uqm::activityKind(uqm::currentActivity()) <= ActivityKind::InEncounter)
	{
		PreProcessStatus(ElementPtr);
	}
}

void ship_postprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	if (ElementPtr->crew_level == 0)
	{
		return;
	}

	GetElementStarShip(ElementPtr, &StarShipPtr);
	const RaceDesc* RDPtr {StarShipPtr->RaceDescPtr};

	if (StarShipPtr->weapon_counter)
	{
		--StarShipPtr->weapon_counter;
	}
	else if ((testFlag(StarShipPtr->cur_status_flags, StatusFlags::Weapon))
			 && DeltaEnergy(ElementPtr,
							-RDPtr->characteristics.weapon_energy_cost))
	{
		uqstl::array<ElementHandle, 6> Weapon;

		uint16_t num_weapons {(*RDPtr->init_weapon_func)(ElementPtr, Weapon.data())};

		if (num_weapons)
		{
			Starship* StarShipPtr {};
			bool played_sfx = false;

			GetElementStarShip(ElementPtr, &StarShipPtr);
			const ElementHandle* WeaponPtr {&Weapon[0]};
			do
			{
				ElementHandle w {*WeaponPtr++};
				if (w)
				{
					Element* EPtr {};

					LockElement(w, &EPtr);
					SetElementStarShip(EPtr, StarShipPtr);
					if (!played_sfx)
					{
						ProcessSound(GetSound(RDPtr->ship_data.ship_sounds, RDPtr->ship_data.weapon_sound_index), EPtr);
						played_sfx = true;
					}
					UnlockElement(w);

					PutElement(w);
				}
			} while (--num_weapons);

			if (!played_sfx)
			{
				ProcessSound(GetSound(RDPtr->ship_data.ship_sounds, RDPtr->ship_data.weapon_sound_index), ElementPtr);
			}
		}

		StarShipPtr->weapon_counter =
			RDPtr->characteristics.weapon_wait;
	}

	if (StarShipPtr->special_counter)
	{
		--StarShipPtr->special_counter;
	}

	if (RDPtr->postprocess_func)
	{
		(*RDPtr->postprocess_func)(ElementPtr);
	}

	if (uqm::activityKind(uqm::currentActivity()) <= ActivityKind::InEncounter)
	{
		PostProcessStatus(ElementPtr);
	}
}

void collision(Element* ElementPtr0, GfxPoint* pPt0,
			   Element* ElementPtr1, GfxPoint* pPt1)
{
	if (!(testFlag(ElementPtr1->state_flags, ElementFlags::FiniteLife)))
	{
		ElementPtr0->state_flags |= ElementFlags::Collision;
		if (GravityMass(ElementPtr1->mass_points))
		{
			// Collision with a planet.
			int16_t damage {};

			damage = ElementPtr0->hit_points >> 2;
			if (damage == 0)
			{
				damage = 1;
			}
			do_damage(ElementPtr0, damage);

			damage = *SoundEffect::TargetDamagedFor1Pt + (damage >> 1);
			if (damage > SoundEffect::TargetDamagedFor6PlusPt)
			{
				damage = *SoundEffect::TargetDamagedFor6PlusPt;
			}
			ProcessSound(GetSound(GameSounds, damage), ElementPtr0);
		}
	}
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static bool spawn_ship(Starship* StarShipPtr)
{
	ElementHandle hShip {};

	RaceDesc* RDPtr {load_ship(StarShipPtr->SpeciesID, true)};
	if (!RDPtr)
	{
		return false;
	}

	StarShipPtr->RaceDescPtr = RDPtr;

	StarShipPtr->ship_input_state = StatusFlags::None;
	StarShipPtr->cur_status_flags = StatusFlags::None;
	StarShipPtr->old_status_flags = StatusFlags::None;

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InEncounter
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
	{
		if (StarShipPtr->crew_level == 0)
		{
			// SIS, already handled from sis_ship.c.
			// RDPtr->ship_info.crew_level = GLOBAL_SIS (CrewEnlisted);
		}
		else
		{
			RDPtr->ship_info.crew_level = StarShipPtr->crew_level;
		}

		if (RDPtr->ship_info.crew_level > RDPtr->ship_info.max_crew)
		{
			RDPtr->ship_info.crew_level = RDPtr->ship_info.max_crew;
		}
	}

	StarShipPtr->energy_counter = 0;
	StarShipPtr->weapon_counter = 0;
	StarShipPtr->special_counter = 0;

	hShip = StarShipPtr->hShip;
	if (hShip == 0)
	{
		hShip = AllocElement();
		if (hShip != 0)
		{
			InsertElement(hShip, GetHeadElement());
		}
	}

	StarShipPtr->hShip = hShip;
	if (StarShipPtr->hShip != 0)
	{
		// Construct an Element for the Starship
		Element* ShipElementPtr {};

		LockElement(hShip, &ShipElementPtr);

		ShipElementPtr->playerNr = StarShipPtr->playerNr;
		ShipElementPtr->crew_level = 0;
		ShipElementPtr->mass_points = RDPtr->characteristics.ship_mass;
		ShipElementPtr->state_flags = ElementFlags::Appearing | ElementFlags::PlayerShip | ElementFlags::IgnoreSimilar;
		ShipElementPtr->turn_wait = 0;
		ShipElementPtr->thrust_wait = 0;
		ShipElementPtr->life_span = NormalLife;
		ShipElementPtr->colorCycleIndex = 0;

		SetPrimType(&DisplayArray[ShipElementPtr->PrimIndex], PrimitiveType::Stamp);
		ShipElementPtr->current.image.farray = RDPtr->ship_data.ship.data();

		if (ShipElementPtr->playerNr == NpcPlayerNum
			&& uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
		{
			// This is the Sa-Matra
			StarShipPtr->ShipFacing = 0;
			ShipElementPtr->current.image.frame =
				SetAbsFrameIndex(RDPtr->ship_data.ship[0],
								 StarShipPtr->ShipFacing);
			ShipElementPtr->current.location.x = LogSpaceWidth() >> 1;
			ShipElementPtr->current.location.y = LogSpaceHeight() >> 1;
			++ShipElementPtr->life_span;
		}
		else
		{
			StarShipPtr->ShipFacing = NormalizeFacing(uqm::random().next());
			if (inHQSpace())
			{ // Only one ship is ever spawned in HyperSpace -- flagship
				uint16_t facing = uqm::flagship().shipFacing;
				// XXX: Solar system reentry test depends on ShipFacing != 0
				if (facing > 0)
				{
					--facing;
				}

				// XXX: This appears to set the facing to a random value
				//   for when the ship returns from an Encounter back
				//   to HyperSpace. However, it is overwritten later
				//   in sis.c. See also r1614.
				//GLOBAL (ShipFacing) = StarShipPtr->ShipFacing + 1;
				StarShipPtr->ShipFacing = facing;

				if (IsHd())
				{
					const uint16_t numFrames = GetFrameCount(RDPtr->ship_data.ship[0]);
					const DrawMode mode = MAKE_DRAW_MODE(DrawKind::Linearburn, 0xFF);
					Color c;

					if (inHyperSpace())
					{
						c = BuildColorRgb(0xFF, 0x00, 0x00);
					}
					else
					{
						c = BuildColorRgb(0x00, 0xE4, 0x00);
					}

					for (uint16_t i {0}; i < numFrames; i++)
					{
						ApplyMask(nullptr, SetAbsFrameIndex(RDPtr->ship_data.ship[0], i), mode, &c);
					}
				}
			}
			ShipElementPtr->current.image.frame =
				SetAbsFrameIndex(RDPtr->ship_data.ship[0],
								 StarShipPtr->ShipFacing);
			do
			{
				ShipElementPtr->current.location.x =
					WrapX(DisplayAlignX(uqm::random().next()));
				ShipElementPtr->current.location.y =
					WrapY(DisplayAlignY(uqm::random().next()));
			} while (CalculateGravity(ShipElementPtr)
					 || TimeSpaceMatterConflict(ShipElementPtr));
		}

		ShipElementPtr->preprocess_func = ship_preprocess;
		ShipElementPtr->postprocess_func = ship_postprocess;
		ShipElementPtr->death_func = ship_death;
		ShipElementPtr->collision_func = collision;
		ZeroVelocityComponents(&ShipElementPtr->velocity);

		SetElementStarShip(ShipElementPtr, StarShipPtr);
		ShipElementPtr->hTarget = 0;

		UnlockElement(hShip);
	}

	return (hShip != 0);
}

// Select a new ship and spawn it.
bool GetNextStarShip(Starship* LastStarShipPtr, uint16_t which_side)
{
	StarshipHandle hBattleShip {GetEncounterStarShip(LastStarShipPtr, which_side)};
	if (hBattleShip)
	{
		Starship* StarShipPtr {LockStarShip(&race_q[which_side], hBattleShip)};
		if (LastStarShipPtr)
		{
			if (StarShipPtr == LastStarShipPtr)
			{
				// Ship has been recycled (on infinite ship worlds).
				LastStarShipPtr = 0;
			}
			else
			{
				StarShipPtr->hShip = LastStarShipPtr->hShip;
			}
		}

		if (!spawn_ship(StarShipPtr))
		{
			UnlockStarShip(&race_q[which_side], hBattleShip);
			return (false);
		}
		UnlockStarShip(&race_q[which_side], hBattleShip);
	}

	if (LastStarShipPtr)
	{
		LastStarShipPtr->hShip = 0;
	}

	return (hBattleShip != 0);
}

bool GetInitialStarShips(void)
{
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::SuperMelee)
	{
		uqstl::array<StarshipHandle, NumPlayers> ships;

		if (!GetInitialMeleeStarShips(ships.data()))
		{
			return false;
		}

		for (uint16_t i {0}; i < NumPlayers; i++)
		{
			const uint16_t playerI = GetPlayerOrder(i);

			Starship* StarShipPtr {LockStarShip(&race_q[playerI], ships[playerI])};
			if (!spawn_ship(StarShipPtr))
			{
				UnlockStarShip(&race_q[playerI], ships[playerI]);
				return false;
			}
			UnlockStarShip(&race_q[playerI], ships[playerI]);
		}
		return true;
	}
	else
	{
		for (int i {NumPlayers}; i > 0; --i)
		{
			if (!GetNextStarShip(nullptr, i - 1))
			{
				return false;
			}
		}
		return true;
	}
}

} // namespace uqm
