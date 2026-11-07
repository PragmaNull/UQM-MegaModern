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

#include "tactrans.h"

#include "battlecontrols.h"
#include "build.h"
#include "collide.h"
#include "globdata.h"
#include "element.h"
#include "ship.h"
#include "status.h"
#include "battle.h"
#include "init.h"
#include "supermelee/pickmele.h"
#ifdef NETPLAY
#include "supermelee/netplay/netmelee.h"
#include "supermelee/netplay/netmisc.h"
#include "supermelee/netplay/notify.h"
#include "supermelee/netplay/proto/ready.h"
#include "supermelee/netplay/packet.h"
#include "supermelee/netplay/packetq.h"
#endif
#include "races.h"
#include "encount.h"
#include "settings.h"
#include "sounds.h"
#include "libs/mathlib.h"
#include "intel.h"
#include "uqm/random/UQMRandom.h"


static void cleanup_dead_ship(Element* ElementPtr);

static bool dittyIsPlaying;
static Starship* winnerStarShip;
// Indicates which ship is the winner of the current battle.
// The winner will be last to pick the next ship.


bool OpponentAlive(Starship* TestStarShipPtr)
{
	HELEMENT hSuccElement {};

	for (HELEMENT hElement {GetHeadElement()}; hElement; hElement = hSuccElement)
	{
		Element* ElementPtr {};
		Starship* StarShipPtr {};

		LockElement(hElement, &ElementPtr);
		hSuccElement = GetSuccElement(ElementPtr);
		GetElementStarShip(ElementPtr, &StarShipPtr);
		UnlockElement(hElement);

		if (StarShipPtr && StarShipPtr != TestStarShipPtr
			&& StarShipPtr->RaceDescPtr->ship_info.crew_level == 0)
		{
			return false;
		}
	}

	return true;
}

static void PlayDitty(Starship* ship)
{
	PlayMusic(ship->RaceDescPtr->ship_data.victory_ditty, false, 3);
	dittyIsPlaying = true;
}

void StopDitty(void)
{
	if (dittyIsPlaying)
	{
		StopMusic();
	}
	dittyIsPlaying = false;
}

static bool DittyPlaying(void)
{
	if (!dittyIsPlaying)
	{
		return false;
	}

	dittyIsPlaying = PLRPlaying((MUSIC_REF)~0);
	return dittyIsPlaying;
}

void ResetWinnerStarShip(void)
{
	winnerStarShip = nullptr;
}

#ifdef NETPLAY
static void readyToEnd2Callback(NetConnection* conn, void* arg)
{
	NetConnection_setState(conn, NetState::EndingBattle2);
	(void)arg;
}

static void readyToEndCallback(NetConnection* conn, void* arg)
{
	// This callback function gets called from inside the function that
	// updates the frame counter, but this is not a problem as the
	// ending frame count will at least be 1 greater than the current
	// frame count.

	BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};

#ifdef NETPLAY_DEBUG
	fmt::print(stderr, "Both sides are ready to end the battle; starting "
					   "end-of-battle synchronisation.\n");
#endif
	NetConnection_setState(conn, NetState::EndingBattle);
	if (battleFrameCount + 1 > battleStateData->endFrameCount)
	{
		battleStateData->endFrameCount = battleFrameCount + 1;
	}
	Netplay_Notify_frameCount(conn, battleFrameCount + 1);
	// The +1 is to ensure that after the remote side receives the
	// frame count it will still receive one more frame data packet,
	// so it will know in advance when the last frame data packet
	// will come so it won't block. It also ensures that the
	// local frame counter won't go past the sent number, which
	// could happen when the function triggering the call to this
	// function is the frame update function which might update
	// the frame counter one more time.
	static_cast<void>(flushPacketQueue(conn));
#ifdef NETPLAY_DEBUG
	fmt::print(stderr, "NETPLAY: [{}] ==> Sent battleFrameCount {}.\n",
			   NetConnection_getPlayerNr(conn), battleFrameCount + 1);
#endif
	Netplay_localReady(conn, readyToEnd2Callback, nullptr, false);
	(void)arg;
}

