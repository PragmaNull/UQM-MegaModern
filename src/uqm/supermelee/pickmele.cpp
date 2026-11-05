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

#define PICKMELE_INTERNAL
#include "pickmele.h"

#include "../battlecontrols.h"
#include "../battle.h"
#include "../build.h"
#include "../controls.h"
#include "../flash.h"
#include "../igfxres.h"
#include "../intel.h"
#include "../master.h"
#include "../nameref.h"
#include "melee.h"
#ifdef NETPLAY
#include "netplay/netmelee.h"
#include "netplay/netmisc.h"
#include "netplay/notify.h"
#endif
#include "../races.h"
#include "../setup.h"
#include "../sounds.h"
#include "libs/callback/async.h"
#include "core/log/log.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


static constexpr auto NumPickmeleeRows {2};
static constexpr auto NumPickmeleeColumns {7};

static auto PickXOffs() { return ResScale(57); }
static auto PickYOffs() { return ResScale(24); }
static auto PickSideOffs() { return ResScale(100); }

static auto NameAreaHeight() { return ResScale(7); }
static auto MeleeWidth() { return ResScale(149); }
static auto MeleeHeight() { return (ResScale(48) + NameAreaHeight()); }

static constexpr auto PickshipTeamNameTextColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)};
static constexpr auto PickshipTeamStartValueColor {BuildColor(MakeRgb15(0x04, 0x05, 0x1F), 0x4B)};


#ifdef NETPLAY
static void reportShipSelected(GETMELEE_STATE* gms, uint16_t index);
#endif


FRAME PickMeleeFrame;


static FleetShipIndex PickMelee_GetShipIndex(uint8_t row, uint8_t col)
{
	return row * NumPickmeleeColumns + col;
}

static uint8_t PickMelee_GetShipRow(FleetShipIndex index)
{
	return index / NumPickmeleeColumns;
}

static uint8_t PickMelee_GetShipColumn(int index)
{
	return index % NumPickmeleeColumns;
}

// Returns the <index>th ship in the queue, or 0 if it is not available.
static HSTARSHIP MeleeShipByQueueIndex(const QUEUE* queue, uint16_t index)
{
	HSTARSHIP hShip {};
	HSTARSHIP hNextShip {};

	for (hShip = GetHeadLink(queue); hShip != 0; hShip = hNextShip)
	{
		const STARSHIP* StarShipPtr = LockStarShip(queue, hShip);
		if (StarShipPtr->index == index)
		{
			hNextShip = hShip;
			if (StarShipPtr->SpeciesID == NO_ID)
			{
				hShip = 0;
			}
			UnlockStarShip(queue, hNextShip);
			break;
		}
		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockStarShip(queue, hShip);
	}

	return hShip;
}

// Returns the <index>th available ship in the queue.
static HSTARSHIP MeleeShipByUsedIndex(const QUEUE* queue, uint16_t index)
{
	HSTARSHIP hShip {};
	HSTARSHIP hNextShip {};

	for (hShip = GetHeadLink(queue); hShip != 0; hShip = hNextShip)
	{
		const STARSHIP* StarShipPtr = LockStarShip(queue, hShip);
		if ((StarShipPtr->SpeciesID != NO_ID) && index-- == 0)
		{
			UnlockStarShip(queue, hShip);
			break;
		}
		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockStarShip(queue, hShip);
	}

	return hShip;
}

#if 0
static uint16_t queueIndexFromShip (HSTARSHIP hShip)
{
	uint16_t result;
	STARSHIP *StarShipPtr = LockStarShip (queue, hShip);
	result = StarShipPtr->index;
	UnlockStarShip (queue, hShip);
}
#endif

// Pre: called does not hold the graphics lock
static void PickMelee_ChangedSelection(GETMELEE_STATE* gms, uint16_t playerI)
{
	GFXRECT r;

	r.corner.x = SafeX() + PickXOffs() + ((IconWidth() + ResScale(2)) * gms->player[playerI].col);
	r.corner.y = SafeY() + PickYOffs() + ((IconHeight() + ResScale(2)) * gms->player[playerI].row) + ((1 - playerI) * PickSideOffs());

	r.extent.width = (IconWidth() + ResScale(2));
	r.extent.height = (IconHeight() + ResScale(2));
	Flash_setRect(gms->player[playerI].flashContext, &r);
}

