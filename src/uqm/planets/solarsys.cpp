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

#include "solarsys.h"

#include "../build.h"
#include "../colors.h"
#include "../controls.h"
#include "../menustat.h"
#include "core/types/enum.h"
#include "lander.h"
// for DrawMenuStateStrings()
#include "../gameopt.h"
#include "../gamestr.h"
#include "../gendef.h"
#include "../globdata.h"
#include "../grpinfo.h"
#include "../hyper.h"
#include "../init.h"
#include "../ipdisp.h"
#include "../nameref.h"
#include "../process.h"
#include "../races.h"
#include "../resinst.h"
#include "../save.h"
#include "../settings.h"
#include "../setup.h"
#include "../shipcont.h"
#include "../sis.h"
#include "../sounds.h"
#include "../starmap.h"
#include "../state.h"
#include "../uqmdebug.h"
#include "core/log/log.h"
#include "core/misc/FatalExit.h"
#include "core/string/StringUtils.h"
#include "libs/graphics/cmap.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "options.h"
#include "scan.h"
// for SOL_X/Y

#include <math.h>
#include <SDL3/SDL_version.h>
#include <time.h>

#include "uqm/random/UQMRandom.h"

namespace uqm
{
//#define DEBUG_STARSEED
//#define DEBUG_SOLARSYS
//#define SMOOTH_SYSTEM_ZOOM  1

static constexpr auto IpFrameRate {c_gameSecond / 30};

// BW: those do not depend on the resolution because numbers too small
// cause crashes in the generation and rendering
static constexpr auto SmallMoonDiameter {7 << 2};
static constexpr auto LargeMoonDiameter {11 << 2};
static constexpr auto PlanetDiameter {29 << 2};
constexpr auto GeneratePerimeter(const auto& a) { return (a * OriginalMapWidth() / OriginalMapHeight()); }

static bool DoIpFlight(SolarSystemState* pSS);
static void DrawInnerPlanets(SystemPlanet* planet);
static void DrawOuterPlanets(int16_t radius);
static void DrawSystem(int16_t radius, bool IsInnerSystem);
static void DrawInnerSystem(void);
static void DrawOuterSystem(void);
static void SetPlanetColorMap(SystemPlanet* planet);
static void ValidateInnerOrbits(void);
static void ValidateOrbits(void);
static int16_t scaleSISWidth(int16_t value);
static int16_t scaleSISHeight(int16_t value);
static int16_t widthPick(void);
static int16_t heightPick(void);

// SolarSysMenu() items
enum class SolarSysMenuItem
{
	// XXX: Must match the enum in menustat.h
	Starmap = 1,
	EquipDevice,
	Cargo,
	Roster,
	GameMenu,
	Navigation,
};
consteval bool isArithmeticEnum(SolarSysMenuItem) { return true; }

SolarSystemState* pSolarSysState;
FrameRef SISIPFrame;
FrameRef SunFrame;
FrameRef OrbitalFrame;
FrameRef OrbitalShield;
FrameRef NebulaFrame;
FrameRef SpaceJunkFrame;
ColorMapRef OrbitalCMap;
ColorMapRef SunCMap;
MusicRef SpaceMusic;

int16_t EncounterRace;
uint8_t EncounterGroup;
// last encountered group info

static FrameRef StarsFrame;
// prepared star-field graphic
static FrameRef SolarSysFrame;
// saved solar system view graphic

static GfxRect scaleRect;
// system zooms in when the flagship enters this rect

static uqstl::array<uint16_t, 7> PBodySize {0, 3, 4, 7, 11, 15, 29};
// Planet sizes primary used for textured planets
// There are actually only 6 sizes, but 0 is reached
// only for WORLD_TYPE_SPECIAL and '!= 0' check prevents
// from nuking SpaceJunkFrame in SetPlanetOldFrame()
static constexpr auto UndefinedOffset {(uint8_t)~0};

uqstl::optional<uqm::UQMRandom> SysGenRNG {};
uqm::UQMRandom* SysGenRNGDebug {};

static auto DisplayToLoc() { return (DisplayFactor() >> 1); }


// Star characteristics
static constexpr auto SunAnimframesNum {32};
static constexpr auto SunZoomSizes {5};
static auto SunIndexOffset() { return ((GetFrameCount(SunFrame) - SunZoomSizes) / SunZoomSizes); }
static auto StarIndexMultiplier() { return (SunIndexOffset() ? SunIndexOffset() : 1); }

static auto UseRgbStars() { return ((!IsHd() && NebulaFrame) || (IsHd() && uqm::UQMOptions::read().hyperStars)); }
static auto AnimatedSun() { return (GetFrameCount(SunFrame) > 10); }

// Resources to load
static auto SisIpMask() { return (is3DO(uqm::UQMOptions::read().flagshipColor) ? SisipMaskPmapAnimRedRes : SisipMaskPmapAnimRes); }
static auto PlanetsMask() { return ((isPC(uqm::UQMOptions::read().planetStyle) && !uqm::UQMOptions::read().texturedPlanets) ? DosOrbplanMaskPmapAnimRes : OrbplanMaskPmapAnimRes); }
static auto SunMask() { return (UseRgbStars() ? SunanimMaskPmapAnimRes : SunMaskPmapAnimRes); }

GfxPoint locationToDisplay(GfxPoint pt, int16_t scaleRadius)
{
	GfxPoint out;

	out.x = (ResScale(OrigSisScreenWidth() >> 1)
			 + (long)pt.x * DisplayToLoc() / scaleRadius);
	out.y = (ResScale(OrigSisScreenHeight() >> 1)
			 + (long)pt.y * DisplayToLoc() / scaleRadius);

	return out;
}

GfxPoint displayToLocation(GfxPoint pt, int16_t scaleRadius)
{
	GfxPoint out;

	out.x = (((long)pt.x - ResScale(OrigSisScreenWidth() >> 1))
			 * scaleRadius / DisplayToLoc());
	out.y = (((long)pt.y - ResScale(OrigSisScreenHeight() >> 1))
			 * scaleRadius / DisplayToLoc());

	return out;
}

GfxPoint planetOuterLocation(uint16_t planetI)
{
	const int16_t scaleRadius = pSolarSysState->SunDesc[0].radius;
	return displayToLocation(
		pSolarSysState->PlanetDesc[planetI].image.origin,
		scaleRadius);
}

bool worldIsPlanet(const SolarSystemState* solarSys, const SystemPlanet* world)
{
	return world->pPrevDesc == solarSys->SunDesc.data();
}

bool worldIsMoon(const SolarSystemState* solarSys, const SystemPlanet* world)
{
	return world->pPrevDesc != solarSys->SunDesc.data();
}

// Returns the planet index of the world. If the world is a moon, then
// this is the index of the planet it is orbiting.
uint16_t planetIndex(const SolarSystemState* solarSys, const SystemPlanet* world)
{
	const SystemPlanet* planet = worldIsPlanet(solarSys, world) ?
									 world :
									 world->pPrevDesc;
	return planet - solarSys->PlanetDesc.data();
}

char moonLetter(const SolarSystemState* solarSys, const SystemPlanet* moon)
{
	assert(!worldIsPlanet(solarSys, moon));
	int i = -1;
	while (moon - solarSys->MoonDesc.data() >= 0)
	{
		if (moon->data_index != PlanetType::HierarchyStarbase && moon->data_index != PlanetType::DestroyedStarbase && moon->data_index != PlanetType::PrecursorStarbase)
		{
			i++;
		}
		moon--;
	}
	return i + 'A';
}

uint16_t moonIndex(const SolarSystemState* solarSys, const SystemPlanet* moon)
{
	assert(!worldIsPlanet(solarSys, moon));
	return moon - solarSys->MoonDesc.data();
}

// Test whether 'world' is the planetI-th planet, and if moonI is not
// set to MatchPlanet, also whether 'world' is the moonI-th moon.
bool matchWorld(const SolarSystemState* solarSys, const SystemPlanet* world,
				uint8_t planetI, uint8_t moonI)
{
	const uint8_t PlanetI = (planetI == MatchPbyte ? solarSys->SunDesc[0].PlanetByte : planetI);
	const uint8_t MoonI = (moonI == MatchMbyte ? solarSys->SunDesc[0].MoonByte : moonI);

	// Check whether we have the right planet.
	if (planetIndex(solarSys, world) != PlanetI)
	{
		return false;
	}

	if (MoonI == MatchPlanet)
	{
		// Only test whether we are at the planet.
		if (!worldIsPlanet(solarSys, world))
		{
			return false;
		}
	}
	else
	{
		// Test whether the moon matches too
		if (!worldIsMoon(solarSys, world))
		{
			return false;
		}

		if (moonIndex(solarSys, world) != MoonI)
		{
			return false;
		}
	}

	return true;
}

bool playerInSolarSystem(void)
{
	return pSolarSysState != nullptr;
}

bool playerInPlanetOrbit(void)
{
	return playerInSolarSystem() && pSolarSysState->InOrbit;
}

bool playerInInnerSystem(void)
{
	assert(playerInSolarSystem());
	assert(pSolarSysState->pBaseDesc == pSolarSysState->PlanetDesc.data()
		   || pSolarSysState->pBaseDesc == pSolarSysState->MoonDesc.data());
	return pSolarSysState->pBaseDesc != pSolarSysState->PlanetDesc.data();
}

static void GenerateTexturedMoons(SolarSystemState* system, SystemPlanet* planet)
{
	uint16_t i {};
	int16_t MoonDiameter {};
	SystemPlanet* pMoonDesc {};
	SystemPlanet* previousOrbitalDesc = pSolarSysState->pOrbitalDesc;
	PlanetData* planetInfo = &pSolarSysState->SysInfo.PlanetInfo;

	for (i = 0, pMoonDesc = &system->MoonDesc[0];
		 i < planet->NumPlanets; ++i, ++pMoonDesc)
	{
		ResourceName maskAnim = nullptr;

		// BW : precompute the generated texture to display it in IP
		if (!(pMoonDesc->data_index & *PlanetType::WorldTypeSpecial))
		{
			DoPlanetaryAnalysis(&pSolarSysState->SysInfo, pMoonDesc);

			if (CurStarDescPtr()->Index == PlotId::Sol)
			{
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 0;
			}

			pSolarSysState->pOrbitalDesc = pMoonDesc;

			if (CurStarDescPtr()->Index == PlotId::Sol)
			{ // png defined moons in Sol
				const uint16_t curr_planet_index =
					planetIndex(pSolarSysState, planet);

				switch (curr_planet_index)
				{
					case 2: // EARTH
						if (i == 1)
						{ // LUNA
							maskAnim = IpLunaMaskAnimRes;
							planetInfo->RotationPeriod = 240 * 29;
						}
						break;
					case 4: // JUPITER
						switch (i)
						{
							case 0: // IO
								maskAnim = IpIoMaskAnimRes;
								planetInfo->RotationPeriod = 390;
								break;
							case 1: // EUROPA
								maskAnim = IpEuropaMaskAnimRes;
								planetInfo->RotationPeriod = 840;
								break;
							case 2: // GANYMEDE
								maskAnim = IpGanymedeMaskAnimRes;
								planetInfo->RotationPeriod = 1728;
								break;
							case 3: // CALLISTO
								maskAnim = IpCallistoMaskAnimRes;
								planetInfo->RotationPeriod = 4008;
								break;
						}
						break;
					case 5: // SATURN
						// TITAN
						maskAnim = IpTitanMaskAnimRes;
						planetInfo->RotationPeriod = 3816;
						break;
					case 7: // NEPTUNE
						// TRITON
						maskAnim = IpTritonMaskAnimRes;
						planetInfo->RotationPeriod = 4300;
						break;
					case 8: // PLUTO
						// CHARON
						maskAnim = IpCharonMaskAnimRes;
						planetInfo->RotationPeriod = 6.4 * EarthHours;
						break;
				}
			}

			MoonDiameter = pMoonDesc->data_index > PlanetType::LastSmallRocky ?
							   LargeMoonDiameter :
							   SmallMoonDiameter;
			GeneratePlanetSurface(pMoonDesc,
								  CaptureDrawable(LoadGraphic(maskAnim)),
								  GeneratePerimeter(MoonDiameter), MoonDiameter);
			pMoonDesc->orbit = pSolarSysState->Orbit;
			PrepareNextRotationFrameForIP(pMoonDesc, 0);

			pSolarSysState->XlatTable.reset();
			DestroyDrawable(ReleaseDrawable(pSolarSysState->TopoFrame));
			pSolarSysState->TopoFrame = 0;
			pSolarSysState->OrbitalCMap.reset();
		}
	}
	pSolarSysState->pOrbitalDesc = previousOrbitalDesc;
}

// Sets the SysGenRNG to the required state first.
static void GenerateMoons(SolarSystemState* system, SystemPlanet* planet)
{
	uint16_t i {};
	//uint16_t facing; ununsed
	SystemPlanet* pMoonDesc {};

	SysGenRNG->reseed(planet->rand_seed);

	//facing = NormalizeFacing (AngleToFacing (
	//		ARCTAN (planet->location.x, planet->location.y))); unused
	for (i = 0, pMoonDesc = &system->MoonDesc[0];
		 i < MaxGenMoons; ++i, ++pMoonDesc)
	{
		pMoonDesc->pPrevDesc = planet;
		if (i >= planet->NumPlanets)
		{
			continue;
		}

		pMoonDesc->temp_color = planet->temp_color;
		pMoonDesc->frame_offset = UndefinedOffset;
	}

	// These are moved down here to satisfy the ComputeSpeed function
	(*system->genFuncs->generateName)(system, planet);
	(*system->genFuncs->generateMoons)(system, planet);
}

FrameRef LoadNebulaeFrame(GfxPoint location)
{
	GfxPoint neb_seed;
	const GfxPoint solPoint = plot_map[*PlotId::Sol].star_pt;

	// Kruzen: Either loading from main menu or option is disabled or
	// at Sol with MegaMod nebulas
	if (!uqm::UQMOptions::read().nebulae || !(LastActivity != ActivityFlag::CheckLoad || NextActivity) || (pointsEqual(location, solPoint) && !classicPackPresent && !inHyperSpace()))
	{
		return nullptr;
	}

	if (!uqm::isOriginalSeed())
	{
		uint32_t rand_val {};

		rand_val = uqm::UQMRandom::step(
			uqm::makeDword(location.y, location.x));

		neb_seed.x = uqm::lowWord(rand_val);
		neb_seed.y = uqm::highWord(rand_val);
	}
	else
	{
		neb_seed.x = location.x;
		neb_seed.y = location.y;
	}

	char path[PATH_MAX] {};

	const uqm::StringTablePtr nebulaRes {uqm::loadStringTable(NebulaResourcesRes)};
	const auto nebulaCount {static_cast<uint16_t>(nebulaRes->size())};
	const auto nebulaIndex {static_cast<uint16_t>(neb_seed.x % nebulaCount)};
	fmt::format_to_n(path, sizeof(path) - 1, "{}", nebulaRes->text(nebulaIndex % nebulaCount));

	if ((neb_seed.y % (nebulaCount + 6)) > nebulaCount
		&& !classicPackPresent && !inHyperSpace())
	{
		return nullptr;
	}

	return CaptureDrawable(LoadGraphicFile(path));
}

static constexpr auto StarColorIndex {0xFF};

static void ChangeSunColor(void)
{
	Color c;
	const StellarSize starSize {StarType(CurStarDescPtr()->Type)};
	uint16_t j {};
	uint16_t maskOffset {};
	uint16_t sizeIter {};
	DrawMode mode;

	// Kruzen: An index of a star color in the palette.
	// This is a custom color.
	// Shoved at the end because of unsatisfied results.
	// Cannot use the same index of pre-existing colors
	// for all stars or some will look ugly.

	c = GetColorMapColor(GetColorMapTableIndex(SunCMap, 0), StarColorIndex);
	mode = MAKE_DRAW_MODE(DrawKind::Alpha, TransferAlpha);
	maskOffset = GetFrameCount(SunFrame) - SunZoomSizes;
	sizeIter = maskOffset / SunZoomSizes;

	for (uint16_t i {*starSize}; i <= starSize + 2; i++)
	{
		FrameRef mask = SetAbsFrameIndex(SunFrame, maskOffset + i);
		for (j = 0; j < sizeIter; j++)
		{
			FrameRef star = SetAbsFrameIndex(SunFrame, i * sizeIter + j);
			ApplyMask(mask, star, mode, &c);
		}
	}
}

void FreeIPData(void)
{
	DestroyDrawable(ReleaseDrawable(SISIPFrame));
	SISIPFrame = 0;
	DestroyDrawable(ReleaseDrawable(SunFrame));
	SunFrame = 0;
	delete SunCMap;
	SunCMap = 0;
	delete OrbitalCMap;
	OrbitalCMap = 0;
	DestroyDrawable(ReleaseDrawable(OrbitalFrame));
	OrbitalFrame = 0;
	DestroyDrawable(ReleaseDrawable(OrbitalShield));
	OrbitalShield = 0;
	DestroyDrawable(ReleaseDrawable(SpaceJunkFrame));
	SpaceJunkFrame = 0;
	DestroyDrawable(ReleaseDrawable(NebulaFrame));
	NebulaFrame = 0;
	DestroyMusic(SpaceMusic);
	SpaceMusic = 0;

	SysGenRNG.reset();
}

void LoadIPData(void)
{
	if (SpaceJunkFrame == 0)
	{
		SpaceJunkFrame = CaptureDrawable(
			LoadGraphic(IpbkgndMaskPmapAnimRes));
		SISIPFrame = CaptureDrawable(LoadGraphic(SisIpMask()));

		OrbitalCMap = uqm::loadStringTable(OrbplanColorMapRes).release();
		OrbitalFrame = CaptureDrawable(
			LoadGraphic(PlanetsMask()));
		OrbitalShield = CaptureDrawable(
			LoadGraphic(OrbshldMaskPmapAnimRes));

		NebulaFrame = LoadNebulaeFrame(CurStarDescPtr()->star_pt);

		SunCMap = uqm::loadStringTable(IpsunColorMapRes).release();
		SunFrame = CaptureDrawable(LoadGraphic(SunMask()));

		SpaceMusic = 0;
	}

	if (!SysGenRNG)
	{
		SysGenRNG.emplace();
		SysGenRNGDebug = &*SysGenRNG;
	}
}

static void sortPlanetPositions(void)
{
	uint16_t i {};
	uqstl::array<int16_t, MaxPlanets + 1> sort_array;

	// When this part is done, sort_array will contain the indices to
	// all planets, sorted on their y position.
	// The sun itself, which has its data located at
	// pSolarSysState->PlanetDesc[-1], is included in this array.
	// Very ugly stuff, but it's correct.

	// Initialise sort_array.
	memset(sort_array.data(), 0, sizeof(sort_array));

	for (i = 0; i <= pSolarSysState->SunDesc[0].NumPlanets; ++i)
	{
		sort_array[i] = i - 1;
	}

	// Sort sort_array, based on the positions of the planets/sun.
	for (i = 0; i <= pSolarSysState->SunDesc[0].NumPlanets; ++i)
	{
		for (uint16_t j {pSolarSysState->SunDesc[0].NumPlanets}; j > i; --j)
		{
			int16_t temp {};

			const int16_t real_i {sort_array[i]};
			const int16_t real_j {sort_array[j]};
			if ((pSolarSysState->PlanetDesc[real_i].image.origin.y > pSolarSysState->PlanetDesc[real_j].image.origin.y)
				|| (pSolarSysState->PlanetDesc[real_i].image.origin.y
						== pSolarSysState->PlanetDesc[real_j].image.origin.y
					&& real_i == -1))
			{
				temp = sort_array[i];
				sort_array[i] = sort_array[j];
				sort_array[j] = temp;
			}
		}
	}

	// Put the results of the sorting in the solar system structure.
	pSolarSysState->FirstPlanetIndex = sort_array[0];
	pSolarSysState->LastPlanetIndex =
		sort_array[pSolarSysState->SunDesc[0].NumPlanets];
	for (i = 0; i <= pSolarSysState->SunDesc[0].NumPlanets; ++i)
	{
		SystemPlanet* planet = &pSolarSysState->PlanetDesc[sort_array[i]];
		planet->NextIndex = sort_array[i + 1];
	}
}

static void initSolarSysSISCharacteristics(void)
{
	uint8_t i {};

	uint8_t num_thrusters {0};
	for (i = 0; i < NumDriveSlots; ++i)
	{
		if (uqm::gameState().sis().DriveSlots[i] == ModuleType::FusionThruster)
		{
			++num_thrusters;
		}
	}
	pSolarSysState->max_ship_speed = (uint8_t)((num_thrusters + 5) * IpShipThrustIncrement);

	pSolarSysState->turn_wait = IpShipTurnWait;
	for (i = 0; i < NumJetSlots; ++i)
	{
		if (uqm::gameState().sis().JetSlots[i] == ModuleType::TurningJets)
		{
			pSolarSysState->turn_wait -= IpShipTurnDecrement;
		}
	}
}

// For Starseed, we will offset the star-coords seed by the custom seed, to
// 'pass along' the seed.  This will ensure each star system has a different
// seeding per custom seed, AND because fleets use their homeworld coords for
// jitter, it even ensures the same homeworld (coords) sourced fleets will have
// different jitters on different seeds.
uint32_t GetRandomSeedForStar(const StarDesc* star)
{
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "Get Random Seed For Star. %05.1f : %05.1f "
					   "[{}] (seed {}) = {}.  Plot ID {} ({}).\n",
			   (float)star->star_pt.x / 10, (float)star->star_pt.y / 10,
			   uqm::makeDword(star->star_pt.x, star->star_pt.y), uqm::currentSeed().value,
			   uqm::makeDword(star->star_pt.x, star->star_pt.y)
				   + (uqm::isStarSeed() ? uqm::currentSeed().value : 0),
			   star->Index, starPresenceString(star->Index));
#endif
	return uqm::makeDword(star->star_pt.x, star->star_pt.y) + (uqm::isStarSeed() ? uqm::currentSeed().value : 0);
}

