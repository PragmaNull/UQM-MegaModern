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

#include "../units.h"
#include "libs/gfxlib.h"
#include "libs/graphics/context.h"
#include "libs/graphics/drawable.h"
#include "../nameref.h"
#include "../igfxres.h"
#include "../init.h"
#include <math.h>

#include "planets.h"
#include "uqm/setup.h"

static constexpr auto NumQuads {4};
static constexpr auto OffscreenPrim {NUM_PRIMS};

static constexpr auto FirstQuad {(1 << 0)};
static constexpr auto SecondQuad {(1 << 1)};
static constexpr auto ThirdQuad {(1 << 2)};
static constexpr auto FourthQuad {(1 << 3)};
static constexpr auto FullVisibility {(1 << 4)};
static constexpr auto AllQuad {(FirstQuad | SecondQuad | ThirdQuad | FourthQuad)};

static bool PointOnScreen(int32_t x, int32_t y)
{
	int32_t extend = 0;

	if (IsHd())
	{
		extend = 5;
	}

	return ((x + extend) >= 0 && x <= SisScreenWidth()
			&& (y + extend) >= 0 && y <= SisScreenHeight());
}

static bool DPointOnScreen(LargePoint* p)
{
	return (p->x >= 0 && p->x <= SisScreenWidth()
			&& p->y >= 0 && p->y <= SisScreenHeight());
}

static void TruncateDPoint(LargePoint* p)
{
	if (p->x > 32767)
	{
		p->x = SisScreenWidth();
	}
	if (p->x < -32768)
	{
		p->x = 0;
	}
	if (p->y > 32767)
	{
		p->y = SisScreenHeight();
	}
	if (p->y < -32768)
	{
		p->y = 0;
	}
}

