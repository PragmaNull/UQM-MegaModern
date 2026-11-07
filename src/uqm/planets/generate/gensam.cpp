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
#include "../../encount.h"
#include "../../gamestr.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateSaMatra_initNpcs(SolarSystemState* solarSys);
static bool GenerateSaMatra_reinitNpcs(SolarSystemState* solarSys);
static bool GenerateSaMatra_generatePlanets(SolarSystemState* solarSys);
static bool GenerateSaMatra_generateMoons(SolarSystemState* solarSys,
										  SystemPlanet* planet);
static bool GenerateSaMatra_generateOrbital(SolarSystemState* solarSys,
											SystemPlanet* world);
static uint16_t GenerateSaMatra_generateEnergy(
	const SolarSystemState* solarSys, const SystemPlanet* world,
	uint16_t whichNode, SurfaceNode* info);
static bool GenerateSaMatra_pickupEnergy(SolarSystemState* solarSys,
										 SystemPlanet* world, uint16_t whichNode);

static void BuildUrquanGuard(SolarSystemState* solarSys);


const GenerateFunctions generateSaMatraFunctions = {
	/* .initNpcs         = */ GenerateSaMatra_initNpcs,
	/* .reinitNpcs       = */ GenerateSaMatra_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSaMatra_generatePlanets,
	/* .generateMoons    = */ GenerateSaMatra_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateSaMatra_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSaMatra_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSaMatra_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateSaMatra_initNpcs(SolarSystemState* solarSys)
{
	if (CurStarDescPtr()->Index == PlotId::Samatra)
	{
		if (!uqm::gameState().flag(uqm::GameFlag::URQUAN_MESSED_UP))
		{
			BuildUrquanGuard(solarSys);
		}
		else
		{ // Exorcise Ur-Quan ghosts upon system reentry
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			// wipe out the group
		}
	}

	if (SpaceMusicOK)
	{
		findRaceSOI();
	}

	(void)solarSys;
	return true;
}

static bool GenerateSaMatra_reinitNpcs(SolarSystemState* solarSys)
{
	bool GuardEngaged {};
	IpGroupHandle hGroup {};
	IpGroupHandle hNextGroup {};

	if (CurStarDescPtr()->Index == PlotId::Samatra)
	{
		GetGroupInfo(GroupsRandom, GroupLoadIp);
		EncounterGroup = 0;
		EncounterRace = -1;
		// Do not want guards to chase the player

		GuardEngaged = false;
		for (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q);
			 hGroup; hGroup = hNextGroup)
		{
			InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
			hNextGroup = _GetSuccLink(GroupPtr);

			if (uqm::gameState().flag(uqm::GameFlag::URQUAN_MESSED_UP))
			{
				GroupPtr->task &= GroupTaskFlag::ReformGroup;
				GroupPtr->task |= *GroupTask::Flee | GroupTaskFlag::IgnoreFlagship;
				GroupPtr->dest_loc = 0;
			}
			else if (GroupPtr->task & GroupTaskFlag::ReformGroup)
			{
				// REFORM_GROUP was set in ipdisp.c:ip_group_collision
				// during a collision with the flagship.
				GroupPtr->task &= ~GroupTaskFlag::ReformGroup;
				GroupPtr->group_counter = 0;

				GuardEngaged = true;
			}

			UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
		}

		if (GuardEngaged)
		{
			uint16_t angle {};
			GfxPoint org;

			org = planetOuterLocation(solarSys->SunDesc[0].PlanetByte);
			angle = ARCTAN(uqm::flagship().ipLocation.x - org.x,
						   uqm::flagship().ipLocation.y - org.y);
			uqm::flagship().ipLocation.x = org.x + Cosine(angle, 3000);
			uqm::flagship().ipLocation.y = org.y + Sine(angle, 3000);
			XFormIPLoc(&uqm::flagship().ipLocation,
					   &uqm::flagship().shipStamp.origin, true);
		}
	}

	(void)solarSys;
	return true;
}

