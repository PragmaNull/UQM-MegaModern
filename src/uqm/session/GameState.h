// The state of one game, as a save file holds it.

#pragma once
#ifndef UQM_SESSION_GAMESTATE_H
#define UQM_SESSION_GAMESTATE_H

#include "core/stl/functional.h"
#include "core/stl/map.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/array.h"
#include "uqm/session/GameClock.h"
#include "uqm/session/GameData.h"
#include "uqm/session/GameFlags.h"
#include "uqm/session/FlagshipState.h"
#include "uqm/random/GameSeed.h"
#include "uqm/sis.h"
#include "uqm/state.h"
#include <cstdint>

namespace uqm
{
struct StarDesc;


class GameState
{
public:
	[[nodiscard]] FlagshipData& sis() noexcept
	{
		return m_sis;
	}

	[[nodiscard]] GameData& game() noexcept
	{
		return m_game;
	}

	[[nodiscard]] GameClock& clock() noexcept
	{
		return m_clock;
	}

	[[nodiscard]] FlagshipState& flagship() noexcept
	{
		return m_flagship;
	}

	// One of the files StarinfoFile, RandgrpinfoFile and DefgrpinfoFile.
	[[nodiscard]] GameStateFile& stateFile(int index) noexcept
	{
		return m_stateFiles[index];
	}

	[[nodiscard]] uint32_t flag(GameFlag flag) const noexcept
	{
		return m_flagValues[uqstl::to_underlying(flag)];
	}

	void setFlag(GameFlag flag, uint32_t value) noexcept
	{
		m_flagValues[uqstl::to_underlying(flag)] = value;
	}

	// The value of a flag by name, such as "SHOFIXTI_VISITS". A name that is not a GameFlag can hold a value too.
	// Returns nullopt for such a name that has no value.
	[[nodiscard]] uqstl::optional<uint32_t> flag(uqstl::string_view name) const;
	void setFlag(uqstl::string_view name, uint32_t value);
	// Sets all flags to 0, and removes the names that are not GameFlags.
	void clearFlags() noexcept;

	// The seed of the game. setSeed() also sets the values that a save keeps.
	[[nodiscard]] const GameSeed& seed() const noexcept
	{
		return m_seed;
	}

	void setSeed(const GameSeed& seed) noexcept
	{
		m_seed = seed;
		m_sis.Seed = seed.value;
		m_sis.ShipSeed = seed.ships ? 1 : 0;
		setFlag(GameFlag::SEED_TYPE, static_cast<uint32_t>(seed.type));
	}

	// The star system that the flagship is in or last visited, or nullptr. It points into star_array.
	[[nodiscard]] StarDesc*& currentStar() noexcept
	{
		return m_currentStar;
	}

private:
	FlagshipData m_sis {};
	GameData m_game {};
	GameClock m_clock {};
	FlagshipState m_flagship {};
	uqstl::array<GameStateFile, NUM_STATE_FILES> m_stateFiles {MakeStateFiles()};
	uqstl::array<uint32_t, c_gameFlagCount> m_flagValues {};
	uqstl::map<uqstl::string, uint32_t, uqstl::less<>> m_otherFlags {};
	StarDesc* m_currentStar {};
	GameSeed m_seed {};
};

// The state of the current session. Use on the game thread only.
[[nodiscard]] GameState& gameState();

// The flagship of the current session. Use on the game thread only.
[[nodiscard]] inline FlagshipState& flagship()
{
	return gameState().flagship();
}


// A flag of the current session, or 0 if it is not set.
uint32_t getGameStateUint(const char* name);
void setGameStateUint(const char* name, uint32_t val);

} // namespace uqm

#endif // UQM_SESSION_GAMESTATE_H