// Kruzen: This function checks how much of an oval is on screen (by me).
// Check occurs in several steps of increasing complexity to sort off easy cases early.
// Step 1: Check of the rectangle diagonal (top left and bottom right corners) -> if both are on screen, then the entire oval is on screen;
// Step 2: Check of the inner rectangle diagonal (top left and bottom right corners are centers of II and IV quads). If both are offscreen,
// then the oval is offscreen too 100%;
// Step 3: Divide main rectangle into 4 quadrants and perform the next steps for every quadrant;
// Step 4: Check if the quad is off screen entirely (by checking if max possible X coord is less than 0 and so on);
// Step 5: Check if atleast one of curve edges of corresponding quad is on screen;
// Step 6: Check the opposite corners of the screen. Corners should form a diagonal that potentially can cross the curve. Run their coordinates
// through ellipse equation. If radius > 1 - point is outside of the ellipse, and if < 1 - inside. In case if one point is inside and the other is
// outside - then the diagonal crosses the curve somewhere on the screen, therefore the curve is visible. Otherwise, if both points are
// inside/outside, then the curve is not on screen.
static uint8_t CheckOvalCollision(LargePoint* p0, LargePoint* p1)
{
	LargePoint mp; // Middle point
	uint8_t quad_visible = 0;
	int32_t y1 {}; // Coords of top left and bottom right corners of inner rect
	double asquared {NAN};
	double bsquared {NAN};
	double a {NAN};
	double b {NAN};
	double x {NAN};
	double y {NAN};
	double r0 {NAN};
	double r1 {NAN};

	// Step 1
	if (DPointOnScreen(p0) && DPointOnScreen(p1))
	{
		quad_visible = AllQuad | FullVisibility;
		return quad_visible;
	}

	const int32_t x0 {p0->x + ((p1->x - p0->x) >> 2)};
	const int32_t x1 {x0 + ((x0 - p0->x) << 1)};

	const int32_t y0 {p0->y + ((p1->y - p0->y) >> 2)};
	y1 = y0 + ((y0 - p0->y) << 1);

	// Step 2
	if (x0 < 0 && y0 < 0 && x1 > SisScreenWidth() && y1 > SisScreenHeight())
	{
		return quad_visible;
	}

	// Step 3
	mp.x = (p0->x + p1->x) >> 1;
	mp.y = (p0->y + p1->y) >> 1;

	// Calculate major and minor radius
	a = (p1->x - p0->x) >> 1;
	b = (p1->y - p0->y) >> 1;
	if (b > a)
	{
		const double p = a;
		a = b;
		b = p;
	}
	asquared = a * a;
	bsquared = b * b;

	// Step 4
	if (!(mp.x > SisScreenWidth() || p1->x < 0 || p0->y > SisScreenHeight() || mp.y < 0 || (x1 > SisScreenWidth() && y0 < 0)))
	{
		// Step 5
		if (PointOnScreen(mp.x, p0->y) || PointOnScreen(p1->x, mp.y))
		{
			quad_visible |= FirstQuad;
		}
		else
		{
			// Step 6
			x = (double)(SisScreenWidth() - mp.x);
			y = (double)(0 - mp.y);
			r0 = ((x * x) / asquared) + ((y * y) / bsquared);

			x = (double)(uqstl::max(0, mp.x) - mp.x);
			y = (double)(uqstl::min(SisScreenHeight(), mp.y) - mp.y);
			r1 = ((x * x) / asquared) + ((y * y) / bsquared);

			if (r0 >= 1.0f && r1 <= 1.0f)
			{
				quad_visible |= FirstQuad;
			}
		}
	}

	// Step 4
	if (!(p0->x > SisScreenWidth() || mp.x < 0 || p0->y > SisScreenHeight() || mp.y < 0 || (x0 < 0 && y0 < 0)))
	{
		// Step 5
		if (PointOnScreen(p0->x, mp.y) || PointOnScreen(mp.x, p0->y))
		{
			quad_visible |= SecondQuad;
		}
		else
		{
			// Step 6
			x = (double)(0 - mp.x);
			y = (double)(0 - mp.y);
			r0 = ((x * x) / asquared) + ((y * y) / bsquared);

			x = (double)(uqstl::min(SisScreenWidth(), mp.x) - mp.x);
			y = (double)(uqstl::min(SisScreenHeight(), mp.y) - mp.y);
			r1 = ((x * x) / asquared) + ((y * y) / bsquared);

			if (r0 >= 1.0f && r1 <= 1.0f)
			{
				quad_visible |= SecondQuad;
			}
		}
	}

	// Step 4
	if (!(p0->x > SisScreenWidth() || mp.x < 0 || mp.y > SisScreenHeight() || p1->y < 0 || (x0 < 0 && y1 > SisScreenHeight())))
	{
		// Step 5
		if (PointOnScreen(p0->x, mp.y) || PointOnScreen(mp.x, p1->y))
		{
			quad_visible |= ThirdQuad;
		}
		else
		{
			// Step 6
			x = (double)(0 - mp.x);
			y = (double)(SisScreenHeight() - mp.y);
			r0 = ((x * x) / asquared) + ((y * y) / bsquared);

			x = (double)(uqstl::min(SisScreenWidth(), mp.x) - mp.x);
			y = (double)(uqstl::max(0, mp.y) - mp.y);
			r1 = ((x * x) / asquared) + ((y * y) / bsquared);

			if (r0 >= 1.0f && r1 <= 1.0f)
			{
				quad_visible |= ThirdQuad;
			}
		}
	}

	// Step 4
	if (!(mp.x > SisScreenWidth() || p1->x < 0 || mp.y > SisScreenHeight() || p1->y < 0 || (x1 > SisScreenWidth() && y1 > SisScreenHeight())))
	{
		// Step 5
		if (PointOnScreen(mp.x, p1->y) || PointOnScreen(p1->x, mp.y))
		{
			quad_visible |= FourthQuad;
		}
		else
		{
			// Step 6
			x = (double)(SisScreenWidth() - mp.x);
			y = (double)(SisScreenHeight() - mp.y);
			r0 = ((x * x) / asquared) + ((y * y) / bsquared);

			x = (double)(uqstl::max(0, mp.x) - mp.x);
			y = (double)(uqstl::max(0, mp.y) - mp.y);
			r1 = ((x * x) / asquared) + ((y * y) / bsquared);

			if (r0 >= 1.0f && r1 <= 1.0f)
			{
				quad_visible |= FourthQuad;
			}
		}
	}

	return quad_visible;
}

