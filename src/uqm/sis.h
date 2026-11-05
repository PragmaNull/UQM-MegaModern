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

#ifndef SIS_H_INCL__
#define SIS_H_INCL__

#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"
#include "planets/elemdata.h" // for NUM_ELEMENT_CATEGORIES
#include "units.h"			  // for RESOLUTION_FACTOR
#include "options/OptionDefs.h"


inline constexpr auto ClearSisRadar {(1 << 2)};
inline constexpr auto DrawSisDisplay {(1 << 3)};

inline constexpr auto UndefinedDelta {0x7FFF};

inline constexpr auto NumDriveSlots {11};
inline constexpr auto NumJetSlots {8};
inline constexpr auto NumModuleSlots {16};

inline constexpr auto CrewPodCapacity {50};
inline constexpr auto StorageBayCapacity {500}; /* km cubed */
inline constexpr auto FuelTankScale {100};
inline constexpr auto FuelTankCapacity {(50 * FuelTankScale)};
inline constexpr auto HefuelTankCapacity {(100 * FuelTankScale)};
inline constexpr auto ModuleCostScale {50};

inline constexpr auto CrewExpenseThreshold {1000};

inline constexpr auto CrewPerRow {5};
inline constexpr auto SbayMassPerRow {50};

inline constexpr auto MaxFuelBars {10};
inline constexpr auto FuelVolumePerRow {(HefuelTankCapacity / MaxFuelBars)};
inline constexpr auto FuelReserve {(10 * FuelTankScale)};
inline constexpr auto FuelCostRu {20};

inline constexpr auto IpShipThrustIncrement {8};
inline constexpr auto IpShipTurnWait {17};
inline constexpr auto IpShipTurnDecrement {2};

inline constexpr auto BioCreditValueDefault {2};

enum
{
	PLANET_LANDER = 0,
	/* thruster types */
	FUSION_THRUSTER,
	/* jet types */
	TURNING_JETS,
	/* module types */
	CREW_POD,
	STORAGE_BAY,
	FUEL_TANK,
	HIGHEFF_FUELSYS,
	DYNAMO_UNIT,
	SHIVA_FURNACE,
	GUN_WEAPON,
	BLASTER_WEAPON,
	CANNON_WEAPON,
	TRACKING_SYSTEM,
	ANTIMISSILE_DEFENSE,

	NUM_PURCHASE_MODULES,

	BOMB_MODULE_0 = NUM_PURCHASE_MODULES,
	BOMB_MODULE_1,
	BOMB_MODULE_2,
	BOMB_MODULE_3,
	BOMB_MODULE_4,
	BOMB_MODULE_5,

	NUM_MODULES /* must be last entry */
};

inline constexpr auto EmptySlot {NUM_MODULES};
inline constexpr auto NumBombModules {10};

inline auto SafePad() { return SafeNeg(3); }

inline auto DriveSideX() { return (ResScale(31) - SafePad()); }
inline auto DriveSideY() { return ResScale(56); }
inline auto DriveTopX() { return (ResScale(33) - SafePad()); }
inline auto DriveTopY() { return (ResScale(86) - DosNumScl(21)); }

inline auto JetSideX() { return (ResScale(71) - SafePad()); }
inline auto JetSideY() { return ResScale(48); }
inline auto JetTopX() { return (ResScale(70) - SafePad()); }
inline auto JetTopY() { return (ResScale(94) - DosNumScl(21)); }

inline auto ModuleSideX() { return (ResScale(17) - SafePad()); }
inline auto ModuleSideY() { return ResScale(14); }
inline auto ModuleTopX() { return (ResScale(17) - SafePad()); }
inline auto ModuleTopY() { return (ResScale(117) - DosNumScl(21)); }

inline auto ShipPieceOffset() { return ResScale(12); }

inline constexpr auto MaxBuiltShips {12};
/* Maximum number of ships escorting the SIS */
inline constexpr auto MaxStowedShips {200}; // Limited to a byte
											/* Maximum number of ships banked at the starbase */