/*
 * When one player's ship dies, there's a delay before the next ship
 * can be chosen. This time depends on the time the ditty is playing
 * and may differ for each side.
 * To synchronise the time, the following protocol is followed:
 * 1. (NetState_inBattle) The Ready protocol is used to let either
 *    party know when they're ready to stop the battle.
 * 2. (NetState_endingBattle) Each party sends the frame number of when
 *    it wants to end the battle, and continues until that point, where
 *    it waits until it has received the frame number of the other party.
 * 3. After a player has both sent and received a frame count, the
 *    simulation continues for each party, until the maximum of both
 *    frame counts has been achieved.
 * 4. The Ready protocol is used to let each side signal that it has
 *    reached the target frame count.
 * 5. The battle ends.
 */
static bool readyForBattleEndPlayer(NetConnection* conn)
{
	const BattleStateData* battleStateData {(BattleStateData*)NetConnection_getStateData(conn)};

	if (NetConnection_getState(conn) == NetState::InterBattle || NetConnection_getState(conn) == NetState::InSetup)
	{
		// This connection is already ready. The entire synchronisation
		// protocol has already been done for this connection.
		return true;
	}

	if (NetConnection_getState(conn) == NetState::InBattle)
	{
		if (Netplay_isLocalReady(conn))
		{
			// We've already sent notice that we are ready, but we're
			// still waiting for the other side to say it's ready too.
			return false;
		}

		// We haven't yet told the other side we're ready. We do so now.
		Netplay_localReady(conn, readyToEndCallback, nullptr, true);
		// This may set the state to endingBattle.

		if (NetConnection_getState(conn) == NetState::InBattle)
		{
			return false;
		}
	}

	assert(NetConnection_getState(conn) == NetState::EndingBattle || NetConnection_getState(conn) == NetState::EndingBattle2);

	// Keep the simulation going as long as the target frame count
	// hasn't been reached yet. Note that if the connection state is
	// NetState_endingBattle, then we haven't yet received the
	// remote frame count, so the target frame count may still rise.
	if (battleFrameCount < battleStateData->endFrameCount)
	{
		return false;
	}

	if (NetConnection_getState(conn) == NetState::EndingBattle)
	{
		// We have reached the target frame count, but we don't know
		// the remote target frame count yet. So we wait until it has
		// come in.
		waitReady(conn);
		// TODO: check whether all connections are still connected.
		assert(NetConnection_getState(conn) == NetState::EndingBattle2);

		// Continue the simulation if the battleFrameCount has gone up.
		if (battleFrameCount < battleStateData->endFrameCount)
		{
			return false;
		}
	}

	// We are ready and wait for the other party to become ready too.
	negotiateReady(conn, true, NetState::InterBattle);

	return true;
}
#endif

bool battleEndReadyHuman(HumanInputContext* context)
{
	(void)context;
	return true;
}

bool battleEndReadyComputer(ComputerInputContext* context)
{
	(void)context;
	return true;
}

#ifdef NETPLAY
bool battleEndReadyNetwork(NetworkInputContext* context)
{
	return readyForBattleEndPlayer(netConnections[context->playerNr]);
}
#endif

// Returns true iff this side is ready to end the battle.
static inline bool readyForBattleEnd(void)
{
#ifndef NETPLAY
	return !DittyPlaying();
#else  /* defined (NETPLAY) */

	if (DittyPlaying())
	{
		return false;
	}

	for (int playerI {0}; playerI < NumPlayers; playerI++)
	{
		if (!PlayerInput[playerI]->handlers->battleEndReady(
				PlayerInput[playerI]))
		{
			return false;
		}
	}

	return true;
#endif /* defined (NETPLAY) */
}

static void preprocess_dead_ship(Element* DeadShipPtr)
{
	SuppressSounds();
	(void)DeadShipPtr; // unused argument
}

static constexpr auto MinDittyFrameCount {((c_gameSecond * 3) / BattleFrameRateTicks)};

