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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gameev.h"

#include "build.h"
#include "clock.h"
#include "starmap.h"
#include "gendef.h"
#include "globdata.h"
#include "hyper.h"
#include "resinst.h"
#include "uqm/lua/UQMLua.h"
#include "lua/luafuncs/customfuncs.h"
#include "lua/luafuncs/customfuncs.h"
#include "core/misc/bittricks.h"
#include "core/log/log.h"
#include "libs/mathlib.h"
#include "nameref.h"
#include "options.h"
#include <stdlib.h>
#include "setup.h"
#include "sounds.h"
#include "planets/lander.h"

static int arilou_entrance_event(int arg);
static int arilou_exit_event(int arg);
static void check_race_growth(void);
static int hyperspace_encounter_event(int arg);
static int kohr_ah_victorious_event(int arg);
static int advance_pkunk_mission(int arg);
static int advance_thradd_mission(int arg);
static int zoqfot_distress_event(int arg);
static int zoqfot_death_event(int arg);
static int shofixti_return_event(int arg);
static int advance_utwig_supox_mission(int arg);
static int kohr_ah_genocide_event(int arg);
static int spathi_shield_event(int arg);
static int advance_ilwrath_mission(int arg);
static int advance_mycon_mission(int arg);
static int arilou_umgah_check(int arg);
static int yehat_rebel_event(int arg);
static int slylandro_ramp_up(int arg);
static int slylandro_ramp_down(int arg);

const static char* eventNames[] = {
	"ARILOU_ENTRANCE_EVENT",
	"ARILOU_EXIT_EVENT",
	"HYPERSPACE_ENCOUNTER_EVENT",
	"KOHR_AH_VICTORIOUS_EVENT",
	"ADVANCE_PKUNK_MISSION",
	"ADVANCE_THRADD_MISSION",
	"ZOQFOT_DISTRESS_EVENT",
	"ZOQFOT_DEATH_EVENT",
	"SHOFIXTI_RETURN_EVENT",
	"ADVANCE_UTWIG_SUPOX_MISSION",
	"KOHR_AH_GENOCIDE_EVENT",
	"SPATHI_SHIELD_EVENT",
	"ADVANCE_ILWRATH_MISSION",
	"ADVANCE_MYCON_MISSION",
	"ARILOU_UMGAH_CHECK",
	"YEHAT_REBEL_EVENT",
	"SLYLANDRO_RAMP_UP",
	"SLYLANDRO_RAMP_DOWN"};

void initEventSystem(void)
{
	// Register functions which can be called from Lua through
	// 'custom.<functionName>'. Right now, these are the event functions
	// which have not been converted to Lua yet.
	const static luaUqm_custom_Function eventFuncs[] = {
		{"arilou_entrance_event",		  arilou_entrance_event	   },
		{"arilou_exit_event",			  arilou_exit_event		   },
		{"hyperspace_encounter_event",  hyperspace_encounter_event },
		{"kohr_ah_victorious_event",	 kohr_ah_victorious_event	 },
		{"advance_pkunk_mission",		  advance_pkunk_mission	   },
		{"advance_thradd_mission",	   advance_thradd_mission	 },
		{"zoqfot_distress_event",		  zoqfot_distress_event	   },
		{"zoqfot_death_event",		   zoqfot_death_event		 },
		{"shofixti_return_event",		  shofixti_return_event	   },
		{"advance_utwig_supox_mission", advance_utwig_supox_mission},
		{"kohr_ah_genocide_event",	   kohr_ah_genocide_event	 },
		{"spathi_shield_event",			spathi_shield_event		   },
		{"advance_ilwrath_mission",		advance_ilwrath_mission	   },
		{"advance_mycon_mission",		  advance_mycon_mission	   },
		{"arilou_umgah_check",		   arilou_umgah_check		 },
		{"yehat_rebel_event",			  yehat_rebel_event		   },
		{"slylandro_ramp_up",			  slylandro_ramp_up		   },
		{"slylandro_ramp_down",			slylandro_ramp_down		   },
		{nullptr,					   nullptr					 }
	  };

	uqm::lua().beginEvents(eventFuncs, EventScriptRes);
}

