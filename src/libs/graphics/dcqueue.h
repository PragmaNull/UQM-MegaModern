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

#ifndef DCQUEUE_H
#define DCQUEUE_H

// Maximum size of the DCQ.  The larger the DCQ, the larger frameskips
// become tolerable before initiating livelock deterrence and game
// slowdown.  Other constants for controlling the frameskip/slowdown
// balance may be found in sdl_common.c near TFB_FlushGraphics.

// Livelock deterrance constants.  Because the entire screen is rarely
// refreshed, we may not drop draw commands on the floor with abandon.
// Furthermore, if the main program is queuing commands at a speed
// comparable to our processing of the commands, we never finish and
// the game freezes.  Thus, if the queue starts out larger than
// DcqForceSlowdownSize, or DcqLivelockMax commands find
// themselves being processed in one go, livelock deterrence is
// enabled, and TFB_FlushGraphics locks the DCQ until it has processed
// all entries.  If batched but pending commands exceed DcqForceBreakSize,
// a continuity break is performed.  This will effectively slow down the
// game logic, a fate we seek to avoid - however, it seems to be unavoidable
// on slower machines.  Even there, it's seems nonexistent outside of
// communications screens.  --Michael

#ifdef DCQ_OF_DOOM
inline constexpr auto DcqMax {512};
inline constexpr auto DcqForceSlowdownSize {128};
inline constexpr auto DcqForceBreakSize {512};
inline constexpr auto DcqLivelockMax {256};
#else
inline constexpr auto DcqMax {16384};
inline constexpr auto DcqForceSlowdownSize {4096};
inline constexpr auto DcqForceBreakSize {16384};
inline constexpr auto DcqLivelockMax {4096};
#endif

#endif
