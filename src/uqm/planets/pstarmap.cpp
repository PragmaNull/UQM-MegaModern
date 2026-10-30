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

#include "scan.h"
#include "../colors.h"
#include "../controls.h"
#include "../menustat.h"
#include "../starmap.h"
#include "../races.h"
#include "../gameopt.h"
#include "../gamestr.h"
#include "../gendef.h"
#include "../globdata.h"
#include "../shipcont.h"
#include "../units.h"
#include "../hyper.h"
#include "../sis.h"
// for DrawHyperCoords(), DrawStatusMessage()
#include "../settings.h"
#include "../setup.h"
#include "../sounds.h"
#include "../state.h"
#include "../init.h"
#include "../uqmdebug.h"
#include "options.h"
#include "core/string/StringUtils.h"
#include "libs/inplib.h"
#include "libs/strlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/memlib.h"
#include "../util.h"
// For get_fuel_to_sol()
#include <stdlib.h>
#include <ctype.h>
// For isdigit()
#include <math.h> // sqrt()
#include "../build.h"
// For StartSphereTracking()
#include "uqm/setupmenu.h"

typedef enum
{
	NORMAL_STARMAP,
	WAR_ERA_STARMAP,
	CONSTELLATION_MAP,
	HOMEWORLDS_MAP,
	RAINBOW_MAP,
	NUM_STARMAPS
} CURRENT_STARMAP_SHOWN;

typedef struct namePlate
{
	GFXRECT rect;
	TEXT text;
	uint8_t index;
} NAMEPLATE;

static void
SwapPlates(NAMEPLATE* n1, NAMEPLATE* n2)
{
	NAMEPLATE temp = *n1;
	*n1 = *n2;
	*n2 = temp;
}

#define IGNORE_MOVING_SOI (1 << 5)
#define PRE_DEATH_SOI (1 << 6)
#define DEATH_SOI (1 << 7)

static GFXPOINT cursorLoc;
static GFXPOINT mapOrigin;
static int zoomLevel;
static FRAME StarMapFrame;
static CURRENT_STARMAP_SHOWN which_starmap;

static inline long
signedDivWithError(long val, long divisor)
{
	int invert = 0;
	if (val < 0)
	{
		invert = 1;
		val = -val;
	}
	val = (val + ROUNDING_ERROR(divisor)) / divisor;
	return invert ? -val : val;
}

#define MAP_FIT_X ((MAX_X_UNIVERSE + 1) / ORIG_SIS_SCREEN_WIDTH + 1)
#define MAP_FIT_XX ((MAX_X_UNIVERSE + 1) / SIS_SCREEN_WIDTH + 1)

static inline int16_t
universeToDispx(long ux)
{
	return signedDivWithError(((ux - mapOrigin.x) << zoomLevel)
								  * ORIG_SIS_SCREEN_WIDTH,
							  MAX_X_UNIVERSE + MAP_FIT_X)
		 + ((ORIG_SIS_SCREEN_WIDTH - 1) >> 1);
}
#define UNIVERSE_TO_DISPX(ux) RES_SCALE(universeToDispx(ux))
#define ORIG_UNIVERSE_TO_DISPX(ux) universeToDispx(ux)

static inline int16_t
universeToDispy(long uy)
{
	return signedDivWithError(((mapOrigin.y - uy) << zoomLevel)
								  * ORIG_SIS_SCREEN_HEIGHT,
							  MAX_Y_UNIVERSE + 2)
		 + ((ORIG_SIS_SCREEN_HEIGHT - 1) >> 1);
}
#define UNIVERSE_TO_DISPY(uy) RES_SCALE(universeToDispy(uy))
#define ORIG_UNIVERSE_TO_DISPY(uy) universeToDispy(uy)

static inline int16_t
dispxToUniverse(int16_t dx)
{
	return (((long)(dx - ((ORIG_SIS_SCREEN_WIDTH - 1) >> 1))
			 * (MAX_X_UNIVERSE + MAP_FIT_X))
			>> zoomLevel)
			 / ORIG_SIS_SCREEN_WIDTH
		 + mapOrigin.x;
}
#define DISP_TO_UNIVERSEX(dx) dispxToUniverse(RES_DESCALE(dx))
#define ORIG_DISP_TO_UNIVERSEX(dx) dispxToUniverse(dx)

static inline int16_t
dispyToUniverse(int16_t dy)
{
	return (((long)(((ORIG_SIS_SCREEN_HEIGHT - 1) >> 1) - dy)
			 * (MAX_Y_UNIVERSE + 2))
			>> zoomLevel)
			 / ORIG_SIS_SCREEN_HEIGHT
		 + mapOrigin.y;
}
#define DISP_TO_UNIVERSEY(dy) dispyToUniverse(RES_DESCALE(dy))
#define ORIG_DISP_TO_UNIVERSEY(dy) dispyToUniverse(dy)

// Old school HD-mod code for Malin's Sol ellipse.
static inline int16_t
universeToDispx2(int16_t ux)
{
	long v = signedDivWithError((((long)ux - mapOrigin.x) << zoomLevel)
									* SIS_SCREEN_WIDTH,
								MAX_X_UNIVERSE + MAP_FIT_XX)
		   + ((SIS_SCREEN_WIDTH - 1) >> 1);
	if (v > 32767)
	{
		return 32767;
	}
	if (v < -32768)
	{
		return -32768;
	}
	return v;
}
static inline int16_t
universeToDispy2(int16_t uy)
{
	long v = signedDivWithError((((long)mapOrigin.y - uy) << zoomLevel)
									* SIS_SCREEN_HEIGHT,
								MAX_Y_UNIVERSE + 2)
		   + ((SIS_SCREEN_HEIGHT - 1) >> 1);
	if (v > 32767)
	{
		return 32767;
	}
	if (v < -32768)
	{
		return -32768;
	}
	return v;
}

#define UNIVERSE_TO_DISPX2(ux) \
	(IS_HD ? universeToDispx2(ux) : universeToDispx(ux))
#define UNIVERSE_TO_DISPY2(uy) \
	(IS_HD ? universeToDispy2(uy) : universeToDispy(uy))

static bool transition_pending;

static void
flashCurrentLocation(GFXPOINT* where, bool force)
{
	static bool redraw = false;
	static uint8_t c = 0;
	static int val = -2;
	static GFXPOINT universe;
	static TimeCount NextTime = 0;

	if (where)
	{
		universe = *where;
	}

	if (GetTimeCounter() >= NextTime)
	{
		NextTime = GetTimeCounter() + (GameTicksPerSecond / 16);

		if (c == 0x00 || c == 0x1A)
		{
			val = -val;
		}
		c += val;

		redraw = true;
	}

	if (force || redraw)
	{
		Color OldColor;
		GFXCONTEXT OldContext;
		STAMP s;

		OldContext = SetContext(SpaceContext);

		OldColor = SetContextForeGroundColor(
			BUILD_COLOR(MAKE_RGB15(c, c, c), c));
		s.origin.x = UNIVERSE_TO_DISPX(universe.x);
		s.origin.y = UNIVERSE_TO_DISPY(universe.y);
		s.frame = IncFrameIndex(StarMapFrame);
		DrawFilledStamp(&s);
		SetContextForeGroundColor(OldColor);

		SetContext(OldContext);

		redraw = false;
	}
}

static void
DrawCursor(int16_t curs_x, int16_t curs_y)
{
	STAMP s;

	s.origin.x = curs_x;
	s.origin.y = curs_y;
	s.frame = StarMapFrame;

	DrawStamp(&s);
}

static void
DrawMarker(GFXPOINT dest, uint8_t type)
{
	STAMP s;

	if (type > 2)
	{
		return;
	}

	s.origin = MAKE_POINT(
		UNIVERSE_TO_DISPX(dest.x),
		UNIVERSE_TO_DISPY(dest.y));
	s.frame = SetAbsFrameIndex(MiscDataFrame, 106 + type);

	DrawStamp(&s);
}

static void
DrawAutoPilot(GFXPOINT* pDstPt)
{
	int16_t dx, dy,
		xincr, yincr,
		xerror, yerror,
		cycle, delta;
	GFXPOINT pt;
	STAMP s;

	if (!inHQSpace())
	{
		pt = CurStarDescPtr->star_pt;
	}
	else
	{
		pt.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
		pt.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
	}

	if (IS_HD)
	{
		s.frame = DecFrameIndex(stars_in_space);
	}

	pt.x = UNIVERSE_TO_DISPX(pt.x);
	pt.y = UNIVERSE_TO_DISPY(pt.y);

	dx = UNIVERSE_TO_DISPX(pDstPt->x) - pt.x;
	if (dx >= 0)
	{
		xincr = 1;
	}
	else
	{
		xincr = -1;
		dx = -dx;
	}
	dx <<= 1;

	dy = UNIVERSE_TO_DISPY(pDstPt->y) - pt.y;
	if (dy >= 0)
	{
		yincr = 1;
	}
	else
	{
		yincr = -1;
		dy = -dy;
	}
	dy <<= 1;

	if (dx >= dy)
	{
		cycle = dx;
	}
	else
	{
		cycle = dy;
	}
	delta = xerror = yerror = cycle >> 1;

	SetContextForeGroundColor(
		BUILD_COLOR(MAKE_RGB15(0x04, 0x04, 0x1F), 0x01));

	delta &= ~1;
	while (delta--)
	{
		if (IS_HD)
		{
			if (delta % 8 == 0 && delta != 0)
			{ // every eighth dot and not the last dot
				s.origin.x = pt.x;
				s.origin.y = pt.y;
				DrawFilledStamp(&s);
			}
		}
		else if (!(delta & 1))
		{
			DrawPoint(&pt);
		}

		if ((xerror -= dx) <= 0)
		{
			pt.x += xincr;
			xerror += cycle;
		}
		if ((yerror -= dy) <= 0)
		{
			pt.y += yincr;
			yerror += cycle;
		}
	}
}

static void
GetSphereRect(FLEET_INFO* FleetPtr, GFXRECT* pRect, GFXRECT* pRepairRect)
{
	long diameter;

	diameter = (long)(FleetPtr->known_strength * SPHERE_RADIUS_INCREMENT);
	pRect->extent.width = UNIVERSE_TO_DISPX(diameter)
						- UNIVERSE_TO_DISPX(0);
	if (pRect->extent.width < 0)
	{
		pRect->extent.width = -pRect->extent.width;
	}
	else if (pRect->extent.width == 0)
	{
		pRect->extent.width = RES_SCALE(1);
	}
	pRect->extent.height = UNIVERSE_TO_DISPY(diameter)
						 - UNIVERSE_TO_DISPY(0);
	if (pRect->extent.height < 0)
	{
		pRect->extent.height = -pRect->extent.height;
	}
	else if (pRect->extent.height == 0)
	{
		pRect->extent.height = RES_SCALE(1);
	}

	pRect->corner.x = UNIVERSE_TO_DISPX(FleetPtr->known_loc.x);
	pRect->corner.y = UNIVERSE_TO_DISPY(FleetPtr->known_loc.y);
	pRect->corner.x -= pRect->extent.width >> 1;
	pRect->corner.y -= pRect->extent.height >> 1;

	{
		TEXT t;
		STRING locString;

		t.baseline.x = pRect->corner.x + (pRect->extent.width >> 1);
		t.baseline.y = pRect->corner.y + (pRect->extent.height >> 1)
					 - RES_SCALE(1);
		t.align = ALIGN_CENTER;
		locString = SetAbsStringTableIndex(FleetPtr->race_strings, 1);
		t.CharCount = GetStringLength(locString);
		t.pStr = (char*)GetStringAddress(locString);
		TextRect(&t, pRepairRect, nullptr);

		if (pRepairRect->corner.x <= 0)
		{
			pRepairRect->corner.x = RES_SCALE(1);
		}
		else if (pRepairRect->corner.x + pRepairRect->extent.width >= SIS_SCREEN_WIDTH)
		{
			pRepairRect->corner.x =
				SIS_SCREEN_WIDTH - pRepairRect->extent.width
				- RES_SCALE(1);
		}
		if (pRepairRect->corner.y <= 0)
		{
			pRepairRect->corner.y = RES_SCALE(1);
		}
		else if (pRepairRect->corner.y + pRepairRect->extent.height >= SIS_SCREEN_HEIGHT)
		{
			pRepairRect->corner.y =
				SIS_SCREEN_HEIGHT - pRepairRect->extent.height
				- RES_SCALE(1);
		}

		BoxUnion(pRepairRect, pRect, pRepairRect);
		pRepairRect->extent.width += RES_SCALE(1);
		pRepairRect->extent.height += RES_SCALE(1);
	}
}

// For showing the War-Era situation in starmap
static void
GetWarEraSphereRect(FLEET_INFO* FleetPtr, GFXRECT* pRect, GFXRECT* pRepairRect)
{
	long diameter = (long)(WarEraStrength(FleetPtr->SpeciesID) * SPHERE_RADIUS_INCREMENT);
	GFXPOINT loc = SeedFleetLocation(FleetPtr, plot_map, WAR_ERA);

	pRect->extent.width = UNIVERSE_TO_DISPX(diameter)
						- UNIVERSE_TO_DISPX(0);
	if (pRect->extent.width < 0)
	{
		pRect->extent.width = -pRect->extent.width;
	}
	else if (pRect->extent.width == 0)
	{
		pRect->extent.width = RES_SCALE(1);
	}
	pRect->extent.height = UNIVERSE_TO_DISPY(diameter)
						 - UNIVERSE_TO_DISPY(0);
	if (pRect->extent.height < 0)
	{
		pRect->extent.height = -pRect->extent.height;
	}
	else if (pRect->extent.height == 0)
	{
		pRect->extent.height = RES_SCALE(1);
	}

	pRect->corner.x = UNIVERSE_TO_DISPX(loc.x);
	pRect->corner.y = UNIVERSE_TO_DISPY(loc.y);
	pRect->corner.x -= pRect->extent.width >> 1;
	pRect->corner.y -= pRect->extent.height >> 1;

	if (pRepairRect->corner.x <= 0)
	{
		pRepairRect->corner.x = RES_SCALE(1);
	}
	else if (pRepairRect->corner.x + pRepairRect->extent.width >= SIS_SCREEN_WIDTH)
	{
		pRepairRect->corner.x = SIS_SCREEN_WIDTH - pRepairRect->extent.width - RES_SCALE(1);
	}

	if (pRepairRect->corner.y <= 0)
	{
		pRepairRect->corner.y = RES_SCALE(1);
	}
	else if (pRepairRect->corner.y + pRepairRect->extent.height >= SIS_SCREEN_HEIGHT)
	{
		pRepairRect->corner.y = SIS_SCREEN_HEIGHT - pRepairRect->extent.height - RES_SCALE(1);
	}

	BoxUnion(pRepairRect, pRect, pRepairRect);
	pRepairRect->extent.width += RES_SCALE(1);
	pRepairRect->extent.height += RES_SCALE(1);
}

