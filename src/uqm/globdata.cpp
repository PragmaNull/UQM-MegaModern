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
#include "libs/luauqm/luauqm.h"
#include "core/log/log.h"
#include "options.h"
#include <assert.h>
#include <stdlib.h>
#include "uqmdebug.h"

#include <time.h> //required to use 'srand(time(nullptr))'

#define OOPS_ALL 0 // Set a plot ID to stamp it to all empty starsystems
static void CreateRadar(void);

GFXCONTEXT RadarContext;
FRAME PlayFrame;

GLOBDATA GlobData;


static void
CreateRadar(void)
{
	if (RadarContext == 0)
	{
		GFXRECT r;
		GFXCONTEXT OldContext;

		RadarContext = CreateContext("RadarContext");
		OldContext = SetContext(RadarContext);
		SetContextFGFrame(Screen);
		r.corner.x = RADAR_X;
		r.corner.y = RADAR_Y;
		r.extent.width = RADAR_WIDTH;
		r.extent.height = RADAR_HEIGHT;
		SetContextClipRect(&r);
		SetContext(OldContext);
	}
}

bool LoadSC2Data(void)
{
	if (FlagStatFrame == 0)
	{
		FlagStatFrame = CaptureDrawable(
			LoadGraphic(FLAGSTAT_MASK_PMAP_ANIM));
		if (FlagStatFrame == nullptr)
		{
			return false;
		}

		MiscDataFrame = CaptureDrawable(
			LoadGraphic(MISCDATA_MASK_PMAP_ANIM));
		if (MiscDataFrame == nullptr)
		{
			return false;
		}

		visitedStarsFrame = CaptureDrawable(
			LoadGraphic(VISITED_STARS_ANIM));
		if (visitedStarsFrame == nullptr)
		{
			return false;
		}

		FontGradFrame = CaptureDrawable(
			LoadGraphic(FONTGRAD_PMAP_ANIM));
	}

	CreateRadar();

	if (inHQSpace())
	{
		GLOBAL(ShipStamp.origin.x) =
			GLOBAL(ShipStamp.origin.y) = -1;
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
	uint16_t num_ships = LAST_MELEE_ID - ARILOU_ID + 1 + 2;
	InitQueue(&GLOBAL(avail_race_q), num_ships, sizeof(FLEET_INFO));
	SPECIES_ID ship_ref = (SPECIES_ID)(ARILOU_ID - 1);
	for (int i = 0; i < num_ships; ++i)
	{
		HFLEETINFO hFleet;
		FLEET_INFO* FleetPtr;

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

		hFleet = AllocLink(&GLOBAL(avail_race_q));
		if (!hFleet)
		{
			continue;
		}
		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hFleet);
		FleetPtr->SpeciesID = ship_ref;

		if (i < num_ships - 1)
		{
			HMASTERSHIP hMasterShip;
			MASTER_SHIP_INFO* MasterPtr;

			hMasterShip = FindMasterShip(ship_ref);
			MasterPtr = LockMasterShip(&master_q, hMasterShip);
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

		UnlockFleetInfo(&GLOBAL(avail_race_q), hFleet);
		PutQueue(&GLOBAL(avail_race_q), hFleet);
	}
}

bool InitGameStructures(void)
{
	uint16_t i;

	InitGlobData();
	// Set Seed Type, then check/start StarSeed
	SET_GAME_STATE(SEED_TYPE, g_seedType);
	GLOBAL_SIS(Difficulty) = uqm::UQMOptions::read().optDifficulty;
	GLOBAL_SIS(Extended) = uqm::UQMOptions::read().extended;
	GLOBAL_SIS(Nomad) = uqm::UQMOptions::read().nomad;
	if (PrimeSeed)
	{
		uqm::UQMOptions::edit().shipSeed = false;
		uqm::UQMOptions::edit().customSeed.value = PrimeA;
	}
	if (isDifficulty(uqm::Difficulty::Hard) && !PrimeSeed && !StarSeed)
	{
		srand(time(nullptr));
		uqm::UQMOptions::edit().customSeed.value = (rand() % ((MAX_SEED - MIN_SEED) + MIN_SEED));
	}
	GLOBAL_SIS(Seed) = uqm::UQMOptions::read().customSeed.value;
	GLOBAL_SIS(ShipSeed) = (uqm::UQMOptions::read().shipSeed ? 1 : 0);
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
	// The new seed will be saved to SIS and uqm::UQMOptions::read().customSeed
	// If non-starseed, this will give us a default plot map,
	// so we still need this.
	if (!InitStarseed(true))
	{
		return (false);
	}
	GLOBAL_SIS(Seed) = uqm::UQMOptions::read().customSeed; // In case Starseed rolls the seed

	PlayFrame = CaptureDrawable(LoadGraphic(PLAYMENU_ANIM));

	// Load ships based on current seed settings
	ReloadMasterShipList(nullptr);
	LoadFleetInfo();

	InitSISContexts();
	LoadSC2Data();

	InitPlanetInfo();
	InitGroupInfo(true);

	GLOBAL(glob_flags) = NUM_READ_SPEEDS >> 1;

	GLOBAL(ElementWorth[COMMON]) = 1;
	GLOBAL_SIS(ElementAmounts[COMMON]) = 0;
	GLOBAL(ElementWorth[CORROSIVE]) = 2;
	GLOBAL_SIS(ElementAmounts[CORROSIVE]) = 0;
	GLOBAL(ElementWorth[BASE_METAL]) = 3;
	GLOBAL_SIS(ElementAmounts[BASE_METAL]) = 0;
	GLOBAL(ElementWorth[NOBLE]) = 4;
	GLOBAL_SIS(ElementAmounts[NOBLE]) = 0;
	GLOBAL(ElementWorth[RARE_EARTH]) = 5;
	GLOBAL_SIS(ElementAmounts[RARE_EARTH]) = 0;
	GLOBAL(ElementWorth[PRECIOUS]) = 6;
	GLOBAL_SIS(ElementAmounts[PRECIOUS]) = 0;
	GLOBAL(ElementWorth[RADIOACTIVE]) = 8;
	GLOBAL_SIS(ElementAmounts[RADIOACTIVE]) = 0;
	GLOBAL(ElementWorth[EXOTIC]) = 25;
	GLOBAL_SIS(ElementAmounts[EXOTIC]) = 0;

	switch (getDifficulty())
	{
		case uqm::Difficulty::Easy:
			for (int i = 0; i < NUM_ELEMENT_CATEGORIES; i++)
			{
				GLOBAL(ElementWorth[i]) *= 2;
			}
			break;
		case uqm::Difficulty::Hard:
			GLOBAL(ElementWorth[EXOTIC]) = 16;
			SET_GAME_STATE(CREW_PURCHASED0, lowByte<uint8_t>(100));
			SET_GAME_STATE(CREW_PURCHASED1, highByte<uint8_t>(100));
			break;
		case uqm::Difficulty::Normal:
		case uqm::Difficulty::ChooseYourOwn:
			break;
	}

	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
	{
		GLOBAL_SIS(DriveSlots[i]) = EMPTY_SLOT + 0;
	}
	GLOBAL_SIS(DriveSlots[5]) =
		GLOBAL_SIS(DriveSlots[6]) = FUSION_THRUSTER;
	for (i = 0; i < NUM_JET_SLOTS; ++i)
	{
		GLOBAL_SIS(JetSlots[i]) = EMPTY_SLOT + 1;
	}
	GLOBAL_SIS(JetSlots[0]) =
		GLOBAL_SIS(JetSlots[6]) = TURNING_JETS;
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
	{
		GLOBAL_SIS(ModuleSlots[i]) = EMPTY_SLOT + 2;
	}
	GLOBAL_SIS(ModuleSlots[15]) = GUN_WEAPON;
	GLOBAL_SIS(ModuleSlots[2]) = CREW_POD;
	// Make crew 31 at start to align with the amount of crew lost on the
	// Tobermoon during the journey from Vela to Sol, Hard and/or Extended
	// mode only
	GLOBAL_SIS(CrewEnlisted) =
		(isDifficulty(uqm::Difficulty::Hard) || EXTENDED) ? 31 : CREW_POD_CAPACITY;
	GLOBAL_SIS(ModuleSlots[8]) = STORAGE_BAY;
	GLOBAL_SIS(ModuleSlots[1]) = FUEL_TANK;
	GLOBAL_SIS(FuelOnBoard) = ifEasyDifficulty(4338, 10 * FUEL_TANK_SCALE);

	if (isDifficulty(uqm::Difficulty::Easy))
	{
		GLOBAL_SIS(DriveSlots[7]) =
			GLOBAL_SIS(DriveSlots[8]) = FUSION_THRUSTER;
		GLOBAL_SIS(JetSlots[1]) =
			GLOBAL_SIS(JetSlots[7]) = TURNING_JETS;
	}

	if (getNomadMode() == uqm::NomadMode::Easy)
	{
		GLOBAL_SIS(DriveSlots[3]) =
			GLOBAL_SIS(DriveSlots[4]) = FUSION_THRUSTER;
		GLOBAL_SIS(JetSlots[2]) =
			GLOBAL_SIS(JetSlots[5]) = TURNING_JETS;
		GLOBAL_SIS(FuelOnBoard) += 20 * FUEL_TANK_SCALE;
	}

	if (uqm::UQMOptions::read().headStart)
	{
		SET_GAME_STATE(FOUND_PLUTO_SPATHI, 2);
		SetHomeworldKnown(SPATHI_HOME);
		if (getNomadMode() == uqm::NomadMode::Off)
		{
			SET_GAME_STATE(MOONBASE_ON_SHIP, 1);
			SET_GAME_STATE(MOONBASE_DESTROYED, 1);
			GLOBAL_SIS(ElementAmounts[RADIOACTIVE]) = 1;
			GLOBAL_SIS(TotalElementMass) = 1;
		}
	}

	loadGameCheats();

	InitQueue(&GLOBAL(stowed_ship_q),
			  MAX_STOWED_SHIPS, sizeof(SHIP_FRAGMENT));
	InitQueue(&GLOBAL(built_ship_q),
			  MAX_BUILT_SHIPS, sizeof(SHIP_FRAGMENT));
	InitQueue(&GLOBAL(npc_built_ship_q), MAX_SHIPS_PER_SIDE,
			  sizeof(SHIP_FRAGMENT));
	InitQueue(&GLOBAL(ip_group_q), MAX_BATTLE_GROUPS,
			  sizeof(IP_GROUP));
	InitQueue(&GLOBAL(encounter_q), MAX_ENCOUNTERS, sizeof(ENCOUNTER));

	GLOBAL(CurrentActivity) = IN_INTERPLANETARY | START_INTERPLANETARY;

	GLOBAL_SIS(ResUnits) = ifEasyDifficulty(2500, 0);
	GLOBAL(CrewCost) = 3;
	GLOBAL(FuelCost) = FUEL_COST_RU; // JMS: Was "20"
	GLOBAL(ModuleCost[PLANET_LANDER]) = 500 / MODULE_COST_SCALE;
	GLOBAL(ModuleCost[FUSION_THRUSTER]) = 500 / MODULE_COST_SCALE;
	GLOBAL(ModuleCost[TURNING_JETS]) = 500 / MODULE_COST_SCALE;
	GLOBAL(ModuleCost[CREW_POD]) = 2000 / MODULE_COST_SCALE;
	GLOBAL(ModuleCost[STORAGE_BAY]) = 750 / MODULE_COST_SCALE;
	GLOBAL(ModuleCost[FUEL_TANK]) = 500 / MODULE_COST_SCALE;
	GLOBAL(ModuleCost[DYNAMO_UNIT]) = 2000 / MODULE_COST_SCALE;
	GLOBAL(ModuleCost[GUN_WEAPON]) = 2000 / MODULE_COST_SCALE;

	GLOBAL_SIS(NumLanders) = ifEasyDifficulty(2, 1);

	uqm::utf8StringCopy(GLOBAL_SIS(ShipName),
						sizeof(GLOBAL_SIS(ShipName)),
						GAME_STRING(NAMING_STRING_BASE + 6)); // UNNAMED
	uqm::utf8StringCopy(GLOBAL_SIS(CommanderName),
						sizeof(GLOBAL_SIS(CommanderName)),
						GAME_STRING(NAMING_STRING_BASE + 6)); // UNNAMED

	SetRaceAllied(HUMAN_SHIP, true);
	CloneShipFragment(HUMAN_SHIP, &GLOBAL(built_ship_q), 0);

	if (uqm::UQMOptions::read().headStart)
	{
		uint8_t SpaCrew = ifEasyDifficulty(MAX_CREW_SIZE, 1);
		AddEscortShips(SPATHI_SHIP, 1);
		// Make the Eluder escort captained by Fwiffo alone or have a full
		// compliment for Easy mode.
		SetEscortCrewComplement(SPATHI_SHIP,
								SpaCrew, NAME_OFFSET + NUM_CAPTAINS_NAMES);
		StartSphereTracking(SPATHI_SHIP);
	}

	GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(plot_map[SOL_DEFINED].star_pt.x);
	GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(plot_map[SOL_DEFINED].star_pt.y);
	CurStarDescPtr = 0;
	GLOBAL(autopilot.x) = ~0;
	GLOBAL(autopilot.y) = ~0;

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
	HFLEETINFO hStarShip;

	UninitQueue(&GLOBAL(encounter_q));
	UninitQueue(&GLOBAL(ip_group_q));
	UninitQueue(&GLOBAL(npc_built_ship_q));
	UninitQueue(&GLOBAL(built_ship_q));
	UninitQueue(&GLOBAL(stowed_ship_q));
	UninitGroupInfo();
	UninitPlanetInfo();

	//    FreeSC2Data ();

	// The only resources avail_race_q owns are the Ur-Quan probe's
	// so free them now
	hStarShip = GetTailLink(&GLOBAL(avail_race_q));
	if (hStarShip)
	{
		FLEET_INFO* FleetPtr;

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		DestroyDrawable(ReleaseDrawable(FleetPtr->melee_icon));
		DestroyDrawable(ReleaseDrawable(FleetPtr->icons));
		DestroyStringTable(ReleaseStringTable(FleetPtr->race_strings));
		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	}

	UninitQueue(&GLOBAL(avail_race_q));

	DestroyDrawable(ReleaseDrawable(PlayFrame));
	PlayFrame = 0;
}

void InitGlobData(void)
{
	uint16_t i;

	i = GLOBAL(glob_flags);
	memset(&GlobData, 0, sizeof(GlobData));
	GLOBAL(glob_flags) = (uint8_t)i;

	GLOBAL(DisplayArray) = DisplayArray;
}


// For debugging purposes, generate a bunch of seeds and then
// calculate how many fall into each time category.
void SeedDEBUG()
{
#define SAMPLE_SIZE 1000
#define START 123000
	int32_t save = uqm::UQMOptions::read().customSeed.value;
	//uint16_t histogram[100] = {[0 ... 99] = 0};
	uint16_t histogram[100] = {0};
	uint16_t decisec;
	clock_t start_clock;
	bool myRNG = false;
	for (decisec = 0; decisec < 100; decisec++)
	{
		histogram[decisec] = 0;
	}

	if (!StarGenRNG)
	{
		fmt::print(stderr, "****Seed Debug Creating a STAR GEN RNG****\n");
		StarGenRNG = RandomContext_New();
		myRNG = true;
	}
	RandomContext_SeedRandom(StarGenRNG, 123456);
	for (uqm::UQMOptions::edit().customSeed.value = START; uqm::UQMOptions::read().customSeed < (START + SAMPLE_SIZE);
		 uqm::UQMOptions::edit().customSeed = uqm::UQMOptions::read().customSeed.value + 1)
	{
		start_clock = clock();
		fmt::print(stderr, "\n\n\nStarting seed {}... ", uqm::UQMOptions::read().customSeed.value);
		InitPlot(plot_map);
		fmt::print(stderr, "seeding stars {}... ", uqm::UQMOptions::read().customSeed.value);
		SeedStarmap(star_array);
		fmt::print(stderr, "seeding plots {}... ", uqm::UQMOptions::read().customSeed.value);
		if (SeedPlot(plot_map, star_array) < NUM_PLOTS)
		{
			fmt::print(stderr, "Failed to seed {}. ", uqm::UQMOptions::read().customSeed.value);
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
	uqm::UQMOptions::edit().customSeed.value = save;
	if (StarGenRNG && myRNG)
	{
		RandomContext_Delete(StarGenRNG);
		StarGenRNG = nullptr;
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
	uint16_t i;
#ifdef DEBUG_STARSEED_TRACE_V
	SeedDEBUG();
	fmt::print(stderr, "CurrentActivity {}\n", GLOBAL(CurrentActivity));
#endif
	DefaultStarmap(star_array);
	if (!StarGenRNG)
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Init Starseed creating a STAR GEN RNG.\n");
#endif
		StarGenRNG = RandomContext_New();
		RandomContext_SeedRandom(StarGenRNG, uqm::UQMOptions::read().customSeed.value);
	}
	if (!StarSeed)
	{
		// Here we will split off, provide default plotmap, return
		// This makes it easier to integrate seemlessly old vs new
		DefaultPlot(plot_map, star_array);
		DefaultQuasispace(portal_map);
		// Done with StarGenRNG for now; will create later if moving fleets
		if (StarGenRNG)
		{
			RandomContext_Delete(StarGenRNG);
		}
		StarGenRNG = nullptr;
		return true;
	}
	if (g_seedType == uqm::SeedType::MRQ)
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
			   uqm::UQMOptions::read().customSeed.value);
	SeedStarmap(star_array);
	if (GLOBAL(CurrentActivity) || !newgame)
	{
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "*+*+* LOADING *+*+*\n");
#endif
		if (SeedPlot(plot_map, star_array) != NUM_PLOTS)
		{
			fmt::print(stderr, "Seed Plot Failed.\n");
			if (StarGenRNG)
			{
				RandomContext_Delete(StarGenRNG);
			}
			StarGenRNG = nullptr;
			return false;
		}
		if (!SeedQuasispace(portal_map, plot_map, star_array))
		{
			fmt::print(stderr, "Seed Quasisapce Failed.\n");
			if (StarGenRNG)
			{
				RandomContext_Delete(StarGenRNG);
			}
			StarGenRNG = nullptr;
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
			fmt::print(stderr, "Seed {} failed ({}).\n", uqm::UQMOptions::read().customSeed.value, ++i);
			uqm::UQMOptions::edit().customSeed.value = uqm::UQMOptions::read().customSeed.value + 1;
			SeedStarmap(star_array);
		}
		if (i >= 100)
		{
			fmt::print(stderr, "Seed Plot Failed.\n");
			if (StarGenRNG)
			{
				RandomContext_Delete(StarGenRNG);
			}
			StarGenRNG = nullptr;
			return false;
		}
		if (!SeedQuasispace(portal_map, plot_map, star_array))
		{
			fmt::print(stderr, "Seed Quasisapce Failed.\n");
			if (StarGenRNG)
			{
				RandomContext_Delete(StarGenRNG);
			}
			StarGenRNG = nullptr;
			return false;
		}
	}
	if (OOPS_ALL > 0)
	{
		for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
		{
			if (!star_array[i].Index)
			{
				star_array[i].Index = OOPS_ALL;
			}
		}
	}
	// In case the seed changed above, reset SIS
	GLOBAL_SIS(Seed) = uqm::UQMOptions::read().customSeed.value;
#ifdef DEBUG_STARSEED
	fmt::print(stderr, "Done seeding {}.\n", uqm::UQMOptions::read().customSeed.value);
#endif
	// Done with StarGenRNG for now; will create later if moving fleets
	if (StarGenRNG)
	{
		RandomContext_Delete(StarGenRNG);
	}
	StarGenRNG = nullptr;
	return (true);
}

bool inFullGame(void)
{
	ACTIVITY act = lowByte(GLOBAL(CurrentActivity));
	return (act == IN_LAST_BATTLE || act == IN_ENCOUNTER || act == IN_HYPERSPACE || act == IN_INTERPLANETARY || act == WON_LAST_BATTLE);
}

bool inSuperMelee(void)
{
	return (lowByte(GLOBAL(CurrentActivity)) == SUPER_MELEE);
	// TODO: && !inMainMenu ()
}

#if 0
bool
inBattle (void)
{
	// TODO: IN_BATTLE is also set while in HyperSpace/QuasiSpace.
	return ((GLOBAL (CurrentActivity) & IN_BATTLE) != 0);
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
	return (lowByte (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY);
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
	return (lowByte(GLOBAL(CurrentActivity)) == IN_HYPERSPACE);
	// IN_HYPERSPACE is also set for QuasiSpace
}

// In HyperSpace
// Pre: inFullGame()
bool inHyperSpace(void)
{
	//assert (inFullGame ());
	return (lowByte(GLOBAL(CurrentActivity)) == IN_HYPERSPACE) && (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1);
	// IN_HYPERSPACE is also set for QuasiSpace
}

// In QuasiSpace
// Pre: inFullGame()
bool inQuasiSpace(void)
{
	//assert (inFullGame ());
	return (lowByte(GLOBAL(CurrentActivity)) == IN_HYPERSPACE) && (GET_GAME_STATE(ARILOU_SPACE_SIDE) > 1);
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
	size_t i;
	size_t len = strlen(pStr);

	if (!pStr || !find || !replace)
	{
		return -1; // pStr, find, or replace is nullptr
	}

	if (uqm::utf8StringPos(pStr, find) == -1)
	{
		return 0; // 'find' not found
	}

	for (i = 0; i < len; i++)
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
