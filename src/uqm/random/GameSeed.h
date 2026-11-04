// The seed that generates the star map, the planets and the seeded ships of a game.
#pragma once
#ifndef UQM_RANDOM_GAMESEED_H_
#define UQM_RANDOM_GAMESEED_H_

#include "libs/math/randomdefs.h"
#include "options/OptionDefs.h"

#include <cstdint>

namespace uqm
{

struct OptionsStruct;

struct GameSeed
{
	SeedType type {SeedType::Original};
	int32_t value {PrimeA};
	// Alien races fly ships that the seed assigns.
	bool ships {};

	friend bool operator==(const GameSeed&, const GameSeed&) = default;
};

// Returns 'seed' with the value PrimeA and no ship seeding if the type is Original.
[[nodiscard]] GameSeed normalized(GameSeed seed) noexcept;

// The seed of a new game from the settings. Hard difficulty with the type Planet gets a random value.
[[nodiscard]] GameSeed newGameSeed(const OptionsStruct& options);

// A random value in MIN_SEED..MAX_SEED.
[[nodiscard]] int32_t randomSeedValue();

// The seed of the current game. The original seed if no game session runs.
[[nodiscard]] const GameSeed& currentSeed() noexcept;

[[nodiscard]] inline bool isOriginalSeed() noexcept
{
	return currentSeed().type == SeedType::Original;
}

// The plot or the Melnorme, Rainbow World and Quasispace locations are seeded.
[[nodiscard]] inline bool isStarSeed() noexcept
{
	return currentSeed().type > SeedType::Planet;
}

} // namespace uqm

#endif // UQM_RANDOM_GAMESEED_H_