uint32_t GetRandomSeedForVar(const GfxPoint point)
{
	return uqm::makeDword(point.x, point.y);
}

static void GenerateTexturedPlanets(void)
{
	uint16_t i {};
	SystemPlanet* pCurDesc {};
	SystemPlanet* previousOrbitalDesc {pSolarSysState->pOrbitalDesc};

	for (i = 0, pCurDesc = pSolarSysState->PlanetDesc.data();
		 i < pSolarSysState->SunDesc[0].NumPlanets; ++i, ++pCurDesc)
	{
		ResourceName maskanim = nullptr;

		DoPlanetaryAnalysis(&pSolarSysState->SysInfo, pCurDesc);

		// BW : precompute the generated texture to display it in IP
		pSolarSysState->pOrbitalDesc = pCurDesc;
		if (CurStarDescPtr()->Index == PlotId::Sol)
		{
			switch (i)
			{
				case 0: /* MERCURY */
					maskanim = IpMercuryMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 3;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod =
						59 * 240;
					break;
				case 1: /* VENUS */
					maskanim = IpVenusMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 177;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod =
						243 * 240;
					break;
				case 2: // EARTH
					maskanim = EarthMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 23;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 240;
					break;
				case 3: // MARS
					maskanim = IpMarsMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 24;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 246;
					break;
				case 4: /* JUPITER */
					maskanim = IpJupiterMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 3;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 98;
					break;
				case 5: /* SATURN */
					maskanim = IpSaturnMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 27;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 102;
					break;
				case 6: /* URANUS */
					maskanim = IpUranusMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 98;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 172;
					break;
				case 7: /* NEPTUNE */
					maskanim = IpNeptuneMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 30;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 182;
					break;
				case 8: /* PLUTO */
					maskanim = IpPlutoMaskAnimRes;
					pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 119;
					pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 1533;
					break;
			}
		}

		GeneratePlanetSurface(pCurDesc,
							  CaptureDrawable(LoadGraphic(maskanim)),
							  GeneratePerimeter(PlanetDiameter), PlanetDiameter);
		pCurDesc->orbit = pSolarSysState->Orbit;
		PrepareNextRotationFrameForIP(pCurDesc, 0);

		// Clean up some parasitic use of pSolarSysState
		pSolarSysState->XlatTable.reset();
		DestroyDrawable(ReleaseDrawable(pSolarSysState->TopoFrame));
		pSolarSysState->TopoFrame = 0;
		pSolarSysState->OrbitalCMap.reset();
		// End clean up
	}
	pSolarSysState->pOrbitalDesc = previousOrbitalDesc;
}

static constexpr auto NumTempRanges {5};

