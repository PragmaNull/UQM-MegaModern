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

#include "element.h"
#include "init.h"
#include "intel.h"
#include "races.h"
#include "ship.h"
#include "sounds.h"
#include "status.h"
#include "uqm/random/UQMRandom.h"
#include "weapon.h"

namespace uqm
{
void spawn_planet(void)
{
	GfxPoint pt = MAKE_POINT(0, 0);

	ElementHandle hPlanetElement {AllocElement()};
	if (hPlanetElement)
	{
		Element* PlanetElementPtr {};
		extern FrameRef planet[];

		LockElement(hPlanetElement, &PlanetElementPtr);
		PlanetElementPtr->playerNr = NeutralPlayerNum;
		PlanetElementPtr->hit_points = 200;
		PlanetElementPtr->state_flags = Appearing;
		PlanetElementPtr->life_span = NormalLife + 1;
		if (uqm::UQMOptions::read().meleeObstacles && !isNetwork())
		{ // Invisible planet with cheats
			PlanetElementPtr->state_flags |= Nonsolid;
			SetPrimType(&DisplayArray[PlanetElementPtr->PrimIndex], PrimitiveType::Point);
			SetPrimColor(&DisplayArray[PlanetElementPtr->PrimIndex],
						 BuildColorRgba(0x00, 0x00, 0x00, 0x00));
			PlanetElementPtr->current.location.x =
				WrapX(DisplayAlignX(uqm::random().next()));
			PlanetElementPtr->current.location.y =
				WrapY(DisplayAlignY(uqm::random().next()));
			PlanetElementPtr->collision_func = nullptr;
			PlanetElementPtr->postprocess_func = nullptr;
			ZeroVelocityComponents(&PlanetElementPtr->velocity);
		}
		else
		{
			SetPrimType(&DisplayArray[PlanetElementPtr->PrimIndex], PrimitiveType::Stamp);
			PlanetElementPtr->current.image.farray = planet;
			PlanetElementPtr->current.image.frame =
				PlanetElementPtr->current.image.farray[0];
			PlanetElementPtr->collision_func = collision;
			PlanetElementPtr->postprocess_func =
				(void (*)(struct Element* ElementPtr))CalculateGravity;
			ZeroVelocityComponents(&PlanetElementPtr->velocity);
			do
			{
				PlanetElementPtr->current.location.x =
					WrapX(DisplayAlignX(uqm::random().next()));
				PlanetElementPtr->current.location.y =
					WrapY(DisplayAlignY(uqm::random().next()));
			} while (CalculateGravity(PlanetElementPtr)
					 || TimeSpaceMatterConflict(PlanetElementPtr));
		}
		PlanetElementPtr->mass_points = PlanetElementPtr->hit_points;
		pt = PlanetElementPtr->current.location;

		UnlockElement(hPlanetElement);

		PutElement(hPlanetElement);
	}
	if (IsExtended() && uqm::gameState().flag(uqm::GameFlag::URQUAN_PROTECTING_SAMATRA))
	{ // Works inconsistently because planet is on top star layer
		// and Sa-Matra on second, therefore scrollig is a bit off
		pt.x += 1200;
		pt.y += 780;

		pt.x = WrapX(pt.x);
		pt.y = WrapY(pt.y);

		SetStarPoint(pt, 31);
	}
}

extern FrameRef asteroid[];

static void spawn_rubble(Element* AsteroidElementPtr)
{
	ElementHandle hRubbleElement {AllocElement()};
	if (hRubbleElement)
	{
		Element* RubbleElementPtr {};

		PutElement(hRubbleElement);
		LockElement(hRubbleElement, &RubbleElementPtr);
		RubbleElementPtr->playerNr = AsteroidElementPtr->playerNr;
		RubbleElementPtr->state_flags = Appearing | FiniteLife | Nonsolid;
		RubbleElementPtr->life_span = 5;
		RubbleElementPtr->turn_wait = RubbleElementPtr->next_turn = 0;
		SetPrimType(&DisplayArray[RubbleElementPtr->PrimIndex], PrimitiveType::Stamp);
		RubbleElementPtr->current.image.farray = asteroid;


		if (!IsHd())
		{
			RubbleElementPtr->current.image.frame = SetAbsFrameIndex(asteroid[0], AngleToFacing(FullCircle));
		}
		else
		{
			RubbleElementPtr->current.image.frame = SetAbsFrameIndex(asteroid[0], 29);
		}

		RubbleElementPtr->current.location = AsteroidElementPtr->current.location;
		RubbleElementPtr->preprocess_func = animation_preprocess;
		RubbleElementPtr->death_func = spawn_asteroid;
		UnlockElement(hRubbleElement);
	}
}

static void asteroid_preprocess(Element* ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		uint16_t frame_index {GetFrameIndex(ElementPtr->current.image.frame)};
		if (ElementPtr->thrust_wait & (1 << 7))
		{
			--frame_index;
		}
		else
		{
			++frame_index;
		}


		if (!IsHd())
		{
			ElementPtr->next.image.frame = SetAbsFrameIndex(ElementPtr->current.image.frame, NormalizeFacing(frame_index));
		}
		else
		{
			ElementPtr->next.image.frame = SetAbsFrameIndex(ElementPtr->current.image.frame, frame_index % 30);
		}

		ElementPtr->state_flags |= Changing;

		ElementPtr->turn_wait = (unsigned char)(ElementPtr->thrust_wait & ((1 << 7) - 1));
	}
}

