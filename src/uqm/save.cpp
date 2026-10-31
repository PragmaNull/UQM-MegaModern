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

#include <assert.h>

#include "save.h"

#include "build.h"
#include "controls.h"
#include "starmap.h"
#include "encount.h"
#include "gamestr.h"
#include "globdata.h"
#include "options.h"
#include "races.h"
#include "shipcont.h"
#include "setup.h"
#include "state.h"
#include "grpintrn.h"
#include "util.h"
#include "hyper.h"
// for SaveSisHyperState()
#include "planets/planets.h"
// for SaveSolarSysLocation() and tests
#include "libs/inplib.h"
#include "core/log/log.h"
#include "colors.h"
#include "libs/file/textfile.h"

#include <yyjson.h>

#pragma clang optimize off

namespace
{

template <typename T>
void addInt(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, T value)
{
	if constexpr (uqstl::is_enum_v<T>)
	{
		addInt(doc, obj, key, static_cast<uqstl::underlying_type_t<T>>(value));
	}
	else if constexpr (uqstl::is_signed_v<T>)
	{
		yyjson_mut_obj_add_sint(doc, obj, key, value);
	}
	else
	{
		yyjson_mut_obj_add_uint(doc, obj, key, value);
	}
}

template <typename T>
void addArray(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, uqstl::span<const T> values)
{
	yyjson_mut_val* array {yyjson_mut_obj_add_arr(doc, obj, key)};
	for (const T value : values)
	{
		if constexpr (uqstl::is_signed_v<T>)
		{
			yyjson_mut_arr_add_sint(doc, array, value);
		}
		else
		{
			yyjson_mut_arr_add_uint(doc, array, value);
		}
	}
}

template <typename Point>
void addPoint(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, Point x, Point y)
{
	yyjson_mut_val* point {yyjson_mut_obj_add_obj(doc, obj, key)};
	addInt(doc, point, "x", x);
	addInt(doc, point, "y", y);
}

void addString(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, uqstl::span<const char> buffer)
{
	yyjson_mut_obj_add_strncpy(doc, obj, key, buffer.data(), strnlen(buffer.data(), buffer.size()));
}

void addShipFragment(yyjson_mut_doc* doc, yyjson_mut_val* obj, const SHIP_FRAGMENT& frag)
{
	addInt(doc, obj, "captains_name_index", frag.captains_name_index);
	addInt(doc, obj, "race_id", frag.race_id);
	addInt(doc, obj, "index", frag.index);
	addInt(doc, obj, "crew_level", frag.crew_level);
	addInt(doc, obj, "max_crew", frag.max_crew);
	addInt(doc, obj, "energy_level", frag.energy_level);
	addInt(doc, obj, "max_energy", frag.max_energy);
}

void addIpGroup(yyjson_mut_doc* doc, yyjson_mut_val* obj, const IP_GROUP& group)
{
	addInt(doc, obj, "group_counter", group.group_counter);
	addInt(doc, obj, "race_id", group.race_id);
	addInt(doc, obj, "sys_loc", group.sys_loc);
	addInt(doc, obj, "task", group.task);
	addInt(doc, obj, "in_system", group.in_system);
	addInt(doc, obj, "dest_loc", group.dest_loc);
	addInt(doc, obj, "orbit_pos", group.orbit_pos);
	addInt(doc, obj, "group_id", group.group_id);
	addPoint(doc, obj, "loc", group.loc.x, group.loc.y);
}

void SaveShipQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, const char* key, QUEUE* pQueue)
{
	uint16_t num_links {CountLinks(pQueue)};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, key)};
	HSHIPFRAG hStarShip {GetHeadLink(pQueue)};
	while (num_links--)
	{
		SHIP_FRAGMENT* FragPtr {LockShipFrag(pQueue, hStarShip)};
		const HSHIPFRAG hNextShip {_GetSuccLink(FragPtr)};
		addShipFragment(doc, yyjson_mut_arr_add_obj(doc, list), *FragPtr);
		UnlockShipFrag(pQueue, hStarShip);
		hStarShip = hNextShip;
	}
}

void SaveRaceQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, QUEUE* pQueue)
{
	uint16_t num_links {CountLinks(pQueue)};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "raceQueue")};
	HFLEETINFO hFleet {GetHeadLink(pQueue)};
	while (num_links--)
	{
		FLEET_INFO* FleetPtr {LockFleetInfo(pQueue, hFleet)};
		const HFLEETINFO hNextFleet {_GetSuccLink(FleetPtr)};

		yyjson_mut_val* fleet {yyjson_mut_arr_add_obj(doc, list)};
		addInt(doc, fleet, "Index", GetIndexFromStarShip(pQueue, hFleet));
		addInt(doc, fleet, "allied_state", FleetPtr->allied_state);
		addInt(doc, fleet, "days_left", FleetPtr->days_left);
		addInt(doc, fleet, "growth_fract", FleetPtr->growth_fract);
		addInt(doc, fleet, "crew_level", FleetPtr->crew_level);
		addInt(doc, fleet, "max_crew", FleetPtr->max_crew);
		addInt(doc, fleet, "growth", FleetPtr->growth);
		addInt(doc, fleet, "max_energy", FleetPtr->max_energy);
		addPoint(doc, fleet, "loc", FleetPtr->loc.x, FleetPtr->loc.y);
		addInt(doc, fleet, "actual_strength", FleetPtr->actual_strength);
		addInt(doc, fleet, "known_strength", FleetPtr->known_strength);
		addPoint(doc, fleet, "known_loc", FleetPtr->known_loc.x, FleetPtr->known_loc.y);
		addInt(doc, fleet, "growth_err_term", FleetPtr->growth_err_term);
		addInt(doc, fleet, "func_index", FleetPtr->func_index);
		addPoint(doc, fleet, "dest_loc", FleetPtr->dest_loc.x, FleetPtr->dest_loc.y);

		UnlockFleetInfo(pQueue, hFleet);
		hFleet = hNextFleet;
	}
}

void SaveGroupQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, QUEUE* pQueue)
{
	if (CountLinks(pQueue) == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "ipGroupQueue")};
	HIPGROUP hNextGroup {};
	for (HIPGROUP hGroup {GetHeadLink(pQueue)}; hGroup; hGroup = hNextGroup)
	{
		IP_GROUP* GroupPtr {LockIpGroup(pQueue, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);
		addIpGroup(doc, yyjson_mut_arr_add_obj(doc, list), *GroupPtr);
		UnlockIpGroup(pQueue, hGroup);
	}
}

void SaveEncounters(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	uint16_t num_links {CountLinks(&GLOBAL(encounter_q))};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "encounters")};
	HENCOUNTER hEncounter {GetHeadLink(&GLOBAL(encounter_q))};
	while (num_links--)
	{
		ENCOUNTER* EncounterPtr {};
		LockEncounter(hEncounter, &EncounterPtr);
		const HENCOUNTER hNextEncounter {GetSuccEncounter(EncounterPtr)};

		yyjson_mut_val* encounter {yyjson_mut_arr_add_obj(doc, list)};
		addInt(doc, encounter, "transition_state", EncounterPtr->transition_state);
		addPoint(doc, encounter, "origin", EncounterPtr->origin.x, EncounterPtr->origin.y);
		addInt(doc, encounter, "radius", EncounterPtr->radius);
		addPoint(doc, encounter, "loc_pt", EncounterPtr->loc_pt.x, EncounterPtr->loc_pt.y);
		addInt(doc, encounter, "race_id", EncounterPtr->race_id);
		addInt(doc, encounter, "flags", EncounterPtr->flags);

		yyjson_mut_val* ships {yyjson_mut_obj_add_arr(doc, encounter, "ships")};
		const uint16_t numShips {uqstl::min<uint16_t>(EncounterPtr->num_ships, MAX_HYPER_SHIPS)};
		for (uint16_t i {}; i < numShips; ++i)
		{
			const BRIEF_SHIP_INFO& ShipInfo {EncounterPtr->ShipList[i]};
			yyjson_mut_val* ship {yyjson_mut_arr_add_obj(doc, ships)};
			addInt(doc, ship, "race_id", ShipInfo.race_id);
			addInt(doc, ship, "crew_level", ShipInfo.crew_level);
			addInt(doc, ship, "max_crew", ShipInfo.max_crew);
			addInt(doc, ship, "max_energy", ShipInfo.max_energy);
		}

		addPoint(doc, encounter, "log", RES_DESCALE(EncounterPtr->log_x), RES_DESCALE(EncounterPtr->log_y));

		UnlockEncounter(hEncounter);
		hEncounter = hNextEncounter;
	}
}

