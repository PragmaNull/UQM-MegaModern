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

#include "velocity.h"

#include "units.h"
#include "core/types/bytes.h"
#include "core/log/log.h"

#define VELOCITY_REMAINDER(v) ((v) & (VELOCITY_SCALE - 1))

void GetCurrentVelocityComponents(VELOCITY_DESC* velocityptr, int16_t* pdx, int16_t* pdy)
{
	*pdx = WORLD_TO_VELOCITY(velocityptr->vector.width)
		 + (velocityptr->fract.width - (int16_t)uqm::highByte(velocityptr->incr.width));
	*pdy = WORLD_TO_VELOCITY(velocityptr->vector.height)
		 + (velocityptr->fract.height - (int16_t)uqm::highByte(velocityptr->incr.height));
}

void GetCurrentVelocityComponentsSdword(VELOCITY_DESC* velocityptr, int32_t* pdx, int32_t* pdy)
{
	*pdx = WORLD_TO_VELOCITY(velocityptr->vector.width)
		 + ((int32_t)velocityptr->fract.width - (int32_t)uqm::highByte(velocityptr->incr.width));
	*pdy = WORLD_TO_VELOCITY(velocityptr->vector.height)
		 + ((int32_t)velocityptr->fract.height - (int32_t)uqm::highByte(velocityptr->incr.height));
}

void GetNextVelocityComponents(VELOCITY_DESC* velocityptr, int16_t* pdx, int16_t* pdy, uint16_t num_frames)
{
	uint16_t e;

	e = (uint16_t)((uint16_t)velocityptr->error.width + ((uint16_t)velocityptr->fract.width * num_frames));

	*pdx = (velocityptr->vector.width * num_frames)
		 + ((int16_t)((int8_t)uqm::lowByte(velocityptr->incr.width))
			* (e >> VELOCITY_SHIFT));

	velocityptr->error.width = VELOCITY_REMAINDER(e);

	e = (uint16_t)((uint16_t)velocityptr->error.height + ((uint16_t)velocityptr->fract.height * num_frames));

	*pdy = (velocityptr->vector.height * num_frames)
		 + ((int16_t)((int8_t)uqm::lowByte(velocityptr->incr.height))
			* (e >> VELOCITY_SHIFT));

	velocityptr->error.height = VELOCITY_REMAINDER(e);
}

// JMS_GFX: New function to prevent overflows in hi-res.
void GetNextVelocityComponentsSdword(VELOCITY_DESC* velocityptr, int32_t* pdx, int32_t* pdy, uint32_t num_frames)
{
	uint32_t e;

	e = (uint32_t)((uint32_t)velocityptr->error.width + ((uint32_t)velocityptr->fract.width * num_frames));

	*pdx = ((int32_t)velocityptr->vector.width * num_frames)
		 + ((int32_t)((int8_t)uqm::lowByte(velocityptr->incr.width))
			* (e >> VELOCITY_SHIFT));

	velocityptr->error.width = (uint16_t)(VELOCITY_REMAINDER(e));

	e = (uint32_t)((uint32_t)velocityptr->error.height + ((uint32_t)velocityptr->fract.height * num_frames));

	*pdy = ((int32_t)velocityptr->vector.height * num_frames)
		 + ((int32_t)((int8_t)uqm::lowByte(velocityptr->incr.height))
			* (e >> VELOCITY_SHIFT));

	velocityptr->error.height = (uint16_t)(VELOCITY_REMAINDER(e));
}

// JMS_GFX: Preventing overflows in hi-res: The int32_t in this function's parameters was int16_t.
void SetVelocityVector(VELOCITY_DESC* velocityptr, int32_t magnitude, uint16_t facing)
{
	uint16_t angle;
	int16_t dx, dy;

	angle = velocityptr->TravelAngle =
		FACING_TO_ANGLE(NORMALIZE_FACING(facing));
	magnitude = WORLD_TO_VELOCITY(magnitude);
	dx = COSINE(angle, magnitude);
	dy = SINE(angle, magnitude);
	if (dx >= 0)
	{
		velocityptr->vector.width = VELOCITY_TO_WORLD(dx);
		velocityptr->incr.width = uqm::makeWord((uint8_t)1, (uint8_t)0);
	}
	else
	{
		dx = -dx;
		velocityptr->vector.width = -VELOCITY_TO_WORLD(dx);
		velocityptr->incr.width =
			uqm::makeWord((uint8_t)0xFF, (uint8_t)(VELOCITY_REMAINDER(dx) << 1));
	}
	if (dy >= 0)
	{
		velocityptr->vector.height = VELOCITY_TO_WORLD(dy);
		velocityptr->incr.height = uqm::makeWord((uint8_t)1, (uint8_t)0);
	}
	else
	{
		dy = -dy;
		velocityptr->vector.height = -VELOCITY_TO_WORLD(dy);
		velocityptr->incr.height =
			uqm::makeWord((uint8_t)0xFF, (uint8_t)(VELOCITY_REMAINDER(dy) << 1));
	}

	velocityptr->fract.width = VELOCITY_REMAINDER(dx);
	velocityptr->fract.height = VELOCITY_REMAINDER(dy);
	velocityptr->error.width = velocityptr->error.height = 0;
}

// JMS_GFX: Preventing overflows in hi-res: The SDWORDs in this function's parameters were SIZEs.
void SetVelocityComponents(VELOCITY_DESC* velocityptr, int32_t dx, int32_t dy)
{
	uint16_t angle;

	if ((angle = ARCTAN(dx, dy)) == FULL_CIRCLE)
	{
		ZeroVelocityComponents(velocityptr);
	}
	else
	{
		if (dx >= 0)
		{
			velocityptr->vector.width = VELOCITY_TO_WORLD(dx);
			velocityptr->incr.width = uqm::makeWord((uint8_t)1, (uint8_t)0);
		}
		else
		{
			dx = -dx;
			velocityptr->vector.width = -VELOCITY_TO_WORLD(dx);
			velocityptr->incr.width =
				uqm::makeWord((uint8_t)0xFF, (uint8_t)(VELOCITY_REMAINDER(dx) << 1));
		}
		if (dy >= 0)
		{
			velocityptr->vector.height = VELOCITY_TO_WORLD(dy);
			velocityptr->incr.height = uqm::makeWord((uint8_t)1, (uint8_t)0);
		}
		else
		{
			dy = -dy;
			velocityptr->vector.height = -VELOCITY_TO_WORLD(dy);
			velocityptr->incr.height =
				uqm::makeWord((uint8_t)0xFF, (uint8_t)(VELOCITY_REMAINDER(dy) << 1));
		}

		velocityptr->fract.width = VELOCITY_REMAINDER(dx);
		velocityptr->fract.height = VELOCITY_REMAINDER(dy);
		velocityptr->error.width = velocityptr->error.height = 0;
	}

	velocityptr->TravelAngle = angle;
}

// JMS_GFX: Preventing overflows in hi-res: The SDWORDs in this function's parameters were SIZEs.
void DeltaVelocityComponents(VELOCITY_DESC* velocityptr, int32_t dx, int32_t dy)
{

	dx += WORLD_TO_VELOCITY(velocityptr->vector.width)
		+ (velocityptr->fract.width - (int16_t)uqm::highByte(velocityptr->incr.width));
	dy += WORLD_TO_VELOCITY(velocityptr->vector.height)
		+ (velocityptr->fract.height - (int16_t)uqm::highByte(velocityptr->incr.height));

	SetVelocityComponents(velocityptr, dx, dy);
}