void spawn_asteroid(Element* ElementPtr)
{
	ElementHandle hAsteroidElement {};

	if ((hAsteroidElement = AllocElement()) == 0)
	{
		if (ElementPtr != 0)
		{
			ElementPtr->state_flags &= ~Disappearing;
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);
			ElementPtr->life_span = 1;
		}
	}
	else
	{
		Element* AsteroidElementPtr {};
		uint16_t val {};

		LockElement(hAsteroidElement, &AsteroidElementPtr);
		AsteroidElementPtr->playerNr = NeutralPlayerNum;
		AsteroidElementPtr->hit_points = 1;
		AsteroidElementPtr->mass_points = 3;
		AsteroidElementPtr->state_flags =
			uqm::UQMOptions::read().meleeObstacles && !isNetwork() ? Disappearing : Appearing;
		AsteroidElementPtr->life_span = NormalLife;
		SetPrimType(&DisplayArray[AsteroidElementPtr->PrimIndex], PrimitiveType::Stamp);
		if ((val = (uint16_t)uqm::random().next()) & (1 << 0))
		{
			if (!(val & (1 << 1)))
			{
				AsteroidElementPtr->current.location.x = 0;
			}
			else
			{
				AsteroidElementPtr->current.location.x = LogSpaceWidth();
			}
			AsteroidElementPtr->current.location.y =
				WrapY(DisplayAlignY(uqm::random().next()));
		}
		else
		{
			AsteroidElementPtr->current.location.x =
				WrapX(DisplayAlignX(uqm::random().next()));
			if (!(val & (1 << 1)))
			{
				AsteroidElementPtr->current.location.y = 0;
			}
			else
			{
				AsteroidElementPtr->current.location.y = LogSpaceHeight();
			}
		}

		{
			// Using these temporary variables because the execution order
			// of function arguments may vary per system, which may break
			// synchronisation on network games.
			const int16_t magnitude =
				DisplayToWorld(((int16_t)uqm::random().next() & 7) + 4);
			const uint16_t facing = (uint16_t)uqm::random().next();
			SetVelocityVector(&AsteroidElementPtr->velocity, magnitude,
							  facing);
		}
		AsteroidElementPtr->current.image.farray = asteroid;
		AsteroidElementPtr->current.image.frame =
			SetAbsFrameIndex(asteroid[0],
							 NormalizeFacing(uqm::random().next()));
		AsteroidElementPtr->turn_wait =
			AsteroidElementPtr->thrust_wait =
				(uint8_t)uqm::random().next() & (uint8_t)((1 << 2) - 1);
		AsteroidElementPtr->thrust_wait |=
			(uint8_t)uqm::random().next() & (uint8_t)(1 << 7);
		AsteroidElementPtr->preprocess_func = asteroid_preprocess;
		AsteroidElementPtr->death_func = spawn_rubble;
		AsteroidElementPtr->collision_func = collision;
		UnlockElement(hAsteroidElement);

		PutElement(hAsteroidElement);
	}
}