static unsigned int
FuelRequiredTo(GFXPOINT dest)
{
	uint16_t fuel_required;
	uint32_t f;
	GFXPOINT pt;

	if (!inHQSpace())
	{
		pt = CurStarDescPtr->star_pt;
	}
	else
	{
		pt.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
		pt.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
	}

	pt.x -= dest.x;
	pt.y -= dest.y;

	f = (uint32_t)((long)pt.x * pt.x + (long)pt.y * pt.y);
	if (f == 0 || GET_GAME_STATE(ARILOU_SPACE_SIDE) > 1)
	{
		fuel_required = 0;
	}
	else
	{
		fuel_required = square_root(f) + (FUEL_TANK_SCALE / 20);
	}

	return fuel_required;
}

// Begin Malin's fuel to Sol ellipse code. Edited by Kruzen
#define MATH_ROUND(X) ((X) + ((int)((X) + 0.5) > (X) ? 1 : 0))

GFXPOINT
GetPointOfEllipse(double a, double b, double radian)
{
	double t[2] = {a * cos(radian), b * sin(radian)};
	return GFXPOINT {(int16_t)MATH_ROUND(t[0]), (int16_t)MATH_ROUND(t[1])};
}

GFXPOINT
ShiftPoint(GFXPOINT p, GFXPOINT s)
{
	return p + s;
}

GFXPOINT
RotatePoint(GFXPOINT p, GFXPOINT Pivot, double radian)
{
	double d[2] = {static_cast<double>(p.x - Pivot.x), static_cast<double>(p.y - Pivot.y)};
	double cosine[2] = {cos(radian), sin(radian)};
	double x = Pivot.x + (d[0] * cosine[0] - d[1] * cosine[1]);
	double y = Pivot.y + (d[0] * cosine[1] + d[1] * cosine[0]);

	return GFXPOINT {(int16_t)MATH_ROUND(x), (int16_t)MATH_ROUND(y)};
}

// Kruzen: Merged together with overflow check. Functions above are redundant
GFXPOINT
CalcEllipsePoint(double a, double b, double rad_one, double rad_two, GFXPOINT Pivot)
{
	double q[2] = {a * cos(rad_one), b * sin(rad_one)};
	double w[2] = {MATH_ROUND(q[0]) + Pivot.x, MATH_ROUND(q[1]) + Pivot.y};

	double d[2] = {w[0] - Pivot.x, w[1] - Pivot.y};
	double cosine[2] = {cos(rad_two), sin(rad_two)};
	double x = Pivot.x + (d[0] * cosine[0] - d[1] * cosine[1]);
	double y = Pivot.y + (d[0] * cosine[1] + d[1] * cosine[0]);

	x = MATH_ROUND(x);
	if (x > 32767.0)
	{
		x = 32767.0;
	}
	if (x < -32768.0)
	{
		x = -32768.0;
	}

	y = MATH_ROUND(y);
	if (y > 32767.0)
	{
		y = 32767.0;
	}
	if (y < -32768.0)
	{
		y = -32768.0;
	}

	return GFXPOINT {UNIVERSE_TO_DISPX2(x), UNIVERSE_TO_DISPY2(y)};
}

bool onScreen(LINE* l, bool ignoreX, bool ignoreY)
{
	return !((l->first.x < 0 && l->second.x < 0 && !ignoreX)
			 || (l->first.x >= SIS_SCREEN_WIDTH
				 && l->second.x >= SIS_SCREEN_WIDTH && !ignoreX)
			 || (l->first.y < 0 && l->second.y < 0 && !ignoreY)
			 || (l->first.y >= SIS_SCREEN_HEIGHT
				 && l->second.y >= SIS_SCREEN_HEIGHT && !ignoreY));
}

static void
DrawNoReturnZone(void)
{
	double dist;
	GFXPOINT sol, sis;
	double halfFuel = GLOBAL_SIS(FuelOnBoard) / 2;

	sol = plot_map[SOL_DEFINED].star_pt;
	sis = GFXPOINT {(int16_t)LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x)),
					(int16_t)LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y))};

	dist = (double)FuelRequiredTo(sol) / 2;

	if (dist <= halfFuel)
	{ // do not draw ellipse when fuel is not enough to reach Sol
		GFXPOINT curr, center, rmax_y, rmin_y;
		double i, Step, ry, rotation;

		ry = sqrt(pow(halfFuel, 2) - pow(dist, 2));

		// on max zoom ellipse edge becomes wobbly, so we cut num of
		// iterations in half on zoomLevel 3 and 4
		Step = (M_PI / (180.0f / (zoomLevel > 2 ? 2 : 1)));
		center = MAKE_POINT((sis.x + sol.x) / 2, (sis.y + sol.y) / 2);
		rotation = atan2(sol.y - sis.y, sol.x - sis.x);

		rmax_y = GFXPOINT {-1, -1};
		rmin_y = GFXPOINT {(int16_t)SIS_SCREEN_WIDTH, (int16_t)SIS_SCREEN_HEIGHT};

		for (i = 0; i < M_PI * 2; i += Step)
		{
			curr = CalcEllipsePoint(halfFuel, ry, i, rotation, center);
			if (curr.y > rmax_y.y)
			{
				rmax_y = curr;
			}

			if (curr.y < rmin_y.y)
			{
				rmin_y = curr;
			}
		}

		if (rmax_y.y >= 0 || rmin_y.y < SIS_SCREEN_HEIGHT)
		{ // If the ellipse is completely off screen - drop it
			LINE L;
			LINE tempLine;
			GFXPOINT prev = CalcEllipsePoint(halfFuel, ry, i - Step, rotation, center);
			int16_t dy;
			double err = ((double)rmax_y.x - (double)rmin_y.x)
					   / ((double)rmax_y.y - (double)rmin_y.y);

			for (i = 0; i < M_PI * 2; i += Step)
			{
				L.first = CalcEllipsePoint(halfFuel, ry, i, rotation, center);
				L.second.x = rmax_y.x
						   - (int16_t)(err * (rmax_y.y - L.first.y));
				L.second.y = L.first.y;

				if (onScreen(&L, false, false))
				{
					DrawLine(&L, 1);
				}

				dy = L.first.y - prev.y;

				MAKE_LINE(&tempLine, L.first, prev);

				if ((abs(dy) > 1)
					&& onScreen(&tempLine, true,
								false))
				{
					LINE L2;
					int16_t iter;
					double y_err = ((double)L.first.x - (double)prev.x)
								 / ((double)L.first.y - (double)prev.y);

					if (dy < 0)
					{
						iter = -1;
					}
					else
					{
						iter = 1;
					}

					while (abs(dy) > 1)
					{
						L2.first.y = L2.second.y = prev.y + dy - iter;
						L2.first.x = L.first.x
								   - (int16_t)(y_err
											   * (L.first.y - L2.first.y));
						L2.second.x = rmax_y.x
									- (int16_t)(err * (rmax_y.y - L2.first.y));

						if (onScreen(&L2, false, false))
						{
							DrawLine(&L2, 1);
						}

						dy -= iter;
					}
				}
				prev = L.first;
			}
		}
	}
}

static void
GetFuelRect(DRECT* r, int32_t diameter, GFXPOINT corner)
{ // Operating with DRECT because of overflows in HD on max zoom
	int32_t x, y, width, height;

	if (diameter < 0)
	{
		diameter = 0;
	}

	// Cap the diameter to a sane range
	if (diameter > MAX_X_UNIVERSE * 4)
	{
		diameter = MAX_X_UNIVERSE * 4;
	}

	// Calculate in case of overflow
	width = RES_SCALE(signedDivWithError(((diameter - mapOrigin.x) << zoomLevel)
											 * ORIG_SIS_SCREEN_WIDTH,
										 MAX_X_UNIVERSE + MAP_FIT_X)
					  + ((ORIG_SIS_SCREEN_WIDTH - 1) >> 1))
		  - UNIVERSE_TO_DISPX(0); // Cannot overflow in current HD resolution

	if (width < 0)
	{
		width = -width;
	}

	height = RES_SCALE(signedDivWithError(((mapOrigin.y - diameter) << zoomLevel)
											  * ORIG_SIS_SCREEN_HEIGHT,
										  MAX_Y_UNIVERSE + 2)
					   + ((ORIG_SIS_SCREEN_HEIGHT - 1) >> 1))
		   - UNIVERSE_TO_DISPY(0); // Cannot overflow in current HD resolution

	if (height < 0)
	{
		height = -height;
	}

	// SIS cannot leave universe boundaries, so cannot overflow either
	x = UNIVERSE_TO_DISPX(corner.x) - (width >> 1);
	y = UNIVERSE_TO_DISPY(corner.y) - (height >> 1);

	r->corner.x = x;
	r->corner.y = y;
	r->extent.width = width;
	r->extent.height = height;
}

static void
DrawFuelCircle(bool secondary)
{
	DRECT r;
	GFXPOINT corner;
	Color OldColor;
	uint32_t OnBoardFuel = GLOBAL_SIS(FuelOnBoard);

	if (secondary)
	{
		OnBoardFuel -= FuelRequiredTo(GLOBAL(autopilot));
		corner = GLOBAL(autopilot);
	}
	else if (!inHQSpace())
	{
		corner = CurStarDescPtr->star_pt;
	}
	else
	{
		corner.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
		corner.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
	}

	GetFuelRect(&r, OnBoardFuel << 1, corner);

	if (secondary)
	{
		OldColor = SetContextForeGroundColor(DKGRAY_COLOR);
		DrawOval(&r, chooseIfHd(1, 6), false);
		SetContextForeGroundColor(OldColor);
	}
	else
	{
		/* Draw circle*/
		OldColor = SetContextForeGroundColor(STARMAP_FUEL_RANGE_COLOR);
		DrawFilledOval(&r);
		SetContextForeGroundColor(OldColor);

		if (testFlag(uqm::UQMOptions::read().optFuelRange, uqm::FuelRangeDisplay::FuelToSol))
		{
			OldColor =
				SetContextForeGroundColor(STARMAP_SECONDARY_RANGE_COLOR);
			if (pointsEqual(corner, plot_map[SOL_DEFINED].star_pt))
			{ // We are at Sol, foci are equal - draw a standard oval
				GetFuelRect(&r, OnBoardFuel, corner);
				DrawFilledOval(&r);
			}
			else
			{
				DrawNoReturnZone();
			}

			SetContextForeGroundColor(OldColor);
		}
	}
}

// Taleden code of drawing ellipse. Unused because not precise enough
/*static void
DrawFuelEllipse ()
{
	Color OldColor;
	GFXPOINT center, sol, sis;
	double ry, dist, angle;
	double halfFuel = GLOBAL_SIS (FuelOnBoard) / 2;

	sol = (GFXPOINT){ SOL_X, SOL_Y };
	sis = (GFXPOINT){ LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
			LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y)) };

	dist = FuelRequiredTo (sol) / 2;

	if (dist >= halfFuel)
		return;
	ry = sqrt (pow (halfFuel, 2) - pow (dist, 2));
	angle = atan2 (sis.y - sol.y, sis.x - sol.x) * 180.0 / M_PI;
	center = MAKE_POINT ((sis.x + sol.x) / 2, (sis.y + sol.y) / 2);

	// convert starmap coords to screen coords
	center.x = UNIVERSE_TO_DISPX (center.x);
	center.y = UNIVERSE_TO_DISPY (center.y);

	halfFuel = UNIVERSE_TO_DISPX (halfFuel) - UNIVERSE_TO_DISPX (0);
	if (halfFuel < 0)
		halfFuel = -halfFuel;
	ry = UNIVERSE_TO_DISPY (ry) - UNIVERSE_TO_DISPY (0);
	if (ry < 0)
		ry = -ry;

	// draw
	OldColor = SetContextForeGroundColor (STARMAP_SECONDARY_RANGE_COLOR);
	DrawRotatedEllipse (center.x, center.y, halfFuel, ry, angle, 1, 0);
	SetContextForeGroundColor (OldColor);
}*/

