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
#ifndef UQM_CONS_RES_H
#define UQM_CONS_RES_H

#include "libs/gfxlib.h"
#include "libs/sndlib.h"

namespace uqm
{
void load_gravity_well(uint8_t selector);
void free_gravity_well(void);

FrameRef load_life_form(uint8_t selector);

inline constexpr auto NumOrbitThemes {6};
MusicRef load_orbit_theme(uint8_t selector);
extern int num_orbit_themes(void);

inline constexpr auto NumMmThemes {3};
MusicRef loadMainMenuMusic(uint8_t selector);

} // namespace uqm

#endif // UQM_CONS_RES_H
