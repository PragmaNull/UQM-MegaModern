// Accepts TCP connections on a port.
#include "Listener.h"

#include "NetImpl.h"

namespace uqm::net
{

struct Listener::Impl : uqstl::enable_shared_from_this<Listener::Impl>
{
	Impl(asio::io_context& io, AcceptHandler onAccept, ErrorHandler onError)
		: m_onAccept {uqstl::move(onAccept)}
		, m_onError {uqstl::move(onError)}
		, m_acceptor {io}
	{
	}

	void start(const ListenOptions& options)
	{
		const asio::ip::tcp protocol {options.ipv4Only ? asio::ip::tcp::v4() : asio::ip::tcp::v6()};
		std::error_code error {};
		m_acceptor.open(protocol, error);
#ifndef _WIN32
		// On Windows, SO_REUSEADDR lets other sockets bind to the same port.
		if (!error)
		{
			m_acceptor.set_option(asio::socket_base::reuse_address {true}, error);
		}
#endif
		if (!error && !options.ipv4Only)
		{
			m_acceptor.set_option(asio::ip::v6_only {false}, error);
		}
		if (!error)
		{
			m_acceptor.bind(asio::ip::tcp::endpoint {protocol, options.port}, error);
		}
		if (!error)
		{
			m_acceptor.listen(options.backlog, error);
		}
		if (error)
		{
			// The error handler runs from Context::poll(), as the other handlers do.
			asio::post(m_acceptor.get_executor(), [self = shared_from_this(), error] {
				self->fail(error);
			});
			return;
		}
		accept();
	}

	void cancel()
	{
		m_open = false;
		std::error_code ignored {};
		m_acceptor.close(ignored);
	}

	[[nodiscard]] uint16_t port() const
	{
		std::error_code error {};
		const asio::ip::tcp::endpoint endpoint {m_acceptor.local_endpoint(error)};
		return error ? 0 : endpoint.port();
	}

private:
	void accept()
	{
		m_acceptor.async_accept(
			[self = shared_from_this()](const std::error_code& error, asio::ip::tcp::socket socket) {
				self->onAccept(error, uqstl::move(socket));
			});
	}

	void onAccept(std::error_code error, asio::ip::tcp::socket socket)
	{
		if (!m_open)
		{
			return;
		}
		if (error)
		{
			fail(error);
			return;
		}
		m_onAccept(Connection {uqstl::make_shared<Connection::Impl>(uqstl::move(socket))});
		if (m_open)
		{
			accept();
		}
	}

	void fail(std::error_code error)
	{
		if (!m_open)
		{
			return;
		}
		cancel();
		m_onError(error);
	}

	AcceptHandler m_onAccept;
	ErrorHandler m_onError;
	asio::ip::tcp::acceptor m_acceptor;
	bool m_open {true};
};

Listener::Listener(Context& context, ListenOptions options, AcceptHandler onAccept, ErrorHandler onError)
	: m_impl {uqstl::make_shared<Impl>(context.impl().io, uqstl::move(onAccept), uqstl::move(onError))}
{
	m_impl->start(options);
}

Listener::~Listener()
{
	m_impl->cancel();
}

uint16_t Listener::port() const
{
	return m_impl->port();
}

} // namespace uqm::net
