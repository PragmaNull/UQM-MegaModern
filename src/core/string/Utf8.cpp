#include "Utf8.h"

#include <utf8proc.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace uqm
{

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
