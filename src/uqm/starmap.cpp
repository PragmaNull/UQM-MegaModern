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
//#define DEBUG_STARSEED
//#define DEBUG_STARSEED_TRACE
//#define DEBUG_STARSEED_TRACE_W
//#define DEBUG_STARSEED_TRACE_X
//#define DEBUG_STARSEED_TRACE_Y
//#define DEBUG_STARSEED_TRACE_Z

#include "starmap.h"

#include <stdlib.h> // bsearch needs this or it cores!
#include <time.h>	// For the clock.

#include "gamestr.h"
#include "globdata.h"
#include "hyper.h" // JSD: For arilou_home in the portal map
#include "libs/gfxlib.h"
#include "StarmapGlobalData.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
// The "starmap_array" variable (from StarmapGlobalData) is only used to intialize
// the "star_array" global variable.  This is now intentionally a copy
// of the data so that it may be manipulated or placed back to default from
// there.  "starmap_array" should never be altered and referenced as little
// as possible.
//StarDesc *star_array;
uqstl::array<StarDesc, NumSolarSystems + NumHyperVortices + 3> star_array {};
//StarDesc star_array[NumSolarSystems + NumHyperVortices + 3] =
//		{[0 ... (NumSolarSystems + NumHyperVortices + 2)] =
//		{{~0, ~0}, 0, 0, 0, 0}};
const GfxPoint* constel_array;
// JSD Give my own starseed
uqstl::optional<uqm::UQMRandom> StarGenRNG {};
uqstl::array<PortalLocation, NumHyperVortices + 1> portal_map {};
//PortalLocation portal_map[NumHyperVortices+1] =
//		{[0 ... (NumHyperVortices)] = {{0, 0}, {0, 0}, nullptr}};


StarDesc* FindStar(StarDesc* LastSDPtr, GfxPoint* puniverse, int16_t xbounds,
				   int16_t ybounds)
{
	int16_t min_y {};
	int16_t max_y {};
	int16_t lo {};
	int16_t hi {};
	StarDesc* BaseSDPtr {};

	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
	{
		BaseSDPtr = star_array.data();
		hi = NumSolarSystems - 1;
	}
	else
	{
		//#define NumHyperVortices 15 //JSD moved to .h
		BaseSDPtr = &star_array[NumSolarSystems + 1];
		hi = (NumHyperVortices + 1) - 1;
	}

	if (LastSDPtr == nullptr)
	{
		lo = 0;
	}
	else if ((lo = LastSDPtr - BaseSDPtr + 1) > hi)
	{
		return (0);
	}
	else
	{
		hi = lo;
	}

	if (ybounds <= 0)
	{
		min_y = max_y = puniverse->y;
	}
	else
	{
		min_y = puniverse->y - ybounds;
		max_y = puniverse->y + ybounds;
	}

	while (lo < hi)
	{
		int16_t mid {};

		mid = (lo + hi) >> 1;
		if (BaseSDPtr[mid].star_pt.y >= min_y)
		{
			hi = mid - 1;
		}
		else
		{
			lo = mid + 1;
		}
	}

	LastSDPtr = &BaseSDPtr[lo];
	if (ybounds < 0 || LastSDPtr->star_pt.y <= max_y)
	{
		int16_t min_x {};
		int16_t max_x {};

		if (xbounds <= 0)
		{
			min_x = max_x = puniverse->x;
		}
		else
		{
			min_x = puniverse->x - xbounds;
			max_x = puniverse->x + xbounds;
		}

		do
		{
			if ((ybounds < 0 || LastSDPtr->star_pt.y >= min_y)
				&& (xbounds < 0
					|| (LastSDPtr->star_pt.x >= min_x
						&& LastSDPtr->star_pt.x <= max_x)))
			{
				return (LastSDPtr);
			}
		} while ((++LastSDPtr)->star_pt.y <= max_y);
	}

	return (0);
}

void GetClusterName(const StarDesc* pSD, char buf[])
{
	const char* pStr {};

	char* pBuf {buf};
	if (pSD->Prefix)
	{
		pStr = GameString(GameStringBase::StarNumber + pSD->Prefix - 1);
		if (pStr)
		{
			while ((*pBuf++ = *pStr++))
				;
			pBuf[-1] = ' ';
		}
	}
	if ((pStr = GameString(pSD->Postfix)) == 0)
	{
		*pBuf = '\0';
	}
	else
	{
		while ((*pBuf++ = *pStr++))
			;
	}
}

// Finds the nearest (constellation = true / false = star) to the
// point P provided on the starmap, returns pointer to that star.
StarDesc* FindNearest(StarDesc* starmap, GfxPoint p, bool constellation)
{
	if (!starmap || p.x == ~0 || p.y == ~0)
	{
		return nullptr;
	}
	uint16_t star_id = 0;
	uint32_t dist {};
	uint32_t min_dist = MaxXUniverse * MaxYUniverse;
	for (uint16_t index {0}; index < NumSolarSystems; index++)
	{
		dist = (starmap[index].star_pt.x - p.x) * (starmap[index].star_pt.x - p.x) + (starmap[index].star_pt.y - p.y) * (starmap[index].star_pt.y - p.y);
		if (dist < min_dist && (starmap[index].Prefix > 0 || !constellation))
		{
			min_dist = dist;
			star_id = index;
		}
	}
	return (&(starmap[star_id]));
}

// Returns a pointer to the closest star to point p on the starmap
StarDesc* FindNearestStar(StarDesc* starmap, GfxPoint p)
{
	return (starmap ? FindNearest(starmap, p, false) : nullptr);
}

// Returns a pointer to the closest constellation to point p on the starmap
StarDesc* FindNearestConstellation(StarDesc* starmap, GfxPoint p)
{
	return (starmap ? FindNearest(starmap, p, true) : nullptr);
}

// plot_map is a global item (like star_map) that keeps track of the location
// on the star map of each plot item's location, like SOL or the SAMATRA.
// It is populated first (default plot lengths from InitPlot, or write your own)
// and can be added to with further AddPlot calls.  Then it is used during
// SeedPlot which sets the starmap locations.  After this point it is kept
// globally to help with the creation/location fleets, dialog generation, and
// any time the game needs to reference these "set" locations.
//
// Plot location is a structure that contains the coordinates assigned to each
// plot (once chosen), as well as a pointer to the located star in the star map.
// Uses dist_sq to store the min and max distance squared to other plots.
// Keeps a running tally of the "plot weight" of each plot when the min/max
// distances are set.
// The COORDS are initialized with {~0, ~0}, the rest 0.
// Plot ID 0 (ARILOU_DEFINED) is ARILOU_HYPERSPACE_PORTAL location.
uqstl::array<PlotLocation, *PlotId::NumPlots> plot_map {};
//PlotLocation plot_map[NUM_PLOTS] = {[0 ... (NUM_PLOTS - 1)] =
//		{{~0, ~0}, nullptr, {[0 ... (NUM_PLOTS - 1)] = 0}}};

static constexpr auto MinPortal {2000}; // The min distance between portals (exits)
static constexpr auto MinVortex {150};	// The min distance between vorticies in QS


// portal_map and portalmap_array work exactly like star_map and StarmapArray
// but for Quasispace.  We use this static one here for reference to initialize
// the global portal_map to the default Prime Seed values.
// These are called Hyperspace Vortices (Vortex) when in Quasispace,
// and Quasispace Portals when in Hyperspace (all of these are exit only)
// The recurring 2-way Portal/Vortex is ARILOU_DEFINED plot loc and 5000x5000
uqstl::array<PortalLocation, NumHyperVortices + 1> portalmap_array {
	{{{4091, 7748},
	  {(-12 * VortexScale) + 5000, (-21 * VortexScale) + 5000},
	  nullptr},
	 {{3184, 4906},
	  {(1 * VortexScale) + 5000, (-20 * VortexScale) + 5000},
	  nullptr},
	 {{9211, 6104},
	  {(-16 * VortexScale) + 5000, (-18 * VortexScale) + 5000},
	  nullptr},
	 {{5673, 1207},
	  {(8 * VortexScale) + 5000, (-17 * VortexScale) + 5000},
	  nullptr},
	 {{1910, 926},
	  {(3 * VortexScale) + 5000, (-13 * VortexScale) + 5000},
	  nullptr},
	 {{8607, 151},
	  {(-21 * VortexScale) + 5000, (-4 * VortexScale) + 5000},
	  nullptr},
	 {{50, 1647},
	  {(-4 * VortexScale) + 5000, (-4 * VortexScale) + 5000},
	  nullptr},
	 {{6117, 4131},
	  {(-12 * VortexScale) + 5000, (-2 * VortexScale) + 5000},
	  nullptr},
	 {{5658, 9712},
	  {(-26 * VortexScale) + 5000, (2 * VortexScale) + 5000},
	  nullptr},
	 {{2302, 3988},
	  {(-17 * VortexScale) + 5000, (7 * VortexScale) + 5000},
	  nullptr},
	 {{112, 9409},
	  {(10 * VortexScale) + 5000, (7 * VortexScale) + 5000},
	  nullptr},
	 {{7752, 8906},
	  {(15 * VortexScale) + 5000, (14 * VortexScale) + 5000},
	  nullptr},
	 {{368, 6332},
	  {(22 * VortexScale) + 5000, (16 * VortexScale) + 5000},
	  nullptr},
	 {{9735, 3153},
	  {(-6 * VortexScale) + 5000, (19 * VortexScale) + 5000},
	  nullptr},
	 {{5850, 6213},
	  {(10 * VortexScale) + 5000, (20 * VortexScale) + 5000},
	  nullptr},
	 {{0, 0},
	  {ArilouHomeX, ArilouHomeY},
	  nullptr}}
};

// Reset the given starmap to the default static starmap array
void DefaultStarmap(StarDesc* starmap)
{
	if (!starmap)
	{
		fmt::print(stderr, "DefaaultStarmap called with nullptr starmap PTR.\n");
		return;
	}
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "DefaultStarmap setting star map to original values.\n");
#endif
	for (uint16_t i {0}; i < NumSolarSystems + 1 + NumHyperVortices + 1 + 1; i++)
	{
		starmap[i] = StarmapArray[i];
	}
}

// Seed the type of each star, randomly selecting a color and
// either dwarf or giant star (super giant is handled by plot).
// Default: 9 Super-giant, 24 giant, 469 dwarf
// 53 white   85 blue   102 green   55 yellow 102 orange 105 red
// 47 w dwarf 80 b dwarf 96 g dwarf 50 y dwarf 97 o dwarf 99 r dwarf
//  5 w giant  4 b giant  4 g giant  2 y giant  5 o giant  4 r giant
//  1 w super  1 b super  2 g super  3 y super  0 o super  2 r super
void SeedStarmap(StarDesc* starmap)
{
	if (!starmap)
	{
		fmt::print(stderr, "SeedStarmap called with nullptr starmap PTR.\n");
		return;
	}
	uint16_t rand_val {};
	if (!StarGenRNG)
	{
		fmt::print(stderr, "****SeedStarmap creating a STAR GEN RNG****\n");
		StarGenRNG.emplace();
	}
	StarGenRNG->reseed(uqm::currentSeed().value);

	for (uint16_t i {0}; i < NumSolarSystems; i++)
	{
		rand_val = StarGenRNG->next();
		// 469 is the default number of dwarf stars, and 24 the number of
		// giant. Super giant is handled elsewhere.
		starmap[i].Type = MakeStar(uqm::lowByte(rand_val) % (469 + 24) >= 24 ? StellarSize::Dwarf : StellarSize::Giant,
								   uqm::highByte(rand_val) % *StarBodyColor::Count,
								   -1);
		starmap[i].Index = 0;
	}
}

