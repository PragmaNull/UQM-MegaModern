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

#include "save.h"

#include <assert.h>

#include "build.h"
#include "controls.h"
#include "encount.h"
#include "gamestr.h"
#include "globdata.h"
#include "grpintrn.h"
#include "hyper.h"
#include "options.h"
#include "races.h"
#include "setup.h"
#include "shipcont.h"
#include "starmap.h"
#include "state.h"
#include "util.h"
// for SaveSisHyperState()
#include "planets/planets.h"
// for SaveSolarSysLocation() and tests
#include <yyjson.h>

#include "colors.h"
#include "core/log/log.h"
#include "libs/file/json.h"
#include "libs/file/textfile.h"
#include "libs/inplib.h"


namespace uqm
{
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
		if constexpr (uqstl::is_enum_v<T>)
		{
			yyjson_mut_arr_add_uint(doc, array, static_cast<uqstl::underlying_type_t<T>>(value));
		}
		else if constexpr (uqstl::is_signed_v<T>)
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

void addShipFragment(yyjson_mut_doc* doc, yyjson_mut_val* obj, const ShipFragment& frag)
{
	addInt(doc, obj, "captains_name_index", frag.captains_name_index);
	addInt(doc, obj, "race_id", frag.race_id);
	addInt(doc, obj, "index", frag.index);
	addInt(doc, obj, "crew_level", frag.crew_level);
	addInt(doc, obj, "max_crew", frag.max_crew);
	addInt(doc, obj, "energy_level", frag.energy_level);
	addInt(doc, obj, "max_energy", frag.max_energy);
}

void addIpGroup(yyjson_mut_doc* doc, yyjson_mut_val* obj, const InterplanetaryGroup& group)
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

void SaveShipQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, const char* key, Queue* pQueue)
{
	uint16_t num_links {CountLinks(pQueue)};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, key)};
	ShipFragmentHandle hStarShip {GetHeadLink(pQueue)};
	while (num_links--)
	{
		const ShipFragment* FragPtr {LockShipFrag(pQueue, hStarShip)};
		const ShipFragmentHandle hNextShip {_GetSuccLink(FragPtr)};
		addShipFragment(doc, yyjson_mut_arr_add_obj(doc, list), *FragPtr);
		UnlockShipFrag(pQueue, hStarShip);
		hStarShip = hNextShip;
	}
}

void SaveRaceQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, Queue* pQueue)
{
	uint16_t num_links {CountLinks(pQueue)};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "raceQueue")};
	FleetInfoHandle hFleet {GetHeadLink(pQueue)};
	while (num_links--)
	{
		const FleetInfo* FleetPtr {LockFleetInfo(pQueue, hFleet)};
		const FleetInfoHandle hNextFleet {_GetSuccLink(FleetPtr)};

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

void SaveGroupQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, Queue* pQueue)
{
	if (CountLinks(pQueue) == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "ipGroupQueue")};
	IpGroupHandle hNextGroup {};
	for (IpGroupHandle hGroup {GetHeadLink(pQueue)}; hGroup; hGroup = hNextGroup)
	{
		const InterplanetaryGroup* GroupPtr {LockIpGroup(pQueue, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);
		addIpGroup(doc, yyjson_mut_arr_add_obj(doc, list), *GroupPtr);
		UnlockIpGroup(pQueue, hGroup);
	}
}

