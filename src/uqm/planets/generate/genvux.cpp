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
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../lander.h"
#include "../lifeform.h"
#include "../planets.h"
#include "genall.h"


namespace uqm
{
static bool GenerateVux_generatePlanets(SolarSystemState* solarSys);
static bool GenerateVux_generateOrbital(SolarSystemState* solarSys,
										SystemPlanet* world);
static uint16_t GenerateVux_generateEnergy(const SolarSystemState*,
										   const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static uint16_t GenerateVux_generateLife(const SolarSystemState*,
										 const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateVux_pickupEnergy(SolarSystemState*, SystemPlanet* world,
									 uint16_t whichNode);
static bool GenerateVux_pickupLife(SolarSystemState*, SystemPlanet* world,
								   uint16_t whichNode);


const GenerateFunctions generateVuxFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateVux_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateVux_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateVux_generateEnergy,
	/* .generateLife     = */ GenerateVux_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateVux_pickupEnergy,
	/* .pickupLife       = */ GenerateVux_pickupLife,
};


static bool GenerateVux_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	pSunDesc->PlanetByte = 0;
	SystemPlanet* pPlanet {&solarSys->PlanetDesc[pSunDesc->PlanetByte]};

	if (uqm::isOriginalSeed())
	{
		uint16_t angle {};

		if (CurStarDescPtr()->Index == PlotId::Maidens)
		{
			GenerateDefault_generatePlanets(solarSys);
			// XXX: this is the second time that this function is
			// called. Is it safe to remove one, or does this change
			// the RNG so that the outcome is different?
			pPlanet->data_index = *PlanetType::Redux;
			pPlanet->radius = EarthRadius * 212L / 100;
			angle = ARCTAN(pPlanet->location.x, pPlanet->location.y);
			pPlanet->location.x = Cosine(angle, pPlanet->radius);
			pPlanet->location.y = Sine(angle, pPlanet->radius);
		}
		else
		{
			if (CurStarDescPtr()->Index == PlotId::Vux)
			{
				pPlanet->data_index = *PlanetType::Redux;
				pPlanet->NumPlanets = 1;
				pPlanet->radius = EarthRadius * 42L / 100;
				angle = HalfCircle + Octant;
			}
			else /* if (CurStarDescPtr()->Index == VUX_BEAST_DEFINED) */
			{
				memmove(&solarSys->PlanetDesc[1],
						&solarSys->PlanetDesc[0],
						sizeof(solarSys->PlanetDesc[0])
							* pSunDesc->NumPlanets);
				++pSunDesc->NumPlanets;

				angle = HalfCircle - Octant;
				pPlanet->data_index = *PlanetType::Water;
				pPlanet->radius = EarthRadius * 110L / 100;
				pPlanet->NumPlanets = 0;
			}

			pPlanet->location.x = Cosine(angle, pPlanet->radius);
			pPlanet->location.y = Sine(angle, pPlanet->radius);
			pPlanet->rand_seed = uqm::makeDword(
				pPlanet->location.x,
				pPlanet->location.y);
		}

		ComputeSpeed(pPlanet, false, 1);
	}
	else
	{
		if (uqm::isStarSeed())
		{
			pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];
		}
		else
		{
			CheckForHabitable(solarSys);
		}

		pPlanet->data_index = GenerateHabitableWorld();
	}

	return true;
}

static bool GenerateVux_generateOrbital(SolarSystemState* solarSys, SystemPlanet* world)
{
	if ((matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		 && (CurStarDescPtr()->Index == PlotId::Vux
			 || (CurStarDescPtr()->Index == PlotId::Maidens
				 && !uqm::gameState().flag(uqm::GameFlag::ZEX_IS_DEAD))))
		&& StartSphereTracking(RaceId::Vux))
	{
		NotifyOthers(*RaceId::Vux, IpnlAllClear);
		PutGroupInfo(GroupsRandom, GroupSaveIp);
		ReinitQueue(&uqm::gameState().game().ip_group_q);
		assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

		CloneShipFragment(RaceId::Vux,
						  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);
		if (CurStarDescPtr()->Index == PlotId::Vux)
		{
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
		}
		else
		{
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 6));
		}

		uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
		InitCommunication(Conversation::Vux);

		if (uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
		{
			return true;
		}

		{
			uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;
			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
			GetGroupInfo(GroupsRandom, GroupLoadIp);

			if (CurStarDescPtr()->Index == PlotId::Vux
				|| !uqm::gameState().flag(uqm::GameFlag::ZEX_IS_DEAD))
			{
				return true;
			}

			RepairSISBorder();
		}
	}

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (CurStarDescPtr()->Index == PlotId::Maidens)
		{
			if (!uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_MAIDENS))
			{
				LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
				solarSys->PlanetSideFrame[1] = CaptureDrawable(
					LoadGraphic(MaidensMaskPmapAnimRes));
				solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(MaidensStrtabRes);
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
			}
		}
		else if (CurStarDescPtr()->Index == PlotId::VuxBeast)
		{
			if (!uqm::gameState().flag(uqm::GameFlag::VUX_BEAST))
			{
				LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
				solarSys->PlanetSideFrame[1] = 0;
				solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(BeastStrtabRes);
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
			}
		}
		else // if (CurStarDescPtr()->Index == VUX_DEFINED)
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
		}
	}

	GenerateDefault_generateOrbital(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& !isDifficulty(uqm::Difficulty::Hard))
	{
		solarSys->SysInfo.PlanetInfo.Weather = 2;
		solarSys->SysInfo.PlanetInfo.Tectonics = 0;
	}

	return true;
}