// Functions which know how a plotmap works call it "plot" so they can access
// these internal define functions.
constexpr auto PlotHardcode(auto* plot, const auto& id) { return (!plot[id].star && plot[id].star_pt.x != ~0 && plot[id].star_pt.y != ~0); }
constexpr auto PlotSet(auto* plot, const auto& id) { return (plot[id].star && plot[id].star_pt.x != ~0 && plot[id].star_pt.y != ~0); }
constexpr auto PlotMax(auto* plot, const auto& pi, const auto& pj) { return ((pi > pj) ? plot[pi].dist_sq[pj] : plot[pj].dist_sq[pi]); }
constexpr auto PlotMin(auto* plot, const auto& pi, const auto& pj) { return ((pi < pj) ? plot[pi].dist_sq[pj] : plot[pj].dist_sq[pi]); }
constexpr void PlotMaxSet(auto* plot, auto pi, auto pj, auto value)
{
	if (pi > pj)
	{
		plot[pi].dist_sq[pj] = value;
	}
	else
	{
		plot[pj].dist_sq[pi] = value;
	}
}
constexpr void PlotMinSet(auto* plot, auto pi, auto pj, auto value)
{
	if (pi < pj)
	{
		plot[pi].dist_sq[pj] = value;
	}
	else
	{
		plot[pj].dist_sq[pi] = value;
	}
}
constexpr auto& PlotWeight(auto* plot, const auto& id) { return (plot[id].dist_sq[id]); }

// This is purely for debugging purposes - print out the plot name by ID
void print_plot_id(uint16_t plot_id)
{
	static uqstl::array<const char* const, 70> plot_name {
		"ARILOU_DEFINED", "SOL_DEFINED", "SHOFIXTI_DEFINED",
		"MAIDENS_DEFINED", "START_COLONY_DEFINED", "SPATHI_DEFINED",
		"ZOQFOT_DEFINED", "MELNORME0_DEFINED", "MELNORME1_DEFINED",
		"MELNORME2_DEFINED", "MELNORME3_DEFINED", "MELNORME4_DEFINED",
		"MELNORME5_DEFINED", "MELNORME6_DEFINED", "MELNORME7_DEFINED",
		"MELNORME8_DEFINED", "TALKING_PET_DEFINED", "CHMMR_DEFINED",
		"SYREEN_DEFINED", "BURVIXESE_DEFINED", "SLYLANDRO_DEFINED",
		"DRUUGE_DEFINED", "BOMB_DEFINED", "AQUA_HELIX_DEFINED",
		"SUN_DEVICE_DEFINED", "TAALO_PROTECTOR_DEFINED",
		"SHIP_VAULT_DEFINED", "URQUAN_WRECK_DEFINED", "VUX_BEAST_DEFINED",
		"SAMATRA_DEFINED", "ZOQ_SCOUT_DEFINED", "MYCON_DEFINED",
		"EGG_CASE0_DEFINED", "EGG_CASE1_DEFINED", "EGG_CASE2_DEFINED",
		"PKUNK_DEFINED", "UTWIG_DEFINED", "SUPOX_DEFINED", "YEHAT_DEFINED",
		"VUX_DEFINED", "ORZ_DEFINED", "THRADD_DEFINED", "RAINBOW0_DEFINED",
		"RAINBOW1_DEFINED", "RAINBOW2_DEFINED", "RAINBOW3_DEFINED",
		"RAINBOW4_DEFINED", "RAINBOW5_DEFINED", "RAINBOW6_DEFINED",
		"RAINBOW7_DEFINED", "RAINBOW8_DEFINED", "RAINBOW9_DEFINED",
		"ILWRATH_DEFINED", "ANDROSYNTH_DEFINED", "MYCON_TRAP_DEFINED",
		"URQUAN0_DEFINED", "URQUAN1_DEFINED", "URQUAN2_DEFINED",
		"KOHRAH0_DEFINED", "KOHRAH1_DEFINED", "KOHRAH2_DEFINED",
		"DESTROYED_STARBASE_DEFINED", "MOTHER_ARK_DEFINED",
		"ZOQ_COLONY0_DEFINED", "ZOQ_COLONY1_DEFINED",
		"ZOQ_COLONY2_DEFINED", "ZOQ_COLONY3_DEFINED", "ALGOLITES_DEFINED",
		"SPATHI_MONUMENT_DEFINED", "EXCAVATION_SITE_DEFINED"};
	fmt::print(stderr, "{} ({})", plot_name[plot_id], plot_id);
}

// The minimum distance you can set a max plot length to; needs to be
// non-zero, LARGER values have more weight.  The maximum distance you
// can set a min plot length to; too large and it will push off the map.
static constexpr auto MinPlot {100};
static constexpr auto MaxPlot {10000};
// We effectively won't care about the weight of a max plot length above this
// value either, it caps
static constexpr auto MaxPweight {100000000}; // (10000 * 10000) or MAX_PLOT * MAX_PLOT

// Internal (although it could be shared)
// Sets the min and max distances (squared internally)
// of two plot IDs on given plotmap.  Also checks to see if already set,
// and remove the plot lengths from existing plotweights before changing.
// Also increments the plot weight totals of the plots.
// Min and max can't be the same value, must be at least 100 apart.
void SetPlotLength(PlotLocation* plot, uint16_t plotA, uint16_t plotB,
				   uint16_t p_min, uint16_t p_max)
{
	uint16_t min = p_min;
	uint16_t max = p_max;
	// Reject bad data
	if (!plot || plotA >= PlotId::NumPlots || plotB >= PlotId::NumPlots || p_min > p_max - 100)
	{
		fmt::print(stderr, "{} called with bad data (PTR {} {} {} {})\n",
				   "SetPlotLength (plotmap, plot, plot, min, max)",
				   plotA, plotB, p_min, p_max);
		return;
	}
	// If you zero out max, it will treat it as MaxPlot (any length)
	// If your plot max/min is too small/large, push it up/down to the cap
	if ((max > MaxPlot) || (max == 0))
	{
		max = MaxPlot;
	}
	if (max < MinPlot)
	{
		max = MinPlot;
	}
	if (min > MaxPlot)
	{
		min = MaxPlot;
	}

	// If the plots already have a min or max, remove it from the plot_weight
	if (PlotMin(plot, plotA, plotB) > 0)
	{
#ifdef DEBUG_STARSEED_TRACE
		fmt::print(stderr, "SetPlotLength reducing plot min ");
		print_plot_id(plotA);
		fmt::print(stderr, " (weight {}) to ", PlotWeight(plot, plotA));
		print_plot_id(plotB);
		fmt::print(stderr, " (weight {}), min {}\n", PlotWeight(plot, plotB),
				   (uint16_t)sqrt(PlotMin(plot, plotA, plotB)));
#endif
		PlotWeight(plot, plotA) -= (uint16_t)sqrt(PlotMin(plot, plotA, plotB));
		PlotWeight(plot, plotB) -= (uint16_t)sqrt(PlotMin(plot, plotA, plotB));
	}
	if (PlotMax(plot, plotA, plotB) != 0 && PlotMax(plot, plotA, plotB) < MaxPlot * MaxPlot)
	{
#ifdef DEBUG_STARSEED_TRACE
		fmt::print(stderr, "SetPlotLength reducing plot max ");
		print_plot_id(plotA);
		fmt::print(stderr, " (weight {}) to ", PlotWeight(plot, plotA));
		print_plot_id(plotB);
		fmt::print(stderr, " (weight {}), max {}\n", PlotWeight(plot, plotB),
				   MaxPlot - (uint16_t)sqrt(PlotMax(plot, plotA, plotB)));
#endif
		PlotWeight(plot, plotA) += (uint16_t)sqrt(PlotMax(plot, plotA, plotB)) - MaxPlot;
		PlotWeight(plot, plotB) += (uint16_t)sqrt(PlotMax(plot, plotA, plotB)) - MaxPlot;
	}

	// Now set the plot lengths
	PlotMinSet(plot, plotA, plotB, min * min);
	PlotMaxSet(plot, plotA, plotB, max * max);

	// Now we increase the weights
	PlotWeight(plot, plotA) += min + MaxPlot - max;
	PlotWeight(plot, plotB) += min + MaxPlot - max;
}

// Internal. (although it could be shared)
// If the min/max plot lengths are set and valid, gives relative weight of
// this plot pair, otherwise 0.  The relative weight is the min distance
// plus the (MaxPlot - the max distance) of each plot thread connected.
uint32_t ConnectedPlot(PlotLocation* plot, uint16_t plotA, uint16_t plotB)
{
	// report bad data
	if (!(plot && (plotA < PlotId::NumPlots) && (plotB < PlotId::NumPlots)))
	{
		fmt::print(stderr, "{} called with bad data PTR {} {}\n",
				   "ConnectedPlot (plotmap, plot_id, plot_id)",
				   plotA, plotB);
		return 0;
	}
	// Plot MIN (capped at max pweight) + MAX weight - PLOT MAX (if valid)
	return (PlotMin(plot, plotA, plotB) > MaxPweight ?
				MaxPweight :
				PlotMin(plot, plotA, plotB))
		 + ((PlotMax(plot, plotA, plotB) == 0 || PlotMax(plot, plotA, plotB) > MaxPweight) ?
				0 :
				MaxPweight - PlotMax(plot, plotA, plotB));
}

// Used by GetNextPlot to re-iterate on a failed plot after pop-back.
// Needs SeedPlot to reset it as well (when it starts the clock).
static uint16_t next_plot = ~0;

// Internal.
// This returns the plot_id (number) with the highest weight on the
// plot map which has yet to be assigned a home, or NUM_PLOTS if all
// plots are allocated For a plot to be set, both int16_t and StarDesc
// ptr need data.  If both are set, we'll just stamp the starmap.
uint16_t GetNextPlot(PlotLocation* plot)
{
	uint16_t plot_id = 0;
	uint32_t top_weight = 0;
	uint16_t i {};
	if (!plot)
	{
		fmt::print(stderr, "GetNextPlot (plotmap) called with bad data PTR\n");
		return 0;
	}

	// If we're already working on one, continue to try that one.
	if (next_plot < PlotId::NumPlots && !PlotSet(plot, next_plot))
	{
		return next_plot;
	}

	// If we have hard coded plots still waiting to be placed, do one.
	for (i = 0; i < PlotId::NumPlots; i++)
	{
		if (PlotHardcode(plot, i))
		{
			return next_plot = i;
		}
	}

#if 0
	// Leaving this in for now as a stern reminder:
	// This makes things worse, even if you turn off the retry-last-plot above.
	// [Otherwise try and find one that's connected to the last one.]
	for (i = 0; i < PlotId::NumPlots && next_plot < PlotId::NumPlots; i++)
	{
		if (ConnectedPlot (plot, next_plot, i) > top_weight && !PlotSet(plot, i))
		{
			plot_id = i;
			top_weight = ConnectedPlot (plot, next_plot, i);
		}
	}
#endif

	// Otherwise find the heaviest plot available.
	for (i = 0; i < PlotId::NumPlots; i++)
	{
		if ((PlotWeight(plot, i) > top_weight) && !(PlotSet(plot, i)))
		{
			plot_id = i;
			top_weight = PlotWeight(plot, i);
		}
	}

	// After loop if top weight is 0 (plot_id will also be 0) there may be
	// unassigned plots without weight, find the first one:
	if (top_weight == 0)
	{
		while ((plot_id < PlotId::NumPlots) && (PlotSet(plot, plot_id)))
		{
			plot_id++;
		}
	}

	// Now plot_id contains either the first unassigned plot with highest
	// weight, OR NUM_PLOTS
#ifdef DEBUG_STARSEED_TRACE_W
	fmt::print(stderr, "Top Weight {} Plot ID {}", top_weight, plot_id);
#endif
	return next_plot = plot_id;
	//return plot_id;
}