void SaveEncounters(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	uint16_t num_links {CountLinks(&uqm::gameState().game().encounter_q)};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "encounters")};
	EncounterHandle hEncounter {GetHeadLink(&uqm::gameState().game().encounter_q)};
	while (num_links--)
	{
		Encounter* EncounterPtr {};
		LockEncounter(hEncounter, &EncounterPtr);
		const EncounterHandle hNextEncounter {GetSuccEncounter(EncounterPtr)};

		yyjson_mut_val* Encounter {yyjson_mut_arr_add_obj(doc, list)};
		addInt(doc, Encounter, "transition_state", EncounterPtr->transition_state);
		addPoint(doc, Encounter, "origin", EncounterPtr->origin.x, EncounterPtr->origin.y);
		addInt(doc, Encounter, "radius", EncounterPtr->radius);
		addPoint(doc, Encounter, "loc_pt", EncounterPtr->loc_pt.x, EncounterPtr->loc_pt.y);
		addInt(doc, Encounter, "race_id", EncounterPtr->race_id);
		addInt(doc, Encounter, "flags", EncounterPtr->flags);

		yyjson_mut_val* ships {yyjson_mut_obj_add_arr(doc, Encounter, "ships")};
		const uint16_t numShips {uqstl::min<uint16_t>(EncounterPtr->num_ships, MaxHyperShips)};
		for (uint16_t i {}; i < numShips; ++i)
		{
			const BriefShipInfo& ShipInfo {EncounterPtr->ShipList[i]};
			yyjson_mut_val* ship {yyjson_mut_arr_add_obj(doc, ships)};
			addInt(doc, ship, "race_id", ShipInfo.race_id);
			addInt(doc, ship, "crew_level", ShipInfo.crew_level);
			addInt(doc, ship, "max_crew", ShipInfo.max_crew);
			addInt(doc, ship, "max_energy", ShipInfo.max_energy);
		}

		addPoint(doc, Encounter, "log", ResDescale(EncounterPtr->log_x), ResDescale(EncounterPtr->log_y));

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
	EventHandle hEvent {GetHeadLink(&uqm::gameState().clock().events())};
	while (num_links--)
	{
		GameEvent* EventPtr {};
		LockEvent(hEvent, &EventPtr);
		const EventHandle hNextEvent {GetSuccEvent(EventPtr)};

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
		if (value > info.maxValue())
		{
			uqm::log::error("Game state '{}' ({}) does not fit in {} bits.", info.name, value, info.bits);
		}
		// The names are string literals, so the document can keep them.
		yyjson_mut_obj_add_uint(doc, flags, info.name.data(), value);
	}
}

void SaveGameState(yyjson_mut_doc* doc, yyjson_mut_val* root, uqm::GameState& gameState)
{
	const GameData& GS {gameState.game()};
	const uint8_t res_scale {static_cast<uint8_t>(uqm::activityKind(uqm::currentActivity()) != ActivityKind::InInterplanetary ? resolutionFactor : 0)};

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
	addPoint(doc, state, "ShipOrigin", ResDescale(flagship.shipStamp.origin.x), ResDescale(flagship.shipStamp.origin.y));
	addInt(doc, state, "ShipFacing", flagship.shipFacing);
	addInt(doc, state, "ip_planet", flagship.ipPlanet);
	addInt(doc, state, "in_orbit", flagship.inOrbit);

	const VelocityDesc& velocity {flagship.velocity};
	yyjson_mut_val* velocityObj {yyjson_mut_obj_add_obj(doc, state, "velocity")};
	addInt(doc, velocityObj, "TravelAngle", velocity.TravelAngle);
	addPoint(doc, velocityObj, "vector", velocity.vector.width >> res_scale, velocity.vector.height >> res_scale);
	addPoint(doc, velocityObj, "fract", velocity.fract.width >> res_scale, velocity.fract.height >> res_scale);
	addPoint(doc, velocityObj, "error", velocity.error.width >> res_scale, velocity.error.height >> res_scale);
	addPoint(doc, velocityObj, "incr", velocity.incr.width >> res_scale, velocity.incr.height >> res_scale);

	SaveGameStateFlags(doc, root, gameState);
}

