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

#include "planets.h"
#include "../starmap.h"
#include "core/types/bytes.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
#include "../clock.h"
#include <math.h>
#include <stdlib.h>

//#define DEBUG_ORBITS

enum
{
	PLANET_NEVER = 0,
	PLANET_RARE = 15,
	PLANET_FEW = 63,
	PLANET_COMMON = 127,
	PLANET_ALWAYS = 255
};

static uint8_t
BlueDistribution(uint8_t which_world)
{
	const uint8_t PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
		{
			PLANET_ALWAYS, /* OOLITE_WORLD */
			PLANET_ALWAYS, /* YTTRIC_WORLD */
			PLANET_ALWAYS, /* QUASI_DEGENERATE_WORLD */
			PLANET_ALWAYS, /* LANTHANIDE_WORLD */
			PLANET_ALWAYS, /* TREASURE_WORLD */
			PLANET_ALWAYS, /* UREA_WORLD */
			PLANET_ALWAYS, /* METAL_WORLD */
			PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
			PLANET_ALWAYS, /* OPALESCENT_WORLD */
			PLANET_ALWAYS, /* CYANIC_WORLD */
			PLANET_ALWAYS, /* ACID_WORLD */
			PLANET_ALWAYS, /* ALKALI_WORLD */
			PLANET_ALWAYS, /* HALIDE_WORLD */
			PLANET_ALWAYS, /* GREEN_WORLD */
			PLANET_ALWAYS, /* COPPER_WORLD */
			PLANET_ALWAYS, /* CARBIDE_WORLD */
			PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
			PLANET_ALWAYS, /* NOBLE_WORLD */
			PLANET_ALWAYS, /* AZURE_WORLD */
			PLANET_NEVER,  /* CHONDRITE_WORLD */
			PLANET_NEVER,  /* PURPLE_WORLD */
			PLANET_NEVER,  /* SUPER_DENSE_WORLD */
			PLANET_NEVER,  /* PELLUCID_WORLD */
			PLANET_NEVER,  /* DUST_WORLD */
			PLANET_NEVER,  /* CRIMSON_WORLD */
			PLANET_NEVER,  /* CIMMERIAN_WORLD */
			PLANET_NEVER,  /* INFRARED_WORLD */
			PLANET_ALWAYS, /* SELENIC_WORLD */
			PLANET_ALWAYS, /* AURIC_WORLD */

			PLANET_ALWAYS, /* FLUORESCENT_WORLD */
			PLANET_ALWAYS, /* ULTRAVIOLET_WORLD */
			PLANET_ALWAYS, /* PLUTONIC_WORLD */
			PLANET_NEVER,  /* RAINBOW_WORLD */
			PLANET_NEVER,  /* SHATTERED_WORLD */
			PLANET_ALWAYS, /* SAPPHIRE_WORLD */
			PLANET_ALWAYS, /* ORGANIC_WORLD */
			PLANET_ALWAYS, /* XENOLITHIC_WORLD */
			PLANET_ALWAYS, /* REDUX_WORLD */
			PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
			PLANET_NEVER,  /* EMERALD_WORLD */
			PLANET_ALWAYS, /* CHLORINE_WORLD */
			PLANET_ALWAYS, /* MAGNETIC_WORLD */
			PLANET_ALWAYS, /* WATER_WORLD */
			PLANET_ALWAYS, /* TELLURIC_WORLD */
			PLANET_NEVER,  /* HYDROCARBON_WORLD */
			PLANET_NEVER,  /* IODINE_WORLD */
			PLANET_NEVER,  /* VINYLOGOUS_WORLD */
			PLANET_NEVER,  /* RUBY_WORLD */
			PLANET_NEVER,  /* MAGMA_WORLD */
			PLANET_NEVER,  /* MAROON_WORLD */
			//PLANET_NEVER,  /* DIAMOND_WORLD */

			PLANET_ALWAYS, /* BLU_GAS_GIANT */
			PLANET_ALWAYS, /* CYA_GAS_GIANT */
			PLANET_ALWAYS, /* GRN_GAS_GIANT */
			PLANET_ALWAYS, /* GRY_GAS_GIANT */
			PLANET_ALWAYS, /* ORA_GAS_GIANT */
			PLANET_ALWAYS, /* PUR_GAS_GIANT */
			PLANET_ALWAYS, /* RED_GAS_GIANT */
			PLANET_ALWAYS, /* VIO_GAS_GIANT */
			PLANET_ALWAYS, /* YEL_GAS_GIANT */
		};

	return (PlanetDistribution[which_world]);
}

