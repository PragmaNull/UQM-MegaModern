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

#include <math.h>
#include <stdlib.h>

#include "../clock.h"
#include "../starmap.h"
#include "core/log/log.h"
#include "core/misc/bittricks.h"
#include "core/types/enum.h"
#include "libs/mathlib.h"
#include "planets.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
//#define DEBUG_ORBITS

enum class PlanetChance
{
	Never = 0,
	Rare = 15,
	Few = 63,
	Common = 127,
	Always = 255
};
consteval bool isArithmeticEnum(PlanetChance) { return true; }

static uint8_t BlueDistribution(uint8_t which_world)
{
	const uqstl::array<PlanetChance, *PlanetType::NumberOfPlanetTypes> PlanetDistribution {
		PlanetChance::Always, /* OOLITE_WORLD */
		PlanetChance::Always, /* YTTRIC_WORLD */
		PlanetChance::Always, /* QUASI_DEGENERATE_WORLD */
		PlanetChance::Always, /* LANTHANIDE_WORLD */
		PlanetChance::Always, /* TREASURE_WORLD */
		PlanetChance::Always, /* UREA_WORLD */
		PlanetChance::Always, /* METAL_WORLD */
		PlanetChance::Always, /* RADIOACTIVE_WORLD */
		PlanetChance::Always, /* OPALESCENT_WORLD */
		PlanetChance::Always, /* CYANIC_WORLD */
		PlanetChance::Always, /* ACID_WORLD */
		PlanetChance::Always, /* ALKALI_WORLD */
		PlanetChance::Always, /* HALIDE_WORLD */
		PlanetChance::Always, /* GREEN_WORLD */
		PlanetChance::Always, /* COPPER_WORLD */
		PlanetChance::Always, /* CARBIDE_WORLD */
		PlanetChance::Always, /* ULTRAMARINE_WORLD */
		PlanetChance::Always, /* NOBLE_WORLD */
		PlanetChance::Always, /* AZURE_WORLD */
		PlanetChance::Never,  /* CHONDRITE_WORLD */
		PlanetChance::Never,  /* PURPLE_WORLD */
		PlanetChance::Never,  /* SUPER_DENSE_WORLD */
		PlanetChance::Never,  /* PELLUCID_WORLD */
		PlanetChance::Never,  /* DUST_WORLD */
		PlanetChance::Never,  /* CRIMSON_WORLD */
		PlanetChance::Never,  /* CIMMERIAN_WORLD */
		PlanetChance::Never,  /* INFRARED_WORLD */
		PlanetChance::Always, /* SELENIC_WORLD */
		PlanetChance::Always, /* AURIC_WORLD */

		PlanetChance::Always, /* FLUORESCENT_WORLD */
		PlanetChance::Always, /* ULTRAVIOLET_WORLD */
		PlanetChance::Always, /* PLUTONIC_WORLD */
		PlanetChance::Never,  /* RAINBOW_WORLD */
		PlanetChance::Never,  /* SHATTERED_WORLD */
		PlanetChance::Always, /* SAPPHIRE_WORLD */
		PlanetChance::Always, /* ORGANIC_WORLD */
		PlanetChance::Always, /* XENOLITHIC_WORLD */
		PlanetChance::Always, /* REDUX_WORLD */
		PlanetChance::Always, /* PRIMORDIAL_WORLD */
		PlanetChance::Never,  /* EMERALD_WORLD */
		PlanetChance::Always, /* CHLORINE_WORLD */
		PlanetChance::Always, /* MAGNETIC_WORLD */
		PlanetChance::Always, /* WATER_WORLD */
		PlanetChance::Always, /* TELLURIC_WORLD */
		PlanetChance::Never,  /* HYDROCARBON_WORLD */
		PlanetChance::Never,  /* IODINE_WORLD */
		PlanetChance::Never,  /* VINYLOGOUS_WORLD */
		PlanetChance::Never,  /* RUBY_WORLD */
		PlanetChance::Never,  /* MAGMA_WORLD */
		PlanetChance::Never,  /* MAROON_WORLD */
		//PLANET_NEVER,  /* DIAMOND_WORLD */

		PlanetChance::Always, /* BLU_GAS_GIANT */
		PlanetChance::Always, /* CYA_GAS_GIANT */
		PlanetChance::Always, /* GRN_GAS_GIANT */
		PlanetChance::Always, /* GRY_GAS_GIANT */
		PlanetChance::Always, /* ORA_GAS_GIANT */
		PlanetChance::Always, /* PUR_GAS_GIANT */
		PlanetChance::Always, /* RED_GAS_GIANT */
		PlanetChance::Always, /* VIO_GAS_GIANT */
		PlanetChance::Always, /* YEL_GAS_GIANT */
	};

	return *(PlanetDistribution[which_world]);
}

