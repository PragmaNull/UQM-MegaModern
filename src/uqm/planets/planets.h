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

#ifndef UQM_PLANETS_PLANETS_H_
#define UQM_PLANETS_PLANETS_H_

#include "libs/mathlib.h"
#include "libs/sndlib.h"
#include "options.h"
#include "../units.h"
#include "../globdata.h"

inline constexpr auto EndInterplanetary {START_INTERPLANETARY};

inline constexpr auto OneYear {365.25};
#ifndef M_PI
#define M_PI 3.141592653589
#endif

enum PlanetScanTypes
{
	MINERAL_SCAN = 0,
	ENERGY_SCAN,
	BIOLOGICAL_SCAN,

	NUM_SCAN_TYPES,
};

inline constexpr auto Sc2MapWidth {210};
inline constexpr auto Sc2MapHeight {67};
inline constexpr auto UqmMapWidth {243};
inline constexpr auto UqmMapHeight {75};
inline auto OriginalMapWidth() { return (uqm::UQMOptions::read().planetTexture ? UqmMapWidth - 1 : Sc2MapWidth); }
inline auto OriginalMapHeight() { return (uqm::UQMOptions::read().planetTexture ? UqmMapHeight : Sc2MapHeight); }
inline auto MapWidth() { return RES_SCALE(UqmMapWidth); }
inline auto MapHeight() { return RES_SCALE(Sc2MapHeight); }
inline auto ScaledMapWidth() { return RES_SCALE(((is3DO(uqm::UQMOptions::read().landerStyle) ? UqmMapWidth : Sc2MapWidth)) - SAFE_NUM(33)); }
inline auto UnscaledMapWidth() { return (((is3DO(uqm::UQMOptions::read().landerStyle) ? UqmMapWidth : Sc2MapWidth)) - SAFE_NUM(33)); }

#define MAP_DIMENSIONS           \
	{Sc2MapWidth, Sc2MapHeight}, \
		{(UqmMapWidth - 1), UqmMapHeight},

enum
{
	BIOLOGICAL_DISASTER = 0,
	EARTHQUAKE_DISASTER,
	LIGHTNING_DISASTER,
	LAVASPOT_DISASTER,

	/* additional lander sounds */
	LANDER_INJURED,
	LANDER_SHOOTS,
	LANDER_HITS,
	LIFEFORM_CANNED,
	LANDER_PICKUP,
	LANDER_FULL,
	LANDER_DEPARTS,
	LANDER_RETURNS,
	LANDER_DESTROYED
};

inline auto MaxScrounged() { return (isPC(uqm::UQMOptions::read().landerStyle) ? 64 : 50); }
inline auto MaxHoldBars() { return (isPC(uqm::UQMOptions::read().landerStyle) ? 64 : 50); }

#define SCALE_RADIUS(r) ((r) << 6)
#define UNSCALE_RADIUS(r) ((r) >> 6)
inline constexpr auto MaxZoomRadius {SCALE_RADIUS(128)};
inline constexpr auto MinZoomRadius {(MaxZoomRadius >> 3)};
inline constexpr auto EarthRadius {SCALE_RADIUS(8)};
inline constexpr auto EarthHours {240};

inline constexpr auto MinPlanetRadius {SCALE_RADIUS(4)};
inline constexpr auto MaxPlanetRadius {SCALE_RADIUS(124)};

inline auto DisplayFactor() { return RES_SCALE((OrigSisScreenWidth() >> 1) - 8); }
inline auto DisplayFactorUs() { return ((OrigSisScreenWidth() >> 1) - 8); }

inline constexpr auto NumScandotTransitions {4};

inline auto MinMoonRadius() { return RES_SCALE(35); }
inline auto MoonDelta() { return RES_SCALE(20); }

inline constexpr auto MaxSuns {1};
inline constexpr auto MaxPlanets {16};
inline constexpr auto MaxMoons {4};
inline constexpr auto MaxGenMoons {MaxMoons};
inline constexpr auto MaxGenPlanets {9};