void do_damage(Element* ElementPtr, int16_t damage)
{
	// God Mode, borrowed from the UQM-HdResolutionFactor debug invincibility code
	if (antiCheat(ElementPtr, false, uqm::GodModeFlags::NoDamage))
	{
		damage = 0;
	}
	if (ElementPtr->state_flags & PlayerShip)
	{
		if (!DeltaCrew(ElementPtr, -damage))
		{
			ElementPtr->life_span = 0;
			ElementPtr->state_flags |= Nonsolid;
		}
	}
	else if (!GravityMass(ElementPtr->mass_points))
	{
		if ((uint8_t)damage < ElementPtr->hit_points)
		{
			ElementPtr->hit_points -= (uint8_t)damage;
		}
		else
		{
			ElementPtr->hit_points = 0;
			ElementPtr->life_span = 0;
			ElementPtr->state_flags |= Nonsolid;
		}
	}
}

static constexpr auto CrewColorLowIntensity {BuildColor(MakeRgb15(0x00, 0x14, 0x00), 0x02)};
static constexpr auto CrewColorHighIntensity {BuildColor(MakeRgb15(0x0A, 0x1E, 0x0A), 0x0A)};
void crew_preprocess(Element* ElementPtr)
{
	ElementHandle hTarget {};

	// Switch from dark to light or vice versa:
	const Color oldColor = GetPrimColor(&DisplayArray[ElementPtr->PrimIndex]);
	const Color newColor = sameColor(oldColor, CrewColorLowIntensity) ?
							   CrewColorHighIntensity :
							   CrewColorLowIntensity;
	SetPrimColor(&DisplayArray[ElementPtr->PrimIndex], newColor);

	ElementPtr->state_flags |= Changing;

	hTarget = ElementPtr->hTarget;
	if (hTarget == 0)
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		if (StarShipPtr && StarShipPtr->RaceDescPtr->ship_info.crew_level)
		{
			ElementPtr->hTarget = StarShipPtr->hShip;
		}
		else
		{
			uint16_t facing {0};
			TrackShip(ElementPtr, &facing);
		}
	}

	if (hTarget)
	{
		const auto CrewDelta = [&] {
			return ResScale(ScaledOne);
		};
		int16_t delta {};
		Element* ShipPtr {};

		LockElement(hTarget, &ShipPtr);
		delta = ShipPtr->current.location.x
			  - ElementPtr->current.location.x;
		delta = WrapDeltaX(delta);
		if (delta > 0)
		{
			ElementPtr->next.location.x += CrewDelta();
		}
		else if (delta < 0)
		{
			ElementPtr->next.location.x -= CrewDelta();
		}

		delta = ShipPtr->current.location.y - ElementPtr->current.location.y;
		delta = WrapDeltaY(delta);
		if (delta > 0)
		{
			ElementPtr->next.location.y += CrewDelta();
		}
		else if (delta < 0)
		{
			ElementPtr->next.location.y -= CrewDelta();
		}
		UnlockElement(hTarget);
	}
}

void crew_collision(Element* ElementPtr0, GfxPoint* pPt0,
					Element* ElementPtr1, GfxPoint* pPt1)
{
	if ((ElementPtr1->state_flags & PlayerShip)
		&& ElementPtr1->life_span >= NormalLife
		&& ElementPtr0->hit_points > 0)
	{
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr1, &StarShipPtr);
		if (!(StarShipPtr->RaceDescPtr->ship_info.ship_flags & CrewImmune))
		{
			ProcessSound(GetSound(GameSounds, SoundEffect::GrabCrew), ElementPtr1);
			DeltaCrew(ElementPtr1, 1);
		}
	}

	ElementPtr0->hit_points = 0;
	ElementPtr0->life_span = 0;
	ElementPtr0->state_flags |= Collision | Disappearing | Nonsolid;
	(void)pPt0; /* Satisfying compiler (unused parameter) */
	(void)pPt1; /* Satisfying compiler (unused parameter) */
}

static constexpr auto CrewLife {300};

