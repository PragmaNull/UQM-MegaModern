// Tests for the byte helpers.
#include <gtest/gtest.h>

#include "core/misc/bittricks.h"

#include "core/stl/array.h"
#include "core/stl/bit.h"

using namespace uqm;

namespace
{

// Reads a value from bytes in memory order.
template <typename T>
T fromBytes(const uqstl::array<uint8_t, sizeof(T)>& bytes)
{
	return uqstl::bit_cast<T>(bytes);
}

} // namespace

TEST(BitTricksTest, ConvertsBigEndianValues)
{
	EXPECT_EQ(convertBigEndian(fromBytes<uint16_t>({0x12, 0x34})), 0x1234);
	EXPECT_EQ(convertBigEndian(fromBytes<uint32_t>({0x12, 0x34, 0x56, 0x78})), 0x12345678u);
}

TEST(BitTricksTest, ConvertsLittleEndianValues)
{
	EXPECT_EQ(convertLittleEndian(fromBytes<uint16_t>({0x34, 0x12})), 0x1234);
	EXPECT_EQ(convertLittleEndian(fromBytes<uint32_t>({0x78, 0x56, 0x34, 0x12})), 0x12345678u);
}

TEST(BitTricksTest, ConvertsBackToTheSameValue)
{
	EXPECT_EQ(convertBigEndian(convertBigEndian(uint32_t {0x12345678})), 0x12345678u);
	EXPECT_EQ(convertLittleEndian(convertLittleEndian(uint16_t {0xABCD})), 0xABCD);
}