static bool GenerateSaMatra_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	SystemPlanet* pPlanet {};

	GenerateDefault_generatePlanets(solarSys);

	if (CurStarDescPtr()->Index == PlotId::Samatra)
	{
		pSunDesc->PlanetByte = 4;
		pSunDesc->MoonByte = 0;

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (uqm::isOriginalSeed())
		{
			pPlanet->NumPlanets = 1;
		}
		else
		{
			pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			if (!pPlanet->NumPlanets)
			{
				pPlanet->NumPlanets++;
			}

			pSunDesc->MoonByte = PlanetByteGen(pPlanet);

			pPlanet->data_index = GenerateWorlds(LargeRocky);
		}
	}

	if (CurStarDescPtr()->Index == PlotId::DestroyedStarbase
		&& IsExtended())
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld();

		if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)
			&& (RaceDead(RaceId::Arilou) || uqm::isStarSeed()))
		{
			pPlanet->data_index |= *PlanetType::PlanetShielded;
		}

		if (!pPlanet->NumPlanets)
		{
			pPlanet->NumPlanets++;
		}

		pSunDesc->MoonByte = PlanetByteGen(pPlanet);
	}

	if ((CurStarDescPtr()->Index >= PlotId::Urquan0
		 && CurStarDescPtr()->Index <= PlotId::Kohrah2)
		&& IsExtended())
	{
		uint8_t i {};

		for (i = 0; i < pSunDesc->NumPlanets; i++)
		{
			if (solarSys->PlanetDesc[i].NumPlanets <= 1)
			{
				break;
			}
		}

		pSunDesc->PlanetByte = i;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (!pPlanet->NumPlanets)
		{
			const uint32_t rand = SysGenRNG->seed();

			pPlanet->NumPlanets = uqm::rangeMinMax(1, 4, rand);
		}

		if (pPlanet->NumPlanets > 1)
		{
			pPlanet->data_index = GenerateWorlds(OnlyLarge);
			pSunDesc->MoonByte = PlanetByteGen(pPlanet);
		}
		else
		{
			pSunDesc->MoonByte = 0;
		}
	}

	return true;
}

static bool GenerateSaMatra_generateMoons(SolarSystemState* solarSys,
										  SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		uint16_t angle {};
		uint32_t rand_val {};
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		SystemPlanet* pMoonDesc = &solarSys->MoonDesc[MoonByte];

		if (CurStarDescPtr()->Index == PlotId::Samatra)
		{
			pMoonDesc->data_index = *PlanetType::SaMatra;

			if (uqm::isOriginalSeed())
			{
				pMoonDesc->radius = MinMoonRadius() + (2 * MoonDelta());
				rand_val = SysGenRNG->next();
				angle = NormalizeAngle(uqm::lowWord(rand_val));
				pMoonDesc->location.x = Cosine(angle, pMoonDesc->radius);
				pMoonDesc->location.y = Sine(angle, pMoonDesc->radius);
				ComputeSpeed(pMoonDesc, true, 1);
			}
		}

		if ((CurStarDescPtr()->Index == PlotId::DestroyedStarbase
			 || ((CurStarDescPtr()->Index >= PlotId::Urquan0)
				 && CurStarDescPtr()->Index <= PlotId::Kohrah2))
			&& IsExtended())
		{
			pMoonDesc->data_index = *PlanetType::DestroyedStarbase;
		}
	}

	return true;
}

