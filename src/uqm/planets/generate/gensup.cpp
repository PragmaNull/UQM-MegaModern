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
#include "../../gamestr.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../state.h"
#include "../lander.h"
#include "../planets.h"
#include "genall.h"


namespace uqm
{
static bool GenerateSupox_generatePlanets(SolarSystemState* solarSys);
static bool GenerateSupox_generateName(const SolarSystemState*,
									   const SystemPlanet* world);
static bool GenerateSupox_generateOrbital(SolarSystemState* solarSys,
										  SystemPlanet* world);
static uint16_t GenerateSupox_generateEnergy(const SolarSystemState*,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateSupox_pickupEnergy(SolarSystemState* solarSys,
									   SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateSupoxFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSupox_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateSupox_generateName,
	/* .generateOrbital  = */ GenerateSupox_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSupox_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSupox_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateSupox_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = *PlanetType::Water;
		pPlanet->NumPlanets = 2;
		pPlanet->radius = EarthRadius * 152L / 100;
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

static bool GenerateSupox_generateName(const SolarSystemState* solarSys,
									   const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& uqm::gameState().flag(uqm::GameFlag::SUPOX_STACK1) > 2)
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName.data(),
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(GameStringBase::PlanetNumber + 38));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateSupox_generateOrbital(SolarSystemState* solarSys,
										  SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (StartSphereTracking(RaceId::Supox))
		{
			NotifyOthers(*RaceId::Supox, IpnlAllClear);
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			CloneShipFragment(RaceId::Supox, &uqm::gameState().game().npc_built_ship_q,
							  InfiniteFleet);

			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			InitCommunication(Conversation::Supox);

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
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(SupoxRuinsStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
			if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION))
			{ // Already picked up the Ultron, skip the report
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 1;
			}
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

static bool GenerateSupox_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
									   uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		GenerateDefault_landerReportCycle(solarSys);

		// The artifact can be picked up from any ruin
		if (!uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION))
		{ // Just picked up the Ultron from a ruin
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(1));
		}

		return false; // do not remove the node
	}

	(void)whichNode;
	return false;
}

static uint16_t GenerateSupox_generateEnergy(const SolarSystemState* solarSys,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}

} // namespace uqm