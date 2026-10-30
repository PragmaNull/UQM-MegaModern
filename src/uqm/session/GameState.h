// The state of one game, as a save file holds it.

// globdata.h includes this file after it defines GAME_STATE, so the include guard comes after it.
#include "uqm/globdata.h"

#ifndef UQM_SESSION_GAMESTATE_H_
#define UQM_SESSION_GAMESTATE_H_

#include "core/stl/functional.h"
#include "core/stl/map.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "uqm/sis.h"
#include "uqm/state.h"
#include <cstdint>

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

	// One of the files STARINFO_FILE, RANDGRPINFO_FILE and DEFGRPINFO_FILE.
	[[nodiscard]] GAME_STATE_FILE& stateFile(int index) noexcept
	{
		return m_stateFiles[index];
	}

	// The value of a game state flag, such as "SHOFIXTI_VISITS", or nullopt if it is not set.
	[[nodiscard]] uqstl::optional<uint32_t> flag(uqstl::string_view name) const;
	void setFlag(uqstl::string_view name, uint32_t value);
	void clearFlags() noexcept;

private:
	SIS_STATE m_sis {};
	GAME_STATE m_game {};
	uqstl::array<GAME_STATE_FILE, NUM_STATE_FILES> m_stateFiles {MakeStateFiles()};
	uqstl::map<uqstl::string, uint32_t, uqstl::less<>> m_flags {};
};

// The state of the current session. Use on the game thread only.
[[nodiscard]] GameState& gameState();

} // namespace uqm

// A flag of the current session, or 0 if it is not set.
uint32_t getGameStateUint(const char* name);
void setGameStateUint(const char* name, uint32_t val);

#endif // UQM_SESSION_GAMESTATE_H_
