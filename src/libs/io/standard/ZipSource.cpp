// Read-only mount source for a zip archive.
// Based on the uio zip code, Copyright (C) 2003 Serge van den Boom, GPL version 2.
#include "libs/io/standard/ZipSource.h"

#include "core/log/log.h"
#include "core/stl/array.h"
#include "libs/io/Path.h"

#include <chrono>
#include <zlib.h>

namespace uqm::io::standard
{

namespace
{

constexpr uint32_t c_endRecordSignature {0x06054b50};
constexpr uint32_t c_centralHeaderSignature {0x02014b50};
constexpr uint32_t c_localHeaderSignature {0x04034b50};
constexpr size_t c_endRecordSize {22};
constexpr size_t c_centralHeaderSize {46};
constexpr size_t c_localHeaderSize {30};
constexpr size_t c_maxCommentSize {0xffff};
constexpr size_t c_inputBufferSize {0x10000};
// Deflated entries up to this size are inflated into memory when opened.
constexpr uint64_t c_memoryEntryLimit {4 * 1024 * 1024};

constexpr uint16_t c_methodStored {0};
constexpr uint16_t c_methodDeflated {8};

constexpr uint8_t c_hostFat {0};
constexpr uint8_t c_hostUnix {3};
constexpr uint8_t c_hostNtfs {11};
constexpr uint8_t c_hostVfat {14};
constexpr uint32_t c_fatDirectoryAttribute {0x10};
constexpr uint32_t c_unixTypeMask {0170000};
constexpr uint32_t c_unixDirectoryType {0040000};

uqstl::error_code badArchive()
{
	return uqstl::make_error_code(uqstl::errc::illegal_byte_sequence);
}

uint16_t readU16(uqstl::span<const uqstl::byte> data, size_t offset)
{
	return static_cast<uint16_t>(uqstl::to_integer<uint16_t>(data[offset]) | (uqstl::to_integer<uint16_t>(data[offset + 1]) << 8));
}

uint32_t readU32(uqstl::span<const uqstl::byte> data, size_t offset)
{
	return static_cast<uint32_t>(readU16(data, offset)) | (static_cast<uint32_t>(readU16(data, offset + 2)) << 16);
}

// Reads exactly 'buffer.size()' bytes at 'position'.
bool readAt(Stream& stream, uint64_t position, uqstl::span<uqstl::byte> buffer, uqstl::error_code& error)
{
	if (!stream.seek(position, error))
	{
		return false;
	}
	if (stream.read(buffer, error) != buffer.size())
	{
		if (!error)
		{
			error = badArchive();
		}
		return false;
	}
	return true;
}

int64_t dosToUnixTime(uint16_t date, uint16_t time)
{
	using namespace uqstl::chrono;
	const year_month_day day {year {1980 + ((date >> 9) & 0x7f)}, month {static_cast<unsigned>((date >> 5) & 0x0f)}, uqstl::chrono::day {static_cast<unsigned>(date & 0x1f)}};
	if (!day.ok())
	{
		return 0;
	}
	const auto timeOfDay {hours {time >> 11} + minutes {(time >> 5) & 0x3f} + seconds {(time & 0x1f) * 2}};
	return duration_cast<seconds>(sys_days {day}.time_since_epoch() + timeOfDay).count();
}

bool isDirectoryAttribute(uint8_t host, uint32_t attributes)
{
	switch (host)
	{
		case c_hostFat:
		case c_hostNtfs:
		case c_hostVfat:
			return (attributes & c_fatDirectoryAttribute) != 0;
		case c_hostUnix:
			return ((attributes >> 16) & c_unixTypeMask) == c_unixDirectoryType;
		default:
			return false;
	}
}

// Owns a zlib inflate state for raw deflate data.
class ZStream
{
public:
	ZStream()
		: m_valid {inflateInit2(&m_stream, -MAX_WBITS) == Z_OK}
	{
	}

	~ZStream()
	{
		if (m_valid)
		{
			inflateEnd(&m_stream);
		}
	}

	ZStream(const ZStream&) = delete;
	ZStream& operator=(const ZStream&) = delete;
	ZStream(ZStream&&) = delete;
	ZStream& operator=(ZStream&&) = delete;

	[[nodiscard]] bool valid() const { return m_valid; }
	bool reset() { return inflateReset(&m_stream) == Z_OK; }
	z_stream& get() { return m_stream; }

private:
	z_stream m_stream {};
	bool m_valid {false};
};

// A read-only stream over bytes in memory.
class MemoryStream final : public Stream
{
public:
	explicit MemoryStream(uqstl::vector<uqstl::byte> data)
		: m_data {uqstl::move(data)}
	{
	}

