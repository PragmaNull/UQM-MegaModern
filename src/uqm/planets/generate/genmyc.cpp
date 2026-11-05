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


static bool GenerateMycon_generatePlanets(SOLARSYS_STATE* solarSys);
static bool GenerateMycon_generateName(const SOLARSYS_STATE*,
									   const PLANET_DESC* world);
static bool GenerateMycon_generateOrbital(SOLARSYS_STATE* solarSys,
										  PLANET_DESC* world);
static uint16_t GenerateMycon_generateEnergy(const SOLARSYS_STATE*,
											 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO*);
static uint16_t GenerateMycon_generateLife(const SOLARSYS_STATE*,
										   const PLANET_DESC* world, uint16_t whichNode, NODE_INFO*);
static bool GenerateMycon_pickupEnergy(SOLARSYS_STATE* solarSys,
									   PLANET_DESC* world, uint16_t whichNode);


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

static bool
GenerateMyconDefenders(uint8_t index)
{
	const uint8_t shift = index - StateOffset;
	bool Survivors {};
	uint16_t state {};
	uint16_t i {};

	if (!(GET_GAME_STATE(HM_ENCOUNTERS) & 1 << shift))
	{

		PutGroupInfo(GroupsRandom, GroupSaveIp);
		ReinitQueue(&GLOBAL(ip_group_q));
		assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

		for (i = 0; i < 4; ++i)
		{
			CloneShipFragment(MYCON_SHIP,
							  &GLOBAL(npc_built_ship_q), 0);
		}

		uqm::currentActivity() |= START_INTERPLANETARY;
		InitCommunication(MYCON_CONVERSATION);

		if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
		{
			return true;
		}

		Survivors = GetHeadLink(&GLOBAL(npc_built_ship_q)) != 0;

		uqm::currentActivity() &= ~START_INTERPLANETARY;
		ReinitQueue(&GLOBAL(npc_built_ship_q));
		GetGroupInfo(GroupsRandom, GroupLoadIp);

		if (Survivors)
		{
			return true;
		}

		state = GET_GAME_STATE(HM_ENCOUNTERS);
		state |= 1 << shift;

		SET_GAME_STATE(HM_ENCOUNTERS, state);

		RepairSISBorder();
	}
	return false;
}

static bool
GenerateMycon_generatePlanets(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	pSunDesc->PlanetByte = 0;

	if (!uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
	}

	PLANET_DESC* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	pPlanet->data_index = SHATTERED_WORLD;

	if (uqm::isOriginalSeed())
	{
		pPlanet->radius = EarthRadius * 80L / 100;
		if (pPlanet->NumPlanets > 2)
		{
			pPlanet->NumPlanets = 2;
		}
		const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		pPlanet->location.x = COSINE(angle, pPlanet->radius);
		pPlanet->location.y = SINE(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}

	return true;
}

static bool
GenerateMycon_generateName(const SOLARSYS_STATE* solarSys,
						   const PLANET_DESC* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (CurStarDescPtr()->Index == EGG_CASE0_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const PLANET_DESC pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(GLOBAL_SIS(PlanetName),
							sizeof(GLOBAL_SIS(PlanetName)),
							GAME_STRING(PLANET_NUMBER_BASE + 42));

		SET_GAME_STATE(BATTLE_PLANET, pPlanetDesc.data_index);
	}

	return true;
}

static bool
GenerateMycon_generateOrbital(SOLARSYS_STATE* solarSys,
							  PLANET_DESC* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if ((CurStarDescPtr()->Index == MYCON_DEFINED
			 || CurStarDescPtr()->Index == SUN_DEVICE_DEFINED)
			&& StartSphereTracking(MYCON_SHIP))
		{
			if (CurStarDescPtr()->Index == MYCON_DEFINED
				|| !GET_GAME_STATE(SUN_DEVICE_UNGUARDED))
			{
				NotifyOthers(MYCON_SHIP, IpnlAllClear);
				PutGroupInfo(GroupsRandom, GroupSaveIp);
				ReinitQueue(&GLOBAL(ip_group_q));
				assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

				if (CurStarDescPtr()->Index == MYCON_DEFINED
					|| !GET_GAME_STATE(MYCON_FELL_FOR_AMBUSH))
				{
					CloneShipFragment(MYCON_SHIP,
									  &GLOBAL(npc_built_ship_q), INFINITE_FLEET);
				}
				else
				{
					const uint16_t sum = difficultyCase(3, 5, 12);

					for (uint16_t i {0}; i < sum; ++i)
					{
						CloneShipFragment(MYCON_SHIP,
										  &GLOBAL(npc_built_ship_q), 0);
					}
				}

				uqm::currentActivity() |= START_INTERPLANETARY;
				if (CurStarDescPtr()->Index == MYCON_DEFINED)
				{
					SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 7);
				}
				else
				{
					SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 6);
				}
				InitCommunication(MYCON_CONVERSATION);

				if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
				{
					return true;
				}

				{
					bool MyconSurvivors {};

					MyconSurvivors =
						GetHeadLink(&GLOBAL(npc_built_ship_q)) != 0;

					uqm::currentActivity() &= ~START_INTERPLANETARY;
					ReinitQueue(&GLOBAL(npc_built_ship_q));
					GetGroupInfo(GroupsRandom, GroupLoadIp);

					if (MyconSurvivors)
					{
						return true;
					}

					SET_GAME_STATE(SUN_DEVICE_UNGUARDED, 1);
					RepairSISBorder();
				}
			}
		}

		switch (CurStarDescPtr()->Index)
		{
			case SUN_DEVICE_DEFINED:
				if (!GET_GAME_STATE(SUN_DEVICE))
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
				if (GET_GAME_STATE(KNOW_ABOUT_SHATTERED) == 0)
				{
					SET_GAME_STATE(KNOW_ABOUT_SHATTERED, 1);
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
		PLANET_INFO* pPlanetInfo = &solarSys->SysInfo.PlanetInfo;
		const uint32_t rand = SysGenRNG->next();

		pPlanetInfo->SurfaceTemperature = uqm::rangeMinMax(100, 300, rand);
	}

	return true;
}

static uint16_t
GenerateMycon_generateEnergy(const SOLARSYS_STATE* solarSys,
							 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	if (CurStarDescPtr()->Index == SUN_DEVICE_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE(SUN_DEVICE))
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

static bool
GenerateMycon_pickupEnergy(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
						   uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == SUN_DEVICE_DEFINED
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(!GET_GAME_STATE(SUN_DEVICE) && whichNode == 0);

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		SET_GAME_STATE(SUN_DEVICE, 1);
		SET_GAME_STATE(SUN_DEVICE_ON_SHIP, 1);
		SET_GAME_STATE(MYCON_VISITS, 0);

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
				SET_GAME_STATE(EGG_CASE0_ON_SHIP, 1);
				break;
			case EGG_CASE1_DEFINED:
				SET_GAME_STATE(EGG_CASE1_ON_SHIP, 1);
				break;
			case EGG_CASE2_DEFINED:
				SET_GAME_STATE(EGG_CASE2_ON_SHIP, 1);
				break;
		}

		return true; // picked up
	}

	(void)whichNode;
	return false;
}

static uint16_t
GenerateMycon_generateLife(const SOLARSYS_STATE* solarSys,
						   const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	// Gee, I wonder why there isn't any life in Mycon systems...
	(void)whichNode;
	(void)solarSys;
	(void)world;
	(void)info;
	return 0;
}