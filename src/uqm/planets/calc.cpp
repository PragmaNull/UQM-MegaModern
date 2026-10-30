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

/* ----------------------------- INCLUDES ---------------------------- */
#include "planets.h"
#include "uqm/starmap.h"
#include "uqm/gendef.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
/* -------------------------------- DATA -------------------------------- */

/* -------------------------------- CODE -------------------------------- */

//#define DEBUG_PLANET_CALC

#define LOW_TEMP 0
#define MED_TEMP 500
#define HIGH_TEMP 1500
#define LOW_TEMP_BONUS 10
#define MED_TEMP_BONUS 25
#define HIGH_TEMP_BONUS 50
#define MAX_TECTONICS 255

enum
{
	RED_SUN_INTENSITY = 0,
	ORANGE_SUN_INTENSITY,
	YELLOW_SUN_INTENSITY,
	GREEN_SUN_INTENSITY,
	BLUE_SUN_INTENSITY,
	WHITE_SUN_INTENSITY
};

static uint16_t
CalcFromBase(uint16_t base, uint16_t variance)
{
	return base + uqm::lowWord(RandomContext_Random(SysGenRNG)) % variance;
}

static inline uint16_t
CalcHalfBaseVariance(uint16_t base)
{
	return CalcFromBase(base, (base >> 1) + 1);
}

static void
CalcSysInfo(SYSTEM_INFO* SysInfoPtr)
{
	SysInfoPtr->StarSize = pSolarSysState->SunDesc[0].data_index;
	switch (STAR_COLOR(CurStarDescPtr->Type))
	{
		case BLUE_BODY:
			SysInfoPtr->StarIntensity = BLUE_SUN_INTENSITY;
			break;
		case GREEN_BODY:
			SysInfoPtr->StarIntensity = GREEN_SUN_INTENSITY;
			break;
		case ORANGE_BODY:
			SysInfoPtr->StarIntensity = ORANGE_SUN_INTENSITY;
			break;
		case RED_BODY:
			SysInfoPtr->StarIntensity = RED_SUN_INTENSITY;
			break;
		case WHITE_BODY:
			SysInfoPtr->StarIntensity = WHITE_SUN_INTENSITY;
			break;
		case YELLOW_BODY:
			SysInfoPtr->StarIntensity = YELLOW_SUN_INTENSITY;
			break;
	}

	switch (STAR_TYPE(CurStarDescPtr->Type))
	{
		case DWARF_STAR:
			SysInfoPtr->StarEnergy =
				(SysInfoPtr->StarIntensity + 1) * DWARF_ENERGY;
			break;
		case GIANT_STAR:
			SysInfoPtr->StarEnergy =
				(SysInfoPtr->StarIntensity + 1) * GIANT_ENERGY;
			break;
		case SUPER_GIANT_STAR:
			SysInfoPtr->StarEnergy =
				(SysInfoPtr->StarIntensity + 1) * SUPERGIANT_ENERGY;
			break;
	}
}

static uint16_t
GeneratePlanetComposition(PLANET_INFO* PlanetInfoPtr, int16_t SurfaceTemp,
						  int16_t radius)
{
	if (PLANSIZE(PlanetInfoPtr->PlanDataPtr->Type) == GAS_GIANT)
	{
		PlanetInfoPtr->Weather = 7 << 5;
		return (GAS_GIANT_ATMOSPHERE);
	}
	else
	{
		uint8_t range;
		uint16_t atmo;

		PlanetInfoPtr->Weather = 0;
		atmo = 0;
		if ((range = uqm::highNibble(PlanetInfoPtr->PlanDataPtr->AtmoAndDensity)) <= HEAVY)
		{
			if (SurfaceTemp < COLD_THRESHOLD)
			{
				--range;
			}
			else if (SurfaceTemp > HOT_THRESHOLD)
			{
				++range;
			}

			if (range <= HEAVY + 1)
			{
				switch (range)
				{
					case LIGHT:
						atmo = THIN_ATMOSPHERE;
						PlanetInfoPtr->Weather = 1 << 5;
						break;
					case MEDIUM:
						atmo = NORMAL_ATMOSPHERE;
						PlanetInfoPtr->Weather = 2 << 5;
						break;
					case HEAVY:
						atmo = THICK_ATMOSPHERE;
						PlanetInfoPtr->Weather = 4 << 5;
						break;
					default:
						atmo = SUPER_THICK_ATMOSPHERE;
						PlanetInfoPtr->Weather = 6 << 5;
						break;
				}

				radius /= EARTH_RADIUS;
				if (radius < 2)
				{
					PlanetInfoPtr->Weather += 1 << 5;
				}
				else if (radius > 10)
				{
					PlanetInfoPtr->Weather -= 1 << 5;
				}
				atmo = CalcHalfBaseVariance(atmo);
			}
		}

		return (atmo);
	}
}

