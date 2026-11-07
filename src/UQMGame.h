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
#ifndef UQM_UQMGAME_H
#define UQM_UQMGAME_H

#include <cstdint>

#include "core/log/logger.h"
#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/thread.h"
#include "core/stl/utility.h"
#include "options/options.h"
#include "uqm/random/GameSeed.h"

namespace uqm
{

class GameSession;
class UQMLua;
enum class SessionKind;

class UQMGame
{
public:
	UQMGame();
	~UQMGame();

	[[nodiscard]] static UQMGame& getInstance();

	[[nodiscard]] uqstl::pair<int, bool> setup(uqstl::span<uqgsl::zstring> args);
	[[nodiscard]] int run();
	void teardown();

	// Use the session functions on the game thread only.
	// Makes a session and starts its game. Ends the session that runs.
	void beginSession(SessionKind kind);
	// Ends the game of the session that runs, and destroys the session.
	void endSession();
	// The session that runs, or nullptr.
	[[nodiscard]] GameSession* session() noexcept
	{
		return m_session.get();
	}

	// The Lua support. It exists between setup() and teardown().
	[[nodiscard]] UQMLua& lua() noexcept
	{
		return *m_lua;
	}

	// The seed that the next new game uses instead of the seed from the settings.
	void setNewGameSeed(const GameSeed& seed) noexcept
	{
		m_newGameSeed = seed;
	}

	// Returns the seed from setNewGameSeed() once, then nullopt.
	[[nodiscard]] uqstl::optional<GameSeed> takeNewGameSeed() noexcept
	{
		return uqstl::exchange(m_newGameSeed, uqstl::nullopt);
	}

	// What the game program does now. It stays between sessions.
	[[nodiscard]] uint16_t& activity() noexcept
	{
		return m_activity;
	}

	// The in-game options of glob_flags. They stay between sessions.
	[[nodiscard]] uint8_t& globalFlags() noexcept
	{
		return m_globalFlags;
	}

private:
	UQMOptions m_options {};
	Logger m_logger {};
	uqstl::jthread m_gameThread {};
	// The return value of Starcon2Main.
	int m_gameExitCode {EXIT_SUCCESS};
	uqstl::unique_ptr<UQMLua> m_lua {};
	uqstl::unique_ptr<GameSession> m_session {};
	uint16_t m_activity {};
	uint8_t m_globalFlags {};
	uqstl::optional<GameSeed> m_newGameSeed {};

	static UQMGame* s_instance;
};

} // namespace uqm

#endif // UQM_UQMGAME_H
