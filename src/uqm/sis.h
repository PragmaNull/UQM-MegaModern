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
#ifndef UQM_SIS_H
#define UQM_SIS_H

#include "core/types/enum.h"
#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"
#include "planets/elemdata.h" // for NUM_ELEMENT_CATEGORIES
#include "units.h"			  // for RESOLUTION_FACTOR
#include "options/OptionDefs.h"


inline constexpr auto ClearSisRadar {1 << 2};
inline constexpr auto DrawSisDisplay {1 << 3};

inline constexpr auto UndefinedDelta {0x7FFF};

inline constexpr auto NumDriveSlots {11};
inline constexpr auto NumJetSlots {8};
inline constexpr auto NumModuleSlots {16};

inline constexpr auto CrewPodCapacity {50};
inline constexpr auto StorageBayCapacity {500}; /* km cubed */
inline constexpr auto FuelTankScale {100};
inline constexpr auto FuelTankCapacity {50 * FuelTankScale};
inline constexpr auto HefuelTankCapacity {100 * FuelTankScale};
inline constexpr auto ModuleCostScale {50};

inline constexpr auto CrewExpenseThreshold {1000};

inline constexpr auto CrewPerRow {5};
inline constexpr auto SbayMassPerRow {50};

inline constexpr auto MaxFuelBars {10};
inline constexpr auto FuelVolumePerRow {HefuelTankCapacity / MaxFuelBars};
inline constexpr auto FuelReserve {10 * FuelTankScale};
inline constexpr auto FuelCostRu {20};

inline constexpr auto IpShipThrustIncrement {8};
inline constexpr auto IpShipTurnWait {17};
inline constexpr auto IpShipTurnDecrement {2};

inline constexpr auto BioCreditValueDefault {2};

enum class ModuleType : uint8_t
{
	PlanetLander = 0,
	/* thruster types */
	FusionThruster,
	/* jet types */
	TurningJets,
	/* module types */
	CrewPod,
	StorageBay,
	FuelTank,
	HigheffFuelsys,
	DynamoUnit,
	ShivaFurnace,
	GunWeapon,
	BlasterWeapon,
	CannonWeapon,
	TrackingSystem,
	AntimissileDefense,

	NumPurchaseModules,

	BombModule0 = NumPurchaseModules,
	BombModule1,
	BombModule2,
	BombModule3,
	BombModule4,
	BombModule5,

	NumModules, /* must be last module */

	// The values of empty slots.
	EmptyDriveSlot = NumModules,
	EmptyJetSlot,
	EmptyModuleSlot,
	EmptyLanderSlot,
};
consteval bool isArithmeticEnum(ModuleType) { return true; }

inline constexpr auto EmptySlot {ModuleType::NumModules};
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

inline constexpr GfxPoint SupportShipPts[MaxBuiltShips] {
	{3 + 0,	30 + (2 * 16)},
	{3 + 42, 30 + (2 * 16)},
	{3 + 0,	30 + (3 * 16)},
	{3 + 42, 30 + (3 * 16)},
	{3 + 0,	30 + (1 * 16)},
	{3 + 42, 30 + (1 * 16)},
	{3 + 0,	30 + (4 * 16)},
	{3 + 42, 30 + (4 * 16)},
	{3 + 0,	30 + (0 * 16)},
	{3 + 42, 30 + (0 * 16)},
	{3 + 0,	30 + (5 * 16)},
	{3 + 42, 30 + (5 * 16)},
};

inline constexpr auto LanderDosX {206};
inline constexpr auto LanderDosY {59};

inline constexpr GfxPoint LanderDosPts[MaxLanders] {
	{LanderDosX,		 LanderDosY	   },
	{LanderDosX + 14, LanderDosY - 6 },
	{LanderDosX + 14, LanderDosY + 9 },
	{LanderDosX,		 LanderDosY - 15},
	{LanderDosX,		 LanderDosY + 15},
	{LanderDosX - 14, LanderDosY - 9 },
	{LanderDosX - 14, LanderDosY + 6 },
	{LanderDosX + 14, LanderDosY - 21},
	{LanderDosX + 14, LanderDosY + 24},
	{LanderDosX - 28, LanderDosY - 3 },
};

inline constexpr auto SisNameSize {32};

struct FlagshipData
{
	int32_t log_x, log_y;

	uint32_t ResUnits;

	uint32_t FuelOnBoard;
	uint16_t CrewEnlisted;
	// Number of crew on board, not counting the captain.
	// Set to (uint16_t) ~0 to indicate game over.
	uint16_t TotalElementMass, TotalBioMass;

	ModuleType ModuleSlots[NumModuleSlots];
	ModuleType DriveSlots[NumDriveSlots];
	ModuleType JetSlots[NumJetSlots];

	uint8_t NumLanders;

	uint16_t ElementAmounts[*MineralCategory::NumElementCategories];

