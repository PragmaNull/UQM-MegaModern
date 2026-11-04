// The random number generator of the game. A planet seed changes its multiplier.
#pragma once
#ifndef UQM_RANDOM_UQMRANDOM_H_
#define UQM_RANDOM_UQMRANDOM_H_

#include "libs/math/Random.h"
#include "options/OptionDefs.h"
#include "uqm/session/GameState.h"

#include <cstdint>

// The seed type of the running game.
extern uqm::SeedType g_seedType;

#define PrimeSeed (g_seedType == uqm::SeedType::Prime)
#define StarSeed (g_seedType > uqm::SeedType::Planet)

namespace uqm
{

class UQMRandom final : public Random<UQMRandom>
{
public:
	using Random::Random;

	// The planet seed of the game if the seed type is Planet and the seed is sane, else PrimeA.
	[[nodiscard]] static uint32_t multiplier() noexcept
	{
		if (g_seedType == SeedType::Planet)
		{
			const int32_t planetSeed {gameState().sis().Seed};
			if (SANE_SEED(planetSeed))
			{
				return static_cast<uint32_t>(planetSeed);
			}
		}
		return PrimeA;
	}
};

// The generator of the current game session.
[[nodiscard]] UQMRandom& random();

} // namespace uqm

#endif // UQM_RANDOM_UQMRANDOM_H_
