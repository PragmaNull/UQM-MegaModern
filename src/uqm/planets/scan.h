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

#ifndef UQM_PLANETS_SCAN_H_
#define UQM_PLANETS_SCAN_H_

typedef struct scan_desc SCAN_DESC;
typedef struct scan_block SCAN_BLOCK;

#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"
#include "planets.h"
#include "core/string/StringUtils.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

struct scan_desc
{
	GFXPOINT start;
	uint16_t start_dot;
	uint16_t num_dots;
	uint16_t dots_per_semi;
};

struct scan_block
{
	GFXPOINT* line_base;
	uint16_t num_scans;
	uint16_t num_same_scans;
	SCAN_DESC* scan_base;
};

extern void ScanSystem(void);

extern void RepairBackRect(GFXRECT* pRect);
extern void GeneratePlanetSide(void);
extern uint16_t callGenerateForScanType(const SOLARSYS_STATE*,
										const PLANET_DESC* world, uint16_t node, uint8_t scanType, NODE_INFO*);
// Returns true if the node should be removed from the surface
extern bool callPickupForScanType(SOLARSYS_STATE* solarSys,
								  PLANET_DESC* world, uint16_t node, uint8_t scanType);

extern void RedrawSurfaceScan(const GFXPOINT* newLoc);
extern GFXCONTEXT GetScanContext(bool* owner);
extern void DestroyScanContext(void);

bool isNodeRetrieved(PLANET_INFO* planetInfo, uint8_t scanType, uint8_t nodeNr);
uint16_t countNodesRetrieved(PLANET_INFO* planetInfo, uint8_t scanType);
void setNodeRetrieved(PLANET_INFO* planetInfo, uint8_t scanType, uint8_t nodeNr);
void setNodeNotRetrieved(PLANET_INFO* planetInfo, uint8_t scanType, uint8_t nodeNr);

void GetPlanetTitle(uqstl::span<char> buf);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_PLANETS_SCAN_H_ */
