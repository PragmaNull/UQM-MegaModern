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

	const auto lead {static_cast<unsigned char>(str[0])};
	if (lead < 0x80)
	{
		str.remove_prefix(1);
		return lead;
	}

	size_t length {};
	UniChar result {};
	UniChar minimum {};
	if ((lead & 0xe0) == 0xc0)
	{
		length = 2;
		result = lead & 0x1f;
		minimum = 0x80;
	}
	else if ((lead & 0xf0) == 0xe0)
	{
		length = 3;
		result = lead & 0x0f;
		minimum = 0x800;
	}
	else if ((lead & 0xf8) == 0xf0)
	{
		length = 4;
		result = lead & 0x07;
		minimum = 0x10000;
	}

	size_t used {1};
	bool valid {length != 0};
	for (; valid && used < length; ++used)
	{
		if (used >= str.size() || !isContinuation(str[used]))
		{
			valid = false;
			break;
		}
		result = (result << 6) | (static_cast<unsigned char>(str[used]) & 0x3f);
	}
	if (valid && result >= minimum)
	{
		str.remove_prefix(length);
		return result;
	}

	log::warn("Warning: Invalid UTF8 sequence starting with byte {:#04x}", lead);
	while (used < str.size() && isContinuation(str[used]))
	{
		++used;
	}
	str.remove_prefix(uqstl::min(used, str.size()));
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
	int length {};
	if (ch < 0x80)
	{
		length = 1;
	}
	else if (ch < 0x800)
	{
		length = 2;
	}
	else if (ch < 0x10000)
	{
		length = 3;
	}
	else if (ch < 0x110000)
	{
		length = 4;
	}
	else
	{
		log::warn("Warning: Invalid or unsupported unicode char ({})", ch);
		return 0;
	}

	if (static_cast<size_t>(length) > out.size())
	{
		return -length;
	}

	static constexpr unsigned char c_leadMarkers[] {0x00, 0x00, 0xc0, 0xe0, 0xf0};
	for (int i {length - 1}; i > 0; --i)
	{
		out[i] = static_cast<char>((ch & 0x3f) | 0x80);
		ch >>= 6;
	}
	out[0] = static_cast<char>(ch | c_leadMarkers[length]);
	return length;
}

bool isGraphChar(UniChar ch)
{
	// All characters except the C0 and C1 controls, and the private use area 0xE000-0xF8FF.
	return (ch > 0xa0 && (ch < 0xe000 || ch > 0xf8ff)) || (ch > 0x20 && ch < 0x7f);
}

bool isPrintChar(UniChar ch)
{
	return ch == 0x20 || ch == 0x09 || isGraphChar(ch);
}

UniChar toUpperChar(UniChar ch)
{
	// Latin-1 only.
	return ch < 0x100 ? static_cast<UniChar>(toupper(static_cast<int>(ch))) : ch;
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
