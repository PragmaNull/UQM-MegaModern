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

#include <ctype.h>

#include "../../build.h"
#include "../../comm.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../lander.h"
#include "core/log/log.h"
#include "genall.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateMelnorme_initNpcs(SolarSystemState* solarSys);
static bool GenerateMelnorme_generatePlanets(SolarSystemState* solarSys);
static bool GenerateMelnorme_generateMoons(SolarSystemState* solarSys,
										   SystemPlanet* planet);
static bool GenerateMelnorme_generateOrbital(SolarSystemState* solarSys,
											 SystemPlanet* world);
static uint16_t GenerateMelnorme_generateEnergy(const SolarSystemState*,
												const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateMelnorme_pickupEnergy(SolarSystemState* solarSys,
										  SystemPlanet* world, uint16_t whichNode);

static uint32_t GetMelnormeRef(void);
static void SetMelnormeRef(uint32_t Ref);


const GenerateFunctions generateMelnormeFunctions = {
	/* .initNpcs         = */ GenerateMelnorme_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateMelnorme_generatePlanets,
	/* .generateMoons    = */ GenerateMelnorme_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateMelnorme_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateMelnorme_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateMelnorme_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateMelnorme_initNpcs(SolarSystemState* solarSys)
{
	if ((IsExtended() && !uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
		|| !IsExtended())
	{
		uqm::gameState().game().BattleGroupRef = GetMelnormeRef();
		if (uqm::gameState().game().BattleGroupRef == 0)
		{
			CloneShipFragment(RaceId::Melnorme, &uqm::gameState().game().npc_built_ship_q, 0);
			uqm::gameState().game().BattleGroupRef = PutGroupInfo(GroupsAddNew, 1);
			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
			SetMelnormeRef(uqm::gameState().game().BattleGroupRef);
		}
	}

	GenerateDefault_initNpcs(solarSys);

	return true;
}

static bool GenerateMelnorme_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pPlanet {};
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	if (CurStarDescPtr()->Index == PlotId::Melnorme1)
	{
		pSunDesc->PlanetByte = 2;
		pSunDesc->MoonByte = 0;

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (IsExtended() && pPlanet->NumPlanets < MaxGenMoons)
		{
			pPlanet->NumPlanets++;
		}

		if (!uqm::isOriginalSeed())
		{
			pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateCrystalWorld();
		}
	}

	if (CurStarDescPtr()->Index == PlotId::Melnorme7)
	{
		pSunDesc->PlanetByte = 3;

		if (!uqm::isOriginalSeed())
		{
			pSunDesc->PlanetByte = PlanetByteGen(pSunDesc);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateWorlds(AllRocky);
		}
	}

	return true;
}

static bool GenerateMelnorme_generateMoons(SolarSystemState* solarSys,
										   SystemPlanet* planet)
{
	GenerateDefault_generateMoons(solarSys, planet);

	if (IsExtended() && CurStarDescPtr()->Index == PlotId::Melnorme1
		&& matchWorld(solarSys, planet, MatchPbyte, MatchPlanet))
	{
		const uint8_t MoonByte = solarSys->SunDesc[0].MoonByte;

		solarSys->MoonDesc[MoonByte].data_index = *PlanetType::PrecursorStarbase;
	}

	return true;
}

static bool GenerateMelnorme_generateOrbital(SolarSystemState* solarSys,
											 SystemPlanet* world)
{
	if (IsExtended() && CurStarDescPtr()->Index == PlotId::Melnorme1 && matchWorld(solarSys, world, MatchPbyte, MatchMbyte))
	{
		/* Starbase */
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(PrecursorBaseStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;

		DoDiscoveryReport(MenuSounds);

		solarSys->SysInfo.PlanetInfo.DiscoveryStrings.reset();
		FreeLanderFont(&solarSys->SysInfo.PlanetInfo);

		return true;
	}

	if (IsExtended() && CurStarDescPtr()->Index == PlotId::Melnorme7 && matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(SteleMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(SteleStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	GenerateDefault_generateOrbital(solarSys, world);

	return true;
}

static uint16_t GenerateMelnorme_generateEnergy(const SolarSystemState* solarSys,
												const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (IsExtended() && CurStarDescPtr()->Index == PlotId::Melnorme7
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateArtifact(
			solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateMelnorme_pickupEnergy(SolarSystemState* solarSys,
										  SystemPlanet* world, uint16_t whichNode)
{
	if (IsExtended() && CurStarDescPtr()->Index == PlotId::Melnorme7 && matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(whichNode == 0);

		GenerateDefault_landerReport(solarSys);

		// The cheese can not be "picked up". It is always on the surface.
		return false;
	}

	(void)whichNode;
	return false;
}

const char* SelectMelnormeRefVar(void)
{
	switch (static_cast<PlotId>(CurStarDescPtr()->Index))
	{
		case PlotId::Melnorme0:
			return "MELNORME0_GRPOFFS";
		case PlotId::Melnorme1:
			return "MELNORME1_GRPOFFS";
		case PlotId::Melnorme2:
			return "MELNORME2_GRPOFFS";
		case PlotId::Melnorme3:
			return "MELNORME3_GRPOFFS";
		case PlotId::Melnorme4:
			return "MELNORME4_GRPOFFS";
		case PlotId::Melnorme5:
			return "MELNORME5_GRPOFFS";
		case PlotId::Melnorme6:
			return "MELNORME6_GRPOFFS";
		case PlotId::Melnorme7:
			return "MELNORME7_GRPOFFS";
		case PlotId::Melnorme8:
			return "MELNORME8_GRPOFFS";
		default:
			return "0";
	}
}

static uint32_t GetMelnormeRef(void)
{
	const char* RefVar = SelectMelnormeRefVar();

	if (isdigit(RefVar[0]))
	{
		uqm::log::warn("GetMelnormeRef(): reference unknown");
		return 0;
	}

	return DGetGameState(RefVar);
}

static void SetMelnormeRef(uint32_t Ref)
{
	const char* RefVar = SelectMelnormeRefVar();

	if (isdigit(RefVar[0]))
	{
		uqm::log::warn("SetMelnormeRef(): reference unknown");
		return;
	}

	DSetGameState(RefVar, Ref);
}

} // namespace uqm