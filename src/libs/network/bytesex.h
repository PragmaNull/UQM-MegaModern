/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Converts values between the host byte order and the network byte order (big-endian).
#ifndef LIBS_NETWORK_BYTESEX_H_
#define LIBS_NETWORK_BYTESEX_H_

#include "core/stl/bit.h"

#include <cstdint>

[[nodiscard]] constexpr uint16_t swapBytes16(uint16_t x)
{
	return static_cast<uint16_t>((x << 8) | (x >> 8));
}

[[nodiscard]] constexpr uint32_t swapBytes32(uint32_t x)
{
	return (x << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | (x >> 24);
}

[[nodiscard]] constexpr uint16_t hton16(uint16_t x)
{
	return uqstl::endian::native == uqstl::endian::big ? x : swapBytes16(x);
}

[[nodiscard]] constexpr uint32_t hton32(uint32_t x)
{
	return uqstl::endian::native == uqstl::endian::big ? x : swapBytes32(x);
}

[[nodiscard]] constexpr uint16_t ntoh16(uint16_t x)
{
	return hton16(x);
}

[[nodiscard]] constexpr uint32_t ntoh32(uint32_t x)
{
	return hton32(x);
}

static_assert(hton16(0x1234) == (uqstl::endian::native == uqstl::endian::big ? 0x1234 : 0x3412));

#endif /* LIBS_NETWORK_BYTESEX_H_ */
