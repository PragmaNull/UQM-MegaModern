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

#pragma once
#ifndef UQM_ENCOUNT_H
#define UQM_ENCOUNT_H


#include "core/types/enum.h"
struct BriefShipInfo;
struct Encounter;

// XXX: temporary, for CONVERSATION
#include "commglue.h"
#include "displist.h"
#include "libs/gfxlib.h"
#include "planets/planets.h"
#include "element.h"
#include "races.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif


typedef HLINK HENCOUNTER;

inline constexpr auto MaxHyperShips {7};

// Encounter.flags
// XXX: Currently, the flags are combined with num_ships into a single uint8_t
//   in the savegames: num_ships occupy the low nibble and flags the high one.
//   Bits 4 and 5 are available for more flags in the savegames,
//   and bits 0-3 available in the game but will not be saved.
inline constexpr auto OneShotEncounter {1 << 7};
inline constexpr auto EncounterReforming {1 << 6};


struct BriefShipInfo
{
	// The only field actually used right now is crew_level
	uint8_t race_id;
	uint16_t crew_level;
	uint16_t max_crew;
	uint8_t max_energy;
};

struct Encounter
{
	// QueueLink elements; must be first
	HENCOUNTER pred, succ;

	HELEMENT hElement;

	int16_t transition_state;
	GfxPoint origin;
	uint16_t radius;
	uint8_t race_id;
	uint8_t num_ships;
	uint8_t flags;
	// See Encounter.flags above
	GfxPoint loc_pt;

	BriefShipInfo ShipList[MaxHyperShips];
	// Only the crew_level member is currently used

	int32_t log_x, log_y;
};

constexpr auto AllocEncounter() { return AllocLink(&uqm::gameState().game().encounter_q); }
constexpr auto PutEncounter(const auto& h) { return PutQueue(&uqm::gameState().game().encounter_q, h); }
constexpr auto InsertEncounter(const auto& h, const auto& i) { return InsertQueue(&uqm::gameState().game().encounter_q, h, i); }
constexpr auto GetHeadEncounter() { return GetHeadLink(&uqm::gameState().game().encounter_q); }

inline void LockEncounter(HENCOUNTER h, Encounter** ppe)
{
	*ppe = reinterpret_cast<Encounter*>(LockLink(&uqm::gameState().game().encounter_q, h));
}
constexpr auto UnlockEncounter(const auto& h) { return UnlockLink(&uqm::gameState().game().encounter_q, h); }
constexpr auto RemoveEncounter(const auto& h) { return RemoveQueue(&uqm::gameState().game().encounter_q, h); }
constexpr auto FreeEncounter(const auto& h) { return FreeLink(&uqm::gameState().game().encounter_q, h); }

constexpr auto GetSuccEncounter(const auto& l) { return _GetSuccLink(l); }

enum class EncounterAction
{
	Hail = 0,
	Attack
};
template <>
inline constexpr bool IsArithmeticEnum<EncounterAction> = true;

extern void EncounterBattle(void);
extern void BuildBattle(uint16_t which_player);
extern uint16_t InitEncounter(void);
extern uint16_t UninitEncounter(void);
extern bool FleetIsInfinite(uint16_t playerNr);
extern void UpdateShipFragCrew(Starship*);

extern void SetBattlePlanet(void);

// Last race the player battled with, or -1 if no battle took place.
// Set to -1 by some funcs to inhibit IP groups from intercepting
// the flagship.
extern int16_t EncounterRace;
extern uint8_t EncounterGroup;

#if 0 //defined(__cplusplus)
}
#endif

#endif // UQM_ENCOUNT_H
