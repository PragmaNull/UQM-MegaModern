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


static bool GenerateMycon_generatePlanets(SolarSystemState* solarSys);
static bool GenerateMycon_generateName(const SolarSystemState*,
									   const SystemPlanet* world);
static bool GenerateMycon_generateOrbital(SolarSystemState* solarSys,
										  SystemPlanet* world);
static uint16_t GenerateMycon_generateEnergy(const SolarSystemState*,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static uint16_t GenerateMycon_generateLife(const SolarSystemState*,
										   const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateMycon_pickupEnergy(SolarSystemState* solarSys,
									   SystemPlanet* world, uint16_t whichNode);


const GenerateFunctions generateMyconFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateMycon_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateMycon_generateName,
	/* .generateOrbital  = */ GenerateMycon_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateMycon_generateEnergy,
	/* .generateLife     = */ GenerateMycon_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateMycon_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};

static constexpr auto StateOffset {29};

static bool GenerateMyconDefenders(uint8_t index)
{
	const uint8_t shift = index - StateOffset;
	bool Survivors {};
	uint16_t state {};
	uint16_t i {};

	if (!(uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << shift))
	{

		PutGroupInfo(GroupsRandom, GroupSaveIp);
		ReinitQueue(&uqm::gameState().game().ip_group_q);
		assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

		for (i = 0; i < 4; ++i)
		{
			CloneShipFragment(MYCON_SHIP,
							  &uqm::gameState().game().npc_built_ship_q, 0);
		}

		uqm::currentActivity() |= START_INTERPLANETARY;
		InitCommunication(MYCON_CONVERSATION);

		if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
		{
			return true;
		}

		Survivors = GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0;

		uqm::currentActivity() &= ~START_INTERPLANETARY;
		ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
		GetGroupInfo(GroupsRandom, GroupLoadIp);

		if (Survivors)
		{
			return true;
		}

		state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);
		state |= 1 << shift;

		uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));

		RepairSISBorder();
	}
	return false;
}

static bool GenerateMycon_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	pSunDesc->PlanetByte = 0;

	if (!uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
	}

	SystemPlanet* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	pPlanet->data_index = SHATTERED_WORLD;

	if (uqm::isOriginalSeed())
	{
		pPlanet->radius = EarthRadius * 80L / 100;
		if (pPlanet->NumPlanets > 2)
		{
			pPlanet->NumPlanets = 2;
		}
		const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}

	return true;
}