	size_t read(uqstl::span<uqstl::byte> buffer, uqstl::error_code& /*error*/) override
	{
		const size_t count {uqstl::min(buffer.size(), m_data.size() - m_position)};
		uqstl::copy_n(m_data.begin() + static_cast<ptrdiff_t>(m_position), count, buffer.begin());
		m_position += count;
		return count;
	}

	bool seek(uint64_t position, uqstl::error_code& /*error*/) override
	{
		m_position = static_cast<size_t>(uqstl::min<uint64_t>(position, m_data.size()));
		return true;
	}

	[[nodiscard]] uint64_t tell() const override { return m_position; }
	[[nodiscard]] uint64_t size() override { return m_data.size(); }

private:
	uqstl::vector<uqstl::byte> m_data {};
	size_t m_position {0};
};

class ZipEntryStream final : public Stream
{
public:
	ZipEntryStream(uqstl::unique_ptr<Stream> archive, uint64_t dataOffset, const ZipSource::Entry& entry)
		: m_archive {uqstl::move(archive)}
		, m_dataOffset {dataOffset}
		, m_entry {entry}
	{
		if (m_entry.deflated)
		{
			m_inflater = uqstl::make_unique<ZStream>();
			m_input.resize(static_cast<size_t>(uqstl::min<uint64_t>(c_inputBufferSize, uqstl::max<uint64_t>(m_entry.compressedSize, 1))));
		}
	}

	[[nodiscard]] bool valid() const { return !m_entry.deflated || m_inflater->valid(); }

	size_t read(uqstl::span<uqstl::byte> buffer, uqstl::error_code& error) override
	{
		const size_t count {static_cast<size_t>(uqstl::min<uint64_t>(buffer.size(), m_entry.size - m_position))};
		if (count == 0)
		{
			return 0;
		}
		const size_t done {m_entry.deflated ? readDeflated(buffer.first(count), error) : readStored(buffer.first(count), error)};
		m_position += done;
		return done;
	}

