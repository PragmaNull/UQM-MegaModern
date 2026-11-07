#pragma once
#ifndef UQM_CORE_STL_OPTIONAL_H
#define UQM_CORE_STL_OPTIONAL_H

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <optional>

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

namespace uqstl
{
template <typename T>
using late_constructed = uqstl::optional<T>;

} // namespace uqstl

#endif // UQM_CORE_STL_OPTIONAL_H