void uninitEventSystem(void)
{
	uqm::lua().endEvents();
}

int eventIdStrToNum(const char* eventIdStr)
{
	const size_t eventCount = std::size(eventNames);

	// Linear search; acceptable for such a small number of events.
	for (size_t eventI {0}; eventI < eventCount; eventI++)
	{
		if (strcmp(eventIdStr, eventNames[eventI]) == 0)
		{
			return eventI;
		}
	}
	return -1;
}

const char* eventIdNumToStr(int eventNum)
{
	const size_t eventCount = std::size(eventNames);
	if (eventNum < 0 || (size_t)eventNum >= eventCount)
	{
		return nullptr;
	}
	return eventNames[eventNum];
}

void AddInitialGameEvents(void)
{
	AddEvent(EventType::Relative, 0, 1, 0, GameEventId::HyperspaceEncounterEvent);
	AddEvent(EventType::Absolute, 3, 17, StartYear, GameEventId::ArilouEntranceEvent);
	AddEvent(EventType::Relative, 0, 0,
			 YearsToKohrahVictory() + difficultyCase(2, 0, -1),
			 GameEventId::KohrAhVictoriousEvent);
	AddEvent(EventType::Relative, 0, 0, 0, GameEventId::SlylandroRampUp);
}

void EventHandler(GameEventId selector)
{
	const char* eventIdStr {eventIdNumToStr(*selector)};
	if (eventIdStr == nullptr)
	{
		uqm::log::warn("Warning: EventHandler(): Event {} is "
					   "unknown.",
					   selector);
		return;
	}

	uqm::lua().callEvent(eventIdStr);
}

void SetRaceDest(uint8_t which_race, int16_t x, int16_t y, uint8_t days_left,
				 uint8_t func_index)
{
	HFLEETINFO hFleet {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, which_race)};
	FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet)};

	FleetPtr->dest_loc.x = x;
	FleetPtr->dest_loc.y = y;
	FleetPtr->days_left = days_left;
	FleetPtr->func_index = func_index;

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hFleet);
}


static int arilou_entrance_event(int arg)
{
	uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE, static_cast<uint32_t>(ArilouGateState::Opening));
	AddEvent(EventType::Relative, 0, 3, 0, GameEventId::ArilouExitEvent);
	(void)arg;
	return 0;
}

static int arilou_exit_event(int arg)
{
	uint16_t month_index {};

	uint16_t year_index {uqm::gameState().clock().year()};
	if ((month_index = uqm::gameState().clock().month() % 12) == 0)
	{
		++year_index;
	}
	++month_index;

	uqm::gameState().setFlag(uqm::GameFlag::ARILOU_SPACE, static_cast<uint32_t>(ArilouGateState::Closing));
	AddEvent(EventType::Absolute,
			 month_index, 17, year_index, GameEventId::ArilouEntranceEvent);

	(void)arg;
	return 0;
}