// Only returns false when there is no ship for the choice.
bool setShipSelected(GETMELEE_STATE* gms, uint16_t playerI, uint16_t choice,
					 bool reportNetwork)
{
	HSTARSHIP ship {};

	assert(!gms->player[playerI].done);

	if (choice == (uint16_t)~0)
	{
		// Random ship selection.
		ship = MeleeShipByUsedIndex(&race_q[playerI],
									gms->player[playerI].randomIndex);
	}
	else
	{
		// Explicit ship selection.
		ship = MeleeShipByQueueIndex(&race_q[playerI], choice);
	}

	if (ship == 0)
	{
		return false;
	}

	gms->player[playerI].choice = choice;
	gms->player[playerI].hBattleShip = ship;
	PlayMenuSound(MENU_SOUND_SUCCESS);
#ifdef NETPLAY
	if (reportNetwork)
	{
		reportShipSelected(gms, choice);
	}
#else
	(void)reportNetwork;
#endif
	gms->player[playerI].done = true;
	return true;
}

// Returns false if aborted.
static bool SelectShip_processInput(GETMELEE_STATE* gms, uint16_t playerI,
									BATTLE_INPUT_STATE inputState)
{
	if (inputState & BattleWeapon)
	{
		if (gms->player[playerI].col == NumPickmeleeColumns && gms->player[playerI].row == 0)
		{
			// Random ship
			(void)setShipSelected(gms, playerI, (uint16_t)~0, true);
		}
		else if (gms->player[playerI].col == NumPickmeleeColumns && gms->player[playerI].row == 1)
		{
			// Selected exit
			if (ConfirmExit())
			{
				return false;
			}
		}
		else
		{
			// Selection is on a ship slot.
			const uint16_t slotNr = PickMelee_GetShipIndex(gms->player[playerI].row,
														   gms->player[playerI].col);
			(void)setShipSelected(gms, playerI, slotNr, true);
			// If the choice is not valid, setShipSelected()
			// will not set .done.
		}
	}
	else
	{
		// Process motion commands.

		uint16_t new_row {gms->player[playerI].row};
		uint16_t new_col {gms->player[playerI].col};
		if (inputState & BattleLeft)
		{
			if (new_col-- == 0)
			{
				new_col = NumPickmeleeColumns;
			}
		}
		else if (inputState & BattleRight)
		{
			if (new_col++ == NumPickmeleeColumns)
			{
				new_col = 0;
			}
		}
		if (inputState & BattleThrust)
		{
			if (new_row-- == 0)
			{
				new_row = NumPickmeleeRows - 1;
			}
		}
		else if (inputState & BattleDown)
		{
			if (++new_row == NumPickmeleeRows)
			{
				new_row = 0;
			}
		}

		if (new_row != gms->player[playerI].row || new_col != gms->player[playerI].col)
		{
			gms->player[playerI].row = new_row;
			gms->player[playerI].col = new_col;

			PlayMenuSound(MENU_SOUND_MOVE);
			PickMelee_ChangedSelection(gms, playerI);
		}
	}

	return true;
}

bool selectShipHuman(HumanInputContext* context, GETMELEE_STATE* gms)
{
	const BATTLE_INPUT_STATE inputState =
		PulsedInputToBattleInput(context->playerNr);

	return SelectShip_processInput(gms, context->playerNr, inputState);
}

static constexpr auto ComputerSelectionDelay {(c_gameSecond / 2)};

bool selectShipComputer(ComputerInputContext* context, GETMELEE_STATE* gms)
{
	const TimeCount now = GetTimeCounter();
	if (now < gms->player[context->playerNr].timeIn + ComputerSelectionDelay)
	{
		return true;
	}

	return SelectShip_processInput(gms, context->playerNr, BattleWeapon);
	// Simulate selection of the random choice button.
}

