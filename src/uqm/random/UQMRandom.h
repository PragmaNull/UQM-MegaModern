// The random number generator of the game. A planet seed changes its multiplier.

#pragma once
#ifndef UQM_RANDOM_UQMRANDOM_H
#define UQM_RANDOM_UQMRANDOM_H

#include "libs/math/Random.h"
#include "uqm/random/GameSeed.h"

#include <cstdint>


namespace uqm
{
class UQMRandom final : public Random<UQMRandom>
{
public:
	using Random::Random;

	// The seed value of the game if the seed type is Planet and the value is sane, else PrimeA.
	[[nodiscard]] static uint32_t multiplier() noexcept
	{
		const GameSeed& seed {currentSeed()};
		if (seed.type == SeedType::Planet && SaneSeed(seed.value))
		{
			return static_cast<uint32_t>(seed.value);
		}
		return PrimeA;
	}
};

// The generator of the current game session.
[[nodiscard]] UQMRandom& random();

} // namespace uqm


#endif // UQM_RANDOM_UQMRANDOM_H