static bool GenerateSaMatra_generateOrbital(SolarSystemState* solarSys,
											SystemPlanet* world)
{
	/* Samatra */
	if (matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		if (CurStarDescPtr()->Index == PlotId::Samatra)
		{
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			if (!uqm::gameState().flag(uqm::GameFlag::URQUAN_MESSED_UP))
			{
				CloneShipFragment((IsExtended()
										   && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY) ?
									   RaceId::BlackUrquan :
									   RaceId::Urquan),
								  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);
			}
			else
			{
				const uint8_t urQuanRemants {difficultyCase<uint8_t>(2, 3, 5)};

				for (uint8_t i = 0; i < urQuanRemants; ++i)
				{
					CloneShipFragment((IsExtended()
											   && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY) ?
										   RaceId::BlackUrquan :
										   RaceId::Urquan),
									  &uqm::gameState().game().npc_built_ship_q, 0);
					CloneShipFragment(RaceId::BlackUrquan,
									  &uqm::gameState().game().npc_built_ship_q, 0);
				}
			}

			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			uqm::gameState().setFlag(uqm::GameFlag::URQUAN_PROTECTING_SAMATRA, static_cast<uint32_t>(1));
			if (IsExtended() && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
			{
				InitCommunication(Conversation::Blackurq);
			}
			else
			{
				InitCommunication(Conversation::Urquan);
			}

			if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
			{
				bool UrquanSurvivors {};

				UrquanSurvivors =
					GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0;

				uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
				ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
				GetGroupInfo(GroupsRandom, GroupLoadIp);
				if (UrquanSurvivors)
				{
					uqm::gameState().setFlag(uqm::GameFlag::URQUAN_PROTECTING_SAMATRA, static_cast<uint32_t>(0));
				}
				else
				{
					if (isDifficulty(uqm::Difficulty::Hard) && (uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << *HardModeEncounter::NoHelpFromPkunk))
					{
						return true;
					}

					EncounterGroup = 0;
					EncounterRace = -1;
					uqm::currentActivity() =
						*ActivityKind::InLastBattle | ActivityFlag::StartEncounter;
					if (uqm::gameState().flag(uqm::GameFlag::YEHAT_CIVIL_WAR)
						&& StartSphereTracking(RaceId::Yehat)
						&& EscortFeasibilityStudy(RaceId::YehatRebel))
					{
						InitCommunication(Conversation::YehatRebel);
					}
				}
			}
			return true;
		}

		if (IsExtended())
		{
			const uint16_t Index = CurStarDescPtr()->Index;
			if (((Index >= PlotId::Urquan0 && Index <= PlotId::Kohrah2))
				|| Index == PlotId::DestroyedStarbase)
			{

				/* Starbase */
				LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

				if (Index == PlotId::DestroyedStarbase)
				{
					solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(DestroyedBaseStrtabRes);
					solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

					// use alternate text if the player
					// hasn't freed the Earth starbase yet
					if (!uqm::gameState().flag(uqm::GameFlag::STARBASE_AVAILABLE))
					{
						solarSys->SysInfo.PlanetInfo.DiscoveryIndex += 1;
					}
				}
				else
				{
					solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(UrquanBaseStrtabRes);
					solarSys->SysInfo.PlanetInfo.DiscoveryIndex = Index >= PlotId::Kohrah0 && Index <= PlotId::Kohrah2 ? 1 : 0;
				}

				DoDiscoveryReport(MenuSounds);

				solarSys->SysInfo.PlanetInfo.DiscoveryStrings.reset();
				FreeLanderFont(&solarSys->SysInfo.PlanetInfo);

				return true;
			}
		}
	}
	else if (CurStarDescPtr()->Index == PlotId::DestroyedStarbase
			 && matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
			 && IsExtended())
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

static constexpr auto NumUrquanGuards0 {12};
static constexpr auto NumUrquanGuards1 {4};

