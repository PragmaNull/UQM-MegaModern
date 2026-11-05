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

#include "globdata.h"

#include "coderes.h"
#include "encount.h"
#include "starmap.h"
#include "master.h"
#include "setup.h"
#include "units.h"
#include "hyper.h"
#include "resinst.h"
#include "nameref.h"
#include "build.h"
#include "state.h"
#include "grpinfo.h"
#include "gamestr.h"
#include "core/log/log.h"
#include "options.h"
#include <assert.h>
#include <stdlib.h>
#include "uqmdebug.h"

#include "UQMGame.h"
#include "uqm/random/UQMRandom.h"

static constexpr auto OopsAll {0}; // Set a plot ID to stamp it to all empty starsystems
static void CreateRadar(void);

GFXCONTEXT RadarContext;
FRAME PlayFrame;


static void
CreateRadar(void)
{
	if (RadarContext == 0)
	{
		GFXRECT r;

		RadarContext = CreateContext("RadarContext");
		GFXCONTEXT OldContext {SetContext(RadarContext)};
		SetContextFGFrame(Screen);
		r.corner.x = RadarX();
		r.corner.y = RadarY();
		r.extent.width = RadarWidth();
		r.extent.height = RadarHeight();
		SetContextClipRect(&r);
		SetContext(OldContext);
	}
}

bool LoadSC2Data(void)
{
	if (FlagStatFrame == 0)
	{
		FlagStatFrame = CaptureDrawable(
			LoadGraphic(FlagstatMaskPmapAnimRes));
		if (FlagStatFrame == nullptr)
		{
			return false;
		}

		MiscDataFrame = CaptureDrawable(
			LoadGraphic(MiscdataMaskPmapAnimRes));
		if (MiscDataFrame == nullptr)
		{
			return false;
		}

		visitedStarsFrame = CaptureDrawable(
			LoadGraphic(VisitedStarsAnimRes));
		if (visitedStarsFrame == nullptr)
		{
			return false;
		}

		FontGradFrame = CaptureDrawable(
			LoadGraphic(FontgradPmapAnimRes));
	}

	CreateRadar();

	if (inHQSpace())
	{
		uqm::flagship().shipStamp.origin.x =
			uqm::flagship().shipStamp.origin.y = -1;
	}

	return true;
}

static void
copyFleetInfo(FLEET_INFO* dst, SHIP_INFO* src, FLEET_STUFF* fleet)
{
	// other leading fields are irrelevant
	dst->crew_level = src->crew_level;
	dst->max_crew = src->max_crew;
	dst->max_energy = src->max_energy;

	dst->shipIdStr = src->idStr;
	dst->race_strings = src->race_strings;
	dst->icons = src->icons;
	dst->melee_icon = src->melee_icon;

	dst->actual_strength = fleet->strength;
	dst->known_loc = fleet->known_loc;
}

