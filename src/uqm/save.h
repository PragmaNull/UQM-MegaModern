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

#pragma once
#ifndef UQM_SAVE_H
#define UQM_SAVE_H

#include "core/misc/bittricks.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "globdata.h"
#include "sis.h" // SaveSummaryData includes FlagshipData in it

namespace uqm
{
// XXX: Theoretically, a player can have 17 devices on board without
//   cheating. We only provide
//   room for 16 below, which is not really a problem since this
//   is only used for displaying savegame summaries. There is also
//   room for only 16 devices on screen.
inline constexpr auto MaxExclusiveDevices {16};
inline constexpr auto SaveNameSize {64};

constexpr uint32_t c_saveFormatVersion {1};

struct SaveSummaryData
{
	FlagshipData SS;
	ActivityKind Activity;
	uint8_t Flags;
	uint8_t day_index, month_index;
	uint16_t year_index;
	uint8_t MCreditLo, MCreditHi;
	uint8_t NumShips, NumDevices;
	uqstl::array<uint8_t, MaxBuiltShips> ShipList;
	uqstl::array<uint8_t, MaxExclusiveDevices> DeviceList;
	char SaveName[SaveNameSize];
	uint8_t res_factor;
};

extern ActivityWord NextActivity;

// Loads only the summary when summary_desc is not null.
extern bool LoadGame(uint16_t which_game, SaveSummaryData* summary_desc);
// Loads a game from the text of a save file. 'file' names the save in error messages.
bool LoadGameFromJson(uqstl::string_view json, uqstl::string_view file, SaveSummaryData* summary_desc);
// Loads a game from a save file at any path.
bool LoadGameFile(const char* path);
uqstl::string saveFileName(uint16_t which_game);

extern void SaveProblem(void);
extern bool SaveGame(uint16_t which_game, SaveSummaryData* summary_desc, const char* name);
// Saves the game to the text of a save file, or nullopt if it cannot.
uqstl::optional<uqstl::string> SaveGameJson(SaveSummaryData* summary_desc, const char* name);

} // namespace uqm

#endif // UQM_SAVE_H
