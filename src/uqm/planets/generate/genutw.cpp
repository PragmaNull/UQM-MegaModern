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
#include "../../gendef.h"
#include "../../starmap.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


static bool GenerateUtwig_initNpcs(SOLARSYS_STATE* solarSys);
static bool GenerateUtwig_generatePlanets(SOLARSYS_STATE* solarSys);
static bool GenerateUtwig_generateName(const SOLARSYS_STATE*,
									   const PLANET_DESC* world);
static bool GenerateUtwig_generateOrbital(SOLARSYS_STATE* solarSys,
										  PLANET_DESC* world);
static uint16_t GenerateUtwig_generateEnergy(const SOLARSYS_STATE*,
											 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO*);
static bool GenerateUtwig_pickupEnergy(SOLARSYS_STATE* solarSys,
									   PLANET_DESC* world, uint16_t whichNode);


const GenerateFunctions generateUtwigFunctions = {
	/* .initNpcs         = */ GenerateUtwig_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateUtwig_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateUtwig_generateName,
	/* .generateOrbital  = */ GenerateUtwig_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateUtwig_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateUtwig_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateUtwig_initNpcs(SOLARSYS_STATE* solarSys)
{
	if (CurStarDescPtr()->Index == BOMB_DEFINED
		&& !uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB))
	{
		ReinitQueue(&uqm::gameState().game().ip_group_q);
		assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

		if (SpaceMusicOK)
		{
			findRaceSOI();
		}
	}
	else
	{
		GenerateDefault_initNpcs(solarSys);
	}

	return true;
}

static bool
GenerateUtwig_generatePlanets(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];
	PLANET_DESC* pPlanet {};

	if (CurStarDescPtr()->Index == UTWIG_DEFINED)
	{
		GenerateDefault_generatePlanets(solarSys);

		if (uqm::isOriginalSeed())
		{
			pSunDesc->PlanetByte = 0;
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = WATER_WORLD;
			pPlanet->NumPlanets = 1;
			pPlanet->radius = EarthRadius * 174L / 100;
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

	if (CurStarDescPtr()->Index == BOMB_DEFINED)
	{
		pSunDesc->PlanetByte = 5;
		pSunDesc->MoonByte = 1;

		if (!uqm::isOriginalSeed())
		{
			if (!uqm::isStarSeed())
			{
				const uint32_t RandVal = SysGenRNG->next();
				const uint8_t PByte = pSunDesc->PlanetByte + 1;
				pSunDesc->NumPlanets =
					(RandVal % (MaxGenPlanets - PByte) + PByte);
			}
			else
			{
				pSunDesc->NumPlanets = (uint8_t)~0;
			}

			FillOrbits(solarSys, pSunDesc->NumPlanets,
					   solarSys->PlanetDesc, false);

			if (uqm::isStarSeed())
			{
				GenerateGasGiantRanged(solarSys);
			}

			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			if (!uqm::isStarSeed())
			{
				pPlanet->data_index = GenerateWorlds(OnlyGas);
			}

			GeneratePlanets(solarSys);

			if (pPlanet->NumPlanets <= pSunDesc->MoonByte)
			{
				pPlanet->NumPlanets = pSunDesc->MoonByte + 1;
			}
		}
		else
		{
			GenerateDefault_generatePlanets(solarSys);
		}
	}

	return true;
}

static bool
GenerateUtwig_generateName(const SOLARSYS_STATE* solarSys,
						   const PLANET_DESC* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (CurStarDescPtr()->Index == UTWIG_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsHomeworldKnown(UTWIG_HOME))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const PLANET_DESC pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(PLANET_NUMBER_BASE + 40));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool
GenerateUtwig_generateOrbital(SOLARSYS_STATE* solarSys,
							  PLANET_DESC* world)
{
	if ((CurStarDescPtr()->Index == UTWIG_DEFINED
		 && matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
		|| (CurStarDescPtr()->Index == BOMB_DEFINED
			&& matchWorld(solarSys, world, MatchPbyte, MatchMbyte)
			&& !uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB)))
	{
		if ((CurStarDescPtr()->Index == UTWIG_DEFINED
			 || !uqm::gameState().flag(uqm::GameFlag::UTWIG_HAVE_ULTRON))
			&& StartSphereTracking(UTWIG_SHIP))
		{
			NotifyOthers(UTWIG_SHIP, IpnlAllClear);
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			CloneShipFragment(UTWIG_SHIP,
							  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);

			uqm::currentActivity() |= START_INTERPLANETARY;
			if (CurStarDescPtr()->Index == UTWIG_DEFINED)
			{
				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			}
			else
			{
				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
			}
			InitCommunication(UTWIG_CONVERSATION);

			if (!(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
			{
				uqm::currentActivity() &= ~START_INTERPLANETARY;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);
			}
			return true;
		}

		if (CurStarDescPtr()->Index == BOMB_DEFINED
			&& !uqm::gameState().flag(uqm::GameFlag::BOMB_UNPROTECTED)
			&& StartSphereTracking(DRUUGE_SHIP))
		{
			const uint16_t sum = difficultyCase(4, 5, 14);

			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			for (uint16_t i {0}; i < sum; ++i)
			{
				CloneShipFragment(DRUUGE_SHIP,
								  &uqm::gameState().game().npc_built_ship_q, 0);
			}
			uqm::currentActivity() |= START_INTERPLANETARY;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
			InitCommunication(DRUUGE_CONVERSATION);

			if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
			{
				return true;
			}

			{
				bool DruugeSurvivors {};

				DruugeSurvivors =
					GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0;

				uqm::currentActivity() &= ~START_INTERPLANETARY;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);

				if (DruugeSurvivors)
				{
					return true;
				}

				RepairSISBorder();
				uqm::gameState().setFlag(uqm::GameFlag::BOMB_UNPROTECTED, static_cast<uint32_t>(1));
			}
		}

		if (CurStarDescPtr()->Index == BOMB_DEFINED)
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(BombMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(BombStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
		}
		else
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	if (CurStarDescPtr()->Index == UTWIG_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& !isDifficulty(uqm::Difficulty::Hard))
	{
		solarSys->SysInfo.PlanetInfo.Weather = 1;
		solarSys->SysInfo.PlanetInfo.Tectonics = 1;
	}

	return true;
}

static uint16_t
GenerateUtwig_generateEnergy(const SOLARSYS_STATE* solarSys,
							 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	if (CurStarDescPtr()->Index == UTWIG_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	if (CurStarDescPtr()->Index == BOMB_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB))
		{ // already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateUtwig_pickupEnergy(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
						   uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == UTWIG_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);
		return false;
	}

	if (CurStarDescPtr()->Index == BOMB_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		assert(!uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB) && whichNode == 0);

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_MANNER, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_HOME_VISITS, static_cast<uint32_t>(0));

		return true; // picked up
	}

	(void)whichNode;
	return false;
}