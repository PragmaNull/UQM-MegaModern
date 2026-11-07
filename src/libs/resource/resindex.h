// Reads resource index files: the JSON files that give the type and the data of each resource.

#pragma once
#ifndef UQM_LIBS_RESOURCE_RESINDEX_H
#define UQM_LIBS_RESOURCE_RESINDEX_H

#include "core/stl/stl.h"
#include "core/stl/map.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/variant.h"
#include "core/stl/vector.h"

#include <cstdint>

// A value in the data of a resource.
using ResourceScalar = uqstl::variant<uqstl::string, int64_t, double, bool>;
// The named parts of the data of a resource, such as the files of a conversation.
using ResourceFields = uqstl::map<uqstl::string, ResourceScalar, uqstl::less<>>;
// The data of a resource: a path or text, a number, a truth value, or named parts.
using ResourceArg = uqstl::variant<uqstl::string, int64_t, double, bool, ResourceFields>;

// Returns nullptr if 'arg' is not a string.
[[nodiscard]] const char* resourceString(const ResourceArg& arg);
// Returns nullptr if 'arg' has no string field 'name'.
[[nodiscard]] const char* resourceStringField(const ResourceArg& arg, uqstl::string_view name);
// Returns nullopt if 'arg' has no integer field 'name'.
[[nodiscard]] uqstl::optional<int64_t> resourceIntField(const ResourceArg& arg, uqstl::string_view name);

namespace uqm
{

struct ResourceEntry
{
	uqstl::string key {};
	uqstl::string type {};
	ResourceArg arg {};
};

inline constexpr int c_resourceIndexVersion {1};

// Reads the entries of a resource index in file order, and puts 'prefix' before each key.
// Returns nullopt if 'json' is not a valid resource index. Skips entries that are not valid.
[[nodiscard]] uqstl::optional<uqstl::vector<ResourceEntry>> resourceIndexFromJson(uqstl::string_view json, uqstl::string_view prefix);

// Reads a resource index in the old format: a "key = TYPE:data" line for each resource, and '#' comments. The data of
// INT32, SHIP, FLT32 and bool resources becomes a number or a truth value, and all other data stays a string.
[[nodiscard]] uqstl::vector<ResourceEntry> resourceIndexFromLegacyText(uqstl::string_view text);

[[nodiscard]] uqstl::string resourceIndexToJson(uqstl::span<const ResourceEntry> entries);

} // namespace uqm

#endif // UQM_LIBS_RESOURCE_RESINDEX_H
