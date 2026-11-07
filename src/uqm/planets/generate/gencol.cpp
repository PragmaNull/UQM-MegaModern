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
#include "../../build.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateColony_initNpcs(SolarSystemState* solarSys);
static bool GenerateColony_generatePlanets(SolarSystemState* solarSys);
static bool GenerateColony_generateMoons(SolarSystemState* solarSys,
										 SystemPlanet* planet);
static bool GenerateColony_generateName(const SolarSystemState*,
										const SystemPlanet* world);
static bool GenerateColony_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world);


const GenerateFunctions generateColonyFunctions = {
	/* .initNpcs         = */ GenerateColony_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateColony_generatePlanets,
	/* .generateMoons    = */ GenerateColony_generateMoons,
	/* .generateName     = */ GenerateColony_generateName,
	/* .generateOrbital  = */ GenerateColony_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateColony_initNpcs(SolarSystemState* solarSys)
{
	HIPGROUP hGroup {};

	uqm::gameState().game().BattleGroupRef = uqm::gameState().flag(uqm::GameFlag::COLONY_GRPOFFS);
	if (uqm::gameState().game().BattleGroupRef == 0)
	{
		CloneShipFragment(RaceId::Urquan,
						  &uqm::gameState().game().npc_built_ship_q, 0);
		uqm::gameState().game().BattleGroupRef = PutGroupInfo(GroupsAddNew, 1);
		ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
		uqm::gameState().setFlag(uqm::GameFlag::COLONY_GRPOFFS, static_cast<uint32_t>(uqm::gameState().game().BattleGroupRef));
	}

	GenerateDefault_initNpcs(solarSys);

	if (uqm::gameState().game().BattleGroupRef
		&& (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q)))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte + 1;

		InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
		GroupPtr->task = *GroupTask::InOrbit;
		GroupPtr->sys_loc = PlanetByte;	 /* orbitting colony */
		GroupPtr->dest_loc = PlanetByte; /* orbitting colony */
		GroupPtr->loc.x = 0;
		GroupPtr->loc.y = 0;
		GroupPtr->group_counter = 0;
		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
	}

	return true;
}

static bool GenerateColony_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	pSunDesc->PlanetByte = 0;
	SystemPlanet* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	FillOrbits(solarSys, (uint8_t)~0, pPlanet, false);

	if (uqm::isOriginalSeed())
	{
		pPlanet->radius = EarthRadius * 115L / 100;
		const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		pPlanet->data_index = *PlanetType::Water | *PlanetType::PlanetShielded;
		ComputeSpeed(pPlanet, false, 1);
		if (IsExtended())
		{
			pPlanet->NumPlanets = 1;
		}
	}
	else
	{
		const uint32_t rand_val = SysGenRNG->next();

		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld() | *PlanetType::PlanetShielded;

		GeneratePlanets(solarSys);

		pPlanet->NumPlanets = (rand_val % 5 == 0 ? 2 : 1);
	}

	return true;
}

static bool GenerateColony_generateMoons(SolarSystemState* solarSys,
										 SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (!uqm::isOriginalSeed())
	{
		return true;
	}

	if (IsExtended()
		&& matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		solarSys->MoonDesc[0].data_index = *PlanetType::Selenic;
	}

	return true;
}

static bool GenerateColony_generateName(const SolarSystemState* solarSys,
										const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(GameStringBase::PlanetNumber + 33));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateColony_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		DoPlanetaryAnalysis(&solarSys->SysInfo, world);

		solarSys->SysInfo.PlanetInfo.AtmoDensity =
			EarthAtmosphere * 98 / 100;
		solarSys->SysInfo.PlanetInfo.Weather = 0;
		solarSys->SysInfo.PlanetInfo.Tectonics = 0;
		solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 28;

		LoadPlanet(nullptr);

		return true;
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

} // namespace uqm