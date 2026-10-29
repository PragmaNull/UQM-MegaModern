// Reads and writes whole text files in a directory.
#include "textfile.h"

#include "core/log/log.h"

#include <fmt/format.h>

namespace uqm
{

uqstl::optional<uqstl::string> readTextFile(const io::Directory& dir, uqgsl::czstring name)
{
	uqstl::error_code error {};
	uqstl::optional<io::File> file {dir.open(name, io::OpenMode::Read, error)};
	if (!file)
	{
		return uqstl::nullopt;
	}

	uqstl::string text(static_cast<size_t>(file->size()), '\0');
	text.resize(file->read(uqstl::as_writable_bytes(uqstl::span {text})));
	if (file->error())
	{
		return uqstl::nullopt;
	}
	return text;
}

bool writeTextFile(const io::Directory& dir, uqgsl::czstring name, uqstl::string_view text)
{
	uqstl::error_code error {};
	uqstl::optional<io::File> file {dir.open(name, io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate, error)};
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
