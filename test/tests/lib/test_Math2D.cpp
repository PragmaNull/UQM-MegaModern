#include <gtest/gtest.h>


#include "lib/Math2D/Math2D.h"
#include "lib/Math2D/Math2DStringUtils.h"
#include <fmt/format.h>

using namespace uqm;

TEST(Math2D, BasicArithmeticAndCompoundAssignments)
{
	const Vec2T<int> a {3, 4};
	const Vec2T<int> b {2, 2};

	auto sum = a + b;
	EXPECT_EQ(sum.x, 5);
	EXPECT_EQ(sum.y, 6);

	auto diff = a - b;
	EXPECT_EQ(diff.x, 1);
	EXPECT_EQ(diff.y, 2);

	auto prod = a * b; // elementwise
	EXPECT_EQ(prod.x, 6);
	EXPECT_EQ(prod.y, 8);

	Vec2T<int> c {5, 6};
	c += Vec2T<int> {1, 2};
	EXPECT_EQ(c.x, 6);
	EXPECT_EQ(c.y, 8);

	c -= Vec2T<int> {1, 1};
	EXPECT_EQ(c.x, 5);
	EXPECT_EQ(c.y, 7);

	c *= Vec2T<int> {2, 3};
	EXPECT_EQ(c.x, 10);
	EXPECT_EQ(c.y, 21);

	c /= Vec2T<int> {2, 3};
	// integer division
	EXPECT_EQ(c.x, 5);
	EXPECT_EQ(c.y, 7);
}

TEST(Math2D, ComparisonAndThreeWay)
{
	const Vec2T<int> a {1, 2};
	const Vec2T<int> b {1, 2};
	const Vec2T<int> c {2, 3};

	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
	EXPECT_TRUE(a != c);
	// spaceship defaulted: equality tested above; ordering uses lexicographic default
	EXPECT_EQ((a <=> c) < 0, true);
}

TEST(Math2D, ScalarMultiplication_ReturnTypesAndValues)
{
	const Vec2T<int> vi {2, 3};

	// int * unsigned -> returns Vec2<unsigned>
	auto r1 = vi * 4u;
	using R1T = decltype(r1);
	static_assert(std::is_same_v<R1T, Vec2T<unsigned int>>, "Expected unsigned return type");
	EXPECT_EQ(r1.x, static_cast<unsigned int>(8));
	EXPECT_EQ(r1.y, static_cast<unsigned int>(12));

	// float * int scalar -> returns Vec2<float> (lhs float)
	const Vec2T<float> vf {1.5f, 2.5f};
	auto r2 = vf * 2;
	using R2T = decltype(r2);
	static_assert(std::is_same_v<R2T, Vec2T<float>>, "Expected float return type");
	EXPECT_FLOAT_EQ(r2.x, 3.0f);
	EXPECT_FLOAT_EQ(r2.y, 5.0f);

	// int * double scalar -> returns Vec2<double>
	auto r3 = vi * 2.0;
	using R3T = decltype(r3);
	static_assert(std::is_same_v<R3T, Vec2T<double>>, "Expected double return type");
	EXPECT_DOUBLE_EQ(r3.x, 4.0);
	EXPECT_DOUBLE_EQ(r3.y, 6.0);

	// When scalar is smaller type, check promotion when sizeof(T) > sizeof(N)
	using SmallInt = short;
	const Vec2T<int> vi2 {3, 4};
	auto r4 = vi2 * static_cast<SmallInt>(2);
	// sizeof(int) > sizeof(short) -> returns Vec2<int>
	static_assert(std::is_same_v<decltype(r4), Vec2T<int>>, "Expected Vec2<int>");
	EXPECT_EQ(r4.x, 6);
	EXPECT_EQ(r4.y, 8);
}

TEST(Math2D, ToStringPrimaryTemplateAndLengthError)
{
	// Use a type that does not have an explicit specialization declared in cpp
	// (float/double/uint32 have explicit declarations there — avoid them).
	const Vec2T<long long> v {123, -456};

	// Provide a buffer large enough
	std::array<char, 64> buf {};
	const uqstl::span<char> span {buf.data(), buf.size()};
	const char* s = toString<long long>(v, span);
	ASSERT_NE(s, nullptr);
	const std::string got {s};
	// expected format "123,-456" (fmt default)
	EXPECT_NE(got.find("123"), std::string::npos);
	EXPECT_NE(got.find("-456"), std::string::npos);

	// Now test too-small buffer triggers std::length_error
	std::array<char, 2> smallBuf {}; // intentionally tiny
	const uqstl::span<char> smallSpan {smallBuf.data(), smallBuf.size()};
	EXPECT_THROW(toString<long long>(v, smallSpan), std::length_error);
}