void SaveEvents(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	uint16_t num_links {CountLinks(&uqm::gameState().clock().events())};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "events")};
	HEVENT hEvent {GetHeadLink(&uqm::gameState().clock().events())};
	while (num_links--)
	{
		EVENT* EventPtr {};
		LockEvent(hEvent, &EventPtr);
		const HEVENT hNextEvent {GetSuccEvent(EventPtr)};

		yyjson_mut_val* event {yyjson_mut_arr_add_obj(doc, list)};
		addInt(doc, event, "day_index", EventPtr->day_index);
		addInt(doc, event, "month_index", EventPtr->month_index);
		addInt(doc, event, "year_index", EventPtr->year_index);
		addInt(doc, event, "func_index", EventPtr->func_index);

		UnlockEvent(hEvent);
		hEvent = hNextEvent;
	}
}

void SaveClockState(yyjson_mut_doc* doc, yyjson_mut_val* obj, const uqm::GameClock& clock)
{
	yyjson_mut_val* clockObj {yyjson_mut_obj_add_obj(doc, obj, "GameClock")};
	addInt(doc, clockObj, "day_index", clock.day());
	addInt(doc, clockObj, "month_index", clock.month());
	addInt(doc, clockObj, "year_index", clock.year());
	addInt(doc, clockObj, "tick_count", clock.tickCount());
	addInt(doc, clockObj, "day_in_ticks", clock.dayInTicks());
}

void SaveGameStateFlags(yyjson_mut_doc* doc, yyjson_mut_val* root, const uqm::GameState& gameState)
{
	yyjson_mut_val* flags {yyjson_mut_obj_add_obj(doc, root, "gameState")};
	for (size_t index {0}; index < uqm::c_gameFlagCount; ++index)
	{
		const uqm::GameFlagInfo& info {uqm::c_gameFlags[index]};
		const uint32_t value {gameState.flag(static_cast<uqm::GameFlag>(index))};
		const uint32_t maxValue {info.bits >= 32 ? UINT32_MAX : (uint32_t {1} << info.bits) - 1};
		if (value > maxValue)
		{
			uqm::log::error("Game state '{}' ({}) does not fit in {} bits.", info.name, value, info.bits);
		}
		// The names are string literals, so the document can keep them.
		yyjson_mut_obj_add_uint(doc, flags, info.name.data(), value);
	}
}

void SaveGameState(yyjson_mut_doc* doc, yyjson_mut_val* root, uqm::GameState& gameState)
{
	const GAME_STATE& GS {gameState.game()};
	const uint8_t res_scale {static_cast<uint8_t>(uqm::lowByte(uqm::currentActivity()) != IN_INTERPLANETARY ? RESOLUTION_FACTOR : 0)};

	yyjson_mut_val* state {yyjson_mut_obj_add_obj(doc, root, "globalState")};
	addInt(doc, state, "glob_flags", uqm::globalFlags());
	addInt(doc, state, "CrewCost", GS.CrewCost);
	addInt(doc, state, "FuelCost", GS.FuelCost);
	addArray<uint8_t>(doc, state, "ModuleCost", GS.ModuleCost);
	addArray<uint8_t>(doc, state, "ElementWorth", GS.ElementWorth);
	addInt(doc, state, "CurrentActivity", uqm::currentActivity());
	SaveClockState(doc, state, gameState.clock());
	const uqm::FlagshipState& flagship {gameState.flagship()};
	addPoint(doc, state, "autopilot", flagship.autopilot.target().x, flagship.autopilot.target().y);
	addPoint(doc, state, "ip_location", flagship.ipLocation.x, flagship.ipLocation.y);
	addPoint(doc, state, "ShipOrigin", RES_DESCALE(flagship.shipStamp.origin.x), RES_DESCALE(flagship.shipStamp.origin.y));
	addInt(doc, state, "ShipFacing", flagship.shipFacing);
	addInt(doc, state, "ip_planet", flagship.ipPlanet);
	addInt(doc, state, "in_orbit", flagship.inOrbit);

	const VELOCITY_DESC& velocity {flagship.velocity};
	yyjson_mut_val* velocityObj {yyjson_mut_obj_add_obj(doc, state, "velocity")};
	addInt(doc, velocityObj, "TravelAngle", velocity.TravelAngle);
	addPoint(doc, velocityObj, "vector", velocity.vector.width >> res_scale, velocity.vector.height >> res_scale);
	addPoint(doc, velocityObj, "fract", velocity.fract.width >> res_scale, velocity.fract.height >> res_scale);
	addPoint(doc, velocityObj, "error", velocity.error.width >> res_scale, velocity.error.height >> res_scale);
	addPoint(doc, velocityObj, "incr", velocity.incr.width >> res_scale, velocity.incr.height >> res_scale);

	SaveGameStateFlags(doc, root, gameState);
}

