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


static bool GenerateVault_generatePlanets(SOLARSYS_STATE* solarSys);
static bool GenerateVault_generateOrbital(SOLARSYS_STATE* solarSys,
										  PLANET_DESC* world);
static uint16_t GenerateVault_generateEnergy(const SOLARSYS_STATE*,
											 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO*);
static bool GenerateVault_pickupEnergy(SOLARSYS_STATE* solarSys,
									   PLANET_DESC* world, uint16_t whichNode);


const GenerateFunctions generateVaultFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateVault_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateVault_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateVault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateVault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};

static bool GenerateVault_generatePlanets(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	pSunDesc->PlanetByte = 0;
	pSunDesc->MoonByte = 0;

	PLANET_DESC* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	if (uqm::isStarSeed())
	{
		pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->NumPlanets =
			SysGenRNG->seed() % 5 == 0 ? 2 : 1;

		pSunDesc->MoonByte = PlanetByteGen(pPlanet);
	}

	if (!uqm::isOriginalSeed())
	{
		pPlanet->data_index = GenerateWorlds(LargeRocky);

		if (!pPlanet->NumPlanets)
		{
			pPlanet->NumPlanets++;
		}
	}

	return true;
}

static bool GenerateVault_generateOrbital(SOLARSYS_STATE* solarSys,
										  PLANET_DESC* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(VaultMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(VaultStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
		if (uqm::gameState().flag(uqm::GameFlag::SHIP_VAULT_UNLOCKED))
		{
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 2;
		}
		else if (uqm::gameState().flag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP))
		{
			if (isDifficulty(uqm::Difficulty::Hard) && !(uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << URQUAN_ENCOUNTER)
				&& !(uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)))
			{
				uint16_t state {};

				PutGroupInfo(GroupsRandom, GroupSaveIp);
				ReinitQueue(&uqm::gameState().game().ip_group_q);
				assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

				for (uint16_t i {0}; i < 6; ++i)
				{
					CloneShipFragment(URQUAN_SHIP,
									  &uqm::gameState().game().npc_built_ship_q, 0);
				}

				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
				uqm::currentActivity() |= START_INTERPLANETARY;
				InitCommunication(URQUAN_CONVERSATION);

				if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
				{
					return true;
				}

				const bool Survivors {GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0};

				uqm::currentActivity() &= ~START_INTERPLANETARY;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);

				if (Survivors)
				{
					return true;
				}

				state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);
				state |= 1 << URQUAN_ENCOUNTER;

				uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));

				RepairSISBorder();
			}
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 1;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);
	return true;
}

static uint16_t GenerateVault_generateEnergy(const SOLARSYS_STATE* solarSys,
											 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateVault_pickupEnergy(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
									   uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		assert(whichNode == 0);

		if (uqm::gameState().flag(uqm::GameFlag::SHIP_VAULT_UNLOCKED))
		{ // Give the final report, "omg empty" and whatnot
			GenerateDefault_landerReportCycle(solarSys);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP))
		{
			GenerateDefault_landerReportCycle(solarSys);
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::SHIP_VAULT_UNLOCKED, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SYREEN_HOME_VISITS, static_cast<uint32_t>(0));
		}
		else
		{
			GenerateDefault_landerReport(solarSys);

			if (!uqm::gameState().flag(uqm::GameFlag::KNOW_SYREEN_VAULT))
			{
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_SYREEN_VAULT, static_cast<uint32_t>(1));
			}
		}

		// The Vault cannot be "picked up". It is always on the surface.
		return false;
	}

	(void)whichNode;
	return false;
}