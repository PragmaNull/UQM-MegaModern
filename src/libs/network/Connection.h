// A TCP connection that gives the received data to a handler and sends data.
#pragma once
#ifndef UQM_LIBS_NETWORK_CONNECTION_H_
#define UQM_LIBS_NETWORK_CONNECTION_H_

#include "core/stl/functional.h"
#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/system_error.h"

#include <cstddef>

namespace uqm::net
{

class Connection
{
public:
	// Gets the received data that is not processed yet. Returns the number of bytes that it processed, or nullopt if
	// the data is not valid.
	using ReceiveHandler = uqstl::function<uqstl::optional<size_t>(uqstl::span<const std::byte> data)>;
	// Called once when the connection closes. The error is empty if the peer closed the connection.
	using CloseHandler = uqstl::function<void(std::error_code error)>;

	struct Impl;
	explicit Connection(uqstl::shared_ptr<Impl> impl) noexcept;
	// Closes the connection. The handlers are not called after this.
	~Connection();
	Connection(Connection&& other) noexcept = default;
	Connection& operator=(Connection&& other) noexcept;
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;

	// Starts to read. The handlers are called from Context::poll(). The receive buffer holds 'bufferSize' bytes; the
	// connection closes with an error if it fills and the handler processes nothing.
	void start(size_t bufferSize, ReceiveHandler onReceive, CloseHandler onClose);
	// Sends all of 'data'. Blocks until the data is sent.
	[[nodiscard]] std::error_code send(uqstl::span<const std::byte> data);
	// Closes the connection. The handlers are not called after this.
	void close();
	[[nodiscard]] bool isOpen() const noexcept;

private:
	uqstl::shared_ptr<Impl> m_impl {};
};

} // namespace uqm::net

#endif // UQM_LIBS_NETWORK_CONNECTION_H_
