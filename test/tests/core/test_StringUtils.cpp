#include <gtest/gtest.h>

#include "core/string/StringUtils.h"
// Optional: include a project header once you start testing real code
// #include "foo/foo.h"

namespace uqm
{

class StringUtilsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Optional setup code
	}

	void TearDown() override
	{
		// Optional cleanup code
	}
};

TEST_F(StringUtilsTest, ToStringLiteral)
{
	EXPECT_STREQ(toString(true), "true");
	EXPECT_STREQ(toString(false), "false");
}

TEST_F(StringUtilsTest, ToStringWithBuffer_Integer)
{
	uqstl::array<char, 32> buf {};
	auto s = toString(12345, buf);
	EXPECT_STREQ(s, "12345");
}

TEST_F(StringUtilsTest, ToStringWithBuffer_SmallBuffer_Bool)
{
	// Buffer too small to hold "true" (4) or "false" (5) plus NUL
	uqstl::array<char, 3> buf {};
	auto s = toString(true, buf);
	EXPECT_STREQ(s, ""); // implementation writes empty string when too small

	buf.fill(0);
	s = toString(false, buf);
	EXPECT_STREQ(s, ""); // too small -> empty
}

TEST_F(StringUtilsTest, ToStringWithBuffer_ExactAndJustBigEnough)
{
	// true length == 4, need buffer.size() > 4 to succeed per impl
	uqstl::array<char, 5> bufTrue {};
	auto s = toString(true, bufTrue);
	EXPECT_STREQ(s, "true");

	// false length == 5, need buffer.size() > 5 to succeed; with 6 it should succeed
	uqstl::array<char, 6> bufFalse {};
	s = toString(false, bufFalse);
	EXPECT_STREQ(s, "false");
}

TEST_F(StringUtilsTest, ToStringFloatFormatting)
{
	uqstl::array<char, 32> buf {};
	auto s = toString(3.14f, buf);
	// fmt by default prints a compact representation
	EXPECT_NE(uqstl::string(s).find("3.14"), uqstl::string::npos);
}

TEST_F(StringUtilsTest, CompareICaseEdgeCases)
{
	EXPECT_TRUE(compareCharICase('A', 'a'));
	EXPECT_FALSE(compareICase("abc", "abcd")); // different lengths
	//EXPECT_TRUE(compareICase(u8"�", u8"�"));	   // identical non-ascii byte-equal
}

TEST_F(StringUtilsTest, IsEmptyVariants)
{
	EXPECT_TRUE(isEmpty(nullptr));
	EXPECT_TRUE(isEmpty(""));
	EXPECT_TRUE(isEmpty(uqstl::string_view("")));
	EXPECT_TRUE(isEmpty(uqstl::string()));
	EXPECT_FALSE(isEmpty("x"));
	EXPECT_FALSE(isEmpty(uqstl::string_view("x")));
	EXPECT_FALSE(isEmpty(uqstl::string("y")));
}

TEST_F(StringUtilsTest, CStrTemplates)
{
	const uqstl::string s = "abc";
	EXPECT_STREQ(c_str(s), "abc");

	const char* c = "xyz";
	EXPECT_STREQ(c_str<const char*>(c), "xyz");

	const uqstl::string_view sv = "hello";
	EXPECT_STREQ(c_str(sv), "hello");
}

TEST_F(StringUtilsTest, ParseBooleanSpecialization_VariousInputs)
{
	bool out = false;
	auto ec = parseStr("TrUe", out);
	EXPECT_EQ(ec, uqstl::errc {});
	EXPECT_TRUE(out);

	ec = parseStr("off", out);
	EXPECT_EQ(ec, uqstl::errc {});
	EXPECT_FALSE(out);

	// Numeric boolean forms:
	ec = parseStr("1", out);
	EXPECT_EQ(ec, uqstl::errc {});
	EXPECT_TRUE(out);

	ec = parseStr("0", out);
	EXPECT_EQ(ec, uqstl::errc {});
	EXPECT_FALSE(out);

	// Leading/trailing whitespace should NOT be accepted (implementation uses direct equality)
	ec = parseStr(" yes", out);
	EXPECT_EQ(ec, uqstl::errc::invalid_argument);
	ec = parseStr("no ", out);
	EXPECT_EQ(ec, uqstl::errc::invalid_argument);

	// Completely invalid
	ec = parseStr("invalid_bool", out);
	EXPECT_EQ(ec, uqstl::errc::invalid_argument);
}