static uint16_t GenerateVux_generateEnergy(const SolarSystemState* solarSys,
										   const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (CurStarDescPtr()->Index == PlotId::Maidens
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_MAIDENS))
		{ // already picked up
			return 0;
		}

		if (info)
		{
			info->loc_pt.x = ScaledMapWidth() / 3;
			info->loc_pt.y = MapHeight() * 5 / 8;
		}

		return 1; // only matters when count is requested
	}

	if (CurStarDescPtr()->Index == PlotId::Vux
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateVux_pickupEnergy(SolarSystemState* solarSys, SystemPlanet* world,
									 uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == PlotId::Maidens
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		assert(!uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_MAIDENS) && whichNode == 0);

		GenerateDefault_landerReport(solarSys);
		SetLanderTakeoff();

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_MAIDENS, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::MAIDENS_ON_SHIP, static_cast<uint32_t>(1));

		return true; // picked up
	}

	if (CurStarDescPtr()->Index == PlotId::Vux
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);
		return false;
	}

	(void)whichNode;
	return false;
}

static auto LifeBool() { return (IsExtended() && uqm::gameState().flag(uqm::GameFlag::VUX_BEAST)); }

static uint16_t GenerateVux_generateLife(const SolarSystemState* solarSys,
										 const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (CurStarDescPtr()->Index == PlotId::Maidens
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const static uqstl::array<int8_t, 13> life {
			9, 9, 9, 9,		/* Carousel Beast */
			14, 14, 14, 14, /* Amorphous Trandicula */
			18, 18, 18, 18, /* Penguin Cyclops */
			-1				/* term */
		};
		const static uqstl::array<int8_t, 12> lifeEx {
			9, 9, 9, 9, 9,		/* Carousel Beast */
			18, 18, 18, 18, 18, /* Penguin Cyclops */
			ZexBeauty,			/* VUX Beast */
			-1					/* term */
		};

		return GeneratePresetLife(
			&solarSys->SysInfo, IsExtended() ? lifeEx.data() : life.data(),
			whichNode, info);
	}

	if (CurStarDescPtr()->Index == PlotId::VuxBeast
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		const static uqstl::array<int8_t, 12> life {
			ZexBeauty,	   /* VUX Beast */
						   // Must be the first node, see pickupLife() below
			3, 3, 3, 3, 3, /* Whackin' Bush */
			8, 8, 8, 8, 8, /* Glowing Medusa */
			-1			   /* term */
		};
		return GeneratePresetLife(
			&solarSys->SysInfo, life.data(), whichNode, info);
	}

	return GenerateDefault_generateLife(solarSys, world, whichNode, info);
}

static bool GenerateVux_pickupLife(SolarSystemState* solarSys, SystemPlanet* world,
								   uint16_t whichNode)
{
	if (CurStarDescPtr()->Index == PlotId::VuxBeast
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (whichNode == 0)
		{ // Picked up Zex' Beauty
			if (uqm::gameState().flag(uqm::GameFlag::VUX_BEAST) && uqm::isStarSeed())
			{
				return true; // no need to core, just pick it up
			}
			assert(!uqm::gameState().flag(uqm::GameFlag::VUX_BEAST));

			GenerateDefault_landerReport(solarSys);
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST_ON_SHIP, static_cast<uint32_t>(1));
		}

		return true; // picked up
	}

	if (LifeBool() && CurStarDescPtr()->Index == PlotId::Maidens
		&& matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		if (whichNode == 10)
		{ // Picked up Zex' Beauty... Again.
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(BeastStrtabRes);
			solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 1;

			GenerateDefault_landerReport(solarSys);
			SetLanderTakeoff();

			uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST, static_cast<uint32_t>(2));
			uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST_ON_SHIP, static_cast<uint32_t>(2));

			if (!uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_MAIDENS))
			{ // Reinitialize the Maiden's report as the Beast report
				// overrides it
				LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
				solarSys->PlanetSideFrame[1] = CaptureDrawable(
					LoadGraphic(MaidensMaskPmapAnimRes));
				solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(MaidensStrtabRes);
				solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
			}
		}

		return true; // picked up
	}

	return GenerateDefault_pickupLife(solarSys, world, whichNode);
}

} // namespace uqm