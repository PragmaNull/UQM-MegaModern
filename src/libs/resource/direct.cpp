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

#include "libs/strings/strintrn.h"
#include "libs/memlib.h"
#include "port.h"

DIRENTRY_REF
LoadDirEntryTable(const uqm::io::Directory& dir, const char* path, const uqm::io::Matcher& matcher)
{
	uqstl::error_code error {};
	const uqstl::optional<uqm::io::Directory> subdir {dir.subdir(path)};
	if (!subdir)
	{
		return ((DIRENTRY_REF)0);
	}
	uqstl::vector<uqstl::string> names {subdir->list("", matcher, error)};
	uqstl::erase_if(names, [&subdir](const uqstl::string& name) {
		if (name.starts_with('.'))
		{
			return true;
		}
		const uqstl::optional<uqm::io::FileInfo> info {subdir->stat(name)};
		return !info || info->isDirectory();
	});

	if (names.empty())
	{
		return ((DIRENTRY_REF)0);
	}

	STRING_TABLE StringTable {AllocStringTable(static_cast<int>(names.size()), 0)};
	STRING_TABLE_DESC* lpST {StringTable};
	if (lpST == 0)
	{
		FreeStringTable(StringTable);
		return ((DIRENTRY_REF)0);
	}
	lpST->size = static_cast<int>(names.size());

	int index {};
	for (const uqstl::string& name : names)
	{
		SetStringTableEntry(StringTable, index++, name.c_str(), static_cast<int>(name.size() + 1));
	}
	return StringTable;
}