// Internal but could be shared
// Sets all plot lengths and weights, locations and pointers to zero.
void ResetPlot(PlotLocation* plot)
{
	if (!plot)
	{
		fmt::print(stderr, "ResetPlot (plotmap) called with bad data PTR.\n");
		return;
	}
	uint16_t j {};
	for (uint16_t i {0}; i < PlotId::NumPlots; i++)
	{
		for (j = 0; j < PlotId::NumPlots; j++)
		{
			plot[i].dist_sq[j] = 0;
		}
		plot[i].star_pt = GfxPoint {~0, ~0};
		plot[i].star = nullptr;
	}
}

static constexpr auto ArilouSpaceX {438};  // We don't seed arilou from the map, they
static constexpr auto ArilouSpaceY {6372}; // are hard code all the way.

// Sets all plot locations to their position on the provided starmap.
void DefaultPlot(PlotLocation* plot, StarDesc* starmap)
{

	if (!plot || !starmap)
	{
		fmt::print(stderr, "DefaultPlot (plotmap, starmap) called {}\n",
				   "with bad data PTR.");
		return;
	}

	ResetPlot(plot);
	plot[0].star_pt = GfxPoint {ArilouSpaceX, ArilouSpaceY};
	plot[0].star = &(starmap[NumSolarSystems + 1 + NumHyperVortices + 1]);
	for (uint16_t i {0}; i < NumSolarSystems; i++)
	{
		if (starmap[i].Index > 0)
		{
			if (starmap[i].Index >= PlotId::NumPlots)
			{
				continue;
			}
			plot[starmap[i].Index].star_pt = starmap[i].star_pt;
			plot[starmap[i].Index].star = &(starmap[i]);
		}
	}
}

// InitPlot will set the plotmap up with weighted connections to help
// the seeding engine determine correctness.
// Extended lore is taken into account regardless so that changing modes
// doesn't alter the seed.
void InitPlot(PlotLocation* plotmap)
{
	if (!plotmap)
	{
		fmt::print(stderr, "InitPlot (plotmap) called with bad data PTR\n");
		return;
	}
	uint16_t j {};
	ResetPlot(plotmap);

	// Set up buffers around anyone with a zone of influence during the war
	// or after the war where homeworlds inside the zone wouldn't make sense
	// Orz are skipped, Androsynth takes care of them.
	const uqstl::array<uqstl::array<uint16_t, 2>, 23> home_map {
		{{*PlotId::Arilou, 300},
		 {*PlotId::Sol, 700},
		 {*PlotId::Yehat, 700},
		 {*PlotId::Shofixti, 700},
		 {*PlotId::Chmmr, 700},
		 {*PlotId::Syreen, 700},
		 {*PlotId::EggCase0, 700},
		 {*PlotId::MotherArk, 700},

		 {*PlotId::Druuge, 1300},
		 {*PlotId::Pkunk, 700},
		 {*PlotId::Utwig, 700},
		 {*PlotId::Supox, 300},
		 {*PlotId::Zoqfot, 700},
		 {*PlotId::Burvixese, 300},
		 {*PlotId::StartColony, 300},

		 {*PlotId::Samatra, 3000},
		 {*PlotId::Androsynth, 1000},
		 {*PlotId::Ilwrath, 1000},
		 {*PlotId::Mycon, 1000},
		 {*PlotId::Spathi, 1000},
		 {*PlotId::Thradd, 1000},
		 {*PlotId::Vux, 1000},
		 {*PlotId::TalkingPet, 1000}}
	 };
	// Total 8 + 7 + 8 currently 23 homeworlds
	// Set the homeworlds apart first (can be overwritten later)
	for (uint16_t i {0}; i < 22; i++)
	{
		for (j = i + 1; j < 23; j++)
		{
			SetPlotLength(plotmap, home_map[i][0], home_map[j][0],
						  (home_map[i][1] > home_map[j][1]) ?
							  home_map[i][1] :
							  home_map[j][1],
						  MaxPlot);
		}
	}

	// Sets up Melnormes and Rainbow Worlds with minimal plot weight
	// also their connections to other races.
	InitMelnormeRainbow(plotmap);

	// The numbers next to a race are their strength.  The numbers at the start
	// of a comment are the standard distance in Prime seed between the plots.

	// Arilou 250 and Earthling, Slylandro
	// 6240 Keep humans far from the conflict at start
	// 1660 Vela is not too close to Sol, but limited by fuel
	// 5030 Arilou are not too close or far, so they can "hide" the humans
	// 5090 Arilou and Orz are not in the same *space*
	// 8480 The Slylandro are far away and annoying to find
	SetPlotLength(plotmap, *PlotId::Sol, *PlotId::Samatra, 5000, MaxPlot);
	SetPlotLength(plotmap, *PlotId::Sol, *PlotId::StartColony, 1500, 5000);
	SetPlotLength(plotmap, *PlotId::Arilou, *PlotId::Orz, 4000, MaxPlot);
	SetPlotLength(plotmap, *PlotId::Arilou, *PlotId::Sol, 3000, 7000);
	SetPlotLength(plotmap, *PlotId::Slylandro, *PlotId::Sol, 6000, MaxPlot);

	// Druuge 1400 and Melnorme
	// 2990 Burvix are outside Druuge space but close enough to lure
	// 650 Alas, the poor algolites.  Keep them in Druuge space for _reasons_
	// 6600 They're not very close to Spathi at all for some reason.
	SetPlotLength(plotmap, *PlotId::Druuge, *PlotId::Burvixese, 2000, 4000);
	SetPlotLength(plotmap, *PlotId::Druuge, *PlotId::Algolites, 0, 1250);
	SetPlotLength(plotmap, *PlotId::Spathi, *PlotId::Algolites, 5000, MaxPlot);

	// Ilwrath 1410 and Chmrr, Chenjesu
	// 1490 The Chenjesu homeworld is between Ilwrath homeworld (guarding)
	// 1760 [Chenjesu] and the Pkunk howeworld
	// 3150 [Pkunk] Which the Ilwrath are attacking
	SetPlotLength(plotmap, *PlotId::Chmmr, *PlotId::Ilwrath, 1000, 3000);
	SetPlotLength(plotmap, *PlotId::Chmmr, *PlotId::Pkunk, 700, 3500);
	SetPlotLength(plotmap, *PlotId::Ilwrath, *PlotId::Pkunk, 3000, 6000);

	// Mycon 1070
	// 150 The Mycon start with the Sun Device very close
	// 510, 240, 520 The Egg Cases (Syra is Egg #0) are all very close as well
	// 1739 The trap should be outside Mycon SoI but not far
	SetPlotLength(plotmap, *PlotId::Mycon, *PlotId::SunDevice, 0, 500);
	SetPlotLength(plotmap, *PlotId::Mycon, *PlotId::EggCase0, 300, 1250);
	SetPlotLength(plotmap, *PlotId::Mycon, *PlotId::EggCase1, 0, 500);
	SetPlotLength(plotmap, *PlotId::Mycon, *PlotId::EggCase2, 0, 800);
	SetPlotLength(plotmap, *PlotId::Mycon, *PlotId::MyconTrap, 1200, 3500);

	// Orz 333 and Androsynth
	// Give Orz the standard UQ/KA push (they don't have other HW pushes)
	// 130 The Orz are very close / around to the Androsynth home
	// 80 The Orz are also around the Playground
	// 120 The Androsynth were near the Dig Site
	// 2150 The Androysnth fled Sol (but not too far because of colony)
	// 680 The Start Colony is near Androsynth space
	SetPlotLength(plotmap, *PlotId::Orz, *PlotId::Samatra, 3000, MaxPlot);
	SetPlotLength(plotmap, *PlotId::Androsynth, *PlotId::Orz, 0, 350);
	SetPlotLength(plotmap, *PlotId::Orz, *PlotId::TaaloProtector, 0, 350);
	SetPlotLength(plotmap, *PlotId::Androsynth,
				  *PlotId::ExcavationSite, 0, 350);
	SetPlotLength(plotmap, *PlotId::Androsynth, *PlotId::Sol, 2000, 6000);
	SetPlotLength(plotmap, *PlotId::Androsynth,
				  *PlotId::StartColony, 350, 1000);

	// Pkunk 666
	// 4410 The Pkunk went a pretty good distance from home
	SetPlotLength(plotmap, *PlotId::Pkunk, *PlotId::Yehat, 3000, 6000);

	// Spathi 1000 and Mmrnmhrm (eventually)
	// 710 The Spathi were close to the contested world
	// 660 The Mmrnmhrm were close to the contested world
	// 1360 The Spathi and Mmrnmhrm were this far apart
	SetPlotLength(plotmap, *PlotId::Spathi, *PlotId::SpathiMonument, 300, 1000);
	SetPlotLength(plotmap, *PlotId::MotherArk, *PlotId::SpathiMonument, 300, 1000);
	SetPlotLength(plotmap, *PlotId::MotherArk, *PlotId::Spathi, 1000, 2000);

	// Supox 333
	// 1290 This is the Supox Utwig Super Friends Zone
	SetPlotLength(plotmap, *PlotId::Supox, *PlotId::Utwig, 300, 3000);

	// Thraddash 833
	// 400 The Thraddash keep the Aqua Helix sacred
	// 6350 Thraddash are far from Chmmr for Ilwrath distraction
	// 5230 Have them start pretty far from Ilwrath space
	// 8090 And also have them start far from the Pkunk space
	SetPlotLength(plotmap, *PlotId::Thradd, *PlotId::AquaHelix, 0, 700);
	SetPlotLength(plotmap, *PlotId::Chmmr, *PlotId::Thradd, 3000, MaxPlot);
	SetPlotLength(plotmap, *PlotId::Ilwrath, *PlotId::Thradd, 3000, MaxPlot);
	SetPlotLength(plotmap, *PlotId::Pkunk, *PlotId::Thradd, 3000, MaxPlot);

	// Umgah 833 (defined by their suffering: TALKING_PET_DEFINED)
	// Alas, no plot threads for them, just standard homeworld stuff

	// Ur Quan and Kohr Ah both 2666, and Syreen,
	// KA/UQ and use the SAMATRA_DEFINED as homeworld
	// The Ship Vault is in the battle zone
	// The UQ Wreck is outside the battle zone
	// 1820 The Ship Vault is specifically close to the Syreen
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::ShipVault, 0, 3000);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::UrquanWreck, 4000, MaxPlot);
	SetPlotLength(plotmap, *PlotId::Syreen, *PlotId::ShipVault, 1700, 2100);
	// The UQ/KA bases are all in or near the battle zone
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::Urquan0, 0, 2500);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::Urquan1, 0, 2500);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::Urquan2, 0, 3000);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::Kohrah0, 0, 2500);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::Kohrah1, 0, 2500);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::Kohrah2, 0, 3000);
	// This is an abandoned slave-shielded planet
	// (also used as random planet name by ZEX)
	SetPlotLength(plotmap,
				  *PlotId::Samatra, *PlotId::DestroyedStarbase, 3000, MaxPlot);

	// Utwig 666
	// 690 The Utwig keep their Bomb handy
	// 3070 The Burvixese were close enough to the Utwig to be targeted
	SetPlotLength(plotmap, *PlotId::Utwig, *PlotId::Bomb, 0, 750);
	SetPlotLength(plotmap, *PlotId::Utwig, *PlotId::Burvixese, 1000, 4000);

	// VUX 900
	// 320 Admiral ZEX (the maidens) is within VUX territory
	// 7950 the VUX beast is far away from ZEX
	SetPlotLength(plotmap, *PlotId::Vux, *PlotId::Maidens, 0, 1000);
	SetPlotLength(plotmap, *PlotId::VuxBeast, *PlotId::Maidens, 5000, MaxPlot);

	// Yehat 750 and Shofixti
	// 2020 The Shofixti and Yehat - brothers for life
	SetPlotLength(plotmap, *PlotId::Shofixti, *PlotId::Yehat, 700, 3000);

	// Zoq Fot 320
	// The ZFP start inside the conflict zone
	// 3850 Loosely bind the ZFP scout - ZFP homeworld
	// 720 and ZFP scout - SOL ; keep in mind Sol - Samatra > 5000
	// 200, 490, 580, 300 ZFP colonies are inside the war zone and
	// relatively near ZFP but not necessarily inside their zone
	SetPlotLength(plotmap, *PlotId::Zoqfot, *PlotId::Samatra, 1500, 2500);
	SetPlotLength(plotmap, *PlotId::Zoqfot, *PlotId::ZoqScout, 2000, 5000);
	SetPlotLength(plotmap, *PlotId::ZoqScout, *PlotId::Sol, 0, 1500);
	SetPlotLength(plotmap, *PlotId::Zoqfot, *PlotId::ZoqColony0, 0, 1250);
	SetPlotLength(plotmap, *PlotId::Zoqfot, *PlotId::ZoqColony1, 0, 1000);
	SetPlotLength(plotmap, *PlotId::Zoqfot, *PlotId::ZoqColony2, 0, 1000);
	SetPlotLength(plotmap, *PlotId::Zoqfot, *PlotId::ZoqColony3, 0, 750);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::ZoqColony0, 0, 2500);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::ZoqColony1, 0, 2500);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::ZoqColony2, 0, 3000);
	SetPlotLength(plotmap, *PlotId::Samatra, *PlotId::ZoqColony3, 0, 3000);
}