// This function is called both when the solar system is generated,
// and when planetary orbit is entered.
// In the former case, the if() block will not be executed,
// which means that the temperature calculated in that case will be
// slightly lower. The result is that the orbits may not always
// have the colour you'd expect based on the true temperature.
// (eg. Beta Corvi I). I don't know what the idea behind this is,
// but the if statement must be there for a reason. -- SvdB
// Update 2013-03-28: The contents of the if() block is probably there to
// model a greenhouse effect. It seems that it is taken into account when
// calculating the actual temperature (when landing or scanning), but not
// when determining the colors of the drawn orbits. (Thanks to James Scott
// for this insight.)
static int16_t
CalcTemp(SYSTEM_INFO* SysInfoPtr, int16_t radius)
{
#define GENERIC_ALBEDO 33 /* In %, 0=black, 100 is reflective */
#define ADJUST_FOR_KELVIN 273
#define PLANET_TEMP_CONSTANT 277L
	uint32_t alb;
	int16_t centigrade, bonus;

	alb = 100 - GENERIC_ALBEDO;
	alb = square_root(square_root(alb * 100 * 10000))
		* PLANET_TEMP_CONSTANT * SysInfoPtr->StarEnergy
		/ ((YELLOW_SUN_INTENSITY + 1) * DWARF_ENERGY);

	centigrade = (int16_t)(alb / square_root(radius * 10000L / EARTH_RADIUS))
			   - ADJUST_FOR_KELVIN;

	bonus = 0;
	if (SysInfoPtr == &pSolarSysState->SysInfo
		&& uqm::highNibble(SysInfoPtr->PlanetInfo.PlanDataPtr->AtmoAndDensity) <= HEAVY)
	{
#define COLD_BONUS 20
#define HOT_BONUS 200
		if (centigrade >= HOT_THRESHOLD)
		{
			bonus = HOT_BONUS;
		}
		else if (centigrade >= COLD_THRESHOLD)
		{
			bonus = COLD_BONUS;
		}

		bonus <<= uqm::highNibble(SysInfoPtr->PlanetInfo.PlanDataPtr->AtmoAndDensity);
		bonus = CalcHalfBaseVariance(bonus);
	}

	return (centigrade + bonus);
}

static uint16_t
CalcRotation(PLANET_INFO* PlanetInfoPtr)
{
	if (PLANSIZE(PlanetInfoPtr->PlanDataPtr->Type) == GAS_GIANT)
	{
		return ((uint16_t)CalcFromBase(80, 80));
	}
	else if (uqm::lowByte(RandomContext_Random(SysGenRNG)) % 10 == 0)
	{
		return ((uint16_t)CalcFromBase((uint16_t)50 * EARTH_HOURS, (uint16_t)200 * EARTH_HOURS));
	}
	else
	{
		return ((uint16_t)CalcFromBase(150, 150));
	}
}

static int16_t
CalcTilt(void)
{ /* Calculate Axial Tilt */
	int16_t tilt;
	uint8_t i;

#define NUM_TOSSES 10
#define TILT_RANGE 180
	tilt = -(TILT_RANGE / 2);
	i = NUM_TOSSES;
	do /* Using added Randomom values to give bell curve */
	{
		tilt += uqm::lowWord(RandomContext_Random(SysGenRNG))
			  % ((TILT_RANGE / NUM_TOSSES) + 1);
	} while (--i);

	return (tilt);
}

uint16_t
CalcGravity(const PLANET_INFO* PlanetInfoPtr)
{
	return (uint32_t)PlanetInfoPtr->PlanetDensity * PlanetInfoPtr->PlanetRadius
		 / 100;
}

