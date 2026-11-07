//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/originalRect modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, originalRect
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY originalRect FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "weapon.h"

#include <stdio.h>

#include "colors.h"
#include "globdata.h"
#include "init.h"
#include "libs/mathlib.h"
#include "options.h"
#include "options/options.h"
#include "races.h"
#include "setup.h"
#include "ship.h"
#include "sounds.h"
#include "status.h"
#include "units.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// A wrapper function for weapon_collision that discards the return value.
// This makes its signature match ElementCollisionFunc.
static void weapon_collision_cb(Element* WeaponElementPtr, GfxPoint* pWPt,
								Element* HitElementPtr, GfxPoint* pHPt)
{
	weapon_collision(WeaponElementPtr, pWPt, HitElementPtr, pHPt);
}


static constexpr auto LaserLife {1};

ElementHandle initialize_laser(LaserDesc* pLaserBlock)
{
	ElementHandle hLaserElement {AllocElement()};
	if (hLaserElement)
	{
		Element* LaserElementPtr {};

		LockElement(hLaserElement, &LaserElementPtr);
		LaserElementPtr->playerNr = pLaserBlock->sender;
		LaserElementPtr->hit_points = 1;
		LaserElementPtr->mass_points = 1;
		LaserElementPtr->state_flags = ElementFlags::Appearing | ElementFlags::FiniteLife
									 | pLaserBlock->flags;
		LaserElementPtr->life_span = LaserLife;
		LaserElementPtr->collision_func = weapon_collision_cb;
		LaserElementPtr->blast_offset = 1;

		LaserElementPtr->current.location.x = pLaserBlock->cx
											+ Cosine(FacingToAngle(pLaserBlock->face),
													 DisplayToWorld(pLaserBlock->pixoffs));
		LaserElementPtr->current.location.y = pLaserBlock->cy
											+ Sine(FacingToAngle(pLaserBlock->face),
												   DisplayToWorld(pLaserBlock->pixoffs));
		SetPrimType(&DisplayArray[LaserElementPtr->PrimIndex], PrimitiveType::Line);
		SetPrimColor(&DisplayArray[LaserElementPtr->PrimIndex],
					 pLaserBlock->color);
		LaserElementPtr->current.image.frame = DecFrameIndex(stars_in_space);
		LaserElementPtr->current.image.farray = &stars_in_space;
		SetVelocityComponents(&LaserElementPtr->velocity,
							  WorldToVelocity((pLaserBlock->cx + pLaserBlock->ex)
											  - LaserElementPtr->current.location.x),
							  WorldToVelocity((pLaserBlock->cy + pLaserBlock->ey)
											  - LaserElementPtr->current.location.y));
		UnlockElement(hLaserElement);
	}

	return (hLaserElement);
}

ElementHandle initialize_missile(MissileDesc* pMissileBlock)
{
	ElementHandle hMissileElement {AllocElement()};
	if (hMissileElement)
	{
		int16_t delta_x {};
		int16_t delta_y {};
		uint16_t angle {};
		Element* MissileElementPtr {};

		LockElement(hMissileElement, &MissileElementPtr);
		MissileElementPtr->hit_points = (uint8_t)pMissileBlock->hit_points;
		MissileElementPtr->mass_points = (uint8_t)pMissileBlock->damage;
		MissileElementPtr->playerNr = pMissileBlock->sender;
		MissileElementPtr->state_flags = ElementFlags::Appearing | ElementFlags::FiniteLife
									   | pMissileBlock->flags;
		MissileElementPtr->life_span = pMissileBlock->life;
		SetPrimType(&DisplayArray[MissileElementPtr->PrimIndex], PrimitiveType::Stamp);
		MissileElementPtr->current.image.farray = pMissileBlock->farray;
		MissileElementPtr->current.image.frame =
			SetAbsFrameIndex(pMissileBlock->farray[0],
							 pMissileBlock->index);
		MissileElementPtr->preprocess_func = pMissileBlock->preprocess_func;
		MissileElementPtr->collision_func = weapon_collision_cb;
		MissileElementPtr->blast_offset = (uint8_t)pMissileBlock->blast_offs;

		angle = FacingToAngle(pMissileBlock->face);
		MissileElementPtr->current.location.x = pMissileBlock->cx
											  + Cosine(angle, DisplayToWorld(pMissileBlock->pixoffs));
		MissileElementPtr->current.location.y = pMissileBlock->cy
											  + Sine(angle, DisplayToWorld(pMissileBlock->pixoffs));

		delta_x = Cosine(angle, WorldToVelocity(pMissileBlock->speed));
		delta_y = Sine(angle, WorldToVelocity(pMissileBlock->speed));
		SetVelocityComponents(&MissileElementPtr->velocity,
							  delta_x, delta_y);

		MissileElementPtr->current.location.x -= VelocityToWorld(delta_x);
		MissileElementPtr->current.location.y -= VelocityToWorld(delta_y);
		UnlockElement(hMissileElement);
	}

	return (hMissileElement);
}