static void check_race_growth(void)
{
	HFLEETINFO hStarShip {};
	HFLEETINFO hNextShip {};

	for (hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hStarShip; hStarShip = hNextShip)
	{
		FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
		hNextShip = _GetSuccLink(FleetPtr);

		if (FleetPtr->actual_strength
			&& FleetPtr->actual_strength != InfiniteRadius)
		{
			int16_t delta_strength {(int8_t)FleetPtr->growth};
			if (FleetPtr->growth_err_term <= FleetPtr->growth_fract)
			{
				if (delta_strength <= 0)
				{
					--delta_strength;
				}
				else
				{
					++delta_strength;
				}
			}
			FleetPtr->growth_err_term -= FleetPtr->growth_fract;

			delta_strength += FleetPtr->actual_strength;
			if (delta_strength <= 0)
			{
				delta_strength = 0;
				FleetPtr->allied_state = *RaceAllegiance::Dead;
			}
			else if (delta_strength > MaxFleetStrength)
			{
				delta_strength = MaxFleetStrength;
			}

			FleetPtr->actual_strength = (uint16_t)delta_strength;
			if (FleetPtr->actual_strength && FleetPtr->days_left)
			{
				FleetPtr->loc.x += (FleetPtr->dest_loc.x - FleetPtr->loc.x)
								 / FleetPtr->days_left;
				FleetPtr->loc.y += (FleetPtr->dest_loc.y - FleetPtr->loc.y)
								 / FleetPtr->days_left;

				if (--FleetPtr->days_left == 0
					&& FleetPtr->func_index != (uint8_t)~0)
				{
					EventHandler(static_cast<GameEventId>(FleetPtr->func_index));
				}
			}
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}
}

static int hyperspace_encounter_event(int arg)
{
	check_race_growth();
	if (inHyperSpace())
	{
		check_hyperspace_encounter();
	}

	AddEvent(EventType::Relative, 0, 1, 0, GameEventId::HyperspaceEncounterEvent);

	(void)arg;
	return 0;
}

static int kohr_ah_victorious_event(int arg)
{
	if (uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION))
	{
		// The Utwig/Supox mission delayed the genocide.
		// Try again in one year.
		AddEvent(EventType::Relative, 0, 0, 1, GameEventId::KohrAhGenocideEvent);
		return 0;
	}

	// No more delay; start the genocide.
	return kohr_ah_genocide_event(arg);
}

static int advance_pkunk_mission(int arg)
{
	HFLEETINFO hPkunk {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Pkunk)};
	FleetInfo* PkunkPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hPkunk)};

	const GfxPoint yehat = SeedFleetLocation(PkunkPtr, plot_map, *PlotId::Yehat);
	const GfxPoint pkunk = SeedFleetLocation(PkunkPtr, plot_map, 0);

	if (PkunkPtr->actual_strength)
	{
		uint8_t MissionState {};

		MissionState = uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION);
		if (PkunkPtr->days_left == 0 && MissionState)
		{
			if ((MissionState & 1) /* made it to Yehat space */
				|| (PkunkPtr->loc.x == yehat.x
					&& PkunkPtr->loc.y == yehat.y)) // North of yehat by 360
			{
				PkunkPtr->actual_strength = 0;
			}
			else if (PkunkPtr->loc.x == pkunk.x // Pkunk center of SOI
					 && PkunkPtr->loc.y == pkunk.y
					 && uqm::gameState().flag(uqm::GameFlag::PKUNK_ON_THE_MOVE))
			{
				uqm::gameState().setFlag(uqm::GameFlag::PKUNK_ON_THE_MOVE, static_cast<uint32_t>(0));
				AddEvent(EventType::Relative, 3, 0, 0, GameEventId::AdvancePkunkMission);
				UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hPkunk);
				return 0;
			}
		}

		if (PkunkPtr->actual_strength == 0)
		{
			uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK, static_cast<uint32_t>(1));
			PkunkPtr->allied_state = *RaceAllegiance::Dead;
			StartSphereTracking(RaceId::Yehat);
		}
		else
		{
			GfxPoint loc;

			if (!(MissionState & 1))
			{
				loc = yehat;
			}
			else
			{
				loc = pkunk;
			}
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_ON_THE_MOVE, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SWITCH, static_cast<uint32_t>(0));
			SetRaceDest(*RaceId::Pkunk, loc.x, loc.y,
						(uint8_t)((365 >> 1) - PkunkPtr->days_left),
						*GameEventId::AdvancePkunkMission);
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MISSION, static_cast<uint32_t>(MissionState + 1));
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hPkunk);

	(void)arg;
	return 0;
}

