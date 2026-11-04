// A TCP connection that gives the received data to a handler and sends data.
#include "NetImpl.h"

namespace uqm::net
{

Connection::Impl::Impl(asio::ip::tcp::socket socket)
	: m_socket {uqstl::move(socket)}
{
	std::error_code ignored {};
	m_socket.set_option(asio::ip::tcp::no_delay {true}, ignored);
}

void Connection::Impl::start(size_t bufferSize, ReceiveHandler onReceive, CloseHandler onClose)
{
	m_buffer.assign(bufferSize, std::byte {});
	m_used = 0;
	m_onReceive = uqstl::move(onReceive);
	m_onClose = uqstl::move(onClose);
	read();
}

std::error_code Connection::Impl::send(uqstl::span<const std::byte> data)
{
	if (!m_open)
	{
		return uqstl::make_error_code(std::errc::not_connected);
	}
	std::error_code error {};
	asio::write(m_socket, asio::buffer(data.data(), data.size()), error);
	return error;
}

void Connection::Impl::close()
{
	m_open = false;
	std::error_code ignored {};
	m_socket.close(ignored);
}

void Connection::Impl::read()
{
	if (m_used == m_buffer.size())
	{
		fail(uqstl::make_error_code(std::errc::no_buffer_space));
		return;
	}
	m_socket.async_read_some(asio::buffer(m_buffer.data() + m_used, m_buffer.size() - m_used),
							 [self = shared_from_this()](const std::error_code& error, size_t count) {
								 self->onRead(error, count);
							 });
}

void Connection::Impl::onRead(std::error_code error, size_t count)
{
	if (!m_open)
	{
		return;
	}
	if (error)
	{
		fail(error == asio::error::eof ? std::error_code {} : error);
		return;
	}
	m_used += count;
	const uqstl::optional<size_t> processed {m_onReceive(uqstl::span<const std::byte> {m_buffer.data(), m_used})};
	if (!m_open)
	{
		return;
	}
	if (!processed || *processed > m_used)
	{
		fail(uqstl::make_error_code(std::errc::bad_message));
		return;
	}
	const auto first {m_buffer.begin() + static_cast<ptrdiff_t>(*processed)};
	uqstl::copy(first, m_buffer.begin() + static_cast<ptrdiff_t>(m_used), m_buffer.begin());
	m_used -= *processed;
	read();
}

void Connection::Impl::fail(std::error_code error)
{
	close();
	if (m_onClose)
	{
		m_onClose(error);
	}
}

Connection::Connection(uqstl::shared_ptr<Impl> impl) noexcept
	: m_impl {uqstl::move(impl)}
{
}

Connection::~Connection()
{
	close();
}

Connection& Connection::operator=(Connection&& other) noexcept
{
	if (this != &other)
	{
		close();
		m_impl = uqstl::move(other.m_impl);
	}
	return *this;
}

void Connection::start(size_t bufferSize, ReceiveHandler onReceive, CloseHandler onClose)
{
	m_impl->start(bufferSize, uqstl::move(onReceive), uqstl::move(onClose));
}

std::error_code Connection::send(uqstl::span<const std::byte> data)
{
	return m_impl ? m_impl->send(data) : uqstl::make_error_code(std::errc::not_connected);
}

void Connection::close()
{
	if (m_impl)
	{
		m_impl->close();
	}
}

bool Connection::isOpen() const noexcept
{
	return m_impl && m_impl->m_open;
}

} // namespace uqm::net
