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

#include "core/types/enum.h"
#include "libs/mathlib.h"
#include "libs/sndlib.h"
#include "options.h"
#include "../units.h"
#include "../globdata.h"

inline constexpr auto EndInterplanetary {ActivityFlag::StartInterplanetary};

inline constexpr auto OneYear {365.25};
#ifndef M_PI
#define M_PI 3.141592653589
#endif

enum class ScanType
{
	Mineral = 0,
	Energy,
	Biological,

	Count,
};
template <>
inline constexpr bool IsArithmeticEnum<ScanType> = true;

inline constexpr auto Sc2MapWidth {210};
inline constexpr auto Sc2MapHeight {67};
inline constexpr auto UqmMapWidth {243};
inline constexpr auto UqmMapHeight {75};
inline auto OriginalMapWidth() { return (uqm::UQMOptions::read().planetTexture ? UqmMapWidth - 1 : Sc2MapWidth); }
inline auto OriginalMapHeight() { return (uqm::UQMOptions::read().planetTexture ? UqmMapHeight : Sc2MapHeight); }
inline auto MapWidth() { return ResScale(UqmMapWidth); }
inline auto MapHeight() { return ResScale(Sc2MapHeight); }
inline auto ScaledMapWidth() { return ResScale(((is3DO(uqm::UQMOptions::read().landerStyle) ? UqmMapWidth : Sc2MapWidth)) - SafeNum(33)); }
inline auto UnscaledMapWidth() { return (((is3DO(uqm::UQMOptions::read().landerStyle) ? UqmMapWidth : Sc2MapWidth)) - SafeNum(33)); }

inline constexpr GfxExtent MapDimensions[] {
	{Sc2MapWidth,		  Sc2MapHeight},
	{(UqmMapWidth - 1), UqmMapHeight},
};

enum class LanderEvent
{
	BiologicalDisaster = 0,
	EarthquakeDisaster,
	LightningDisaster,
	LavaspotDisaster,

	/* additional lander sounds */
	LanderInjured,
	LanderShoots,
	LanderHits,
	LifeformCanned,
	LanderPickup,
	LanderFull,
	LanderDeparts,
	LanderReturns,
	LanderDestroyed
};
template <>
inline constexpr bool IsArithmeticEnum<LanderEvent> = true;

inline auto MaxScrounged() { return (isPC(uqm::UQMOptions::read().landerStyle) ? 64 : 50); }
inline auto MaxHoldBars() { return (isPC(uqm::UQMOptions::read().landerStyle) ? 64 : 50); }

constexpr auto ScaleRadius(const auto& r) { return ((r) << 6); }
constexpr auto UnscaleRadius(const auto& r) { return ((r) >> 6); }
inline constexpr auto MaxZoomRadius {ScaleRadius(128)};
inline constexpr auto MinZoomRadius {(MaxZoomRadius >> 3)};
inline constexpr auto EarthRadius {ScaleRadius(8)};
inline constexpr auto EarthHours {240};

inline constexpr auto MinPlanetRadius {ScaleRadius(4)};
inline constexpr auto MaxPlanetRadius {ScaleRadius(124)};

inline auto DisplayFactor() { return ResScale((OrigSisScreenWidth() >> 1) - 8); }
inline auto DisplayFactorUs() { return ((OrigSisScreenWidth() >> 1) - 8); }

inline constexpr auto NumScandotTransitions {4};

inline auto MinMoonRadius() { return ResScale(35); }
inline auto MoonDelta() { return ResScale(20); }

inline constexpr auto MaxSuns {1};
inline constexpr auto MaxPlanets {16};
inline constexpr auto MaxMoons {4};
inline constexpr auto MaxGenMoons {MaxMoons};
inline constexpr auto MaxGenPlanets {9};

inline auto MapBorderHeight() { return ResScale(5); }

//#define ScanScreenHeight() \//
//		(SisScreenHeight() - MapHeight() - MapBorderHeight())

