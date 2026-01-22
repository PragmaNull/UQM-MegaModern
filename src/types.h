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

/* Data types used in the UQM port
 * Draft 3
 * Compiled from various sources
 */
#pragma once
#ifndef TYPES_H_
#define TYPES_H_

#include "config.h"

#include <cinttypes>
#include <stddef.h>

//
//#	define UQM_INT16   short
//#	define UQM_INT32   int

//#endif

///* Figure out how to support 64-bit datatypes */
//#if !defined(UQM_INT64) && !defined(__STRICT_ANSI__)
//#	if defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_C) || defined(__ARMCC__)
//#		define UQM_INT64    long long
//#	elif defined(_MSC_VER) || defined(__BORLANDC__)
//#		define UQM_INT64    __int64
//#	endif
//#endif /* !__STRICT_ANSI__ */
//
//#ifdef _MSC_VER
//#	if (_MSC_VER >= 1800)
//#		include <stdbool.h>
//#	endif
//#endif

// ISO C99 compatible boolean types. The ISO C99 standard defines:
// - An object declared as type _Bool, large enough to store the values 0
//   and 1, the rank of which is less than the rank of all other standard
//   integer types.
// - A macro "bool", which expands to "_Bool".
// - A macro "true", which expands to the integer constant 1, suitable for
//   use in #if preprocessing directives.
// - A macro "false", which expands to the integer constant 0, suitable for
//   use in #if preprocessing directives.
// - A macro "__bool_true_false_are_defined", which expands to the integer
//   constant 1, suitable for use in #if preprocessing directives.
//#ifndef __bool_true_false_are_defined
//#undef bool
//#undef false
//#undef true
//#ifndef HAVE__BOOL
//typedef unsigned char _Bool;
//#endif  /* HAVE_BOOL */
//#define bool _Bool
//#define true 1
//#define false 0
//#define __bool_true_false_are_defined
//#endif  /* __bool_true_false_are_defined */

using uint8 = uint8_t;
using sint8 = int8_t;
using uint16 = uint16_t;
using sint16 = int16_t;
using uint32 = uint32_t;
using sint32 = int32_t;
using uint64 = uint64_t;
using sint64 = int64_t;


#undef SINT8_MIN
#undef SINT8_MAX
#undef SINT16_MIN
#undef SINT16_MAX
#undef SINT32_MIN
#undef SINT32_MAX
#undef SINT64_MIN
#undef SINT64_MAX

#define SINT8_MIN  std::numeric_limits<sint8>::min()
#define SINT8_MAX  std::numeric_limits<sint8>::max()
#define SINT16_MIN std::numeric_limits<sint16>::min()
#define SINT16_MAX std::numeric_limits<sint16>::max()
#define SINT32_MIN std::numeric_limits<sint32>::min()
#define SINT32_MAX std::numeric_limits<sint32>::max()
#define SINT64_MIN std::numeric_limits<sint64>::min()
#define SINT64_MAX std::numeric_limits<sint64>::max()

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

//// Lifted from SDL_config.h
//#if !defined(HAVE_STDINT_H) && !defined(_STDINT_H_)
///* Most everything except Visual Studio 2008 and earlier has stdint.h now */
//#	if defined(_MSC_VER) && (_MSC_VER < 1600)
//#		undef UINT8_MAX
//#		undef UINT16_MAX
//#		undef UINT32_MAX
//#		undef UINT64_MAX
//
//#		define UINT8_MAX  0xff /* 255U */
//#		define UINT16_MAX 0xffff /* 65535U */
//#		define UINT32_MAX 0xffffffff /* 4294967295U */
//#		define UINT64_MAX 0xffffffffffffffff /* 18446744073709551615U */
//#	else
//#		define HAVE_STDINT_H 1
//#		include <stdint.h>
//#	endif /* Visual Studio 2008 */
//#endif /* !_STDINT_H_ && !HAVE_STDINT_H */
//
//#if 0 //defined(__cplusplus)
//}
//#endif

#endif /* TYPES_H_ */