static uint8_t GreenDistribution(uint8_t which_world)
{
	const uqstl::array<PlanetChance, *PlanetType::NumberOfPlanetTypes> PlanetDistribution {
		PlanetChance::Never,  /* OOLITE_WORLD */
		PlanetChance::Never,  /* YTTRIC_WORLD */
		PlanetChance::Always, /* QUASI_DEGENERATE_WORLD */
		PlanetChance::Always, /* LANTHANIDE_WORLD */
		PlanetChance::Always, /* TREASURE_WORLD */
		PlanetChance::Always, /* UREA_WORLD */
		PlanetChance::Always, /* METAL_WORLD */
		PlanetChance::Always, /* RADIOACTIVE_WORLD */
		PlanetChance::Always, /* OPALESCENT_WORLD */
		PlanetChance::Always, /* CYANIC_WORLD */
		PlanetChance::Always, /* ACID_WORLD */
		PlanetChance::Always, /* ALKALI_WORLD */
		PlanetChance::Always, /* HALIDE_WORLD */
		PlanetChance::Always, /* GREEN_WORLD */
		PlanetChance::Always, /* COPPER_WORLD */
		PlanetChance::Always, /* CARBIDE_WORLD */
		PlanetChance::Always, /* ULTRAMARINE_WORLD */
		PlanetChance::Always, /* NOBLE_WORLD */
		PlanetChance::Always, /* AZURE_WORLD */
		PlanetChance::Always, /* CHONDRITE_WORLD */
		PlanetChance::Always, /* PURPLE_WORLD */
		PlanetChance::Always, /* SUPER_DENSE_WORLD */
		PlanetChance::Always, /* PELLUCID_WORLD */
		PlanetChance::Never,  /* DUST_WORLD */
		PlanetChance::Never,  /* CRIMSON_WORLD */
		PlanetChance::Never,  /* CIMMERIAN_WORLD */
		PlanetChance::Never,  /* INFRARED_WORLD */
		PlanetChance::Always, /* SELENIC_WORLD */
		PlanetChance::Always, /* AURIC_WORLD */

		PlanetChance::Always, /* FLUORESCENT_WORLD */
		PlanetChance::Always, /* ULTRAVIOLET_WORLD */
		PlanetChance::Always, /* PLUTONIC_WORLD */
		PlanetChance::Never,  /* RAINBOW_WORLD */
		PlanetChance::Never,  /* SHATTERED_WORLD */
		PlanetChance::Never,  /* SAPPHIRE_WORLD */
		PlanetChance::Always, /* ORGANIC_WORLD */
		PlanetChance::Always, /* XENOLITHIC_WORLD */
		PlanetChance::Always, /* REDUX_WORLD */
		PlanetChance::Always, /* PRIMORDIAL_WORLD */
		PlanetChance::Always, /* EMERALD_WORLD */
		PlanetChance::Always, /* CHLORINE_WORLD */
		PlanetChance::Always, /* MAGNETIC_WORLD */
		PlanetChance::Always, /* WATER_WORLD */
		PlanetChance::Always, /* TELLURIC_WORLD */
		PlanetChance::Always, /* HYDROCARBON_WORLD */
		PlanetChance::Always, /* IODINE_WORLD */
		PlanetChance::Never,  /* VINYLOGOUS_WORLD */
		PlanetChance::Never,  /* RUBY_WORLD */
		PlanetChance::Never,  /* MAGMA_WORLD */
		PlanetChance::Never,  /* MAROON_WORLD */
		//PLANET_NEVER,  /* DIAMOND_WORLD */

		PlanetChance::Always, /* BLU_GAS_GIANT */
		PlanetChance::Always, /* CYA_GAS_GIANT */
		PlanetChance::Always, /* GRN_GAS_GIANT */
		PlanetChance::Always, /* GRY_GAS_GIANT */
		PlanetChance::Always, /* ORA_GAS_GIANT */
		PlanetChance::Always, /* PUR_GAS_GIANT */
		PlanetChance::Always, /* RED_GAS_GIANT */
		PlanetChance::Always, /* VIO_GAS_GIANT */
		PlanetChance::Always, /* YEL_GAS_GIANT */
	};

	return *(PlanetDistribution[which_world]);
}

