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
#ifndef UQM_SUPERMELEE_MELEE_H
#define UQM_SUPERMELEE_MELEE_H

#include "core/types/enum.h"
#include "../init.h"
#include "libs/gfxlib.h"
#include "libs/mathlib.h"
#include "libs/sndlib.h"
#include "libs/time/timelib.h"
#include "libs/reslib.h"
#include "netplay/packet.h"

namespace uqm
{
// for NetplayAbortReason and NetplayResetReason.

struct MeleeState;

inline constexpr auto NumMeleeRows {2};
inline auto NumMeleeColumns() { return SafeBool(7, 6); }
//#define NumMeleeColumns() 7
inline auto TrueMeleeFleetSize() { return (NumMeleeRows * NumMeleeColumns()); }
inline constexpr auto MeleeFleetSize {14};
inline auto IconWidth() { return ResScale(16); }
inline auto IconHeight() { return ResScale(16); }

extern FrameRef PickMeleeFrame;

inline constexpr auto PickBgColor {BuildColor(MakeRgb15(0x00, 0x01, 0x0F), 0x01)};
inline constexpr auto PickValueColor {BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C)};
// Used for the current fleet value in the next ship selection
// in SuperMelee.

inline constexpr auto MaxTeamChars {30};
inline constexpr auto NumPickCols {5};
inline constexpr auto NumPickRows {5};

enum class MeleeMenuItem
{
#ifdef NETPLAY
	NetTop,
#endif
	ControlsTop,
	SaveTop,
	LoadTop,
	StartMelee,
	LoadBot,
	SaveBot,
	ControlsBot,
	//#ifdef NETPLAY
	//	NET_BOT,
	//#endif
	QuitBot,
	EditMelee, // Editing a fleet or the team name
	BuildPick  // Selecting a ship to add to a fleet
};
consteval bool isArithmeticEnum(MeleeMenuItem) { return true; }

} // namespace uqm

#include "loadmele.h"
#include "meleesetup.h"
#include "meleeship.h"

namespace uqm
{
inline constexpr auto MeleeStateIndexDone {(uint16_t)-1};

struct MeleeState
{
	bool (*InputFunc)(struct MeleeState* pInputState);

	bool Initialized;
	bool meleeStarted;
	MeleeMenuItem MeleeOption;
	uint16_t side;
	uint16_t row;
	uint16_t col;
	MeleeSetup* meleeSetup;
	struct melee_load_state load;
	MeleeShip currentShip;
	// The ship currently displayed. Not really needed.
	// Also the current ship position when selecting a ship.
	uint16_t CurIndex;
	// Current position in the team string when editing it.
	// Set to MeleeStateIndexDone when done.
	bool buildPickConfirmed;
	// Used by DoPickShip () to communicate to the calling
	// function BuildPickShip() whether a ship has been selected
	// to add to the fleet, or whether the operation has been
	// cancelled. If a ship was selected, it is set in
	// currentShip.
	/* RNG state for all local random decisions, i.e. those
			 * decisions that are not shared among network parties. */
	TimeCount LastInputTime;

	MusicRef hMusic;
};

extern void Melee(void);

// Some prototypes for use by loadmele.c:
bool DoMelee(MeleeState* pMS);
enum class MeleeFrameIndex;
void DrawMeleeIcon(uint16_t which_icon, bool HiLite);
void GetShipBox(GfxRect* pRect, uint16_t side, uint16_t row, uint16_t col);
void RepairMeleeFrame(const GfxRect* pRect);
void DrawMeleeShipStrings(MeleeState* pMS, MeleeShip NewStarShip);
extern FrameRef MeleeFrame;
void Melee_flashSelection(MeleeState* pMS);
extern void QuickRepair(uint16_t whichFrame, GfxRect* pRect);

uint16_t GetShipValue(MeleeShip StarShip);

void updateRandomSeed(MeleeState* pMS, uint16_t side, uint32_t seed);
void confirmationCancelled(MeleeState* pMS, uint16_t side);
void connectedFeedback(NetConnection* conn);
void abortFeedback(NetConnection* conn, NetplayAbortReason reason);
void resetFeedback(NetConnection* conn, NetplayResetReason reason,
				   bool byRemote);
void errorFeedback(NetConnection* conn);
void closeFeedback(NetConnection* conn);

bool Melee_LocalChange_ship(MeleeState* pMS, uint16_t side,
							FleetShipIndex index, MeleeShip ship);
bool Melee_LocalChange_teamName(MeleeState* pMS, uint16_t side,
								const char* name);
bool Melee_LocalChange_fleet(MeleeState* pMS, size_t teamNr,
							 const MeleeShip* fleet);
bool Melee_LocalChange_team(MeleeState* pMS, size_t teamNr,
							const MeleeTeam* team);

void Melee_bootstrapSyncTeam(MeleeState* pMS, size_t teamNr);

void Melee_RemoteChange_ship(MeleeState* pMS, NetConnection* conn,
							 uint16_t side, FleetShipIndex index, MeleeShip ship);
void Melee_RemoteChange_teamName(MeleeState* pMS, NetConnection* conn,
								 uint16_t side, const char* name);

void DrawShipPickerText(GfxStamp stamp);

enum class MeleeFrameIndex
{ // meleemenu.ani enumerator
	MeleeBackground = 0,

	HumanConTop,
	WeakCyborgTop,
	GoodCyborgTop,
	AwesCyborgTop,

	HumanConTopHl,
	WeakCyborgTopHl,
	GoodCyborgTopHl,
	AwesCyborgTopHl,

	HumanConBott,
	WeakCyborgBott,
	GoodCyborgBott,
	AwesCyborgBott,

	HumanConBottHl,
	WeakCyborgBottHl,
	GoodCyborgBottHl,
	AwesCyborgBottHl,

	LoadButtonTop,
	SaveButtonTop,
	LoadButtonTopHl,
	SaveButtonTopHl,

	SaveButtonBott,
	LoadButtonBott,
	SaveButtonBottHl,
	LoadButtonBottHl,

	BattleButton,
	BattleButtonHl,

	ShipPickKb,
	ShipPickXb,
	ShipPickPs,

	TeamPickKb,
	TeamPickXb,
	TeamPickPs,

	QuitButton,
	QuitButtonHl,

	NetworkConTop,
	NetworkConTopHl,

	NetworkConBott,
	NetworkConBottHl,

	NetButtonTop,
	NetButtonTopHl,
	NetButtonBott,
	NetButtonBottHl,

	ShipDesc,

	ConfirmPc,
	CancelPc,
	UpDownPc,
	SpecialPc,

	ConfirmXb,
	CancelXb,
	UpDownXb,
	SpecialXb,

	ConfirmPs,
	CancelPs,
	UpDownPs,
	SpecialPs,
};
consteval bool isArithmeticEnum(MeleeFrameIndex) { return true; }

} // namespace uqm

#endif // UQM_SUPERMELEE_MELEE_H
