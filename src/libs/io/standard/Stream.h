// Unbuffered byte stream that a mount source opens.
#pragma once
#ifndef UQM_LIBS_IO_STANDARD_STREAM_H_
#define UQM_LIBS_IO_STANDARD_STREAM_H_

#include "core/stl/stl.h"

#include <cstddef>
#include <cstdint>
#include <system_error>

namespace uqm::io::standard
{

class Stream
{
public:
	Stream() = default;
	virtual ~Stream() = default;

	Stream(const Stream&) = delete;
	Stream& operator=(const Stream&) = delete;
	Stream(Stream&&) = delete;
	Stream& operator=(Stream&&) = delete;

	// Returns the number of bytes read. Fewer than requested means the end of the stream or an error.
	virtual size_t read(uqstl::span<uqstl::byte> buffer, uqstl::error_code& error) = 0;
	// Returns the number of bytes written.
	virtual size_t write(uqstl::span<const uqstl::byte> buffer, uqstl::error_code& error);
	// Moves to the absolute position 'position'.
	virtual bool seek(uint64_t position, uqstl::error_code& error) = 0;
	[[nodiscard]] virtual uint64_t tell() const = 0;
	[[nodiscard]] virtual uint64_t size() = 0;
	virtual bool flush(uqstl::error_code& error);
};

inline size_t Stream::write(uqstl::span<const uqstl::byte> /*buffer*/, uqstl::error_code& error)
{
	error = uqstl::make_error_code(uqstl::errc::bad_file_descriptor);
	return 0;
}

inline bool Stream::flush(uqstl::error_code& /*error*/)
{
	return true;
}

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_STREAM_H_
