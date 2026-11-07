#pragma once
#ifndef UQM_GRPINTRN_H
#define UQM_GRPINTRN_H

// For IPGROUP
#include "grpinfo.h"

// For ShipFragment
#include "races.h"

// For GameStateFile
#include "state.h"

//#define DEBUG_GROUPS

// A group header describes battle groups present in a star system. There is
//    at most 1 group header per system.
// 'Random' group info file (RandgrpinfoFile) always contains only one
//    group header record, which describes the last-visited star system,
//    (which may be the current system). Thus the randomly generated groups
//    are valid for 7 days (set in PutGroupInfo) after the player leaves
//    the system, or until the player enters another star system.
struct BattleGroupHeader
{
	uint8_t NumGroups;
	uint8_t day_index, month_index;
	uint16_t star_index, year_index;
	// day_index, month_index, year_index specify when
	//   random groups expire (if you were to leave the system
	//   by going to HSpace and stay there till such time)
	// star_index is the index of a star this group header
	//   applies to; ~0 means uninited
	uint32_t GroupOffset[NumSavedBattleGroups + 1];
	// Absolute offsets of group definitions in a state file
	// Group 0 is a list of groups present in solarsys
	//    (RandgrpinfoFile only)
	// Groups 1..max are definitions of actual battle groups
	//    containing ship makeup and status

	// Each group has the following format:
	// 1 byte, RaceType (LastEncGroup in Group 0)
	// 1 byte, NumShips (NumGroups in Group 0)
	// Ships follow:
	// 1 byte, RaceType
	// 16 bytes, part of ShipFragment struct
	//                  (part of InterplanetaryGroup struct in Group 0)
};

void ReadGroupHeader(GameStateFile* fp, BattleGroupHeader* pGH);
void WriteGroupHeader(GameStateFile* fp, const BattleGroupHeader* pGH);
void ReadShipFragment(GameStateFile* fp, ShipFragment* FragPtr);
void WriteShipFragment(GameStateFile* fp, const ShipFragment* FragPtr);
void ReadIpGroup(GameStateFile* fp, InterplanetaryGroup* GroupPtr);
void WriteIpGroup(GameStateFile* fp, const InterplanetaryGroup* GroupPtr);

#endif // UQM_GRPINTRN_H