void cleanup_dead_ship(Element* DeadShipPtr)
{
	Starship* DeadStarShipPtr {};

	SuppressSounds();

	GetElementStarShip(DeadShipPtr, &DeadStarShipPtr);
	{
		// Ship explosion has finished, or ship has just warped out
		// if DeadStarShipPtr->crew_level != 0
		bool MusicStarted {};
		HELEMENT hElement {};
		HELEMENT hSuccElement {};

		/* Record crew left after the battle */
		DeadStarShipPtr->crew_level =
			DeadStarShipPtr->RaceDescPtr->ship_info.crew_level;

		MusicStarted = false;

		for (hElement = GetHeadElement(); hElement; hElement = hSuccElement)
		{
			Element* ElementPtr {};
			Starship* StarShipPtr {};

			LockElement(hElement, &ElementPtr);
			hSuccElement = GetSuccElement(ElementPtr);
			GetElementStarShip(ElementPtr, &StarShipPtr);
			// Get the Starship that this Element belongs to.

			if (StarShipPtr == DeadStarShipPtr)
			{
				// This Element belongs to the dead ship; it may be the
				// ship's own Element.
				SetElementStarShip(ElementPtr, 0);

				if (!(ElementPtr->state_flags & CrewObject)
					|| ElementPtr->preprocess_func != crew_preprocess)
				{
					// Set the Element up for deletion.
					SetPrimType(&DisplayArray[ElementPtr->PrimIndex],
								NoPrim);
					ElementPtr->life_span = 0;
					ElementPtr->state_flags =
						Nonsolid | Disappearing | FiniteLife;
					ElementPtr->preprocess_func = 0;
					ElementPtr->postprocess_func = 0;
					ElementPtr->death_func = 0;
					ElementPtr->collision_func = 0;
				}
			}

			if (StarShipPtr
				&& (StarShipPtr->cur_status_flags & PlayVictoryDitty))
			{
				// StarShipPtr points to the remaining ship.
				MusicStarted = true;
				PlayDitty(StarShipPtr);
				StarShipPtr->cur_status_flags &= ~PlayVictoryDitty;
			}

			UnlockElement(hElement);
		}

		// The ship will be "alive" for at least 2 more frames to make sure
		// the elements it owns (set up for deletion above) expire first.
		// Ditty does NOT play in the following circumstances:
		//  * The winning ship dies before the loser finishes exploding
		//  * At the moment the losing ship dies, the winner has started
		//    the warp out sequence
		DeadShipPtr->life_span = MusicStarted ? MinDittyFrameCount : 1;
		if (DeadStarShipPtr == winnerStarShip)
		{ // This ship died but won the battle. We need to keep it alive
			// longer than the dead opponent ship so that the winning player
			// picks last.
			DeadShipPtr->life_span = MinDittyFrameCount + 1;
		}
		DeadShipPtr->death_func = new_ship;
		DeadShipPtr->preprocess_func = preprocess_dead_ship;
		DeadShipPtr->state_flags &= ~Disappearing;
		// XXX: this increment was originally done by another piece of code
		//   just below this one. I am almost sure it is not needed, but it
		//   keeps the original framecount.
		++DeadShipPtr->life_span;
		SetElementStarShip(DeadShipPtr, DeadStarShipPtr);
	}
}

static void setMinShipLifeSpan(Element* ship, uint16_t life_span)
{
	if (ship->death_func == new_ship)
	{ // The ship has finished exploding or warping out, and now
		// we can work with the remaining Element
		assert(ship->state_flags & FiniteLife);
		assert(!(ship->state_flags & Disappearing));
		if (ship->life_span < life_span)
		{
			ship->life_span = life_span;
		}
	}
}

static void setMinStarShipLifeSpan(Starship* starShip, uint16_t life_span)
{
	Element* ship {};

	LockElement(starShip->hShip, &ship);
	setMinShipLifeSpan(ship, life_span);
	UnlockElement(starShip->hShip);
}

static void checkOtherShipLifeSpan(Element* deadShip)
{
	Starship* deadStarShip {};

	GetElementStarShip(deadShip, &deadStarShip);

	if (winnerStarShip != nullptr && deadStarShip != winnerStarShip
		&& winnerStarShip->RaceDescPtr->ship_info.crew_level == 0)
	{ // The opponent ship also died but won anyway (e.g. Glory device)
		// We need to keep the opponent ship alive longer so that the
		// winning player picks last.
		setMinStarShipLifeSpan(winnerStarShip, deadShip->life_span + 1);
	}
	else if (winnerStarShip == nullptr)
	{ // Both died at the same time, or the loser has already expired
		HELEMENT hNextElement {};

		// Find the other dead ship(s) and keep them alive for at least as
		// long as this ship.
		for (HELEMENT hElement {GetHeadElement()}; hElement; hElement = hNextElement)
		{
			Element* Element {};
			Starship* starShip {};

			LockElement(hElement, &Element);
			hNextElement = GetSuccElement(Element);
			GetElementStarShip(Element, &starShip);

			if (starShip != nullptr && Element != deadShip
				&& starShip->RaceDescPtr->ship_info.crew_level == 0)
			{ // This is another dead ship
				setMinShipLifeSpan(Element, deadShip->life_span);
			}

			UnlockElement(hElement);
		}
	}
}