TEST_F(StringUtilsTest, ParseIntegers_EdgeCases)
{
	int value = 0;

	// Normal parse
	auto ec = parseStr("42", value);
	EXPECT_EQ(ec, uqstl::errc {});
	EXPECT_EQ(value, 42);

	// Characters after the number are an error, and the value does not change
	ec = parseStr("12abc", value);
	EXPECT_EQ(ec, uqstl::errc::invalid_argument);
	EXPECT_EQ(value, 42);

	// Leading whitespace is not skipped by from_chars -> invalid_argument
	ec = parseStr(" 45", value);
	EXPECT_EQ(ec, uqstl::errc::invalid_argument);

	// Overflow should return result_out_of_range
	long long big = 0;
	uqstl::string huge = uqstl::to_string(uqstl::numeric_limits<long long>::max());
	huge.push_back('9'); // make it overflow
	ec = parseStr(huge, big);
	EXPECT_EQ(ec, uqstl::errc::result_out_of_range);
}

TEST_F(StringUtilsTest, ParseEnumUnderlying)
{
	enum class TestColor : int
	{
		Red = 1,
		Blue = 2,
		Green = 3
	};
	TestColor c = TestColor::Red;

	// valid numeric enum string
	auto ec = parseStr("2", c);
	EXPECT_EQ(ec, uqstl::errc {});
	EXPECT_EQ(c, TestColor::Blue);

	// invalid
	ec = parseStr("x", c);
	EXPECT_EQ(ec, uqstl::errc::invalid_argument);
}

enum class TestEnum
{
	a=10,
	b,
	c
};

enum class TestEnumWithToStringImpl
{
	x=20,
	y,
	z
};
static constexpr const char* toStringImpl(TestEnumWithToStringImpl v)
{
	if (v == TestEnumWithToStringImpl::x)
	{
		return "x-string";
	}
	if (v == TestEnumWithToStringImpl::y)
	{
		return "y-string";
	}
	if (v == TestEnumWithToStringImpl::z)
	{
		return "z-string";
	}
	return "???";
}

TEST_F(StringUtilsTest, FormatEnum)
{
	EXPECT_STREQ("10", fmt::format("{}", TestEnum::a).c_str());
	EXPECT_STREQ("a", fmt::format("{:s}", TestEnum::a).c_str());
	EXPECT_STREQ("a->10", fmt::format("{:sn}", TestEnum::a).c_str());
	EXPECT_STREQ("a->0xa", fmt::format("{:sn#x}", TestEnum::a).c_str());

	EXPECT_STREQ("20", fmt::format("{}", TestEnumWithToStringImpl::x).c_str());
	EXPECT_STREQ("x-string", fmt::format("{:s}", TestEnumWithToStringImpl::x).c_str());
	EXPECT_STREQ("x-string->20", fmt::format("{:sn}", TestEnumWithToStringImpl::x).c_str());
	EXPECT_STREQ("x-string->0x14", fmt::format("{:sn#x}", TestEnumWithToStringImpl::x).c_str());
}

