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

#include "options/options.h"
#include "options.h"
#include "colors.h"
#include "globdata.h"
#include "status.h"
#include "init.h"
#include "races.h"
#include "ship.h"
#include "setup.h"
#include "sounds.h"
#include "units.h"
#include "libs/mathlib.h"

#include <stdio.h>
#include "uqm/random/UQMRandom.h"

// A wrapper function for weapon_collision that discards the return value.
// This makes its signature match ElementCollisionFunc.
static void
weapon_collision_cb(ELEMENT* WeaponElementPtr, GFXPOINT* pWPt,
					ELEMENT* HitElementPtr, GFXPOINT* pHPt)
{
	weapon_collision(WeaponElementPtr, pWPt, HitElementPtr, pHPt);
}


HELEMENT
initialize_laser(LASER_BLOCK* pLaserBlock)
{
	HELEMENT hLaserElement {AllocElement()};
	if (hLaserElement)
	{
#define LASER_LIFE 1
		ELEMENT* LaserElementPtr {};

		LockElement(hLaserElement, &LaserElementPtr);
		LaserElementPtr->playerNr = pLaserBlock->sender;
		LaserElementPtr->hit_points = 1;
		LaserElementPtr->mass_points = 1;
		LaserElementPtr->state_flags = APPEARING | FINITE_LIFE
									 | pLaserBlock->flags;
		LaserElementPtr->life_span = LASER_LIFE;
		LaserElementPtr->collision_func = weapon_collision_cb;
		LaserElementPtr->blast_offset = 1;

		LaserElementPtr->current.location.x = pLaserBlock->cx
											+ COSINE(FACING_TO_ANGLE(pLaserBlock->face),
													 DISPLAY_TO_WORLD(pLaserBlock->pixoffs));
		LaserElementPtr->current.location.y = pLaserBlock->cy
											+ SINE(FACING_TO_ANGLE(pLaserBlock->face),
												   DISPLAY_TO_WORLD(pLaserBlock->pixoffs));
		SetPrimType(&DisplayArray[LaserElementPtr->PrimIndex], LINE_PRIM);
		SetPrimColor(&DisplayArray[LaserElementPtr->PrimIndex],
					 pLaserBlock->color);
		LaserElementPtr->current.image.frame = DecFrameIndex(stars_in_space);
		LaserElementPtr->current.image.farray = &stars_in_space;
		SetVelocityComponents(&LaserElementPtr->velocity,
							  WORLD_TO_VELOCITY((pLaserBlock->cx + pLaserBlock->ex)
												- LaserElementPtr->current.location.x),
							  WORLD_TO_VELOCITY((pLaserBlock->cy + pLaserBlock->ey)
												- LaserElementPtr->current.location.y));
		UnlockElement(hLaserElement);
	}

	return (hLaserElement);
}

HELEMENT
initialize_missile(MISSILE_BLOCK* pMissileBlock)
{
	HELEMENT hMissileElement {AllocElement()};
	if (hMissileElement)
	{
		int16_t delta_x {};
		int16_t delta_y {};
		uint16_t angle {};
		ELEMENT* MissileElementPtr {};

		LockElement(hMissileElement, &MissileElementPtr);
		MissileElementPtr->hit_points = (uint8_t)pMissileBlock->hit_points;
		MissileElementPtr->mass_points = (uint8_t)pMissileBlock->damage;
		MissileElementPtr->playerNr = pMissileBlock->sender;
		MissileElementPtr->state_flags = APPEARING | FINITE_LIFE
									   | pMissileBlock->flags;
		MissileElementPtr->life_span = pMissileBlock->life;
		SetPrimType(&DisplayArray[MissileElementPtr->PrimIndex], STAMP_PRIM);
		MissileElementPtr->current.image.farray = pMissileBlock->farray;
		MissileElementPtr->current.image.frame =
			SetAbsFrameIndex(pMissileBlock->farray[0],
							 pMissileBlock->index);
		MissileElementPtr->preprocess_func = pMissileBlock->preprocess_func;
		MissileElementPtr->collision_func = weapon_collision_cb;
		MissileElementPtr->blast_offset = (uint8_t)pMissileBlock->blast_offs;

		angle = FACING_TO_ANGLE(pMissileBlock->face);
		MissileElementPtr->current.location.x = pMissileBlock->cx
											  + COSINE(angle, DISPLAY_TO_WORLD(pMissileBlock->pixoffs));
		MissileElementPtr->current.location.y = pMissileBlock->cy
											  + SINE(angle, DISPLAY_TO_WORLD(pMissileBlock->pixoffs));

		delta_x = COSINE(angle, WORLD_TO_VELOCITY(pMissileBlock->speed));
		delta_y = SINE(angle, WORLD_TO_VELOCITY(pMissileBlock->speed));
		SetVelocityComponents(&MissileElementPtr->velocity,
							  delta_x, delta_y);

		MissileElementPtr->current.location.x -= VELOCITY_TO_WORLD(delta_x);
		MissileElementPtr->current.location.y -= VELOCITY_TO_WORLD(delta_y);
		UnlockElement(hMissileElement);
	}

	return (hMissileElement);
}

