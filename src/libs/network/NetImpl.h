// The internal state of the network classes. Only the network sources include this header, because it includes Asio.

#pragma once
#ifndef UQM_LIBS_NETWORK_NETIMPL_H
#define UQM_LIBS_NETWORK_NETIMPL_H

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#include <asio.hpp>

#include "Connection.h"
#include "NetContext.h"

#include "core/stl/vector.h"

namespace uqm::net
{

struct Context::Impl
{
	asio::io_context io {};
};

// The handlers of the asynchronous operations keep the state alive until they run.
struct Connection::Impl : uqstl::enable_shared_from_this<Connection::Impl>
{
	explicit Impl(asio::ip::tcp::socket socket);

	void start(size_t bufferSize, ReceiveHandler onReceive, CloseHandler onClose);
	[[nodiscard]] std::error_code send(uqstl::span<const std::byte> data);
	void close();

	bool m_open {true};

private:
	void read();
	void onRead(std::error_code error, size_t count);
	// Closes the connection and calls the close handler.
	void fail(std::error_code error);

	asio::ip::tcp::socket m_socket;
	uqstl::vector<std::byte> m_buffer {};
	size_t m_used {};
	ReceiveHandler m_onReceive {};
	CloseHandler m_onClose {};
};

} // namespace uqm::net

#endif // UQM_LIBS_NETWORK_NETIMPL_H