TEST_F(StringUtilsTest, FormatEnumNames)
{
	EXPECT_STREQ("10, 11, 12", fmt::format("{}", EnumNames<TestEnum>{}).c_str());
	EXPECT_STREQ("a, b, c", fmt::format("{:s}", EnumNames<TestEnum>{}).c_str());
	EXPECT_STREQ("a->10 | b->11 | c->12", fmt::format("{:|sn}", EnumNames<TestEnum>{}).c_str());

	EXPECT_STREQ("20 | 21 | 22", fmt::format("{:|}", EnumNames<TestEnumWithToStringImpl> {}).c_str());
	EXPECT_STREQ("x-string, y-string, z-string", fmt::format("{:s}", EnumNames<TestEnumWithToStringImpl> {}).c_str());
	EXPECT_STREQ("x-string->20 | y-string->21 | z-string->22", fmt::format("{:|sn}", EnumNames<TestEnumWithToStringImpl> {}).c_str());
}

TEST_F(StringUtilsTest, StrncpySafe_BasicCopy)
{
	uqstl::array<char, 10> dest {};
	const uqstl::string_view src = "hello";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 5);
	EXPECT_STREQ(dest.data(), "hello");
	EXPECT_EQ(dest[5], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_ExactFit)
{
	uqstl::array<char, 6> dest {}; // exactly "hello" + '\0'
	const uqstl::string_view src = "hello";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 5);
	EXPECT_STREQ(dest.data(), "hello");
	EXPECT_EQ(dest[5], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_Truncation)
{
	uqstl::array<char, 6> dest {}; // can hold 5 chars + '\0'
	const uqstl::string_view src = "hello world";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 5);
	EXPECT_STREQ(dest.data(), "hello");
	EXPECT_EQ(dest[5], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_EmptySource)
{
	uqstl::array<char, 10> dest {};
	dest.fill('x'); // fill with non-null
	const uqstl::string_view src = "";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 0);
	EXPECT_EQ(dest[0], '\0'); // empy source should still null-terminate
}

TEST_F(StringUtilsTest, StrncpySafe_EmptyDestination)
{
	uqstl::array<char, 0> dest {};
	const uqstl::string_view src = "hello";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 0);
}

TEST_F(StringUtilsTest, StrncpySafe_SingleCharBuffer)
{
	uqstl::array<char, 1> dest {};
	const uqstl::string_view src = "hello";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 0);
	EXPECT_EQ(dest[0], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_TwoCharBuffer)
{
	uqstl::array<char, 2> dest {};
	const uqstl::string_view src = "hello";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 1);
	EXPECT_EQ(dest[0], 'h');
	EXPECT_EQ(dest[1], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_SourceLongerThanDest)
{
	uqstl::array<char, 5> dest {};
	const uqstl::string_view src = "abcdefghijklmnop";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 4);
	EXPECT_STREQ(dest.data(), "abcd");
	EXPECT_EQ(dest[4], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_SourceShorterThanDest)
{
	uqstl::array<char, 20> dest {};
	dest.fill('x'); // fill with non-null to verify proper termination
	const uqstl::string_view src = "hi";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 2);
	EXPECT_STREQ(dest.data(), "hi");
	EXPECT_EQ(dest[2], '\0');
	// Note: unlike strncpy, this doesn't zero-fill the rest
	EXPECT_EQ(dest[3], 'x'); // original content remains
}

TEST_F(StringUtilsTest, StrncpySafe_SpecialCharacters)
{
	uqstl::array<char, 20> dest {};
	const uqstl::string_view src = "line1\nline2\ttab";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, src.size());
	EXPECT_STREQ(dest.data(), "line1\nline2\ttab");
}

TEST_F(StringUtilsTest, StrncpySafe_NullTerminatorInSource)
{
	// string_view can contain embedded nulls
	const char srcData[] = {'a', 'b', '\0', 'c', 'd'};
	const uqstl::string_view src(srcData, 5);
	uqstl::array<char, 10> dest {};

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 2);
	// The copied data includes the embedded null
	EXPECT_EQ(dest[0], 'a');
	EXPECT_EQ(dest[1], 'b');
	EXPECT_EQ(dest[2], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_MaxSizeString)
{
	uqstl::array<char, 100> dest {};
	const uqstl::string longSrc(99, 'a'); // 99 'a' characters

	const size_t copied = strncpy_safe(dest, longSrc);

	EXPECT_EQ(copied, 99);
	EXPECT_EQ(dest[99], '\0');
	EXPECT_EQ(uqstl::strlen(dest.data()), 99);
}