void SaveSisState(yyjson_mut_doc* doc, yyjson_mut_val* obj, const SIS_STATE& SS)
{
	yyjson_mut_val* sis {yyjson_mut_obj_add_obj(doc, obj, "sis")};
	addPoint(doc, sis, "log", RES_DESCALE(SS.log_x), RES_DESCALE(SS.log_y));
	addInt(doc, sis, "ResUnits", SS.ResUnits);
	addInt(doc, sis, "FuelOnBoard", SS.FuelOnBoard);
	addInt(doc, sis, "CrewEnlisted", SS.CrewEnlisted);
	addInt(doc, sis, "TotalElementMass", SS.TotalElementMass);
	addInt(doc, sis, "TotalBioMass", SS.TotalBioMass);
	addArray<uint8_t>(doc, sis, "ModuleSlots", SS.ModuleSlots);
	addArray<uint8_t>(doc, sis, "DriveSlots", SS.DriveSlots);
	addArray<uint8_t>(doc, sis, "JetSlots", SS.JetSlots);
	addInt(doc, sis, "NumLanders", SS.NumLanders);
	addArray<uint16_t>(doc, sis, "ElementAmounts", SS.ElementAmounts);
	addString(doc, sis, "ShipName", SS.ShipName);
	addString(doc, sis, "CommanderName", SS.CommanderName);
	addString(doc, sis, "PlanetName", SS.PlanetName);
	addInt(doc, sis, "Difficulty", SS.Difficulty);
	addInt(doc, sis, "Extended", SS.Extended);
	addInt(doc, sis, "Nomad", SS.Nomad);
	addInt(doc, sis, "Seed", SS.Seed);
	addInt(doc, sis, "ShipSeed", SS.ShipSeed);
}

void SaveSummary(yyjson_mut_doc* doc, yyjson_mut_val* root, const SUMMARY_DESC& Summ)
{
	yyjson_mut_val* summary {yyjson_mut_obj_add_obj(doc, root, "summary")};
	addString(doc, summary, "SaveName", Summ.SaveName);
	addInt(doc, summary, "Activity", Summ.Activity);
	addInt(doc, summary, "Flags", Summ.Flags);
	addInt(doc, summary, "day_index", Summ.day_index);
	addInt(doc, summary, "month_index", Summ.month_index);
	addInt(doc, summary, "year_index", Summ.year_index);
	addInt(doc, summary, "MCreditLo", Summ.MCreditLo);
	addInt(doc, summary, "MCreditHi", Summ.MCreditHi);
	addArray<uint8_t>(doc, summary, "ShipList", uqstl::span {Summ.ShipList}.first(Summ.NumShips));
	addArray<uint8_t>(doc, summary, "DeviceList", uqstl::span {Summ.DeviceList}.first(Summ.NumDevices));
	addInt(doc, summary, "res_factor", Summ.res_factor);
	SaveSisState(doc, summary, Summ.SS);
}