//static uint8_t
//CheckOvalCollisionOld (GfxPoint *ch_one, GfxPoint *ch_two)
//{
//	GfxPoint mp;
//	BresenhamLine ClipLine;
//	GfxRect ClipRect;
//	uint8_t quad_visible = 0;
//
//	ClipRect.corner.x = ClipRect.corner.y = 0;
//	ClipRect.extent.width = SisScreenWidth();
//	ClipRect.extent.height = SisScreenHeight();
//
//	mp.x = (ch_one->x + ch_two->x) >> 1;
//	mp.y = (ch_one->y + ch_two->y) >> 1;
//
//	if (ch_one->y >= 0 && ch_one->y < ClipRect.extent.height
//		&& ch_two->x >= 0 && ch_two->x < ClipRect.extent.width)
//		quad_visible |= FirstQuad;
//	else
//	{
//		ClipLine.first.x = mp.x;
//		ClipLine.first.y = ch_one->y;
//		ClipLine.second.x = ch_two->x;
//		ClipLine.second.y = mp.y;
//		if (_clip_line ((LargeRect *)&ClipRect, &ClipLine))
//			quad_visible |= FirstQuad;
//	}
//
//	if (ch_one->y >= 0 && ch_one->y < ClipRect.extent.height
//		&& ch_one->x >= 0 && ch_one->x < ClipRect.extent.width)
//		quad_visible |= SecondQuad;
//	else
//	{
//		ClipLine.first.x = mp.x;
//		ClipLine.first.y = ch_one->y;
//		ClipLine.second.x = ch_one->x;
//		ClipLine.second.y = mp.y;
//		if (_clip_line ((LargeRect *)&ClipRect, &ClipLine))
//			quad_visible |= SecondQuad;
//	}
//
//	if (ch_two->y >= 0 && ch_two->y < ClipRect.extent.height
//		&& ch_one->x >= 0 && ch_one->x < ClipRect.extent.width)
//		quad_visible |= ThirdQuad;
//	else
//	{
//		ClipLine.first.x = mp.x;
//		ClipLine.first.y = ch_two->y;
//		ClipLine.second.x = ch_one->x;
//		ClipLine.second.y = mp.y;
//		if (_clip_line ((LargeRect *)&ClipRect, &ClipLine))
//			quad_visible |= ThirdQuad;
//	}
//
//	if (ch_two->y >= 0 && ch_two->y < ClipRect.extent.height
//		&& ch_two->x >= 0 && ch_two->x < ClipRect.extent.width)
//		quad_visible |= FourthQuad;
//	else
//	{
//		ClipLine.first.x = mp.x;
//		ClipLine.first.y = ch_two->y;
//		ClipLine.second.x = ch_two->x;
//		ClipLine.second.y = mp.y;
//		if (_clip_line ((LargeRect *)&ClipRect, &ClipLine))
//			quad_visible |= FourthQuad;
//	}
//
//	return quad_visible;
//}

