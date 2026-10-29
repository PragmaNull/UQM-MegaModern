// Converts key binding templates to and from the keys.json format.
#pragma once
#ifndef UQM_LIBS_INPUT_KEYCONFIG_H_
#define UQM_LIBS_INPUT_KEYCONFIG_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

#include <optional>

namespace uqm
{

struct KeyTemplate
{
	uqstl::string name {};
	// One list of gesture strings, e.g. "key Up", for each control.
	uqstl::vector<uqstl::vector<uqstl::string>> bindings {};
};

// 'controlNames' gives the JSON name of each control, in binding order.
[[nodiscard]] uqstl::string keyConfigToJson(uqstl::span<const KeyTemplate> templates, uqstl::span<const uqgsl::czstring> controlNames);

// Returns nullopt if 'json' is not a valid key configuration.
[[nodiscard]] uqstl::optional<uqstl::vector<KeyTemplate>> keyConfigFromJson(uqstl::string_view json, uqstl::span<const uqgsl::czstring> controlNames);

} // namespace uqm

#endif // UQM_LIBS_INPUT_KEYCONFIG_H_