inline constexpr auto MaxLanders {10};

#define SUPPORT_SHIP_PTS         \
	{3 + 0, 30 + (2 * 16)},      \
		{3 + 42, 30 + (2 * 16)}, \
		{3 + 0, 30 + (3 * 16)},  \
		{3 + 42, 30 + (3 * 16)}, \
		{3 + 0, 30 + (1 * 16)},  \
		{3 + 42, 30 + (1 * 16)}, \
		{3 + 0, 30 + (4 * 16)},  \
		{3 + 42, 30 + (4 * 16)}, \
		{3 + 0, 30 + (0 * 16)},  \
		{3 + 42, 30 + (0 * 16)}, \
		{3 + 0, 30 + (5 * 16)},  \
		{3 + 42, 30 + (5 * 16)},

inline constexpr auto LanderDosX {206};
inline constexpr auto LanderDosY {59};

#define LANDER_DOS_PTS                      \
	{LanderDosX, LanderDosY},               \
		{LanderDosX + 14, LanderDosY - 6},  \
		{LanderDosX + 14, LanderDosY + 9},  \
		{LanderDosX, LanderDosY - 15},      \
		{LanderDosX, LanderDosY + 15},      \
		{LanderDosX - 14, LanderDosY - 9},  \
		{LanderDosX - 14, LanderDosY + 6},  \
		{LanderDosX + 14, LanderDosY - 21}, \
		{LanderDosX + 14, LanderDosY + 24}, \
		{LanderDosX - 28, LanderDosY - 3},

inline constexpr auto SisNameSize {32};

typedef struct
{
	int32_t log_x, log_y;

	uint32_t ResUnits;

	uint32_t FuelOnBoard;
	uint16_t CrewEnlisted;
	// Number of crew on board, not counting the captain.
	// Set to (uint16_t) ~0 to indicate game over.
	uint16_t TotalElementMass, TotalBioMass;

	uint8_t ModuleSlots[NumModuleSlots];
	uint8_t DriveSlots[NumDriveSlots];
	uint8_t JetSlots[NumJetSlots];

	uint8_t NumLanders;

	uint16_t ElementAmounts[NUM_ELEMENT_CATEGORIES];

	char ShipName[SisNameSize];
	char CommanderName[SisNameSize];
	char PlanetName[SisNameSize];
	uqm::Difficulty Difficulty;
	uint8_t Extended;
	uqm::NomadMode Nomad;
	uint8_t ShipSeed;
	int32_t Seed;
	uint8_t SaveVersion;
} SIS_STATE;

inline constexpr auto OverrideLanderFlags {(1 << 7)};
inline constexpr auto AfterBombInstalled {(1 << 7)};

extern void RepairSISBorder(void);
extern void InitSISContexts(void);
extern void DrawSISFrame(void);
extern void ClearSISRect(uint8_t ClearFlags);
extern void SetFlashRect(const GFXRECT* pRect, bool purple);
extern void SetAdditionalRect(const GFXRECT* pRect, uint16_t number);
extern void DumpAdditionalRect(void);
extern void PreUpdateFlashRect(void);
extern void PostUpdateFlashRect(void);
extern bool PauseFlash(void);
extern void ContinueFlash(void);

inline auto SfrMenu3do() { return ((GFXRECT*)~0L); }
inline auto SfrMenuAny() { return ((GFXRECT*)~1L); }
inline auto SfrMenuNon() { return ((GFXRECT*)~2L); }
extern void DrawHyperCoords(GFXPOINT puniverse);
extern void DrawSaveInfo(SIS_STATE SisState);
extern void DrawSISTitle(const char* pStr);

