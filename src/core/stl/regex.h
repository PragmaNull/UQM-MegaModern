// Includes the standard <regex> header from the selected STL.

#pragma once
#ifndef UQM_CORE_STL_REGEX_H
#define UQM_CORE_STL_REGEX_H

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <regex>

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

#endif // UQM_CORE_STL_REGEX_H
