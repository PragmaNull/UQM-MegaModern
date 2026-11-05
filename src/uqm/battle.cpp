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

#include "battle.h"

#include "battlecontrols.h"
#include "controls.h"
#include "init.h"
#include "element.h"
#include "ship.h"
#include "process.h"
#include "tactrans.h"
// for flee_preprocess()
#include "intel.h"
#ifdef NETPLAY
#include "supermelee/netplay/netmelee.h"
#ifdef NETPLAY_CHECKSUM
#include "supermelee/netplay/checksum.h"
#endif
#include "supermelee/netplay/notifyall.h"
#endif
#include "supermelee/pickmele.h"
#include "resinst.h"
#include "nameref.h"
#include "setup.h"
#include "settings.h"
#include "sounds.h"
#include "libs/callback/async.h"
#include "libs/graphics/gfx_common.h"
#include "core/log/log.h"
#include "libs/mathlib.h"
#include "globdata.h"
#include "libs/input/sdl/vcontrol.h"
#include "hyper.h"
#include "uqm/random/UQMRandom.h"
// for EraseRadar()


uint8_t battle_counter[NumSides];
// The number of ships still available for battle to each side.
// A ship that has warped out is no longer available.
bool instantVictory;
size_t battleInputOrder[NumSides];
// Indices of the sides in the order their input is processed.
// Network sides are last so that the sides will never be waiting
// on eachother, and games with a 0 frame delay are theoretically
// possible.
#ifdef NETPLAY
BattleFrameCounter battleFrameCount;
// Used for synchronisation purposes during netplay.
#endif

bool RunAwayAllowed(void)
{
	return ((uqm::lowByte(uqm::currentActivity()) == IN_ENCOUNTER
			 || uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE)
			&& (getNomadMode() == uqm::NomadMode::Easy
				|| GET_GAME_STATE(STARBASE_AVAILABLE))
			&& (isDifficulty(uqm::Difficulty::Easy) || !GET_GAME_STATE(BOMB_CARRIER)));
}

static void
DoRunAway(STARSHIP* StarShipPtr)
{
	ELEMENT* ElementPtr {};

	LockElement(StarShipPtr->hShip, &ElementPtr);
	if (GetPrimType(&DisplayArray[ElementPtr->PrimIndex]) == STAMP_PRIM
		&& ElementPtr->life_span == NormalLife
		&& !(ElementPtr->state_flags & FiniteLife)
		&& ElementPtr->mass_points != MaxShipMass * 10
		&& !(ElementPtr->state_flags & Appearing))
	{
		battle_counter[0]--;

		ElementPtr->turn_wait = 3;
		ElementPtr->thrust_wait = 4;
		ElementPtr->colorCycleIndex = 0;
		ElementPtr->preprocess_func = flee_preprocess;
		ElementPtr->mass_points = MaxShipMass * 10;
		ZeroVelocityComponents(&ElementPtr->velocity);
		StarShipPtr->cur_status_flags &=
			~(ShipAtMaxSpeed | ShipBeyondMaxSpeed);

		SetPrimColor(&DisplayArray[ElementPtr->PrimIndex],
					 BuildColor(MakeRgb15(0x0B, 0x00, 0x00), 0x2E));
		// XXX: I think this is supposed to be the same as the
		// first entry of the color cycle table in flee_preeprocess,
		// but it is slightly different (0x0A as red value). - SvdB.
		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], STAMPFILL_PRIM);

		StarShipPtr->ship_input_state = 0;
	}
	UnlockElement(StarShipPtr->hShip);
}