static bool GenerateMycon_generateName(const SolarSystemState* solarSys,
									   const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (CurStarDescPtr()->Index == EGG_CASE0_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(PLANET_NUMBER_BASE + 42));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateMycon_generateOrbital(SolarSystemState* solarSys,
										  SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if ((CurStarDescPtr()->Index == MYCON_DEFINED
			 || CurStarDescPtr()->Index == SUN_DEVICE_DEFINED)
			&& StartSphereTracking(MYCON_SHIP))
		{
			if (CurStarDescPtr()->Index == MYCON_DEFINED
				|| !uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE_UNGUARDED))
			{
				NotifyOthers(MYCON_SHIP, IpnlAllClear);
				PutGroupInfo(GroupsRandom, GroupSaveIp);
				ReinitQueue(&uqm::gameState().game().ip_group_q);
				assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

				if (CurStarDescPtr()->Index == MYCON_DEFINED
					|| !uqm::gameState().flag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH))
				{
					CloneShipFragment(MYCON_SHIP,
									  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);
				}
				else
				{
					const uint16_t sum = difficultyCase(3, 5, 12);

					for (uint16_t i {0}; i < sum; ++i)
					{
						CloneShipFragment(MYCON_SHIP,
										  &uqm::gameState().game().npc_built_ship_q, 0);
					}
				}

				uqm::currentActivity() |= START_INTERPLANETARY;
				if (CurStarDescPtr()->Index == MYCON_DEFINED)
				{
					uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
				}
				else
				{
					uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
				}
				InitCommunication(MYCON_CONVERSATION);

				if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
				{
					return true;
				}

				{
					bool MyconSurvivors {};

					MyconSurvivors =
						GetHeadLink(&uqm::gameState().game().npc_built_ship_q) != 0;

					uqm::currentActivity() &= ~START_INTERPLANETARY;
					ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
					GetGroupInfo(GroupsRandom, GroupLoadIp);

					if (MyconSurvivors)
					{
						return true;
					}

					uqm::gameState().setFlag(uqm::GameFlag::SUN_DEVICE_UNGUARDED, static_cast<uint32_t>(1));
					RepairSISBorder();
				}
			}
		}

		switch (CurStarDescPtr()->Index)
		{
			case SUN_DEVICE_DEFINED:
				if (!uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE))
				{
					LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
						CaptureDrawable(
							LoadGraphic(
								SunDeviceMaskPmapAnimRes));
					solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(SunDeviceStrtabRes);
					solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
				}
				break;
			case EGG_CASE0_DEFINED:
			case EGG_CASE1_DEFINED:
			case EGG_CASE2_DEFINED:
				if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) == 0)
				{
					uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(1));
				}

				if (isDifficulty(uqm::Difficulty::Hard) && StartSphereTracking(MYCON_SHIP))
				{
					if (GenerateMyconDefenders(CurStarDescPtr()->Index))
					{
						return true;
					}
				}

				if (!isNodeRetrieved(&solarSys->SysInfo.PlanetInfo,
									 ENERGY_SCAN, 0))
				{
					LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
						CaptureDrawable(
							LoadGraphic(EggCaseMaskPmapAnimRes));
					solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(EggCaseStrtabRes);
					solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
				}
				break;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& !uqm::isOriginalSeed()
		&& (CurStarDescPtr()->Index != EGG_CASE0_DEFINED || isDifficulty(uqm::Difficulty::Hard)))
	{
		PlanetData* pPlanetInfo = &solarSys->SysInfo.PlanetInfo;
		const uint32_t rand = SysGenRNG->next();

		pPlanetInfo->SurfaceTemperature = uqm::rangeMinMax(100, 300, rand);
	}

	return true;
}

static uint16_t GenerateMycon_generateEnergy(const SolarSystemState* solarSys,
											 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (CurStarDescPtr()->Index == SUN_DEVICE_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE))
		{ // already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	if ((CurStarDescPtr()->Index == EGG_CASE0_DEFINED
		 || CurStarDescPtr()->Index == EGG_CASE1_DEFINED
		 || CurStarDescPtr()->Index == EGG_CASE2_DEFINED)
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		// XXX: DiscoveryStrings is set by generateOrbital() only when the
		//   node has not been picked up yet
		if (!solarSys->SysInfo.PlanetInfo.DiscoveryStrings)
		{ // already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateMycon_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
									   uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == SUN_DEVICE_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(!uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE) && whichNode == 0);

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		uqm::gameState().setFlag(uqm::GameFlag::SUN_DEVICE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SUN_DEVICE_ON_SHIP, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_VISITS, static_cast<uint32_t>(0));

		return true; // picked up
	}

	if ((CurStarDescPtr()->Index == EGG_CASE0_DEFINED
		 || CurStarDescPtr()->Index == EGG_CASE1_DEFINED
		 || CurStarDescPtr()->Index == EGG_CASE2_DEFINED)
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(whichNode == 0);

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		switch (CurStarDescPtr()->Index)
		{
			case EGG_CASE0_DEFINED:
				uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE0_ON_SHIP, static_cast<uint32_t>(1));
				break;
			case EGG_CASE1_DEFINED:
				uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE1_ON_SHIP, static_cast<uint32_t>(1));
				break;
			case EGG_CASE2_DEFINED:
				uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE2_ON_SHIP, static_cast<uint32_t>(1));
				break;
		}

		return true; // picked up
	}

	(void)whichNode;
	return false;
}

static uint16_t GenerateMycon_generateLife(const SolarSystemState* solarSys,
										   const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	// Gee, I wonder why there isn't any life in Mycon systems...
	(void)whichNode;
	(void)solarSys;
	(void)world;
	(void)info;
	return 0;
}