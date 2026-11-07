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
#ifndef UQM_LIBS_MATHLIB_H
#define UQM_LIBS_MATHLIB_H

#include "core/misc/bittricks.h"

#include "core/stl/string.h"

#include <cmath>
#include <zlib.h>

#include "math/Random.h"

// The integer square root of 'value', rounded down.
inline uint16_t square_root(uint32_t value)
{
	return static_cast<uint16_t>(std::sqrt(static_cast<double>(value)));
}
// The CRC-32 of 'str'.
inline uint32_t crc32b(uqstl::string_view str)
{
	return static_cast<uint32_t>(crc32(0, reinterpret_cast<const Bytef*>(str.data()), static_cast<uInt>(str.size())));
}

inline static float scaleThing(float original, float thingToScale)
{
	return (original / thingToScale);
}


#endif // UQM_LIBS_MATHLIB_H