	bool seek(uint64_t position, uqstl::error_code& error) override
	{
		position = uqstl::min(position, m_entry.size);
		if (!m_entry.deflated)
		{
			m_position = position;
			return true;
		}

		if (position < m_position)
		{
			if (!m_inflater->reset())
			{
				error = uqstl::make_error_code(uqstl::errc::io_error);
				return false;
			}
			m_inflater->get().avail_in = 0;
			m_compressedRead = 0;
			m_position = 0;
		}
		uqstl::array<uqstl::byte, 4096> scratch {};
		while (m_position < position)
		{
			const size_t count {static_cast<size_t>(uqstl::min<uint64_t>(scratch.size(), position - m_position))};
			if (read(uqstl::span {scratch}.first(count), error) == 0)
			{
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] uint64_t tell() const override { return m_position; }
	[[nodiscard]] uint64_t size() override { return m_entry.size; }

private:
	size_t readStored(uqstl::span<uqstl::byte> buffer, uqstl::error_code& error)
	{
		const uint64_t archivePosition {m_dataOffset + m_position};
		if (m_archivePosition != archivePosition && !m_archive->seek(archivePosition, error))
		{
			return 0;
		}
		const size_t count {m_archive->read(buffer, error)};
		m_archivePosition = archivePosition + count;
		return count;
	}

	size_t readDeflated(uqstl::span<uqstl::byte> buffer, uqstl::error_code& error)
	{
		z_stream& stream {m_inflater->get()};
		stream.next_out = reinterpret_cast<Bytef*>(buffer.data());
		stream.avail_out = static_cast<uInt>(buffer.size());
		while (stream.avail_out > 0)
		{
			if (stream.avail_in == 0)
			{
				const size_t chunk {static_cast<size_t>(uqstl::min<uint64_t>(m_input.size(), m_entry.compressedSize - m_compressedRead))};
				if (chunk == 0)
				{
					error = badArchive();
					break;
				}
				const uqstl::span<uqstl::byte> input {uqstl::span {m_input}.first(chunk)};
				if (!readAt(*m_archive, m_dataOffset + m_compressedRead, input, error))
				{
					break;
				}
				m_compressedRead += chunk;
				stream.next_in = reinterpret_cast<Bytef*>(input.data());
				stream.avail_in = static_cast<uInt>(chunk);
			}
			const int result {inflate(&stream, Z_SYNC_FLUSH)};
			if (result == Z_STREAM_END)
			{
				break;
			}
			if (result != Z_OK)
			{
				error = badArchive();
				break;
			}
		}
		return buffer.size() - stream.avail_out;
	}

	uqstl::unique_ptr<Stream> m_archive {};
	// The position of m_archive, if known.
	uint64_t m_archivePosition {UINT64_MAX};
	uint64_t m_dataOffset {0};
	ZipSource::Entry m_entry {};
	uint64_t m_position {0};
	uint64_t m_compressedRead {0};
	uqstl::unique_ptr<ZStream> m_inflater {};
	uqstl::vector<uqstl::byte> m_input {};
};

} // namespace

ZipSource::ZipSource(uqstl::shared_ptr<const MountSource> archive, uqstl::string path)
	: m_archive {uqstl::move(archive)}
	, m_path {uqstl::move(path)}
{
	m_directories.emplace("", uqstl::vector<uqstl::string> {});
}

uqstl::shared_ptr<ZipSource> ZipSource::create(uqstl::shared_ptr<const MountSource> archive, uqstl::string path, uqstl::error_code& error)
{
	auto source {uqstl::make_shared<ZipSource>(uqstl::move(archive), uqstl::move(path))};
	if (!source->readCentralDirectory(error))
	{
		return nullptr;
	}
	return source;
}

bool ZipSource::readCentralDirectory(uqstl::error_code& error)
{
	const uqstl::unique_ptr<Stream> stream {m_archive->open(m_path, OpenMode::Read, error)};
	if (stream == nullptr)
	{
		return false;
	}

	const uint64_t fileSize {stream->size()};
	if (fileSize < c_endRecordSize)
	{
		error = badArchive();
		return false;
	}
	const uint64_t searchStart {fileSize > c_endRecordSize + c_maxCommentSize ? fileSize - c_endRecordSize - c_maxCommentSize : 0};
	uqstl::vector<uqstl::byte> tail(static_cast<size_t>(fileSize - searchStart));
	if (!readAt(*stream, searchStart, tail, error))
	{
		return false;
	}

	size_t endRecord {tail.size() - c_endRecordSize + 1};
	do
	{
		if (endRecord == 0)
		{
			error = badArchive();
			return false;
		}
		--endRecord;
	} while (readU32(tail, endRecord) != c_endRecordSignature);

	const uint16_t numEntries {readU16(tail, endRecord + 10)};
	const uint32_t directorySize {readU32(tail, endRecord + 12)};
	const uint32_t directoryOffset {readU32(tail, endRecord + 16)};
	if (numEntries == 0xffff || directoryOffset == 0xffffffff)
	{
		uqm::log::error("Zip64 archive '{}' is not supported.", m_path);
		error = uqstl::make_error_code(uqstl::errc::not_supported);
		return false;
	}

	uqstl::vector<uqstl::byte> directory(directorySize);
	if (!readAt(*stream, directoryOffset, directory, error))
	{
		return false;
	}

	size_t offset {0};
	for (uint16_t i {0}; i < numEntries; ++i)
	{
		if (offset + c_centralHeaderSize > directory.size() || readU32(directory, offset) != c_centralHeaderSignature)
		{
			error = badArchive();
			return false;
		}
		const uqstl::span<const uqstl::byte> header {uqstl::span {directory}.subspan(offset)};
		const uint8_t host {uqstl::to_integer<uint8_t>(header[5])};
		const uint16_t flags {readU16(header, 8)};
		const uint16_t method {readU16(header, 10)};
		const uint16_t nameLength {readU16(header, 28)};
		const size_t entrySize {c_centralHeaderSize + nameLength + readU16(header, 30) + readU16(header, 32)};
		if (offset + entrySize > directory.size())
		{
			error = badArchive();
			return false;
		}
		offset += entrySize;

		uqstl::string name(nameLength, '\0');
		for (size_t c {0}; c < nameLength; ++c)
		{
			name[c] = uqstl::to_integer<char>(header[c_centralHeaderSize + c]);
		}

		Entry entry {};
		entry.compressedSize = readU32(header, 20);
		entry.size = readU32(header, 24);
		entry.headerOffset = readU32(header, 42);
		entry.modifiedTime = dosToUnixTime(readU16(header, 14), readU16(header, 12));
		entry.deflated = method == c_methodDeflated;

		if (method != c_methodStored && method != c_methodDeflated)
		{
			uqm::log::warn("Zip entry '{}' uses unsupported method {}; skipped.", name, method);
			continue;
		}
		if ((flags & 1) != 0)
		{
			uqm::log::warn("Zip entry '{}' is encrypted; skipped.", name);
			continue;
		}
		if (entry.compressedSize == 0xffffffff || entry.size == 0xffffffff || entry.headerOffset == 0xffffffff)
		{
			uqm::log::warn("Zip64 entry '{}' is not supported; skipped.", name);
			continue;
		}

		const bool isDirectory {name.ends_with('/') || isDirectoryAttribute(host, readU32(header, 38))};
		addEntry(path::normalize(name), isDirectory, entry);
	}
	return true;
}

void ZipSource::addEntry(uqstl::string name, bool isDirectory, const Entry& entry)
{
	if (name.empty())
	{
		return;
	}
	if (isDirectory)
	{
		addDirectory(name);
		return;
	}
	if (m_directories.contains(name) || m_files.contains(name))
	{
		uqm::log::warn("Zip entry '{}' is a duplicate; skipped.", name);
		return;
	}
	if (linkToParent(name))
	{
		m_files.emplace(uqstl::move(name), entry);
	}
}

void ZipSource::addDirectory(const uqstl::string& name)
{
	if (m_directories.contains(name))
	{
		return;
	}
	if (m_files.contains(name))
	{
		uqm::log::warn("Zip entry '{}' is a file and a directory; skipped.", name);
		return;
	}
	if (linkToParent(name))
	{
		m_directories.emplace(name, uqstl::vector<uqstl::string> {});
	}
}

bool ZipSource::linkToParent(const uqstl::string& name)
{
	const uqstl::string parent {path::parent(name)};
	addDirectory(parent);
	const auto directory {m_directories.find(parent)};
	if (directory == m_directories.end())
	{
		return false;
	}
	directory->second.emplace_back(path::fileName(name));
	return true;
}

uqstl::optional<FileInfo> ZipSource::stat(uqstl::string_view path) const
{
	if (m_directories.contains(path))
	{
		return FileInfo {.type = FileType::Directory};
	}
	const auto file {m_files.find(path)};
	if (file == m_files.end())
	{
		return uqstl::nullopt;
	}
	return FileInfo {.type = FileType::Regular, .size = file->second.size, .modifiedTime = file->second.modifiedTime};
}

uqstl::vector<uqstl::string> ZipSource::list(uqstl::string_view path, uqstl::error_code& error) const
{
	const auto directory {m_directories.find(path)};
	if (directory == m_directories.end())
	{
		return fail<uqstl::vector<uqstl::string>>(error, m_files.contains(path) ? uqstl::errc::not_a_directory : uqstl::errc::no_such_file_or_directory);
	}
	return directory->second;
}

uqstl::unique_ptr<Stream> ZipSource::open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const
{
	if (testAllBits(mode, OpenMode::Write))
	{
		return fail<uqstl::unique_ptr<Stream>>(error, uqstl::errc::read_only_file_system);
	}
	const auto file {m_files.find(path)};
	if (file == m_files.end())
	{
		return fail<uqstl::unique_ptr<Stream>>(error, m_directories.contains(path) ? uqstl::errc::is_a_directory : uqstl::errc::no_such_file_or_directory);
	}
	const Entry& entry {file->second};

	uqstl::unique_ptr<Stream> archive {m_archive->open(m_path, OpenMode::Read, error)};
	if (archive == nullptr)
	{
		return nullptr;
	}
	uqstl::array<uqstl::byte, c_localHeaderSize> header {};
	if (!readAt(*archive, entry.headerOffset, header, error))
	{
		return nullptr;
	}
	if (readU32(header, 0) != c_localHeaderSignature)
	{
		error = badArchive();
		return nullptr;
	}
	const uint64_t dataOffset {entry.headerOffset + c_localHeaderSize + readU16(header, 26) + readU16(header, 28)};

	auto stream {uqstl::make_unique<ZipEntryStream>(uqstl::move(archive), dataOffset, entry)};
	if (!stream->valid())
	{
		return fail<uqstl::unique_ptr<Stream>>(error, uqstl::errc::not_enough_memory);
	}
	if (!entry.deflated || entry.size > c_memoryEntryLimit)
	{
		return stream;
	}

	// Inflate small entries into memory.
	uqstl::vector<uqstl::byte> data(static_cast<size_t>(entry.size));
	if (stream->read(data, error) != data.size())
	{
		return fail<uqstl::unique_ptr<Stream>>(error, uqstl::errc::illegal_byte_sequence);
	}
	return uqstl::make_unique<MemoryStream>(uqstl::move(data));
}

} // namespace uqm::io::standard