#ifdef NETPLAY
bool selectShipNetwork(NetworkInputContext* context, GETMELEE_STATE* gms)
{
	flushPacketQueues();
	// Sets gms->player[context->playerNr].remoteSelected if input
	// is received.
	if (gms->player[context->playerNr].remoteSelected)
	{
		gms->player[context->playerNr].done = true;
	}

	return true;
}
#endif

// Select a new ship from the fleet for battle.
// Returns 'true' if no choice has been made yet; this function is to be
// called again later.
// Returns 'false' if a choice has been made. gms->hStarShip is set
// to the chosen (or randomly selected) ship, or to 0 if 'exit' has
// been chosen.
/* TODO: Include player timeouts */
static bool DoGetMelee(GETMELEE_STATE* gms)
{
	bool done {};
	uint16_t playerI {};

	SetMenuSounds(MenuSoundNone, MenuSoundNone);

	if (!gms->Initialized)
	{
		gms->Initialized = true;
		return true;
	}

	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		if (!gms->player[playerI].selecting)
		{
			continue;
		}

		if (!gms->player[playerI].done)
		{
			Flash_process(gms->player[playerI].flashContext);
		}
	}

	SleepThread(c_gameSecond / 120);

#ifdef NETPLAY
	netInput();

	if (!allConnected())
	{
		goto aborted;
	}
#endif

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		goto aborted;
	}

	done = true;
	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		if (!gms->player[playerI].selecting)
		{
			continue;
		}

		if (!gms->player[playerI].done)
		{
			if (!PlayerInput[playerI]->handlers->selectShip(
					PlayerInput[playerI], gms))
			{
				goto aborted;
			}

			if (gms->player[playerI].done)
			{
				Flash_terminate(gms->player[playerI].flashContext);
				gms->player[playerI].flashContext = nullptr;
			}
			else
			{
				done = false;
			}
		}
	}

#ifdef NETPLAY
	flushPacketQueues();
#endif
	return !done;

aborted:
#ifdef NETPLAY
	flushPacketQueues();
#endif
	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		if (!gms->player[playerI].selecting)
		{
			continue;
		}

		gms->player[playerI].choice = 0;
		gms->player[playerI].hBattleShip = 0;
	}
	uqm::currentActivity() &= ~CHECK_ABORT;
	return false;
}

static uint16_t GetRaceQueueValue(const QUEUE* queue)
{
	HSTARSHIP hBattleShip {};
	HSTARSHIP hNextShip {};

	uint16_t result {0};
	for (hBattleShip = GetHeadLink(queue);
		 hBattleShip != 0; hBattleShip = hNextShip)
	{
		const STARSHIP* StarShipPtr = LockStarShip(queue, hBattleShip);
		hNextShip = _GetSuccLink(StarShipPtr);

		if (StarShipPtr->SpeciesID == NO_ID)
		{
			continue; // Not active any more.
		}

		result += StarShipPtr->ship_cost;

		UnlockStarShip(queue, hBattleShip);
	}

	return result;
}

// Cross out the icon for the dead ship.
// 'frame' is the PickMeleeFrame for the player.
// 'shipI' is the index in the ship list.
// Pre: caller holds the graphics lock.
static void CrossOutShip(FRAME frame, uint16_t shipNr)
{
	STAMP s;
	const uint8_t row = PickMelee_GetShipRow(shipNr);
	const uint8_t col = PickMelee_GetShipColumn(shipNr);

	GFXCONTEXT OldContext {SetContext(OffScreenContext)};

	SetContextFGFrame(frame);

	s.origin.x = ResScale(3) + ((IconWidth() + ResScale(2)) * col);
	s.origin.y = ResScale(9) + ((IconHeight() + ResScale(2)) * row);
	s.frame = SetAbsFrameIndex(StatusFrame, 3);
	// Cross for through the ship image.
	DrawStamp(&s);

	SetContext(OldContext);
}

