// The context that runs the network operations on the thread that polls it.
#include "NetImpl.h"

namespace uqm::net
{

Context::Context()
	: m_impl {uqstl::make_unique<Impl>()}
{
}

Context::~Context() = default;

void Context::poll(uqstl::chrono::milliseconds timeout)
{
	asio::io_context& io {m_impl->io};
	if (io.stopped())
	{
		io.restart();
	}
	if (io.run_one_for(timeout) > 0)
	{
		io.poll();
	}
}

} // namespace uqm::net
