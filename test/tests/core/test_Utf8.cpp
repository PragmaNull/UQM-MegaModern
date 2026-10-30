// Tests for the UTF-8 helpers.
#include <gtest/gtest.h>

#include "core/string/Utf8.h"

namespace uqm
{

TEST(Utf8Test, IsValid_AsciiAndEmpty)
{
	EXPECT_TRUE(isValidUtf8(""));
	EXPECT_TRUE(isValidUtf8("Hello, World!"));
}

TEST(Utf8Test, IsValid_MultiByte)
{
	EXPECT_TRUE(isValidUtf8("k\xc3\xa4se"));	  // käse
	EXPECT_TRUE(isValidUtf8("\xe2\x82\xac"));	  // €
	EXPECT_TRUE(isValidUtf8("\xf0\x9f\x9a\x80")); // U+1F680
}

TEST(Utf8Test, IsValid_RejectsMalformed)
{
	EXPECT_FALSE(isValidUtf8("\x80"));			   // lone continuation byte
	EXPECT_FALSE(isValidUtf8("\xc3"));			   // truncated sequence
	EXPECT_FALSE(isValidUtf8("\xc0\xaf"));		   // overlong encoding
	EXPECT_FALSE(isValidUtf8("\xed\xa0\x80"));	   // surrogate U+D800
	EXPECT_FALSE(isValidUtf8("\xf4\x90\x80\x80")); // above U+10FFFF
}

TEST(Utf8Test, ToUpper_Ascii)
{
	EXPECT_EQ(toUpperUtf8("Hello, World 42!"), "HELLO, WORLD 42!");
	EXPECT_EQ(toUpperUtf8(""), "");
}

TEST(Utf8Test, ToUpper_Latin1)
{
	EXPECT_EQ(toUpperUtf8("k\xc3\xa4se \xc3\xb6l \xc3\xbc"), "K\xc3\x84SE \xc3\x96L \xc3\x9c"); // käse öl ü
	EXPECT_EQ(toUpperUtf8("\xc3\xa9t\xc3\xa9"), "\xc3\x89T\xc3\x89");							// été
}

TEST(Utf8Test, ToUpper_GreekAndCyrillic)
{
	EXPECT_EQ(toUpperUtf8("\xce\xb1\xce\xb2\xce\xb3"), "\xce\x91\xce\x92\xce\x93"); // αβγ
	EXPECT_EQ(toUpperUtf8("\xd0\xbc\xd0\xb8\xd1\x80"), "\xd0\x9c\xd0\x98\xd0\xa0"); // мир
}

TEST(Utf8Test, ToUpper_SharpS)
{
	EXPECT_EQ(toUpperUtf8("stra\xc3\x9f\x65"), "STRA\xe1\xba\x9e\x45"); // straße -> STRAẞE
}

TEST(Utf8Test, ToUpper_LengthCanChange)
{
	EXPECT_EQ(toUpperUtf8("\xc9\x90"), "\xe2\xb1\xaf"); // U+0250 -> U+2C6F
}

TEST(Utf8Test, ToUpper_InvalidBytesCopied)
{
	EXPECT_EQ(toUpperUtf8("a\x80\x62\xc3"), "A\x80\x42\xc3");
}

#ifdef _WIN32
TEST(Utf8Test, ToWideChar_Converts)
{
	EXPECT_EQ(toWideChar("Hello"), L"Hello");
	EXPECT_EQ(toWideChar("k\xc3\xa4se \xe2\x82\xac"), L"käse €");
	EXPECT_EQ(toWideChar("\xf0\x9f\x9a\x80"), L"\U0001F680");
}

TEST(Utf8Test, ToWideChar_NoTrailingNull)
{
	EXPECT_EQ(toWideChar("abc").size(), 3u);
}

TEST(Utf8Test, ToWideChar_EmptyOrInvalid)
{
	EXPECT_TRUE(toWideChar("").empty());
	EXPECT_TRUE(toWideChar("a\x80").empty());
}
#endif

TEST(Utf8Test, GetCharFromString_DecodesAndAdvances)
{
	const char* text {"a\xc3\xa4\xe2\x82\xac\xf0\x9f\x9a\x80"}; // a ä € U+1F680
	const char* ptr {text};
	EXPECT_EQ(getCharFromString(&ptr), UniChar {'a'});
	EXPECT_EQ(getCharFromString(&ptr), UniChar {0xe4});
	EXPECT_EQ(getCharFromString(&ptr), UniChar {0x20ac});
	EXPECT_EQ(getCharFromString(&ptr), UniChar {0x1f680});
	EXPECT_EQ(ptr, text + 10);
	EXPECT_EQ(getCharFromString(&ptr), UniChar {0});
	EXPECT_EQ(ptr, text + 11);
}

TEST(Utf8Test, GetCharFromString_SkipsBadSequence)
{
	const char* text {"\xc3(\x80\x80z"};
	const char* ptr {text};
	EXPECT_EQ(getCharFromString(&ptr), UniChar {0});
	EXPECT_EQ(*ptr, '(');

	const char* overlong {"\xc0\x80x"};
	ptr = overlong;
	EXPECT_EQ(getCharFromString(&ptr), UniChar {0});
	EXPECT_EQ(*ptr, 'x');
}

TEST(Utf8Test, CountAndPosition)
{
	EXPECT_EQ(utf8StringCount("k\xc3\xa4se"), 4u);
	EXPECT_EQ(utf8StringCount(""), 0u);
	EXPECT_EQ(utf8StringCount(uqstl::string_view {"abcdef", 3}), 3u);
	EXPECT_EQ(utf8CharCount("|12|x|", '|'), 3u);
	EXPECT_EQ(utf8StringPos("k\xc3\xa4se", 's'), 2);
	EXPECT_EQ(utf8StringPos("abc", 'z'), -1);
	EXPECT_EQ(utf8StringPos("abc", 0), 3);
}

TEST(Utf8Test, SkipChars)
{
	const char* text {"\xc3\xa4\xc3\xb6x"};
	EXPECT_EQ(skipUTF8Chars(text, 2), text + 4);
	EXPECT_EQ(skipUTF8Chars(text, 10), text + 5);

	char buffer[] {"ab"};
	char* mutableEnd {skipUTF8Chars(buffer, 1)};
	EXPECT_EQ(mutableEnd, buffer + 1);
}

TEST(Utf8Test, CopyDoesNotSplitCharacters)
{
	char buffer[4] {};
	EXPECT_EQ(utf8StringCopy(buffer, sizeof(buffer), "ab\xc3\xa4"), buffer);
	EXPECT_STREQ(buffer, "ab");

	EXPECT_EQ(utf8StringCopy(buffer, sizeof(buffer), "abc"), buffer);
	EXPECT_STREQ(buffer, "abc");

	EXPECT_EQ(utf8StringCopy(buffer, 0, "abc"), nullptr);
}

TEST(Utf8Test, GetStringFromChar_Encodes)
{
	char out[4] {};
	EXPECT_EQ(getStringFromChar(out, 0x20ac), 3);
	EXPECT_EQ(uqstl::string_view(out, 3), "\xe2\x82\xac");
	EXPECT_EQ(getStringFromChar(uqstl::span {out, 1}, 0x20ac), -3);
	EXPECT_EQ(getStringFromChar(out, 0x110000), 0);
}

TEST(Utf8Test, CharClasses)
{
	EXPECT_TRUE(isGraphChar('a'));
	EXPECT_FALSE(isGraphChar(' '));
	EXPECT_TRUE(isPrintChar(' '));
	EXPECT_FALSE(isGraphChar(0xe000));
	EXPECT_EQ(toUpperChar('a'), UniChar {'A'});
	EXPECT_EQ(toUpperChar(0x20ac), UniChar {0x20ac});
}

} // namespace uqm
