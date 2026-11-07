// Reads and writes whole files in a directory.

#pragma once
#ifndef UQM_LIBS_FILE_TEXTFILE_H
#define UQM_LIBS_FILE_TEXTFILE_H

#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "libs/io/FileSystem.h"

namespace uqm
{

// Return nullopt if the file does not exist or cannot be read. The string has capacity for 'padding' more characters.
uqstl::optional<uqstl::string> readTextFile(const io::Directory& dir, uqgsl::czstring name, size_t padding = 0);
// Reads a file at a path of the host file system, for example a path from the command line.
uqstl::optional<uqstl::string> readTextFile(uqgsl::czstring path, size_t padding = 0);
uqstl::optional<uqstl::vector<uqstl::byte>> readBinaryFile(const io::Directory& dir, uqgsl::czstring name);

// Replaces the file. Returns false on error.
bool writeTextFile(const io::Directory& dir, uqgsl::czstring name, uqstl::string_view text);

// Renames an unreadable file to '<name>.bad', replacing an older one, and logs it.
void moveToBadFile(const io::Directory& dir, uqgsl::czstring name);

} // namespace uqm

#endif // UQM_LIBS_FILE_TEXTFILE_H
