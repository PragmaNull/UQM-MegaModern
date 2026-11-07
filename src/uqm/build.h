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
#ifndef UQM_BUILD_H
#define UQM_BUILD_H

#include "races.h"
#include "displist.h"
#include "core/misc/bittricks.h"
#include "starmap.h"
#include "uqm/random/UQMRandom.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

inline constexpr auto NumRaceLabels {3};
inline constexpr auto NumClassLabels {2};
inline constexpr auto NameOffset {NumRaceLabels + NumClassLabels};
inline constexpr auto NumCaptainsNames {16};

constexpr auto PickCaptainName() { return (((uint16_t)uqm::random().next() & (NumCaptainsNames - 1)) + NameOffset); }

extern HLINK Build(Queue* pQueue, SpeciesId SpeciesID);
extern HSHIPFRAG CloneShipFragment(RaceId shipIndex, Queue* pDstQueue,
								   uint16_t crew_level);
extern HLINK GetStarShipFromIndex(Queue* pShipQ, uint16_t Index);
extern HFLEETINFO GetSeededFleetFromIndex(uint16_t Index);
extern HSHIPFRAG GetEscortByStarShipIndex(uint16_t index);
extern uint8_t NameCaptain(Queue* pQueue, SpeciesId SpeciesID);

extern uint16_t GetIndexFromStarShip(Queue* pShipQ, HLINK hStarShip);
extern int SetEscortCrewComplement(RaceId which_ship, uint16_t crew_level,
								   uint8_t captain);

extern SpeciesId ShipIdStrToIndex(const char* shipIdStr);
extern RaceId RaceIdStrToIndex(const char* raceIdStr);
extern uint16_t AddEscortShips(RaceId race, int16_t count);
extern uint16_t CalculateEscortsWorth(void);
extern uint16_t CalculateEscortsPoints(void);
extern bool CanBuyPoints(HFLEETINFO hFleet);
extern bool ShipsReady(RaceId race);
extern void PrepareShip(RaceId race);
extern bool SetRaceAllied(RaceId race, bool flag);
extern uint16_t StartSphereTracking(RaceId race);
extern bool CheckSphereTracking(RaceId race);
extern bool KillRace(RaceId race);
extern uint16_t CountEscortShips(RaceId race);
extern bool HaveEscortShip(RaceId race);
extern uint16_t EscortFeasibilityStudy(RaceId race);
extern uint16_t CheckAlliance(RaceId race);
extern bool RaceDead(RaceId race);
extern uint16_t RemoveSomeEscortShips(RaceId race, uint16_t count);
extern uint16_t RemoveEscortShips(RaceId race);

extern RaceDesc* load_ship(SpeciesId SpeciesID, bool LoadBattleData);
extern void free_ship(RaceDesc* RaceDescPtr, bool FreeIconData,
					  bool FreeBattleData);
extern void loadGameCheats(void);
// WarEraStrength gives the hard coded strength values (formerly an array)
extern uint16_t WarEraStrength(SpeciesId SpeciesID);
// SeedFleetLocation moves the fleet to the plot location specified in visit.
extern GfxPoint SeedFleetLocation(FleetInfo* FleetPtr, PlotLocation* plotmap,
								  uint16_t visit);
// SeedFleet does initial fleet placement for StarSeed
extern void SeedFleet(FleetInfo* FleetPtr, PlotLocation* plotmap);
// SeedShip handles ship seeding, used in build.c to handle the load window
extern SpeciesId SeedShip(SpeciesId SpeciesID, bool loadWindow);
extern bool legacySave;
extern uint8_t GTFO;

#if 0 //defined(__cplusplus)
}
#endif

#endif // UQM_BUILD_H