TEST_F(StringUtilsTest, StrncpySafe_OverwriteExistingData)
{
	uqstl::array<char, 10> dest {};
	const uqstl::string_view src1 = "hello";
	const uqstl::string_view src2 = "hi";

	strncpy_safe(dest, src1);
	EXPECT_STREQ(dest.data(), "hello");

	// Overwrite with shorter string
	const size_t copied = strncpy_safe(dest, src2);

	EXPECT_EQ(copied, 2);
	EXPECT_STREQ(dest.data(), "hi");
	// The 'l' characters remain after the new null terminator
	EXPECT_EQ(dest[2], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_WithCStyleString)
{
	uqstl::array<char, 10> dest {};
	const char* cstr = "test";
	const uqstl::string_view src(cstr);

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 4);
	EXPECT_STREQ(dest.data(), "test");
}

TEST_F(StringUtilsTest, StrncpySafe_WithStdString)
{
	uqstl::array<char, 10> dest {};
	const uqstl::string stdstr = "example";
	const uqstl::string_view src(stdstr);

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 7);
	EXPECT_STREQ(dest.data(), "example");
}

TEST_F(StringUtilsTest, StrncpySafe_ReturnValue)
{
	uqstl::array<char, 5> dest {};

	// When truncated, returns number actually copied (not including null)
	const size_t copied1 = strncpy_safe(dest, "abcdefgh");
	EXPECT_EQ(copied1, 4);

	// When not truncated, returns source size
	dest.fill(0);
	const size_t copied2 = strncpy_safe(dest, "abc");
	EXPECT_EQ(copied2, 3);

	// Empty source
	dest.fill(0);
	const size_t copied3 = strncpy_safe(dest, "");
	EXPECT_EQ(copied3, 0);
}

TEST_F(StringUtilsTest, StrncpySafe_BoundaryConditions)
{
	// Test SIZE_MAX scenario (unlikely but important for safety)
	uqstl::array<char, 3> dest {};
	const uqstl::string_view src = "ab";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 2);
	EXPECT_STREQ(dest.data(), "ab");
	EXPECT_EQ(dest[2], '\0');
}

TEST_F(StringUtilsTest, StrncpySafe_SpanFromVector)
{
	uqstl::vector<char> dest(10);
	const uqstl::string_view src = "vector";

	const size_t copied = strncpy_safe(uqstl::span<char>(dest), src);

	EXPECT_EQ(copied, 6);
	EXPECT_STREQ(dest.data(), "vector");
}

TEST_F(StringUtilsTest, StrncpySafe_AllWhitespace)
{
	uqstl::array<char, 10> dest {};
	const uqstl::string_view src = "   \t\n  ";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, src.size());
	EXPECT_STREQ(dest.data(), "   \t\n  ");
}

TEST_F(StringUtilsTest, StrncpySafe_NumericStrings)
{
	uqstl::array<char, 15> dest {};
	const uqstl::string_view src = "1234567890";

	const size_t copied = strncpy_safe(dest, src);

	EXPECT_EQ(copied, 10);
	EXPECT_STREQ(dest.data(), "1234567890");
}

TEST_F(StringUtilsTest, StrncpySafe_Appending)
{
	uqstl::array<char, 15> dest {};
	const uqstl::string_view src = "1234567890";

	std::span<char> destSpan(dest);
	const size_t copied = strncpy_safe(destSpan, src.substr(0, 5));
	EXPECT_EQ(copied, 5);
	EXPECT_STREQ(dest.data(), "12345");

	destSpan = destSpan.subspan(5);
	const size_t copied2 = strncpy_safe(destSpan, src.substr(5));
	EXPECT_EQ(copied2, 5);
	EXPECT_STREQ(dest.data(), "1234567890");
}