// This function is called when dead ship Element's life_span reaches 0
void new_ship(Element* DeadShipPtr)
{
	Starship* DeadStarShipPtr {};

	GetElementStarShip(DeadShipPtr, &DeadStarShipPtr);

	if (!readyForBattleEnd())
	{
		DeadShipPtr->state_flags &= ~Disappearing;
		++DeadShipPtr->life_span;

		// Keep the winner alive longer, or in a simultaneous destruction
		// tie, keep the other dead ship alive so that readyForBattleEnd()
		// is called for only one ship at a time.
		// When a ship has been destroyed, each side of a network
		// connection waits until the other side is ready.
		// When two ships die at the same time, this is handled for one
		// ship after the other.
		checkOtherShipLifeSpan(DeadShipPtr);
		return;
	}

	// Once a ship is being picked, we do not care about the winner anymore
	winnerStarShip = nullptr;

	{
		StopDitty();
		StopMusic();
		StopSound();

		SetElementStarShip(DeadShipPtr, 0);
		const bool RestartMusic {OpponentAlive(DeadStarShipPtr)};

		free_ship(DeadStarShipPtr->RaceDescPtr, true, true);
		DeadStarShipPtr->RaceDescPtr = 0;

		// Graphics are batched while the draw queue is processed,
		// but we are going to draw the ship selection box now
		UnbatchGraphics();

#ifdef NETPLAY
		initBattleStateDataConnections();
		{
			const bool allOk =
				negotiateReadyConnections(true, NetState::InterBattle);
			// We are already in NetState_interBattle, but all
			// sides just need to pass this checkpoint before
			// going on.
			if (!allOk)
			{
				// Some network connection has been reset.
				uqm::currentActivity() &= ~IN_BATTLE;
				BatchGraphics();
				return;
			}
		}
#endif /* NETPLAY */

		if (!FleetIsInfinite(DeadStarShipPtr->playerNr))
		{ // This may be a dead ship (crew_level == 0) or a warped out ship
			UpdateShipFragCrew(DeadStarShipPtr);
			// Deactivate the ship (cannot be selected)
			DeadStarShipPtr->SpeciesID = NO_ID;
		}

		if (GetNextStarShip(DeadStarShipPtr, DeadStarShipPtr->playerNr))
		{
#ifdef NETPLAY
			{
				const bool allOk =
					negotiateReadyConnections(true, NetState::InBattle);
				if (!allOk)
				{
					// Some network connection has been reset.
					uqm::currentActivity() &= ~IN_BATTLE;
					BatchGraphics();
					return;
				}
			}
#endif
			if (RestartMusic)
			{
				BattleSong(true);
			}
		}
		else if (battle_counter[0] == 0 || battle_counter[1] == 0)
		{
			// One player is out of ships. The battle is over.
			uqm::currentActivity() &= ~IN_BATTLE;
		}
#ifdef NETPLAY
		else
		{
			// Battle has been aborted.
			uqm::currentActivity() |= CHECK_ABORT;
		}
#endif
		BatchGraphics();
	}
}

