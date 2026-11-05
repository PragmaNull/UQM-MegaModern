#pragma once
#if !defined CORE_PLATFORM_PLATFORM_H_
#define CORE_PLATFORM_PLATFORM_H_

#include "core/stl/stl.h"
#include "core/stl/memory.h"

#include <cstdio>
#include <ctime>

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

#endif /* CORE_PLATFORM_PLATFORM_H_ */