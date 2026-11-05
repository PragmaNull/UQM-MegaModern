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


#include "uqmdebug.h"
#include "core/stl/atomic.h"

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "clock.h"
#include "starmap.h"
#include "intel.h"
#include "sis.h"
#include "status.h"
#include "gamestr.h"
#include "gameev.h"
#include "gendef.h"
#include "globdata.h"
#include "planets/lifeform.h"
#include "planets/scan.h"
#include "races.h"
#include "setup.h"
#include "starmap.h"
#include "state.h"
#include "core/platform/platform.h"
#include "core/string/StringUtils.h"
#include "libs/mathlib.h"
#include "uqm/lua/UQMLua.h"
#include "tactrans.h"
#include "StarmapGlobalData.h"

#include <stdio.h>
#include <errno.h>

uqstl::atomic<void (*)(void)> debugHook {nullptr};
bool DebugKeyPressed;

// Move the Flagship to the destination of the autopilot.
// Should only be called from HyperSpace/QuasiSpace.
// It can be called from debugHook directly after entering HS/QS though.
void doInstantMove(void)
{
	// Move to the new location:
	if (!uqm::flagship().autopilot.hasTarget())
	{
		// If no destination has been selected, use the current location
		// as the destination.
		uqm::flagship().autopilot.setTarget({LogxToUniverse(uqm::gameState().sis().log_x), LogyToUniverse(uqm::gameState().sis().log_y)});
	}
	else
	{
		// A new destination has been selected.
		uqm::gameState().sis().log_x = UniverseToLogx(uqm::flagship().autopilot.target().x);
		uqm::gameState().sis().log_y = UniverseToLogy(uqm::flagship().autopilot.target().y);
	}

	// Check for a solar systems at the destination.
	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
	{
		// If there's a solar system at the destination, enter it.
		GFXPOINT target {uqm::flagship().autopilot.target()};
		CurStarDescPtr() = FindStar(0, &target, 0, 0);
		if (CurStarDescPtr())
		{
			// Leave HyperSpace/QuasiSpace if we're there:
			uqm::gameState().setFlag(uqm::GameFlag::USED_BROADCASTER, static_cast<uint32_t>(0));
			uqm::currentActivity() &= ~IN_BATTLE;

			// Enter IP:
			uqm::flagship().shipFacing = 0;
			uqm::flagship().ipPlanet = 0;
			uqm::flagship().inOrbit = 0;
			// This causes the ship position in IP to be reset.
			uqm::currentActivity() |= START_INTERPLANETARY;
		}
	}

	// Turn off the autopilot:
	uqm::flagship().autopilot.clear();
}

