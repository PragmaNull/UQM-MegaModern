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
#include "core/misc/bittricks.h"

namespace uqm
{
constexpr auto VelocityRemainder(const auto& v) { return ((v) & (VelocityScale - 1)); }

void GetCurrentVelocityComponents(VelocityDesc* velocityptr, int16_t* pdx, int16_t* pdy)
{
	*pdx = WorldToVelocity(velocityptr->vector.width)
		 + (velocityptr->fract.width - (int16_t)uqm::highByte(velocityptr->incr.width));
	*pdy = WorldToVelocity(velocityptr->vector.height)
		 + (velocityptr->fract.height - (int16_t)uqm::highByte(velocityptr->incr.height));
}

void GetCurrentVelocityComponentsSdword(VelocityDesc* velocityptr, int32_t* pdx, int32_t* pdy)
{
	*pdx = WorldToVelocity(velocityptr->vector.width)
		 + ((int32_t)velocityptr->fract.width - (int32_t)uqm::highByte(velocityptr->incr.width));
	*pdy = WorldToVelocity(velocityptr->vector.height)
		 + ((int32_t)velocityptr->fract.height - (int32_t)uqm::highByte(velocityptr->incr.height));
}

void GetNextVelocityComponents(VelocityDesc* velocityptr, int16_t* pdx, int16_t* pdy, uint16_t num_frames)
{
	uint16_t e {(uint16_t)((uint16_t)velocityptr->error.width + ((uint16_t)velocityptr->fract.width * num_frames))};

	*pdx = (velocityptr->vector.width * num_frames)
		 + ((int16_t)((int8_t)uqm::lowByte(velocityptr->incr.width))
			* (e >> VelocityShift));

	velocityptr->error.width = VelocityRemainder(e);

	e = (uint16_t)((uint16_t)velocityptr->error.height + ((uint16_t)velocityptr->fract.height * num_frames));

	*pdy = (velocityptr->vector.height * num_frames)
		 + ((int16_t)((int8_t)uqm::lowByte(velocityptr->incr.height))
			* (e >> VelocityShift));

	velocityptr->error.height = VelocityRemainder(e);
}

// JMS_GFX: New function to prevent overflows in hi-res.
void GetNextVelocityComponentsSdword(VelocityDesc* velocityptr, int32_t* pdx, int32_t* pdy, uint32_t num_frames)
{
	uint32_t e {(uint32_t)((uint32_t)velocityptr->error.width + ((uint32_t)velocityptr->fract.width * num_frames))};

	*pdx = ((int32_t)velocityptr->vector.width * num_frames)
		 + ((int32_t)((int8_t)uqm::lowByte(velocityptr->incr.width))
			* (e >> VelocityShift));

	velocityptr->error.width = (uint16_t)(VelocityRemainder(e));

	e = (uint32_t)((uint32_t)velocityptr->error.height + ((uint32_t)velocityptr->fract.height * num_frames));

	*pdy = ((int32_t)velocityptr->vector.height * num_frames)
		 + ((int32_t)((int8_t)uqm::lowByte(velocityptr->incr.height))
			* (e >> VelocityShift));

	velocityptr->error.height = (uint16_t)(VelocityRemainder(e));
}

// JMS_GFX: Preventing overflows in hi-res: The int32_t in this function's parameters was int16_t.
void SetVelocityVector(VelocityDesc* velocityptr, int32_t magnitude, uint16_t facing)
{
	uint16_t angle {};
	int16_t dx {};
	int16_t dy {};

	angle = velocityptr->TravelAngle =
		FacingToAngle(NormalizeFacing(facing));
	magnitude = WorldToVelocity(magnitude);
	dx = Cosine(angle, magnitude);
	dy = Sine(angle, magnitude);
	if (dx >= 0)
	{
		velocityptr->vector.width = VelocityToWorld(dx);
		velocityptr->incr.width = uqm::makeWord((uint8_t)1, (uint8_t)0);
	}
	else
	{
		dx = -dx;
		velocityptr->vector.width = -VelocityToWorld(dx);
		velocityptr->incr.width =
			uqm::makeWord((uint8_t)0xFF, (uint8_t)(VelocityRemainder(dx) << 1));
	}
	if (dy >= 0)
	{
		velocityptr->vector.height = VelocityToWorld(dy);
		velocityptr->incr.height = uqm::makeWord((uint8_t)1, (uint8_t)0);
	}
	else
	{
		dy = -dy;
		velocityptr->vector.height = -VelocityToWorld(dy);
		velocityptr->incr.height =
			uqm::makeWord((uint8_t)0xFF, (uint8_t)(VelocityRemainder(dy) << 1));
	}

	velocityptr->fract.width = VelocityRemainder(dx);
	velocityptr->fract.height = VelocityRemainder(dy);
	velocityptr->error.width = velocityptr->error.height = 0;
}

// JMS_GFX: Preventing overflows in hi-res: The SDWORDs in this function's parameters were SIZEs.
void SetVelocityComponents(VelocityDesc* velocityptr, int32_t dx, int32_t dy)
{
	uint16_t angle {};

	if ((angle = ARCTAN(dx, dy)) == FullCircle)
	{
		ZeroVelocityComponents(velocityptr);
	}
	else
	{
		if (dx >= 0)
		{
			velocityptr->vector.width = VelocityToWorld(dx);
			velocityptr->incr.width = uqm::makeWord((uint8_t)1, (uint8_t)0);
		}
		else
		{
			dx = -dx;
			velocityptr->vector.width = -VelocityToWorld(dx);
			velocityptr->incr.width =
				uqm::makeWord((uint8_t)0xFF, (uint8_t)(VelocityRemainder(dx) << 1));
		}
		if (dy >= 0)
		{
			velocityptr->vector.height = VelocityToWorld(dy);
			velocityptr->incr.height = uqm::makeWord((uint8_t)1, (uint8_t)0);
		}
		else
		{
			dy = -dy;
			velocityptr->vector.height = -VelocityToWorld(dy);
			velocityptr->incr.height =
				uqm::makeWord((uint8_t)0xFF, (uint8_t)(VelocityRemainder(dy) << 1));
		}

		velocityptr->fract.width = VelocityRemainder(dx);
		velocityptr->fract.height = VelocityRemainder(dy);
		velocityptr->error.width = velocityptr->error.height = 0;
	}

	velocityptr->TravelAngle = angle;
}

// JMS_GFX: Preventing overflows in hi-res: The SDWORDs in this function's parameters were SIZEs.
void DeltaVelocityComponents(VelocityDesc* velocityptr, int32_t dx, int32_t dy)
{

	dx += WorldToVelocity(velocityptr->vector.width)
		+ (velocityptr->fract.width - (int16_t)uqm::highByte(velocityptr->incr.width));
	dy += WorldToVelocity(velocityptr->vector.height)
		+ (velocityptr->fract.height - (int16_t)uqm::highByte(velocityptr->incr.height));

	SetVelocityComponents(velocityptr, dx, dy);
}

} // namespace uqm
