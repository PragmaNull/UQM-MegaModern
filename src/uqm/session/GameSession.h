// The objects of one game: a full game or a Super Melee session.
#pragma once
#ifndef UQM_SESSION_GAMESESSION_H_
#define UQM_SESSION_GAMESESSION_H_

#include "uqm/session/GameState.h"

namespace uqm
{

enum class SessionKind
{
	FullGame,
	SuperMelee,
};

class GameSession
{
public:
	explicit GameSession(SessionKind kind);
	GameSession(const GameSession&) = delete;
	GameSession& operator=(const GameSession&) = delete;

	[[nodiscard]] SessionKind kind() const noexcept
	{
		return m_kind;
	}

	[[nodiscard]] GameState& state() noexcept
	{
		return m_state;
	}

	// Sets up the game systems. The session must be the current session.
	void startGame();
	// Releases the game systems. The session must be the current session.
	void endGame();

private:
	SessionKind m_kind;
	GameState m_state {};
};

} // namespace uqm

#endif // UQM_SESSION_GAMESESSION_H_
