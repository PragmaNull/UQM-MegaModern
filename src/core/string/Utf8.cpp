// UTF-8 decoding, encoding, validation, case conversion and Windows wide-string conversion.
#include "Utf8.h"

#include "core/log/log.h"

#include <utf8proc.h>

#include <algorithm>
#include <cctype>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace uqm
{

namespace
{

bool isContinuation(char c)
{
	return (static_cast<unsigned char>(c) & 0xc0) == 0x80;
}

// Decodes the first character of 'str' and removes its bytes.
// Returns 0 at the end of 'str', at '\0', and for a bad sequence, which it skips with its continuation bytes.
UniChar decodeChar(uqstl::string_view& str)
{
	if (str.empty())
	{
		return 0;
	}

	utf8proc_int32_t codepoint {};
	const utf8proc_ssize_t length {utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t*>(str.data()),
													static_cast<utf8proc_ssize_t>(str.size()), &codepoint)};
	if (length > 0)
	{
		str.remove_prefix(static_cast<size_t>(length));
		return static_cast<UniChar>(codepoint);
	}

	log::warn("Warning: Invalid UTF8 sequence starting with byte {:#04x}", static_cast<unsigned char>(str[0]));
	size_t skip {1};
	while (skip < str.size() && isContinuation(str[skip]))
	{
		++skip;
	}
	str.remove_prefix(skip);
	return 0;
}

} // namespace

UniChar getCharFromString(const char** ptr)
{
	if (**ptr == '\0')
	{
		++*ptr;
		return 0;
	}

	// A character has at most 4 bytes. Do not read past the terminating '\0'.
	uqstl::string_view rest {*ptr, strnlen(*ptr, 4)};
	const UniChar result {decodeChar(rest)};
	*ptr = rest.data();
	if (result == 0)
	{
		while (isContinuation(**ptr))
		{
			++*ptr;
		}
	}
	return result;
}

size_t utf8StringCount(uqstl::string_view str)
{
	size_t count {};
	while (decodeChar(str) != 0)
	{
		++count;
	}
	return count;
}

size_t utf8CharCount(uqstl::string_view str, UniChar ch)
{
	size_t count {};
	while (!str.empty())
	{
		if (decodeChar(str) == ch)
		{
			++count;
		}
	}
	return count;
}

int utf8StringPos(uqstl::string_view str, UniChar ch)
{
	int pos {};
	for (; !str.empty() && str[0] != '\0'; ++pos)
	{
		if (decodeChar(str) == ch)
		{
			return pos;
		}
	}
	return ch == 0 ? pos : -1;
}

const char* skipUTF8Chars(const char* str, size_t count)
{
	while (count-- > 0 && *str != '\0')
	{
		getCharFromString(&str);
	}
	return str;
}

char* utf8StringCopy(char* dst, size_t size, uqstl::string_view src)
{
	if (size == 0)
	{
		return nullptr;
	}

	size_t length {uqstl::min(src.size(), size - 1)};
	if (length < src.size())
	{
		// Do not end in the middle of a character.
		while (length > 0 && isContinuation(src[length]))
		{
			--length;
		}
	}
	src.copy(dst, length);
	dst[length] = '\0';
	return dst;
}

int utf8StringCompare(uqstl::string_view str1, uqstl::string_view str2)
{
	// TODO: compare in collating order.
	return str1.compare(str2);
}

int getStringFromChar(uqstl::span<char> out, UniChar ch)
{
	const auto codepoint {static_cast<utf8proc_int32_t>(ch)};
	if (ch > 0x10ffff || !utf8proc_codepoint_valid(codepoint))
	{
		log::warn("Warning: Invalid or unsupported unicode char ({})", ch);
		return 0;
	}

	utf8proc_uint8_t encoded[4] {};
	const auto length {static_cast<int>(utf8proc_encode_char(codepoint, encoded))};
	if (static_cast<size_t>(length) > out.size())
	{
		return -length;
	}
	uqstl::copy_n(reinterpret_cast<const char*>(encoded), length, out.begin());
	return length;
}

bool isGraphChar(UniChar ch)
{
	if (ch > 0x10ffff)
	{
		return false;
	}
	switch (utf8proc_category(static_cast<utf8proc_int32_t>(ch)))
	{
		case UTF8PROC_CATEGORY_CN: // unassigned
		case UTF8PROC_CATEGORY_CC: // control
		case UTF8PROC_CATEGORY_CF: // format
		case UTF8PROC_CATEGORY_CS: // surrogate
		case UTF8PROC_CATEGORY_CO: // private use
		case UTF8PROC_CATEGORY_ZS: // space
		case UTF8PROC_CATEGORY_ZL: // line separator
		case UTF8PROC_CATEGORY_ZP: // paragraph separator
			return false;
		default:
			return true;
	}
}

bool isPrintChar(UniChar ch)
{
	return ch == ' ' || ch == '\t' || isGraphChar(ch);
}

UniChar toUpperChar(UniChar ch)
{
	return ch > 0x10ffff ? ch : static_cast<UniChar>(utf8proc_toupper(static_cast<utf8proc_int32_t>(ch)));
}

bool isValidUtf8(uqstl::string_view str)
{
	auto* ptr {reinterpret_cast<const utf8proc_uint8_t*>(str.data())};
	auto remaining {static_cast<utf8proc_ssize_t>(str.size())};

	while (remaining > 0)
	{
		utf8proc_int32_t codepoint {};
		const utf8proc_ssize_t length {utf8proc_iterate(ptr, remaining, &codepoint)};
		if (length < 0)
		{
			return false;
		}
		ptr += length;
		remaining -= length;
	}
	return true;
}

uqstl::string toUpperUtf8(uqstl::string_view str)
{
	uqstl::string result {};
	result.reserve(str.size());

	auto* ptr {reinterpret_cast<const utf8proc_uint8_t*>(str.data())};
	auto remaining {static_cast<utf8proc_ssize_t>(str.size())};

	while (remaining > 0)
	{
		utf8proc_int32_t codepoint {};
		const utf8proc_ssize_t length {utf8proc_iterate(ptr, remaining, &codepoint)};
		if (length < 0)
		{
			result.push_back(static_cast<char>(*ptr));
			++ptr;
			--remaining;
			continue;
		}

		utf8proc_uint8_t encoded[4] {};
		const utf8proc_ssize_t encodedLength {utf8proc_encode_char(utf8proc_toupper(codepoint), encoded)};
		result.append(reinterpret_cast<const char*>(encoded), static_cast<size_t>(encodedLength));

		ptr += length;
		remaining -= length;
	}
	return result;
}

#ifdef _WIN32
uqstl::wstring toWideChar(uqstl::string_view str)
{
	const int length {static_cast<int>(str.size())};
	const int charCount {MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), length, nullptr, 0)};
	if (charCount <= 0)
	{
		return {};
	}

	uqstl::wstring out(charCount, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), length, out.data(), charCount);
	return out;
}
#endif

} // namespace uqm