// playerNr should be 0 or 1
STARSHIP*
findPlayerShip(int16_t playerNr)
{
	HELEMENT hNextElement {};

	for (HELEMENT hElement {GetHeadElement()}; hElement; hElement = hNextElement)
	{
		ELEMENT* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		hNextElement = GetSuccElement(ElementPtr);

		if ((ElementPtr->state_flags & PlayerShip) && ElementPtr->playerNr == playerNr)
		{
			STARSHIP* StarShipPtr {};
			GetElementStarShip(ElementPtr, &StarShipPtr);
			UnlockElement(hElement);
			return StarShipPtr;
		}

		UnlockElement(hElement);
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////

void resetEnergyBattle(void)
{
	const STARSHIP* StarShipPtr {};
	uint16_t delta {};

	if (!(uqm::currentActivity() & IN_BATTLE) || inHQSpace())
	{
		return;
	}

	if (testFlag(PlayerControl[1], PlayerControlFlags::Human))
	{
		StarShipPtr = findPlayerShip(NpcPlayerNum);
	}
	else if (testFlag(PlayerControl[0], PlayerControlFlags::Human))
	{
		StarShipPtr = findPlayerShip(RpgPlayerNum);
	}
	else
	{
		StarShipPtr = nullptr;
	}

	if (StarShipPtr == nullptr || StarShipPtr->RaceDescPtr == nullptr)
	{
		return;
	}

	delta = StarShipPtr->RaceDescPtr->ship_info.max_energy - StarShipPtr->RaceDescPtr->ship_info.energy_level;

	GFXCONTEXT OldContext {SetContext(StatusContext)};
	DeltaEnergy((ELEMENT*)StarShipPtr->hShip, delta);
	SetContext(OldContext);
}

// Kills the opponent of the player controlled ship
static void
scuttleOpponent(void)
{
	const STARSHIP* StarShipPtr {};
	uint16_t delta {};
	GFXCONTEXT OldContext {};

	if (!(uqm::currentActivity() & IN_BATTLE) || inHQSpace())
	{
		return;
	}

	if (testFlag(PlayerControl[1], PlayerControlFlags::Human))
	{
		StarShipPtr = findPlayerShip(RpgPlayerNum);
	}
	else if (testFlag(PlayerControl[0], PlayerControlFlags::Human))
	{
		StarShipPtr = findPlayerShip(NpcPlayerNum);
	}
	else
	{
		StarShipPtr = nullptr;
	}

	if (StarShipPtr == nullptr || StarShipPtr->RaceDescPtr == nullptr)
	{
		return;
	}

	delta = StarShipPtr->RaceDescPtr->ship_info.crew_level;

	if (delta > 0)
	{
		OldContext = SetContext(StatusContext);
		DeltaCrew((ELEMENT*)StarShipPtr->hShip, -delta);
		SetContext(OldContext);
		ship_death((ELEMENT*)StarShipPtr->hShip);
	}
}

// Zeroes out all ship's velocity, freezing them in their tracks
static void
HaltShips(void)
{
	const STARSHIP* StarShipPtr {};
	ELEMENT* ElementPtr {};

	if (!(uqm::currentActivity() & IN_BATTLE) || inHQSpace())
	{
		return;
	}

	for (uint8_t i {0}; i < 2; i++)
	{
		StarShipPtr = findPlayerShip(i);

		if (StarShipPtr == nullptr || StarShipPtr->RaceDescPtr == nullptr)
		{
			return;
		}

		LockElement(StarShipPtr->hShip, &ElementPtr);
		ZeroVelocityComponents(&ElementPtr->velocity);
		UnlockElement(StarShipPtr->hShip);
	}
}

#if defined(DEBUG) || defined(USE_DEBUG_KEY)

static void dumpEventCallback(const EVENT* eventPtr, void* arg);

static void starRecurse(STAR_DESC* star, void* arg);
static void planetRecurse(STAR_DESC* star, SOLARSYS_STATE* system,
						  PLANET_DESC* planet, void* arg);
static void moonRecurse(STAR_DESC* star, SOLARSYS_STATE* system,
						PLANET_DESC* planet, PLANET_DESC* moon, void* arg);

static void dumpSystemCallback(const STAR_DESC* star,
							   const SOLARSYS_STATE* system, void* arg);
static void dumpPlanetCallback(const PLANET_DESC* planet, void* arg);
static void dumpMoonCallback(const PLANET_DESC* moon, void* arg);
static void dumpWorld(FILE* out, const PLANET_DESC* world);

typedef struct TallyResourcesArg TallyResourcesArg;
static void tallySystemPreCallback(const STAR_DESC* star, const SOLARSYS_STATE* system, void* arg);
static void tallySystemPostCallback(const STAR_DESC* star, const SOLARSYS_STATE* system, void* arg);
static void tallyPlanetCallback(const PLANET_DESC* planet, void* arg);
static void tallyMoonCallback(const PLANET_DESC* moon, void* arg);
static void tallyResourcesWorld(TallyResourcesArg* arg,
								const PLANET_DESC* world);

static void dumpPlanetTypeCallback(int index, const PlanetFrame* planet,
								   void* arg);

bool disableInteractivity = false;
bool EnableDebugEvents = false;

// Must be called on the Starcon2Main thread.
// This function is called synchronously wrt the game logic thread.
void debugKeyPressedSynchronous(void)
{
	if (!DebugKeyPressed)
	{
		printf("Debug Key Activated\n\n");
		equipShip();
		showSpheres(false);
		uqm::gameState().setFlag(uqm::GameFlag::KNOW_QS_PORTAL, static_cast<uint32_t>(~0));
		uqm::gameState().setFlag(uqm::GameFlag::KNOW_HOMEWORLD, static_cast<uint32_t>(~0));
	}

	forwardToNextEvent(true);

	if (EnableDebugEvents)
	{
		// State modifying:
		giveDevices();

		// Give the player the ships you can't ally with under normal
		// conditions.
		clearEscorts();
		AddEscortShips(ARILOU_SHIP, 1);
		AddEscortShips(PKUNK_SHIP, 1);
		AddEscortShips(VUX_SHIP, 1);
		AddEscortShips(YEHAT_SHIP, 1);
		AddEscortShips(MELNORME_SHIP, 1);
		AddEscortShips(DRUUGE_SHIP, 1);
		AddEscortShips(ILWRATH_SHIP, 1);
		AddEscortShips(MYCON_SHIP, 1);
		AddEscortShips(SLYLANDRO_SHIP, 1);
		AddEscortShips(UMGAH_SHIP, 1);
		AddEscortShips(URQUAN_SHIP, 1);
		AddEscortShips(BLACK_URQUAN_SHIP, 1);

		resetCrewBattle();
		resetEnergyBattle();
		activateAllShips();
		uqm::gameState().setFlag(uqm::GameFlag::MELNORME_CREDIT1, static_cast<uint32_t>(100));
		uqm::gameState().sis().ResUnits = 100000;

		// Informational:
		dumpEvents(stderr);

		// Graphical and textual:
		debugContexts();
	}

	DebugKeyPressed = true;
}

void debugKey2PressedSynchronous(void)
{
	scuttleOpponent();
}

void debugKey3PressedSynchronous(void)
{
	HaltShips();
}

void debugKey4PressedSynchronous(void)
{
}

// Can be called on any thread, but usually on main()
// This function is called asynchronously wrt the game logic thread,
// which means locking applies. Use carefully.
// TODO: Once game logic thread is purged of graphics and clock locks,
//   this function may not call graphics and game clock functions at all.
void debugKeyPressed(void)
{
	if (EnableDebugEvents)
	{
		// Informational:
		dumpStrings(stdout);
		dumpPlanetTypes(stderr);
		debugHook = dumpUniverseToFile;
		// This will cause dumpUniverseToFile to be called from the
		// Starcon2Main loop. Calling it from here would give threading
		// problems.
		debugHook = tallyResourcesToFile;
		// This will cause tallyResourcesToFile to be called from the
		// Starcon2Main loop. Calling it from here would give threading
		// problems.

		// Interactive:
		uqm::lua().runDebugConsole();
	}
}

void debugKey2Pressed(void)
{
}

void debugKey3Pressed(void)
{
}

void debugKey4Pressed(void)
{
}

////////////////////////////////////////////////////////////////////////////

// Fast forwards to the next event.
// If skipHEE is set, HYPERSPACE_ENCOUNTER_EVENTs are skipped.
// Must be called from the Starcon2Main thread.
// TODO: LockGameClock may be removed since it is only
//   supposed to be called synchronously wrt the game logic thread.
void forwardToNextEvent(bool skipHEE)
{
	MoveGameClockToNextEvent(skipHEE);
}

const char*
eventName(uint8_t func_index)
{
	switch (func_index)
	{
		case ARILOU_ENTRANCE_EVENT:
			return "ARILOU_ENTRANCE_EVENT";
		case ARILOU_EXIT_EVENT:
			return "ARILOU_EXIT_EVENT";
		case HYPERSPACE_ENCOUNTER_EVENT:
			return "HYPERSPACE_ENCOUNTER_EVENT";
		case KOHR_AH_VICTORIOUS_EVENT:
			return "KOHR_AH_VICTORIOUS_EVENT";
		case ADVANCE_PKUNK_MISSION:
			return "ADVANCE_PKUNK_MISSION";
		case ADVANCE_THRADD_MISSION:
			return "ADVANCE_THRADD_MISSION";
		case ZOQFOT_DISTRESS_EVENT:
			return "ZOQFOT_DISTRESS";
		case ZOQFOT_DEATH_EVENT:
			return "ZOQFOT_DEATH_EVENT";
		case SHOFIXTI_RETURN_EVENT:
			return "SHOFIXTI_RETURN_EVENT";
		case ADVANCE_UTWIG_SUPOX_MISSION:
			return "ADVANCE_UTWIG_SUPOX_MISSION";
		case KOHR_AH_GENOCIDE_EVENT:
			return "KOHR_AH_GENOCIDE_EVENT";
		case SPATHI_SHIELD_EVENT:
			return "SPATHI_SHIELD_EVENT";
		case ADVANCE_ILWRATH_MISSION:
			return "ADVANCE_ILWRATH_MISSION";
		case ADVANCE_MYCON_MISSION:
			return "ADVANCE_MYCON_MISSION";
		case ARILOU_UMGAH_CHECK:
			return "ARILOU_UMGAH_CHECK";
		case YEHAT_REBEL_EVENT:
			return "YEHAT_REBEL_EVENT";
		case SLYLANDRO_RAMP_UP:
			return "SLYLANDRO_RAMP_UP";
		case SLYLANDRO_RAMP_DOWN:
			return "SLYLANDRO_RAMP_DOWN";
		default:
			// Should not happen
			return "???";
	}
}

const char*
raceName(uint8_t func_index)
{
	switch (func_index)
	{
		case ARILOU_ID:
			return "Arilou Lalee'lay";
		case CHMMR_ID:
			return "Chmmr";
		case EARTHLING_ID:
			return "Human";
		case ORZ_ID:
			return "Orz";
		case PKUNK_ID:
			return "Pkunk";
		case SHOFIXTI_ID:
			return "Shofixti";
		case SPATHI_ID:
			return "Spathi";
		case SUPOX_ID:
			return "Supox";
		case THRADDASH_ID:
			return "Thraddash";
		case UTWIG_ID:
			return "Utwig";
		case VUX_ID:
			return "Vux";
		case YEHAT_ID:
			return "Yehat";
		case MELNORME_ID:
			return "Melnorme";
		case DRUUGE_ID:
			return "Druuge";
		case ILWRATH_ID:
			return "Ilwrath";
		case MYCON_ID:
			return "Mycon";
		case SLYLANDRO_ID:
			return "Slylandro";
		case UMGAH_ID:
			return "Umgah";
		case UR_QUAN_ID:
			return "Ur-Quan";
		case ZOQFOTPIK_ID:
			return "Zoq-Fot-Pik";
		case SYREEN_ID:
			return "Syreen";
		case KOHR_AH_ID:
			return "Kohr-Ah";
		case ANDROSYNTH_ID:
			return "Androsynth";
		case CHENJESU_ID:
			return "Chenjesu";
		case MMRNMHRM_ID:
			return "Mmrnmhrm";
		default:
			// Should not happen
			return "???";
	}
}

static void
dumpEventCallback(const EVENT* eventPtr, void* arg)
{
	FILE* out = (FILE*)arg;
	dumpEvent(out, eventPtr);
}

void dumpEvent(FILE* out, const EVENT* eventPtr)
{
	fmt::print(out, "{:4}/{:02}/{:02}: {}\n",
			   eventPtr->year_index,
			   eventPtr->month_index,
			   eventPtr->day_index,
			   eventName(eventPtr->func_index));
}

void dumpEvents(FILE* out)
{
	LockGameClock();
	ForAllEvents(reinterpret_cast<void (*)(LINK*, void*)>(dumpEventCallback), out);
	UnlockGameClock();
}

////////////////////////////////////////////////////////////////////////////

// NB: Ship maximum speed and turning rate aren't updated in
// HyperSpace/QuasiSpace or in melee.
void equipShip(void)
{
	int i;

	// Don't do anything unless in the full game.
	if (uqm::lowByte(uqm::currentActivity()) == SUPER_MELEE)
	{
		return;
	}

	// Thrusters:
	for (i = 0; i < NumDriveSlots; i++)
	{
		uqm::gameState().sis().DriveSlots[i] = FUSION_THRUSTER;
	}

	// Turning jets:
	for (i = 0; i < NumJetSlots; i++)
	{
		uqm::gameState().sis().JetSlots[i] = TURNING_JETS;
	}

	// Shields:
	uqm::gameState().setFlag(uqm::GameFlag::LANDER_SHIELDS, static_cast<uint32_t>((1 << EARTHQUAKE_DISASTER) | (1 << BIOLOGICAL_DISASTER) | (1 << LIGHTNING_DISASTER) | (1 << LAVASPOT_DISASTER)));
	// Lander upgrades:
	uqm::gameState().setFlag(uqm::GameFlag::IMPROVED_LANDER_SPEED, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::IMPROVED_LANDER_CARGO, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::IMPROVED_LANDER_SHOT, static_cast<uint32_t>(1));

	// Modules:
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2)
	{
		// The Precursor bomb has not been installed.
		// This is the original TFB testing layout.
		i = 0;
		uqm::gameState().sis().ModuleSlots[i++] = HIGHEFF_FUELSYS;
		uqm::gameState().sis().ModuleSlots[i++] = CREW_POD;
		uqm::gameState().sis().ModuleSlots[i++] = STORAGE_BAY;
		uqm::gameState().sis().ModuleSlots[i++] = STORAGE_BAY;
		uqm::gameState().sis().ModuleSlots[i++] = ANTIMISSILE_DEFENSE;
		uqm::gameState().sis().ModuleSlots[i++] = TRACKING_SYSTEM;
		uqm::gameState().sis().ModuleSlots[i++] = TRACKING_SYSTEM;
		uqm::gameState().sis().ModuleSlots[i++] = TRACKING_SYSTEM;
		uqm::gameState().sis().ModuleSlots[i++] = DYNAMO_UNIT;
		uqm::gameState().sis().ModuleSlots[i++] = DYNAMO_UNIT;
		uqm::gameState().sis().ModuleSlots[i++] = DYNAMO_UNIT;
		uqm::gameState().sis().ModuleSlots[i++] = SHIVA_FURNACE;
		uqm::gameState().sis().ModuleSlots[i++] = SHIVA_FURNACE;
		uqm::gameState().sis().ModuleSlots[i++] = SHIVA_FURNACE;
		uqm::gameState().sis().ModuleSlots[i++] = CANNON_WEAPON;
		uqm::gameState().sis().ModuleSlots[i++] = CANNON_WEAPON;

		// Landers:
		uqm::gameState().sis().NumLanders = MaxLanders;
	}
	else
	{
		// The Precursor bomb has been installed.
		i = NumBombModules;
		uqm::gameState().sis().ModuleSlots[i++] = HIGHEFF_FUELSYS;
		uqm::gameState().sis().ModuleSlots[i++] = CREW_POD;
		uqm::gameState().sis().ModuleSlots[i++] = DYNAMO_UNIT;
		uqm::gameState().sis().ModuleSlots[i++] = SHIVA_FURNACE;
		uqm::gameState().sis().ModuleSlots[i++] = CANNON_WEAPON;
		uqm::gameState().sis().ModuleSlots[i++] = TRACKING_SYSTEM;
	}

	assert(i <= NumModuleSlots);

	// Fill the fuel and crew compartments to the maximum.
	uqm::gameState().sis().FuelOnBoard = FuelReserve;
	uqm::gameState().sis().CrewEnlisted = 0;
	for (i = 0; i < NumModuleSlots; i++)
	{
		switch (uqm::gameState().sis().ModuleSlots[i])
		{
			case CREW_POD:
				uqm::gameState().sis().CrewEnlisted += CrewPodCapacity;
				break;
			case FUEL_TANK:
				uqm::gameState().sis().FuelOnBoard += FuelTankCapacity;
				break;
			case HIGHEFF_FUELSYS:
				uqm::gameState().sis().FuelOnBoard += HefuelTankCapacity;
				break;
		}
	}

	// Update the maximum speed and turning rate when in interplanetary.
	if (pSolarSysState != nullptr)
	{
		// Thrusters:
		pSolarSysState->max_ship_speed = 5 * IpShipThrustIncrement;
		for (i = 0; i < NumDriveSlots; i++)
		{
			if (uqm::gameState().sis().DriveSlots[i] == FUSION_THRUSTER)
			{
				pSolarSysState->max_ship_speed += IpShipThrustIncrement;
			}
		}

		// Turning jets:
		pSolarSysState->turn_wait = IpShipTurnWait;
		for (i = 0; i < NumJetSlots; i++)
		{
			if (uqm::gameState().sis().JetSlots[i] == TURNING_JETS)
			{
				pSolarSysState->turn_wait -= IpShipTurnDecrement;
			}
		}
	}

	// Make sure everything is redrawn:
	if (inHQSpace() || uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY)
	{
		DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);
	}
}