void SaveStarDesc(yyjson_mut_doc* doc, yyjson_mut_val* root, const STAR_DESC& SD)
{
	yyjson_mut_val* star {yyjson_mut_obj_add_obj(doc, root, "starDesc")};
	addPoint(doc, star, "star_pt", SD.star_pt.x, SD.star_pt.y);
	addInt(doc, star, "Type", SD.Type);
	addInt(doc, star, "Index", SD.Index);
	addInt(doc, star, "Prefix", SD.Prefix);
	addInt(doc, star, "Postfix", SD.Postfix);
}

} // namespace

static void
PrepareSummary(SUMMARY_DESC* SummPtr, const char* name)
{
	SummPtr->SS = uqm::gameState().sis();

	SummPtr->Activity = uqm::lowByte(uqm::currentActivity());
	switch (SummPtr->Activity)
	{
		case IN_HYPERSPACE:
			if (inQuasiSpace())
			{
				SummPtr->Activity = IN_QUASISPACE;
			}
			break;
		case IN_INTERPLANETARY:
			// Get a better planet name for summary
			GetPlanetOrMoonName(SummPtr->SS.PlanetName,
								sizeof(SummPtr->SS.PlanetName));
			if (GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) == (uint8_t)~0)
			{
				SummPtr->Activity = IN_STARBASE;
			}
			else if (playerInPlanetOrbit())
			{
				SummPtr->Activity = IN_PLANET_ORBIT;
			}
			break;
		case IN_LAST_BATTLE:
			uqm::utf8StringCopy(SummPtr->SS.PlanetName,
								sizeof(SummPtr->SS.PlanetName),
								GAME_STRING(PLANET_NUMBER_BASE + 32)); // Sa-Matra
			break;
	}

	SummPtr->MCreditLo = GET_GAME_STATE(MELNORME_CREDIT0);
	SummPtr->MCreditHi = GET_GAME_STATE(MELNORME_CREDIT1);

	{
		HSHIPFRAG hStarShip, hNextShip;

		for (hStarShip = GetHeadLink(&GLOBAL(built_ship_q)),
			SummPtr->NumShips = 0;
			 hStarShip; hStarShip = hNextShip,
			++SummPtr->NumShips)
		{
			SHIP_FRAGMENT* StarShipPtr;

			StarShipPtr = LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
			hNextShip = _GetSuccLink(StarShipPtr);
			SummPtr->ShipList[SummPtr->NumShips] = StarShipPtr->race_id;
			UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
		}
	}

	SummPtr->NumDevices = InventoryDevices(SummPtr->DeviceList,
										   MAX_EXCLUSIVE_DEVICES);

	SummPtr->Flags = GET_GAME_STATE(LANDER_SHIELDS)
				   | (GET_GAME_STATE(IMPROVED_LANDER_SPEED) << (4 + 0))
				   | (GET_GAME_STATE(IMPROVED_LANDER_CARGO) << (4 + 1))
				   | (GET_GAME_STATE(IMPROVED_LANDER_SHOT) << (4 + 2))
				   | ((GET_GAME_STATE(CHMMR_BOMB_STATE) < 2 ? 0 : 1) << (4 + 3));

	SummPtr->day_index = uqm::gameState().clock().day();
	SummPtr->month_index = uqm::gameState().clock().month();
	SummPtr->year_index = uqm::gameState().clock().year();
	SummPtr->SaveName[SAVE_NAME_SIZE - 1] = 0;
	uqm::strncpy_safe(SummPtr->SaveName, name);
	SummPtr->res_factor = RESOLUTION_FACTOR;
}

