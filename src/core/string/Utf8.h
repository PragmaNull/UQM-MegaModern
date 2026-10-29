// UTF-8 validation, case conversion and Windows wide-string conversion.
#pragma once
#ifndef UQM_CORE_STRING_UTF8_H_
#define UQM_CORE_STRING_UTF8_H_

#include "core/stl/string.h"

namespace uqm
{

bool isValidUtf8(uqstl::string_view str);

// Invalid bytes are copied unchanged.
uqstl::string toUpperUtf8(uqstl::string_view str);

#ifdef _WIN32
// Returns an empty string if 'str' is not valid UTF-8.
uqstl::wstring toWideChar(uqstl::string_view str);
#endif

} // namespace uqm

#endif // UQM_CORE_STRING_UTF8_H_
