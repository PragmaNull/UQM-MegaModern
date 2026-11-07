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


namespace uqm
{
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


bool GenerateDefault_initNpcs(SolarSystemState* solarSys)
{
	if (!GetGroupInfo(uqm::gameState().game().BattleGroupRef, GroupInitIp))
	{
		uqm::gameState().game().BattleGroupRef = 0;
		BuildGroups();
	}

	if (SpaceMusicOK)
	{
		findRaceSOI();
	}

	(void)solarSys;
	return true;
}

bool GenerateDefault_reinitNpcs(SolarSystemState* solarSys)
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

bool GenerateDefault_uninitNpcs(SolarSystemState* solarSys)
{
	PutGroupInfo(GroupsRandom, GroupSaveIp);
	ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
	ReinitQueue(&uqm::gameState().game().ip_group_q);

	(void)solarSys;
	return true;
}

bool GenerateDefault_generatePlanets(SolarSystemState* solarSys)
{
	FillOrbits(solarSys, (uint8_t)~0, solarSys->PlanetDesc, false);
	GeneratePlanets(solarSys);
	return true;
}

bool GenerateDefault_generateMoons(SolarSystemState* solarSys, SystemPlanet* planet)
{
	FillOrbits(solarSys, planet->NumPlanets, solarSys->MoonDesc, false);
	return true;
}

bool GenerateDefault_generateName(const SolarSystemState* solarSys,
								  const SystemPlanet* world)
{
	const uint16_t i = planetIndex(solarSys, world);
	uqm::utf8StringCopy(uqm::gameState().sis().PlanetName, sizeof(uqm::gameState().sis().PlanetName),
						GameString(GameStringBase::PlanetNumber + (9 + 7) + i));
	uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(world->data_index));

	return true;
}

bool GenerateDefault_generateOrbital(SolarSystemState* solarSys, SystemPlanet* world)
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

	SolarSystemInfo* sysInfo {&solarSys->SysInfo};

	DoPlanetaryAnalysis(sysInfo, world);
	uint32_t rand_val {SysGenRNG->seed()};

	sysInfo->PlanetInfo.ScanSeed[*ScanType::Biological] = rand_val;
	GenerateLifeForms(sysInfo, GenerateAll, nullptr);
	rand_val = SysGenRNG->seed();

	sysInfo->PlanetInfo.ScanSeed[*ScanType::Mineral] = rand_val;
	GenerateMineralDeposits(sysInfo, GenerateAll, nullptr);

	sysInfo->PlanetInfo.ScanSeed[*ScanType::Energy] = rand_val;
	LoadPlanet(nullptr);

	return true;
}

uint16_t GenerateDefault_generateMinerals(const SolarSystemState* solarSys,
										  const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	return GenerateMineralDeposits(&solarSys->SysInfo, whichNode, info);
	(void)world;
}

bool GenerateDefault_pickupMinerals(SolarSystemState* solarSys, SystemPlanet* world,
									uint16_t whichNode)
{
	// Minerals do not need any extra handling as of now
	(void)solarSys;
	(void)world;
	(void)whichNode;
	return true;
}

uint16_t GenerateDefault_generateEnergy(const SolarSystemState* solarSys,
										const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	(void)whichNode;
	(void)solarSys;
	(void)world;
	(void)info;
	return 0;
}

bool GenerateDefault_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
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

uint16_t GenerateDefault_generateLife(const SolarSystemState* solarSys,
									  const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	return GenerateLifeForms(&solarSys->SysInfo, whichNode, info);
	(void)world;
}

bool GenerateDefault_pickupLife(SolarSystemState* solarSys, SystemPlanet* world,
								uint16_t whichNode)
{
	// Bio does not need any extra handling as of now
	(void)solarSys;
	(void)world;
	(void)whichNode;
	return true;
}

uint16_t GenerateDefault_generateArtifact(const SolarSystemState* solarSys,
										  uint16_t whichNode, SurfaceNode* info)
{
	// Generate an energy node at a random location
	return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Energy, 1, 0,
							   whichNode, info);
}

uint16_t GenerateDefault_generateRuins(const SolarSystemState* solarSys,
									   uint16_t whichNode, SurfaceNode* info)
{
	// Generate a standard spread of city ruins of a destroyed civilization
	return GenerateRandomNodes(&solarSys->SysInfo, *ScanType::Energy,
							   NumRaceRuins, 0, whichNode, info);
}

static inline void runLanderReport(void)
{
	UnbatchGraphics();
	DoDiscoveryReport(MenuSounds);
	BatchGraphics();
}

