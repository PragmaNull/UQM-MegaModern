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

#pragma once
#ifndef UQM_LIBS_STRLIB_H
#define UQM_LIBS_STRLIB_H

#include "core/misc/bittricks.h"
#include "port.h"
#include "libs/io/FileSystem.h"
#include "core/string/Utf8.h"

#include <stddef.h>

#include "libs/strings/StringTable.h"


/* This has to go here because reslib requires the above typedefs. */
#include "libs/reslib.h"

extern bool InstallStringTableResType(void);
const extern char* LoadStringInstance(RESOURCE res);

namespace uqm
{
// Returns an empty pointer if the resource is not a string table.
[[nodiscard]] StringTablePtr loadStringTable(RESOURCE res);

} // namespace uqm

inline constexpr auto UnicharTab {0x0009};
inline constexpr uqgsl::czstring StrTab {"\x09"};
inline constexpr auto UnicharSpace {0x0020};
inline constexpr uqgsl::czstring StrSpace {"\x20"};
inline constexpr auto UnicharDegreeSign {0x00b0};
inline constexpr uqgsl::czstring StrDegreeSign {"\xC2\xB0"};
inline constexpr auto UnicharInfinitySign {0x221e};
inline constexpr uqgsl::czstring StrInfinitySign {"\xE2\x88\x9E"};
inline constexpr auto UnicharEarthSign {0x2641};
inline constexpr uqgsl::czstring StrEarthSign {"\xE2\x99\x81"};
inline constexpr auto UnicharMiddleDot {0x00b7};
inline constexpr uqgsl::czstring StrMiddleDot {"\xC2\xB7"};
inline constexpr auto UnicharBullet {0x2022};
inline constexpr uqgsl::czstring StrBullet {"\xE2\x80\xA2"};
inline constexpr auto UnicharCross {0x2573};
inline constexpr uqgsl::czstring StrCross {"\xE2\x95\xB3"};
inline constexpr auto UnicharSquare {0x25A1};
inline constexpr uqgsl::czstring StrSquare {"\xE2\x96\xA1"};
inline constexpr auto UnicharTriangle {0x25B3};
inline constexpr uqgsl::czstring StrTriangle {"\xE2\x96\xB3"};
inline constexpr auto UnicharCircle {0x25EF};
inline constexpr uqgsl::czstring StrCircle {"\xE2\x97\xAf"};
inline constexpr auto UnicharPipe {0x007C};
inline constexpr uqgsl::czstring StrPipe {"\x7C"};
inline constexpr auto UnicharAmp {0x0026};
inline constexpr uqgsl::czstring StrAmp {"\x26"};
inline constexpr auto UnicharColon {0x003A};
inline constexpr uqgsl::czstring StrColon {"\x3A"};


inline constexpr uqgsl::czstring StrNull {"null"};

// Skip the alignment marker "|n|" or the padding marker ":n:" at the start of 'str',
// and add n (scaled to the resolution) to the offset.
const char* AlignText(const char* str, int16_t* loc_x);
const char* AddPadd(const char* str, int16_t* padding);

#endif // UQM_LIBS_STRLIB_H