void SaveSisState(yyjson_mut_doc* doc, yyjson_mut_val* obj, const FlagshipData& SS)
{
	yyjson_mut_val* sis {yyjson_mut_obj_add_obj(doc, obj, "sis")};
	addPoint(doc, sis, "log", ResDescale(SS.log_x), ResDescale(SS.log_y));
	addInt(doc, sis, "ResUnits", SS.ResUnits);
	addInt(doc, sis, "FuelOnBoard", SS.FuelOnBoard);
	addInt(doc, sis, "CrewEnlisted", SS.CrewEnlisted);
	addInt(doc, sis, "TotalElementMass", SS.TotalElementMass);
	addInt(doc, sis, "TotalBioMass", SS.TotalBioMass);
	addArray<ModuleType>(doc, sis, "ModuleSlots", SS.ModuleSlots);
	addArray<ModuleType>(doc, sis, "DriveSlots", SS.DriveSlots);
	addArray<ModuleType>(doc, sis, "JetSlots", SS.JetSlots);
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

void SaveSummary(yyjson_mut_doc* doc, yyjson_mut_val* root, const SaveSummaryData& Summ)
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

void SaveStarDesc(yyjson_mut_doc* doc, yyjson_mut_val* root, const StarDesc& SD)
{
	yyjson_mut_val* star {yyjson_mut_obj_add_obj(doc, root, "starDesc")};
	addPoint(doc, star, "star_pt", SD.star_pt.x, SD.star_pt.y);
	addInt(doc, star, "Type", SD.Type);
	addInt(doc, star, "Index", SD.Index);
	addInt(doc, star, "Prefix", SD.Prefix);
	addInt(doc, star, "Postfix", SD.Postfix);
}

} // namespace

static void PrepareSummary(SaveSummaryData* SummPtr, const char* name)
{
	SummPtr->SS = uqm::gameState().sis();

	SummPtr->Activity = uqm::activityKind(uqm::currentActivity());
	switch (SummPtr->Activity)
	{
		case ActivityKind::InHyperspace:
			if (inQuasiSpace())
			{
				SummPtr->Activity = ActivityKind::InQuasispace;
			}
			break;
		case ActivityKind::InInterplanetary:
			// Get a better planet name for summary
			GetPlanetOrMoonName(SummPtr->SS.PlanetName);
			if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) == (uint8_t)~0)
			{
				SummPtr->Activity = ActivityKind::InStarbase;
			}
			else if (uqm::flagship().inOrbit != 0)
			{
				SummPtr->Activity = ActivityKind::InPlanetOrbit;
			}
			break;
		case ActivityKind::InLastBattle:
			uqm::utf8StringCopy(SummPtr->SS.PlanetName,
								GameString(GameStringBase::PlanetNumber + 32)); // Sa-Matra
			break;
		default:
			break;
	}

	SummPtr->MCreditLo = uqm::gameState().flag(uqm::GameFlag::MELNORME_CREDIT0);
	SummPtr->MCreditHi = uqm::gameState().flag(uqm::GameFlag::MELNORME_CREDIT1);

	{
		ShipFragmentHandle hStarShip {};
		ShipFragmentHandle hNextShip {};

		for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q),
			SummPtr->NumShips = 0;
			 hStarShip; hStarShip = hNextShip,
			++SummPtr->NumShips)
		{
			const ShipFragment* StarShipPtr {LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip)};
			hNextShip = _GetSuccLink(StarShipPtr);
			SummPtr->ShipList[SummPtr->NumShips] = StarShipPtr->race_id;
			UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
		}
	}

	SummPtr->NumDevices = InventoryDevices(SummPtr->DeviceList);

	SummPtr->Flags = uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS)
				   | (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SPEED) << (4 + 0))
				   | (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_CARGO) << (4 + 1))
				   | (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SHOT) << (4 + 2))
				   | ((uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2 ? 0 : 1) << (4 + 3));

	SummPtr->day_index = uqm::gameState().clock().day();
	SummPtr->month_index = uqm::gameState().clock().month();
	SummPtr->year_index = uqm::gameState().clock().year();
	SummPtr->SaveName[SaveNameSize - 1] = 0;
	uqm::strncpy_safe(SummPtr->SaveName, name);
	SummPtr->res_factor = resolutionFactor;
}

static constexpr auto MaxMsgLines {1};