// Draw the value of the fleet in the top right of the PickMeleeFrame.
// Pre: caller holds the graphics lock.
static void UpdatePickMeleeFleetValue(FRAME frame, uint16_t which_player)
{
	GFXRECT r;
	TEXT t;

	uint16_t value {GetRaceQueueValue(&race_q[which_player])};

	GFXCONTEXT OldContext {SetContext(OffScreenContext)};
	SetContextFGFrame(frame);

	// Erase the old value text.
	GetFrameRect(frame, &r);
	r.extent.width -= ResScale(4);
	t.baseline.x = r.extent.width;
	r.corner.x = r.extent.width - ResScale(6 * 3);
	r.corner.y = ResScale(2);
	r.extent.width = ResScale(6 * 3);
	r.extent.height = ResScale(7 - 2);
	SetContextForeGroundColor(PickBgColor);
	DrawFilledRectangle(&r);

	// Draw the new value text.
	char buf[40] {};
	fmt::format_to_n(buf, sizeof(buf) - 1, "{}", value);
	t.baseline.y = ResScale(7);
	t.align = ALIGN_RIGHT;
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}
	SetContextForeGroundColor(PickValueColor);
	font_DrawText(&t);

	SetContext(OldContext);
}

// Create a frame for each player to display their current fleet in,
// to be used when selecting the next ship to fight with.
void BuildPickMeleeFrame(void)
{
	STAMP s;
	GFXCONTEXT OldContext = SetContext(OffScreenContext);

	if (PickMeleeFrame)
	{
		DestroyDrawable(ReleaseDrawable(PickMeleeFrame));
	}

	PickMeleeFrame = CaptureDrawable(CreateDrawable(
		WantPixmap, MeleeWidth(), MeleeHeight(), 2));
	s.origin.x = 0;
	s.origin.y = 0;

	s.frame = CaptureDrawable(LoadGraphic(MeleePickMaskPmapAnimRes));
	SetContextFGFrame(PickMeleeFrame);
	DrawStamp(&s);

	s.frame = IncFrameIndex(s.frame);
	SetContextFGFrame(IncFrameIndex(PickMeleeFrame));
	DrawStamp(&s);

	DestroyDrawable(ReleaseDrawable(s.frame));

	SetContext(OldContext);
}

