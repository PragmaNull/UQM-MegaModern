// Ends the game after a fatal error.

#pragma once
#ifndef UQM_CORE_MISC_FATALEXIT_H
#define UQM_CORE_MISC_FATALEXIT_H

#include <cstdlib>

namespace uqm
{

// Aborts in debug builds, so that a debugger stops here. Exits with EXIT_FAILURE in other builds.
[[noreturn]] inline void fatalExit()
{
#ifdef DEBUG
	std::abort();
#else
	std::exit(EXIT_FAILURE);
#endif
}

} // namespace uqm

#endif // UQM_CORE_MISC_FATALEXIT_H