static void explosion_preprocess(Element* ShipPtr)
{
	uint8_t i {};

	i = (NumExplosionFrames * 3) - ShipPtr->life_span;
	switch (i)
	{
		case 25:
			ShipPtr->preprocess_func = nullptr;
			FALLTHROUGH;
		case 0:
		case 1:
		case 2:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
			i = 1;
			break;
		case 3:
		case 4:
		case 5:
		case 18:
		case 19:
			i = 2;
			break;
		case 15:
			SetPrimType(&DisplayArray[ShipPtr->PrimIndex], NoPrim);
			ShipPtr->state_flags |= Changing;
			FALLTHROUGH;
		default:
			i = 3;
			break;
	}

	do
	{
		HELEMENT hElement {AllocElement()};
		if (hElement)
		{
			uint16_t angle {};
			uint16_t dist {};
			Element* ElementPtr {};
			extern FRAME explosion[];

			PutElement(hElement);
			LockElement(hElement, &ElementPtr);
			ElementPtr->playerNr = NeutralPlayerNum;
			ElementPtr->state_flags = Appearing | FiniteLife | Nonsolid;
			ElementPtr->life_span = 9;
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex], PrimitiveType::Stamp);
			ElementPtr->current.image.farray = explosion;
			ElementPtr->current.image.frame = explosion[0];
			uint32_t rand_val {uqm::random().next()};
			angle = uqm::lowByte(uqm::highWord(rand_val));
			dist = DisplayToWorld(ResScale(uqm::lowByte(uqm::lowWord(rand_val)) % 8));
			if (uqm::highByte(uqm::lowWord(rand_val)) < 256 * 1 / 3)
			{
				dist += DisplayToWorld(ResScale(8));
			}
			ElementPtr->current.location.x =
				ShipPtr->current.location.x + Cosine(angle, dist);
			ElementPtr->current.location.y =
				ShipPtr->current.location.y + Sine(angle, dist);
			ElementPtr->preprocess_func = animation_preprocess;
			rand_val = uqm::random().next();
			angle = uqm::lowByte(uqm::lowWord(rand_val));
			dist = WorldToVelocity(
				DisplayToWorld(ResScale(uqm::highByte(uqm::lowWord(rand_val)) % 5)));
			SetVelocityComponents(&ElementPtr->velocity,
								  Cosine(angle, dist), Sine(angle, dist));
			UnlockElement(hElement);
		}
	} while (--i);
}

void StopAllBattleMusic(void)
{
	StopDitty();
	StopMusic();
}

Starship* FindAliveStarShip(Element* deadShip)
{
	Starship* aliveShip = nullptr;
	HELEMENT hNextElement {};

	// Find the remaining ship, if any, and see if it is still alive.
	for (HELEMENT hElement {GetHeadElement()}; hElement; hElement = hNextElement)
	{
		Element* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		if ((ElementPtr->state_flags & PlayerShip)
			&& ElementPtr != deadShip
			/* and not running away */
			&& ElementPtr->mass_points <= MaxShipMass + 1)
		{
			GetElementStarShip(ElementPtr, &aliveShip);
			assert(aliveShip != nullptr);
			if (aliveShip->RaceDescPtr->ship_info.crew_level == 0
				/* reincarnating Pkunk is not actually dead */
				&& ElementPtr->mass_points != MaxShipMass + 1)
			{
				aliveShip = nullptr;
			}

			UnlockElement(hElement);
			break;
		}
		hNextElement = GetSuccElement(ElementPtr);
		UnlockElement(hElement);
	}

	return aliveShip;
}

Starship* GetWinnerStarShip(void)
{
	return winnerStarShip;
}

void SetWinnerStarShip(Starship* winner)
{
	if (winner == nullptr)
	{
		return; // nothing to do
	}

	winner->cur_status_flags |= PlayVictoryDitty;

	// The winner is set once per battle. If both ships die, this function is
	// called twice, once for each ship. We need to preserve the winner
	// determined on the first call.
	if (winnerStarShip == nullptr)
	{
		winnerStarShip = winner;
	}
}

void RecordShipDeath(Element* deadShip)
{
	Starship* deadStarShip {};

	GetElementStarShip(deadShip, &deadStarShip);
	assert(deadStarShip != nullptr);

	if (deadShip->mass_points <= MaxShipMass)
	{ // Not running away.
		// When a ship tries to run away, it is (dis)counted in DoRunAway(),
		// so when it dies while running away, we will not count it again
		assert(deadStarShip->playerNr >= 0);
		battle_counter[deadStarShip->playerNr]--;
	}

	if (uqm::lowByte(uqm::currentActivity()) == SUPER_MELEE)
	{
		MeleeShipDeath(deadStarShip);
	}
}