void DrawOval(LargeRect* pRect, uint8_t num_off_pixels, bool scaled)
{
	int16_t x {};
	int16_t y {};
	int16_t A {};
	int16_t B {};
	int64_t TwoAsquared {};
	int64_t dy {};
	Primitive prim[NumQuads];
	LargePoint p0;
	LargePoint p1;
	const bool use_pointprim = (!scaled && num_off_pixels <= 1);
	uint8_t render {};

	p0.x = pRect->corner.x;
	p0.y = pRect->corner.y;
	p1.x = p0.x + pRect->extent.width - 1;
	p1.y = p0.y + pRect->extent.height - 1;
	// Kruzen: adapted from the original code. If rect is defined incorrectly - make it flat as a pancake and draw a line.
	if (p1.x <= p0.x || p1.y <= p0.y)
	{
		GfxLine corners;

		if (p1.x < p0.x)
		{
			p1.x = p0.x;
		}
		if (p1.y < p0.y)
		{
			p1.y = p0.y;
		}

		TruncateDPoint(&p0);
		TruncateDPoint(&p1);

		corners.first.x = (int16_t)p0.x;
		corners.first.y = (int16_t)p0.y;

		corners.second.x = (int16_t)p1.x;
		corners.second.y = (int16_t)p1.y;

		DrawLine(&corners, 1);
		return;
	}

	const uint8_t quad_visible {CheckOvalCollision(&p0, &p1)};

	if (!quad_visible)
	{
		return;
	}

	uint16_t StartPrim {EndOfList};
	for (x = 0; x < NumQuads; ++x)
	{
		if (quad_visible & (1 << x))
		{
			SetPrimNextLink(&prim[x], StartPrim);
			SetPrimType(&prim[x], use_pointprim ? POINT_PRIM : STAMPFILL_PRIM);
			prim[x].Object.Stamp.frame =
				DecFrameIndex(stars_in_space);
			SetPrimColor(&prim[x], _get_context_fg_color());
			SetPrimFlags(&prim[x], 0);
			StartPrim = x;
		}
		else // Kruzen: just to be sure so DrawBatch() skip it
		{
			SetPrimType(&prim[x], OffscreenPrim);
		}
	}

	A = pRect->extent.width >> 1;
	B = pRect->extent.height >> 1;

	x = 0;
	y = B;

	int64_t Asquared {((int64_t)A * A) << 1};
	int64_t Bsquared {((int64_t)B * B) << 1};
	do
	{
		Asquared >>= 1;
		Bsquared >>= 1;
		TwoAsquared = Asquared << 1;
		dy = TwoAsquared * B;
	} while (dy / B != TwoAsquared);
	const int64_t TwoBsquared {Bsquared << 1};

	int64_t dx {0};
	int64_t d {Bsquared - (dy >> 1) + (Asquared >> 2)};

	uint16_t off {0};
	A += pRect->corner.x;
	B += pRect->corner.y;

	// Kruzen: a check for prim being on screen is added
	if (use_pointprim)
	{
		while (dx < dy)
		{
			if (off-- == 0)
			{
				render = 0;
				prim[0].Object.Point.x = prim[3].Object.Point.x = A + x;
				prim[0].Object.Point.y = prim[1].Object.Point.y = B - y;
				prim[1].Object.Point.x = prim[2].Object.Point.x = A - x;
				prim[2].Object.Point.y = prim[3].Object.Point.y = B + y;

				if (quad_visible & FullVisibility)
				{
					DrawBatch(prim, StartPrim, 0);
				}
				else
				{
					if (quad_visible & FirstQuad)
					{
						if (PointOnScreen(A + x, B - y))
						{
							SetPrimType(&prim[0], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[0], OffscreenPrim);
						}
					}

					if (quad_visible & SecondQuad)
					{
						if (PointOnScreen(A - x, B - y))
						{
							SetPrimType(&prim[1], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[1], OffscreenPrim);
						}
					}

					if (quad_visible & ThirdQuad)
					{
						if (PointOnScreen(A - x, B + y))
						{
							SetPrimType(&prim[2], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[2], OffscreenPrim);
						}
					}

					if (quad_visible & FourthQuad)
					{
						if (PointOnScreen(A + x, B + y))
						{
							SetPrimType(&prim[3], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[3], OffscreenPrim);
						}
					}

					if (render > 0)
					{
						DrawBatch(prim, StartPrim, 0);
					}
				}
				off = num_off_pixels;
			}

			if (d > 0)
			{
				--y;
				dy -= TwoAsquared;
				d -= dy;
			}

			++x;
			dx += TwoBsquared;
			d += Bsquared + dx;
		}
	}
	else
	{
		while (dx < dy)
		{
			if (off-- == 0)
			{ // Kruzen: Use correct struct for these. I have no idea how that worked before
				render = 0;
				prim[0].Object.Stamp.origin.x = prim[3].Object.Stamp.origin.x = A + x;
				prim[0].Object.Stamp.origin.y = prim[1].Object.Stamp.origin.y = B - y;
				prim[1].Object.Stamp.origin.x = prim[2].Object.Stamp.origin.x = A - x;
				prim[2].Object.Stamp.origin.y = prim[3].Object.Stamp.origin.y = B + y;

				if (quad_visible & FullVisibility)
				{
					DrawBatch(prim, StartPrim, 0);
				}
				else
				{
					if (quad_visible & FirstQuad)
					{
						if (PointOnScreen(A + x, B - y))
						{
							SetPrimType(&prim[0], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[0], OffscreenPrim);
						}
					}

					if (quad_visible & SecondQuad)
					{
						if (PointOnScreen(A - x, B - y))
						{
							SetPrimType(&prim[1], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[1], OffscreenPrim);
						}
					}

					if (quad_visible & ThirdQuad)
					{
						if (PointOnScreen(A - x, B + y))
						{
							SetPrimType(&prim[2], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[2], OffscreenPrim);
						}
					}

					if (quad_visible & FourthQuad)
					{
						if (PointOnScreen(A + x, B + y))
						{
							SetPrimType(&prim[3], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[3], OffscreenPrim);
						}
					}

					if (render > 0)
					{
						DrawBatch(prim, StartPrim, 0);
					}
				}
				off = num_off_pixels;
			}

			if (d > 0)
			{
				--y;
				dy -= TwoAsquared;
				d -= dy;
			}

			++x;
			dx += TwoBsquared;
			d += Bsquared + dx;
		}
	}

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

	if (use_pointprim)
	{
		while (y >= 0)
		{
			if (off-- == 0)
			{
				render = 0;
				prim[0].Object.Point.x = prim[3].Object.Point.x = A + x;
				prim[0].Object.Point.y = prim[1].Object.Point.y = B - y;
				prim[1].Object.Point.x = prim[2].Object.Point.x = A - x;
				prim[2].Object.Point.y = prim[3].Object.Point.y = B + y;

				if (quad_visible & FullVisibility)
				{
					DrawBatch(prim, StartPrim, 0);
				}
				else
				{
					if (quad_visible & FirstQuad)
					{
						if (PointOnScreen(A + x, B - y))
						{
							SetPrimType(&prim[0], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[0], OffscreenPrim);
						}
					}

					if (quad_visible & SecondQuad)
					{
						if (PointOnScreen(A - x, B - y))
						{
							SetPrimType(&prim[1], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[1], OffscreenPrim);
						}
					}

					if (quad_visible & ThirdQuad)
					{
						if (PointOnScreen(A - x, B + y))
						{
							SetPrimType(&prim[2], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[2], OffscreenPrim);
						}
					}

					if (quad_visible & FourthQuad)
					{
						if (PointOnScreen(A + x, B + y))
						{
							SetPrimType(&prim[3], POINT_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[3], OffscreenPrim);
						}
					}

					if (render > 0)
					{
						DrawBatch(prim, StartPrim, 0);
					}
				}
				off = num_off_pixels;
			}

			if (d < 0)
			{
				++x;
				dx += TwoBsquared;
				d += dx;
			}

			--y;
			dy -= TwoAsquared;
			d += Asquared - dy;
		}
	}
	else
	{
		while (y >= 0)
		{
			if (off-- == 0)
			{
				render = 0;
				prim[0].Object.Stamp.origin.x = prim[3].Object.Stamp.origin.x = A + x;
				prim[0].Object.Stamp.origin.y = prim[1].Object.Stamp.origin.y = B - y;
				prim[1].Object.Stamp.origin.x = prim[2].Object.Stamp.origin.x = A - x;
				prim[2].Object.Stamp.origin.y = prim[3].Object.Stamp.origin.y = B + y;

				if (quad_visible & FullVisibility)
				{
					DrawBatch(prim, StartPrim, 0);
				}
				else
				{
					if (quad_visible & FirstQuad)
					{
						if (PointOnScreen(A + x, B - y))
						{
							SetPrimType(&prim[0], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[0], OffscreenPrim);
						}
					}

					if (quad_visible & SecondQuad)
					{
						if (PointOnScreen(A - x, B - y))
						{
							SetPrimType(&prim[1], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[1], OffscreenPrim);
						}
					}

					if (quad_visible & ThirdQuad)
					{
						if (PointOnScreen(A - x, B + y))
						{
							SetPrimType(&prim[2], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[2], OffscreenPrim);
						}
					}

					if (quad_visible & FourthQuad)
					{
						if (PointOnScreen(A + x, B + y))
						{
							SetPrimType(&prim[3], STAMPFILL_PRIM);
							render++;
						}
						else
						{
							SetPrimType(&prim[3], OffscreenPrim);
						}
					}

					if (render > 0)
					{
						DrawBatch(prim, StartPrim, 0);
					}
				}
				off = num_off_pixels;
			}

			if (d < 0)
			{
				++x;
				dx += TwoBsquared;
				d += dx;
			}

			--y;
			dy -= TwoAsquared;
			d += Asquared - dy;
		}
	}
}