static void
setupBattleInputOrder(void)
{
	size_t i {};

#ifndef NETPLAY
	for (i = 0; i < NumSides; i++)
	{
		battleInputOrder[i] = i;
	}
#else
	int j {};

	i = 0;
	// First put the locally controlled players in the array.
	for (j = 0; j < NumSides; j++)
	{
		if (!testFlag(PlayerControl[j], PlayerControlFlags::Network))
		{
			battleInputOrder[i] = j;
			i++;
		}
	}

	// Next put the network controlled players in the array.
	for (j = 0; j < NumSides; j++)
	{
		if (testFlag(PlayerControl[j], PlayerControlFlags::Network))
		{
			battleInputOrder[i] = j;
			i++;
		}
	}
#endif
}

BATTLE_INPUT_STATE
frameInputHuman(HumanInputContext* context, STARSHIP* StarShipPtr)
{
	(void)StarShipPtr;
	return CurrentInputToBattleInput(context->playerNr);
}

static void
ProcessInput(void)
{
#ifdef NETPLAY
	netInput();
#endif

	const bool CanRunAway {RunAwayAllowed()};

	for (size_t sideI {0}; sideI < NumSides; sideI++)
	{
		HSTARSHIP hBattleShip {};
		HSTARSHIP hNextShip {};
		const size_t cur_player = battleInputOrder[sideI];

		for (hBattleShip = GetHeadLink(&race_q[cur_player]);
			 hBattleShip != 0; hBattleShip = hNextShip)
		{
			BATTLE_INPUT_STATE InputState {};

			STARSHIP* StarShipPtr {LockStarShip(&race_q[cur_player], hBattleShip)};
			hNextShip = _GetSuccLink(StarShipPtr);

			if (StarShipPtr->hShip)
			{
				// TODO: review and see if we have to do this every frame, or
				//   if we can do this once somewhere
				StarShipPtr->control = PlayerControl[cur_player];

				InputState = PlayerInput[cur_player]->handlers->frameInput(
					PlayerInput[cur_player], StarShipPtr);

#ifdef NETPLAY
				if (!testFlag(PlayerControl[cur_player], PlayerControlFlags::Network))
				{
					BattleInputBuffer* bib = getBattleInputBuffer(cur_player);
					Netplay_NotifyAll_battleInput(InputState);
					flushPacketQueues();

					BattleInputBuffer_push(bib, InputState);
					// Add this input to the end of the buffer.
					BattleInputBuffer_pop(bib, &InputState);
					// Get the input from the front of the buffer.
				}
#endif

				StarShipPtr->ship_input_state = 0;
				if (StarShipPtr->RaceDescPtr->ship_info.crew_level)
				{
					if (InputState & BattleLeft)
					{
						StarShipPtr->ship_input_state |= LeftFlag;
					}
					else if (InputState & BattleRight)
					{
						StarShipPtr->ship_input_state |= RightFlag;
					}
					if (InputState & BattleThrust)
					{
						StarShipPtr->ship_input_state |= ThrustFlag;
					}
					if (InputState & BattleThrustAlt)
					{
						StarShipPtr->ship_input_state |= ThrustFlag;
					}
					if (InputState & BattleWeapon)
					{
						StarShipPtr->ship_input_state |= WeaponFlag;
					}
					if (InputState & BattleSpecial)
					{
						StarShipPtr->ship_input_state |= SpecialFlag;
					}

					if (CanRunAway && cur_player == 0 && ((InputState & BattleEscape) || WarpFromMenu))
					{
						WarpFromMenu = false;
						DoRunAway(StarShipPtr);
					}
				}
			}

			UnlockStarShip(&race_q[cur_player], hBattleShip);
		}
	}

#ifdef NETPLAY
	flushPacketQueues();
#endif

	if (uqm::currentActivity() & (CHECK_LOAD | CHECK_ABORT))
	{
		uqm::currentActivity() &= ~IN_BATTLE;
	}
}

static MUSIC_REF BattleRef;
uint8_t inHSpace = 0;

