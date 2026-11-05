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
#include "../../setup.h"
#include "../../state.h"
#include "libs/mathlib.h"
#include "../../../options.h"
#include "uqm/random/UQMRandom.h"


static bool GenerateTalkingPet_generatePlanets(SOLARSYS_STATE* solarSys);
static bool GenerateTalkingPet_generateOrbital(SOLARSYS_STATE* solarSys,
											   PLANET_DESC* world);
static uint16_t GenerateTalkingPet_generateEnergy(const SOLARSYS_STATE*,
												  const PLANET_DESC* world, uint16_t whichNode, NODE_INFO*);
static bool GenerateTalkingPet_pickupEnergy(SOLARSYS_STATE* solarSys,
											PLANET_DESC* world, uint16_t whichNode);

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


static bool
GenerateTalkingPet_generatePlanets(SOLARSYS_STATE* solarSys)
{
	PLANET_DESC* pPlanet {};
	PLANET_DESC* pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets(solarSys);

	if (uqm::isOriginalSeed())
	{
		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = TELLURIC_WORLD;
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

static bool
GenerateTalkingPet_generateOrbital(SOLARSYS_STATE* solarSys,
								   PLANET_DESC* world)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet)
		&& (GET_GAME_STATE(UMGAH_ZOMBIE_BLOBBIES)
			|| !GET_GAME_STATE(TALKING_PET)
			|| StartSphereTracking(UMGAH_SHIP)))
	{
		NotifyOthers(UMGAH_SHIP, IpnlAllClear);
		PutGroupInfo(GroupsRandom, GroupSaveIp);
		ReinitQueue(&GLOBAL(ip_group_q));
		assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

		if (StartSphereTracking(UMGAH_SHIP))
		{
			uqm::currentActivity() |= START_INTERPLANETARY;
			SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 7);
			if (!GET_GAME_STATE(UMGAH_ZOMBIE_BLOBBIES))
			{
				CloneShipFragment(UMGAH_SHIP,
								  &GLOBAL(npc_built_ship_q), INFINITE_FLEET);
				InitCommunication(UMGAH_CONVERSATION);
			}
			else
			{
				for (uint16_t i {0}; i < 10; ++i)
				{
					CloneShipFragment(UMGAH_SHIP,
									  &GLOBAL(npc_built_ship_q), 0);
				}
				InitCommunication(TALKING_PET_CONVERSATION);
			}
		}

		if (!(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
		{
			bool UmgahSurvivors {};

			UmgahSurvivors = GetHeadLink(
								 &GLOBAL(npc_built_ship_q))
						  != 0;
			uqm::currentActivity() &= ~START_INTERPLANETARY;

			if (GET_GAME_STATE(PLAYER_HYPNOTIZED))
			{
				ZapToUrquanEncounter();
			}
			else if (GET_GAME_STATE(UMGAH_ZOMBIE_BLOBBIES)
					 && !UmgahSurvivors)
			{
				// Defeated the zombie fleet.
				InitCommunication(TALKING_PET_CONVERSATION);
			}
			else if (!(StartSphereTracking(UMGAH_SHIP)))
			{
				// The Kohr-Ah have destroyed the Umgah, but the
				// talking pet survived.
				InitCommunication(TALKING_PET_CONVERSATION);
			}

			ReinitQueue(&GLOBAL(npc_built_ship_q));
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

static uint16_t
GenerateTalkingPet_generateEnergy(const SOLARSYS_STATE* solarSys,
								  const PLANET_DESC* world, uint16_t whichNode, NODE_INFO* info)
{
	if (matchWorld(solarSys, world, MatchPbyte, MatchPlanet))
	{
		return GenerateDefault_generateRuins(solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateTalkingPet_pickupEnergy(SOLARSYS_STATE* solarSys,
								PLANET_DESC* world, uint16_t whichNode)
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

static void
ZapToUrquanEncounter(void)
{
	HENCOUNTER hEncounter {};

	if ((hEncounter = AllocEncounter())
		|| (hEncounter = GetHeadEncounter()))
	{
		int16_t dx {};
		int16_t dy {};
		ENCOUNTER* EncounterPtr {};
		HFLEETINFO hStarShip {};

		LockEncounter(hEncounter, &EncounterPtr);

		if (hEncounter == GetHeadEncounter())
		{
			RemoveEncounter(hEncounter);
		}
		memset(EncounterPtr, 0, sizeof(*EncounterPtr));

		InsertEncounter(hEncounter, GetHeadEncounter());

		hStarShip = GetStarShipFromIndex(
			&GLOBAL(avail_race_q), URQUAN_SHIP);
		const FLEET_INFO* TemplatePtr {LockFleetInfo(&GLOBAL(avail_race_q), hStarShip)};
		EncounterPtr->origin = TemplatePtr->loc;
		EncounterPtr->radius = TemplatePtr->actual_strength;
		EncounterPtr->race_id = URQUAN_SHIP;
		EncounterPtr->num_ships = 1;
		EncounterPtr->flags = OneShotEncounter;
		BRIEF_SHIP_INFO* BSIPtr {&EncounterPtr->ShipList[0]};
		BSIPtr->race_id = URQUAN_SHIP;
		BSIPtr->crew_level = TemplatePtr->crew_level;
		BSIPtr->max_crew = TemplatePtr->max_crew;
		BSIPtr->max_energy = TemplatePtr->max_energy;

		HFLEETINFO hUmgah = GetStarShipFromIndex(&GLOBAL(avail_race_q), UMGAH_SHIP);
		FLEET_INFO* UmgahPtr =
			LockFleetInfo(&GLOBAL(avail_race_q), hUmgah);

		EncounterPtr->loc_pt = SeedFleetLocation(UmgahPtr, plot_map, SAMATRA_DEFINED);
		UnlockFleetInfo(&GLOBAL(avail_race_q), hUmgah);

		EncounterPtr->log_x = UniverseToLogx(EncounterPtr->loc_pt.x);
		EncounterPtr->log_y = UniverseToLogy(EncounterPtr->loc_pt.y);
		GLOBAL_SIS(log_x) = EncounterPtr->log_x;
		GLOBAL_SIS(log_y) = EncounterPtr->log_y;
		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);

		{
			SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond * 2));
			MoveGameClockDays(LostDays);
		}

		uqm::currentActivity() =
			uqm::makeWord(IN_HYPERSPACE, 0) | START_ENCOUNTER;
		SetBattlePlanet();

		dx = CurStarDescPtr()->star_pt.x - EncounterPtr->loc_pt.x;
		dy = CurStarDescPtr()->star_pt.y - EncounterPtr->loc_pt.y;
		dx = (int16_t)square_root((long)dx * dx + (long)dy * dy)
		   + (FuelTankScale >> 1);

		if (!uqm::UQMOptions::read().infiniteFuel)
		{
			DeltaSISGauges(0, -dx, 0);
		}

		if (GLOBAL_SIS(FuelOnBoard) < 5 * FuelTankScale)
		{
			dx = ((5 + ((uint16_t)uqm::random().next() % 5)) * FuelTankScale)
			   - (int16_t)GLOBAL_SIS(FuelOnBoard);
			DeltaSISGauges(0, dx, 0);
		}
		DrawSISMessage(nullptr);
		// DrawHyperCoords (EncounterPtr->loc_pt);
		uqm::flagship().shipStamp.origin = EncounterPtr->loc_pt;
		uqm::flagship().shipFacing = 6;


		UnlockEncounter(hEncounter);
	}
}