// Sets up the plot threads for rainbow worlds and Melnormes, including
// the Melnorme 1 near Sol, and Melnorme 7 near Thraddash
void InitMelnormeRainbow(PlotLocation* plotmap)
{
	uint16_t i {};
	uint16_t j {};
	// Melnormes and Rainbow worlds all push each other pretty far away
	// to keep them scattered nicely. We then zero out their weights so
	// that they are picked last.  In practice 2500 (rainbow) and
	// 2800 (melnorme) are about as high as you can go before the
	// seeding algorithm slows way down.
	for (i = *PlotId::Rainbow0; i <= PlotId::Rainbow9; i++)
	{
		plotmap[i].star = nullptr;
		plotmap[i].star_pt = GfxPoint {~0, ~0};
		for (j = *PlotId::Rainbow0; j <= PlotId::Rainbow9; j++)
		{
			if (i != j)
			{
				SetPlotLength(plotmap, i, j, 2500, MaxPlot);
			}
		}
	}
	for (i = *PlotId::Melnorme0; i <= PlotId::Melnorme8; i++)
	{
		plotmap[i].star = nullptr;
		plotmap[i].star_pt = GfxPoint {~0, ~0};
		for (j = *PlotId::Melnorme0; j <= PlotId::Melnorme8; j++)
		{
			if (i != j)
			{
				SetPlotLength(plotmap, i, j, 2750, MaxPlot);
			}
		}
	}

	// Zero out their weights - just seeding them mostly last removes a
	// lot of the issues.
	for (i = *PlotId::Rainbow0; i <= PlotId::Rainbow9; i++)
	{
		plotmap[i].dist_sq[i] *= 0;
	}
	for (i = *PlotId::Melnorme0; i <= PlotId::Melnorme8; i++)
	{
		plotmap[i].dist_sq[i] *= 0;
	}

	// 670 Tanaka mentions a rainbow world, in Yehat space
	// 580 Thraddash mention a rainbow world,
	// 2350 Slylandro mention a rainbow world "not far"
	// 520 Supox mention a rainbow world "gross star color"
	// 500 By some strange coincidence, Melnormes have outpost #1 near SOL.
	// 160 Thraddash also have the stele on Melnorme #7
	SetPlotLength(plotmap, *PlotId::Yehat, *PlotId::Rainbow0, 0, 1000);
	SetPlotLength(plotmap, *PlotId::Thradd, *PlotId::Rainbow5, 0, 1000);
	SetPlotLength(plotmap, *PlotId::Slylandro, *PlotId::Rainbow4, 1500, 3500);
	SetPlotLength(plotmap, *PlotId::Supox, *PlotId::Rainbow7, 0, 1000);
	SetPlotLength(plotmap, *PlotId::Sol, *PlotId::Melnorme1, 0, 1000);
	SetPlotLength(plotmap, *PlotId::Thradd, *PlotId::Melnorme7, 0, 1000);
}

// Internal
// Returns true if all placed plots from plot_id's perspective ONLY are valid
// Don't need to worry about if stars are correct, as they may temporarily
// go past validation check but eventually if the star is in a bad spot,
// it will be replaced.  Although it may also impose irrelevant restrictions
// in the interim.  Do need to worry about undefined plot {~0,~0}.
bool CheckValid(PlotLocation* plot, uint16_t plot_id)
{
	uint32_t distance_sq {};
	if (!plot || plot_id >= PlotId::NumPlots)
	{
		fmt::print(stderr, "CheckValid (plotmap, plot_id) called {}.\n"
						   "with bad data or nullptr: PTR",
				   plot_id);
		return false;
	}
	if (!PlotSet(plot, plot_id))
	{
		fmt::print(stderr, "CheckValid (plotmap, plot_id) called {}.\n"
						   "with un-set plot:",
				   plot_id);
		return false;
	}
	for (uint16_t i {0}; i < PlotId::NumPlots; i++)
	{
		if (i == plot_id)
		{
			continue; // Don't check yourself
		}
		if (!PlotSet(plot, i))
		{
			continue;
		}
		if (PlotMin(plot, plot_id, i) == 0 && PlotMax(plot, plot_id, i) == MaxPweight)
		{
			continue;
		}
		distance_sq = ((plot[plot_id].star_pt.x - plot[i].star_pt.x) * (plot[plot_id].star_pt.x - plot[i].star_pt.x) + (plot[plot_id].star_pt.y - plot[i].star_pt.y) * (plot[plot_id].star_pt.y - plot[i].star_pt.y));
#ifdef DEBUG_STARSEED_TRACE_X
		fmt::print(stderr, "__dsq {} mindsq {} maxdsq {}__", distance_sq,
				   PlotMin(plot, plot_id, i), PlotMax(plot, plot_id, i));
#endif
		if (distance_sq < PlotMin(plot, plot_id, i) || (distance_sq > PlotMax(plot, plot_id, i) && PlotMax(plot, plot_id, i) > 0))
		{
			return false;
		}
	}
	return true;
}


static bool NeedsHabitable(uint8_t sIndex)
{
	switch (static_cast<PlotId>(sIndex))
	{
		case PlotId::Shofixti:
		case PlotId::StartColony:
		case PlotId::Zoqfot:
		case PlotId::TalkingPet:
		case PlotId::Syreen:
		case PlotId::Burvixese:
		case PlotId::Druuge:
		case PlotId::SunDevice:
		case PlotId::VuxBeast:
		case PlotId::Mycon:
		case PlotId::EggCase0:
		case PlotId::EggCase1:
		case PlotId::EggCase2:
		case PlotId::Pkunk:
		case PlotId::Utwig:
		case PlotId::Yehat:
		case PlotId::Vux:
		case PlotId::Orz:
		case PlotId::Thradd:
		case PlotId::Androsynth:
		case PlotId::MyconTrap:
		case PlotId::ZoqColony0:
		case PlotId::ZoqColony1:
		case PlotId::ZoqColony2:
		case PlotId::ZoqColony3:
		case PlotId::Algolites:
			return true;
		default:
			return false;
	}
}

