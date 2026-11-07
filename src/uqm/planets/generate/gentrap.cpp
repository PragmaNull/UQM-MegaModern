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

#include "genall.h"
#include "../planets.h"
#include "uqm/random/UQMRandom.h"
#include "uqm/globdata.h"


namespace uqm
{
static bool GenerateTrap_generatePlanets(SolarSystemState* solarSys);
static bool GenerateTrap_generateOrbital(SolarSystemState* solarSys,
										 SystemPlanet* world);


const GenerateFunctions generateTrapFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateTrap_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateTrap_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateTrap_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = *PlanetType::Telluric;
		pPlanet->NumPlanets = 1;
		pPlanet->radius = EarthRadius * 203L / 100;
		const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}
	else
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld();
	}

	return true;
}

static bool GenerateTrap_generateOrbital(SolarSystemState* solarSys, SystemPlanet* world)
{
	GenerateDefault_generateOrbital(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (uqm::isOriginalSeed())
		{
			solarSys->SysInfo.PlanetInfo.AtmoDensity =
				EarthAtmosphere * 2;
			solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 35;
		}
		if (!isDifficulty(uqm::Difficulty::Hard))
		{
			solarSys->SysInfo.PlanetInfo.Weather = 3;
			solarSys->SysInfo.PlanetInfo.Tectonics = 1;
		}
	}

	return true;
}

} // namespace uqm