static uint8_t OrangeDistribution(uint8_t which_world)
{
	const uqstl::array<PlanetChance, *PlanetType::NumberOfPlanetTypes> PlanetDistribution {
		PlanetChance::Never,  /* OOLITE_WORLD */
		PlanetChance::Never,  /* YTTRIC_WORLD */
		PlanetChance::Never,  /* QUASI_DEGENERATE_WORLD */
		PlanetChance::Never,  /* LANTHANIDE_WORLD */
		PlanetChance::Never,  /* TREASURE_WORLD */
		PlanetChance::Always, /* UREA_WORLD */
		PlanetChance::Never,  /* METAL_WORLD */
		PlanetChance::Always, /* RADIOACTIVE_WORLD */
		PlanetChance::Never,  /* OPALESCENT_WORLD */
		PlanetChance::Never,  /* CYANIC_WORLD */
		PlanetChance::Always, /* ACID_WORLD */
		PlanetChance::Always, /* ALKALI_WORLD */
		PlanetChance::Always, /* HALIDE_WORLD */
		PlanetChance::Always, /* GREEN_WORLD */
		PlanetChance::Always, /* COPPER_WORLD */
		PlanetChance::Always, /* CARBIDE_WORLD */
		PlanetChance::Always, /* ULTRAMARINE_WORLD */
		PlanetChance::Always, /* NOBLE_WORLD */
		PlanetChance::Always, /* AZURE_WORLD */
		PlanetChance::Always, /* CHONDRITE_WORLD */
		PlanetChance::Always, /* PURPLE_WORLD */
		PlanetChance::Always, /* SUPER_DENSE_WORLD */
		PlanetChance::Always, /* PELLUCID_WORLD */
		PlanetChance::Always, /* DUST_WORLD */
		PlanetChance::Always, /* CRIMSON_WORLD */
		PlanetChance::Always, /* CIMMERIAN_WORLD */
		PlanetChance::Always, /* INFRARED_WORLD */
		PlanetChance::Always, /* SELENIC_WORLD */
		PlanetChance::Never,  /* AURIC_WORLD */

		PlanetChance::Never,  /* FLUORESCENT_WORLD */
		PlanetChance::Never,  /* ULTRAVIOLET_WORLD */
		PlanetChance::Never,  /* PLUTONIC_WORLD */
		PlanetChance::Never,  /* RAINBOW_WORLD */
		PlanetChance::Never,  /* SHATTERED_WORLD */
		PlanetChance::Never,  /* SAPPHIRE_WORLD */
		PlanetChance::Never,  /* ORGANIC_WORLD */
		PlanetChance::Never,  /* XENOLITHIC_WORLD */
		PlanetChance::Never,  /* REDUX_WORLD */
		PlanetChance::Never,  /* PRIMORDIAL_WORLD */
		PlanetChance::Never,  /* EMERALD_WORLD */
		PlanetChance::Always, /* CHLORINE_WORLD */
		PlanetChance::Always, /* MAGNETIC_WORLD */
		PlanetChance::Always, /* WATER_WORLD */
		PlanetChance::Always, /* TELLURIC_WORLD */
		PlanetChance::Always, /* HYDROCARBON_WORLD */
		PlanetChance::Always, /* IODINE_WORLD */
		PlanetChance::Always, /* VINYLOGOUS_WORLD */
		PlanetChance::Never,  /* RUBY_WORLD */
		PlanetChance::Always, /* MAGMA_WORLD */
		PlanetChance::Always, /* MAROON_WORLD */
		//PLANET_ALWAYS, /* DIAMOND_WORLD */

		PlanetChance::Always, /* BLU_GAS_GIANT */
		PlanetChance::Always, /* CYA_GAS_GIANT */
		PlanetChance::Always, /* GRN_GAS_GIANT */
		PlanetChance::Always, /* GRY_GAS_GIANT */
		PlanetChance::Always, /* ORA_GAS_GIANT */
		PlanetChance::Always, /* PUR_GAS_GIANT */
		PlanetChance::Always, /* RED_GAS_GIANT */
		PlanetChance::Always, /* VIO_GAS_GIANT */
		PlanetChance::Always, /* YEL_GAS_GIANT */
	};

	return *(PlanetDistribution[which_world]);
}

static uint8_t RedDistribution(uint8_t which_world)
{
	const uqstl::array<PlanetChance, *PlanetType::NumberOfPlanetTypes> PlanetDistribution {
		PlanetChance::Never,  /* OOLITE_WORLD */
		PlanetChance::Never,  /* YTTRIC_WORLD */
		PlanetChance::Never,  /* QUASI_DEGENERATE_WORLD */
		PlanetChance::Never,  /* LANTHANIDE_WORLD */
		PlanetChance::Never,  /* TREASURE_WORLD */
		PlanetChance::Always, /* UREA_WORLD */
		PlanetChance::Always, /* METAL_WORLD */
		PlanetChance::Never,  /* RADIOACTIVE_WORLD */
		PlanetChance::Never,  /* OPALESCENT_WORLD */
		PlanetChance::Never,  /* CYANIC_WORLD */
		PlanetChance::Never,  /* ACID_WORLD */
		PlanetChance::Never,  /* ALKALI_WORLD */
		PlanetChance::Never,  /* HALIDE_WORLD */
		PlanetChance::Never,  /* GREEN_WORLD */
		PlanetChance::Never,  /* COPPER_WORLD */
		PlanetChance::Never,  /* CARBIDE_WORLD */
		PlanetChance::Always, /* ULTRAMARINE_WORLD */
		PlanetChance::Always, /* NOBLE_WORLD */
		PlanetChance::Always, /* AZURE_WORLD */
		PlanetChance::Always, /* CHONDRITE_WORLD */
		PlanetChance::Always, /* PURPLE_WORLD */
		PlanetChance::Always, /* SUPER_DENSE_WORLD */
		PlanetChance::Always, /* PELLUCID_WORLD */
		PlanetChance::Always, /* DUST_WORLD */
		PlanetChance::Always, /* CRIMSON_WORLD */
		PlanetChance::Always, /* CIMMERIAN_WORLD */
		PlanetChance::Always, /* INFRARED_WORLD */
		PlanetChance::Always, /* SELENIC_WORLD */
		PlanetChance::Never,  /* AURIC_WORLD */

		PlanetChance::Never,  /* FLUORESCENT_WORLD */
		PlanetChance::Never,  /* ULTRAVIOLET_WORLD */
		PlanetChance::Never,  /* PLUTONIC_WORLD */
		PlanetChance::Never,  /* RAINBOW_WORLD */
		PlanetChance::Never,  /* SHATTERED_WORLD */
		PlanetChance::Never,  /* SAPPHIRE_WORLD */
		PlanetChance::Never,  /* ORGANIC_WORLD */
		PlanetChance::Never,  /* XENOLITHIC_WORLD */
		PlanetChance::Never,  /* REDUX_WORLD */
		PlanetChance::Never,  /* PRIMORDIAL_WORLD */
		PlanetChance::Never,  /* EMERALD_WORLD */
		PlanetChance::Never,  /* CHLORINE_WORLD */
		PlanetChance::Always, /* MAGNETIC_WORLD */
		PlanetChance::Always, /* WATER_WORLD */
		PlanetChance::Always, /* TELLURIC_WORLD */
		PlanetChance::Always, /* HYDROCARBON_WORLD */
		PlanetChance::Always, /* IODINE_WORLD */
		PlanetChance::Always, /* VINYLOGOUS_WORLD */
		PlanetChance::Always, /* RUBY_WORLD */
		PlanetChance::Always, /* MAGMA_WORLD */
		PlanetChance::Always, /* MAROON_WORLD */
		//PLANET_ALWAYS, /* DIAMOND_WORLD */

		PlanetChance::Always, /* BLU_GAS_GIANT */
		PlanetChance::Always, /* CYA_GAS_GIANT */
		PlanetChance::Always, /* GRN_GAS_GIANT */
		PlanetChance::Always, /* GRY_GAS_GIANT */
		PlanetChance::Always, /* ORA_GAS_GIANT */
		PlanetChance::Always, /* PUR_GAS_GIANT */
		PlanetChance::Always, /* RED_GAS_GIANT */
		PlanetChance::Always, /* VIO_GAS_GIANT */
		PlanetChance::Always, /* YEL_GAS_GIANT */
	};

	return *(PlanetDistribution[which_world]);
}