inline auto MapBorderHeight() { return RES_SCALE(5); }

//#define ScanScreenHeight() \//
//		(SisScreenHeight() - MapHeight() - MapBorderHeight())

// Unscaled so that the math generates a proper output for HdResolutionFactor
inline auto ScanScreenHeight() { return (OrigSisScreenHeight() - Sc2MapHeight - RES_DESCALE(MapBorderHeight())); }

inline auto PcRotationRate() { return (c_gameSecond / RES_SCALE(12)); }
inline auto ThreedoRotationRate() { return (c_gameSecond * 6 / RES_SCALE(Sc2MapWidth)); }
inline auto UqmRotationRate() { return (c_gameSecond * 12 / MapWidth()); }

#define PLANET_ROTATION_RATE(a) (!a ? PcRotationRate() : \
									  (a == 1 ? ThreedoRotationRate() : UqmRotationRate()))

extern bool useDosSpheres;
inline auto UseDosSpheres() { return RES_SCALE(useDosSpheres ? 2 : 0); }

inline auto PlanetOrgX() { return RES_SCALE(OrigSisScreenWidth() >> 1); }
#define PLANET_ORG_Y \
	RES_SCALE(ScanScreenHeight() >> 1) - UseDosSpheres()

inline constexpr auto NumRaceRuins {16};

#define MIN_BRIGHTNESS(a) ((a == ENERGY_SCAN) ? (50) : (55))
#define MAX_BRIGHTNESS(a) ((a == ENERGY_SCAN) ? (90) : (135))

typedef struct planet_desc PLANET_DESC;
typedef struct star_desc STAR_DESC;
typedef struct node_info NODE_INFO;
typedef struct planet_orbit PLANET_ORBIT;
typedef struct solarsys_state SOLARSYS_STATE;
// JSD plot_map is an array of "plot locations" for each plot in the game,
// the contents of which are COORDS to the star system (for dialogs and fleets)
// as well as internal required distance parameters used during the initial seed.
// It also might make sense for this and star_desc to be defined in starmap.h and
// the corresponding functions & the star_map object in starmap.c.
typedef struct plot_location PLOT_LOCATION;
typedef struct portal_location PORTAL_LOCATION;

#include "generate.h"
#include "lifeform.h"
#include "plandata.h"
#include "sundata.h"
#include "../gendef.h" //JSD need gendef.h unless we move plots & starmap to starmap
#include "uqm/random/UQMRandom.h"

typedef struct
{
	GFXPOINT p[4];
	uint32_t m[4];
} MAP3D_POINT;

struct planet_orbit
{
	FRAME TopoZoomFrame;
	// 4x scaled topo image for planet-side
	int8_t* lpTopoData;
	// normal topo data; expressed in elevation levels
	// data is signed for planets other than gas giants
	// transformed to light variance map for 3d planet
	FRAME SphereFrame;
	// rotating 3d planet frames (current and next)
	FRAME ObjectFrame;
	// any extra planetary object (shield, atmo, rings)
	// automatically drawn if present
	FRAME TintFrame;
	// tinted topo images for current scan type (dynamic)
	Color TintColor;
	// the color of the last used tint
	Color* TopoColors;
	// RGBA version of topo image; for 3d planet
	Color** ScanColors;
	// RGBA version of scan colors; for 3d planet
	Color* ScratchArray;
	// temp RGBA data for whatever transforms (nuked often)
	FRAME WorkFrame;
	// any extra frame workspace (for dynamic objects)
	FRAME BackFrame;
	// background frame to make shields transparent with nebulae on
	// BW: extra stuff for animated IP
	uint32_t** light_diff;
	MAP3D_POINT** map_rotate;
	// doubly dynamically allocated depending on map size

	// stuff to draw DOS spheres
	FRAME TopoMask;
	uint8_t* sphereBytes;
	COLORMAP sphereMap;
	uint16_t scanType;

