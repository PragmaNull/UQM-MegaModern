// Mount source for a directory on the host file system.
#include "libs/io/standard/DirectorySource.h"

#include <cerrno>
#include <chrono>
#include <fstream>

namespace uqm::io::standard
{

namespace
{

namespace fs = uqstl::filesystem;

class DiskStream final : public Stream
{
public:
	explicit DiskStream(bool text)
		: m_text {text}
	{
	}

	[[nodiscard]] bool translatesLineEnds() const override { return m_text; }

	bool open(const fs::path& path, uqstl::ios_base::openmode mode, uqstl::error_code& error)
	{
		errno = 0;
		if (!m_text)
		{
			// File buffers the reads.
			m_buffer.pubsetbuf(nullptr, 0);
		}
		if (m_buffer.open(path, mode) == nullptr)
		{
			error = errno != 0 ? uqstl::error_code {errno, uqstl::generic_category()} : uqstl::make_error_code(uqstl::errc::io_error);
			return false;
		}
		return true;
	}

	size_t read(uqstl::span<uqstl::byte> buffer, uqstl::error_code& /*error*/) override
	{
		const uqstl::streamsize count {m_buffer.sgetn(reinterpret_cast<char*>(buffer.data()), static_cast<uqstl::streamsize>(buffer.size()))};
		return static_cast<size_t>(uqstl::max<uqstl::streamsize>(count, 0));
	}

	size_t write(uqstl::span<const uqstl::byte> buffer, uqstl::error_code& error) override
	{
		m_size.reset();
		const uqstl::streamsize count {m_buffer.sputn(reinterpret_cast<const char*>(buffer.data()), static_cast<uqstl::streamsize>(buffer.size()))};
		if (count < static_cast<uqstl::streamsize>(buffer.size()))
		{
			error = uqstl::make_error_code(uqstl::errc::io_error);
		}
		return static_cast<size_t>(uqstl::max<uqstl::streamsize>(count, 0));
	}

	bool seek(uint64_t position, uqstl::error_code& error) override
	{
		const auto result {m_buffer.pubseekpos(static_cast<uqstl::streamoff>(position))};
		if (result == uqstl::streampos {uqstl::streamoff {-1}})
		{
			error = uqstl::make_error_code(uqstl::errc::invalid_seek);
			return false;
		}
		return true;
	}

	[[nodiscard]] uint64_t tell() const override
	{
		const auto position {m_buffer.pubseekoff(0, uqstl::ios_base::cur)};
		return position == uqstl::streampos {uqstl::streamoff {-1}} ? 0 : static_cast<uint64_t>(uqstl::streamoff {position});
	}

	[[nodiscard]] uint64_t size() override
	{
		if (!m_size)
		{
			const auto current {m_buffer.pubseekoff(0, uqstl::ios_base::cur)};
			const auto end {m_buffer.pubseekoff(0, uqstl::ios_base::end)};
			m_buffer.pubseekpos(current);
			m_size = end == uqstl::streampos {uqstl::streamoff {-1}} ? 0 : static_cast<uint64_t>(uqstl::streamoff {end});
		}
		return *m_size;
	}