// Returns an orbital SystemPlanet when player is in orbit
static SystemPlanet* LoadSolarSys(void)
{
	uint16_t i {};
	SystemPlanet* orbital = nullptr;
	uqstl::array<Color, NumTempRanges> temp_color_array {
		BuildColor(MakeRgb15(0x00, 0x00, 0x0E), 0x54),
		BuildColor(MakeRgb15(0x00, 0x06, 0x08), 0x62),
		BuildColor(MakeRgb15(0x00, 0x0B, 0x00), 0x6D),
		BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2D),
		BuildColor(MakeRgb15(0x0F, 0x08, 0x00), 0x75),
	};

	if (NebulaFrame)
	{
		const uint8_t brightness = (uqm::UQMOptions::read().nebulaevol + 1) * 5;
		for (i = 0; i < NumTempRanges; i++)
		{
			IncreaseBrightness(&temp_color_array[i].r, brightness);
			IncreaseBrightness(&temp_color_array[i].g, brightness);
			IncreaseBrightness(&temp_color_array[i].b, brightness);
		}
	}

	SysGenRNG->reseed(GetRandomSeedForStar(CurStarDescPtr()));

	SunFrame = SetAbsFrameIndex(SunFrame,
								*StarType(CurStarDescPtr()->Type) * StarIndexMultiplier());

	SystemPlanet* pCurDesc {&pSolarSysState->SunDesc[0]};
	pCurDesc->pPrevDesc = 0;
	pCurDesc->rand_seed = SysGenRNG->next();
	pCurDesc->data_index = *StarType(CurStarDescPtr()->Type);
	pCurDesc->location.x = 0;
	pCurDesc->location.y = 0;
	pCurDesc->image.origin = pCurDesc->location;
	pCurDesc->image.frame = SunFrame;

	(*pSolarSysState->genFuncs->generatePlanets)(pSolarSysState);
	if (uqm::gameState().flag(uqm::GameFlag::PLANETARY_CHANGE))
	{
		PutPlanetInfo();
		uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_CHANGE, static_cast<uint32_t>(0));
	}

	for (i = 0, pCurDesc = pSolarSysState->PlanetDesc.data();
		 i < MaxPlanets; ++i, ++pCurDesc)
	{
		pCurDesc->pPrevDesc = &pSolarSysState->SunDesc[0];
		pCurDesc->image.origin = pCurDesc->location;
		if (i >= pSolarSysState->SunDesc[0].NumPlanets)
		{
			pCurDesc->image.frame = 0;
		}
		else
		{
			uint16_t index {};
			SolarSystemInfo SysInfo;

			DoPlanetaryAnalysis(&SysInfo, pCurDesc);
			index = (SysInfo.PlanetInfo.SurfaceTemperature + 250) / 100;
			if (index >= NumTempRanges)
			{
				index = NumTempRanges - 1;
			}

			pCurDesc->temp_color = temp_color_array[index];
		}
		pCurDesc->frame_offset = UndefinedOffset;
	}

	sortPlanetPositions();

	if (!uqm::flagship().ipPlanet)
	{ // Outer system
		pSolarSysState->pBaseDesc = pSolarSysState->PlanetDesc.data();
		pSolarSysState->pOrbitalDesc = nullptr;
	}
	else
	{ // Inner system
		pSolarSysState->SunDesc[0].location = uqm::flagship().ipLocation;
		uqm::flagship().ipLocation = displayToLocation(
			uqm::flagship().shipStamp.origin, MaxZoomRadius);

		i = uqm::flagship().ipPlanet - 1;
		pSolarSysState->pOrbitalDesc = &pSolarSysState->PlanetDesc[i];
		GenerateMoons(pSolarSysState, pSolarSysState->pOrbitalDesc);
		pSolarSysState->pBaseDesc = pSolarSysState->MoonDesc.data();

		uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_LANDING, static_cast<uint32_t>(0));
	}

	initSolarSysSISCharacteristics();

	if (uqm::flagship().inOrbit)
	{ // Only when loading a game into orbital
		i = uqm::flagship().inOrbit - 1;
		if (i == 0)
		{ // Orbiting the planet itself
			orbital = pSolarSysState->pBaseDesc->pPrevDesc;
		}
		else
		{ // Orbiting a moon
			// -1 because planet itself is 1, and moons have to be 1-based
			i -= 1;
			orbital = &pSolarSysState->MoonDesc[i];
		}
		uqm::flagship().ipLocation = pSolarSysState->SunDesc[0].location;
		uqm::flagship().inOrbit = 0;
	}
	else
	{
		i = uqm::flagship().shipFacing;
		// XXX: Solar system reentry test depends on ShipFacing != 0
		if (i == 0)
		{
			++i;
		}

		uqm::flagship().shipStamp.frame = SetAbsFrameIndex(SISIPFrame, i - 1);
	}

	return orbital;
}

static void saveNonOrbitalLocation(void)
{
	// XXX: Solar system reentry test depends on ShipFacing != 0
	uqm::flagship().shipFacing = GetFrameIndex(uqm::flagship().shipStamp.frame) + 1;
	uqm::flagship().inOrbit = 0;
	if (!playerInInnerSystem())
	{
		uqm::flagship().ipPlanet = 0;
	}
	else
	{
		// ip_planet is 1-based because code tests for ip_planet!=0
		uqm::flagship().ipPlanet = 1 + planetIndex(pSolarSysState, pSolarSysState->pOrbitalDesc);
		uqm::flagship().ipLocation = pSolarSysState->SunDesc[0].location;
	}
}