static uint8_t WhiteDistribution(uint8_t which_world)
{
	const uqstl::array<PlanetChance, *PlanetType::NumberOfPlanetTypes> PlanetDistribution {
		PlanetChance::Always, /* OOLITE_WORLD */
		PlanetChance::Always, /* YTTRIC_WORLD */
		PlanetChance::Always, /* QUASI_DEGENERATE_WORLD */
		PlanetChance::Always, /* LANTHANIDE_WORLD */
		PlanetChance::Always, /* TREASURE_WORLD */
		PlanetChance::Always, /* UREA_WORLD */
		PlanetChance::Always, /* METAL_WORLD */
		PlanetChance::Always, /* RADIOACTIVE_WORLD */
		PlanetChance::Always, /* OPALESCENT_WORLD */
		PlanetChance::Always, /* CYANIC_WORLD */
		PlanetChance::Always, /* ACID_WORLD */
		PlanetChance::Always, /* ALKALI_WORLD */
		PlanetChance::Always, /* HALIDE_WORLD */
		PlanetChance::Never,  /* GREEN_WORLD */
		PlanetChance::Never,  /* COPPER_WORLD */
		PlanetChance::Never,  /* CARBIDE_WORLD */
		PlanetChance::Never,  /* ULTRAMARINE_WORLD */
		PlanetChance::Never,  /* NOBLE_WORLD */
		PlanetChance::Never,  /* AZURE_WORLD */
		PlanetChance::Never,  /* CHONDRITE_WORLD */
		PlanetChance::Never,  /* PURPLE_WORLD */
		PlanetChance::Never,  /* SUPER_DENSE_WORLD */
		PlanetChance::Never,  /* PELLUCID_WORLD */
		PlanetChance::Never,  /* DUST_WORLD */
		PlanetChance::Never,  /* CRIMSON_WORLD */
		PlanetChance::Never,  /* CIMMERIAN_WORLD */
		PlanetChance::Never,  /* INFRARED_WORLD */
		PlanetChance::Always, /* SELENIC_WORLD */
		PlanetChance::Always, /* AURIC_WORLD */

		PlanetChance::Always, /* FLUORESCENT_WORLD */
		PlanetChance::Always, /* ULTRAVIOLET_WORLD */
		PlanetChance::Always, /* PLUTONIC_WORLD */
		PlanetChance::Never,  /* RAINBOW_WORLD */
		PlanetChance::Never,  /* SHATTERED_WORLD */
		PlanetChance::Always, /* SAPPHIRE_WORLD */
		PlanetChance::Always, /* ORGANIC_WORLD */
		PlanetChance::Always, /* XENOLITHIC_WORLD */
		PlanetChance::Always, /* REDUX_WORLD */
		PlanetChance::Always, /* PRIMORDIAL_WORLD */
		PlanetChance::Always, /* EMERALD_WORLD */
		PlanetChance::Never,  /* CHLORINE_WORLD */
		PlanetChance::Never,  /* MAGNETIC_WORLD */
		PlanetChance::Never,  /* WATER_WORLD */
		PlanetChance::Never,  /* TELLURIC_WORLD */
		PlanetChance::Never,  /* HYDROCARBON_WORLD */
		PlanetChance::Never,  /* IODINE_WORLD */
		PlanetChance::Always, /* VINYLOGOUS_WORLD */
		PlanetChance::Always, /* RUBY_WORLD */
		PlanetChance::Never,  /* MAGMA_WORLD */
		PlanetChance::Never,  /* MAROON_WORLD */
		//PLANET_NEVER,  /* DIAMOND_WORLD */

		PlanetChance::Always, /* BLU_GAS_GIANT */
		PlanetChance::Always, /* CYA_GAS_GIANT */
		PlanetChance::Always, /* GRN_GAS_GIANT */
		PlanetChance::Always, /* GRY_GAS_GIANT */
		PlanetChance::Always, /* ORA_GAS_GIANT */
		PlanetChance::Always, /* PUR_GAS_GIANT */
		PlanetChance::Always, /* RED_GAS_GIANT */
		PlanetChance::Always, /* VIO_GAS_GIANT */
		PlanetChance::Always, /* YEL_GAS_GIANT */
	};

	return *(PlanetDistribution[which_world]);
}

