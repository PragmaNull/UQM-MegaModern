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
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../lander.h"
#include "../planets.h"
#include "genall.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
static bool GenerateChmmr_generatePlanets(SolarSystemState* solarSys);
static bool GenerateChmmr_generateMoons(SolarSystemState* solarSys,
										SystemPlanet* planet);
static bool GenerateChmmr_generateOrbital(SolarSystemState* solarSys,
										  SystemPlanet* world);
static uint16_t GenerateChmmr_generateEnergy(const SolarSystemState*,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateChmmr_pickupEnergy(SolarSystemState* solarSys,
									   SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateChmmrFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateChmmr_generatePlanets,
	/* .generateMoons    = */ GenerateChmmr_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateChmmr_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateChmmr_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateChmmr_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateChmmr_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pPlanet {};
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	if (CurStarDescPtr()->Index == PlotId::Chmmr)
	{
		pSunDesc->PlanetByte = 1;
		pSunDesc->MoonByte = 0;

		if (uqm::isOriginalSeed())
		{
			GenerateDefault_generatePlanets(solarSys);

			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = *PlanetType::Sapphire;
			pPlanet->NumPlanets = 1;
		}
		else
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
				pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
			}

			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateCrystalWorld();

			if (!pPlanet->NumPlanets)
			{
				pPlanet->NumPlanets++;
			}
		}

		if (!uqm::gameState().flag(uqm::GameFlag::CHMMR_UNLEASHED))
		{
			pPlanet->data_index |= *PlanetType::PlanetShielded;
		}
	}

	if (CurStarDescPtr()->Index == PlotId::MotherArk)
	{
		GenerateDefault_generatePlanets(solarSys);

		pSunDesc->PlanetByte = 3;

		if (!uqm::isOriginalSeed())
		{
			pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
		}

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (IsExtended())
		{
			pPlanet->data_index = GenerateCrystalWorld();
		}
	}

	return true;
}

static bool GenerateChmmr_generateMoons(SolarSystemState* solarSys, SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (CurStarDescPtr()->Index == PlotId::Chmmr
		&& matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		uint16_t angle {};
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		SystemPlanet* pMoonDesc = &solarSys->MoonDesc[MoonByte];

		if (!RaceDead(RaceId::Chmmr))
		{
			pMoonDesc->data_index = *PlanetType::HierarchyStarbase;
		}
		else
		{
			pMoonDesc->data_index = *PlanetType::DestroyedStarbase;
		}

		pMoonDesc->radius = MinMoonRadius();
		const uint32_t rand_val {SysGenRNG->next()};
		angle = NormalizeAngle(uqm::lowWord(rand_val));
		pMoonDesc->location.x = Cosine(angle, pMoonDesc->radius);
		pMoonDesc->location.y = Sine(angle, pMoonDesc->radius);
		ComputeSpeed(pMoonDesc, true, 1);
	}

	return true;
}

static bool GenerateChmmr_generateOrbital(SolarSystemState* solarSys,
										  SystemPlanet* world)
{
	if (CurStarDescPtr()->Index == PlotId::Chmmr
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const bool HardModeBS = isDifficulty(uqm::Difficulty::Hard) && !uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)
							 && !(uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << *HardModeEncounter::Ilwrath);

		if (RaceDead(RaceId::Chmmr))
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] = CaptureDrawable(
				LoadGraphic(RuinsMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(ChmmrHomeStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

			GenerateDefault_generateOrbital(solarSys, world);

			return true;
		}

		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_UNLEASHED))
		{
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			InitCommunication(Conversation::Chmmr);

			if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 2)
			{
				uqm::currentActivity() |= EndInterplanetary;
			}

			return true;
		}
		else if (uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE_ON_SHIP)
				 && ((!uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED)
					  && StartSphereTracking(RaceId::Ilwrath))
					 || HardModeBS))
		{
			uint16_t state {};

			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED) && isDifficulty(uqm::Difficulty::Hard))
			{
				uint16_t lim {};

				if (StartSphereTracking(RaceId::Ilwrath))
				{
					lim = 14;
				}
				else
				{
					lim = 6;
				}

				for (uint16_t i {0}; i < lim; ++i)
				{
					CloneShipFragment(RaceId::Ilwrath,
									  &uqm::gameState().game().npc_built_ship_q, 0);
				}
			}
			else
			{
				CloneShipFragment(RaceId::Ilwrath,
								  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);
			}

			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			InitCommunication(Conversation::Ilwrath);

			if (uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
			{
				return true;
			}

			const bool Survivors {GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0};

			uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
			GetGroupInfo(GroupsRandom, GroupLoadIp);

			if (Survivors)
			{
				return true;
			}

			state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);
			state |= 1 << *HardModeEncounter::Ilwrath;
			uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));

			RepairSISBorder();
		}
	}

	if (CurStarDescPtr()->Index == PlotId::Chmmr
		&& matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		/* Starbase */
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(
			RaceDead(RaceId::Chmmr) ? UrquanBaseStrtabRes : ChmmrBaseStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

		DoDiscoveryReport(MenuSounds);

		solarSys->SysInfo.PlanetInfo.DiscoveryStrings.reset();
		FreeLanderFont(&solarSys->SysInfo.PlanetInfo);

		return true;
	}

	if (IsExtended() && CurStarDescPtr()->Index == PlotId::MotherArk
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const bool MelnormeInfo =
			uqm::gameState().flag(uqm::GameFlag::MELNORME_ALIEN_INFO_STACK) >= 8;
		const bool ChmmrStack = uqm::gameState().flag(uqm::GameFlag::CHMMR_HOME_VISITS);

		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(MotherArkMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(MotherArkStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

		if (MelnormeInfo || ChmmrStack)
		{
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex += 1;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

static uint16_t GenerateChmmr_generateEnergy(const SolarSystemState* solarSys,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{

	if (CurStarDescPtr()->Index == PlotId::Chmmr
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// Standard ruins report
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	if (CurStarDescPtr()->Index == PlotId::MotherArk
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateChmmr_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
									   uint16_t whichNode)
{

	if (CurStarDescPtr()->Index == PlotId::Chmmr
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);
		return false;
	}

	if (IsExtended() && CurStarDescPtr()->Index == PlotId::MotherArk
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(whichNode == 0);

		GenerateDefault_landerReport(solarSys);

		// The Ark cannot be "picked up". It is always on the surface.
		return false;
	}

	(void)whichNode;
	return false;
}

} // namespace uqm