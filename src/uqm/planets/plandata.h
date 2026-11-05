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

#ifndef UQM_PLANETS_PLANDATA_H_
#define UQM_PLANETS_PLANDATA_H_

#include "libs/reslib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

/*------------------------------ Type Defines ----------------------------- */


enum
{
	SMALL_ROCKY_WORLD = 0,
	LARGE_ROCKY_WORLD,
	GAS_GIANT,

	NUM_PLANET_TYPES
};

enum
{
	DWARF_STAR = 0,
	GIANT_STAR,
	SUPER_GIANT_STAR,

	NUM_STAR_TYPES
};

enum
{
	BLUE_BODY = 0,
	GREEN_BODY,
	ORANGE_BODY,
	RED_BODY,
	WHITE_BODY,
	GRAY_BODY = WHITE_BODY,
	YELLOW_BODY,

	NUM_STAR_COLORS,

	CYAN_BODY = NUM_STAR_COLORS,
	PURPLE_BODY,
	VIOLET_BODY,
	NUM_PLANET_COLORS
};

enum
{
	OWNER_NOBODY = 0,
	OWNER_NEUTRAL,
	OWNER_HIERARCHY,

	OWNER_PLAYER = (1 << 2)
};

inline constexpr auto StarOwnerShift {0};
inline constexpr auto StarTypeShift {3};  /* STAR_OWNER_SHIFT + 3 */
inline constexpr auto StarColorShift {5}; /* STAR_TYPE_SHIFT + 2 */
inline constexpr auto StarColorMask {(uint8_t)(0xFF << StarColorShift)};
inline constexpr auto StarTypeMask {(uint8_t)((0xFF << StarTypeShift) & ~StarColorMask)};
inline constexpr auto StarOwnerMask {(uint8_t)((0xFF << StarOwnerShift) & ~(StarColorMask | StarTypeMask))};
inline constexpr auto StarUnknownMask {(StarOwnerMask & ~OWNER_PLAYER)};

constexpr auto MakeStar(const auto& t, const auto& c, const auto& o) { return (uint8_t)((((uint8_t)(t) << StarTypeShift) & StarTypeMask) | (((uint8_t)(c) << StarColorShift) & StarColorMask) | (((uint8_t)(o) << StarOwnerShift) & StarOwnerMask)); }
constexpr auto StarType(const auto& f) { return (uint8_t)(((f)&StarTypeMask) >> StarTypeShift); }
constexpr auto StarColor(const auto& f) { return (uint8_t)(((f)&StarColorMask) >> StarColorShift); }
constexpr auto StarOwner(const auto& f) { return (uint8_t)(((f)&StarOwnerMask) >> StarOwnerShift); }


inline constexpr auto PlanSizeMask {0x03};

inline constexpr auto TopoAlgo {(0 << 2)};
inline constexpr auto CrateredAlgo {(1 << 2)};
inline constexpr auto GasGiantAlgo {(2 << 2)};
inline constexpr auto PlanAlgoMask {0x0C};

constexpr auto Plansize(const auto& type) { return ((uint8_t)((type)&PlanSizeMask)); }
constexpr auto Planalgo(const auto& type) { return ((uint8_t)((type)&PlanAlgoMask)); }
constexpr auto Plancolor(const auto& type) { return uqm::highNibble(type); }

inline constexpr auto ThinAtmosphere {10};
inline constexpr auto NormalAtmosphere {75};
inline constexpr auto ThickAtmosphere {200};
inline constexpr auto SuperThickAtmosphere {2500};
inline constexpr auto GasGiantAtmosphere {0xFFFF};

enum
{
	FIRST_ROCKY_WORLD = 0,
	FIRST_SMALL_ROCKY_WORLD = FIRST_ROCKY_WORLD,

	OOLITE_WORLD = FIRST_SMALL_ROCKY_WORLD,
	YTTRIC_WORLD,
	QUASI_DEGENERATE_WORLD,
	LANTHANIDE_WORLD,
	TREASURE_WORLD,
	UREA_WORLD,
	METAL_WORLD,
	RADIOACTIVE_WORLD,
	OPALESCENT_WORLD,
	CYANIC_WORLD,
	ACID_WORLD,
	ALKALI_WORLD,
	HALIDE_WORLD,
	GREEN_WORLD,
	COPPER_WORLD,
	CARBIDE_WORLD,
	ULTRAMARINE_WORLD,
	NOBLE_WORLD,
	AZURE_WORLD,
	CHONDRITE_WORLD,
	PURPLE_WORLD,
	SUPER_DENSE_WORLD,
	PELLUCID_WORLD,
	DUST_WORLD,
	CRIMSON_WORLD,
	CIMMERIAN_WORLD,
	INFRARED_WORLD,
	SELENIC_WORLD,
	AURIC_WORLD,
	LAST_SMALL_ROCKY_WORLD = AURIC_WORLD,