static uint8_t YellowDistribution(uint8_t which_world)
{
	const uqstl::array<PlanetChance, *PlanetType::NumberOfPlanetTypes> PlanetDistribution {
		PlanetChance::Never,  /* OOLITE_WORLD */
		PlanetChance::Never,  /* YTTRIC_WORLD */
		PlanetChance::Never,  /* QUASI_DEGENERATE_WORLD */
		PlanetChance::Never,  /* LANTHANIDE_WORLD */
		PlanetChance::Always, /* TREASURE_WORLD */
		PlanetChance::Always, /* UREA_WORLD */
		PlanetChance::Always, /* METAL_WORLD */
		PlanetChance::Always, /* RADIOACTIVE_WORLD */
		PlanetChance::Always, /* OPALESCENT_WORLD */
		PlanetChance::Always, /* CYANIC_WORLD */
		PlanetChance::Always, /* ACID_WORLD */
		PlanetChance::Always, /* ALKALI_WORLD */
		PlanetChance::Always, /* HALIDE_WORLD */
		PlanetChance::Always, /* GREEN_WORLD */
		PlanetChance::Always, /* COPPER_WORLD */
		PlanetChance::Always, /* CARBIDE_WORLD */
		PlanetChance::Always, /* ULTRAMARINE_WORLD */
		PlanetChance::Always, /* NOBLE_WORLD */
		PlanetChance::Always, /* AZURE_WORLD */
		PlanetChance::Always, /* CHONDRITE_WORLD */
		PlanetChance::Always, /* PURPLE_WORLD */
		PlanetChance::Always, /* SUPER_DENSE_WORLD */
		PlanetChance::Always, /* PELLUCID_WORLD */
		PlanetChance::Always, /* DUST_WORLD */
		PlanetChance::Always, /* CRIMSON_WORLD */
		PlanetChance::Always, /* CIMMERIAN_WORLD */
		PlanetChance::Always, /* INFRARED_WORLD */
		PlanetChance::Always, /* SELENIC_WORLD */
		PlanetChance::Always, /* AURIC_WORLD */

		PlanetChance::Never,  /* FLUORESCENT_WORLD */
		PlanetChance::Never,  /* ULTRAVIOLET_WORLD */
		PlanetChance::Never,  /* PLUTONIC_WORLD */
		PlanetChance::Never,  /* RAINBOW_WORLD */
		PlanetChance::Never,  /* SHATTERED_WORLD */
		PlanetChance::Never,  /* SAPPHIRE_WORLD */
		PlanetChance::Always, /* ORGANIC_WORLD */
		PlanetChance::Always, /* XENOLITHIC_WORLD */
		PlanetChance::Always, /* REDUX_WORLD */
		PlanetChance::Always, /* PRIMORDIAL_WORLD */
		PlanetChance::Never,  /* EMERALD_WORLD */
		PlanetChance::Always, /* CHLORINE_WORLD */
		PlanetChance::Always, /* MAGNETIC_WORLD */
		PlanetChance::Always, /* WATER_WORLD */
		PlanetChance::Always, /* TELLURIC_WORLD */
		PlanetChance::Always, /* HYDROCARBON_WORLD */
		PlanetChance::Always, /* IODINE_WORLD */
		PlanetChance::Always, /* VINYLOGOUS_WORLD */
		PlanetChance::Never,  /* RUBY_WORLD */
		PlanetChance::Always, /* MAGMA_WORLD */
		PlanetChance::Always, /* MAROON_WORLD */
		//PLANET_ALWAYS, /* DIAMOND_WORLD */

		PlanetChance::Always, /* BLU_GAS_GIANT */
		PlanetChance::Always, /* CYA_GAS_GIANT */
		PlanetChance::Always, /* GRN_GAS_GIANT */
		PlanetChance::Always, /* GRY_GAS_GIANT */
		PlanetChance::Always, /* ORA_GAS_GIANT */
		PlanetChance::Always, /* PUR_GAS_GIANT */
		PlanetChance::Always, /* RED_GAS_GIANT */
		PlanetChance::Always, /* VIO_GAS_GIANT */
		PlanetChance::Always, /* YEL_GAS_GIANT */
	};

	return *(PlanetDistribution[which_world]);
}

static constexpr auto DwarfRockDist {MinPlanetRadius};
static constexpr auto DwarfGasgDist {ScaleRadius(12)};

static constexpr auto GiantRockDist {ScaleRadius(8)};
static constexpr auto GiantGasgDist {ScaleRadius(13)};

static constexpr auto SupergiantRockDist {ScaleRadius(16)};
static constexpr auto SupergiantGasgDist {ScaleRadius(33)};

void ComputeSpeed(SystemPlanet* planet, bool GeneratingMoons,
				  uint16_t rand_val)
{
	//BW : empiric values, which would give roughly correct
	// rotation periods for most moons in the solar system
	if (GeneratingMoons)
	{
		planet->orb_speed =
			FullCircle / (29 * pow((double)planet->radius / (MinMoonRadius() + (MaxGenMoons - 1) * MoonDelta()), 1.5));
		if ((planet->pPrevDesc->data_index & ~*PlanetType::PlanetShielded)
			>= PlanetType::FirstGasGiant)
		{
			planet->orb_speed *= 2;
		}
		if (!(rand_val % 7))
		{
			planet->orb_speed = -planet->orb_speed;
		}
	}
	else
	{
		planet->orb_speed =
			FullCircle / (OneYear * pow((double)planet->radius / EarthRadius, 1.5));
	}
}

