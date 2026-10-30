#pragma once
#if !defined CORE_PLATFORM_PLATFORM_H_
#define CORE_PLATFORM_PLATFORM_H_

#include "core/stl/stl.h"

#include <ctime>

namespace uqm
{
uqstl::string getEnvironmentValue(uqgsl::czstring env_name);
bool setEnvironmentValue(uqgsl::czstring name, uqgsl::czstring value, bool overwrite);
uqstl::string strerror(int err);
uqstl::tm localtime(time_t time);
// Sets the name that debuggers show for the calling thread.
void setCurrentThreadName(uqgsl::czstring name);
} // namespace uqm

#endif /* CORE_PLATFORM_PLATFORM_H_ */