bool isHomeworld(uint8_t Index)
{
	bool raceBool = false;

	switch (Index)
	{
		case CHMMR_DEFINED:
			if (IsHomeworldKnown(CHMMR_HOME)
				&& (CheckAlliance(CHMMR_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case ORZ_DEFINED:
			if (IsHomeworldKnown(ORZ_HOME)
				&& (CheckAlliance(ORZ_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case PKUNK_DEFINED:
			if (IsHomeworldKnown(PKUNK_HOME)
				&& (CheckAlliance(PKUNK_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case SHOFIXTI_DEFINED:
			if (IsHomeworldKnown(SHOFIXTI_HOME)
				&& (CheckAlliance(SHOFIXTI_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case SPATHI_DEFINED:
			if (IsHomeworldKnown(SPATHI_HOME)
				&& (CheckAlliance(SPATHI_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case SUPOX_DEFINED:
			if (IsHomeworldKnown(SUPOX_HOME)
				&& (CheckAlliance(SUPOX_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case THRADD_DEFINED:
			if (IsHomeworldKnown(THRADDASH_HOME)
				&& (CheckAlliance(THRADDASH_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case UTWIG_DEFINED:
			if (IsHomeworldKnown(UTWIG_HOME)
				&& (CheckAlliance(UTWIG_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case VUX_DEFINED:
			if (IsHomeworldKnown(VUX_HOME)
				&& (CheckAlliance(VUX_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case YEHAT_DEFINED:
			if (IsHomeworldKnown(YEHAT_HOME)
				&& (CheckAlliance(YEHAT_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case DRUUGE_DEFINED:
			if (IsHomeworldKnown(DRUUGE_HOME)
				&& (CheckAlliance(DRUUGE_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case ILWRATH_DEFINED:
			if (IsHomeworldKnown(ILWRATH_HOME)
				&& (CheckAlliance(ILWRATH_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case MYCON_DEFINED:
			if (IsHomeworldKnown(MYCON_HOME)
				&& (CheckAlliance(MYCON_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case SLYLANDRO_DEFINED:
			if (IsHomeworldKnown(SLYLANDRO_HOME))
			{
				raceBool = true;
			}
			break;
		case UMGAH_DEFINED:
			if (IsHomeworldKnown(UMGAH_HOME)
				&& (CheckAlliance(UMGAH_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case ZOQFOT_DEFINED:
			if (IsHomeworldKnown(ZOQFOTPIK_HOME)
				&& (CheckAlliance(ZOQFOTPIK_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case SYREEN_DEFINED:
			if (IsHomeworldKnown(SYREEN_HOME)
				&& (CheckAlliance(SYREEN_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case ANDROSYNTH_DEFINED:
			if (IsHomeworldKnown(ANDROSYNTH_HOME)
				&& (CheckAlliance(ANDROSYNTH_SHIP) != DEAD_GUY || StarSeed))
			{
				raceBool = true;
			}
			break;
		case SOL_DEFINED:
		case START_COLONY_DEFINED:
			raceBool = true;
			break;
	}

	return raceBool;
}

const char*
markerBuf(const int star_index, const char* marker_state)
{
	static char buf[255];

	// marker_state is the middle part of the Game States
	// "SYS_VISITED_##" or "SYS_PLYR_MARKER_##" which is used to
	// differentiate between which kind of marker we're working with.

	fmt::format_to_sz_n(buf, "SYS_{}_{:02}", marker_state,
						star_index / 32);

	return buf;
}

bool isStarMarked(const int star_index, const char* marker_state)
{
	int starIndex = star_index;
	uint32_t starData;

	if (star_index == INTERNAL_STAR_INDEX)
	{
		starIndex = (uint16_t)(CurStarDescPtr - star_array);
	}

	starData = D_GET_GAME_STATE(markerBuf(starIndex, marker_state));

	return (starData >> (starIndex % 32)) & 1;
}

void setStarMarked(const int star_index, const char* marker_state)
{
	int starIndex = star_index;
	uint32_t starData;

	if (starIndex == INTERNAL_STAR_INDEX)
	{
		starIndex = (uint16_t)(CurStarDescPtr - star_array);
	}

	starData = D_GET_GAME_STATE(markerBuf(starIndex, marker_state));
	starData ^= (1 << (starIndex % 32));
	D_SET_GAME_STATE(markerBuf(starIndex, marker_state), starData);
}

static int16_t
CheckTextsIntersect(GFXRECT* curr, GFXRECT* prev)
{
	if (((curr->corner.x + curr->extent.width) <= prev->corner.x) || ((prev->corner.x + prev->extent.width) <= curr->corner.x))
	{
		return 0;
	}

	if (((curr->corner.y + curr->extent.height) <= prev->corner.y) || ((prev->corner.y + prev->extent.height) <= curr->corner.y))
	{
		return 0;
	}

	return ((prev->extent.height + RES_SCALE(1)) - abs(curr->corner.y - prev->corner.y));
}

static void
AdjustTextRect(GFXRECT* r, TEXT* t)
{
	int16_t offs;
	if (r->corner.x <= 0)
	{
		offs = r->corner.x - RES_SCALE(1);
		t->baseline.x -= offs;
		r->corner.x -= offs;
	}
	else if (r->corner.x + r->extent.width
			 >= SIS_SCREEN_WIDTH)
	{
		offs = (r->corner.x + r->extent.width)
			 - SIS_SCREEN_WIDTH + RES_SCALE(1);
		t->baseline.x -= offs;
		r->corner.x -= offs;
	}
	if (r->corner.y <= 0)
	{
		offs = r->corner.y - RES_SCALE(1);
		t->baseline.y -= offs;
		r->corner.y -= offs;
	}
	else if (r->corner.y + r->extent.height
			 >= SIS_SCREEN_HEIGHT)
	{
		offs = (r->corner.y + r->extent.height)
			 - SIS_SCREEN_HEIGHT + RES_SCALE(1);
		t->baseline.y -= offs;
		r->corner.y -= offs;
	}
}

static void
DrawRaceName(TEXT* t, Color* c)
{
	// The text color is slightly lighter than the color of
	// the SoI.
	c->r = (c->r >= 0xff - CC5TO8(0x03)) ?
			   0xff :
			   c->r + CC5TO8(0x03);
	c->g = (c->g >= 0xff - CC5TO8(0x03)) ?
			   0xff :
			   c->g + CC5TO8(0x03);
	c->b = (c->b >= 0xff - CC5TO8(0x03)) ?
			   0xff :
			   c->b + CC5TO8(0x03);

	SetContextForeGroundColor(*c);
	font_DrawText(t);
}

Color RaceColor(uint16_t index)
{
	if (uqm::UQMOptions::read().sphereColors == uqm::SphereOfInfluenceColors::Default)
	{
		static constexpr Color RaceColors[] = {RACE_COLORS};
		static constexpr uint16_t NumColors = sizeof(RaceColors) / sizeof(Color);
		if (index < NumColors)
		{
			return RaceColors[index];
		}
	}
	else if (uqm::UQMOptions::read().sphereColors == uqm::SphereOfInfluenceColors::Starseed)
	{
		static constexpr Color StarseedColors[] =
			{
				BUILD_COLOR(MAKE_RGB15_INIT(0x00, 0x00, 0x17), 0x00), // ARILOU_SHIP Bold blue
				BUILD_COLOR(MAKE_RGB15_INIT(0x10, 0x15, 0x19), 0x00), // CHMMR_SHIP Pale blue-grey
				BUILD_COLOR(MAKE_RGB15_INIT(0x09, 0x09, 0x1B), 0x00), // HUMAN_SHIP Blue
				BUILD_COLOR(MAKE_RGB15_INIT(0x0D, 0x00, 0x05), 0x00), // ORZ_SHIP Plum
				BUILD_COLOR(MAKE_RGB15_INIT(0x1C, 0x07, 0x19), 0x00), // PKUNK_SHIP Pinkunk
				BUILD_COLOR(MAKE_RGB15_INIT(0x0A, 0x06, 0x00), 0x00), // SHOFIXTI_SHIP Marsupial brown
				BUILD_COLOR(MAKE_RGB15_INIT(0x15, 0x12, 0x03), 0x00), // SPATHI_SHIP Mustard yellow
				BUILD_COLOR(MAKE_RGB15_INIT(0x05, 0x14, 0x01), 0x00), // SUPOX_SHIP Leaf green
				BUILD_COLOR(MAKE_RGB15_INIT(0x00, 0x06, 0x08), 0x00), // THRADDASH_SHIP Dark cyan
				BUILD_COLOR(MAKE_RGB15_INIT(0x1A, 0x16, 0x12), 0x00), // UTWIG_SHIP Beige
				BUILD_COLOR(MAKE_RGB15_INIT(0x06, 0x16, 0x0F), 0x00), // VUX_SHIP Very Ugly Aqua
				BUILD_COLOR(MAKE_RGB15_INIT(0x0A, 0x00, 0x11), 0x00), // YEHAT_SHIP Royal purple
				BUILD_COLOR(MAKE_RGB15_INIT(0x1A, 0x16, 0x08), 0x00), // MELNORME_SHIP Yellowish
				BUILD_COLOR(MAKE_RGB15_INIT(0x0F, 0x00, 0x00), 0x00), // DRUUGE_SHIP Crimson (tm)
				BUILD_COLOR(MAKE_RGB15_INIT(0x0E, 0x00, 0x0E), 0x00), // ILWRATH_SHIP Purple
				BUILD_COLOR(MAKE_RGB15_INIT(0x14, 0x00, 0x0C), 0x00), // MYCON_SHIP Fungal fuchsia
				BUILD_COLOR(MAKE_RGB15_INIT(0x1D, 0x00, 0x07), 0x00), // SLYLANDRO_SHIP Red Alert
				BUILD_COLOR(MAKE_RGB15_INIT(0x0B, 0x0C, 0x00), 0x00), // UMGAH_SHIP Olive
				BUILD_COLOR(MAKE_RGB15_INIT(0x00, 0x08, 0x00), 0x00), // URQUAN_SHIP Dark Green
				BUILD_COLOR(MAKE_RGB15_INIT(0x16, 0x0D, 0x00), 0x00), // ZOQFOTPIK_SHIP Orange
				BUILD_COLOR(MAKE_RGB15_INIT(0x15, 0x07, 0x03), 0x00), // SYREEN_SHIP Slightly-orange red
				BUILD_COLOR(MAKE_RGB15_INIT(0x06, 0x06, 0x06), 0x00), // BLACK_URQUAN_SHIP Dark grey
				BUILD_COLOR(MAKE_RGB15_INIT(0x09, 0x00, 0x15), 0x00), // ANDROSYNTH_SHIP Violet
				BUILD_COLOR(MAKE_RGB15_INIT(0x00, 0x15, 0x1C), 0x00), // CHENJESU_SHIP Cyan
				BUILD_COLOR(MAKE_RGB15_INIT(0x10, 0x10, 0x10), 0x00), // MMRNMHRM_SHIP Silver
				BUILD_COLOR(MAKE_RGB15_INIT(0x19, 0x10, 0x1F), 0x00), // YEHAT_REBEL_SHIP Lavender
			};

		static constexpr uint16_t NumColors = sizeof(StarseedColors) / sizeof(Color);

		if (index < NumColors)
		{
			return StarseedColors[index];
		}
	}
	// Hot pink (just a fail state but also reserved)
	return Color BUILD_COLOR(MAKE_RGB15_INIT(0x1C, 0x00, 0x0C), 0x00);
#if 0
	// A way to choose colors entirely "fairly", although colors don't really map
	// in such an even distribution.
		uint8_t i, j, k;
		for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) for (k = 0; k < 3; k++)
			race_colors[i * 9 + j * 3 + k] = (Color) BUILD_COLOR (MAKE_RGB15_INIT (14 * i + 1, 14 * j + 1, 14 * k + 1), 0x00);
#endif
}

static void
DrawStarMap(uint16_t race_update, GFXRECT* pClipRect)
{
#define GRID_DELTA 500
	int16_t i;
	uint16_t which_space;
	// long diameter;
	GFXRECT r, old_r;
	GFXPOINT oldOrigin = {0, 0};
	STAMP s;
	FRAME star_frame;
	STAR_DESC* SDPtr;
	bool draw_cursor;

	if (pClipRect == (GFXRECT*)-1)
	{
		pClipRect = 0;
		draw_cursor = false;
	}
	else
	{
		draw_cursor = true;
	}

	SetContext(SpaceContext);
	if (pClipRect)
	{
		GetContextClipRect(&old_r);
		pClipRect->corner.x += old_r.corner.x;
		pClipRect->corner.y += old_r.corner.y;
		SetContextClipRect(pClipRect);
		pClipRect->corner.x -= old_r.corner.x;
		pClipRect->corner.y -= old_r.corner.y;
		// Offset the origin so that we draw the correct gfx in the
		// cliprect
		oldOrigin = SetContextOrigin(MAKE_POINT(-pClipRect->corner.x,
												-pClipRect->corner.y));
	}

	if (transition_pending)
	{
		SetTransitionSource(nullptr);
	}
	BatchGraphics();

	which_space = GET_GAME_STATE(ARILOU_SPACE_SIDE);

	if (which_space <= 1)
	{
		SDPtr = &star_array[0];
		SetContextForeGroundColor(
			BUILD_COLOR(MAKE_RGB15(0x00, 0x00, 0x07), 0x57));
		SetContextBackGroundColor(BLACK_COLOR);
	}
	else
	{
		SDPtr = &star_array[NUM_SOLAR_SYSTEMS + 1];
		SetContextForeGroundColor(
			BUILD_COLOR(MAKE_RGB15(0x00, 0x0B, 0x00), 0x6D));
		SetContextBackGroundColor(
			BUILD_COLOR(MAKE_RGB15(0x00, 0x08, 0x00), 0x6E));
	}
	ClearDrawable();

	if (which_starmap != CONSTELLATION_MAP
		&& (race_update == 0 && which_space < 2)
		&& !(uqm::UQMOptions::read().infiniteFuel || GLOBAL_SIS(FuelOnBoard) == 0))
	{ // Draw the fuel range circle
		DrawFuelCircle(false);
	}

	{ // Horizontal lines
		r.corner.x = UNIVERSE_TO_DISPX(0);
		r.extent.width = (SIS_SCREEN_WIDTH << zoomLevel)
					   - (RES_SCALE(1) << zoomLevel);
		r.extent.height = RES_SCALE(1);

		for (i = MAX_Y_UNIVERSE; i >= 0; i -= GRID_DELTA)
		{
			r.corner.y = UNIVERSE_TO_DISPY(i);
			DrawFilledRectangle(&r);
		}

		r.corner.y = UNIVERSE_TO_DISPY(0);
		DrawFilledRectangle(&r);

		// Vertical lines
		r.corner.y = UNIVERSE_TO_DISPY(MAX_Y_UNIVERSE) + RES_SCALE(1);
		r.extent.width = RES_SCALE(1);
		r.extent.height = (SIS_SCREEN_HEIGHT << zoomLevel) - RES_SCALE(1);

		for (i = 0; i < MAX_Y_UNIVERSE; i += GRID_DELTA)
		{
			r.corner.x = UNIVERSE_TO_DISPX(i);
			DrawFilledRectangle(&r);
		}

		// Edge rounding error compensation
		// so the bar wouldn't leak over the edge
		r.corner.x = UNIVERSE_TO_DISPX(MAX_X_UNIVERSE);
		r.corner.y = UNIVERSE_TO_DISPY(MAX_Y_UNIVERSE);
		r.extent.height = (SIS_SCREEN_HEIGHT << zoomLevel) + RES_SCALE(1);
		if (r.extent.height - RES_SCALE(1)
			> (-(UNIVERSE_TO_DISPY(MAX_Y_UNIVERSE)
				 - UNIVERSE_TO_DISPY(0))))
		{
			r.extent.height -= RES_SCALE(1);
		}
		DrawFilledRectangle(&r);
	}

	if (which_starmap != CONSTELLATION_MAP
		&& (race_update == 0 && which_space < 2)
		&& !(uqm::UQMOptions::read().infiniteFuel || GLOBAL_SIS(FuelOnBoard) == 0)
		&& (testFlag(uqm::UQMOptions::read().optFuelRange, uqm::FuelRangeDisplay::FuelAtDestination))
		&& (GLOBAL(autopilot.x) != ~0 && GLOBAL(autopilot.y) != ~0))
	{ // Draw the autopilot fuel range circle (on top of the grid)
		DrawFuelCircle(true);
	}

	star_frame = SetRelFrameIndex(StarMapFrame, 2);
	if (which_space <= 1 && which_starmap != CONSTELLATION_MAP)
	{
		uint16_t index;
		uint16_t race_index = (race_update & 0x1F) - 1;
		HFLEETINFO hStarShip, hNextShip;
		NAMEPLATE nameplate[26];
		uint8_t currMax = 0;

		for (index = 0,
			hStarShip = GetHeadLink(&GLOBAL(avail_race_q));
			 hStarShip != 0; ++index, hStarShip = hNextShip)
		{
			FLEET_INFO* FleetPtr;

			FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
			hNextShip = _GetSuccLink(FleetPtr);

			if ((FleetPtr->known_strength && which_starmap != WAR_ERA_STARMAP) || (which_starmap == WAR_ERA_STARMAP && WarEraStrength(FleetPtr->SpeciesID) && index < NUM_BUILDABLE_SHIPS))
			{
				GFXRECT repair_r;

				if (which_starmap == WAR_ERA_STARMAP)
				{
					GetWarEraSphereRect(FleetPtr, &r, &repair_r);
				}
				else
				{
					GetSphereRect(FleetPtr, &r, &repair_r);
				}


				if (r.corner.x < SIS_SCREEN_WIDTH
					&& r.corner.y < SIS_SCREEN_HEIGHT
					&& r.corner.x + r.extent.width > 0
					&& r.corner.y + r.extent.height > 0
					&& (pClipRect == 0
						|| (repair_r.corner.x < pClipRect->corner.x
													+ pClipRect->extent.width
							&& repair_r.corner.y < pClipRect->corner.y
													   + pClipRect->extent.height
							&& repair_r.corner.x + repair_r.extent.width
								   > pClipRect->corner.x
							&& repair_r.corner.y + repair_r.extent.height
								   > pClipRect->corner.y)))
				{
					Color c;
					TEXT t;
					STRING locString;
					char yehat_rebels[] {"+Yehat+"};

					c = RaceColor(index);
					if (index == race_index)
					{
						SetContextForeGroundColor(WHITE_COLOR);
					}
					else
					{
						SetContextForeGroundColor(c);
					}

					{
						DRECT dr = RECT_TO_DRECT(r);
						DrawOval(&dr, 0, IS_HD);
					}

					if (isPC(uqm::UQMOptions::read().whichFonts))
					{
						SetContextFont(TinyFont);
					}
					else
					{
						SetContextFont(TinyFontBold);
					}

					t.baseline.x = r.corner.x + (r.extent.width >> 1);
					t.baseline.y = r.corner.y + (r.extent.height >> 1)
								 - RES_SCALE(1);
					t.align = ALIGN_CENTER;

					// For drawing War-Era starmap.
					if (which_starmap == WAR_ERA_STARMAP)
					{
						switch (index)
						{
							case PKUNK_SHIP:
							case THRADDASH_SHIP:
							case DRUUGE_SHIP:
								t.pStr = GAME_STRING(STAR_STRING_BASE + 132);
								t.CharCount = (uint16_t)strlen(t.pStr);
								break;
							case ANDROSYNTH_SHIP:
								locString = SetAbsStringTableIndex(
									FleetPtr->race_strings, 0);
								t.pStr = (char*)GetStringAddress(locString);
								t.CharCount = GetStringLength(locString);
								break;
							default:
								locString = SetAbsStringTableIndex(
									FleetPtr->race_strings, 1);
								t.CharCount = GetStringLength(locString);
								t.pStr = (char*)GetStringAddress(locString);
								break;
						}
					}
					else if (index == YEHAT_REBEL_SHIP && uqm::UQMOptions::read().sphereColors == uqm::SphereOfInfluenceColors::Starseed)
					{
						t.CharCount = 7;
						t.pStr = yehat_rebels;
					}
					else
					{
						locString = SetAbsStringTableIndex(
							FleetPtr->race_strings, 1);
						t.CharCount = GetStringLength(locString);
						t.pStr = (char*)GetStringAddress(locString);
					}

					TextRect(&t, &r, nullptr);

					if (index == race_index && race_update & IGNORE_MOVING_SOI)
					{
						AdjustTextRect(&r, &t);
						DrawRaceName(&t, &c);
					}
					else
					{
						nameplate[currMax].rect = r;
						nameplate[currMax].text = t;
						nameplate[currMax].index = index;
						currMax++;
					}
				}
			}
			else if (index == race_index && race_update & (PRE_DEATH_SOI | DEATH_SOI))
			{ // Kruzen: SoI is dead, but we need to fade nameplate
				TEXT t;
				STRING locString;
				GFXRECT repair_r;

				locString = SetAbsStringTableIndex(FleetPtr->race_strings, 1);
				t.CharCount = GetStringLength(locString);
				t.pStr = (char*)GetStringAddress(locString);

				GetSphereRect(FleetPtr, &r, &repair_r);

				t.baseline.x = r.corner.x + (r.extent.width >> 1);
				t.baseline.y = r.corner.y + (r.extent.height >> 1)
							 - RES_SCALE(1);
				t.align = ALIGN_CENTER;

				TextRect(&t, &r, nullptr);

				nameplate[currMax].rect = r;
				nameplate[currMax].text = t;
				nameplate[currMax].index = race_update - 1;
				currMax++;
			}
			UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		}

		if (currMax > 0)
		{
			uint8_t j, k;
			bool swapped;
			int16_t offs;
			TEXT t;
			Color c;
			uint8_t mid = currMax;

			for (j = 0; j < currMax - 1; j++)
			{ // Sort nameplates by Y-axis from top to bottom
				swapped = false;
				for (k = 0; k < currMax - j - 1; k++)
				{
					if (nameplate[k].rect.corner.y > nameplate[k + 1].rect.corner.y)
					{
						SwapPlates(&nameplate[k], &nameplate[k + 1]);
						swapped = true;
					}
				}
				// If no two elements were swapped by inner loop,
				// then break
				if (swapped == false)
				{
					break;
				}
			}

			for (j = 0; j < currMax; j++, mid--)
			{
				if (nameplate[j].index & PRE_DEATH_SOI)
				{
					c = WHITE_COLOR;
				}
				else if (nameplate[j].index & DEATH_SOI)
				{
					c = BUILD_SHADE_RGBA(0x80);
				}
				else
				{
					c = RaceColor(nameplate[j].index & 0x1F);
				}
				r = nameplate[j].rect;
				t = nameplate[j].text;

				AdjustTextRect(&r, &t);

				r.corner.y += RES_SCALE(1);
				r.extent.height -= RES_SCALE(2);

				if (r.corner.y > (SIS_SCREEN_HEIGHT >> 1))
				{
					break;
				}

				for (k = 0; k < j; k++)
				{
					if ((offs = CheckTextsIntersect(&r, &nameplate[k].rect)) != 0)
					{
						r.corner.y += offs;
						t.baseline.y += offs;
					}
				}
				nameplate[j].rect = r;

				DrawRaceName(&t, &c);
			}

			for (j = 1; j <= mid; j++)
			{
				if (nameplate[currMax - j].index & PRE_DEATH_SOI)
				{
					c = WHITE_COLOR;
				}
				else if (nameplate[currMax - j].index & DEATH_SOI)
				{
					c = BUILD_SHADE_RGBA(0x80);
				}
				else
				{
					c = RaceColor(nameplate[currMax - j].index & 0x1F);
				}
				r = nameplate[currMax - j].rect;
				t = nameplate[currMax - j].text;

				AdjustTextRect(&r, &t);

				r.corner.y += RES_SCALE(1);
				r.extent.height -= RES_SCALE(2);

				for (k = 1; k < j; k++)
				{
					if ((offs = CheckTextsIntersect(&r, &nameplate[currMax - k].rect)) != 0)
					{
						r.corner.y -= offs;
						t.baseline.y -= offs;
					}
				}
				nameplate[currMax - j].rect = r;

				DrawRaceName(&t, &c);
			}
		}
	}

	// Kruzen: This draws the constellation lines on the constellation
	// starmap.
	if (which_space <= 1 && which_starmap == CONSTELLATION_MAP)
	{
// Debug toy, it prints out a color map
#ifdef DEBUG_COLOR_PALETTE
		int i;
		int j;
		int k;
		for (i = 0; i < 32; i++)
		{
			for (j = 0; j < 32; j++)
			{
				for (k = 0; k < 32; k++)
				{
					TEXT t;
					char pStr[7];
					pStr[0] = (i / 16 ? '1' : '0');
					pStr[1] = (i % 16 > 9 ? i % 16 - 10 + 'A' : i % 16 + '0');
					pStr[2] = (j / 16 ? '1' : '0');
					pStr[3] = (j % 16 > 9 ? j % 16 - 10 + 'A' : j % 16 + '0');
					pStr[4] = (k / 16 ? '1' : '0');
					pStr[5] = (k % 16 > 9 ? k % 16 - 10 + 'A' : k % 16 + '0');
					pStr[6] = '\0';
					SetContextFont(TinyFontBold);
					SetContextForeGroundColor(BUILD_COLOR(MAKE_RGB15(i, j, k), 0x00));
					t.baseline.x = UNIVERSE_TO_DISPX(300 * j + (k / 16 ? 150 : 0));
					t.baseline.y = UNIVERSE_TO_DISPY(300 * i + k % 16 * 300 / 16) + 3;
					t.align = ALIGN_CENTER;
					t.pStr = pStr;
					t.CharCount = 10;
					font_DrawText(&t);
				}
			}
		}
#else
		Color oldColor;
		const GFXPOINT* CNPtr;
		LINE l;
		uint8_t c = 0x3F + IF_HD(0x11);
		CNPtr = &constel_array[0];

		oldColor = SetContextForeGroundColor(
			BUILD_COLOR_RGBA(c, c, c, 0xFF));

		while (CNPtr->x < MAX_X_UNIVERSE && CNPtr->y < MAX_Y_UNIVERSE)
		{ // Have to add 2 because of HD nature (can't get exact middle of 4x4)
			l.first.x = UNIVERSE_TO_DISPX(CNPtr->x) + IF_HD(2);
			l.first.y = UNIVERSE_TO_DISPY(CNPtr->y) + IF_HD(2);
			CNPtr++;
			l.second.x = UNIVERSE_TO_DISPX(CNPtr->x) + IF_HD(2);
			l.second.y = UNIVERSE_TO_DISPY(CNPtr->y) + IF_HD(2);
			CNPtr++;
			DrawLine(&l, 1);
		}
		SetContextForeGroundColor(oldColor);
#endif
	}

	// This draws markers over known alien Homeworlds
	// and labels the quasispace portal locations in hyperspace.
	if (which_space <= 1 && which_starmap == HOMEWORLDS_MAP)
	{
		uint16_t i;

		for (i = 0; i < (NUM_SOLAR_SYSTEMS + 1); ++i)
		{
			uint8_t Index = star_array[i].Index;
			if (isHomeworld(Index))
			{
				DrawMarker(star_array[i].star_pt, true);
			}
		}
		for (i = 0; i < NUM_HYPER_VORTICES; ++i)
		{
			if (!(GET_GAME_STATE(KNOW_QS_PORTAL) & (1 << i)))
			{
				continue;
			}
			TEXT t;
			char pStr[2];
			pStr[0] = 'A' + i;
			pStr[1] = 0;
			SetContextFont(TinyFontBold);
			// "Quasispace Green"
			SetContextForeGroundColor(BUILD_COLOR(MAKE_RGB15(0x00, 0x1A, 0x00), 0x2F));
			t.baseline.x = UNIVERSE_TO_DISPX(portal_map[i].star_pt.x);
			t.baseline.y = UNIVERSE_TO_DISPY(portal_map[i].star_pt.y) + 3;
			t.align = ALIGN_CENTER;
			t.pStr = pStr;
			t.CharCount = 2;
			font_DrawText(&t);
		}
	}

	// This draws markers over the Rainbow worlds
	if (which_space <= 1 && which_starmap == RAINBOW_MAP)
	{
		uint16_t rainbow_mask;

		rainbow_mask = MAKE_WORD(
			GET_GAME_STATE(RAINBOW_WORLD0),
			GET_GAME_STATE(RAINBOW_WORLD1));

		if (rainbow_mask == 0)
		{
			which_starmap = NORMAL_STARMAP;
		}
		else
		{
			uint16_t i;
			for (i = RAINBOW0_DEFINED; i <= RAINBOW9_DEFINED; i++)
			{
				if (rainbow_mask & (1 << (i - RAINBOW0_DEFINED)))
				{
					DrawMarker(plot_map[i].star_pt, true);
				}
			}
		}
	}

	do
	{ // Draws all the stars
		uint8_t star_type;
		static uint16_t i = 0;

		i = i >= NUM_SOLAR_SYSTEMS ? 0 : i;

		star_type = SDPtr->Type;

		s.origin.x = UNIVERSE_TO_DISPX(SDPtr->star_pt.x);
		s.origin.y = UNIVERSE_TO_DISPY(SDPtr->star_pt.y);
		if (which_space <= 1)
		{
			if (which_starmap == NORMAL_STARMAP
				&& isStarMarked(i, "PLYR_MARKER"))
			{ // This draws markers over tagged star systems
				DrawMarker(SDPtr->star_pt, 2);
			}

			if (uqm::UQMOptions::read().showVisitedStars && isStarMarked(i, "VISITED")
				&& which_starmap == NORMAL_STARMAP
				&& SDPtr->Index != SOL_DEFINED)
			{
				s.frame = SetRelFrameIndex(visitedStarsFrame,
										   STAR_TYPE(star_type)
												   * NUM_STAR_COLORS
											   + STAR_COLOR(star_type));
			}
			else
			{
				s.frame = SetRelFrameIndex(star_frame,
										   STAR_TYPE(star_type)
												   * NUM_STAR_COLORS
											   + STAR_COLOR(star_type));
			}

			++i;
		}
		else if (SDPtr->star_pt.x == ARILOU_HOME_X
				 && SDPtr->star_pt.y == ARILOU_HOME_Y)
		{
			s.frame = SetRelFrameIndex(star_frame,
									   (int)SUPER_GIANT_STAR * (int)NUM_STAR_COLORS + (int)GREEN_BODY); // TODO PragmaNull: this is weird enum arithmetic. helper function?
		}
		else
		{
			s.frame = SetRelFrameIndex(star_frame,
									   (int)GIANT_STAR * (int)NUM_STAR_COLORS + (int)GREEN_BODY); // TODO PragmaNull: this is weird enum arithmetic. helper function?
		}
		DrawStamp(&s);

		++SDPtr;
	} while (SDPtr->star_pt.x <= MAX_X_UNIVERSE
			 && SDPtr->star_pt.y <= MAX_Y_UNIVERSE);

	if (GET_GAME_STATE(ARILOU_SPACE))
	{
		if (which_space <= 1)
		{
			s.origin.x = UNIVERSE_TO_DISPX(plot_map[ARILOU_DEFINED].star_pt.x);
			s.origin.y = UNIVERSE_TO_DISPY(plot_map[ARILOU_DEFINED].star_pt.y);
		}
		else
		{
			s.origin.x = UNIVERSE_TO_DISPX(QUASI_SPACE_X);
			s.origin.y = UNIVERSE_TO_DISPY(QUASI_SPACE_Y);
		}
		s.frame = SetRelFrameIndex(star_frame,
								   (int)GIANT_STAR * (int)NUM_STAR_COLORS + (int)GREEN_BODY);
		DrawStamp(&s);
	}

	if (race_update == 0
		&& GLOBAL(autopilot.x) != ~0
		&& GLOBAL(autopilot.y) != ~0)
	{
		DrawAutoPilot(&GLOBAL(autopilot));
		if (IS_HD)
		{
			DrawMarker(GLOBAL(autopilot), false);
		}
	}

	if (transition_pending)
	{
		GetContextClipRect(&r);
		ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
		transition_pending = false;
	}

	if (pClipRect)
	{
		SetContextClipRect(&old_r);
		SetContextOrigin(oldOrigin);
	}

	if (race_update == 0 && draw_cursor)
	{
		GetContextClipRect(&r);
		LoadIntoExtraScreen(&r);
		DrawCursor(UNIVERSE_TO_DISPX(cursorLoc.x),
				   UNIVERSE_TO_DISPY(cursorLoc.y));
		flashCurrentLocation(nullptr, true);
	}

	UnbatchGraphics();
}

static void
EraseCursor(int16_t curs_x, int16_t curs_y)
{
	GFXRECT r;

	GetFrameRect(StarMapFrame, &r);

	if ((r.corner.x += curs_x) < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	else if (r.corner.x + r.extent.width >= SIS_SCREEN_WIDTH)
	{
		r.extent.width = SIS_SCREEN_WIDTH - r.corner.x;
	}
	if ((r.corner.y += curs_y) < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	else if (r.corner.y + r.extent.height >= SIS_SCREEN_HEIGHT)
	{
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;
	}

#ifndef OLD
	RepairBackRect(&r);
#else  /* NEW */
	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;
	DrawStarMap(0, &r);
#endif /* OLD */
}

static void
ZoomStarMap(int16_t dir)
{
#define MAX_ZOOM_SHIFT 4
	if (dir > 0)
	{
		if (zoomLevel < MAX_ZOOM_SHIFT)
		{
			++zoomLevel;
			mapOrigin = cursorLoc;

			DrawStarMap(0, nullptr);
			SleepThread(GameTicksPerSecond / 8);
		}
	}
	else if (dir < 0)
	{
		if (zoomLevel > 0)
		{
			if (zoomLevel > 1)
			{
				mapOrigin = cursorLoc;
			}
			else
			{
				mapOrigin.x = MAX_X_UNIVERSE >> 1;
				mapOrigin.y = MAX_Y_UNIVERSE >> 1;
			}
			--zoomLevel;

			DrawStarMap(0, nullptr);
			SleepThread(GameTicksPerSecond / 8);
		}
	}
}

static void
UpdateCursorLocation(int sx, int sy, const GFXPOINT* newpt)
{ // Kruzen: ORIG_SIS_SCREEN_WIDTH/HEIGHT for viewport follows cursor mode
	// We're scaling the result of s.origin afterwards, but calculating
	// everything in SD values. So we can use max zoom in HD
	STAMP s;
	GFXPOINT pt;

	pt.x = ORIG_UNIVERSE_TO_DISPX(cursorLoc.x);
	pt.y = ORIG_UNIVERSE_TO_DISPY(cursorLoc.y);

	if (newpt)
	{ // absolute move
		sx = sy = 0;
		s.origin.x = ORIG_UNIVERSE_TO_DISPX(newpt->x);
		s.origin.y = ORIG_UNIVERSE_TO_DISPY(newpt->y);
		cursorLoc = *newpt;
	}
	else
	{ // incremental move
		s.origin.x = pt.x + sx;
		s.origin.y = pt.y + sy;
	}

	if (sx)
	{
		cursorLoc.x = ORIG_DISP_TO_UNIVERSEX(s.origin.x) - sx;
		while (ORIG_UNIVERSE_TO_DISPX(cursorLoc.x) == pt.x)
		{
			cursorLoc.x += sx;
		}

		if (cursorLoc.x < 0)
		{
			cursorLoc.x = 0;
		}
		else if (cursorLoc.x > MAX_X_UNIVERSE)
		{
			cursorLoc.x = MAX_X_UNIVERSE;
		}

		s.origin.x = ORIG_UNIVERSE_TO_DISPX(cursorLoc.x);
	}

	if (sy)
	{
		cursorLoc.y = ORIG_DISP_TO_UNIVERSEY(s.origin.y) + sy;
		while (ORIG_UNIVERSE_TO_DISPY(cursorLoc.y) == pt.y)
		{
			cursorLoc.y -= sy;
		}

		if (cursorLoc.y < 0)
		{
			cursorLoc.y = 0;
		}
		else if (cursorLoc.y > MAX_Y_UNIVERSE)
		{
			cursorLoc.y = MAX_Y_UNIVERSE;
		}

		s.origin.y = ORIG_UNIVERSE_TO_DISPY(cursorLoc.y);
	}


	if (s.origin.x < 0 || s.origin.y < 0
		|| s.origin.x >= ORIG_SIS_SCREEN_WIDTH
		|| s.origin.y >= ORIG_SIS_SCREEN_HEIGHT)
	{
		mapOrigin = cursorLoc;
		DrawStarMap(0, nullptr);

		s.origin.x = ORIG_UNIVERSE_TO_DISPX(cursorLoc.x);
		s.origin.y = ORIG_UNIVERSE_TO_DISPY(cursorLoc.y);
	}
	else
	{
		BatchGraphics();
		EraseCursor(RES_SCALE(pt.x), RES_SCALE(pt.y));
		DrawCursor(RES_SCALE(s.origin.x), RES_SCALE(s.origin.y));
		flashCurrentLocation(nullptr, true);
		UnbatchGraphics();
	}
}

#define CURSOR_INFO_BUFSIZE 256

int starIndex(GFXPOINT starPt)
{
	uint16_t i;

	for (i = 0; i <= NUM_SOLAR_SYSTEMS; i++)
	{
		if (star_array[i].star_pt.x == starPt.x
			&& star_array[i].star_pt.y == starPt.y)
		{
			break;
		}
	}
	return i;
}

static void
UpdateCursorInfo(std::span<char> prevbuf)
{
	char buf[CURSOR_INFO_BUFSIZE] {};
	GFXPOINT pt;
	STAR_DESC* SDPtr;
	STAR_DESC* BestSDPtr;

	if (which_starmap == NORMAL_STARMAP)
	{ // "(Star Search: F6 | Toggle Maps: F7)"
		uqm::utf8StringCopy(buf, sizeof(buf), GAME_STRING(FEEDBACK_STRING_BASE + 2 + (is3DO(uqm::UQMOptions::read().whichFonts) || IS_PAD)));
	}
	else
	{
		uqm::utf8StringCopy(buf, sizeof(buf),
							GAME_STRING(FEEDBACK_STRING_BASE + 3 + which_starmap));
	}

	pt.x = UNIVERSE_TO_DISPX(cursorLoc.x);
	pt.y = UNIVERSE_TO_DISPY(cursorLoc.y);

	SDPtr = BestSDPtr = 0;
	while ((SDPtr = FindStar(SDPtr, &cursorLoc, 75, 75)))
	{
		if (UNIVERSE_TO_DISPX(SDPtr->star_pt.x) == pt.x
			&& UNIVERSE_TO_DISPY(SDPtr->star_pt.y) == pt.y
			&& (BestSDPtr == 0
				|| STAR_TYPE(SDPtr->Type) >= STAR_TYPE(BestSDPtr->Type)))
		{
			BestSDPtr = SDPtr;
		}
	}

	if (BestSDPtr)
	{
		// JMS: For masking the names of QS portals not yet entered.
		uint8_t whichPortal = BestSDPtr->Postfix - 133;

		// A star is near the cursor:
		// Snap cursor onto star
		cursorLoc = BestSDPtr->star_pt;

		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) >= 2
			&& !(GET_GAME_STATE(KNOW_QS_PORTAL) & (1 << whichPortal)))
		{
			// "UNKNOWN" string
			uqm::utf8StringCopy(buf, sizeof(buf),
								GAME_STRING(STAR_STRING_BASE + 132));
		}
		else if (GET_GAME_STATE(ARILOU_SPACE_SIDE) < 2 || whichPortal == NUM_HYPER_VORTICES)
		{
			// Hyperspace side, or Arilou homeworld name once known
			GetClusterName(BestSDPtr, buf);
		}
		else
		{
			// Use whichPortal (based on the Postfix of the star) to determine
			// which portal_map entity this maps to and pull the nearest star.
			// We then use that to construct the exit point name.
			// This means quasi star array must have rigidly ordered Postfix.
			// portal_map[whichPortal].star_pt - coords of the exit
			//                  nearest_star - STAR_DESC* of nearest constell.
			char starnameBuf[CURSOR_INFO_BUFSIZE] = "";
			uqm::utf8StringCopy(starnameBuf, sizeof(starnameBuf), GAME_STRING(portal_map[whichPortal].nearest_star->Postfix));
			// Abbreviate names longer than 11 at the 10th (or less)
			// non-vowel character.
			if (strlen(starnameBuf) > 11)
			{
				uint16_t i;
				for (i = 9; i > 5; i--)
				{
					if (starnameBuf[i] != 'a' && starnameBuf[i] != 'e' && starnameBuf[i] != 'i' && starnameBuf[i] != 'o' && starnameBuf[i] != 'u' && starnameBuf[i] != 'y')
					{
						break;
					}
				}
				starnameBuf[i + 1] = '.';
				starnameBuf[i + 2] = '\0';
			}
			snprintf(buf, sizeof(buf),
					 "[%c] To %05.1f : %05.1f (Near %.12s)",
					 'A' + whichPortal,
					 (float)portal_map[whichPortal].star_pt.x / 10,
					 (float)portal_map[whichPortal].star_pt.y / 10,
					 starnameBuf);
		}
	}
	else
	{ // No star found. Reset the coordinates to the cursor's location
		// Kruzen: bucket to avoid cursor misplacement due to
		// asymmetric DISP_TO_UNIVERSE functions
		int16_t bucket;

		bucket = DISP_TO_UNIVERSEX(pt.x);
		if (bucket < 0)
		{
			cursorLoc.x = 0;
		}
		else if (bucket > MAX_X_UNIVERSE)
		{
			cursorLoc.x = MAX_X_UNIVERSE;
		}
		bucket = DISP_TO_UNIVERSEY(pt.y);
		if (bucket < 0)
		{
			cursorLoc.y = 0;
		}
		else if (bucket > MAX_Y_UNIVERSE)
		{
			cursorLoc.y = MAX_Y_UNIVERSE;
		}
	}

	if (GET_GAME_STATE(ARILOU_SPACE))
	{
		GFXPOINT ari_pt;

		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
		{
			ari_pt = plot_map[ARILOU_DEFINED].star_pt;
		}
		else
		{
			ari_pt.x = QUASI_SPACE_X;
			ari_pt.y = QUASI_SPACE_Y;
		}

		if (UNIVERSE_TO_DISPX(ari_pt.x) == pt.x
			&& UNIVERSE_TO_DISPY(ari_pt.y) == pt.y)
		{
			cursorLoc = ari_pt;
			uqm::utf8StringCopy(buf, sizeof(buf),
								GAME_STRING(STAR_STRING_BASE + 132));
		}
	}

	DrawHyperCoords(cursorLoc);
	if (strncmp(buf, prevbuf.data(), sizeof(buf)) != 0)
	{
		uqm::strncpy_safe(prevbuf, buf);

		// Cursor is on top of a star. Display its name.
		if (BestSDPtr)
		{
			if (uqm::UQMOptions::read().showVisitedStars
				&& isStarMarked(starIndex(BestSDPtr->star_pt),
								"VISITED"))
			{
				char visBuf[CURSOR_INFO_BUFSIZE] = "";

				uqm::utf8StringCopy(visBuf, sizeof(visBuf), buf);
				// This is how you get rid of compiler warnings over string
				// length:  %.30s where "30" is a limit.  Enjoy. ~JSD~
				//snprintf(buf, sizeof buf, "%c %.251s %c", '(', visBuf, ')');
				fmt::format_to_sz_n(buf, "( {:251} )", visBuf);
			}

			DrawSISMessage(buf);
		}
		// Cursor is elsewhere.
		else
		{
			// In HS, display default star search button name.
			if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
			{
				GFXCONTEXT OldContext;
				OldContext = SetContext(OffScreenContext);

				if (which_starmap == WAR_ERA_STARMAP)
				{
					SetContextForeGroundColor(
						BUILD_COLOR(
							MAKE_RGB15(0x18, 0x00, 0x00), 0x00));
				}
				else
				{
					SetContextForeGroundColor(
						BUILD_COLOR(
							MAKE_RGB15(0x0E, 0xA7, 0xD9), 0x00));
				}

				DrawSISMessageEx(buf, -1, -1, DSME_MYCOLOR);
				SetContext(OldContext);
			}
			else
			{ // In QS, don't display star search button - the search is
				// unusable.
				const char* sz GAME_STRING(NAVIGATION_STRING_BASE + 1);
				uqm::strncpy_safe(buf, sz);
				DrawSISMessage(buf);
			}
		}
	}
}

static unsigned int
FuelRequired(void)
{
	return FuelRequiredTo(cursorLoc);
}

static void
UpdateFuelRequirement(void)
{
	char buf[80] {};
	uint16_t fuel_required = FuelRequired();

	fmt::format_to_n(buf, sizeof(buf) - 1, "{} {}.{}",
					 GAME_STRING(NAVIGATION_STRING_BASE + 4),
					 fuel_required / FUEL_TANK_SCALE,
					 (fuel_required % FUEL_TANK_SCALE) / 10);

	DrawStatusMessage(buf);
}

#define STAR_SEARCH_BUFSIZE 256

typedef struct starsearch_state
{
	// TODO: pMS field is probably not needed anymore
	MENU_STATE* pMS;
	char Text[STAR_SEARCH_BUFSIZE];
	char LastText[STAR_SEARCH_BUFSIZE];
	uint32_t LastChangeTime;
	int FirstIndex;
	int CurIndex;
	int LastIndex;
	bool SingleClust;
	bool SingleMatch;
	char Buffer[STAR_SEARCH_BUFSIZE];
	const char* Prefix;
	const char* Cluster;
	int PrefixLen;
	int ClusterLen;
	int ClusterPos;
	int SortedStars[NUM_SOLAR_SYSTEMS];
} STAR_SEARCH_STATE;

static int
compStarName(const void* ptr1, const void* ptr2)
{
	int index1;
	int index2;

	index1 = *(const int*)ptr1;
	index2 = *(const int*)ptr2;
	if (star_array[index1].Postfix != star_array[index2].Postfix)
	{
		return uqm::utf8StringCompare(GAME_STRING(star_array[index1].Postfix),
									  GAME_STRING(star_array[index2].Postfix));
	}

	if (star_array[index1].Prefix < star_array[index2].Prefix)
	{
		return -1;
	}

	if (star_array[index1].Prefix > star_array[index2].Prefix)
	{
		return 1;
	}

	return 0;
}

static void
SortStarsOnName(STAR_SEARCH_STATE* pSS)
{
	int i;
	int* sorted = pSS->SortedStars;

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
	{
		sorted[i] = i;
	}

	qsort(sorted, NUM_SOLAR_SYSTEMS, sizeof(int), compStarName);
}

static void
SplitStarName(STAR_SEARCH_STATE* pSS)
{
	char* buf = pSS->Buffer;
	char* next;
	char* sep = nullptr;

	pSS->Prefix = 0;
	pSS->PrefixLen = 0;
	pSS->Cluster = 0;
	pSS->ClusterLen = 0;
	pSS->ClusterPos = 0;

	// skip leading space
	for (next = buf; *next != '\0' && uqm::getCharFromString((const char**)&next) == (isPC(uqm::UQMOptions::read().whichFonts) ? UNICHAR_SPACE : UNICHAR_TAB);
		 buf = next)
		;
	if (*buf == '\0')
	{ // no text
		return;
	}

	pSS->Prefix = buf;

	// See if player gave a prefix
	for (buf = next; *next != '\0' && uqm::getCharFromString((const char**)&next) != (isPC(uqm::UQMOptions::read().whichFonts) ? UNICHAR_SPACE : UNICHAR_TAB);
		 buf = next)
		;
	if (*buf != '\0')
	{ // found possibly separating ' '
		sep = buf;
		// skip separating space
		for (buf = next; *next != '\0' && uqm::getCharFromString((const char**)&next) == (isPC(uqm::UQMOptions::read().whichFonts) ? UNICHAR_SPACE : UNICHAR_TAB);
			 buf = next)
			;
	}

	if (*buf == '\0')
	{ // reached the end -- cluster only
		pSS->Cluster = pSS->Prefix;
		pSS->ClusterLen = uqm::utf8StringCount(pSS->Cluster);
		pSS->ClusterPos = uqm::utf8StringCount(uqstl::string_view {pSS->Buffer, static_cast<size_t>(pSS->Cluster - pSS->Buffer)});
		pSS->Prefix = 0;
		return;
	}

	// consider the rest cluster name (whatever there is)
	pSS->Cluster = buf;
	pSS->ClusterLen = uqm::utf8StringCount(pSS->Cluster);
	pSS->ClusterPos = uqm::utf8StringCount(uqstl::string_view {pSS->Buffer, static_cast<size_t>(pSS->Cluster - pSS->Buffer)});
	*sep = '\0'; // split
	pSS->PrefixLen = uqm::utf8StringCount(pSS->Prefix);
}

static inline int
SkipStarCluster(int* sortedStars, int istar)
{
	int Postfix = star_array[sortedStars[istar]].Postfix;

	for (++istar; istar < NUM_SOLAR_SYSTEMS && star_array[sortedStars[istar]].Postfix == Postfix;
		 ++istar)
		;
	return istar;
}

static int
FindNextStarIndex(STAR_SEARCH_STATE* pSS, int from, bool WithinClust)
{
	int i;

	if (!pSS->Cluster)
	{
		return -1; // nothing to search for
	}

	for (i = from; i < NUM_SOLAR_SYSTEMS; ++i)
	{
		STAR_DESC* SDPtr = &star_array[pSS->SortedStars[i]];
		char FullName[STAR_SEARCH_BUFSIZE];
		char* ClusterName = GAME_STRING(SDPtr->Postfix);
		const char* sptr;
		const char* dptr;
		int dlen;
		int c;

		dlen = uqm::utf8StringCount(ClusterName);
		if (pSS->ClusterLen > dlen)
		{ // no match, skip the rest of cluster
			i = SkipStarCluster(pSS->SortedStars, i) - 1;
			continue;
		}

		for (c = 0, sptr = pSS->Cluster, dptr = ClusterName;
			 c < pSS->ClusterLen; ++c)
		{
			uqm::UniChar sc = uqm::getCharFromString(&sptr);
			uqm::UniChar dc = uqm::getCharFromString(&dptr);

			if (uqm::toUpperChar(sc) != uqm::toUpperChar(dc))
			{
				break;
			}
		}

		if (c < pSS->ClusterLen)
		{ // no match here, skip the rest of cluster
			i = SkipStarCluster(pSS->SortedStars, i) - 1;
			continue;
		}

		if (pSS->Prefix && !SDPtr->Prefix)
		{
			// we were given a prefix but found a singular star;
			// that is a no match
			continue;
		}

		if (WithinClust)
		{
			// searching within clusters; any prefix is a match
			break;
		}

		if (!pSS->Prefix)
		{ // searching for cluster name only
			// return only the first stars in a cluster
			if (i == 0 || SDPtr->Postfix != star_array[pSS->SortedStars[i - 1]].Postfix)
			{ // found one
				break;
			}
			else
			{ // another star in the same cluster, skip cluster
				i = SkipStarCluster(pSS->SortedStars, i) - 1;
				continue;
			}
		}

		// check prefix
		GetClusterName(SDPtr, FullName);
		dlen = uqm::utf8StringCount(FullName);
		if (pSS->PrefixLen > dlen)
		{
			continue;
		}

		for (c = 0, sptr = pSS->Prefix, dptr = FullName;
			 c < pSS->PrefixLen; ++c)
		{
			uqm::UniChar sc = uqm::getCharFromString(&sptr);
			uqm::UniChar dc = uqm::getCharFromString(&dptr);

			if (uqm::toUpperChar(sc) != uqm::toUpperChar(dc))
			{
				break;
			}
		}

		if (c >= pSS->PrefixLen)
		{
			break; // found one
		}
	}

	return (i < NUM_SOLAR_SYSTEMS) ? i : -1;
}

static void
DrawMatchedStarName(TEXTENTRY_STATE* pTES)
{
	STAR_SEARCH_STATE* pSS = (STAR_SEARCH_STATE*)pTES->CbParam;
	char buf[STAR_SEARCH_BUFSIZE] {};
	int16_t ExPos = 0;
	int16_t CurPos = -1;
	STAR_DESC* SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
	uint16_t flags;

	if (pSS->SingleClust || pSS->SingleMatch)
	{ // draw full star name
		GetClusterName(SDPtr, buf);
		ExPos = -1;
		flags = DSME_SETFR;
	}
	else
	{ // draw substring match
		char* pstr = buf;

		uqm::strncpy_safe(buf, pSS->Text);
		ExPos = pSS->ClusterPos;
		pstr = uqm::skipUTF8Chars(pstr, pSS->ClusterPos);

		uqm::strncpy_safe({pstr, sizeof(buf) - uqstl::distance(buf, pstr)}, pSS->Text);
		ExPos += pSS->ClusterLen;
		CurPos = pTES->CursorPos;

		flags = DSME_CLEARFR;
		if (pTES->JoystickMode)
		{
			flags |= DSME_BLOCKCUR;
		}
	}

	DrawSISMessageEx(buf, CurPos, ExPos, flags);
	DrawHyperCoords(cursorLoc);
}

static void
MatchNextStar(STAR_SEARCH_STATE* pSS, bool Reset)
{
	if (Reset)
	{
		pSS->FirstIndex = -1; // reset cache
	}

	if (pSS->FirstIndex < 0)
	{ // first time after changes
		pSS->CurIndex = -1;
		pSS->LastIndex = -1;
		pSS->SingleClust = false;
		pSS->SingleMatch = false;
		uqm::strncpy_safe(pSS->Buffer, pSS->Text);
		SplitStarName(pSS);
	}

	pSS->CurIndex = FindNextStarIndex(pSS, pSS->CurIndex + 1,
									  pSS->SingleClust);
	if (pSS->FirstIndex < 0) // first search
	{
		pSS->FirstIndex = pSS->CurIndex;
	}

	if (pSS->CurIndex >= 0)
	{ // remember as last (searching forward-only)
		pSS->LastIndex = pSS->CurIndex;
	}
	else
	{ // wrap around
		pSS->CurIndex = pSS->FirstIndex;

		if (pSS->FirstIndex == pSS->LastIndex && pSS->FirstIndex != -1)
		{
			if (!pSS->Prefix)
			{ // only one cluster matching
				pSS->SingleClust = true;
			}
			else
			{ // exact match
				pSS->SingleMatch = true;
			}
		}
	}
}

static bool
OnStarNameChange(TEXTENTRY_STATE* pTES)
{
	STAR_SEARCH_STATE* pSS = (STAR_SEARCH_STATE*)pTES->CbParam;
	uint16_t flags;
	bool ret = true;

	if (strncmp(pSS->Text, pSS->LastText, sizeof(pSS->Text)) != 0)
	{ // string changed
		pSS->LastChangeTime = GetTimeCounter();
		uqm::strncpy_safe(pSS->LastText, pSS->Text);

		// reset the search
		MatchNextStar(pSS, true);
	}

	if (pSS->CurIndex < 0)
	{ // nothing found
		if (pSS->Text[0] == '\0')
		{
			flags = DSME_SETFR;
		}
		else
		{
			flags = DSME_CLEARFR;
		}
		if (pTES->JoystickMode)
		{
			flags |= DSME_BLOCKCUR;
		}

		ret = DrawSISMessageEx(pSS->Text, pTES->CursorPos, -1, flags);
	}
	else
	{
		STAR_DESC* SDPtr;

		// move the cursor to the found star
		SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
		UpdateCursorLocation(0, 0, &SDPtr->star_pt);

		DrawMatchedStarName(pTES);
		UpdateFuelRequirement();
	}

	return ret;
}

static bool
OnStarNameFrame(TEXTENTRY_STATE* pTES)
{
	STAR_SEARCH_STATE* pSS = (STAR_SEARCH_STATE*)pTES->CbParam;

	if (PulsedInputState.menu[KEY_MENU_NEXT])
	{ // search for next match
		STAR_DESC* SDPtr;

		MatchNextStar(pSS, false);

		if (pSS->CurIndex < 0)
		{ // nothing found
			if (PulsedInputState.menu[KEY_MENU_NEXT])
			{
				PlayMenuSound(MENU_SOUND_FAILURE);
			}
			return true;
		}

		// move the cursor to the found star
		SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
		UpdateCursorLocation(0, 0, &SDPtr->star_pt);

		DrawMatchedStarName(pTES);
		UpdateFuelRequirement();
	}

	flashCurrentLocation(nullptr, false);

	SleepThread(GameTicksPerSecond / 30);

	return true;
}

bool coords_only(char* s)
{
	uint8_t i, count = 0;
	uint8_t countD = 0, countC = 0;
	uint8_t j = (uint8_t)strlen(s);
	//const char *pattern = "^\d*(\.\d+)?:\d*(\.\d+)?$";

	for (i = 0; i < j; i++)
	{
		if (s[i] == '.')
		{
			count++;
			countD++;
		}
		else if (s[i] == ':')
		{
			count++;
			countC++;
		}
		else if (isdigit(s[i]) == 0)
		{
			return false;
		}
		else
		{
			count++;
		}
	}
	return i == j && countD <= 2 && countC == 1;
}

static bool
DoStarSearch(MENU_STATE* pMS)
{
	TEXTENTRY_STATE tes;
	STAR_SEARCH_STATE* pss;
	bool success;

	pss = (STAR_SEARCH_STATE*)HMalloc(sizeof(*pss));
	if (!pss)
	{
		return false;
	}

	DrawSISMessageEx("", 0, 0, DSME_SETFR);

	TextEntry3DO = (bool)is3DO(uqm::UQMOptions::read().whichFonts);

	pss->pMS = pMS;
	pss->LastChangeTime = 0;
	pss->Text[0] = '\0';
	pss->LastText[0] = '\0';
	pss->FirstIndex = -1;
	SortStarsOnName(pss);

	// text entry setup
	tes.Initialized = false;
	tes.BaseStr = pss->Text;
	tes.MaxSize = sizeof(pss->Text);
	tes.CursorPos = 0;
	tes.CbParam = pss;
	tes.ChangeCallback = OnStarNameChange;
	tes.FrameCallback = OnStarNameFrame;

	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay();
	success = DoTextEntry(&tes);

	if (coords_only(tes.BaseStr))
	{
		GFXPOINT coord;

		{
			uqstl::vector<uqstl::string> parts;
			uqm::tokenize(uqstl::string_view {tes.BaseStr}, parts, ':', false);
			coord.x = (int16_t)(atof(parts[0].c_str()) * 10);
			coord.y = (int16_t)(atof(parts[1].c_str()) * 10);
		}

		if (coord.x > MAX_X_UNIVERSE || coord.y > MAX_Y_UNIVERSE
			|| coord.x < 0 || coord.y < 0)
		{
			success = false;
		}
		else
		{
			UpdateCursorLocation(0, 0, &coord);
		}

		success = true;
	}

	DrawSISMessageEx(pss->Text, -1, -1, DSME_CLEARFR);

	HFree(pss);

	TextEntry3DO = false;

	return success;
}

void DoBubbleWarp(bool UseFuel)
{
	PlayMenuSound(MENU_SOUND_BUBBLEWARP);

	if (UseFuel)
	{
		DeltaSISGauges(0, -(int)FuelRequired(), 0);
	}

	if (lowByte(GLOBAL(CurrentActivity)) == IN_INTERPLANETARY)
	{
		// We're in a solar system; exit it.
		GLOBAL(CurrentActivity) |= END_INTERPLANETARY;
		// Set a hook to move to the new location:
		debugHook = doInstantMove;
	}
	else
	{ // Move to the new location immediately.
		doInstantMove();
	}
}


static void
AdvancedAutoPilot(void)
{
	const uint16_t portalFuelCost {difficultyCase<uint16_t>(5, 20, 20)};
	// (Based on the Python QuasiSpace Calculator by Lukrative525
	// https://www.reddit.com/r/starcontrol/comments/zpyhkc/quasispace_calculator/
	GFXPOINT current_position;
	GFXPOINT destination = GLOBAL(autopilot);
	GFXPOINT portal_coordinates;
	double distance, fuel_no_portal, fuel_with_portal;
	double minimum = 0.0;
	uint8_t i;
	uint8_t index = 0;
	uint16_t KnownQSPortals = GET_GAME_STATE(KNOW_QS_PORTAL);

	current_position.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
	current_position.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));

	if (pointsEqual(current_position, destination))
	{
		return;
	}

	for (i = 0; i < NUM_HYPER_VORTICES; i++)
	{
		distance = ptDistance(destination, portal_map[i].star_pt);

		if (!isDifficulty(uqm::Difficulty::Easy) && !(KnownQSPortals & (1 << i)))
		{
			distance = MAX_X_UNIVERSE * MAX_Y_UNIVERSE;
		}

		if (i == 0 || distance < minimum)
		{
			minimum = distance;
			index = i + 1;
		}
	}

	portal_coordinates = star_array[NUM_SOLAR_SYSTEMS + index].star_pt;

	fuel_no_portal = ptDistance(current_position, destination) / 100;
	fuel_with_portal = minimum / 100 + portalFuelCost;

	if (fuel_no_portal < fuel_with_portal)
	{
		return;
	}

	SaveAdvancedAutoPilot(destination);
	SaveAdvancedQuasiPilot(portal_coordinates);

	if (playerInSolarSystem())
	{
		GLOBAL(autopilot) = current_position;
	}

	if (inHyperSpace())
	{
		InvokeSpawner();
	}
}

static bool
DoMoveCursor(MENU_STATE* pMS)
{
#define MIN_ACCEL_DELAY (GameTicksPerSecond / 60)
#define MAX_ACCEL_DELAY (GameTicksPerSecond / 8)
#define STEP_ACCEL_DELAY (GameTicksPerSecond / 120)
	static char last_buf[CURSOR_INFO_BUFSIZE];
	uint32_t TimeIn = GetTimeCounter();
	static uint16_t moveRepeats;
	bool isMove = false;

	if (!pMS->Initialized)
	{
		GFXPOINT universe;

		pMS->Initialized = true;
		pMS->InputFunc = DoMoveCursor;

		if (!inHQSpace())
		{
			universe = CurStarDescPtr->star_pt;
		}
		else
		{
			universe.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
			universe.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
		}
		flashCurrentLocation(&universe, false);

		last_buf[0] = '\0';
		UpdateCursorInfo(last_buf);
		UpdateFuelRequirement();

		return true;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		FlushInput();

		if ((uqm::UQMOptions::read().bubbleWarp && !uqm::UQMOptions::read().infiniteFuel && !inQuasiSpace())
			&& GLOBAL(autopilot.x) != ~0 && GLOBAL(autopilot.y) != ~0
			&& GLOBAL_SIS(FuelOnBoard) >= FuelRequired())
		{
			DoBubbleWarp(true);
		}

		if (!inQuasiSpace()
			&& ValidPoint(GLOBAL(autopilot)))
		{
			if (uqm::UQMOptions::read().smartAutoPilot)
			{
				SaveLastLoc(
					MAKE_POINT(
						LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x)),
						LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y))));
			}

			if (uqm::UQMOptions::read().advancedAutoPilot
				&& GET_GAME_STATE(PORTAL_SPAWNER_ON_SHIP))
			{
				AdvancedAutoPilot();
			}
		}

		return false;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		/*printf ("Fuel Available: {} | Fuel Requirement: {}\n",
				GLOBAL_SIS (FuelOnBoard), FuelRequired());*/

		FlushInput();

		if (uqm::UQMOptions::read().bubbleWarp && (uqm::UQMOptions::read().infiniteFuel || inQuasiSpace()))
		{
			GLOBAL(autopilot) = cursorLoc;
			DoBubbleWarp(false);
			return false;
		}
		else
		{
			if (GLOBAL(autopilot.x) == cursorLoc.x
				&& GLOBAL(autopilot.y) == cursorLoc.y)
			{
				GLOBAL(autopilot.x) = GLOBAL(autopilot.y) = ~0;
			}
			else
			{
				GLOBAL(autopilot) = cursorLoc;
			}
			DrawStarMap(0, nullptr);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_SEARCH]
			 || (PulsedInputState.menu[KEY_MENU_ZOOM_IN]
				 && PulsedInputState.menu[KEY_MENU_ZOOM_OUT]))
	{
		FlushInput();

		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
		{ // HyperSpace search
			GFXPOINT oldpt = cursorLoc;

			if (!DoStarSearch(pMS))
			{ // search failed or canceled - return cursor
				UpdateCursorLocation(0, 0, &oldpt);
			}
			FlushCursorRect();
			// make sure cmp fails
			uqm::strncpy_safe(last_buf, "  <random garbage>  ");
			UpdateCursorInfo(last_buf);
			UpdateFuelRequirement();

			SetMenuRepeatDelay(MIN_ACCEL_DELAY, MAX_ACCEL_DELAY,
							   STEP_ACCEL_DELAY, true);
			SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_NONE);
		}
		else
		{ // no search in QuasiSpace
			PlayMenuSound(MENU_SOUND_FAILURE);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_TOGGLEMAP])
	{
		FlushInput();

		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
		{
			uint8_t NewState;
			NewState = which_starmap;

			if (NewState == RAINBOW_MAP)
			{
				NewState = NORMAL_STARMAP;
			}
			else
			{
				++NewState;
			}

			if (NewState != which_starmap)
			{
				which_starmap = (CURRENT_STARMAP_SHOWN)NewState;
			}

			PlayMenuSound(MENU_SOUND_MOVE);

			DrawStarMap(0, nullptr);
			last_buf[0] = '\0';
			UpdateCursorInfo(last_buf);
		}
		else
		{ // no alternate maps in QuasiSpace
			PlayMenuSound(MENU_SOUND_FAILURE);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_SPECIAL])
	{
		FlushInput();

		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
		{
			setStarMarked(starIndex(cursorLoc), "PLYR_MARKER");

			DrawStarMap(0, nullptr);
		}
		else
		{
			PlayMenuSound(MENU_SOUND_FAILURE);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_DELETE])
	{
		FlushInput();

		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
		{
			uint16_t i;

			for (i = 0; i <= NUM_SOLAR_SYSTEMS; i++)
			{
				if (isStarMarked(i, "PLYR_MARKER"))
				{
					setStarMarked(i, "PLYR_MARKER");
					DrawStarMap(0, nullptr);
					SleepThread(GameTicksPerSecond / 8);
				}
			}
		}
		else
		{
			PlayMenuSound(MENU_SOUND_FAILURE);
		}
	}
	else
	{
		int8_t sx, sy;
		int16_t ZoomIn, ZoomOut;

		ZoomIn = ZoomOut = 0;
		if (PulsedInputState.menu[KEY_MENU_ZOOM_IN])
		{
			ZoomIn = 1;
		}
		else if (PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
		{
			ZoomOut = 1;
		}

		ZoomStarMap(ZoomIn - ZoomOut);

		sx = sy = 0;
		if (PulsedInputState.menu[KEY_MENU_LEFT])
		{
			sx = -1;
		}
		if (PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			sx = 1;
		}
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			sy = -1;
		}
		if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			sy = 1;
		}

		// Double the cursor speed when the "Next" key is held down
		if (DirKeysPress() && CurrentInputState.menu[KEY_MENU_NEXT])
		{
			sx *= 2;
			sy *= 2;
		}

		if (sx != 0 || sy != 0)
		{
			UpdateCursorLocation(sx, sy, nullptr);
			UpdateCursorInfo(last_buf);
			UpdateFuelRequirement();
			isMove = true;
		}

		SleepThreadUntil(TimeIn + MIN_ACCEL_DELAY);
	}

	if (isMove)
	{
		++moveRepeats;
	}
	else
	{
		moveRepeats = 0;
	}

	flashCurrentLocation(nullptr, false);

	return !(GLOBAL(CurrentActivity) & CHECK_ABORT);
}

static void
RepairMap(uint16_t update_race, GFXRECT* pLastRect, GFXRECT* pNextRect)
{
	GFXRECT r;

	/* make a rect big enough for text */
	r.extent.width = 50;
	r.corner.x = (pNextRect->corner.x + (pNextRect->extent.width >> 1))
			   - (r.extent.width >> 1);
	if (r.corner.x < 0)
	{
		r.corner.x = 0;
	}
	else if (r.corner.x + r.extent.width >= SIS_SCREEN_WIDTH)
	{
		r.corner.x = SIS_SCREEN_WIDTH - r.extent.width;
	}
	r.extent.height = 9;
	r.corner.y = (pNextRect->corner.y + (pNextRect->extent.height >> 1))
			   - r.extent.height;
	if (r.corner.y < 0)
	{
		r.corner.y = 0;
	}
	else if (r.corner.y + r.extent.height >= SIS_SCREEN_HEIGHT)
	{
		r.corner.y = SIS_SCREEN_HEIGHT - r.extent.height;
	}
	BoxUnion(pLastRect, &r, &r);
	BoxUnion(pNextRect, &r, &r);
	*pLastRect = *pNextRect;

	if (r.corner.x < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	if (r.corner.x + r.extent.width > SIS_SCREEN_WIDTH)
	{
		r.extent.width = SIS_SCREEN_WIDTH - r.corner.x;
	}
	if (r.corner.y < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	if (r.corner.y + r.extent.height > SIS_SCREEN_HEIGHT)
	{
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;
	}

	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;

	DrawStarMap(update_race, &r);
}

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

static void
UpdateMap(void)
{
	uint8_t ButtonState, VisibleChange;
	bool MapDrawn, Interrupted;
	uint16_t index;
	HFLEETINFO hStarShip, hNextShip;
	uint16_t GrowthFactor;

	FlushInput();
	ButtonState = 1; /* assume a button down */

	MapDrawn = Interrupted = false;
	for (index = 1,
		hStarShip = GetHeadLink(&GLOBAL(avail_race_q));
		 hStarShip; ++index, hStarShip = hNextShip)
	{
		FLEET_INFO* FleetPtr;

		FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
		hNextShip = _GetSuccLink(FleetPtr);

		if (ButtonState)
		{
			if (!AnyButtonPress(true))
			{
				ButtonState = 0;
			}
		}
		else if ((Interrupted = (bool)(Interrupted || AnyButtonPress(true))))
		{
			MapDrawn = true;
		}

		if (FleetPtr->known_strength)
		{
			int16_t dx, dy, delta;
			GFXRECT r, last_r, temp_r0, temp_r1;
			uint16_t str;

			dx = FleetPtr->loc.x - FleetPtr->known_loc.x;
			dy = FleetPtr->loc.y - FleetPtr->known_loc.y;
			if (dx || dy)
			{
				int16_t xincr, yincr,
					xerror, yerror,
					cycle;

				if (dx >= 0)
				{
					xincr = 1;
				}
				else
				{
					xincr = -1;
					dx = -dx;
				}
				dx <<= 1;

				if (dy >= 0)
				{
					yincr = 1;
				}
				else
				{
					yincr = -1;
					dy = -dy;
				}
				dy <<= 1;

				if (dx >= dy)
				{
					cycle = dx;
				}
				else
				{
					cycle = dy;
				}
				delta = xerror = yerror = cycle >> 1;

				if (!MapDrawn)
				{
					DrawStarMap((uint16_t)~0, nullptr);
					MapDrawn = true;
				}

				GetSphereRect(FleetPtr, &temp_r0, &last_r);
				last_r.extent.width += RES_SCALE(1) + IF_HD(1);	 // dot in HD is 5px wide for AA to work
				last_r.extent.height += RES_SCALE(1) + IF_HD(1); // so we have to add extra 1
				VisibleChange = false;
				do
				{
					do
					{
						if ((xerror -= dx) <= 0)
						{
							FleetPtr->known_loc.x += xincr;
							xerror += cycle;
						}
						if ((yerror -= dy) <= 0)
						{
							FleetPtr->known_loc.y += yincr;
							yerror += cycle;
						}
						GetSphereRect(FleetPtr, &temp_r1, &r);
					} while (delta--
							 && ((delta & 0x1F)
								 || (temp_r0.corner.x == temp_r1.corner.x
									 && temp_r0.corner.y == temp_r1.corner.y)));

					if (ButtonState)
					{
						if (!AnyButtonPress(true))
						{
							ButtonState = 0;
						}
					}
					else if ((Interrupted = (bool)(Interrupted || AnyButtonPress(true))))
					{
						MapDrawn = true;
						goto DoneSphereMove;
					}

					r.extent.width += RES_SCALE(1) + IF_HD(1);
					r.extent.height += RES_SCALE(1) + IF_HD(1);
					if (temp_r0.corner.x != temp_r1.corner.x
						|| temp_r0.corner.y != temp_r1.corner.y)
					{ // Ignore name stacking during movement
						VisibleChange = true;
						RepairMap(index | IGNORE_MOVING_SOI, &last_r, &r);
						SleepThread(GameTicksPerSecond / 24);
					}
				} while (delta >= 0);
				if (VisibleChange)
				{
					RepairMap((uint16_t)~0, &last_r, &r);
				}

DoneSphereMove:
				FleetPtr->known_loc = FleetPtr->loc;
			}

			delta = FleetPtr->actual_strength - FleetPtr->known_strength;
			if (delta)
			{
				if (!MapDrawn)
				{
					DrawStarMap((uint16_t)~0, nullptr);
					MapDrawn = true;
				}

				if (delta > 0)
				{
					dx = 1;
				}
				else
				{
					delta = -delta;
					dx = -1;
				}
				--delta;

				GetSphereRect(FleetPtr, &temp_r0, &last_r);
				last_r.extent.width += RES_SCALE(1);
				last_r.extent.height += RES_SCALE(1);
				// Kruzen: Font size to clean up double space because of text stacking now
				last_r.extent.height = MAX(last_r.extent.height, RES_SCALE(14));
				VisibleChange = false;

				/*printf("{}: {}\n", raceName (index),
						FleetPtr->actual_strength);*/

				str = FleetPtr->known_strength;

				GrowthFactor = delta > 0 ? FleetPtr->actual_strength : FleetPtr->known_strength;

				do
				{
					do
					{
						FleetPtr->known_strength += dx;
						GetSphereRect(FleetPtr, &temp_r1, &r);
					} while (delta--
							 && ((delta & 0xF)
								 || temp_r0.extent.height
										== temp_r1.extent.height));

					if (ButtonState)
					{
						if (!AnyButtonPress(true))
						{
							ButtonState = 0;
						}
					}
					else if ((Interrupted = (bool)(Interrupted || AnyButtonPress(true))))
					{
						MapDrawn = true;
						goto DoneSphereGrowth;
					}
					r.extent.width += RES_SCALE(1);
					r.extent.height += RES_SCALE(1);
					if ((temp_r0.extent.height != temp_r1.extent.height) && !(str > 0 && FleetPtr->known_strength == 0))
					{ // Update race SOI size IF the race didn't die out
						VisibleChange = true;
						RepairMap(index, &last_r, &r);
						SleepThread(
							GameTicksPerSecond / (12 + GrowthFactor / 44));
					}
					else if (str > 0 && FleetPtr->known_strength == 0)
					{ // Flash dying race name
						VisibleChange = true;
						RepairMap(index | PRE_DEATH_SOI, &last_r, &r);
						SleepThread(GameTicksPerSecond / 12);
						RepairMap(index | DEATH_SOI, &last_r, &r);
						SleepThread(GameTicksPerSecond / 12);
					}
				} while (delta >= 0);
				if (VisibleChange
					|| temp_r0.extent.width != temp_r1.extent.width)
				{
					RepairMap((uint16_t)~0, &last_r, &r);
				}

DoneSphereGrowth:
				FleetPtr->known_strength = FleetPtr->actual_strength;
			}
		}

		UnlockFleetInfo(&GLOBAL(avail_race_q), hStarShip);
	}
}

template <typename T>
auto ifGamepad(T gamepadValue, T keyboardValue = {0})
{
	return RES_SCALE(uqm::UQMOptions::read().optControllerType != uqm::ControllerType::KeyboardMouse ? gamepadValue : keyboardValue);
}

static void
DrawStarmapHelper(void)
{

	GFXCONTEXT OldContext;
	STAMP s;
	TEXT t;
	GFXRECT r;
	int16_t leading;
	int frame_index;

	OldContext = SetContext(StatusContext);

	BatchGraphics();

	DrawFlagStatDisplay(GAME_STRING(STATUS_STRING_BASE + 7));

	SetContextFont(TinyFont);
	GetContextFontLeading(&leading);

	r.corner.x = RES_SCALE(4);
	r.corner.y = RES_SCALE(34);

	frame_index = uqm::UQMOptions::read().optControllerType == uqm::ControllerType::KeyboardMouse ? 0 : 5 * static_cast<int>(uqm::UQMOptions::read().optControllerType.value);

	// :Maps
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index);
	s.origin = r.corner;
	DrawStamp(&s);

	SetContextForeGroundColor(MODULE_NAME_COLOR);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE(12) - ifGamepad(2);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GAME_STRING(STATUS_STRING_BASE + 12);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// :Add->O
	s.origin.y += RES_SCALE(10);
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 1);
	DrawStamp(&s);

	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE(16) - ifGamepad(6);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GAME_STRING(STATUS_STRING_BASE + 13);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	s.origin.x = t.baseline.x + RES_SCALE(28);
	s.origin.y += RES_SCALE(4);
	s.frame = SetAbsFrameIndex(MiscDataFrame, 108);
	DrawStamp(&s);

	// Cursor Speed
	s.origin.x = r.corner.x;
	s.origin.y += RES_SCALE(6);
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 2);
	DrawStamp(&s);

	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE(18) - ifGamepad(8);
	t.baseline.y = s.origin.y + leading;
	t.pStr = ":";
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	s.origin.x = t.baseline.x + RES_SCALE(7);
	s.origin.y += RES_SCALE(4);
	s.frame = SetAbsFrameIndex(MiscDataFrame, 48);
	DrawStamp(&s);

	t.align = ALIGN_LEFT;
	t.baseline.x += RES_SCALE(13);
	t.pStr = GAME_STRING(STATUS_STRING_BASE + 14);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// :Search
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 3);
	s.origin.x = r.corner.x;
	s.origin.y += RES_SCALE(7) - ifGamepad(1);
	DrawStamp(&s);

	t.baseline.x = s.origin.x + RES_SCALE(12) + ifGamepad(11);
	t.baseline.y = s.origin.y + leading + RES_SCALE(5) - ifGamepad(5);
	t.pStr = GAME_STRING(STATUS_STRING_BASE + 15);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// :Zoom
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 4);
	s.origin.y += RES_SCALE(21) - ifGamepad(11);
	DrawStamp(&s);

	t.baseline.x = s.origin.x + RES_SCALE(30) - ifGamepad(18);
	t.baseline.y = s.origin.y + leading + RES_SCALE(5);
	t.pStr = GAME_STRING(STATUS_STRING_BASE + 16);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// Separator
	r.corner.y = RES_SCALE(118);
	r.extent.width = FIELD_WIDTH - RES_SCALE(3);
	r.extent.height = RES_SCALE(1);
	SetContextForeGroundColor(CARGO_SELECTED_BACK_COLOR);
	DrawFilledRectangle(&r);

	// FUEL:
	SetContextForeGroundColor(MODULE_NAME_COLOR);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE(2);
	t.baseline.y = r.corner.y + leading + RES_SCALE(1);
	t.pStr = GAME_STRING(STATUS_STRING_BASE + 8); // FUEL:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// Current amount of fuel
	SetContextForeGroundColor(MODULE_PRICE_COLOR);
	t.align = ALIGN_RIGHT;
	t.baseline.x = r.extent.width + RES_SCALE(2);
	t.baseline.y = r.corner.y + leading + RES_SCALE(1);
	t.pStr = WholeFuelValue();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	UnbatchGraphics();

	SetContext(OldContext);
}

bool StarMap(void)
{
	MENU_STATE MenuState;
	GFXPOINT universe;
	//FRAME OldFrame;
	GFXRECT clip_r;
	GFXCONTEXT OldContext;

	memset(&MenuState, 0, sizeof(MenuState));

	which_starmap = NORMAL_STARMAP;

	zoomLevel = 0;
	mapOrigin.x = MAX_X_UNIVERSE >> 1;
	mapOrigin.y = MAX_Y_UNIVERSE >> 1;
	StarMapFrame = SetAbsFrameIndex(MiscDataFrame, 48);

	if (!inHQSpace())
	{
		universe = CurStarDescPtr->star_pt;
	}
	else
	{
		universe.x = LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x));
		universe.y = LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y));
	}

	cursorLoc = GLOBAL(autopilot);
	if (cursorLoc.x == ~0 && cursorLoc.y == ~0)
	{
		cursorLoc = universe;
	}

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		if (playerInPlanetOrbit())
		{
			DrawMenuStateStrings(PM_ALT_SCAN, 1);
		}
		else
		{
			DrawMenuStateStrings(PM_ALT_STARMAP, 0);
		}
	}

	MenuState.InputFunc = DoMoveCursor;
	MenuState.Initialized = false;

	transition_pending = true;
	if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
	{
		UpdateMap();
	}
	else
	{ // This zooms the Quasi map in by 2 if within the local Quasi star
		// cluster.
		if ((universe.x <= ARILOU_HOME_X && universe.y <= ARILOU_HOME_Y)
			&& (universe.x >= 4480 && universe.y >= 4580))
		{
			zoomLevel = 2;
		}
	}

	if (uqm::UQMOptions::read().submenu)
	{
		DrawStarmapHelper();
	}

	DrawStarMap(0, (GFXRECT*)-1);
	transition_pending = false;

	BatchGraphics();
	OldContext = SetContext(SpaceContext);
	GetContextClipRect(&clip_r);
	SetContext(OldContext);
	LoadIntoExtraScreen(&clip_r);
	DrawCursor(UNIVERSE_TO_DISPX(cursorLoc.x),
			   UNIVERSE_TO_DISPY(cursorLoc.y));
	UnbatchGraphics();

	SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_NONE);
	SetMenuRepeatDelay(MIN_ACCEL_DELAY, MAX_ACCEL_DELAY, STEP_ACCEL_DELAY,
					   true);

	DoInput(&MenuState, false);

	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay();

	DrawHyperCoords(universe);
	if (GLOBAL(autopilot.x) != ~0 && GLOBAL(autopilot.y) != ~0)
	{
		DrawAutoPilotMessage(false);
	}
	else
	{
		DrawSISMessage(nullptr);
	}
	DrawStatusMessage(nullptr);

	if (uqm::UQMOptions::read().submenu)
	{
		DeltaSISGauges(UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
	}

	/*if (GLOBAL (autopilot.x) == universe.x
			&& GLOBAL (autopilot.y) == universe.y)
		GLOBAL (autopilot.x) = GLOBAL (autopilot.y) = ~0;*/

	return (GLOBAL(autopilot.x) != ~0
			&& GLOBAL(autopilot.y) != ~0);
}
