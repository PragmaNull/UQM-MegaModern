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

#pragma once
#ifndef UQM_HYPER_H
#define UQM_HYPER_H

#include "element.h"
#include "units.h"

namespace uqm
{
// for UnitScreenWidth()/HEIGHT

#if 0 //defined(__cplusplus)
extern "C" {
#endif

inline constexpr auto NumRadarScreens {12};

inline auto RadarScanWidth() { return ResDescale(UnitScreenWidth() * NumRadarScreens); }
inline auto RadarScanHeight() { return ResDescale(UnitScreenHeight() * NumRadarScreens); }

// QuasiSpace coordinates of the two-way portal
inline constexpr auto QuasiSpaceX {5000};
inline constexpr auto QuasiSpaceY {5000};

// QuasiSpace coordinates of the Arilou home world
inline constexpr auto ArilouHomeX {6134}; // (QUASI_SPACE_X + ((RADAR_SCAN_WIDTH >> 1) * 3))
inline constexpr auto ArilouHomeY {5900}; // (QUASI_SPACE_Y + ((RADAR_SCAN_HEIGHT >> 1) * 3))

extern bool LoadHyperspace(void);
extern bool FreeHyperspace(void);
extern void SeedUniverse(void);
extern void EraseRadar(void);
extern void MoveSIS(int32_t* pdx, int32_t* pdy);

extern void FreeHyperData(void);
extern void check_hyperspace_encounter(void);
extern bool hyper_transition(Element* ElementPtr);

extern void HyperspaceMenu(void);
extern void SaveSisHyperState(void);

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_HYPER_H