	FIRST_LARGE_ROCKY_WORLD,
	FLUORESCENT_WORLD = FIRST_LARGE_ROCKY_WORLD,
	ULTRAVIOLET_WORLD,
	PLUTONIC_WORLD,
	RAINBOW_WORLD,
	SHATTERED_WORLD,
	SAPPHIRE_WORLD,
	ORGANIC_WORLD,
	XENOLITHIC_WORLD,
	REDUX_WORLD,
	PRIMORDIAL_WORLD,
	EMERALD_WORLD,
	CHLORINE_WORLD,
	MAGNETIC_WORLD,
	WATER_WORLD,
	TELLURIC_WORLD,
	HYDROCARBON_WORLD,
	IODINE_WORLD,
	VINYLOGOUS_WORLD,
	RUBY_WORLD,
	MAGMA_WORLD,
	MAROON_WORLD,
	//DIAMOND_WORLD,
	LAST_LARGE_ROCKY_WORLD = MAROON_WORLD,
	LAST_ROCKY_WORLD = LAST_LARGE_ROCKY_WORLD,

	FIRST_GAS_GIANT,
	BLU_GAS_GIANT = FIRST_GAS_GIANT, /* Gas Giants */
	CYA_GAS_GIANT,
	GRN_GAS_GIANT,
	GRY_GAS_GIANT,
	ORA_GAS_GIANT,
	PUR_GAS_GIANT,
	RED_GAS_GIANT,
	VIO_GAS_GIANT,
	YEL_GAS_GIANT,
	LAST_GAS_GIANT = YEL_GAS_GIANT,

	NUMBER_OF_PLANET_TYPES,

	WORLD_TYPE_SPECIAL = 0x80,
	PLANET_SHIELDED = WORLD_TYPE_SPECIAL,

	HIERARCHY_STARBASE = 127 | WORLD_TYPE_SPECIAL,
	SA_MATRA = 126 | WORLD_TYPE_SPECIAL,
	DESTROYED_STARBASE = 125 | WORLD_TYPE_SPECIAL,
	PRECURSOR_STARBASE = 124 | WORLD_TYPE_SPECIAL,
};

inline constexpr auto NumberOfSmallRockyWorlds {(LAST_SMALL_ROCKY_WORLD - FIRST_SMALL_ROCKY_WORLD + 1)};
inline constexpr auto NumberOfLargeRockyWorlds {(LAST_LARGE_ROCKY_WORLD - FIRST_LARGE_ROCKY_WORLD + 1)};
inline constexpr auto NumberOfRockyWorlds {(NumberOfSmallRockyWorlds + NumberOfLargeRockyWorlds)};
inline constexpr auto NumberOfGasGiants {(LAST_GAS_GIANT - FIRST_GAS_GIANT + 1)};

// TODO: This struct is highly alignment and padding dependent and
//   should not be used! The data is loaded as binary from files and
//   cast to this struct.
struct XLAT_DESC
{
	const int16_t level_tab[3];
	const uint8_t xlat_tab[256];
};

struct ELEMENT_ENTRY
{
	uint8_t ElementType;
	/* Index of this element in element_array */
	uint8_t Density;
	/* bits 0-3: quantity of the deposits (maximum number of
			 *           deposits), one of Few, Moderate, or Numerous
			 * bits 4-7: quality of the deposit, one of LOW, MEDIUM, or HEAVY
			 */
};

// PlanetFrame describes a type of planet. It is not used to describe
// individual planets.
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
#define NUM_USEFUL_ELEMENTS 8
	ELEMENT_ENTRY UsefulElements[NUM_USEFUL_ELEMENTS];
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

struct PLANET_INFO
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

	uint32_t ScanSeed[NUM_SCAN_TYPES];
	uint32_t ScanRetrieveMask[NUM_SCAN_TYPES];

	// JMS: This stores the amount of partially scavenged minerals.
	//
	// How it works: When the mineral deposits are generated in scan.c, first the deposit
	// is given its normal kiloton-size (decided by pseudo-random number). Then, the value
	// stored in this list is subtracted from the initial value.
	// There can be max uint32_t = 32 mineral deposits on the planet, thus the 32.
	//
	// This can be later made to support also partially scavenged biodata.
	uint8_t PartiallyScavengedList[NUM_SCAN_TYPES][32];

	uqm::StringTablePtr DiscoveryStrings {};
	size_t DiscoveryIndex {};
	FONT LanderFont;
	FRAME LanderFontEff;
};

enum
{
	GAS_DENSITY,
	LIGHT_DENSITY,
	LOW_DENSITY,
	NORMAL_DENSITY,
	HIGH_DENSITY,
	SUPER_DENSITY
};

extern uint16_t CalcGravity(const PLANET_INFO*);

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

#endif /* UQM_PLANETS_PLANDATA_H_ */