static void FreeSolarSys(void)
{
	uint16_t i {};
	SystemPlanet* pCurDesc {};

	if (pSolarSysState->InIpFlight)
	{
		pSolarSysState->InIpFlight = false;

		if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
		{
			saveNonOrbitalLocation();
			SetMusicPosition();
		}
	}

	DestroyDrawable(ReleaseDrawable(SolarSysFrame));
	SolarSysFrame = nullptr;

	StopMusic();

	if (uqm::UQMOptions::read().texturedPlanets)
	{
		// BW: clean up data generated for textured IP planets
		for (i = 0, pCurDesc = pSolarSysState->PlanetDesc.data();
			 i < pSolarSysState->SunDesc[0].NumPlanets; ++i, ++pCurDesc)
		{
			DestroyOrbitStruct(&pCurDesc->orbit, PlanetDiameter);
			// JMS: Not sure if these do any good...
			pSolarSysState->XlatTable.reset();
			DestroyDrawable(ReleaseDrawable(pSolarSysState->TopoFrame));
			pSolarSysState->TopoFrame = 0;
			pSolarSysState->OrbitalCMap.reset();
			// JMS ends.
		}

		// BW: if we were in Inner System, clean up data for textured IP
		// moons
		if (playerInInnerSystem())
		{
			uint16_t numMoons {};
			if (worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
			{
				numMoons =
					pSolarSysState->pOrbitalDesc->pPrevDesc->NumPlanets;
			}
			else
			{
				numMoons = pSolarSysState->pOrbitalDesc->NumPlanets;
			}

			for (i = 0, pCurDesc = pSolarSysState->MoonDesc.data();
				 i < numMoons; ++i, ++pCurDesc)
			{
				if (!(pCurDesc->data_index & *PlanetType::WorldTypeSpecial))
				{
					const int16_t diameterPick =
						pCurDesc->data_index > PlanetType::LastSmallRocky ?
							LargeMoonDiameter :
							SmallMoonDiameter;

					DestroyOrbitStruct(&pCurDesc->orbit, diameterPick);
					pCurDesc->frame_offset = UndefinedOffset;
				}
			}
		}
	}
	else
	{ // we need to destroy DOS style planets because they are cut from
		// the main frame and stored separately therefore they aren't
		// destroyed with OrbitFrame
		// also reset all planet offsets
		for (i = 0, pCurDesc = pSolarSysState->PlanetDesc.data();
			 i < pSolarSysState->SunDesc[0].NumPlanets; ++i, ++pCurDesc)
		{
			if (isPC(uqm::UQMOptions::read().planetStyle))
			{
				DestroyDrawable(ReleaseDrawable(pCurDesc->image.frame));
				pCurDesc->image.frame = 0;
			}
			pCurDesc->frame_offset = UndefinedOffset;
		}

		if (playerInInnerSystem())
		{
			uint16_t numMoons {};
			if (worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
			{
				numMoons =
					pSolarSysState->pOrbitalDesc->pPrevDesc->NumPlanets;
			}
			else
			{
				numMoons = pSolarSysState->pOrbitalDesc->NumPlanets;
			}

			for (i = 0, pCurDesc = pSolarSysState->MoonDesc.data();
				 i < numMoons; ++i, ++pCurDesc)
			{
				if (!(pCurDesc->data_index & *PlanetType::WorldTypeSpecial) && isPC(uqm::UQMOptions::read().planetStyle))
				{
					DestroyDrawable(ReleaseDrawable(
						pCurDesc->image.frame));
					pCurDesc->image.frame = 0;
				}
				pCurDesc->frame_offset = UndefinedOffset;
			}
		}
	}
	// FreeIPData ();
}

static FrameRef getCollisionFrame(SystemPlanet* planet, uint16_t WaitPlanet)
{
	if (pSolarSysState->WaitIntersect != (uint16_t)~0
		&& pSolarSysState->WaitIntersect != WaitPlanet)
	{
		return DecFrameIndex(stars_in_space);
	}
	else
	{ // Existing collisions are cleared only once the ship does not
		// intersect anymore with a full planet image
		return planet->image.frame;
	}
}

// Returns the planet with which the flagship is colliding
static SystemPlanet* CheckIntersect(void)
{
	uint16_t i {};
	IntersectInfo ShipIntersect;
	IntersectInfo PlanetIntersect;
	uint16_t NewWaitPlanet {};
	uint8_t PlanetOffset {};
	uint8_t MoonOffset {};

	// Check collisions with the system center object
	// This may be the planet in inner view, or the sun
	SystemPlanet* pCurDesc {pSolarSysState->pBaseDesc->pPrevDesc};
	PlanetOffset = pCurDesc - pSolarSysState->PlanetDesc.data() + 1;
	MoonOffset = 1; // the planet itself

	ShipIntersect.IntersectStamp.origin = uqm::flagship().shipStamp.origin;
	ShipIntersect.EndPoint = ShipIntersect.IntersectStamp.origin;
	ShipIntersect.IntersectStamp.frame = uqm::flagship().shipStamp.frame;

	PlanetIntersect.IntersectStamp.origin.x =
		ResScale(OrigSisScreenWidth() >> 1);
	PlanetIntersect.IntersectStamp.origin.y =
		ResScale(OrigSisScreenHeight() >> 1);
	PlanetIntersect.EndPoint = PlanetIntersect.IntersectStamp.origin;

	PlanetIntersect.IntersectStamp.frame = getCollisionFrame(pCurDesc,
															 uqm::makeWord(PlanetOffset, MoonOffset));

	/*log_add (log_Debug,"Nyt: x:{}, y:{}",
			PlanetIntersect.IntersectStamp.origin.x,
			PlanetIntersect.IntersectStamp.origin.y);*/

	// Start with no collisions
	NewWaitPlanet = 0;

	if (pCurDesc != pSolarSysState->SunDesc.data() /* can't intersect with sun */
		&& DrawablesIntersect(&ShipIntersect,
							  &PlanetIntersect, MaxTimeValue))
	{
#ifdef DEBUG_SOLARSYS
		uqm::log::debug("0: Planet {}, Moon {}", PlanetOffset,
						MoonOffset);
#endif /* DEBUG_SOLARSYS */
		NewWaitPlanet = uqm::makeWord(PlanetOffset, MoonOffset);
		if (pSolarSysState->WaitIntersect != (uint16_t)~0
			&& pSolarSysState->WaitIntersect != NewWaitPlanet)
		{
			pSolarSysState->WaitIntersect = NewWaitPlanet;
#ifdef DEBUG_SOLARSYS
			uqm::log::debug(
				"Star index = {}, Planet index = {}, <{}, {}>",
				CurStarDescPtr() - star_array,
				pCurDesc - pSolarSysState->PlanetDesc,
				pSolarSysState->SunDesc[0].location.x,
				pSolarSysState->SunDesc[0].location.y);
#endif /* DEBUG_SOLARSYS */
			return pCurDesc;
		}
	}

	for (i = pCurDesc->NumPlanets,
		pCurDesc = pSolarSysState->pBaseDesc;
		 i; --i, ++pCurDesc)
	{
		PlanetIntersect.IntersectStamp.origin = pCurDesc->image.origin;
		PlanetIntersect.EndPoint = PlanetIntersect.IntersectStamp.origin;
		if (playerInInnerSystem())
		{
			PlanetOffset = pCurDesc->pPrevDesc - pSolarSysState->PlanetDesc.data();
			MoonOffset = pCurDesc - pSolarSysState->MoonDesc.data() + 2;
		}
		else
		{
			PlanetOffset = pCurDesc - pSolarSysState->PlanetDesc.data();
			MoonOffset = 0;
		}
		++PlanetOffset;
		PlanetIntersect.IntersectStamp.frame = getCollisionFrame(pCurDesc,
																 uqm::makeWord(PlanetOffset, MoonOffset));

		/*log_add (log_Debug, "Ship x:{} y:{}. Planet x:{}, y:{}",
				ShipIntersect.IntersectStamp.origin.x,
				ShipIntersect.IntersectStamp.origin.y,
				PlanetIntersect.IntersectStamp.origin.x,
				PlanetIntersect.IntersectStamp.origin.y);*/

		if (DrawablesIntersect(&ShipIntersect,
							   &PlanetIntersect, MaxTimeValue))
		{
#ifdef DEBUG_SOLARSYS
			uqm::log::debug("1: Planet {}, Moon {}", PlanetOffset,
							MoonOffset);
#endif /* DEBUG_SOLARSYS */
			NewWaitPlanet = uqm::makeWord(PlanetOffset, MoonOffset);

			if (pSolarSysState->WaitIntersect == (uint16_t)~0)
			{ // All collisions disallowed, but the ship is still
				// colliding with something. Collisions will remain
				// disabled.
				break;
			}
			else if (pSolarSysState->WaitIntersect == NewWaitPlanet)
			{ // Existing and continued collision -- ignore
				continue;
			}

			// Collision with a new planet/moon. This may cause a
			// transition to an inner system or start an orbital view.
			pSolarSysState->WaitIntersect = NewWaitPlanet;
			return pCurDesc;
		}
	}

	// This records the planet/moon with which the ship just collided
	// It may be a previously existing collision also (the value won't
	// change) If all collisions were disabled, this will reenable then
	// once the ship stops colliding with any planets
	if (pSolarSysState->WaitIntersect != (uint16_t)~0 || NewWaitPlanet == 0)
	{
		pSolarSysState->WaitIntersect = NewWaitPlanet;
	}

	return nullptr;
}

static void GetOrbitRect(GfxRect* pRect, int16_t dx, int16_t dy, int16_t radius,
						 int xnumer, int ynumer, int denom)
{
	pRect->corner.x = ResScale(OrigSisScreenWidth() >> 1)
					+ (long)-dx * xnumer / denom;
	pRect->corner.y = ResScale(OrigSisScreenHeight() >> 1)
					+ (long)-dy * ynumer / denom;
	pRect->extent.width = (long)radius * (xnumer << 1) / denom;
	pRect->extent.height = pRect->extent.width >> 1;
}

static void GetPlanetOrbitRect(GfxRect* r, SystemPlanet* planet, int sizeNumer,
							   int dyNumer, int denom)
{
	int16_t dx {planet->radius};
	int16_t dy {planet->radius};
	if (sizeNumer > DisplayFactor())
	{
		dx = dx + planet->location.x;
		dy = (dy + planet->location.y) << 1;
	}
	GetOrbitRect(r, dx, dy, planet->radius, sizeNumer, dyNumer, denom);
}

static void SetPlanetOldFrame(SystemPlanet* planet, uint16_t index, uint16_t color)
{
	GfxRect r;

	FrameRef oldFrame {SetAbsFrameIndex(OrbitalFrame, index)};
	GetFrameRect(oldFrame, &r);

	r.corner.y = 0; // messed because of sprite hotspot
	r.corner.x = (r.extent.width / *StarBodyColor::NumPlanetColors) * color;
	r.extent.width /= *StarBodyColor::NumPlanetColors;

	// If previous frame wasn't SpaceJunkFrame
	if (planet->image.frame && planet->size != 0)
	{
		DestroyDrawable(ReleaseDrawable(planet->image.frame));
		planet->image.frame = 0;
	}

	planet->image.frame = CaptureDrawable(CopyFrameRect(oldFrame, &r));
	SetFrameHot(planet->image.frame, GetFrameHot(oldFrame));
}

static void ValidateOrbit(SystemPlanet* planet, int sizeNumer, int dyNumer, int denom)
{
	uint16_t index {};

	if (uqm::UQMOptions::read().orbitingPlanets)
	{
		// BW: recompute planet position to account for orbiting
		// uint16_t newAngle;
		// newAngle = NormalizeAngle(planet->angle
		// 		+ (uint16_t)(daysElapsed() * planet->orb_speed));
		// planet->location.x = Cosine (newAngle, planet->radius);
		// planet->location.y = Sine (newAngle, planet->radius);
		double newAngle {NAN};
		newAngle = (planet->angle + daysElapsed() * planet->orb_speed)
					 * M_PI / 32
				 - M_PI / 2;
		planet->location.x = (int16_t)(cos(newAngle) * planet->radius);
		planet->location.y = (int16_t)(sin(newAngle) * planet->radius);
	}

	if (sizeNumer <= DisplayFactor())
	{ // All planets in outer view, and moons in inner
		GfxRect r;

		GetPlanetOrbitRect(&r, planet, sizeNumer, dyNumer, denom);

		// Calculate the location of the planet's image
		r.corner.x += (r.extent.width >> 1);
		r.corner.y += (r.extent.height >> 1);
		r.corner.x += (long)planet->location.x * sizeNumer / denom;
		// Ellipse function always has coefficients a^2 = 2 * b^2
		r.corner.y += (long)planet->location.y * (sizeNumer / 2) / denom;

		planet->image.origin = r.corner;
	}

	// Calculate the size and lighting angle of planet's image and
	// set the image that will be drawn
	index = planet->data_index & ~*PlanetType::WorldTypeSpecial;
	if (index < PlanetType::NumberOfPlanetTypes)
	{ // The world is a normal planetary body (planet or moon)
		uint8_t Type {};
		uint16_t angle {};
		uint8_t offset = 0;

		Type = PlanData[index].Type;
		uint16_t Size {*Plansize(Type)};
		if (sizeNumer > DisplayFactor())
		{
			Size += 3;
		}
		else if (worldIsMoon(pSolarSysState, planet))
		{
			Size += 2;
		}
		else if (denom <= (MaxZoomRadius >> 2))
		{
			++Size;
			if (denom == MinZoomRadius)
			{
				++Size;
			}
		}

		if (worldIsPlanet(pSolarSysState, planet))
		{ // Planet
			angle = ARCTAN(planet->location.x, planet->location.y);
		}
		else
		{ // Moon
			angle = ARCTAN(planet->pPrevDesc->location.x,
						   planet->pPrevDesc->location.y);
		}

		offset = (Size << FacingShift) + NormalizeFacing(AngleToFacing(angle));

		if (planet->frame_offset != offset)
		{
			planet->frame_offset = offset;

			if (!uqm::UQMOptions::read().texturedPlanets && isPC(uqm::UQMOptions::read().planetStyle))
			{
				SetPlanetOldFrame(planet, offset, *Plancolor(Type));
			}
			else
			{
				planet->image.frame = SetAbsFrameIndex(OrbitalFrame,
													   offset);
			}

			// Normal planets have sizes start from 1st array Element
			planet->size = Size + 1;
		}
	}
	else if (planet->data_index > PlanetType::WorldTypeSpecial && planet->frame_offset == UndefinedOffset)
	{
		uint8_t frameNum = 0;

		// these have no animations - so set the frame once
		// it'll reset on leave
		planet->frame_offset = 0;
		planet->size = 0;

		switch (static_cast<PlanetType>(planet->data_index))
		{
			case PlanetType::PrecursorStarbase:
				frameNum = 23;
				break;
			case PlanetType::DestroyedStarbase:
				frameNum = 22;
				break;
			case PlanetType::SaMatra:
				frameNum = 19;
				break;
			case PlanetType::HierarchyStarbase:
			default:
				frameNum = 16;
				break;
		}
		planet->image.frame = SetAbsFrameIndex(SpaceJunkFrame, frameNum);
	}
}

static void DrawOrbit(SystemPlanet* planet, int sizeNumer, int dyNumer, int denom)
{
	GfxRect r;
	LargeRect dr;

	GetPlanetOrbitRect(&r, planet, sizeNumer, dyNumer, denom);

	dr = RECT_TO_DRECT(r);
	SetContextForeGroundColor(planet->temp_color);
	if (!uqm::UQMOptions::read().unscaledStarSystem)
	{
		DrawOval(&dr, chooseIfHd(1, 6), false);
	}
	else
	{
		DrawOval(&dr, 1, false);
	}
}

static int16_t FindRadius(GfxPoint shipLoc, int16_t fromRadius)
{
	int16_t nextRadius {};
	GfxPoint displayLoc;

	do
	{
		fromRadius >>= 1;
		if (fromRadius > MinZoomRadius)
		{
			nextRadius = fromRadius >> 1;
		}
		else
		{
			nextRadius = 0; // scaleRect will be nul
		}

		GetOrbitRect(&scaleRect, nextRadius, nextRadius, nextRadius,
					 DisplayFactor(), ResScale(DisplayFactorUs() >> 2),
					 fromRadius);
		displayLoc = locationToDisplay(shipLoc, fromRadius);

	} while (pointWithinRect(scaleRect, displayLoc));

	return fromRadius;
}

static StatusFlags flagship_inertial_thrust(uint16_t CurrentAngle)
{
	int16_t max_ship_speed {};
	int16_t cur_delta_x {};
	int16_t cur_delta_y {};

	max_ship_speed = pSolarSysState->max_ship_speed << 1;
	const uint16_t thrust_increment {IpShipThrustIncrement << 1};
	VelocityDesc* VelocityPtr {&uqm::flagship().velocity};
	GetCurrentVelocityComponents(VelocityPtr, &cur_delta_x, &cur_delta_y);
	const uint16_t TravelAngle {GetVelocityTravelAngle(VelocityPtr)};
	if (TravelAngle == CurrentAngle
		&& cur_delta_x == Cosine(CurrentAngle, max_ship_speed)
		&& cur_delta_y == Sine(CurrentAngle, max_ship_speed))
	{ // already maxed-out acceleration
		return (StatusFlags::ShipAtMaxSpeed);
	}
	else
	{
		int16_t delta_x {};
		int16_t delta_y {};
		uint32_t max_speed {};

		delta_x = cur_delta_x + Cosine(CurrentAngle, thrust_increment);
		delta_y = cur_delta_y + Sine(CurrentAngle, thrust_increment);
		uint32_t desired_speed {VelocitySquared(delta_x, delta_y)};
		max_speed = pow(max_ship_speed, 2);

		if (desired_speed <= max_speed)
		{ // normal acceleration
			SetVelocityComponents(VelocityPtr, delta_x, delta_y);
		}
		else if (TravelAngle == CurrentAngle)
		{ // normal max acceleration, same vector
			SetVelocityComponents(VelocityPtr,
								  Cosine(CurrentAngle, max_ship_speed),
								  Sine(CurrentAngle, max_ship_speed));
			return (StatusFlags::ShipAtMaxSpeed);
		}
		else
		{ // maxed-out acceleration at an angle to current travel vector
			// thrusting at an angle while at max velocity only changes
			// the travel vector, but does not really change the velocity

			VelocityDesc v = *VelocityPtr;

			DeltaVelocityComponents(&v,
									Cosine(CurrentAngle, thrust_increment >> 1)
										- Cosine(TravelAngle, thrust_increment),
									Sine(CurrentAngle, thrust_increment >> 1)
										- Sine(TravelAngle, thrust_increment));
			GetCurrentVelocityComponents(&v, &cur_delta_x, &cur_delta_y);
			desired_speed = VelocitySquared(cur_delta_x, cur_delta_y);

			if (desired_speed > max_speed)
			{
				SetVelocityComponents(VelocityPtr,
									  Cosine(CurrentAngle, max_ship_speed),
									  Sine(CurrentAngle, max_ship_speed));
				return (StatusFlags::ShipAtMaxSpeed);
			}

			*VelocityPtr = v;
		}

		return StatusFlags::None;
	}
}

static constexpr auto North {0};
static constexpr auto East {4};
static constexpr auto South {8};
static constexpr auto West {12};

static int16_t ScreenCompass(uint16_t index)
{
	int16_t facing {};
	const GfxPoint scrLoc = uqm::flagship().shipStamp.origin;
	const GfxExtent sisScr = {(int16_t)SisScreenWidth(), (int16_t)SisScreenHeight()};
	const bool westOfCenter = scrLoc.x < (sisScr.width >> 1);
	const bool northOfCenter = scrLoc.y < (sisScr.height >> 1);
	const int quadrant = (westOfCenter ? 0 : 1) | (northOfCenter ? 0 : 2);

	switch (quadrant)
	{
		case 0: // NorthWest Quadrant
			facing = (scrLoc.x < scrLoc.y) ? West : North;
			break;
		case 1: // NorthEast Quadrant
			facing = ((sisScr.width - scrLoc.x) < scrLoc.y) ? East : North;
			break;
		case 2: // SouthWest Quadrant
			facing = (scrLoc.x < (sisScr.height - scrLoc.y)) ? West : South;
			break;
		case 3: // SouthEast Quadrant
			facing = ((sisScr.width - scrLoc.x)
					  < (sisScr.height - scrLoc.y)) ?
						 East :
						 South;
			break;
		default:
			return index;
	}

	return facing;
}

static constexpr auto ShipThrust {-1};
static constexpr auto TurnLeft {1};
static constexpr auto TurnRight {-1};

static GfxPoint TurnAbout(int16_t delta_x, int16_t delta_y)
{
	uint16_t index {};
	GfxPoint delta = {delta_x, delta_y};

	if (!uqm::UQMOptions::read().smartAutoPilot)
	{
		delta.y = ShipThrust;
		return delta;
	}

	index = GetFrameIndex(uqm::flagship().shipStamp.frame);

	int16_t facing {ScreenCompass(index)};

	if ((int)facing != index)
	{
		const int normalizedFacing = NormalizeFacing(index - facing);
		const int AngleTo = AngleToFacing(HalfCircle);

		if (normalizedFacing >= AngleTo)
		{
			facing = NormalizeFacing(facing + TurnRight);
			delta.x++;
		}
		else if ((int)index != (int)facing)
		{
			facing = NormalizeFacing(facing + TurnLeft);
			delta.x--;
		}
	}
	else
	{
		delta.y = ShipThrust;
	}

	return delta;
}

static constexpr auto ThrustWait {1};

static void ProcessShipControls(void)
{
	uint16_t index {};
	int16_t delta_y {};

	if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Up]
		|| CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Thrust])
	{
		delta_y = -1;
	}
	else
	{
		delta_y = 0;
	}

	int16_t delta_x {0};

	if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Left])
	{
		delta_x -= 1;
	}
	if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][*BattleKey::Right])
	{
		delta_x += 1;
	}

	if (delta_x || delta_y < 0)
	{
		uqm::flagship().autopilot.clear();

		if (ValidPoint(LoadAdvancedAutoPilot())
			|| ValidPoint(LoadAdvancedQuasiPilot()))
		{
			ZeroAdvancedAutoPilot();
			ZeroAdvancedQuasiPilot();
		}
	}
	else if (uqm::flagship().autopilot.hasTarget())
	{
		GfxPoint delta;
		delta = TurnAbout(delta_x, delta_y);

		delta_x = delta.x;
		delta_y = delta.y;
	}
	else
	{
		delta_y = 0;
	}

	index = GetFrameIndex(uqm::flagship().shipStamp.frame);
	if (pSolarSysState->turn_counter)
	{
		--pSolarSysState->turn_counter;
	}
	else if (delta_x)
	{
		if (delta_x < 0)
		{
			index = NormalizeFacing(index - 1);
		}
		else
		{
			index = NormalizeFacing(index + 1);
		}

		uqm::flagship().shipStamp.frame =
			SetAbsFrameIndex(uqm::flagship().shipStamp.frame, index);

		pSolarSysState->turn_counter = pSolarSysState->turn_wait;
	}
	if (pSolarSysState->thrust_counter)
	{
		--pSolarSysState->thrust_counter;
	}
	else if (delta_y < 0)
	{
		flagship_inertial_thrust(FacingToAngle(index));

		pSolarSysState->thrust_counter = ThrustWait;
	}
}