bool GenerateDefault_landerReport(SolarSystemState* solarSys)
{
	PlanetData* planetInfo = &solarSys->SysInfo.PlanetInfo;

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

bool GenerateDefault_landerReportCycle(SolarSystemState* solarSys)
{
	PlanetData* planetInfo = &solarSys->SysInfo.PlanetInfo;

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
void GeneratePlanets(SolarSystemState* solarSys)
{
	uint16_t i {};
	SystemPlanet* planet {};

	for (i = solarSys->SunDesc[0].NumPlanets,
		planet = &solarSys->PlanetDesc[0];
		 i; --i, ++planet)
	{
		const uint32_t rand_val {SysGenRNG->next()};
		const uint8_t byte_val {uqm::lowByte(rand_val)};

		uint8_t num_moons {0};
		const uint8_t type {PlanData[planet->data_index & ~*PlanetType::PlanetShielded].Type};
		switch (Plansize(type))
		{
			case PlanetSize::LargeRockyWorld:
				if (byte_val < 0x00FF * 25 / 100)
				{
					if (byte_val < 0x00FF * 5 / 100)
					{
						++num_moons;
					}
					++num_moons;
				}
				break;
			case PlanetSize::GasGiant:
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
			default:
				break;
		}
		planet->NumPlanets = num_moons;
	}
}

uint8_t GenerateWorlds(uint8_t whichType)
{
	uint8_t planet {*PlanetType::FirstSmallRocky};

	if (whichType & SmallRocky)
	{
		planet = *PlanetType::FirstSmallRocky + SysGenRNG->next() % NumberOfSmallRockyWorlds;
	}
	else if (whichType & LargeRocky)
	{
		planet = *PlanetType::FirstLargeRocky + SysGenRNG->next() % (NumberOfLargeRockyWorlds - 2);
		// Skip over rainbow_world and shattered_world, which are adjacent.
		if (planet >= PlanetType::Rainbow)
		{
			planet += 2;
		}
	}
	else if (whichType & AllRocky)
	{
		planet = *PlanetType::FirstRocky + SysGenRNG->next() % (NumberOfRockyWorlds - 2);
		// Skip over rainbow_world and shattered_world, which are adjacent.
		if (planet >= PlanetType::Rainbow)
		{
			planet += 2;
		}
	}
	else if (whichType & OnlyLarge)
	{
		planet = *PlanetType::FirstLargeRocky + SysGenRNG->next() % (NumberOfLargeRockyWorlds + NumberOfGasGiants - 2);
		// Skip over rainbow_world and shattered_world, which are adjacent.
		if (planet >= PlanetType::Rainbow)
		{
			planet += 2;
		}
	}
	else if (whichType & OnlyGas)
	{
		planet = *PlanetType::FirstGasGiant + SysGenRNG->next() % NumberOfGasGiants;
	}

	return planet;
}

static constexpr auto DwarfGasgDist {ScaleRadius(12)};

void GenerateGasGiantRanged(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
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
	SystemPlanet* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

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

uint8_t GenerateCrystalWorld(void)
{
	const int crystalArray[] = {
		*PlanetType::Sapphire,
		*PlanetType::Emerald,
		*PlanetType::Ruby};
	return crystalArray[SysGenRNG->next() % 3];
}

uint8_t GenerateDesolateWorld(void)
{
	const int desolateArray[] = {
		*PlanetType::Dust,
		*PlanetType::Crimson,
		*PlanetType::Urea};
	return desolateArray[SysGenRNG->next() % 3];
}

uint8_t GenerateHabitableWorld(void)
{
	const int habitableArray[] = {
		*PlanetType::Primordial,
		*PlanetType::Water,
		*PlanetType::Telluric,
		*PlanetType::Redux};
	return habitableArray[SysGenRNG->next() % 4];
}

uint8_t GenerateGasGiantWorld(void)
{
	return *PlanetType::FirstGasGiant + SysGenRNG->next() % NumberOfGasGiants;
}

// input: 1 <= min <= max
// output: min <= RNG <= max
// min 0 will be treated 1; min >= max will return max
uint8_t GenerateMinPlanets(uint8_t min)
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
uint8_t GenerateNumberOfPlanets(uint8_t minimum)
{
	const uint8_t roll = MaxGenPlanets - minimum;
	const uint8_t adjust = minimum + 1;
	return (SysGenRNG->next() % roll) + adjust;
}

// "Random PlanetByte Generator"
uint8_t PlanetByteGen(SystemPlanet* pPDesc)
{
	return SysGenRNG->next() % pPDesc->NumPlanets;
}

static void check_yehat_rebellion(void)
{
	HIPGROUP hGroup {};
	HIPGROUP hNextGroup {};

	// XXX: Is there a better way to do this? I could not find one.
	//   When you talk to a Yehat ship (YEHAT_SHIP) and start the rebellion,
	//   there is no battle following the comm. There is *never* a battle in
	//   an Encounter with Rebels, but the group race_id (YEHAT_REBEL_SHIP)
	//   is different from Royalists (YEHAT_SHIP). There is *always* a battle
	//   in an Encounter with Royalists.
	// TRANSLATION: "If the civil war has not started yet, or the player
	//   battled a ship -- bail."
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_CIVIL_WAR) || EncounterRace >= 0)
	{
		return; // not this time
	}

	// Send Yehat groups to flee the system, but only if the player
	// has actually talked to a ship.
	for (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q); hGroup;
		 hGroup = hNextGroup)
	{
		InterplanetaryGroup* GroupPtr = LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
		hNextGroup = _GetSuccLink(GroupPtr);
		// IGNORE_FLAGSHIP was set in ipdisp.c:ip_group_collision()
		// during a collision with the flagship.
		if (GroupPtr->race_id == RaceId::Yehat
			&& (GroupPtr->task & GroupTaskFlag::IgnoreFlagship))
		{
			GroupPtr->task &= GroupTaskFlag::ReformGroup;
			GroupPtr->task |= *GroupTask::Flee | GroupTaskFlag::IgnoreFlagship;
			GroupPtr->dest_loc = 0;
		}
		UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
	}
}

} // namespace uqm