	bool flush(uqstl::error_code& error) override
	{
		if (m_buffer.pubsync() == -1)
		{
			error = uqstl::make_error_code(uqstl::errc::io_error);
			return false;
		}
		return true;
	}

private:
	// Seeking does not change the file, so const functions can seek.
	mutable uqstl::filebuf m_buffer {};
	bool m_text {false};
	uqstl::optional<uint64_t> m_size {};
};

int64_t toUnixTime(fs::file_time_type time)
{
	const auto systemTime {uqstl::chrono::clock_cast<uqstl::chrono::system_clock>(time)};
	return uqstl::chrono::duration_cast<uqstl::chrono::seconds>(systemTime.time_since_epoch()).count();
}

uqstl::ios_base::openmode openFlags(OpenMode mode, bool exists)
{
	uqstl::ios_base::openmode flags {};
	if (!testAllBits(mode, OpenMode::Text))
	{
		flags |= uqstl::ios_base::binary;
	}
	if (!testAllBits(mode, OpenMode::Write))
	{
		return flags | uqstl::ios_base::in;
	}

	flags |= uqstl::ios_base::out;
	if (testAllBits(mode, OpenMode::Append))
	{
		// open() empties an existing file first if the mode has Truncate.
		flags |= uqstl::ios_base::app;
		return testAllBits(mode, OpenMode::Read) ? flags | uqstl::ios_base::in : flags;
	}
	if (testAllBits(mode, OpenMode::Truncate) || !exists)
	{
		if (testAllBits(mode, OpenMode::Read))
		{
			flags |= uqstl::ios_base::in | uqstl::ios_base::trunc;
		}
		return flags;
	}
	// Keep the contents of an existing file.
	return flags | uqstl::ios_base::in;
}

} // namespace

DirectorySource::DirectorySource(fs::path root)
	: m_root {uqstl::move(root)}
{
}

fs::path DirectorySource::hostPath(uqstl::string_view path) const
{
	if (path.empty())
	{
		return m_root;
	}
	return m_root / fs::path {uqstl::string {path}};
}

uqstl::optional<FileInfo> DirectorySource::stat(uqstl::string_view path) const
{
	uqstl::error_code error {};
	const fs::directory_entry entry {hostPath(path), error};
	if (error || !entry.exists(error))
	{
		return uqstl::nullopt;
	}

	FileInfo info {};
	info.type = entry.is_directory(error) ? FileType::Directory : FileType::Regular;
	info.writable = (entry.status(error).permissions() & fs::perms::owner_write) != fs::perms::none;
	if (!info.isDirectory())
	{
		const uintmax_t size {entry.file_size(error)};
		info.size = error ? 0 : size;
	}
	const fs::file_time_type modified {entry.last_write_time(error)};
	if (!error)
	{
		info.modifiedTime = toUnixTime(modified);
	}
	return info;
}

uqstl::vector<uqstl::string> DirectorySource::list(uqstl::string_view path, uqstl::error_code& error) const
{
	uqstl::vector<uqstl::string> names {};
	for (fs::directory_iterator it {hostPath(path), error}; !error && it != fs::directory_iterator {}; it.increment(error))
	{
		try
		{
			names.push_back(it->path().filename().string());
		}
		catch (const uqstl::system_error&)
		{
			// The name cannot be shown in the narrow character set.
		}
	}
	return names;
}

uqstl::unique_ptr<Stream> DirectorySource::open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const
{
	const fs::path host {hostPath(path)};
	if (!testAllBits(mode, OpenMode::Write))
	{
		auto stream {uqstl::make_unique<DiskStream>(testAllBits(mode, OpenMode::Text))};
		if (stream->open(host, openFlags(mode, true), error))
		{
			return stream;
		}
		uqstl::error_code statusError {};
		const fs::file_status status {fs::status(host, statusError)};
		if (!fs::exists(status))
		{
			error = uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
		}
		else if (fs::is_directory(status))
		{
			error = uqstl::make_error_code(uqstl::errc::is_a_directory);
		}
		return nullptr;
	}

	uqstl::error_code statusError {};
	const fs::file_status status {fs::status(host, statusError)};
	const bool exists {fs::exists(status)};

	if (exists && fs::is_directory(status))
	{
		return fail<uqstl::unique_ptr<Stream>>(error, uqstl::errc::is_a_directory);
	}
	if (testAllBits(mode, OpenMode::Write))
	{
		if (exists && testAllBits(mode, OpenMode::Create | OpenMode::Exclusive))
		{
			return fail<uqstl::unique_ptr<Stream>>(error, uqstl::errc::file_exists);
		}
		if (!exists && !testAllBits(mode, OpenMode::Create))
		{
			return fail<uqstl::unique_ptr<Stream>>(error, uqstl::errc::no_such_file_or_directory);
		}
	}
	else if (!exists)
	{
		return fail<uqstl::unique_ptr<Stream>>(error, uqstl::errc::no_such_file_or_directory);
	}

	if (exists && testAllBits(mode, OpenMode::Write | OpenMode::Append | OpenMode::Truncate))
	{
		fs::resize_file(host, 0, error);
		if (error)
		{
			return nullptr;
		}
	}

	auto stream {uqstl::make_unique<DiskStream>(testAllBits(mode, OpenMode::Text))};
	if (!stream->open(host, openFlags(mode, exists), error))
	{
		return nullptr;
	}
	return stream;
}

uqstl::error_code DirectorySource::makeDir(uqstl::string_view path) const
{
	uqstl::error_code error {};
	if (!fs::create_directory(hostPath(path), error) && !error)
	{
		error = uqstl::make_error_code(uqstl::errc::file_exists);
	}
	return error;
}

uqstl::error_code DirectorySource::remove(uqstl::string_view path) const
{
	uqstl::error_code error {};
	if (fs::is_directory(hostPath(path), error))
	{
		return uqstl::make_error_code(uqstl::errc::is_a_directory);
	}
	error.clear();
	if (!fs::remove(hostPath(path), error) && !error)
	{
		error = uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
	}
	return error;
}

uqstl::error_code DirectorySource::rename(uqstl::string_view from, uqstl::string_view to) const
{
	uqstl::error_code error {};
	if (fs::exists(hostPath(to), error))
	{
		return uqstl::make_error_code(uqstl::errc::file_exists);
	}
	error.clear();
	fs::rename(hostPath(from), hostPath(to), error);
	return error;
}

} // namespace uqm::io::standard
