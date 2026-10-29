// File name filter for directory listings.
#pragma once
#ifndef UQM_LIBS_IO_MATCHER_H_
#define UQM_LIBS_IO_MATCHER_H_

#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"

#include <cstdint>
#include <regex>

namespace uqm::io
{

class Matcher
{
public:
	enum class Kind : uint8_t
	{
		Literal,
		Prefix,
		Suffix,
		Substring,
		// ECMAScript regular expression, found anywhere in the name.
		Regex,
	};

	// Matches all names.
	Matcher() = default;

	// Returns nullopt if 'pattern' is not a valid regular expression.
	static uqstl::optional<Matcher> create(Kind kind, uqstl::string_view pattern);

	[[nodiscard]] bool matches(uqstl::string_view name) const;

private:
	Matcher(Kind kind, uqstl::string pattern, uqstl::shared_ptr<const uqstl::regex> regex);

	Kind m_kind {Kind::Prefix};
	uqstl::string m_pattern {};
	uqstl::shared_ptr<const uqstl::regex> m_regex {};
};

} // namespace uqm::io

#endif // UQM_LIBS_IO_MATCHER_H_
