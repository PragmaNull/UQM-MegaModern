// A linear congruential random number generator. A derived class supplies the multiplier.
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

// Copyright (c) 1989, Robert Leyland and Scott Anderson

#pragma once
#ifndef UQM_LIBS_MATH_RANDOM_H
#define UQM_LIBS_MATH_RANDOM_H

#include "libs/math/randomdefs.h"

#include <cstdint>

namespace uqm
{

// 'Derived' must have the function 'static uint32_t multiplier() noexcept'.
template <typename Derived>
class Random
{
public:
	static constexpr uint32_t Modulus {UINT32_MAX / 2};
	static constexpr uint32_t DefaultSeed {12345};

	constexpr Random() noexcept = default;
	constexpr explicit Random(uint32_t seed) noexcept
		: m_seed {seed}
	{
	}

	// Returns the next number in 1..Modulus.
	uint32_t next() noexcept
	{
		m_seed = step(m_seed);
		return m_seed;
	}

	// Sets the seed and returns the old seed. Changes a seed outside 1..Modulus to a seed in that range.
	uint32_t reseed(uint32_t seed) noexcept
	{
		if (seed == 0)
		{
			seed = 1;
		}
		else if (seed > Modulus)
		{
			seed -= Modulus;
		}

		const uint32_t oldSeed {m_seed};
		m_seed = seed;
		return oldSeed;
	}

	[[nodiscard]] constexpr uint32_t seed() const noexcept
	{
		return m_seed;
	}

	// Returns the number that follows 'seed'.
	[[nodiscard]] static uint32_t step(uint32_t seed) noexcept
	{
		const uint32_t a {Derived::multiplier()};
		const uint32_t q {Modulus / a};
		const uint32_t r {Modulus % a};

		uint32_t result {a * (seed % q) - r * (seed / q)};
		if (result > Modulus)
		{
			result -= Modulus;
		}
		else if (result == 0)
		{
			result = 1;
		}
		return result;
	}

private:
	uint32_t m_seed {DefaultSeed};
};

// The generator with the multiplier PrimeA.
class PrimeRandom final : public Random<PrimeRandom>
{
public:
	using Random::Random;

	[[nodiscard]] static constexpr uint32_t multiplier() noexcept
	{
		return PrimeA;
	}
};

// Maps 'rand' to min..max. Returns 'max' if 'min' is more than 'max'.
[[nodiscard]] constexpr int rangeMinMax(int min, int max, uint32_t rand) noexcept
{
	if (min > max)
	{
		return max;
	}
	return static_cast<int>(rand % static_cast<uint32_t>((max + 1) - min)) + min;
}

} // namespace uqm

#endif // UQM_LIBS_MATH_RANDOM_H
