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

#include "core/stl/memory.h"

namespace
{

// The data of an entry, or nullptr if the entry is empty.
STRINGPTR entryData(STRING String)
{
	return String->bytes.empty() ? nullptr : String->bytes.data();
}

// The data of the entry in the block 'block' (0 for the text) at the index of 'String', or nullptr.
STRINGPTR blockData(STRING String, int block)
{
	return entryData(&String->parent->strings[String->index + block * String->parent->size]);
}

} // namespace

STRING_TABLE
AllocStringTable(int num_entries, int flags)
{
	auto strtab {uqstl::make_unique<STRING_TABLE_DESC>()};
	int multiplier {1};
	for (const int flag : {HAS_NAMEINDEX, HAS_SOUND_CLIPS, HAS_TIMESTAMP})
	{
		multiplier += (flags & flag) != 0 ? 1 : 0;
	}
	strtab->flags = static_cast<unsigned short>(flags);
	strtab->size = num_entries;
	strtab->strings.resize(static_cast<size_t>(num_entries * multiplier));
	for (int i {0}; i < num_entries * multiplier; ++i)
	{
		strtab->strings[i].parent = strtab.get();
		strtab->strings[i].index = i;
	}
	return strtab.release();
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
	if ((StringTable != 0) && (StringTable->size > 0))
	{
		return StringTable->strings.data();
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
	return (uint16_t)uqm::utf8StringCount(String->bytes);
}

uint16_t
GetStringLengthBin(STRING String)
{
	if (String == nullptr)
	{
		return 0;
	}
	return static_cast<uint16_t>(String->bytes.size());
}

STRINGPTR
GetStringName(STRING String)
{
	if (String == nullptr || String->parent == nullptr || !(String->parent->flags & HAS_NAMEINDEX))
	{
		return nullptr;
	}
	return blockData(String, 1);
}

STRINGPTR
GetStringSoundClip(STRING String)
{
	if (String == nullptr || String->parent == nullptr || !(String->parent->flags & HAS_SOUND_CLIPS))
	{
		return nullptr;
	}
	return blockData(String, (String->parent->flags & HAS_NAMEINDEX) ? 2 : 1);
}

STRINGPTR
GetStringTimeStamp(STRING String)
{
	if (String == nullptr || String->parent == nullptr || !(String->parent->flags & HAS_TIMESTAMP))
	{
		return nullptr;
	}
	const int block {1 + ((String->parent->flags & HAS_NAMEINDEX) ? 1 : 0) + ((String->parent->flags & HAS_SOUND_CLIPS) ? 1 : 0)};
	return blockData(String, block);
}

STRINGPTR
GetStringAddress(STRING String)
{
	if (String == nullptr)
	{
		return nullptr;
	}
	return entryData(String);
}

STRING
GetStringByName(STRING_TABLE StringTable, const char* index)
{
	const auto it {StringTable->nameIndex.find(index)};
	return it != StringTable->nameIndex.end() ? it->second : nullptr;
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
