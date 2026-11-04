// The network context that the netplay connections use.
#include "netcontext.h"

#include "core/stl/optional.h"

namespace
{

uqstl::optional<uqm::net::Context> s_context {};

} // namespace

void initNetplayContext()
{
	s_context.emplace();
}

void uninitNetplayContext()
{
	s_context.reset();
}

uqm::net::Context& netplayContext()
{
	return *s_context;
}
