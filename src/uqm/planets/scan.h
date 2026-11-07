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
#ifndef UQM_PLANETS_SCAN_H
#define UQM_PLANETS_SCAN_H

#include "core/misc/bittricks.h"
#include "core/string/StringUtils.h"
#include "libs/gfxlib.h"
#include "planets.h"

namespace uqm
{
struct ScanDesc
{
	GfxPoint start;
	uint16_t start_dot;
	uint16_t num_dots;
	uint16_t dots_per_semi;
};

struct ScanBlock
{
	GfxPoint* line_base;
	uint16_t num_scans;
	uint16_t num_same_scans;
	ScanDesc* scan_base;
};

extern void ScanSystem(void);

extern void RepairBackRect(GfxRect* pRect);
extern void GeneratePlanetSide(void);
extern uint16_t callGenerateForScanType(const SolarSystemState*,
										const SystemPlanet* world, uint16_t node, ScanType scanType, SurfaceNode*);
// Returns true if the node should be removed from the surface
extern bool callPickupForScanType(SolarSystemState* solarSys,
								  SystemPlanet* world, uint16_t node, ScanType scanType);

extern void RedrawSurfaceScan(const GfxPoint* newLoc);
extern GfxContextRef GetScanContext(bool* owner);
extern void DestroyScanContext(void);

bool isNodeRetrieved(PlanetData* planetInfo, ScanType scanType, uint8_t nodeNr);
uint16_t countNodesRetrieved(PlanetData* planetInfo, ScanType scanType);
void setNodeRetrieved(PlanetData* planetInfo, ScanType scanType, uint8_t nodeNr);
void setNodeNotRetrieved(PlanetData* planetInfo, ScanType scanType, uint8_t nodeNr);

void GetPlanetTitle(uqstl::span<char> buf);

} // namespace uqm

#endif // UQM_PLANETS_SCAN_H