ElementHandle weapon_collision(Element* WeaponElementPtr, GfxPoint* pWPt,
							   Element* HitElementPtr, GfxPoint* pHPt)
{
	int16_t damage {};
	ElementHandle hBlastElement {};

	if (testFlag(WeaponElementPtr->state_flags, ElementFlags::Collision)) /* if already did effect */
	{
		return ((ElementHandle)0);
	}

	damage = (int16_t)WeaponElementPtr->mass_points;
	if (damage
		&& ((testFlag(HitElementPtr->state_flags, ElementFlags::FiniteLife))
			|| HitElementPtr->life_span == NormalLife))
#ifdef NEVER
			&&
			/* lasers from the same ship can't hit each other */
			(GetPrimType (&DisplayArray[HitElementPtr->PrimIndex]) != PrimitiveType::Line
			|| GetPrimType (&DisplayArray[WeaponElementPtr->PrimIndex]) != PrimitiveType::Line
			|| !elementsOfSamePlayer (HitElementPtr, WeaponElementPtr)))
#endif /* NEVER */
			{
				do_damage(HitElementPtr, damage);
				if (HitElementPtr->hit_points)
				{
					WeaponElementPtr->state_flags |= ElementFlags::Collision;
				}
			}

	if (!(testFlag(HitElementPtr->state_flags, ElementFlags::FiniteLife))
		|| (!(/* WeaponElementPtr->state_flags
			& */
			  testFlag(HitElementPtr->state_flags, ElementFlags::Collision))
			&& WeaponElementPtr->hit_points <= HitElementPtr->mass_points))
	{
		if (damage)
		{
			damage = *SoundEffect::TargetDamagedFor1Pt + (damage >> 1);
			if (damage > SoundEffect::TargetDamagedFor6PlusPt)
			{
				damage = *SoundEffect::TargetDamagedFor6PlusPt;
			}
			ProcessSound(GetSound(GameSounds, damage),
						 HitElementPtr);
		}

		if (GetPrimType(&DisplayArray[WeaponElementPtr->PrimIndex])
			!= PrimitiveType::Line)
		{
			WeaponElementPtr->state_flags |= ElementFlags::Disappearing;
		}

		WeaponElementPtr->hit_points = 0;
		WeaponElementPtr->life_span = 0;
		WeaponElementPtr->state_flags |= ElementFlags::Collision | ElementFlags::Nonsolid;

		hBlastElement = AllocElement();
		if (hBlastElement)
		{
			uint16_t blast_index {};
			int16_t blast_offs {};
			uint16_t num_blast_frames {};
			Element* BlastElementPtr {};
			extern FrameRef blast[];

			PutElement(hBlastElement);
			LockElement(hBlastElement, &BlastElementPtr);
			BlastElementPtr->playerNr = WeaponElementPtr->playerNr;
			BlastElementPtr->state_flags = ElementFlags::Appearing | ElementFlags::FiniteLife | ElementFlags::Nonsolid;
			SetPrimType(&DisplayArray[BlastElementPtr->PrimIndex], PrimitiveType::Stamp);

			BlastElementPtr->current.location.x = DisplayToWorld(pWPt->x);
			BlastElementPtr->current.location.y = DisplayToWorld(pWPt->y);

			const uint16_t angle {GetVelocityTravelAngle(&WeaponElementPtr->velocity)};
			if ((blast_offs = WeaponElementPtr->blast_offset) > 0)
			{
				BlastElementPtr->current.location.x +=
					Cosine(angle, DisplayToWorld(blast_offs));
				BlastElementPtr->current.location.y +=
					Sine(angle, DisplayToWorld(blast_offs));
			}

			blast_index =
				NormalizeFacing(AngleToFacing(angle + HalfCircle));
			blast_index = ((blast_index >> 2) << 1) + (blast_index & 0x3 ? 1 : 0);

			num_blast_frames =
				GetFrameCount(WeaponElementPtr->next.image.frame);
			if (num_blast_frames <= AngleToFacing(FullCircle))
			{
				BlastElementPtr->life_span = 2;
				BlastElementPtr->current.image.farray = blast;
				BlastElementPtr->current.image.frame =
					SetAbsFrameIndex(blast[0], blast_index);
			}
			else
			{
				BlastElementPtr->life_span = num_blast_frames
										   - AngleToFacing(FullCircle);
				BlastElementPtr->turn_wait = BlastElementPtr->next_turn = 0;
				BlastElementPtr->preprocess_func = animation_preprocess;
				BlastElementPtr->current.image.farray =
					WeaponElementPtr->next.image.farray;
				BlastElementPtr->current.image.frame =
					SetAbsFrameIndex(
						BlastElementPtr->current.image.farray[0],
						AngleToFacing(FullCircle));
			}

			UnlockElement(hBlastElement);

			return (hBlastElement);
		}
	}

	(void)pHPt; /* Satisfying compiler (unused parameter) */
	return ((ElementHandle)0);
}