////////////////////////////////////////////////////////////////////////////

void giveDevices(void)
{
	uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::GLOWING_ROD_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::SUN_DEVICE_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(1));
	//SET_GAME_STATE (ULTRON_CONDITION, 2);
	//SET_GAME_STATE (ULTRON_CONDITION, 3);
	//SET_GAME_STATE (ULTRON_CONDITION, 4);
	uqm::gameState().setFlag(uqm::GameFlag::MAIDENS_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::AQUA_HELIX_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::UMGAH_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE0_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE1_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE2_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::PORTAL_KEY_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::MOONBASE_ON_SHIP, static_cast<uint32_t>(1));

	// Not strictly a device (although it originally was one).
	uqm::gameState().setFlag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP, static_cast<uint32_t>(1));
}

////////////////////////////////////////////////////////////////////////////

void clearEscorts(void)
{
	HSHIPFRAG hStarShip, hNextShip;

	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q);
		 hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT* StarShipPtr;

		StarShipPtr = LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);

		RemoveQueue(&uqm::gameState().game().built_ship_q, hStarShip);
		FreeShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}

	DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);
}

////////////////////////////////////////////////////////////////////////////

#if 0
// Not needed anymore, but could be useful in the future.

// Find the HELEMENT belonging to the Flagship.
static HELEMENT
findFlagshipElement (void)
{
	HELEMENT hElement, hNextElement;
	ELEMENT *ElementPtr;

	// Find the ship element.
	for (hElement = GetTailElement (); hElement != 0;
			hElement = hNextElement)
	{
		LockElement (hElement, &ElementPtr);

		if ((ElementPtr->state_flags & PlayerShip) != 0)
		{
			UnlockElement (hElement);
			return hElement;
		}

		hNextElement = GetPredElement (ElementPtr);
		UnlockElement (hElement);
	}
	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////

void showSpheres(bool Animated)
{
	uint8_t i;

	if (Animated)
	{ // Alternative which allows you to view
		// the animated SOI expand for each race
		for (i = 0; i <= BLACK_URQUAN_SHIP; i++)
		{
			StartSphereTracking((RACE_ID)i);
		}
	}
	else
	{
		HFLEETINFO hStarShip, hNextShip;

		for (hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
			 hStarShip != nullptr; hStarShip = hNextShip)
		{
			FLEET_INFO* FleetPtr;

			FleetPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
			hNextShip = _GetSuccLink(FleetPtr);

			if ((FleetPtr->actual_strength != InfiniteRadius) && (FleetPtr->known_strength != FleetPtr->actual_strength))
			{
				FleetPtr->known_strength = FleetPtr->actual_strength;
				FleetPtr->known_loc = FleetPtr->loc;
			}

			UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
		}
	}

	if (IsExtended())
	{
		HFLEETINFO hSyreen = GetStarShipFromIndex(
			&uqm::gameState().game().avail_race_q, SYREEN_SHIP);
		FLEET_INFO* SyreenPtr = LockFleetInfo(
			&uqm::gameState().game().avail_race_q, hSyreen);
		HFLEETINFO hChmmr = GetStarShipFromIndex(
			&uqm::gameState().game().avail_race_q, CHMMR_SHIP);
		FLEET_INFO* ChmmrPtr = LockFleetInfo(
			&uqm::gameState().game().avail_race_q, hChmmr);

		SyreenPtr->actual_strength = 300 / SphereRadiusIncrement * 2;
		SyreenPtr->loc = SeedFleetLocation(SyreenPtr, plot_map, HOME);
		StartSphereTracking(SYREEN_SHIP);

		ChmmrPtr->actual_strength = 986 / SphereRadiusIncrement * 2;
		ChmmrPtr->loc = SeedFleetLocation(ChmmrPtr, plot_map, HOME);
		StartSphereTracking(CHMMR_SHIP);

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hSyreen);
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hChmmr);
	}
}

////////////////////////////////////////////////////////////////////////////

