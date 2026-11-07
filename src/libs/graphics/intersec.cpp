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

#include "core/log/log.h"
#include "libs/graphics/context.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/tfb_draw.h"
#include "uqm/units.h"

//#define DEBUG_INTERSEC

static inline bool images_intersect(ImageBox* box1, ImageBox* box2, const GfxRect* rect)
{
	return TFB_DrawImage_Intersect(box1->FramePtr->image, box1->Box.corner,
								   box2->FramePtr->image, box2->Box.corner, rect);
}

static TimeValue frame_intersect(IntersectInfo* pControl0, GfxRect* pr0,
								 IntersectInfo* pControl1, GfxRect* pr1, TimeValue t0,
								 TimeValue t1)
{
	int32_t time_error0 {};
	int32_t time_error1 {};
	int32_t cycle0 {};
	int32_t cycle1 {};
	int32_t xincr0 {};
	int32_t yincr0 {};
	int32_t xincr1 {};
	int32_t yincr1 {};
	int32_t xerror0 {};
	int32_t xerror1 {};
	int32_t yerror0 {};
	int32_t yerror1 {};
	int32_t xstep0 {};
	int32_t ystep0 {};
	int32_t xstep1 {};
	int32_t ystep1 {};
	GfxRect r_intersect;
	ImageBox IB0;
	ImageBox IB1;
	bool check0 {};
	bool check1 {};

	IB0.FramePtr = pControl0->IntersectStamp.frame;
	IB0.Box.corner = pr0->corner;
	IB0.Box.extent.width = GetFrameWidth(IB0.FramePtr);
	IB0.Box.extent.height = GetFrameHeight(IB0.FramePtr);
	IB1.FramePtr = pControl1->IntersectStamp.frame;
	IB1.Box.corner = pr1->corner;
	IB1.Box.extent.width = GetFrameWidth(IB1.FramePtr);
	IB1.Box.extent.height = GetFrameHeight(IB1.FramePtr);

	const int32_t iterator {uqm::ResScale(1)};

	int32_t dx_0 {pr0->extent.width};
	int32_t dy_0 {pr0->extent.height};
	if (dx_0 >= 0)
	{
		xincr0 = 1;
		xstep0 = iterator;
	}
	else
	{
		xincr0 = -1;
		xstep0 = -iterator;
		dx_0 = -dx_0;
	}
	if (dy_0 >= 0)
	{
		yincr0 = 1;
		ystep0 = iterator;
	}
	else
	{
		yincr0 = -1;
		ystep0 = -iterator;
		dy_0 = -dy_0;
	}
	if (dx_0 >= dy_0)
	{
		cycle0 = dx_0;
	}
	else
	{
		cycle0 = dy_0;
	}
	xerror0 = yerror0 = cycle0;

	int32_t dx_1 {pr1->extent.width};
	int32_t dy_1 {pr1->extent.height};
	if (dx_1 >= 0)
	{
		xincr1 = 1;
		xstep1 = iterator;
	}
	else
	{
		xincr1 = -1;
		xstep1 = -iterator;
		dx_1 = -dx_1;
	}
	if (dy_1 >= 0)
	{
		yincr1 = 1;
		ystep1 = iterator;
	}
	else
	{
		yincr1 = -1;
		ystep1 = -iterator;
		dy_1 = -dy_1;
	}
	if (dx_1 >= dy_1)
	{
		cycle1 = dx_1;
	}
	else
	{
		cycle1 = dy_1;
	}
	xerror1 = yerror1 = cycle1;

	check0 = check1 = false;
	if (t0 <= 1)
	{
		time_error0 = time_error1 = 0;
		if (t0 == 0)
		{
			++t0;
			goto CheckFirstIntersection;
		}
	}
	else
	{
		int32_t delta {};
		long error {};

		uint32_t start {(uint32_t)cycle0 * (uint32_t)(t0 - 1)};
		time_error0 = start & ((1 << TimeShift) - 1);
		if ((start >>= (uint32_t)TimeShift) > 0)
		{
			if ((error = (long)xerror0
					   - (long)dx_0 * (long)start)
				> 0)
			{
				xerror0 = (int32_t)error;
			}
			else
			{
				delta = -(int32_t)(error / (long)cycle0) + 1;
				IB0.Box.corner.x += xincr0 * delta;
				xerror0 = (int32_t)(error + (long)cycle0 * (long)delta);
			}
			if ((error = (long)yerror0
					   - (long)dy_0 * (long)start)
				> 0)
			{
				yerror0 = (int32_t)error;
			}
			else
			{
				delta = -(int32_t)(error / (long)cycle0) + 1;
				IB0.Box.corner.y += yincr0 * delta;
				yerror0 = (int32_t)(error + (long)cycle0 * (long)delta);
			}
			pr0->corner = IB0.Box.corner;
		}

		start = (uint32_t)cycle1 * (uint32_t)(t0 - 1);
		time_error1 = start & ((1 << TimeShift) - 1);
		if ((start >>= (uint32_t)TimeShift) > 0)
		{
			if ((error = (long)xerror1
					   - (long)dx_1 * (long)start)
				> 0)
			{
				xerror1 = (int32_t)error;
			}
			else
			{
				delta = -(int32_t)(error / (long)cycle1) + 1;
				IB1.Box.corner.x += xincr1 * delta;
				xerror1 = (int32_t)(error + (long)cycle1 * (long)delta);
			}
			if ((error = (long)yerror1
					   - (long)dy_1 * (long)start)
				> 0)
			{
				yerror1 = (int32_t)error;
			}
			else
			{
				delta = -(int32_t)(error / (long)cycle1) + 1;
				IB1.Box.corner.y += yincr1 * delta;
				yerror1 = (int32_t)(error + (long)cycle1 * (long)delta);
			}
			pr1->corner = IB1.Box.corner;
		}
	}

	pControl0->last_time_val = pControl1->last_time_val = t0;
	do
	{
		++t0;
		if ((time_error0 += cycle0) >= (1 << TimeShift))
		{
			if ((xerror0 -= dx_0) <= 0)
			{
				IB0.Box.corner.x += xstep0;
				xerror0 += cycle0;
			}
			if ((yerror0 -= dy_0) <= 0)
			{
				IB0.Box.corner.y += ystep0;
				yerror0 += cycle0;
			}

			check0 = true;
			time_error0 -= (1 << TimeShift);
		}

		if ((time_error1 += cycle1) >= (1 << TimeShift))
		{
			if ((xerror1 -= dx_1) <= 0)
			{
				IB1.Box.corner.x += xstep1;
				xerror1 += cycle1;
			}
			if ((yerror1 -= dy_1) <= 0)
			{
				IB1.Box.corner.y += ystep1;
				yerror1 += cycle1;
			}

			check1 = true;
			time_error1 -= (1 << TimeShift);
		}

		if (check0 || check1)
		{ /* if check0 && check1, this may not be quite right --
						 * if shapes had a pixel's separation to begin with
						 * and both moved toward each other, you would actually
						 * get a pixel overlap but since the last positions were
						 * separated by a pixel, the shapes wouldn't be touching
						 * each other.
						 */
CheckFirstIntersection:
			if (BoxIntersect(&IB0.Box, &IB1.Box, &r_intersect) != IntersectCode::None
				&& images_intersect(&IB0, &IB1, &r_intersect))
			{
				return (t0);
			}

			if (check0)
			{
				pr0->corner = IB0.Box.corner;
				pControl0->last_time_val = t0;
				check0 = false;
			}
			if (check1)
			{
				pr1->corner = IB1.Box.corner;
				pControl1->last_time_val = t0;
				check1 = false;
			}
		}
	} while (t0 <= t1);

	return ((TimeValue)0);
}

