// Connects to a TCP host: resolves the host, then tries its addresses one after the other.
#include "Connector.h"

#include "NetImpl.h"

namespace uqm::net
{

struct Connector::Impl : uqstl::enable_shared_from_this<Connector::Impl>
{
	Impl(asio::io_context& io, ConnectOptions options, ConnectHandler onConnect, ErrorHandler onError)
		: m_options {uqstl::move(options)}
		, m_onConnect {uqstl::move(onConnect)}
		, m_onError {uqstl::move(onError)}
		, m_resolver {io}
		, m_socket {io}
		, m_timer {io}
	{
	}

	void start()
	{
		m_resolver.async_resolve(m_options.host, m_options.service,
								 [self = shared_from_this()](const std::error_code& error,
															 const asio::ip::tcp::resolver::results_type& results) {
									 self->onResolve(error, results);
								 });
	}

	void cancel()
	{
		m_open = false;
		m_resolver.cancel();
		m_timer.cancel();
		std::error_code ignored {};
		m_socket.close(ignored);
	}

private:
	void onResolve(std::error_code error, const asio::ip::tcp::resolver::results_type& results)
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
		for (const asio::ip::tcp::endpoint& endpoint : results)
		{
			if (!m_options.ipv4Only || endpoint.address().is_v4())
			{
				m_endpoints.push_back(endpoint);
			}
		}
		if (m_endpoints.empty())
		{
			fail(uqstl::make_error_code(std::errc::address_family_not_supported));
			return;
		}
		tryNext();
	}

	// Tries the next address, or waits for the next try after the last address.
	void tryNext()
	{
		++m_attempt;
		std::error_code ignored {};
		m_socket.close(ignored);
		if (m_next == m_endpoints.size())
		{
			if (!m_options.retryDelay)
			{
				fail(uqstl::make_error_code(std::errc::timed_out));
				return;
			}
			m_next = 0;
			m_timer.expires_after(*m_options.retryDelay);
			m_timer.async_wait([self = shared_from_this(), attempt = m_attempt](const std::error_code& error) {
				if (self->m_open && !error && attempt == self->m_attempt)
				{
					self->tryNext();
				}
			});
			return;
		}
		m_socket.async_connect(m_endpoints[m_next++],
							   [self = shared_from_this(), attempt = m_attempt](const std::error_code& error) {
								   self->onConnect(error, attempt);
							   });
		m_timer.expires_after(m_options.timeout);
		m_timer.async_wait([self = shared_from_this(), attempt = m_attempt](const std::error_code& error) {
			if (self->m_open && !error && attempt == self->m_attempt)
			{
				std::error_code ignored {};
				self->m_socket.close(ignored);
			}
		});
	}

	void onConnect(std::error_code error, uint32_t attempt)
	{
		if (!m_open || attempt != m_attempt)
		{
			return;
		}
		if (error)
		{
			tryNext();
			return;
		}
		m_open = false;
		m_timer.cancel();
		m_onConnect(Connection {uqstl::make_shared<Connection::Impl>(uqstl::move(m_socket))});
	}

	void fail(std::error_code error)
	{
		m_open = false;
		m_onError(error);
	}

	ConnectOptions m_options;
	ConnectHandler m_onConnect;
	ErrorHandler m_onError;
	asio::ip::tcp::resolver m_resolver;
	asio::ip::tcp::socket m_socket;
	asio::steady_timer m_timer;
	uqstl::vector<asio::ip::tcp::endpoint> m_endpoints {};
	size_t m_next {};
	// Makes the handlers of an earlier attempt do nothing.
	uint32_t m_attempt {};
	bool m_open {true};
};

Connector::Connector(Context& context, ConnectOptions options, ConnectHandler onConnect, ErrorHandler onError)
	: m_impl {uqstl::make_shared<Impl>(context.impl().io, uqstl::move(options), uqstl::move(onConnect),
									   uqstl::move(onError))}
{
	m_impl->start();
}

Connector::~Connector()
{
	m_impl->cancel();
}

} // namespace uqm::net
