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
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "libs/mathlib.h"

#include <string.h>
#include "uqm/random/UQMRandom.h"


static bool GenerateDruuge_generatePlanets(SolarSystemState* solarSys);
static bool GenerateDruuge_generateName(const SolarSystemState*,
										const SystemPlanet* world);
static bool GenerateDruuge_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world);
static uint16_t GenerateDruuge_generateEnergy(const SolarSystemState*,
											  const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateDruuge_pickupEnergy(SolarSystemState* solarSys,
										SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateDruugeFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateDruuge_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDruuge_generateName,
	/* .generateOrbital  = */ GenerateDruuge_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDruuge_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDruuge_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateDruuge_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pPlanet {};
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		memmove(&solarSys->PlanetDesc[1], &solarSys->PlanetDesc[0],
				sizeof(solarSys->PlanetDesc[0]) * pSunDesc->NumPlanets);
		++pSunDesc->NumPlanets;

		pPlanet->data_index = *PlanetType::Dust;
		pPlanet->radius = EarthRadius * 50L / 100;
		pPlanet->NumPlanets = 0;
		const uint16_t angle {HalfCircle - Octant};
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		pPlanet->rand_seed =
			uqm::makeDword(pPlanet->location.x, pPlanet->location.y);
		ComputeSpeed(pPlanet, false, 1);
	}
	else
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateDesolateWorld();
	}

	return true;
}

static bool GenerateDruuge_generateName(const SolarSystemState* solarSys,
										const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsHomeworldKnown(*HomeId::Druuge))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(GameStringBase::PlanetNumber + 41));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateDruuge_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (StartSphereTracking(RaceId::Druuge))
		{
			NotifyOthers(*RaceId::Druuge, IpnlAllClear);
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			CloneShipFragment(RaceId::Druuge,
							  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);

			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			InitCommunication(Conversation::Druuge);
			if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
			{
				uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);
			}
			return true;
		}
		else
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(DruugeRuinsStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
			if (uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
			{ // Already picked up Rosy Sphere, skip the report
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 1;
			}
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& !isDifficulty(uqm::Difficulty::Hard))
	{
		solarSys->SysInfo.PlanetInfo.Weather = 3;
		solarSys->SysInfo.PlanetInfo.Tectonics = 2;
	}

	return true;
}

static bool GenerateDruuge_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
										uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		GenerateDefault_landerReportCycle(solarSys);

		// The artifact can be picked up from any ruin
		if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
		{ // Just picked up the Rosy Sphere from a ruin
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(1));
		}

		return false; // do not remove the node
	}

	(void)whichNode;
	return false;
}

static uint16_t GenerateDruuge_generateEnergy(const SolarSystemState* solarSys,
											  const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}