static uint16_t
CalcTectonics(uint16_t base, uint16_t temp)
{
	uint16_t tect = CalcFromBase(base, 3 << 5);
#ifdef OLD
	if (temp >= HIGH_TEMP)
	{
		tect += HIGH_TEMP_BONUS;
	}
	else if (temp >= MED_TEMP)
	{
		tect += MED_TEMP_BONUS;
	}
	else if (temp >= LOW_TEMP)
	{
		tect += LOW_TEMP_BONUS;
	}
#else  /* !OLD */
	(void)temp; /* silence compiler whining */
#endif /* OLD */
	return tect;
}

// This code moved from planets/surface.c:CalcLifeForms()
static int
CalcLifeChance(const PLANET_INFO* PlanetInfoPtr)
{
	int16_t life_var = 0;

	if (PLANSIZE(PlanetInfoPtr->PlanDataPtr->Type) == GAS_GIANT)
	{
		return -1;
	}

	if (PlanetInfoPtr->SurfaceTemperature < -151)
	{
		life_var -= 300;
	}
	else if (PlanetInfoPtr->SurfaceTemperature < -51)
	{
		life_var -= 100;
	}
	else if (PlanetInfoPtr->SurfaceTemperature < 0)
	{
		life_var += 100;
	}
	else if (PlanetInfoPtr->SurfaceTemperature < 50)
	{
		life_var += 300;
	}
	else if (PlanetInfoPtr->SurfaceTemperature < 150)
	{
		life_var += 50;
	}
	else if (PlanetInfoPtr->SurfaceTemperature < 250)
	{
		life_var -= 100;
	}
	else if (PlanetInfoPtr->SurfaceTemperature < 500)
	{
		life_var -= 400;
	}
	else
	{
		life_var -= 800;
	}

	if (PlanetInfoPtr->AtmoDensity == 0)
	{
		life_var -= 1000;
	}
	else if (PlanetInfoPtr->AtmoDensity < 15)
	{
		life_var += 100;
	}
	else if (PlanetInfoPtr->AtmoDensity < 30)
	{
		life_var += 200;
	}
	else if (PlanetInfoPtr->AtmoDensity < 100)
	{
		life_var += 300;
	}
	else if (PlanetInfoPtr->AtmoDensity < 1000)
	{
		life_var += 150;
	}
	else if (PlanetInfoPtr->AtmoDensity < 2500)
		;
	else
	{
		life_var -= 100;
	}

#ifndef NOTYET
	life_var += 200 + 80 + 80;
#else  /* NOTYET */
	if (PlanetInfoPtr->SurfaceGravity < 10)
		;
	else if (PlanetInfoPtr->SurfaceGravity < 35)
	{
		life_var += 50;
	}
	else if (PlanetInfoPtr->SurfaceGravity < 75)
	{
		life_var += 100;
	}
	else if (PlanetInfoPtr->SurfaceGravity < 150)
	{
		life_var += 200;
	}
	else if (PlanetInfoPtr->SurfaceGravity < 400)
	{
		life_var += 50;
	}
	else if (PlanetInfoPtr->SurfaceGravity < 800)
		;
	else
	{
		life_var -= 100;
	}

	if (PlanetInfoPtr->Tectonics < 1)
	{
		life_var += 80;
	}
	else if (PlanetInfoPtr->Tectonics < 2)
	{
		life_var += 70;
	}
	else if (PlanetInfoPtr->Tectonics < 3)
	{
		life_var += 60;
	}
	else if (PlanetInfoPtr->Tectonics < 4)
	{
		life_var += 50;
	}
	else if (PlanetInfoPtr->Tectonics < 5)
	{
		life_var += 25;
	}
	else if (PlanetInfoPtr->Tectonics < 6)
		;
	else
	{
		life_var -= 100;
	}

	if (PlanetInfoPtr->Weather < 1)
	{
		life_var += 80;
	}
	else if (PlanetInfoPtr->Weather < 2)
	{
		life_var += 70;
	}
	else if (PlanetInfoPtr->Weather < 3)
	{
		life_var += 60;
	}
	else if (PlanetInfoPtr->Weather < 4)
	{
		life_var += 50;
	}
	else if (PlanetInfoPtr->Weather < 5)
	{
		life_var += 25;
	}
	else if (PlanetInfoPtr->Weather < 6)
		;
	else
	{
		life_var -= 100;
	}
#endif /* NOTYET */

	return life_var;
}

