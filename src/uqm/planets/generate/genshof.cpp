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
#include "../../build.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../istrtab.h"
#include "../../nameref.h"
#include "../../sounds.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "../lander.h"
#include "../planets.h"
#include "uqm/random/UQMRandom.h"


static bool GenerateShofixti_initNpcs(SOLARSYS_STATE* solarSys);
static bool GenerateShofixti_reinitNpcs(SOLARSYS_STATE* solarSys);
static bool GenerateShofixti_uninitNpcs(SOLARSYS_STATE* solarSys);
static bool GenerateShofixti_generatePlanets(SOLARSYS_STATE* solarSys);
static bool GenerateShofixti_generateMoons(SOLARSYS_STATE* solarSys,
										   PLANET_DESC* planet);
static bool GenerateShofixti_generateName(const SOLARSYS_STATE*,
										  const PLANET_DESC* world);
static bool GenerateShofixti_generateOrbital(SOLARSYS_STATE* solarSys,
											 PLANET_DESC* world);

static void check_old_shofixti(void);


const GenerateFunctions generateShofixtiFunctions = {
	/* .initNpcs         = */ GenerateShofixti_initNpcs,
	/* .reinitNpcs       = */ GenerateShofixti_reinitNpcs,
	/* .uninitNpcs       = */ GenerateShofixti_uninitNpcs,
	/* .generatePlanets  = */ GenerateShofixti_generatePlanets,
	/* .generateMoons    = */ GenerateShofixti_generateMoons,
	/* .generateName     = */ GenerateShofixti_generateName,
	/* .generateOrbital  = */ GenerateShofixti_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateShofixti_initNpcs(SOLARSYS_STATE* solarSys)
{
	if (!uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_RECRUITED)
		&& (!uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_KIA)
			|| (!uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_BRO_KIA)
				&& uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP))))
	{
		uqm::gameState().game().BattleGroupRef = uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_GRPOFFS);
		if (uqm::gameState().game().BattleGroupRef == 0
			|| !GetGroupInfo(uqm::gameState().game().BattleGroupRef, GroupInitIp))
		{
			HSHIPFRAG hStarShip {};

			if (uqm::gameState().game().BattleGroupRef == 0)
			{
				uqm::gameState().game().BattleGroupRef = ~0L;
			}

			hStarShip = CloneShipFragment(SHOFIXTI_SHIP,
										  &uqm::gameState().game().npc_built_ship_q, 1);
			if (hStarShip)
			{ /* Set old Shofixti name; his brother if Tanaka died */
				SHIP_FRAGMENT* FragPtr = LockShipFrag(
					&uqm::gameState().game().npc_built_ship_q, hStarShip);
				/* Name Tanaka or Katana (+1) */
				FragPtr->captains_name_index =
					NameOffset + NumCaptainsNames + (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_KIA) & 1);
				UnlockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip);
			}

			uqm::gameState().game().BattleGroupRef = PutGroupInfo(
				uqm::gameState().game().BattleGroupRef, 1);
			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
			uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_GRPOFFS, static_cast<uint32_t>(uqm::gameState().game().BattleGroupRef));
		}
	}

	// This was originally a fallthrough to REINIT_NPCS.
	// XXX: is the call to check_old_shofixti() needed?
	GenerateDefault_initNpcs(solarSys);
	check_old_shofixti();

	return true;
}

static bool GenerateShofixti_reinitNpcs(SOLARSYS_STATE* solarSys)
{
	GenerateDefault_reinitNpcs(solarSys);
	check_old_shofixti();

	(void)solarSys;
	return true;
}

static bool GenerateShofixti_uninitNpcs(SOLARSYS_STATE* solarSys)
{
	if (uqm::gameState().game().BattleGroupRef
		&& !uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_RECRUITED)
		&& GetHeadLink(&uqm::gameState().game().ip_group_q) == 0)
	{
		if (!uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_KIA))
		{
			uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_KIA, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_VISITS, static_cast<uint32_t>(0));
			if (isDifficulty(uqm::Difficulty::Hard))
			{
				uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_BRO_KIA, static_cast<uint32_t>(1));
			}
		}
		else if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_BRO_KIA, static_cast<uint32_t>(1));
		}
	}

	GenerateDefault_uninitNpcs(solarSys);
	return true;
}

static constexpr auto NumPlanets {6};