// Flags for DrawSISMessageEx (may be OR'ed):
inline constexpr auto DsmeNone {0};
inline constexpr auto DsmeSetfr {(1 << 0)};
// Set the flash rectangle to the message area.
inline constexpr auto DsmeClearfr {(1 << 1)};
// Disable the flash rectangle.
inline constexpr auto DsmeBlockcur {(1 << 2)};
// Use a block cursor instead of an insertion point cursor,
// when editing in the message field.
inline constexpr auto DsmeMycolor {(1 << 3)};
// Use the current foreground color, instead of the default.
extern bool DrawSISMessageEx(const char* pStr, int16_t CurPos,
							 int16_t ExPos, uint16_t flags);

extern void DrawSISMessage(const char* pStr);
extern void DateToString(char* buf, size_t bufLen,
						 uint8_t month_index, uint8_t day_index, uint16_t year_index);

// Returned GFXRECT is relative to the StatusContext
extern void GetStatusMessageRect(GFXRECT* r);
extern void DrawStatusMessage(const char* pStr);
typedef enum
{
	SMM_UNDEFINED = 0,
	SMM_DATE,
	SMM_RES_UNITS,
	SMM_CREDITS,
	SMM_WARNING,
	SMM_ALERT,

	SMM_DEFAULT = SMM_DATE,
} StatMsgMode;
// Sets the new mode and return the previous
extern StatMsgMode SetStatusMessageMode(StatMsgMode);

extern void DrawLanders(void);
extern void DrawStorageBays(bool Refresh);
extern void GetGaugeRect(GFXRECT* pRect, bool IsCrewRect);
extern void DrawFlagshipStats(void);
void DrawAutoPilotMessage(bool Reset);

extern void DeltaSISGauges(int16_t crew_delta, int32_t fuel_delta, int resunit_delta);

extern uint16_t GetCrewCount(void);
extern uint16_t GetModuleCrewCapacity(uint8_t moduleType);

extern uint16_t GetCrewPodCapacity(void);
extern uint16_t GetCPodCapacity(GFXPOINT* ppt);

extern uint16_t GetModuleStorageCapacity(uint8_t moduleType);
extern uint16_t GetStorageBayCapacity(void);
extern uint16_t GetSBayCapacity(GFXPOINT* ppt);

extern uint32_t GetModuleFuelCapacity(uint8_t moduleType);
extern uint32_t GetFuelTankCapacity(void);

extern uint16_t CountSISPieces(uint8_t piece_type);

extern void DrawFlagshipName(bool InStatusArea, bool NewGame);
extern void DrawCaptainsName(bool NewGame);

extern void DrawFuelInFTanks(bool isOutfit);
extern FRAME hdFuelFrame; // Frame for HdResolutionFactor fuel with smooth gradient

typedef enum
{
	SIS_MAIN_FRAME = 0,
	SIS_STAT_FRAME,
	DEVICE_CARGO_FRAME,
	SIS_STAT_REPAIR_FRAME,
	SIS_STAT_SUMM_FRAME,
	SIS_MSG_FRAME,
	SIS_TITLE_FRAME,
	CAPTAIN_FRAME,
	CAP_NAME_FRAME,
	STAT_MSG_FRAME,
	SIS_RADAR_FRAME,
	TEXT_LABEL_FRAME,
	SIS_REPAIR_FRAME,
	LANDER_3DO_FRAME,
	LANDER_DOS_FRAME,
	SIS_COMM_FRAME,
} BORDER_ORDER;

typedef enum
{
	DOS_MENU_HILITE = 0,
	SPECIAL_BEVEL,
	THIN_INNER_BEVEL = SPECIAL_BEVEL + 8,
	THICK_INNER_BEVEL = THIN_INNER_BEVEL + 8,
	THIN_OUTER_BEVEL = THICK_INNER_BEVEL + 8,
	THICK_OUTER_BEVEL = THIN_OUTER_BEVEL + 8,
	THICK_OUT_INV_BEVEL = THICK_OUTER_BEVEL + 8,
} BEVEL_LEVEL;


#endif /* SIS_H_INCL__ */
