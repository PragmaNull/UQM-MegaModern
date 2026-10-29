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

#include "resintrn.h"
#include "libs/memlib.h"
#include "core/log/log.h"


void* GetResourceData(uqm::io::File& fp, uint32_t length)
{
	void* result;
	uint32_t compLen;

	// Resource data used to be prefixed by its length in package files.
	// A valid length prefix indicated compressed data, and
	// a length prefix ~0 meant uncompressed.
	// Currently, .ct and .xlt files still carry a ~0 length prefix.
	if (ReadResFile(&compLen, sizeof(compLen), 1, fp) != 1)
	{
		return nullptr;
	}
	if (compLen != ~(uint32_t)0)
	{
		uqm::log::warn("LZ-compressed binary data not supported");
		return nullptr;
	}
	length -= sizeof(uint32_t);

	result = AllocResourceData(length);
	if (!result)
	{
		return nullptr;
	}

	if (ReadResFile(result, 1, length, fp) != length)
	{
		FreeResourceData(result);
		result = nullptr;
	}

	return result;
}
