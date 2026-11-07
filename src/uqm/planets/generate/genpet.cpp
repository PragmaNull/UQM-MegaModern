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
#include "../planets.h"
#include "../../build.h"
#include "../../comm.h"
#include "../../encount.h"
#include "../../starmap.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../state.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static bool GenerateTalkingPet_generatePlanets(SolarSystemState* solarSys);
static bool GenerateTalkingPet_generateOrbital(SolarSystemState* solarSys,
											   SystemPlanet* world);
static uint16_t GenerateTalkingPet_generateEnergy(const SolarSystemState*,
												  const SystemPlanet* world, uint16_t whichNode, SurfaceNode*);
static bool GenerateTalkingPet_pickupEnergy(SolarSystemState* solarSys,
											SystemPlanet* world, uint16_t whichNode);

static void ZapToUrquanEncounter(void);


const GenerateFunctions generateTalkingPetFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateTalkingPet_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateTalkingPet_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateTalkingPet_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateTalkingPet_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool GenerateTalkingPet_generatePlanets(SolarSystemState* solarSys)
{
	SystemPlanet* pPlanet {};
	SystemPlanet* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = *PlanetType::Telluric;
		pPlanet->radius = EarthRadius * 204L / 100;
		const uint16_t angle {ARCTAN(pPlanet->location.x, pPlanet->location.y)};
		pPlanet->location.x = Cosine(angle, pPlanet->radius);
		pPlanet->location.y = Sine(angle, pPlanet->radius);
		ComputeSpeed(pPlanet, false, 1);
	}
	else
	{
		pSunDesc->PlanetByte = PickClosestHabitable(solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld();
	}

	return true;
}

static bool GenerateTalkingPet_generateOrbital(SolarSystemState* solarSys,
											   SystemPlanet* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& (uqm::gameState().flag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES)
			|| !uqm::gameState().flag(uqm::GameFlag::TALKING_PET)
			|| StartSphereTracking(RaceId::Umgah)))
	{
		NotifyOthers(*RaceId::Umgah, IpnlAllClear);
		PutGroupInfo(GroupsRandom, GroupSaveIp);
		ReinitQueue(&uqm::gameState().game().ip_group_q);
		assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

		if (StartSphereTracking(RaceId::Umgah))
		{
			uqm::currentActivity() |= ActivityFlag::StartInterplanetary;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
			if (!uqm::gameState().flag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES))
			{
				CloneShipFragment(RaceId::Umgah,
								  &uqm::gameState().game().npc_built_ship_q, InfiniteFleet);
				InitCommunication(Conversation::Umgah);
			}
			else
			{
				for (uint16_t i {0}; i < 10; ++i)
				{
					CloneShipFragment(RaceId::Umgah,
									  &uqm::gameState().game().npc_built_ship_q, 0);
				}
				InitCommunication(Conversation::TalkingPet);
			}
		}

		if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
		{
			bool UmgahSurvivors {};

			UmgahSurvivors = GetHeadLink(
								 &uqm::gameState().game().npc_built_ship_q)
						  != 0;
			uqm::currentActivity() &= ~ActivityFlag::StartInterplanetary;

			if (uqm::gameState().flag(uqm::GameFlag::PLAYER_HYPNOTIZED))
			{
				ZapToUrquanEncounter();
			}
			else if (uqm::gameState().flag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES)
					 && !UmgahSurvivors)
			{
				// Defeated the zombie fleet.
				InitCommunication(Conversation::TalkingPet);
			}
			else if (!(StartSphereTracking(RaceId::Umgah)))
			{
				// The Kohr-Ah have destroyed the Umgah, but the
				// talking pet survived.
				InitCommunication(Conversation::TalkingPet);
			}

			ReinitQueue(&uqm::gameState().game().npc_built_ship_q);
			GetGroupInfo(GroupsRandom, GroupLoadIp);
		}

		return true;
	}

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable(LoadGraphic(RuinsMaskPmapAnimRes));
		solarSys->SysInfo.PlanetInfo.DiscoveryStrings = uqm::loadStringTable(RuinsStrtabRes);
		solarSys->SysInfo.PlanetInfo.DiscoveryIndex = 0;
	}

	GenerateDefault_generateOrbital(solarSys, world);

	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		solarSys->SysInfo.PlanetInfo.Weather = 0;
	}

	return true;
}

