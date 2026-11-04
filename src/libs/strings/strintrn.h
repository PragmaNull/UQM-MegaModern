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

#ifndef LIBS_STRINGS_STRINTRN_H_
#define LIBS_STRINGS_STRINTRN_H_

#include <stdio.h>
#include "core/stl/map.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "libs/strlib.h"
#include "libs/reslib.h"

// Copies 'len' bytes of 'value' into the entry. A length of 0 empties the entry.
void SetStringTableEntry(uqm::StringTable* strtab, int index, const void* value, int len);

void* _GetStringData(const ResourceFile& resource);
void* _GetBinaryTableData(const ResourceFile& resource);
void _GetConversationData(const ResourceArg& arg, RESOURCE_DATA* resdata);

#endif /* LIBS_STRINGS_STRINTRN_H_ */