TEST(Math2D, StreamExtractionValidAndInvalid)
{
	// valid comma separated
	{
		std::istringstream in("10,20");
		Vec2T<int> out {};
		in >> out;
		EXPECT_FALSE(in.fail());
		EXPECT_EQ(out.x, 10);
		EXPECT_EQ(out.y, 20);
	}

	// valid 'x' separator
	{
		std::istringstream in("3x4");
		Vec2T<int> out {};
		in >> out;
		EXPECT_FALSE(in.fail());
		EXPECT_EQ(out.x, 3);
		EXPECT_EQ(out.y, 4);
	}

	// whitespace between numbers and separator
	{
		std::istringstream in("  7 ,  8 ");
		Vec2T<int> out {};
		in >> out;
		// operator>> uses formatted extraction which will consume whitespace before numeric input
		EXPECT_FALSE(in.fail());
		EXPECT_EQ(out.x, 7);
		EXPECT_EQ(out.y, 8);
	}

	// invalid separator -> failbit set
	{
		std::istringstream in("1;2");
		Vec2T<int> out {};
		in >> out;
		EXPECT_TRUE(in.fail());
	}

	// non-numeric values -> fail
	{
		std::istringstream in("a,b");
		Vec2T<int> out {};
		in >> out;
		EXPECT_TRUE(in.fail());
	}
}

TEST(Math2D, FmtFormatter_DefaultFormatting)
{
	Vec2T<double> v {1.256, -3.56};
	// default formatting uses the base formatter for doubles and inserts comma between components
	auto s = fmt::format("{}", v);
	// Should contain both components separated by a comma
	EXPECT_NE(s.find("1.256"), std::string::npos);
	EXPECT_NE(s.find("-3.56"), std::string::npos);
	EXPECT_NE(s.find(','), std::string::npos);

	// Test formatting with precision via format spec for components (use e.g. ".1")
	// Format spec is passed to base formatter, e.g. "{:.1}" -> we need to supply same spec for both components.
	// The custom formatter consumes optional wrapper char then forwards remaining spec to base formatter.
	auto s2 = fmt::format("{:.{}}", v, 3);
	// Components formatted with 2 decimals
	EXPECT_NE(s2.find("1.26"), std::string::npos);
	EXPECT_NE(s2.find("-3.56"), std::string::npos);
}

TEST(Math2D, FmtFormatter_FormattingWitBraces)
{
	Vec2T<double> v {1.256, -3.56};

	// surround with braces.
	auto s = fmt::format("{:(}", v);
	EXPECT_STREQ("(1.256,-3.56)", s.c_str());

	auto s2 = fmt::format("{:<}", v);
	EXPECT_STREQ("<1.256,-3.56>", s2.c_str());

	// mix bracket specification with number formatting
	Vec2T<int> vi {123, 456};
	auto s3 = fmt::format("{:[#06x}", vi);
	EXPECT_STREQ("[0x007b,0x01c8]", s3.c_str());
}

TEST(Math2D, EdgeCasesLargeValuesUnsigned)
{
	// large unsigned values formatting & arithmetic
	const Vec2T<uint32_t> u {std::numeric_limits<uint32_t>::max(), 0u};
	// scalar multiply by 1 should keep values
	auto res = u * 1u;
	static_assert(std::is_same_v<decltype(res), Vec2T<uint32_t>>);
	EXPECT_EQ(res.x, std::numeric_limits<uint32_t>::max());
	EXPECT_EQ(res.y, 0u);

	// toString for a large signed type (primary template)
	const Vec2T<long long> big {std::numeric_limits<long long>::max(), std::numeric_limits<long long>::min()};
	std::array<char, 128> buf {};
	EXPECT_NO_THROW({
		auto out = toString<long long>(big, uqstl::span<char> {buf.data(), buf.size()});
		EXPECT_NE(std::string(out).size(), 0u);
	});
}

TEST(Math2D, MixedSignedUnsignedBehavior)
{
	const Vec2T<int> vi {-1, 2};
	// multiply by unsigned scalar -> returns Vec2<unsigned>
	auto r = vi * 3u;
	static_assert(std::is_same_v<decltype(r), Vec2T<unsigned int>>);
	// -1 as unsigned wraps; verify cast behavior occurs as in code
	EXPECT_EQ(r.x, static_cast<unsigned int>(static_cast<unsigned int>(-1) * 3u));
}
