#pragma once
#ifndef UQM_CORE_PLATFORM_PLATFORM_H
#define UQM_CORE_PLATFORM_PLATFORM_H

#include <cstdio>
#include <ctime>

#include "core/stl/memory.h"
#include "core/stl/stl.h"

namespace uqm
{
struct FileCloser
{
	void operator()(std::FILE* file) const noexcept;
};

using FilePtr = uqstl::unique_ptr<std::FILE, FileCloser>;

// Opens a file with std::fopen modes. Returns nullptr and sets errno on failure.
FilePtr openFile(uqgsl::czstring path, uqgsl::czstring mode);

uqstl::string getEnvironmentValue(uqgsl::czstring env_name);
bool setEnvironmentValue(uqgsl::czstring name, uqgsl::czstring value, bool overwrite);
uqstl::string strerror(int err);
uqstl::tm localtime(time_t time);
} // namespace uqm


#endif // UQM_CORE_PLATFORM_PLATFORM_H
