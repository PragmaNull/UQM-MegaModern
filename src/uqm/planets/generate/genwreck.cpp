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
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../resinst.h"
#include "../../state.h"
#include "../../build.h"
#include "libs/mathlib.h"
#include "../../comm.h"
#include "uqm/random/UQMRandom.h"

static bool GenerateWreck_generatePlanets(SOLARSYS_STATE* solarSys);
static bool GenerateWreck_generateOrbital(SOLARSYS_STATE* solarSys,
										  PLANET_DESC* world);
static uint16_t GenerateWreck_generateEnergy(const SOLARSYS_STATE*,
											 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO*);
static bool GenerateWreck_pickupEnergy(SOLARSYS_STATE* solarSys,
									   PLANET_DESC* world, uint16_t whichNode);


const GenerateFunctions generateWreckFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateWreck_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateWreck_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateWreck_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateWreck_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};

static bool GenerateWreck_generatePlanets(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];
	PLANET_DESC* pPlanet {};

	pSunDesc->PlanetByte = 6;

	if (!uqm::isOriginalSeed())
	{
		if (!uqm::isStarSeed())
		{
			const uint32_t RandVal = SysGenRNG->next();
			const uint8_t PByte = pSunDesc->PlanetByte + 1;
			pSunDesc->NumPlanets =
				(RandVal % (MaxGenPlanets - PByte) + PByte);

			FillOrbits(solarSys, pSunDesc->NumPlanets,
					   solarSys->PlanetDesc, false);
			GeneratePlanets(solarSys);
		}
		else
		{
			GenerateDefault_generatePlanets(solarSys);
		}

		if (uqm::isStarSeed())
		{
			pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
		}

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateWorlds(SmallRocky);
	}
	else
	{
		GenerateDefault_generatePlanets(solarSys);
	}

	return true;
}

static bool GenerateWreck_generateOrbital(SOLARSYS_STATE* solarSys,
										  PLANET_DESC* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (isDifficulty(uqm::Difficulty::Hard) && !(uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << PROBE_ENCOUNTER))
		{
			uint16_t sum {};

			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			if (uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP))
			{
				sum = 2;
			}
			else
			{
				sum = 4;
			}

			for (uint16_t i {0}; i < sum; ++i)
			{
				CloneShipFragment(SLYLANDRO_SHIP,
								  &uqm::gameState().game().npc_built_ship_q, 0);
			}

			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
			uqm::currentActivity() |= START_INTERPLANETARY;
			InitCommunication(SLYLANDRO_CONVERSATION);

			if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
			{
				return true;
			}

			{
				const bool Survivors =
					GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0;

				uqm::currentActivity() &= ~START_INTERPLANETARY;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);

				if (Survivors)
				{
					return true;
				}

				{
					uint16_t state {};

					state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);

					state |= 1 << PROBE_ENCOUNTER;

					uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));
				}

				RepairSISBorder();
			}
		}
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(WreckMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(WreckStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
		if (uqm::gameState().flag(uqm::GameFlag::PORTAL_KEY))
		{ // Already picked it up, skip the first report
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 1;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);
	return true;
}

static uint16_t GenerateWreck_generateEnergy(const SOLARSYS_STATE* solarSys,
											 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateWreck_pickupEnergy(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
									   uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(whichNode == 0);

		GenerateDefault_landerReportCycle(solarSys);

		if (!uqm::gameState().flag(uqm::GameFlag::PORTAL_KEY))
		{
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::PORTAL_KEY, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::PORTAL_KEY_ON_SHIP, static_cast<uint32_t>(1));
		}

		// The Wreck cannot be "picked up". It is always on the surface.
		return false;
	}

	(void)whichNode;
	return false;
}