// Send the Thraddash to fight the Kohr-Ah.
static int advance_thradd_mission(int arg)
{
	uint8_t MissionState {};

	HFLEETINFO hThradd {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Thraddash)};
	FleetInfo* ThraddPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hThradd)};

	MissionState = uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION);
	if (ThraddPtr->actual_strength && MissionState < 3)
	{
		GfxPoint loc;

		if (MissionState < 2)
		{ /* attacking */
			loc = SeedFleetLocation(ThraddPtr, plot_map, *PlotId::Samatra);
		}
		else
		{ /* returning */
			loc = SeedFleetLocation(ThraddPtr, plot_map, 0);
		}

		if (MissionState == 1)
		{ /* arrived at Kohr-Ah, engaging */

			const int16_t strength_loss {(int16_t)(ThraddPtr->actual_strength >> 1)};
			ThraddPtr->growth = (uint8_t)(-strength_loss / 14);
			ThraddPtr->growth_fract = (uint8_t)(((strength_loss % 14) << 8) / 14);
			ThraddPtr->growth_err_term = 255 >> 1;
		}
		else
		{
			if (MissionState != 0)
			{ /* stop losses */
				ThraddPtr->growth = 0;
				ThraddPtr->growth_fract = 0;
			}
		}
		SetRaceDest(*RaceId::Thraddash, loc.x, loc.y, 14, *GameEventId::AdvanceThraddMission);
	}
	++MissionState;
	uqm::gameState().setFlag(uqm::GameFlag::THRADD_MISSION, static_cast<uint32_t>(MissionState));

	if (MissionState == 4 && uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
	{ /* returned home - notify the Ilwrath */
		AddEvent(EventType::Relative, 0, 0, 0, GameEventId::AdvanceIlwrathMission);
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hThradd);

	(void)arg;
	return 0;
}

static int zoqfot_distress_event(int arg)
{
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		&& CurStarDescPtr()
		&& CurStarDescPtr()->Index == PlotId::Zoqfot)
	{
		AddEvent(EventType::Relative, 0, 7, 0, GameEventId::ZoqfotDistressEvent);
	}
	else
	{
		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_DISTRESS, static_cast<uint32_t>(1));
		AddEvent(EventType::Relative, 6, 0, 0, GameEventId::ZoqfotDeathEvent);
	}

	(void)arg;
	return 0;
}

static int zoqfot_death_event(int arg)
{
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		&& CurStarDescPtr()
		&& CurStarDescPtr()->Index == PlotId::Zoqfot)
	{
		AddEvent(EventType::Relative, 0, 7, 0, GameEventId::ZoqfotDeathEvent);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS))
	{
		HFLEETINFO hZoqFot {};

		hZoqFot = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
									   *RaceId::Zoqfotpik);
		FleetInfo* ZoqFotPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hZoqFot)};
		ZoqFotPtr->actual_strength = 0;
		ZoqFotPtr->allied_state = *RaceAllegiance::Dead;
		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hZoqFot);

		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_DISTRESS, static_cast<uint32_t>(2));
	}

	(void)arg;
	return 0;
}

static int shofixti_return_event(int arg)
{
	HFLEETINFO hShofixti {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Shofixti)};
	FleetInfo* ShofixtiPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hShofixti)};
	if (ShofixtiPtr->actual_strength)
	{
		ShofixtiPtr->growth = 0;
		ShofixtiPtr->growth_fract = 0;
		(void)arg;
		return 0;
	}
	if (IsExtended())
	{
		ShofixtiPtr->actual_strength = 150 / SphereRadiusIncrement * 2;
		ShofixtiPtr->loc = SeedFleetLocation(ShofixtiPtr, plot_map, *PlotId::Home);
		StartSphereTracking(RaceId::Shofixti);
		ShofixtiPtr->growth = 1;
		ShofixtiPtr->growth_fract = 0;
		AddEvent(EventType::Relative, 3, 0, 0, GameEventId::ShofixtiReturnEvent);
	}
	SetRaceAllied(RaceId::Shofixti, true);
	uqm::gameState().game().CrewCost -= ifHardDifficulty(1, 2);
	/* crew is not an issue anymore */
	uqm::gameState().setFlag(uqm::GameFlag::CREW_PURCHASED0, static_cast<uint32_t>(0));
	uqm::gameState().setFlag(uqm::GameFlag::CREW_PURCHASED1, static_cast<uint32_t>(0));

	(void)arg;
	return 0;
}

