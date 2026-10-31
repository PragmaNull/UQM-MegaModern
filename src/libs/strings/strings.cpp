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

#include "strintrn.h"

STRING_TABLE
AllocStringTable(int num_entries, int flags)
{
	return new uqm::StringTable {static_cast<size_t>(num_entries), flags};
}

void FreeStringTable(STRING_TABLE strtab)
{
	delete strtab;
}

bool DestroyStringTable(STRING_TABLE StringTable)
{
	FreeStringTable(StringTable);
	return true;
}

STRING
CaptureStringTable(STRING_TABLE StringTable)
{
	return StringTable != nullptr && StringTable->size() > 0 ? StringTable->entry(0) : nullptr;
}

STRING_TABLE
ReleaseStringTable(STRING String)
{
	return GetStringTable(String);
}

STRING_TABLE
GetStringTable(STRING String)
{
	return String != nullptr ? String->parent : nullptr;
}

uint16_t
GetStringTableCount(STRING String)
{
	return String != nullptr && String->parent != nullptr ? static_cast<uint16_t>(String->parent->size()) : 0;
}

uint16_t
GetStringTableIndex(STRING String)
{
	return String != nullptr ? static_cast<uint16_t>(String->index) : 0;
}

STRING
SetAbsStringTableIndex(STRING String, uint16_t StringTableIndex)
{
	if (String == nullptr || String->parent == nullptr)
	{
		return nullptr;
	}
	return String->parent->entry(StringTableIndex % String->parent->size());
}

STRING
SetRelStringTableIndex(STRING String, int16_t StringTableOffs)
{
	if (String == nullptr || String->parent == nullptr)
	{
		return nullptr;
	}
	const auto size {static_cast<int>(String->parent->size())};
	int index {(String->index + StringTableOffs) % size};
	if (index < 0)
	{
		index += size;
	}
	return String->parent->entry(static_cast<size_t>(index));
}

uint16_t
GetStringLength(STRING String)
{
	return String != nullptr ? static_cast<uint16_t>(uqm::utf8StringCount(String->bytes)) : 0;
}

uint16_t
GetStringLengthBin(STRING String)
{
	return String != nullptr ? static_cast<uint16_t>(String->bytes.size()) : 0;
}

const char*
GetStringName(STRING String)
{
	return String != nullptr && String->parent != nullptr ? String->parent->name(String->index) : nullptr;
}

const char*
GetStringSoundClip(STRING String)
{
	return String != nullptr && String->parent != nullptr ? String->parent->soundClip(String->index) : nullptr;
}

const char*
GetStringTimeStamp(STRING String)
{
	return String != nullptr && String->parent != nullptr ? String->parent->timestamp(String->index) : nullptr;
}

STRINGPTR
GetStringAddress(STRING String)
{
	return String != nullptr && !String->bytes.empty() ? String->bytes.data() : nullptr;
}

STRING
GetStringByName(STRING_TABLE StringTable, const char* index)
{
	const auto found {StringTable->find(index)};
	return found ? StringTable->entry(*found) : nullptr;
}

bool CheckResString(RESOURCE res)
{
	bool check = false;
	STRING string = CaptureStringTable(LoadStringTableInstance(res));

	if (string != nullptr)
	{
		check = true;
	}

	DestroyStringTable(ReleaseStringTable(string));
	string = 0;

	return check;
}
