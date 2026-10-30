// The objects of one game: a full game or a Super Melee session.

#include "GameSession.h"

#include "UQMGame.h"
#include "core/log/log.h"
#include "uqm/clock.h"
#include "uqm/gameev.h"
#include "uqm/lua/luastate.h"

namespace uqm
{

GameSession::GameSession(SessionKind kind)
	: m_kind {kind}
{
}

void GameSession::startGame()
{
	if (m_kind != SessionKind::FullGame)
	{
		return;
	}
	luaUqm_reinitState();
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

ACTIVITY& currentActivity()
{
	return UQMGame::getInstance().activity();
}

uint8_t& globalFlags()
{
	return UQMGame::getInstance().globalFlags();
}

} // namespace uqm