void DrawFilledOval(LargeRect* pRect)
{ // Kruzen: originally there was standard rect, but drawing fuel circle in HdResolutionFactor on max zoom causes overflow (Width ~65k)
	// So unless we want to expand standard rect to support 4 bytes per dimension - use this
	int16_t x {};
	int16_t y {};
	int16_t A {};
	int16_t B {};
	int64_t TwoAsquared {};
	int64_t dy {};
	Primitive prim[NumQuads >> 1];
	GfxPoint first;
	GfxPoint second;
	LargePoint p0;
	LargePoint p1;
	uint16_t lines_r = 0;

	p0.x = pRect->corner.x;
	p0.y = pRect->corner.y;
	p1.x = p0.x + pRect->extent.width - 1;
	p1.y = p0.y + pRect->extent.height - 1;
	// Kruzen: adapted from the original code. If rect is defined incorrectly - make it flat as a pancake and draw a line.
	if (p1.x <= p0.x || p1.y <= p0.y)
	{
		GfxLine corners;

		if (p1.x < p0.x)
		{
			p1.x = p0.x;
		}
		if (p1.y < p0.y)
		{
			p1.y = p0.y;
		}

		TruncateDPoint(&p0);
		TruncateDPoint(&p1);

		corners.first.x = (int16_t)p0.x;
		corners.first.y = (int16_t)p0.y;

		corners.second.x = (int16_t)p1.x;
		corners.second.y = (int16_t)p1.y;

		DrawLine(&corners, 1);
		return;
	}

	uint16_t StartPrim {EndOfList};
	for (x = 0; x < (NumQuads >> 1); ++x)
	{
		SetPrimNextLink(&prim[x], StartPrim);
		SetPrimType(&prim[x], LINE_PRIM);
		SetPrimColor(&prim[x], _get_context_fg_color());

		StartPrim = x;
	}

	A = pRect->extent.width >> 1;
	B = pRect->extent.height >> 1;

	x = 0;
	y = B;

	int64_t Asquared {((int64_t)A * A) << 1};
	int64_t Bsquared {((int64_t)B * B) << 1};
	do
	{
		Asquared >>= 1;
		Bsquared >>= 1;
		TwoAsquared = Asquared << 1;
		dy = TwoAsquared * B;
	} while (dy / B != TwoAsquared);
	const int64_t TwoBsquared {Bsquared << 1};

	int64_t dx {0};
	int64_t d {Bsquared - (dy >> 1) + (Asquared >> 2)};

	A += pRect->corner.x;
	B += pRect->corner.y;
	while (dx < dy)
	{
		if (d > 0)
		{ // Kruzen: originally PrimType was FILLRECT with height of 1
			// Changed to line and added a skip if offscreen
			lines_r = 0;

			first.x = uqstl::max(A - x, 0);
			second.x = uqstl::min(A - x + (x << 1) + 1, SisScreenWidth());
			first.y = second.y = B - y;
			if (((B - y) >= 0 && (B - y) <= SisScreenHeight()))
			{
				SetPrimType(&prim[0], LINE_PRIM);
				prim[0].Object.Line.first = first;
				prim[0].Object.Line.second = second;
				lines_r++;
			}
			else
			{
				SetPrimType(&prim[0], OffscreenPrim);
			}

			first.y = second.y = B + y;
			if (((B + y) >= 0 && (B + y) <= SisScreenHeight()))
			{
				SetPrimType(&prim[1], LINE_PRIM);
				prim[1].Object.Line.first = first;
				prim[1].Object.Line.second = second;
				lines_r++;
			}
			else
			{
				SetPrimType(&prim[1], OffscreenPrim);
			}

			if (lines_r > 0)
			{
				DrawBatch(prim, StartPrim, 0);
			}

			--y;
			dy -= TwoAsquared;
			d -= dy;
		}

		++x;
		dx += TwoBsquared;
		d += Bsquared + dx;
	}

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

	while (y >= 0)
	{ // Kruzen: originally PrimType was FILLRECT with height of 1
		// Changed to line and added a skip if offscreen
		lines_r = 0;
		first.x = uqstl::max(A - x, 0);
		second.x = uqstl::min(A - x + (x << 1) + 1, SisScreenWidth());
		first.y = second.y = B - y;
		if (((B - y) >= 0 && (B - y) <= SisScreenHeight()))
		{
			SetPrimType(&prim[0], LINE_PRIM);
			prim[0].Object.Line.first = first;
			prim[0].Object.Line.second = second;
			lines_r++;
		}
		else
		{
			SetPrimType(&prim[0], OffscreenPrim);
		}

		first.y = second.y = B + y;
		if (((B + y) >= 0 && (B + y) <= SisScreenHeight()))
		{
			SetPrimType(&prim[1], LINE_PRIM);
			prim[1].Object.Line.first = first;
			prim[1].Object.Line.second = second;
			lines_r++;
		}
		else
		{
			SetPrimType(&prim[1], OffscreenPrim);
		}

		if (lines_r > 0)
		{
			DrawBatch(prim, StartPrim, 0);
		}

		if (d < 0)
		{
			++x;
			dx += TwoBsquared;
			d += dx;
		}

		--y;
		dy -= TwoAsquared;
		d += Asquared - dy;
	}
}

