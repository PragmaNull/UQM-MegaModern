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
#include "../../gendef.h"
#include "../../grpinfo.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../../globdata.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
static bool GenerateRainbowWorld_initNpcs(SolarSystemState* solarSys);
static bool GenerateRainbowWorld_generatePlanets(
	SolarSystemState* solarSys);
static bool GenerateRainbowWorld_generateOrbital(SolarSystemState* solarSys,
												 SystemPlanet* world);
static void GenerateSlylandro(SolarSystemState* solarSys);

const GenerateFunctions generateRainbowWorldFunctions = {
	/* .initNpcs         = */ GenerateRainbowWorld_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateRainbowWorld_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateRainbowWorld_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};

static bool GenerateRainbowWorld_initNpcs(SolarSystemState* solarSys)
{
	if (isDifficulty(uqm::Difficulty::Hard) && uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_MULTIPLIER) > 0)
	{
		GenerateSlylandro(solarSys);
	}
	else
	{
		GenerateDefault_initNpcs(solarSys);
	}

	return true;
}

static bool GenerateRainbowWorld_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	pSunDesc->PlanetByte = 0;
	SystemPlanet* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	pPlanet->data_index = *PlanetType::Rainbow;

	if (uqm::isOriginalSeed())
	{
		pPlanet->NumPlanets = 0;
		pPlanet->radius = EarthRadius * 50L / 100;
		uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		if (angle <= Quadrant)
		{
			angle += Quadrant;
		}
		else if (angle >= FullCircle - Quadrant)
		{
			angle -= Quadrant;
		}
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}

	return true;
}

static bool GenerateRainbowWorld_generateOrbital(SolarSystemState* solarSys,
												 SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& CurStarDescPtr()->Index >= PlotId::Rainbow0
		&& CurStarDescPtr()->Index <= PlotId::Rainbow9)
	{
		uint16_t rainbow_mask {};

		rainbow_mask = uqm::makeWord(
			uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD0),
			uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD1));

		rainbow_mask |= 1 << (CurStarDescPtr()->Index - *PlotId::Rainbow0);
		uqm::gameState().setFlag(uqm::GameFlag::RAINBOW_WORLD0, static_cast<uint32_t>(uqm::lowByte(rainbow_mask)));
		uqm::gameState().setFlag(uqm::GameFlag::RAINBOW_WORLD1, static_cast<uint32_t>(uqm::highByte(rainbow_mask)));
	}

	GenerateDefault_generateOrbital(solarSys, world);
	return true;
}

static void GenerateSlylandro(SolarSystemState* solarSys)
{
	IpGroupHandle hGroup {};
	IpGroupHandle hNextGroup {};
	uint8_t angle {};
	uint8_t num_groups {};
	uint8_t which_group {};

	if (!GetGroupInfo(uqm::gameState().game().BattleGroupRef, GroupInitIp))
	{ // This code will run if we have no battle group generated
		// or all are expired
		uqm::gameState().game().BattleGroupRef = 0;
		/* 1-3, 3-5, 5-7, 7-9 Probes total */
		num_groups = (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_MULTIPLIER) * 2) - 1 + (uint16_t)uqm::random().next() % 3;
		which_group = 0;
		do
		{
			CloneShipFragment(RaceId::Slylandro,
							  &uqm::gameState().game().npc_built_ship_q, 0);
			PutGroupInfo(GroupsRandom, ++which_group);
			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
		} while (--num_groups);

		GetGroupInfo(GroupsRandom, GroupInitIp);
	}
	// Fresh groups or not - force probes to rotate around rainbow
	// world and not spread around the system
	angle = (uint16_t)uqm::random().next() % 9; // Initial angle = 0 - Octant
	for (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q);
		 hGroup; hGroup = hNextGroup)
	{
		InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);

		GroupPtr->task = *GroupTask::InOrbit;
		GroupPtr->sys_loc = solarSys->SunDesc[0].PlanetByte + 1;
		GroupPtr->dest_loc = GroupPtr->sys_loc;
		GroupPtr->orbit_pos =
			NormalizeFacing(AngleToFacing(angle));
		GroupPtr->group_counter = 0;
		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);

		// Next ship in queue will add random value to its angle
		// between Octant and HalfCircle
		angle += ((uint16_t)uqm::random().next() % 25) + Octant;

		// Normalize angle
		if (angle > FullCircle)
		{
			angle -= FullCircle;
		}
	}
}

} // namespace uqm