void activateAllShips(void)
{
	HFLEETINFO hStarShip, hNextShip;

	for (hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hStarShip != nullptr; hStarShip = hNextShip)
	{
		FLEET_INFO* FleetPtr;

		FleetPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
		hNextShip = _GetSuccLink(FleetPtr);

		if (FleetPtr->icons != nullptr)
		// Skip the Ur-Quan probe.
		{
			FleetPtr->allied_state = GOOD_GUY;
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}
}

////////////////////////////////////////////////////////////////////////////

void forAllStars(void (*callback)(STAR_DESC*, void*), void* arg)
{
	int i;

	for (i = 0; i < NumSolarSystems; i++)
	{
		callback(&star_array[i], arg);
	}
}

void forAllPlanets(STAR_DESC* star, SOLARSYS_STATE* system, void (*callback)(STAR_DESC*, SOLARSYS_STATE*, PLANET_DESC*, void*), void* arg)
{
	uint16_t i;

	assert(CurStarDescPtr() == star);
	assert(pSolarSysState == system);

	for (i = 0; i < system->SunDesc[0].NumPlanets; i++)
	{
		callback(star, system, &system->PlanetDesc[i], arg);
	}
}

void forAllMoons(STAR_DESC* star, SOLARSYS_STATE* system, PLANET_DESC* planet,
				 void (*callback)(STAR_DESC*, SOLARSYS_STATE*, PLANET_DESC*,
								  PLANET_DESC*, void*),
				 void* arg)
{
	uint16_t i;

	assert(pSolarSysState == system);

	for (i = 0; i < planet->NumPlanets; i++)
	{
		callback(star, system, planet, &system->MoonDesc[i], arg);
	}
}

////////////////////////////////////////////////////////////////////////////

// Must be called from the Starcon2Main thread.
// TODO: LockGameClock may be removed
void UniverseRecurse(UniverseRecurseArg* universeRecurseArg)
{
	ACTIVITY savedActivity;

	if (universeRecurseArg->systemFuncPre == nullptr
		&& universeRecurseArg->systemFuncPost == nullptr
		&& universeRecurseArg->planetFuncPre == nullptr
		&& universeRecurseArg->planetFuncPost == nullptr
		&& universeRecurseArg->moonFunc == nullptr)
	{
		return;
	}

	LockGameClock();
	//TFB_DEBUG_HALT = 1;
	savedActivity = uqm::currentActivity();
	disableInteractivity = true;

	forAllStars(starRecurse, (void*)universeRecurseArg);

	disableInteractivity = false;
	uqm::currentActivity() = savedActivity;
	UnlockGameClock();
}

static void
starRecurse(STAR_DESC* star, void* arg)
{
	UniverseRecurseArg* universeRecurseArg = (UniverseRecurseArg*)arg;

	SOLARSYS_STATE SolarSysState {};
	SOLARSYS_STATE* oldPSolarSysState = pSolarSysState;
	STAR_DESC* oldStarDescPtr = CurStarDescPtr();
	CurStarDescPtr() = star;

	SysGenRNGDebug->reseed(GetRandomSeedForStar(star));

	SolarSysState.SunDesc[0].pPrevDesc = 0;
	SolarSysState.SunDesc[0].rand_seed = SysGenRNGDebug->next();
	SolarSysState.SunDesc[0].data_index = StarType(star->Type);
	SolarSysState.SunDesc[0].location.x = 0;
	SolarSysState.SunDesc[0].location.y = 0;
	//SolarSysState.SunDesc[0].radius = MinZoomRadius;
	SolarSysState.genFuncs = getGenerateFunctions(star->Index);

	pSolarSysState = &SolarSysState;
	(*SolarSysState.genFuncs->generatePlanets)(&SolarSysState);

	if (universeRecurseArg->systemFuncPre != nullptr)
	{
		(*universeRecurseArg->systemFuncPre)(
			star, &SolarSysState, universeRecurseArg->arg);
	}

	if (universeRecurseArg->planetFuncPre != nullptr
		|| universeRecurseArg->planetFuncPost != nullptr
		|| universeRecurseArg->moonFunc != nullptr)
	{
		forAllPlanets(star, &SolarSysState, planetRecurse,
					  (void*)universeRecurseArg);
	}

	if (universeRecurseArg->systemFuncPost != nullptr)
	{
		(*universeRecurseArg->systemFuncPost)(
			star, &SolarSysState, universeRecurseArg->arg);
	}

	pSolarSysState = oldPSolarSysState;
	CurStarDescPtr() = oldStarDescPtr;
}

static void
planetRecurse(STAR_DESC* star, SOLARSYS_STATE* system, PLANET_DESC* planet,
			  void* arg)
{
	UniverseRecurseArg* universeRecurseArg = (UniverseRecurseArg*)arg;

	assert(CurStarDescPtr() == star);
	assert(pSolarSysState == system);

	planet->pPrevDesc = &system->SunDesc[0];

	if (universeRecurseArg->planetFuncPre != nullptr)
	{
		system->pOrbitalDesc = planet;
		DoPlanetaryAnalysis(&system->SysInfo, planet);
		// When GenerateDefaultFunctions is used as genFuncs,
		// generateOrbital will also call DoPlanetaryAnalysis,
		// but with other GenerateFunctions this is not guaranteed.
		(*system->genFuncs->generateOrbital)(system, planet);
		(*universeRecurseArg->planetFuncPre)(
			planet, universeRecurseArg->arg);
	}

	if (universeRecurseArg->moonFunc != nullptr)
	{
		SysGenRNGDebug->reseed(planet->rand_seed);

		(*system->genFuncs->generateMoons)(system, planet);

		forAllMoons(star, system, planet, moonRecurse,
					(void*)universeRecurseArg);
	}

	if (universeRecurseArg->planetFuncPost != nullptr)
	{
		system->pOrbitalDesc = planet;
		DoPlanetaryAnalysis(&system->SysInfo, planet);
		// When GenerateDefaultFunctions is used as genFuncs,
		// generateOrbital will also call DoPlanetaryAnalysis,
		// but with other GenerateFunctions this is not guaranteed.
		(*system->genFuncs->generateOrbital)(system, planet);
		(*universeRecurseArg->planetFuncPost)(
			planet, universeRecurseArg->arg);
	}
}

static void
moonRecurse(STAR_DESC* star, SOLARSYS_STATE* system, PLANET_DESC* planet,
			PLANET_DESC* moon, void* arg)
{
	UniverseRecurseArg* universeRecurseArg = (UniverseRecurseArg*)arg;

	assert(CurStarDescPtr() == star);
	assert(pSolarSysState == system);

	moon->pPrevDesc = planet;

	if (universeRecurseArg->moonFunc != nullptr)
	{
		system->pOrbitalDesc = moon;
		if (moon->data_index != HIERARCHY_STARBASE
			&& moon->data_index != SA_MATRA
			&& moon->data_index != DESTROYED_STARBASE
			&& moon->data_index != PRECURSOR_STARBASE)
		{
			DoPlanetaryAnalysis(&system->SysInfo, moon);
			// When GenerateDefaultFunctions is used as genFuncs,
			// generateOrbital will also call DoPlanetaryAnalysis,
			// but with other GenerateFunctions this is not guaranteed.
		}
		(*system->genFuncs->generateOrbital)(system, moon);
		(*universeRecurseArg->moonFunc)(
			moon, universeRecurseArg->arg);
	}
}

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	FILE* out;
} DumpUniverseArg;

// Must be called from the Starcon2Main thread.
void dumpUniverse(FILE* out)
{
	DumpUniverseArg dumpUniverseArg;
	UniverseRecurseArg universeRecurseArg;

	dumpUniverseArg.out = out;

	universeRecurseArg.systemFuncPre = dumpSystemCallback;
	universeRecurseArg.systemFuncPost = nullptr;
	universeRecurseArg.planetFuncPre = dumpPlanetCallback;
	universeRecurseArg.planetFuncPost = nullptr;
	universeRecurseArg.moonFunc = dumpMoonCallback;
	universeRecurseArg.arg = (void*)&dumpUniverseArg;

	UniverseRecurse(&universeRecurseArg);
}

static constexpr uqgsl::czstring UniverseDumpFile {"PlanetInfo"};

// Must be called from the Starcon2Main thread.
void dumpUniverseToFile(void)
{
	const uqm::FilePtr out {uqm::openFile(UniverseDumpFile, "w")};
	if (out == nullptr)
	{
		fmt::print(stderr, "Error: Could not open file '{}' for "
						   "writing: {}",
				   UniverseDumpFile, uqm::strerror(errno));
		return;
	}

	dumpUniverse(out.get());

	fmt::print(stdout, "*** Star dump complete. The game may be in an "
					   "undefined state.");
	// Data generation may have changed the game state,
	// in particular for special planet generation.
}

static void
dumpSystemCallback(const STAR_DESC* star, const SOLARSYS_STATE* system,
				   void* arg)
{
	FILE* out = ((DumpUniverseArg*)arg)->out;
	dumpSystem(out, star, system);
}

void dumpSystem(FILE* out, const STAR_DESC* star, const SOLARSYS_STATE* system)
{
	char name[256];
	char buf[40];

	GetClusterName(star, name);
	fmt::format_to_sz_n(buf, "{} {}",
						bodyColorString(StarColor(star->Type)),
						starTypeString(StarType(star->Type)));

	fmt::print(out, "{:>22}  ({:3}.{:1}, {:3}.{:1}) {:>19}  {}\n",
			   name,
			   star->star_pt.x / 10, star->star_pt.x % 10,
			   star->star_pt.y / 10, star->star_pt.y % 10,
			   buf,
			   starPresenceString(star->Index));

	(void)system; /* satisfy compiler */
}

const char*
bodyColorString(uint8_t col)
{
	switch (col)
	{
		case BLUE_BODY:
			return "blue";
		case GREEN_BODY:
			return "green";
		case ORANGE_BODY:
			return "orange";
		case RED_BODY:
			return "red";
		case WHITE_BODY:
			return "white";
		case YELLOW_BODY:
			return "yellow";
		case CYAN_BODY:
			return "cyan";
		case PURPLE_BODY:
			return "purple";
		case VIOLET_BODY:
			return "violet";
		default:
			// Should not happen
			return "???";
	}
}

const char*
starTypeString(uint8_t type)
{
	switch (type)
	{
		case DWARF_STAR:
			return "dwarf";
		case GIANT_STAR:
			return "giant";
		case SUPER_GIANT_STAR:
			return "super giant";
		default:
			// Should not happen
			return "???";
	}
}