// Plotify (internal) takes
// starmap - map you are plotting
// star - star you are adjusting to the plot
// Using the provided starmap, set the passed star to proper plot
// specifications, using other plots on the starmap, and/or using
// the global StarmapArray constants.
// We let the colors mostly stay random but this is easily changed.
void Plotify(StarDesc* starmap, StarDesc* star)
{
	if (!starmap || !star)
	{
		fmt::print(stderr, "Plotify (starmap, star) called with nullptr PTR.\n");
		return;
	}
	uint16_t i = 0;

	while (star->Index != StarmapArray[i].Index)
	{
		if (++i >= NumSolarSystems)
		{
			return; // It doesn't have one
		}
	}

	// Section 1: change the star to the right size for the plot
	if (StarType(StarmapArray[i].Type) == StellarSize::SuperGiant)
	{
		star->Type = MakeStar(
			StellarSize::SuperGiant,
			StarColor(star->Type),
			StarOwner(star->Type));
	}
	if (StarType(StarmapArray[i].Type) == StellarSize::Giant)
	{
		star->Type = MakeStar(
			StellarSize::Giant,
			StarColor(star->Type),
			StarOwner(star->Type));
	}
	if (StarType(StarmapArray[i].Type) == StellarSize::Dwarf)
	{
		star->Type = MakeStar(
			StellarSize::Dwarf,
			StarColor(star->Type),
			StarOwner(star->Type));
	}

	// Section 2: change colors of stars based on plot
	// Specific homeworlds that work best with their correct color star (SOL)
	if (NeedsHabitable(star->Index))
	{
		const StarBodyColor star_color {StarColor(star->Type)};
		const uint32_t rand_val = GetRandomSeedForStar(star);

		if (StarType(star->Type) == StellarSize::Dwarf && star_color == StarBodyColor::Red)
		{
			const uqstl::array<uint8_t, 5> color_array {*StarBodyColor::Blue, *StarBodyColor::Green, *StarBodyColor::Orange,
														*StarBodyColor::White, *StarBodyColor::Yellow};

			star->Type = MakeStar(
				StarType(star->Type), color_array[rand_val % 5],
				StarOwner(star->Type));
		}
		else if (StarType(star->Type) == StellarSize::Giant
				 && (star_color == StarBodyColor::Blue || star_color == StarBodyColor::Green
					 || star_color == StarBodyColor::White))
		{
			const uqstl::array<uint8_t, 3> color_array {*StarBodyColor::Orange, *StarBodyColor::Red, *StarBodyColor::Yellow};

			star->Type = MakeStar(
				StarType(star->Type), color_array[rand_val % 3],
				StarOwner(star->Type));
		}
	}
	if (star->Index == PlotId::Sol)
	{
		star->Type = StarmapArray[i].Type;
	}
	// On world 44s, the Eye Of Dogar is Green, of course
	// All other channels are false gods
	if (star->Index == PlotId::Ilwrath)
	{
		const uint16_t newcolor = (*StarColor(star->Type) + uqm::currentSeed().value) % 5;
		if (uqm::currentSeed().value % 44 == 0 || uqm::currentSeed().value % 100 == 44)
		{
			star->Type = StarmapArray[i].Type;
		}
		else
		{
			star->Type = MakeStar(
				StarType(star->Type),
				newcolor >= StarBodyColor::Green ? newcolor + 1 : newcolor,
				StarOwner(star->Type));
		}
	}
#if 0
	// Shofixti can't be orange due to planet types
	// Not needed any more, after altering shofixti code a bit
	if (star->Index == PlotId::Shofixti)
	{
		uint16_t newcolor = (StarColor (star->Type) + uqm::currentSeed().value) % 5;
		star->Type = MakeStar (
			StarType (star->Type),
			newcolor >= StarBodyColor::Orange ? newcolor + 1 : newcolor,
			StarOwner (star->Type));
	}
#endif
	// Green, Blue, and White giants are too hot for spathis.
	// This is an asthetic choice but a good one.
	if (star->Index == PlotId::Spathi)
	{
		if (StarColor(star->Type) == StarBodyColor::White)
		{
			star->Type = MakeStar(
				StarType(star->Type),
				StarBodyColor::Yellow,
				StarOwner(star->Type));
		}
		if (StarColor(star->Type) == StarBodyColor::Blue)
		{
			star->Type = MakeStar(
				StarType(star->Type),
				StarBodyColor::Orange,
				StarOwner(star->Type));
		}
		if (StarColor(star->Type) == StarBodyColor::Green)
		{
			star->Type = MakeStar(
				StarType(star->Type),
				StarBodyColor::Red,
				StarOwner(star->Type));
		}
	}
	// Supox will NOT be the same color as rainbow world
	// This swaps yellow/blue, red/white, and green/orange
	if (star->Index == PlotId::Supox)
	{
		int j = 0;
		while (PlotId::Rainbow0 != starmap[j].Index)
		{
			if (++j >= NumSolarSystems)
			{
				return; // It doesn't have one
			}
		}
		// This will either increment or decriment based on even/odd
		// and then % 6 to put back into star_color.
		star->Type = MakeStar(StellarSize::Dwarf,
							  (*StarColor(starmap[j].Type) + 6 + *StarColor(starmap[j].Type) % 2 - (*StarColor(starmap[j].Type) + 1) % 2) % 6,
							  StarOwner(star->Type));
	}
	// Rainbow worlds all get RAINBOW0's color, whatever it is.
	// We will use that later in the alien dialogs.
	if ((star->Index > PlotId::Rainbow0) && (star->Index <= PlotId::Rainbow9))
	{
		int j = 0;
		while (PlotId::Rainbow0 != starmap[j].Index)
		{
			if (++j >= NumSolarSystems)
			{
				return; // It doesn't have one
			}
		}
		star->Type = MakeStar(StarType(star->Type),
							  StarColor(starmap[j].Type),
							  StarOwner(star->Type));
	}

	// Section 3: Mess with names of star systems
	// Melnormes live in super giants, and super giants take the Alpha
	// of their system.
	if ((star->Index >= PlotId::Melnorme0) && (star->Index <= PlotId::Melnorme8) && (star->Prefix > 1))
	{
		int j = 0;
		while ((starmap[j].Postfix != star->Postfix) || (starmap[j].Prefix != 1))
		{
			if (++j >= NumSolarSystems)
			{
				return; // It doesn't have one
			}
		}
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Melnorme {} swapping {} {} at {}.{} : {}.{} for "
						   "{} {} at {}.{} : {}.{}\n",
				   star->Index - PlotId::Melnorme0, star->Prefix, star->Postfix,
				   star->star_pt.x / 10, star->star_pt.x % 10,
				   star->star_pt.y / 10, star->star_pt.y % 10,
				   starmap[j].Prefix, starmap[j].Postfix,
				   starmap[j].star_pt.x / 10, starmap[j].star_pt.x % 10,
				   starmap[j].star_pt.y / 10, starmap[j].star_pt.y % 10);
#endif
		starmap[j].Prefix = star->Prefix;
		star->Prefix = 1;
	}
	// Humans and their egos.
	if (star->Index == PlotId::Sol)
	{
		int j = 0;
		while (starmap[j].Postfix != 129) // Postfix 129 = "Sol"
		{
			if (++j >= NumSolarSystems)
			{
				return; // Somehow, Sol does not exist
			}
		}
		if (star->Prefix == 0)
		{
			starmap[j].Postfix = star->Postfix;
			star->Postfix = 129;
		}
		else
		{
			int prefix = 0;
			int k {};
			// Re-order the stars to remove new Sol's prefix
			for (k = 0; k < NumSolarSystems; k++)
			{
				if (starmap[k].Postfix == star->Postfix && starmap[k].Prefix > star->Prefix)
				{
					starmap[k].Prefix--;
				}
			}
			// New Sol gets its name
			star->Prefix = 0;
			star->Postfix = 129;
			// Find the nearest constellation to old Sol
			const StarDesc* pStr = FindNearestConstellation(starmap,
															starmap[j].star_pt);
			// Find the highest Prefix
			for (k = 0; k < NumSolarSystems; k++)
			{
				if (starmap[k].Postfix == pStr->Postfix && starmap[k].Prefix > prefix)
				{
					prefix = starmap[k].Prefix;
				}
			}
			// Old Sol gets new Prefix/Postfix.  It shouldn't be possible,
			// but just in case, cap Prefix at 14 ("Xi")
			starmap[j].Postfix = pStr->Postfix;
			starmap[j].Prefix = (prefix >= 14 ? 14 : prefix + 1);
			// Old Sol JUST MIGHT be a Melnorme, in which case
			Plotify(starmap, &starmap[j]);
		}
	}
	// Alas, the poor algolites... wherever they are, the stars
	// carry their name (Postfix 113 = "Algol").
	if (star->Index == PlotId::Algolites)
	{
		int j = 0;
		for (j = 0; j < NumSolarSystems; j++)
		{
			if (&starmap[j] == star)
			{
				continue; // Not yet...
			}
			if (starmap[j].Postfix == star->Postfix)
			{
				starmap[j].Postfix = 113;
			}
			else if (starmap[j].Postfix == 113)
			{
				starmap[j].Postfix = star->Postfix;
			}
		}
		star->Postfix = 113;
	}
}

#ifdef DEBUG_STARSEED_TRACE
uint16_t last_err = PlotId::NumPlots;
void DebugPlotTicker(uint16_t plot_id)
{
	if (last_err == plot_id)
	{
		fmt::print(stderr, ".");
	}
	else
	{
		fmt::print(stderr, "\n");
		print_plot_id(plot_id);
		last_err = plot_id;
	}
}
#endif

// The amount we skip around in the starmap, must be coprime with
// NumSolarSystems (502)
static constexpr auto StarFactor {89};

// SeedPlot will create a seeded location for each remaining item on the
// plotmap which does not have an assigned location, within the bounds
// provided by the min/max range variables.  If a valid location can be found,
// recurse further until complete, otherwise return failed plot ID which is
// used by previous iteration to retry with differnt locations.
// NUM_PLOTS = success; NUM_PLOTS + 1 = timed out
uint16_t SeedPlot(PlotLocation* plotmap, StarDesc* starmap)
{
	static bool timer_running = false;
	static clock_t timer;
	if (!plotmap || !starmap)
	{
		fmt::print(stderr, "SeedPlot (plotmap, starmap) called with nullptr PTR.\n");
		return 0;
	}
	// The clock.  The first time this is called the clock is not running.
	// It will start the clock, and set timer_running true globally, and
	// my_clock true *locally*, which is how you detect the top layer of the
	// recursion and thus stop the clock on failure.
	bool my_clock = false;
	uint16_t rand_val {};
	uint16_t plot_id {};
	uint16_t star_id {};
	uint16_t i {};
	uint16_t return_id {};
	// timelimit is in deciseconds, if loading 60 seconds, if new 2 seconds
#ifdef DEBUG_STARSEED_TRACE_Z
	uqstl::array<bool, NumSolarSystems> tried {false};
	//bool tried[NumSolarSystems] = {[0 ... NumSolarSystems - 1] = false};
	for (i = 0; i < tried.size(); i++)
	{
		tried[i] = 0;
	}
#endif
	const uint16_t timelimit = (uqm::currentActivity() ? 600 : 20);
#ifdef DEBUG_STARSEED_TRACE
	timelimit = (uqm::currentActivity() ? 3600 : 3600);
#endif

	if (!StarGenRNG)
	{
		fmt::print(stderr, "****SeedPlot creating a STAR GEN RNG****\n");
		StarGenRNG.emplace();
	}
	if (!timer_running)
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Starting a timer; global activity {} | "
						   "uqm::currentSeed().value {} | NUM_PLOTS {}.\n",
				   uqm::currentActivity(),
				   uqm::currentSeed().value, PlotId::NumPlots);
#endif
		// Basically a wrapper around the recursion.
		timer_running = true;
		next_plot = ~0;
		my_clock = true;
		timer = clock();
		StarGenRNG->reseed(uqm::currentSeed().value);
		// nullptr out all the plot pointers so that it "places" pregens
		// in order to properly Plotify () the pregens.  ARILOU don't need.
		for (i = 1; i < PlotId::NumPlots; i++)
		{
			plotmap[i].star = nullptr;
		}
		// Just in case hard coded ARILOU doesn't have a pointer
		// use Arilou homeworld pointer so we can just skip it
		if (plotmap[*PlotId::Arilou].star_pt.x != ~0 && plotmap[*PlotId::Arilou].star_pt.y != ~0)
		{
			plotmap[*PlotId::Arilou].star =
				&(starmap[NumSolarSystems + 1 + NumHyperVortices + 1]);
		}
#if 0
		// Place down any plots which were already assigned in plotmap
		for (i = 1; i < PlotId::NumPlots; i++)
		{
			//if (plotmap[i].star_pt.x != ~0 && plotmap[i].star_pt.x != ~0)
			if ((plotmap[i].star = FindNearestStar (starmap, plotmap[i].star_pt)))
			{
				//plotmap[i].star = FindNearestStar (starmap, plotmap[i].star_pt);
#ifdef DEBUG_STARSEED
				print_plot_id (i);
				fmt::print (stderr, " being placed in pre-generated location "
						"%05.1f : %05.1f (given %05.1f : %05.1f)\n",
						(float) plotmap[i].star->star_pt.x / 10,
						(float) plotmap[i].star->star_pt.x / 10,
						(float) plotmap[i].star_pt.x / 10,
						(float) plotmap[i].star_pt.y / 10);
#endif
				if (plotmap[i].star->Index)
				{
					print_plot_id (i);
					fmt::print (stderr, " cannot be placed in location "
							"%05.1f : %05.1f due to existing plot ",
							(float) plotmap[i].star->star_pt.x / 10,
							(float) plotmap[i].star->star_pt.x / 10);
					print_plot_id (plotmap[i].star->Index);
					fmt::print (stderr, ", aborting...\n");
					return i;
				}
				plotmap[i].star_pt = plotmap[i].star->star_pt;
				plotmap[i].star->Index = i;
			}
		}
#endif
	}
	else if ((clock() - timer) / 100000 > timelimit)
	{
		fmt::print(stderr, "TIME'S UP!  Giving up on seed {}.\n", uqm::currentSeed().value);
		timer_running = false;
		return *PlotId::NumPlots + 1;
	}

	rand_val = StarGenRNG->next();
	// choose a plot by weight
	if ((plot_id = GetNextPlot(plotmap)) == PlotId::NumPlots)
	{
		// Sanity check the placements - they all pass PlotSet(plot, id)
		// but may be missing from starmap.
		//
		// All plots are assigned, but any without location allocated
		// were passed in and need Plotify().
		//next_plot = ~0;
		timer_running = false;
		for (i = 1; i < PlotId::NumPlots; i++)
		{
			if (!plotmap[i].star || plotmap[i].star != FindNearestStar(starmap, plotmap[i].star_pt))
			{
				print_plot_id(i);
				fmt::print(stderr, " star ptr was unassigned, corrected.\n");
				plotmap[i].star = FindNearestStar(starmap, plotmap[i].star_pt);
			}
			if ((plotmap[i].star_pt.x != plotmap[i].star->star_pt.x) || (plotmap[i].star_pt.y != plotmap[i].star->star_pt.y))
			{
				print_plot_id(i);
				fmt::print(stderr, " coords don't match star ptr, corrected.\n");
				plotmap[i].star_pt = plotmap[i].star->star_pt;
			}
			if (plotmap[i].star->Index > 0 && plotmap[i].star->Index != i)
			{
				print_plot_id(i);
				fmt::print(stderr, " plot wants to be at star %05.1f : %05.1f "
								   "but plot ID ",
						   (float)plotmap[i].star_pt.x / 10,
						   (float)plotmap[i].star_pt.y / 10);
				print_plot_id(plotmap[i].star->Index);
				fmt::print(stderr, " is there, ABORT!!!!\n");
				return i;
			}
			if (plotmap[i].star->Index != i)
			{
				print_plot_id(i);
				fmt::print(stderr, " plot location was missing from star, "
								   "corrected.\n");
				plotmap[i].star->Index = i;
				Plotify(starmap, plotmap[i].star);
			}
		}
#ifdef DEBUG_STARSEED
		for (i = 1; i < PlotId::NumPlots; i++)
		{
			print_plot_id(i);
			fmt::print(stderr, " at %05.1f : %05.1f.\n",
					   (float)plotmap[i].star_pt.x / 10,
					   (float)plotmap[i].star_pt.y / 10);
		}
#endif
		return *PlotId::NumPlots;
	}