static uint8_t
GreenDistribution(uint8_t which_world)
{
	const uint8_t PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
		{
			PLANET_NEVER,  /* OOLITE_WORLD */
			PLANET_NEVER,  /* YTTRIC_WORLD */
			PLANET_ALWAYS, /* QUASI_DEGENERATE_WORLD */
			PLANET_ALWAYS, /* LANTHANIDE_WORLD */
			PLANET_ALWAYS, /* TREASURE_WORLD */
			PLANET_ALWAYS, /* UREA_WORLD */
			PLANET_ALWAYS, /* METAL_WORLD */
			PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
			PLANET_ALWAYS, /* OPALESCENT_WORLD */
			PLANET_ALWAYS, /* CYANIC_WORLD */
			PLANET_ALWAYS, /* ACID_WORLD */
			PLANET_ALWAYS, /* ALKALI_WORLD */
			PLANET_ALWAYS, /* HALIDE_WORLD */
			PLANET_ALWAYS, /* GREEN_WORLD */
			PLANET_ALWAYS, /* COPPER_WORLD */
			PLANET_ALWAYS, /* CARBIDE_WORLD */
			PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
			PLANET_ALWAYS, /* NOBLE_WORLD */
			PLANET_ALWAYS, /* AZURE_WORLD */
			PLANET_ALWAYS, /* CHONDRITE_WORLD */
			PLANET_ALWAYS, /* PURPLE_WORLD */
			PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
			PLANET_ALWAYS, /* PELLUCID_WORLD */
			PLANET_NEVER,  /* DUST_WORLD */
			PLANET_NEVER,  /* CRIMSON_WORLD */
			PLANET_NEVER,  /* CIMMERIAN_WORLD */
			PLANET_NEVER,  /* INFRARED_WORLD */
			PLANET_ALWAYS, /* SELENIC_WORLD */
			PLANET_ALWAYS, /* AURIC_WORLD */

			PLANET_ALWAYS, /* FLUORESCENT_WORLD */
			PLANET_ALWAYS, /* ULTRAVIOLET_WORLD */
			PLANET_ALWAYS, /* PLUTONIC_WORLD */
			PLANET_NEVER,  /* RAINBOW_WORLD */
			PLANET_NEVER,  /* SHATTERED_WORLD */
			PLANET_NEVER,  /* SAPPHIRE_WORLD */
			PLANET_ALWAYS, /* ORGANIC_WORLD */
			PLANET_ALWAYS, /* XENOLITHIC_WORLD */
			PLANET_ALWAYS, /* REDUX_WORLD */
			PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
			PLANET_ALWAYS, /* EMERALD_WORLD */
			PLANET_ALWAYS, /* CHLORINE_WORLD */
			PLANET_ALWAYS, /* MAGNETIC_WORLD */
			PLANET_ALWAYS, /* WATER_WORLD */
			PLANET_ALWAYS, /* TELLURIC_WORLD */
			PLANET_ALWAYS, /* HYDROCARBON_WORLD */
			PLANET_ALWAYS, /* IODINE_WORLD */
			PLANET_NEVER,  /* VINYLOGOUS_WORLD */
			PLANET_NEVER,  /* RUBY_WORLD */
			PLANET_NEVER,  /* MAGMA_WORLD */
			PLANET_NEVER,  /* MAROON_WORLD */
			//PLANET_NEVER,  /* DIAMOND_WORLD */

			PLANET_ALWAYS, /* BLU_GAS_GIANT */
			PLANET_ALWAYS, /* CYA_GAS_GIANT */
			PLANET_ALWAYS, /* GRN_GAS_GIANT */
			PLANET_ALWAYS, /* GRY_GAS_GIANT */
			PLANET_ALWAYS, /* ORA_GAS_GIANT */
			PLANET_ALWAYS, /* PUR_GAS_GIANT */
			PLANET_ALWAYS, /* RED_GAS_GIANT */
			PLANET_ALWAYS, /* VIO_GAS_GIANT */
			PLANET_ALWAYS, /* YEL_GAS_GIANT */
		};

	return (PlanetDistribution[which_world]);
}

