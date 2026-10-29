//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "port.h"
#include "resintrn.h"
#include "core/log/log.h"

uqstl::optional<uqm::io::File>
res_OpenResFile(const uqm::io::Directory& dir, const char* filename, uqm::io::OpenMode mode)
{
	uqstl::error_code error {};
	return dir.open(filename, mode, error);
}

uqgsl::owner<uqm::io::File*> res_NewResFile(const uqm::io::Directory& dir, const char* filename)
{
	uqstl::error_code error {};
	uqstl::optional<uqm::io::File> file {dir.open(filename, uqm::io::OpenMode::Read, error)};
	if (!file)
	{
		errno = error.default_error_condition().value();
		return nullptr;
	}
	return new uqm::io::File {uqstl::move(*file)};
}

void* res_LoadResFile(const uqm::io::Directory& dir, const char* filename, ResourceLoadFileFun* loadFun)
{
	uqstl::error_code error {};
	uqstl::optional<uqm::io::File> file {dir.open(filename, uqm::io::OpenMode::Read, error)};
	uint32_t length {1};
	if (file)
	{
		length = static_cast<uint32_t>(file->size());
		if (length == 0)
		{
			uqm::log::warn("Warning: Trying to load empty file '{}'.", filename);
			return nullptr;
		}
	}
	else if (error != uqstl::errc::is_a_directory)
	{
		return nullptr;
	}
	uqm::log::debug("\t'{}' -- {} bytes", filename, length);

	_cur_resfile_name = filename;
	void* const data {loadFun(file ? &*file : nullptr, length)};
	_cur_resfile_name = nullptr;
	return data;
}

bool DeleteResFile(const uqm::io::Directory& dir, const char* filename)
{
	return !dir.remove(filename);
}

size_t
ReadResFile(void* lpBuf, size_t size, size_t count, uqm::io::File& fp)
{
	if (size == 0 || count == 0)
	{
		return 0;
	}
	return fp.read({static_cast<uqstl::byte*>(lpBuf), size * count}) / size;
}

size_t
WriteResFile(const void* lpBuf, size_t size, size_t count, uqm::io::File& fp)
{
	if (size == 0 || count == 0)
	{
		return 0;
	}
	return fp.write({static_cast<const uqstl::byte*>(lpBuf), size * count}) / size;
}

int GetResFileChar(uqm::io::File& fp)
{
	return fp.getChar();
}

int PutResFileChar(char ch, uqm::io::File& fp)
{
	const uqstl::byte value {static_cast<uqstl::byte>(ch)};
	return fp.write({&value, 1}) == 1 ? static_cast<unsigned char>(ch) : EOF;
}

int PutResFileNewline(uqm::io::File& fp)
{
	int retval;

#ifdef WIN32
	PutResFileChar('\r', fp);
#endif
	retval = PutResFileChar('\n', fp);
	return (retval);
}

long SeekResFile(uqm::io::File& fp, long offset, int whence)
{
	uqm::io::SeekFrom from {uqm::io::SeekFrom::Start};
	if (whence == SEEK_CUR)
	{
		from = uqm::io::SeekFrom::Current;
	}
	else if (whence == SEEK_END)
	{
		from = uqm::io::SeekFrom::End;
	}
	return fp.seek(offset, from) ? 0 : -1;
}

long TellResFile(uqm::io::File& fp)
{
	return static_cast<long>(fp.tell());
}

size_t
LengthResFile(uqm::io::File& fp)
{
	return static_cast<size_t>(fp.size());
}
