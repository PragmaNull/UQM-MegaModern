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
#include "../lander.h"
#include "../planets.h"
#include "../../build.h"
#include "../../comm.h"
#include "../../gamestr.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"

static bool GenerateSyreen_generatePlanets(SolarSystemState* solarSys);
static bool GenerateSyreen_generateMoons(SolarSystemState* solarSys,
										 SystemPlanet* planet);
static bool GenerateSyreen_generateName(const SolarSystemState*,
										const SystemPlanet* world);
static bool GenerateSyreen_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world);
static uint16_t GenerateSyreen_generateEnergy(const SolarSystemState*,
											  const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateSyreen_pickupEnergy(SolarSystemState* solarSys,
										SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateSyreenFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSyreen_generatePlanets,
	/* .generateMoons    = */ GenerateSyreen_generateMoons,
	/* .generateName     = */ GenerateSyreen_generateName,
	/* .generateOrbital  = */ GenerateSyreen_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSyreen_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSyreen_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateSyreen_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	pSunDesc->MoonByte = 0;

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = *PlanetType::Water; // | PLANET_SHIELDED;
		pPlanet->NumPlanets = 1;
	}
	else
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld();

		if (!pPlanet->NumPlanets)
		{
			pPlanet->NumPlanets++;
		}
	}

	if (!RaceDead(RaceId::Syreen))
	{
		pPlanet->data_index |= *PlanetType::PlanetShielded;
	}

	return true;
}

static bool GenerateSyreen_generateMoons(SolarSystemState* solarSys,
										 SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		SystemPlanet* pMoonDesc = &solarSys->MoonDesc[MoonByte];

		if (!RaceDead(RaceId::Syreen))
		{
			pMoonDesc->data_index = *PlanetType::HierarchyStarbase;
		}
		else
		{
			pMoonDesc->data_index = *PlanetType::DestroyedStarbase;
		}

		if (uqm::isOriginalSeed() || uqm::isStarSeed())
		{
			pMoonDesc->radius = MinMoonRadius();
			pMoonDesc->location.x = Cosine(Quadrant, pMoonDesc->radius);
			pMoonDesc->location.y = Sine(Quadrant, pMoonDesc->radius);
			ComputeSpeed(pMoonDesc, true, 1);
		}
	}

	return true;
}

static bool GenerateSyreen_generateName(const SolarSystemState* solarSys,
										const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& (uqm::gameState().flag(uqm::GameFlag::SYREEN_HOME_VISITS)
			|| uqm::gameState().flag(uqm::GameFlag::SYREEN_KNOW_ABOUT_MYCON)))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(GameStringBase::PlanetNumber + 39));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateSyreen_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world)
{

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{ /* Gaia */
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

		GenerateDefault_generateOrbital(solarSys, world);

		solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 19;
		solarSys->SysInfo.PlanetInfo.AtmoDensity =
			EarthAtmosphere * 9 / 10;

		if (!isDifficulty(uqm::Difficulty::Hard))
		{
			solarSys->SysInfo.PlanetInfo.Tectonics = 0;
			solarSys->SysInfo.PlanetInfo.Weather = 0;
		}

		return true;
	}

	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		/* Starbase */
		if (!RaceDead(RaceId::Syreen))
		{
			InitCommunication(Conversation::Syreen);

			return true;
		}
		else
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(SyreenBaseStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

			DoDiscoveryReport(MenuSounds);

			solarSys->SysInfo.PlanetInfo.DiscoveryStrings.reset();
			FreeLanderFont(&solarSys->SysInfo.PlanetInfo);

			return true;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

static uint16_t GenerateSyreen_generateEnergy(const SolarSystemState* solarSys,
											  const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateSyreen_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
										uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{ // Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);
		return false;
	}

	(void)whichNode;
	return false;
}