void LoadFleetInfo(void)
{ /* Yehat Rebels and Ur-Quan probe */
	const uint16_t num_ships = LAST_MELEE_ID - ARILOU_ID + 1 + 2;
	uqm::gameState().game().avail_race_q.reset(num_ships, sizeof(FLEET_INFO));
	SPECIES_ID ship_ref = (SPECIES_ID)(ARILOU_ID - 1);
	for (int i = 0; i < num_ships; ++i)
	{
		HFLEETINFO hFleet {};
		FLEET_INFO* FleetPtr {};

		if (i < num_ships - 2)
		{
			ship_ref = (SPECIES_ID)((int)(ship_ref) + 1);
		}
		else if (i == num_ships - 2)
		{
			ship_ref = YEHAT_ID;
		}
		else /* (i == num_ships - 1) */
		{
			ship_ref = UR_QUAN_PROBE_ID;
		}

		hFleet = AllocLink(&uqm::gameState().game().avail_race_q);
		if (!hFleet)
		{
			continue;
		}
		FleetPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
		FleetPtr->SpeciesID = ship_ref;

		if (i < num_ships - 1)
		{
			HMASTERSHIP hMasterShip {FindMasterShip(ship_ref)};
			MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hMasterShip)};
			// Grab a copy of loaded icons and strings (not owned)
			copyFleetInfo(FleetPtr, &MasterPtr->ShipInfo,
						  &MasterPtr->Fleet);
			UnlockMasterShip(&master_q, hMasterShip);
			// If the game is seeded, move the fleet to starting position
			SeedFleet(FleetPtr, plot_map);
			// Rebel special case
			if (i == YEHAT_REBEL_SHIP)
			{
				FleetPtr->actual_strength = 0;
			}
#ifdef DEBUG_SPHERE_COLOR
			switch (i)
			{
				case HUMAN_SHIP:
					FleetPtr->actual_strength = 200;
					FleetPtr->known_loc = plot_map[SOL_DEFINED].star_pt;
					break;
				case SHOFIXTI_SHIP:
					FleetPtr->actual_strength = 150;
					FleetPtr->known_loc = plot_map[SHOFIXTI_DEFINED].star_pt;
					break;
				case MELNORME_SHIP:
					FleetPtr->actual_strength = 300;
					FleetPtr->known_loc = plot_map[MELNORME6_DEFINED].star_pt;
					break;
				case ANDROSYNTH_SHIP:
					FleetPtr->actual_strength = 100;
					FleetPtr->known_loc = plot_map[START_COLONY_DEFINED].star_pt;
					break;
				case CHENJESU_SHIP:
					FleetPtr->actual_strength = 100;
					FleetPtr->known_loc = plot_map[ILWRATH_DEFINED].star_pt;
					break;
				case MMRNMHRM_SHIP:
					FleetPtr->actual_strength = 100;
					FleetPtr->known_loc = plot_map[MOTHER_ARK_DEFINED].star_pt;
					break;
				case SLYLANDRO_SHIP:
					FleetPtr->actual_strength = 300;
					FleetPtr->known_loc = plot_map[SLYLANDRO_DEFINED].star_pt;
					break;
				case YEHAT_REBEL_SHIP:
					FleetPtr->actual_strength = 200;
					FleetPtr->known_loc = plot_map[RAINBOW0_DEFINED].star_pt;
					break;
				default:
					break;
			}
#endif
		}
		else
		{
			// Ur-Quan probe.
			RACE_DESC* RDPtr = load_ship(FleetPtr->SpeciesID,
										 false);
			if (RDPtr)
			{ // Grab a copy of loaded icons and strings
				copyFleetInfo(FleetPtr, &RDPtr->ship_info,
							  &RDPtr->fleet);
				// avail_race_q owns these resources now
				free_ship(RDPtr, false, false);
			}
		}

		FleetPtr->allied_state = BAD_GUY;
		FleetPtr->known_strength = 0;
		FleetPtr->loc = FleetPtr->known_loc;
		FleetPtr->growth = 0;
		FleetPtr->growth_fract = 0;
		FleetPtr->growth_err_term = 255 >> 1;
		FleetPtr->days_left = 0;
		FleetPtr->func_index = ~0;
		FleetPtr->can_build = false;
		if (uqm::UQMOptions::read().unlockShips && i < LAST_MELEE_ID)
		{
			FleetPtr->can_build = true;
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
		PutQueue(&uqm::gameState().game().avail_race_q, hFleet);
	}
}

bool InitGameStructures(void)
{
	uint16_t i {};

	uqm::gameState().sis().Difficulty = uqm::UQMOptions::read().optDifficulty;
	uqm::gameState().sis().Extended = uqm::UQMOptions::read().extended;
	uqm::gameState().sis().Nomad = uqm::UQMOptions::read().nomad;
	// Set the seed, then check/start StarSeed
	const uqstl::optional<uqm::GameSeed> chosenSeed {uqm::UQMGame::getInstance().takeNewGameSeed()};
	uqm::gameState().setSeed(chosenSeed ? uqm::normalized(*chosenSeed) : uqm::newGameSeed(uqm::UQMOptions::read()));
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "Starting a NEW game with seed type {}, {}\n",
			   optSeedType,
			   (optSeedType == 0) ? "Default Game Mode (no seeding)" :
			   (optSeedType == 1) ? "Seed Planets (SysGenRNG only)" :
			   (optSeedType == 2) ? "MRQ (Melnorme, Rainbow, and Quasispace)" :
			   (optSeedType == 3) ? "Seed Plot (Starseed)" :
									"UNKNOWN");
