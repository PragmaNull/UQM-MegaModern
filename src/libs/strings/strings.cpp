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
	auto* strtab {new STRING_TABLE_DESC {}};
	int i, multiplier = 1;

	if (flags & HAS_NAMEINDEX)
	{
		multiplier++;
	}
	if (flags & HAS_SOUND_CLIPS)
	{
		multiplier++;
	}
	if (flags & HAS_TIMESTAMP)
	{
		multiplier++;
	}
	strtab->flags = flags;
	strtab->size = num_entries;
	num_entries *= multiplier;
	strtab->strings = new STRING_TABLE_ENTRY_DESC[num_entries] {};
	for (i = 0; i < num_entries; i++)
	{
		strtab->strings[i].data = nullptr;
		strtab->strings[i].length = 0;
		strtab->strings[i].parent = strtab;
		strtab->strings[i].index = i;
	}
	strtab->nameIndex = nullptr;
	return strtab;
}

void FreeStringTable(STRING_TABLE strtab)
{
	int i, multiplier = 1;

	if (strtab == nullptr)
	{
		return;
	}

	if (strtab->flags & HAS_SOUND_CLIPS)
	{
		multiplier++;
	}
	if (strtab->flags & HAS_TIMESTAMP)
	{
		multiplier++;
	}

	for (i = 0; i < strtab->size * multiplier; i++)
	{
		delete[] strtab->strings[i].data;
	}

	delete strtab->nameIndex;
	strtab->nameIndex = nullptr;
	delete[] strtab->strings;
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
	if ((StringTable != 0) && (StringTable->size > 0))
	{
		return StringTable->strings;
	}

	return nullptr;
}

STRING_TABLE
ReleaseStringTable(STRING String)
{
	STRING_TABLE StringTable;

	StringTable = GetStringTable(String);

	return (StringTable);
}

STRING_TABLE
GetStringTable(STRING String)
{
	if (String && String->parent)
	{
		return String->parent;
	}
	return nullptr;
}

uint16_t
GetStringTableCount(STRING String)
{
	if (String && String->parent)
	{
		return String->parent->size;
	}
	return 0;
}

uint16_t
GetStringTableIndex(STRING String)
{
	if (String)
	{
		return String->index;
	}
	return 0;
}

STRING
SetAbsStringTableIndex(STRING String, uint16_t StringTableIndex)
{
	STRING_TABLE StringTablePtr;

	if (!String)
	{
		return nullptr;
	}

	StringTablePtr = String->parent;

	if (StringTablePtr == nullptr)
	{
		String = nullptr;
	}
	else
	{
		StringTableIndex = StringTableIndex % StringTablePtr->size;
		String = &StringTablePtr->strings[StringTableIndex];
	}

	return (String);
}

STRING
SetRelStringTableIndex(STRING String, int16_t StringTableOffs)
{
	STRING_TABLE StringTablePtr;

	if (!String)
	{
		return nullptr;
	}

	StringTablePtr = String->parent;

	if (StringTablePtr == nullptr)
	{
		String = nullptr;
	}
	else
	{
		uint16_t StringTableIndex;

		while (StringTableOffs < 0)
		{
			StringTableOffs += StringTablePtr->size;
		}
		StringTableIndex = (String->index + StringTableOffs)
						 % StringTablePtr->size;

		String = &StringTablePtr->strings[StringTableIndex];
	}

	return (String);
}

uint16_t
GetStringLength(STRING String)
{
	if (String == nullptr)
	{
		return 0;
	}
	return (uint16_t)uqm::utf8StringCount(uqstl::string_view {String->data, static_cast<size_t>(String->length)});
}

uint16_t
GetStringLengthBin(STRING String)
{
	if (String == nullptr)
	{
		return 0;
	}
	return String->length;
}

STRINGPTR
GetStringName(STRING String)
{
	STRING_TABLE StringTablePtr;
	uint16_t StringIndex;

	if (String == nullptr)
	{
		return nullptr;
	}

	StringTablePtr = String->parent;
	if (StringTablePtr == nullptr)
	{
		return nullptr;
	}

	StringIndex = String->index;

	if (!(StringTablePtr->flags & HAS_NAMEINDEX))
	{
		return nullptr;
	}
	StringIndex += StringTablePtr->size;

	String = &StringTablePtr->strings[StringIndex];
	if (String->length == 0)
	{
		return nullptr;
	}

	return String->data;
}

STRINGPTR
GetStringSoundClip(STRING String)
{
	STRING_TABLE StringTablePtr;
	uint16_t StringIndex;

	if (String == nullptr)
	{
		return nullptr;
	}

	StringTablePtr = String->parent;
	if (StringTablePtr == nullptr)
	{
		return nullptr;
	}

	StringIndex = String->index;
	if (!(StringTablePtr->flags & HAS_SOUND_CLIPS))
	{
		return nullptr;
	}
	StringIndex += StringTablePtr->size;

	if (StringTablePtr->flags & HAS_NAMEINDEX)
	{
		StringIndex += StringTablePtr->size;
	}

	String = &StringTablePtr->strings[StringIndex];
	if (String->length == 0)
	{
		return nullptr;
	}

	return String->data;
}

STRINGPTR
GetStringTimeStamp(STRING String)
{
	STRING_TABLE StringTablePtr;
	uint16_t StringIndex;

	if (String == nullptr)
	{
		return nullptr;
	}

	StringTablePtr = String->parent;
	if (StringTablePtr == nullptr)
	{
		return nullptr;
	}

	StringIndex = String->index;
	if (!(StringTablePtr->flags & HAS_TIMESTAMP))
	{
		return nullptr;
	}
	StringIndex += StringTablePtr->size;

	if (StringTablePtr->flags & HAS_NAMEINDEX)
	{
		StringIndex += StringTablePtr->size;
	}

	if (StringTablePtr->flags & HAS_SOUND_CLIPS)
	{
		StringIndex += StringTablePtr->size;
	}

	String = &StringTablePtr->strings[StringIndex];
	if (String->length == 0)
	{
		return nullptr;
	}

	return String->data;
}

STRINGPTR
GetStringAddress(STRING String)
{
	if (String == nullptr)
	{
		return nullptr;
	}
	return String->data;
}

STRING
GetStringByName(STRING_TABLE StringTable, const char* index)
{
	if (StringTable->nameIndex == nullptr)
	{
		return nullptr;
	}
	auto it = StringTable->nameIndex->find(index);
	return it != StringTable->nameIndex->end() ? it->second : nullptr;
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
