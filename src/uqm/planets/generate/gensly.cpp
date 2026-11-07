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

#include "../../comm.h"
#include "../../gamestr.h"
#include "../planets.h"
#include "genall.h"


namespace uqm
{
static bool GenerateSlylandro_generatePlanets(SolarSystemState* solarSys);
static bool GenerateSlylandro_generateName(const SolarSystemState*,
										   const SystemPlanet* world);
static bool GenerateSlylandro_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world);


const GenerateFunctions generateSlylandroFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSlylandro_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateSlylandro_generateName,
	/* .generateOrbital  = */ GenerateSlylandro_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateSlylandro_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = 3;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = *PlanetType::RedGasGiant;
		pPlanet->NumPlanets = 1;
	}
	else
	{
		GenerateGasGiantRanged(solarSys);
	}

	return true;
}

static bool GenerateSlylandro_generateName(const SolarSystemState* solarSys,
										   const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_HOME_VISITS))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(GameStringBase::PlanetNumber + 36));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateSlylandro_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		InitCommunication(Conversation::SlylandroHome);
		return true;
	}

	GenerateDefault_generateOrbital(solarSys, world);
	return true;
}

} // namespace uqm