#endif
	// During NEW game we want to time more aggressively and reseed
	// if it takes too long to create a map with a seed.
	// The new seed will be saved to the game seed.
	// If non-starseed, this will give us a default plot map,
	// so we still need this.
	if (!InitStarseed(true))
	{
		return (false);
	}

	PlayFrame = CaptureDrawable(LoadGraphic(PlaymenuAnimRes));

	// Load ships based on current seed settings
	ReloadMasterShipList(nullptr);
	LoadFleetInfo();

	InitSISContexts();
	LoadSC2Data();

	InitPlanetInfo();
	InitGroupInfo(true);

	uqm::globalFlags() = NumReadSpeeds >> 1;

	uqm::gameState().game().ElementWorth[COMMON] = 1;
	uqm::gameState().sis().ElementAmounts[COMMON] = 0;
	uqm::gameState().game().ElementWorth[CORROSIVE] = 2;
	uqm::gameState().sis().ElementAmounts[CORROSIVE] = 0;
	uqm::gameState().game().ElementWorth[BASE_METAL] = 3;
	uqm::gameState().sis().ElementAmounts[BASE_METAL] = 0;
	uqm::gameState().game().ElementWorth[NOBLE] = 4;
	uqm::gameState().sis().ElementAmounts[NOBLE] = 0;
	uqm::gameState().game().ElementWorth[RARE_EARTH] = 5;
	uqm::gameState().sis().ElementAmounts[RARE_EARTH] = 0;
	uqm::gameState().game().ElementWorth[PRECIOUS] = 6;
	uqm::gameState().sis().ElementAmounts[PRECIOUS] = 0;
	uqm::gameState().game().ElementWorth[RADIOACTIVE] = 8;
	uqm::gameState().sis().ElementAmounts[RADIOACTIVE] = 0;
	uqm::gameState().game().ElementWorth[EXOTIC] = 25;
	uqm::gameState().sis().ElementAmounts[EXOTIC] = 0;

	switch (getDifficulty())
	{
		case uqm::Difficulty::Easy:
			for (int i = 0; i < NUM_ELEMENT_CATEGORIES; i++)
			{
				uqm::gameState().game().ElementWorth[i] *= 2;
			}
			break;
		case uqm::Difficulty::Hard:
			uqm::gameState().game().ElementWorth[EXOTIC] = 16;
			uqm::gameState().setFlag(uqm::GameFlag::CREW_PURCHASED0, static_cast<uint32_t>(uqm::lowByte<uint8_t>(100)));
			uqm::gameState().setFlag(uqm::GameFlag::CREW_PURCHASED1, static_cast<uint32_t>(uqm::highByte<uint8_t>(100)));
			break;
		case uqm::Difficulty::Normal:
		case uqm::Difficulty::ChooseYourOwn:
			break;
	}

	for (i = 0; i < NumDriveSlots; ++i)
	{
		uqm::gameState().sis().DriveSlots[i] = EmptySlot + 0;
	}
	uqm::gameState().sis().DriveSlots[5] =
		uqm::gameState().sis().DriveSlots[6] = FUSION_THRUSTER;
	for (i = 0; i < NumJetSlots; ++i)
	{
		uqm::gameState().sis().JetSlots[i] = EmptySlot + 1;
	}
	uqm::gameState().sis().JetSlots[0] =
		uqm::gameState().sis().JetSlots[6] = TURNING_JETS;
	for (i = 0; i < NumModuleSlots; ++i)
	{
		uqm::gameState().sis().ModuleSlots[i] = EmptySlot + 2;
	}
	uqm::gameState().sis().ModuleSlots[15] = GUN_WEAPON;
	uqm::gameState().sis().ModuleSlots[2] = CREW_POD;
	// Make crew 31 at start to align with the amount of crew lost on the
	// Tobermoon during the journey from Vela to Sol, Hard and/or Extended
	// mode only
	uqm::gameState().sis().CrewEnlisted =
		(isDifficulty(uqm::Difficulty::Hard) || IsExtended()) ? 31 : CrewPodCapacity;
	uqm::gameState().sis().ModuleSlots[8] = STORAGE_BAY;
	uqm::gameState().sis().ModuleSlots[1] = FUEL_TANK;
	uqm::gameState().sis().FuelOnBoard = ifEasyDifficulty(4338, 10 * FuelTankScale);

	if (isDifficulty(uqm::Difficulty::Easy))
	{
		uqm::gameState().sis().DriveSlots[7] =
			uqm::gameState().sis().DriveSlots[8] = FUSION_THRUSTER;
		uqm::gameState().sis().JetSlots[1] =
			uqm::gameState().sis().JetSlots[7] = TURNING_JETS;
	}

	if (getNomadMode() == uqm::NomadMode::Easy)
	{
		uqm::gameState().sis().DriveSlots[3] =
			uqm::gameState().sis().DriveSlots[4] = FUSION_THRUSTER;
		uqm::gameState().sis().JetSlots[2] =
			uqm::gameState().sis().JetSlots[5] = TURNING_JETS;
		uqm::gameState().sis().FuelOnBoard += 20 * FuelTankScale;
	}

	if (uqm::UQMOptions::read().headStart)
	{
		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(2));
		SetHomeworldKnown(SPATHI_HOME);
		if (getNomadMode() == uqm::NomadMode::Off)
		{
			uqm::gameState().setFlag(uqm::GameFlag::MOONBASE_ON_SHIP, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::MOONBASE_DESTROYED, static_cast<uint32_t>(1));
			uqm::gameState().sis().ElementAmounts[RADIOACTIVE] = 1;
			uqm::gameState().sis().TotalElementMass = 1;
		}
	}

	loadGameCheats();

	uqm::gameState().game().stowed_ship_q.reset(MaxStowedShips, sizeof(SHIP_FRAGMENT));
	uqm::gameState().game().built_ship_q.reset(MaxBuiltShips, sizeof(SHIP_FRAGMENT));
	uqm::gameState().game().npc_built_ship_q.reset(MaxShipsPerSide, sizeof(SHIP_FRAGMENT));
	uqm::gameState().game().ip_group_q.reset(MaxBattleGroups, sizeof(IP_GROUP));
	uqm::gameState().game().encounter_q.reset(MaxEncounters, sizeof(ENCOUNTER));

	uqm::currentActivity() = IN_INTERPLANETARY | START_INTERPLANETARY;

	uqm::gameState().sis().ResUnits = ifEasyDifficulty(2500, 0);
	uqm::gameState().game().CrewCost = 3;
	uqm::gameState().game().FuelCost = FuelCostRu; // JMS: Was "20"
	uqm::gameState().game().ModuleCost[PLANET_LANDER] = 500 / ModuleCostScale;
	uqm::gameState().game().ModuleCost[FUSION_THRUSTER] = 500 / ModuleCostScale;
	uqm::gameState().game().ModuleCost[TURNING_JETS] = 500 / ModuleCostScale;
	uqm::gameState().game().ModuleCost[CREW_POD] = 2000 / ModuleCostScale;
	uqm::gameState().game().ModuleCost[STORAGE_BAY] = 750 / ModuleCostScale;
	uqm::gameState().game().ModuleCost[FUEL_TANK] = 500 / ModuleCostScale;
	uqm::gameState().game().ModuleCost[DYNAMO_UNIT] = 2000 / ModuleCostScale;
	uqm::gameState().game().ModuleCost[GUN_WEAPON] = 2000 / ModuleCostScale;

	uqm::gameState().sis().NumLanders = ifEasyDifficulty(2, 1);

	uqm::utf8StringCopy(uqm::gameState().sis().ShipName,
						sizeof(uqm::gameState().sis().ShipName),
						GameString(NAMING_STRING_BASE + 6)); // UNNAMED
	uqm::utf8StringCopy(uqm::gameState().sis().CommanderName,
						sizeof(uqm::gameState().sis().CommanderName),
						GameString(NAMING_STRING_BASE + 6)); // UNNAMED

	SetRaceAllied(HUMAN_SHIP, true);
	CloneShipFragment(HUMAN_SHIP, &uqm::gameState().game().built_ship_q, 0);

	if (uqm::UQMOptions::read().headStart)
	{
		const uint8_t SpaCrew = ifEasyDifficulty(MaxCrewSize, 1);
		AddEscortShips(SPATHI_SHIP, 1);
		// Make the Eluder escort captained by Fwiffo alone or have a full
		// compliment for Easy mode.
		SetEscortCrewComplement(SPATHI_SHIP,
								SpaCrew, NameOffset + NumCaptainsNames);
		StartSphereTracking(SPATHI_SHIP);
	}

	uqm::gameState().sis().log_x = UniverseToLogx(plot_map[SOL_DEFINED].star_pt.x);
	uqm::gameState().sis().log_y = UniverseToLogy(plot_map[SOL_DEFINED].star_pt.y);
	CurStarDescPtr() = 0;
	uqm::flagship().autopilot.clear();

	ZeroLastLoc();
	ZeroAdvancedAutoPilot();
	ZeroAdvancedQuasiPilot();

	return (true);
}