void DrawEllipseQuadrants(int cx, int cy, int x, int y, int shear, int filled)
{
	GfxPoint p;
	GfxLine l;

	if (filled && y != 0)
	{
		l.first.x = l.second.x = cx - x;
		l.first.y = cy - shear - y;
		l.second.y = cy - shear + y;
		DrawLine(&l, 1);

		if (x != 0)
		{
			l.first.x = l.second.x = cx + x;
			l.first.y = cy + shear - y;
			l.second.y = cy + shear + y;
			DrawLine(&l, 1);
		}
	}
	else
	{
		p.x = cx - x;
		p.y = cy - shear - y;
		DrawPoint(&p);
		p.y = cy - shear + y;
		DrawPoint(&p);

		if (x != 0)
		{
			p.x = cx + x;
			p.y = cy + shear - y;
			DrawPoint(&p);
			p.y = cy + shear + y;
			DrawPoint(&p);
		}
	}
}

void DrawEllipse(int cx, int cy, int rx, int ry, int shear, int filled, int dotted)
{
	// adapted from https://zingl.github.io/Bresenham.pdf section 2.1
	int x = rx;
	int y = 0;
	int s = shear;
	int d = 0;
	const int sRound = (shear < 0) ? -(rx / 2) : (rx / 2);
	const int64_t dex = 2 * (long)ry * ry;
	int64_t ex = (long)ry * ry - (long)rx * dex;
	const int64_t dey = 2 * (long)rx * rx;
	int64_t ey = (long)rx * rx;
	int64_t e = ex + ey;
	int64_t e2 {};

	if (rx < 0)
	{
		rx = 0;
	}
	if (ry < 0)
	{
		ry = 0;
	}
	if (!rx || !ry)
	{
		GfxLine l;
		l.first.x = cx - rx;
		l.first.y = cy - ry - shear;
		l.second.x = cx + rx;
		l.second.y = cy + ry + shear;
		DrawLine(&l, 1);
		return;
	}

	do
	{
		if (!filled && !d--)
		{
			d = dotted;
			s = (int)(((int64_t)x * shear + sRound) / rx);
			DrawEllipseQuadrants(cx, cy, x, y, -s, 0);
		}

		e2 = e * 2;
		if (e2 >= ex)
		{
			if (filled)
			{
				s = (int)(((int64_t)x * shear + sRound) / rx);
				DrawEllipseQuadrants(cx, cy, x, y, -s, 1);
			}
			x--;
			ex += dex;
			e += ex;
		}
		if (e2 <= ey)
		{
			y++;
			ey += dey;
			e += ey;
		}
	} while (x > 0);

	// when x=0, y must be ry and s must be 0
	DrawEllipseQuadrants(cx, cy, 0, ry, 0, filled);
}

void DrawRotatedEllipse(int cx, int cy, int rx, int ry, int angle_deg, int filled, int dotted)
{
	// based on https://zingl.github.io/Bresenham.pdf section 4.3
	const double rx2 = (double)rx * rx;
	const double ry2 = (double)ry * ry;
	const double theta = (angle_deg % 90) * M_PI / 180.0;
	const double st = sin(theta);
	const double ct = cos(theta);
	const double xd2 = rx2 * ct * ct + ry2 * st * st;
	const double xd = sqrt(xd2);
	const int shear = (int)(((rx2 - ry2) * st * ct) / xd + 0.5);

	rx = (int)(xd + 0.5);
	ry = (int)(sqrt((rx2 * ry2) / xd2) + 0.5);
	DrawEllipse(cx, cy, rx, ry, shear, filled, dotted);
}