void FillOrbits(SolarSystemState* system, uint8_t NumPlanets,
				SystemPlanet* pBaseDesc, bool TypesDefined)
{ /* Generate Planets in orbit around star */
	uint8_t MaxPlanet {};
	uint32_t loopCounter = 0;
	uint8_t orbitDiff {};

	struct
	{
		uint16_t MinRockyDist, MinGasGDist;
	} Suns[] =
		{
			{DwarfRockDist,		DwarfGasgDist	 },
			{GiantRockDist,		GiantGasgDist	 },
			{SupergiantRockDist, SupergiantGasgDist},
	};
#ifdef DEBUG_ORBITS
	char buf[256];
	char stype[] = {'D', 'G', 'S'};
	char scolor[] = {'B', 'G', 'O', 'R', 'W', 'Y'};
#endif /* DEBUG_ORBITS */

	if (NumPlanets == NumplanetsPdesc)
	{ // To shorten the redundancy of the function in repeated areas.
		NumPlanets = system->SunDesc[0].NumPlanets;
		pBaseDesc = system->PlanetDesc.data();
	}

	SystemPlanet* pPD {pBaseDesc};
	const uint16_t StarSize {system->SunDesc[0].data_index};
	const StarBodyColor starColor {StarColor(CurStarDescPtr()->Type)};

	if (NumPlanets == (uint8_t)~0)
	{
		// XXX: This is pretty funny. Instead of calling RNG once, like so:
		//     1 + Random % MAX_GENERATED_PLANETS
		//   we spin in a loop until the result > 0.
		//   Note that this behavior must be kept to preserve the universe.
		do
		{
			NumPlanets = uqm::lowWord(SysGenRNG->next())
					   % (MaxGenPlanets + 1);
		} while (NumPlanets == 0);
		system->SunDesc[0].NumPlanets = NumPlanets;
	}

#ifdef DEBUG_ORBITS
	GetClusterName(CurStarDescPtr(), buf);
	uqm::log::debug("cluster name = {}  color = %c type = %c", buf,
					scolor[StarColor(CurStarDescPtr()->Type)],
					stype[StarType(CurStarDescPtr()->Type)]);
#endif /* DEBUG_ORBITS */
	const bool GeneratingMoons {(bool)(pBaseDesc == system->MoonDesc.data())};
	const bool GasGiant = pPD && pPD->pPrevDesc && (pPD->pPrevDesc->data_index & ~*PlanetType::PlanetShielded) >= PlanetType::FirstGasGiant;
	if (GeneratingMoons)
	{
		MaxPlanet = *(uqm::isOriginalSeed() || (uqm::isStarSeed() && !GasGiant) ? PlanetType::FirstLargeRocky : (uqm::isStarSeed() ? PlanetType::FirstGasGiant : PlanetType::LastLargeRocky));
	}
	else
	{
		MaxPlanet = *PlanetType::NumberOfPlanetTypes;
	}
	const uint8_t PlanetCount {NumPlanets};
	while (NumPlanets--)
	{
		uint8_t chance {};
		uint32_t rand_val {};
		uint32_t min_radius {};
		int16_t delta_r {};
		const SystemPlanet* pLocPD {};

		do
		{
			rand_val = SysGenRNG->next();
			if (TypesDefined)
			{
				rand_val = 0;
			}
			else
			{
				pPD->data_index =
					(uint8_t)(uqm::highByte(uqm::lowWord(rand_val)) % MaxPlanet);
			}


			chance = *PlanetChance::Never;
			switch (starColor)
			{
				case StarBodyColor::Blue:
					chance = BlueDistribution(pPD->data_index);
					break;
				case StarBodyColor::Green:
					chance = GreenDistribution(pPD->data_index);
					break;
				case StarBodyColor::Orange:
					chance = OrangeDistribution(pPD->data_index);
					break;
				case StarBodyColor::Red:
					chance = RedDistribution(pPD->data_index);
					break;
				case StarBodyColor::White:
					chance = WhiteDistribution(pPD->data_index);
					break;
				case StarBodyColor::Yellow:
					chance = YellowDistribution(pPD->data_index);
					break;
				default:
					break;
			}
		} while (uqm::lowByte(uqm::lowWord(rand_val)) >= chance);

		if (pPD->data_index < PlanetType::FirstGasGiant)
		{
			min_radius = Suns[StarSize].MinRockyDist;
		}
		else
		{
			min_radius = Suns[StarSize].MinGasGDist;
		}
RelocatePlanet:
		rand_val = SysGenRNG->next();
		if (GeneratingMoons)
		{
			pPD->radius = MinMoonRadius()
						+ ((uqm::lowWord(rand_val) % MaxGenMoons) * MoonDelta());
			for (pLocPD = pPD - 1; pLocPD >= pBaseDesc; --pLocPD)
			{
				if (pPD->radius == pLocPD->radius)
				{
					goto RelocatePlanet;
				}
			}
			pPD->NumPlanets = 0;
		}
		else
		{
			pPD->radius =
				(uqm::lowWord(rand_val) % (MaxPlanetRadius - min_radius))
				+ min_radius;
			for (pLocPD = pPD - 1; pLocPD >= pBaseDesc; --pLocPD)
			{
				if (loopCounter < UINT16_MAX)
				{
					delta_r = UnscaleRadius(pLocPD->radius) / 5 - UnscaleRadius(pPD->radius) / 5;
					orbitDiff = 1;
				}
				else
				{
					delta_r = UnscaleRadius(pLocPD->radius) - UnscaleRadius(pPD->radius);
					orbitDiff = 5;
				}

				if (delta_r <= 0)
				{
					delta_r = -delta_r;
				}
				if (delta_r <= orbitDiff && loopCounter < (UINT16_MAX * 2))
				{
					loopCounter++;
					goto RelocatePlanet;
				}
			}
		}


		rand_val = SysGenRNG->next();
		// Initial angle & coordinates as in Vanilla UQM
		// Still used to compute rand_seed and the position
		// of the planet at the start of the game
		pPD->angle = NormalizeAngle(uqm::lowWord(rand_val));
		pPD->location.x = Cosine(pPD->angle, pPD->radius);
		pPD->location.y = Sine(pPD->angle, pPD->radius);
		if (GeneratingMoons)
		{
			pPD->rand_seed = uqm::makeDword(
				Cosine(pPD->angle, ResDescale(pPD->radius)),
				Sine(pPD->angle, ResDescale(pPD->radius)));
		}
		else
		{
			pPD->rand_seed = uqm::makeDword(pPD->location.x, pPD->location.y);
		}
		// Angle is kept for reference but location will be adjusted
		// to take orbiting into account
		ComputeSpeed(pPD, GeneratingMoons, uqm::highWord(rand_val));

		++pPD;
	}

	{
		for (uint8_t i {0}; i < PlanetCount; ++i)
		{
			for (uint8_t j {(uint8_t)(PlanetCount - 1)}; j > i; --j)
			{
				if (pBaseDesc[i].radius > pBaseDesc[j].radius)
				{
					SystemPlanet temp;

					temp = pBaseDesc[i];
					pBaseDesc[i] = pBaseDesc[j];
					pBaseDesc[j] = temp;
				}
			}
		}
	}
}