void FreeSC2Data(void)
{
	DestroyContext(RadarContext);
	RadarContext = 0;
	DestroyDrawable(ReleaseDrawable(FontGradFrame));
	FontGradFrame = 0;
	DestroyDrawable(ReleaseDrawable(MiscDataFrame));
	MiscDataFrame = 0;
	DestroyDrawable(ReleaseDrawable(visitedStarsFrame));
	visitedStarsFrame = 0;
	DestroyDrawable(ReleaseDrawable(FlagStatFrame));
	FlagStatFrame = 0;
}

void UninitGameStructures(void)
{
	uqm::gameState().game().encounter_q.release();
	uqm::gameState().game().ip_group_q.release();
	uqm::gameState().game().npc_built_ship_q.release();
	uqm::gameState().game().built_ship_q.release();
	uqm::gameState().game().stowed_ship_q.release();
	UninitGroupInfo();
	UninitPlanetInfo();

	//    FreeSC2Data ();

	// The only resources avail_race_q owns are the Ur-Quan probe's
	// so free them now
	HFLEETINFO hStarShip {GetTailLink(&uqm::gameState().game().avail_race_q)};
	if (hStarShip)
	{
		const FLEET_INFO* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
		DestroyDrawable(ReleaseDrawable(FleetPtr->melee_icon));
		DestroyDrawable(ReleaseDrawable(FleetPtr->icons));
		delete FleetPtr->race_strings;
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}

	uqm::gameState().game().avail_race_q.release();

	DestroyDrawable(ReleaseDrawable(PlayFrame));
	PlayFrame = 0;
}


