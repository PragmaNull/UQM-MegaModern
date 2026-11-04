// The network context that the netplay connections use.
#pragma once
#ifndef UQM_SUPERMELEE_NETPLAY_NETCONTEXT_H_
#define UQM_SUPERMELEE_NETPLAY_NETCONTEXT_H_

#include "libs/network/NetContext.h"

void initNetplayContext();
// Destroys the context. Close the connections before this.
void uninitNetplayContext();
[[nodiscard]] uqm::net::Context& netplayContext();

#endif // UQM_SUPERMELEE_NETPLAY_NETCONTEXT_H_