static bool GenerateShofixti_generatePlanets(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];
	uint16_t i {};

	pSunDesc->PlanetByte = 0;
	pSunDesc->MoonByte = 0;

	if (uqm::isOriginalSeed())
	{
		pSunDesc->NumPlanets = NumPlanets;
		for (i = 0; i < NumPlanets; ++i)
		{
			PLANET_DESC* pCurDesc = &solarSys->PlanetDesc[i];

			pCurDesc->NumPlanets = 0;
			if (i < (NumPlanets >> 1))
			{
				pCurDesc->data_index = SELENIC_WORLD;
			}
			else
			{
				pCurDesc->data_index = METAL_WORLD;
			}
		}

		FillOrbits(solarSys, NumPlanets, solarSys->PlanetDesc, true);
	}
	else
	{
		uint32_t rand_val = SysGenRNG->next();
		uint8_t planet = 0;

		pSunDesc->NumPlanets = GenerateMinPlanets(2);
		if (pSunDesc->NumPlanets > 9)
		{
			pSunDesc->NumPlanets = 9;
		}

		FillOrbits(solarSys, NumplanetsPdesc, nullptr, false);

		if (uqm::isStarSeed())
		{
			pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
		}

		// The only benefit to pre-stamping is that it shuffles them.
		// But this causes problems when the star is orange, due to metal
		// type. Easier to seed them then stamp them in a shuffled loop so
		// that all types work. This relies on MaxPlanets <= 9 for
		// randomness to work, otherwise you'll have to count the bitshifts
		// and re-random every 4th time.
		for (i = 0; i < pSunDesc->NumPlanets; ++i)
		{
			solarSys->PlanetDesc[i].data_index = METAL_WORLD;
		}
		for (i = 0; i < pSunDesc->NumPlanets / 2; ++i)
		{
			uint8_t offset = uqm::lowByte(rand_val) % (pSunDesc->NumPlanets - i);

			while (solarSys->PlanetDesc[planet].data_index == SELENIC_WORLD || offset > 0)
			{
				if (solarSys->PlanetDesc[planet].data_index == METAL_WORLD)
				{
					offset--;
				}
				planet = (planet + 1) % pSunDesc->NumPlanets;
			}

			solarSys->PlanetDesc[planet].data_index = SELENIC_WORLD;
			rand_val = rand_val >> 8;
		}
	}

	if (getNomadMode() != uqm::NomadMode::Off && CheckAlliance(SHOFIXTI_SHIP) == GOOD_GUY)
	{
		solarSys->PlanetDesc[pSunDesc->PlanetByte].NumPlanets = 1;
	}

	return true;
}

static bool GenerateShofixti_generateMoons(SOLARSYS_STATE* solarSys,
										   PLANET_DESC* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (getNomadMode() != uqm::NomadMode::Off && matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		PLANET_DESC* pMoonDesc = &solarSys->MoonDesc[MoonByte];

		pMoonDesc->data_index = HIERARCHY_STARBASE;
	}

	return true;
}

static bool GenerateShofixti_generateName(const SOLARSYS_STATE* solarSys,
										  const PLANET_DESC* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const PLANET_DESC pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(PLANET_NUMBER_BASE + 35));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateShofixti_generateOrbital(SOLARSYS_STATE* solarSys,
											 PLANET_DESC* world)
{
	if (getNomadMode() != uqm::NomadMode::Off && matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		if (CheckAlliance(SHOFIXTI_SHIP) == GOOD_GUY)
		{
			const bool MaxShips = (CountEscortShips(SHOFIXTI_SHIP) < ifHardDifficulty(1, 2) ? true : false);
			const bool RoomInFleet = EscortFeasibilityStudy(SHOFIXTI_SHIP);
			const uint8_t Index = !MaxShips ? 0 : (!RoomInFleet ? 1 : 2);

			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(ShofixtiBaseStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = Index;

			DoDiscoveryReport(MenuSounds);

			if (Index == 2)
			{
				AddEscortShips(SHOFIXTI_SHIP, (
												  CountEscortShips(SHOFIXTI_SHIP) == 1 ? 1 : 2));
			}

			solarSys->SysInfo.PlanetInfo.DiscoveryStrings.reset();
			FreeLanderFont(&solarSys->SysInfo.PlanetInfo);
			return true;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

static void check_old_shofixti(void)
{
	if (!uqm::gameState().game().BattleGroupRef)
	{
		return; // nothing to check
	}

	HIPGROUP hGroup {GetHeadLink(&uqm::gameState().game().ip_group_q)};
	if (!hGroup)
	{
		return; // still nothing to check
	}

	IP_GROUP* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
	// REFORM_GROUP was set in ipdisp.c:ip_group_collision()
	// during a collision with the flagship.
	if (GroupPtr->race_id == SHOFIXTI_SHIP
		&& (GroupPtr->task & REFORM_GROUP)
		&& uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_RECRUITED))
	{
		GroupPtr->task = FLEE | IGNORE_FLAGSHIP | REFORM_GROUP;
		GroupPtr->dest_loc = 0;
	}
	UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
}