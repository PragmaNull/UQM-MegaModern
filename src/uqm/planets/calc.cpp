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
#include "core/log/log.h"
#include "core/types/enum.h"
#include "libs/mathlib.h"
#include "planets.h"
#include "uqm/gendef.h"
#include "uqm/starmap.h"

namespace uqm
{
/* -------------------------------- DATA -------------------------------- */

/* -------------------------------- CODE -------------------------------- */

//#define DEBUG_PLANET_CALC

static constexpr auto LowTemp {0};
static constexpr auto MedTemp {500};
static constexpr auto HighTemp {1500};
static constexpr auto LowTempBonus {10};
static constexpr auto MedTempBonus {25};
static constexpr auto HighTempBonus {50};
static constexpr auto MaxTectonics {255};

enum class SunIntensity
{
	Red = 0,
	Orange,
	Yellow,
	Green,
	Blue,
	White
};
consteval bool isArithmeticEnum(SunIntensity) { return true; }

static uint16_t CalcFromBase(uint16_t base, uint16_t variance)
{
	return base + uqm::lowWord(SysGenRNG->next()) % variance;
}

static inline uint16_t CalcHalfBaseVariance(uint16_t base)
{
	return CalcFromBase(base, (base >> 1) + 1);
}

static void CalcSysInfo(SolarSystemInfo* SysInfoPtr)
{
	SysInfoPtr->StarSize = pSolarSysState->SunDesc[0].data_index;
	switch (StarColor(CurStarDescPtr()->Type))
	{
		case StarBodyColor::Blue:
			SysInfoPtr->StarIntensity = *SunIntensity::Blue;
			break;
		case StarBodyColor::Green:
			SysInfoPtr->StarIntensity = *SunIntensity::Green;
			break;
		case StarBodyColor::Orange:
			SysInfoPtr->StarIntensity = *SunIntensity::Orange;
			break;
		case StarBodyColor::Red:
			SysInfoPtr->StarIntensity = *SunIntensity::Red;
			break;
		case StarBodyColor::White:
			SysInfoPtr->StarIntensity = *SunIntensity::White;
			break;
		case StarBodyColor::Yellow:
			SysInfoPtr->StarIntensity = *SunIntensity::Yellow;
			break;
		default:
			break;
	}

	switch (StarType(CurStarDescPtr()->Type))
	{
		case StellarSize::Dwarf:
			SysInfoPtr->StarEnergy =
				(SysInfoPtr->StarIntensity + 1) * DwarfEnergy;
			break;
		case StellarSize::Giant:
			SysInfoPtr->StarEnergy =
				(SysInfoPtr->StarIntensity + 1) * GiantEnergy;
			break;
		case StellarSize::SuperGiant:
			SysInfoPtr->StarEnergy =
				(SysInfoPtr->StarIntensity + 1) * SupergiantEnergy;
			break;
		default:
			break;
	}
}

static uint16_t GeneratePlanetComposition(PlanetData* PlanetInfoPtr, int16_t SurfaceTemp,
										  int16_t radius)
{
	if (Plansize(PlanetInfoPtr->PlanDataPtr->Type) == PlanetSize::GasGiant)
	{
		PlanetInfoPtr->Weather = 7 << 5;
		return (GasGiantAtmosphere);
	}
	else
	{
		DepositSize range {};

		PlanetInfoPtr->Weather = 0;
		uint16_t atmo {0};
		if ((range = static_cast<DepositSize>(uqm::highNibble(PlanetInfoPtr->PlanDataPtr->AtmoAndDensity))) <= DepositSize::Heavy)
		{
			if (SurfaceTemp < ColdThreshold)
			{
				--range;
			}
			else if (SurfaceTemp > HotThreshold)
			{
				++range;
			}

			if (range <= DepositSize::Heavy + 1)
			{
				switch (range)
				{
					case DepositSize::Light:
						atmo = ThinAtmosphere;
						PlanetInfoPtr->Weather = 1 << 5;
						break;
					case DepositSize::Medium:
						atmo = NormalAtmosphere;
						PlanetInfoPtr->Weather = 2 << 5;
						break;
					case DepositSize::Heavy:
						atmo = ThickAtmosphere;
						PlanetInfoPtr->Weather = 4 << 5;
						break;
					default:
						atmo = SuperThickAtmosphere;
						PlanetInfoPtr->Weather = 6 << 5;
						break;
				}

				radius /= EarthRadius;
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

static constexpr auto GenericAlbedo {33}; /* In %, 0=black, 100 is reflective */
static constexpr auto AdjustForKelvin {273};
static constexpr auto PlanetTempConstant {277L};
static constexpr auto ColdBonus {20};
static constexpr auto HotBonus {200};

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
static int16_t CalcTemp(SolarSystemInfo* SysInfoPtr, int16_t radius)
{
	int16_t centigrade {};

	uint32_t alb {100 - GenericAlbedo};
	alb = square_root(square_root(alb * 100 * 10000))
		* PlanetTempConstant * SysInfoPtr->StarEnergy
		/ ((*SunIntensity::Yellow + 1) * DwarfEnergy);

	centigrade = (int16_t)(alb / square_root(radius * 10000L / EarthRadius))
			   - AdjustForKelvin;

	int16_t bonus {0};
	if (SysInfoPtr == &pSolarSysState->SysInfo
		&& uqm::highNibble(SysInfoPtr->PlanetInfo.PlanDataPtr->AtmoAndDensity) <= DepositSize::Heavy)
	{
		if (centigrade >= HotThreshold)
		{
			bonus = HotBonus;
		}
		else if (centigrade >= ColdThreshold)
		{
			bonus = ColdBonus;
		}

		bonus <<= uqm::highNibble(SysInfoPtr->PlanetInfo.PlanDataPtr->AtmoAndDensity);
		bonus = CalcHalfBaseVariance(bonus);
	}

	return (centigrade + bonus);
}

static constexpr auto NumTosses {10};
static constexpr auto TiltRange {180};

static uint16_t CalcRotation(PlanetData* PlanetInfoPtr)
{
	if (Plansize(PlanetInfoPtr->PlanDataPtr->Type) == PlanetSize::GasGiant)
	{
		return ((uint16_t)CalcFromBase(80, 80));
	}
	else if (uqm::lowByte(SysGenRNG->next()) % 10 == 0)
	{
		return ((uint16_t)CalcFromBase((uint16_t)50 * EarthHours, (uint16_t)200 * EarthHours));
	}
	else
	{
		return ((uint16_t)CalcFromBase(150, 150));
	}
}

static int16_t CalcTilt(void)
{ /* Calculate Axial Tilt */

	int16_t tilt {-(TiltRange / 2)};
	uint8_t i {NumTosses};
	do /* Using added Randomom values to give bell curve */
	{
		tilt += uqm::lowWord(SysGenRNG->next())
			  % ((TiltRange / NumTosses) + 1);
	} while (--i);

	return (tilt);
}

uint16_t CalcGravity(const PlanetData* PlanetInfoPtr)
{
	return (uint32_t)PlanetInfoPtr->PlanetDensity * PlanetInfoPtr->PlanetRadius
		 / 100;
}

static uint16_t CalcTectonics(uint16_t base, uint16_t temp)
{
	const uint16_t tect = CalcFromBase(base, 3 << 5);
#ifdef OLD
	if (temp >= HighTemp)
	{
		tect += HighTempBonus;
	}
	else if (temp >= MedTemp)
	{
		tect += MedTempBonus;
	}
	else if (temp >= LowTemp)
	{
		tect += LowTempBonus;
	}
#else  /* !OLD */
	(void)temp; /* silence compiler whining */
#endif /* OLD */
	return tect;
}

// This code moved from planets/surface.c:CalcLifeForms()
static int CalcLifeChance(const PlanetData* PlanetInfoPtr)
{
	int16_t life_var = 0;

	if (Plansize(PlanetInfoPtr->PlanDataPtr->Type) == PlanetSize::GasGiant)
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

static constexpr auto SmallRadius {25};
static constexpr auto LargeRadius {75};
static constexpr auto MinGasRadius {300};
static constexpr auto MaxGasRadius {1500};

// Sets the SysGenRNG to the required state first.
void DoPlanetaryAnalysis(SolarSystemInfo* SysInfoPtr, SystemPlanet* pPlanetDesc)
{
	assert((pPlanetDesc->data_index & ~*PlanetType::WorldTypeSpecial)
		   < PlanetType::NumberOfPlanetTypes);

	SysGenRNG->reseed(pPlanetDesc->rand_seed);

	CalcSysInfo(SysInfoPtr);

#ifdef DEBUG_PLANET_CALC
	{
		uqstl::array<uqstl::array<uint8_t, 8>, 6> ColorClass {
			{
				"Red",
				"Orange",
				"Yellow",
				"Green",
				"Blue",
				"White",
			 }
		 };
		uqstl::array<uqstl::array<uint8_t, 12>, 3> SizeName {
			{
				"Dwarf",
				"Giant",
				"Supergiant",
			 }
		 };

		uqm::log::debug("{} {}",
						ColorClass[SysInfoPtr->StarIntensity],
						SizeName[SysInfoPtr->StarSize]);
		uqm::log::debug("Stellar Energy: {} (sol = 3)",
						SysInfoPtr->StarEnergy);
	}
#endif /* DEBUG_PLANET_CALC */

	{
		int16_t radius {};

		SysInfoPtr->PlanetInfo.PlanDataPtr =
			&PlanData[pPlanetDesc->data_index & ~*PlanetType::PlanetShielded];

		if (pPlanetDesc->pPrevDesc == pSolarSysState->SunDesc.data())
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
		switch (static_cast<DensityClass>(uqm::lowNibble(SysInfoPtr->PlanetInfo.PlanDataPtr->AtmoAndDensity)))
		{
			case DensityClass::Gas:
				SysInfoPtr->PlanetInfo.PlanetDensity = 20;
				break;
			case DensityClass::Light:
				SysInfoPtr->PlanetInfo.PlanetDensity = 33;
				break;
			case DensityClass::Low:
				SysInfoPtr->PlanetInfo.PlanetDensity = 60;
				break;
			case DensityClass::Normal:
				SysInfoPtr->PlanetInfo.PlanetDensity = 100;
				break;
			case DensityClass::High:
				SysInfoPtr->PlanetInfo.PlanetDensity = 150;
				break;
			case DensityClass::Super:
				SysInfoPtr->PlanetInfo.PlanetDensity = 200;
				break;
		}
		SysInfoPtr->PlanetInfo.PlanetDensity +=
			(SysInfoPtr->PlanetInfo.PlanetDensity / 20)
			- (uqm::lowWord(SysGenRNG->next())
			   % (SysInfoPtr->PlanetInfo.PlanetDensity / 10));

		switch (Plansize(SysInfoPtr->PlanetInfo.PlanDataPtr->Type))
		{
			case PlanetSize::SmallRockyWorld:
				SysInfoPtr->PlanetInfo.PlanetRadius = CalcHalfBaseVariance(SmallRadius);
				break;
			case PlanetSize::LargeRockyWorld:
				SysInfoPtr->PlanetInfo.PlanetRadius = CalcHalfBaseVariance(LargeRadius);
				break;
			case PlanetSize::GasGiant:
				SysInfoPtr->PlanetInfo.PlanetRadius =
					CalcFromBase(MinGasRadius, MaxGasRadius - MinGasRadius);
				break;
			default:
				break;
		}

		SysInfoPtr->PlanetInfo.RotationPeriod = CalcRotation(&SysInfoPtr->PlanetInfo);
		SysInfoPtr->PlanetInfo.SurfaceGravity = CalcGravity(&SysInfoPtr->PlanetInfo);
		SysInfoPtr->PlanetInfo.AxialTilt = CalcTilt();
		if ((SysInfoPtr->PlanetInfo.Tectonics =
				 CalcTectonics(SysInfoPtr->PlanetInfo.PlanDataPtr->BaseTectonics,
							   SysInfoPtr->PlanetInfo.SurfaceTemperature))
			> MaxTectonics)
		{
			SysInfoPtr->PlanetInfo.Tectonics = MaxTectonics;
		}

		SysInfoPtr->PlanetInfo.AtmoDensity =
			GeneratePlanetComposition(&SysInfoPtr->PlanetInfo,
									  SysInfoPtr->PlanetInfo.SurfaceTemperature, radius);

		SysInfoPtr->PlanetInfo.Tectonics >>= 5;
		SysInfoPtr->PlanetInfo.Weather >>= 5;

		SysInfoPtr->PlanetInfo.LifeChance = CalcLifeChance(&SysInfoPtr->PlanetInfo);

#ifdef DEBUG_PLANET_CALC
		radius = (int16_t)((uint32_t)UnscaleRadius(radius) * 100 / UnscaleRadius(EarthRadius));
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
						SysInfoPtr->PlanetInfo.AtmoDensity / EarthAtmosphere,
						(SysInfoPtr->PlanetInfo.AtmoDensity * 100 / EarthAtmosphere) % 100);
		if (SysInfoPtr->PlanetInfo.AtmoDensity == 0)
		{
			uqm::log::debug("\tAtmosphere: (Vacuum)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < ThinAtmosphere)
		{
			uqm::log::debug("\tAtmosphere: (Thin)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < NormalAtmosphere)
		{
			uqm::log::debug("\tAtmosphere: (Normal)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < ThickAtmosphere)
		{
			uqm::log::debug("\tAtmosphere: (Thick)");
		}
		else if (SysInfoPtr->PlanetInfo.AtmoDensity < SuperThickAtmosphere)
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

} // namespace uqm