// Put the ship icons in the PickMeleeFrame, and create a queue
// for each player.
// XXX TODO: split off creating the queue into a separate function.
void FillPickMeleeFrame(MeleeSetup* setup)
{
	GFXCONTEXT OldContext {SetContext(OffScreenContext)};

	for (uint16_t i {0}; i < NumSides; ++i)
	{
		GFXRECT r;
		TEXT t;
		STAMP s;

		const uint16_t sideI {GetPlayerOrder(i)};
		const uint16_t side {!sideI};

		s.frame = SetAbsFrameIndex(PickMeleeFrame, side);
		SetContextFGFrame(s.frame);

		GetFrameRect(s.frame, &r);
		t.baseline.x = r.extent.width >> 1;
		t.baseline.y = r.extent.height - NameAreaHeight() + ResScale(4);

		r.corner.x += ResScale(2);
		r.corner.y += ResScale(2);
		r.extent.width -= ResScale((2 * 2) + (ResDescale(IconWidth()) + 2) + 1);
		r.extent.height -= ResScale(2 * 2) + NameAreaHeight();
		SetContextForeGroundColor(PickBgColor);
		DrawFilledRectangle(&r);

		r.corner.x += ResScale(2);
		r.extent.width += ResScale((ResDescale(IconWidth()) + 2) - (2 * 2));
		r.corner.y += r.extent.height;
		r.extent.height = NameAreaHeight();
		DrawFilledRectangle(&r);

		// Team name at the bottom of the frame:
		t.align = ALIGN_CENTER;
		t.pStr = MeleeSetup_getTeamName(setup, sideI);
		t.CharCount = (uint16_t)~0;
		if (isPC(uqm::UQMOptions::read().whichFonts))
		{
			SetContextFont(TinyFont);
		}
		else
		{
			SetContextFont(TinyFontBold);
		}
		SetContextForeGroundColor(PickshipTeamNameTextColor);
		font_DrawText(&t);

		char buf[30] {};
		// Total team value of the starting team:
		fmt::format_to_n(buf, sizeof(buf) - 1, "{}", MeleeSetup_getFleetValue(setup, sideI));
		t.baseline.x = ResScale(4);
		t.baseline.y = ResScale(7);
		t.align = ALIGN_LEFT;
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
		SetContextForeGroundColor(PickshipTeamStartValueColor);
		font_DrawText(&t);

		assert(CountLinks(&race_q[side]) == 0);

		for (FleetShipIndex index {0}; index < MeleeFleetSize; index++)
		{
			MeleeShip StarShip;

			StarShip = MeleeSetup_getShip(setup, sideI, index);
			if (StarShip == MELEE_NONE)
			{
				continue;
			}

			{
				uint8_t ship_cost {};
				uint8_t captains_name_index {};

				HMASTERSHIP hMasterShip {GetStarShipFromIndex(&master_q, StarShip)};
				const MASTER_SHIP_INFO* MasterPtr {LockMasterShip(&master_q, hMasterShip)};

				captains_name_index = NameCaptain(&race_q[side],
												  MasterPtr->SpeciesID);
				HSTARSHIP hBuiltShip {Build(&race_q[side], MasterPtr->SpeciesID)};

				// Draw the icon.
				const uint8_t row {PickMelee_GetShipRow(index)};
				const uint8_t col {PickMelee_GetShipColumn(index)};
				s.origin.x = ResScale(4) + ((IconWidth() + ResScale(2)) * col);
				s.origin.y = ResScale(10) + ((IconHeight() + ResScale(2)) * row);
				s.frame = MasterPtr->ShipInfo.icons;
				DrawStamp(&s);

				ship_cost = MasterPtr->ShipInfo.ship_cost;
				UnlockMasterShip(&master_q, hMasterShip);

				STARSHIP* BuiltShipPtr {LockStarShip(&race_q[side], hBuiltShip)};
				BuiltShipPtr->index = index;
				BuiltShipPtr->ship_cost = ship_cost;
				BuiltShipPtr->playerNr = side;
				BuiltShipPtr->captains_name_index = captains_name_index;
				// The next ones are not used in Melee
				BuiltShipPtr->crew_level = 0;
				BuiltShipPtr->max_crew = 0;
				BuiltShipPtr->race_strings = 0;
				BuiltShipPtr->icons = 0;
				BuiltShipPtr->RaceDescPtr = 0;
				UnlockStarShip(&race_q[side], hBuiltShip);
			}
		}
	}

	SetContext(OldContext);
}

void DestroyPickMeleeFrame(void)
{
	DestroyDrawable(ReleaseDrawable(PickMeleeFrame));
	PickMeleeFrame = 0;
}

// Pre: caller holds the graphics lock.
static void DrawPickMeleeFrame(uint16_t which_player)
{
	STAMP s;

	GFXCONTEXT oldContext {SetContext(SpaceContext)};
	s.frame = SetAbsFrameIndex(PickMeleeFrame, which_player);
	s.origin.x = PickXOffs() - ResScale(3);
	s.origin.y = PickYOffs() - ResScale(9) + ((1 - which_player) * PickSideOffs());
	DrawStamp(&s);
	// Draw the selection box to screen.

	SetContext(oldContext);
}

// Pre: caller holds the graphics lock.
void MeleeGameOver(void)
{
	TimeCount TimeOut;
	bool ButtonState {};

	// Show the battle result.
	for (uint16_t playerI {0}; playerI < NumPlayers; playerI++)
	{
		DrawPickMeleeFrame(playerI);
	}


#ifdef NETPLAY
	negotiateReadyConnections(true, NetState_inSetup);
#endif

	TimeOut = GetTimeCounter() + (c_gameSecond * 4);

	bool PressState {PulsedInputState.menu[KEY_MENU_SELECT] || PulsedInputState.menu[KEY_MENU_CANCEL]};
	do
	{
		UpdateInputState();
		ButtonState = PulsedInputState.menu[KEY_MENU_SELECT] || PulsedInputState.menu[KEY_MENU_CANCEL];
		if (PressState)
		{
			PressState = ButtonState;
			ButtonState = false;
		}

		Async_process();
		TaskSwitch();
	} while (!(uqm::currentActivity() & CHECK_ABORT)
			 && (!ButtonState
				 && (!testFlag((PlayerControl[0] & PlayerControl[1]), PlayerControlFlags::Psytron)
					 || GetTimeCounter() < TimeOut)));
}

