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
#include "../lander.h"
#include "../../encount.h"
#include "../../gamestr.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../races.h"
#include "../../setup.h"
#include "../../state.h"
#include "../../sounds.h"
#include "libs/mathlib.h"


static void check_yehat_rebellion(void);


const GenerateFunctions generateDefaultFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateDefault_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateDefault_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


bool GenerateDefault_initNpcs(SOLARSYS_STATE* solarSys)
{
	if (!GetGroupInfo(GLOBAL(BattleGroupRef), GroupInitIp))
	{
		GLOBAL(BattleGroupRef) = 0;
		BuildGroups();
	}

	if (SpaceMusicOK)
	{
		findRaceSOI();
	}

	(void)solarSys;
	return true;
}

bool GenerateDefault_reinitNpcs(SOLARSYS_STATE* solarSys)
{
	GetGroupInfo(GroupsRandom, GroupLoadIp);
	// This is not a great place to do the Yehat rebellion check, but
	// since you can start the rebellion in any star system (not just
	// the Homeworld), I could not find a better place for it.
	// At least it is better than where it was originally.
	check_yehat_rebellion();

	(void)solarSys;
	return true;
}

bool GenerateDefault_uninitNpcs(SOLARSYS_STATE* solarSys)
{
	PutGroupInfo(GroupsRandom, GroupSaveIp);
	ReinitQueue(&GLOBAL(npc_built_ship_q));
	ReinitQueue(&GLOBAL(ip_group_q));

	(void)solarSys;
	return true;
}

bool GenerateDefault_generatePlanets(SOLARSYS_STATE* solarSys)
{
	FillOrbits(solarSys, (uint8_t)~0, solarSys->PlanetDesc, false);
	GeneratePlanets(solarSys);
	return true;
}

bool GenerateDefault_generateMoons(SOLARSYS_STATE* solarSys, PLANET_DESC* planet)
{
	FillOrbits(solarSys, planet->NumPlanets, solarSys->MoonDesc, false);
	return true;
}

bool GenerateDefault_generateName(const SOLARSYS_STATE* solarSys,
								  const PLANET_DESC* world)
{
	const uint16_t i = planetIndex(solarSys, world);
	uqm::utf8StringCopy(GLOBAL_SIS(PlanetName), sizeof(GLOBAL_SIS(PlanetName)),
						GameString(PLANET_NUMBER_BASE + (9 + 7) + i));
	SET_GAME_STATE(BATTLE_PLANET, world->data_index);

	return true;
}

bool GenerateDefault_generateOrbital(SOLARSYS_STATE* solarSys, PLANET_DESC* world)
{
#ifdef DEBUG_SOLARSYS
	if (worldIsPlanet(solarSys, world))
	{
		uqm::log::debug("Planet index = {}",
						planetIndex(solarSys, world));
	}
	else
	{
		uqm::log::debug("Planet index = {}, Moon index = {}",
						planetIndex(solarSys, world),
						moonIndex(solarSys, world));
	}
#endif /* DEBUG_SOLARSYS */

	SYSTEM_INFO* sysInfo {&solarSys->SysInfo};

	DoPlanetaryAnalysis(sysInfo, world);
	uint32_t rand_val {SysGenRNG->seed()};

	sysInfo->PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] = rand_val;
	GenerateLifeForms(sysInfo, GenerateAll, nullptr);
	rand_val = SysGenRNG->seed();

	sysInfo->PlanetInfo.ScanSeed[MINERAL_SCAN] = rand_val;
	GenerateMineralDeposits(sysInfo, GenerateAll, nullptr);

	sysInfo->PlanetInfo.ScanSeed[ENERGY_SCAN] = rand_val;
	LoadPlanet(nullptr);

	return true;
}

uint16_t
GenerateDefault_generateMinerals(const SOLARSYS_STATE* solarSys,
								 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	return GenerateMineralDeposits(&solarSys->SysInfo, whichNode, info);
	(void)world;
}

bool GenerateDefault_pickupMinerals(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
									uint16_t whichNode)
{
	// Minerals do not need any extra handling as of now
	(void)solarSys;
	(void)world;
	(void)whichNode;
	return true;
}

uint16_t
GenerateDefault_generateEnergy(const SOLARSYS_STATE* solarSys,
							   const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	(void)whichNode;
	(void)solarSys;
	(void)world;
	(void)info;
	return 0;
}

bool GenerateDefault_pickupEnergy(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
								  uint16_t whichNode)
{
	// This should never be called since every energy node needs
	// special handling and the function should be overridden
	assert(false);
	(void)solarSys;
	(void)world;
	(void)whichNode;
	return false;
}

uint16_t
GenerateDefault_generateLife(const SOLARSYS_STATE* solarSys,
							 const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	return GenerateLifeForms(&solarSys->SysInfo, whichNode, info);
	(void)world;
}