// Unscaled so that the math generates a proper output for HdResolutionFactor
inline auto ScanScreenHeight() { return (OrigSisScreenHeight() - Sc2MapHeight - ResDescale(MapBorderHeight())); }

inline auto PcRotationRate() { return (c_gameSecond / ResScale(12)); }
inline auto ThreedoRotationRate() { return (c_gameSecond * 6 / ResScale(Sc2MapWidth)); }
inline auto UqmRotationRate() { return (c_gameSecond * 12 / MapWidth()); }

constexpr auto PlanetRotationRate(const auto& a) { return (!a ? PcRotationRate() : (a == 1 ? ThreedoRotationRate() : UqmRotationRate())); }

extern bool useDosSpheres;
inline auto UseDosSpheres() { return ResScale(useDosSpheres ? 2 : 0); }

inline auto PlanetOrgX() { return ResScale(OrigSisScreenWidth() >> 1); }
inline auto PlanetOrgY() { return ResScale(ScanScreenHeight() >> 1) - UseDosSpheres(); }

inline constexpr auto NumRaceRuins {16};

constexpr auto MinBrightness(const auto& a) { return ((a == ScanType::Energy) ? (50) : (55)); }
constexpr auto MaxBrightness(const auto& a) { return ((a == ScanType::Energy) ? (90) : (135)); }

struct SystemPlanet;
struct StarDesc;
struct SurfaceNode;
struct PlanetOrbit;
struct SolarSystemState;
// JSD plot_map is an array of "plot locations" for each plot in the game,
// the contents of which are COORDS to the star system (for dialogs and fleets)
// as well as internal required distance parameters used during the initial seed.
// It also might make sense for this and StarDesc to be defined in starmap.h and
// the corresponding functions & the star_map object in starmap.c.
struct PlotLocation;
struct PortalLocation;

#include "generate.h"
#include "lifeform.h"
#include "plandata.h"
#include "sundata.h"
#include "../gendef.h" //JSD need gendef.h unless we move plots & starmap to starmap
#include "uqm/random/UQMRandom.h"

struct MapPoint3D
{
	GfxPoint p[4];
	uint32_t m[4];
};

struct PlanetOrbit
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
	MapPoint3D** map_rotate;
	// doubly dynamically allocated depending on map size

	// stuff to draw DOS spheres
	FRAME TopoMask;
	uint8_t* sphereBytes;
	COLORMAP sphereMap;
	ScanType scanType;

	FRAME Shade;
	Color* ShadeColors;
};

