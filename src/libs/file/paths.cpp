// Expands and creates the host directories that the game uses.
#include "paths.h"

#include "core/log/log.h"
#include "core/platform/platform.h"

#include <cctype>
#include <filesystem>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

namespace fs = uqstl::filesystem;

namespace uqm
{

namespace
{

uqstl::string homeDir()
{
	uqstl::string home {getEnvironmentValue("HOME")};
	if (!home.empty())
	{
		return home;
	}
#ifdef _WIN32
	return getEnvironmentValue("USERPROFILE");
#else
	const passwd* entry {getpwuid(getuid())};
	return entry ? uqstl::string {entry->pw_dir} : uqstl::string {};
#endif
}

// Replaces the environment variables in the path with their values. An unset variable becomes empty.
uqstl::optional<uqstl::string> expandEnvironment(uqstl::string_view path)
{
	uqstl::string result {};
	size_t pos {};
	while (pos < path.size())
	{
#ifdef _WIN32
		if (path[pos] == '%')
		{
			const size_t end {path.find('%', pos + 1)};
			if (end == uqstl::string_view::npos)
			{
				return uqstl::nullopt;
			}
			result += getEnvironmentValue(uqstl::string {path.substr(pos + 1, end - pos - 1)}.c_str());
			pos = end + 1;
			continue;
		}
#else
		if (path[pos] == '$')
		{
			size_t nameStart {pos + 1};
			size_t nameEnd {};
			size_t next {};
			if (nameStart < path.size() && path[nameStart] == '{')
			{
				++nameStart;
				nameEnd = path.find('}', nameStart);
				if (nameEnd == uqstl::string_view::npos)
				{
					return uqstl::nullopt;
				}
				next = nameEnd + 1;
			}
			else
			{
				nameEnd = nameStart;
				while (nameEnd < path.size() && (isalnum(static_cast<unsigned char>(path[nameEnd])) || path[nameEnd] == '_'))
				{
					++nameEnd;
				}
				next = nameEnd;
			}
			result += getEnvironmentValue(uqstl::string {path.substr(nameStart, nameEnd - nameStart)}.c_str());
			pos = next;
			continue;
		}
#endif
		result += path[pos];
		++pos;
	}
	return result;
}

bool isSeparator(char c)
{
#ifdef _WIN32
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

} // namespace

uqstl::optional<uqstl::string> expandPath(uqstl::string_view path)
{
	uqstl::optional<uqstl::string> expanded {expandEnvironment(path)};
	if (!expanded)
	{
		return uqstl::nullopt;
	}

	if (!expanded->empty() && (*expanded)[0] == '~')
	{
		// Only "~/" is supported, not "~user/".
		if (expanded->size() > 1 && !isSeparator((*expanded)[1]))
		{
			return uqstl::nullopt;
		}
		const uqstl::string home {homeDir()};
		if (home.empty())
		{
			return uqstl::nullopt;
		}
		expanded->replace(0, 1, home);
	}

	uqstl::error_code error {};
	const fs::path absolute {fs::absolute(fs::path {*expanded}, error)};
	if (error)
	{
		return uqstl::nullopt;
	}
	return absolute.lexically_normal().generic_string();
}

bool createDirectories(uqgsl::czstring path)
{
	uqstl::error_code error {};
	fs::create_directories(fs::path {path}, error);
	if (error)
	{
		log::error("Could not create directory '{}': {}", path, error.message());
		return false;
	}
	return true;
}

bool fileExists(uqgsl::czstring path)
{
	uqstl::error_code error {};
	return fs::exists(fs::path {path}, error);
}

} // namespace uqm