static int advance_utwig_supox_mission(int arg)
{
	uint8_t MissionState {};

	HFLEETINFO hUtwig {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Utwig)};
	FleetInfo* UtwigPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hUtwig)};
	HFLEETINFO hSupox {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Supox)};
	FleetInfo* SupoxPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hSupox)};
	HFLEETINFO hKohrAh {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::BlackUrquan)};
	FleetInfo* KohrAhPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hKohrAh)};

	MissionState = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
	if (UtwigPtr->actual_strength && MissionState < 5)
	{
		if (MissionState == 1)
		{
			AddEvent(EventType::Relative, 0, (160 >> 1), 0,
					 GameEventId::AdvanceUtwigSupoxMission);

			int16_t strength_loss {(int16_t)(UtwigPtr->actual_strength >> 1)};
			UtwigPtr->growth = (uint8_t)(-strength_loss / 160);
			UtwigPtr->growth_fract =
				(uint8_t)(((strength_loss % 160) << 8) / 160);
			UtwigPtr->growth_err_term = 255 >> 1;

			strength_loss = (int16_t)(SupoxPtr->actual_strength >> 1);
			if (strength_loss)
			{
				SupoxPtr->growth = (uint8_t)(-strength_loss / 160);
				SupoxPtr->growth_fract =
					(uint8_t)(((strength_loss % 160) << 8) / 160);
				SupoxPtr->growth_err_term = 255 >> 1;
			}

			if (IsExtended())
			{
				strength_loss = (int16_t)(KohrAhPtr->actual_strength >> 5);
				KohrAhPtr->growth = (uint8_t)(-strength_loss / 160);
				KohrAhPtr->growth_fract =
					(uint8_t)(((strength_loss % 160) << 8) / 160);
				KohrAhPtr->growth_err_term = 255 >> 1;
			}

			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(0));
		}
		else if (MissionState == 2)
		{
			AddEvent(EventType::Relative, 0, (160 >> 1), 0,
					 GameEventId::AdvanceUtwigSupoxMission);
			++MissionState;
		}
		else
		{
			GfxPoint utwig;
			GfxPoint supox;

			if (MissionState == 0)
			{
				// Set utwig and supox to their samatra attack point
				utwig = SeedFleetLocation(UtwigPtr, plot_map, *PlotId::Samatra);
				supox = SeedFleetLocation(SupoxPtr, plot_map, *PlotId::Samatra);
			}
			else
			{
				// Send utwig and supox home
				utwig = SeedFleetLocation(UtwigPtr, plot_map, 0);
				supox = SeedFleetLocation(SupoxPtr, plot_map, 0);

				UtwigPtr->growth = 0;
				UtwigPtr->growth_fract = 0;
				SupoxPtr->growth = 0;
				SupoxPtr->growth_fract = 0;

				if (IsExtended())
				{
					KohrAhPtr->growth = 0;
					KohrAhPtr->growth_fract = 0;
				}

				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(0));
			}
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_INFO, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_INFO, static_cast<uint32_t>(0));
			SetRaceDest(*RaceId::Utwig, utwig.x, utwig.y, 21, *GameEventId::AdvanceUtwigSupoxMission);
			SetRaceDest(*RaceId::Supox, supox.x, supox.y, 21, (uint8_t)~0);
		}
	}
	uqm::gameState().setFlag(uqm::GameFlag::UTWIG_SUPOX_MISSION, static_cast<uint32_t>(MissionState + 1));

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hSupox);
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hUtwig);
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hKohrAh);

	(void)arg;
	return 0;
}

