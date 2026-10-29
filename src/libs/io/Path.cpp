// Helpers for paths in the virtual file system.
#include "libs/io/Path.h"

#include "core/stl/vector.h"

namespace uqm::io::path
{

uqstl::string normalize(uqstl::string_view path)
{
	uqstl::vector<uqstl::string_view> parts {};
	size_t start {0};
	while (start <= path.size())
	{
		const size_t end {uqstl::min(path.find('/', start), path.size())};
		const uqstl::string_view part {path.substr(start, end - start)};
		if (part == "..")
		{
			if (!parts.empty())
			{
				parts.pop_back();
			}
		}
		else if (!part.empty() && part != ".")
		{
			parts.push_back(part);
		}
		start = end + 1;
	}

	uqstl::string result {};
	for (const uqstl::string_view part : parts)
	{
		if (!result.empty())
		{
			result += '/';
		}
		result += part;
	}
	return result;
}

uqstl::string join(uqstl::string_view base, uqstl::string_view relative)
{
	if (relative.starts_with('/'))
	{
		return normalize(relative);
	}
	uqstl::string combined {base};
	combined += '/';
	combined += relative;
	return normalize(combined);
}

bool isWithin(uqstl::string_view path, uqstl::string_view prefix)
{
	if (prefix.empty())
	{
		return true;
	}
	return path.starts_with(prefix) && (path.size() == prefix.size() || path[prefix.size()] == '/');
}

uqstl::string_view relativeTo(uqstl::string_view path, uqstl::string_view prefix)
{
	if (prefix.empty())
	{
		return path;
	}
	if (path.size() == prefix.size())
	{
		return {};
	}
	return path.substr(prefix.size() + 1);
}

uqstl::string_view parent(uqstl::string_view path)
{
	const size_t slash {path.rfind('/')};
	return slash == uqstl::string_view::npos ? uqstl::string_view {} : path.substr(0, slash);
}

uqstl::string_view fileName(uqstl::string_view path)
{
	const size_t slash {path.rfind('/')};
	return slash == uqstl::string_view::npos ? path : path.substr(slash + 1);
}

} // namespace uqm::io::path
