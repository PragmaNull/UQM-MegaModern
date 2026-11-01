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

#ifndef LIBS_STRLIB_H_
#define LIBS_STRLIB_H_

#include "core/misc/bittricks.h"
#include "port.h"
#include "libs/io/FileSystem.h"
#include "core/string/Utf8.h"

#include <stddef.h>

#include "libs/strings/StringTable.h"


/* This has to go here because reslib requires the above typedefs. */
#include "libs/reslib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

extern bool InstallStringTableResType(void);
extern const char* LoadStringInstance(RESOURCE res);

namespace uqm
{

// Returns an empty pointer if the resource is not a string table.
[[nodiscard]] StringTablePtr loadStringTable(RESOURCE res);

} // namespace uqm

#define UNICHAR_TAB 0x0009
#define STR_TAB "\x09"
#define UNICHAR_SPACE 0x0020
#define STR_SPACE "\x20"
#define UNICHAR_DEGREE_SIGN 0x00b0
#define STR_DEGREE_SIGN "\xC2\xB0"
#define UNICHAR_INFINITY_SIGN 0x221e
#define STR_INFINITY_SIGN "\xE2\x88\x9E"
#define UNICHAR_EARTH_SIGN 0x2641
#define STR_EARTH_SIGN "\xE2\x99\x81"
#define UNICHAR_MIDDLE_DOT 0x00b7
#define STR_MIDDLE_DOT "\xC2\xB7"
#define UNICHAR_BULLET 0x2022
#define STR_BULLET "\xE2\x80\xA2"
#define UNICHAR_CROSS 0x2573
#define STR_CROSS "\xE2\x95\xB3"
#define UNICHAR_SQUARE 0x25A1
#define STR_SQUARE "\xE2\x96\xA1"
#define UNICHAR_TRIANGLE 0x25B3
#define STR_TRIANGLE "\xE2\x96\xB3"
#define UNICHAR_CIRCLE 0x25EF
#define STR_CIRCLE "\xE2\x97\xAf"
#define UNICHAR_PIPE 0x007C
#define STR_PIPE "\x7C"
#define UNICHAR_AMP 0x0026
#define STR_AMP "\x26"
#define UNICHAR_COLON 0x003A
#define STR_COLON "\x3A"


#define STR_NULL "null"

// Skip the alignment marker "|n|" or the padding marker ":n:" at the start of 'str',
// and add n (scaled to the resolution) to the offset.
const char* AlignText(const char* str, int16_t* loc_x);
const char* AddPadd(const char* str, int16_t* padding);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* LIBS_STRLIB_H_ */