// ─── tokenize (includeEmpty = true) ─────────────────────────────────────────

TEST_F(StringUtilsTest, Tokenize_CharDelim_BasicSplit)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,b,c", out, ',', true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
	EXPECT_EQ(out[2], "c");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_NoDelimiterPresent)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("hello", out, ',', true);
	ASSERT_EQ(out.size(), 1u);
	EXPECT_EQ(out[0], "hello");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_EmptyInput)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("", out, ',', true);
	ASSERT_EQ(out.size(), 1u);
	EXPECT_EQ(out[0], "");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_LeadingDelimiter)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize(",a,b", out, ',', true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "");
	EXPECT_EQ(out[1], "a");
	EXPECT_EQ(out[2], "b");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_TrailingDelimiter)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,b,", out, ',', true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
	EXPECT_EQ(out[2], "");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_ConsecutiveDelimiters)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,,b", out, ',', true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "");
	EXPECT_EQ(out[2], "b");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_OnlyDelimiters)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize(",,,", out, ',', true);
	ASSERT_EQ(out.size(), 4u);
	for (const auto& token : out)
	{
		EXPECT_EQ(token, "");
	}
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_SingleCharTokens)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a", out, ',', true);
	ASSERT_EQ(out.size(), 1u);
	EXPECT_EQ(out[0], "a");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_WhitespaceDelimiter)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("hello world foo", out, ' ', true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "hello");
	EXPECT_EQ(out[1], "world");
	EXPECT_EQ(out[2], "foo");
}

TEST_F(StringUtilsTest, Tokenize_CharDelim_TokensAreViewsIntoOriginal)
{
	const uqstl::string_view input = "one,two,three";
	uqstl::vector<uqstl::string_view> out;
	tokenize(input, out, ',', true);
	ASSERT_EQ(out.size(), 3u);
	// Each token should point into the original buffer
	EXPECT_EQ(out[0].data(), input.data());
	EXPECT_EQ(out[1].data(), input.data() + 4);
	EXPECT_EQ(out[2].data(), input.data() + 8);
}

TEST_F(StringUtilsTest, Tokenize_SvDelim_BasicCharSet)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,b;c", out, ",;", true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
	EXPECT_EQ(out[2], "c");
}

TEST_F(StringUtilsTest, Tokenize_SvDelim_MixedDelimiters)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("hello world\tfoo", out, " \t", true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "hello");
	EXPECT_EQ(out[1], "world");
	EXPECT_EQ(out[2], "foo");
}

TEST_F(StringUtilsTest, Tokenize_SvDelim_NoDelimiterInInput)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("hello", out, ",;", true);
	ASSERT_EQ(out.size(), 1u);
	EXPECT_EQ(out[0], "hello");
}

TEST_F(StringUtilsTest, Tokenize_SvDelim_EmptyInput)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("", out, ",", true);
	ASSERT_EQ(out.size(), 1u);
	EXPECT_EQ(out[0], "");
}

TEST_F(StringUtilsTest, Tokenize_SvDelim_ConsecutiveDelimiters)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,,b", out, ",", true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "");
	EXPECT_EQ(out[2], "b");
}

TEST_F(StringUtilsTest, Tokenize_SvDelim_TrailingDelimiter)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("x.y-", out, ".-", true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "x");
	EXPECT_EQ(out[1], "y");
	EXPECT_EQ(out[2], "");
}

TEST_F(StringUtilsTest, Tokenize_SvDelim_SingleCharDelimSet)
{
	// string_view with one char behaves the same as a char delimiter
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,b,c", out, ",", true);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
	EXPECT_EQ(out[2], "c");
}