void StartShipExplosion(Element* ShipPtr, bool playSound)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	if (!uqm::UQMOptions::read().extended || isNetwork())
	{
		ZeroVelocityComponents(&ShipPtr->velocity);
	}

	DeltaEnergy(ShipPtr,
				-(int16_t)StarShipPtr->RaceDescPtr->ship_info.energy_level);

	ShipPtr->life_span = NumExplosionFrames * 3;
	ShipPtr->state_flags &= ~Disappearing;
	ShipPtr->state_flags |= FiniteLife | Nonsolid;
	ShipPtr->preprocess_func = explosion_preprocess;
	ShipPtr->postprocess_func = PostProcessStatus;
	ShipPtr->death_func = cleanup_dead_ship;
	ShipPtr->hTarget = 0;

	if (playSound)
	{
		PlaySound(GetSound(GameSounds, SoundEffect::ShipExplodes),
				  CalcSoundPosition(ShipPtr), ShipPtr, GameSoundPriority + 1);
	}
}

void ship_death(Element* ShipPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);

	SetMusicPosition();
	StopAllBattleMusic();

	// If the winning ship dies before the ditty starts, do not play it.
	// e.g. a ship can die after the opponent begins exploding but
	// before the explosion is over.
	StarShipPtr->cur_status_flags &= ~PlayVictoryDitty;

	StartShipExplosion(ShipPtr, true);

	Starship* winner {FindAliveStarShip(ShipPtr)};
	SetWinnerStarShip(winner);
	RecordShipDeath(ShipPtr);
}

static constexpr auto StartIonColor {BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7A)};

// Called from the death_func of an Element for an ion trail pixel, or a
// ship shadow (when warping in/out).
static void cycle_ion_trail(Element* ElementPtr)
{
	const static Color colorTab[] =
		{
			BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7a),
			BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7b),
			BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7c),
			BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7d),
			BuildColor(MakeRgb15(0x1F, 0x07, 0x00), 0x7e),
			BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7f),
			BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x2a),
			BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2b),
			BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2c),
			BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2d),
			BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2e),
			BuildColor(MakeRgb15(0x0B, 0x00, 0x00), 0x2f),
		};
	const size_t colorTabCount = std::size(colorTab);

	assert(!(ElementPtr->state_flags & PlayerShip));

	ElementPtr->colorCycleIndex++;
	if (ElementPtr->colorCycleIndex != colorTabCount)
	{
		ElementPtr->life_span = ElementPtr->thrust_wait;
		// Reset the life span.

		SetPrimColor(&DisplayArray[ElementPtr->PrimIndex],
					 colorTab[ElementPtr->colorCycleIndex]);

		ElementPtr->state_flags &= ~Disappearing;
		ElementPtr->state_flags |= Changing;
	} // else, the Element disappears.
}

static constexpr auto IonLife {1};

void spawn_ion_trail(Element* ElementPtr, int16_t x_offset, int16_t y_offset)
{
	assert(ElementPtr->state_flags & PlayerShip);

	HELEMENT hIonElement {AllocElement()};
	if (hIonElement)
	{
		uint16_t angle {};
		GfxRect r;
		Element* IonElementPtr {};
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		angle = FacingToAngle(StarShipPtr->ShipFacing) + HalfCircle;
		GetFrameRect(StarShipPtr->RaceDescPtr->ship_data.ship[0], &r);
		r.extent.height = DisplayToWorld(r.extent.height + r.corner.y);

		InsertElement(hIonElement, GetHeadElement());
		LockElement(hIonElement, &IonElementPtr);
		IonElementPtr->playerNr = NeutralPlayerNum;
		IonElementPtr->state_flags = Appearing | FiniteLife | Nonsolid;
		IonElementPtr->thrust_wait = IonLife;
		IonElementPtr->life_span = IonElementPtr->thrust_wait;
		// When the Element "dies", in the death_func
		// 'cycle_ion_trail', it is given new life a number of
		// times, by setting life_span to thrust_wait.

		if (!IsHd())
		{
			SetPrimType(&DisplayArray[IonElementPtr->PrimIndex], PrimitiveType::Point);
			IonElementPtr->current.image.frame = DecFrameIndex(stars_in_space);
			IonElementPtr->current.image.farray = &stars_in_space;
		}
		else
		{
			SetPrimType(&DisplayArray[IonElementPtr->PrimIndex], PrimitiveType::StampFill);
			IonElementPtr->current.image.frame = SetAbsFrameIndex(stars_misc[0], 1);
			IonElementPtr->current.image.farray = stars_misc;
			IonElementPtr->next.image.frame = IonElementPtr->current.image.frame;
			IonElementPtr->next.image.farray = IonElementPtr->current.image.farray;
		}
		SetPrimColor(&DisplayArray[IonElementPtr->PrimIndex],
					 StartIonColor);
		IonElementPtr->colorCycleIndex = 0;
		IonElementPtr->current.location = ElementPtr->current.location;
		IonElementPtr->current.location.x +=
			(int16_t)Cosine(angle, r.extent.height) + x_offset;
		IonElementPtr->current.location.y +=
			(int16_t)Sine(angle, r.extent.height) + y_offset;
		IonElementPtr->death_func = cycle_ion_trail;

		SetElementStarShip(IonElementPtr, StarShipPtr);

		{
			/* normally done during preprocess, but because
			 * object is being inserted at head rather than
			 * appended after tail it may never get preprocessed.
			 */
			IonElementPtr->next = IonElementPtr->current;
			--IonElementPtr->life_span;
			IonElementPtr->state_flags |= PreProcessFlag;
		}

		UnlockElement(hIonElement);
	}
}

