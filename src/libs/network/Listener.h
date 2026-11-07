// Accepts TCP connections on a port.

#pragma once
#ifndef UQM_LIBS_NETWORK_LISTENER_H
#define UQM_LIBS_NETWORK_LISTENER_H

#include "Connection.h"
#include "NetContext.h"

#include "core/stl/functional.h"
#include "core/stl/memory.h"
#include "core/stl/system_error.h"

#include <cstdint>

namespace uqm::net
{

struct ListenOptions
{
	// If 0, the system selects a port.
	uint16_t port {};
	bool ipv4Only {};
	int backlog {2};
};

class Listener
{
public:
	using AcceptHandler = uqstl::function<void(Connection connection)>;
	using ErrorHandler = uqstl::function<void(std::error_code error)>;

	// Starts to listen. The handlers are called from Context::poll(). After an error, the listener stops.
	Listener(Context& context, ListenOptions options, AcceptHandler onAccept, ErrorHandler onError);
	// Stops listening. The handlers are not called after this.
	~Listener();
	Listener(const Listener&) = delete;
	Listener& operator=(const Listener&) = delete;

	// The port that the listener uses, or 0 if it could not listen.
	[[nodiscard]] uint16_t port() const;

	struct Impl;

private:
	uqstl::shared_ptr<Impl> m_impl {};
};

} // namespace uqm::net

#endif // UQM_LIBS_NETWORK_LISTENER_H