static int kohr_ah_genocide_event(int arg)
{
	uint8_t Index {};
	int16_t best_dx {};
	int16_t best_dy {};
	HFLEETINFO hStarShip {};
	HFLEETINFO hNextShip {};
	HFLEETINFO hBlackUrquan {};

	if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY)
		&& uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		&& CurStarDescPtr()
		&& CurStarDescPtr()->Index == PlotId::Samatra)
	{
		AddEvent(EventType::Relative, 0, 7, 0, GameEventId::KohrAhGenocideEvent);
		return 0;
	}

	hBlackUrquan = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
										*RaceId::BlackUrquan);
	const FleetInfo* BlackUrquanPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hBlackUrquan)};

	long best_dist {-1};
	best_dx = plot_map[*PlotId::Sol].star_pt.x - BlackUrquanPtr->loc.x;
	best_dy = plot_map[*PlotId::Sol].star_pt.y - BlackUrquanPtr->loc.y;
	for (Index = 0, hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hStarShip; ++Index, hStarShip = hNextShip)
	{
		FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
		hNextShip = _GetSuccLink(FleetPtr);

		if (Index != RaceId::BlackUrquan
			&& Index != RaceId::Urquan
			&& FleetPtr->actual_strength != InfiniteRadius)
		{
			int16_t dx {};
			int16_t dy {};

			dx = FleetPtr->loc.x - BlackUrquanPtr->loc.x;
			dy = FleetPtr->loc.y - BlackUrquanPtr->loc.y;
			if (dx == 0 && dy == 0)
			{
				// Arrived at the victim's home world. Cleanse it.
				FleetPtr->allied_state = *RaceAllegiance::Dead;
				FleetPtr->actual_strength = 0;
			}
			else if (FleetPtr->actual_strength)
			{
				const long dist {(long)dx * dx + (long)dy * dy};
				if (best_dist < 0 || dist < best_dist || Index == RaceId::Druuge)
				{
					best_dist = dist;
					best_dx = dx;
					best_dy = dy;

					if (Index == RaceId::Druuge)
					{
						hNextShip = 0;
					}
				}
			}
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}

	if (best_dist < 0 && best_dx == 0 && best_dy == 0)
	{
		// All spheres of influence are gone - game over.
		uqm::currentActivity() &= ~ActivityFlag::InBattle;
		uqm::gameState().sis().CrewEnlisted = (uint16_t)~0;

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_KILLED_ALL, static_cast<uint32_t>(1));
	}
	else
	{
		// Moving towards new race to cleanse.
		uint16_t speed {};

		if (best_dist < 0)
		{
			best_dist = (long)best_dx * best_dx + (long)best_dy * best_dy;
		}

		speed = square_root(best_dist) / 158;
		if (speed == 0)
		{
			speed = 1;
		}
		else if (speed > 255)
		{
			speed = 255;
		}

		if (uqm::UQMOptions::read().cheatMode)
		{
			speed = 0;
		}

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_FRENZY, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_REASONS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_PLEAD, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_VISITS, static_cast<uint32_t>(0));
		SetRaceDest(*RaceId::BlackUrquan,
					BlackUrquanPtr->loc.x + best_dx,
					BlackUrquanPtr->loc.y + best_dy,
					(uint8_t)speed, *GameEventId::KohrAhGenocideEvent);
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hBlackUrquan);

	(void)arg;
	return 0;
}

static int spathi_shield_event(int arg)
{
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		&& CurStarDescPtr()
		&& CurStarDescPtr()->Index == PlotId::Spathi)
	{
		AddEvent(EventType::Relative, 0, 7, 0, GameEventId::SpathiShieldEvent);
	}
	else
	{
		HFLEETINFO hSpathi {};

		hSpathi = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
									   *RaceId::Spathi);
		FleetInfo* SpathiPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hSpathi)};

		if (SpathiPtr->actual_strength)
		{
			SetRaceAllied(RaceId::Spathi, false);
			if (isDifficulty(uqm::Difficulty::Hard))
			{
				RemoveEscortShips(RaceId::Spathi);
			}
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_SHIELDED_SELVES, static_cast<uint32_t>(1));
			SpathiPtr->actual_strength = 0;
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hSpathi);
	}

	(void)arg;
	return 0;
}

static constexpr auto MaddLength {128};

