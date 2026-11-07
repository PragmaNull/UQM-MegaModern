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
#ifndef UQM_PLANETS_SUNDATA_H
#define UQM_PLANETS_SUNDATA_H

#include "plandata.h"
#include "core/misc/bittricks.h"

namespace uqm
{
/*------------------------------ Global Data ------------------------------ */


inline constexpr auto DwarfEnergy {1};
inline constexpr auto GiantEnergy {5};
inline constexpr auto SupergiantEnergy {20};

struct SolarSystemInfo
{
	uint8_t StarSize;
	uint8_t StarIntensity;
	uint16_t StarEnergy;

	PlanetData PlanetInfo;
};

inline constexpr auto GenerateAll {(uint16_t)~0};

extern uint16_t GenerateMineralDeposits(const SolarSystemInfo*, uint16_t whichDeposit,
										SurfaceNode* info);
extern uint16_t GenerateLifeForms(const SolarSystemInfo*, uint16_t whichLife,
								  SurfaceNode* info);
extern void GenerateRandomLocation(GfxPoint* loc);
extern uint16_t GenerateRandomNodes(const SolarSystemInfo*, uint16_t scan, uint16_t numNodes,
									uint16_t type, uint16_t whichNode, SurfaceNode* info);
extern uint16_t CustomMineralDeposits(const SolarSystemInfo* SysInfoPtr, uint16_t which_deposit,
									  SurfaceNode* info, uint16_t numNodes, uint16_t type, uint8_t quality);
extern uint16_t CustomMineralDeposit(SurfaceNode* info, uint16_t type, uint8_t quality,
									 GfxPoint location);
// Generate lifeforms from a preset lifeTypes[] array
extern uint16_t GeneratePresetLife(const SolarSystemInfo*,
								   const int8_t* lifeTypes, uint16_t whichLife, SurfaceNode* info);


inline constexpr auto SupergiantElementDensity {8};


extern void DoPlanetaryAnalysis(SolarSystemInfo* SysInfoPtr,
								SystemPlanet* pPlanetDesc);

} // namespace uqm

#endif // UQM_PLANETS_SUNDATA_H