static constexpr auto SampleSize {1000};

// For debugging purposes, generate a bunch of seeds and then
// calculate how many fall into each time category.
void SeedDEBUG()
{
	constexpr uint32_t SampleSeedStart {123000};
	const uqm::GameSeed save {uqm::currentSeed()};
	uqm::GameSeed sample {save};
	//uint16_t histogram[100] = {[0 ... 99] = 0};
	uint16_t histogram[100] = {0};
	uint16_t decisec {};
	clock_t start_clock {};
	bool myRNG = false;
	for (decisec = 0; decisec < 100; decisec++)
	{
		histogram[decisec] = 0;
	}

	if (!StarGenRNG)
	{
		fmt::print(stderr, "****Seed Debug Creating a STAR GEN RNG****\n");
		StarGenRNG.emplace();
		myRNG = true;
	}
	StarGenRNG->reseed(123456);
	for (sample.value = SampleSeedStart; sample.value < (SampleSeedStart + SampleSize); ++sample.value)
	{
		uqm::gameState().setSeed(sample);
		start_clock = clock();
		fmt::print(stderr, "\n\n\nStarting seed {}... ", uqm::currentSeed().value);
		InitPlot(plot_map);
		fmt::print(stderr, "seeding stars {}... ", uqm::currentSeed().value);
		SeedStarmap(star_array);
		fmt::print(stderr, "seeding plots {}... ", uqm::currentSeed().value);
		if (SeedPlot(plot_map, star_array) < NUM_PLOTS)
		{
			fmt::print(stderr, "Failed to seed {}. ", uqm::currentSeed().value);
			decisec = 98;
		}
		else
		{
			decisec = (double)(clock() - start_clock) / 100000;
		}
		fmt::print(stderr, "Complete %6.6f seconds.\n",
				   (double)(clock() - start_clock) / 1000000);
		if (decisec > 99)
		{
			decisec = 99;
		}
		histogram[decisec]++;
	}
	for (decisec = 0; decisec < 100; decisec++)
	{
		if (decisec % 10 == 0)
		{
			fmt::print(stderr, "\n");
		}
		fmt::print(stderr, "%3d ", histogram[decisec]);
	}
	uqm::gameState().setSeed(save);
	if (StarGenRNG && myRNG)
	{
		StarGenRNG.reset();
	}
}

