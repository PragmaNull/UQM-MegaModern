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
#ifndef UQM_PLANETS_PLANDATA_H
#define UQM_PLANETS_PLANDATA_H

#include "core/types/enum.h"
#include "libs/reslib.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

/*------------------------------ Type Defines ----------------------------- */


enum class PlanetSize : uint8_t
{
	SmallRockyWorld = 0,
	LargeRockyWorld,
	GasGiant,

	Count
};
consteval bool isArithmeticEnum(PlanetSize) { return true; }

enum class StellarSize : uint8_t
{
	Dwarf = 0,
	Giant,
	SuperGiant,

	Count
};
consteval bool isArithmeticEnum(StellarSize) { return true; }

enum class StarBodyColor : uint8_t
{
	Blue = 0,
	Green,
	Orange,
	Red,
	White,
	Gray = White,
	Yellow,

	Count,

	Cyan = Count,
	Purple,
	Violet,
	NumPlanetColors
};
consteval bool isArithmeticEnum(StarBodyColor) { return true; }

enum class SystemOwner
{
	Nobody = 0,
	Neutral,
	Hierarchy,

	Player = (1 << 2)
};
consteval bool isArithmeticEnum(SystemOwner) { return true; }

inline constexpr auto StarOwnerShift {0};
inline constexpr auto StarTypeShift {3};  /* STAR_OWNER_SHIFT + 3 */
inline constexpr auto StarColorShift {5}; /* STAR_TYPE_SHIFT + 2 */
inline constexpr auto StarColorMask {(uint8_t)(0xFF << StarColorShift)};
inline constexpr auto StarTypeMask {(uint8_t)((0xFF << StarTypeShift) & ~StarColorMask)};
inline constexpr auto StarOwnerMask {(uint8_t)((0xFF << StarOwnerShift) & ~(StarColorMask | StarTypeMask))};
inline constexpr auto StarUnknownMask {StarOwnerMask & ~*SystemOwner::Player};

constexpr auto MakeStar(const auto& t, const auto& c, const auto& o) { return (uint8_t)((((uint8_t)(t) << StarTypeShift) & StarTypeMask) | (((uint8_t)(c) << StarColorShift) & StarColorMask) | (((uint8_t)(o) << StarOwnerShift) & StarOwnerMask)); }
constexpr auto StarType(const auto& f) { return static_cast<StellarSize>(((f)&StarTypeMask) >> StarTypeShift); }
constexpr auto StarColor(const auto& f) { return static_cast<StarBodyColor>(((f)&StarColorMask) >> StarColorShift); }
constexpr auto StarOwner(const auto& f) { return static_cast<SystemOwner>(((f)&StarOwnerMask) >> StarOwnerShift); }


inline constexpr auto PlanSizeMask {0x03};

inline constexpr auto TopoAlgo {0 << 2};
inline constexpr auto CrateredAlgo {1 << 2};
inline constexpr auto GasGiantAlgo {2 << 2};
inline constexpr auto PlanAlgoMask {0x0C};

constexpr auto Plansize(const auto& type) { return static_cast<PlanetSize>((type)&PlanSizeMask); }
constexpr auto Planalgo(const auto& type) { return ((uint8_t)((type)&PlanAlgoMask)); }
constexpr auto Plancolor(const auto& type) { return static_cast<StarBodyColor>(uqm::highNibble(type)); }

inline constexpr auto ThinAtmosphere {10};
inline constexpr auto NormalAtmosphere {75};
inline constexpr auto ThickAtmosphere {200};
inline constexpr auto SuperThickAtmosphere {2500};
inline constexpr auto GasGiantAtmosphere {0xFFFF};

enum class PlanetType
{
	FirstRocky = 0,
	FirstSmallRocky = FirstRocky,

	Oolite = FirstSmallRocky,
	Yttric,
	QuasiDegenerate,
	Lanthanide,
	Treasure,
	Urea,
	Metal,
	Radioactive,
	Opalescent,
	Cyanic,
	Acid,
	Alkali,
	Halide,
	Green,
	Copper,
	Carbide,
	Ultramarine,
	Noble,
	Azure,
	Chondrite,
	Purple,
	SuperDense,
	Pellucid,
	Dust,
	Crimson,
	Cimmerian,
	Infrared,
	Selenic,
	Auric,
	LastSmallRocky = Auric,

	FirstLargeRocky,
	Fluorescent = FirstLargeRocky,
	Ultraviolet,
	Plutonic,
	Rainbow,
	Shattered,
	Sapphire,
	Organic,
	Xenolithic,
	Redux,
	Primordial,
	Emerald,
	Chlorine,
	Magnetic,
	Water,
	Telluric,
	Hydrocarbon,
	Iodine,
	Vinylogous,
	Ruby,
	Magma,
	Maroon,
	//DIAMOND_WORLD,
	LastLargeRocky = Maroon,
	LastRocky = LastLargeRocky,

	FirstGasGiant,
	BluGasGiant = FirstGasGiant, /* Gas Giants */
	CyaGasGiant,
	GrnGasGiant,
	GryGasGiant,
	OraGasGiant,
	PurGasGiant,
	RedGasGiant,
	VioGasGiant,
	YelGasGiant,
	LastGasGiant = YelGasGiant,

	NumberOfPlanetTypes,

	WorldTypeSpecial = 0x80,
	PlanetShielded = WorldTypeSpecial,