void AbandonShip(Element* ShipPtr, Element* TargetPtr,
				 uint16_t crew_loss)
{
	int16_t dx {};
	int16_t dy {};
	uint16_t direction {};
	GfxRect r;
	Starship* StarShipPtr {};
	ElementHandle hCrew {};
	IntersectInfo ShipIntersect;

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->ship_info.ship_flags & CrewImmune)
	{
		return;
	}

	ShipIntersect = ShipPtr->IntersectControl;
	GetFrameRect(ShipIntersect.IntersectStamp.frame, &r);

	if ((direction = GetVelocityTravelAngle(
			 &ShipPtr->velocity))
		== FullCircle)
	{
		dx = dy = 0;
	}
	else
	{
		const auto MoreThanEnough = [&] {
			return ResScale(100);
		};
		direction += HalfCircle;
		dx = Cosine(direction, MoreThanEnough());
		dy = Sine(direction, MoreThanEnough());
	}

	while (crew_loss-- && (hCrew = AllocElement()))
	{
		Element* CrewPtr {};

		DeltaCrew(ShipPtr, -1);

		PutElement(hCrew);
		LockElement(hCrew, &CrewPtr);
		CrewPtr->playerNr = NeutralPlayerNum;
		CrewPtr->hit_points = 1;
		CrewPtr->state_flags = Appearing | FiniteLife | CrewObject;
		CrewPtr->life_span = CrewLife;
		if (!IsHd())
		{
			SetPrimType(&DisplayArray[CrewPtr->PrimIndex], PrimitiveType::Point);
			CrewPtr->current.image.frame = DecFrameIndex(stars_in_space);
			CrewPtr->current.image.farray = &stars_in_space;
		}
		else
		{
			SetPrimType(&DisplayArray[CrewPtr->PrimIndex], PrimitiveType::StampFill);
			CrewPtr->current.image.frame = SetAbsFrameIndex(stars_misc[0], 0);
			CrewPtr->current.image.farray = stars_misc;
		}
		SetPrimColor(&DisplayArray[CrewPtr->PrimIndex],
					 BuildColor(MakeRgb15(0x00, 0x14, 0x00), 0x02));
		CrewPtr->preprocess_func = crew_preprocess;
		CrewPtr->collision_func = crew_collision;

		SetElementStarShip(CrewPtr, StarShipPtr);

		GetElementStarShip(TargetPtr, &StarShipPtr);
		CrewPtr->hTarget = StarShipPtr->hShip;

		{
			IntersectInfo CrewIntersect;

			ShipIntersect.IntersectStamp.origin =
				ShipPtr->IntersectControl.EndPoint;

			const int16_t w {(int16_t)((uint16_t)uqm::random().next() % r.extent.width)};
			const int16_t h {(int16_t)((uint16_t)uqm::random().next() % r.extent.height)};
			CrewIntersect.EndPoint = ShipIntersect.EndPoint;
			CrewIntersect.IntersectStamp.frame = DecFrameIndex(stars_in_space);
			if (dx == 0 && dy == 0)
			{
				CrewIntersect.EndPoint.x += w - (r.extent.width >> 1);
				CrewIntersect.EndPoint.y += h - (r.extent.height >> 1);
				CrewIntersect.IntersectStamp.origin =
					TargetPtr->IntersectControl.EndPoint;
			}
			else
			{
				if (dx == 0)
				{
					CrewIntersect.EndPoint.x += w - (r.extent.width >> 1);
				}
				else if (dx > 0)
				{
					CrewIntersect.EndPoint.x += w;
				}
				else
				{
					CrewIntersect.EndPoint.x -= w;
				}
				if (dy == 0)
				{
					CrewIntersect.EndPoint.y += h - (r.extent.height >> 1);
				}
				else if (dy > 0)
				{
					CrewIntersect.EndPoint.y += h;
				}
				else
				{
					CrewIntersect.EndPoint.y -= h;
				}
				CrewIntersect.IntersectStamp.origin.x =
					CrewIntersect.EndPoint.x + dx;
				CrewIntersect.IntersectStamp.origin.y =
					CrewIntersect.EndPoint.y + dy;
			}

			DrawablesIntersect(&CrewIntersect,
							   &ShipIntersect, MaxTimeValue);

			CrewPtr->current.location.x =
				DisplayToWorld(CrewIntersect.EndPoint.x);
			CrewPtr->current.location.y =
				DisplayToWorld(CrewIntersect.EndPoint.y);
		}
		UnlockElement(hCrew);
	}
}

} // namespace uqm
