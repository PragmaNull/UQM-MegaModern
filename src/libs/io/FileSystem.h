// The file system types that the game uses.
#pragma once
#ifndef UQM_LIBS_IO_FILESYSTEM_H_
#define UQM_LIBS_IO_FILESYSTEM_H_

#include "libs/io/standard/FileSystem.h"

namespace uqm::io
{

using FileSystem = standard::FileSystem;
using File = FileSystem::File;
using Directory = FileSystem::Directory;
using Mount = FileSystem::Mount;

} // namespace uqm::io

#endif // UQM_LIBS_IO_FILESYSTEM_H_
