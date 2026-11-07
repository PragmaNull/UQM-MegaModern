// The network context that the netplay connections use.

#pragma once
#ifndef UQM_SUPERMELEE_NETPLAY_NETCONTEXT_H
#define UQM_SUPERMELEE_NETPLAY_NETCONTEXT_H

#include "libs/network/NetContext.h"

namespace uqm
{
void initNetplayContext();
// Destroys the context. Close the connections before this.
void uninitNetplayContext();
[[nodiscard]] uqm::net::Context& netplayContext();

} // namespace uqm

#endif // UQM_SUPERMELEE_NETPLAY_NETCONTEXT_H
