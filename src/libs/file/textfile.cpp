// Reads and writes whole text files in a uio directory.
#include "textfile.h"

#include "core/log/log.h"
#include "libs/reslib.h"

#include <fmt/format.h>

namespace uqm
{

uqstl::optional<uqstl::string> readTextFile(uio_DirHandle* dir, uqgsl::czstring name)
{
	uio_Stream* stream {uio_fopen(dir, name, "rb")};
	if (stream == nullptr)
	{
		return uqstl::nullopt;
	}

	uqstl::string text {};
	char buffer[4096] {};
	size_t count {};
	while ((count = uio_fread(buffer, 1, sizeof buffer, stream)) > 0)
	{
		text.append(buffer, count);
	}
	const bool failed {uio_ferror(stream) != 0};
	uio_fclose(stream);

	if (failed)
	{
		return uqstl::nullopt;
	}
	return text;
}

bool writeTextFile(uio_DirHandle* dir, uqgsl::czstring name, uqstl::string_view text)
{
	uio_Stream* stream {res_OpenResFile(dir, name, "wb")};
	if (stream == nullptr)
	{
		return false;
	}

	const bool written {WriteResFile(text.data(), 1, text.size(), stream) == text.size()};
	const bool closed {res_CloseResFile(stream)};
	return written && closed;
}

void moveToBadFile(uio_DirHandle* dir, uqgsl::czstring name)
{
	const uqstl::string badName {fmt::format("{}.bad", name)};
	uio_unlink(dir, badName.c_str());
	if (uio_rename(dir, name, dir, badName.c_str()) == 0)
	{
		uqm::log::warn("Moved unreadable {} to {}; using defaults.", name, badName);
	}
	else
	{
		uqm::log::error("Could not move unreadable {} to {}.", name, badName);
	}
}

} // namespace uqm