bool GenerateDefault_pickupLife(SOLARSYS_STATE* solarSys, PLANET_DESC* world,
								uint16_t whichNode)
{
	// Bio does not need any extra handling as of now
	(void)solarSys;
	(void)world;
	(void)whichNode;
	return true;
}

uint16_t
GenerateDefault_generateArtifact(const SOLARSYS_STATE* solarSys,
								 uint16_t whichNode, NODE_INFO* info)
{
	// Generate an energy node at a random location
	return GenerateRandomNodes(&solarSys->SysInfo, ENERGY_SCAN, 1, 0,
							   whichNode, info);
}

uint16_t
GenerateDefault_generateRuins(const SOLARSYS_STATE* solarSys,
							  uint16_t whichNode, NODE_INFO* info)
{
	// Generate a standard spread of city ruins of a destroyed civilization
	return GenerateRandomNodes(&solarSys->SysInfo, ENERGY_SCAN,
							   NumRaceRuins, 0, whichNode, info);
}

static inline void
runLanderReport(void)
{
	UnbatchGraphics();
	DoDiscoveryReport(MenuSounds);
	BatchGraphics();
}

bool GenerateDefault_landerReport(SOLARSYS_STATE* solarSys)
{
	PLANET_INFO* planetInfo = &solarSys->SysInfo.PlanetInfo;

	if (!planetInfo->DiscoveryStrings)
	{
		return false;
	}

	runLanderReport();

	// XXX: A non-cycling report is given only once and has to be deleted
	//   in some circumstances (like the Syreen Vault). It does not
	//   hurt to simply delete it in all cases. Nothing should rely on
	//   the presence of DiscoveryStrings, but the Syreen Vault and the
	//   Mycon Egg Cases rely on its absence.
	planetInfo->DiscoveryStrings.reset();

	return true;
}

bool GenerateDefault_landerReportCycle(SOLARSYS_STATE* solarSys)
{
	PLANET_INFO* planetInfo = &solarSys->SysInfo.PlanetInfo;

	if (!planetInfo->DiscoveryStrings)
	{
		return false;
	}

	runLanderReport();
	// Advance to the next report. Stop after the last report.
	if (++planetInfo->DiscoveryIndex >= planetInfo->DiscoveryStrings->size())
	{
		planetInfo->DiscoveryStrings.reset();
	}

	return true;
}

// NB. This function modifies the RNG state.
void GeneratePlanets(SOLARSYS_STATE* solarSys)
{
	uint16_t i {};
	PLANET_DESC* planet {};

	for (i = solarSys->SunDesc[0].NumPlanets,
		planet = &solarSys->PlanetDesc[0];
		 i; --i, ++planet)
	{
		const uint32_t rand_val {SysGenRNG->next()};
		const uint8_t byte_val {uqm::lowByte(rand_val)};

		uint8_t num_moons {0};
		const uint8_t type {PlanData[planet->data_index & ~PLANET_SHIELDED].Type};
		switch (Plansize(type))
		{
			case LARGE_ROCKY_WORLD:
				if (byte_val < 0x00FF * 25 / 100)
				{
					if (byte_val < 0x00FF * 5 / 100)
					{
						++num_moons;
					}
					++num_moons;
				}
				break;
			case GAS_GIANT:
				if (byte_val < 0x00FF * 90 / 100)
				{
					if (byte_val < 0x00FF * 75 / 100)
					{
						if (byte_val < 0x00FF * 50 / 100)
						{
							if (byte_val < 0x00FF * 25 / 100)
							{
								++num_moons;
							}
							++num_moons;
						}
						++num_moons;
					}
					++num_moons;
				}
				break;
		}
		planet->NumPlanets = num_moons;
	}
}

uint8_t
GenerateWorlds(uint8_t whichType)
{
	uint8_t planet = FIRST_SMALL_ROCKY_WORLD;

	if (whichType & SmallRocky)
	{
		planet = FIRST_SMALL_ROCKY_WORLD + SysGenRNG->next() % NumberOfSmallRockyWorlds;
	}
	else if (whichType & LargeRocky)
	{
		planet = FIRST_LARGE_ROCKY_WORLD + SysGenRNG->next() % (NumberOfLargeRockyWorlds - 2);
		// Skip over rainbow_world and shattered_world, which are adjacent.
		if (planet >= RAINBOW_WORLD)
		{
			planet += 2;
		}
	}
	else if (whichType & AllRocky)
	{
		planet = FIRST_ROCKY_WORLD + SysGenRNG->next() % (NumberOfRockyWorlds - 2);
		// Skip over rainbow_world and shattered_world, which are adjacent.
		if (planet >= RAINBOW_WORLD)
		{
			planet += 2;
		}
	}
	else if (whichType & OnlyLarge)
	{
		planet = FIRST_LARGE_ROCKY_WORLD + SysGenRNG->next() % (NumberOfLargeRockyWorlds + NumberOfGasGiants - 2);
		// Skip over rainbow_world and shattered_world, which are adjacent.
		if (planet >= RAINBOW_WORLD)
		{
			planet += 2;
		}
	}
	else if (whichType & OnlyGas)
	{
		planet = FIRST_GAS_GIANT + SysGenRNG->next() % NumberOfGasGiants;
	}

	return planet;
}