static void
SaveProblemMessage(STAMP* MsgStamp)
{
#define MAX_MSG_LINES 1
	GFXRECT r = {
		{0, 0},
		{0, 0}
	  };
	uint16_t i;
	TEXT t;
	char* ppStr[MAX_MSG_LINES];

	// TODO: This should probably just use DoPopupWindow()

	ppStr[0] = GAME_STRING(SAVEGAME_STRING_BASE + 2);

	SetContextFont(StarConFont);

	t.baseline.x = t.baseline.y = 0;
	t.align = ALIGN_CENTER;
	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		GFXRECT tr;

		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
		{
			break;
		}
		t.CharCount = (uint16_t)~0;
		TextRect(&t, &tr, nullptr);
		if (i == 0)
		{
			r = tr;
		}
		else
		{
			BoxUnion(&tr, &r, &r);
		}
		t.baseline.y += 11;
	}
	t.baseline.x = (RES_SCALE(ORIG_SIS_SCREEN_WIDTH >> 1)
					- (r.extent.width >> 1))
				 - r.corner.x;
	t.baseline.y = (RES_SCALE(ORIG_SIS_SCREEN_HEIGHT >> 1)
					- (r.extent.height >> 1))
				 - r.corner.y;
	r.corner.x += t.baseline.x - RES_SCALE(4);
	r.corner.y += t.baseline.y - RES_SCALE(4);
	r.extent.width += RES_SCALE(8);
	r.extent.height += RES_SCALE(8);

	*MsgStamp = SaveContextFrame(&r);

	BatchGraphics();
	DrawStarConBox(&r, RES_SCALE(2), SHADOWBOX_MEDIUM_COLOR,
				   SHADOWBOX_DARK_COLOR, true, DKGRAY_COLOR, true, TRANSPARENT);
	SetContextForeGroundColor(
		isPC(uqm::UQMOptions::read().whichFonts) ? WHITE_COLOR : LTGRAY_COLOR);

	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
		{
			break;
		}
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += 11;
	}
	UnbatchGraphics();
}

void SaveProblem(void)
{
	STAMP s;
	GFXCONTEXT OldContext;

	OldContext = SetContext(SpaceContext);
	SaveProblemMessage(&s);
	FlushGraphics();

	WaitForAnyButton(true, WAIT_INFINITE, false);

	// Restore the screen under the message
	DrawStamp(&s);
	SetContext(OldContext);
	DestroyDrawable(ReleaseDrawable(s.frame));
}

static void
SaveFlagshipState(void)
{
	if (inHQSpace())
	{
		// Player is in HyperSpace or QuasiSpace.
		SaveSisHyperState();
	}
	else if (playerInSolarSystem())
	{
		SaveSolarSysLocation();
	}
}

static void
SaveStarInfo(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	GAME_STATE_FILE* fp {OpenStateFile(STARINFO_FILE, "rb")};
	if (fp)
	{
		uint32_t flen {LengthStateFile(fp)};
		if (flen % 4)
		{
			uqm::log::warn("Unexpected Star Info length! Expected "
						   "an integral number of DWORDS.\n");
		}
		else
		{
			yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "starInfo")};
			while (flen)
			{
				uint32_t val {};
				sread_32(fp, &val);
				yyjson_mut_arr_add_uint(doc, list, val);
				flen -= 4;
			}
		}
		CloseStateFile(fp);
	}
}

static void
SaveBattleGroup(yyjson_mut_doc* doc, yyjson_mut_val* list, GAME_STATE_FILE* fp,
				uint32_t encounter_id, uint32_t grpoffs)
{
	GROUP_HEADER h;
	SeekStateFile(fp, grpoffs, SEEK_SET);
	ReadGroupHeader(fp, &h);

	yyjson_mut_val* battleGroup {yyjson_mut_arr_add_obj(doc, list)};
	addInt(doc, battleGroup, "encounter", encounter_id);
	yyjson_mut_obj_add_bool(doc, battleGroup, "current", grpoffs && GLOBAL(BattleGroupRef) == grpoffs);
	addInt(doc, battleGroup, "star_index", h.star_index);
	addInt(doc, battleGroup, "day_index", h.day_index);
	addInt(doc, battleGroup, "month_index", h.month_index);
	addInt(doc, battleGroup, "year_index", h.year_index);

	yyjson_mut_val* groups {yyjson_mut_obj_add_arr(doc, battleGroup, "groups")};
	for (int i {1}; i <= h.NumGroups; ++i)
	{
		uint8_t icon {};
		uint8_t NumShips {};
		SeekStateFile(fp, h.GroupOffset[i], SEEK_SET);
		sread_8(fp, &icon);
		sread_8(fp, &NumShips);

		yyjson_mut_val* group {yyjson_mut_arr_add_obj(doc, groups)};
		addInt(doc, group, "icon", icon);
		yyjson_mut_val* ships {yyjson_mut_obj_add_arr(doc, group, "ships")};
		for (int j {}; j < NumShips; ++j)
		{
			uint8_t race_outer {};
			SHIP_FRAGMENT sf;
			sread_8(fp, &race_outer);
			ReadShipFragment(fp, &sf);

			yyjson_mut_val* ship {yyjson_mut_arr_add_obj(doc, ships)};
			addInt(doc, ship, "race_outer", race_outer);
			addShipFragment(doc, ship, sf);
		}
	}
}