// Sets the SysGenRNG to the required state first.
void DoPlanetaryAnalysis(SYSTEM_INFO* SysInfoPtr, PLANET_DESC* pPlanetDesc)
{
	assert((pPlanetDesc->data_index & ~WORLD_TYPE_SPECIAL)
		   < NUMBER_OF_PLANET_TYPES);

	RandomContext_SeedRandom(SysGenRNG, pPlanetDesc->rand_seed);

	CalcSysInfo(SysInfoPtr);

#ifdef DEBUG_PLANET_CALC
	{
		uint8_t ColorClass[6][8] = {
			"Red",
			"Orange",
			"Yellow",
			"Green",
			"Blue",
			"White",
		};
		uint8_t SizeName[3][12] = {
			"Dwarf",
			"Giant",
			"Supergiant",
		};

		uqm::log::debug("{} {}",
						ColorClass[SysInfoPtr->StarIntensity],
						SizeName[SysInfoPtr->StarSize]);
		uqm::log::debug("Stellar Energy: {} (sol = 3)",
						SysInfoPtr->StarEnergy);
	}
#endif /* DEBUG_PLANET_CALC */

	{
		int16_t radius;

		SysInfoPtr->PlanetInfo.PlanDataPtr =
			&PlanData[pPlanetDesc->data_index & ~PLANET_SHIELDED];

		if (pPlanetDesc->pPrevDesc == pSolarSysState->SunDesc)
		{
			radius = pPlanetDesc->radius;
		}
		else
		{
			radius = pPlanetDesc->pPrevDesc->radius;
		}
		SysInfoPtr->PlanetInfo.PlanetToSunDist = radius;

		SysInfoPtr->PlanetInfo.SurfaceTemperature =
			CalcTemp(SysInfoPtr, radius);
		switch (uqm::lowNibble(SysInfoPtr->PlanetInfo.PlanDataPtr->AtmoAndDensity))
		{
			case GAS_DENSITY:
				SysInfoPtr->PlanetInfo.PlanetDensity = 20;
				break;
			case LIGHT_DENSITY:
				SysInfoPtr->PlanetInfo.PlanetDensity = 33;
				break;
			case LOW_DENSITY:
				SysInfoPtr->PlanetInfo.PlanetDensity = 60;
				break;
			case NORMAL_DENSITY:
				SysInfoPtr->PlanetInfo.PlanetDensity = 100;
				break;
			case HIGH_DENSITY:
				SysInfoPtr->PlanetInfo.PlanetDensity = 150;
				break;
			case SUPER_DENSITY:
				SysInfoPtr->PlanetInfo.PlanetDensity = 200;
				break;
		}
		SysInfoPtr->PlanetInfo.PlanetDensity +=
			(SysInfoPtr->PlanetInfo.PlanetDensity / 20)
			- (uqm::lowWord(RandomContext_Random(SysGenRNG))
			   % (SysInfoPtr->PlanetInfo.PlanetDensity / 10));

		switch (PLANSIZE(SysInfoPtr->PlanetInfo.PlanDataPtr->Type))
		{
			case SMALL_ROCKY_WORLD:
#define SMALL_RADIUS 25
				SysInfoPtr->PlanetInfo.PlanetRadius = CalcHalfBaseVariance(SMALL_RADIUS);
				break;
			case LARGE_ROCKY_WORLD:
#define LARGE_RADIUS 75
				SysInfoPtr->PlanetInfo.PlanetRadius = CalcHalfBaseVariance(LARGE_RADIUS);
				break;
			case GAS_GIANT:
#define MIN_GAS_RADIUS 300
#define MAX_GAS_RADIUS 1500
				SysInfoPtr->PlanetInfo.PlanetRadius =
					CalcFromBase(MIN_GAS_RADIUS, MAX_GAS_RADIUS - MIN_GAS_RADIUS);
				break;
		}

		SysInfoPtr->PlanetInfo.RotationPeriod = CalcRotation(&SysInfoPtr->PlanetInfo);
		SysInfoPtr->PlanetInfo.SurfaceGravity = CalcGravity(&SysInfoPtr->PlanetInfo);
		SysInfoPtr->PlanetInfo.AxialTilt = CalcTilt();
		if ((SysInfoPtr->PlanetInfo.Tectonics =
				 CalcTectonics(SysInfoPtr->PlanetInfo.PlanDataPtr->BaseTectonics,
							   SysInfoPtr->PlanetInfo.SurfaceTemperature))
			> MAX_TECTONICS)
		{
			SysInfoPtr->PlanetInfo.Tectonics = MAX_TECTONICS;
		}

		SysInfoPtr->PlanetInfo.AtmoDensity =
			GeneratePlanetComposition(&SysInfoPtr->PlanetInfo,
									  SysInfoPtr->PlanetInfo.SurfaceTemperature, radius);

		SysInfoPtr->PlanetInfo.Tectonics >>= 5;
		SysInfoPtr->PlanetInfo.Weather >>= 5;

		SysInfoPtr->PlanetInfo.LifeChance = CalcLifeChance(&SysInfoPtr->PlanetInfo);

#ifdef DEBUG_PLANET_CALC
		radius = (int16_t)((uint32_t)UNSCALE_RADIUS(radius) * 100 / UNSCALE_RADIUS(EARTH_RADIUS));
		uqm::log::debug("\tOrbital Distance   : {}.{:02} AU", radius / 100, radius % 100);
		//log_add (log_Debug, "\tPlanetary Mass : {}.%02d Earth masses",
		// SysInfoPtr->PlanetInfo.PlanetMass / 100,
		// SysInfoPtr->PlanetInfo.PlanetMass % 100);
		uqm::log::debug("\tPlanetary Radius   : {}.{:02} Earth radii",
						SysInfoPtr->PlanetInfo.PlanetRadius / 100,
						SysInfoPtr->PlanetInfo.PlanetRadius % 100);
		uqm::log::debug("\tSurface Gravity: {}.{:02} gravities",
						SysInfoPtr->PlanetInfo.SurfaceGravity / 100,
						SysInfoPtr->PlanetInfo.SurfaceGravity % 100);
		uqm::log::debug("\tSurface Temperature: {} degrees C",
						SysInfoPtr->PlanetInfo.SurfaceTemperature);
		uqm::log::debug("\tAxial Tilt : {} degrees",
						abs(SysInfoPtr->PlanetInfo.AxialTilt));
		uqm::log::debug("\tTectonics : Class {}",
						SysInfoPtr->PlanetInfo.Tectonics + 1);
		uqm::log::debug("\tAtmospheric Density: {}.{:02}",
						SysInfoPtr->PlanetInfo.AtmoDensity / EARTH_ATMOSPHERE,
						(SysInfoPtr->PlanetInfo.AtmoDensity * 100 / EARTH_ATMOSPHERE) % 100);
		if (SysInfoPtr->PlanetInfo.AtmoDensity == 0)
		{
			uqm::log::debug("\tAtmosphere: (Vacuum)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < THIN_ATMOSPHERE)
		{
			uqm::log::debug("\tAtmosphere: (Thin)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < NORMAL_ATMOSPHERE)
		{
			uqm::log::debug("\tAtmosphere: (Normal)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < THICK_ATMOSPHERE)
		{
			uqm::log::debug("\tAtmosphere: (Thick)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < SUPER_THICK_ATMOSPHERE)
		{
			uqm::log::debug("\tAtmosphere: (Super thick)");
		}
		else
		{
			uqm::log::debug("\tAtmosphere: (Gas Giant)");
		}

		uqm::log::debug("\tWeather   : Class {}",
						SysInfoPtr->PlanetInfo.Weather + 1);

		if (SysInfoPtr->PlanetInfo.RotationPeriod >= 480)
		{
			uqm::log::debug("\tLength of day  : {}.{} Earth days",
							SysInfoPtr->PlanetInfo.RotationPeriod / 240,
							SysInfoPtr->PlanetInfo.RotationPeriod % 240);
		}
		else
		{
			uqm::log::debug("\tLength of day  : {}.{} Earth hours",
							SysInfoPtr->PlanetInfo.RotationPeriod / 10,
							SysInfoPtr->PlanetInfo.RotationPeriod % 10);
		}
#endif /* DEBUG_PLANET_CALC */
	}
}
