// Value types shared by all file system back ends.
#pragma once
#ifndef UQM_LIBS_IO_TYPES_H_
#define UQM_LIBS_IO_TYPES_H_

#include "core/stl/stl.h"
#include "core/types/enum.h"

#include <cstdint>

namespace uqm::io
{

enum class OpenMode : uint8_t
{
	None = 0,
	Read = 1 << 0,
	Write = 1 << 1,
	// Create the file if it does not exist.
	Create = 1 << 2,
	// Remove the contents of an existing file.
	Truncate = 1 << 3,
	// Fail if the file exists. Use with Create.
	Exclusive = 1 << 4,
	// Translate line endings, if the platform does that.
	Text = 1 << 5,
	// Write at the end of the file.
	Append = 1 << 6,
};

ENUM_BITWISE_OPS(OpenMode)

enum class SeekFrom : uint8_t
{
	Start,
	Current,
	End,
};

enum class FileType : uint8_t
{
	Regular,
	Directory,
};

struct FileInfo
{
	FileType type {FileType::Regular};
	uint64_t size {0};
	// Seconds since the Unix epoch.
	int64_t modifiedTime {0};
	bool writable {false};

	[[nodiscard]] bool isDirectory() const { return type == FileType::Directory; }
};

// Where a new mount goes in the mount stack. Higher mounts hide lower mounts.
enum class MountPosition : uint8_t
{
	Top,
	Bottom,
	// Directly above the relative mount.
	Above,
	// Directly below the relative mount.
	Below,
};

enum class Access : uint8_t
{
	ReadOnly,
	ReadWrite,
};

// The 'size' bytes at 'data'.
inline uqstl::span<uqstl::byte> asBytes(void* data, size_t size)
{
	return {static_cast<uqstl::byte*>(data), size};
}

inline uqstl::span<const uqstl::byte> asBytes(const void* data, size_t size)
{
	return {static_cast<const uqstl::byte*>(data), size};
}

} // namespace uqm::io

#endif // UQM_LIBS_IO_TYPES_H_