#ifdef DEBUG_STARSEED_TRACE
	fmt::print(stderr, "\nSelected ");
	print_plot_id(plot_id);
#endif

	// Coordinates and no pointer means seed a static value (first)
	// We do it this way so we can Plotify at the end
	if (plotmap[plot_id].star_pt.x != ~0 && plotmap[plot_id].star_pt.y != ~0)
	{
		plotmap[plot_id].star = FindNearestStar(starmap,
												plotmap[plot_id].star_pt);
#ifdef DEBUG_STARSEED
#ifndef DEBUG_STARSEED_TRACE
		fmt::print(stderr, "Selected ");
		print_plot_id(plot_id);
#endif
		fmt::print(stderr, " placing in pre-generated location "
						   "%05.1f : %05.1f (given %05.1f : %05.1f)\n",
				   (float)plotmap[plot_id].star->star_pt.x / 10,
				   (float)plotmap[plot_id].star->star_pt.y / 10,
				   (float)plotmap[plot_id].star_pt.x / 10,
				   (float)plotmap[plot_id].star_pt.y / 10);
#endif
		if (plotmap[plot_id].star->Index && plotmap[plot_id].star->Index != plot_id)
		{
			print_plot_id(plot_id);
			fmt::print(stderr, " cannot be placed in location "
							   "%05.1f : %05.1f due to existing plot ",
					   (float)plotmap[plot_id].star->star_pt.x / 10,
					   (float)plotmap[plot_id].star->star_pt.y / 10);
			print_plot_id(plotmap[plot_id].star->Index);
			fmt::print(stderr, ", aborting...\n");
			return plot_id;
		}
		plotmap[plot_id].star_pt = plotmap[plot_id].star->star_pt;
		plotmap[plot_id].star->Index = plot_id;

		return_id = SeedPlot(plotmap, starmap);

		if (return_id == PlotId::NumPlots)
		{
			timer_running = false;
			Plotify(starmap, plotmap[plot_id].star);
			return return_id;
		}
		// Either we ran out of time or downstream seeding failed, pop layer
		plotmap[plot_id].star->Index = 0;
		plotmap[plot_id].star = nullptr;
		if (my_clock)
		{
			fmt::print(stderr, "Complete failure, stopping clock.\n");
			timer_running = false;
		}
		return return_id;
	}
	// Otherwise find this plot an empty star to call home, then recurse
	for (i = 0; i < NumSolarSystems; i++)
	{
#ifdef DEBUG_STARSEED_TRACE
		DebugPlotTicker(plot_id);
#endif
		// ARILOU is a special case, we pick a point not a star
		if (plot_id == PlotId::Arilou)
		{
			star_id = (rand_val + i * StarFactor) % 100;
			plotmap[*PlotId::Arilou].star_pt = GfxPoint {
				(star_id / 10) * 1000 + uqm::lowByte(rand_val) * 100 / 256,
				(star_id % 10) * 1000 + uqm::highByte(rand_val) * 100 / 256};
			plotmap[*PlotId::Arilou].star = FindNearestStar(starmap,
															plotmap[*PlotId::Arilou].star_pt);
			// Anything less than 2.0 units from a star is too close
			if ((plotmap[*PlotId::Arilou].star) && ((plotmap[*PlotId::Arilou].star_pt.x - plotmap[*PlotId::Arilou].star->star_pt.x) * (plotmap[*PlotId::Arilou].star_pt.x - plotmap[*PlotId::Arilou].star->star_pt.x) + (plotmap[*PlotId::Arilou].star_pt.y - plotmap[*PlotId::Arilou].star->star_pt.y) * (plotmap[*PlotId::Arilou].star_pt.y - plotmap[*PlotId::Arilou].star->star_pt.y) < 400))
			{
				continue;
			}
			// Arilou homeworld pointer for reasons
			star_id = NumSolarSystems + 1 + NumHyperVortices + 1;
			plotmap[plot_id].star = &(starmap[star_id]);
		}
		else
		{
			star_id = (rand_val + i * StarFactor) % NumSolarSystems;
			// Not empty, requeue
			if (starmap[star_id].Index != 0)
			{
				continue;
			}
			// Put the plot in the starsystem
			starmap[star_id].Index = plot_id;
			// Put the starsystem in the plot
			plotmap[plot_id].star_pt = starmap[star_id].star_pt;
			plotmap[plot_id].star = &(starmap[star_id]);
		}
		// If this is valid, then try and move on to the next plot.
		// If recursion succeeds, success.  Otherwise, if unrelated to us,
		// pop this layer entirely, or if is related to us, requeue this.
		// This is not 100% mathematically complete; if the cause of failure
		// is because the star I selected made it no longer possible for the
		// other plots to complete.  Additionally, it should attempt to
		// repick the failed plot, altering the seeding tree going forward
		// and losing significant areas of the solution set.  This is
		// preferable to a high failure %.
		if (CheckValid(plotmap, plot_id))
		{
#ifdef DEBUG_STARSEED_TRACE
			print_plot_id(plot_id);
			fmt::print(stderr, " trying %05.1f : %05.1f ",
					   (float)plotmap[plot_id].star_pt.x / 10,
					   (float)plotmap[plot_id].star_pt.y / 10);
#endif
			return_id = SeedPlot(plotmap, starmap);
			if (return_id == PlotId::NumPlots)
			{
				timer_running = false;
				if (plot_id != PlotId::Arilou)
				{
					Plotify(starmap, plotmap[plot_id].star);
				}
				return return_id;
			}
			// If we ran out of time or the plot is not heavily connected,
			// pop this layer
			if (return_id > PlotId::NumPlots || ConnectedPlot(plotmap, plot_id, return_id) <= 2000000)
			{
#ifdef DEBUG_STARSEED_TRACE_Y
				fmt::print(stderr, " [{}] Popping ", return_id);
				print_plot_id(plot_id);
				fmt::print(stderr, ", from %05.1f : %05.1f [C {}]\n",
						   (float)plotmap[plot_id].star_pt.x / 10,
						   (float)plotmap[plot_id].star_pt.y / 10,
						   (return_id > PlotId::NumPlots) ? -1 :
															(uint32_t)sqrt(ConnectedPlot(plotmap, plot_id, return_id)));
#endif
				if (plot_id != PlotId::Arilou)
				{
					starmap[star_id].Index = 0;
				}
				plotmap[plot_id].star_pt = GfxPoint {~0, ~0};
				plotmap[plot_id].star = nullptr;
				if (my_clock)
				{
					fmt::print(stderr, "Complete failure, stopping clock.\n");
					timer_running = false;
				}
				return return_id;
			}
		}
		// admit defeat and move on to the next star system
#ifdef DEBUG_STARSEED_TRACE_Y
		fmt::print(stderr, "Removing ");
		print_plot_id(plot_id);
		fmt::print(stderr, ", from %05.1f : %05.1f, ",
				   (float)plotmap[plot_id].star_pt.x / 10,
				   (float)plotmap[plot_id].star_pt.y / 10);
#endif
		if (plot_id != PlotId::Arilou)
		{
			starmap[star_id].Index = 0;
		}
		plotmap[plot_id].star_pt = GfxPoint {~0, ~0};
		plotmap[plot_id].star = nullptr;
	}
#ifdef DEBUG_STARSEED_TRACE_Z
	fmt::print(stderr, "RAN OUT OF PERMUTATIONS.\n");
#endif
	// Ran out of stars to try - this particular branch is impossible
	// return my own plot ID to pop above
	if (my_clock)
	{
		fmt::print(stderr, "Complete failure, stopping clock.\n");
		timer_running = false;
	}
	return (plot_id);
}

// Reset the quasispace portal map to the static default portalmap_array
void DefaultQuasispace(PortalLocation* portalmap)
{
	if (!portalmap)
	{
		fmt::print(stderr, "DefaultQuasispace (portalmap) called"
						   "with nullptr PTR.\n");
		return;
	}

	for (uint16_t i {0}; i < NumHyperVortices + 1; i++)
	{
		portalmap[i].star_pt = portalmap_array[i].star_pt;
		portalmap[i].quasi_pt = portalmap_array[i].quasi_pt;
		portalmap[i].nearest_star = FindNearestConstellation(const_cast<StarDesc*>(StarmapArray.data()), portalmap[i].star_pt);
		if (!portalmap[i].nearest_star)
		{
			fmt::print(stderr, "BAD Quasi Portal %c at %05.1f : %05.1f, {}",
					   'A' + i, (float)portalmap[i].star_pt.x / 10,
					   (float)portalmap[i].star_pt.y / 10,
					   "but no star found with FindNearestStar.\n");
		}
	}
}

