// Reads and writes whole text files in a uio directory.
#pragma once
#ifndef UQM_LIBS_FILE_TEXTFILE_H_
#define UQM_LIBS_FILE_TEXTFILE_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "libs/uio.h"

#include <optional>

namespace uqm
{

// Returns nullopt if the file does not exist or cannot be read.
uqstl::optional<uqstl::string> readTextFile(uio_DirHandle* dir, uqgsl::czstring name);

// Replaces the file. Returns false on error.
bool writeTextFile(uio_DirHandle* dir, uqgsl::czstring name, uqstl::string_view text);

// Renames an unreadable file to '<name>.bad', replacing an older one, and logs it.
void moveToBadFile(uio_DirHandle* dir, uqgsl::czstring name);

} // namespace uqm

#endif // UQM_LIBS_FILE_TEXTFILE_H_
