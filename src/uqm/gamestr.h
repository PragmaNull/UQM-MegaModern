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

/*
 * This file contains definitions relating to using strings specific to
 * the game. libs/strlib.h is for the string library.
 */

#ifndef UQM_GAMESTR_H_
#define UQM_GAMESTR_H_


#include "libs/strlib.h"
#include "sis.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

inline constexpr auto StarStringCount {149};
inline constexpr auto DeviceStringCount {29};
inline constexpr auto CargoStringCount {14};
inline constexpr auto ElementsStringCount {133};
inline constexpr auto ScanStringCount {66};
inline constexpr auto StarNumberCount {14};
inline constexpr auto PlanetNumberCount {43};
inline constexpr auto MonthsStringCount {12};
inline constexpr auto FeedbackStringCount {8};
inline constexpr auto StarbaseStringCount {(12 + 2 * NUM_PURCHASE_MODULES + 2)};
inline constexpr auto EncounterStringCount {8};
inline constexpr auto NavigationStringCount {9};
inline constexpr auto NamingStringCount {8};
inline constexpr auto MeleeStringCount {25};
inline constexpr auto SavegameStringCount {8};
inline constexpr auto OptionStringCount {10};
inline constexpr auto QuitmenuStringCount {5};
inline constexpr auto StatusStringCount {22};
inline constexpr auto FlagshipStringCount {13};
inline constexpr auto OrbitscanStringCount {19};
inline constexpr auto MainmenuStringCount {74};
inline constexpr auto NetmeleeStringCount {34};
inline constexpr auto BiologicalStringCount {26};
inline constexpr auto PlaymenuStringCount {30};
inline constexpr auto LabelStringCount {26};

enum
{
	STAR_STRING_BASE = 0,
	DEVICE_STRING_BASE = STAR_STRING_BASE + StarStringCount,
	CARGO_STRING_BASE = DEVICE_STRING_BASE + DeviceStringCount,
	ELEMENTS_STRING_BASE = CARGO_STRING_BASE + CargoStringCount,
	SCAN_STRING_BASE = ELEMENTS_STRING_BASE + ElementsStringCount,
	STAR_NUMBER_BASE = SCAN_STRING_BASE + ScanStringCount,
	PLANET_NUMBER_BASE = STAR_NUMBER_BASE + StarNumberCount,
	MONTHS_STRING_BASE = PLANET_NUMBER_BASE + PlanetNumberCount,
	FEEDBACK_STRING_BASE = MONTHS_STRING_BASE + MonthsStringCount,
	STARBASE_STRING_BASE = FEEDBACK_STRING_BASE + FeedbackStringCount,
	ENCOUNTER_STRING_BASE = STARBASE_STRING_BASE + StarbaseStringCount,
	NAVIGATION_STRING_BASE = ENCOUNTER_STRING_BASE + EncounterStringCount,
	NAMING_STRING_BASE = NAVIGATION_STRING_BASE + NavigationStringCount,
	MELEE_STRING_BASE = NAMING_STRING_BASE + NamingStringCount,
	SAVEGAME_STRING_BASE = MELEE_STRING_BASE + MeleeStringCount,
	OPTION_STRING_BASE = SAVEGAME_STRING_BASE + SavegameStringCount,
	QUITMENU_STRING_BASE = OPTION_STRING_BASE + OptionStringCount,
	STATUS_STRING_BASE = QUITMENU_STRING_BASE + QuitmenuStringCount,
	FLAGSHIP_STRING_BASE = STATUS_STRING_BASE + StatusStringCount,
	ORBITSCAN_STRING_BASE = FLAGSHIP_STRING_BASE + FlagshipStringCount,
	MAINMENU_STRING_BASE = ORBITSCAN_STRING_BASE + OrbitscanStringCount,
	NETMELEE_STRING_BASE = MAINMENU_STRING_BASE + MainmenuStringCount,
	BIOLOGICAL_STRING_BASE = NETMELEE_STRING_BASE + NetmeleeStringCount,
	PLAYMENU_STRING_BASE = BIOLOGICAL_STRING_BASE + BiologicalStringCount,
	LABEL_STRING_BASE = PLAYMENU_STRING_BASE + PlaymenuStringCount,

	GAMESTR_COUNT = LABEL_STRING_BASE + LabelStringCount
};


#define GAME_STRING(i) (GameStrings->text(i))

inline constexpr auto DosStarbaseStringBase {(STARBASE_STRING_BASE + 12)};
inline constexpr auto TdoStarbaseStringBase {(DosStarbaseStringBase + NUM_PURCHASE_MODULES)};
inline constexpr auto EndStarbaseStringBase {(TdoStarbaseStringBase + NUM_PURCHASE_MODULES)};

extern uqm::StringTablePtr GameStrings;

extern void SetCursorRect(GFXRECT* r, GFXCONTEXT context);
extern void FlushCursorRect(void);
void SetCursorFlashBlock(bool state);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_GAMESTR_H_ */