static constexpr auto TransitionLife {1};

// Preprocess function for spawning a ship into or out of battle.
// Used when a new ship warps in, or a ship escapes by warping out, but not
// when a Pkunk ship is reborn.
void ship_transition(Element* ElementPtr)
{
	if (ElementPtr->state_flags & PlayerShip)
	{
		if (ElementPtr->state_flags & Appearing)
		{
			ElementPtr->life_span = HyperjumpLife;
			ElementPtr->preprocess_func = ship_transition;
			ElementPtr->postprocess_func = nullptr;
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);
			ElementPtr->state_flags |= Nonsolid | FiniteLife | Changing;
		}
		else if (ElementPtr->life_span < HyperjumpLife)
		{
			if (ElementPtr->life_span == NormalLife
				&& ElementPtr->crew_level)
			{
				ElementPtr->current.image.frame =
					ElementPtr->next.image.frame =
						SetEquFrameIndex(
							ElementPtr->current.image.farray[0],
							ElementPtr->current.image.frame);
				SetPrimType(&DisplayArray[ElementPtr->PrimIndex], PrimitiveType::Stamp);
				InitIntersectStartPoint(ElementPtr);
				InitIntersectEndPoint(ElementPtr);
				InitIntersectFrame(ElementPtr);
				ZeroVelocityComponents(&ElementPtr->velocity);
				ElementPtr->state_flags &= ~(Nonsolid | FiniteLife);
				ElementPtr->state_flags |= Changing;

				ElementPtr->preprocess_func = ship_preprocess;
				ElementPtr->postprocess_func = ship_postprocess;
			}

			return;
		}
	}

	{
		HELEMENT hShipImage {};
		Element* ShipImagePtr {};
		Starship* StarShipPtr {};

		GetElementStarShip(ElementPtr, &StarShipPtr);
		LockElement(StarShipPtr->hShip, &ShipImagePtr);

		if (!(ShipImagePtr->state_flags & Nonsolid))
		{
			ElementPtr->preprocess_func = nullptr;
		}
		else if ((hShipImage = AllocElement()))
		{
			const auto TransitionSpeed = [&] {
				return DisplayToWorld(ResScale(40));
			};
			uint16_t angle {};

			PutElement(hShipImage);

			angle = FacingToAngle(StarShipPtr->ShipFacing);

			LockElement(hShipImage, &ShipImagePtr);
			ShipImagePtr->playerNr = NeutralPlayerNum;
			ShipImagePtr->state_flags = Appearing | FiniteLife | Nonsolid;
			ShipImagePtr->thrust_wait = TransitionLife;
			ShipImagePtr->life_span = ShipImagePtr->thrust_wait;
			// When the Element "dies", in the death_func
			// 'cycle_ion_trail', it is given new life a number of
			// times, by setting life_span to thrust_wait.
			SetPrimType(&DisplayArray[ShipImagePtr->PrimIndex],
						PrimitiveType::StampFill);
			SetPrimColor(&DisplayArray[ShipImagePtr->PrimIndex],
						 StartIonColor);
			ShipImagePtr->colorCycleIndex = 0;
			ShipImagePtr->current.image = ElementPtr->current.image;
			ShipImagePtr->current.location = ElementPtr->current.location;
			if (!(ElementPtr->state_flags & PlayerShip))
			{
				ShipImagePtr->current.location.x +=
					Cosine(angle, TransitionSpeed());
				ShipImagePtr->current.location.y +=
					Sine(angle, TransitionSpeed());
				ElementPtr->preprocess_func = nullptr;
			}
			else if (ElementPtr->crew_level)
			{
				// JMS_GFX: Circumventing overflows by using temp variables
				// instead of subtracting straight from the GfxPoint sized
				// ShipImagePtr->current.location.
				const int32_t temp_x = (int32_t)ShipImagePtr->current.location.x - Cosine(angle, TransitionSpeed()) * (ElementPtr->life_span - 1);
				const int32_t temp_y = (int32_t)ShipImagePtr->current.location.y - Sine(angle, TransitionSpeed()) * (ElementPtr->life_span - 1);

				ShipImagePtr->current.location.x = WrapX(temp_x);
				ShipImagePtr->current.location.y = WrapY(temp_y);
			}
			ShipImagePtr->preprocess_func = ship_transition;
			ShipImagePtr->death_func = cycle_ion_trail;
			SetElementStarShip(ShipImagePtr, StarShipPtr);

			UnlockElement(hShipImage);
		}

		UnlockElement(StarShipPtr->hShip);
	}
}

