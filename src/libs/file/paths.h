// Expands and creates the host directories that the game uses.
#pragma once
#ifndef UQM_LIBS_FILE_PATHS_H_
#define UQM_LIBS_FILE_PATHS_H_

#include "core/stl/stl.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"

namespace uqm
{

// Expands environment variables (%NAME% on Windows, $NAME and ${NAME} on other systems)
// and a leading "~/", makes the path absolute, and removes "." and ".." parts.
// The result uses '/' as the separator. Returns nullopt if the path is not valid.
uqstl::optional<uqstl::string> expandPath(uqstl::string_view path);

// Creates the directory and its parents. Logs and returns false on error.
bool createDirectories(uqgsl::czstring path);

bool fileExists(uqgsl::czstring path);

} // namespace uqm

#endif // UQM_LIBS_FILE_PATHS_H_