	FRAME Shade;
	Color* ShadeColors;
};

#if 0 //defined(__cplusplus)
extern "C" {
#endif

struct planet_desc
{
	uint32_t rand_seed; // seed for topography and node generation

	uint8_t data_index; // what planet is this
	uint8_t NumPlanets; // number of moons
	int16_t radius;		// radius of planet orbit
	GFXPOINT location;	// coords on screen

	Color temp_color;	// color of planet orbit
	uint16_t NextIndex; // index to a next planet
	STAMP image;		// image of a planet in IP view

	PLANET_DESC* pPrevDesc;
	// The Sun or planet that this world is orbiting around.

	// BW : new stuff for animated solar systems
	PLANET_ORBIT orbit; // Link to moon(s)
	uint16_t size;		// size of a planet

	uint16_t angle;
	int rotFrameIndex, rotPointIndex, rotwidth, rotheight;
	double orb_speed;
	double rot_speed;
	// Handles rotation and orbiting

	uint8_t PlanetByte;
	uint8_t MoonByte;
	// Handles hard coded planets/moons for Custom Seed
	uint8_t frame_offset;
};

struct star_desc
{
	GFXPOINT star_pt;
	uint8_t Type;
	uint8_t Index;
	uint8_t Prefix;
	uint8_t Postfix;
};

struct node_info
{
	// This structire is filled in when a generateMinerals, generateEnergy,
	// or generateLife call is made.
	GFXPOINT loc_pt;
	// Position of the mineral/bio/energy node on the planet.
	uint16_t density;
	// For bio and energy: undefined
	// For minerals the low byte is the gross size of the
	// deposit (this determines the image), and the high
	// byte is the fine size (the actual quantity).
	uint16_t type;
	// For minerals: the type of element
	// For bio: the type of the creature.
	//       0 through NumCreatureTypes - 1 are normal creatures,
	//       NumCreatureTypes     is an Evil One
	//       NumCreatureTypes + 1 is a Brainbox Bulldozer
	//       NumCreatureTypes + 2 is Zex' Beauty
	// For energy: undefined
};

// See doc/devel/generate for information on how this structure is filled.
struct solarsys_state
{
	// Standard field required by DoInput()
	bool (*InputFunc)(struct solarsys_state*);

	bool InIpFlight;
	// Set to true when player is flying around in interplanetary
	// Reset to false when going into orbit or encounter

	uint16_t WaitIntersect;
	// Planet/moon number with which the flagship should not
	// collide. For example, if the player just left the planet or
	// inner system. If set to (uint16_t)~0, all planet collisions are
	// disabled until the flagship stops intersecting with all
	// planets.
	PLANET_DESC SunDesc[MaxSuns];
	PLANET_DESC PlanetDesc[MaxPlanets];
	// Description of the planets in the system.
	// Only defined after a call to (*genFuncs)->generatePlanets()
	// and overwritten by subsequent calls.
	PLANET_DESC MoonDesc[MaxGenMoons];
	// Description of the moons orbiting the planet pointed to
	// by pBaseDesc.
	// Only defined after a call to (*genFuncs)->generateMoons()
	// as its argument, and overwritten by subsequent calls.
	PLANET_DESC* pBaseDesc;
	// In outer system: points to PlanetDesc[]
	// In inner system: points to MoonDesc[]
	PLANET_DESC* pOrbitalDesc;
	// In orbit: points into PlanetDesc or MoonDesc to the planet
	// currently orbiting.
	// In inner system: points into PlanetDesc to the planet whose
	// inner system the ship is inside
	int16_t FirstPlanetIndex, LastPlanetIndex;
	// The planets get sorted on their image.origin.y value.
	// PlanetDesc[FirstPlanetIndex] is the planet with the lowest
	// image.origin.y, and PlanetDesc[LastPlanetIndex] has the
	// highest image.origin.y.
	// PlanetDesc[PlanetDesc[i].NextIndex] is the next planet
	// after PlanetDesc[i] in the ordering.

	uint8_t turn_counter;
	uint8_t turn_wait;
	uint8_t thrust_counter;
	uint8_t max_ship_speed;

	uqm::StringTablePtr XlatTable {};
	const void* XlatPtr;
	uqm::StringTablePtr OrbitalCMap {};
	uint16_t OrbitalCMapIndex;

	SYSTEM_INFO SysInfo;

	const GenerateFunctions* genFuncs;
	// Functions to call to fill in various parts of this
	// structure. See generate.h, doc/devel/generate

	FRAME PlanetSideFrame[3 + MaxLifeVariation];
	/* Frames for planet-side elements.
			 * [0] = bio cannister
			 * [1] = energy node (world-specific)
			 * [2] = unused (formerly static slave shield, presumed)
			 * [3] = bio 1 (world-specific)
			 * [4] = bio 2 (world-specific)
			 * [5] = bio 3 (world-specific)
			 */
	FRAME TopoFrame;
	PLANET_ORBIT Orbit;
	bool InOrbit;
	// Set to true when player hits a world in an inner system
	// Homeworld encounters count as 'in orbit'
	FRAME ScanFrame[NUM_SCAN_TYPES];
	// For PC scan, generated from TopoFrame
};

// The other part of PLOT_LOCATION
// star_pt is the coords where the plot is seeded, for public consumption.
// star is the pointer to the star description on the starmap on which this
// plotmap was seeded.
// Because Arilou have no hyperspace home, must keep star_pt.  Because "starmap"
// could be any variable, need to have pointer to star in map.
// dist_sq is the min or max distance to the plot in the array (it stores both),
// or the "weight" of the plot at that location when called reflexively.
// dist_sq is internal and accessed by the "PLOT_" defines on "plot" variable.
struct plot_location
{
	GFXPOINT star_pt;
	STAR_DESC* star;
	uint32_t dist_sq[NUM_PLOTS];
};

// PORTAL_LOCATION aka portal_location.  The star_pt is the location in
// hyperspace, while quasi_pt is the location in quasispace.  The
// nearest_star variable is the pointer to the star description on the
// starmap provided of the nearest actual star.
struct portal_location
{
	GFXPOINT star_pt;
	GFXPOINT quasi_pt;
	STAR_DESC* nearest_star;
};

extern SOLARSYS_STATE* pSolarSysState;
extern MUSIC_REF SpaceMusic;
extern GFXCONTEXT PlanetContext;
extern bool use3DOSpheres;

// Random context used for all solar system, planets, and surfaces
// generation
extern uqstl::optional<uqm::UQMRandom> SysGenRNG;
extern uqm::UQMRandom* SysGenRNGDebug;

bool playerInSolarSystem(void);
bool playerInPlanetOrbit(void);
bool playerInInnerSystem(void);
bool worldIsPlanet(const SOLARSYS_STATE* solarSys,
				   const PLANET_DESC* world);
bool worldIsMoon(const SOLARSYS_STATE* solarSys,
				 const PLANET_DESC* world);
uint16_t planetIndex(const SOLARSYS_STATE* solarSys,
					 const PLANET_DESC* world);
uint16_t moonIndex(const SOLARSYS_STATE* solarSys, const PLANET_DESC* moon);
inline constexpr auto MatchPlanet {((uint8_t)-1)};
inline constexpr auto MatchPbyte {((uint8_t)-2)};
inline constexpr auto MatchMbyte {((uint8_t)-3)};
bool matchWorld(const SOLARSYS_STATE* solarSys, const PLANET_DESC* world,
				uint8_t planetI, uint8_t moonI);

uint32_t GetRandomSeedForStar(const STAR_DESC* star);

GFXPOINT locationToDisplay(GFXPOINT pt, int16_t scaleRadius);
GFXPOINT displayToLocation(GFXPOINT pt, int16_t scaleRadius);
GFXPOINT planetOuterLocation(uint16_t planetI);

extern void DestroyOrbitStruct(PLANET_ORBIT* Orbit, int16_t height);
extern void LoadPlanet(FRAME SurfDefFrame);
extern void DrawPlanet(int dy, Color tintColor);
extern void DrawPCScanTint(uint16_t cur_scan);
extern void FreePlanet(void);
extern void LoadStdLanderFont(PLANET_INFO* info);
extern void FreeLanderFont(PLANET_INFO* info);

extern void ExploreSolarSys(void);
extern void DrawStarBackGround(void);
extern FRAME GetStarBackFround(void);
extern void XFormIPLoc(GFXPOINT* pIn, GFXPOINT* pOut, bool ToDisplay);
extern void DrawOval(DRECT* pRect, uint8_t num_off_pixels, bool scaled);
extern void DrawFilledOval(DRECT* pRect);
extern void DrawEllipse(int cx, int cy, int rx, int ry, int shear,
						int filled, int dotted);
extern void DrawRotatedEllipse(int cx, int cy, int rx, int ry,
							   int angle_deg, int filled, int dotted);
extern void ComputeSpeed(PLANET_DESC* planet, bool GeneratingMoons,
						 uint16_t rand_val);
inline constexpr auto NumplanetsPdesc {((uint8_t)-2)};
extern void FillOrbits(SOLARSYS_STATE* system, uint8_t NumPlanets,
					   PLANET_DESC* pBaseDesc, bool TypesDefined);
extern void InitLander(uint8_t LanderFlags);
extern void InitPCLander(bool Loading);
extern void DestroyPCLanderContext(void);

extern void InitSphereRotation(int direction, bool shielded,
							   uint16_t width, uint16_t height);
extern void UninitSphereRotation(void);
extern void PrepareNextRotationFrame(void);
extern void PrepareNextRotationFrameForIP(PLANET_DESC* pPlanetDesc,
										  int16_t frameCounter);
extern void DrawPlanetSphere(int x, int y, bool back);
extern void DrawDefaultPlanetSphere(void);
extern void RerenderPlanetSphere(void);
extern void RenderDOSPlanetSphere(PLANET_ORBIT* Orbit, FRAME MaskFrame,
								  int offset);
extern void Render3DOPlanetSphere(PLANET_ORBIT* Orbit, FRAME MaskFrame,
								  int offset, uint16_t rotwidth, uint16_t height);
extern void RenderPlanetSphere(PLANET_ORBIT* Orbit, FRAME Frame,
							   int offset, bool shielded, bool doThrob, uint16_t width,
							   uint16_t height, uint16_t radius);
extern void SetShieldThrobEffect(FRAME FromFrame, int offset,
								 FRAME ToFrame);
extern void Draw3DOShield(STAMP ShieldFrame);

extern void ZoomInPlanetSphere(void);
extern void RotatePlanetSphere(bool keepRate, STAMP* onTop);

extern void DrawScannedObjects(bool Reversed);
extern void GetPlanetTopography(PLANET_DESC* pPlanetDesc, FRAME SurfDefFrame);
extern void GeneratePlanetSurface(PLANET_DESC* pPlanetDesc,
								  FRAME SurfDefFrame, uint16_t width, uint16_t height);
extern void DeltaTopography(uint16_t num_iterations, int8_t* DepthArray,
							GFXRECT* pRect, int16_t depth_delta);

extern void TransformColor(Color* c, uint16_t scan);

extern void DrawPlanetSurfaceBorder(void);

extern FRAME GetStarBackGround(bool encounter);
const extern char* GetNamedPlanetaryBody(void);
extern void GetPlanetOrMoonName(char* buf, uint16_t bufsize);

extern void PlanetOrbitMenu(void);
extern void SaveSolarSysLocation(void);

extern uint8_t PickClosestHabitable(SOLARSYS_STATE* solarSys);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_PLANETS_PLANETS_H_ */
