// Includes the standard <system_error> header from the selected STL.

#pragma once
#ifndef UQM_CORE_STL_SYSTEM_ERROR_H
#define UQM_CORE_STL_SYSTEM_ERROR_H

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <system_error>

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

#endif // UQM_CORE_STL_SYSTEM_ERROR_H