// ─── tokenize (includeEmpty = false) ────────────────────────────────────────

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_BasicSplitUnchanged)
{
	// No empty tokens in input — result is identical to includeEmpty=true
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,b,c", out, ',', false);
	ASSERT_EQ(out.size(), 3u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
	EXPECT_EQ(out[2], "c");
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_LeadingDelimiterSkipped)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize(",a,b", out, ',', false);
	ASSERT_EQ(out.size(), 2u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_TrailingDelimiterSkipped)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,b,", out, ',', false);
	ASSERT_EQ(out.size(), 2u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_ConsecutiveDelimitersSkipped)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("a,,b", out, ',', false);
	ASSERT_EQ(out.size(), 2u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_OnlyDelimiters)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize(",,,", out, ',', false);
	EXPECT_TRUE(out.empty());
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_EmptyInput)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("", out, ',', false);
	EXPECT_TRUE(out.empty());
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_LeadingAndTrailing)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize(",a,b,", out, ',', false);
	ASSERT_EQ(out.size(), 2u);
	EXPECT_EQ(out[0], "a");
	EXPECT_EQ(out[1], "b");
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_SvDelim_MixedWhitespace)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize("  hello\t\tworld  ", out, " \t", false);
	ASSERT_EQ(out.size(), 2u);
	EXPECT_EQ(out[0], "hello");
	EXPECT_EQ(out[1], "world");
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_SvDelim_OnlyDelimiters)
{
	uqstl::vector<uqstl::string_view> out;
	tokenize(",;,;", out, ",;", false);
	EXPECT_TRUE(out.empty());
}

TEST_F(StringUtilsTest, Tokenize_ExcludeEmpty_NoDelimiterPresent)
{
	// No delimiter at all — single non-empty token is always emitted
	uqstl::vector<uqstl::string_view> out;
	tokenize("hello", out, ',', false);
	ASSERT_EQ(out.size(), 1u);
	EXPECT_EQ(out[0], "hello");
}

// ─── trimLeft ───────────────────────────────────────────────────────────────

TEST_F(StringUtilsTest, TrimLeft_NoLeadingWhitespace)
{
	EXPECT_EQ(trimLeft("hello"), "hello");
	EXPECT_EQ(trimLeft("hello world"), "hello world");
}

TEST_F(StringUtilsTest, TrimLeft_LeadingSpaces)
{
	EXPECT_EQ(trimLeft("   hello"), "hello");
	EXPECT_EQ(trimLeft(" x"), "x");
}

TEST_F(StringUtilsTest, TrimLeft_LeadingMixedWhitespace)
{
	EXPECT_EQ(trimLeft(" \t\r\nhello"), "hello");
}

TEST_F(StringUtilsTest, TrimLeft_TrailingWhitespaceUntouched)
{
	EXPECT_EQ(trimLeft("hello   "), "hello   ");
}

TEST_F(StringUtilsTest, TrimLeft_BothEnds_OnlyLeftTrimmed)
{
	EXPECT_EQ(trimLeft("  hello  "), "hello  ");
}

TEST_F(StringUtilsTest, TrimLeft_EmptyInput)
{
	EXPECT_EQ(trimLeft(""), "");
}

TEST_F(StringUtilsTest, TrimLeft_AllWhitespace_ReturnsEmpty)
{
	const uqstl::string_view input {"   "};
	EXPECT_EQ(trimLeft(input), "");
	EXPECT_EQ(trimLeft(input).data(), input.data() + input.size());
}

TEST_F(StringUtilsTest, TrimLeft_CustomTrimChars)
{
	EXPECT_EQ(trimLeft("***hello", "*"), "hello");
	EXPECT_EQ(trimLeft("abcXYZ", "abc"), "XYZ");
}

TEST_F(StringUtilsTest, TrimLeft_ReturnsViewIntoOriginal)
{
	const uqstl::string_view input {"   hello"};
	const uqstl::string_view result {trimLeft(input)};
	EXPECT_EQ(result.data(), input.data() + 3);
	EXPECT_EQ(result.size(), 5u);
}