uint8_t PickClosestHabitable(SolarSystemState* solarSys)
{
	const uqstl::array<uqstl::array<int16_t, 2>, *StarBodyColor::Count> hRangesD {
		{
			{860, 1790},  // blue
			{540, 1150},  // green
			{140, 280},	  // orange
			{0, 0},		  // red
			{1240, 2560}, // white
			{320, 640}	  // yellow
		}
	 };
	const uqstl::array<uqstl::array<int16_t, 2>, *StarBodyColor::Count> hRangesG {
		{
			{0, 0},		  // blue
			{0, 0},		  // green
			{3410, 7120}, // orange
			{860, 1800},  // red
			{0, 0},		  // white
			{7630, 7936}  // yellow
		}
	 };
	uint8_t i {};
	int16_t hRangeMin = 0;
	int16_t hRangeMax = 0;
	int16_t hRangeMed = 0;
	uint8_t pByte = 0;
	SystemPlanet* pPlanet {};
	const uint32_t rand = SysGenRNG->seed();

	uint8_t numPlanets {solarSys->SunDesc[0].NumPlanets};
	SystemPlanet* pPD {solarSys->PlanetDesc.data()};

	const StarBodyColor starColor {StarColor(CurStarDescPtr()->Type)};
	const StellarSize starType {StarType(CurStarDescPtr()->Type)};

	if (starType == StellarSize::SuperGiant)
	{
		return numPlanets--;
	}

	if (starType == StellarSize::Dwarf)
	{
		if (starColor == StarBodyColor::Red)
		{
			return 0;
		}

		hRangeMin = hRangesD[*starColor][0];
		hRangeMax = hRangesD[*starColor][1];
	}

	if (starType == StellarSize::Giant)
	{
		if (starColor == StarBodyColor::Blue || starColor == StarBodyColor::Green
			|| starColor == StarBodyColor::White)
		{

			pPlanet = &pPD[numPlanets--];

			pPlanet->radius = MaxPlanetRadius;
			pPlanet->angle = NormalizeAngle(uqm::lowWord(rand));
			pPlanet->location.x = Cosine(pPlanet->angle, pPlanet->radius);
			pPlanet->location.y = Sine(pPlanet->angle, pPlanet->radius);
			ComputeSpeed(pPlanet, false, uqm::highWord(rand));

			return numPlanets--;
		}

		hRangeMin = hRangesG[*starColor][0];
		hRangeMax = hRangesG[*starColor][1];
	}

	hRangeMed = (hRangeMin + hRangeMax) / 2;

	if (numPlanets > 1)
	{
		int16_t dist = pPD[0].radius;
		for (i = 1; i < numPlanets; i++)
		{
			if (abs(dist - hRangeMed) >= abs(pPD[i].radius - hRangeMed))
			{
				dist = pPD[i].radius;
				pByte = i;
			}
		}
	}
	else
	{
		pByte = 0;
	}

	pPlanet = &pPD[pByte];

	if (pPlanet->radius < hRangeMin || pPlanet->radius > hRangeMax)
	{
		int16_t min {};
		int16_t max {};

		if (pPlanet->radius < hRangeMin)
		{
			min = hRangeMin;
			max = hRangeMed;
		}
		else if (pPlanet->radius > hRangeMax)
		{
			min = hRangeMed;
			max = hRangeMax;
		}

		pPlanet->radius = uqm::rangeMinMax(min, max, rand);

		pPlanet->angle = NormalizeAngle(uqm::lowWord(rand));
		pPlanet->location.x = Cosine(pPlanet->angle, pPlanet->radius);
		pPlanet->location.y = Sine(pPlanet->angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, uqm::highWord(rand));
	}

	return pByte;
}

