// Buffered open file. Closes the file when destroyed.
#pragma once
#ifndef UQM_LIBS_IO_STANDARD_FILE_H_
#define UQM_LIBS_IO_STANDARD_FILE_H_

#include "core/stl/memory.h"
#include "core/stl/vector.h"
#include "libs/io/Types.h"
#include "libs/io/standard/Stream.h"

#include <system_error>

namespace uqm::io::standard
{

class File
{
public:
	static constexpr int c_endOfFile {-1};

	File() = default;
	explicit File(uqstl::unique_ptr<Stream> stream);

	[[nodiscard]] bool isOpen() const { return m_stream != nullptr; }
	void close();

	// Returns the number of bytes read. Fewer than requested means the end of the file or an error.
	size_t read(uqstl::span<uqstl::byte> buffer);
	// Returns the number of bytes written.
	size_t write(uqstl::span<const uqstl::byte> buffer);
	// Return false if not all bytes were read or written.
	bool readExact(uqstl::span<uqstl::byte> buffer) { return read(buffer) == buffer.size(); }
	bool writeAll(uqstl::span<const uqstl::byte> buffer) { return write(buffer) == buffer.size(); }
	// Reads the bytes of one value. Returns false if not all bytes were read.
	template <typename T>
		requires uqstl::is_trivially_copyable_v<T>
	bool readValue(T& value)
	{
		return readExact(uqstl::as_writable_bytes(uqstl::span {&value, 1}));
	}
	// Reads up to and including the next newline, at most buffer.size() - 1 characters, and adds a null character.
	// Returns false if no characters were read.
	bool readLine(uqstl::span<char> buffer);
	// Returns the next byte, or c_endOfFile.
	int getChar();
	// Moves back 'count' bytes that were just read. Returns false if they are no longer in the buffer.
	bool unget(size_t count = 1);

	bool seek(int64_t offset, SeekFrom from);
	[[nodiscard]] uint64_t tell() const;
	[[nodiscard]] uint64_t size();
	bool flush();

	// True after a read stopped at the end of the file.
	[[nodiscard]] bool eof() const { return m_eof; }
	[[nodiscard]] uqstl::error_code error() const { return m_error; }
	void clearError();

private:
	static constexpr size_t c_bufferSize {16384};

	// Returns false and sets the error if the file is closed.
	bool checkOpen();
	// Moves the stream to the position the reader sees, and drops the read buffer.
	bool dropReadBuffer();
	bool fillReadBuffer();

	uqstl::unique_ptr<Stream> m_stream {};
	uqstl::vector<uqstl::byte> m_buffer {};
	size_t m_bufferPos {0};
	size_t m_bufferEnd {0};
	bool m_eof {false};
	uqstl::error_code m_error {};
};

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_FILE_H_