#if 0 //defined(__cplusplus)
extern "C" {
#endif

struct SystemPlanet
{
	uint32_t rand_seed; // seed for topography and node generation

	uint8_t data_index; // what planet is this
	uint8_t NumPlanets; // number of moons
	int16_t radius;		// radius of planet orbit
	GfxPoint location;	// coords on screen

	Color temp_color;	// color of planet orbit
	uint16_t NextIndex; // index to a next planet
	GfxStamp image;		// image of a planet in IP view

	SystemPlanet* pPrevDesc;
	// The Sun or planet that this world is orbiting around.

	// BW : new stuff for animated solar systems
	PlanetOrbit orbit; // Link to moon(s)
	uint16_t size;	   // size of a planet

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

struct StarDesc
{
	GfxPoint star_pt;
	uint8_t Type;
	uint8_t Index;
	uint8_t Prefix;
	uint8_t Postfix;
};

struct SurfaceNode
{
	// This structire is filled in when a generateMinerals, generateEnergy,
	// or generateLife call is made.
	GfxPoint loc_pt;
	// Position of the mineral/bio/energy node on the planet.
	uint16_t density;
	// For bio and energy: undefined
	// For minerals the low byte is the gross size of the
	// deposit (this determines the image), and the high
	// byte is the fine size (the actual quantity).
	uint16_t type;
	// For minerals: the type of Element
	// For bio: the type of the creature.
	//       0 through NumCreatureTypes - 1 are normal creatures,
	//       NumCreatureTypes     is an Evil One
	//       NumCreatureTypes + 1 is a Brainbox Bulldozer
	//       NumCreatureTypes + 2 is Zex' Beauty
	// For energy: undefined
};

// See doc/devel/generate for information on how this structure is filled.
struct SolarSystemState
{
	// Standard field required by DoInput()
	bool (*InputFunc)(struct SolarSystemState*);

	bool InIpFlight;
	// Set to true when player is flying around in interplanetary
	// Reset to false when going into orbit or Encounter

	uint16_t WaitIntersect;
	// Planet/moon number with which the flagship should not
	// collide. For example, if the player just left the planet or
	// inner system. If set to (uint16_t)~0, all planet collisions are
	// disabled until the flagship stops intersecting with all
	// planets.
	SystemPlanet SunDesc[MaxSuns];
	SystemPlanet PlanetDesc[MaxPlanets];
	// Description of the planets in the system.
	// Only defined after a call to (*genFuncs)->generatePlanets()
	// and overwritten by subsequent calls.
	SystemPlanet MoonDesc[MaxGenMoons];
	// Description of the moons orbiting the planet pointed to
	// by pBaseDesc.
	// Only defined after a call to (*genFuncs)->generateMoons()
	// as its argument, and overwritten by subsequent calls.
	SystemPlanet* pBaseDesc;
	// In outer system: points to PlanetDesc[]
	// In inner system: points to MoonDesc[]
	SystemPlanet* pOrbitalDesc;
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

	SolarSystemInfo SysInfo;

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
	PlanetOrbit Orbit;
	bool InOrbit;
	// Set to true when player hits a world in an inner system
	// Homeworld encounters count as 'in orbit'
	FRAME ScanFrame[*ScanType::Count];
	// For PC scan, generated from TopoFrame
};

// The other part of PlotLocation
// star_pt is the coords where the plot is seeded, for public consumption.
// star is the pointer to the star description on the starmap on which this
// plotmap was seeded.
// Because Arilou have no hyperspace home, must keep star_pt.  Because "starmap"
// could be any variable, need to have pointer to star in map.
// dist_sq is the min or max distance to the plot in the array (it stores both),
// or the "weight" of the plot at that location when called reflexively.
// dist_sq is internal and accessed by the "PLOT_" defines on "plot" variable.
struct PlotLocation
{
	GfxPoint star_pt;
	StarDesc* star;
	uint32_t dist_sq[*PlotId::NumPlots];
};

// PortalLocation aka PortalLocation.  The star_pt is the location in
// hyperspace, while quasi_pt is the location in quasispace.  The
// nearest_star variable is the pointer to the star description on the
// starmap provided of the nearest actual star.
struct PortalLocation
{
	GfxPoint star_pt;
	GfxPoint quasi_pt;
	StarDesc* nearest_star;
};

extern SolarSystemState* pSolarSysState;
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
bool worldIsPlanet(const SolarSystemState* solarSys,
				   const SystemPlanet* world);
bool worldIsMoon(const SolarSystemState* solarSys,
				 const SystemPlanet* world);
uint16_t planetIndex(const SolarSystemState* solarSys,
					 const SystemPlanet* world);
uint16_t moonIndex(const SolarSystemState* solarSys, const SystemPlanet* moon);
inline constexpr auto MatchPlanet {((uint8_t)-1)};
inline constexpr auto MatchPbyte {((uint8_t)-2)};
inline constexpr auto MatchMbyte {((uint8_t)-3)};
bool matchWorld(const SolarSystemState* solarSys, const SystemPlanet* world,
				uint8_t planetI, uint8_t moonI);

uint32_t GetRandomSeedForStar(const StarDesc* star);

GfxPoint locationToDisplay(GfxPoint pt, int16_t scaleRadius);
GfxPoint displayToLocation(GfxPoint pt, int16_t scaleRadius);
GfxPoint planetOuterLocation(uint16_t planetI);

extern void DestroyOrbitStruct(PlanetOrbit* Orbit, int16_t height);
extern void LoadPlanet(FRAME SurfDefFrame);
extern void DrawPlanet(int dy, Color tintColor);
extern void DrawPCScanTint(uint16_t cur_scan);
extern void FreePlanet(void);
extern void LoadStdLanderFont(PlanetData* info);
extern void FreeLanderFont(PlanetData* info);

extern void ExploreSolarSys(void);
extern void DrawStarBackGround(void);
extern FRAME GetStarBackFround(void);
extern void XFormIPLoc(GfxPoint* pIn, GfxPoint* pOut, bool ToDisplay);
extern void DrawOval(LargeRect* pRect, uint8_t num_off_pixels, bool scaled);
extern void DrawFilledOval(LargeRect* pRect);
extern void DrawEllipse(int cx, int cy, int rx, int ry, int shear,
						int filled, int dotted);
extern void DrawRotatedEllipse(int cx, int cy, int rx, int ry,
							   int angle_deg, int filled, int dotted);
extern void ComputeSpeed(SystemPlanet* planet, bool GeneratingMoons,
						 uint16_t rand_val);
inline constexpr auto NumplanetsPdesc {((uint8_t)-2)};
extern void FillOrbits(SolarSystemState* system, uint8_t NumPlanets,
					   SystemPlanet* pBaseDesc, bool TypesDefined);
extern void InitLander(uint8_t LanderFlags);
extern void InitPCLander(bool Loading);
extern void DestroyPCLanderContext(void);

extern void InitSphereRotation(int direction, bool shielded,
							   uint16_t width, uint16_t height);
extern void UninitSphereRotation(void);
extern void PrepareNextRotationFrame(void);
extern void PrepareNextRotationFrameForIP(SystemPlanet* pPlanetDesc,
										  int16_t frameCounter);
extern void DrawPlanetSphere(int x, int y, bool back);
extern void DrawDefaultPlanetSphere(void);
extern void RerenderPlanetSphere(void);
extern void RenderDOSPlanetSphere(PlanetOrbit* Orbit, FRAME MaskFrame,
								  int offset);
extern void Render3DOPlanetSphere(PlanetOrbit* Orbit, FRAME MaskFrame,
								  int offset, uint16_t rotwidth, uint16_t height);
extern void RenderPlanetSphere(PlanetOrbit* Orbit, FRAME Frame,
							   int offset, bool shielded, bool doThrob, uint16_t width,
							   uint16_t height, uint16_t radius);
extern void SetShieldThrobEffect(FRAME FromFrame, int offset,
								 FRAME ToFrame);
extern void Draw3DOShield(GfxStamp ShieldFrame);

extern void ZoomInPlanetSphere(void);
extern void RotatePlanetSphere(bool keepRate, GfxStamp* onTop);

extern void DrawScannedObjects(bool Reversed);
extern void GetPlanetTopography(SystemPlanet* pPlanetDesc, FRAME SurfDefFrame);
extern void GeneratePlanetSurface(SystemPlanet* pPlanetDesc,
								  FRAME SurfDefFrame, uint16_t width, uint16_t height);
extern void DeltaTopography(uint16_t num_iterations, int8_t* DepthArray,
							GfxRect* pRect, int16_t depth_delta);

extern void TransformColor(Color* c, uint16_t scan);

extern void DrawPlanetSurfaceBorder(void);

extern FRAME GetStarBackGround(bool Encounter);
const extern char* GetNamedPlanetaryBody(void);
extern void GetPlanetOrMoonName(char* buf, uint16_t bufsize);

extern void PlanetOrbitMenu(void);
extern void SaveSolarSysLocation(void);

extern uint8_t PickClosestHabitable(SolarSystemState* solarSys);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_PLANETS_PLANETS_H_ */