// Initialize the plot map, star array, and quasi portal map.
// This is called during either new or load, whether or not the new game is
// a seeded game as we will need to reset global variables regardless.
//
// Assumes the space for global arrays already allocated for:
// star_array - a copy of the starmap (starmap_array)
// plot_map - a plot array
// portal_map - a quasispace portal array
bool InitStarseed(bool newgame)
{
	uint16_t i {};
#ifdef DEBUG_STARSEED_TRACE_V
	SeedDEBUG();
	fmt::print(stderr, "CurrentActivity {}\n", uqm::currentActivity());
#endif
	DefaultStarmap(star_array);
	if (!StarGenRNG)
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Init Starseed creating a STAR GEN RNG.\n");
#endif
		StarGenRNG.emplace();
		StarGenRNG->reseed(uqm::currentSeed().value);
	}
	if (!uqm::isStarSeed())
	{
		// Here we will split off, provide default plotmap, return
		// This makes it easier to integrate seemlessly old vs new
		DefaultPlot(plot_map, star_array);
		DefaultQuasispace(portal_map);
		// Done with StarGenRNG for now; will create later if moving fleets
		StarGenRNG.reset();
		return true;
	}
	if (uqm::currentSeed().type == uqm::SeedType::MRQ)
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Setting MQR shuffle.\n");
#endif
		DefaultPlot(plot_map, star_array);
		InitMelnormeRainbow(plot_map);
	}
	else // optSeedType == OPTVAL_STAR
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Setting full plot shuffle.\n");
#endif
		InitPlot(plot_map);
	}
	fmt::print(stderr, "Starting map generation using seed {}.\n",
			   uqm::currentSeed().value);
	SeedStarmap(star_array);
	if (uqm::currentActivity() || !newgame)
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "*+*+* LOADING *+*+*\n");
#endif
		if (SeedPlot(plot_map, star_array) != NUM_PLOTS)
		{
			fmt::print(stderr, "Seed Plot Failed.\n");
			StarGenRNG.reset();
			return false;
		}
		if (!SeedQuasispace(portal_map, plot_map, star_array))
		{
			fmt::print(stderr, "Seed Quasisapce Failed.\n");
			StarGenRNG.reset();
			return false;
		}
	}
	else
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "*+*+* NEW GAME *+*+*\n");
#endif
		i = 0;
		// if it fails to seed the plot we need to roll the starmap too
		// otherwise load game will not be correct, it will have other
		// seed's stars and this seed's plot.  Boo.
		while (SeedPlot(plot_map, star_array) != NUM_PLOTS && i < 100)
		{
			fmt::print(stderr, "Seed {} failed ({}).\n", uqm::currentSeed().value, ++i);
			uqm::GameSeed nextSeed {uqm::currentSeed()};
			++nextSeed.value;
			uqm::gameState().setSeed(nextSeed);
			SeedStarmap(star_array);
		}
		if (i >= 100)
		{
			fmt::print(stderr, "Seed Plot Failed.\n");
			StarGenRNG.reset();
			return false;
		}
		if (!SeedQuasispace(portal_map, plot_map, star_array))
		{
			fmt::print(stderr, "Seed Quasisapce Failed.\n");
			StarGenRNG.reset();
			return false;
		}
	}
	if (OopsAll > 0)
	{
		for (i = 0; i < NumSolarSystems; i++)
		{
			if (!star_array[i].Index)
			{
				star_array[i].Index = OopsAll;
			}
		}
	}
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "Done seeding {}.\n", uqm::currentSeed().value);
#endif
	// Done with StarGenRNG for now; will create later if moving fleets
	StarGenRNG.reset();
	return (true);
}

