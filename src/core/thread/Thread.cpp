// Starts named threads, and other thread utilities.
#include "Thread.h"

#include <chrono>

#ifdef _WIN32
#include "core/string/Utf8.h"

#include <windows.h>
#else
#include <pthread.h>
#endif

namespace uqm
{

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

void taskSwitch()
{
	uqstl::this_thread::sleep_for(uqstl::chrono::milliseconds {1});
}

} // namespace uqm
