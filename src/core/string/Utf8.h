// UTF-8 decoding, encoding, validation, case conversion and Windows wide-string conversion.
#pragma once
#ifndef UQM_CORE_STRING_UTF8_H_
#define UQM_CORE_STRING_UTF8_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"

#include <cstdint>

namespace uqm
{

// A Unicode code point.
using UniChar = uint32_t;

// Decodes one character and moves 'ptr' to the next one.
// Returns 0 for the terminating '\0', and also for a bad sequence, which it skips.
UniChar getCharFromString(const char** ptr);

// The number of characters before the first '\0' or bad sequence.
size_t utf8StringCount(uqstl::string_view str);
// The number of times that 'ch' occurs.
size_t utf8CharCount(uqstl::string_view str, UniChar ch);
// The character index of the first 'ch', or -1.
int utf8StringPos(uqstl::string_view str, UniChar ch);
// Skips 'count' characters, or to the end of the string.
const char* skipUTF8Chars(const char* str, size_t count);

inline char* skipUTF8Chars(char* str, size_t count)
{
	return str + (skipUTF8Chars(static_cast<const char*>(str), count) - str);
}

// Copies 'src' to 'dst' and adds '\0'. Does not cut a character in two.
// Returns nullptr if 'size' is 0.
char* utf8StringCopy(char* dst, size_t size, uqstl::string_view src);
int utf8StringCompare(uqstl::string_view str1, uqstl::string_view str2);

// Encodes 'ch' into 'out' without '\0'. Returns the number of bytes,
// 0 for a character that cannot be encoded, or minus the number of bytes needed if 'out' is too small.
int getStringFromChar(uqstl::span<char> out, UniChar ch);

// Simple Latin-1 character classes.
bool isGraphChar(UniChar ch);
bool isPrintChar(UniChar ch);
UniChar toUpperChar(UniChar ch);

bool isValidUtf8(uqstl::string_view str);

// Invalid bytes are copied unchanged.
uqstl::string toUpperUtf8(uqstl::string_view str);

#ifdef _WIN32
// Returns an empty string if 'str' is not valid UTF-8.
uqstl::wstring toWideChar(uqstl::string_view str);
#endif

} // namespace uqm

#endif // UQM_CORE_STRING_UTF8_H_