HELEMENT
weapon_collision(ELEMENT* WeaponElementPtr, GFXPOINT* pWPt,
				 ELEMENT* HitElementPtr, GFXPOINT* pHPt)
{
	int16_t damage {};
	HELEMENT hBlastElement {};

	if (WeaponElementPtr->state_flags & COLLISION) /* if already did effect */
	{
		return ((HELEMENT)0);
	}

	damage = (int16_t)WeaponElementPtr->mass_points;
	if (damage
		&& ((HitElementPtr->state_flags & FINITE_LIFE)
			|| HitElementPtr->life_span == NORMAL_LIFE))
#ifdef NEVER
			&&
			/* lasers from the same ship can't hit each other */
			(GetPrimType (&DisplayArray[HitElementPtr->PrimIndex]) != LINE_PRIM
			|| GetPrimType (&DisplayArray[WeaponElementPtr->PrimIndex]) != LINE_PRIM
			|| !elementsOfSamePlayer (HitElementPtr, WeaponElementPtr)))
#endif /* NEVER */
			{
				do_damage(HitElementPtr, damage);
				if (HitElementPtr->hit_points)
				{
					WeaponElementPtr->state_flags |= COLLISION;
				}
			}

	if (!(HitElementPtr->state_flags & FINITE_LIFE)
		|| (!(/* WeaponElementPtr->state_flags
			& */
			  HitElementPtr->state_flags
			  & COLLISION)
			&& WeaponElementPtr->hit_points <= HitElementPtr->mass_points))
	{
		if (damage)
		{
			damage = TARGET_DAMAGED_FOR_1_PT + (damage >> 1);
			if (damage > TARGET_DAMAGED_FOR_6_PLUS_PT)
			{
				damage = TARGET_DAMAGED_FOR_6_PLUS_PT;
			}
			ProcessSound(GetSound(GameSounds, damage),
						 HitElementPtr);
		}

		if (GetPrimType(&DisplayArray[WeaponElementPtr->PrimIndex])
			!= LINE_PRIM)
		{
			WeaponElementPtr->state_flags |= DISAPPEARING;
		}

		WeaponElementPtr->hit_points = 0;
		WeaponElementPtr->life_span = 0;
		WeaponElementPtr->state_flags |= COLLISION | NONSOLID;

		hBlastElement = AllocElement();
		if (hBlastElement)
		{
			uint16_t blast_index {};
			int16_t blast_offs {};
			uint16_t num_blast_frames {};
			ELEMENT* BlastElementPtr {};
			extern FRAME blast[];

			PutElement(hBlastElement);
			LockElement(hBlastElement, &BlastElementPtr);
			BlastElementPtr->playerNr = WeaponElementPtr->playerNr;
			BlastElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			SetPrimType(&DisplayArray[BlastElementPtr->PrimIndex], STAMP_PRIM);

			BlastElementPtr->current.location.x = DISPLAY_TO_WORLD(pWPt->x);
			BlastElementPtr->current.location.y = DISPLAY_TO_WORLD(pWPt->y);

			const uint16_t angle {GetVelocityTravelAngle(&WeaponElementPtr->velocity)};
			if ((blast_offs = WeaponElementPtr->blast_offset) > 0)
			{
				BlastElementPtr->current.location.x +=
					COSINE(angle, DISPLAY_TO_WORLD(blast_offs));
				BlastElementPtr->current.location.y +=
					SINE(angle, DISPLAY_TO_WORLD(blast_offs));
			}

			blast_index =
				NORMALIZE_FACING(ANGLE_TO_FACING(angle + HALF_CIRCLE));
			blast_index = ((blast_index >> 2) << 1) + (blast_index & 0x3 ? 1 : 0);

			num_blast_frames =
				GetFrameCount(WeaponElementPtr->next.image.frame);
			if (num_blast_frames <= ANGLE_TO_FACING(FULL_CIRCLE))
			{
				BlastElementPtr->life_span = 2;
				BlastElementPtr->current.image.farray = blast;
				BlastElementPtr->current.image.frame =
					SetAbsFrameIndex(blast[0], blast_index);
			}
			else
			{
				BlastElementPtr->life_span = num_blast_frames
										   - ANGLE_TO_FACING(FULL_CIRCLE);
				BlastElementPtr->turn_wait = BlastElementPtr->next_turn = 0;
				BlastElementPtr->preprocess_func = animation_preprocess;
				BlastElementPtr->current.image.farray =
					WeaponElementPtr->next.image.farray;
				BlastElementPtr->current.image.frame =
					SetAbsFrameIndex(
						BlastElementPtr->current.image.farray[0],
						ANGLE_TO_FACING(FULL_CIRCLE));
			}

			UnlockElement(hBlastElement);

			return (hBlastElement);
		}
	}

	(void)pHPt; /* Satisfying compiler (unused parameter) */
	return ((HELEMENT)0);
}

