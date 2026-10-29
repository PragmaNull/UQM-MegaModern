// Buffered open file. Closes the file when destroyed.
#include "libs/io/standard/File.h"

#include <algorithm>

namespace uqm::io::standard
{

File::File(uqstl::unique_ptr<Stream> stream)
	: m_stream {uqstl::move(stream)}
{
}

void File::close()
{
	if (m_stream != nullptr)
	{
		m_stream->flush(m_error);
	}
	m_stream.reset();
	m_bufferPos = 0;
	m_bufferEnd = 0;
}

size_t File::read(uqstl::span<uqstl::byte> buffer)
{
	if (m_stream == nullptr)
	{
		m_error = uqstl::make_error_code(uqstl::errc::bad_file_descriptor);
		return 0;
	}

	size_t done {0};
	while (done < buffer.size())
	{
		const uqstl::span<uqstl::byte> rest {buffer.subspan(done)};
		if (m_bufferPos < m_bufferEnd)
		{
			const size_t count {uqstl::min(rest.size(), m_bufferEnd - m_bufferPos)};
			uqstl::copy_n(m_buffer.begin() + static_cast<ptrdiff_t>(m_bufferPos), count, rest.begin());
			m_bufferPos += count;
			done += count;
		}
		else if (rest.size() >= c_bufferSize || m_stream->translatesLineEnds())
		{
			const size_t count {m_stream->read(rest, m_error)};
			done += count;
			if (count < rest.size())
			{
				m_eof = !m_error;
				break;
			}
		}
		else if (!fillReadBuffer())
		{
			m_eof = !m_error;
			break;
		}
	}
	return done;
}

size_t File::write(uqstl::span<const uqstl::byte> buffer)
{
	if (m_stream == nullptr)
	{
		m_error = uqstl::make_error_code(uqstl::errc::bad_file_descriptor);
		return 0;
	}
	if (!dropReadBuffer())
	{
		return 0;
	}
	return m_stream->write(buffer, m_error);
}

bool File::readLine(uqstl::span<char> buffer)
{
	if (buffer.empty())
	{
		return false;
	}
	size_t count {0};
	while (count + 1 < buffer.size())
	{
		const int c {getChar()};
		if (c == c_endOfFile)
		{
			break;
		}
		buffer[count++] = static_cast<char>(c);
		if (c == '\n')
		{
			break;
		}
	}
	buffer[count] = '\0';
	return count > 0;
}

int File::getChar()
{
	if (m_bufferPos >= m_bufferEnd && (m_stream == nullptr || !fillReadBuffer()))
	{
		m_eof = m_stream != nullptr && !m_error;
		return c_endOfFile;
	}
	return uqstl::to_integer<int>(m_buffer[m_bufferPos++]);
}

bool File::unget(size_t count)
{
	if (count > m_bufferPos)
	{
		return false;
	}
	m_bufferPos -= count;
	m_eof = false;
	return true;
}

bool File::seek(int64_t offset, SeekFrom from)
{
	if (m_stream == nullptr)
	{
		m_error = uqstl::make_error_code(uqstl::errc::bad_file_descriptor);
		return false;
	}

	int64_t base {0};
	switch (from)
	{
		case SeekFrom::Start:
			break;
		case SeekFrom::Current:
			base = static_cast<int64_t>(tell());
			break;
		case SeekFrom::End:
			base = static_cast<int64_t>(size());
			break;
	}
	if (base + offset < 0)
	{
		m_error = uqstl::make_error_code(uqstl::errc::invalid_argument);
		return false;
	}

	m_bufferPos = 0;
	m_bufferEnd = 0;
	m_eof = false;
	return m_stream->seek(static_cast<uint64_t>(base + offset), m_error);
}

uint64_t File::tell() const
{
	if (m_stream == nullptr)
	{
		return 0;
	}
	return m_stream->tell() - (m_bufferEnd - m_bufferPos);
}

uint64_t File::size()
{
	return m_stream == nullptr ? 0 : m_stream->size();
}

bool File::flush()
{
	if (m_stream == nullptr)
	{
		m_error = uqstl::make_error_code(uqstl::errc::bad_file_descriptor);
		return false;
	}
	return dropReadBuffer() && m_stream->flush(m_error);
}

void File::clearError()
{
	m_error.clear();
	m_eof = false;
}

bool File::dropReadBuffer()
{
	if (m_bufferPos == m_bufferEnd)
	{
		m_bufferPos = 0;
		m_bufferEnd = 0;
		return true;
	}
	const uint64_t position {tell()};
	m_bufferPos = 0;
	m_bufferEnd = 0;
	return m_stream->seek(position, m_error);
}

bool File::fillReadBuffer()
{
	if (m_buffer.empty())
	{
		m_buffer.resize(c_bufferSize);
	}
	// Read streams that translate line ends one byte at a time.
	const size_t readSize {m_stream->translatesLineEnds() ? 1 : m_buffer.size()};
	m_bufferPos = 0;
	m_bufferEnd = m_stream->read(uqstl::span {m_buffer}.first(readSize), m_error);
	return m_bufferEnd > 0;
}

} // namespace uqm::io::standard
