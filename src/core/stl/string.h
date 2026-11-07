#pragma once
#ifndef UQM_CORE_STL_STRING_H
#define UQM_CORE_STL_STRING_H

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <string>
#include <string_view>

namespace gsl_lite
{
using not_null_czstring = not_null<czstring>;
}

namespace std
{
template <size_t N>
using fixed_string = string; // std library has no fixed string :(
}

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

#endif // UQM_CORE_STL_STRING_H