const char*
starPresenceString(uint8_t index)
{
	switch (index)
	{
		case 0:
			// nothing
			return "";
		case SOL_DEFINED:
			return "Sol";
		case SHOFIXTI_DEFINED:
			return "Shofixti male";
		case MAIDENS_DEFINED:
			return "Shofixti maidens";
		case START_COLONY_DEFINED:
			return "Starting colony";
		case SPATHI_DEFINED:
			return "Spathi home";
		case ZOQFOT_DEFINED:
			return "Zoq-Fot-Pik home";
		case MELNORME0_DEFINED:
			return "Melnorme trader #0";
		case MELNORME1_DEFINED:
			return "Melnorme trader #1";
		case MELNORME2_DEFINED:
			return "Melnorme trader #2";
		case MELNORME3_DEFINED:
			return "Melnorme trader #3";
		case MELNORME4_DEFINED:
			return "Melnorme trader #4";
		case MELNORME5_DEFINED:
			return "Melnorme trader #5";
		case MELNORME6_DEFINED:
			return "Melnorme trader #6";
		case MELNORME7_DEFINED:
			return "Melnorme trader #7";
		case MELNORME8_DEFINED:
			return "Melnorme trader #8";
		case TALKING_PET_DEFINED:
			return "Talking Pet";
		case CHMMR_DEFINED:
			return "Chmmr home";
		case SYREEN_DEFINED:
			return "Syreen home";
		case BURVIXESE_DEFINED:
			return "Burvixese ruins";
		case SLYLANDRO_DEFINED:
			return "Slylandro home";
		case DRUUGE_DEFINED:
			return "Druuge home";
		case BOMB_DEFINED:
			return "Precursor bomb";
		case AQUA_HELIX_DEFINED:
			return "Aqua Helix";
		case SUN_DEVICE_DEFINED:
			return "Sun Device";
		case TAALO_PROTECTOR_DEFINED:
			return "Taalo Shield";
		case SHIP_VAULT_DEFINED:
			return "Syreen ship vault";
		case URQUAN_WRECK_DEFINED:
			return "Ur-Quan ship wreck";
		case VUX_BEAST_DEFINED:
			return "Zex' beauty";
		case SAMATRA_DEFINED:
			return "Sa-Matra";
		case ZOQ_SCOUT_DEFINED:
			return "Zoq-Fot-Pik scout";
		case MYCON_DEFINED:
			return "Mycon home";
		case EGG_CASE0_DEFINED:
			return "Mycon egg shell #0";
		case EGG_CASE1_DEFINED:
			return "Mycon egg shell #1";
		case EGG_CASE2_DEFINED:
			return "Mycon egg shell #2";
		case PKUNK_DEFINED:
			return "Pkunk home";
		case UTWIG_DEFINED:
			return "Utwig home";
		case SUPOX_DEFINED:
			return "Supox home";
		case YEHAT_DEFINED:
			return "Yehat home";
		case VUX_DEFINED:
			return "Vux home";
		case ORZ_DEFINED:
			return "Orz home";
		case THRADD_DEFINED:
			return "Thraddash home";
		case RAINBOW0_DEFINED:
		case RAINBOW1_DEFINED:
		case RAINBOW2_DEFINED:
		case RAINBOW3_DEFINED:
		case RAINBOW4_DEFINED:
		case RAINBOW5_DEFINED:
		case RAINBOW6_DEFINED:
		case RAINBOW7_DEFINED:
		case RAINBOW8_DEFINED:
		case RAINBOW9_DEFINED:
			return "Rainbow world";
		case ILWRATH_DEFINED:
			return "Ilwrath home";
		case ANDROSYNTH_DEFINED:
			return "Androsynth ruins";
		case MYCON_TRAP_DEFINED:
			return "Mycon trap";
		case URQUAN0_DEFINED:
		case URQUAN1_DEFINED:
		case URQUAN2_DEFINED:
		case KOHRAH0_DEFINED:
		case KOHRAH1_DEFINED:
		case KOHRAH2_DEFINED:
		case DESTROYED_STARBASE_DEFINED:
			return "Destroyed Starbase";
		case MOTHER_ARK_DEFINED:
			return "Mother-Ark";
		case ZOQ_COLONY0_DEFINED:
		case ZOQ_COLONY1_DEFINED:
		case ZOQ_COLONY2_DEFINED:
		case ZOQ_COLONY3_DEFINED:
			return "Zoq Fot Ruins";
		case ALGOLITES_DEFINED:
			return "Algolites";
		default:
			// Should not happen
			return "???";
	}
}

static void
dumpPlanetCallback(const PLANET_DESC* planet, void* arg)
{
	FILE* out = ((DumpUniverseArg*)arg)->out;
	dumpPlanet(out, planet);
}

void dumpPlanet(FILE* out, const PLANET_DESC* planet)
{
	(*pSolarSysState->genFuncs->generateName)(pSolarSysState, planet);
	fmt::print(out, "- {:>37}  {}\n", uqm::gameState().sis().PlanetName,
			   planetTypeString(planet->data_index & ~PLANET_SHIELDED));
	dumpWorld(out, planet);
}

static void
dumpMoonCallback(const PLANET_DESC* moon, void* arg)
{
	FILE* out = ((DumpUniverseArg*)arg)->out;
	dumpMoon(out, moon);
}

void dumpMoon(FILE* out, const PLANET_DESC* moon)
{
	const char* typeStr;

	if (moon->data_index == HIERARCHY_STARBASE)
	{
		typeStr = "StarBase";
	}
	else if (moon->data_index == SA_MATRA)
	{
		typeStr = "Sa-Matra";
	}
	else if (moon->data_index == DESTROYED_STARBASE)
	{
		typeStr = "Destroyed StarBase";
	}
	else if (moon->data_index == PRECURSOR_STARBASE)
	{
		typeStr = "Precursor StarBase";
	}
	else
	{
		typeStr = planetTypeString(moon->data_index & ~PLANET_SHIELDED);
	}
	fmt::print(out, "  - Moon {:>30}  {}\n",
			   'a' + (char)(moon - &pSolarSysState->MoonDesc[0]), typeStr);

	dumpWorld(out, moon);
}

static void
dumpWorld(FILE* out, const PLANET_DESC* world)
{
	PLANET_INFO* info;

	if (world->data_index == HIERARCHY_STARBASE)
	{
		return;
	}

	if (world->data_index == SA_MATRA)
	{
		return;
	}

	if (world->data_index == DESTROYED_STARBASE)
	{
		return;
	}

	if (world->data_index == PRECURSOR_STARBASE)
	{
		return;
	}

	info = &pSolarSysState->SysInfo.PlanetInfo;
	fmt::print(out, "          AxialTilt:  {}\n", info->AxialTilt);
	fmt::print(out, "          Tectonics:  {}\n", info->Tectonics);
	fmt::print(out, "          Weather:    {}\n", info->Weather);
	fmt::print(out, "          Density:    {}\n", info->PlanetDensity);
	fmt::print(out, "          Radius:     {}\n", info->PlanetRadius);
	fmt::print(out, "          Gravity:    {}\n", info->SurfaceGravity);
	fmt::print(out, "          Temp:       {}\n", info->SurfaceTemperature);
	fmt::print(out, "          Day:        {}\n", info->RotationPeriod);
	fmt::print(out, "          Atmosphere: {}\n", info->AtmoDensity);
	fmt::print(out, "          LifeChance: {}\n", info->LifeChance);
	fmt::print(out, "          DistToSun:  {}\n", info->PlanetToSunDist);

	if (world->data_index & PLANET_SHIELDED)
	{
		// Slave-shielded planet
		return;
	}

	fmt::print(out, "          Bio: {:4}    Min: {:4}\n",
			   calculateBioValue(pSolarSysState, world),
			   calculateMineralValue(pSolarSysState, world));
}

void printWorld(const PLANET_DESC* world)
{
	PLANET_INFO* info;
	char buf[200] {};
	GFXPOINT universe = CurStarDescPtr()->star_pt;

	if (world->data_index == HIERARCHY_STARBASE
		|| world->data_index == SA_MATRA
		|| world->data_index == DESTROYED_STARBASE
		|| world->data_index == PRECURSOR_STARBASE)
	{
		return;
	}

	const uqm::FilePtr file {uqm::openFile("planetLog.txt", "a")};
	if (file == nullptr)
	{
		return;
	}
	FILE* const fp {file.get()};

	fmt::print(fp, "Coords:     {:03}.{:01} : {:03}.{:01}\n",
			   universe.x / 10, universe.x % 10,
			   universe.y / 10, universe.y % 10);

	GetClusterName(CurStarDescPtr(), buf);

	fmt::print(fp, "Star:       {}\n", buf);

	GetPlanetTitle({buf, sizeof(buf)});

	if (strcmp(buf, uqm::gameState().sis().PlanetName) != 0)
	{
		fmt::print(fp, "Planet:     {}\n", uqm::gameState().sis().PlanetName);
	}

	info = &pSolarSysState->SysInfo.PlanetInfo;
	fmt::print(fp, "World:      {}\n\n", buf);
	fmt::print(fp, "DistToSun:  {}\n", info->PlanetToSunDist);
	fmt::print(fp, "Atmosphere: {}\n", info->AtmoDensity);
	fmt::print(fp, "Temp:       {}\n", info->SurfaceTemperature);
	fmt::print(fp, "Weather:    {}\n", info->Weather);
	fmt::print(fp, "Tectonics:  {}\n\n", info->Tectonics);
	fmt::print(fp, "Density:    {}\n", info->PlanetDensity);
	fmt::print(fp, "Radius:     {}\n", info->PlanetRadius);
	fmt::print(fp, "Gravity:    {}\n", info->SurfaceGravity);
	fmt::print(fp, "Day:        {}\n", info->RotationPeriod);
	fmt::print(fp, "AxialTilt:  {}\n\n", info->AxialTilt);

	if (world->data_index & PLANET_SHIELDED)
	{ // Slave-shielded planet
		fmt::print(fp, "LifeChance: {}\n", info->LifeChance);
		fmt::print(fp, "____________________________________\n\n");
		return;
	}
	else
	{
		fmt::print(fp, "LifeChance: {}\n", info->LifeChance);
	}

	fmt::print(fp, "Bio: {:4}    Min: {:4}\n",
			   calculateBioValue(pSolarSysState, world),
			   calculateMineralValue(pSolarSysState, world));
	fmt::print(fp, "____________________________________\n\n");
}