FRAME
ModifySilhouette(ELEMENT* ElementPtr, STAMP* modify_stamp,
				 uint8_t modify_flags)
{
	GFXRECT r {};
	GFXRECT originalRect {};
	INTERSECT_CONTROL ShipIntersect;
	INTERSECT_CONTROL ObjectIntersect;
	STARSHIP* StarShipPtr {};
	GFXCONTEXT OldContext {};

	FRAME f {0};
	ObjectIntersect.IntersectStamp = *modify_stamp;
	GetFrameRect(ObjectIntersect.IntersectStamp.frame, &originalRect);

	GetElementStarShip(ElementPtr, &StarShipPtr);
	if (modify_flags & MODIFY_IMAGE)
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

		ObjectIntersect.IntersectStamp.origin.x += STATUS_WIDTH >> 1;
		ObjectIntersect.IntersectStamp.origin.y += RES_SCALE(31);
	}

	ObjectIntersect.IntersectStamp.origin.y +=
		status_y_offsets[ElementPtr->playerNr];

	if (modify_flags & MODIFY_SWAP)
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
// Cloaked ships won't be detected, except when the APPEARING flag is
// set for the Tracker.
int16_t
TrackShip(ELEMENT* Tracker, uint16_t* pfacing)
{
	HELEMENT hNextShip {};
	ELEMENT* Trackee {};

	int16_t best_delta {0};
	int16_t best_delta_facing {-1};

	HELEMENT hShip {Tracker->hTarget};
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
		if ((Trackee->state_flags & PLAYER_SHIP)
			&& !elementsOfSamePlayer(Trackee, Tracker)
			&& (!OBJECT_CLOAKED(Trackee)
				|| ((Tracker->state_flags & PLAYER_SHIP)
					&& (Tracker->state_flags & APPEARING))))
		{
			STARSHIP* StarShipPtr; // NOLINT(cppcoreguidelines-init-variables)

CheckTracking:
			GetElementStarShip(Trackee, &StarShipPtr);
			if (Trackee->life_span
				&& StarShipPtr->RaceDescPtr->ship_info.crew_level)
			{
				int16_t delta_x {};
				int16_t delta_y {};
				int16_t delta_facing {};

				if (Tracker->state_flags & PRE_PROCESS)
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

				delta_x = WRAP_DELTA_X(delta_x);
				delta_y = WRAP_DELTA_Y(delta_y);
				delta_facing = NORMALIZE_FACING(
					ANGLE_TO_FACING(ARCTAN(delta_x, delta_y)) - *pfacing);

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
		if (best_delta_facing == ANGLE_TO_FACING(HALF_CIRCLE))
		{
			facing += (((uint8_t)uqm::random().next() & 1) << 1) - 1;
		}
		else if (best_delta_facing < ANGLE_TO_FACING(HALF_CIRCLE))
		{
			++facing;
		}
		else
		{
			--facing;
		}
		*pfacing = NORMALIZE_FACING(facing);
	}

	return (best_delta_facing);
}
