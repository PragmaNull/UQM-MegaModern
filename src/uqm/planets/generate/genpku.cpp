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
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


static bool GeneratePkunk_generatePlanets(SOLARSYS_STATE* solarSys);
static bool GeneratePkunk_generateOrbital(SOLARSYS_STATE* solarSys,
										  PLANET_DESC* world);
static uint16_t GeneratePkunk_generateEnergy(const SOLARSYS_STATE*,
											 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO*);
static bool GeneratePkunk_pickupEnergy(SOLARSYS_STATE* solarSys,
									   PLANET_DESC* world, uint16_t whichNode);


const GenerateFunctions generatePkunkFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GeneratePkunk_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GeneratePkunk_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GeneratePkunk_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GeneratePkunk_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GeneratePkunk_generatePlanets(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];

	solarSys->SunDesc[0].PlanetByte = 0;
	PLANET_DESC* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pPlanet->data_index = WATER_WORLD;
		pPlanet->NumPlanets = 1;
		pPlanet->radius = EarthRadius * 104L / 100;
		const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		pPlanet->location.x = COSINE(angle, pPlanet->radius);
		pPlanet->location.y = SINE(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}
	else
	{
		if (uqm::isStarSeed())
		{
			pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];
		}
		else
		{
			CheckForHabitable(solarSys);
		}

		pPlanet->data_index = GenerateHabitableWorld();
	}

	return true;
}

static bool
GeneratePkunk_generateOrbital(SOLARSYS_STATE* solarSys, PLANET_DESC* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (StartSphereTracking(PKUNK_SHIP))
		{
			NotifyOthers(PKUNK_SHIP, IpnlAllClear);
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&GLOBAL(ip_group_q));
			assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

			CloneShipFragment(PKUNK_SHIP,
							  &GLOBAL(npc_built_ship_q), INFINITE_FLEET);

			uqm::currentActivity() |= START_INTERPLANETARY;
			SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 7);
			InitCommunication(PKUNK_CONVERSATION);

			if (!(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
			{
				uqm::currentActivity() &= ~START_INTERPLANETARY;
				ReinitQueue(&GLOBAL(npc_built_ship_q));
				GetGroupInfo(GroupsRandom, GroupLoadIp);
			}
			return true;
		}
		else
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(PkunkRuinsStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
			if (GET_GAME_STATE(CLEAR_SPINDLE))
			{ // Already picked up the Clear Spindle, skip the report
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 1;
			}
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

static bool
GeneratePkunk_pickupEnergy(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
						   uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		GenerateDefault_landerReportCycle(solarSys);

		// The artifact can be picked up from any ruin
		if (!GET_GAME_STATE(CLEAR_SPINDLE))
		{ // Just picked up the Clear Spindle from a ruin
			SetLanderTakeoff();

			SET_GAME_STATE(CLEAR_SPINDLE, 1);
			SET_GAME_STATE(CLEAR_SPINDLE_ON_SHIP, 1);
		}

		return false; // do not remove the node
	}

	(void)whichNode;
	return false;
}

static uint16_t
GeneratePkunk_generateEnergy(const SOLARSYS_STATE* solarSys,
							 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}