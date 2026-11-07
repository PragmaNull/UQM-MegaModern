// Reads JSON data files with yyjson.

#pragma once
#ifndef UQM_LIBS_FILE_JSON_H
#define UQM_LIBS_FILE_JSON_H

#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <yyjson.h>

#include "core/stl/limits.h"
#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

namespace uqm
{

using JsonDoc = uqstl::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)>;

// Returns an empty JsonDoc, and logs the error with 'what', if 'text' is not valid JSON.
[[nodiscard]] JsonDoc parseJson(uqstl::string_view text, uqstl::string_view what, yyjson_read_flag flags = YYJSON_READ_NOFLAG);

// Returns the root object of 'doc' if its "version" is 'version'. Otherwise logs with 'what' and returns nullptr.
[[nodiscard]] yyjson_val* versionedRoot(const JsonDoc& doc, int64_t version, uqstl::string_view what);

// Returns the value of 'key' in the root object of 'doc' if the "version" of the root is 'version' and the value is of
// 'type'. Otherwise logs with 'what' and returns nullptr.
[[nodiscard]] yyjson_val* versionedMember(const JsonDoc& doc, int64_t version, const char* key, yyjson_type type,
										  uqstl::string_view what);

[[nodiscard]] inline uqstl::string jsonString(yyjson_val* value)
{
	return {yyjson_get_str(value), yyjson_get_len(value)};
}

// Returns nullopt if 'value' is not an integer from 'min' to 'max'.
template <typename T>
[[nodiscard]] uqstl::optional<T> jsonInt(yyjson_val* value, T min = uqstl::numeric_limits<T>::min(),
										 T max = uqstl::numeric_limits<T>::max())
{
	if (yyjson_is_uint(value))
	{
		const uint64_t number {yyjson_get_uint(value)};
		if (max >= 0 && number <= static_cast<uint64_t>(max) && (min <= 0 || number >= static_cast<uint64_t>(min)))
		{
			return static_cast<T>(number);
		}
	}
	else if (yyjson_is_sint(value))
	{
		const int64_t number {yyjson_get_sint(value)};
		if (number >= static_cast<int64_t>(min) && number <= static_cast<int64_t>(max))
		{
			return static_cast<T>(number);
		}
	}
	return uqstl::nullopt;
}

// Returns nullopt if 'value' is not an array of at most 'maxCount' strings.
[[nodiscard]] uqstl::optional<uqstl::vector<uqstl::string>> jsonStringArray(yyjson_val* value, size_t maxCount = SIZE_MAX);

// Returns the value of 'E' whose name, with a lowercase first letter, is 'name'. For example "waitDitty" is WaitDitty.
template <typename E>
[[nodiscard]] uqstl::optional<E> enumFromJsonName(uqstl::string_view name)
{
	for (const auto& [value, enumName] : magic_enum::enum_entries<E>())
	{
		if (!name.empty() && name.size() == enumName.size() && name.substr(1) == enumName.substr(1)
			&& name.front() == static_cast<char>(enumName.front() | 0x20))
		{
			return value;
		}
	}
	return uqstl::nullopt;
}

} // namespace uqm

#endif // UQM_LIBS_FILE_JSON_H
