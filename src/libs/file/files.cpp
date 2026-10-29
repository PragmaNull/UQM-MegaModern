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

// Contains code handling files

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "port.h"
#include "libs/uio.h"
#include "config.h"
#include <cstdint>
#include "filintrn.h"
#include "libs/memlib.h"
#include "core/log/log.h"

static int copyError(uio_Handle* srcHandle, uio_Handle* dstHandle,
					 uio_DirHandle* unlinkHandle, const char* unlinkPath, uint8_t* buf);

bool fileExists(const char* name)
{
	return access(name, F_OK) == 0;
}

bool fileExists2(uio_DirHandle* dir, const char* fileName)
{
	uio_Stream* stream;

	stream = uio_fopen(dir, fileName, "rb");
	if (stream == nullptr)
	{
		return 0;
	}

	uio_fclose(stream);
	return 1;
}

/*
 * Copy a file with path srcName to a file with name newName.
 * If the destination already exists, the operation fails.
 * Links are followed.
 * Special files (fifos, char devices, block devices, etc) will be
 * read as long as there is data available and the destination will be
 * a regular file with that data.
 * The new file will have the same permissions as the old.
 * If an error occurs during copying, an attempt will be made to
 * remove the copy.
 */
int copyFile(uio_DirHandle* srcDir, const char* srcName,
			 uio_DirHandle* dstDir, const char* newName)
{
	uio_Handle *src, *dst;
	struct stat sb;
#define BUFSIZE 65536
	uint8_t *buf, *bufPtr;
	ssize_t numInBuf, numWritten;

	src = uio_open(srcDir, srcName, O_RDONLY
#ifdef WIN32
										| O_BINARY
#endif
				   ,
				   0);
	if (src == nullptr)
	{
		return -1;
	}

	if (uio_fstat(src, &sb) == -1)
	{
		return copyError(src, nullptr, nullptr, nullptr, nullptr);
	}

	dst = uio_open(dstDir, newName, O_WRONLY | O_CREAT | O_EXCL
#ifdef WIN32
										| O_BINARY
#endif
				   ,
				   sb.st_mode);
	if (dst == nullptr)
	{
		return copyError(src, nullptr, nullptr, nullptr, nullptr);
	}

	buf = (uint8_t*)HMalloc(BUFSIZE);
	// This was originally a statically allocated buffer,
	// but as this function might be run from a thread with
	// a small stack, this is better.
	while (1)
	{
		numInBuf = uio_read(src, buf, BUFSIZE);
		if (numInBuf == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			return copyError(src, dst, dstDir, newName, buf);
		}
		if (numInBuf == 0)
		{
			break;
		}

		bufPtr = buf;
		do
		{
			numWritten = uio_write(dst, bufPtr, numInBuf);
			if (numWritten == -1)
			{
				if (errno == EINTR)
				{
					continue;
				}
				return copyError(src, dst, dstDir, newName, buf);
			}
			numInBuf -= numWritten;
			bufPtr += numWritten;
		} while (numInBuf > 0);
	}

	HFree(buf);
	uio_close(src);
	uio_close(dst);
	errno = 0;
	return 0;
}

/*
 * Closes srcHandle if it's not -1.
 * Closes dstHandle if it's not -1.
 * Removes unlinkpath from the unlinkHandle dir if it's not nullptr.
 * Frees 'buf' if not nullptr.
 * Always returns -1.
 * errno is what was before the call.
 */
static int
copyError(uio_Handle* srcHandle, uio_Handle* dstHandle,
		  uio_DirHandle* unlinkHandle, const char* unlinkPath, uint8_t* buf)
{
	int savedErrno;

	savedErrno = errno;

	uqm::log::debug("Error while copying: {}", uqm::strerror(errno));

	if (srcHandle != nullptr)
	{
		uio_close(srcHandle);
	}

	if (dstHandle != nullptr)
	{
		uio_close(dstHandle);
	}

	if (unlinkPath != nullptr)
	{
		uio_unlink(unlinkHandle, unlinkPath);
	}

	if (buf != nullptr)
	{
		HFree(buf);
	}

	errno = savedErrno;
	return -1;
}
