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


#include "core/types/enum.h"
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
inline constexpr auto StarbaseStringCount {12 + 2 * *ModuleType::NumPurchaseModules + 2};
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

enum class GameStringBase
{
	StarString = 0,
	DeviceString = StarString + StarStringCount,
	CargoString = DeviceString + DeviceStringCount,
	ElementsString = CargoString + CargoStringCount,
	ScanString = ElementsString + ElementsStringCount,
	StarNumber = ScanString + ScanStringCount,
	PlanetNumber = StarNumber + StarNumberCount,
	MonthsString = PlanetNumber + PlanetNumberCount,
	FeedbackString = MonthsString + MonthsStringCount,
	StarbaseString = FeedbackString + FeedbackStringCount,
	EncounterString = StarbaseString + StarbaseStringCount,
	NavigationString = EncounterString + EncounterStringCount,
	NamingString = NavigationString + NavigationStringCount,
	MeleeString = NamingString + NamingStringCount,
	SavegameString = MeleeString + MeleeStringCount,
	OptionString = SavegameString + SavegameStringCount,
	QuitmenuString = OptionString + OptionStringCount,
	StatusString = QuitmenuString + QuitmenuStringCount,
	FlagshipString = StatusString + StatusStringCount,
	OrbitscanString = FlagshipString + FlagshipStringCount,
	MainmenuString = OrbitscanString + OrbitscanStringCount,
	NetmeleeString = MainmenuString + MainmenuStringCount,
	BiologicalString = NetmeleeString + NetmeleeStringCount,
	PlaymenuString = BiologicalString + BiologicalStringCount,
	LabelString = PlaymenuString + PlaymenuStringCount,

	GamestrCount = LabelString + LabelStringCount
};
template <>
inline constexpr bool IsArithmeticEnum<GameStringBase> = true;


inline constexpr auto DosStarbaseStringBase {(GameStringBase::StarbaseString + 12)};
inline constexpr auto TdoStarbaseStringBase {DosStarbaseStringBase + *ModuleType::NumPurchaseModules};
inline constexpr auto EndStarbaseStringBase {TdoStarbaseStringBase + *ModuleType::NumPurchaseModules};

extern uqm::StringTablePtr GameStrings;
inline auto GameString(size_t i) { return GameStrings->text(i); }
inline auto GameString(ArithmeticEnum auto i) { return GameStrings->text(static_cast<size_t>(*i)); }

extern void SetCursorRect(GfxRect* r, GFXCONTEXT context);
extern void FlushCursorRect(void);
void SetCursorFlashBlock(bool state);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_GAMESTR_H_ */