static int advance_ilwrath_mission(int arg)
{
	uint8_t ThraddState {};

	HFLEETINFO hIlwrath {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Ilwrath)};
	FleetInfo* IlwrathPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hIlwrath)};
	HFLEETINFO hThradd {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Thraddash)};
	FleetInfo* ThraddPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hThradd)};
	const GfxPoint thradd_home = SeedFleetLocation(ThraddPtr, plot_map, 0);
	const GfxPoint conflict = SeedFleetLocation(IlwrathPtr, plot_map, *PlotId::Thradd);
	if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
	{
		IlwrathPtr->actual_strength = 0;
		IlwrathPtr->allied_state = *RaceAllegiance::Dead;
		if ((IsExtended() && ThraddPtr->allied_state != RaceAllegiance::Good) || !IsExtended())
		{
			ThraddPtr->actual_strength = 0;
			ThraddPtr->allied_state = *RaceAllegiance::Dead;
		}
	}
	else if (IlwrathPtr->actual_strength)
	{
		if (IlwrathPtr->dest_loc.x != conflict.x
			|| IlwrathPtr->dest_loc.y != conflict.y)
		{
			SetRaceDest(*RaceId::Ilwrath, conflict.x, conflict.y, 90,
						*GameEventId::AdvanceIlwrathMission);
		}
		else
		{
			int16_t strength_loss {};

			if (IlwrathPtr->days_left == 0)
			{ /* arrived for battle */
				uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::HELIX_UNPROTECTED, static_cast<uint32_t>(1));
				strength_loss = (int16_t)IlwrathPtr->actual_strength;
				IlwrathPtr->growth = (uint8_t)(-strength_loss / MaddLength);
				IlwrathPtr->growth_fract =
					(uint8_t)(((strength_loss % MaddLength) << 8) / MaddLength);
				SetRaceDest(*RaceId::Ilwrath, (conflict.x + thradd_home.x) / 2,
							(conflict.y + thradd_home.y) / 2,
							MaddLength - 1, *GameEventId::AdvanceIlwrathMission);

				if (IsExtended() && ThraddPtr->allied_state == RaceAllegiance::Good)
				{
					strength_loss = (int16_t)(ThraddPtr->actual_strength * 0.25); // Smarterer math
				}
				else
				{
					strength_loss = (int16_t)ThraddPtr->actual_strength;
				}

				ThraddPtr->growth = (uint8_t)(-strength_loss / MaddLength);
				ThraddPtr->growth_fract =
					(uint8_t)(((strength_loss % MaddLength) << 8) / MaddLength);

				if ((IsExtended() && ThraddPtr->allied_state != RaceAllegiance::Good) || !IsExtended())
				{
					uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(0));
					if (ThraddPtr->allied_state == RaceAllegiance::Good)
					{
						SetRaceAllied(RaceId::Thraddash, false);
					}
				}
			}

			ThraddState = uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION);
			if (ThraddState == 0 || ThraddState > 3)
			{ /* never went to Kohr-Ah or returned */
				SetRaceDest(*RaceId::Thraddash,
							(conflict.x + thradd_home.x) / 2,
							(conflict.y + thradd_home.y) / 2,
							IlwrathPtr->days_left + 1, (uint8_t)~0);
			}
			else if (ThraddState < 3)
			{ /* recall on the double */
				SetRaceDest(*RaceId::Thraddash, thradd_home.x, thradd_home.y, 10,
							*GameEventId::AdvanceThraddMission);
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_MISSION, static_cast<uint32_t>(3));
			}
		}
	}

	if (IsExtended() && ThraddPtr->allied_state == RaceAllegiance::Good && !IlwrathPtr->actual_strength)
	{
		ThraddPtr->growth = 0;
		ThraddPtr->growth_fract = 0;
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH, static_cast<uint32_t>(0));
		SetRaceDest(*RaceId::Thraddash, thradd_home.x, thradd_home.y, 3, (uint8_t)~0);
		if (!uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX))
		{
			uqm::gameState().setFlag(uqm::GameFlag::HELIX_UNPROTECTED, static_cast<uint32_t>(0));
		}
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hThradd);
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hIlwrath);

	(void)arg;
	return 0;
}