static uint8_t
OrangeDistribution(uint8_t which_world)
{
	const uint8_t PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
		{
			PLANET_NEVER,  /* OOLITE_WORLD */
			PLANET_NEVER,  /* YTTRIC_WORLD */
			PLANET_NEVER,  /* QUASI_DEGENERATE_WORLD */
			PLANET_NEVER,  /* LANTHANIDE_WORLD */
			PLANET_NEVER,  /* TREASURE_WORLD */
			PLANET_ALWAYS, /* UREA_WORLD */
			PLANET_NEVER,  /* METAL_WORLD */
			PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
			PLANET_NEVER,  /* OPALESCENT_WORLD */
			PLANET_NEVER,  /* CYANIC_WORLD */
			PLANET_ALWAYS, /* ACID_WORLD */
			PLANET_ALWAYS, /* ALKALI_WORLD */
			PLANET_ALWAYS, /* HALIDE_WORLD */
			PLANET_ALWAYS, /* GREEN_WORLD */
			PLANET_ALWAYS, /* COPPER_WORLD */
			PLANET_ALWAYS, /* CARBIDE_WORLD */
			PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
			PLANET_ALWAYS, /* NOBLE_WORLD */
			PLANET_ALWAYS, /* AZURE_WORLD */
			PLANET_ALWAYS, /* CHONDRITE_WORLD */
			PLANET_ALWAYS, /* PURPLE_WORLD */
			PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
			PLANET_ALWAYS, /* PELLUCID_WORLD */
			PLANET_ALWAYS, /* DUST_WORLD */
			PLANET_ALWAYS, /* CRIMSON_WORLD */
			PLANET_ALWAYS, /* CIMMERIAN_WORLD */
			PLANET_ALWAYS, /* INFRARED_WORLD */
			PLANET_ALWAYS, /* SELENIC_WORLD */
			PLANET_NEVER,  /* AURIC_WORLD */

			PLANET_NEVER,  /* FLUORESCENT_WORLD */
			PLANET_NEVER,  /* ULTRAVIOLET_WORLD */
			PLANET_NEVER,  /* PLUTONIC_WORLD */
			PLANET_NEVER,  /* RAINBOW_WORLD */
			PLANET_NEVER,  /* SHATTERED_WORLD */
			PLANET_NEVER,  /* SAPPHIRE_WORLD */
			PLANET_NEVER,  /* ORGANIC_WORLD */
			PLANET_NEVER,  /* XENOLITHIC_WORLD */
			PLANET_NEVER,  /* REDUX_WORLD */
			PLANET_NEVER,  /* PRIMORDIAL_WORLD */
			PLANET_NEVER,  /* EMERALD_WORLD */
			PLANET_ALWAYS, /* CHLORINE_WORLD */
			PLANET_ALWAYS, /* MAGNETIC_WORLD */
			PLANET_ALWAYS, /* WATER_WORLD */
			PLANET_ALWAYS, /* TELLURIC_WORLD */
			PLANET_ALWAYS, /* HYDROCARBON_WORLD */
			PLANET_ALWAYS, /* IODINE_WORLD */
			PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
			PLANET_NEVER,  /* RUBY_WORLD */
			PLANET_ALWAYS, /* MAGMA_WORLD */
			PLANET_ALWAYS, /* MAROON_WORLD */
			//PLANET_ALWAYS, /* DIAMOND_WORLD */

			PLANET_ALWAYS, /* BLU_GAS_GIANT */
			PLANET_ALWAYS, /* CYA_GAS_GIANT */
			PLANET_ALWAYS, /* GRN_GAS_GIANT */
			PLANET_ALWAYS, /* GRY_GAS_GIANT */
			PLANET_ALWAYS, /* ORA_GAS_GIANT */
			PLANET_ALWAYS, /* PUR_GAS_GIANT */
			PLANET_ALWAYS, /* RED_GAS_GIANT */
			PLANET_ALWAYS, /* VIO_GAS_GIANT */
			PLANET_ALWAYS, /* YEL_GAS_GIANT */
		};

	return (PlanetDistribution[which_world]);
}

static uint8_t
RedDistribution(uint8_t which_world)
{
	const uint8_t PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
		{
			PLANET_NEVER,  /* OOLITE_WORLD */
			PLANET_NEVER,  /* YTTRIC_WORLD */
			PLANET_NEVER,  /* QUASI_DEGENERATE_WORLD */
			PLANET_NEVER,  /* LANTHANIDE_WORLD */
			PLANET_NEVER,  /* TREASURE_WORLD */
			PLANET_ALWAYS, /* UREA_WORLD */
			PLANET_ALWAYS, /* METAL_WORLD */
			PLANET_NEVER,  /* RADIOACTIVE_WORLD */
			PLANET_NEVER,  /* OPALESCENT_WORLD */
			PLANET_NEVER,  /* CYANIC_WORLD */
			PLANET_NEVER,  /* ACID_WORLD */
			PLANET_NEVER,  /* ALKALI_WORLD */
			PLANET_NEVER,  /* HALIDE_WORLD */
			PLANET_NEVER,  /* GREEN_WORLD */
			PLANET_NEVER,  /* COPPER_WORLD */
			PLANET_NEVER,  /* CARBIDE_WORLD */
			PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
			PLANET_ALWAYS, /* NOBLE_WORLD */
			PLANET_ALWAYS, /* AZURE_WORLD */
			PLANET_ALWAYS, /* CHONDRITE_WORLD */
			PLANET_ALWAYS, /* PURPLE_WORLD */
			PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
			PLANET_ALWAYS, /* PELLUCID_WORLD */
			PLANET_ALWAYS, /* DUST_WORLD */
			PLANET_ALWAYS, /* CRIMSON_WORLD */
			PLANET_ALWAYS, /* CIMMERIAN_WORLD */
			PLANET_ALWAYS, /* INFRARED_WORLD */
			PLANET_ALWAYS, /* SELENIC_WORLD */
			PLANET_NEVER,  /* AURIC_WORLD */

			PLANET_NEVER,  /* FLUORESCENT_WORLD */
			PLANET_NEVER,  /* ULTRAVIOLET_WORLD */
			PLANET_NEVER,  /* PLUTONIC_WORLD */
			PLANET_NEVER,  /* RAINBOW_WORLD */
			PLANET_NEVER,  /* SHATTERED_WORLD */
			PLANET_NEVER,  /* SAPPHIRE_WORLD */
			PLANET_NEVER,  /* ORGANIC_WORLD */
			PLANET_NEVER,  /* XENOLITHIC_WORLD */
			PLANET_NEVER,  /* REDUX_WORLD */
			PLANET_NEVER,  /* PRIMORDIAL_WORLD */
			PLANET_NEVER,  /* EMERALD_WORLD */
			PLANET_NEVER,  /* CHLORINE_WORLD */
			PLANET_ALWAYS, /* MAGNETIC_WORLD */
			PLANET_ALWAYS, /* WATER_WORLD */
			PLANET_ALWAYS, /* TELLURIC_WORLD */
			PLANET_ALWAYS, /* HYDROCARBON_WORLD */
			PLANET_ALWAYS, /* IODINE_WORLD */
			PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
			PLANET_ALWAYS, /* RUBY_WORLD */
			PLANET_ALWAYS, /* MAGMA_WORLD */
			PLANET_ALWAYS, /* MAROON_WORLD */
			//PLANET_ALWAYS, /* DIAMOND_WORLD */

			PLANET_ALWAYS, /* BLU_GAS_GIANT */
			PLANET_ALWAYS, /* CYA_GAS_GIANT */
			PLANET_ALWAYS, /* GRN_GAS_GIANT */
			PLANET_ALWAYS, /* GRY_GAS_GIANT */
			PLANET_ALWAYS, /* ORA_GAS_GIANT */
			PLANET_ALWAYS, /* PUR_GAS_GIANT */
			PLANET_ALWAYS, /* RED_GAS_GIANT */
			PLANET_ALWAYS, /* VIO_GAS_GIANT */
			PLANET_ALWAYS, /* YEL_GAS_GIANT */
		};

	return (PlanetDistribution[which_world]);
}

