// The state of one game, as a save file holds it.

// globdata.h includes this file after it defines GAME_STATE, so the include guard comes after it.
#include "uqm/globdata.h"

#ifndef UQM_SESSION_GAMESTATE_H_
#define UQM_SESSION_GAMESTATE_H_

#include "uqm/sis.h"

namespace uqm
{

class GameState
{
public:
	[[nodiscard]] SIS_STATE& sis() noexcept
	{
		return m_sis;
	}

	[[nodiscard]] GAME_STATE& game() noexcept
	{
		return m_game;
	}

private:
	SIS_STATE m_sis {};
	GAME_STATE m_game {};
};

// The state of the current session. Use on the game thread only.
[[nodiscard]] GameState& gameState();

} // namespace uqm

#endif // UQM_SESSION_GAMESTATE_H_
