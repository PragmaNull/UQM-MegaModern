// Tests for the linear congruential random number generator.
#include <gtest/gtest.h>

#include "core/stl/array.h"
#include "libs/math/Random.h"

using namespace uqm;

namespace
{

class FixedRandom final : public Random<FixedRandom>
{
public:
	using Random::Random;

	static uint32_t multiplier() noexcept
	{
		return 123456789;
	}
};

} // namespace

TEST(Random, PrimeSequenceIsUnchanged)
{
	PrimeRandom random {};
	constexpr uqstl::array<uint32_t, 5> expected {207482415, 1790989824, 2035175616, 77048696, 24794531};
	for (const uint32_t value : expected)
	{
		EXPECT_EQ(random.next(), value);
	}
}

TEST(Random, DerivedMultiplierIsUsed)
{
	FixedRandom random {};
	constexpr uqstl::array<uint32_t, 5> expected {1508154500, 894187951, 919173206, 2007942179, 2028037541};
	for (const uint32_t value : expected)
	{
		EXPECT_EQ(random.next(), value);
	}
}

TEST(Random, ReseedReturnsOldSeedAndFixesRange)
{
	PrimeRandom random {};
	EXPECT_EQ(random.reseed(0), PrimeRandom::DefaultSeed);
	EXPECT_EQ(random.seed(), 1u);
	random.reseed(PrimeRandom::Modulus + 5);
	EXPECT_EQ(random.seed(), 5u);
}

TEST(Random, StepMatchesNext)
{
	PrimeRandom random {777};
	const uint32_t expected {PrimeRandom::step(777)};
	EXPECT_EQ(random.next(), expected);
}

TEST(Random, RangeMinMax)
{
	EXPECT_EQ(rangeMinMax(-10, 40, 0), -10);
	EXPECT_EQ(rangeMinMax(-10, 40, 50), 40);
	EXPECT_EQ(rangeMinMax(1, 4, 7), 4);
	EXPECT_EQ(rangeMinMax(5, 2, 7), 2);
}
