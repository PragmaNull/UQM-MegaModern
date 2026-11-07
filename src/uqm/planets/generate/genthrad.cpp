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
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../lander.h"
#include "../planets.h"
#include "genall.h"


namespace uqm
{
static bool GenerateThraddash_generatePlanets(SolarSystemState* solarSys);
static bool GenerateThraddash_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world);
static uint16_t GenerateThraddash_generateEnergy(const SolarSystemState*,
												 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateThraddash_pickupEnergy(SolarSystemState* solarSys,
										   SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateThraddashFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateThraddash_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateThraddash_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateThraddash_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateThraddash_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateThraddash_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		uint16_t angle {};

		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (CurStarDescPtr()->Index == PlotId::AquaHelix)
		{
			pPlanet->data_index = *PlanetType::Primordial;
			pPlanet->radius = EarthRadius * 65L / 100;
			angle = ARCTAN(pPlanet->location.x, pPlanet->location.y);
			pPlanet->location.x = Cosine(angle, pPlanet->radius);
			pPlanet->location.y = Sine(angle, pPlanet->radius);
		}
		else /* CurStarDescPtr()->Index == THRADD_DEFINED */
		{
			pPlanet->data_index = *PlanetType::Water;
			pPlanet->NumPlanets = 0;
			pPlanet->radius = EarthRadius * 98L / 100;
			angle = ARCTAN(pPlanet->location.x, pPlanet->location.y);
			pPlanet->location.x = Cosine(angle, pPlanet->radius);
			pPlanet->location.y = Sine(angle, pPlanet->radius);
		}

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

static bool GenerateThraddash_generateOrbital(SolarSystemState* solarSys,
											  SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (StartSphereTracking(RaceId::Thraddash)
			&& (CurStarDescPtr()->Index == PlotId::Thradd
				|| (!uqm::gameState().flag(uqm::GameFlag::HELIX_UNPROTECTED)
					&& (uint8_t)(uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION) - 1) >= 3)))
		{
			NotifyOthers(*RaceId::Thraddash, IpnlAllClear);
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			CloneShipFragment(RaceId::Thraddash, &uqm::gameState().game().npc_built_ship_q,
							  InfiniteFleet);

			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			if (CurStarDescPtr()->Index == PlotId::Thradd)
			{
				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			}
			else
			{
				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
			}
			InitCommunication(Conversation::Thradd);

			if (uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
			{
				return true;
			}

			uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
			GetGroupInfo(GroupsRandom, GroupLoadIp);

			if (CurStarDescPtr()->Index == PlotId::Thradd
				|| (!uqm::gameState().flag(uqm::GameFlag::HELIX_UNPROTECTED)
					&& (uint8_t)(uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION) - 1) >= 3))
			{
				return true;
			}

			RepairSISBorder();
			// reachable if you're frendly with thraddsh and talk to
			// them at helix world
		}
		else if (isDifficulty(uqm::Difficulty::Hard)
				 && CurStarDescPtr()->Index == PlotId::AquaHelix
				 && !uqm::gameState().flag(uqm::GameFlag::HELIX_UNPROTECTED)
				 && !(uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS)
					  & 1 << *HardModeEncounter::Thraddash)
				 && (StartSphereTracking(RaceId::Thraddash)
					 || !(uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))))
		{
			uint16_t sum {};

			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			if (!StartSphereTracking(RaceId::Thraddash)
				|| uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
			{
				sum = 6;
			}
			else
			{
				sum = 12;
			}

			for (uint16_t i {0}; i < sum; ++i)
			{
				CloneShipFragment(RaceId::Thraddash,
								  &uqm::gameState().game().npc_built_ship_q, 0);
			}

			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			InitCommunication(Conversation::Thradd);

			if (uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
			{
				return true;
			}

			{
				const bool Survivors =
					GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0;

				uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);

				if (Survivors && !uqm::gameState().flag(uqm::GameFlag::HELIX_UNPROTECTED))
				{
					return true;
				}

				{
					uint16_t state {};

					state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);

					state |= 1 << *HardModeEncounter::Thraddash;

					uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));
				}

				RepairSISBorder();
			}
		}

		if (CurStarDescPtr()->Index == PlotId::AquaHelix
			&& !uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX))
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(AquaMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(AquaStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
		}
		else if (CurStarDescPtr()->Index == PlotId::Thradd)
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);
	return true;
}

static uint16_t GenerateThraddash_generateEnergy(const SolarSystemState* solarSys,
												 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (CurStarDescPtr()->Index == PlotId::Thradd
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	if (CurStarDescPtr()->Index == PlotId::AquaHelix
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX))
		{ // already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateThraddash_pickupEnergy(SolarSystemState* solarSys,
										   SystemPlanet* world, uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == PlotId::Thradd
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);
		return false;
	}

	if (CurStarDescPtr()->Index == PlotId::AquaHelix
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		FleetInfoHandle hThradd =
			GetStarShipFromIndex(
				&uqm::gameState().game().avail_race_q, *RaceId::Thraddash);
		FleetInfo* ThraddPtr =
			LockFleetInfo(&uqm::gameState().game().avail_race_q, hThradd);
		int16_t strength_loss {};

		assert(!uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX) && whichNode == 0);

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		uqm::gameState().setFlag(uqm::GameFlag::HELIX_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::AQUA_HELIX, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::AQUA_HELIX_ON_SHIP, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::HELIX_UNPROTECTED, static_cast<uint32_t>(1));
		if (IsExtended() && ThraddPtr->allied_state == RaceAllegiance::Good
			&& uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
		{
			SetRaceAllied(RaceId::Thraddash, false);
			RemoveEscortShips(RaceId::Thraddash);
			strength_loss = (int16_t)(ThraddPtr->actual_strength);
			ThraddPtr->growth =
				(uint8_t)(-strength_loss / ThraddPtr->days_left);
			ThraddPtr->growth_fract =
				(uint8_t)(((strength_loss % ThraddPtr->days_left) << 8)
						  / ThraddPtr->days_left);
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(0));
		}

		return true; // picked up
	}

	(void)whichNode;
	return false;
}

} // namespace uqm