static uint8_t
WhiteDistribution(uint8_t which_world)
{
	const uint8_t PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
		{
			PLANET_ALWAYS, /* OOLITE_WORLD */
			PLANET_ALWAYS, /* YTTRIC_WORLD */
			PLANET_ALWAYS, /* QUASI_DEGENERATE_WORLD */
			PLANET_ALWAYS, /* LANTHANIDE_WORLD */
			PLANET_ALWAYS, /* TREASURE_WORLD */
			PLANET_ALWAYS, /* UREA_WORLD */
			PLANET_ALWAYS, /* METAL_WORLD */
			PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
			PLANET_ALWAYS, /* OPALESCENT_WORLD */
			PLANET_ALWAYS, /* CYANIC_WORLD */
			PLANET_ALWAYS, /* ACID_WORLD */
			PLANET_ALWAYS, /* ALKALI_WORLD */
			PLANET_ALWAYS, /* HALIDE_WORLD */
			PLANET_NEVER,  /* GREEN_WORLD */
			PLANET_NEVER,  /* COPPER_WORLD */
			PLANET_NEVER,  /* CARBIDE_WORLD */
			PLANET_NEVER,  /* ULTRAMARINE_WORLD */
			PLANET_NEVER,  /* NOBLE_WORLD */
			PLANET_NEVER,  /* AZURE_WORLD */
			PLANET_NEVER,  /* CHONDRITE_WORLD */
			PLANET_NEVER,  /* PURPLE_WORLD */
			PLANET_NEVER,  /* SUPER_DENSE_WORLD */
			PLANET_NEVER,  /* PELLUCID_WORLD */
			PLANET_NEVER,  /* DUST_WORLD */
			PLANET_NEVER,  /* CRIMSON_WORLD */
			PLANET_NEVER,  /* CIMMERIAN_WORLD */
			PLANET_NEVER,  /* INFRARED_WORLD */
			PLANET_ALWAYS, /* SELENIC_WORLD */
			PLANET_ALWAYS, /* AURIC_WORLD */

			PLANET_ALWAYS, /* FLUORESCENT_WORLD */
			PLANET_ALWAYS, /* ULTRAVIOLET_WORLD */
			PLANET_ALWAYS, /* PLUTONIC_WORLD */
			PLANET_NEVER,  /* RAINBOW_WORLD */
			PLANET_NEVER,  /* SHATTERED_WORLD */
			PLANET_ALWAYS, /* SAPPHIRE_WORLD */
			PLANET_ALWAYS, /* ORGANIC_WORLD */
			PLANET_ALWAYS, /* XENOLITHIC_WORLD */
			PLANET_ALWAYS, /* REDUX_WORLD */
			PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
			PLANET_ALWAYS, /* EMERALD_WORLD */
			PLANET_NEVER,  /* CHLORINE_WORLD */
			PLANET_NEVER,  /* MAGNETIC_WORLD */
			PLANET_NEVER,  /* WATER_WORLD */
			PLANET_NEVER,  /* TELLURIC_WORLD */
			PLANET_NEVER,  /* HYDROCARBON_WORLD */
			PLANET_NEVER,  /* IODINE_WORLD */
			PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
			PLANET_ALWAYS, /* RUBY_WORLD */
			PLANET_NEVER,  /* MAGMA_WORLD */
			PLANET_NEVER,  /* MAROON_WORLD */
			//PLANET_NEVER,  /* DIAMOND_WORLD */

			PLANET_ALWAYS, /* BLU_GAS_GIANT */
			PLANET_ALWAYS, /* CYA_GAS_GIANT */
			PLANET_ALWAYS, /* GRN_GAS_GIANT */
			PLANET_ALWAYS, /* GRY_GAS_GIANT */
			PLANET_ALWAYS, /* ORA_GAS_GIANT */
			PLANET_ALWAYS, /* PUR_GAS_GIANT */
			PLANET_ALWAYS, /* RED_GAS_GIANT */
			PLANET_ALWAYS, /* VIO_GAS_GIANT */
			PLANET_ALWAYS, /* YEL_GAS_GIANT */
		};

	return (PlanetDistribution[which_world]);
}