uint16_t
calculateBioValue(const SOLARSYS_STATE* system, const PLANET_DESC* world)
{
	uint16_t result;
	uint16_t numBio;
	uint16_t i;

	assert(system->pOrbitalDesc == world);

	numBio = callGenerateForScanType(system, world, GenerateAll,
									 BIOLOGICAL_SCAN, nullptr);

	result = 0;
	for (i = 0; i < numBio; i++)
	{
		NODE_INFO info;
		callGenerateForScanType(system, world, i, BIOLOGICAL_SCAN, &info);
		result += BioCreditValueDefault * uqm::lowNibble(CreatureData[info.type].ValueAndHitPoints);
	}
	return result;
}

void generateBioIndex(const SOLARSYS_STATE* system, const PLANET_DESC* world,
					  uint16_t bio[])
{
	uint16_t numBio;
	uint16_t i;

	assert(system->pOrbitalDesc == world);

	numBio = callGenerateForScanType(system, world, GenerateAll,
									 BIOLOGICAL_SCAN, nullptr);

	for (i = 0; i < NumCreatureTypes + NumSpecialCreatureTypes; i++)
	{
		bio[i] = 0;
	}

	for (i = 0; i < numBio; i++)
	{
		NODE_INFO info;
		callGenerateForScanType(system, world, i, BIOLOGICAL_SCAN, &info);
		bio[info.type]++;
	}
}

uint16_t
calculateMineralValue(const SOLARSYS_STATE* system, const PLANET_DESC* world)
{
	uint16_t result;
	uint16_t numDeposits;
	uint16_t i;

	assert(system->pOrbitalDesc == world);

	numDeposits = callGenerateForScanType(system, world, GenerateAll,
										  MINERAL_SCAN, nullptr);

	result = 0;
	for (i = 0; i < numDeposits; i++)
	{
		NODE_INFO info;
		callGenerateForScanType(system, world, i, MINERAL_SCAN, &info);
		result += uqm::highByte(info.density) * uqm::gameState().game().ElementWorth[ElementCategory(info.type)];
	}
	return result;
}

void generateMineralIndex(const SOLARSYS_STATE* system, const PLANET_DESC* world,
						  uint16_t minerals[])
{
	uint16_t numDeposits;
	uint16_t i;

	assert(system->pOrbitalDesc == world);

	numDeposits = callGenerateForScanType(system, world, GenerateAll,
										  MINERAL_SCAN, nullptr);

	for (i = 0; i < NUM_ELEMENT_CATEGORIES; i++)
	{
		minerals[i] = 0;
	}

	for (i = 0; i < numDeposits; i++)
	{
		NODE_INFO info;
		callGenerateForScanType(system, world, i, MINERAL_SCAN, &info);
		minerals[ElementCategory(info.type)] += uqm::highByte(info.density);
	}
}

////////////////////////////////////////////////////////////////////////////

struct TallyResourcesArg
{
	FILE* out;
	uint16_t mineralCount;
	uint16_t bioCount;
};

// Must be called from the Starcon2Main thread.
void tallyResources(FILE* out)
{
	TallyResourcesArg tallyResourcesArg;
	UniverseRecurseArg universeRecurseArg;

	tallyResourcesArg.out = out;

	universeRecurseArg.systemFuncPre = tallySystemPreCallback;
	universeRecurseArg.systemFuncPost = tallySystemPostCallback;
	universeRecurseArg.planetFuncPre = tallyPlanetCallback;
	universeRecurseArg.planetFuncPost = nullptr;
	universeRecurseArg.moonFunc = tallyMoonCallback;
	universeRecurseArg.arg = (void*)&tallyResourcesArg;

	UniverseRecurse(&universeRecurseArg);
}

static constexpr uqgsl::czstring ResourceTallyFile {"ResourceTally"};

// Must be called from the Starcon2Main thread.
void tallyResourcesToFile(void)
{
	const uqm::FilePtr out {uqm::openFile(ResourceTallyFile, "w")};
	if (out == nullptr)
	{
		fmt::print(stderr, "Error: Could not open file '{}' for "
						   "writing: {}\n",
				   ResourceTallyFile, uqm::strerror(errno));
		return;
	}

	tallyResources(out.get());

	fmt::print(stdout, "*** Resource tally complete. The game may be in an "
					   "undefined state.\n");
	// Data generation may have changed the game state,
	// in particular for special planet generation.
}

static void
tallySystemPreCallback(const STAR_DESC* star, const SOLARSYS_STATE* system,
					   void* arg)
{
	TallyResourcesArg* tallyResourcesArg = (TallyResourcesArg*)arg;
	tallyResourcesArg->mineralCount = 0;
	tallyResourcesArg->bioCount = 0;

	(void)star;	  /* satisfy compiler */
	(void)system; /* satisfy compiler */
}

static void
tallySystemPostCallback(const STAR_DESC* star, const SOLARSYS_STATE* system,
						void* arg)
{
	char name[256];
	TallyResourcesArg* tallyResourcesArg = (TallyResourcesArg*)arg;
	FILE* out = tallyResourcesArg->out;

	GetClusterName(star, name);
	fmt::print(out, "{}\t{}\t{}\n", name, tallyResourcesArg->mineralCount,
			   tallyResourcesArg->bioCount);

	(void)star;	  /* satisfy compiler */
	(void)system; /* satisfy compiler */
}

static void
tallyPlanetCallback(const PLANET_DESC* planet, void* arg)
{
	tallyResourcesWorld((TallyResourcesArg*)arg, planet);
}

static void
tallyMoonCallback(const PLANET_DESC* moon, void* arg)
{
	tallyResourcesWorld((TallyResourcesArg*)arg, moon);
}

static void
tallyResourcesWorld(TallyResourcesArg* arg, const PLANET_DESC* world)
{
	if (world->data_index == HIERARCHY_STARBASE)
	{
		return;
	}

	if (world->data_index == SA_MATRA)
	{
		return;
	}

	if (world->data_index == DESTROYED_STARBASE)
	{
		return;
	}

	if (world->data_index == PRECURSOR_STARBASE)
	{
		return;
	}

	arg->bioCount += calculateBioValue(pSolarSysState, world),
		arg->mineralCount += calculateMineralValue(pSolarSysState, world);
}

////////////////////////////////////////////////////////////////////////////

void forAllPlanetTypes(void (*callback)(int, const PlanetFrame*, void*),
					   void* arg)
{
	int i;

	for (i = 0; i < NUMBER_OF_PLANET_TYPES; i++)
	{
		callback(i, &PlanetsArray[i], arg);
	}
}

typedef struct
{
	FILE* out;
} DumpPlanetTypesArg;

void dumpPlanetTypes(FILE* out)
{
	DumpPlanetTypesArg dumpPlanetTypesArg;
	dumpPlanetTypesArg.out = out;

	forAllPlanetTypes(dumpPlanetTypeCallback, (void*)&dumpPlanetTypesArg);
}

static void
dumpPlanetTypeCallback(int index, const PlanetFrame* planetType, void* arg)
{
	DumpPlanetTypesArg* dumpPlanetTypesArg = (DumpPlanetTypesArg*)arg;

	dumpPlanetType(dumpPlanetTypesArg->out, index, planetType);
}

void dumpPlanetType(FILE* out, int index, const PlanetFrame* planetType)
{
	int i;
	fmt::print(out,
			   "{}\n"
			   "\tType: {}\n"
			   "\tColor: {}\n"
			   "\tSurface generation algoritm: {}\n"
			   "\tTectonics: {}\n"
			   "\tAtmosphere: {}\n"
			   "\tDensity: {}\n"
			   "\tElements:\n",
			   planetTypeString(index),
			   worldSizeString(Plansize(planetType->Type)),
			   bodyColorString(Plancolor(planetType->Type)),
			   worldGenAlgoString(Planalgo(planetType->Type)),
			   tectonicsString(planetType->BaseTectonics),
			   atmosphereString(uqm::highNibble(planetType->AtmoAndDensity)),
			   densityString(uqm::lowNibble(planetType->AtmoAndDensity)));
	for (i = 0; i < NumUsefulElements; i++)
	{
		const ELEMENT_ENTRY* entry;
		entry = &planetType->UsefulElements[i];
		if (entry->Density == 0)
		{
			continue;
		}
		fmt::print(out, "\t\t0 to {} {}-quality (+{}) deposits of {} ({})\n",
				   DepositQuantity(entry->Density),
				   depositQualityString(DepositQuality(entry->Density)),
				   DepositQuality(entry->Density) * 5,
				   GameString(ELEMENTS_STRING_BASE + entry->ElementType),
				   GameString(CARGO_STRING_BASE + 2 + ElementCategory(entry->ElementType)));
	}
	fmt::print(out, "\n");
}