// Seeds the quasispace to hyperspace portals in hyperspace, ensuring a minimum
// distance between each.  Then randomizes the quasispace vortices using a
// random 0..25 * VortexScale (similar to how they appear to be created),
// and keep them a minimum distance apart as well.
// Then sort the quasi side based on Y value ascending (starmap requirement)
// Then shove the coords into the star_map (QUASI side) in the same order
bool SeedQuasispace(PortalLocation* portalmap, PlotLocation* plotmap,
					StarDesc* starmap)
{
	PortalLocation swap;
	uint16_t rand_val {};
	uint16_t i {};
	uint16_t j {};
	bool valid {};
	if (!portalmap || !plotmap || !starmap)
	{
		fmt::print(stderr, "Seed Quasispace called with nullptr pointer(s).\n");
		return false;
	}
	if (!StarGenRNG)
	{
		fmt::print(stderr, "****SeedQuasispace creating a STAR GEN RNG****\n");
		StarGenRNG.emplace();
	}
	StarGenRNG->reseed(uqm::currentSeed().value);
	// NumHyperVortices is the Arilou Homeworld, but no reason to move it
	for (i = 0; i < NumHyperVortices; i++)
	{
		do
		{
			valid = true;
			// Place a portal near, but not too near, SOL first.
			if (i == 0)
			{
				rand_val = StarGenRNG->next();
				// I know this seems arbitrary but rand(256) * 9 gives a value
				// between 0 and 2295.  Half of that is below 1147, the other
				// half above.
				// We locate the portal by up to 1147 (114.7) in any direction.
				// Also note int16_t is just int16_t and can be negative.
				portalmap[i].star_pt = GfxPoint {
					(int16_t)uqm::lowByte(rand_val) * 9 + plotmap[*PlotId::Sol].star_pt.x - 1147,
					(int16_t)uqm::highByte(rand_val) * 9 + plotmap[*PlotId::Sol].star_pt.y - 1147};
				// Take care of anything off the map (w/in 18.9) or too close
				// to SOL.  Here, 250000 (50.0 squared) is the min distance
				// squared from SOL to portal and 1000000 (100.0 squared) is
				// the max distance squared from SOL to portal.
				// Note: as long as MinPortal > 1000 + 500 (1500) you don't
				// have to check for other portals too close to SOL
				// (as long as 500 is that limit).
				if ((portalmap[i].star_pt.x < 189) || (portalmap[i].star_pt.x > MaxXUniverse - 189) || (portalmap[i].star_pt.y < 189) || (portalmap[i].star_pt.y > MaxYUniverse - 189) || ((portalmap[i].star_pt.x - plotmap[*PlotId::Sol].star_pt.x) * (portalmap[i].star_pt.x - plotmap[*PlotId::Sol].star_pt.x) + (portalmap[i].star_pt.y - plotmap[*PlotId::Sol].star_pt.y) * (portalmap[i].star_pt.y - plotmap[*PlotId::Sol].star_pt.y) < 250000) || ((portalmap[i].star_pt.x - plotmap[*PlotId::Sol].star_pt.x) * (portalmap[i].star_pt.x - plotmap[*PlotId::Sol].star_pt.x) + (portalmap[i].star_pt.y - plotmap[*PlotId::Sol].star_pt.y) * (portalmap[i].star_pt.y - plotmap[*PlotId::Sol].star_pt.y) > 1000000))
				{
					valid = false;
					continue;
				}
			}
			// Place a portal, make sure it is MinPortal from the others
			else
			{
				rand_val = StarGenRNG->next();
				// Squeeze gently at the sides to prevent being close to
				// map edges.  This should give a value between
				// 189 [0] and 9810 [255].
				// ([rnd] 255 * 9999 / 265 = 9621) + 189 = 9810
				// (or 189 below the top)
				portalmap[i].star_pt = GfxPoint {uqm::lowByte(rand_val) * MaxXUniverse / 265 + 189,
												 uqm::highByte(rand_val) * MaxYUniverse / 265 + 189};
				for (j = 0; j < i; j++)
				{
					if ((portalmap[i].star_pt.x - portalmap[j].star_pt.x) * (portalmap[i].star_pt.x - portalmap[j].star_pt.x) + (portalmap[i].star_pt.y - portalmap[j].star_pt.y) * (portalmap[i].star_pt.y - portalmap[j].star_pt.y) < MinPortal * MinPortal)
					{
						valid = false;
					}
				}
				if (!valid)
				{
					continue;
				}
			}
			if (portalmap[i].star_pt.x <= 0 || portalmap[i].star_pt.x >= MaxXUniverse || portalmap[i].star_pt.y <= 0 || portalmap[i].star_pt.y >= MaxYUniverse)
			{
				fmt::print(stderr, "BAD Quasi Portal %c at %05.1f : %05.1f, {}",
						   'A' + i, (float)portalmap[i].star_pt.x / 10,
						   (float)portalmap[i].star_pt.y / 10,
						   "but no star found with FindNearestStar.\n");
				valid = false;
				continue;
			}
			// Find the nearest constellation name to assign to this portal
			portalmap[i].nearest_star = FindNearestConstellation(star_array.data(), portalmap[i].star_pt);
			if (!portalmap[i].nearest_star)
			{
				fmt::print(stderr, "BAD Quasi Portal %c at %05.1f : %05.1f, {}",
						   'A' + i, (float)portalmap[i].star_pt.x / 10,
						   (float)portalmap[i].star_pt.y / 10,
						   "but no star found with FindNearestStar.\n");
				valid = false;
				continue;
			}
			if ((portalmap[i].nearest_star->star_pt.x - portalmap[i].star_pt.x) * (portalmap[i].nearest_star->star_pt.x - portalmap[i].star_pt.x) + (portalmap[i].nearest_star->star_pt.y - portalmap[i].star_pt.y) * (portalmap[i].nearest_star->star_pt.y - portalmap[i].star_pt.y) < 400)
			{
#ifdef DEBUG_STARSEED
				fmt::print(stderr, "Picked Quasi Portal %c at %05.1f : %05.1f, ",
						   'A' + i, (float)portalmap[i].star_pt.x / 10,
						   (float)portalmap[i].star_pt.y / 10);
				fmt::print(stderr, "nearest star found at %05.1f : %05.1f, ",
						   (float)portalmap[i].nearest_star->star_pt.x / 10,
						   (float)portalmap[i].nearest_star->star_pt.y / 10);
				fmt::print(stderr, "portal TOO CLOSE to star.\n");
#endif
				valid = false;
				continue;
			}
		} while (!valid);
		valid = false;
		// Now we give a random vortex scaled position,
		// keeping specific distances in mind.
		while (!valid)
		{
			valid = true;
			rand_val = StarGenRNG->next();
			portalmap[i].quasi_pt = GfxPoint {(uqm::lowByte(rand_val) * 51 / 256 - 25) * VortexScale + 5000,
											  (uqm::highByte(rand_val) * 51 / 256 - 25) * VortexScale + 5000};
			for (j = 0; j < i; j++)
			{
				if ((portalmap[i].quasi_pt.x - portalmap[j].quasi_pt.x) * (portalmap[i].quasi_pt.x - portalmap[j].quasi_pt.x) + (portalmap[i].quasi_pt.y - portalmap[j].quasi_pt.y) * (portalmap[i].quasi_pt.y - portalmap[j].quasi_pt.y) < MinVortex * MinVortex)
				{
					valid = false;
				}
			}
			if ((portalmap[i].quasi_pt.x - 5000) * (portalmap[i].quasi_pt.x - 5000) + (portalmap[i].quasi_pt.y - 5000) * (portalmap[i].quasi_pt.y - 5000) < MinVortex * MinVortex)
			{
				valid = false;
			}
		}
	}
	// Now we must sort the quasi side and place in the starmap
	valid = false;
	while (!valid)
	{
		valid = true;
		// Yes I know bubble sorts are lazy but we really only do this once.
		for (i = 0; i < NumHyperVortices - 1; i++)
		{
			for (j = i + 1; j < NumHyperVortices; j++)
			{
				if (portalmap[i].quasi_pt.y > portalmap[j].quasi_pt.y || (portalmap[i].quasi_pt.y == portalmap[j].quasi_pt.y && portalmap[i].quasi_pt.x > portalmap[j].quasi_pt.x))
				{
					valid = false;
					swap = portalmap[i];
					portalmap[i] = portalmap[j];
					portalmap[j] = swap;
				}
			}
		}
	}
	for (i = 0, j = NumSolarSystems + 1; i < NumHyperVortices; ++i, ++j)
	{
		starmap[j].star_pt = portalmap[i].quasi_pt;
	}
	return true;
}

struct PlotIdMap
{
	const char* idStr;
	uint16_t id;
};

// The PlotIdMap is sorted **by name** for the binary search function.
static constexpr uqstl::array<PlotIdMap, 70> plotIdMap {
	{
		{"algolites", *PlotId::Algolites},	 // Algol [IV]
		{"androsynth", *PlotId::Androsynth}, // Eta Vulpeculae
		{"aqua helix", *PlotId::AquaHelix},	 // Zeta Draconis
		{"arilou", *PlotId::Arilou},		 // 43.8 : 637.2
		{"bomb", *PlotId::Bomb},			 // Zeta Hyades [VI-B]
		{"burvixese", *PlotId::Burvixese},	 // Arcturus [I]
		{"chmmr", *PlotId::Chmmr},			 // Procyon
		{"destroyed starbase", *PlotId::DestroyedStarbase},
		{"druuge", *PlotId::Druuge},	   // Zeta Persei [I]
		{"egg case 0", *PlotId::EggCase0}, // Beta Copernicus [I]
		{"egg case 1", *PlotId::EggCase1},
		{"egg case 2", *PlotId::EggCase2},
		{"excavation site", *PlotId::ExcavationSite}, // ALPHA LALANDE I
		{"ilwrath", *PlotId::Ilwrath},				  // .. / .. / 022.9, 366.6
		{"kohrah 0", *PlotId::Kohrah0},
		{"kohrah 1", *PlotId::Kohrah1},
		{"kohrah 2", *PlotId::Kohrah2},
		{"maidens", *PlotId::Maidens}, // Alpha Cerenkov [I]
		{"melnorme 0", *PlotId::Melnorme0},
		{"melnorme 1", *PlotId::Melnorme1}, // Alpha Centauri (SOL)
		{"melnorme 2", *PlotId::Melnorme2},
		{"melnorme 3", *PlotId::Melnorme3},
		{"melnorme 4", *PlotId::Melnorme4},
		{"melnorme 5", *PlotId::Melnorme5},
		{"melnorme 6", *PlotId::Melnorme6},
		{"melnorme 7", *PlotId::Melnorme7}, // Alpha Apodis (Thraddash)
		{"melnorme 8", *PlotId::Melnorme8},
		{"mother ark", *PlotId::MotherArk}, //Delta Virginis (not said)
		{"mycon", *PlotId::Mycon},			// Brahe / 629.1, 220.8
		{"mycon trap", *PlotId::MyconTrap}, // Organon [1] [I]
		{"orz", *PlotId::Orz},				// Gamma Vulpeculae
		{"pkunk", *PlotId::Pkunk},			// Gamma Krueger [I]
		{"rainbow 0", *PlotId::Rainbow0},	// Zeta Sextantis (Yehat)
		{"rainbow 1", *PlotId::Rainbow1},	//
		{"rainbow 2", *PlotId::Rainbow2},	//
		{"rainbow 3", *PlotId::Rainbow3},	//
		{"rainbow 4", *PlotId::Rainbow4},	// Slylandro (blue pair)
		{"rainbow 5", *PlotId::Rainbow5},	// Epsilon Draconis (Thradd)
		{"rainbow 6", *PlotId::Rainbow6},
		{"rainbow 7", *PlotId::Rainbow7}, // Beta Leporis (Supox)
		{"rainbow 8", *PlotId::Rainbow8},
		{"rainbow 9", *PlotId::Rainbow9},	// GIANT rbw (Groombridge)
											// Thraddash / Slylandro.
		{"samatra", *PlotId::Samatra},		// Delta Crateris
		{"ship vault", *PlotId::ShipVault}, // Epsilon Camelopardalis
		{"shofixti", *PlotId::Shofixti},	// Delta Gorno
		{"slylandro", *PlotId::Slylandro},	// Beta Corvi
		{"sol", *PlotId::Sol},
		{"spathi", *PlotId::Spathi},				  // Epsilon Gruis
		{"spathi monument", *PlotId::SpathiMonument}, // Beta Herculis
		{"start colony", *PlotId::StartColony},		  // Vela
		{"sun device", *PlotId::SunDevice},			  // Beta Brahe
		{"supox", *PlotId::Supox},
		{"syreen", *PlotId::Syreen},				  // Betelgeuse
		{"taalo protector", *PlotId::TaaloProtector}, // Delta Vulpeculae
		{"talking pet", *PlotId::TalkingPet},		  // Beta Orionis
		{"thraddash", *PlotId::Thradd},				  // Delta Draconis
		{"urquan 0", *PlotId::Urquan0},
		{"urquan 1", *PlotId::Urquan1},
		{"urquan 2", *PlotId::Urquan2},
		{"urquan wreck", *PlotId::UrquanWreck}, // Alpha Pavonis
		{"utwig", *PlotId::Utwig},				// Aquarii [constellation]
		{"vux", *PlotId::Vux},					// Luyten [star group]
		{"vux beast", *PlotId::VuxBeast},		// Delta Lyncis [I]
		{"yehat", *PlotId::Yehat},				// .. / Serpentis
		{"zoq colony 0", *PlotId::ZoqColony0},
		{"zoq colony 1", *PlotId::ZoqColony1},
		{"zoq colony 2", *PlotId::ZoqColony2},
		{"zoq colony 3", *PlotId::ZoqColony3},
		{"zoq scout", *PlotId::ZoqScout}, // Rigel
		{"zoqfot", *PlotId::Zoqfot}		  // Alpha Tucanae
	}
}; // Currently 70 total (correct number)

static int PlotIdCompare(const void* id1, const void* id2)
{
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "compare {} and {}.\n",
			   ((PlotIdMap*)id1)->idStr, ((PlotIdMap*)id2)->idStr);
#endif
	return strcmp(((PlotIdMap*)id1)->idStr, ((PlotIdMap*)id2)->idStr);
}