static uint8_t
YellowDistribution(uint8_t which_world)
{
	const uint8_t PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
		{
			PLANET_NEVER,  /* OOLITE_WORLD */
			PLANET_NEVER,  /* YTTRIC_WORLD */
			PLANET_NEVER,  /* QUASI_DEGENERATE_WORLD */
			PLANET_NEVER,  /* LANTHANIDE_WORLD */
			PLANET_ALWAYS, /* TREASURE_WORLD */
			PLANET_ALWAYS, /* UREA_WORLD */
			PLANET_ALWAYS, /* METAL_WORLD */
			PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
			PLANET_ALWAYS, /* OPALESCENT_WORLD */
			PLANET_ALWAYS, /* CYANIC_WORLD */
			PLANET_ALWAYS, /* ACID_WORLD */
			PLANET_ALWAYS, /* ALKALI_WORLD */
			PLANET_ALWAYS, /* HALIDE_WORLD */
			PLANET_ALWAYS, /* GREEN_WORLD */
			PLANET_ALWAYS, /* COPPER_WORLD */
			PLANET_ALWAYS, /* CARBIDE_WORLD */
			PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
			PLANET_ALWAYS, /* NOBLE_WORLD */
			PLANET_ALWAYS, /* AZURE_WORLD */
			PLANET_ALWAYS, /* CHONDRITE_WORLD */
			PLANET_ALWAYS, /* PURPLE_WORLD */
			PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
			PLANET_ALWAYS, /* PELLUCID_WORLD */
			PLANET_ALWAYS, /* DUST_WORLD */
			PLANET_ALWAYS, /* CRIMSON_WORLD */
			PLANET_ALWAYS, /* CIMMERIAN_WORLD */
			PLANET_ALWAYS, /* INFRARED_WORLD */
			PLANET_ALWAYS, /* SELENIC_WORLD */
			PLANET_ALWAYS, /* AURIC_WORLD */

			PLANET_NEVER,  /* FLUORESCENT_WORLD */
			PLANET_NEVER,  /* ULTRAVIOLET_WORLD */
			PLANET_NEVER,  /* PLUTONIC_WORLD */
			PLANET_NEVER,  /* RAINBOW_WORLD */
			PLANET_NEVER,  /* SHATTERED_WORLD */
			PLANET_NEVER,  /* SAPPHIRE_WORLD */
			PLANET_ALWAYS, /* ORGANIC_WORLD */
			PLANET_ALWAYS, /* XENOLITHIC_WORLD */
			PLANET_ALWAYS, /* REDUX_WORLD */
			PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
			PLANET_NEVER,  /* EMERALD_WORLD */
			PLANET_ALWAYS, /* CHLORINE_WORLD */
			PLANET_ALWAYS, /* MAGNETIC_WORLD */
			PLANET_ALWAYS, /* WATER_WORLD */
			PLANET_ALWAYS, /* TELLURIC_WORLD */
			PLANET_ALWAYS, /* HYDROCARBON_WORLD */
			PLANET_ALWAYS, /* IODINE_WORLD */
			PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
			PLANET_NEVER,  /* RUBY_WORLD */
			PLANET_ALWAYS, /* MAGMA_WORLD */
			PLANET_ALWAYS, /* MAROON_WORLD */
			//PLANET_ALWAYS, /* DIAMOND_WORLD */

			PLANET_ALWAYS, /* BLU_GAS_GIANT */
			PLANET_ALWAYS, /* CYA_GAS_GIANT */
			PLANET_ALWAYS, /* GRN_GAS_GIANT */
			PLANET_ALWAYS, /* GRY_GAS_GIANT */
			PLANET_ALWAYS, /* ORA_GAS_GIANT */
			PLANET_ALWAYS, /* PUR_GAS_GIANT */
			PLANET_ALWAYS, /* RED_GAS_GIANT */
			PLANET_ALWAYS, /* VIO_GAS_GIANT */
			PLANET_ALWAYS, /* YEL_GAS_GIANT */
		};

	return (PlanetDistribution[which_world]);
}

#define DWARF_ROCK_DIST MIN_PLANET_RADIUS
#define DWARF_GASG_DIST SCALE_RADIUS(12)

#define GIANT_ROCK_DIST SCALE_RADIUS(8)
#define GIANT_GASG_DIST SCALE_RADIUS(13)

