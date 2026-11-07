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

// BW 2011: fixed using pixmaps to define planet surfaces so it actually works
// on planets you can land on. The second frame of planetmask.ani has to be a
// (Black & White) indexed pic with 128 colors top. Lesser indices (black)
// will correspond to lower altitudes.

#include "planets.h"
#include "scan.h"
#include "../nameref.h"
#include "../resinst.h"
#include "../setup.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
#include "../starmap.h"
#include "../gendef.h"
#include "../colors.h"
#include <math.h>
#include <time.h>

namespace uqm
{
#undef PROFILE_ROTATION

// define USE_ALPHA_SHIELD to use an aloha overlay instead of
// an additive overlay for the shield effect
#undef USE_ALPHA_SHIELD

static constexpr auto ShieldGlowComp {120};
static constexpr auto ShieldReflectComp {100};

static constexpr auto NumBatchPoints {64};
static auto Radius() { return ResScale(33); }
//2*Radius()
static auto Tworadius() { return (Radius() << 1); }
//Radius()^2
constexpr auto Radius2(const auto& a) { return (a * a); }
// distance beyond which all pixels are transparent (for aa)
constexpr auto RadiusThres(const auto& a) { return ((a + 1) * (a + 1)); }

#if 0
static auto SphereSpanX() { return (MapWidth() >> 1); }
#else
static auto SphereSpanX() { return (MapHeight()); }
#endif
// XXX: technically, the sphere's span over X should be MapWidth()/2
// but this causes visible surface compression over X, because
// the surface dims ratio is H x H*PI, instead of H x 2*H
// see bug #885

static constexpr auto DiffuseBits {16};
static constexpr auto AaWeightBits {16};

#ifndef M_TWOPI
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_TWOPI (M_PI * 2.0)
#endif
#ifndef M_DEG2RAD
#define M_DEG2RAD (M_TWOPI / 360.0)
#endif

// BW: dynamically allocated in the orbit structure
// JMS_GFX: Changed initialization to constant numbers since Diameter is
// now variably defined The value 330 is the value that's reached at the
// biggest resolution, 4x.
// uint32_t light_diff[330][330]; //uint32_t light_diff[Diameter][Diameter];

// BW: Moved to planets.h
// typedef struct
// {
// 	GfxPoint p[4];
// 	uint32_t m[4];
// } MapPoint3D;

// BW: dynamically allocated in the orbit structure
// JMS_GFX: Changed initialization to constant numbers since Diameter is
// now variably defined The value 330 is the value that's reached at the
// biggest resolution, 4x.
// MapPoint3D map_rotate[330][330];
// //MapPoint3D map_rotate[Diameter][Diameter];

struct Point3D
{
	double x, y, z;
};

const static Color tintColors[] =
	{
		ScanMineralTintColor,
		ScanEnergyTintColor,
		ScanBiologicalTintColor,
};

uint8_t clip_channel(int c)
{
	if (c > 255)
	{
		c = 255;
	}
	if (c < 0)
	{
		c = 0;
	}
	return c;
}

void TransformColor(Color* c, uint16_t scan)
{
	if (scan == ScanType::Count)
	{
		return;
	}

	c->r = c->g = c->b = (c->r + c->g + c->b) / 3;

	switch (static_cast<ScanType>(scan))
	{
		case ScanType::Mineral:
			{
				c->b = 0x00;
				c->g = 0x00;
				break;
			}
		case ScanType::Energy:
			{
				c->r = (c->r / 3) * 2;
				c->b = c->r;
				c->g = c->r;
				break;
			}
		case ScanType::Biological:
			{
				c->r = 0x00;
				c->b = 0x00;
				break;
			}
		default:
			break;
	}
}

int8_t ColorDelta(uint16_t scan, uint32_t avg)
{
	if (scan == ScanType::Count)
	{
		return 0;
	}
	else
	{
		int8_t diff = 0;

		if (avg > (uint32_t)MaxBrightness(scan))
		{
			diff = -(int8_t)(avg - MaxBrightness(scan));
		}
		else if (avg < (uint32_t)MinBrightness(scan))
		{
			diff = MinBrightness(scan) - avg;
		}

		return diff;
	}
}

void RepairColorOpacity(Color* TopoColors, int width, int height)
{
	int x {};
	int y {};

	Color* c {TopoColors};

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++, c++)
		{
			if (c->a != 0xFF)
			{
				c->a = 0xFF;
			}
		}
	}
}

void SetPlanetColors(COLORMAPPTR cmap)
{ // Setting colors for table that is used to color spheres
	// Hacky, but imitates the original rather well
	// That way we can have only 1 set of spheres and color them
	// appropriately
	uint8_t* ctab {};
	const uint16_t numcolors = 128;
	uqstl::array<Color, numcolors> colors {};

	Color* c {colors.data()};
	uint8_t* cbase {(uint8_t*)GetColorMapAddress(pSolarSysState->OrbitalCMap.get(), pSolarSysState->OrbitalCMapIndex)};

	for (uint16_t i {0}; i < numcolors; ++i, ++c)
	{
		ctab = (cbase + 2) + i * 3;

		*c = BuildColorRgba(ctab[0], ctab[1], ctab[2], 0xFF);
	}

	SetColorMapColors(colors.data(), cmap, 0, numcolors);
}

void AdjustColor(Color* c, uint16_t scan, uint16_t height, uint16_t width, int8_t diff)
{
	uint16_t x {};

	for (uint16_t y {0}; y < height; ++y)
	{
		for (x = 0; x < width; ++x, ++c)
		{
			switch (static_cast<ScanType>(scan))
			{
				case ScanType::Mineral:
					{
						c->r += diff;
						break;
					}
				case ScanType::Energy:
					{
						c->r += diff;
						c->g += diff;
						c->b += diff;
						break;
					}
				case ScanType::Biological:
					{
						c->g += diff;
						break;
					}
				default:
					break;
			}
		}
	}
}

RESOURCE GetAltColorMap(SystemPlanet* pPlanetDesc)
{
	switch (static_cast<PlanetType>(pPlanetDesc->data_index))
	{
		case PlanetType::Metal: /* MERCURY */
			return MercuryColorTabRes;
		case PlanetType::Primordial: /* VENUS */
			return VenusColorTabRes;
		case PlanetType::Dust: /* MARS */
			return MarsColorTabRes;
		case PlanetType::Pellucid: /* PLUTO & CALLISTO */
			{
				if (worldIsPlanet(pSolarSysState, pPlanetDesc))
				{
					return PlutoColorTabRes;
				}
				else
				{
					return CallistoColorTabRes;
				}
			}
		case PlanetType::Selenic: /* LUNA & CHARON */
			{
				if (planetIndex(pSolarSysState, pPlanetDesc) == 2)
				{
					return LunaColorTabRes;
				}
				else
				{
					return CharonColorTabRes;
				}
			}
		case PlanetType::Radioactive: /* IO */
			return IoColorTabRes;
		case PlanetType::Halide: /* EUROPA */
			return EuropaColorTabRes;
		case PlanetType::Cyanic: /* GANYMEDE */
			return GanymedeColorTabRes;
		case PlanetType::Alkali: /* TITAN */
			return TitanColorTabRes;
		case PlanetType::Vinylogous: /* TRITON */
			return TritonColorTabRes;
		default:
			return nullptr;
	}
}

static void ExpandLevelMasks(PlanetOrbit* Orbit)
{ // Expand mask frame to avoid null spaces
	uint16_t spherespanx {};
	int16_t width {};
	int16_t height {};
	uint32_t halfBound {};
	const PlanetData* PlanetInfo {};

	PlanetInfo = &pSolarSysState->SysInfo.PlanetInfo;
	spherespanx = Orbit->SphereFrame->Bounds.width;
	width = Orbit->TopoMask->Bounds.width;
	height = Orbit->TopoMask->Bounds.height;

	halfBound = width + spherespanx;
	const uint32_t colorSize {height * (halfBound)};

	uqstl::vector<Color> colorBuf(colorSize);
	Color* colors {colorBuf.data()};

	ReadFramePixelColors(Orbit->TopoMask, colors, halfBound, height);

	// Destroy mask to recreate it later
	DestroyDrawable(ReleaseDrawable(Orbit->TopoMask));
	Orbit->TopoMask = 0;

	for (uint32_t y {0}; y < colorSize; y += halfBound)
	{
		memcpy(colors + y + width, colors + y,
			   spherespanx * sizeof(Color));
	}

	if (PlanetInfo->AxialTilt == 0)
	{
		Orbit->TopoMask = CaptureDrawable(
			CreateDrawable(WantPixmap, halfBound, height, 1));

		WriteFramePixelColors(Orbit->TopoMask, colors, halfBound, height);
	}
	else
	{ // Because sphere frame if 75x67 we need to expand it's height to
		// avoid information loss and flat lines on the edges of the sphere
		// itself
		FrameRef dupe {};
		double err {NAN};

		err = sin(M_DEG2RAD * (double)PlanetInfo->AxialTilt)
			* (spherespanx - height);

		if (err < 0)
		{
			err = -err;
		}

		err = ceil(err);
		// Get the closest acceptable value (always ceil)

		dupe = CaptureDrawable(
			CreateDrawable(WantPixmap, halfBound, height, 1));
		WriteFramePixelColors(dupe, colors, halfBound, height);

		Orbit->TopoMask = CaptureDrawable(
			RescaleFrame(dupe, halfBound, height + (int16_t)err));
		DestroyDrawable(ReleaseDrawable(dupe));
	}
}

static void RenderLevelMasks(FrameRef mask, int8_t* pTopoData, bool SurfDef)
{ // Kruzen: Originally there were 3 frames for levels 2,3,4 that was
	// drawn backwards using masks and prerendered colored spheres
	// Mask code has been nuked some time ago therefore I'm rendering 1
	// frame and using RED channel as offset multiplier I need it to be
	// frame because I need to rotate it later in case of orbital tilt
	uint16_t i {};
	int16_t base {};
	int16_t d {};
	GfxPoint pt;
	Primitive BatchArray[NumBatchPoints];
	const int8_t* pSrc {};
	GfxPoint oldOrigin;
	GfxRect ClipRect;
	int16_t num_frames = 3;
	const int16_t* level_tab {};

	FrameRef OldFrame {SetContextFGFrame(mask)};
	oldOrigin = SetContextOrigin(MAKE_POINT(0, 0));
	GetContextClipRect(&ClipRect);
	SetContextClipRect(nullptr);

	const int16_t w {mask->Bounds.width};
	const int16_t h {mask->Bounds.height};

	Primitive* pBatch {&BatchArray[0]};
	for (i = 0; i < NumBatchPoints; ++i, ++pBatch)
	{
		SetPrimNextLink(pBatch, i + 1);
		SetPrimType(pBatch, PrimitiveType::Point);
		SetPrimFlags(pBatch, 0);
	}
	SetPrimNextLink(&pBatch[-1], EndOfList);

	const PlanetFrame* PlanDataPtr {&PlanData[pSolarSysState->pOrbitalDesc->data_index & ~*PlanetType::PlanetShielded]};
	const uint8_t AlgoType {Planalgo(PlanDataPtr->Type)};

	if (SurfDef)
	{ // Planets given by a pixmap have elevations between -128 and +128
		base = 256;
	}
	else
	{
		base = PlanDataPtr->base_elevation;
	}

	const PlanetColorTranslation* xlatDesc {(const PlanetColorTranslation*)pSolarSysState->XlatPtr};
	level_tab = (const int16_t*)xlatDesc->level_tab;

	while (num_frames > 0)
	{
		num_frames--;
		i = NumBatchPoints;
		pBatch = &BatchArray[i];

		pSrc = pTopoData;
		for (pt.y = 0; pt.y < h; ++pt.y)
		{
			for (pt.x = 0; pt.x < w; ++pt.x)
			{
				d = *pSrc++;
				d += base;

				if (AlgoType == GasGiantAlgo)
				{ // make elevation value non-negative
					d &= 255;
				}
				else
				{
					if (d < 0)
					{
						d = 0;
					}
					else if (d > 255)
					{
						d = 255;
					}
				}

				if (d >= level_tab[num_frames]
					&& (num_frames == 2
						|| d < level_tab[num_frames + 1]))
				{
					--pBatch;
					pBatch->Object.Point.x = pt.x;
					pBatch->Object.Point.y = pt.y;

					SetPrimColor(pBatch,
								 BuildColorRgba(0x01 * (num_frames + 1),
												0x00, 0x00, 0xFF));

					if (--i == 0)
					{ // flush the batch and start the next one
						DrawBatch(BatchArray, 0, 0);
						i = NumBatchPoints;
						pBatch = &BatchArray[i];
					}
				}
			}
		}

		if (i < NumBatchPoints)
		{
			DrawBatch(BatchArray, i, 0);
		}
	}
	SetContextClipRect(&ClipRect);
	SetContextOrigin(oldOrigin);
	SetContextFGFrame(OldFrame);
}

