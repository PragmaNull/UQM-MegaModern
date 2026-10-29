// File system types of the back end that the build selects.
#pragma once
#ifndef UQM_LIBS_IO_FILESYSTEM_H_
#define UQM_LIBS_IO_FILESYSTEM_H_

#include "libs/io/Concepts.h"

// Generated from Backend.h.in. Sets UQM_IO_BACKEND_NAMESPACE.
#include "libs/io/Backend.h"

namespace uqm::io
{

using FileSystem = UQM_IO_BACKEND_NAMESPACE::FileSystem;
using File = FileSystem::File;
using Directory = FileSystem::Directory;
using Mount = FileSystem::Mount;

static_assert(FileSystemImpl<FileSystem>);

} // namespace uqm::io

#endif // UQM_LIBS_IO_FILESYSTEM_H_