static void enterInnerSystem(SystemPlanet* planet)
{
	const auto InnerEntryDistance = [&] {
		return (MinMoonRadius() + ((MaxGenMoons - 1) * MoonDelta()) + (MoonDelta() >> 2)) + NsafeNumScl(5);
	};
	uint16_t angle {};

	// Calculate the inner system entry location and facing
	angle = FacingToAngle(GetFrameIndex(uqm::flagship().shipStamp.frame))
		  + HalfCircle;

	uqm::flagship().shipStamp.origin.x = ResScale(OrigSisScreenWidth() >> 1)
									   + Cosine(angle, InnerEntryDistance());
	uqm::flagship().shipStamp.origin.y = ResScale(OrigSisScreenHeight() >> 1)
									   + Sine(angle, InnerEntryDistance());

	if (uqm::flagship().shipStamp.origin.y < 0)
	{
		uqm::flagship().shipStamp.origin.y = ResScale(1);
	}
	else if (uqm::flagship().shipStamp.origin.y >= SisScreenHeight())
	{
		uqm::flagship().shipStamp.origin.y =
			(SisScreenHeight() - ResScale(1)) - ResScale(1);
	}

	uqm::flagship().ipLocation = displayToLocation(
		uqm::flagship().shipStamp.origin, MaxZoomRadius);

	pSolarSysState->SunDesc[0].location =
		planetOuterLocation(planetIndex(pSolarSysState, planet));
	ZeroVelocityComponents(&uqm::flagship().velocity);

	GenerateMoons(pSolarSysState, planet);
	if (uqm::UQMOptions::read().texturedPlanets)
	{
		GenerateTexturedMoons(pSolarSysState, planet);
	}

	pSolarSysState->pBaseDesc = pSolarSysState->MoonDesc.data();
	pSolarSysState->pOrbitalDesc = planet;
}

static void leaveInnerSystem(SystemPlanet* planet)
{
	uint16_t outerPlanetWait {};
	uint16_t i {};
	SystemPlanet* pMoonDesc {};

	pSolarSysState->pBaseDesc = pSolarSysState->PlanetDesc.data();
	pSolarSysState->pOrbitalDesc = nullptr;

	outerPlanetWait = uqm::makeWord(
		planet - pSolarSysState->PlanetDesc.data() + 1, 0);
	// BW: planet may have moved while we were into Inner System
	ValidateOrbit(planet, DisplayFactor(),
				  ResScale(DisplayFactorUs() / 4),
				  pSolarSysState->SunDesc[0].radius);
	pSolarSysState->SunDesc[0].location =
		planetOuterLocation(planetIndex(pSolarSysState, planet));
	uqm::flagship().ipLocation = pSolarSysState->SunDesc[0].location;
	XFormIPLoc(&uqm::flagship().ipLocation, &uqm::flagship().shipStamp.origin, true);
	ZeroVelocityComponents(&uqm::flagship().velocity);

	// Now the ship is in outer system (as per game logic)

	if (uqm::UQMOptions::read().texturedPlanets)
	{ // BW: clean up data generated for textured IP moons
		for (i = 0, pMoonDesc = pSolarSysState->MoonDesc.data();
			 i < planet->NumPlanets; ++i, ++pMoonDesc)
		{
			if (!(pMoonDesc->data_index & *PlanetType::WorldTypeSpecial))
			{
				const int16_t diameterPick =
					pMoonDesc->data_index > PlanetType::LastSmallRocky ?
						LargeMoonDiameter :
						SmallMoonDiameter;

				DestroyOrbitStruct(&pMoonDesc->orbit, diameterPick);
			}
			pMoonDesc->frame_offset = UndefinedOffset;
		}
	} // End clean up
	else if (isPC(uqm::UQMOptions::read().planetStyle))
	{
		for (i = 0, pMoonDesc = pSolarSysState->MoonDesc.data();
			 i < planet->NumPlanets; ++i, ++pMoonDesc)
		{
			if (!(pMoonDesc->data_index & *PlanetType::WorldTypeSpecial))
			{
				DestroyDrawable(ReleaseDrawable(pMoonDesc->image.frame));
				pMoonDesc->image.frame = 0;
			}
			pMoonDesc->frame_offset = UndefinedOffset;
		}
	}

	pSolarSysState->WaitIntersect = outerPlanetWait;
	// See if we also intersect with another planet, and if we do,
	// disable collisions comletely until we stop intersecting
	// with any planet at all.
	CheckIntersect();
	if (pSolarSysState->WaitIntersect != outerPlanetWait)
	{
		pSolarSysState->WaitIntersect = (uint16_t)~0;
	}
}

static void enterOrbital(SystemPlanet* planet)
{
	ZeroVelocityComponents(&uqm::flagship().velocity);
	pSolarSysState->pOrbitalDesc = planet;
	pSolarSysState->InOrbit = true;
}

static bool CheckShipLocation(int16_t* newRadius)
{
	int16_t radius {};
	bool SISonScreen {};

	radius = pSolarSysState->SunDesc[0].radius;
	*newRadius = pSolarSysState->SunDesc[0].radius;

	SISonScreen = (uqm::flagship().shipStamp.origin.x < 0
				   || uqm::flagship().shipStamp.origin.x >= SisScreenWidth()
				   || uqm::flagship().shipStamp.origin.y < 0
				   || uqm::flagship().shipStamp.origin.y >= SisScreenHeight());

	if (SISonScreen)
	{
		// The ship leaves the screen.
		if (!playerInInnerSystem())
		{ // Outer zoom-out transition
			if (radius == MaxZoomRadius)
			{
				// The ship leaves IP.
				uqm::currentActivity() |= EndInterplanetary;
				return false; // no location change
			}

			*newRadius = FindRadius(uqm::flagship().ipLocation,
									MaxZoomRadius << 1);
		}
		else
		{
			leaveInnerSystem(pSolarSysState->pOrbitalDesc);

			if (pointWithinRect(scaleRect, uqm::flagship().shipStamp.origin))
			{
				*newRadius = FindRadius(uqm::flagship().ipLocation, radius);
				pSolarSysState->SunDesc[0].radius = *newRadius;
			}
			else if (SISonScreen)
			{
				*newRadius = FindRadius(uqm::flagship().ipLocation,
										MaxZoomRadius << 1);
				pSolarSysState->SunDesc[0].radius = *newRadius;
			}
		}

		return true;
	}

	if (!playerInInnerSystem()
		&& pointWithinRect(scaleRect, uqm::flagship().shipStamp.origin))
	{ // Outer zoom-in transition
		*newRadius = FindRadius(uqm::flagship().ipLocation, radius);
		return true;
	}

	if (uqm::flagship().autopilot.target().x == ~0 && uqm::flagship().autopilot.target().y == ~0)
	{ // Not on autopilot -- may collide with a planet
		SystemPlanet* planet = CheckIntersect();
		if (planet)
		{ // Collision with a planet
			if (playerInInnerSystem())
			{ // Entering planet orbit (scans, etc.)
				enterOrbital(planet);
				return false; // no location change
			}
			else
			{ // Transition to inner system
				enterInnerSystem(planet);
				return true;
			}
		}
	}

	return false; // no location change
}

static void DrawSystemTransition(bool inner)
{
	SetTransitionSource(nullptr);
	BatchGraphics();
	if (inner)
	{
		DrawInnerSystem();
	}
	else
	{
		DrawOuterSystem();
	}
	RedrawQueue(false);
	ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);
	UnbatchGraphics();
}

static void TransitionSystemIn(void)
{
	SetContext(SpaceContext);
	DrawSystemTransition(playerInInnerSystem());
}

static constexpr auto NumSteps {10};

static void ScaleSystem(int16_t new_radius)
{
#ifdef SMOOTH_SYSTEM_ZOOM
	// XXX: This appears to have been an attempt to zoom the system view
	//   in a different way. This code zooms gradually instead of
	//   doing a crossfade from one zoom level to the other.
	// TODO: Do not loop here, and instead increment the zoom level
	//   in IP_frame() with a function drawing the new zoom. The ship
	//   controls are not handled in the loop, and the flagship
	//   can collide with a group while zooming, and that is not handled
	//   100% correctly.
	uint16_t i;
	int16_t old_radius;
	int16_t d, step;

	old_radius = pSolarSysState->SunDesc[0].radius;

	assert(old_radius != 0);
	assert(old_radius != new_radius);

	d = new_radius - old_radius;
	step = d / NumSteps;

	for (i = 0; i < NumSteps - 1; ++i)
	{
		pSolarSysState->SunDesc[0].radius += step;
		XFormIPLoc(&uqm::flagship().ipLocation, &uqm::flagship().shipStamp.origin,
				   true);

		BatchGraphics();
		DrawOuterSystem();
		RedrawQueue(false);
		UnbatchGraphics();

		SleepThread(GameTicksPerSecond / 30);
	}

	// Final zoom step
	pSolarSysState->SunDesc[0].radius = new_radius;
	XFormIPLoc(&uqm::flagship().ipLocation, &uqm::flagship().shipStamp.origin, true);

	BatchGraphics();
	DrawOuterSystem();
	RedrawQueue(false);
	UnbatchGraphics();

#else  // !SMOOTH_SYSTEM_ZOOM
	GfxRect r;

	pSolarSysState->SunDesc[0].radius = new_radius;
	XFormIPLoc(&uqm::flagship().ipLocation, &uqm::flagship().shipStamp.origin, true);

	GetContextClipRect(&r);
	SetTransitionSource(&r);
	BatchGraphics();
	DrawOuterSystem();
	RedrawQueue(false);
	ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
	UnbatchGraphics();
#endif // SMOOTH_SYSTEM_ZOOM
}

static void RestoreSystemView(void)
{
	GfxStamp s;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SolarSysFrame;
	DrawStamp(&s);
}

static void DrawTexturedBody(SystemPlanet* planet, GfxStamp s)
{
	int oldScale {};
	int16_t moonDiameter {};
	const uint16_t size = PBodySize[planet->size];

	BatchGraphics();
	const tfb::ScaleMode oldMode = SetGraphicScaleMode(tfb::ScaleMode::Bilinear);

	if (worldIsMoon(pSolarSysState, planet))
	{
		moonDiameter = planet->data_index > PlanetType::LastSmallRocky ?
						   LargeMoonDiameter :
						   SmallMoonDiameter;

		oldScale = SetGraphicScale(
			GscaleIdentity * ResScale(size) / moonDiameter);
	}
	else
	{
		oldScale = SetGraphicScale(
			GscaleIdentity * ResScale(size)
			/ PlanetDiameter);
	}

	s.frame = planet->orbit.SphereFrame;
	DrawStamp(&s);

	if (planet->orbit.ObjectFrame)
	{
		s.frame = planet->orbit.ObjectFrame;
		DrawStamp(&s);
	}

	SetGraphicScale(oldScale);
	SetGraphicScaleMode(oldMode);

	UnbatchGraphics();
}

void RotatePlanets(bool IsInnerSystem)
{
	SystemPlanet* planet {};
	SystemPlanet* moon {};
	static int16_t frameCounter;
	uint16_t i {};

	// Do not try to rotate planets that haven't been generated yet.
	if (!pSolarSysState->PlanetDesc[0].orbit.lpTopoData)
	{
		return;
	}

	++frameCounter;

	if (IsInnerSystem)
	{
		planet = pSolarSysState->pOrbitalDesc;
		PrepareNextRotationFrameForIP(planet, frameCounter);
		for (i = 0; i < planet->NumPlanets; ++i)
		{
			moon = &pSolarSysState->MoonDesc[i];
			if (!(moon->data_index & *PlanetType::WorldTypeSpecial))
			{
				PrepareNextRotationFrameForIP(moon, frameCounter);
			}
		}
	}
	else
	{
		for (i = pSolarSysState->SunDesc[0].NumPlanets,
			planet = &pSolarSysState->PlanetDesc[0];
			 i; --i, ++planet)
		{
			PrepareNextRotationFrameForIP(planet, frameCounter);
		}
	}
}

// Normally called by DoIpFlight() to process a frame
static void IP_frame(void)
{
	int16_t newRadius {};

	SetContext(SpaceContext);
	ProcessShipControls();

	const bool locChange {CheckShipLocation(&newRadius)};
	if (locChange)
	{
		if (playerInInnerSystem())
		{ // Entering inner system
			DrawSystemTransition(true);
		}
		else if (pSolarSysState->SunDesc[0].radius == newRadius)
		{ // Leaving inner system to outer
			DrawSystemTransition(false);
		}
		else
		{ // Zooming outer system
			ScaleSystem(newRadius);
		}
	}
	else if (!pSolarSysState->InOrbit)
	{ // Just flying around, minding own business..
		BatchGraphics();
		RestoreSystemView();
		if (AnimatedSun() || uqm::UQMOptions::read().orbitingPlanets || uqm::UQMOptions::read().texturedPlanets)
		{
			// BW: recompute planet position to account for orbiting
			if (playerInInnerSystem())
			{
				// Draw the inner system view
				ValidateInnerOrbits();
				DrawInnerPlanets(pSolarSysState->pOrbitalDesc);
			}
			else
			{
				// Draw the outer system view
				ValidateOrbits();
				DrawOuterPlanets(pSolarSysState->SunDesc[0].radius);
			}
		}
		RedrawQueue(false);
		DrawAutoPilotMessage(false);
		UnbatchGraphics();
	}
}