const char*
planetTypeString(int typeIndex)
{
	static char typeStr[40];

	if (typeIndex >= FIRST_GAS_GIANT)
	{
		// "Gas Giant"
		fmt::format_to_sz_n(typeStr, "{}",
							GameString(SCAN_STRING_BASE + 4 + 51));
	}
	else
	{
		// "<type> World" (eg. "Water World")
		fmt::format_to_sz_n(typeStr, "{} {}",
							GameString(SCAN_STRING_BASE + 4 + typeIndex),
							GameString(SCAN_STRING_BASE + 4 + 50));
	}
	return typeStr;
}

// size is what you get from Plansize (planetFrame.Type)
const char*
worldSizeString(uint8_t size)
{
	switch (size)
	{
		case SMALL_ROCKY_WORLD:
			return "small rocky world";
		case LARGE_ROCKY_WORLD:
			return "large rocky world";
		case GAS_GIANT:
			return "gas giant";
		default:
			// Should not happen
			return "???";
	}
}

// algo is what you get from Planalgo (planetFrame.Type)
const char*
worldGenAlgoString(uint8_t algo)
{
	switch (algo)
	{
		case TopoAlgo:
			return "TOPO_ALGO";
		case CrateredAlgo:
			return "CRATERED_ALGO";
		case GasGiantAlgo:
			return "GAS_GIANT_ALGO";
		default:
			// Should not happen
			return "???";
	}
}

// tectonics is what you get from planetFrame.BaseTechtonics
// not reentrant
const char*
tectonicsString(uint8_t tectonics)
{
	static char buf[sizeof "-127"];
	switch (tectonics)
	{
		case NoTectonics:
			return "none";
		case LowTectonics:
			return "low";
		case MedTectonics:
			return "medium";
		case HighTectonics:
			return "high";
		case SuperTectonics:
			return "super";
		default:
			fmt::format_to_sz_n(buf, "{}", tectonics);
			return buf;
	}
}

// atmosphere is what you get from HINIBBLE (planetFrame.AtmoAndDensity)
const char*
atmosphereString(uint8_t atmosphere)
{
	switch (atmosphere)
	{
		case LIGHT:
			return "thin";
		case MEDIUM:
			return "normal";
		case HEAVY:
			return "thick";
		default:
			return "super thick";
	}
}

// density is what you get from LONIBBLE (planetFrame.AtmoAndDensity)
const char*
densityString(uint8_t density)
{
	switch (density)
	{
		case GAS_DENSITY:
			return "gaseous";
		case LIGHT_DENSITY:
			return "light";
		case LOW_DENSITY:
			return "low";
		case NORMAL_DENSITY:
			return "normal";
		case HIGH_DENSITY:
			return "high";
		case SUPER_DENSITY:
			return "super high";
		default:
			// Should not happen
			return "???";
	}
}

// quality is what you get from DepositQuality (elementEntry.Density)
const char*
depositQualityString(uint8_t quality)
{
	switch (quality)
	{
		case LIGHT:
			return "low";
		case MEDIUM:
			return "medium";
		case HEAVY:
			return "high";
		default:
			// Should not happen
			return "???";
	}
}

////////////////////////////////////////////////////////////////////////////

void resetCrewBattle(void)
{
	STARSHIP* StarShipPtr;
	uint16_t delta;
	GFXCONTEXT OldContext;

	if (!(uqm::currentActivity() & IN_BATTLE) || (inHQSpace()))
	{
		return;
	}

	StarShipPtr = findPlayerShip(RpgPlayerNum);
	if (StarShipPtr == nullptr || StarShipPtr->RaceDescPtr == nullptr)
	{
		return;
	}

	delta = StarShipPtr->RaceDescPtr->ship_info.max_crew - StarShipPtr->RaceDescPtr->ship_info.crew_level;

	OldContext = SetContext(StatusContext);
	DeltaCrew((ELEMENT*)StarShipPtr->hShip, delta);
	SetContext(OldContext);
}

////////////////////////////////////////////////////////////////////////////

// This function should help in making sure that gamestr.h matches
// gamestrings.txt.
void dumpStrings(FILE* out)
{
	struct
	{
		const char* name;
		size_t base;
		size_t count;
	} categories[] = {
		{"0",					  0,					  0					},
		{"STAR_STRING_BASE",		 STAR_STRING_BASE,	   StarStringCount	  },
		{"DEVICE_STRING_BASE",	   DEVICE_STRING_BASE,	   DeviceStringCount	},
		{"CARGO_STRING_BASE",	  CARGO_STRING_BASE,		 CargoStringCount	 },
		{"ELEMENTS_STRING_BASE",	 ELEMENTS_STRING_BASE,   ElementsStringCount  },
		{"SCAN_STRING_BASE",		 SCAN_STRING_BASE,	   ScanStringCount	  },
		{"STAR_NUMBER_BASE",		 STAR_NUMBER_BASE,	   StarNumberCount	  },
		{"PLANET_NUMBER_BASE",	   PLANET_NUMBER_BASE,	   PlanetNumberCount	},
		{"MONTHS_STRING_BASE",	   MONTHS_STRING_BASE,	   MonthsStringCount	},
		{"FEEDBACK_STRING_BASE",	 FEEDBACK_STRING_BASE,   FeedbackStringCount  },
		{"STARBASE_STRING_BASE",	 STARBASE_STRING_BASE,   StarbaseStringCount  },
		{"ENCOUNTER_STRING_BASE",  ENCOUNTER_STRING_BASE,	 EncounterStringCount },
		{"NAVIGATION_STRING_BASE", NAVIGATION_STRING_BASE, NavigationStringCount},
		{"NAMING_STRING_BASE",	   NAMING_STRING_BASE,	   NamingStringCount	},
		{"MELEE_STRING_BASE",	  MELEE_STRING_BASE,		 MeleeStringCount	 },
		{"SAVEGAME_STRING_BASE",	 SAVEGAME_STRING_BASE,   SavegameStringCount  },
		{"OPTION_STRING_BASE",	   OPTION_STRING_BASE,	   OptionStringCount	},
		{"QUITMENU_STRING_BASE",	 QUITMENU_STRING_BASE,   QuitmenuStringCount  },
		{"STATUS_STRING_BASE",	   STATUS_STRING_BASE,	   StatusStringCount	},
		{"FLAGSHIP_STRING_BASE",	 FLAGSHIP_STRING_BASE,   FlagshipStringCount  },
		{"ORBITSCAN_STRING_BASE",  ORBITSCAN_STRING_BASE,	 OrbitscanStringCount },
		{"MAINMENU_STRING_BASE",	 MAINMENU_STRING_BASE,   MainmenuStringCount  },
		{"NETMELEE_STRING_BASE",	 NETMELEE_STRING_BASE,   NetmeleeStringCount  },
		{"BIOLOGICAL_STRING_BASE", BIOLOGICAL_STRING_BASE, BiologicalStringCount},
		{"GAMESTR_COUNT",		  GAMESTR_COUNT,			 (size_t)-1		   }
	};
	size_t numCategories = std::size(categories);
	size_t numStrings = GameStrings->size();
	size_t stringI;
	size_t categoryI = 0;

	// Start with a sanity check to see if gamestr.h has been changed but
	// not this file.
	for (categoryI = 0; categoryI < numCategories - 1; categoryI++)
	{
		if (categories[categoryI].base + categories[categoryI].count != categories[categoryI + 1].base)
		{
			fmt::print(stderr, "Error: String category list in dumpStrings() is "
							   "not up to date.\n");
			return;
		}
	}

	if (GAMESTR_COUNT != numStrings)
	{
		fmt::print(stderr, "Warning: GAMESTR_COUNT is {}, but GameStrings "
						   "contains {} strings.\n",
				   GAMESTR_COUNT, numStrings);
	}

	categoryI = 0;
	for (stringI = 0; stringI < numStrings; stringI++)
	{
		while (categoryI < numCategories && stringI >= categories[categoryI + 1].base)
		{
			categoryI++;
		}
		fmt::print(out, "[ {} + {} ]  {}\n", categories[categoryI].name,
				   stringI - categories[categoryI].base, GameString((uint16_t)stringI));
	}
}

////////////////////////////////////////////////////////////////////////////