void flee_preprocess(Element* ElementPtr)
{
	Starship* StarShipPtr {};

	if (--ElementPtr->turn_wait == 0)
	{
		const static Color colorTab[] =
			{
				BuildColor(MakeRgb15(0x0A, 0x00, 0x00), 0x2E),
				BuildColor(MakeRgb15(0x0E, 0x00, 0x00), 0x2D),
				BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C),
				BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
				BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2A),
				BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x29),
				BuildColor(MakeRgb15(0x1F, 0x04, 0x04), 0x28),
				BuildColor(MakeRgb15(0x1F, 0x0A, 0x0A), 0x27),
				BuildColor(MakeRgb15(0x1F, 0x0F, 0x0F), 0x26),
				BuildColor(MakeRgb15(0x1F, 0x13, 0x13), 0x25),
				BuildColor(MakeRgb15(0x1F, 0x19, 0x19), 0x24),
				BuildColor(MakeRgb15(0x1F, 0x13, 0x13), 0x25),
				BuildColor(MakeRgb15(0x1F, 0x0F, 0x0F), 0x26),
				BuildColor(MakeRgb15(0x1F, 0x0A, 0x0A), 0x27),
				BuildColor(MakeRgb15(0x1F, 0x04, 0x04), 0x28),
				BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x29),
				BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2A),
				BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
				BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C),
				BuildColor(MakeRgb15(0x0E, 0x00, 0x00), 0x2D),
			};
		const size_t colorTabCount = std::size(colorTab);

		ElementPtr->colorCycleIndex++;
		if (ElementPtr->colorCycleIndex == colorTabCount)
		{
			ElementPtr->colorCycleIndex = 0;
		}

		SetPrimColor(&DisplayArray[ElementPtr->PrimIndex],
					 colorTab[ElementPtr->colorCycleIndex]);

		if (ElementPtr->colorCycleIndex == 0)
		{
			--ElementPtr->thrust_wait;
		}

		ElementPtr->turn_wait = ElementPtr->thrust_wait;
		if (ElementPtr->turn_wait)
		{
			ElementPtr->turn_wait = ((ElementPtr->turn_wait - 1) >> 1) + 1;
		}
		else if (ElementPtr->colorCycleIndex != (colorTabCount / 2))
		{
			ElementPtr->turn_wait = 1;
		}
		else
		{
			ElementPtr->death_func = cleanup_dead_ship;
			ElementPtr->crew_level = 0;

			ElementPtr->life_span = HyperjumpLife + 1;
			ElementPtr->preprocess_func = ship_transition;
			ElementPtr->postprocess_func = nullptr;
			SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);
			ElementPtr->state_flags |= Nonsolid | FiniteLife | Changing;
		}
	}

	GetElementStarShip(ElementPtr, &StarShipPtr);
	StarShipPtr->cur_status_flags &=
		~(LeftFlag | RightFlag | ThrustFlag | WeaponFlag | SpecialFlag);
	// Ignore control input when fleeing.
	PreProcessStatus(ElementPtr);
}
