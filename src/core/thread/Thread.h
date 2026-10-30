// Starts named threads, and other thread utilities.
#pragma once
#ifndef UQM_CORE_THREAD_THREAD_H_
#define UQM_CORE_THREAD_THREAD_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"

#include <stop_token>
#include <thread>
#include <utility>

namespace uqm
{

// Sets the name that debuggers show for the calling thread.
void setCurrentThreadName(uqgsl::czstring name);

// Sleeps for a short time, so that busy-wait loops do not use all of a CPU core.
void taskSwitch();

// Starts a thread with the name, which calls func(stopToken).
// The destructor of the result requests a stop and waits for the thread.
template <typename Func>
uqstl::jthread makeThread(uqstl::string name, Func&& func)
{
	return uqstl::jthread {[name = uqstl::move(name), func = uqstl::forward<Func>(func)](uqstl::stop_token stopToken) mutable {
		setCurrentThreadName(name.c_str());
		func(stopToken);
	}};
}

} // namespace uqm

#endif // UQM_CORE_THREAD_THREAD_H_