static Color
hsvaToRgba(double hue, double sat, double val, uint8_t alpha)
{
	unsigned int hi = (int)(hue / 60.0);
	double f = (hue / 60.0) - ((int)(hue / 60.0));
	double p = val * (1.0 - sat);
	double q = val * (1.0 - f * sat);
	double t = val * (1.0 - (1.0 - f * sat));

	// Convert p, q, t, and v from [0..1] to [0..255]
	uint8_t pb = (uint8_t)(p * 255.0 + 0.5);
	uint8_t qb = (uint8_t)(q * 255.0 + 0.5);
	uint8_t tb = (uint8_t)(t * 255.0 + 0.5);
	uint8_t vb = (uint8_t)(val * 255.0 + 0.5);

	assert(hue >= 0.0 && hue < 360.0);
	assert(sat >= 0 && sat <= 1.0);
	assert(val >= 0 && val <= 1.0);
	/*fmt::print(stderr, "hsva = (%.1f, %.2f, %.2f, %.2d)\n",
			hue, sat, val, alpha);*/

	assert(hi < 6);
	switch (hi)
	{
		case 0:
			return BuildColorRgba(vb, tb, pb, alpha);
		case 1:
			return BuildColorRgba(qb, vb, pb, alpha);
		case 2:
			return BuildColorRgba(pb, vb, tb, alpha);
		case 3:
			return BuildColorRgba(pb, qb, vb, alpha);
		case 4:
			return BuildColorRgba(tb, pb, vb, alpha);
		case 5:
			return BuildColorRgba(vb, pb, qb, alpha);
	}

	// Should not happen.
	return BuildColorRgba(0, 0, 0, alpha);
}

// Returns true iff this context has a visible FRAME.
static bool
isContextVisible(GFXCONTEXT context)
{
	FRAME contextFrame;

	// Save the original context.
	GFXCONTEXT oldContext = SetContext(context);

	// Get the frame of the specified context.
	contextFrame = GetContextFGFrame();

	// Restore the original context.
	SetContext(oldContext);

	return contextFrame == Screen;
}

static size_t
countVisibleContexts(void)
{
	size_t contextCount;
	GFXCONTEXT context;

	contextCount = 0;
	for (context = GetFirstContext(); context != nullptr;
		 context = GetNextContext(context))
	{
		if (!isContextVisible(context))
		{
			continue;
		}

		contextCount++;
	}

	return contextCount;
}

static void
drawContext(GFXCONTEXT context, double hue /* no pun intended */)
{
	//FRAME drawFrame; unused
	GFXCONTEXT oldContext;
	FONT oldFont;
	DrawMode oldMode;
	Color oldFgCol;
	Color rectCol;
	Color lineCol;
	Color textCol;
	//bool haveClippingRect; unused
	GFXRECT rect;
	LINE line;
	TEXT text;
	GFXPOINT p1, p2, p3, p4;

	//drawFrame = GetContextFGFrame (); unused
	rectCol = hsvaToRgba(hue, 1.0, 0.5, 100);
	lineCol = hsvaToRgba(hue, 1.0, 1.0, 90);
	textCol = lineCol;

	// Save the original context.
	oldContext = SetContext(context);

	// Get the clipping rectangle of the specified context.
	GetContextClipRect(&rect);

	// Switch back the old context; we're going to draw in it.
	(void)SetContext(oldContext);

	p1 = rect.corner;
	p2.x = rect.corner.x + rect.extent.width - 1;
	p2.y = rect.corner.y;
	p3.x = rect.corner.x;
	p3.y = rect.corner.y + rect.extent.height - 1;
	p4.x = rect.corner.x + rect.extent.width - 1;
	p4.y = rect.corner.y + rect.extent.height - 1;

	oldFgCol = SetContextForeGroundColor(rectCol);
	DrawFilledRectangle(&rect);

	SetContextForeGroundColor(lineCol);
	line.first = p1;
	line.second = p2;
	DrawLine(&line, 1);
	line.first = p2;
	line.second = p4;
	DrawLine(&line, 1);
	line.first = p1;
	line.second = p3;
	DrawLine(&line, 1);
	line.first = p3;
	line.second = p4;
	DrawLine(&line, 1);
	line.first = p1;
	line.second = p4;
	DrawLine(&line, 1);
	line.first = p2;
	line.second = p3;
	DrawLine(&line, 1);
	// Gimme C'99! So I can do:
	//     DrawLine ((LINE) { .first = p1, .second = p2 })

	oldFont = SetContextFont(TinyFont);
	SetContextForeGroundColor(textCol);
	// Text prim does not yet support alpha via Color.a
	oldMode = SetContextDrawMode(MAKE_DRAW_MODE(DRAW_ALPHA, textCol.a));
	text.baseline.x = (p1.x + (p2.x + 1)) / 2;
	text.baseline.y = p1.y + 8;
	text.pStr = GetContextName(context);
	text.align = ALIGN_CENTER;
	text.CharCount = (uint16_t)~0;
	font_DrawText(&text);
	(void)SetContextDrawMode(oldMode);

	(void)SetContextForeGroundColor(oldFgCol);
	(void)SetContextFont(oldFont);
}

static void
describeContext(FILE* out, const GFXCONTEXT context)
{
	GFXRECT rect;
	GFXCONTEXT oldContext = SetContext(context);

	GetContextClipRect(&rect);
	fmt::print(out, "Context '{}':\n"
					"\tClipRect = ({}, {})-({}, {})  ({} x {})\n",
			   GetContextName(context),
			   rect.corner.x, rect.corner.y,
			   rect.corner.x + rect.extent.width,
			   rect.corner.y + rect.extent.height,
			   rect.extent.width, rect.extent.height);

	SetContext(oldContext);
}


typedef struct wait_state
{
	// standard state required by DoInput
	bool (*InputFunc)(struct wait_state* self);
} WAIT_STATE;


// Maybe move to elsewhere, where it can be reused?
static bool
waitForKey(struct wait_state* self)
{
	if (PulsedInputState.menu[KEY_MENU_SELECT] || PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		return false;
	}

	SleepThread(c_gameSecond / 20);

	(void)self;
	return true;
}

// Maybe move to elsewhere, where it can be reused?
static FRAME
getScreen(void)
{
	GFXCONTEXT oldContext = SetContext(ScreenContext);
	FRAME savedFrame;
	GFXRECT screenRect;

	screenRect.corner.x = 0;
	screenRect.corner.y = 0;
	screenRect.extent.width = CanvasWidth;
	screenRect.extent.height = CanvasHeight;
	savedFrame = CaptureDrawable(LoadDisplayPixmap(&screenRect, (FRAME)0));

	(void)SetContext(oldContext);
	return savedFrame;
}

static void
putScreen(FRAME savedFrame)
{
	STAMP stamp;

	GFXCONTEXT oldContext = SetContext(ScreenContext);

	stamp.origin.x = 0;
	stamp.origin.y = 0;
	stamp.frame = savedFrame;
	DrawStamp(&stamp);

	(void)SetContext(oldContext);
}

// Show the contexts on the screen.
// Must be called from the main thread.
void debugContexts(void)
{
	static uqstl::atomic<bool> inDebugContexts {false};
	// Prevent this function from being called from within itself.

	GFXCONTEXT orgContext;
	GFXCONTEXT debugDrawContext;
	// We're going to use this context to draw in.
	FRAME debugDrawFrame;
	double hueIncrement;
	size_t visibleContextI;
	GFXCONTEXT context;
	size_t contextCount;
	FRAME savedScreen;

	// Prevent this function from being called from within itself.
	if (inDebugContexts.exchange(true))
	{
		return;
	}
	const auto resetGuard {uqgsl::finally([] {
		inDebugContexts = false;
	})};

	contextCount = countVisibleContexts();
	if (contextCount == 0)
	{
		return;
	}

	savedScreen = getScreen();
	FlushGraphics();
	// Make sure that the screen has actually been captured,
	// before we use the frame.

	// Create a new frame to draw on.
	debugDrawContext = CreateContext("debugDrawContext");
	// New work frame is a copy of the original.
	debugDrawFrame = CaptureDrawable(CloneFrame(savedScreen));
	orgContext = SetContext(debugDrawContext);
	SetContextFGFrame(debugDrawFrame);

	hueIncrement = 360.0 / contextCount;

	visibleContextI = 0;
	for (context = GetFirstContext(); context != nullptr;
		 context = GetNextContext(context))
	{
		if (context == debugDrawContext)
		{
			// Skip our own context.
			continue;
		}

		if (isContextVisible(context))
		{
			// Only draw the visible contexts.
			drawContext(context, visibleContextI * hueIncrement);
			visibleContextI++;
		}

		describeContext(stderr, context);
	}

	// Blit the final debugging frame to the screen.
	putScreen(debugDrawFrame);

	// Wait for a key:
	{
		WAIT_STATE state;
		state.InputFunc = waitForKey;
		DoInput(&state, true);
	}

	SetContext(orgContext);

	// Destroy the debugging frame and context.
	DestroyContext(debugDrawContext);
	// This does nothing with the drawable set with
	// SetContextFGFrame().
	DestroyDrawable(ReleaseDrawable(debugDrawFrame));

	putScreen(savedScreen);

	DestroyDrawable(ReleaseDrawable(savedScreen));
}

#endif /* DEBUG */
