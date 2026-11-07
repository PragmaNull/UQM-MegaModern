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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once
#ifndef UQM_STARMAP_H
#define UQM_STARMAP_H

#include "core/misc/bittricks.h"
#include "planets/planets.h"
#include "uqm/session/GameState.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

// Moving NumHyperVortices here for portal map; one fewer than the map
// because of the Arilou homeworld.
inline constexpr auto NumSolarSystems {502};
inline constexpr auto NumHyperVortices {15};

inline StarDesc*& CurStarDescPtr() { return uqm::gameState().currentStar(); }
// star_array is now a static buffer we will copy the starmap_array into, and
// then manipulate from there, so that original starmap is never altered.
extern StarDesc star_array[];
const extern GfxPoint* constel_array;

// Global plot_map stores star_pt (coords) used while seeding into starmap
// Once seeded the plot_map will contain StarDesc pointers to the
// star locations on the starmap as well.
extern PlotLocation plot_map[*PlotId::NumPlots];

// Global portal_map stores "star_pt" coords to each portal's exit,
// as well as "quasi_pt" coords in quasispace, and the StarDesc of
// the "closest constellation" for naming purposes.
// Also Arilou homeworld but this is still mostly hardcoded.
extern PortalLocation portal_map[NumHyperVortices + 1];

// A globally available seed for seeding the starmap and the plots it contains.
extern uqstl::optional<uqm::UQMRandom> StarGenRNG;

extern StarDesc* FindStar(StarDesc* pLastStar, GfxPoint* puniverse,
						  int16_t xbounds, int16_t ybounds);

// Populates buf with the full name of the star at pSD
// May not be used much any more ***
extern void GetClusterName(const StarDesc* pSD, char buf[]);

// Returns the closest star to point p on the given starmap
StarDesc* FindNearestStar(StarDesc* starmap, GfxPoint p);

// Returns the star in the closest constellation to point p on the starmap
// Constellation only returns stars with a Prefix > 0
StarDesc* FindNearestConstellation(StarDesc* starmap, GfxPoint p);

// Resets the starmap given to the default values of the static starmap_array.
void DefaultStarmap(StarDesc* starmap);

// Seeds the provided starmap with no plot, random size and color.
// Super Giant Star does not generate randomly, these are MELNORME#_DEFINED
void SeedStarmap(StarDesc* starmap);

// Sets the plot to the default values of the provided starmap
void DefaultPlot(PlotLocation* plot, StarDesc* starmap);

// Sets the basic "push" framework of Melnormes and rainbow worlds
// and associated plot threads to other plot IDs.
void InitMelnormeRainbow(PlotLocation* plotmap);

// InitPlot sets the plotmap object up with the standard set of plot
// requirements based on the default UQM plot line.  This gives the required
// min/max distance of each plot laden star from related plot laden stars.
void InitPlot(PlotLocation* plotmap);

// SeedPlot seeds the initialized plot map into the starmap given
// It selects the next plot which needs to be... plotted... and places it into
// the starmap, setting the Index as appropriate, then iterates until all plots
// are placed.
uint16_t SeedPlot(PlotLocation* plotmap, StarDesc* starmap);

// Sets the portal map to the default values of the static portalmap_array
void DefaultQuasispace(PortalLocation* portalmap);

// Seeds the portal map, selecting valid locations referencing the
// provided plot map and star map.
bool SeedQuasispace(PortalLocation* portalmap, PlotLocation* plotmap,
					StarDesc* starmap);

// Converts the given plot string to the plot ID enum value,
// e.g. "pkunk" -> PKUNK_DEFINED
uint16_t PlotIdStrToIndex(const char* plotIdStr);

// JSD end of changes to this file

inline constexpr auto InternalStarIndex {-1};

extern bool isStarMarked(const int star_index,
						 const char* marker_state);
extern void setStarMarked(const int star_index, const char* marker_state);

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_STARMAP_H