static void RenderTopography(FrameRef DstFrame, int8_t* pTopoData, int w, int h,
							 bool SurfDef, COLORMAPPTR scanColors)
{
	int16_t base {};
	int16_t d {};
	const uint8_t* ctab {};
	uint8_t* cbase {};
	uint32_t i {};
	uint32_t sqr {};
	const uint8_t* xlat_tab {};

	if (!pSolarSysState->XlatTable)
	{ // There is currently nothing we can do w/o an xlat table
		// This is still called for Earth for 4x scaled topo, but we
		// do not need it because we cannot land on Earth.
		uqm::log::warn(
			"No xlat table -- could not generate surface.\n");
		return;
	}

	const PlanetFrame* PlanDataPtr {&PlanData[pSolarSysState->pOrbitalDesc->data_index & ~*PlanetType::PlanetShielded]};
	const uint8_t AlgoType {Planalgo(PlanDataPtr->Type)};

	if (SurfDef)
	{ // Planets given by a pixmap have elevations between -128 and
		// +128
		base = 256;
	}
	else
	{
		base = PlanDataPtr->base_elevation;
	}

	const PlanetColorTranslation* xlatDesc {(const PlanetColorTranslation*)pSolarSysState->XlatPtr};
	xlat_tab = (const uint8_t*)xlatDesc->xlat_tab;

	if (scanColors == nullptr)
	{
		cbase = (uint8_t*)GetColorMapAddress(pSolarSysState->OrbitalCMap.get(), pSolarSysState->OrbitalCMapIndex);
	}
	else
	{
		cbase = (uint8_t*)scanColors;
	}

	const int8_t* pSrc {pTopoData};
	sqr = w * h;

	uqstl::vector<Color> mapBuf(sqr);
	Color* map {mapBuf.data()};
	Color* pix {map};

	for (i = 0; i < sqr; ++i, ++pSrc, ++pix)
	{
		d = *pSrc;
		if (AlgoType == GasGiantAlgo)
		{ // make elevation value non-negative
			if (uqm::UQMOptions::read().sphereType == 1)
			{
				d += base;
			}
			d &= 255;
		}
		else
		{
			d += base;
			if (d < 0)
			{
				d = 0;
			}
			else if (d > 255)
			{
				d = 255;
			}
		}

		d = xlat_tab[d] - cbase[0];
		ctab = (cbase + 2) + d * 3;

		// using new truecolor tables ripped from DOS version
		*pix = BuildColorRgb(ctab[0], ctab[1], ctab[2]);
	}

	WriteFramePixelColors(DstFrame, map, w, h);
}

static inline void P3mult(Point3D* res, Point3D* vec, double cnst)
{
	res->x = vec->x * cnst;
	res->y = vec->y * cnst;
	res->z = vec->z * cnst;
}

static inline void P3sub(Point3D* res, Point3D* v1, Point3D* v2)
{
	res->x = v1->x - v2->x;
	res->y = v1->y - v2->y;
	res->z = v1->z - v2->z;
}