void MeleeShipDeath(STARSHIP* ship)
{
	// Deactivate fleet position.
	ship->SpeciesID = NO_ID;

	FRAME frame {SetAbsFrameIndex(PickMeleeFrame, ship->playerNr)};
	CrossOutShip(frame, ship->index);
	UpdatePickMeleeFleetValue(frame, ship->playerNr);
}

// Post: the NetState for all players is NetState_interBattle
static bool GetMeleeStarShips(uint16_t playerMask, HSTARSHIP* ships)
{
	uint16_t playerI {};
	GETMELEE_STATE gmstate;
	TimeCount now;

#ifdef NETPLAY
	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		if ((playerMask & (1 << playerI)) == 0)
		{
			continue;
		}

		// XXX: This does not have to be done per connection.
		const NetConnection* conn {netConnections[playerI]};
		if (conn != nullptr)
		{
			BattleStateData* battleStateData {};
			battleStateData =
				(BattleStateData*)NetConnection_getStateData(conn);
			battleStateData->getMeleeState = &gmstate;
		}
	}
#endif

	bool ok {true};

	now = GetTimeCounter();
	gmstate.InputFunc = DoGetMelee;
	gmstate.Initialized = false;
	for (uint16_t i {0}; i < NumPlayers; ++i)
	{
		// We have to use uqm::random().next() results in specific order
		playerI = GetPlayerOrder(i);
		gmstate.player[playerI].selecting =
			(playerMask & (1 << playerI)) != 0;
		gmstate.player[playerI].ships_left = battle_counter[playerI];

		// We determine in advance which ship would be chosen if the player
		// wants a random ship, to keep it simple to keep network parties
		// synchronised.
		gmstate.player[playerI].randomIndex =
			(uint16_t)uqm::random().next() % gmstate.player[playerI].ships_left;
		gmstate.player[playerI].done = false;

		if (!gmstate.player[playerI].selecting)
		{
			continue;
		}

		gmstate.player[playerI].timeIn = now;
		gmstate.player[playerI].row = 0;
		gmstate.player[playerI].col = NumPickmeleeColumns;
#ifdef NETPLAY
		gmstate.player[playerI].remoteSelected = false;
#endif

		gmstate.player[playerI].flashContext =
			Flash_createHighlight(ScreenContext, nullptr);
		Flash_setMergeFactors(gmstate.player[playerI].flashContext,
							  2, 3, 2);
		Flash_setFrameTime(gmstate.player[playerI].flashContext,
						   c_gameSecond / 16);
		Flash_setPulseBox(gmstate.player[playerI].flashContext,
						  (bool)isPC(uqm::UQMOptions::read().whichMenu));
#ifdef NETPLAY
		if (testFlag(PlayerControl[playerI], PlayerControlFlags::Network))
		{
			Flash_setSpeed(gmstate.player[playerI].flashContext,
						   c_gameSecond / 2, TimePeriod {}, c_gameSecond / 2, TimePeriod {});
		}
		else
#endif
		{
			Flash_setSpeed(gmstate.player[playerI].flashContext, TimePeriod {}, c_gameSecond / 16, TimePeriod {}, c_gameSecond / 16);
		}
		PickMelee_ChangedSelection(&gmstate, playerI);
		Flash_start(gmstate.player[playerI].flashContext);
	}

#ifdef NETPLAY
	{
		// NB. gmstate.player[].randomIndex and gmstate.player[].done must
		// be initialised before negotiateReadyConnections is completed, to
		// ensure that they are initialised when the SelectShip packet
		// arrives.
		const bool allOk = negotiateReadyConnections(true, NetState_selectShip);
		if (!allOk)
		{
			// Some network connection has been reset.
			ok = false;
		}
	}