static void BuildUrquanGuard(SolarSystemState* solarSys)
{
	uint8_t ship1 {};
	uint8_t ship2 {};
	uint8_t b0 {};
	uint8_t b1 {};
	GfxPoint org;
	IpGroupHandle hNextGroup {};

	uqm::gameState().game().BattleGroupRef = uqm::gameState().flag(uqm::GameFlag::SAMATRA_GRPOFFS);

	if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
	{
		ship1 = *RaceId::Urquan;
		ship2 = *RaceId::BlackUrquan;
	}
	else
	{
		ship1 = *RaceId::BlackUrquan;
		ship2 = *(!IsExtended() ? RaceId::Urquan : RaceId::BlackUrquan);
	}

	assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

	for (b0 = 0; b0 < MaxShipsPerSide; ++b0)
	{
		CloneShipFragment((RaceId)ship1, &uqm::gameState().game().npc_built_ship_q, 0);
	}

	if (uqm::gameState().game().BattleGroupRef == 0)
	{
		uqm::gameState().game().BattleGroupRef = PutGroupInfo(GroupsAddNew, 1);
		uqm::gameState().setFlag(uqm::GameFlag::SAMATRA_GRPOFFS, static_cast<uint32_t>(uqm::gameState().game().BattleGroupRef));
	}

	for (b0 = 1; b0 <= NumUrquanGuards0; ++b0)
	{
		PutGroupInfo(uqm::gameState().game().BattleGroupRef, b0);
	}

	ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
	for (b0 = 0; b0 < MaxShipsPerSide; ++b0)
	{
		CloneShipFragment((RaceId)ship2, &uqm::gameState().game().npc_built_ship_q, 0);
	}

	for (b0 = 1; b0 <= NumUrquanGuards1; ++b0)
	{
		PutGroupInfo(uqm::gameState().game().BattleGroupRef,
					 (uint8_t)(NumUrquanGuards0 + b0));
	}

	ReinitQueue(&uqm::gameState().game().npc_built_ship_q);

	GetGroupInfo(uqm::gameState().game().BattleGroupRef, GroupInitIp);

	org = planetOuterLocation(solarSys->SunDesc[0].PlanetByte);
	IpGroupHandle hGroup {GetHeadLink(&uqm::gameState().game().ip_group_q)};
	for (b0 = 0, b1 = 0;
		 b0 < NumUrquanGuards0;
		 ++b0, b1 += FullCircle / (NumUrquanGuards0 + NumUrquanGuards1))
	{
		if (b1 % (FullCircle / NumUrquanGuards1) == 0)
		{
			b1 += FullCircle / (NumUrquanGuards0 + NumUrquanGuards1);
		}

		InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);
		GroupPtr->task = *GroupTask::OnStation | GroupTaskFlag::IgnoreFlagship;
		GroupPtr->sys_loc = 0;
		GroupPtr->dest_loc = solarSys->SunDesc[0].PlanetByte + 1;
		GroupPtr->orbit_pos = NormalizeFacing(AngleToFacing(b1));
		GroupPtr->group_counter = 0;
		GroupPtr->loc.x = org.x + Cosine(b1, StationRadius);
		GroupPtr->loc.y = org.y + Sine(b1, StationRadius);
		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
		hGroup = hNextGroup;
	}

	for (b0 = 0, b1 = 0;
		 b0 < NumUrquanGuards1;
		 ++b0, b1 += FullCircle / NumUrquanGuards1)
	{
		InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);
		GroupPtr->task = *GroupTask::OnStation | GroupTaskFlag::IgnoreFlagship;
		GroupPtr->sys_loc = 0;
		GroupPtr->dest_loc = solarSys->SunDesc[0].PlanetByte + 1;
		GroupPtr->orbit_pos = NormalizeFacing(AngleToFacing(b1));
		GroupPtr->group_counter = 0;
		GroupPtr->loc.x = org.x + Cosine(b1, StationRadius);
		GroupPtr->loc.y = org.y + Sine(b1, StationRadius);
		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
		hGroup = hNextGroup;
	}

	(void)solarSys;
}

static uint16_t GenerateSaMatra_generateEnergy(const SolarSystemState* solarSys,
											   const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (IsExtended()
		&& CurStarDescPtr()->Index == PlotId::DestroyedStarbase
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateSaMatra_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
										 uint16_t whichNode)
{
	if (IsExtended()
		&& CurStarDescPtr()->Index == PlotId::DestroyedStarbase
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		GenerateDefault_landerReportCycle(solarSys);

		return false; // do not remove the node
	}

	(void)whichNode;
	return false;
}

} // namespace uqm