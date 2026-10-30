#include "platform.h"

#include <stdlib.h>
#include <system_error>
#include "core/string/StringUtils.h"

#ifdef _WIN32
#include "core/string/Utf8.h"

#include <windows.h>
#else
#include <pthread.h>
#endif

namespace uqm
{

uqstl::string getEnvironmentValue(uqgsl::czstring env_name)
{
#ifdef _MSC_VER
	// Windows version
	char* buffer {};
	size_t bufferSize {};
	if (_dupenv_s(&buffer, &bufferSize, env_name) == 0 && buffer != nullptr)
	{
		uqstl::string ret {buffer};

		free(buffer);
		return ret;
	}
#else
	// This also works on Windows, but gives a warning
	if (char* buffer = std::getenv(env_name.c_str()); buffer != nullptr)
	{
		return {buffer};
	}
#endif
	return {};
}

bool setEnvironmentValue(uqgsl::czstring name, uqgsl::czstring value, bool overwrite)
{
#ifndef HAVE_SETENV
	if (!overwrite)
	{
		if (const uqstl::string old {getEnvironmentValue(name)}; old.empty())
		{
			return false;
		}
	}

	const size_t nameLen {strlen(name)};
	const size_t valueLen {strlen(value)};

	const size_t bufLen {nameLen + valueLen + 2}; // +1 for '=' and +1 for null terminator
	char* string {new char[bufLen] {}};

	// "NAME=VALUE\0"
	fmt::format_to_sz_n(string, bufLen, "{}={}", name, value);

	// putenv() does NOT make a copy, but takes ownership of the string passed.
	return _putenv(string) == 0;

#else
	return setenv(name, value, static_cast<int>(overwrite)) == 0;

#endif
}


uqstl::string strerror(int err)
{
	return uqstl::generic_category().message(err);
}

uqstl::tm localtime(time_t time)
{
	uqstl::tm result {};
#ifdef _WIN32
	localtime_s(&result, &time);
#else
	localtime_r(&time, &result);
#endif
	return result;
}

void setCurrentThreadName(uqgsl::czstring name)
{
#ifdef _WIN32
	(void)SetThreadDescription(GetCurrentThread(), toWideChar(name).c_str());
#elif defined(__APPLE__)
	(void)pthread_setname_np(name);
#else
	// Linux limits the name to 15 bytes.
	const uqstl::string shortName {uqstl::string_view {name}.substr(0, 15)};
	(void)pthread_setname_np(pthread_self(), shortName.c_str());
#endif
}


} // namespace uqm