	char ShipName[SisNameSize];
	char CommanderName[SisNameSize];
	char PlanetName[SisNameSize];
	uqm::Difficulty Difficulty;
	uint8_t Extended;
	uqm::NomadMode Nomad;
	uint8_t ShipSeed;
	int32_t Seed;
	uint8_t SaveVersion;
};

inline constexpr auto OverrideLanderFlags {1 << 7};
inline constexpr auto AfterBombInstalled {1 << 7};

extern void RepairSISBorder(void);
extern void InitSISContexts(void);
extern void DrawSISFrame(void);
extern void ClearSISRect(uint8_t ClearFlags);
extern void SetFlashRect(const GfxRect* pRect, bool purple);
extern void SetAdditionalRect(const GfxRect* pRect, uint16_t number);
extern void DumpAdditionalRect(void);
extern void PreUpdateFlashRect(void);
extern void PostUpdateFlashRect(void);
extern bool PauseFlash(void);
extern void ContinueFlash(void);

inline auto SfrMenu3do() { return ((GfxRect*)~0L); }
inline auto SfrMenuAny() { return ((GfxRect*)~1L); }
inline auto SfrMenuNon() { return ((GfxRect*)~2L); }
extern void DrawHyperCoords(GfxPoint puniverse);
extern void DrawSaveInfo(FlagshipData SisState);
extern void DrawSISTitle(const char* pStr);

// Flags for DrawSISMessageEx (may be OR'ed):
inline constexpr auto DsmeNone {0};
inline constexpr auto DsmeSetfr {1 << 0};
// Set the flash rectangle to the message area.
inline constexpr auto DsmeClearfr {1 << 1};
// Disable the flash rectangle.
inline constexpr auto DsmeBlockcur {1 << 2};
// Use a block cursor instead of an insertion point cursor,
// when editing in the message field.
inline constexpr auto DsmeMycolor {1 << 3};
// Use the current foreground color, instead of the default.
extern bool DrawSISMessageEx(const char* pStr, int16_t CurPos,
							 int16_t ExPos, uint16_t flags);

extern void DrawSISMessage(const char* pStr);
extern void DateToString(char* buf, size_t bufLen,
						 uint8_t month_index, uint8_t day_index, uint16_t year_index);

// Returned GfxRect is relative to the StatusContext
extern void GetStatusMessageRect(GfxRect* r);
extern void DrawStatusMessage(const char* pStr);
enum class StatMsgMode
{
	Undefined = 0,
	Date,
	ResUnits,
	Credits,
	Warning,
	Alert,

	Default = Date,
};
// Sets the new mode and return the previous
extern StatMsgMode SetStatusMessageMode(StatMsgMode);

extern void DrawLanders(void);
extern void DrawStorageBays(bool Refresh);
extern void GetGaugeRect(GfxRect* pRect, bool IsCrewRect);
extern void DrawFlagshipStats(void);
void DrawAutoPilotMessage(bool Reset);

extern void DeltaSISGauges(int16_t crew_delta, int32_t fuel_delta, int resunit_delta);

extern uint16_t GetCrewCount(void);
extern uint16_t GetModuleCrewCapacity(uint8_t moduleType);

extern uint16_t GetCrewPodCapacity(void);
extern uint16_t GetCPodCapacity(GfxPoint* ppt);

extern uint16_t GetModuleStorageCapacity(uint8_t moduleType);
extern uint16_t GetStorageBayCapacity(void);
extern uint16_t GetSBayCapacity(GfxPoint* ppt);

extern uint32_t GetModuleFuelCapacity(uint8_t moduleType);
extern uint32_t GetFuelTankCapacity(void);

extern uint16_t CountSISPieces(uint8_t piece_type);

extern void DrawFlagshipName(bool InStatusArea, bool NewGame);
extern void DrawCaptainsName(bool NewGame);

extern void DrawFuelInFTanks(bool isOutfit);
extern FRAME hdFuelFrame; // Frame for HdResolutionFactor fuel with smooth gradient

enum class BorderFrameIndex
{
	SisMain = 0,
	SisStat,
	DeviceCargo,
	SisStatRepair,
	SisStatSumm,
	SisMsg,
	SisTitle,
	Captain,
	CapName,
	StatMsg,
	SisRadar,
	TextLabel,
	SisRepair,
	Lander3do,
	LanderDos,
	SisComm,
};
consteval bool isArithmeticEnum(BorderFrameIndex) { return true; }

enum class BevelLevel
{
	DosMenuHilite = 0,
	SpecialBevel,
	ThinInnerBevel = SpecialBevel + 8,
	ThickInnerBevel = ThinInnerBevel + 8,
	ThinOuterBevel = ThickInnerBevel + 8,
	ThickOuterBevel = ThinOuterBevel + 8,
	ThickOutInvBevel = ThickOuterBevel + 8,
};
consteval bool isArithmeticEnum(BevelLevel) { return true; }


#endif // UQM_SIS_H
