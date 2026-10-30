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
#ifndef UQMGAME_H_
#define UQMGAME_H_

#include "core/stl/stl.h"
#include "options/options.h"
#include "core/log/logger.h"

#include "core/stl/memory.h"
#include "core/stl/thread.h"
#include <cstdint>

namespace uqm
{

class GameSession;
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
	GameSession& beginSession(SessionKind kind);
	// Ends the game of the session that runs, and destroys the session.
	void endSession();
	// The session that runs, or nullptr.
	[[nodiscard]] GameSession* session() noexcept
	{
		return m_session.get();
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
	uqstl::unique_ptr<GameSession> m_session {};
	uint16_t m_activity {};
	uint8_t m_globalFlags {};

	static UQMGame* s_instance;
};

} // namespace uqm

#endif /* UQMGAME_H_ */