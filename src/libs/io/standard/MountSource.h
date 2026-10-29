// Interface for one physical file tree in the mount stack.
#pragma once
#ifndef UQM_LIBS_IO_STANDARD_MOUNTSOURCE_H_
#define UQM_LIBS_IO_STANDARD_MOUNTSOURCE_H_

#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "libs/io/Types.h"
#include "libs/io/standard/Stream.h"

#include <system_error>

namespace uqm::io::standard
{

// Paths are normal paths relative to the root of the source. All functions are safe to call from many threads.
class MountSource
{
public:
	MountSource() = default;
	virtual ~MountSource() = default;

	MountSource(const MountSource&) = delete;
	MountSource& operator=(const MountSource&) = delete;
	MountSource(MountSource&&) = delete;
	MountSource& operator=(MountSource&&) = delete;

	[[nodiscard]] virtual uqstl::optional<FileInfo> stat(uqstl::string_view path) const = 0;
	// Returns the names of the entries in the directory 'path', in no specific order.
	[[nodiscard]] virtual uqstl::vector<uqstl::string> list(uqstl::string_view path, uqstl::error_code& error) const = 0;
	// Returns nullptr and sets 'error' if the file cannot be opened.
	[[nodiscard]] virtual uqstl::unique_ptr<Stream> open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const = 0;

	[[nodiscard]] virtual bool canWrite() const { return false; }
	virtual uqstl::error_code makeDir(uqstl::string_view path) const;
	virtual uqstl::error_code remove(uqstl::string_view path) const;
	virtual uqstl::error_code rename(uqstl::string_view from, uqstl::string_view to) const;
};

// Sets 'error' to 'code' and returns 'result'.
template <typename T>
T fail(uqstl::error_code& error, uqstl::errc code, T result = T {})
{
	error = uqstl::make_error_code(code);
	return result;
}

inline uqstl::error_code MountSource::makeDir(uqstl::string_view /*path*/) const
{
	return uqstl::make_error_code(uqstl::errc::read_only_file_system);
}

inline uqstl::error_code MountSource::remove(uqstl::string_view /*path*/) const
{
	return uqstl::make_error_code(uqstl::errc::read_only_file_system);
}

inline uqstl::error_code MountSource::rename(uqstl::string_view /*from*/, uqstl::string_view /*to*/) const
{
	return uqstl::make_error_code(uqstl::errc::read_only_file_system);
}

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_MOUNTSOURCE_H_