// ─── trimRight ──────────────────────────────────────────────────────────────

TEST_F(StringUtilsTest, TrimRight_NoTrailingWhitespace)
{
	EXPECT_EQ(trimRight("hello"), "hello");
	EXPECT_EQ(trimRight("hello world"), "hello world");
}

TEST_F(StringUtilsTest, TrimRight_TrailingSpaces)
{
	EXPECT_EQ(trimRight("hello   "), "hello");
	EXPECT_EQ(trimRight("x "), "x");
}

TEST_F(StringUtilsTest, TrimRight_TrailingMixedWhitespace)
{
	EXPECT_EQ(trimRight("hello \t\r\n"), "hello");
}

TEST_F(StringUtilsTest, TrimRight_LeadingWhitespaceUntouched)
{
	EXPECT_EQ(trimRight("   hello"), "   hello");
}

TEST_F(StringUtilsTest, TrimRight_BothEnds_OnlyRightTrimmed)
{
	EXPECT_EQ(trimRight("  hello  "), "  hello");
}

TEST_F(StringUtilsTest, TrimRight_EmptyInput)
{
	EXPECT_EQ(trimRight(""), "");
}

TEST_F(StringUtilsTest, TrimRight_AllWhitespace_ReturnsEmpty)
{
	const uqstl::string_view input {"   "};
	EXPECT_EQ(trimRight(input), "");
	EXPECT_EQ(trimRight(input).data(), input.data());
}

TEST_F(StringUtilsTest, TrimRight_CustomTrimChars)
{
	EXPECT_EQ(trimRight("hello***", "*"), "hello");
	EXPECT_EQ(trimRight("XYZabc", "abc"), "XYZ");
}

TEST_F(StringUtilsTest, TrimRight_ReturnsViewIntoOriginal)
{
	const uqstl::string_view input {"hello   "};
	const uqstl::string_view result {trimRight(input)};
	EXPECT_EQ(result.data(), input.data());
	EXPECT_EQ(result.size(), 5u);
}

// ─── trim ───────────────────────────────────────────────────────────────────

TEST_F(StringUtilsTest, Trim_NoWhitespace)
{
	EXPECT_EQ(trim("hello"), "hello");
}

TEST_F(StringUtilsTest, Trim_LeadingOnly)
{
	EXPECT_EQ(trim("   hello"), "hello");
}

TEST_F(StringUtilsTest, Trim_TrailingOnly)
{
	EXPECT_EQ(trim("hello   "), "hello");
}

TEST_F(StringUtilsTest, Trim_BothEnds)
{
	EXPECT_EQ(trim("  hello  "), "hello");
	EXPECT_EQ(trim(" \t hello \r\n"), "hello");
}

TEST_F(StringUtilsTest, Trim_EmptyInput)
{
	EXPECT_EQ(trim(""), "");
}

TEST_F(StringUtilsTest, Trim_AllWhitespace_ReturnsEmpty)
{
	EXPECT_EQ(trim("   "), "");
	EXPECT_EQ(trim(" \t\r\n"), "");
}

TEST_F(StringUtilsTest, Trim_InternalWhitespacePreserved)
{
	EXPECT_EQ(trim("  hello world  "), "hello world");
	EXPECT_EQ(trim("  a  b  c  "), "a  b  c");
}

TEST_F(StringUtilsTest, Trim_CustomTrimChars)
{
	EXPECT_EQ(trim("***hello***", "*"), "hello");
	EXPECT_EQ(trim("abcXYZabc", "abc"), "XYZ");
}

TEST_F(StringUtilsTest, Trim_ReturnsViewIntoOriginal)
{
	const uqstl::string_view input {"  hello  "};
	const uqstl::string_view result {trim(input)};
	EXPECT_EQ(result.data(), input.data() + 2);
	EXPECT_EQ(result.size(), 5u);
}

} // namespace uqm