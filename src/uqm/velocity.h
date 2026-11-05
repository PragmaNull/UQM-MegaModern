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

#ifndef UQM_VELOCITY_H_
#define UQM_VELOCITY_H_

#include <string.h> /* for memset */
#include "libs/gfxlib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

typedef struct velocity_desc
{
	uint16_t TravelAngle;
	EXTENT vector;
	EXTENT fract;
	EXTENT error;
	EXTENT incr;
} VELOCITY_DESC;

constexpr auto ZeroVelocityComponents(const auto& pv) { return memset(pv, 0, sizeof(*(pv))); }
constexpr auto& GetVelocityTravelAngle(auto* pv) { return pv->TravelAngle; }

extern void GetCurrentVelocityComponents(VELOCITY_DESC* velocityptr, int16_t* pdx, int16_t* pdy);

// JMS_GFX: New function to prevent overflows in hi-res.
extern void GetCurrentVelocityComponentsSdword(VELOCITY_DESC* velocityptr, int32_t* pdx, int32_t* pdy);

extern void GetNextVelocityComponents(VELOCITY_DESC* velocityptr, int16_t* pdx, int16_t* pdy, uint16_t num_frames);

// JMS_GFX: New function to prevent overflows in hi-res.
extern void GetNextVelocityComponentsSdword(VELOCITY_DESC* velocityptr, int32_t* pdx, int32_t* pdy, uint32_t num_frames);

extern void SetVelocityVector(VELOCITY_DESC* velocityptr, int32_t magnitude, uint16_t facing); // JMS_GFX: Changed int16_t to int32_t to prevent overflows

extern void SetVelocityComponents(VELOCITY_DESC* velocityptr, int32_t dx, int32_t dy); // JMS_GFX: Changed int16_t to int32_t to prevent overflows

extern void DeltaVelocityComponents(VELOCITY_DESC* velocityptr, int32_t dx, int32_t dy); // JMS_GFX: Changed int16_t to int32_t to prevent overflows

static inline bool IsVelocityZero(VELOCITY_DESC* vptr)
{
	return vptr->vector.width == 0 && vptr->vector.height == 0 && vptr->incr.width == 0 && vptr->incr.height == 0 && vptr->fract.width == 0 && vptr->fract.height == 0;
}

static inline uint32_t VelocitySquared(int16_t dx, int16_t dy)
{
	return (uint32_t)((long)dx * dx + (long)dy * dy);
}

inline constexpr auto VelocityShift {5};
inline constexpr auto VelocityScale {(1 << VelocityShift)};


constexpr auto VelocityToWorld(const auto& v) { return ((v) >> VelocityShift); }
constexpr auto WorldToVelocity(const auto& l) { return ((l) << VelocityShift); }

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_VELOCITY_H_ */