static bool CheckZoomLevel(void)
{
	GfxPoint shipLoc;

	const bool InnerSystem {playerInInnerSystem()};
	if (InnerSystem)
	{
		shipLoc = pSolarSysState->SunDesc[0].location;
	}
	else
	{
		shipLoc = uqm::flagship().ipLocation;
	}

	pSolarSysState->SunDesc[0].radius = FindRadius(shipLoc,
												   MaxZoomRadius << 1);
	if (!InnerSystem)
	{ // Update ship stamp since the radius probably changed
		XFormIPLoc(&shipLoc, &uqm::flagship().shipStamp.origin, true);
	}

	return InnerSystem;
}

static void ValidateOrbits(void)
{
	uint16_t i {};
	SystemPlanet* planet {};

	for (i = pSolarSysState->SunDesc[0].NumPlanets,
		planet = &pSolarSysState->PlanetDesc[0];
		 i; --i, ++planet)
	{
		ValidateOrbit(planet, DisplayFactor(),
					  ResScale(DisplayFactorUs() / 4),
					  pSolarSysState->SunDesc[0].radius);
	}
}

static void ValidateInnerOrbits(void)
{
	assert(playerInInnerSystem());

	SystemPlanet* planet {pSolarSysState->pOrbitalDesc};
	ValidateOrbit(planet, ResScale(DisplayFactorUs() * 4),
				  DisplayFactor(), planet->radius);

	for (uint16_t i {0}; i < planet->NumPlanets; ++i)
	{
		SystemPlanet* moon = &pSolarSysState->MoonDesc[i];
		ValidateOrbit(moon, 2, 1, 2);
	}
}

static void DrawInnerSystem(void)
{
	ValidateInnerOrbits();
	DrawSystem(pSolarSysState->pOrbitalDesc->radius, true);
	if (AnimatedSun() || uqm::UQMOptions::read().orbitingPlanets || uqm::UQMOptions::read().texturedPlanets)
	{
		DrawInnerPlanets(pSolarSysState->pOrbitalDesc);
	}
	DrawSISTitle(uqm::gameState().sis().PlanetName);
}

static void DrawOuterSystem(void)
{
	ValidateOrbits();
	DrawSystem(pSolarSysState->SunDesc[0].radius, false);
	if (AnimatedSun() || uqm::UQMOptions::read().orbitingPlanets || uqm::UQMOptions::read().texturedPlanets)
	{
		DrawOuterPlanets(pSolarSysState->SunDesc[0].radius);
	}
	DrawHyperCoords(CurStarDescPtr()->star_pt);
}

ResourceName spaceMusicSwitch(uint8_t SpeciesID)
{
	switch (static_cast<SpeciesId>(SpeciesID))
	{
		case SpeciesId::Arilou:
			return ArilouSpaceMusicRes;
		case SpeciesId::Chmmr:
			return ChmmrSpaceMusicRes;
		case SpeciesId::Orz:
			return OrzSpaceMusicRes;
		case SpeciesId::Pkunk:
			return PkunkSpaceMusicRes;
		case SpeciesId::Spathi:
			return SpathiSpaceMusicRes;
		case SpeciesId::Supox:
			return SupoxSpaceMusicRes;
		case SpeciesId::Thraddash:
			return ThraddashSpaceMusicRes;
		case SpeciesId::Utwig:
			return UtwigSpaceMusicRes;
		case SpeciesId::Vux:
			return VuxSpaceMusicRes;
		case SpeciesId::Yehat:
			return YehatSpaceMusicRes;
		case SpeciesId::Druuge:
			return DruugeSpaceMusicRes;
		case SpeciesId::Ilwrath:
			return IlwrathSpaceMusicRes;
		case SpeciesId::Mycon:
			return MyconSpaceMusicRes;
		case SpeciesId::Umgah:
			return UmgahSpaceMusicRes;
		case SpeciesId::UrQuan:
		case SpeciesId::KohrAh:
			return UrquanSpaceMusicRes;
		case SpeciesId::Zoqfotpik:
			return ZoqfotpikSpaceMusicRes;
		case SpeciesId::Syreen:
			return SyreenSpaceMusicRes;
		case SpeciesId::SaMatra:
			return KohrahSpaceMusicRes;
		default:
			return IpMusicRes;
	}
}

static void playSpaceMusic(void)
{
	if (!SpaceMusic)
	{
		if (SpaceMusicOK)
		{
			findRaceSOI();
			SpaceMusic = LoadMusicInstance(spaceMusicSwitch(spaceMusicBySOI));
		}
		else
		{
			SpaceMusic = LoadMusicInstance(IpMusicRes);
		}
	}

	// Do not start playing the music if we entered the solarsys only
	// to load a game (load invoked from Main menu)
	// XXX: This is quite hacky
	if (!PLRPlaying((MusicRef)~0) && (LastActivity != ActivityFlag::CheckLoad || NextActivity))
	{
		PlayMusicResume(SpaceMusic, NormalVolume);
	}
}

void ResetSolarSys(void)
{
	// Originally there was a flash_task test here, however, I found no
	// cases where flash_task could be set at the time of call. The test
	// was probably needed on 3DO when IP_frame() was a task.
	assert(!pSolarSysState->InIpFlight);

	DrawMenuStateStrings(MenuString::Starmap, -(MenuString::Navigate - MenuString::Scan));

	InitDisplayList();
	// This also spawns the flagship Element
	DoMissions();

	// Figure out and note which planet/moon we just left, if any
	// This records any existing collision and prevents the ship
	// from entering planets until a new collision occurs.
	// TODO: this may need logic similar to one in leaveInnerSystem()
	//   for when the ship collides with more than one planet at
	//   the same time. While quite rare, it's still possible.
	CheckIntersect();

	pSolarSysState->InIpFlight = true;

	playSpaceMusic();
}