static uint16_t GenerateTalkingPet_generateEnergy(const SolarSystemState* solarSys,
												  const SystemPlanet* world, uint16_t whichNode, SurfaceNode* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}

static bool GenerateTalkingPet_pickupEnergy(SolarSystemState* solarSys,
											SystemPlanet* world, uint16_t whichNode)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle(solarSys);
		return false;
	}

	(void)whichNode;
	return false;
}

static constexpr auto LostDays {15};

static void ZapToUrquanEncounter(void)
{
	EncounterHandle hEncounter {};

	if ((hEncounter = AllocEncounter())
		|| (hEncounter = GetHeadEncounter()))
	{
		int16_t dx {};
		int16_t dy {};
		Encounter* EncounterPtr {};
		FleetInfoHandle hStarShip {};

		LockEncounter(hEncounter, &EncounterPtr);

		if (hEncounter == GetHeadEncounter())
		{
			RemoveEncounter(hEncounter);
		}
		memset(EncounterPtr, 0, sizeof(*EncounterPtr));

		InsertEncounter(hEncounter, GetHeadEncounter());

		hStarShip = GetStarShipFromIndex(
			&uqm::gameState().game().avail_race_q, *RaceId::Urquan);
		const FleetInfo* TemplatePtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
		EncounterPtr->origin = TemplatePtr->loc;
		EncounterPtr->radius = TemplatePtr->actual_strength;
		EncounterPtr->race_id = *RaceId::Urquan;
		EncounterPtr->num_ships = 1;
		EncounterPtr->flags = OneShotEncounter;
		BriefShipInfo* BSIPtr {&EncounterPtr->ShipList[0]};
		BSIPtr->race_id = *RaceId::Urquan;
		BSIPtr->crew_level = TemplatePtr->crew_level;
		BSIPtr->max_crew = TemplatePtr->max_crew;
		BSIPtr->max_energy = TemplatePtr->max_energy;

		FleetInfoHandle hUmgah = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Umgah);
		FleetInfo* UmgahPtr =
			LockFleetInfo(&uqm::gameState().game().avail_race_q, hUmgah);

		EncounterPtr->loc_pt = SeedFleetLocation(UmgahPtr, plot_map, *PlotId::Samatra);
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hUmgah);

		EncounterPtr->log_x = UniverseToLogx(EncounterPtr->loc_pt.x);
		EncounterPtr->log_y = UniverseToLogy(EncounterPtr->loc_pt.y);
		uqm::gameState().sis().log_x = EncounterPtr->log_x;
		uqm::gameState().sis().log_y = EncounterPtr->log_y;
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);

		{
			SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond * 2));
			MoveGameClockDays(LostDays);
		}

		uqm::currentActivity() =
			uqm::makeWord(ActivityKind::InHyperspace, 0) | ActivityFlag::StartEncounter;
		SetBattlePlanet();

		dx = CurStarDescPtr()->star_pt.x - EncounterPtr->loc_pt.x;
		dy = CurStarDescPtr()->star_pt.y - EncounterPtr->loc_pt.y;
		dx = (int16_t)square_root((long)dx * dx + (long)dy * dy)
		   + (FuelTankScale >> 1);

		if (!uqm::UQMOptions::read().infiniteFuel)
		{
			DeltaSISGauges(0, -dx, 0);
		}

		if (uqm::gameState().sis().FuelOnBoard < 5 * FuelTankScale)
		{
			dx = ((5 + ((uint16_t)uqm::random().next() % 5)) * FuelTankScale)
			   - (int16_t)uqm::gameState().sis().FuelOnBoard;
			DeltaSISGauges(0, dx, 0);
		}
		DrawSISMessage(nullptr);
		// DrawHyperCoords (EncounterPtr->loc_pt);
		uqm::flagship().shipStamp.origin = EncounterPtr->loc_pt;
		uqm::flagship().shipFacing = 6;


		UnlockEncounter(hEncounter);
	}
}

} // namespace uqm