// Checks that a save file loads and saves again with the same content.
#pragma once
#ifndef UQM_SESSION_SAVECHECK_H_
#define UQM_SESSION_SAVECHECK_H_

#include "core/stl/string.h"

namespace uqm
{

// Loads the save file in a new full game session, saves it again, and compares the two.
// Logs each difference as an error. Returns true if they are the same.
[[nodiscard]] bool checkSaveRoundTrip(const uqstl::string& path);

} // namespace uqm

#endif // UQM_SESSION_SAVECHECK_H_