TimeValue DrawablesIntersect(IntersectInfo* pControl0,
							 IntersectInfo* pControl1, TimeValue max_time_val)
{
	GfxRect r0;
	GfxRect r1;

	if (!ContextActive() || max_time_val == 0)
	{
		return ((TimeValue)0);
	}
	else if (max_time_val > MaxTimeValue)
	{
		max_time_val = MaxTimeValue;
	}

	pControl0->last_time_val = pControl1->last_time_val = 0;

	r0.corner = pControl0->IntersectStamp.origin;
	r1.corner = pControl1->IntersectStamp.origin;

	r0.extent.width = pControl0->EndPoint.x - r0.corner.x;
	r0.extent.height = pControl0->EndPoint.y - r0.corner.y;
	r1.extent.width = pControl1->EndPoint.x - r1.corner.x;
	r1.extent.height = pControl1->EndPoint.y - r1.corner.y;

	FrameRef FramePtr0 {pControl0->IntersectStamp.frame};
	if (FramePtr0 == 0)
	{
		return (0);
	}
	r0.corner.x -= FramePtr0->hotSpot.x;
	r0.corner.y -= FramePtr0->hotSpot.y;

	FrameRef FramePtr1 {pControl1->IntersectStamp.frame};
	if (FramePtr1 == 0)
	{
		return (0);
	}
	r1.corner.x -= FramePtr1->hotSpot.x;
	r1.corner.y -= FramePtr1->hotSpot.y;

	int32_t dy {r1.corner.y - r0.corner.y};
	int32_t time_y_0 {dy - GetFrameHeight(FramePtr0) + 1};
	int32_t time_y_1 {dy + GetFrameHeight(FramePtr1) - 1};
	dy = r0.extent.height - r1.extent.height;

	if ((time_y_0 <= 0 && time_y_1 >= 0)
		|| (time_y_0 > 0 && dy >= time_y_0)
		|| (time_y_1 < 0 && dy <= time_y_1))
	{
		int32_t dx {r1.corner.x - r0.corner.x};
		int32_t time_x_0 {dx - GetFrameWidth(FramePtr0) + 1};
		int32_t time_x_1 {dx + GetFrameWidth(FramePtr1) - 1};
		dx = r0.extent.width - r1.extent.width;

		if ((time_x_0 <= 0 && time_x_1 >= 0)
			|| (time_x_0 > 0 && dx >= time_x_0)
			|| (time_x_1 < 0 && dx <= time_x_1))
		{
			TimeValue intersect_time {};

			if (dx == 0 && dy == 0)
			{
				time_y_0 = time_y_1 = 0;
			}
			else
			{
				int32_t t {};
				long time_beg {};
				long time_end {};
				long fract {};

				if (time_y_1 < 0)
				{
					t = time_y_0;
					time_y_0 = -time_y_1;
					time_y_1 = -t;
				}
				else if (time_y_0 <= 0)
				{
					if (dy < 0)
					{
						time_y_1 = -time_y_0;
					}
					time_y_0 = 0;
				}
				if (dy < 0)
				{
					dy = -dy;
				}
				if (dy < time_y_1)
				{
					time_y_1 = dy;
				}
				/* just to be safe, widen search area */
				--time_y_0;
				++time_y_1;

				if (time_x_1 < 0)
				{
					t = time_x_0;
					time_x_0 = -time_x_1;
					time_x_1 = -t;
				}
				else if (time_x_0 <= 0)
				{
					if (dx < 0)
					{
						time_x_1 = -time_x_0;
					}
					time_x_0 = 0;
				}
				if (dx < 0)
				{
					dx = -dx;
				}
				if (dx < time_x_1)
				{
					time_x_1 = dx;
				}
				/* just to be safe, widen search area */
				--time_x_0;
				++time_x_1;

#ifdef DEBUG_INTERSEC
				uqm::log::debug("FramePtr0<{}, {}> --> <{}, {}>",
								GetFrameWidth(FramePtr0), GetFrameHeight(FramePtr0),
								r0.corner.x, r0.corner.y);
				uqm::log::debug("FramePtr1<{}, {}> --> <{}, {}>",
								GetFrameWidth(FramePtr1), GetFrameHeight(FramePtr1),
								r1.corner.x, r1.corner.y);
				uqm::log::debug("time_x({}, {})-{}, time_y({}, {})-{}",
								time_x_0, time_x_1, dx, time_y_0, time_y_1, dy);
#endif /* DEBUG_INTERSEC */
				if (dx == 0)
				{
					time_beg = time_y_0;
					time_end = time_y_1;
					fract = dy;
				}
				else if (dy == 0)
				{
					time_beg = time_x_0;
					time_end = time_x_1;
					fract = dx;
				}
				else
				{
					long time_x {(long)time_x_0 * (long)dy};
					long time_y {(long)time_y_0 * (long)dx};
					time_beg = time_x < time_y ? time_y : time_x;

					time_x = (long)time_x_1 * (long)dy;
					time_y = (long)time_y_1 * (long)dx;
					time_end = time_x > time_y ? time_y : time_x;

					fract = (long)dx * (long)dy;
				}

				if ((time_beg <<= TimeShift) < fract)
				{
					time_y_0 = 0;
				}
				else
				{
					time_y_0 = (int32_t)(time_beg / fract);
				}

				if (time_end >= fract /* just in case of overflow */
					|| (time_end <<= TimeShift) >= fract * (long)max_time_val)
				{
					time_y_1 = max_time_val - 1;
				}
				else
				{
					time_y_1 = (int32_t)((time_end + fract - 1) / fract) - 1;
				}
			}

#ifdef DEBUG_INTERSEC
			uqm::log::debug("start_time = {}, end_time = {}",
							time_y_0, time_y_1);
#endif /* DEBUG_INTERSEC */
			if (time_y_0 <= time_y_1
				&& (intersect_time = frame_intersect(
						pControl0, &r0, pControl1, &r1,
						(TimeValue)time_y_0, (TimeValue)time_y_1)))
			{
				FramePtr0 = pControl0->IntersectStamp.frame;
				pControl0->EndPoint.x = r0.corner.x + FramePtr0->hotSpot.x;
				pControl0->EndPoint.y = r0.corner.y + FramePtr0->hotSpot.y;
				FramePtr1 = pControl1->IntersectStamp.frame;
				pControl1->EndPoint.x = r1.corner.x + FramePtr1->hotSpot.x;
				pControl1->EndPoint.y = r1.corner.y + FramePtr1->hotSpot.y;

				return (intersect_time);
			}
		}
	}

	return ((TimeValue)0);
}
