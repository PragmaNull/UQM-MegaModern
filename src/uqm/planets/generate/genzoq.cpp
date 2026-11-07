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

#include "../../build.h"
#include "../../comm.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../planets.h"
#include "genall.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateZoqFotPik_initNpcs(SolarSystemState* solarSys);
static bool GenerateZoqFotPik_generatePlanets(SolarSystemState* solarSys);
static bool GenerateZoqFotPik_generateMoons(SolarSystemState* solarSys,
											SystemPlanet* planet);
static bool GenerateZoqFotPik_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world);
static uint16_t GenerateZoqFotPik_generateEnergy(const SolarSystemState*,
												 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateZoqFotPik_pickupEnergy(SolarSystemState* solarSys,
										   SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateZoqFotPikFunctions = {
	/* .initNpcs         = */ GenerateZoqFotPik_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateZoqFotPik_generatePlanets,
	/* .generateMoons    = */ GenerateZoqFotPik_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateZoqFotPik_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateZoqFotPik_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateZoqFotPik_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateZoqFotPik_initNpcs(SolarSystemState* solarSys)
{
	if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS) != 1)
	{
		GenerateDefault_initNpcs(solarSys);
	}

	if (SpaceMusicOK)
	{
		findRaceSOI();
	}

	return true;
}

static bool GenerateZoqFotPik_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	GenerateDefault_generatePlanets(solarSys);

	if (CurStarDescPtr()->Index == PlotId::Zoqfot)
	{
		if (uqm::isOriginalSeed())
		{
			pSunDesc->PlanetByte = 0;
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = *PlanetType::Water;
			pPlanet->NumPlanets = 1;
			pPlanet->radius = EarthRadius * 138L / 100;
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
	}
	else if (IsExtended())
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (CurStarDescPtr()->Index == PlotId::ZoqColony2)
		{
			pPlanet->NumPlanets = 1;
			pSunDesc->MoonByte = 0;
			pPlanet->data_index = GenerateGasGiantWorld();

			if (!uqm::isOriginalSeed())
			{ // preserving the integrity of the universe
				while (pPlanet->NumPlanets == 0)
				{
					GeneratePlanets(solarSys);
				}
				pSunDesc->MoonByte = PlanetByteGen(pPlanet);
			}
		}
		else
		{
			pPlanet->data_index = GenerateHabitableWorld();
		}
	}

	return true;
}

static bool GenerateZoqFotPik_generateMoons(SolarSystemState* solarSys,
											SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (CurStarDescPtr()->Index == PlotId::ZoqColony2
		&& matchWorld(solarSys, planet, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		SystemPlanet* pMoonDesc = &solarSys->MoonDesc[MoonByte];

		pMoonDesc->data_index = GenerateHabitableWorld();
	}

	return true;
}

static bool GenerateZoqFotPik_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world)
{
	if (CurStarDescPtr()->Index == PlotId::Zoqfot
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (StartSphereTracking(RaceId::Zoqfotpik))
		{
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS))
			{
				CloneShipFragment(RaceId::BlackUrquan,
								  &uqm::gameState().game().npc_built_ship_q, 0);

				uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
				InitCommunication(Conversation::Blackurq);

				if (uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
				{
					return true;
				}

				if (GetHeadLink(&uqm::gameState().game().npc_built_ship_q))
				{
					uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
					ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
					GetGroupInfo(GroupsRandom, GroupLoadIp);
					return true;
				}
			}

			CloneShipFragment(RaceId::Zoqfotpik,
							  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);

			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			InitCommunication(Conversation::Zoqfotpik);

			if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
			{
				uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);
			}

			return true;
		}

		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	if (((CurStarDescPtr()->Index == PlotId::ZoqColony0
		  || CurStarDescPtr()->Index == PlotId::ZoqColony1
		  || CurStarDescPtr()->Index == PlotId::ZoqColony3)
		 && matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
		&& IsExtended())
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(ZfpruinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	if (CurStarDescPtr()->Index == PlotId::ZoqColony2
		&& matchWorld(solarSys, world, MatchPbyte, MatchMbyte)
		&& IsExtended())
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(ZfpruinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	GenerateDefault_generateOrbital(solarSys, world);

	if ((CurStarDescPtr()->Index == PlotId::Zoqfot || IsExtended())
		&& CurStarDescPtr()->Index != PlotId::ZoqColony2
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& !isDifficulty(uqm::Difficulty::Hard))
	{
		const uint32_t rand = SysGenRNG->seed();
		PlanetData* PlanetInfo = &solarSys->SysInfo.PlanetInfo;

		PlanetInfo->Weather = 1;
		PlanetInfo->Tectonics = 1;
		PlanetInfo->SurfaceTemperature = uqm::rangeMinMax(-10, 40, rand);
	}

	if (CurStarDescPtr()->Index == PlotId::ZoqColony2
		&& matchWorld(solarSys, world, MatchPbyte, MatchMbyte)
		&& IsExtended() && !isDifficulty(uqm::Difficulty::Hard))
	{
		const uint32_t rand = SysGenRNG->seed();
		PlanetData* PlanetInfo = &solarSys->SysInfo.PlanetInfo;

		PlanetInfo->Weather = 1;
		PlanetInfo->Tectonics = 1;
		PlanetInfo->SurfaceTemperature = uqm::rangeMinMax(-10, 40, rand);
	}

	return true;
}

static constexpr auto NumRuins {4};

static uint16_t GenerateZoqFotPik_generateEnergy(const SolarSystemState* solarSys,
												 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (CurStarDescPtr()->Index == PlotId::Zoqfot)
		{
			return GenerateDefault_generateRuins(
				solarSys, whichNode, info);
		}

		if ((CurStarDescPtr()->Index == PlotId::ZoqColony0
			 || CurStarDescPtr()->Index == PlotId::ZoqColony1
			 || CurStarDescPtr()->Index == PlotId::ZoqColony3)
			&& IsExtended())
		{
			return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Energy,
									   NumRuins, 0, whichNode, info);
		}
	}

	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte)
		&& CurStarDescPtr()->Index == PlotId::ZoqColony2
		&& IsExtended())
	{
		return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Energy,
								   NumRuins, 0, whichNode, info);
	}

	return 0;
}

static bool GenerateZoqFotPik_pickupEnergy(SolarSystemState* solarSys,
										   SystemPlanet* world, uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& CurStarDescPtr()->Index != PlotId::ZoqColony2)
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);

		return false;
	}

	if (CurStarDescPtr()->Index == PlotId::ZoqColony2
		&& matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		GenerateDefault_landerReportCycle(solarSys);

		return false;
	}

	(void)whichNode;
	return false;
}

} // namespace uqm