	HierarchyStarbase = 127 | WorldTypeSpecial,
	SaMatra = 126 | WorldTypeSpecial,
	DestroyedStarbase = 125 | WorldTypeSpecial,
	PrecursorStarbase = 124 | WorldTypeSpecial,
};
consteval bool isArithmeticEnum(PlanetType) { return true; }

inline constexpr auto NumberOfSmallRockyWorlds {PlanetType::LastSmallRocky - PlanetType::FirstSmallRocky + 1};
inline constexpr auto NumberOfLargeRockyWorlds {PlanetType::LastLargeRocky - PlanetType::FirstLargeRocky + 1};
inline constexpr auto NumberOfRockyWorlds {NumberOfSmallRockyWorlds + NumberOfLargeRockyWorlds};
inline constexpr auto NumberOfGasGiants {PlanetType::LastGasGiant - PlanetType::FirstGasGiant + 1};

// TODO: This struct is highly alignment and padding dependent and
//   should not be used! The data is loaded as binary from files and
//   cast to this struct.
struct PlanetColorTranslation
{
	const int16_t level_tab[3];
	const uint8_t xlat_tab[256];
};

struct ElementDeposit
{
	uint8_t ElementType;
	/* Index of this Element in element_array */
	uint8_t Density;
	/* bits 0-3: quantity of the deposits (maximum number of
			 *           deposits), one of Few, Moderate, or Numerous
			 * bits 4-7: quality of the deposit, one of LOW, MEDIUM, or HEAVY
			 */
};

// PlanetFrame describes a type of planet. It is not used to describe
// individual planets.
inline constexpr auto NumUsefulElements {8};

struct PlanetFrame
{
	uint8_t Type;
	/* bits 0-1: size, one of SMALL_ROCKY_WORLD, LARGE_ROCKY_WORLD, or
			 *           GAS_GIANT
			 * bits 2-3: map creation algoritm, one of TopoAlgo,
			 *           CrateredAlgo, or GasGiantAlgo
			 * bits 4-7: interplanetary color, one of BLUE_BODY, GREEN_BODY,
			 *           ORANGE_BODY, RED_BODY, WHITE_BODY (same as
			 *           GRAY_BODY), YELLOW_BODY, CYAN_BODY, PURPLE_BODY,
			 *           VIOLET_BODY)
             */
	uint8_t BaseTectonics;
	/* Base constant for calculation of tectonic activity,
			 * relative to Earth at 100.
			 * One of: NoTectonics, LowTectonics, MedTectonics,
			 *         HighTectonics, or SuperTectonics
			 */
	uint8_t AtmoAndDensity;
	/* bits 0-3: planet density, one of GAS_DENSITY, LIGHT_DENSITY,
			 *           LOW_DENSITY, NORMAL_DENSITY, HIGH_DENSITY,
			 *           SUPER_DENSITY
			 * bits 4-7: atmosphere, one of LIGHT, MEDIUM, HEAVY, or
			 *           (no define for this) super thick.
			 */
	ElementDeposit UsefulElements[NumUsefulElements];
	/* Minerals on the planet */

	RESOURCE CMapInstance;
	/* Color map */
	RESOURCE XlatTabInstance;
	/* Color translation map */

	// Parameters for map-generation algoritms:
	uint16_t num_faults;
	int16_t fault_depth;
	uint16_t num_blemishes;
	int16_t base_elevation;
};

struct PlanetData
{
	int16_t AxialTilt;
	uint16_t Tectonics;
	uint16_t Weather;
	uint16_t PlanetDensity;
	uint16_t PlanetRadius;
	uint16_t SurfaceGravity;
	int16_t SurfaceTemperature;
	uint16_t RotationPeriod;
	uint16_t AtmoDensity;
	int16_t LifeChance;
	uint32_t PlanetToSunDist;

	const PlanetFrame* PlanDataPtr;

	uint32_t ScanSeed[*ScanType::Count];
	uint32_t ScanRetrieveMask[*ScanType::Count];

	// JMS: This stores the amount of partially scavenged minerals.
	//
	// How it works: When the mineral deposits are generated in scan.c, first the deposit
	// is given its normal kiloton-size (decided by pseudo-random number). Then, the value
	// stored in this list is subtracted from the initial value.
	// There can be max uint32_t = 32 mineral deposits on the planet, thus the 32.
	//
	// This can be later made to support also partially scavenged biodata.
	uint8_t PartiallyScavengedList[*ScanType::Count][32];

	uqm::StringTablePtr DiscoveryStrings {};
	size_t DiscoveryIndex {};
	FONT LanderFont;
	FRAME LanderFontEff;
};

enum class DensityClass
{
	Gas,
	Light,
	Low,
	Normal,
	High,
	Super
};
consteval bool isArithmeticEnum(DensityClass) { return true; }

extern uint16_t CalcGravity(const PlanetData*);

inline constexpr auto EarthAtmosphere {50};

inline constexpr auto ColdThreshold {-40};
inline constexpr auto HotThreshold {100};

/*------------------------------ Global Data ------------------------------ */

inline constexpr auto NoTectonics {0};
inline constexpr auto LowTectonics {40};
inline constexpr auto MedTectonics {80};
inline constexpr auto HighTectonics {140};
inline constexpr auto SuperTectonics {200};

const extern PlanetFrame* PlanData;

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_PLANETS_PLANDATA_H