static void SaveProblemMessage(GfxStamp* MsgStamp)
{
	GfxRect r = {
		{0, 0},
		{0, 0}
	  };
	uint16_t i {};
	GfxText t;
	uqstl::array<uqgsl::czstring, MaxMsgLines> ppStr;

	// TODO: This should probably just use DoPopupWindow()

	ppStr[0] = GameString(GameStringBase::SavegameString + 2);

	SetContextFont(StarConFont);

	t.baseline.x = t.baseline.y = 0;
	t.align = TextAlign::Center;
	for (i = 0; i < MaxMsgLines; ++i)
	{
		GfxRect tr;

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
	t.baseline.x = (ResScale(OrigSisScreenWidth() >> 1)
					- (r.extent.width >> 1))
				 - r.corner.x;
	t.baseline.y = (ResScale(OrigSisScreenHeight() >> 1)
					- (r.extent.height >> 1))
				 - r.corner.y;
	r.corner.x += t.baseline.x - ResScale(4);
	r.corner.y += t.baseline.y - ResScale(4);
	r.extent.width += ResScale(8);
	r.extent.height += ResScale(8);

	*MsgStamp = SaveContextFrame(&r);

	BatchGraphics();
	DrawStarConBox(&r, ResScale(2), ShadowboxMediumColor,
				   ShadowboxDarkColor, true, DkgrayColor, true, Transparent);
	SetContextForeGroundColor(
		isPC(uqm::UQMOptions::read().whichFonts) ? WhiteColor : LtgrayColor);

	for (i = 0; i < ppStr.size(); ++i)
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
	GfxStamp s;

	{
		const ContextScope contextScope {SpaceContext};
		SaveProblemMessage(&s);
		FlushGraphics();

		WaitForAnyButton(true, WaitInfinite, false);

		// Restore the screen under the message
		DrawStamp(&s);
	}
	DestroyDrawable(ReleaseDrawable(s.frame));
}

static void SaveFlagshipState(void)
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

static void SaveStarInfo(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	GameStateFile* fp {OpenStateFile(StarinfoFile, "rb")};
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

static void SaveBattleGroup(yyjson_mut_doc* doc, yyjson_mut_val* list, GameStateFile* fp,
							uint32_t encounter_id, uint32_t grpoffs)
{
	BattleGroupHeader h;
	SeekStateFile(fp, grpoffs, SEEK_SET);
	ReadGroupHeader(fp, &h);

	yyjson_mut_val* battleGroup {yyjson_mut_arr_add_obj(doc, list)};
	addInt(doc, battleGroup, "encounter", encounter_id);
	yyjson_mut_obj_add_bool(doc, battleGroup, "current", grpoffs && uqm::gameState().game().BattleGroupRef == grpoffs);
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
			ShipFragment sf;
			sread_8(fp, &race_outer);
			ReadShipFragment(fp, &sf);

			yyjson_mut_val* ship {yyjson_mut_arr_add_obj(doc, ships)};
			addInt(doc, ship, "race_outer", race_outer);
			addShipFragment(doc, ship, sf);
		}
	}
}

static uint32_t GetBattleGroupOffset(int encounterIndex)
{
	// The reason for this switch, even though the group offsets are
	// successive, is because GET_GAME_STATE is a #define, which stringizes
	// its argument.
	switch (encounterIndex)
	{
		case 1:
			return uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_GRPOFFS);
		case 2:
			return uqm::gameState().flag(uqm::GameFlag::ZOQFOT_GRPOFFS);
		case 3:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME0_GRPOFFS);
		case 4:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME1_GRPOFFS);
		case 5:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME2_GRPOFFS);
		case 6:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME3_GRPOFFS);
		case 7:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME4_GRPOFFS);
		case 8:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME5_GRPOFFS);
		case 9:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME6_GRPOFFS);
		case 10:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME7_GRPOFFS);
		case 11:
			return uqm::gameState().flag(uqm::GameFlag::MELNORME8_GRPOFFS);
		case 12:
			return uqm::gameState().flag(uqm::GameFlag::URQUAN_PROBE_GRPOFFS);
		case 13:
			return uqm::gameState().flag(uqm::GameFlag::COLONY_GRPOFFS);
		case 14:
			return uqm::gameState().flag(uqm::GameFlag::SAMATRA_GRPOFFS);
		default:
			uqm::log::warn("SetBattleGroupOffset: invalid encounter"
						   " index.\n");
			return 0;
	}
}