uint16_t PlotIdStrToIndex(const char* plotIdStr)
{
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "START PlotIdStrToIndex {}.\n", plotIdStr);
#endif
	PlotIdMap key = {/* .idStr = */ plotIdStr, /* .id = */ std::numeric_limits<uint16_t>::max()};
	const PlotIdMap* found = (PlotIdMap*)bsearch(&key, plotIdMap.data(), plotIdMap.size(),
												 sizeof plotIdMap[0], PlotIdCompare);
	if (found == nullptr)
	{
		return *PlotId::NumPlots + 1;
	}
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "END PlotIdStrToIndex {} {}.\n", plotIdStr, found->id);
#endif
	return found->id;
}
#if 0
// Arilou 250 and why not Human, Slylandro
// Druuge 1400
// Ilwrath 1410 and Chmrr, Chenjesu technically
// Mycon 1070
// Orz 333 and Androsynth
// Pkunk 666
// Spathi 1000 and Mmrnmhrm (eventually)
// Supox 333	
// Thraddash 833
// Umgah 833 (poor Umgah are defined by their suffering)
// Ur Quan and Kohr Ah both 2666, and Syreen, KA/UQ and use the SAMATRA_DEFINED as homeworld
// Utwig 666
// VUX 900
// Yehat 750 and Shofixti
// Zoq Fot 320
/*
ARILOU_ID:
CHMMR_ID:
EARTHLING_ID:
ORZ_ID:
PKUNK_ID:
SHOFIXTI_ID:
SPATHI_ID:
SUPOX_ID:
THRADDASH_ID:
UTWIG_ID:
VUX_ID:
YEHAT_ID:
MELNORME_ID:
DRUUGE_ID:
ILWRATH_ID:
MYCON_ID:
SLYLANDRO_ID:
UMGAH_ID:
UR_QUAN_ID:
ZOQFOTPIK_ID:
SYREEN_ID:
KOHR_AH_ID:
ANDROSYNTH_ID:
CHENJESU_ID:
MMRNMHRM_ID:
*/

//
// Known location (SoI)		Map homeworld	offset		Jitter	Str		J/S %
// androsyn      0, 0,
// arilou        438, 6372,		438, 6372	same		0		250		0
// chenjesu      0, 0,
// chmmr         0, 0,
// druuge        9500, 2792,	9469, 2806	31, -14		34.015	1400	2.43
// human         1752, 1450,		same
// ilwrath       48, 1700,		229, 3666	Not using this one
// kohrah        6000, 6250,	6200, 5935	-200, 315	373.129	2666	14.00
// melnorme      MaxXUniverse >> 1, MaxYUniverse >> 1,
// mmrnmhrm      0, 0,
// mycon         6392, 2200,	6291, 2208	101, -8		101.316	1070	9.47
// orz           3608, 2637,	3713, 2537	-113, 100	145.000	333		43.54
// pkunk         502, 401,		522, 525	-20, -124	125.603	666		18.86
// shofixti      0, 0,
// slylandr      333, 9812,		same
// spathi        2549, 3600,	2416, 3687	133, -87	158.928	1000	15.89
// supox         7468, 9246,	7414, 9124	54, 122		133.417	333		40.07
// syreen        0, 0,
// thradd        2535, 8358,	2535, 8358	same??		0		833		0
// umgah         1798, 6000,	1978, 5968	-180, 32	182.822	833		21.95
// urquan        5750, 6000,	6200, 5935	-450, 65	454.670	2666	17.05
// utwig         8534, 8797,	8630, 8693	-96, 104	141.534	666		21.25
// vux           4412, 1558,	4333, 1687	79, -129	151.268	900		16.80
// yehat         4970, 40,		4923, 294	47, -254	258.312	750		34.44
// zoqfot        3761, 5333,	4000, 5437	-239, -104	260.647	320		81.45
//
// Default starmaps for races and plots
// {{2908, 269}, MakeStar (DWARF_STAR, BLUE_BODY, -1), SHOFIXTI_DEFINED, 4, 82},
// {{4923, 294}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), YEHAT_DEFINED, 3, 74},
// {{ 522, 525}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), PKUNK_DEFINED, 3, 92},
// {{6858, 577}, MakeStar (DWARF_STAR, GREEN_BODY, -1), MYCON_TRAP_DEFINED, 0, 123},
// {{4681, 916}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), RAINBOW0_DEFINED, 6, 75},
// {{9333, 937}, MakeStar (SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME0_DEFINED, 2, 5},
// {{1559, 993}, MakeStar (SUPER_GIANT_STAR, RED_BODY, -1), MELNORME1_DEFINED, 1, 80},
// {{1752, 1450}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), SOL_DEFINED, 0, 129},
// {{4333, 1687}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), VUX_DEFINED, 2, 88},
// {{3345, 1931}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), START_COLONY_DEFINED, 0, 98},
// {{3352, 1940}, MakeStar (SUPER_GIANT_STAR, WHITE_BODY, -1), MELNORME2_DEFINED, 0, 97},
// {{4221, 1986}, MakeStar (DWARF_STAR, BLUE_BODY, -1), MAIDENS_DEFINED, 1, 100},
// {{6479, 2062}, MakeStar (DWARF_STAR, GREEN_BODY, -1), EGG_CASE1_DEFINED, 3, 71},
// {{2104, 2083}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), ZOQ_SCOUT_DEFINED, 0, 118},
// {{6291, 2208}, MakeStar (DWARF_STAR, GREEN_BODY, -1), MYCON_DEFINED, 5, 71},
// {{ 742, 2268}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), CHMMR_DEFINED, 0, 117},
// {{6395, 2312}, MakeStar (DWARF_STAR, GREEN_BODY, -1), SUN_DEVICE_DEFINED, 2, 12},
// {{3713, 2537}, MakeStar (DWARF_STAR, GREEN_BODY, -1), ORZ_DEFINED, 3, 86},
// {{3587, 2566}, MakeStar (DWARF_STAR, GREEN_BODY, -1), ANDROSYNTH_DEFINED, 7, 86},
// {{3654, 2587}, MakeStar (SUPER_GIANT_STAR, GREEN_BODY, -1), MELNORME3_DEFINED, 1, 86},
// {{3721, 2619}, MakeStar (DWARF_STAR, GREEN_BODY, -1), TAALO_PROTECTOR_DEFINED, 4, 86},
// {{6008, 2631}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), EGG_CASE0_DEFINED, 2, 14},
// {{3499, 2648}, MakeStar (DWARF_STAR, BLUE_BODY, -1), EXCAVATION_SITE_DEFINED, 1, 87},
// {{6354, 2729}, MakeStar (DWARF_STAR, WHITE_BODY, -1), EGG_CASE2_DEFINED, 3, 12},
// {{9469, 2806}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), DRUUGE_DEFINED, 6, 61},
// {{6020, 2979}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), RAINBOW1_DEFINED, 3, 13},
// {{9000, 3250}, MakeStar (DWARF_STAR, RED_BODY, -1), ALGOLITES_DEFINED, 0, 113},
// {{2354, 3291}, MakeStar (SUPER_GIANT_STAR, RED_BODY, -1), MELNORME4_DEFINED, 1, 106},
// {{1104, 3333}, MakeStar (DWARF_STAR, WHITE_BODY, -1), MOTHER_ARK_DEFINED, 4, 84},
// {{1758, 3418}, MakeStar (DWARF_STAR, BLUE_BODY, -1), SPATHI_MONUMENT_DEFINED, 2, 108},
// {{ 229, 3666}, MakeStar (DWARF_STAR, GREEN_BODY, -1), ILWRATH_DEFINED, 1, 76},
// {{2416, 3687}, MakeStar (GIANT_STAR, ORANGE_BODY, -1), SPATHI_DEFINED, 5, 109},
// {{4125, 3770}, MakeStar (DWARF_STAR, BLUE_BODY, -1), SYREEN_DEFINED, 0, 114},
// {{5937, 3937}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), SHIP_VAULT_DEFINED, 5, 10},
// {{5708, 4604}, MakeStar (DWARF_STAR, RED_BODY, -1), URQUAN0_DEFINED, 0, 104},
// {{5006, 5011}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), URQUAN1_DEFINED, 3, 11},
// {{3679, 5068}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), ZOQ_COLONY0_DEFINED, 3, 17},
// {{7416, 5083}, MakeStar (DWARF_STAR, GREEN_BODY, -1), RAINBOW2_DEFINED, 3, 68},
// {{3770, 5250}, MakeStar (DWARF_STAR, GREEN_BODY, -1), ZOQ_COLONY1_DEFINED, 2, 17},
// {{7020, 5291}, MakeStar (DWARF_STAR, BLUE_BODY, -1), KOHRAH0_DEFINED, 2, 68},
// {{3416, 5437}, MakeStar (DWARF_STAR, BLUE_BODY, -1), ZOQ_COLONY3_DEFINED, 2, 16},
// {{4000, 5437}, MakeStar (DWARF_STAR, GREEN_BODY, -1), ZOQFOT_DEFINED, 1, 18},
// {{3937, 5625}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), ZOQ_COLONY2_DEFINED, 2, 18},
// {{9645, 5791}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), BURVIXESE_DEFINED, 0, 130},
// {{6200, 5935}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), SAMATRA_DEFINED, 4, 21},
// {{1978, 5968}, MakeStar (DWARF_STAR, GREEN_BODY, -1), TALKING_PET_DEFINED, 2, 54},
// {{4625, 6000}, MakeStar (DWARF_STAR, BLUE_BODY, -1), URQUAN2_DEFINED, 1, 131},
// {{1578, 6668}, MakeStar (SUPER_GIANT_STAR, GREEN_BODY, -1), MELNORME5_DEFINED, 1, 56},
// {{5145, 6958}, MakeStar (DWARF_STAR, BLUE_BODY, -1), KOHRAH1_DEFINED, 8, 28},
// {{8625, 7000}, MakeStar (DWARF_STAR, GREEN_BODY, -1), RAINBOW3_DEFINED, 1, 41},
// {{ 395, 7458}, MakeStar (DWARF_STAR, BLUE_BODY, -1), RAINBOW4_DEFINED, 2, 60},
// {{6479, 7541}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), KOHRAH2_DEFINED, 0, 38},
// {{5875, 7729}, MakeStar (SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME6_DEFINED, 1, 34},
// {{2836, 7857}, MakeStar (DWARF_STAR, WHITE_BODY, -1), RAINBOW5_DEFINED, 5, 53},
// {{ 562, 8000}, MakeStar (GIANT_STAR, GREEN_BODY, -1), URQUAN_WRECK_DEFINED, 1, 59},
// {{5437, 8270}, MakeStar (DWARF_STAR, RED_BODY, -1), RAINBOW6_DEFINED, 5, 48},
// {{2535, 8358}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), THRADD_DEFINED, 4, 53},
// {{2582, 8507}, MakeStar (SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME7_DEFINED, 1, 2},
// {{7666, 8666}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), RAINBOW7_DEFINED, 2, 46},
// {{2776, 8673}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), AQUA_HELIX_DEFINED, 6, 53},
// {{8630, 8693}, MakeStar (DWARF_STAR, GREEN_BODY, -1), UTWIG_DEFINED, 2, 3},
// {{8534, 8797}, MakeStar (DWARF_STAR, ORANGE_BODY, -1), RAINBOW8_DEFINED, 3, 3},
// {{ 333, 8916}, MakeStar (DWARF_STAR, GREEN_BODY, -1), DESTROYED_STARBASE_DEFINED, 3, 57}, // here
// {{9960, 9042}, MakeStar (GIANT_STAR, WHITE_BODY, -1), RAINBOW9_DEFINED, 0, 42},
// {{7414, 9124}, MakeStar (DWARF_STAR, GREEN_BODY, -1), SUPOX_DEFINED, 2, 47},
// {{8500, 9372}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), BOMB_DEFINED, 6, 45},
// {{9159, 9745}, MakeStar (SUPER_GIANT_STAR, BLUE_BODY, -1), MELNORME8_DEFINED, 1, 4},
// {{5704, 9795}, MakeStar (DWARF_STAR, YELLOW_BODY, -1), VUX_BEAST_DEFINED, 4, 49},
// {{ 333, 9812}, MakeStar (DWARF_STAR, GREEN_BODY, -1), SLYLANDRO_DEFINED, 2, 27},
#endif

} // namespace uqm
