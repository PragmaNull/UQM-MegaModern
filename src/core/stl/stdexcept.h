// Includes the standard <stdexcept> header from the selected STL.
#pragma once
#ifndef UQM_CORE_STL_STDEXCEPT_H_
#define UQM_CORE_STL_STDEXCEPT_H_

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <stdexcept>

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

#endif /* UQM_CORE_STL_STDEXCEPT_H_ */
