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

#include "../colors.h"
#include "../controls.h"
#include "../gameopt.h"
#include "../gamestr.h"
#include "../gendef.h"
#include "../globdata.h"
#include "../hyper.h"
#include "../menustat.h"
#include "../races.h"
#include "../shipcont.h"
#include "../sis.h"
#include "../starmap.h"
#include "../units.h"
#include "core/types/enum.h"
#include "scan.h"
// for DrawHyperCoords(), DrawStatusMessage()
#include "../init.h"
#include "../settings.h"
#include "../setup.h"
#include "../sounds.h"
#include "../state.h"
#include "../uqmdebug.h"
#include "../util.h"
#include "core/string/StringUtils.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"
#include "libs/strlib.h"
#include "options.h"
// For get_fuel_to_sol()
#include <ctype.h>
#include <stdlib.h>
// For isdigit()
#include <math.h> // sqrt()

#include "../build.h"
// For StartSphereTracking()
#include "uqm/random/UQMRandom.h"
#include "uqm/setupmenu.h"

namespace uqm
{
enum class StarmapView
{
	NormalStarmap,
	WarEraStarmap,
	ConstellationMap,
	HomeworldsMap,
	RainbowMap,
	Count
};
consteval bool isArithmeticEnum(StarmapView) { return true; }

struct Nameplate
{
	GfxRect rect;
	GfxText text;
	uint8_t index;
};

static void SwapPlates(Nameplate* n1, Nameplate* n2)
{
	const Nameplate temp = *n1;
	*n1 = *n2;
	*n2 = temp;
}

static constexpr auto IgnoreMovingSoi {1 << 5};
static constexpr auto PreDeathSoi {1 << 6};
static constexpr auto DeathSoi {1 << 7};

static GfxPoint cursorLoc;
static GfxPoint mapOrigin;
static int zoomLevel;
static FrameRef StarMapFrame;
static StarmapView which_starmap;

static inline long signedDivWithError(long val, long divisor)
{
	int invert = 0;
	if (val < 0)
	{
		invert = 1;
		val = -val;
	}
	val = (val + RoundingError(divisor)) / divisor;
	return invert ? -val : val;
}

static auto MapFitX() { return ((MaxXUniverse + 1) / OrigSisScreenWidth() + 1); }
static auto MapFitXx() { return ((MaxXUniverse + 1) / SisScreenWidth() + 1); }

static inline int16_t universeToDispx(long ux)
{
	return signedDivWithError(((ux - mapOrigin.x) << zoomLevel)
								  * OrigSisScreenWidth(),
							  MaxXUniverse + MapFitX())
		 + ((OrigSisScreenWidth() - 1) >> 1);
}
constexpr auto UniverseToDispx(const auto& ux) { return ResScale(universeToDispx(ux)); }
constexpr auto OrigUniverseToDispx(const auto& ux) { return universeToDispx(ux); }

static inline int16_t universeToDispy(long uy)
{
	return signedDivWithError(((mapOrigin.y - uy) << zoomLevel)
								  * OrigSisScreenHeight(),
							  MaxYUniverse + 2)
		 + ((OrigSisScreenHeight() - 1) >> 1);
}
constexpr auto UniverseToDispy(const auto& uy) { return ResScale(universeToDispy(uy)); }
constexpr auto OrigUniverseToDispy(const auto& uy) { return universeToDispy(uy); }

static inline int16_t dispxToUniverse(int16_t dx)
{
	return (((long)(dx - ((OrigSisScreenWidth() - 1) >> 1))
			 * (MaxXUniverse + MapFitX()))
			>> zoomLevel)
			 / OrigSisScreenWidth()
		 + mapOrigin.x;
}
constexpr auto DispToUniversex(const auto& dx) { return dispxToUniverse(ResDescale(dx)); }
constexpr auto OrigDispToUniversex(const auto& dx) { return dispxToUniverse(dx); }

static inline int16_t dispyToUniverse(int16_t dy)
{
	return (((long)(((OrigSisScreenHeight() - 1) >> 1) - dy)
			 * (MaxYUniverse + 2))
			>> zoomLevel)
			 / OrigSisScreenHeight()
		 + mapOrigin.y;
}
constexpr auto DispToUniversey(const auto& dy) { return dispyToUniverse(ResDescale(dy)); }
constexpr auto OrigDispToUniversey(const auto& dy) { return dispyToUniverse(dy); }

// Old school HdResolutionFactor-mod code for Malin's Sol ellipse.
static inline int16_t universeToDispx2(int16_t ux)
{
	const long v = signedDivWithError((((long)ux - mapOrigin.x) << zoomLevel)
										  * SisScreenWidth(),
									  MaxXUniverse + MapFitXx())
				 + ((SisScreenWidth() - 1) >> 1);
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
static inline int16_t universeToDispy2(int16_t uy)
{
	const long v = signedDivWithError((((long)mapOrigin.y - uy) << zoomLevel)
										  * SisScreenHeight(),
									  MaxYUniverse + 2)
				 + ((SisScreenHeight() - 1) >> 1);
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

constexpr auto UniverseToDispx2(const auto& ux) { return (IsHd() ? universeToDispx2(ux) : universeToDispx(ux)); }
constexpr auto UniverseToDispy2(const auto& uy) { return (IsHd() ? universeToDispy2(uy) : universeToDispy(uy)); }

static bool transition_pending;

static void flashCurrentLocation(GfxPoint* where, bool force)
{
	static bool redraw = false;
	static uint8_t c = 0;
	static int val = -2;
	static GfxPoint universe;
	static TimeCount NextTime {};

	if (where)
	{
		universe = *where;
	}

	if (GetTimeCounter() >= NextTime)
	{
		NextTime = GetTimeCounter() + (c_gameSecond / 16);

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
		GfxStamp s;

		GfxContextRef OldContext {SetContext(SpaceContext)};

		OldColor = SetContextForeGroundColor(
			BuildColor(MakeRgb15(c, c, c), c));
		s.origin.x = UniverseToDispx(universe.x);
		s.origin.y = UniverseToDispy(universe.y);
		s.frame = IncFrameIndex(StarMapFrame);
		DrawFilledStamp(&s);
		SetContextForeGroundColor(OldColor);

		SetContext(OldContext);

		redraw = false;
	}
}

static void DrawCursor(int16_t curs_x, int16_t curs_y)
{
	GfxStamp s;

	s.origin.x = curs_x;
	s.origin.y = curs_y;
	s.frame = StarMapFrame;

	DrawStamp(&s);
}

static void DrawMarker(GfxPoint dest, uint8_t type)
{
	GfxStamp s;

	if (type > 2)
	{
		return;
	}

	s.origin = MAKE_POINT(
		UniverseToDispx(dest.x),
		UniverseToDispy(dest.y));
	s.frame = SetAbsFrameIndex(MiscDataFrame, 106 + type);

	DrawStamp(&s);
}

static void DrawAutoPilot(GfxPoint* pDstPt)
{
	int16_t dx {};
	int16_t dy {};
	int16_t xincr {};
	int16_t yincr {};
	int16_t xerror {};
	int16_t yerror {};
	int16_t cycle {};
	int16_t delta {};
	GfxPoint pt;
	GfxStamp s;

	if (!inHQSpace())
	{
		pt = CurStarDescPtr()->star_pt;
	}
	else
	{
		pt.x = LogxToUniverse(uqm::gameState().sis().log_x);
		pt.y = LogyToUniverse(uqm::gameState().sis().log_y);
	}

	if (IsHd())
	{
		s.frame = DecFrameIndex(stars_in_space);
	}

	pt.x = UniverseToDispx(pt.x);
	pt.y = UniverseToDispy(pt.y);

	dx = UniverseToDispx(pDstPt->x) - pt.x;
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

	dy = UniverseToDispy(pDstPt->y) - pt.y;
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
		BuildColor(MakeRgb15(0x04, 0x04, 0x1F), 0x01));

	delta &= ~1;
	while (delta--)
	{
		if (IsHd())
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

static void GetSphereRect(FleetInfo* FleetPtr, GfxRect* pRect, GfxRect* pRepairRect)
{
	const long diameter {(long)(FleetPtr->known_strength * SphereRadiusIncrement)};
	pRect->extent.width = UniverseToDispx(diameter)
						- UniverseToDispx(0);
	if (pRect->extent.width < 0)
	{
		pRect->extent.width = -pRect->extent.width;
	}
	else if (pRect->extent.width == 0)
	{
		pRect->extent.width = ResScale(1);
	}
	pRect->extent.height = UniverseToDispy(diameter)
						 - UniverseToDispy(0);
	if (pRect->extent.height < 0)
	{
		pRect->extent.height = -pRect->extent.height;
	}
	else if (pRect->extent.height == 0)
	{
		pRect->extent.height = ResScale(1);
	}

	pRect->corner.x = UniverseToDispx(FleetPtr->known_loc.x);
	pRect->corner.y = UniverseToDispy(FleetPtr->known_loc.y);
	pRect->corner.x -= pRect->extent.width >> 1;
	pRect->corner.y -= pRect->extent.height >> 1;

	{
		GfxText t;

		t.baseline.x = pRect->corner.x + (pRect->extent.width >> 1);
		t.baseline.y = pRect->corner.y + (pRect->extent.height >> 1)
					 - ResScale(1);
		t.align = TextAlign::Center;
		t.CharCount = (uint16_t)~0;
		t.pStr = FleetPtr->race_strings->text(1);
		TextRect(&t, pRepairRect, nullptr);

		if (pRepairRect->corner.x <= 0)
		{
			pRepairRect->corner.x = ResScale(1);
		}
		else if (pRepairRect->corner.x + pRepairRect->extent.width >= SisScreenWidth())
		{
			pRepairRect->corner.x =
				SisScreenWidth() - pRepairRect->extent.width
				- ResScale(1);
		}
		if (pRepairRect->corner.y <= 0)
		{
			pRepairRect->corner.y = ResScale(1);
		}
		else if (pRepairRect->corner.y + pRepairRect->extent.height >= SisScreenHeight())
		{
			pRepairRect->corner.y =
				SisScreenHeight() - pRepairRect->extent.height
				- ResScale(1);
		}

		BoxUnion(pRepairRect, pRect, pRepairRect);
		pRepairRect->extent.width += ResScale(1);
		pRepairRect->extent.height += ResScale(1);
	}
}

// For showing the War-Era situation in starmap
static void GetWarEraSphereRect(FleetInfo* FleetPtr, GfxRect* pRect, GfxRect* pRepairRect)
{
	const long diameter = (long)(WarEraStrength(FleetPtr->SpeciesID) * SphereRadiusIncrement);
	const GfxPoint loc = SeedFleetLocation(FleetPtr, plot_map.data(), *PlotId::WarEra);

	pRect->extent.width = UniverseToDispx(diameter)
						- UniverseToDispx(0);
	if (pRect->extent.width < 0)
	{
		pRect->extent.width = -pRect->extent.width;
	}
	else if (pRect->extent.width == 0)
	{
		pRect->extent.width = ResScale(1);
	}
	pRect->extent.height = UniverseToDispy(diameter)
						 - UniverseToDispy(0);
	if (pRect->extent.height < 0)
	{
		pRect->extent.height = -pRect->extent.height;
	}
	else if (pRect->extent.height == 0)
	{
		pRect->extent.height = ResScale(1);
	}

	pRect->corner.x = UniverseToDispx(loc.x);
	pRect->corner.y = UniverseToDispy(loc.y);
	pRect->corner.x -= pRect->extent.width >> 1;
	pRect->corner.y -= pRect->extent.height >> 1;

	if (pRepairRect->corner.x <= 0)
	{
		pRepairRect->corner.x = ResScale(1);
	}
	else if (pRepairRect->corner.x + pRepairRect->extent.width >= SisScreenWidth())
	{
		pRepairRect->corner.x = SisScreenWidth() - pRepairRect->extent.width - ResScale(1);
	}

	if (pRepairRect->corner.y <= 0)
	{
		pRepairRect->corner.y = ResScale(1);
	}
	else if (pRepairRect->corner.y + pRepairRect->extent.height >= SisScreenHeight())
	{
		pRepairRect->corner.y = SisScreenHeight() - pRepairRect->extent.height - ResScale(1);
	}

	BoxUnion(pRepairRect, pRect, pRepairRect);
	pRepairRect->extent.width += ResScale(1);
	pRepairRect->extent.height += ResScale(1);
}

static unsigned int FuelRequiredTo(GfxPoint dest)
{
	uint16_t fuel_required {};
	GfxPoint pt;

	if (!inHQSpace())
	{
		pt = CurStarDescPtr()->star_pt;
	}
	else
	{
		pt.x = LogxToUniverse(uqm::gameState().sis().log_x);
		pt.y = LogyToUniverse(uqm::gameState().sis().log_y);
	}

	pt.x -= dest.x;
	pt.y -= dest.y;

	const uint32_t f {(uint32_t)((long)pt.x * pt.x + (long)pt.y * pt.y)};
	if (f == 0 || uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) > 1)
	{
		fuel_required = 0;
	}
	else
	{
		fuel_required = square_root(f) + (FuelTankScale / 20);
	}

	return fuel_required;
}

// Begin Malin's fuel to Sol ellipse code. Edited by Kruzen
constexpr auto MathRound(const auto& X) { return ((X) + ((int)((X) + 0.5) > (X) ? 1 : 0)); }

GfxPoint GetPointOfEllipse(double a, double b, double radian)
{
	const uqstl::array<double, 2> t {a * cos(radian), b * sin(radian)};
	return GfxPoint {(int16_t)MathRound(t[0]), (int16_t)MathRound(t[1])};
}

GfxPoint ShiftPoint(GfxPoint p, GfxPoint s)
{
	return p + s;
}

GfxPoint RotatePoint(GfxPoint p, GfxPoint Pivot, double radian)
{
	const uqstl::array<double, 2> d {static_cast<double>(p.x - Pivot.x), static_cast<double>(p.y - Pivot.y)};
	const uqstl::array<double, 2> cosine {cos(radian), sin(radian)};
	const double x = Pivot.x + (d[0] * cosine[0] - d[1] * cosine[1]);
	const double y = Pivot.y + (d[0] * cosine[1] + d[1] * cosine[0]);

	return GfxPoint {(int16_t)MathRound(x), (int16_t)MathRound(y)};
}

// Kruzen: Merged together with overflow check. Functions above are redundant
GfxPoint CalcEllipsePoint(double a, double b, double rad_one, double rad_two, GfxPoint Pivot)
{
	const uqstl::array<double, 2> q {a * cos(rad_one), b * sin(rad_one)};
	const uqstl::array<double, 2> w {MathRound(q[0]) + Pivot.x, MathRound(q[1]) + Pivot.y};

	const uqstl::array<double, 2> d {w[0] - Pivot.x, w[1] - Pivot.y};
	const uqstl::array<double, 2> cosine {cos(rad_two), sin(rad_two)};
	double x = Pivot.x + (d[0] * cosine[0] - d[1] * cosine[1]);
	double y = Pivot.y + (d[0] * cosine[1] + d[1] * cosine[0]);

	x = MathRound(x);
	if (x > 32767.0)
	{
		x = 32767.0;
	}
	if (x < -32768.0)
	{
		x = -32768.0;
	}

	y = MathRound(y);
	if (y > 32767.0)
	{
		y = 32767.0;
	}
	if (y < -32768.0)
	{
		y = -32768.0;
	}

	return GfxPoint {UniverseToDispx2(x), UniverseToDispy2(y)};
}

bool onScreen(GfxLine* l, bool ignoreX, bool ignoreY)
{
	return !((l->first.x < 0 && l->second.x < 0 && !ignoreX)
			 || (l->first.x >= SisScreenWidth()
				 && l->second.x >= SisScreenWidth() && !ignoreX)
			 || (l->first.y < 0 && l->second.y < 0 && !ignoreY)
			 || (l->first.y >= SisScreenHeight()
				 && l->second.y >= SisScreenHeight() && !ignoreY));
}

static void DrawNoReturnZone(void)
{
	double dist {NAN};
	GfxPoint sol;
	GfxPoint sis;
	const double halfFuel = uqm::gameState().sis().FuelOnBoard / 2;

	sol = plot_map[*PlotId::Sol].star_pt;
	sis = GfxPoint {(int16_t)LogxToUniverse(uqm::gameState().sis().log_x),
					(int16_t)LogyToUniverse(uqm::gameState().sis().log_y)};

	dist = (double)FuelRequiredTo(sol) / 2;

	if (dist <= halfFuel)
	{ // do not draw ellipse when fuel is not enough to reach Sol
		GfxPoint curr;
		GfxPoint center;
		GfxPoint rmax_y;
		GfxPoint rmin_y;
		double i {NAN};
		double Step {NAN};
		double ry {NAN};
		double rotation {NAN};

		ry = sqrt(pow(halfFuel, 2) - pow(dist, 2));

		// on max zoom ellipse edge becomes wobbly, so we cut num of
		// iterations in half on zoomLevel 3 and 4
		Step = (M_PI / (180.0f / (zoomLevel > 2 ? 2 : 1)));
		center = MAKE_POINT((sis.x + sol.x) / 2, (sis.y + sol.y) / 2);
		rotation = atan2(sol.y - sis.y, sol.x - sis.x);

		rmax_y = GfxPoint {-1, -1};
		rmin_y = GfxPoint {(int16_t)SisScreenWidth(), (int16_t)SisScreenHeight()};

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

		if (rmax_y.y >= 0 || rmin_y.y < SisScreenHeight())
		{ // If the ellipse is completely off screen - drop it
			GfxLine L;
			GfxLine tempLine;
			GfxPoint prev = CalcEllipsePoint(halfFuel, ry, i - Step, rotation, center);
			int16_t dy {};
			const double err = ((double)rmax_y.x - (double)rmin_y.x)
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
					GfxLine L2;
					int16_t iter {};
					const double y_err = ((double)L.first.x - (double)prev.x)
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

static void GetFuelRect(LargeRect* r, int32_t diameter, GfxPoint corner)
{ // Operating with LargeRect because of overflows in HdResolutionFactor on max zoom
	int32_t x {};
	int32_t y {};
	int32_t width {};
	int32_t height {};

	if (diameter < 0)
	{
		diameter = 0;
	}

	// Cap the diameter to a sane range
	if (diameter > MaxXUniverse * 4)
	{
		diameter = MaxXUniverse * 4;
	}

	// Calculate in case of overflow
	width = ResScale(signedDivWithError(((diameter - mapOrigin.x) << zoomLevel)
											* OrigSisScreenWidth(),
										MaxXUniverse + MapFitX())
					 + ((OrigSisScreenWidth() - 1) >> 1))
		  - UniverseToDispx(0); // Cannot overflow in current HdResolutionFactor resolution

	if (width < 0)
	{
		width = -width;
	}

	height = ResScale(signedDivWithError(((mapOrigin.y - diameter) << zoomLevel)
											 * OrigSisScreenHeight(),
										 MaxYUniverse + 2)
					  + ((OrigSisScreenHeight() - 1) >> 1))
		   - UniverseToDispy(0); // Cannot overflow in current HdResolutionFactor resolution

	if (height < 0)
	{
		height = -height;
	}

	// SIS cannot leave universe boundaries, so cannot overflow either
	x = UniverseToDispx(corner.x) - (width >> 1);
	y = UniverseToDispy(corner.y) - (height >> 1);

	r->corner.x = x;
	r->corner.y = y;
	r->extent.width = width;
	r->extent.height = height;
}

static void DrawFuelCircle(bool secondary)
{
	LargeRect r;
	GfxPoint corner;
	Color OldColor;
	uint32_t OnBoardFuel = uqm::gameState().sis().FuelOnBoard;

	if (secondary)
	{
		OnBoardFuel -= FuelRequiredTo(uqm::flagship().autopilot.target());
		corner = uqm::flagship().autopilot.target();
	}
	else if (!inHQSpace())
	{
		corner = CurStarDescPtr()->star_pt;
	}
	else
	{
		corner.x = LogxToUniverse(uqm::gameState().sis().log_x);
		corner.y = LogyToUniverse(uqm::gameState().sis().log_y);
	}

	GetFuelRect(&r, OnBoardFuel << 1, corner);

	if (secondary)
	{
		OldColor = SetContextForeGroundColor(DkgrayColor);
		DrawOval(&r, chooseIfHd(1, 6), false);
		SetContextForeGroundColor(OldColor);
	}
	else
	{
		/* Draw circle*/
		OldColor = SetContextForeGroundColor(StarmapFuelRangeColor);
		DrawFilledOval(&r);
		SetContextForeGroundColor(OldColor);

		if (testFlag(uqm::UQMOptions::read().optFuelRange, uqm::FuelRangeDisplay::FuelToSol))
		{
			OldColor =
				SetContextForeGroundColor(StarmapSecondaryRangeColor);
			if (pointsEqual(corner, plot_map[*PlotId::Sol].star_pt))
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
	GfxPoint center, sol, sis;
	double ry, dist, angle;
	double halfFuel = GLOBAL_SIS (FuelOnBoard) / 2;

	sol = (GfxPoint){ SOL_X, SOL_Y };
	sis = (GfxPoint){ LogxToUniverse (GLOBAL_SIS (log_x)),
			LogyToUniverse (GLOBAL_SIS (log_y)) };

	dist = FuelRequiredTo (sol) / 2;

	if (dist >= halfFuel)
		return;
	ry = sqrt (pow (halfFuel, 2) - pow (dist, 2));
	angle = atan2 (sis.y - sol.y, sis.x - sol.x) * 180.0 / M_PI;
	center = MAKE_POINT ((sis.x + sol.x) / 2, (sis.y + sol.y) / 2);

	// convert starmap coords to screen coords
	center.x = UniverseToDispx (center.x);
	center.y = UniverseToDispy (center.y);

	halfFuel = UniverseToDispx (halfFuel) - UniverseToDispx (0);
	if (halfFuel < 0)
		halfFuel = -halfFuel;
	ry = UniverseToDispy (ry) - UniverseToDispy (0);
	if (ry < 0)
		ry = -ry;

	// draw
	OldColor = SetContextForeGroundColor (StarmapSecondaryRangeColor);
	DrawRotatedEllipse (center.x, center.y, halfFuel, ry, angle, 1, 0);
	SetContextForeGroundColor (OldColor);
}*/

bool isHomeworld(uint8_t Index)
{
	bool raceBool = false;

	switch (static_cast<PlotId>(Index))
	{
		case PlotId::Chmmr:
			if (IsHomeworldKnown(*HomeId::Chmmr)
				&& (CheckAlliance(RaceId::Chmmr) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Orz:
			if (IsHomeworldKnown(*HomeId::Orz)
				&& (CheckAlliance(RaceId::Orz) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Pkunk:
			if (IsHomeworldKnown(*HomeId::Pkunk)
				&& (CheckAlliance(RaceId::Pkunk) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Shofixti:
			if (IsHomeworldKnown(*HomeId::Shofixti)
				&& (CheckAlliance(RaceId::Shofixti) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Spathi:
			if (IsHomeworldKnown(*HomeId::Spathi)
				&& (CheckAlliance(RaceId::Spathi) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Supox:
			if (IsHomeworldKnown(*HomeId::Supox)
				&& (CheckAlliance(RaceId::Supox) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Thradd:
			if (IsHomeworldKnown(*HomeId::Thraddash)
				&& (CheckAlliance(RaceId::Thraddash) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Utwig:
			if (IsHomeworldKnown(*HomeId::Utwig)
				&& (CheckAlliance(RaceId::Utwig) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Vux:
			if (IsHomeworldKnown(*HomeId::Vux)
				&& (CheckAlliance(RaceId::Vux) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Yehat:
			if (IsHomeworldKnown(*HomeId::Yehat)
				&& (CheckAlliance(RaceId::Yehat) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Druuge:
			if (IsHomeworldKnown(*HomeId::Druuge)
				&& (CheckAlliance(RaceId::Druuge) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Ilwrath:
			if (IsHomeworldKnown(*HomeId::Ilwrath)
				&& (CheckAlliance(RaceId::Ilwrath) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Mycon:
			if (IsHomeworldKnown(*HomeId::Mycon)
				&& (CheckAlliance(RaceId::Mycon) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Slylandro:
			if (IsHomeworldKnown(*HomeId::Slylandro))
			{
				raceBool = true;
			}
			break;
		case UmgahDefined:
			if (IsHomeworldKnown(*HomeId::Umgah)
				&& (CheckAlliance(RaceId::Umgah) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Zoqfot:
			if (IsHomeworldKnown(*HomeId::Zoqfotpik)
				&& (CheckAlliance(RaceId::Zoqfotpik) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Syreen:
			if (IsHomeworldKnown(*HomeId::Syreen)
				&& (CheckAlliance(RaceId::Syreen) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Androsynth:
			if (IsHomeworldKnown(*HomeId::Androsynth)
				&& (CheckAlliance(RaceId::Androsynth) != RaceAllegiance::Dead || uqm::isStarSeed()))
			{
				raceBool = true;
			}
			break;
		case PlotId::Sol:
		case PlotId::StartColony:
			raceBool = true;
			break;
		default:
			break;
	}

	return raceBool;
}

const char* markerBuf(const int star_index, const char* marker_state)
{
	static uqstl::array<char, 255> buf;

	// marker_state is the middle part of the Game States
	// "SYS_VISITED_##" or "SYS_PLYR_MARKER_##" which is used to
	// differentiate between which kind of marker we're working with.

	fmt::format_to_sz_n(buf, "SYS_{}_{:02}", marker_state,
						star_index / 32);

	return buf.data();
}

bool isStarMarked(const int star_index, const char* marker_state)
{
	int starIndex = star_index;

	if (star_index == InternalStarIndex)
	{
		starIndex = (uint16_t)(CurStarDescPtr() - star_array.data());
	}

	const uint32_t starData {DGetGameState(markerBuf(starIndex, marker_state))};

	return (starData >> (starIndex % 32)) & 1;
}

void setStarMarked(const int star_index, const char* marker_state)
{
	int starIndex = star_index;

	if (starIndex == InternalStarIndex)
	{
		starIndex = (uint16_t)(CurStarDescPtr() - star_array.data());
	}

	uint32_t starData {DGetGameState(markerBuf(starIndex, marker_state))};
	starData ^= (1 << (starIndex % 32));
	DSetGameState(markerBuf(starIndex, marker_state), starData);
}

static int16_t CheckTextsIntersect(GfxRect* curr, GfxRect* prev)
{
	if (((curr->corner.x + curr->extent.width) <= prev->corner.x) || ((prev->corner.x + prev->extent.width) <= curr->corner.x))
	{
		return 0;
	}

	if (((curr->corner.y + curr->extent.height) <= prev->corner.y) || ((prev->corner.y + prev->extent.height) <= curr->corner.y))
	{
		return 0;
	}

	return ((prev->extent.height + ResScale(1)) - abs(curr->corner.y - prev->corner.y));
}

static void AdjustTextRect(GfxRect* r, GfxText* t)
{
	int16_t offs {};
	if (r->corner.x <= 0)
	{
		offs = r->corner.x - ResScale(1);
		t->baseline.x -= offs;
		r->corner.x -= offs;
	}
	else if (r->corner.x + r->extent.width
			 >= SisScreenWidth())
	{
		offs = (r->corner.x + r->extent.width)
			 - SisScreenWidth() + ResScale(1);
		t->baseline.x -= offs;
		r->corner.x -= offs;
	}
	if (r->corner.y <= 0)
	{
		offs = r->corner.y - ResScale(1);
		t->baseline.y -= offs;
		r->corner.y -= offs;
	}
	else if (r->corner.y + r->extent.height
			 >= SisScreenHeight())
	{
		offs = (r->corner.y + r->extent.height)
			 - SisScreenHeight() + ResScale(1);
		t->baseline.y -= offs;
		r->corner.y -= offs;
	}
}

static void DrawRaceName(GfxText* t, Color* c)
{
	// The text color is slightly lighter than the color of
	// the SoI.
	c->r = (c->r >= 0xff - Cc5to8(0x03)) ?
			   0xff :
			   c->r + Cc5to8(0x03);
	c->g = (c->g >= 0xff - Cc5to8(0x03)) ?
			   0xff :
			   c->g + Cc5to8(0x03);
	c->b = (c->b >= 0xff - Cc5to8(0x03)) ?
			   0xff :
			   c->b + Cc5to8(0x03);

	SetContextForeGroundColor(*c);
	font_DrawText(t);
}

Color RaceColor(uint16_t index)
{
	if (uqm::UQMOptions::read().sphereColors == uqm::SphereOfInfluenceColors::Default)
	{
		static constexpr uint16_t NumColors = uqstl::size(RaceColors);
		if (index < NumColors)
		{
			return RaceColors[index];
		}
	}
	else if (uqm::UQMOptions::read().sphereColors == uqm::SphereOfInfluenceColors::Starseed)
	{
		static constexpr uqstl::array<Color, 26> StarseedColors {
			BuildColor(MakeRgb15(0x00, 0x00, 0x17), 0x00), // ARILOU_SHIP Bold blue
			BuildColor(MakeRgb15(0x10, 0x15, 0x19), 0x00), // CHMMR_SHIP Pale blue-grey
			BuildColor(MakeRgb15(0x09, 0x09, 0x1B), 0x00), // HUMAN_SHIP Blue
			BuildColor(MakeRgb15(0x0D, 0x00, 0x05), 0x00), // ORZ_SHIP Plum
			BuildColor(MakeRgb15(0x1C, 0x07, 0x19), 0x00), // PKUNK_SHIP Pinkunk
			BuildColor(MakeRgb15(0x0A, 0x06, 0x00), 0x00), // SHOFIXTI_SHIP Marsupial brown
			BuildColor(MakeRgb15(0x15, 0x12, 0x03), 0x00), // SPATHI_SHIP Mustard yellow
			BuildColor(MakeRgb15(0x05, 0x14, 0x01), 0x00), // SUPOX_SHIP Leaf green
			BuildColor(MakeRgb15(0x00, 0x06, 0x08), 0x00), // THRADDASH_SHIP Dark cyan
			BuildColor(MakeRgb15(0x1A, 0x16, 0x12), 0x00), // UTWIG_SHIP Beige
			BuildColor(MakeRgb15(0x06, 0x16, 0x0F), 0x00), // VUX_SHIP Very Ugly Aqua
			BuildColor(MakeRgb15(0x0A, 0x00, 0x11), 0x00), // YEHAT_SHIP Royal purple
			BuildColor(MakeRgb15(0x1A, 0x16, 0x08), 0x00), // MELNORME_SHIP Yellowish
			BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x00), // DRUUGE_SHIP Crimson (tm)
			BuildColor(MakeRgb15(0x0E, 0x00, 0x0E), 0x00), // ILWRATH_SHIP Purple
			BuildColor(MakeRgb15(0x14, 0x00, 0x0C), 0x00), // MYCON_SHIP Fungal fuchsia
			BuildColor(MakeRgb15(0x1D, 0x00, 0x07), 0x00), // SLYLANDRO_SHIP Red Alert
			BuildColor(MakeRgb15(0x0B, 0x0C, 0x00), 0x00), // UMGAH_SHIP Olive
			BuildColor(MakeRgb15(0x00, 0x08, 0x00), 0x00), // URQUAN_SHIP Dark Green
			BuildColor(MakeRgb15(0x16, 0x0D, 0x00), 0x00), // ZOQFOTPIK_SHIP Orange
			BuildColor(MakeRgb15(0x15, 0x07, 0x03), 0x00), // SYREEN_SHIP Slightly-orange red
			BuildColor(MakeRgb15(0x06, 0x06, 0x06), 0x00), // BLACK_URQUAN_SHIP Dark grey
			BuildColor(MakeRgb15(0x09, 0x00, 0x15), 0x00), // ANDROSYNTH_SHIP Violet
			BuildColor(MakeRgb15(0x00, 0x15, 0x1C), 0x00), // CHENJESU_SHIP Cyan
			BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x00), // MMRNMHRM_SHIP Silver
			BuildColor(MakeRgb15(0x19, 0x10, 0x1F), 0x00), // YEHAT_REBEL_SHIP Lavender
		};

		static constexpr uint16_t NumColors = uqstl::size(StarseedColors);

		if (index < NumColors)
		{
			return StarseedColors[index];
		}
	}
	// Hot pink (just a fail state but also reserved)
	return BuildColor(MakeRgb15(0x1C, 0x00, 0x0C), 0x00);
#if 0
	// A way to choose colors entirely "fairly", although colors don't really map
	// in such an even distribution.
		uint8_t i, j, k;
		for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) for (k = 0; k < 3; k++)
			race_colors[i * 9 + j * 3 + k] = (Color) BuildColor (MakeRgb15 (14 * i + 1, 14 * j + 1, 14 * k + 1), 0x00);
#endif
}

static constexpr auto GridDelta {500};

static void DrawStarMap(uint16_t race_update, GfxRect* pClipRect)
{
	int16_t i {};
	uint16_t which_space {};
	// long diameter;
	GfxRect r;
	GfxRect old_r;
	GfxPoint oldOrigin = {0, 0};
	GfxStamp s;
	const StarDesc* SDPtr {};
	bool draw_cursor {};

	if (pClipRect == (GfxRect*)-1)
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

	which_space = uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE);

	if (which_space <= 1)
	{
		SDPtr = &star_array[0];
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x00, 0x00, 0x07), 0x57));
		SetContextBackGroundColor(BlackColor);
	}
	else
	{
		SDPtr = &star_array[NumSolarSystems + 1];
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x00, 0x0B, 0x00), 0x6D));
		SetContextBackGroundColor(
			BuildColor(MakeRgb15(0x00, 0x08, 0x00), 0x6E));
	}
	ClearDrawable();

	if (which_starmap != StarmapView::ConstellationMap
		&& (race_update == 0 && which_space < 2)
		&& !(uqm::UQMOptions::read().infiniteFuel || uqm::gameState().sis().FuelOnBoard == 0))
	{ // Draw the fuel range circle
		DrawFuelCircle(false);
	}

	{ // Horizontal lines
		r.corner.x = UniverseToDispx(0);
		r.extent.width = (SisScreenWidth() << zoomLevel)
					   - (ResScale(1) << zoomLevel);
		r.extent.height = ResScale(1);

		for (i = MaxYUniverse; i >= 0; i -= GridDelta)
		{
			r.corner.y = UniverseToDispy(i);
			DrawFilledRectangle(&r);
		}

		r.corner.y = UniverseToDispy(0);
		DrawFilledRectangle(&r);

		// Vertical lines
		r.corner.y = UniverseToDispy(MaxYUniverse) + ResScale(1);
		r.extent.width = ResScale(1);
		r.extent.height = (SisScreenHeight() << zoomLevel) - ResScale(1);

		for (i = 0; i < MaxYUniverse; i += GridDelta)
		{
			r.corner.x = UniverseToDispx(i);
			DrawFilledRectangle(&r);
		}

		// Edge rounding error compensation
		// so the bar wouldn't leak over the edge
		r.corner.x = UniverseToDispx(MaxXUniverse);
		r.corner.y = UniverseToDispy(MaxYUniverse);
		r.extent.height = (SisScreenHeight() << zoomLevel) + ResScale(1);
		if (r.extent.height - ResScale(1)
			> (-(UniverseToDispy(MaxYUniverse)
				 - UniverseToDispy(0))))
		{
			r.extent.height -= ResScale(1);
		}
		DrawFilledRectangle(&r);
	}

	if (which_starmap != StarmapView::ConstellationMap
		&& (race_update == 0 && which_space < 2)
		&& !(uqm::UQMOptions::read().infiniteFuel || uqm::gameState().sis().FuelOnBoard == 0)
		&& (testFlag(uqm::UQMOptions::read().optFuelRange, uqm::FuelRangeDisplay::FuelAtDestination))
		&& (uqm::flagship().autopilot.hasTarget()))
	{ // Draw the autopilot fuel range circle (on top of the grid)
		DrawFuelCircle(true);
	}

	FrameRef star_frame {SetRelFrameIndex(StarMapFrame, 2)};
	if (which_space <= 1 && which_starmap != StarmapView::ConstellationMap)
	{
		uint16_t index {};
		const uint16_t race_index = (race_update & 0x1F) - 1;
		FleetInfoHandle hStarShip {};
		FleetInfoHandle hNextShip {};
		uqstl::array<Nameplate, 26> nameplate;
		uint8_t currMax = 0;

		for (index = 0,
			hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
			 hStarShip != 0; ++index, hStarShip = hNextShip)
		{
			FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
			hNextShip = _GetSuccLink(FleetPtr);

			if ((FleetPtr->known_strength && which_starmap != StarmapView::WarEraStarmap) || (which_starmap == StarmapView::WarEraStarmap && WarEraStrength(FleetPtr->SpeciesID) && index < RaceId::NumBuildableShips))
			{
				GfxRect repair_r;

				if (which_starmap == StarmapView::WarEraStarmap)
				{
					GetWarEraSphereRect(FleetPtr, &r, &repair_r);
				}
				else
				{
					GetSphereRect(FleetPtr, &r, &repair_r);
				}

				if (r.corner.x < SisScreenWidth()
					&& r.corner.y < SisScreenHeight()
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
					GfxText t;
					char yehat_rebels[] {"+Yehat+"};

					c = RaceColor(index);
					if (index == race_index)
					{
						SetContextForeGroundColor(WhiteColor);
					}
					else
					{
						SetContextForeGroundColor(c);
					}

					{
						LargeRect dr = RECT_TO_DRECT(r);
						DrawOval(&dr, 0, IsHd());
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
								 - ResScale(1);
					t.align = TextAlign::Center;

					// For drawing War-Era starmap.
					if (which_starmap == StarmapView::WarEraStarmap)
					{
						switch (static_cast<RaceId>(index))
						{
							case RaceId::Pkunk:
							case RaceId::Thraddash:
							case RaceId::Druuge:
								t.pStr = GameString(GameStringBase::StarString + 132);
								t.CharCount = (uint16_t)strlen(t.pStr);
								break;
							case RaceId::Androsynth:
								t.pStr = FleetPtr->race_strings->text(0);
								t.CharCount = (uint16_t)~0;
								break;
							default:
								t.CharCount = (uint16_t)~0;
								t.pStr = FleetPtr->race_strings->text(1);
								break;
						}
					}
					else if (index == RaceId::YehatRebel && uqm::UQMOptions::read().sphereColors == uqm::SphereOfInfluenceColors::Starseed)
					{
						t.CharCount = 7;
						t.pStr = yehat_rebels;
					}
					else
					{
						t.CharCount = (uint16_t)~0;
						t.pStr = FleetPtr->race_strings->text(1);
					}

					TextRect(&t, &r, nullptr);

					if (index == race_index && race_update & IgnoreMovingSoi)
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
			else if (index == race_index && race_update & (PreDeathSoi | DeathSoi))
			{ // Kruzen: SoI is dead, but we need to fade nameplate
				GfxText t;
				GfxRect repair_r;

				t.CharCount = (uint16_t)~0;
				t.pStr = FleetPtr->race_strings->text(1);

				GetSphereRect(FleetPtr, &r, &repair_r);

				t.baseline.x = r.corner.x + (r.extent.width >> 1);
				t.baseline.y = r.corner.y + (r.extent.height >> 1)
							 - ResScale(1);
				t.align = TextAlign::Center;

				TextRect(&t, &r, nullptr);

				nameplate[currMax].rect = r;
				nameplate[currMax].text = t;
				nameplate[currMax].index = race_update - 1;
				currMax++;
			}
			UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
		}

		if (currMax > 0)
		{
			uint8_t j {};
			uint8_t k {};
			bool swapped {};
			int16_t offs {};
			GfxText t;
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
				if (nameplate[j].index & PreDeathSoi)
				{
					c = WhiteColor;
				}
				else if (nameplate[j].index & DeathSoi)
				{
					c = BuildShadeRgba(0x80);
				}
				else
				{
					c = RaceColor(nameplate[j].index & 0x1F);
				}
				r = nameplate[j].rect;
				t = nameplate[j].text;

				AdjustTextRect(&r, &t);

				r.corner.y += ResScale(1);
				r.extent.height -= ResScale(2);

				if (r.corner.y > (SisScreenHeight() >> 1))
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
				if (nameplate[currMax - j].index & PreDeathSoi)
				{
					c = WhiteColor;
				}
				else if (nameplate[currMax - j].index & DeathSoi)
				{
					c = BuildShadeRgba(0x80);
				}
				else
				{
					c = RaceColor(nameplate[currMax - j].index & 0x1F);
				}
				r = nameplate[currMax - j].rect;
				t = nameplate[currMax - j].text;

				AdjustTextRect(&r, &t);

				r.corner.y += ResScale(1);
				r.extent.height -= ResScale(2);

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
	if (which_space <= 1 && which_starmap == StarmapView::ConstellationMap)
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
					GfxText t;
					uqstl::array<char, 7> pStr;
					pStr[0] = (i / 16 ? '1' : '0');
					pStr[1] = (i % 16 > 9 ? i % 16 - 10 + 'A' : i % 16 + '0');
					pStr[2] = (j / 16 ? '1' : '0');
					pStr[3] = (j % 16 > 9 ? j % 16 - 10 + 'A' : j % 16 + '0');
					pStr[4] = (k / 16 ? '1' : '0');
					pStr[5] = (k % 16 > 9 ? k % 16 - 10 + 'A' : k % 16 + '0');
					pStr[6] = '\0';
					SetContextFont(TinyFontBold);
					SetContextForeGroundColor(BuildColor(MakeRgb15(i, j, k), 0x00));
					t.baseline.x = UniverseToDispx(300 * j + (k / 16 ? 150 : 0));
					t.baseline.y = UniverseToDispy(300 * i + k % 16 * 300 / 16) + 3;
					t.align = TextAlign::Center;
					t.pStr = pStr;
					t.CharCount = 10;
					font_DrawText(&t);
				}
			}
		}
#else
		Color oldColor;
		GfxLine l;
		const uint8_t c = 0x3F + IfHd(0x11);
		const GfxPoint* CNPtr {&constel_array[0]};

		oldColor = SetContextForeGroundColor(
			BuildColorRgba(c, c, c, 0xFF));

		while (CNPtr->x < MaxXUniverse && CNPtr->y < MaxYUniverse)
		{ // Have to add 2 because of HdResolutionFactor nature (can't get exact middle of 4x4)
			l.first.x = UniverseToDispx(CNPtr->x) + IfHd(2);
			l.first.y = UniverseToDispy(CNPtr->y) + IfHd(2);
			CNPtr++;
			l.second.x = UniverseToDispx(CNPtr->x) + IfHd(2);
			l.second.y = UniverseToDispy(CNPtr->y) + IfHd(2);
			CNPtr++;
			DrawLine(&l, 1);
		}
		SetContextForeGroundColor(oldColor);
#endif
	}

	// This draws markers over known alien Homeworlds
	// and labels the quasispace portal locations in hyperspace.
	if (which_space <= 1 && which_starmap == StarmapView::HomeworldsMap)
	{
		uint16_t i {};

		for (i = 0; i < (NumSolarSystems + 1); ++i)
		{
			const uint8_t Index = star_array[i].Index;
			if (isHomeworld(Index))
			{
				DrawMarker(star_array[i].star_pt, true);
			}
		}
		for (i = 0; i < NumHyperVortices; ++i)
		{
			if (!(uqm::gameState().flag(uqm::GameFlag::KNOW_QS_PORTAL) & (1 << i)))
			{
				continue;
			}
			GfxText t;
			uqstl::array<char, 2> pStr;
			pStr[0] = 'A' + i;
			pStr[1] = 0;
			SetContextFont(TinyFontBold);
			// "Quasispace Green"
			SetContextForeGroundColor(BuildColor(MakeRgb15(0x00, 0x1A, 0x00), 0x2F));
			t.baseline.x = UniverseToDispx(portal_map[i].star_pt.x);
			t.baseline.y = UniverseToDispy(portal_map[i].star_pt.y) + 3;
			t.align = TextAlign::Center;
			t.pStr = pStr.data();
			t.CharCount = 2;
			font_DrawText(&t);
		}
	}

	// This draws markers over the Rainbow worlds
	if (which_space <= 1 && which_starmap == StarmapView::RainbowMap)
	{
		uint16_t rainbow_mask {};

		rainbow_mask = uqm::makeWord(
			uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD0),
			uqm::gameState().flag(uqm::GameFlag::RAINBOW_WORLD1));

		if (rainbow_mask == 0)
		{
			which_starmap = StarmapView::NormalStarmap;
		}
		else
		{
			for (uint16_t i {*PlotId::Rainbow0}; i <= PlotId::Rainbow9; i++)
			{
				if (rainbow_mask & (1 << (i - *PlotId::Rainbow0)))
				{
					DrawMarker(plot_map[i].star_pt, true);
				}
			}
		}
	}

	do
	{ // Draws all the stars
		static uint16_t i = 0;

		i = i >= NumSolarSystems ? 0 : i;

		const uint8_t star_type {SDPtr->Type};

		s.origin.x = UniverseToDispx(SDPtr->star_pt.x);
		s.origin.y = UniverseToDispy(SDPtr->star_pt.y);
		if (which_space <= 1)
		{
			if (which_starmap == StarmapView::NormalStarmap
				&& isStarMarked(i, "PLYR_MARKER"))
			{ // This draws markers over tagged star systems
				DrawMarker(SDPtr->star_pt, 2);
			}

			if (uqm::UQMOptions::read().showVisitedStars && isStarMarked(i, "VISITED")
				&& which_starmap == StarmapView::NormalStarmap
				&& SDPtr->Index != PlotId::Sol)
			{
				s.frame = SetRelFrameIndex(visitedStarsFrame,
										   *StarType(star_type)
												   * *StarBodyColor::Count
											   + *StarColor(star_type));
			}
			else
			{
				s.frame = SetRelFrameIndex(star_frame,
										   *StarType(star_type)
												   * *StarBodyColor::Count
											   + *StarColor(star_type));
			}

			++i;
		}
		else if (SDPtr->star_pt.x == ArilouHomeX
				 && SDPtr->star_pt.y == ArilouHomeY)
		{
			s.frame = SetRelFrameIndex(star_frame,
									   (int)StellarSize::SuperGiant * (int)StarBodyColor::Count + (int)StarBodyColor::Green); // TODO PragmaNull: this is weird enum arithmetic. helper function?
		}
		else
		{
			s.frame = SetRelFrameIndex(star_frame,
									   (int)StellarSize::Giant * (int)StarBodyColor::Count + (int)StarBodyColor::Green); // TODO PragmaNull: this is weird enum arithmetic. helper function?
		}
		DrawStamp(&s);

		++SDPtr;
	} while (SDPtr->star_pt.x <= MaxXUniverse
			 && SDPtr->star_pt.y <= MaxYUniverse);

	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE))
	{
		if (which_space <= 1)
		{
			s.origin.x = UniverseToDispx(plot_map[*PlotId::Arilou].star_pt.x);
			s.origin.y = UniverseToDispy(plot_map[*PlotId::Arilou].star_pt.y);
		}
		else
		{
			s.origin.x = UniverseToDispx(QuasiSpaceX);
			s.origin.y = UniverseToDispy(QuasiSpaceY);
		}
		s.frame = SetRelFrameIndex(star_frame,
								   (int)StellarSize::Giant * (int)StarBodyColor::Count + (int)StarBodyColor::Green);
		DrawStamp(&s);
	}

	if (race_update == 0
		&& uqm::flagship().autopilot.hasTarget())
	{
		GfxPoint target {uqm::flagship().autopilot.target()};
		DrawAutoPilot(&target);
		if (IsHd())
		{
			DrawMarker(uqm::flagship().autopilot.target(), false);
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
		DrawCursor(UniverseToDispx(cursorLoc.x),
				   UniverseToDispy(cursorLoc.y));
		flashCurrentLocation(nullptr, true);
	}

	UnbatchGraphics();
}

static void EraseCursor(int16_t curs_x, int16_t curs_y)
{
	GfxRect r;

	GetFrameRect(StarMapFrame, &r);

	if ((r.corner.x += curs_x) < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	else if (r.corner.x + r.extent.width >= SisScreenWidth())
	{
		r.extent.width = SisScreenWidth() - r.corner.x;
	}
	if ((r.corner.y += curs_y) < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	else if (r.corner.y + r.extent.height >= SisScreenHeight())
	{
		r.extent.height = SisScreenHeight() - r.corner.y;
	}

#ifndef OLD
	RepairBackRect(&r);
#else  /* NEW */
	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;
	DrawStarMap(0, &r);
#endif /* OLD */
}

static constexpr auto MaxZoomShift {4};

static void ZoomStarMap(int16_t dir)
{
	if (dir > 0)
	{
		if (zoomLevel < MaxZoomShift)
		{
			++zoomLevel;
			mapOrigin = cursorLoc;

			DrawStarMap(0, nullptr);
			SleepThread(c_gameSecond / 8);
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
				mapOrigin.x = MaxXUniverse >> 1;
				mapOrigin.y = MaxYUniverse >> 1;
			}
			--zoomLevel;

			DrawStarMap(0, nullptr);
			SleepThread(c_gameSecond / 8);
		}
	}
}

static void UpdateCursorLocation(int sx, int sy, const GfxPoint* newpt)
{ // Kruzen: OrigSisScreenWidth()/HEIGHT for viewport follows cursor mode
	// We're scaling the result of s.origin afterwards, but calculating
	// everything in SD values. So we can use max zoom in HdResolutionFactor
	GfxStamp s;
	GfxPoint pt;

	pt.x = OrigUniverseToDispx(cursorLoc.x);
	pt.y = OrigUniverseToDispy(cursorLoc.y);

	if (newpt)
	{ // absolute move
		sx = sy = 0;
		s.origin.x = OrigUniverseToDispx(newpt->x);
		s.origin.y = OrigUniverseToDispy(newpt->y);
		cursorLoc = *newpt;
	}
	else
	{ // incremental move
		s.origin.x = pt.x + sx;
		s.origin.y = pt.y + sy;
	}

	if (sx)
	{
		cursorLoc.x = OrigDispToUniversex(s.origin.x) - sx;
		while (OrigUniverseToDispx(cursorLoc.x) == pt.x)
		{
			cursorLoc.x += sx;
		}

		if (cursorLoc.x < 0)
		{
			cursorLoc.x = 0;
		}
		else if (cursorLoc.x > MaxXUniverse)
		{
			cursorLoc.x = MaxXUniverse;
		}

		s.origin.x = OrigUniverseToDispx(cursorLoc.x);
	}

	if (sy)
	{
		cursorLoc.y = OrigDispToUniversey(s.origin.y) + sy;
		while (OrigUniverseToDispy(cursorLoc.y) == pt.y)
		{
			cursorLoc.y -= sy;
		}

		if (cursorLoc.y < 0)
		{
			cursorLoc.y = 0;
		}
		else if (cursorLoc.y > MaxYUniverse)
		{
			cursorLoc.y = MaxYUniverse;
		}

		s.origin.y = OrigUniverseToDispy(cursorLoc.y);
	}

	if (s.origin.x < 0 || s.origin.y < 0
		|| s.origin.x >= OrigSisScreenWidth()
		|| s.origin.y >= OrigSisScreenHeight())
	{
		mapOrigin = cursorLoc;
		DrawStarMap(0, nullptr);

		s.origin.x = OrigUniverseToDispx(cursorLoc.x);
		s.origin.y = OrigUniverseToDispy(cursorLoc.y);
	}
	else
	{
		BatchGraphics();
		EraseCursor(ResScale(pt.x), ResScale(pt.y));
		DrawCursor(ResScale(s.origin.x), ResScale(s.origin.y));
		flashCurrentLocation(nullptr, true);
		UnbatchGraphics();
	}
}

static constexpr auto CursorInfoBufsize {256};

int starIndex(GfxPoint starPt)
{
	uint16_t i {};

	for (i = 0; i <= NumSolarSystems; i++)
	{
		if (star_array[i].star_pt.x == starPt.x
			&& star_array[i].star_pt.y == starPt.y)
		{
			break;
		}
	}
	return i;
}

static void UpdateCursorInfo(std::span<char> prevbuf)
{
	uqstl::array<char, CursorInfoBufsize> buf {};
	GfxPoint pt;
	StarDesc* SDPtr {};
	StarDesc* BestSDPtr {};

	if (which_starmap == StarmapView::NormalStarmap)
	{ // "(Star Search: F6 | Toggle Maps: F7)"
		uqm::utf8StringCopy(buf, GameString(GameStringBase::FeedbackString + 2 + (is3DO(uqm::UQMOptions::read().whichFonts) || IsPad())));
	}
	else
	{
		uqm::utf8StringCopy(buf,
							GameString(GameStringBase::FeedbackString + 3 + *which_starmap));
	}

	pt.x = UniverseToDispx(cursorLoc.x);
	pt.y = UniverseToDispy(cursorLoc.y);

	SDPtr = BestSDPtr = 0;
	while ((SDPtr = FindStar(SDPtr, &cursorLoc, 75, 75)))
	{
		if (UniverseToDispx(SDPtr->star_pt.x) == pt.x
			&& UniverseToDispy(SDPtr->star_pt.y) == pt.y
			&& (BestSDPtr == 0
				|| StarType(SDPtr->Type) >= StarType(BestSDPtr->Type)))
		{
			BestSDPtr = SDPtr;
		}
	}

	if (BestSDPtr)
	{
		// JMS: For masking the names of QS portals not yet entered.
		const uint8_t whichPortal = BestSDPtr->Postfix - 133;

		// A star is near the cursor:
		// Snap cursor onto star
		cursorLoc = BestSDPtr->star_pt;

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) >= 2
			&& !(uqm::gameState().flag(uqm::GameFlag::KNOW_QS_PORTAL) & (1 << whichPortal)))
		{
			// "UNKNOWN" string
			uqm::utf8StringCopy(buf,
								GameString(GameStringBase::StarString + 132));
		}
		else if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) < 2 || whichPortal == NumHyperVortices)
		{
			// Hyperspace side, or Arilou homeworld name once known
			GetClusterName(BestSDPtr, buf.data());
		}
		else
		{
			// Use whichPortal (based on the Postfix of the star) to determine
			// which portal_map entity this maps to and pull the nearest star.
			// We then use that to construct the exit point name.
			// This means quasi star array must have rigidly ordered Postfix.
			// portal_map[whichPortal].star_pt - coords of the exit
			//                  nearest_star - StarDesc* of nearest constell.
			uqstl::array<char, CursorInfoBufsize> starnameBuf {};
			uqm::utf8StringCopy(starnameBuf, GameString(portal_map[whichPortal].nearest_star->Postfix));
			// Abbreviate names longer than 11 at the 10th (or less)
			// non-vowel character.
			if (strlen(starnameBuf.data()) > 11)
			{
				uint16_t i {};
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
			snprintf(buf.data(), sizeof(buf),
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

		int16_t bucket {DispToUniversex(pt.x)};
		if (bucket < 0)
		{
			cursorLoc.x = 0;
		}
		else if (bucket > MaxXUniverse)
		{
			cursorLoc.x = MaxXUniverse;
		}
		bucket = DispToUniversey(pt.y);
		if (bucket < 0)
		{
			cursorLoc.y = 0;
		}
		else if (bucket > MaxYUniverse)
		{
			cursorLoc.y = MaxYUniverse;
		}
	}

	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE))
	{
		GfxPoint ari_pt;

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			ari_pt = plot_map[*PlotId::Arilou].star_pt;
		}
		else
		{
			ari_pt.x = QuasiSpaceX;
			ari_pt.y = QuasiSpaceY;
		}

		if (UniverseToDispx(ari_pt.x) == pt.x
			&& UniverseToDispy(ari_pt.y) == pt.y)
		{
			cursorLoc = ari_pt;
			uqm::utf8StringCopy(buf,
								GameString(GameStringBase::StarString + 132));
		}
	}

	DrawHyperCoords(cursorLoc);
	if (strncmp(buf.data(), prevbuf.data(), sizeof(buf)) != 0)
	{
		uqm::strncpy_safe(prevbuf, buf.data());

		// Cursor is on top of a star. Display its name.
		if (BestSDPtr)
		{
			if (uqm::UQMOptions::read().showVisitedStars
				&& isStarMarked(starIndex(BestSDPtr->star_pt),
								"VISITED"))
			{
				uqstl::array<char, CursorInfoBufsize> visBuf {};

				uqm::utf8StringCopy(visBuf, buf.data());
				// This is how you get rid of compiler warnings over string
				// length:  %.30s where "30" is a limit.  Enjoy. ~JSD~
				//snprintf(buf, sizeof buf, "%c %.251s %c", '(', visBuf, ')');
				fmt::format_to_sz_n(buf, "( {:251} )", visBuf.data());
			}

			DrawSISMessage(buf.data());
		}
		// Cursor is elsewhere.
		else
		{
			// In HS, display default star search button name.
			if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
			{
				GfxContextRef OldContext {SetContext(OffScreenContext)};

				if (which_starmap == StarmapView::WarEraStarmap)
				{
					SetContextForeGroundColor(
						BuildColor(
							MakeRgb15(0x18, 0x00, 0x00), 0x00));
				}
				else
				{
					SetContextForeGroundColor(
						BuildColor(
							MakeRgb15(0x0E, 0xA7, 0xD9), 0x00));
				}

				DrawSISMessageEx(buf.data(), -1, -1, SisMessageFlags::MyColor);
				SetContext(OldContext);
			}
			else
			{ // In QS, don't display star search button - the search is
				// unusable.
				const char* sz {GameString(GameStringBase::NavigationString + 1)};
				uqm::strncpy_safe(buf, sz);
				DrawSISMessage(buf.data());
			}
		}
	}
}

static unsigned int FuelRequired(void)
{
	return FuelRequiredTo(cursorLoc);
}

static void UpdateFuelRequirement(void)
{
	uqstl::array<char, 80> buf {};
	const uint16_t fuel_required = FuelRequired();

	fmt::format_to_n(buf.data(), sizeof(buf) - 1, "{} {}.{}",
					 GameString(GameStringBase::NavigationString + 4),
					 fuel_required / FuelTankScale,
					 (fuel_required % FuelTankScale) / 10);

	DrawStatusMessage(buf.data());
}

static constexpr auto StarSearchBufsize {256};

struct StarSearchState
{
	// TODO: pMS field is probably not needed anymore
	MenuState* pMS;
	uqstl::array<char, StarSearchBufsize> Text;
	uqstl::array<char, StarSearchBufsize> LastText;
	TimeCount LastChangeTime;
	int FirstIndex;
	int CurIndex;
	int LastIndex;
	bool SingleClust;
	bool SingleMatch;
	uqstl::array<char, StarSearchBufsize> Buffer;
	const char* Prefix;
	const char* Cluster;
	int PrefixLen;
	int ClusterLen;
	int ClusterPos;
	uqstl::array<int, NumSolarSystems> SortedStars;
};

static bool starNameLess(int index1, int index2)
{
	const StarDesc& star1 {star_array[index1]};
	const StarDesc& star2 {star_array[index2]};
	if (star1.Postfix != star2.Postfix)
	{
		return uqm::utf8StringCompare(GameString(star1.Postfix), GameString(star2.Postfix)) < 0;
	}
	return star1.Prefix < star2.Prefix;
}

static void SortStarsOnName(StarSearchState* pSS)
{
	int* const sorted {pSS->SortedStars.data()};
	uqstl::iota(sorted, sorted + NumSolarSystems, 0);
	uqstl::sort(sorted, sorted + NumSolarSystems, starNameLess);
}

static void SplitStarName(StarSearchState* pSS)
{
	char* buf = pSS->Buffer.data();
	char* next {};
	char* sep = nullptr;

	pSS->Prefix = 0;
	pSS->PrefixLen = 0;
	pSS->Cluster = 0;
	pSS->ClusterLen = 0;
	pSS->ClusterPos = 0;

	// skip leading space
	for (next = buf; *next != '\0' && uqm::getCharFromString((const char**)&next) == (isPC(uqm::UQMOptions::read().whichFonts) ? UnicharSpace : UnicharTab);
		 buf = next)
		;
	if (*buf == '\0')
	{ // no text
		return;
	}

	pSS->Prefix = buf;

	// See if player gave a prefix
	for (buf = next; *next != '\0' && uqm::getCharFromString((const char**)&next) != (isPC(uqm::UQMOptions::read().whichFonts) ? UnicharSpace : UnicharTab);
		 buf = next)
		;
	if (*buf != '\0')
	{ // found possibly separating ' '
		sep = buf;
		// skip separating space
		for (buf = next; *next != '\0' && uqm::getCharFromString((const char**)&next) == (isPC(uqm::UQMOptions::read().whichFonts) ? UnicharSpace : UnicharTab);
			 buf = next)
			;
	}

	if (*buf == '\0')
	{ // reached the end -- cluster only
		pSS->Cluster = pSS->Prefix;
		pSS->ClusterLen = uqm::utf8StringCount(pSS->Cluster);
		pSS->ClusterPos = uqm::utf8StringCount(uqstl::string_view {pSS->Buffer.data(), static_cast<size_t>(pSS->Cluster - pSS->Buffer.data())});
		pSS->Prefix = 0;
		return;
	}

	// consider the rest cluster name (whatever there is)
	pSS->Cluster = buf;
	pSS->ClusterLen = uqm::utf8StringCount(pSS->Cluster);
	pSS->ClusterPos = uqm::utf8StringCount(uqstl::string_view {pSS->Buffer.data(), static_cast<size_t>(pSS->Cluster - pSS->Buffer.data())});
	*sep = '\0'; // split
	pSS->PrefixLen = uqm::utf8StringCount(pSS->Prefix);
}

static inline int SkipStarCluster(int* sortedStars, int istar)
{
	const int Postfix = star_array[sortedStars[istar]].Postfix;

	for (++istar; istar < NumSolarSystems && star_array[sortedStars[istar]].Postfix == Postfix;
		 ++istar)
		;
	return istar;
}

static int FindNextStarIndex(StarSearchState* pSS, int from, bool WithinClust)
{
	int i {};

	if (!pSS->Cluster)
	{
		return -1; // nothing to search for
	}

	for (i = from; i < NumSolarSystems; ++i)
	{
		const StarDesc* SDPtr = &star_array[pSS->SortedStars[i]];
		uqstl::array<char, StarSearchBufsize> FullName;
		const char* ClusterName = GameString(SDPtr->Postfix);
		const char* sptr {};
		const char* dptr {};
		int dlen {};
		int c {};

		dlen = uqm::utf8StringCount(ClusterName);
		if (pSS->ClusterLen > dlen)
		{ // no match, skip the rest of cluster
			i = SkipStarCluster(pSS->SortedStars.data(), i) - 1;
			continue;
		}

		for (c = 0, sptr = pSS->Cluster, dptr = ClusterName;
			 c < pSS->ClusterLen; ++c)
		{
			const uqm::UniChar sc = uqm::getCharFromString(&sptr);
			const uqm::UniChar dc = uqm::getCharFromString(&dptr);

			if (uqm::toUpperChar(sc) != uqm::toUpperChar(dc))
			{
				break;
			}
		}

		if (c < pSS->ClusterLen)
		{ // no match here, skip the rest of cluster
			i = SkipStarCluster(pSS->SortedStars.data(), i) - 1;
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
				i = SkipStarCluster(pSS->SortedStars.data(), i) - 1;
				continue;
			}
		}

		// check prefix
		GetClusterName(SDPtr, FullName.data());
		dlen = uqm::utf8StringCount(FullName.data());
		if (pSS->PrefixLen > dlen)
		{
			continue;
		}

		for (c = 0, sptr = pSS->Prefix, dptr = FullName.data();
			 c < pSS->PrefixLen; ++c)
		{
			const uqm::UniChar sc = uqm::getCharFromString(&sptr);
			const uqm::UniChar dc = uqm::getCharFromString(&dptr);

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

	return (i < NumSolarSystems) ? i : -1;
}

static void DrawMatchedStarName(TextEntryState* pTES)
{
	StarSearchState* pSS = (StarSearchState*)pTES->CbParam;
	uqstl::array<char, StarSearchBufsize> buf {};
	int16_t ExPos = 0;
	int16_t CurPos = -1;
	const StarDesc* SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
	SisMessageFlags flags {};

	if (pSS->SingleClust || pSS->SingleMatch)
	{ // draw full star name
		GetClusterName(SDPtr, buf.data());
		ExPos = -1;
		flags = SisMessageFlags::SetFlashRect;
	}
	else
	{ // draw substring match
		char* pstr = buf.data();

		uqm::strncpy_safe(buf, pSS->Text.data());
		ExPos = pSS->ClusterPos;
		pstr = uqm::skipUTF8Chars(pstr, pSS->ClusterPos);

		uqm::strncpy_safe({pstr, sizeof(buf) - uqstl::distance(buf.data(), pstr)}, pSS->Text.data());
		ExPos += pSS->ClusterLen;
		CurPos = pTES->CursorPos;

		flags = SisMessageFlags::ClearFlashRect;
		if (pTES->JoystickMode)
		{
			flags |= SisMessageFlags::BlockCursor;
		}
	}

	DrawSISMessageEx(buf.data(), CurPos, ExPos, flags);
	DrawHyperCoords(cursorLoc);
}

static void MatchNextStar(StarSearchState* pSS, bool Reset)
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
		uqm::strncpy_safe(pSS->Buffer, pSS->Text.data());
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

static bool OnStarNameChange(TextEntryState* pTES)
{
	StarSearchState* pSS = (StarSearchState*)pTES->CbParam;
	SisMessageFlags flags {};
	bool ret = true;

	if (strncmp(pSS->Text.data(), pSS->LastText.data(), sizeof(pSS->Text)) != 0)
	{ // string changed
		pSS->LastChangeTime = GetTimeCounter();
		uqm::strncpy_safe(pSS->LastText, pSS->Text.data());

		// reset the search
		MatchNextStar(pSS, true);
	}

	if (pSS->CurIndex < 0)
	{ // nothing found
		if (pSS->Text[0] == '\0')
		{
			flags = SisMessageFlags::SetFlashRect;
		}
		else
		{
			flags = SisMessageFlags::ClearFlashRect;
		}
		if (pTES->JoystickMode)
		{
			flags |= SisMessageFlags::BlockCursor;
		}

		ret = DrawSISMessageEx(pSS->Text.data(), pTES->CursorPos, -1, flags);
	}
	else
	{
		// move the cursor to the found star
		const StarDesc* SDPtr {&star_array[pSS->SortedStars[pSS->CurIndex]]};
		UpdateCursorLocation(0, 0, &SDPtr->star_pt);

		DrawMatchedStarName(pTES);
		UpdateFuelRequirement();
	}

	return ret;
}

static bool OnStarNameFrame(TextEntryState* pTES)
{
	StarSearchState* pSS = (StarSearchState*)pTES->CbParam;

	if (PulsedInputState.menu[*MenuKey::MenuNext])
	{ // search for next match

		MatchNextStar(pSS, false);

		if (pSS->CurIndex < 0)
		{ // nothing found
			if (PulsedInputState.menu[*MenuKey::MenuNext])
			{
				PlayMenuSound(MenuSoundEffect::Failure);
			}
			return true;
		}

		// move the cursor to the found star
		const StarDesc* SDPtr {&star_array[pSS->SortedStars[pSS->CurIndex]]};
		UpdateCursorLocation(0, 0, &SDPtr->star_pt);

		DrawMatchedStarName(pTES);
		UpdateFuelRequirement();
	}

	flashCurrentLocation(nullptr, false);

	SleepThread(c_gameSecond / 30);

	return true;
}

bool coords_only(char* s)
{
	uint8_t i {};
	uint8_t count = 0;
	uint8_t countD = 0;
	uint8_t countC = 0;
	const uint8_t j = (uint8_t)strlen(s);
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

static bool DoStarSearch(MenuState* pMS)
{
	TextEntryState tes;
	StarSearchState searchState {};
	StarSearchState* pss {&searchState};

	DrawSISMessageEx("", 0, 0, SisMessageFlags::SetFlashRect);

	TextEntry3DO = (bool)is3DO(uqm::UQMOptions::read().whichFonts);

	pss->pMS = pMS;
	pss->LastChangeTime = TimeCount {};
	pss->Text[0] = '\0';
	pss->LastText[0] = '\0';
	pss->FirstIndex = -1;
	SortStarsOnName(pss);

	// text entry setup
	tes.Initialized = false;
	tes.BaseStr = pss->Text.data();
	tes.MaxSize = sizeof(pss->Text);
	tes.CursorPos = 0;
	tes.CbParam = pss;
	tes.ChangeCallback = OnStarNameChange;
	tes.FrameCallback = OnStarNameFrame;

	SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
	SetDefaultMenuRepeatDelay();
	bool success {DoTextEntry(&tes)};

	if (coords_only(tes.BaseStr))
	{
		GfxPoint coord;

		{
			uqstl::vector<uqstl::string> parts;
			uqm::tokenize(uqstl::string_view {tes.BaseStr}, parts, ':', false);
			coord.x = (int16_t)(atof(parts[0].c_str()) * 10);
			coord.y = (int16_t)(atof(parts[1].c_str()) * 10);
		}

		if (coord.x > MaxXUniverse || coord.y > MaxYUniverse
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

	DrawSISMessageEx(pss->Text.data(), -1, -1, SisMessageFlags::ClearFlashRect);

	TextEntry3DO = false;

	return success;
}

void DoBubbleWarp(bool UseFuel)
{
	PlayMenuSound(MenuSoundEffect::BubbleWarp);

	if (UseFuel)
	{
		DeltaSISGauges(0, -(int)FuelRequired(), 0);
	}

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary)
	{
		// We're in a solar system; exit it.
		uqm::currentActivity() |= EndInterplanetary;
		// Set a hook to move to the new location:
		debugHook = doInstantMove;
	}
	else
	{ // Move to the new location immediately.
		doInstantMove();
	}
}

static void AdvancedAutoPilot(void)
{
	const uint16_t portalFuelCost {difficultyCase<uint16_t>(5, 20, 20)};
	// (Based on the Python QuasiSpace Calculator by Lukrative525
	// https://www.reddit.com/r/starcontrol/comments/zpyhkc/quasispace_calculator/
	GfxPoint current_position;
	const GfxPoint destination = uqm::flagship().autopilot.target();
	GfxPoint portal_coordinates;
	double distance {NAN};
	double fuel_no_portal {NAN};
	double fuel_with_portal {NAN};
	double minimum = 0.0;
	uint8_t index = 0;
	const uint16_t KnownQSPortals = uqm::gameState().flag(uqm::GameFlag::KNOW_QS_PORTAL);

	current_position.x = LogxToUniverse(uqm::gameState().sis().log_x);
	current_position.y = LogyToUniverse(uqm::gameState().sis().log_y);

	if (pointsEqual(current_position, destination))
	{
		return;
	}

	for (uint8_t i {0}; i < NumHyperVortices; i++)
	{
		distance = ptDistance(destination, portal_map[i].star_pt);

		if (!isDifficulty(uqm::Difficulty::Easy) && !(KnownQSPortals & (1 << i)))
		{
			distance = MaxXUniverse * MaxYUniverse;
		}

		if (i == 0 || distance < minimum)
		{
			minimum = distance;
			index = i + 1;
		}
	}

	portal_coordinates = star_array[NumSolarSystems + index].star_pt;

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
		uqm::flagship().autopilot.setTarget(current_position);
	}

	if (inHyperSpace())
	{
		InvokeSpawner();
	}
}

static constexpr auto MinAccelDelay {c_gameSecond / 60};
static constexpr auto MaxAccelDelay {c_gameSecond / 8};
static constexpr auto StepAccelDelay {c_gameSecond / 120};

static bool DoMoveCursor(MenuState* pMS)
{
	static uqstl::array<char, CursorInfoBufsize> last_buf;
	const TimeCount TimeIn {GetTimeCounter()};
	static uint16_t moveRepeats;
	bool isMove = false;

	if (!pMS->Initialized)
	{
		GfxPoint universe;

		pMS->Initialized = true;
		pMS->InputFunc = DoMoveCursor;

		if (!inHQSpace())
		{
			universe = CurStarDescPtr()->star_pt;
		}
		else
		{
			universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
			universe.y = LogyToUniverse(uqm::gameState().sis().log_y);
		}
		flashCurrentLocation(&universe, false);

		last_buf[0] = '\0';
		UpdateCursorInfo(last_buf);
		UpdateFuelRequirement();

		return true;
	}
	else if (PulsedInputState.menu[*MenuKey::MenuCancel])
	{
		FlushInput();

		if ((uqm::UQMOptions::read().bubbleWarp && !uqm::UQMOptions::read().infiniteFuel && !inQuasiSpace())
			&& uqm::flagship().autopilot.hasTarget()
			&& uqm::gameState().sis().FuelOnBoard >= FuelRequired())
		{
			DoBubbleWarp(true);
		}

		if (!inQuasiSpace()
			&& ValidPoint(uqm::flagship().autopilot.target()))
		{
			if (uqm::UQMOptions::read().smartAutoPilot)
			{
				SaveLastLoc(
					MAKE_POINT(
						LogxToUniverse(uqm::gameState().sis().log_x),
						LogyToUniverse(uqm::gameState().sis().log_y)));
			}

			if (uqm::UQMOptions::read().advancedAutoPilot
				&& uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP))
			{
				AdvancedAutoPilot();
			}
		}

		return false;
	}
	else if (PulsedInputState.menu[*MenuKey::MenuSelect])
	{
		/*printf ("Fuel Available: {} | Fuel Requirement: {}\n",
				GLOBAL_SIS (FuelOnBoard), FuelRequired());*/

		FlushInput();

		if (uqm::UQMOptions::read().bubbleWarp && (uqm::UQMOptions::read().infiniteFuel || inQuasiSpace()))
		{
			uqm::flagship().autopilot.setTarget(cursorLoc);
			DoBubbleWarp(false);
			return false;
		}
		else
		{
			if (uqm::flagship().autopilot.target().x == cursorLoc.x
				&& uqm::flagship().autopilot.target().y == cursorLoc.y)
			{
				uqm::flagship().autopilot.clear();
			}
			else
			{
				uqm::flagship().autopilot.setTarget(cursorLoc);
			}
			DrawStarMap(0, nullptr);
		}
	}
	else if (PulsedInputState.menu[*MenuKey::MenuSearch]
			 || (PulsedInputState.menu[*MenuKey::MenuZoomIn]
				 && PulsedInputState.menu[*MenuKey::MenuZoomOut]))
	{
		FlushInput();

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{ // HyperSpace search
			const GfxPoint oldpt = cursorLoc;

			if (!DoStarSearch(pMS))
			{ // search failed or canceled - return cursor
				UpdateCursorLocation(0, 0, &oldpt);
			}
			FlushCursorRect();
			// make sure cmp fails
			uqm::strncpy_safe(last_buf, "  <random garbage>  ");
			UpdateCursorInfo(last_buf);
			UpdateFuelRequirement();

			SetMenuRepeatDelay(MinAccelDelay, MaxAccelDelay,
							   StepAccelDelay, true);
			SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
		}
		else
		{ // no search in QuasiSpace
			PlayMenuSound(MenuSoundEffect::Failure);
		}
	}
	else if (PulsedInputState.menu[*MenuKey::MenuTogglemap])
	{
		FlushInput();

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			uint8_t NewState {};
			NewState = *which_starmap;

			if (NewState == StarmapView::RainbowMap)
			{
				NewState = *StarmapView::NormalStarmap;
			}
			else
			{
				++NewState;
			}

			if (NewState != which_starmap)
			{
				which_starmap = (StarmapView)NewState;
			}

			PlayMenuSound(MenuSoundEffect::Move);

			DrawStarMap(0, nullptr);
			last_buf[0] = '\0';
			UpdateCursorInfo(last_buf);
		}
		else
		{ // no alternate maps in QuasiSpace
			PlayMenuSound(MenuSoundEffect::Failure);
		}
	}
	else if (PulsedInputState.menu[*MenuKey::MenuSpecial])
	{
		FlushInput();

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			setStarMarked(starIndex(cursorLoc), "PLYR_MARKER");

			DrawStarMap(0, nullptr);
		}
		else
		{
			PlayMenuSound(MenuSoundEffect::Failure);
		}
	}
	else if (PulsedInputState.menu[*MenuKey::MenuDelete])
	{
		FlushInput();

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			for (uint16_t i {0}; i <= NumSolarSystems; i++)
			{
				if (isStarMarked(i, "PLYR_MARKER"))
				{
					setStarMarked(i, "PLYR_MARKER");
					DrawStarMap(0, nullptr);
					SleepThread(c_gameSecond / 8);
				}
			}
		}
		else
		{
			PlayMenuSound(MenuSoundEffect::Failure);
		}
	}
	else
	{
		int8_t sx {};
		int8_t sy {};
		int16_t ZoomIn {};
		int16_t ZoomOut {};

		ZoomIn = ZoomOut = 0;
		if (PulsedInputState.menu[*MenuKey::MenuZoomIn])
		{
			ZoomIn = 1;
		}
		else if (PulsedInputState.menu[*MenuKey::MenuZoomOut])
		{
			ZoomOut = 1;
		}

		ZoomStarMap(ZoomIn - ZoomOut);

		sx = sy = 0;
		if (PulsedInputState.menu[*MenuKey::MenuLeft])
		{
			sx = -1;
		}
		if (PulsedInputState.menu[*MenuKey::MenuRight])
		{
			sx = 1;
		}
		if (PulsedInputState.menu[*MenuKey::MenuUp])
		{
			sy = -1;
		}
		if (PulsedInputState.menu[*MenuKey::MenuDown])
		{
			sy = 1;
		}

		// Double the cursor speed when the "Next" key is held down
		if (DirKeysPress() && CurrentInputState.menu[*MenuKey::MenuNext])
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

		SleepThreadUntil(TimeIn + MinAccelDelay);
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

	return !(uqm::currentActivity() & ActivityFlag::CheckAbort);
}

static void RepairMap(uint16_t update_race, GfxRect* pLastRect, GfxRect* pNextRect)
{
	GfxRect r;

	/* make a rect big enough for text */
	r.extent.width = 50;
	r.corner.x = (pNextRect->corner.x + (pNextRect->extent.width >> 1))
			   - (r.extent.width >> 1);
	if (r.corner.x < 0)
	{
		r.corner.x = 0;
	}
	else if (r.corner.x + r.extent.width >= SisScreenWidth())
	{
		r.corner.x = SisScreenWidth() - r.extent.width;
	}
	r.extent.height = 9;
	r.corner.y = (pNextRect->corner.y + (pNextRect->extent.height >> 1))
			   - r.extent.height;
	if (r.corner.y < 0)
	{
		r.corner.y = 0;
	}
	else if (r.corner.y + r.extent.height >= SisScreenHeight())
	{
		r.corner.y = SisScreenHeight() - r.extent.height;
	}
	BoxUnion(pLastRect, &r, &r);
	BoxUnion(pNextRect, &r, &r);
	*pLastRect = *pNextRect;

	if (r.corner.x < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	if (r.corner.x + r.extent.width > SisScreenWidth())
	{
		r.extent.width = SisScreenWidth() - r.corner.x;
	}
	if (r.corner.y < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	if (r.corner.y + r.extent.height > SisScreenHeight())
	{
		r.extent.height = SisScreenHeight() - r.corner.y;
	}

	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;

	DrawStarMap(update_race, &r);
}

static void UpdateMap(void)
{
	uint8_t ButtonState {};
	uint8_t VisibleChange {};
	bool MapDrawn {};
	bool Interrupted {};
	uint16_t index {};
	FleetInfoHandle hStarShip {};
	FleetInfoHandle hNextShip {};
	uint16_t GrowthFactor {};

	FlushInput();
	ButtonState = 1; /* assume a button down */

	MapDrawn = Interrupted = false;
	for (index = 1,
		hStarShip = GetHeadLink(&uqm::gameState().game().avail_race_q);
		 hStarShip; ++index, hStarShip = hNextShip)
	{
		FleetInfo* FleetPtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip)};
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
			int16_t dx {};
			int16_t dy {};
			int16_t delta {};
			GfxRect r;
			GfxRect last_r;
			GfxRect temp_r0;
			GfxRect temp_r1;
			uint16_t str {};

			dx = FleetPtr->loc.x - FleetPtr->known_loc.x;
			dy = FleetPtr->loc.y - FleetPtr->known_loc.y;
			if (dx || dy)
			{
				int16_t xincr {};
				int16_t yincr {};
				int16_t xerror {};
				int16_t yerror {};
				int16_t cycle {};

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
				last_r.extent.width += ResScale(1) + IfHd(1);  // dot in HdResolutionFactor is 5px wide for AA to work
				last_r.extent.height += ResScale(1) + IfHd(1); // so we have to add extra 1
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

					r.extent.width += ResScale(1) + IfHd(1);
					r.extent.height += ResScale(1) + IfHd(1);
					if (temp_r0.corner.x != temp_r1.corner.x
						|| temp_r0.corner.y != temp_r1.corner.y)
					{ // Ignore name stacking during movement
						VisibleChange = true;
						RepairMap(index | IgnoreMovingSoi, &last_r, &r);
						SleepThread(c_gameSecond / 24);
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
				last_r.extent.width += ResScale(1);
				last_r.extent.height += ResScale(1);
				// Kruzen: Font size to clean up double space because of text stacking now
				last_r.extent.height = uqstl::max(last_r.extent.height, static_cast<int16_t>(ResScale(14)));
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
					r.extent.width += ResScale(1);
					r.extent.height += ResScale(1);
					if ((temp_r0.extent.height != temp_r1.extent.height) && !(str > 0 && FleetPtr->known_strength == 0))
					{ // Update race SOI size IF the race didn't die out
						VisibleChange = true;
						RepairMap(index, &last_r, &r);
						SleepThread(
							c_gameSecond / (12 + GrowthFactor / 44));
					}
					else if (str > 0 && FleetPtr->known_strength == 0)
					{ // Flash dying race name
						VisibleChange = true;
						RepairMap(index | PreDeathSoi, &last_r, &r);
						SleepThread(c_gameSecond / 12);
						RepairMap(index | DeathSoi, &last_r, &r);
						SleepThread(c_gameSecond / 12);
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

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hStarShip);
	}
}

template <typename T>
auto ifGamepad(T gamepadValue, T keyboardValue = {0})
{
	return ResScale(uqm::UQMOptions::read().optControllerType != uqm::ControllerType::KeyboardMouse ? gamepadValue : keyboardValue);
}

static void DrawStarmapHelper(void)
{

	GfxStamp s;
	GfxText t;
	GfxRect r;
	int16_t leading {};

	GfxContextRef OldContext {SetContext(StatusContext)};

	BatchGraphics();

	DrawFlagStatDisplay(GameString(GameStringBase::StatusString + 7));

	SetContextFont(TinyFont);
	GetContextFontLeading(&leading);

	r.corner.x = ResScale(4);
	r.corner.y = ResScale(34);

	const int frame_index {uqm::UQMOptions::read().optControllerType == uqm::ControllerType::KeyboardMouse ? 0 : 5 * uqstl::to_underlying(uqm::UQMOptions::read().optControllerType.value)};

	// :Maps
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index);
	s.origin = r.corner;
	DrawStamp(&s);

	SetContextForeGroundColor(ModuleNameColor);
	t.align = TextAlign::Left;
	t.baseline.x = r.corner.x + ResScale(12) - ifGamepad(2);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GameString(GameStringBase::StatusString + 12);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// :Add->O
	s.origin.y += ResScale(10);
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 1);
	DrawStamp(&s);

	t.align = TextAlign::Left;
	t.baseline.x = r.corner.x + ResScale(16) - ifGamepad(6);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GameString(GameStringBase::StatusString + 13);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	s.origin.x = t.baseline.x + ResScale(28);
	s.origin.y += ResScale(4);
	s.frame = SetAbsFrameIndex(MiscDataFrame, 108);
	DrawStamp(&s);

	// Cursor Speed
	s.origin.x = r.corner.x;
	s.origin.y += ResScale(6);
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 2);
	DrawStamp(&s);

	t.align = TextAlign::Left;
	t.baseline.x = r.corner.x + ResScale(18) - ifGamepad(8);
	t.baseline.y = s.origin.y + leading;
	t.pStr = ":";
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	s.origin.x = t.baseline.x + ResScale(7);
	s.origin.y += ResScale(4);
	s.frame = SetAbsFrameIndex(MiscDataFrame, 48);
	DrawStamp(&s);

	t.align = TextAlign::Left;
	t.baseline.x += ResScale(13);
	t.pStr = GameString(GameStringBase::StatusString + 14);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// :Search
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 3);
	s.origin.x = r.corner.x;
	s.origin.y += ResScale(7) - ifGamepad(1);
	DrawStamp(&s);

	t.baseline.x = s.origin.x + ResScale(12) + ifGamepad(11);
	t.baseline.y = s.origin.y + leading + ResScale(5) - ifGamepad(5);
	t.pStr = GameString(GameStringBase::StatusString + 15);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// :Zoom
	s.frame = SetAbsFrameIndex(SubmenuFrame, frame_index + 4);
	s.origin.y += ResScale(21) - ifGamepad(11);
	DrawStamp(&s);

	t.baseline.x = s.origin.x + ResScale(30) - ifGamepad(18);
	t.baseline.y = s.origin.y + leading + ResScale(5);
	t.pStr = GameString(GameStringBase::StatusString + 16);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// Separator
	r.corner.y = ResScale(118);
	r.extent.width = FieldWidth() - ResScale(3);
	r.extent.height = ResScale(1);
	SetContextForeGroundColor(CargoSelectedBackColor);
	DrawFilledRectangle(&r);

	// FUEL:
	SetContextForeGroundColor(ModuleNameColor);
	t.align = TextAlign::Left;
	t.baseline.x = r.corner.x + ResScale(2);
	t.baseline.y = r.corner.y + leading + ResScale(1);
	t.pStr = GameString(GameStringBase::StatusString + 8); // FUEL:
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	// Current amount of fuel
	SetContextForeGroundColor(ModulePriceColor);
	t.align = TextAlign::Right;
	t.baseline.x = r.extent.width + ResScale(2);
	t.baseline.y = r.corner.y + leading + ResScale(1);
	t.pStr = WholeFuelValue();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	UnbatchGraphics();

	SetContext(OldContext);
}

bool StarMap(void)
{
	MenuState menuState;
	GfxPoint universe;
	//FrameRef OldFrame;
	GfxRect clip_r;

	memset(&menuState, 0, sizeof(menuState));

	which_starmap = StarmapView::NormalStarmap;

	zoomLevel = 0;
	mapOrigin.x = MaxXUniverse >> 1;
	mapOrigin.y = MaxYUniverse >> 1;
	StarMapFrame = SetAbsFrameIndex(MiscDataFrame, 48);

	if (!inHQSpace())
	{
		universe = CurStarDescPtr()->star_pt;
	}
	else
	{
		universe.x = LogxToUniverse(uqm::gameState().sis().log_x);
		universe.y = LogyToUniverse(uqm::gameState().sis().log_y);
	}

	cursorLoc = uqm::flagship().autopilot.target();
	if (cursorLoc.x == ~0 && cursorLoc.y == ~0)
	{
		cursorLoc = universe;
	}

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		if (playerInPlanetOrbit())
		{
			DrawMenuStateStrings(MenuString::AltScan, 1);
		}
		else
		{
			DrawMenuStateStrings(MenuString::AltStarmap, 0);
		}
	}

	menuState.InputFunc = DoMoveCursor;
	menuState.Initialized = false;

	transition_pending = true;
	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
	{
		UpdateMap();
	}
	else
	{ // This zooms the Quasi map in by 2 if within the local Quasi star
		// cluster.
		if ((universe.x <= ArilouHomeX && universe.y <= ArilouHomeY)
			&& (universe.x >= 4480 && universe.y >= 4580))
		{
			zoomLevel = 2;
		}
	}

	if (uqm::UQMOptions::read().submenu)
	{
		DrawStarmapHelper();
	}

	DrawStarMap(0, (GfxRect*)-1);
	transition_pending = false;

	BatchGraphics();
	GfxContextRef OldContext {SetContext(SpaceContext)};
	GetContextClipRect(&clip_r);
	SetContext(OldContext);
	LoadIntoExtraScreen(&clip_r);
	DrawCursor(UniverseToDispx(cursorLoc.x),
			   UniverseToDispy(cursorLoc.y));
	UnbatchGraphics();

	SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
	SetMenuRepeatDelay(MinAccelDelay, MaxAccelDelay, StepAccelDelay,
					   true);

	DoInput(&menuState, false);

	SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
	SetDefaultMenuRepeatDelay();

	DrawHyperCoords(universe);
	if (uqm::flagship().autopilot.hasTarget())
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
		DeltaSISGauges(UndefinedDelta, UndefinedDelta, UndefinedDelta);
	}

	/*if (GLOBAL (autopilot.x) == universe.x
			&& GLOBAL (autopilot.y) == universe.y)
		GLOBAL (autopilot.x) = GLOBAL (autopilot.y) = ~0;*/

	return (uqm::flagship().autopilot.hasTarget());
}

} // namespace uqm