static int advance_mycon_mission(int arg)
{
	HFLEETINFO hMycon {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Mycon)};
	FleetInfo* MyconPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hMycon)};

	const GfxPoint home = SeedFleetLocation(MyconPtr, plot_map, 0);
	const GfxPoint trap = SeedFleetLocation(MyconPtr, plot_map, *PlotId::MyconTrap);

	if (MyconPtr->actual_strength)
	{
		if (MyconPtr->growth)
		{
			// Head back.
			uqm::gameState().setFlag(uqm::GameFlag::MYCON_KNOW_AMBUSH, static_cast<uint32_t>(1));
			SetRaceDest(*RaceId::Mycon, home.x, home.y, 30, (uint8_t)~0);

			if (IsExtended())
			{
				HFLEETINFO hSyreen {};

				hSyreen = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
											   *RaceId::Syreen);
				FleetInfo* SyreenPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hSyreen)};
				if (SyreenPtr)
				{
					const GfxPoint syra = SeedFleetLocation(SyreenPtr, plot_map, 0);
					SetRaceDest(*RaceId::Syreen, syra.x, syra.y, 30, (uint8_t)~0);
					SyreenPtr->growth = 0;
					SyreenPtr->growth_fract = 0;
				}
			}

			MyconPtr->growth = 0;
			MyconPtr->growth_fract = 0;
		}
		else if (MyconPtr->loc.x != trap.x || MyconPtr->loc.y != trap.y)
		{
			SetRaceDest(*RaceId::Mycon, trap.x, trap.y, 30, *GameEventId::AdvanceMyconMission);
		}
		// To Organon.
		else
		{
			// Endure losses at Organon.

			AddEvent(EventType::Relative, 0, 14, 0, GameEventId::AdvanceMyconMission);
			const int16_t strength_loss {(int16_t)(MyconPtr->actual_strength >> 1)};
			MyconPtr->growth = (uint8_t)(-strength_loss / 14);
			MyconPtr->growth_fract = (uint8_t)(((strength_loss % 14) << 8) / 14);
			MyconPtr->growth_err_term = 255 >> 1;
			if (IsExtended())
			{
				HFLEETINFO hSyreen {};

				hSyreen = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
											   *RaceId::Syreen);
				FleetInfo* SyreenPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hSyreen)};
				if (SyreenPtr)
				{
					// The rest?... well we took care of most of them...
					// in our own special way.
					SyreenPtr->growth = (uint8_t)(strength_loss / 28);
					SyreenPtr->growth_fract =
						(uint8_t)(((strength_loss % 28) << 8) / 28);
				}
			}
		}
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hMycon);

	(void)arg;
	return 0;
}

static int arilou_umgah_check(int arg)
{
	uqm::gameState().setFlag(uqm::GameFlag::ARILOU_CHECKED_UMGAH, static_cast<uint32_t>(2));

	(void)arg;
	return 0;
}

static int yehat_rebel_event(int arg)
{
	HFLEETINFO hRebel {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::YehatRebel)};
	FleetInfo* RebelPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hRebel)};
	HFLEETINFO hRoyalist {GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Yehat)};
	FleetInfo* RoyalistPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hRoyalist)};
	RoyalistPtr->actual_strength = RoyalistPtr->actual_strength * 2 / 3;
	RebelPtr->actual_strength = RoyalistPtr->actual_strength;
	RebelPtr->loc = SeedFleetLocation(RebelPtr, plot_map, *PlotId::Yehat);
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hRoyalist);
	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hRebel);
	StartSphereTracking(RaceId::YehatRebel);

	(void)arg;
	return 0;
}

static int slylandro_ramp_up(int arg)
{
	if (!uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP))
	{
		uint8_t ramp_factor {};

		ramp_factor = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_MULTIPLIER);
		if (++ramp_factor <= 4)
		{
			uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_MULTIPLIER, static_cast<uint32_t>(ramp_factor));
			AddEvent(EventType::Relative, 0, 182, 0, GameEventId::SlylandroRampUp);
		}
	}

	(void)arg;
	return 0;
}

static int slylandro_ramp_down(int arg)
{
	uint8_t ramp_factor {};

	ramp_factor = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_MULTIPLIER);
	if (--ramp_factor)
	{
		AddEvent(EventType::Relative, 0, 23, 0, GameEventId::SlylandroRampDown);
	}
	uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_MULTIPLIER, static_cast<uint32_t>(ramp_factor));

	(void)arg;
	return 0;
}
