// Helpers for paths in the virtual file system.
#pragma once
#ifndef UQM_LIBS_IO_PATH_H_
#define UQM_LIBS_IO_PATH_H_

#include "core/stl/string.h"

namespace uqm::io::path
{

// A normal path has no leading or trailing '/', no empty, "." or ".." parts. The root is "".

// Returns the normal form of 'path'. A ".." part removes the part before it.
uqstl::string normalize(uqstl::string_view path);

// Returns 'relative' resolved against the normal path 'base'. A 'relative' that starts with '/' starts at the root.
uqstl::string join(uqstl::string_view base, uqstl::string_view relative);

// Returns true if the normal path 'path' is 'prefix' or is inside it.
bool isWithin(uqstl::string_view path, uqstl::string_view prefix);

// Returns the part of 'path' after 'prefix'. 'path' must be within 'prefix'.
uqstl::string_view relativeTo(uqstl::string_view path, uqstl::string_view prefix);

// Returns the normal path 'path' without its last part.
uqstl::string_view parent(uqstl::string_view path);

// Returns the last part of the normal path 'path'.
uqstl::string_view fileName(uqstl::string_view path);

} // namespace uqm::io::path

#endif // UQM_LIBS_IO_PATH_H_
