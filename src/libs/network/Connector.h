// Connects to a TCP host: resolves the host, then tries its addresses one after the other.

#pragma once
#ifndef UQM_LIBS_NETWORK_CONNECTOR_H
#define UQM_LIBS_NETWORK_CONNECTOR_H

#include "Connection.h"
#include "NetContext.h"

#include "core/stl/chrono.h"
#include "core/stl/functional.h"
#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/system_error.h"

namespace uqm::net
{

struct ConnectOptions
{
	uqstl::string host {};
	uqstl::string service {};
	bool ipv4Only {};
	// The time to wait for each address.
	uqstl::chrono::milliseconds timeout {2000};
	// The time to wait after all the addresses fail before the next try. If empty, the connector reports the error.
	uqstl::optional<uqstl::chrono::milliseconds> retryDelay {};
};

class Connector
{
public:
	using ConnectHandler = uqstl::function<void(Connection connection)>;
	using ErrorHandler = uqstl::function<void(std::error_code error)>;

	// Starts to connect. One of the handlers is called once, from Context::poll().
	Connector(Context& context, ConnectOptions options, ConnectHandler onConnect, ErrorHandler onError);
	// Stops the connection attempt. The handlers are not called after this.
	~Connector();
	Connector(const Connector&) = delete;
	Connector& operator=(const Connector&) = delete;

	struct Impl;

private:
	uqstl::shared_ptr<Impl> m_impl {};
};

} // namespace uqm::net

#endif // UQM_LIBS_NETWORK_CONNECTOR_H
