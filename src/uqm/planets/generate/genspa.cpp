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
#include "../scan.h"
#include "../../build.h"
#include "../../comm.h"
#include "../../gamestr.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateSpathi_generatePlanets(SolarSystemState* solarSys);
static bool GenerateSpathi_generateMoons(SolarSystemState* solarSys,
										 SystemPlanet* planet);
static bool GenerateSpathi_generateName(const SolarSystemState*,
										const SystemPlanet* world);
static bool GenerateSpathi_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world);
static uint16_t GenerateSpathi_generateEnergy(const SolarSystemState*,
											  const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static uint16_t GenerateSpathi_generateLife(const SolarSystemState*,
											const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateSpathi_pickupEnergy(SolarSystemState* solarSys,
										SystemPlanet* world, uint16_t whichNode);
static bool GenerateSpathi_pickupLife(SolarSystemState* solarSys,
									  SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateSpathiFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSpathi_generatePlanets,
	/* .generateMoons    = */ GenerateSpathi_generateMoons,
	/* .generateName     = */ GenerateSpathi_generateName,
	/* .generateOrbital  = */ GenerateSpathi_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSpathi_generateEnergy,
	/* .generateLife     = */ GenerateSpathi_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSpathi_pickupEnergy,
	/* .pickupLife       = */ GenerateSpathi_pickupLife,
};


static bool GenerateSpathi_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	if (CurStarDescPtr()->Index == PlotId::Spathi)
	{
		pSunDesc->NumPlanets = 1;
		pSunDesc->PlanetByte = 0;
		pSunDesc->MoonByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		FillOrbits(solarSys, NumplanetsPdesc, nullptr, false);

		pPlanet->NumPlanets = 1;

		if (uqm::isOriginalSeed())
		{
			pPlanet->radius = EarthRadius * 1150L / 100;
			const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
			pPlanet->location.x = Cosine(angle, pPlanet->radius);
			pPlanet->location.y = Sine(angle, pPlanet->radius);
			pPlanet->data_index = *PlanetType::Water;
			ComputeSpeed(pPlanet, false, 1);
		}
		else
		{
			pPlanet->PlanetByte = PickClosestHabitable(solarSys);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateHabitableWorld();
		}

		if (uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES)
			&& !(IsExtended() && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)
				 && RaceDead(RaceId::Orz)))
		{
			pPlanet->data_index |= *PlanetType::PlanetShielded;
		}

		return true;
	}

	GenerateDefault_generatePlanets(solarSys);

	if (IsExtended())
	{
		if (CurStarDescPtr()->Index == PlotId::Algolites)
		{
			pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateHabitableWorld();
		}

		if (CurStarDescPtr()->Index == PlotId::SpathiMonument)
		{
			pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateCrystalWorld();
		}
	}

	return true;
}

static bool GenerateSpathi_generateMoons(SolarSystemState* solarSys,
										 SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (CurStarDescPtr()->Index == PlotId::Spathi
		&& matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		SystemPlanet* pMoonDesc = &solarSys->MoonDesc[MoonByte];

		if (uqm::isOriginalSeed())
		{
			uint16_t angle {};

			pMoonDesc->data_index = *PlanetType::Pellucid;
			pMoonDesc->radius = MinMoonRadius() + MoonDelta();
			angle = NormalizeAngle(
				uqm::lowWord(SysGenRNG->next()));
			pMoonDesc->location.x = Cosine(angle, pMoonDesc->radius);
			pMoonDesc->location.y = Sine(angle, pMoonDesc->radius);
			ComputeSpeed(pMoonDesc, true, 1);
		}
		else
		{
			pMoonDesc->data_index = GenerateWorlds(SmallRocky);
		}
	}

	return true;
}