static uint32_t
GetBattleGroupOffset(int encounterIndex)
{
	// The reason for this switch, even though the group offsets are
	// successive, is because GET_GAME_STATE is a #define, which stringizes
	// its argument.
	switch (encounterIndex)
	{
		case 1:
			return GET_GAME_STATE(SHOFIXTI_GRPOFFS);
		case 2:
			return GET_GAME_STATE(ZOQFOT_GRPOFFS);
		case 3:
			return GET_GAME_STATE(MELNORME0_GRPOFFS);
		case 4:
			return GET_GAME_STATE(MELNORME1_GRPOFFS);
		case 5:
			return GET_GAME_STATE(MELNORME2_GRPOFFS);
		case 6:
			return GET_GAME_STATE(MELNORME3_GRPOFFS);
		case 7:
			return GET_GAME_STATE(MELNORME4_GRPOFFS);
		case 8:
			return GET_GAME_STATE(MELNORME5_GRPOFFS);
		case 9:
			return GET_GAME_STATE(MELNORME6_GRPOFFS);
		case 10:
			return GET_GAME_STATE(MELNORME7_GRPOFFS);
		case 11:
			return GET_GAME_STATE(MELNORME8_GRPOFFS);
		case 12:
			return GET_GAME_STATE(URQUAN_PROBE_GRPOFFS);
		case 13:
			return GET_GAME_STATE(COLONY_GRPOFFS);
		case 14:
			return GET_GAME_STATE(SAMATRA_GRPOFFS);
		default:
			uqm::log::warn("SetBattleGroupOffset: invalid encounter"
						   " index.\n");
			return 0;
	}
}

static void
SaveGroups(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	yyjson_mut_val* battleGroups {};
	GAME_STATE_FILE* fp {OpenStateFile(RANDGRPINFO_FILE, "rb")};
	if (fp && LengthStateFile(fp) > 0)
	{
		GROUP_HEADER h;
		uint8_t lastenc {};
		uint8_t count {};
		ReadGroupHeader(fp, &h);
		SeekStateFile(fp, h.GroupOffset[0], SEEK_SET);
		sread_8(fp, &lastenc);
		sread_8(fp, &count);

		yyjson_mut_val* groupList {yyjson_mut_obj_add_obj(doc, root, "groupList")};
		addInt(doc, groupList, "LastEnc", lastenc);
		yyjson_mut_val* groups {yyjson_mut_obj_add_arr(doc, groupList, "groups")};
		for (int i {}; i < count; ++i)
		{
			uint8_t race_outer {};
			IP_GROUP ip;
			sread_8(fp, &race_outer);
			ReadIpGroup(fp, &ip);

			yyjson_mut_val* group {yyjson_mut_arr_add_obj(doc, groups)};
			addInt(doc, group, "race_outer", race_outer);
			addIpGroup(doc, group, ip);
		}

		battleGroups = yyjson_mut_obj_add_arr(doc, root, "battleGroups");
		SaveBattleGroup(doc, battleGroups, fp, 0, 0);
		CloseStateFile(fp);
	}
	fp = OpenStateFile(DEFGRPINFO_FILE, "rb");
	if (fp && LengthStateFile(fp) > 0)
	{
		for (int encounter_index {1}; encounter_index < 15; encounter_index++)
		{
			const uint32_t grpoffs {GetBattleGroupOffset(encounter_index)};
			if (grpoffs)
			{
				if (!battleGroups)
				{
					battleGroups = yyjson_mut_obj_add_arr(doc, root, "battleGroups");
				}
				SaveBattleGroup(doc, battleGroups, fp, encounter_index, grpoffs);
			}
		}
		CloseStateFile(fp);
	}
}

