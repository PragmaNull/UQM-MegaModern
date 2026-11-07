// Reads and writes whole files in a directory.
#include "textfile.h"

#include <fmt/format.h>

#include "core/log/log.h"
#include "core/stl/fstream.h"
#include "core/stl/iterator.h"

namespace uqm
{

namespace
{

// Reads the whole file into a container of byte-sized elements, with room for 'padding' more elements.
template <typename Container>
uqstl::optional<Container> readWholeFile(const io::Directory& dir, uqgsl::czstring name, size_t padding = 0)
{
	io::Result<io::File> file {dir.open(name, io::OpenMode::Read)};
	if (!file)
	{
		return uqstl::nullopt;
	}

	const size_t size {static_cast<size_t>(file->size())};
	Container data {};
	data.reserve(size + padding);
	data.resize(size);
	data.resize(file->read(uqstl::as_writable_bytes(uqstl::span {data})));
	if (file->error())
	{
		return uqstl::nullopt;
	}
	return data;
}

} // namespace

uqstl::optional<uqstl::string> readTextFile(const io::Directory& dir, uqgsl::czstring name, size_t padding)
{
	return readWholeFile<uqstl::string>(dir, name, padding);
}

uqstl::optional<uqstl::string> readTextFile(uqgsl::czstring path, size_t padding)
{
	uqstl::ifstream file {path, uqstl::ios::binary | uqstl::ios::ate};
	if (!file)
	{
		return uqstl::nullopt;
	}
	const auto size {static_cast<size_t>(file.tellg())};
	uqstl::string text {};
	text.reserve(size + padding);
	text.resize(size);
	file.seekg(0);
	if (!file.read(text.data(), static_cast<uqstl::streamsize>(size)))
	{
		return uqstl::nullopt;
	}
	return text;
}

uqstl::optional<uqstl::vector<uqstl::byte>> readBinaryFile(const io::Directory& dir, uqgsl::czstring name)
{
	return readWholeFile<uqstl::vector<uqstl::byte>>(dir, name);
}

bool writeTextFile(const io::Directory& dir, uqgsl::czstring name, uqstl::string_view text)
{
	io::Result<io::File> file {dir.open(name, io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate)};
	if (!file)
	{
		return false;
	}

	const bool written {file->write(uqstl::as_bytes(uqstl::span {text})) == text.size()};
	return written && file->flush();
}

void moveToBadFile(const io::Directory& dir, uqgsl::czstring name)
{
	const uqstl::string badName {fmt::format("{}.bad", name)};
	(void)dir.remove(badName);
	if (!dir.rename(name, dir, badName))
	{
		uqm::log::warn("Moved unreadable {} to {}; using defaults.", name, badName);
	}
	else
	{
		uqm::log::error("Could not move unreadable {} to {}.", name, badName);
	}
}

} // namespace uqm