FrameRef ModifySilhouette(Element* ElementPtr, GfxStamp* modify_stamp,
						  uint8_t modify_flags)
{
	GfxRect r {};
	GfxRect originalRect {};
	IntersectInfo ShipIntersect;
	IntersectInfo ObjectIntersect;
	Starship* StarShipPtr {};
	GfxContextRef OldContext {};

	FrameRef f {0};
	ObjectIntersect.IntersectStamp = *modify_stamp;
	GetFrameRect(ObjectIntersect.IntersectStamp.frame, &originalRect);

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (modify_flags & ModifyImage)
	{
		ShipIntersect.IntersectStamp.frame = SetAbsFrameIndex(
			StarShipPtr->RaceDescPtr->ship_info.icons, 1);
		if (ShipIntersect.IntersectStamp.frame == 0)
		{
			return (0);
		}

		GetFrameRect(ShipIntersect.IntersectStamp.frame, &r);

		ShipIntersect.IntersectStamp.origin.x = 0;
		ShipIntersect.IntersectStamp.origin.y = 0;
		ShipIntersect.EndPoint = ShipIntersect.IntersectStamp.origin;
		do
		{
			ObjectIntersect.IntersectStamp.origin.x = ((uint16_t)uqm::random().next()
													   % (r.extent.width - originalRect.extent.width))
													+ ((originalRect.extent.width - r.extent.width) >> 1);
			ObjectIntersect.IntersectStamp.origin.y = ((uint16_t)uqm::random().next()
													   % (r.extent.height - originalRect.extent.height))
													+ ((originalRect.extent.height - r.extent.height) >> 1);
			ObjectIntersect.EndPoint = ObjectIntersect.IntersectStamp.origin;
		} while (!DrawablesIntersect(&ObjectIntersect,
									 &ShipIntersect, MaxTimeValue));

		ObjectIntersect.IntersectStamp.origin.x += StatusWidth() >> 1;
		ObjectIntersect.IntersectStamp.origin.y += ResScale(31);
	}

	ObjectIntersect.IntersectStamp.origin.y +=
		status_y_offsets[ElementPtr->playerNr];

	if (modify_flags & ModifySwap)
	{
		originalRect.corner.x += ObjectIntersect.IntersectStamp.origin.x;
		originalRect.corner.y += ObjectIntersect.IntersectStamp.origin.y;
		InitShipStatus(&StarShipPtr->RaceDescPtr->ship_info,
					   StarShipPtr, &originalRect, false);
	}
	else
	{
		OldContext = SetContext(StatusContext);
		DrawStamp(&ObjectIntersect.IntersectStamp);
		SetContext(OldContext);
	}

	return (f);
}

