// The objects of one game: a full game or a Super Melee session.

#include "GameSession.h"

#include "core/log/log.h"
#include "libs/time/timelib.h"
#include "uqm/clock.h"
#include "uqm/gameev.h"
#include "uqm/globdata.h"
#include "uqm/lua/UQMLua.h"
#include "UQMGame.h"


namespace uqm
{
GameSession::GameSession(SessionKind kind)
	: m_kind {kind}
{
	m_random.reseed(static_cast<uint32_t>(GetTimeCounter().time_since_epoch().count()));
}

void GameSession::startGame()
{
	if (m_kind != SessionKind::FullGame)
	{
		return;
	}
	lua().openState();
	InitGameStructures();
	InitGameClock();
	initEventSystem();
	AddInitialGameEvents();
}

void GameSession::endGame()
{
	if (m_kind != SessionKind::FullGame)
	{
		return;
	}
	uninitEventSystem();
	UninitGameClock();
	UninitGameStructures();
	lua().closeState();
}

GameState& gameState()
{
	if (GameSession* const session {UQMGame::getInstance().session()})
	{
		return session->state();
	}
	static GameState s_noSession {};
	static bool s_reported {};
	if (!s_reported)
	{
		s_reported = true;
		log::error("The game state was used while no game session runs.");
	}
	return s_noSession;
}

ActivityWord& currentActivity()
{
	return UQMGame::getInstance().activity();
}

uint8_t& globalFlags()
{
	return UQMGame::getInstance().globalFlags();
}

} // namespace uqm