#endif
	SetDefaultMenuRepeatDelay();

	SetContext(OffScreenContext);


	DoInput(&gmstate, false);
	WaitForSoundEnd(0);


	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		if (!gmstate.player[playerI].selecting)
		{
			continue;
		}

		if (gmstate.player[playerI].done)
		{
			// Flash rectangle is already terminated.
			ships[playerI] = gmstate.player[playerI].hBattleShip;
		}
		else
		{
			Flash_terminate(gmstate.player[playerI].flashContext);
			gmstate.player[playerI].flashContext = nullptr;
			ok = false;
		}
	}

#ifdef NETPLAY
	if (ok)
	{
		if (!negotiateReadyConnections(true, NetState_interBattle))
		{
			ok = false;
		}
	}
	else
	{
		setStateConnections(NetState_interBattle);
	}
#endif

	if (!ok)
	{
		// Aborting.
		uqm::currentActivity() &= ~IN_BATTLE;
	}

#ifdef NETPLAY
	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		if ((playerMask & (1 << playerI)) == 0)
		{
			continue;
		}

		// XXX: This does not have to be done per connection.
		const NetConnection* conn {netConnections[playerI]};
		if (conn != nullptr && NetConnection_isConnected(conn))
		{
			BattleStateData* battleStateData {};
			battleStateData =
				(BattleStateData*)NetConnection_getStateData(conn);
			battleStateData->getMeleeState = nullptr;
		}
	}
#endif

	return ok;
}

bool GetInitialMeleeStarShips(HSTARSHIP* result)
{
	uint16_t playerI {};
	uint16_t playerMask {};

	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		FRAME frame {SetAbsFrameIndex(PickMeleeFrame, playerI)};
		UpdatePickMeleeFleetValue(frame, playerI);
		DrawPickMeleeFrame(playerI);
	}

	// Fade in
	SleepThreadUntil(FadeScreen(FadeAllToColor, c_gameSecond / 2)
					 + c_gameSecond / 60);
	FlushColorXForms();

	playerMask = 0;
	for (playerI = 0; playerI < NumPlayers; playerI++)
	{
		playerMask |= (1 << playerI);
	}

	return GetMeleeStarShips(playerMask, result);
}

// Get the next ship to use in SuperMelee.
bool GetNextMeleeStarShip(uint16_t which_player, HSTARSHIP* result)
{
	uint16_t playerMask {};
	HSTARSHIP ships[NumPlayers];

	DrawPickMeleeFrame(which_player);

	playerMask = 1 << which_player;
	const bool ok {GetMeleeStarShips(playerMask, ships)};
	if (ok)
	{
		*result = ships[which_player];
	}

	return ok;
}

#ifdef NETPLAY
// Called when a ship selection has arrived from a remote player.
bool updateMeleeSelection(GETMELEE_STATE* gms, uint16_t playerI, uint16_t ship)
{
	if (gms == nullptr || !gms->player[playerI].selecting || gms->player[playerI].done)
	{
		// This happens when we get an update message from a connection
		// for who we are not selecting a ship.
		uqm::log::warn("Unexpected ship selection packet "
					   "received.\n");
		return false;
	}

	if (!setShipSelected(gms, playerI, ship, false))
	{
		uqm::log::warn("Invalid ship selection received from remote "
					   "party.\n");
		return false;
	}

	gms->player[playerI].remoteSelected = true;
	return true;
}

static void reportShipSelected(GETMELEE_STATE* gms, uint16_t index)
{
	for (size_t playerI {0}; playerI < NumPlayers; playerI++)
	{
		NetConnection* conn = netConnections[playerI];

		if (conn == nullptr)
		{
			continue;
		}

		if (!NetConnection_isConnected(conn))
		{
			continue;
		}

		Netplay_Notify_shipSelected(conn, index);
	}
	(void)gms;
}
#endif