#define SUPERGIANT_ROCK_DIST SCALE_RADIUS(16)
#define SUPERGIANT_GASG_DIST SCALE_RADIUS(33)

void ComputeSpeed(PLANET_DESC* planet, bool GeneratingMoons,
				  uint16_t rand_val)
{
	//BW : empiric values, which would give roughly correct
	// rotation periods for most moons in the solar system
	if (GeneratingMoons)
	{
		planet->orb_speed =
			FULL_CIRCLE / (29 * pow((double)planet->radius / (MIN_MOON_RADIUS + (MAX_GEN_MOONS - 1) * MOON_DELTA), 1.5));
		if ((planet->pPrevDesc->data_index & ~PLANET_SHIELDED)
			>= FIRST_GAS_GIANT)
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
			FULL_CIRCLE / (ONE_YEAR * pow((double)planet->radius / EARTH_RADIUS, 1.5));
	}
}

void FillOrbits(SOLARSYS_STATE* system, uint8_t NumPlanets,
				PLANET_DESC* pBaseDesc, bool TypesDefined)
{ /* Generate Planets in orbit around star */
	uint8_t StarColor, PlanetCount, MaxPlanet;
	bool GeneratingMoons;
	uint16_t StarSize;
	PLANET_DESC* pPD;
	uint32_t loopCounter = 0;
	uint8_t orbitDiff;

	struct
	{
		uint16_t MinRockyDist, MinGasGDist;
	} Suns[] =
		{
			{DWARF_ROCK_DIST,	  DWARF_GASG_DIST	 },
			{GIANT_ROCK_DIST,	  GIANT_GASG_DIST	 },
			{SUPERGIANT_ROCK_DIST, SUPERGIANT_GASG_DIST},
	};
#ifdef DEBUG_ORBITS
	char buf[256];
	char stype[] = {'D', 'G', 'S'};
	char scolor[] = {'B', 'G', 'O', 'R', 'W', 'Y'};
#endif /* DEBUG_ORBITS */

	if (NumPlanets == NUMPLANETS_PDESC)
	{ // To shorten the redundancy of the function in repeated areas.
		NumPlanets = system->SunDesc[0].NumPlanets;
		pBaseDesc = system->PlanetDesc;
	}

	pPD = pBaseDesc;
	StarSize = system->SunDesc[0].data_index;
	StarColor = STAR_COLOR(CurStarDescPtr->Type);

	if (NumPlanets == (uint8_t)~0)
	{
		// XXX: This is pretty funny. Instead of calling RNG once, like so:
		//     1 + Random % MAX_GENERATED_PLANETS
		//   we spin in a loop until the result > 0.
		//   Note that this behavior must be kept to preserve the universe.
		do
		{
			NumPlanets = uqm::lowWord(RandomContext_Random(SysGenRNG))
					   % (MAX_GEN_PLANETS + 1);
		} while (NumPlanets == 0);
		system->SunDesc[0].NumPlanets = NumPlanets;
	}

#ifdef DEBUG_ORBITS
	GetClusterName(CurStarDescPtr, buf);
	uqm::log::debug("cluster name = {}  color = %c type = %c", buf,
					scolor[STAR_COLOR(CurStarDescPtr->Type)],
					stype[STAR_TYPE(CurStarDescPtr->Type)]);
#endif /* DEBUG_ORBITS */
	GeneratingMoons = (bool)(pBaseDesc == system->MoonDesc);
	bool GasGiant = pPD && pPD->pPrevDesc && (pPD->pPrevDesc->data_index & ~PLANET_SHIELDED) >= FIRST_GAS_GIANT;
	if (GeneratingMoons)
	{
		MaxPlanet = (PrimeSeed || (StarSeed && !GasGiant) ? FIRST_LARGE_ROCKY_WORLD : (StarSeed ? FIRST_GAS_GIANT : LAST_LARGE_ROCKY_WORLD));
	}
	else
	{
		MaxPlanet = NUMBER_OF_PLANET_TYPES;
	}
	PlanetCount = NumPlanets;
	while (NumPlanets--)
	{
		uint8_t chance;
		uint32_t rand_val, min_radius;
		int16_t delta_r;
		PLANET_DESC* pLocPD;

		do
		{
			rand_val = RandomContext_Random(SysGenRNG);
			if (TypesDefined)
			{
				rand_val = 0;
			}
			else
			{
				pPD->data_index =
					(uint8_t)(uqm::highByte(uqm::lowWord(rand_val)) % MaxPlanet);
			}


			chance = PLANET_NEVER;
			switch (StarColor)
			{
				case BLUE_BODY:
					chance = BlueDistribution(pPD->data_index);
					break;
				case GREEN_BODY:
					chance = GreenDistribution(pPD->data_index);
					break;
				case ORANGE_BODY:
					chance = OrangeDistribution(pPD->data_index);
					break;
				case RED_BODY:
					chance = RedDistribution(pPD->data_index);
					break;
				case WHITE_BODY:
					chance = WhiteDistribution(pPD->data_index);
					break;
				case YELLOW_BODY:
					chance = YellowDistribution(pPD->data_index);
					break;
			}
		} while (uqm::lowByte(uqm::lowWord(rand_val)) >= chance);

		if (pPD->data_index < FIRST_GAS_GIANT)
		{
			min_radius = Suns[StarSize].MinRockyDist;
		}
		else
		{
			min_radius = Suns[StarSize].MinGasGDist;
		}
RelocatePlanet:
		rand_val = RandomContext_Random(SysGenRNG);
		if (GeneratingMoons)
		{
			pPD->radius = MIN_MOON_RADIUS
						+ ((uqm::lowWord(rand_val) % MAX_GEN_MOONS) * MOON_DELTA);
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
				(uqm::lowWord(rand_val) % (MAX_PLANET_RADIUS - min_radius))
				+ min_radius;
			for (pLocPD = pPD - 1; pLocPD >= pBaseDesc; --pLocPD)
			{
				if (loopCounter < UINT16_MAX)
				{
					delta_r = UNSCALE_RADIUS(pLocPD->radius) / 5 - UNSCALE_RADIUS(pPD->radius) / 5;
					orbitDiff = 1;
				}
				else
				{
					delta_r = UNSCALE_RADIUS(pLocPD->radius) - UNSCALE_RADIUS(pPD->radius);
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


		rand_val = RandomContext_Random(SysGenRNG);
		// Initial angle & coordinates as in Vanilla UQM
		// Still used to compute rand_seed and the position
		// of the planet at the start of the game
		pPD->angle = NORMALIZE_ANGLE(uqm::lowWord(rand_val));
		pPD->location.x = COSINE(pPD->angle, pPD->radius);
		pPD->location.y = SINE(pPD->angle, pPD->radius);
		if (GeneratingMoons)
		{
			pPD->rand_seed = uqm::makeDword(
				COSINE(pPD->angle, RES_DESCALE(pPD->radius)),
				SINE(pPD->angle, RES_DESCALE(pPD->radius)));
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
		uint8_t i;

		for (i = 0; i < PlanetCount; ++i)
		{
			uint8_t j;

			for (j = (uint8_t)(PlanetCount - 1); j > i; --j)
			{
				if (pBaseDesc[i].radius > pBaseDesc[j].radius)
				{
					PLANET_DESC temp;

					temp = pBaseDesc[i];
					pBaseDesc[i] = pBaseDesc[j];
					pBaseDesc[j] = temp;
				}
			}
		}
	}
}

uint8_t
PickClosestHabitable(SOLARSYS_STATE* solarSys)
{
	const int16_t hRangesD[NUM_STAR_COLORS][2] = {
		{860,  1790}, // blue
		{540,  1150}, // green
		{140,  280 }, // orange
		{0,	0	 }, // red
		{1240, 2560}, // white
		{320,  640 }  // yellow
	};
	const int16_t hRangesG[NUM_STAR_COLORS][2] = {
		{0,	0	 }, // blue
		{0,	0	 }, // green
		{3410, 7120}, // orange
		{860,  1800}, // red
		{0,	0	 }, // white
		{7630, 7936}  // yellow
	};
	uint8_t starColor, starType, numPlanets, i;
	int16_t hRangeMin = 0;
	int16_t hRangeMax = 0;
	int16_t hRangeMed = 0;
	uint8_t pByte = 0;
	PLANET_DESC* pPD;
	PLANET_DESC* pPlanet;
	uint32_t rand = RandomContext_GetSeed(SysGenRNG);

	numPlanets = solarSys->SunDesc[0].NumPlanets;
	pPD = solarSys->PlanetDesc;

	starColor = STAR_COLOR(CurStarDescPtr->Type);
	starType = STAR_TYPE(CurStarDescPtr->Type);

	if (starType == SUPER_GIANT_STAR)
	{
		return numPlanets--;
	}

	if (starType == DWARF_STAR)
	{
		if (starColor == RED_BODY)
		{
			return 0;
		}

		hRangeMin = hRangesD[starColor][0];
		hRangeMax = hRangesD[starColor][1];
	}

	if (starType == GIANT_STAR)
	{
		if (starColor == BLUE_BODY || starColor == GREEN_BODY
			|| starColor == WHITE_BODY)
		{

			pPlanet = &pPD[numPlanets--];

			pPlanet->radius = MAX_PLANET_RADIUS;
			pPlanet->angle = NORMALIZE_ANGLE(uqm::lowWord(rand));
			pPlanet->location.x = COSINE(pPlanet->angle, pPlanet->radius);
			pPlanet->location.y = SINE(pPlanet->angle, pPlanet->radius);
			ComputeSpeed(pPlanet, false, uqm::highWord(rand));

			return numPlanets--;
		}

		hRangeMin = hRangesG[starColor][0];
		hRangeMax = hRangesG[starColor][1];
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
		int16_t min, max;

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

		pPlanet->radius = RangeMinMax(min, max, rand);

		pPlanet->angle = NORMALIZE_ANGLE(uqm::lowWord(rand));
		pPlanet->location.x = COSINE(pPlanet->angle, pPlanet->radius);
		pPlanet->location.y = SINE(pPlanet->angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, uqm::highWord(rand));
	}

	return pByte;
}

bool CheckForHabitable(SOLARSYS_STATE* solarSys)
{
	const int16_t HabitableRanges[NUM_STAR_COLORS][2] = {
		{853,  1790}, // blue
		{544,  1151}, // green
		{139,  287 }, // orange
		{68,	 68  }, // red
		{1231, 2569}, // white
		{312,  778 }  // yellow
	};
#define CLOSEST_RADIUS HabitableRanges[ORANGE_BODY][0]
	PLANET_DESC* pPD;
	uint8_t starColor;
	int16_t i;
	uint8_t planetByte = solarSys->SunDesc[0].PlanetByte;
	uint8_t numPlanets = solarSys->SunDesc[0].NumPlanets;
	int16_t habitableRangeMin, habitableRangeMax, newRadius; // , radiusDiff;
	int16_t oldRadius;
	uint32_t planetRadii[MAX_GEN_PLANETS];
	uint32_t rand_val;
	// static int16_t diffCheck, min_radius;

	starColor = STAR_COLOR(CurStarDescPtr->Type);
	// Terrible, but efficient, hack to ensure some semblance of sanity.
	// Eventually, we will need further code if we care about habitable homes.
	if (StarSeed && starColor == RED_BODY)
	{
		starColor = ORANGE_BODY;
	}

	pPD = solarSys->PlanetDesc;
	oldRadius = pPD[planetByte].radius;

	habitableRangeMin = HabitableRanges[starColor][0];
	habitableRangeMax = HabitableRanges[starColor][1];

	// This ensures the planets are at least a certain distance apart, which
	// mirrors UNSCALE_RADIUS logic from FillOrbits (radius / 2^6 / 5)
	if (StarSeed && numPlanets > 1 && pPD[1].radius < habitableRangeMax + 320
		&& pPD[1].radius > habitableRangeMin + 320)
	{
		habitableRangeMax = pPD[1].radius - 320;
	}

	if ((oldRadius >= habitableRangeMin && oldRadius <= habitableRangeMax)
		|| starColor == RED_BODY || planetByte > 0)
	{
		return false;
	}

	for (i = 0; i < numPlanets; ++i)
	{
		planetRadii[i] = pPD[i].radius;
	}

	rand_val = RandomContext_Random(SysGenRNG);

	newRadius = (uqm::lowWord(rand_val) % (habitableRangeMax - habitableRangeMin) + habitableRangeMin);

	// radiusDiff = abs (oldRadius - newRadius);

	planetRadii[planetByte] = newRadius;

	if ((StarSeed && numPlanets == 1) || planetRadii[planetByte] < planetRadii[planetByte + 1])
	{
		pPD[planetByte].radius = planetRadii[planetByte];
		pPD[planetByte].angle = NORMALIZE_ANGLE(uqm::lowWord(rand_val));
		pPD[planetByte].location.x =
			COSINE(pPD[planetByte].angle, pPD[planetByte].radius);
		pPD[planetByte].location.y =
			SINE(pPD[planetByte].angle, pPD[planetByte].radius);
		ComputeSpeed(&pPD[planetByte], false, uqm::highWord(rand_val));

		return true;
	}
	else
	{
		return false;
	}

	/*if (planetByte > 0 && newRadius < pPD[planetByte-1].radius)
	{
		uint32_t radialSection = planetRadii[planetByte] - CLOSEST_RADIUS;
		uint8_t numRockySections = radialSection / DWARF_ROCK_DIST;
		uint8_t numGassySections = radialSection / DWARF_GASG_DIST;

		printf ("radialSection {}, numRockySections {}, numGassySections {}\n", radialSection, numRockySections, numGassySections);

		for (i = planetByte - 1; i > -1; --i)
		{
			rand_val = RandomContext_Random (SysGenRNG);

			planetRadii[i] = (uqm::lowWord(rand_val) % (
				planetRadii[(i + 1)] - CLOSEST_RADIUS) + CLOSEST_RADIUS);
		}

		for (i = planetByte - 1; i > -1; --i)
		{
			if (pPD[i].data_index < FIRST_GAS_GIANT)
				min_radius = DWARF_ROCK_DIST;
			else
				min_radius = DWARF_GASG_DIST;

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
			if (planetRadii[i] > CLOSEST_RADIUS)
			{
				rand_val = RandomContext_Random (SysGenRNG);
				pPD[i].radius = planetRadii[i];
				pPD[i].angle = NORMALIZE_ANGLE (uqm::lowWord(rand_val));
				pPD[i].location.x =
					COSINE (pPD[i].angle, pPD[i].radius);
				pPD[i].location.y =
					SINE (pPD[i].angle, pPD[i].radius);
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
	//				PLANET_DESC temp;

	//				temp = solarSys->PlanetDesc[i];
	//				solarSys->PlanetDesc[i] = solarSys->PlanetDesc[j];
	//				solarSys->PlanetDesc[j] = temp;
	//			}
	//		}
	//	}
	//}
}
