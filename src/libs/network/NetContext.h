// The context that runs the network operations on the thread that polls it.

#pragma once
#ifndef UQM_LIBS_NETWORK_NETCONTEXT_H
#define UQM_LIBS_NETWORK_NETCONTEXT_H

#include "core/stl/chrono.h"
#include "core/stl/memory.h"

namespace uqm::net
{

// The network objects of a context must be destroyed before the context.
class Context
{
public:
	Context();
	~Context();
	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	// Waits up to 'timeout' for a network event, then runs the handlers of all the events that are ready. Returns at
	// once if there are no network operations.
	void poll(uqstl::chrono::milliseconds timeout);

	struct Impl;
	[[nodiscard]] Impl& impl() noexcept
	{
		return *m_impl;
	}

private:
	uqstl::unique_ptr<Impl> m_impl {};
};

} // namespace uqm::net

#endif // UQM_LIBS_NETWORK_NETCONTEXT_H