static bool GenerateSpathi_generateName(const SolarSystemState* solarSys,
										const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (CurStarDescPtr()->Index == PlotId::Spathi
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsHomeworldKnown(*HomeId::Spathi))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(GameStringBase::PlanetNumber + 37));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateSpathi_generateOrbital(SolarSystemState* solarSys,
										   SystemPlanet* world)
{
	uint32_t rand_val {};

	if (CurStarDescPtr()->Index == PlotId::Spathi)
	{
		if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
		{ /* Spathiwa's moon */
			if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES)
				&& StartSphereTracking(RaceId::Spathi))
			{
				NotifyOthers(*RaceId::Spathi, IpnlAllClear);
				PutGroupInfo(GroupsRandom, GroupSaveIp);
				ReinitQueue(&uqm::gameState().game().ip_group_q);
				assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

				CloneShipFragment(RaceId::Spathi, &uqm::gameState().game().npc_built_ship_q,
								  InfiniteFleet);

				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
				uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
				InitCommunication(Conversation::Spathi);

				if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
				{
					uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
					ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
					GetGroupInfo(GroupsRandom, GroupLoadIp);
				}
				return true;
			}

			DoPlanetaryAnalysis(&solarSys->SysInfo, world);
			rand_val = SysGenRNG->seed();

			solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Biological] =
				rand_val;
			GenerateLifeForms(&solarSys->SysInfo, GenerateAll, nullptr);
			rand_val = SysGenRNG->seed();

			solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Mineral] = rand_val;
			GenerateMineralDeposits(
				&solarSys->SysInfo, GenerateAll, nullptr);

			solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Energy] = rand_val;


			if (!isDifficulty(uqm::Difficulty::Hard))
			{
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 28;
			}

			if (!uqm::gameState().flag(uqm::GameFlag::UMGAH_BROADCASTERS))
			{
				LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
				solarSys->PlanetSideFrame[1] =
					CaptureDrawable(
						LoadGraphic(UmgahBcsMaskPmapAnimRes));
				solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(UmgahBcsStrtabRes);
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
				if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES))
				{ // The first report talks extensively about Spathi
					// slave-shielding selves. If they never did so, the
					// report makes no sense, so use an alternate.
					solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 1;
				}
			}

			LoadPlanet(nullptr);
			return true;
		}
		else if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
		{
			/* visiting Spathiwa */
			DoPlanetaryAnalysis(&solarSys->SysInfo, world);
			rand_val = SysGenRNG->seed();

			solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Mineral] = rand_val;
			GenerateMineralDeposits(
				&solarSys->SysInfo, GenerateAll, nullptr);
			rand_val = SysGenRNG->seed();

			solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Biological] =
				rand_val;

			if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)
				&& uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES)
				&& RaceDead(RaceId::Orz)
				&& IsExtended())
			{
				solarSys->SysInfo.PlanetInfo.ScanSeed[*ScanType::Energy] =
					rand_val;

				LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
				solarSys->PlanetSideFrame[1] =
					CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
				solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
			}

			solarSys->SysInfo.PlanetInfo.PlanetRadius = 120;
			solarSys->SysInfo.PlanetInfo.SurfaceGravity = 23;
			if (!isDifficulty(uqm::Difficulty::Hard))
			{
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 31;
			}

			LoadPlanet(nullptr);

			return true;
		}
	}

	if (CurStarDescPtr()->Index == PlotId::Algolites
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(AlgoliteRuinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	if (CurStarDescPtr()->Index == PlotId::SpathiMonument
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(
				LoadGraphic(MonumentMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(SpathiMonumentStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	GenerateDefault_generateOrbital(solarSys, world);

	if (CurStarDescPtr()->Index == PlotId::Algolites
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
		solarSys->SysInfo.PlanetInfo.Weather = 0;
	}

	return true;
}

static uint16_t GenerateSpathi_generateEnergy(const SolarSystemState* solarSys,
											  const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (CurStarDescPtr()->Index == PlotId::Spathi)
	{
		if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
		{
			// This check is redundant since the retrieval bit will keep
			// the node from showing up again
			if (uqm::gameState().flag(uqm::GameFlag::UMGAH_BROADCASTERS))
			{ // already picked up
				return 0;
			}

			return GenerateDefault_generateArtifact(
				solarSys, whichNode, info);
		}
		else if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
				 && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY) && RaceDead(RaceId::Orz)
				 && uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES)
				 && IsExtended())
		{
			return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Energy, 4,
									   0, whichNode, info);
		}
	}

	if (CurStarDescPtr()->Index == PlotId::Algolites
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Energy, 6,
								   0, whichNode, info);
	}

	if (CurStarDescPtr()->Index == PlotId::SpathiMonument
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateSpathi_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
										uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == PlotId::Spathi)
	{
		if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
		{
			assert(!uqm::gameState().flag(uqm::GameFlag::UMGAH_BROADCASTERS) && whichNode == 0);

			GenerateDefault_landerReport(solarSys);
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::UMGAH_BROADCASTERS, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::UMGAH_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(1));

			return true; // picked up
		}
		else if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
				 && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY) && RaceDead(RaceId::Orz)
				 && uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES)
				 && IsExtended())
		{
			GenerateDefault_landerReportCycle(solarSys);

			return false; // do not remove the node
		}
	}

	if ((CurStarDescPtr()->Index == PlotId::Algolites
		 || CurStarDescPtr()->Index == PlotId::SpathiMonument)
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& IsExtended())
	{
		GenerateDefault_landerReportCycle(solarSys);

		return false; // do not remove the node
	}

	(void)whichNode;
	return false;
}

static constexpr auto NumEvilOnes {32};

static uint16_t GenerateSpathi_generateLife(const SolarSystemState* solarSys,
											const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (CurStarDescPtr()->Index == PlotId::Spathi
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Biological,
								   NumEvilOnes, EvilOne, whichNode, info);
	}

	return 0;
}

static bool GenerateSpathi_pickupLife(SolarSystemState* solarSys, SystemPlanet* world,
									  uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == PlotId::Spathi
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(!uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED) && !uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES));

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_CREATURES_EXAMINED, static_cast<uint32_t>(1));
		if (countNodesRetrieved(
				&solarSys->SysInfo.PlanetInfo, ScanType::Biological)
				+ 1
			== NumEvilOnes)
		{ // last creature picked up
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED, static_cast<uint32_t>(1));
		}

		return true; // picked up
	}

	return GenerateDefault_pickupLife(solarSys, world, whichNode);
}

} // namespace uqm