static constexpr auto DwarfGasgDist {ScaleRadius(12)};

void GenerateGasGiantRanged(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];
	uint8_t i {};
	const uint32_t rand = SysGenRNG->seed();

	for (i = 0; i < pSunDesc->NumPlanets; i++)
	{
		if (solarSys->PlanetDesc[i].radius >= DwarfGasgDist)
		{
			break;
		}
	}

	if (i == pSunDesc->NumPlanets)
	{
		i = rand % (pSunDesc->NumPlanets);
	}
	else
	{
		i += rand % (pSunDesc->NumPlanets - i);
	}
	pSunDesc->PlanetByte = i;
	PLANET_DESC* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	pPlanet->data_index = GenerateWorlds(OnlyGas);

	if (solarSys->PlanetDesc[i].radius < DwarfGasgDist)
	{
		pPlanet->radius =
			uqm::rangeMinMax(DwarfGasgDist, MaxPlanetRadius, rand);
		const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}
}

uint8_t
GenerateCrystalWorld(void)
{
	const int crystalArray[] = {
		SAPPHIRE_WORLD,
		EMERALD_WORLD,
		RUBY_WORLD};
	return crystalArray[SysGenRNG->next() % 3];
}

uint8_t
GenerateDesolateWorld(void)
{
	const int desolateArray[] = {
		DUST_WORLD,
		CRIMSON_WORLD,
		UREA_WORLD};
	return desolateArray[SysGenRNG->next() % 3];
}

uint8_t
GenerateHabitableWorld(void)
{
	const int habitableArray[] = {
		PRIMORDIAL_WORLD,
		WATER_WORLD,
		TELLURIC_WORLD,
		REDUX_WORLD};
	return habitableArray[SysGenRNG->next() % 4];
}

uint8_t
GenerateGasGiantWorld(void)
{
	return FIRST_GAS_GIANT + SysGenRNG->next() % NumberOfGasGiants;
}

// input: 1 <= min <= max
// output: min <= RNG <= max
// min 0 will be treated 1; min >= max will return max
uint8_t
GenerateMinPlanets(uint8_t min)
{
	const uint8_t max = MaxGenPlanets + 1;

	if (min == 0)
	{
		min = 1;
	}
	if (min >= MaxGenPlanets)
	{
		min = MaxGenPlanets;
	}

	return SysGenRNG->next() % (max - min) + min;
}

// input: 0 <= minimum < MaxGenPlanets
// output: minimum + 1 <= RNG <= MaxGenPlanets
uint8_t
GenerateNumberOfPlanets(uint8_t minimum)
{
	const uint8_t roll = MaxGenPlanets - minimum;
	const uint8_t adjust = minimum + 1;
	return (SysGenRNG->next() % roll) + adjust;
}

// "Random PlanetByte Generator"
uint8_t
PlanetByteGen(PLANET_DESC* pPDesc)
{
	return SysGenRNG->next() % pPDesc->NumPlanets;
}

static void
check_yehat_rebellion(void)
{
	HIPGROUP hGroup {};
	HIPGROUP hNextGroup {};

	// XXX: Is there a better way to do this? I could not find one.
	//   When you talk to a Yehat ship (YEHAT_SHIP) and start the rebellion,
	//   there is no battle following the comm. There is *never* a battle in
	//   an encounter with Rebels, but the group race_id (YEHAT_REBEL_SHIP)
	//   is different from Royalists (YEHAT_SHIP). There is *always* a battle
	//   in an encounter with Royalists.
	// TRANSLATION: "If the civil war has not started yet, or the player
	//   battled a ship -- bail."
	if (!GET_GAME_STATE(YEHAT_CIVIL_WAR) || EncounterRace >= 0)
	{
		return; // not this time
	}

	// Send Yehat groups to flee the system, but only if the player
	// has actually talked to a ship.
	for (hGroup = GetHeadLink(&GLOBAL(ip_group_q)); hGroup;
		 hGroup = hNextGroup)
	{
		IP_GROUP* GroupPtr = LockIpGroup(&GLOBAL(ip_group_q), hGroup);
		hNextGroup = _GetSuccLink(GroupPtr);
		// IGNORE_FLAGSHIP was set in ipdisp.c:ip_group_collision()
		// during a collision with the flagship.
		if (GroupPtr->race_id == YEHAT_SHIP
			&& (GroupPtr->task & IGNORE_FLAGSHIP))
		{
			GroupPtr->task &= REFORM_GROUP;
			GroupPtr->task |= FLEE | IGNORE_FLAGSHIP;
			GroupPtr->dest_loc = 0;
		}
		UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
	}
}