static void SaveGroups(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	yyjson_mut_val* battleGroups {};
	GameStateFile* fp {OpenStateFile(RandgrpinfoFile, "rb")};
	if (fp && LengthStateFile(fp) > 0)
	{
		BattleGroupHeader h;
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
			InterplanetaryGroup ip;
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
	fp = OpenStateFile(DefgrpinfoFile, "rb");
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

bool SaveGame(uint16_t which_game, SaveSummaryData* SummPtr, const char* name)
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


// Makes the save document from the game state.
static JsonMutDoc WriteGameDocument(const SaveSummaryData& summary)
{
	StarDesc SD {};
	if (CurStarDescPtr())
	{
		SD = *CurStarDescPtr();
	}

	JsonMutDoc doc {newJsonDoc()};
	yyjson_mut_val* root {yyjson_mut_obj(doc.get())};
	yyjson_mut_doc_set_root(doc.get(), root);
	yyjson_mut_obj_add_uint(doc.get(), root, "version", c_saveFormatVersion);

	SaveSummary(doc.get(), root, summary);
	SaveGameState(doc.get(), root, uqm::gameState());

	SaveRaceQueue(doc.get(), root, &uqm::gameState().game().avail_race_q);
	// START_INTERPLANETARY is only set when saving from Homeworld
	//   Encounter screen. When the game is loaded, the
	//   GenerateOrbitalFunction for the current star system
	//   create the Encounter anew and populate the npc queue.
	if (!(uqm::currentActivity() & ActivityFlag::StartInterplanetary))
	{
		if (uqm::currentActivity() & ActivityFlag::StartEncounter)
		{
			SaveShipQueue(doc.get(), root, "npcShipQueue", &uqm::gameState().game().npc_built_ship_q);
		}
		else if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary)
		{
			// XXX: Technically, this queue does not need to be
			//   saved/loaded at all. IP groups will be reloaded
			//   from group state files. But the original code did,
			//   and so will we until we can prove we do not need to.
			SaveGroupQueue(doc.get(), root, &uqm::gameState().game().ip_group_q);
		}
	}
	SaveShipQueue(doc.get(), root, "shipQueue", &uqm::gameState().game().built_ship_q);
	SaveShipQueue(doc.get(), root, "stowedShipQueue", &uqm::gameState().game().stowed_ship_q);

	SaveEvents(doc.get(), root);
	// Black globes in HyperSpace and QuasiSpace
	SaveEncounters(doc.get(), root);
	SaveStarInfo(doc.get(), root);
	SaveGroups(doc.get(), root);
	SaveStarDesc(doc.get(), root, SD);
	return doc;
}

uqstl::optional<uqstl::string> SaveGameJson(SaveSummaryData* SummPtr, const char* name)
{
	// XXX: Backup: SaveFlagshipState() overwrites ip_location
	const GfxPoint pt {uqm::flagship().ipLocation};
	SaveFlagshipState();
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		&& !(uqm::currentActivity()
			 & (ActivityFlag::StartEncounter | ActivityFlag::StartInterplanetary)))
	{
		PutGroupInfo(GroupsRandom, GroupSaveIp);
	}

	PrepareSummary(SummPtr, name);
	const JsonMutDoc doc {WriteGameDocument(*SummPtr)};

	// XXX: Restore
	uqm::flagship().ipLocation = pt;
	// Only relevant when loading a game and must be cleaned
	uqm::flagship().inOrbit = 0;

	return writeJson(doc, JsonWritePretty | YYJSON_WRITE_ALLOW_INVALID_UNICODE);
}

} // namespace uqm