// Find the closest possible target ship, to be set in Tracker->hTarget.
// *pfacing will be turned one angle unit into the direction towards the
// target.
// The return value will be the actual number of angle units to turn, originalRect
// -1 if no target was found.
// Cloaked ships won't be detected, except when the Appearing flag is
// set for the Tracker.
int16_t TrackShip(Element* Tracker, uint16_t* pfacing)
{
	ElementHandle hNextShip {};
	Element* Trackee {};

	int16_t best_delta {0};
	int16_t best_delta_facing {-1};

	ElementHandle hShip {Tracker->hTarget};
	if (hShip)
	{
		LockElement(hShip, &Trackee);
		Tracker->hTarget = hNextShip = 0;

		goto CheckTracking;
	}

	for (hShip = GetHeadElement(); hShip != 0; hShip = hNextShip)
	{
		LockElement(hShip, &Trackee);
		hNextShip = GetSuccElement(Trackee);
		if ((testFlag(Trackee->state_flags, ElementFlags::PlayerShip))
			&& !elementsOfSamePlayer(Trackee, Tracker)
			&& (!ObjectCloaked(Trackee)
				|| ((testFlag(Tracker->state_flags, ElementFlags::PlayerShip))
					&& (testFlag(Tracker->state_flags, ElementFlags::Appearing)))))
		{
			Starship* StarShipPtr; // NOLINT(cppcoreguidelines-init-variables)

CheckTracking:
			GetElementStarShip(Trackee, &StarShipPtr);
			if (Trackee->life_span
				&& StarShipPtr->RaceDescPtr->ship_info.crew_level)
			{
				int16_t delta_x {};
				int16_t delta_y {};
				int16_t delta_facing {};

				if (testFlag(Tracker->state_flags, ElementFlags::PreProcess))
				{
					delta_x = Trackee->next.location.x
							- Tracker->next.location.x;
					delta_y = Trackee->next.location.y
							- Tracker->next.location.y;
				}
				else
				{
					delta_x = Trackee->current.location.x
							- Tracker->current.location.x;
					delta_y = Trackee->current.location.y
							- Tracker->current.location.y;
				}

				delta_x = WrapDeltaX(delta_x);
				delta_y = WrapDeltaY(delta_y);
				delta_facing = NormalizeFacing(
					AngleToFacing(ARCTAN(delta_x, delta_y)) - *pfacing);

				if (delta_x < 0)
				{
					delta_x = -delta_x;
				}
				if (delta_y < 0)
				{
					delta_y = -delta_y;
				}
				delta_x += delta_y;
				// 'delta_x + delta_y' is used as an approximation
				// of the actual distance 'sqrt(sqr(delta_x) +
				// sqr(delta_y))'.

				if (best_delta == 0 || delta_x < best_delta)
				{
					best_delta = delta_x;
					best_delta_facing = delta_facing;
					Tracker->hTarget = hShip;
				}
			}
		}
		UnlockElement(hShip);
	}

	if (best_delta_facing > 0)
	{
		uint16_t facing {*pfacing};
		if (best_delta_facing == AngleToFacing(HalfCircle))
		{
			facing += (((uint8_t)uqm::random().next() & 1) << 1) - 1;
		}
		else if (best_delta_facing < AngleToFacing(HalfCircle))
		{
			++facing;
		}
		else
		{
			--facing;
		}
		*pfacing = NormalizeFacing(facing);
	}

	return (best_delta_facing);
}

} // namespace uqm