bool CheckForHabitable(SolarSystemState* solarSys)
{
	const uqstl::array<uqstl::array<int16_t, 2>, *StarBodyColor::Count> HabitableRanges {
		{
			{853, 1790},  // blue
			{544, 1151},  // green
			{139, 287},	  // orange
			{68, 68},	  // red
			{1231, 2569}, // white
			{312, 778}	  // yellow
		}
	 };

	int16_t i {};
	const uint8_t planetByte = solarSys->SunDesc[0].PlanetByte;
	const uint8_t numPlanets = solarSys->SunDesc[0].NumPlanets;
	int16_t newRadius {}; // , radiusDiff;
	uqstl::array<uint32_t, MaxGenPlanets> planetRadii;
	// static int16_t diffCheck, min_radius;

	StarBodyColor starColor {StarColor(CurStarDescPtr()->Type)};
	// Terrible, but efficient, hack to ensure some semblance of sanity.
	// Eventually, we will need further code if we care about habitable homes.
	if (uqm::isStarSeed() && starColor == StarBodyColor::Red)
	{
		starColor = StarBodyColor::Orange;
	}

	SystemPlanet* pPD {solarSys->PlanetDesc.data()};
	const int16_t oldRadius {pPD[planetByte].radius};

	const int16_t habitableRangeMin {HabitableRanges[*starColor][0]};
	int16_t habitableRangeMax {HabitableRanges[*starColor][1]};

	// This ensures the planets are at least a certain distance apart, which
	// mirrors UnscaleRadius logic from FillOrbits (radius / 2^6 / 5)
	if (uqm::isStarSeed() && numPlanets > 1 && pPD[1].radius < habitableRangeMax + 320
		&& pPD[1].radius > habitableRangeMin + 320)
	{
		habitableRangeMax = pPD[1].radius - 320;
	}

	if ((oldRadius >= habitableRangeMin && oldRadius <= habitableRangeMax)
		|| starColor == StarBodyColor::Red || planetByte > 0)
	{
		return false;
	}

	for (i = 0; i < numPlanets; ++i)
	{
		planetRadii[i] = pPD[i].radius;
	}

	const uint32_t rand_val {SysGenRNG->next()};

	newRadius = (uqm::lowWord(rand_val) % (habitableRangeMax - habitableRangeMin) + habitableRangeMin);

	// radiusDiff = abs (oldRadius - newRadius);

	planetRadii[planetByte] = newRadius;

	if ((uqm::isStarSeed() && numPlanets == 1) || planetRadii[planetByte] < planetRadii[planetByte + 1])
	{
		pPD[planetByte].radius = planetRadii[planetByte];
		pPD[planetByte].angle = NormalizeAngle(uqm::lowWord(rand_val));
		pPD[planetByte].location.x =
			Cosine(pPD[planetByte].angle, pPD[planetByte].radius);
		pPD[planetByte].location.y =
			Sine(pPD[planetByte].angle, pPD[planetByte].radius);
		ComputeSpeed(&pPD[planetByte], false, uqm::highWord(rand_val));

		return true;
	}
	else
	{
		return false;
	}

	/*if (planetByte > 0 && newRadius < pPD[planetByte-1].radius)
	{
		uint32_t radialSection = planetRadii[planetByte] - ClosestRadius;
		uint8_t numRockySections = radialSection / DwarfRockDist;
		uint8_t numGassySections = radialSection / DwarfGasgDist;

		printf ("radialSection {}, numRockySections {}, numGassySections {}\n", radialSection, numRockySections, numGassySections);

		for (i = planetByte - 1; i > -1; --i)
		{
			rand_val = SysGenRNG->next();

			planetRadii[i] = (uqm::lowWord(rand_val) % (
				planetRadii[(i + 1)] - ClosestRadius) + ClosestRadius);
		}

		for (i = planetByte - 1; i > -1; --i)
		{
			if (pPD[i].data_index < FIRST_GAS_GIANT)
				min_radius = DwarfRockDist;
			else
				min_radius = DwarfGasgDist;

			if (i > -1)
			{
				if (planetRadii[i] < planetRadii[i - 1])
					planetRadii[i] = planetRadii[i - 1];
			}

			radiusDiff = planetRadii[(i + 1)] - min_radius;

			if (planetRadii[i] > radiusDiff)
				planetRadii[i] = radiusDiff;
		}

		for (i = planetByte - 1; i > -1; --i)
		{
			if (planetRadii[i] > ClosestRadius)
			{
				rand_val = SysGenRNG->next();
				pPD[i].radius = planetRadii[i];
				pPD[i].angle = NormalizeAngle (uqm::lowWord(rand_val));
				pPD[i].location.x =
					Cosine (pPD[i].angle, pPD[i].radius);
				pPD[i].location.y =
					Sine (pPD[i].angle, pPD[i].radius);
				ComputeSpeed (&pPD[i], false, uqm::highWord(rand_val));
			}
		}
	}*/

	//{	// Renumber all the planets
	//	uint8_t i;

	//	for (i = 0; i < numPlanets; ++i)
	//	{
	//		uint8_t j;

	//		for (j = (uint8_t)(numPlanets - 1); j > i; --j)
	//		{
	//			if (solarSys->PlanetDesc[i].radius > solarSys->PlanetDesc[j].radius)
	//			{
	//				SystemPlanet temp;

	//				temp = solarSys->PlanetDesc[i];
	//				solarSys->PlanetDesc[i] = solarSys->PlanetDesc[j];
	//				solarSys->PlanetDesc[j] = temp;
	//			}
	//		}
	//	}
	//}
}

} // namespace uqm