static void EnterPlanetOrbit(void)
{
	if (pSolarSysState->InIpFlight)
	{ // This means we hit a planet in IP flight; not a Load into orbit
		FreeSolarSys();
	}

	if (worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
	{ // Moon -- use its origin
		// XXX: The conversion functions do not error-correct, so the
		//   point we set here will change once flag_ship_preprocess()
		//   in ipdisp.c starts over again.
		uqm::flagship().shipStamp.origin =
			pSolarSysState->pOrbitalDesc->image.origin;

		// JMS_GFX: Draw the moon letter when orbiting a moon
		if (!(GetNamedPlanetaryBody()) && isPC(uqm::UQMOptions::read().whichFonts)
			&& !(pSolarSysState->pOrbitalDesc->data_index
				 & *PlanetType::WorldTypeSpecial))
		{
			fmt::format_to_sz_n(uqm::gameState().sis().PlanetName
									+ strlen(uqm::gameState().sis().PlanetName),
								4, "-{}{}",
								moonLetter(pSolarSysState, pSolarSysState->pOrbitalDesc),
								'\0');
		}
	}
	else
	{ // Planet -- its origin is for the outer view, so use mid-screen
		uqm::flagship().shipStamp.origin.x =
			ResScale(OrigSisScreenWidth() >> 1);
		uqm::flagship().shipStamp.origin.y =
			ResScale(OrigSisScreenHeight() >> 1);
	}

	GetPlanetInfo();
	(*pSolarSysState->genFuncs->generateOrbital)(pSolarSysState,
												 pSolarSysState->pOrbitalDesc);
	LastActivity &= ~(ActivityFlag::CheckLoad | ActivityFlag::CheckRestart);
	if ((uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad | ActivityFlag::StartEncounter)) || uqm::gameState().sis().CrewEnlisted == (uint16_t)~0
		|| uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 2)
	{
		return;
	}

	// Implement a to-do in generate.h for a better test
	if (pSolarSysState->TopoFrame)
	{ // We've entered orbit; LoadPlanet() called planet surface-gen code
		PlanetOrbitMenu();
		FreePlanet();
	}
	// Otherwise, generateOrbital function started a homeworld
	// conversation, and we did not get to the planet no matter what.

	// START_ENCOUNTER could be set by Devices menu a number of ways:
	// Talking Pet, Sun Device or a Caster over Chmmr, or
	// a Caster for Ilwrath
	// Could also have blown self up with Utwig Bomb
	if (!(uqm::currentActivity() & (ActivityFlag::StartEncounter | ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
		&& uqm::gameState().sis().CrewEnlisted != (uint16_t)~0)
	{ // Reload the system and return to the inner view
		const SystemPlanet* orbital = LoadSolarSys();
		assert(!orbital);
		CheckZoomLevel();
		ValidateOrbits();
		ValidateInnerOrbits();
		ResetSolarSys();

		if (uqm::UQMOptions::read().texturedPlanets)
		{
			if (worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
			{
				GenerateTexturedMoons(pSolarSysState,
									  pSolarSysState->pOrbitalDesc->pPrevDesc);
			}
			else
			{
				GenerateTexturedMoons(pSolarSysState,
									  pSolarSysState->pOrbitalDesc);
			}
		}

		RepairSISBorder();
		TransitionSystemIn();
	}
}

static void InitSolarSys(void)
{
	LoadIPData();
	LoadLanderData();

	const bool Reentry {(uqm::flagship().shipFacing != 0)};
	if (!Reentry)
	{
		uqm::flagship().autopilot.clear();

		uqm::flagship().shipStamp.origin.x =
			ResScale(OrigSisScreenWidth() >> 1);
		uqm::flagship().shipStamp.origin.y = SisScreenHeight() - ResScale(2);

		uqm::flagship().ipLocation = displayToLocation(
			uqm::flagship().shipStamp.origin,
			MaxZoomRadius);
	}

	SetColorMap(GetColorMapAddress(SunCMap, *StarColor(CurStarDescPtr()->Type)));
	if (GetFrameCount(SunFrame) > 5)
	{
		ChangeSunColor();
	}

	StarsFrame = GetStarBackGround(false);

	SetContext(SpaceContext);
	SetContextFGFrame(Screen);
	SetContextBackGroundColor(BlackColor);

	SystemPlanet* orbital {LoadSolarSys()};
	const bool InnerSystem {CheckZoomLevel()};
	ValidateOrbits();

	if (InnerSystem)
	{
		ValidateInnerOrbits();
	}

	if (Reentry)
	{
		(*pSolarSysState->genFuncs->reinitNpcs)(pSolarSysState);
	}
	else
	{
		EncounterRace = -1;
		EncounterGroup = 0;
		uqm::gameState().game().BattleGroupRef = 0;
		ReinitQueue(&uqm::gameState().game().ip_group_q);
		ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
		(*pSolarSysState->genFuncs->initNpcs)(pSolarSysState);
	}

	if (orbital)
	{
		enterOrbital(orbital);
	}
	else
	{ // Draw the borders, the system (inner or outer) and
		// fade/transition
		SetContext(SpaceContext);

		SetTransitionSource(nullptr);
		BatchGraphics();

		DrawSISFrame();
		DrawSISMessage(nullptr);

		ResetSolarSys();

		if (!isStarMarked(InternalStarIndex, "VISITED"))
		{
			setStarMarked(InternalStarIndex, "VISITED");
		}

		if (isStarMarked(InternalStarIndex, "PLYR_MARKER"))
		{
			setStarMarked(InternalStarIndex, "PLYR_MARKER");
		}

		// JMS: This is to prevent flashing the 3do "navigate"
		// unnecessarily whilst starting a new game.
		// SetFlashRect (nullptr);

		if (LastActivity == *(ActivityFlag::CheckLoad | ActivityFlag::CheckRestart))
		{ // Starting a new game, NOT from load!
			// We have to fade the screen in from intro or menu
			DrawOuterSystem();
			RedrawQueue(false);
			UnbatchGraphics();
			FadeScreen(ScreenFadeType::FadeAllToColor, c_gameSecond / 2);
			NewGameInit = true;

			LastActivity = 0;
		}
		else if (LastActivity == ActivityFlag::CheckLoad && !NextActivity)
		{ // Called just to load a game; invoked from Main menu
			// No point in drawing anything
			UnbatchGraphics();
		}
		else
		{ // Entered a new system, or loaded into inner or outer
			if (InnerSystem)
			{
				if (uqm::UQMOptions::read().texturedPlanets)
				{
					GenerateTexturedMoons(pSolarSysState,
										  pSolarSysState->pOrbitalDesc);
				}
				DrawInnerSystem();
			}
			else
			{
				DrawOuterSystem();
			}
			RedrawQueue(false);
			ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);
			UnbatchGraphics();

			LastActivity &= ~ActivityFlag::CheckLoad;
		}
	}
}

static void endInterPlanetary(void)
{
	uqm::currentActivity() &= ~EndInterplanetary;

	if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
	{
		// These are game state changing ops and so cannot be
		// called once another game has been loaded!
		(*pSolarSysState->genFuncs->uninitNpcs)(pSolarSysState);
		uqm::gameState().setFlag(uqm::GameFlag::USED_BROADCASTER, static_cast<uint32_t>(0));
	}
}

// Find the closest planet to a point, in interplanetary.
static SystemPlanet* closestPlanetInterPlanetary(const GfxPoint* point)
{
	uint8_t i {};
	uint32_t bestDistSquared {};
	SystemPlanet* bestPlanet = nullptr;

	assert(pSolarSysState != nullptr);

	const uint8_t numPlanets {pSolarSysState->SunDesc[0].NumPlanets};

	bestDistSquared = (uint32_t)-1; // Maximum value of uint32_t.
	for (i = 0; i < numPlanets; i++)
	{
		SystemPlanet* planet = &pSolarSysState->PlanetDesc[i];

		const int16_t dx = point->x - planet->image.origin.x;
		const int16_t dy = point->y - planet->image.origin.y;

		const uint32_t distSquared = (uint32_t)((long)dx * dx + (long)dy * dy);
		if (distSquared < bestDistSquared)
		{
			bestDistSquared = distSquared;
			bestPlanet = planet;
		}
	}

	return bestPlanet;
}

static void UninitSolarSys(void)
{
	FreeSolarSys();

	// FreeLanderData (); // JMS: This is not needed since the landerframes
	// won't reload if they're already loaded once.
	FreeIPData(); // JMS This IS necessary.

	DestroyDrawable(ReleaseDrawable(StarsFrame));
	StarsFrame = nullptr;

	if (uqm::currentActivity() & EndInterplanetary)
	{
		endInterPlanetary();
		return;
	}

	if ((uqm::currentActivity() & ActivityFlag::StartEncounter) && EncounterGroup)
	{
		GetGroupInfo(uqm::gameState().game().BattleGroupRef, EncounterGroup);
		// Generate the Encounter location name based on the closest planet

		if (uqm::flagship().ipPlanet == 0)
		{
			const SystemPlanet* planet =
				closestPlanetInterPlanetary(
					&uqm::flagship().shipStamp.origin);

			(*pSolarSysState->genFuncs->generateName)(
				pSolarSysState, planet);
		}
	}
}

static void CalcSunSize(SystemPlanet* pSunDesc, int16_t radius)
{
	static uint8_t frameCount = 0;
	int16_t index = 0;

	if (radius <= (MaxZoomRadius >> 1))
	{
		++index;
		if (radius <= (MaxZoomRadius >> 2))
		{
			++index;
		}
	}

	pSunDesc->image.origin.x = ResScale(OrigSisScreenWidth() >> 1);
	pSunDesc->image.origin.y = ResScale(OrigSisScreenHeight() >> 1);

	// JMS: Animating IP sun in hi-res modes...
	if (AnimatedSun())
	{
		pSunDesc->image.frame =
			SetRelFrameIndex(SunFrame, index * SunAnimframesNum + frameCount);
		frameCount = (frameCount + 1) & 0x1F;
	}
	else
	{
		pSunDesc->image.frame = SetRelFrameIndex(SunFrame, index);
	}
}

static void SetPlanetColorMap(SystemPlanet* planet)
{
	const uint16_t index = planet->data_index & ~*PlanetType::WorldTypeSpecial;
	assert(index < PlanetType::NumberOfPlanetTypes);
	SetColorMap(GetColorMapAddress(OrbitalCMap, *Plancolor(PlanData[index].Type)));
}

static void DrawInnerPlanets(SystemPlanet* planet)
{
	GfxStamp s;
	uint16_t i {};
	SystemPlanet* moon {};

	s.origin.x = ResScale(OrigSisScreenWidth() >> 1);
	s.origin.y = ResScale(OrigSisScreenHeight() >> 1);

	if (uqm::UQMOptions::read().texturedPlanets)
	{ // Draw the planet image
		RotatePlanets(true);
		DrawTexturedBody(planet, s);

		// Draw the moon images
		for (i = planet->NumPlanets, moon = pSolarSysState->MoonDesc.data();
			 i; --i, ++moon)
		{
			if (moon->data_index & *PlanetType::WorldTypeSpecial)
			{
				DrawStamp(&moon->image);
			}
			else
			{
				DrawTexturedBody(moon, moon->image);
			}
		}
	}
	else
	{ // Draw the planet image
		SetPlanetColorMap(planet);
		s.frame = planet->image.frame;

		i = planet->data_index & ~*PlanetType::WorldTypeSpecial;
		if (i < PlanetType::NumberOfPlanetTypes
			&& (planet->data_index & *PlanetType::PlanetShielded))
		{ // Shielded world looks "shielded" in inner view
			// s.frame = SetAbsFrameIndex (SpaceJunkFrame, 17);
			const uint16_t angle = ARCTAN(planet->location.x, planet->location.y);

			s.frame = SetAbsFrameIndex(OrbitalShield, NormalizeFacing(
														  AngleToFacing(angle)));
		}
		DrawStamp(&s);

		// Draw the moon images
		for (i = planet->NumPlanets, moon = pSolarSysState->MoonDesc.data();
			 i; --i, ++moon)
		{
			if (!(moon->data_index & *PlanetType::WorldTypeSpecial))
			{
				SetPlanetColorMap(moon);
			}
			DrawStamp(&moon->image);
		}
	}
}

static void DrawOuterPlanets(int16_t radius)
{
	SystemPlanet* pCurDesc {};

	CalcSunSize(&pSolarSysState->SunDesc[0], radius);
	if (uqm::UQMOptions::read().orbitingPlanets)
	{
		sortPlanetPositions();
	}

	int16_t index {pSolarSysState->FirstPlanetIndex};
	for (;;)
	{
		pCurDesc = &pSolarSysState->PlanetDesc[index];

		if (pCurDesc == &pSolarSysState->SunDesc[0])
		{ // It's a sun
			DrawStamp(&pCurDesc->image);
		}
		else
		{ // It's a planet
			if (uqm::UQMOptions::read().texturedPlanets)
			{
				RotatePlanets(false);
				DrawTexturedBody(pCurDesc, pCurDesc->image);
			}
			else
			{
				SetPlanetColorMap(pCurDesc);
				DrawStamp(&pCurDesc->image);
			}
		}
		if (index == pSolarSysState->LastPlanetIndex)
		{
			break;
		}
		index = pCurDesc->NextIndex;
	}
}

static void DrawSystem(int16_t radius, bool IsInnerSystem)
{
	uint8_t i {};
	SystemPlanet* pCurDesc {};
	SystemPlanet* pBaseDesc {};
	GfxStamp s;

	if (uqm::UQMOptions::read().texturedPlanets)
	{
		// BW: This to test if we have already rendered
		if (!pSolarSysState->PlanetDesc[0].orbit.lpTopoData)
		{
			GenerateTexturedPlanets();
		}
	}

	if (!SolarSysFrame)
	{ // Create the saved view graphic
		GfxRect clipRect;

		GetContextClipRect(&clipRect);
		SolarSysFrame = CaptureDrawable(CreateDrawable(CreateFlags::WantPixmap,
													   clipRect.extent.width, clipRect.extent.height, 1));
	}

	GfxContextRef oldContext {SetContext(OffScreenContext)};
	SetContextFGFrame(SolarSysFrame);
	SetContextClipRect(nullptr);

	DrawStarBackGround();

	pBaseDesc = pSolarSysState->pBaseDesc;
	if (IsInnerSystem)
	{ // Draw the inner system view *planet's* orbit segment
		pCurDesc = pSolarSysState->pOrbitalDesc;
		DrawOrbit(pCurDesc, ResScale(DisplayFactorUs() * 4),
				  DisplayFactor(), radius);
	}

	// Draw the planet orbits or moon orbits
	for (i = pBaseDesc->pPrevDesc->NumPlanets, pCurDesc = pBaseDesc;
		 i; --i, ++pCurDesc)
	{
		if (IsInnerSystem)
		{
			DrawOrbit(pCurDesc, 2, 1, 2);
		}
		else
		{
			DrawOrbit(pCurDesc, DisplayFactor(),
					  ResScale(DisplayFactorUs() / 4), radius);
		}
	}

	if (!uqm::UQMOptions::read().orbitingPlanets && !uqm::UQMOptions::read().texturedPlanets)
	{
		if (IsInnerSystem) // Draw the inner system view
		{
			DrawInnerPlanets(pSolarSysState->pOrbitalDesc);
		}
		else // Draw the outer system view
		{
			DrawOuterPlanets(radius);
		}
	}

	SetContext(oldContext);

	// Draw the now-saved view graphic
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SolarSysFrame;
	DrawStamp(&s);
}

void DrawStarBackGround(void)
{
	GfxStamp s;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = StarsFrame;
	DrawStamp(&s);
}

FrameRef GetStarBackFround(void)
{
	if (StarsFrame)
	{
		return StarsFrame;
	}
	else
	{
		return nullptr;
	}
}

static constexpr auto NumDimPieces {8};
static constexpr auto NumDimDrawn {5};
static constexpr auto NumBrtPieces {8};
static constexpr auto NumBrtDrawn {30};

static void DrawBackgroundStars(uint8_t num_pieces, uint8_t num_drawn, uint8_t start_index,
								uqm::UQMRandom& SysRNG, FrameRef junk)
{
	GfxStamp s;
	uint16_t j {};
	uint32_t rand_val {};

	s.frame = SetAbsFrameIndex(junk, start_index);
	for (uint16_t i {0}; i < num_pieces; ++i)
	{
		for (j = 0; j < num_drawn; ++j)
		{
			rand_val = SysRNG.next();
			s.origin.x = scaleSISWidth(uqm::lowWord(rand_val) % widthPick());
			s.origin.y = scaleSISHeight(uqm::highWord(rand_val) % heightPick());
			DrawStamp(&s);
		}
		s.frame = IncFrameIndex(s.frame);
	}
}

static void DrawNebula(FrameRef nebula)
{
	GfxStamp s;
	DrawMode oldMode;

	if (!nebula)
	{
		return;
	}

	oldMode = SetContextDrawMode(MAKE_DRAW_MODE(DrawKind::Alpha,
												(uqm::UQMOptions::read().nebulaevol + 1) * 5));
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = nebula;
	DrawStamp(&s);
	SetContextDrawMode(oldMode);
}

static FrameRef CreateStarBackGround(uqm::UQMRandom& SysRNG, FrameRef nebula, FrameRef junk)
{
	uint8_t num_brt_drawn {};
	uint8_t start_index {};
	GfxRect clipRect;
	FrameRef frame {};
	const bool hdScaled = (!uqm::UQMOptions::read().unscaledStarSystem || !IsHd());

	// Use SpaceContext to find out the dimensions of the background
	GfxContextRef oldContext {SetContext(SpaceContext)};
	GetContextClipRect(&clipRect);

	// Prepare a pre-drawn stars frame for this system
	frame = CaptureDrawable(CreateDrawable(CreateFlags::WantPixmap,
										   clipRect.extent.width, clipRect.extent.height, 1));
	SetContext(OffScreenContext);
	SetContextFGFrame(frame);
	SetContextClipRect(nullptr);
	SetContextBackGroundColor(BlackColor);

	ClearDrawable();

	start_index = hdScaled ? 0 : 25;
	DrawBackgroundStars(NumDimPieces, NumDimDrawn, start_index,
						SysRNG, junk);

	start_index += NumDimPieces;
	num_brt_drawn = NumBrtDrawn + (hdScaled ? 0 : 60);
	DrawBackgroundStars(NumBrtPieces, num_brt_drawn, start_index,
						SysRNG, junk);

	DrawNebula(nebula);

	SetContext(oldContext);

	return frame;
}

FrameRef GetStarBackGround(bool Encounter)
{
	GfxPoint location;

	if (!Encounter)
	{ // Casually entering any solar system
		SysGenRNG->reseed(GetRandomSeedForStar(CurStarDescPtr()));
		return CreateStarBackGround(*SysGenRNG, NebulaFrame,
									SpaceJunkFrame);
	}

	// Battle Segue - generate new independant frame

	if (!uqm::UQMOptions::read().nebulae) // Load default frame
	{
		return CaptureDrawable(LoadGraphic(SeguePmapAnimRes));
	}

	// Generate new independant frame, START_ENCOUNTER unloads all solarsys
	// resources so we have to load them again temporary

	uqm::UQMRandom SysRNG {};
	FrameRef junk {CaptureDrawable(LoadGraphic(IpbkgndMaskPmapAnimRes))};

	if (CurStarDescPtr())
	{
		location = CurStarDescPtr()->star_pt;
	}
	else
	{
		location = MAKE_POINT(
			LogxToUniverse(uqm::gameState().sis().log_x),
			LogyToUniverse(uqm::gameState().sis().log_y));
	}

	SysRNG.reseed(GetRandomSeedForVar(location));

	FrameRef nebula {LoadNebulaeFrame(location)};
	FrameRef result {CreateStarBackGround(SysRNG, nebula, junk)};

	DestroyDrawable(ReleaseDrawable(junk));
	junk = 0;
	DestroyDrawable(ReleaseDrawable(nebula));
	nebula = 0;

	return result;
}

void XFormIPLoc(GfxPoint* pIn, GfxPoint* pOut, bool ToDisplay)
{
	if (ToDisplay)
	{
		*pOut = locationToDisplay(*pIn,
								  pSolarSysState->SunDesc[0].radius);
	}
	else
	{
		*pOut = displayToLocation(*pIn,
								  pSolarSysState->SunDesc[0].radius);
	}
}

void ExploreSolarSys(void)
{
	SolarSystemState SolarSysState {};

	if (CurStarDescPtr() == 0)
	{
		GfxPoint universe;

		universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
		universe.y = LogyToUniverse(uqm::gameState().sis().log_y);
		CurStarDescPtr() = FindStar(0, &universe, 1, 1);
		if (!CurStarDescPtr())
		{
			uqm::log::critical(
				"ExploreSolarSys(): do not know where you are!");
			uqm::fatalExit();
		}
	}
	uqm::gameState().sis().log_x = UniverseToLogx(CurStarDescPtr()->star_pt.x);
	uqm::gameState().sis().log_y = UniverseToLogy(CurStarDescPtr()->star_pt.y);

	pSolarSysState = &SolarSysState;

	SolarSysState.genFuncs = getGenerateFunctions(CurStarDescPtr()->Index);
#ifdef DEBUG_STARSEED
	if (CurStarDescPtr()->Index)
	{
		fmt::print(stderr, "*** Entering {} ({}) System at "
						   "%05.1f : %05.1f ***\n",
				   starPresenceString(CurStarDescPtr()->Index),
				   CurStarDescPtr()->Index,
				   (float)CurStarDescPtr()->star_pt.x / 10,
				   (float)CurStarDescPtr()->star_pt.y / 10);
	}
#endif

	InitSolarSys();
	SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
	SolarSysState.InputFunc = DoIpFlight;

	DoInput(&SolarSysState, false);

	UninitSolarSys();
	pSolarSysState = 0;
}

const char* GetNamedPlanetaryBody(void)
{
	if (!CurStarDescPtr() || !playerInSolarSystem()
		|| !playerInInnerSystem())
	{
		return nullptr; // Not inside an inner system, so no name
	}

	assert(pSolarSysState->pOrbitalDesc != nullptr);

	if (CurStarDescPtr()->Index == PlotId::Sol)
	{ // Planets and moons in Sol
		int planet {};

		planet =
			planetIndex(pSolarSysState, pSolarSysState->pOrbitalDesc);

		if (worldIsPlanet(pSolarSysState, pSolarSysState->pOrbitalDesc))
		{ // A planet
			return GameString(GameStringBase::PlanetNumber + planet);
		}

		// Moons
		const int moon {moonIndex(pSolarSysState, pSolarSysState->pOrbitalDesc)};
		switch (planet)
		{
			case 2: // Earth
				switch (moon)
				{
					case 0: // Starbase
						return GameString(GameStringBase::StarbaseString + 0);
					case 1: // Luna
						return GameString(GameStringBase::PlanetNumber + 9);
				}
				break;
			case 4: // Jupiter
				switch (moon)
				{
					case 0: // Io
						return GameString(GameStringBase::PlanetNumber + 10);
					case 1: // Europa
						return GameString(GameStringBase::PlanetNumber + 11);
					case 2: // Ganymede
						return GameString(GameStringBase::PlanetNumber + 12);
					case 3: // Callisto
						return GameString(GameStringBase::PlanetNumber + 13);
				}
				break;
			case 5:			   // Saturn
				if (moon == 0) // Titan
				{
					return GameString(GameStringBase::PlanetNumber + 14);
				}
				break;
			case 7:			   // Neptune
				if (moon == 0) // Triton
				{
					return GameString(GameStringBase::PlanetNumber + 15);
				}
				break;
			case 8:			   // Pluto
				if (moon == 0) // Charon
				{
					return GameString(GameStringBase::PlanetNumber + 34);
				}
				break;
		}
	}
	else if (CurStarDescPtr()->Index == PlotId::Samatra
			 && matchWorld(pSolarSysState, pSolarSysState->pOrbitalDesc,
						   pSolarSysState->SunDesc[0].PlanetByte,
						   pSolarSysState->SunDesc[0].MoonByte))
	{ // Sa-Matra
		return GameString(GameStringBase::PlanetNumber + 32);
	}
	else if (CurStarDescPtr()->Index > 0
			 && matchWorld(pSolarSysState, pSolarSysState->pOrbitalDesc,
						   pSolarSysState->SunDesc[0].PlanetByte, MatchPlanet))
	{
		switch (static_cast<PlotId>(CurStarDescPtr()->Index))
		{
			case PlotId::Shofixti: // Kyabetsu
				if (IsHomeworldKnown(*HomeId::Shofixti))
				{
					return GameString(GameStringBase::PlanetNumber + 35);
				}
				break;
			case PlotId::StartColony: // Unzervalt
				return GameString(GameStringBase::PlanetNumber + 33);
			case PlotId::Spathi: // Spathiwa
				if (IsHomeworldKnown(*HomeId::Spathi))
				{
					return GameString(GameStringBase::PlanetNumber + 37);
				}
				break;
			case PlotId::Syreen: // Gaia
				if ((uqm::gameState().flag(uqm::GameFlag::SYREEN_HOME_VISITS)
					 || uqm::gameState().flag(uqm::GameFlag::SYREEN_KNOW_ABOUT_MYCON)))
				{
					return GameString(GameStringBase::PlanetNumber + 39);
				}
				break;
			case PlotId::Slylandro: // Source
				if (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_HOME_VISITS))
				{
					return GameString(GameStringBase::PlanetNumber + 36);
				}
				break;
			case PlotId::Druuge: // Trade HQ
				if (IsHomeworldKnown(*HomeId::Druuge))
				{
					return GameString(GameStringBase::PlanetNumber + 41);
				}
				break;
			case PlotId::EggCase0: // Syra
				return GameString(GameStringBase::PlanetNumber + 42);
			case PlotId::Utwig: // Fahz
				if (IsHomeworldKnown(*HomeId::Utwig))
				{
					return GameString(GameStringBase::PlanetNumber + 40);
				}
				break;
			case PlotId::Supox: // Vlik
				if (uqm::gameState().flag(uqm::GameFlag::SUPOX_STACK1) > 2)
				{
					return GameString(GameStringBase::PlanetNumber + 38);
				}
				break;
			default:
				return nullptr;
		}
	}
	return nullptr;
}

void GetPlanetOrMoonName(char* buf, uint16_t bufsize)
{
	int i {};
	bool name_has_suffix = false;

	const char* named {GetNamedPlanetaryBody()};
	if (named)
	{
		uqm::utf8StringCopy(buf, bufsize, named);
		return;
	}

	// Either not named or we already have a name
	uqm::utf8StringCopy(buf, bufsize, uqm::gameState().sis().PlanetName);

	if (!playerInSolarSystem() || !playerInInnerSystem()
		|| worldIsPlanet(pSolarSysState, pSolarSysState->pOrbitalDesc)
		|| is3DO(uqm::UQMOptions::read().whichFonts))
	{ // Outer or inner system or orbiting a planet
		return;
	}

	// Orbiting an unnamed moon
	i = strlen(buf);
	const char* tempbuf {buf};
	buf += i;
	bufsize -= i;
	const int moon {moonIndex(pSolarSysState, pSolarSysState->pOrbitalDesc)};

	// log_add (log_Debug,"last %02d, i {}", tempbuf[i-1], i);
	// JMS: Prevent printing something like 'planet II-A-A' in summary
	// screen.
	if (i > 0)
	{
		if (tempbuf[i - 1] == 'A' || tempbuf[i - 1] == 'B'
			|| tempbuf[i - 1] == 'C' || tempbuf[i - 1] == 'D')
		{
			name_has_suffix = true;
		}
	}

	if (bufsize >= 3 && !name_has_suffix)
	{
		fmt::format_to_sz_n(buf, bufsize, "-{}", 'A' + moon);
		buf[bufsize - 1] = '\0';
	}
}

void SaveSolarSysLocation(void)
{
	assert(playerInSolarSystem());

	// This is a two-stage saving procedure
	// Stage 1: called when saving from inner/outer view
	// Stage 2: called when saving from orbital

	if (!playerInPlanetOrbit())
	{
		saveNonOrbitalLocation();
	}
	else
	{ // In orbit around a planet.
		uint8_t moon {};

		// Update the starinfo.dat file if necessary.
		if (uqm::gameState().flag(uqm::GameFlag::PLANETARY_CHANGE))
		{
			PutPlanetInfo();
			uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_CHANGE, static_cast<uint32_t>(0));
		}

		// GLOBAL (ip_planet) is already set
		assert(uqm::flagship().ipPlanet != 0);

		// has to be at least 1 because code tests for in_orbit!=0
		moon = 1; /* the planet itself */
		if (worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
		{
			moon += moonIndex(pSolarSysState,
							  pSolarSysState->pOrbitalDesc);
			// +1 because moons have to be 1-based
			moon += 1;
		}
		uqm::flagship().inOrbit = moon;
	}
}

static bool DoSolarSysMenu(MenuState* pMS)
{
	bool select = PulsedInputState.menu[*MenuKey::MenuSelect];

	if ((uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
		|| uqm::gameState().sis().CrewEnlisted == (uint16_t)~0)
	{
		return false;
	}

	const bool handled {DoMenuChooser(pMS, MenuString::Starmap)};
	if (handled)
	{
		return true;
	}

	if (LastActivity == ActivityFlag::CheckLoad)
	{
		select = true; // Selected LOAD from main menu
	}

	if (!select)
	{
		return true;
	}

	SetFlashRect(nullptr, false);

	switch (pMS->CurState)
	{
		case *SolarSysMenuItem::EquipDevice:
			select = DevicesMenu();
			if (uqm::currentActivity() & ActivityFlag::StartEncounter)
			{ // Invoked Talking Pet or a Caster for Ilwrath
				// Going into conversation
				return false;
			}
			break;
		case *SolarSysMenuItem::Cargo:
			CargoMenu();
			break;
		case *SolarSysMenuItem::Roster:
			select = RosterMenu();
			break;
		case *SolarSysMenuItem::GameMenu:
			if (!GameOptions())
			{
				return false; // abort or load
			}
			break;
		case *SolarSysMenuItem::Starmap:
			StarMap();
			if (uqm::currentActivity() & ActivityFlag::CheckAbort)
			{
				return false;
			}

			TransitionSystemIn();

			if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
			{
				DrawMenuStateStrings(MenuString::Starmap, *SolarSysMenuItem::Navigation);
			}
			// Fall through !!!
		case *SolarSysMenuItem::Navigation:
			return false;
	}

	if (!(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		if (select)
		{ // 3DO menu jumps to NAVIGATE after a successful submenu run
			if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
			{
				pMS->CurState = *SolarSysMenuItem::Navigation;
			}
			DrawMenuStateStrings(MenuString::Starmap, pMS->CurState);
		}
		SetFlashRect(SfrMenu3do(), false);
	}

	return true;
}

static void SolarSysMenu(void)
{
	MenuState menuState;

	memset(&menuState, 0, sizeof menuState);

	if (LastActivity == ActivityFlag::CheckLoad)
	{ // Selected LOAD from main menu
		menuState.CurState = *SolarSysMenuItem::GameMenu;
	}
	else
	{
		DrawMenuStateStrings(MenuString::Starmap, *SolarSysMenuItem::Starmap);
		menuState.CurState = *SolarSysMenuItem::Starmap;
	}

	DrawStatusMessage(nullptr);
	SetFlashRect(SfrMenu3do(), false);

	SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
	menuState.InputFunc = DoSolarSysMenu;
	DoInput(&menuState, true);

	if (!(uqm::currentActivity() & ActivityFlag::CheckLoad))
	{
		DrawMenuStateStrings(MenuString::Starmap, -SolarSysMenuItem::Navigation);
	}
}

static bool DoIpFlight(SolarSystemState* pSS)
{
	//static TimeCount NextTime; unused
	const bool cancel = PulsedInputState.menu[*MenuKey::MenuCancel];

	if (pSS->InOrbit)
	{ // CheckShipLocation() or InitSolarSys() sent us to orbital
		EnterPlanetOrbit();
		SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
		pSS->InOrbit = false;
	}
	else if (!NewGameInit && (cancel || LastActivity == ActivityFlag::CheckLoad))
	{
		SolarSysMenu();
		SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
	}
	else if (!(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		static TimeCount TimeOutIP;
		static TimeCount TimeOutClock;
		const TimeCount Now = GetTimeCounter();

		assert(pSS->InIpFlight);

		if (Now >= TimeOutClock)
		{
			GameClockTick();
			TimeOutClock = Now + ClockFrameRate;
		}

		if (Now >= TimeOutIP)
		{
			IP_frame();
			TimeOutIP = Now + IpFrameRate;
		}

		if (NewGameInit)
		{
			SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
			SettingsMenu(false);
			SolarSysMenu();
			SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
		}
	}

	return (!(uqm::currentActivity()
			  & (ActivityFlag::StartEncounter | EndInterplanetary
				 | ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
			&& uqm::gameState().sis().CrewEnlisted != (uint16_t)~0);
}

static int16_t widthPick(void)
{
	const auto& sis_screen_dimensions {SisScreenDimensions};

	return sis_screen_dimensions[uqm::UQMOptions::read().starBackground].width;
}

static int16_t heightPick(void)
{
	const auto& sis_screen_dimensions {SisScreenDimensions};

	return sis_screen_dimensions[uqm::UQMOptions::read().starBackground].height;
}

static int16_t scaleSISWidth(int16_t value)
{
	float percent = 1;

	if (widthPick() != OrigSisScreenWidth())
	{
		percent = scaleThing(OrigSisScreenWidth(), widthPick());
	}

	return ResScale((int16_t)(value * percent));
}

static int16_t scaleSISHeight(int16_t value)
{
	float percent = 1;

	if (heightPick() != OrigSisScreenHeight())
	{
		percent = scaleThing(OrigSisScreenHeight(), heightPick());
	}

	return ResScale((int16_t)(value * percent));
}

void InitialIntersect(void)
{ // need to check collision with moons on load
	// rare but annoying
	CheckIntersect();
}

} // namespace uqm