bool inFullGame(void)
{
	const ACTIVITY act = uqm::lowByte(uqm::currentActivity());
	return (act == IN_LAST_BATTLE || act == IN_ENCOUNTER || act == IN_HYPERSPACE || act == IN_INTERPLANETARY || act == WON_LAST_BATTLE);
}

bool inSuperMelee(void)
{
	return (uqm::lowByte(uqm::currentActivity()) == SUPER_MELEE);
	// TODO: && !inMainMenu ()
}

#if 0
bool
inBattle (void)
{
	// TODO: IN_BATTLE is also set while in HyperSpace/QuasiSpace.
	return ((uqm::currentActivity() & IN_BATTLE) != 0);
}
#endif

#if 0
// Disabled for now as there are similar functions in uqm/planets/planets.h
// Pre: inFullGame()
bool
inInterPlanetary (void)
{
	assert (inFullGame ());
	return (pSolarSysState != nullptr);
}

// Pre: inFullGame()
bool
inSolarSystem (void)
{
	assert (inFullGame ());
	return (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY);
}

// Pre: inFullGame()
bool
inOrbit (void)
{
	assert (inFullGame ());
	return (pSolarSysState != nullptr) &&
			(pSolarSysState->pOrbitalDesc != nullptr);
}
#endif

// In HyperSpace or QuasiSpace
// Pre: inFullGame()
bool inHQSpace(void)
{
	//assert (inFullGame ());
	return (uqm::lowByte(uqm::currentActivity()) == IN_HYPERSPACE);
	// IN_HYPERSPACE is also set for QuasiSpace
}

// In HyperSpace
// Pre: inFullGame()
bool inHyperSpace(void)
{
	//assert (inFullGame ());
	return (uqm::lowByte(uqm::currentActivity()) == IN_HYPERSPACE) && (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1);
	// IN_HYPERSPACE is also set for QuasiSpace
}

// In QuasiSpace
// Pre: inFullGame()
bool inQuasiSpace(void)
{
	//assert (inFullGame ());
	return (uqm::lowByte(uqm::currentActivity()) == IN_HYPERSPACE) && (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) > 1);
	// IN_HYPERSPACE is also set for QuasiSpace
}

//
//OPT_CONSOLETYPE
//is3DO(int optWhich)
//{
//	return optWhich == uqm::EmulationMode::Console3DO ? OPTVAL_3DO : OPTVAL_PC;
//}

//EnumerationMode
//isPC(int optWhich)
//{
//	return optWhich == uqm::EmulationMode::PC ? OPTVAL_PC : OPTVAL_3DO;
//}
//
//OPT_CONSOLETYPE
//is3DO(int optWhich)
//{
//	return optWhich == uqm::EmulationMode::Console3DO ? OPTVAL_3DO : OPTVAL_PC;
//}

// Does not work with UTF encoding!
// Basic function for replacing all instances of character "find"
// with character "replace"
// Returns the number of replaced characters on success
// -1 if any input is nullptr, and 0 if 'find' isn't found
int replaceChar(char* pStr, const char find, const char replace)
{
	int count = 0;
	const size_t len = strlen(pStr);

	if (!pStr || !find || !replace)
	{
		return -1; // pStr, find, or replace is nullptr
	}

	if (uqm::utf8StringPos(pStr, find) == -1)
	{
		return 0; // 'find' not found
	}

	for (size_t i {0}; i < len; i++)
	{
		if (pStr[i] == find)
		{
			pStr[i] = replace;
			if (pStr[i] == replace)
			{
				count++;
			}
		}
	}

	return count;
}
