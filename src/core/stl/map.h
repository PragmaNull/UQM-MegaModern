#pragma once
#ifndef UQM_CORE_STL_MAP_H
#define UQM_CORE_STL_MAP_H

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <map>
#include <unordered_map>

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

#endif // UQM_CORE_STL_MAP_H
