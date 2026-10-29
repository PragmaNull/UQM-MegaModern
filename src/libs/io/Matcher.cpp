// File name filter for directory listings.
#include "libs/io/Matcher.h"

namespace uqm::io
{

Matcher::Matcher(Kind kind, uqstl::string pattern, uqstl::shared_ptr<const uqstl::regex> regex)
	: m_kind {kind}
	, m_pattern {uqstl::move(pattern)}
	, m_regex {uqstl::move(regex)}
{
}

uqstl::optional<Matcher> Matcher::create(Kind kind, uqstl::string_view pattern)
{
	if (kind != Kind::Regex)
	{
		return Matcher {kind, uqstl::string {pattern}, nullptr};
	}
	try
	{
		auto regex {uqstl::make_shared<const uqstl::regex>(pattern.begin(), pattern.end())};
		return Matcher {kind, uqstl::string {pattern}, uqstl::move(regex)};
	}
	catch (const uqstl::regex_error&)
	{
		return uqstl::nullopt;
	}
}

bool Matcher::matches(uqstl::string_view name) const
{
	switch (m_kind)
	{
		case Kind::Literal:
			return name == m_pattern;
		case Kind::Prefix:
			return name.starts_with(m_pattern);
		case Kind::Suffix:
			return name.ends_with(m_pattern);
		case Kind::Substring:
			return name.find(m_pattern) != uqstl::string_view::npos;
		case Kind::Regex:
			return uqstl::regex_search(name.begin(), name.end(), *m_regex);
	}
	return false;
}

} // namespace uqm::io
