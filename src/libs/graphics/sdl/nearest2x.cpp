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

// Core algorithm of the Bilinear screen scaler
//  Template
//    When this file is built standalone is produces a plain C version

#include "libs/graphics/sdl/sdl_common.h"
#include <cstdint>
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"

// Nearest Neighbor scaling to 2x
// The name expands to
//		Scale_Nearest (for plain C)
//		[others when platforms are added]
void SCALE_(Nearest)(SDL_Surface* src, SDL_Surface* dst, SDL_Rect* r)
{
	int y;
	const int rw = r->w, rh = r->h;
	const int sp = src->pitch, dp = dst->pitch;
	const int bpp = dst->format->BytesPerPixel;
	const int slen = sp / bpp, dlen = dp / bpp;
	const int dsrc = slen - rw, ddst = (dlen - rw) * 2;

	Uint32* src_p = (Uint32*)src->pixels;
	Uint32* dst_p = (Uint32*)dst->pixels;

	// Nothing to scale
	if (rw == 0 || rh == 0)
	{
		return;
	}

	SCALE_(PlatInit)();

	// move ptrs to the first updated pixel
	src_p += slen * r->y + r->x;
	dst_p += (dlen * r->y + r->x) * 2;

	// Plain C version
	for (y = 0; y < rh; ++y)
	{
		int x;
		for (x = 0; x < rw; ++x, ++src_p, dst_p += 2)
		{
			Uint32 pix = *src_p;
			dst_p[0] = pix;
			dst_p[1] = pix;
			dst_p[dlen] = pix;
			dst_p[dlen + 1] = pix;
		}
		dst_p += ddst;
		src_p += dsrc;
	}

	SCALE_(PlatDone)();
}
