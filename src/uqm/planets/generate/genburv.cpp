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

#include "../../globdata.h"
#include "../../nameref.h"
#include "../../resinst.h"
#include "../lander.h"
#include "../planets.h"
#include "genall.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateBurvixese_generatePlanets(SolarSystemState* solarSys);
static bool GenerateBurvixese_generateMoons(SolarSystemState* solarSys,
											SystemPlanet* planet);
static bool GenerateBurvixese_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world);
static uint16_t GenerateBurvixese_generateEnergy(const SolarSystemState*,
												 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateBurvixese_pickupEnergy(SolarSystemState* solarSys,
										   SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateBurvixeseFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateBurvixese_generatePlanets,
	/* .generateMoons    = */ GenerateBurvixese_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateBurvixese_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateBurvixese_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateBurvixese_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateBurvixese_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pPlanet {};
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	pSunDesc->MoonByte = 0;
	pSunDesc->PlanetByte = 0;

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		uint16_t angle {};

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = *PlanetType::Redux;
		pPlanet->NumPlanets = 1;
		pPlanet->radius = EarthRadius * 39L / 100;
		angle = ARCTAN(pPlanet->location.x, pPlanet->location.y);
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}
	else
	{
		const uint32_t rand_val = SysGenRNG->next();

		//pSunDesc->PlanetByte = PickClosestHabitable (solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld();

		// A large rocky with 1 moon has a 1 in 5 chance of a second moon.
		pPlanet->NumPlanets = (rand_val % 5 == 0 ? 2 : 1);
		// Probably use second moon?  Reuse is OK because 3 and 5 are coprime.
		if (pPlanet->NumPlanets == 2)
		{
			pSunDesc->MoonByte = (rand_val % 3 == 0 ? 0 : 1);
		}
	}

	return true;
}

static bool GenerateBurvixese_generateMoons(SolarSystemState* solarSys,
											SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (!uqm::isOriginalSeed())
	{
		return true;
	}

	if (matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		SystemPlanet* pMoonDesc = &solarSys->MoonDesc[MoonByte];
		uint16_t angle {};

		pMoonDesc->data_index = *PlanetType::Selenic;
		pMoonDesc->radius =
			MinMoonRadius() + (MaxGenMoons - 1) * MoonDelta();
		const uint32_t rand_val {SysGenRNG->next()};
		angle = NormalizeAngle(uqm::lowWord(rand_val));
		pMoonDesc->location.x = Cosine(angle, pMoonDesc->radius);
		pMoonDesc->location.y = Sine(angle, pMoonDesc->radius);
		ComputeSpeed(pMoonDesc, true, 1);
	}

	return true;
}

static bool GenerateBurvixese_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world)
{
	DoPlanetaryAnalysis(&solarSys->SysInfo, world);
	uint32_t rand_val {SysGenRNG->seed()};

	solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Biological] = rand_val;
	GenerateLifeForms(&solarSys->SysInfo, GenerateAll, nullptr);
	rand_val = SysGenRNG->seed();

	solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Mineral] = rand_val;
	GenerateMineralDeposits(&solarSys->SysInfo, GenerateAll, nullptr);

	solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Energy] = rand_val;

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(
				LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(BurvRuinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

		if (!isDifficulty(uqm::Difficulty::Hard))
		{
			solarSys->SysInfo.PlanetInfo.Weather = 0;
			solarSys->SysInfo.PlanetInfo.Tectonics = 0;
		}
	}
	else if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte)
			 && !uqm::gameState().flag(uqm::GameFlag::BURVIXESE_BROADCASTERS))
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] = CaptureDrawable(
			LoadGraphic(BurvBcsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(BurvBcsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

		if (!isDifficulty(uqm::Difficulty::Hard))
		{
			solarSys->SysInfo.PlanetInfo.Weather = 2;
			solarSys->SysInfo.PlanetInfo.Tectonics = 1;
		}
	}

	LoadPlanet(nullptr);

	return true;
}

static uint16_t GenerateBurvixese_generateEnergy(const SolarSystemState* solarSys,
												 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (uqm::gameState().flag(uqm::GameFlag::BURVIXESE_BROADCASTERS))
		{ // already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateBurvixese_pickupEnergy(SolarSystemState* solarSys,
										   SystemPlanet* world, uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);
		return false;
	}

	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		assert(!uqm::gameState().flag(uqm::GameFlag::BURVIXESE_BROADCASTERS) && whichNode == 0);

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		uqm::gameState().setFlag(uqm::GameFlag::BURVIXESE_BROADCASTERS, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(1));

		return true; // picked up
	}

	(void)whichNode;
	return false;
}

} // namespace uqm