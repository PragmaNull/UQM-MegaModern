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
#pragma once

#ifndef LIBS_TIME_TIMELIB_H_
#define LIBS_TIME_TIMELIB_H_

#define TIMELIB SDL

#include <chrono>
#include "libs/compiler.h"


typedef uint32_t TimeCount;
typedef uint32_t TimePeriod;

/* GameTicksPerSecond is the LCM of all the fractions of a second the game uses.
 * Battle is 24 FPS, Landers are 35 FPS, most UI-level things are 15 FPS,
 * The Interplanetary flight is 30 FPS, Comm ambient animation is 40 FPS,
 * (also Comm Oscilloscope is 32 FPS, but it does not require a stable
 * timer and currently runs within the Comm ambient anim paradigm anyway)
 * Thus, the minimum value for GameTicksPerSecond is 840. */
#if TIMELIB == SDL
constexpr uint32_t GameTicksPerSecond {840};
static constexpr uint32_t getTicksForFramerate(const uint32_t desiredFps)
{
	return GameTicksPerSecond / desiredFps;
}
#endif


extern void InitTimeSystem(void);
extern void UnInitTimeSystem(void);
extern TimeCount GetTimeCounter(void);


#endif /* LIBS_TIME_TIMELIB_H_ */
