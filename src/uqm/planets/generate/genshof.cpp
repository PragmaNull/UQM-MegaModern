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
#include "../../gamestr.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../istrtab.h"
#include "../../sounds.h"
#include "../../state.h"
#include "../lander.h"
#include "../planets.h"
#include "genall.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateShofixti_initNpcs(SolarSystemState* solarSys);
static bool GenerateShofixti_reinitNpcs(SolarSystemState* solarSys);
static bool GenerateShofixti_uninitNpcs(SolarSystemState* solarSys);
static bool GenerateShofixti_generatePlanets(SolarSystemState* solarSys);
static bool GenerateShofixti_generateMoons(SolarSystemState* solarSys,
										   SystemPlanet* planet);
static bool GenerateShofixti_generateName(const SolarSystemState*,
										  const SystemPlanet* world);
static bool GenerateShofixti_generateOrbital(SolarSystemState* solarSys,
											 SystemPlanet* world);

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


static bool GenerateShofixti_initNpcs(SolarSystemState* solarSys)
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
			ShipFragmentHandle hStarShip {};

			if (uqm::gameState().game().BattleGroupRef == 0)
			{
				uqm::gameState().game().BattleGroupRef = ~0L;
			}

			hStarShip = CloneShipFragment(RaceId::Shofixti,
										  &uqm::gameState().game().npc_built_ship_q, 1);
			if (hStarShip)
			{ /* Set old Shofixti name; his brother if Tanaka died */
				ShipFragment* FragPtr = LockShipFrag(
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

static bool GenerateShofixti_reinitNpcs(SolarSystemState* solarSys)
{
	GenerateDefault_reinitNpcs(solarSys);
	check_old_shofixti();

	(void)solarSys;
	return true;
}

static bool GenerateShofixti_uninitNpcs(SolarSystemState* solarSys)
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

static bool GenerateShofixti_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];
	uint16_t i {};

	pSunDesc->PlanetByte = 0;
	pSunDesc->MoonByte = 0;

	if (uqm::isOriginalSeed())
	{
		pSunDesc->NumPlanets = NumPlanets;
		for (i = 0; i < NumPlanets; ++i)
		{
			SystemPlanet* pCurDesc = &solarSys->PlanetDesc[i];

			pCurDesc->NumPlanets = 0;
			if (i < (NumPlanets >> 1))
			{
				pCurDesc->data_index = *PlanetType::Selenic;
			}
			else
			{
				pCurDesc->data_index = *PlanetType::Metal;
			}
		}

		FillOrbits(solarSys, NumPlanets, solarSys->PlanetDesc.data(), true);
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
			solarSys->PlanetDesc[i].data_index = *PlanetType::Metal;
		}
		for (i = 0; i < pSunDesc->NumPlanets / 2; ++i)
		{
			uint8_t offset = uqm::lowByte(rand_val) % (pSunDesc->NumPlanets - i);

			while (solarSys->PlanetDesc[planet].data_index == PlanetType::Selenic || offset > 0)
			{
				if (solarSys->PlanetDesc[planet].data_index == PlanetType::Metal)
				{
					offset--;
				}
				planet = (planet + 1) % pSunDesc->NumPlanets;
			}

			solarSys->PlanetDesc[planet].data_index = *PlanetType::Selenic;
			rand_val = rand_val >> 8;
		}
	}

	if (getNomadMode() != uqm::NomadMode::Off && CheckAlliance(RaceId::Shofixti) == RaceAllegiance::Good)
	{
		solarSys->PlanetDesc[pSunDesc->PlanetByte].NumPlanets = 1;
	}

	return true;
}

static bool GenerateShofixti_generateMoons(SolarSystemState* solarSys,
										   SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (getNomadMode() != uqm::NomadMode::Off && matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;
		SystemPlanet* pMoonDesc = &solarSys->MoonDesc[MoonByte];

		pMoonDesc->data_index = *PlanetType::HierarchyStarbase;
	}

	return true;
}

static bool GenerateShofixti_generateName(const SolarSystemState* solarSys,
										  const SystemPlanet* world)
{
	GenerateDefault_generateName(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const uint8_t PlanetByte = solarSys->SunDesc[0].PlanetByte;
		const SystemPlanet pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		uqm::utf8StringCopy(uqm::gameState().sis().PlanetName.data(),
							sizeof(uqm::gameState().sis().PlanetName),
							GameString(GameStringBase::PlanetNumber + 35));

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(pPlanetDesc.data_index));
	}

	return true;
}

static bool GenerateShofixti_generateOrbital(SolarSystemState* solarSys,
											 SystemPlanet* world)
{
	if (getNomadMode() != uqm::NomadMode::Off && matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		if (CheckAlliance(RaceId::Shofixti) == RaceAllegiance::Good)
		{
			const bool MaxShips = (CountEscortShips(RaceId::Shofixti) < ifHardDifficulty(1, 2) ? true : false);
			const bool RoomInFleet = EscortFeasibilityStudy(RaceId::Shofixti);
			const uint8_t Index = !MaxShips ? 0 : (!RoomInFleet ? 1 : 2);

			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(ShofixtiBaseStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = Index;

			DoDiscoveryReport(MenuSounds);

			if (Index == 2)
			{
				AddEscortShips(RaceId::Shofixti, (
													 CountEscortShips(RaceId::Shofixti) == 1 ? 1 : 2));
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

	IpGroupHandle hGroup {GetHeadLink(&uqm::gameState().game().ip_group_q)};
	if (!hGroup)
	{
		return; // still nothing to check
	}

	InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
	// REFORM_GROUP was set in ipdisp.c:ip_group_collision()
	// during a collision with the flagship.
	if (GroupPtr->race_id == RaceId::Shofixti
		&& (GroupPtr->task & GroupTaskFlag::ReformGroup)
		&& uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_RECRUITED))
	{
		GroupPtr->task = *GroupTask::Flee | GroupTaskFlag::IgnoreFlagship | GroupTaskFlag::ReformGroup;
		GroupPtr->dest_loc = 0;
	}
	UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
}

} // namespace uqm