void BattleSong(bool DoPlay)
{
	if (BattleRef == 0)
	{
		if (inHyperSpace())
		{
			BattleRef = LoadMusicInstance(HyperspaceMusicRes);
			inHSpace = 1;
		}
		else if (inQuasiSpace())
		{
			BattleRef = LoadMusicInstance(QuasispaceMusicRes);
			inHSpace = 2;
		}
		else
		{
			if (uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE)
			{
				BattleRef = LoadMusicInstance(BattleMusicSamatraRes);

				if (BattleRef == 0)
				{
					BattleRef = LoadMusicInstance(BattleMusicRes);
				}
			}
			else
			{
				BattleRef = LoadMusicInstance(BattleMusicRes);
			}

			inHSpace = 0;
		}
	}

	if (DoPlay)
	{
		PlayMusicResume(BattleRef, NormalVolume);
	}
}

void FreeBattleSong(void)
{
	DestroyMusic(BattleRef);
	BattleRef = 0;
}

static bool
DoBattle(BATTLE_STATE* bs)
{
	extern uint16_t nth_frame;
	GFXRECT r;

	SetMenuSounds(MenuSoundNone, MenuSoundNone);

#if defined(NETPLAY) && defined(NETPLAY_CHECKSUM)
	if (getNumNetConnections() > 0 && battleFrameCount % NetplayChecksumInterval == 0)
	{
		crc_State state;

		crc_init(&state);
		crc_processState(&state);
		const Checksum checksum {(Checksum)crc_finish(&state)};

		Netplay_NotifyAll_checksum((uint32_t)battleFrameCount,
								   (uint32_t)checksum);
		flushPacketQueues();
		addLocalChecksum(battleFrameCount, checksum);
	}
#endif
	ProcessInput();
	// Also calls NetInput()
#if defined(NETPLAY) && defined(NETPLAY_CHECKSUM)
	if (getNumNetConnections() > 0)
	{
		const size_t delay = getBattleInputDelay();

		if (battleFrameCount >= delay
			&& (battleFrameCount - delay) % NetplayChecksumInterval == 0)
		{
			if (!(uqm::currentActivity() & CHECK_ABORT))
			{
				if (!verifyChecksums(battleFrameCount - delay))
				{
					uqm::currentActivity() |= CHECK_ABORT;
					resetConnections(ResetReason_syncLoss);
				}
			}
		}
	}
#endif

	if (bs->first_time)
	{
		r.corner.x = SisOrgX();
		r.corner.y = SisOrgY();
		r.extent.width = SisScreenWidth();
		r.extent.height = SisScreenHeight();
		SetTransitionSource(&r);
	}
	BatchGraphics();

	// Call the callback function, if set
	if (bs->frame_cb)
	{
		bs->frame_cb();
	}

	RedrawQueue(true);

	if (bs->first_time)
	{
		bs->first_time = false;
		ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
	}
	UnbatchGraphics();
	if ((!(uqm::currentActivity() & IN_BATTLE)) || (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
	{
		return false;
	}

	const uint8_t battle_speed {uqm::highByte(nth_frame)};
	if (battle_speed == (uint8_t)~0)
	{ // maximum speed, nothing rendered at all
		Async_process();
		TaskSwitch();
	}
	else
	{
		SleepThreadUntil(bs->NextTime
						 + BattleFrameRateTicks / (battle_speed + 1));
		bs->NextTime = GetTimeCounter();
	}

	if ((uqm::currentActivity() & IN_BATTLE) == 0)
	{
		return false;
	}

#ifdef NETPLAY
	battleFrameCount++;
#endif
	return true;
}

#ifdef NETPLAY
uint16_t
GetPlayerOrder(uint16_t i)
{
	// Iff 'myTurn' is set on a connection, the local party will be
	// processed first.
	// If neither is network controlled, the top player (1) is handled
	// first.
	if ((testFlag(PlayerControl[0], PlayerControlFlags::Network) && !NetConnection_getDiscriminant(netConnections[0]))
		|| (testFlag(PlayerControl[1], PlayerControlFlags::Network) && NetConnection_getDiscriminant(netConnections[1])))
	{
		return i;
	}
	else
	{
		return 1 - i;
	}
}
#endif

// Let each player pick his ship.
static bool
selectAllShips(int16_t num_ships)
{
	if (num_ships == 1)
	{
		// HyperSpace in full game.
		return GetNextStarShip(nullptr, 0);
	}

#ifdef NETPLAY
	if (testFlag(PlayerControl[0], PlayerControlFlags::Network) && testFlag(PlayerControl[1], PlayerControlFlags::Network))
	{
		uqm::log::error("Only one side at a time can be network "
						"controlled.\n");
		return false;
	}
#endif

	return GetInitialStarShips();
}

bool Battle(BattleFrameCallback* callback)
{
	if (uqm::lowByte(uqm::currentActivity()) != SUPER_MELEE)
	{
		// In Supermelee, the RNG is already initialised.
		uqm::random().reseed(static_cast<uint32_t>(GetTimeCounter().time_since_epoch().count()));
	}

	BattleSong(false);

	int16_t num_ships {InitShips()};

	if (instantVictory)
	{
		num_ships = 0;
		battle_counter[0] = 1;
		battle_counter[1] = 0;
		instantVictory = false;
	}

	if (num_ships)
	{
		BATTLE_STATE bs;

		uqm::currentActivity() |= IN_BATTLE;
		battle_counter[0] = CountLinks(&race_q[0]);
		battle_counter[1] = CountLinks(&race_q[1]);

		if (uqm::UQMOptions::read().meleeScale != uqm::MeleeScaleMode::Step)
		{
			SetGraphicScaleMode(toTFBScaleMode(uqm::UQMOptions::read().meleeScale));
		}

		setupBattleInputOrder();
#ifdef NETPLAY
		initBattleInputBuffers();
#ifdef NETPLAY_CHECKSUM
		initChecksumBuffers();
#endif /* NETPLAY_CHECKSUM */
		battleFrameCount = 0;
		ResetWinnerStarShip();
		setBattleStateConnections(&bs);
#endif /* NETPLAY */

		if (!selectAllShips(num_ships))
		{
			uqm::currentActivity() |= CHECK_ABORT;
			goto AbortBattle;
		}

		BattleSong(true);

		bs.NextTime = TimeCount {};
#ifdef NETPLAY
		initBattleStateDataConnections();
		{
			const bool allOk = negotiateReadyConnections(true, NetState_inBattle);
			if (!allOk)
			{
				uqm::currentActivity() |= CHECK_ABORT;
				goto AbortBattle;
			}
		}
#endif /* NETPLAY */
		bs.InputFunc = DoBattle;
		bs.frame_cb = callback;
		bs.first_time = inHQSpace();

		if (bs.first_time)
		{
			EraseRadar();
		}

		DoInput(&bs, false);

AbortBattle:
		if (uqm::lowByte(uqm::currentActivity()) == SUPER_MELEE)
		{
			if (uqm::currentActivity() & CHECK_ABORT)
			{
				// Do not return to the main menu when a game is aborted,
				// (just to the supermelee menu).
#ifdef NETPLAY
				waitResetConnections(NetState_inSetup);
				// A connection may already be in inSetup (set from
				// GetMeleeStarship). This is not a problem, although
				// it will generate a warning in debug mode.
#endif

				uqm::currentActivity() &= ~CHECK_ABORT;
			}
			else
			{
				// Show the result of the battle.
				MeleeGameOver();
			}
		}

#ifdef NETPLAY
		uninitBattleInputBuffers();
#ifdef NETPLAY_CHECKSUM
		uninitChecksumBuffers();
#endif /* NETPLAY_CHECKSUM */
		setBattleStateConnections(nullptr);
#endif /* NETPLAY */

		SetMusicPosition();
		StopDitty();
		StopMusic();
		StopSound();
	}

	UninitShips();
	FreeBattleSong();

	return (bool)(num_ships < 0);
}
