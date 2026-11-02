// std::expected, and a helper that makes an error result.
#pragma once
#ifndef UQM_CORE_STL_EXPECTED_H_
#define UQM_CORE_STL_EXPECTED_H_

#include "core/stl/stl.h"

#ifndef USING_EASTL

#include <expected>
#include <system_error>

#else

static_assert(false, "EASTL is not supported yet. If you want to add support for it, submit a PR!");
#endif /* !defined(USING_EASTL)*/

namespace uqm
{

// An error result with 'code'.
inline uqstl::unexpected<uqstl::error_code> failure(uqstl::errc code)
{
	return uqstl::unexpected {uqstl::make_error_code(code)};
}

} // namespace uqm

#endif /* UQM_CORE_STL_EXPECTED_H_ */