uqstl::string saveFileName(uint16_t which_game)
{
	return fmt::format("uqmsave.{:02}.json", which_game);
}

bool SaveGame(uint16_t which_game, SUMMARY_DESC* SummPtr, const char* name)
{
	const auto text {SaveGameJson(SummPtr, name)};
	const uqstl::string file {saveFileName(which_game)};
	if (!text || !uqm::writeTextFile(uqm::saveDir(), file.c_str(), *text))
	{
		DeleteResFile(uqm::saveDir(), file.c_str());
		return false;
	}
	return true;
}

using MutDocPtr = uqstl::unique_ptr<yyjson_mut_doc, decltype(&yyjson_mut_doc_free)>;
using TextPtr = uqstl::unique_ptr<char, decltype(&free)>;

// Makes the save document from the game state.
static MutDocPtr
WriteGameDocument(const SUMMARY_DESC& summary)
{
	STAR_DESC SD {};
	if (CurStarDescPtr)
	{
		SD = *CurStarDescPtr;
	}

	MutDocPtr doc {yyjson_mut_doc_new(nullptr), &yyjson_mut_doc_free};
	yyjson_mut_val* root {yyjson_mut_obj(doc.get())};
	yyjson_mut_doc_set_root(doc.get(), root);
	yyjson_mut_obj_add_uint(doc.get(), root, "version", c_saveFormatVersion);

	SaveSummary(doc.get(), root, summary);
	SaveGameState(doc.get(), root, uqm::gameState());

	SaveRaceQueue(doc.get(), root, &GLOBAL(avail_race_q));
	// START_INTERPLANETARY is only set when saving from Homeworld
	//   encounter screen. When the game is loaded, the
	//   GenerateOrbitalFunction for the current star system
	//   create the encounter anew and populate the npc queue.
	if (!(uqm::currentActivity() & START_INTERPLANETARY))
	{
		if (uqm::currentActivity() & START_ENCOUNTER)
		{
			SaveShipQueue(doc.get(), root, "npcShipQueue", &GLOBAL(npc_built_ship_q));
		}
		else if (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY)
		{
			// XXX: Technically, this queue does not need to be
			//   saved/loaded at all. IP groups will be reloaded
			//   from group state files. But the original code did,
			//   and so will we until we can prove we do not need to.
			SaveGroupQueue(doc.get(), root, &GLOBAL(ip_group_q));
		}
	}
	SaveShipQueue(doc.get(), root, "shipQueue", &GLOBAL(built_ship_q));
	SaveShipQueue(doc.get(), root, "stowedShipQueue", &GLOBAL(stowed_ship_q));

	SaveEvents(doc.get(), root);
	// Black globes in HyperSpace and QuasiSpace
	SaveEncounters(doc.get(), root);
	SaveStarInfo(doc.get(), root);
	SaveGroups(doc.get(), root);
	SaveStarDesc(doc.get(), root, SD);
	return doc;
}

uqstl::optional<uqstl::string> SaveGameJson(SUMMARY_DESC* SummPtr, const char* name)
{
	// XXX: Backup: SaveFlagshipState() overwrites ip_location
	const GFXPOINT pt {uqm::flagship().ipLocation};
	SaveFlagshipState();
	if (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
		&& !(uqm::currentActivity()
			 & (START_ENCOUNTER | START_INTERPLANETARY)))
	{
		PutGroupInfo(GROUPS_RANDOM, GROUP_SAVE_IP);
	}

	PrepareSummary(SummPtr, name);
	const MutDocPtr doc {WriteGameDocument(*SummPtr)};

	// XXX: Restore
	uqm::flagship().ipLocation = pt;
	// Only relevant when loading a game and must be cleaned
	uqm::flagship().inOrbit = 0;

	size_t length {};
	const TextPtr text {
		yyjson_mut_write(doc.get(), YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INVALID_UNICODE | YYJSON_WRITE_NEWLINE_AT_END, &length),
		&free};
	if (!text)
	{
		return uqstl::nullopt;
	}
	return uqstl::string {text.get(), length};
}
