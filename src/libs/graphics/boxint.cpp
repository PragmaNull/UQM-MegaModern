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

#include "gfxintrn.h"

INTERSECT_CODE
BoxIntersect(GFXRECT* pr1, GFXRECT* pr2, GFXRECT* pinter)
{
	int16_t x1 {};
	int16_t delta {};

	INTERSECT_CODE intersect_code {IntersectNoclip};

	x1 = pr1->corner.x - pr2->corner.x;

	int16_t w1 {pr1->extent.width};
	int16_t w2 {pr2->extent.width};
	if ((delta = w2 - x1) <= w1)
	{
		if (delta != w1)
		{
			w1 = delta;
			intersect_code &= ~IntersectNoclip;
		}
		intersect_code |= IntersectRight;
	}
	if (x1 <= 0)
	{
		if (x1 < 0)
		{
			w1 += x1;
			x1 = 0;
			intersect_code &= ~IntersectNoclip;
		}
		intersect_code |= IntersectLeft;
	}

	if (w1 > 0)
	{
#define h2 w2
		int16_t y1 {};

		y1 = pr1->corner.y - pr2->corner.y;

		int16_t h1 {pr1->extent.height};
		h2 = pr2->extent.height;
		if ((delta = h2 - y1) <= h1)
		{
			if (delta != h1)
			{
				h1 = delta;
				intersect_code &= ~IntersectNoclip;
			}
			intersect_code |= IntersectBottom;
		}
		if (y1 <= 0)
		{
			if (y1 < 0)
			{
				h1 += y1;
				y1 = 0;
				intersect_code &= ~IntersectNoclip;
			}
			intersect_code |= IntersectTop;
		}

		if (h1 > 0)
		{
			pinter->corner.x = x1 + pr2->corner.x;
			pinter->corner.y = y1 + pr2->corner.y;
			pinter->extent.width = w1;
			pinter->extent.height = h1;

			return (intersect_code);
		}
#undef h2
	}

	return ((INTERSECT_CODE)0);
}

void BoxUnion(GFXRECT* pr1, GFXRECT* pr2, GFXRECT* punion)
{
#if NEVER // Part of lower FIXME.
	int16_t x2, y2, w2, h2;
#endif // NEVER

	// Union is A AND B, put together, correct?  Returns a bigger box that
	// encompasses the two.
	punion->corner.x = uqstl::min(pr1->corner.x, pr2->corner.x);
	punion->corner.y = uqstl::min(pr1->corner.y, pr2->corner.y);

	punion->extent.width = uqstl::max(pr1->corner.x + pr1->extent.width,
									  pr2->corner.x + pr2->extent.width)
						 - punion->corner.x;
	punion->extent.height = uqstl::max(pr1->corner.y + pr1->extent.height,
									   pr2->corner.y + pr2->extent.height)
						  - punion->corner.y;


#if NEVER // FIXME - I think this is broken, but keeping it around for reference
		  // FIXME - just in case.

#if 1 /* alter based on 0 widths */

	x2 =
		(pr1->corner.x < pr2->corner.x) ? pr1->corner.x : pr2->corner.x;

	y2 =
		(pr1->corner.y < pr2->corner.y) ? pr1->corner.y : pr2->corner.y;

	w2 = (((pr1->corner.x + pr1->extent.width) > (pr2->corner.x + pr2->extent.width)) ?
			  (pr1->corner.x + pr1->extent.width) :
			  (pr2->corner.x + pr2->extent.width))
	   - punion->corner.x;

	h2 = (((pr1->corner.y + pr1->extent.height) > (pr2->corner.y + pr2->extent.height)) ?
			  (pr1->corner.y + pr1->extent.height) :
			  (pr2->corner.y + pr2->extent.height))
	   - punion->corner.y;
#else
	int16_t delta;
	int16_t x1, y1, w1, h1;

	x1 = pr1->corner.x;
	w1 = pr1->extent.width;
	x2 = pr2->corner.x;
	w2 = pr2->extent.width;
	if ((delta = x1 - x2) >= 0)
	{
		w1 += delta;
	}
	else
	{
		w2 -= delta;
		x2 += delta;
	}

	y1 = pr1->corner.y;
	h1 = pr1->extent.height;
	y2 = pr2->corner.y;
	h2 = pr2->extent.height;
	if ((delta = y1 - y2) >= 0)
	{
		h1 += delta;
	}
	else
	{
		h2 -= delta;
		y2 += delta;
	}

	if ((delta = w1 - w2) > 0)
	{
		w2 += delta;
	}
	if ((delta = h1 - h2) > 0)
	{
		h2 += delta;
	}
#endif

	punion->corner.x = x2;
	punion->corner.y = y2;
	punion->extent.width = w2;
	punion->extent.height = h2;

#endif // NEVER
}