static inline double P3dot(Point3D* v1, Point3D* v2)
{
	return (v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}

static inline void P3norm(Point3D* res, Point3D* vec)
{
	const double mag = sqrt(P3dot(vec, vec));
	P3mult(res, vec, 1 / mag);
}

static constexpr auto AmbientLight {0.1};
static constexpr auto LightZ {1.2};
static constexpr auto LightIntens {0.3};
static constexpr auto Mshi {2};

// GenerateSphereMask builds a shadow map for the rotating planet
//  loc indicates the planet's position relative to the sun
static void GenerateSphereMask(GfxPoint loc, uint16_t radius)
{
	GfxPoint pt;
	Point3D light;
	double lrad {NAN};
	const uint32_t step = 1 << DiffuseBits;
	int y {};
	int x {};
	const uint16_t tworadius = radius << 1;
	const uint16_t radius_thres = (radius + 1) * (radius + 1);
	const uint16_t radius_2 = radius * radius;

	// lrad is the distance from the sun to the planet
	lrad = sqrt(loc.x * loc.x + loc.y * loc.y);
	// light is the sun's position.  the z-coordinate is whatever
	// looks good
	light.x = -((double)loc.x);
	light.y = -((double)loc.y);
	light.z = LightZ * lrad;
	P3norm(&light, &light);

	for (pt.y = 0, y = -radius; pt.y <= tworadius; ++pt.y, y++)
	{
		const uint32_t y_2 = y * y;

		for (pt.x = 0, x = -radius; pt.x <= tworadius; ++pt.x, x++)
		{
			const uint32_t x_2 = x * x;
			const uint32_t rad_2 = x_2 + y_2;
			uint32_t diff_int = 0;
			Point3D norm;
			double diff {NAN};

			if (rad_2 < radius_thres)
			{
				// norm is the sphere's surface normal.
				norm.x = (double)x;
				norm.y = (double)y;
				norm.z = (sqrt(radius_2 - x_2) * sqrt(radius_2 - y_2)) / radius;
				P3norm(&norm, &norm);
				// diffuse component is norm dot light
				diff = P3dot(&norm, &light);
				// negative diffuse is bad
				if (diff < 0)
				{
					diff = 0.0;
				}
#if 0
				// Specular is not used in practice and is left here
				//  if someone decides to use it later for some reason.
				//  Specular highlight is only good for perfectly smooth
				//  surfaces, like balls (of which planets are not)
				//  This is the Phong equation
				double fb, spec;
				Point3D rvec;
				Point3D view;

				// always view along the z-axis
				// ideally use a view point, and have the view change
				// per pixel, but that is too much effort for now.
				// the view MUST be normalized!
				view.x = 0;
				view.y = 0;
				view.z = 1.0;

				// specular highlight is the phong equation:
				// (rvec dot view)^Mshi
				// where rvec = (2*diff)*norm - light (reflection of light
				// around norm)
				P3mult (&rvec, &norm, 2 * diff);
				P3sub (&rvec, &rvec, &light);
				fb = P3dot (&rvec, &view);
				if (fb > 0.0)
					spec = LightIntens * pow (fb, Mshi);
				else
					spec = 0;
#endif
				// adjust for the ambient light
				if (diff < AmbientLight)
				{
					diff = AmbientLight;
				}
				// Now we antialias the edge of the spere to look nice
				if (rad_2 > radius_2)
				{
					diff *= 1 - (sqrt(rad_2) - radius);
					if (diff < 0)
					{
						diff = 0;
					}
				}
				// diff_int allows us multiply by a ratio without using
				// floating-point.
				diff_int = (uint32_t)(diff * step);
			}

			pSolarSysState->Orbit.light_diff[pt.y][pt.x] = diff_int;
		}
	}
}

//create_aa_points creates weighted averages for
//  4 points around the 'ideal' point at x,y
//  the concept is to compute the weight based on the
//  distance from the integer location points to the ideal point
static void create_aa_points(MapPoint3D* ppt, double x, double y, uint16_t height)
{
	double deltax {NAN};
	double deltay {NAN};
	double inv_deltax {NAN};
	double inv_deltay {NAN};
	int16_t nextx {};
	int16_t nexty {};
	double d1 {NAN};
	double d2 {NAN};
	double d3 {NAN};
	double d4 {NAN};
	double m[4];
	const uint16_t spherespanx = height;

	if (x < 0)
	{
		x = 0;
	}
	else if (x >= spherespanx)
	{
		x = spherespanx - 1;
	}
	if (y < 0)
	{
		y = 0;
	}
	else if (y >= height)
	{
		y = height - 1;
	}

	// get  the integer value of this point
	ppt->p[0].x = (int16_t)x;
	ppt->p[0].y = (int16_t)y;
	deltax = x - ppt->p[0].x;
	deltay = y - ppt->p[0].y;

	// if this point doesn't need modificaton, set m[0]=0
	if (deltax == 0 && deltay == 0)
	{
		ppt->m[0] = 0;
		return;
	}

	// get the neighboring points surrounding the 'ideal' point
	if (deltax != 0)
	{
		nextx = ppt->p[0].x + 1;
	}
	else
	{
		nextx = ppt->p[0].x;
	}
	if (deltay != 0)
	{
		nexty = ppt->p[0].y + 1;
	}
	else
	{
		nexty = ppt->p[0].y;
	}
	//(x1,y)
	ppt->p[1].x = nextx;
	ppt->p[1].y = ppt->p[0].y;
	//(x,y1)
	ppt->p[2].x = ppt->p[0].x;
	ppt->p[2].y = nexty;
	//(x1y1)
	ppt->p[3].x = nextx;
	ppt->p[3].y = nexty;
	//the square  1x1, so opposite poinnts are at 1-delta
	inv_deltax = 1.0 - fabs(deltax);
	inv_deltax *= inv_deltax;
	inv_deltay = 1.0 - fabs(deltay);
	inv_deltay *= inv_deltay;
	deltax *= deltax;
	deltay *= deltay;
	//d1-d4 contain the distances from the poinnts to the ideal point
	d1 = sqrt(deltax + deltay);
	d2 = sqrt(inv_deltax + deltay);
	d3 = sqrt(deltax + inv_deltay);
	d4 = sqrt(inv_deltax + inv_deltay);
	//compute the weights.  the sum(ppt->m[])=65536
	m[0] = 1 / (1 + d1 * (1 / d2 + 1 / d3 + 1 / d4));
	m[1] = m[0] * d1 / d2;
	m[2] = m[0] * d1 / d3;
	m[3] = m[0] * d1 / d4;

	for (uint16_t i {0}; i < 4; i++)
	{
		ppt->m[i] = (uint32_t)(m[i] * (1 << AaWeightBits) + 0.5);
	}
}

static inline uint8_t get_color_channel(Color c, int channel)
{
	switch (channel)
	{
		case 0:
			return c.r;
		case 1:
			return c.g;
		case 2:
			return c.b;
		default:
			return 0;
	}
}

// Creates either a red, green, or blue value by
// computing the weighted averages of the 4 points in p
static uint8_t get_avg_channel(Color p[4], uint32_t mult[4], int channel)
{
	uint32_t ci = 0;

	//sum(mult[])==65536
	//c is the red/green/blue value of this pixel
	for (uint16_t j {0}; j < 4; j++)
	{
		const uint8_t c = get_color_channel(p[j], channel);
		ci += c * mult[j];
	}
	ci >>= AaWeightBits;
	//check for overflow
	if (ci > 255)
	{
		ci = 255;
	}

	return ((uint8_t)ci);
}

// CreateSphereTiltMap creates 'map_rotate' to map the topo data
//  for a tilted planet.  It also does the sphere->plane mapping
static void CreateSphereTiltMap(int angle, uint16_t height, uint16_t radius)
{
	int x {};
	int y {};
	const double multx = ((double)height / M_PI);
	const double multy = ((double)height / M_PI);
	const double xadj = ((double)height / 2.0);

	for (y = -radius; y <= radius; y++)
	{
		const int y_2 = y * y;

		for (x = -radius; x <= radius; x++)
		{
			double dx {NAN};
			double dy {NAN};
			double newx {NAN};
			double newy {NAN};
			double da {NAN};
			double rad {NAN};
			double rad_2 {NAN};
			double xa {NAN};
			double ya {NAN};
			MapPoint3D* ppt =
				&pSolarSysState->Orbit.map_rotate[y + radius]
												 [x + radius];

			rad_2 = x * x + y_2;

			if (rad_2 >= RadiusThres(radius))
			{ // pixel won't be present
				ppt->p[0].x = x + radius;
				ppt->p[0].y = y + radius;
				ppt->m[0] = 0;

				continue;
			}

			rad = sqrt(rad_2);
			// antialiasing goes beyond the actual radius
			if (rad >= radius)
			{
				rad = (double)radius - 0.1;
			}

			da = atan2((double)y, (double)x);
			// compute the planet-tilt
			da += M_DEG2RAD * -angle;
			dx = rad * cos(da);
			dy = rad * sin(da);

			// Map the sphere onto a plane
			xa = acos(-dx / radius);
			ya = acos(-dy / radius);
			newx = multx * xa;
			newy = multy * ya;
			// Adjust for vertical curvature
			if (ya <= 0.05 || ya >= 3.1 /* almost PI */)
			{
				newx = xadj; // exact centerline
			}
			else
			{
				newx = xadj + ((newx - xadj) / sin(ya));
			}

			create_aa_points(ppt, newx, newy, height);
		}
	}
}

//CreateShieldMask
// The shield is created in two parts.  This routine creates the Halo.
// The red tint of the planet is currently applied in RenderPlanetSphere
// This was done because the shield glows and needs to modify how the
// planet gets lit. Currently, the planet area is transparent in the mask
// made by this routine, but a filter can be applied if desired too.

// HALO rim size
static auto ShieldHalo() { return ResScale(playerInPlanetOrbit() ? 7 : 14); }
static auto ShieldRadius() { return (Radius() + ShieldHalo()); }


static constexpr auto ShieldHaloGlow {ShieldGlowComp + ShieldReflectComp};
static constexpr auto ShieldHaloGlowMin {ShieldHaloGlow >> 2};

static FrameRef CreateShieldMask(uint16_t radius)
{
	Color clear;
	int x {};
	int y {};
	FrameRef ShieldFrame {};
	const PlanetOrbit* Orbit = &pSolarSysState->Orbit;
	const uint16_t shieldradius = ShieldRadius() * radius / Radius();
	const uint16_t shielddiam = (shieldradius << 1) + 1;

	ShieldFrame = CaptureDrawable(
		CreateDrawable(WantPixmap | WantAlpha,
					   shielddiam, shielddiam, 1));

	Color* pix {Orbit->ScratchArray};
	//  This is 100% transparent.
	clear = BuildColorRgba(0, 0, 0, 0);

	for (y = -shieldradius; y <= shieldradius; y++)
	{
		for (x = -shieldradius; x <= shieldradius; ++x, ++pix)
		{
			const int rad_2 = x * x + y * y;
			// This is a non-transparent red for the halo
			int red = ShieldHaloGlow;
			int alpha = 255;
			double rad {NAN};

			if (rad_2 >= RadiusThres(shieldradius))
			{ // outside all bounds
				*pix = clear;
				continue;
			}
			// Inside the halo
			if (rad_2 <= Radius2(radius))
			{ // planet's pixels, ours transparent
				*pix = clear;
				continue;
			}

			// The halo itself
			rad = sqrt(rad_2);

			if (rad <= radius + 0.8)
			{ // pixels common between the shield and planet
				// do antialiasing using alpha
				alpha = (int)(red * (rad - radius));
				red = 255;
			}
			else
			{ // shield pixels
				red -= (int)((red - ShieldHaloGlowMin) * (rad - radius)
							 / (ShieldHalo() * radius / Radius()));
				if (red < 0)
				{
					red = 0;
				}
			}

			if (uqm::UQMOptions::read().nebulae)
			{
				if (alpha != 255)
				{
					*pix = BuildColorRgba(red, 0, 0, alpha);
				}
				else
				{
					*pix = BuildColorRgba(255, 0, 0, red);
				}
			}
			else
			{
				*pix = BuildColorRgba(red, 0, 0, alpha);
			}
		}
	}

	WriteFramePixelColors(ShieldFrame, Orbit->ScratchArray,
						  shielddiam, shielddiam);
	SetFrameHot(ShieldFrame, MAKE_HOT_SPOT(shieldradius + 1,
										   shieldradius + 1));

	return ShieldFrame;
}

FrameRef SaveBackFrame(uint16_t radius)
{
	GfxRect r;
	const uint16_t shieldradius = ShieldRadius() * radius / Radius();
	const uint16_t shielddiam = (shieldradius << 1) + 1;

	r.corner = MAKE_POINT((ResScale(OrigSisScreenWidth() >> 1))
							  - (shieldradius + 1),
						  PlanetOrgY() - (shieldradius + 1));
	r.extent.height = r.extent.width = shielddiam;

	FrameRef BackFrame {CaptureDrawable(CopyFrameRect(GetStarBackFround(), &r))};

	SetFrameHot(BackFrame, MAKE_HOT_SPOT(shieldradius + 1,
										 shieldradius + 1));

	return BackFrame;
}

// SetShieldThrobEffect adjusts the red levels in the shield glow graphic
//  the throbbing cycle is tied to the planet rotation cycle
static constexpr auto ShieldThrobs {12};
// throb cycles per revolution
static auto ThrobCycle() { return ((ResScale(OriginalMapWidth()) << 8) / ShieldThrobs); }
static auto ThrobHalfCycle() { return (ThrobCycle() >> 1); }

static constexpr auto ThrobMaxLevel {256};
static constexpr auto ThrobMinLevel {100};
static constexpr auto ThrobDLevel {ThrobMaxLevel - ThrobMinLevel};

static inline int shield_level(int offset)
{
	offset = (offset << 8) % ThrobCycle();
	int level {abs(offset - ThrobHalfCycle())};
	level = ThrobMinLevel + level * ThrobDLevel / ThrobHalfCycle();

	return level;
}

// See description above
// offset is effectively the angle of rotation around the planet's axis
void SetShieldThrobEffect(FrameRef ShieldFrame, int offset, FrameRef ThrobFrame)
{
	int i {};
	const PlanetOrbit* Orbit = &pSolarSysState->Orbit;
	Color* pix {};

	const int level {shield_level(offset)};

	const int width {GetFrameWidth(ShieldFrame)};
	const int height {GetFrameHeight(ShieldFrame)};
	ReadFramePixelColors(ShieldFrame, Orbit->ScratchArray, width, height);

	for (i = 0, pix = Orbit->ScratchArray; i < width * height; ++i, ++pix)
	{
		Color p = *pix;

		if (uqm::UQMOptions::read().nebulae)
		{
			p.a = p.a * level / ThrobMaxLevel;
		}
		else
		{
			if (p.a == 255)
			{ // adjust color data for full-alpha pixels
				p.r = p.r * level / ThrobMaxLevel;
				p.g = p.g * level / ThrobMaxLevel;
				p.b = p.b * level / ThrobMaxLevel;
			}
			else if (p.a > 0)
			{ // adjust alpha for translucent pixels
				p.a = p.a * level / ThrobMaxLevel;
			}
		}

		*pix = p;
	}

	WriteFramePixelColors(ThrobFrame, Orbit->ScratchArray, width, height);
	SetFrameHot(ThrobFrame, GetFrameHot(ShieldFrame));
}

void Draw3DOShield(GfxStamp ShieldFrame)
{
	static uint8_t i;
	static uint8_t cr;
	static bool flip;
	DrawMode oldmode;
	const uint8_t factor[6] = {0xFF, 0xE6, 0xC5, 0xA4, 0x83, 0x62};

	if (factor[i] == 0x62)
	{
		flip = true;
	}
	else if (factor[i] == 0xFF)
	{
		flip = false;
	}

	oldmode = SetContextDrawMode(MAKE_DRAW_MODE(DrawKind::Additive,
												factor[i]));

	DrawStamp(&ShieldFrame);

	SetContextDrawMode(oldmode);

	if (cr % chooseIfHd(3, 4) == 0)
	{
		i += flip ? -1 : 1;
	}

	cr++;
}

// Apply the shield to the topo image
static void ApplyShieldTint(void)
{
	DrawMode mode;
	DrawMode oldMode;
	Color tint;
	GfxRect r;

	// TopoFrame will be permanently changed
	FrameRef oldFrame {SetContextFGFrame(pSolarSysState->TopoFrame)};
	SetContextClipRect(nullptr);
	GetContextClipRect(&r);

	tint = BuildColorRgba(0xff, 0x00, 0x00, 0xff);
#ifdef USE_ALPHA_SHIELD
	mode = MAKE_DRAW_MODE(DrawKind::Alpha, 150);
#else
	mode = MAKE_DRAW_MODE(DrawKind::Additive, DrawFactor1);
#endif
	oldMode = SetContextDrawMode(mode);
	SetContextForeGroundColor(tint);
	DrawFilledRectangle(&r);
	SetContextDrawMode(oldMode);
	SetContextFGFrame(oldFrame);
}

static inline uint8_t calc_map_light(uint8_t val, uint32_t dif, int lvf)
{
	int i {};

	// apply diffusion
	i = (dif * val) >> DiffuseBits;
	// apply light variance for 3d lighting effect
	i += (lvf * val) >> 7;

	if (i < 0)
	{
		i = 0;
	}
	else if (i > 255)
	{
		i = 255;
	}

	return ((uint8_t)i);
}

static inline Color get_map_pixel(Color* pixels, int x, int y, uint16_t width, uint16_t spherespanx)
{
	/* if (y * (width + spherespanx) + x > 463000)
		log_add (log_Warning,"x:{}, y:{}, width:{}, spherespanx:{}, "
				"slot:{}. Max:{}", x, y, width, spherespanx,
				y * (width + spherespanx) + x,
				(MapHeight() + 1) * (MapWidth() + spherespanx)); */
	return pixels[y * (width + spherespanx) + x];
}

static inline Color apply_alpha_pixel(Color pix, int scan)
{
	Color c;
	c.r = (tintColors[scan].r * 0x45 / 0xFF)
		+ (pix.r * 0xFF * (0xFF - 0x45) / (255 * 255));
	c.g = (tintColors[scan].g * 0x45 / 0xFF)
		+ (pix.g * 0xFF * (0xFF - 0x45) / (255 * 255));
	c.b = (tintColors[scan].b * 0x45 / 0xFF)
		+ (pix.b * 0xFF * (0xFF - 0x45) / (255 * 255));

	return c;
}

static inline Color apply_additive_pixel(Color pix, int scan)
{
	Color c;
	c.r = clip_channel(pix.r + ((tintColors[scan].r * 0x45) >> 8));
	c.g = clip_channel(pix.g + ((tintColors[scan].g * 0x45) >> 8));
	c.b = clip_channel(pix.b + ((tintColors[scan].b * 0x45) >> 8));

	return c;
}

static inline int get_map_elev(int8_t* elevs, int x, int y, int offset, uint16_t width)
{
	return elevs[y * width + (offset + x) % width];
}

// RenderPlanetSphere builds a frame for the rotating planet view
// offset is effectively the angle of rotation around the planet's axis
// We use the SDL routines to directly write to the SDL_Surface to improve
// performance
void RenderPlanetSphere(PlanetOrbit* Orbit, FrameRef MaskFrame, int offset,
						bool shielded, bool doThrob, uint16_t width, uint16_t height,
						uint16_t radius)
{
	GfxPoint pt;
	Color clear;
	int x {};
	int y {};
	Color* pixels {};
	const uint16_t spherespanx = height;
	const uint16_t tworadius = radius << 1;
	const uint16_t diameter = tworadius + 1;

#if PROFILE_ROTATION
	static clock_t t = 0;
	static int frames_done = 1;
	clock_t t1;
	t1 = clock();
#endif

	const int shLevel {shield_level(offset)};

	Color* pix {Orbit->ScratchArray};
	clear = BuildColorRgba(0, 0, 0, 0);

	if ((Orbit->scanType < ScanType::Count) && Orbit->ScanColors)
	{
		pixels = Orbit->ScanColors[*Orbit->scanType] + offset;
	}
	else
	{
		pixels = Orbit->TopoColors + offset;
	}

	int8_t* elevs {Orbit->lpTopoData};

	for (pt.y = 0, y = -radius; pt.y <= tworadius; ++pt.y, ++y)
	{
		for (pt.x = 0, x = -radius; pt.x <= tworadius; ++pt.x, ++x, ++pix)
		{
			Color c;
			const uint32_t diffus = Orbit->light_diff[pt.y][pt.x];
			int i {};
			MapPoint3D* ppt = &Orbit->map_rotate[pt.y][pt.x];
			int lvf {}; // light variance factor

			if (diffus == 0)
			{ // full diffusion
				*pix = clear;
				continue;
			}

			// get pixel from topo map and factor from light variance map
			if (ppt->m[0] == 0)
			{ // exact pixel from the topo map
				c = get_map_pixel(pixels, ppt->p[0].x, ppt->p[0].y, width,
								  spherespanx);
				lvf = get_map_elev(elevs, ppt->p[0].x, ppt->p[0].y,
								   offset, width);
			}
			else
			{ // fractional pixel -- blend from 4
				Color p[4];
				int lvsum {};

				// compute 'ideal' pixel
				for (i = 0; i < 4; i++)
				{
					p[i] = get_map_pixel(pixels, ppt->p[i].x, ppt->p[i].y,
										 width, spherespanx);
				}

				c.r = get_avg_channel(p, ppt->m, 0);
				c.g = get_avg_channel(p, ppt->m, 1);
				c.b = get_avg_channel(p, ppt->m, 2);

				// compute 'ideal' light variance
				for (i = 0, lvsum = 0; i < 4; i++)
				{
					lvsum += get_map_elev(elevs, ppt->p[0].x, ppt->p[0].y,
										  offset, width)
						   * ppt->m[i];
				}
				lvf = lvsum >> AaWeightBits;
			}

			// Apply the lighting model.  This also bounds the sphere
			// to make it circular.
			if (shielded)
			{
				int r {};

				// add lite red filter (3/4) component
				c.g = (c.g >> 1) + (c.g >> 2);
				c.b = (c.b >> 1) + (c.b >> 2);

				c.r = calc_map_light(c.r, diffus, lvf);
				c.g = calc_map_light(c.g, diffus, lvf);
				c.b = calc_map_light(c.b, diffus, lvf);

				// The shield is glow + reflect (+ filter for others)
				r = calc_map_light(ShieldReflectComp, diffus, 0);
				r += ShieldGlowComp;

				if (doThrob)
				{ // adjust red level for throbbing shield
					r = r * shLevel / ThrobMaxLevel;
				}

				r += c.r;
				if (r > 255)
				{
					r = 255;
				}
				c.r = r;
			}
			else
			{
				c.r = calc_map_light(c.r, diffus, lvf);
				c.g = calc_map_light(c.g, diffus, lvf);
				c.b = calc_map_light(c.b, diffus, lvf);
			}

			c.a = 0xff;

			if (uqm::UQMOptions::read().scanStyle == uqm::EmulationMode::Console3DO && uqm::UQMOptions::read().tintPlanSphere == uqm::EmulationMode::PC
				&& Orbit->scanType < ScanType::Count)
			{
				*pix = apply_alpha_pixel(c, *Orbit->scanType);
			}
			else
			{
				*pix = c;
			}
		}
	}

	WriteFramePixelColors(MaskFrame, Orbit->ScratchArray, diameter,
						  diameter);
	SetFrameHot(MaskFrame, MAKE_HOT_SPOT(radius + 1, radius + 1));

#if PROFILE_ROTATION
	t += clock() - t1;
	if (frames_done == width)
	{
		uqm::log::debug("Rotation frames/sec: {}/{}(msec)={}",
						frames_done,
						(long int)(((double)t / CLOCKS_PER_SEC) * 1000.0 + 0.5),
						frames_done / ((double)t / CLOCKS_PER_SEC + 0.5));
		frames_done = 1;
		t = clock() - t1;
	}
	else
	{
		frames_done++;
	}
#endif
}

void RenderDOSPlanetSphere(PlanetOrbit* Orbit, FrameRef MaskFrame, int offset)
{
	if (!Orbit->TopoMask)
	{
		return;
	}
	else
	{ // Prepare new frame (oh god...)
		uint16_t x {};
		uint16_t y {};
		const int16_t width = MaskFrame->Bounds.width;
		const int16_t height = Orbit->TopoMask->Bounds.height;
		GfxRect r;
		const PlanetData* PlanetInfo = &pSolarSysState->SysInfo.PlanetInfo;

		r.corner.y = 0;
		r.corner.x = offset;
		r.extent.width = width;
		r.extent.height = height;

		// Get rect with offset
		FrameRef dupeframe {CaptureDrawable(CopyFrameRect(Orbit->TopoMask, &r))};

		if (PlanetInfo->AxialTilt != 0)
		{ // We need to tilt the frame
			GfxStamp s;
			FrameRef baseframe {};
			FrameRef rotFrame {};
			DrawMode oldMode;
			const int16_t trueheight = MaskFrame->Bounds.height;

			rotFrame = CaptureDrawable(
				RotateFrame(dupeframe, PlanetInfo->AxialTilt));
			GetFrameRect(rotFrame, &r);
			baseframe = CaptureDrawable(
				CreateDrawable(WantPixmap, (int16_t)width, trueheight, 1));

			// Draw everything in offscreen context
			GfxContextRef oldContext {SetContext(OffScreenContext)};
			SetContextFGFrame(baseframe);
			SetContextClipRect(nullptr);

			oldMode = SetContextDrawMode(DrawReplaceMode);

			SetFrameHot(rotFrame, MAKE_HOT_SPOT(0, 0));

			s.origin.x = (width - r.extent.width) / 2 - 1;
			s.origin.y = (trueheight - r.extent.height) / 2;
			s.frame = rotFrame;
			DrawStamp(&s);

			SetContextDrawMode(oldMode);
			SetContext(oldContext);

			// We got our colors - dump everything to avoid memory leak
			ReadFramePixelColors(baseframe, Orbit->ScratchArray, width,
								 trueheight);
			DestroyDrawable(ReleaseDrawable(baseframe));
			DestroyDrawable(ReleaseDrawable(rotFrame));
		}
		else
		{
			ReadFramePixelColors(dupeframe, Orbit->ScratchArray, width,
								 height);
		}

		uint8_t* pix {Orbit->sphereBytes};
		const Color* color {Orbit->ScratchArray};

		// Set indexes for sphere frame pixel by pixel
		for (y = 0; y < MaskFrame->Bounds.height; ++y)
		{
			for (x = 0; x < MaskFrame->Bounds.width; ++x, ++color, ++pix)
			{
				if (*pix < 0xFF) // If not transparent
				{				 // Normalize index to first 32-bit range, then add
					// offset from mask
					*pix = *pix - ((*pix / 32) * 32) + (color->r * 32);
				}
			}
		}
		WriteFramePixelIndexes(MaskFrame, Orbit->sphereBytes,
							   MaskFrame->Bounds.width, MaskFrame->Bounds.height);

		DestroyDrawable(ReleaseDrawable(dupeframe));
		dupeframe = 0;
	}
}

void Render3DOPlanetSphere(PlanetOrbit* Orbit, FrameRef MaskFrame, int offset,
						   uint16_t rotwidth, uint16_t height)
{
	int x {};
	int y {};
	Color clear;
	GfxPoint pt;
	const uint16_t spherespanx = height;
	const uint16_t radius = (spherespanx >> 1) - IfHd(2);
	const uint16_t tworadius = radius << 1;
	const uint16_t diameter = tworadius + 1;

	Color* c {Orbit->ScratchArray};
	Color* shade {Orbit->ShadeColors};
	Color* pixels {Orbit->TopoColors + offset};
	clear = BuildColorRgba(0, 0, 0, 0);

	for (pt.y = 0, y = -radius; pt.y <= tworadius; ++pt.y, ++y)
	{
		for (pt.x = 0, x = -radius; pt.x <= tworadius; ++pt.x, ++x, ++c,
			++shade)
		{
			const MapPoint3D* ppt = &Orbit->map_rotate[pt.y][pt.x];

			if (ppt->m[0] == 0 || shade->r == 0xFF)
			{ // exact pixel from the topo map
				*c = clear;
				continue;
			}
			else
			{
				*c = get_map_pixel(pixels, ppt->p[0].x, ppt->p[0].y,
								   rotwidth, spherespanx);

				c->r = clip_channel(c->r - shade->r);
				c->g = clip_channel(c->g - shade->g);
				c->b = clip_channel(c->b - shade->b);

				if (uqm::UQMOptions::read().tintPlanSphere == uqm::EmulationMode::PC
					&& Orbit->scanType < ScanType::Count)
				{
					if (uqm::UQMOptions::read().scanStyle == uqm::EmulationMode::Console3DO)
					{
						*c = apply_additive_pixel(
							*c, *Orbit->scanType);
					}
					else if (uqm::UQMOptions::read().scanStyle == uqm::EmulationMode::PC)
					{
						TransformColor(c, *Orbit->scanType);
					}
				}
			}
		}
	}

	WriteFramePixelColors(MaskFrame, Orbit->ScratchArray, diameter,
						  diameter);
	SetFrameHot(MaskFrame, MAKE_HOT_SPOT(radius + 1, radius + 1));
}

static constexpr auto RangeShift {6};

static constexpr auto DitherVariance {1 << (RangeShift - 3)};

static void DitherMap(int8_t* DepthArray, uint16_t width, uint16_t height)
{
	uint32_t i {};
	int8_t* elev {};
	uint32_t rand_val = 0;

	for (i = 0, elev = DepthArray; i < (uint32_t)(width * height); ++i,
		++elev)
	{
		// Use up the random value byte by byte
		if ((i & 3) == 0)
		{
			rand_val = SysGenRNG->next();
		}
		else
		{
			rand_val >>= 8;
		}

		// Bring the elevation point up or down
		*elev += DitherVariance / 2 - (rand_val & (DitherVariance - 1));
	}
}

static void MakeCrater(GfxRect* pRect, int8_t* DepthArray, int16_t rim_delta, int16_t crater_delta, bool SetDepth, uint16_t width)
{
	int16_t lf_x {};
	int16_t rt_x {};
	int16_t A {};
	int16_t B {};
	int32_t Asquared {};
	int32_t Bsquared {};
	uint32_t TopIndex {};
	uint32_t BotIndex {};

	A = pRect->extent.width >> 1;
	B = pRect->extent.height >> 1;

	int16_t x {0};
	int16_t y {B};

	Asquared = (uint32_t)A * A;
	const int32_t TwoAsquared {Asquared << 1};
	Bsquared = (uint32_t)B * B;
	const int32_t TwoBsquared {Bsquared << 1};

	int32_t dx {0};
	int32_t dy {TwoAsquared * B};
	int32_t d {Bsquared - (dy >> 1) + (Asquared >> 2)};

	A += pRect->corner.x;
	B += pRect->corner.y;
	TopIndex = (B - y) * width;
	BotIndex = (B + y) * width;
	uint32_t rim_pixels {1};
	while (dx < dy)
	{
		if (d > 0)
		{
			lf_x = A - x;
			rt_x = A + x;
			if (SetDepth)
			{
				memset(&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
				memset(&DepthArray[BotIndex + lf_x], 0, rt_x - lf_x + 1);
			}
			if (lf_x == rt_x)
			{
				DepthArray[TopIndex + lf_x] += rim_delta;
				DepthArray[BotIndex + lf_x] += rim_delta;
				rim_pixels = 0;
			}
			else
			{
				do
				{
					DepthArray[TopIndex + lf_x] += rim_delta;
					DepthArray[BotIndex + lf_x] += rim_delta;
					if (lf_x != rt_x)
					{
						DepthArray[TopIndex + rt_x] += rim_delta;
						DepthArray[BotIndex + rt_x] += rim_delta;
					}
					++lf_x;
					--rt_x;
				} while (--rim_pixels);

				while (lf_x < rt_x)
				{
					DepthArray[TopIndex + lf_x] += crater_delta;
					DepthArray[BotIndex + lf_x] += crater_delta;
					DepthArray[TopIndex + rt_x] += crater_delta;
					DepthArray[BotIndex + rt_x] += crater_delta;
					++lf_x;
					--rt_x;
				}

				if (lf_x == rt_x)
				{
					DepthArray[TopIndex + lf_x] += crater_delta;
					DepthArray[BotIndex + lf_x] += crater_delta;
				}
			}

			--y;
			TopIndex += width;
			BotIndex -= width;
			dy -= TwoAsquared;
			d -= dy;
		}

		++rim_pixels;
		++x;
		dx += TwoBsquared;
		d += Bsquared + dx;
	}

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

	while (y > 0)
	{
		lf_x = A - x;
		rt_x = A + x;
		if (SetDepth)
		{
			memset(&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
			memset(&DepthArray[BotIndex + lf_x], 0, rt_x - lf_x + 1);
		}
		if (lf_x == rt_x)
		{
			DepthArray[TopIndex + lf_x] += rim_delta;
			DepthArray[BotIndex + lf_x] += rim_delta;
		}
		else
		{
			do
			{
				DepthArray[TopIndex + lf_x] += rim_delta;
				DepthArray[BotIndex + lf_x] += rim_delta;
				if (lf_x != rt_x)
				{
					DepthArray[TopIndex + rt_x] += rim_delta;
					DepthArray[BotIndex + rt_x] += rim_delta;
				}
				++lf_x;
				--rt_x;
			} while (--rim_pixels);

			while (lf_x < rt_x)
			{
				DepthArray[TopIndex + lf_x] += crater_delta;
				DepthArray[BotIndex + lf_x] += crater_delta;
				DepthArray[TopIndex + rt_x] += crater_delta;
				DepthArray[BotIndex + rt_x] += crater_delta;
				++lf_x;
				--rt_x;
			}

			if (lf_x == rt_x)
			{
				DepthArray[TopIndex + lf_x] += crater_delta;
				DepthArray[BotIndex + lf_x] += crater_delta;
			}
		}

		if (d < 0)
		{
			++x;
			dx += TwoBsquared;
			d += dx;
		}

		rim_pixels = 1;
		--y;
		TopIndex += width;
		BotIndex -= width;
		dy -= TwoAsquared;
		d += Asquared - dy;
	}

	lf_x = A - x;
	rt_x = A + x;
	if (SetDepth)
	{
		memset(&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
	}
	if (lf_x == rt_x)
	{
		DepthArray[TopIndex + lf_x] += rim_delta;
	}
	else
	{
		do
		{
			DepthArray[TopIndex + lf_x] += rim_delta;
			if (lf_x != rt_x)
			{
				DepthArray[TopIndex + rt_x] += rim_delta;
			}
			++lf_x;
			--rt_x;
		} while (--rim_pixels);

		while (lf_x < rt_x)
		{
			DepthArray[TopIndex + lf_x] += crater_delta;
			DepthArray[TopIndex + rt_x] += crater_delta;
			++lf_x;
			--rt_x;
		}

		if (lf_x == rt_x)
		{
			DepthArray[TopIndex + lf_x] += crater_delta;
		}
	}
}

static constexpr auto NumBandColors {4};

static constexpr auto MaxStorms {8};

static void MakeStorms(uint16_t storm_count, int8_t* DepthArray, uint16_t width,
					   uint16_t height)
{
	uint16_t i {};
	GfxRect storm_r[MaxStorms];
	GfxRect* pstorm_r {};

	pstorm_r = &storm_r[i = storm_count];
	while (i--)
	{
		bool intersect {};
		uint32_t rand_val {};
		uint16_t loword {};
		uint16_t hiword {};
		int16_t band_delta {};

		--pstorm_r;
		do
		{
			uint16_t j {};

			intersect = false;

			rand_val = SysGenRNG->next();
			loword = uqm::lowWord(rand_val);
			hiword = uqm::highWord(rand_val);
			switch (uqm::highByte(hiword) & 31)
			{
				case 0:
					pstorm_r->extent.height =
						(uqm::lowByte(hiword) % (OriginalMapHeight() >> 2))
						+ (OriginalMapHeight() >> 2);
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					pstorm_r->extent.height =
						(uqm::lowByte(hiword) % (OriginalMapHeight() >> 3))
						+ (OriginalMapHeight() >> 3);
					break;
				default:
					pstorm_r->extent.height =
						(uqm::lowByte(hiword) % (OriginalMapHeight() >> 4))
						+ 4;
					break;
			}

			if (pstorm_r->extent.height <= 4)
			{
				pstorm_r->extent.height += 4;
			}

			rand_val = SysGenRNG->next();
			loword = uqm::lowWord(rand_val);
			hiword = uqm::highWord(rand_val);

			pstorm_r->extent.width = pstorm_r->extent.height
								   + (uqm::lowByte(loword) % pstorm_r->extent.height);

			pstorm_r->corner.x = uqm::highByte(loword)
							   % (OriginalMapWidth() - pstorm_r->extent.width);
			pstorm_r->corner.y = uqm::lowByte(loword)
							   % (OriginalMapHeight() - pstorm_r->extent.height);

			pstorm_r->corner.x = pstorm_r->corner.x * width
							   / OriginalMapWidth();
			pstorm_r->extent.width = pstorm_r->extent.width * width
								   / OriginalMapWidth();
			pstorm_r->corner.y = pstorm_r->corner.y * height
							   / OriginalMapHeight();
			pstorm_r->extent.height = pstorm_r->extent.height * height
									/ OriginalMapHeight();

			for (j = i + 1; j < storm_count; ++j)
			{
				int16_t x {};
				int16_t y {};
				int16_t w {};
				int16_t h {};

				x = storm_r[j].corner.x - pstorm_r->corner.x;
				y = storm_r[j].corner.y - pstorm_r->corner.y;
				w = x + storm_r[j].extent.width + 4;
				h = y + storm_r[j].extent.height + 4;
				intersect = (bool)(w > 0 && h > 0
								   && x < pstorm_r->extent.width + 4
								   && y < pstorm_r->extent.height + 4);
				if (intersect)
				{
					break;
				}
			}

		} while (intersect);

		MakeCrater(pstorm_r, DepthArray, 6, 6, false, width);
		++pstorm_r->corner.x;
		++pstorm_r->corner.y;
		pstorm_r->extent.width -= 2;
		pstorm_r->extent.height -= 2;

		band_delta = uqm::highByte(loword) & ((3 << RangeShift) + 20);

		MakeCrater(pstorm_r, DepthArray,
				   band_delta, band_delta, true, width);
		++pstorm_r->corner.x;
		++pstorm_r->corner.y;
		pstorm_r->extent.width -= 2;
		pstorm_r->extent.height -= 2;

		band_delta += 2;
		if (pstorm_r->extent.width > 2 && pstorm_r->extent.height > 2)
		{
			MakeCrater(pstorm_r, DepthArray,
					   band_delta, band_delta, true, width);
			++pstorm_r->corner.x;
			++pstorm_r->corner.y;
			pstorm_r->extent.width -= 2;
			pstorm_r->extent.height -= 2;
		}

		band_delta += 2;
		if (pstorm_r->extent.width > 2 && pstorm_r->extent.height > 2)
		{
			MakeCrater(pstorm_r, DepthArray,
					   band_delta, band_delta, true, width);
			++pstorm_r->corner.x;
			++pstorm_r->corner.y;
			pstorm_r->extent.width -= 2;
			pstorm_r->extent.height -= 2;
		}

		band_delta += 4;
		MakeCrater(pstorm_r, DepthArray,
				   band_delta, band_delta, true, width);
	}
}

static void MakeGasGiant(uint16_t num_bands, int8_t* DepthArray, GfxRect* pRect, int16_t depth_delta)
{
	int16_t last_y {};
	int16_t next_y {};
	int16_t band_error {};
	int16_t band_bump {};
	int16_t band_delta {};
	uint16_t j {};
	uint16_t band_height {};
	uint16_t loword {};
	uint16_t hiword {};
	uint32_t rand_val {};

	band_height = OriginalMapHeight() / num_bands;
	band_bump = OriginalMapHeight() % num_bands;
	band_error = num_bands >> 1;
	int8_t* lpDst {DepthArray};

	band_delta = ((uqm::lowWord(SysGenRNG->next())
				   & (NumBandColors - 1))
				  << RangeShift)
			   + (1 << (RangeShift - 1));
	last_y = next_y = 0;
	for (uint16_t i {num_bands}; i > 0; --i)
	{
		int16_t cur_y {};

		rand_val = SysGenRNG->next();
		loword = uqm::lowWord(rand_val);
		hiword = uqm::highWord(rand_val);

		next_y += band_height;
		if ((band_error -= band_bump) < 0)
		{
			++next_y;
			band_error += num_bands;
		}
		if (i == 1)
		{
			cur_y = pRect->extent.height;
		}
		else
		{
			GfxRect r;

			cur_y = next_y
				  + ((band_height - 2) >> 1)
				  - ((uqm::lowByte(hiword) % (band_height - 2)) + 1);
			cur_y = cur_y * pRect->extent.height / OriginalMapHeight();
			r.corner.x = r.corner.y = 0;
			r.extent.width = pRect->extent.width;
			r.extent.height = 5 * pRect->extent.height
							/ OriginalMapHeight();
			DeltaTopography(50,
							&DepthArray[(cur_y - (r.extent.height >> 1))
										* r.extent.width],
							&r, depth_delta);
		}

		for (j = cur_y - last_y; j > 0; --j)
		{
			uint16_t k {};

			for (k = pRect->extent.width; k > 0; --k)
			{
				*lpDst++ += band_delta;
			}
		}

		last_y = cur_y;
		band_delta = (band_delta
					  + ((((uqm::lowByte(loword) & 1) << 1) - 1) << RangeShift))
				   & (((1 << RangeShift) * NumBandColors) - 1);
	}

	MakeStorms(4 + (SysGenRNG->next() & 3) + 1,
			   DepthArray, pRect->extent.width, pRect->extent.height);

	DitherMap(DepthArray, pRect->extent.width, pRect->extent.height);
}

static void ValidateMap(int8_t* DepthArray, uint16_t width, uint16_t height)
{
	uint8_t state {};
	uint8_t pixel_count[2];
	uint8_t lb[2];
	uint32_t i {};

	i = width - 1;
	int8_t* lpDst {DepthArray};
	int8_t last_byte {*lpDst++};
	state = pixel_count[0] = pixel_count[1] = 0;
	do
	{
		if (pixel_count[state]++ == 0)
		{
			lb[state] = last_byte;
		}

		if (last_byte > *lpDst)
		{
			if (last_byte - *lpDst > 128)
			{
				state ^= 1;
			}
		}
		else
		{
			if (*lpDst - last_byte > 128)
			{
				state ^= 1;
			}
		}
		last_byte = *lpDst++;
	} while (--i);

	i = width * height;
	lpDst = DepthArray;
	if (pixel_count[0] > pixel_count[1])
	{
		last_byte = lb[0];
	}
	else
	{
		last_byte = lb[1];
	}
	do
	{
		if (last_byte > *lpDst)
		{
			if (last_byte - *lpDst > 128)
			{
				*lpDst = last_byte;
			}
		}
		else
		{
			if (*lpDst - last_byte > 128)
			{
				*lpDst = last_byte;
			}
		}
		last_byte = *lpDst++;
	} while (--i);
}

static void planet_orbit_init(uint16_t width, uint16_t height, bool forOrbit)
{
	PlanetOrbit* Orbit = &pSolarSysState->Orbit;
	const uint16_t spherespanx = height;
	const uint16_t shieldradius = (height >> 1) * ShieldRadius() / Radius();
	const uint16_t shielddiam = (shieldradius << 1) + 1;
	const uint16_t diameter = height + 1;
	uint16_t i {};

	{ // always needed
		Orbit->lpTopoData = new int8_t[width * height] {};

		Orbit->TopoZoomFrame = 0;
		Orbit->ObjectFrame = 0;

		// tints for 3DO scan
		if (forOrbit && uqm::UQMOptions::read().scanStyle != uqm::EmulationMode::PC)
		{
			Orbit->TintFrame = CaptureDrawable(CreateDrawable(
				WantPixmap, width, height, 1));
		}
		else
		{
			Orbit->TintFrame = 0;
		}

		Orbit->TintColor = BlackColor;

		Orbit->TopoColors = nullptr;
		Orbit->ScanColors = nullptr;

		Orbit->ScratchArray = new Color[shielddiam * shielddiam];

		Orbit->WorkFrame = 0;
		Orbit->BackFrame = 0;

		Orbit->light_diff = nullptr;
		Orbit->map_rotate = nullptr;

		Orbit->TopoMask = 0;
		Orbit->sphereBytes = nullptr;
		Orbit->sphereMap = 0;
		Orbit->scanType = ScanType::Count;

		Orbit->Shade = 0;
		Orbit->ShadeColors = nullptr;
	}

	if (!forOrbit || uqm::UQMOptions::read().sphereType)
	{ // UQM & 3DO sphere
		Orbit->SphereFrame = CaptureDrawable(CreateDrawable(
			WantPixmap | WantAlpha, diameter, diameter, 2));

		Orbit->TopoColors = new Color[height * (width + spherespanx)];

		if (forOrbit && isPC(uqm::UQMOptions::read().scanStyle) && isPC(uqm::UQMOptions::read().tintPlanSphere)
			&& !use3DOSpheres)
		{ // generate only on that conditions and then use if not nullptr
			Orbit->ScanColors = new Color*[*ScanType::Count];
			for (i = 0; i < ScanType::Count; i++)
			{
				Orbit->ScanColors[i] = new Color[height * (width + spherespanx)];
			}
		}

		if (!use3DOSpheres)
		{
			Orbit->light_diff = new uint32_t*[diameter];
		}

		Orbit->map_rotate = new MapPoint3D*[diameter];

		for (i = 0; i < diameter; i++)
		{
			if (!use3DOSpheres)
			{
				Orbit->light_diff[i] = new uint32_t[diameter];
			}
			Orbit->map_rotate[i] = new MapPoint3D[diameter];
		}

		if (use3DOSpheres)
		{
			Orbit->Shade =
				CaptureDrawable(LoadGraphic(PlanetMaskShadeRes));

			Orbit->ShadeColors = new Color[GetFrameWidth(Orbit->Shade) * GetFrameHeight(Orbit->Shade)];
		}
	}
	else
	{ // DOS sphere
		Orbit->SphereFrame =
			CaptureDrawable(LoadGraphic(DosPlanetMaskAnimRes));

		Orbit->TopoMask = CaptureDrawable(CreateDrawable(
			WantPixmap, (int16_t)width, (int16_t)height, 1));

		Orbit->sphereBytes = new uint8_t[GetFrameWidth(Orbit->SphereFrame) * GetFrameHeight(Orbit->SphereFrame)];

		Orbit->sphereMap =
			uqm::loadStringTable(DosSphereColorTabRes).release();
		SetColorMap(GetColorMapAddress(Orbit->sphereMap, 0));
	}
}

static unsigned frandom(void)
{
	static unsigned seed = 0x12345678;

	if (seed == 0)
	{
		seed = 15807;
	}
	seed = (seed >> 4) * 227;

	return seed;
}

static constexpr auto ScaleShift {8};

static inline int TopoVarianceFactor(int step, int allowed, int min)
{
	return ((abs(step) * allowed) >> ScaleShift) + min;
}

static inline int TopoVarianceCalc(int factor)
{
	if (factor == 0)
	{
		return 0;
	}
	else
	{
		return (frandom() % factor) - (factor >> 1);
	}
}

static constexpr auto AvgVariance {250};
static constexpr auto StepRange {4 - 1};

static void TopoScale4x(int8_t* pDstTopo, int8_t* pSrcTopo, int num_faults,
						int fault_var)
{
	// Interpolate the topographical data by connecting the elevations
	// to their nearest neighboors using straight lines (in random
	// direction) with random variance factors defined by
	// num_faults and fault_var args
	int x {};
	int y {};
	const int w = ScaledMapWidth();
	const int h = MapHeight();
	const int spitch = ScaledMapWidth();
	const int dpitch = ScaledMapWidth() * 4;
	int elev[5][5];
	const static struct line_def_t
	{
		int x0, y0, x1, y1;
		int dx, dy;
	} fill_lines[4][6] =
		{
			{
				// diag set 0
				{0, 2, 2, 0, 1, -1},
				{0, 3, 3, 0, 1, -1},
				{0, 4, 4, 0, 1, -1},
				{1, 4, 4, 1, 1, -1},
				{2, 4, 4, 2, 1, -1},
				{-1, -1, -1, -1, 0, 0}, // term
			},
			{
				// diag set 1
				{0, 2, 2, 4, 1, 1},
				{0, 1, 3, 4, 1, 1},
				{0, 0, 4, 4, 1, 1},
				{1, 0, 4, 3, 1, 1},
				{2, 0, 4, 2, 1, 1},
				{-1, -1, -1, -1, 0, 0}, // term
			},
			{
				// horizontal
				{0, 1, 4, 1, 1, 0},
				{0, 2, 4, 2, 1, 0},
				{0, 3, 4, 3, 1, 0},
				{-1, -1, -1, -1, 0, 0}, // term
			},
			{
				// vertical
				{1, 0, 1, 4, 0, 1},
				{2, 0, 2, 4, 0, 1},
				{3, 0, 3, 4, 0, 1},
				{-1, -1, -1, -1, 0, 0}, // term
			},
	};

	uqstl::vector<int> prevrowBuf(ScaledMapWidth() * 4 + 1);
	int* prevrow {prevrowBuf.data()};

	const int var_allow {(num_faults << ScaleShift) / AvgVariance};
	const int var_min {fault_var << ScaleShift};

	//memset (pDstTopo, 0, MapWidth() * MapHeight() * 16);

	// init the first row in advance
	const int8_t* pSrc {pSrcTopo};
	int* prow {prevrow};
	prow[0] = ((int)pSrc[0]) << ScaleShift;
	for (x = 0; x < w; ++x, ++pSrc, prow += 4)
	{
		int x2 {};

		// next point in row
		if (x < w - 1)
		{
			// one right
			prow[4] = ((int)pSrc[1]) << ScaleShift;
		}
		else
		{
			// wrap around
			prow[4] = ((int)pSrc[1 - spitch]) << ScaleShift;
		}

		// compute elevations between 2 points
		int val {prow[0]};
		const int step {(prow[4] - val) / StepRange};
		const int rndfact {TopoVarianceFactor(step, var_allow, var_min)};
		for (x2 = 1, val += step; x2 < 4; ++x2, val += step)
		{
			prow[x2] = val + TopoVarianceCalc(rndfact);
		}
	}

	pSrc = pSrcTopo;
	int8_t* pDst {pDstTopo};
	for (y = 0; y < h; ++y, pDst += dpitch * 3)
	{
		int x2 {};
		int y2 {};
		int8_t* p {};
		const struct line_def_t* pld {};

		prow = prevrow;
		// prime the first interpolated column
		elev[4][0] = prow[0];
		if (y < h - 1)
		{
			elev[4][4] = ((int)pSrc[spitch]) << ScaleShift;
		}
		else
		{
			elev[4][4] = elev[4][0];
		}
		// compute elevations for interpolated column
		int val {elev[4][0]};
		int step {(elev[4][4] - val) / StepRange};
		int rndfact {TopoVarianceFactor(step, var_allow, var_min)};
		for (y2 = 1, val += step; y2 < 4; ++y2, val += step)
		{
			elev[4][y2] = val + TopoVarianceCalc(rndfact);
		}

		for (x = 0; x < w; ++x, ++pSrc, pDst += 4, prow += 4)
		{
			// recall the first interpolated row from prevrow
			for (x2 = 0; x2 <= 4; ++x2)
			{
				elev[x2][0] = prow[x2];
			}
			// recall the first interpolated column
			for (y2 = 1; y2 <= 4; ++y2)
			{
				elev[0][y2] = elev[4][y2];
			}

			if (y < h - 1)
			{
				if (x < w - 1)
				{
					// one right, one down
					elev[4][4] = ((int)pSrc[1 + spitch]) << ScaleShift;
				}
				else
				{
					// wrap around, one down
					elev[4][4] = ((int)pSrc[1]) << ScaleShift;
				}
			}
			else
			{
				elev[4][4] = elev[4][0];
			}

			// compute elevations for the rest of square borders first
			val = elev[0][4];
			step = (elev[4][4] - val) / StepRange;
			rndfact = TopoVarianceFactor(step, var_allow, var_min);
			for (x2 = 1, val += step; x2 < 4; ++x2, val += step)
			{
				elev[x2][4] = val + TopoVarianceCalc(rndfact);
			}

			val = elev[4][0];
			step = (elev[4][4] - val) / StepRange;
			rndfact = TopoVarianceFactor(step, var_allow, var_min);
			for (y2 = 1, val += step; y2 < 4; ++y2, val += step)
			{
				elev[4][y2] = val + TopoVarianceCalc(rndfact);
			}

			// fill in the rest by connecting opposing elevations
			// some randomness to determine which elevations to connect
			for (pld = fill_lines[frandom() & 3]; pld->x0 >= 0; ++pld)
			{
				x2 = pld->x0;
				y2 = pld->y0;
				val = elev[x2][y2];
				int num_steps {pld->x1 - pld->x0};
				if (num_steps == 0)
				{
					num_steps = pld->y1 - pld->y0;
				}
				step = (elev[pld->x1][pld->y1] - val) / num_steps;
				rndfact = TopoVarianceFactor(step, var_allow, var_min);

				for (x2 += pld->dx, y2 += pld->dy, val += step;
					 x2 != pld->x1 || y2 != pld->y1;
					 x2 += pld->dx, y2 += pld->dy, val += step)
				{
					elev[x2][y2] = val + TopoVarianceCalc(rndfact);
				}
			}

			// output the interpolated topography
			for (y2 = 0; y2 < 4; ++y2)
			{
				p = pDst + y2 * dpitch;
				for (x2 = 0; x2 < 4; ++x2, ++p)
				{
					int e = elev[x2][y2] >> ScaleShift;
					if (e > 127)
					{
						e = 127;
					}
					else if (e < -128)
					{
						e = -128;
					}
					*p = (int8_t)e;
				}
			}

			// save last interpolated row to prevrow for later
			for (x2 = 0; x2 < 4; ++x2)
			{
				prow[x2] = elev[x2][4];
			}
		}
		// save last row point
		prow[0] = elev[4][4];
	}
}

// GenerateLightMap produces a surface light variance map for the
//  rotating planet by, first, transforming absolute elevation data
//  into normalized relative and then applying a weighted
//  average-median of surrounding points
// Lots of pure Voodoo here ;)
//  the goal is a 3D illusion, not mathematically correct lighting

static constexpr auto LmapAvgBlock {(Sc2MapHeight + 64) / 5};
static constexpr auto LmapMaxDist {(LmapAvgBlock + 1) >> 1};
static constexpr auto LmapWeightThres {LmapMaxDist * 2 / 3};

struct elev_block_t
{
	int min;
	int max;
	int avg;
};

static inline void get_vblock_avg(elev_block_t* pblk, int8_t* pTopo, int x, int y,
								  uint16_t width, uint16_t height)
{
	const int8_t* elev = pTopo;
	int i {};
	int min = 127;
	int max = -127;
	int avg = 0;
	int total_weight = 0;

	// surface wraps around along x
	x = (x + width) % width;

	int y0 {y - LmapMaxDist};
	int y1 {y + LmapMaxDist};
	if (y0 < 0)
	{
		y0 = 0;
	}
	if (y1 > height)
	{
		y1 = height;
	}

	elev = pTopo + y0 * height + x;
	for (i = y0; i < y1; ++i, elev += height)
	{
		const int delta = abs(i - y);
		int weight = 255; // full weight
		const int v = *elev;

		if (delta >= LmapWeightThres)
		{ // too far -- progressively reduced weight
			weight = weight * (LmapMaxDist - delta + 1)
				   / (LmapMaxDist - LmapWeightThres + 2);
		}

		if (v > max)
		{
			max = v;
		}
		if (v < min)
		{
			min = v;
		}
		avg += pblk->avg * weight;
		total_weight += weight;
	}
	avg /= total_weight;

	pblk->min = min;
	pblk->max = max;
	pblk->avg = avg / (y1 - y0);
}

static constexpr auto LmapBlocks {2 * LmapMaxDist + 1};

// See description above
static void GenerateLightMap(int8_t* pTopo, int w, int h)
{
	int x {};
	int y {};
	elev_block_t vblocks[LmapBlocks];
	// we use a running block average to reduce the amount of work
	// where a block is a vertical line of map points
	int8_t* elev {};
	int sfact {};

	// normalize the topo data
	int min {127};
	int max {-128};
	for (elev = pTopo; elev != pTopo + w * h; ++elev)
	{
		// Louis Delacroix: Bug#1151
		if (*elev > max)
		{
			max = *elev;
		}
		if (*elev < min)
		{
			min = *elev;
		}
	}
	int med {(min + max) / 2};
	const int spread {max - med};

	if (spread == 0)
	{ // perfectly smooth surface -- nothing to do but
		// level it out completely
		if (max != 0)
		{
			memset(pTopo, 0, w * h);
		}
		return;
	}

	// these are whatever looks right
	if (spread < 10)
	{
		sfact = 30; // minimal spread
	}
	else if (spread < 30)
	{
		sfact = 60;
	}
	else
	{
		sfact = 100; // full spread
	}

	// apply spread
	for (elev = pTopo; elev != pTopo + w * h; ++elev)
	{
		// Louis Delacroix: Bug#1151
		*elev = (*elev - med) * sfact / spread;
	}

	// compute and apply weighted averages of surrounding points
	for (y = 0, elev = pTopo; y < h; ++y)
	{
		elev_block_t* pblk {};
		int i {};

		// prime the running block average
		// get the minimum, maximum and avg elevation for each block
		for (i = -LmapMaxDist; i < LmapMaxDist; ++i)
		{
			// blocks wrap around on both sides
			pblk = vblocks + ((i + LmapBlocks) % LmapBlocks);

			get_vblock_avg(pblk, pTopo, i, y, w, h);
		}

		for (x = 0; x < w; ++x, ++elev)
		{
			int avg = 0;
			int total_weight = 0;

			min = 127;
			max = -127;

			// prepare next block as we move along x
			pblk = vblocks + ((x + LmapMaxDist) % LmapBlocks);
			get_vblock_avg(pblk, pTopo, x + LmapMaxDist, y, w, h);

			// compute the min, max and weighted avg of blocks
			for (i = x - LmapMaxDist; i <= x + LmapMaxDist; ++i)
			{
				const int delta = abs(i - x);
				int weight = 255; // full weight

				pblk = vblocks + ((i + LmapBlocks) % LmapBlocks);

				if (delta >= LmapWeightThres)
				{ // too far -- progressively reduced weight
					weight = weight * (LmapMaxDist - delta + 1)
						   / (LmapMaxDist - LmapWeightThres + 2);
				}

				if (pblk->max > max)
				{
					max = pblk->max;
				}
				if (pblk->min < min)
				{
					min = pblk->min;
				}

				avg += pblk->avg * weight;
				total_weight += weight;
			}
			avg /= total_weight;

			// This is mostly Voodoo
			// figure out what kind of relative lighting factor
			// to assign to this point
#if 0
			// relative to median
			med = (min + max) / 2; // median
			*elev = (int)*elev - med;
#else
			// relative to median of (average, median)
			med = (min + max) / 2; // median
			med = (med + avg) / 2;
			*elev = (int)*elev - med;
#endif
		}
	}
}

void load_color_resources(SystemPlanet* pPlanetDesc,
						  const PlanetFrame* PlanDataPtr, PlanetData* PlanetInfo,
						  bool dosshielded, bool LoadCustomColorTable)
{
	if (LoadCustomColorTable)
	{ // JMS: Planets with special colormaps
		pSolarSysState->OrbitalCMap = uqm::loadStringTable(GetAltColorMap(pPlanetDesc));
		pSolarSysState->OrbitalCMapIndex = 0;
		if (pSolarSysState->OrbitalCMap != nullptr)
		{ // Load successful - grab XLAT and return
			pSolarSysState->XlatTable = uqm::loadStringTable(SpecialCmapXlatTabRes);
			pSolarSysState->XlatPtr = pSolarSysState->XlatTable->bytes(0).data();
			return;
		} // Else just load standard color table
	}

	// JMS: Normal planets
	size_t xlatIndex {0};
	pSolarSysState->OrbitalCMap = uqm::loadStringTable(dosshielded ? DosShieldedColorTabRes : PlanDataPtr->CMapInstance);
	pSolarSysState->OrbitalCMapIndex = 0;
	pSolarSysState->XlatTable = uqm::loadStringTable(PlanDataPtr->XlatTabInstance);

	if (PlanetInfo->SurfaceTemperature > HotThreshold)
	{
		pSolarSysState->OrbitalCMapIndex = 2;
		xlatIndex = 2;
	}
	else if (PlanetInfo->SurfaceTemperature > ColdThreshold)
	{
		pSolarSysState->OrbitalCMapIndex = 1;
		xlatIndex = 1;
	}

	pSolarSysState->XlatPtr = pSolarSysState->XlatTable->bytes(xlatIndex).data();
}

void generate_surface_frame(uint16_t width, uint16_t height, PlanetOrbit* Orbit,
							const PlanetFrame* PlanDataPtr)
{ // Generate planet surface elevation data and look
	GfxRect r;
	uint16_t i {};

	r.corner.x = r.corner.y = 0;
	r.extent.width = width;
	r.extent.height = height;

	memset(Orbit->lpTopoData, 0, width * height);
	switch (Planalgo(PlanDataPtr->Type))
	{
		case GasGiantAlgo:
			MakeGasGiant(PlanDataPtr->num_faults,
						 Orbit->lpTopoData, &r, PlanDataPtr->fault_depth);
			break;
		case TopoAlgo:
		case CrateredAlgo:
			if (PlanDataPtr->num_faults)
			{
				DeltaTopography(PlanDataPtr->num_faults,
								Orbit->lpTopoData, &r,
								PlanDataPtr->fault_depth);
			}

			for (i = 0; i < PlanDataPtr->num_blemishes; ++i)
			{
				GfxRect crater_r;

				uint16_t loword {uqm::lowWord(SysGenRNG->next())};
				switch (uqm::highByte(loword) & 31)
				{
					case 0:
						crater_r.extent.width =
							(uqm::lowByte(loword)
							 % (OriginalMapHeight() >> 2))
							+ (OriginalMapHeight() >> 2);
						break;
					case 1:
					case 2:
					case 3:
					case 4:
						crater_r.extent.width =
							(uqm::lowByte(loword)
							 % (OriginalMapHeight() >> 3))
							+ (OriginalMapHeight() >> 3);
						break;
					default:
						crater_r.extent.width =
							(uqm::lowByte(loword)
							 % (OriginalMapHeight() >> 4))
							+ 4;
						break;
				}

				loword = uqm::lowWord(SysGenRNG->next());
				crater_r.extent.height = crater_r.extent.width;
				crater_r.corner.x = uqm::highByte(loword)
								  % (OriginalMapWidth()
									 - crater_r.extent.width);
				crater_r.corner.y = uqm::lowByte(loword)
								  % (OriginalMapHeight()
									 - crater_r.extent.height);

				// BW: ... then scale them up
				crater_r.extent.width = crater_r.extent.width
									  * height / OriginalMapHeight();
				crater_r.extent.height = crater_r.extent.width;
				crater_r.corner.x = crater_r.corner.x
								  * width / OriginalMapWidth();
				crater_r.corner.y = crater_r.corner.y
								  * height / OriginalMapHeight();

				MakeCrater(&crater_r, Orbit->lpTopoData,
						   PlanDataPtr->fault_depth << 2,
						   -(PlanDataPtr->fault_depth << 2),
						   false, width);
			}
			if (Planalgo(PlanDataPtr->Type) == CrateredAlgo)
			{
				DitherMap(Orbit->lpTopoData, width, height);
			}
			ValidateMap(Orbit->lpTopoData, width, height);
			break;
	}
	pSolarSysState->TopoFrame = CaptureDrawable(
		CreateDrawable(WantPixmap, (int16_t)width,
					   (int16_t)height, 1));

	RenderTopography(pSolarSysState->TopoFrame,
					 Orbit->lpTopoData, width, height, false, nullptr);
}

void GetPlanetTopography(SystemPlanet* pPlanetDesc, FrameRef SurfDefFrame)
{
	const PlanetData* PlanetInfo = &pSolarSysState->SysInfo.PlanetInfo;
	PlanetOrbit* Orbit = &pSolarSysState->Orbit;
	uint16_t width {};
	uint16_t height {};
	const bool shielded = (pPlanetDesc->data_index & *PlanetType::PlanetShielded);

	width = ScaledMapWidth();
	height = MapHeight();

	const PlanetFrame* PlanDataPtr {&PlanData[pPlanetDesc->data_index & ~*PlanetType::PlanetShielded]};

	if (SurfDefFrame)
	{
		if (GetFrameWidth(SurfDefFrame) != width
			|| GetFrameHeight(SurfDefFrame) != height)
		{
			pSolarSysState->TopoFrame = CaptureDrawable(RescaleFrame(
				SurfDefFrame, width, height));
		}
		else
		{
			pSolarSysState->TopoFrame = CaptureDrawable(CloneFrame(SurfDefFrame));
		}
	}
	else
	{
		size_t xlatIndex {0};
		pSolarSysState->OrbitalCMap = uqm::loadStringTable(shielded && useDosSpheres ? DosShieldedColorTabRes : PlanDataPtr->CMapInstance);
		pSolarSysState->OrbitalCMapIndex = 0;
		pSolarSysState->XlatTable = uqm::loadStringTable(PlanDataPtr->XlatTabInstance);

		if (PlanetInfo->SurfaceTemperature > HotThreshold)
		{
			pSolarSysState->OrbitalCMapIndex = 2;
			xlatIndex = 2;
		}
		else if (PlanetInfo->SurfaceTemperature > ColdThreshold)
		{
			pSolarSysState->OrbitalCMapIndex = 1;
			xlatIndex = 1;
		}

		pSolarSysState->XlatPtr = pSolarSysState->XlatTable->bytes(xlatIndex).data();

		Orbit->lpTopoData = new int8_t[width * height] {};
		SysGenRNG->reseed(pPlanetDesc->rand_seed);
		generate_surface_frame(width, height, Orbit, PlanDataPtr);

		pSolarSysState->OrbitalCMap.reset();
		pSolarSysState->XlatTable.reset();
		delete[] Orbit->lpTopoData;
		Orbit->lpTopoData = nullptr;
	}
}

// Sets the SysGenRNG to the required state first.
void GeneratePlanetSurface(SystemPlanet* pPlanetDesc, FrameRef SurfDefFrame,
						   uint16_t width, uint16_t height)
{
	PlanetData* PlanetInfo = &pSolarSysState->SysInfo.PlanetInfo;
	GfxPoint loc;
	GfxContextRef TopoContext {};
	PlanetOrbit* Orbit = &pSolarSysState->Orbit;
	bool SurfDef = false;
	const bool shielded = (pPlanetDesc->data_index & *PlanetType::PlanetShielded) != 0;
	uint16_t spherespanx {};
	uint16_t radius {};
	bool ForIP {};
	const bool customTexture =
		solTexturesPresent && CurStarDescPtr()->Index == PlotId::Sol;

	if (!width && !height)
	{
		width = ScaledMapWidth();
		height = MapHeight();
		spherespanx = SphereSpanX();
		radius = Radius();
		ForIP = false;

		useDosSpheres = uqm::UQMOptions::read().sphereType == 0;
		use3DOSpheres = uqm::UQMOptions::read().sphereType == 1;
	}
	else
	{
		spherespanx = height;
		radius = (height >> 1) - IfHd(2);
		ForIP = true;

		useDosSpheres = false;
		use3DOSpheres = false;
	}

	SysGenRNG->reseed(pPlanetDesc->rand_seed);

	TopoContext = CreateContext("Plangen.TopoContext");
	GfxContextRef OldContext {SetContext(TopoContext)};

	planet_orbit_init(width, height, !ForIP);

	const PlanetFrame* PlanDataPtr {&PlanData[pPlanetDesc->data_index & ~*PlanetType::PlanetShielded]};

	load_color_resources(
		pPlanetDesc, PlanDataPtr, PlanetInfo,
		shielded && useDosSpheres, (customTexture && !ForIP && !useDosSpheres && !shielded && PlanetInfo->AtmoDensity != GasGiantAtmosphere));

	if (SurfDefFrame)
	{ // This is a defined planet; pixmap for the topography and
		// elevation data is supplied in Surface Definition frame
		bool DeleteDef = false;
		bool DeleteElev = false;
		FrameRef ElevFrame = 0;
		uint16_t index = 0;

		// load special frame to render Earth with DOS spheres on
		if (GetFrameCount(SurfDefFrame) == 4 && useDosSpheres && !ForIP)
		{
			index = 2;
		}

		// surface pixmap
		SurfDef = true;
		SurfDefFrame = SetAbsFrameIndex(SurfDefFrame, index);
		if (GetFrameWidth(SurfDefFrame) != width
			|| GetFrameHeight(SurfDefFrame) != height)
		{
			pSolarSysState->TopoFrame = CaptureDrawable(RescaleFrame(
				SurfDefFrame, width, height));
			// will not need the passed FrameRef anymore
			DeleteDef = true;
		}
		else
		{
			pSolarSysState->TopoFrame = SurfDefFrame;
		}

		if (GetFrameCount(SurfDefFrame) > 1)
		{ // 2nd frame is elevation data
			int i {};
			int8_t* elev {};

			ElevFrame = SetAbsFrameIndex(SurfDefFrame, index + 1);
			if (GetFrameWidth(ElevFrame) != width
				|| GetFrameHeight(ElevFrame) != height)
			{ // Should ALWAYS be paletted
				ElevFrame = CaptureDrawable(
					RescaleFrame(ElevFrame, width, height));
				DeleteElev = true;
			}

			// grab the elevation data in 1 byte per pixel format
			ReadFramePixelIndexes(ElevFrame, (uint8_t*)Orbit->lpTopoData,
								  width, height, !ForIP);
			// the supplied data is in unsigned format, must convert
			for (i = 0, elev = Orbit->lpTopoData;
				 i < width * height;
				 ++i, ++elev)
			{
				*elev = *(uint8_t*)elev - 128;
			}
		}
		else
		{ // no elevation data -- planet flat as a pancake
			memset(Orbit->lpTopoData, 0, width * height);
		}

		if (DeleteDef)
		{
			DestroyDrawable(ReleaseDrawable(SurfDefFrame));
		}
		if (DeleteElev)
		{
			DestroyDrawable(ReleaseDrawable(ElevFrame));
		}
	}
	else
	{
		generate_surface_frame(width, height, Orbit, PlanDataPtr);
	}

	if (!ForIP && useDosSpheres)
	{
		RenderLevelMasks(Orbit->TopoMask, Orbit->lpTopoData, SurfDef);
		SetPlanetColors(GetColorMapAddress(Orbit->sphereMap, 0));
		if (Orbit->TopoMask != nullptr)
		{
			ExpandLevelMasks(Orbit);
		}
		else
		{
			uqm::log::warn("No planet mask generated.\n");
		}
	}

	if (!ForIP && isPC(uqm::UQMOptions::read().scanStyle) && !shielded)
	{
		uint16_t i {};

		if (SurfDef)
		{
			for (i = 0; i < ScanType::Count; i++)
			{
				uint16_t x {};

				pSolarSysState->ScanFrame[i] = CaptureDrawable(
					CreateDrawable(WantPixmap, (int16_t)width,
								   (int16_t)height, 1));

				uqstl::vector<Color> mapBuf(width * height);
				Color* map {mapBuf.data()};
				ReadFramePixelColors(
					pSolarSysState->TopoFrame, map, width, height);

				Color* pix {map};

				for (uint16_t y {0}; y < height; ++y)
				{
					for (x = 0; x < width; ++x, ++pix)
					{
						TransformColor(pix, i);
					}
				}

				WriteFramePixelColors(
					pSolarSysState->ScanFrame[i], map, width, height);
			}
		}
		else
		{
			const uqm::StringTablePtr scanTable {uqm::loadStringTable(ScanColorTabRes)};

			for (i = 0; i < ScanType::Count; i++)
			{
				pSolarSysState->ScanFrame[i] = CaptureDrawable(
					CreateDrawable(WantPixmap, (int16_t)width,
								   (int16_t)height, 1));

				RenderTopography(pSolarSysState->ScanFrame[i],
								 Orbit->lpTopoData, width, height, false, GetColorMapAddress(scanTable.get(), i));
			}
		}
	}

	if (!ForIP && !shielded
		&& PlanetInfo->AtmoDensity != GasGiantAtmosphere)
	{ // produce 4x scaled topo image for Planetside
		// for the planets that we can land on

		if (isPC(uqm::UQMOptions::read().landerStyle) && !IsHd() && !SurfDefFrame)
		{ // crispy PC-DOS landscape
			Orbit->TopoZoomFrame = CaptureDrawable(
				RescaleFrame(
					pSolarSysState->TopoFrame,
					width << 2, height << 2));
		}
		else
		{ // usual smooth 3DO landscape
			uqstl::vector<int8_t> scaledTopo(ScaledMapWidth() * 4 * MapHeight() * 4);

			Orbit->TopoZoomFrame = CaptureDrawable(CreateDrawable(
				WantPixmap, width << 2, height << 2, 1));

			TopoScale4x(scaledTopo.data(), Orbit->lpTopoData,
						PlanDataPtr->num_faults, PlanDataPtr->fault_depth * (Planalgo(PlanDataPtr->Type) == CrateredAlgo ? 2 : 1));
			RenderTopography(Orbit->TopoZoomFrame, scaledTopo.data(),
							 ScaledMapWidth() * 4, MapHeight() * 4, SurfDef, nullptr);
		}
	}

	if (Orbit->TopoColors)
	{ // Generate a pixel array from the Topography map.
		// We use this instead of lpTopoData because it needs to be
		// WAP_WIDTH+SphereSpanX() wide and we need this method for Earth
		// anyway. It may be more efficient to build it from lpTopoData
		// instead of the FRAMPTR though.
		uint32_t y {};

		ReadFramePixelColors(pSolarSysState->TopoFrame, Orbit->TopoColors,
							 width + spherespanx, height);
		if (SurfDef)
		{
			RepairColorOpacity(Orbit->TopoColors, width + spherespanx, height);
		}
		// Extend the width from MapWidth() to MapWidth()+SphereSpanX()
		for (y = 0; y < (uint32_t)(height * (width + spherespanx));
			 y += width + spherespanx)
		{
			memcpy(Orbit->TopoColors + y + width, Orbit->TopoColors + y,
				   spherespanx * sizeof(Orbit->TopoColors[0]));
		}
	}

	if (Orbit->ScanColors)
	{ // prepare colors for every scan tint if we ever created them
		uint32_t y {};

		for (uint16_t i {0}; i < ScanType::Count; i++)
		{
			ReadFramePixelColors(pSolarSysState->ScanFrame[i],
								 Orbit->ScanColors[i], width + spherespanx, height);

			for (y = 0; y < (uint32_t)(height * (width + spherespanx));
				 y += width + spherespanx)
			{
				memcpy(Orbit->ScanColors[i] + y + width,
					   Orbit->ScanColors[i] + y,
					   spherespanx * sizeof(Orbit->ScanColors[0][0]));
			}
		}
	}

	if (Planalgo(PlanDataPtr->Type) == GasGiantAlgo
		|| (customTexture && use3DOSpheres))
	{ // convert topo data to a light map, based on relative
		// map point elevations
		memset(Orbit->lpTopoData, 0, width * height);
	}
	else
	{
		GenerateLightMap(Orbit->lpTopoData, width, height);
	}

	if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->SunDesc[0])
	{ // this is a planet -- get its location
		loc = pSolarSysState->pOrbitalDesc->location;
	}
	else
	{ // this is a moon -- get its planet's location
		loc = pSolarSysState->pOrbitalDesc->pPrevDesc->location;
	}

	// Rotating planet sphere initialization
	if (!(useDosSpheres || use3DOSpheres) || ForIP)
	{
		GenerateSphereMask(loc, radius);
		CreateSphereTiltMap(PlanetInfo->AxialTilt, height, radius);
	}
	else if (useDosSpheres || use3DOSpheres)
	{
		uint16_t facing {};
		facing =
			NormalizeFacing(AngleToFacing(ARCTAN(loc.x, loc.y)));

		if (useDosSpheres)
		{
			Orbit->SphereFrame =
				SetAbsFrameIndex(Orbit->SphereFrame, facing & 14);
			ReadFramePixelIndexes(Orbit->SphereFrame, Orbit->sphereBytes,
								  GetFrameWidth(Orbit->SphereFrame),
								  GetFrameHeight(Orbit->SphereFrame), true);
		}
		else if (use3DOSpheres)
		{
			CreateSphereTiltMap(PlanetInfo->AxialTilt, height, radius);
			Orbit->Shade =
				SetAbsFrameIndex(Orbit->Shade, facing);
			ReadFramePixelColors(Orbit->Shade, Orbit->ShadeColors,
								 GetFrameWidth(Orbit->Shade),
								 GetFrameHeight(Orbit->Shade));
		}
	}
	if (shielded)
	{
		Orbit->ObjectFrame =
			((uqm::UQMOptions::read().sphereType < 2 && !ForIP) ?
				 (useDosSpheres ?
					  CaptureDrawable(LoadGraphic(DosShieldMaskAnimRes)) :
					  CaptureDrawable(LoadGraphic(TdoShieldMaskAnimRes))) :
				 CreateShieldMask(radius));

		// Create background frame if we have nebula on
		// but not for IP and if we're using DOS shield
		if ((uqm::UQMOptions::read().nebulae || use3DOSpheres) && !ForIP && !useDosSpheres)
		{
			Orbit->BackFrame = SaveBackFrame(radius);
		}
	}

	const int32_t PlanetRotation {(1 - 2 * (PlanetInfo->AxialTilt & 1))};

	InitSphereRotation(PlanetRotation, shielded, width, height);

	if (ForIP)
	{
		pPlanetDesc->rotwidth = width;
		pPlanetDesc->rotheight = height;
		pPlanetDesc->rotFrameIndex = 0;
		pPlanetDesc->rotPointIndex = 0;
		pPlanetDesc->rot_speed =
			((double)(pPlanetDesc->rotwidth * PlanetRotation * 240))
			/ PlanetInfo->RotationPeriod;
	}

	if (shielded && !useDosSpheres)
	{ // This overwrites pSolarSysState->TopoFrame, so everything that
		// needs it has to come before
		ApplyShieldTint();
	}

	SetContext(OldContext);
	DestroyContext(TopoContext);
}

} // namespace uqm