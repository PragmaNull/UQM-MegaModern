// The seed that generates the star map, the planets and the seeded ships of a game.
#include "uqm/random/GameSeed.h"

#include "UQMGame.h"
#include "options/options.h"
#include "uqm/session/GameSession.h"

#include <random>


namespace uqm
{
GameSeed normalized(GameSeed seed) noexcept
{
	if (seed.type == SeedType::Original)
	{
		seed.value = PrimeA;
		seed.ships = false;
	}
	return seed;
}

GameSeed newGameSeed(const OptionsStruct& options)
{
	GameSeed seed {.type = options.seedType.value, .value = options.customSeed.value, .ships = options.shipSeed.value};
	if (options.optDifficulty == Difficulty::Hard && seed.type == SeedType::Planet)
	{
		seed.value = randomSeedValue();
	}
	return normalized(seed);
}

int32_t randomSeedValue()
{
	std::random_device device {};
	std::uniform_int_distribution<int32_t> distribution {MinSeed, MaxSeed};
	return distribution(device);
}

const GameSeed& currentSeed() noexcept
{
	if (GameSession* const session {UQMGame::getInstance().session()})
	{
		return session->state().seed();
	}
	static const GameSeed s_noSession {};
	return s_noSession;
}

} // namespace uqm
