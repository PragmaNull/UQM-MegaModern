// Includes the standard <charconv> header from the selected STL.

#pragma once
#ifndef UQM_CORE_STL_CHARCONV_H
#define UQM_CORE_STL_CHARCONV_H

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <charconv>

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

#endif // UQM_CORE_STL_CHARCONV_H
