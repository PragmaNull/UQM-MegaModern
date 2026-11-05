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

#ifndef UQM_UNITS_H_
#define UQM_UNITS_H_

#include "libs/gfxlib.h"
#include "../options.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

extern int CanvasWidth;
extern int CanvasHeight;
extern unsigned int resolutionFactor;

/* Most basic resolution units. */
inline constexpr auto HdResolutionFactor {2};


inline auto IsHd() { return (resolutionFactor != HdResolutionFactor ? false : true); }
#define RES_SCALE(a) ((a) << resolutionFactor)
#define RES_DESCALE(a) ((a) >> resolutionFactor)
//#define chooseIfHd(a,b) (!IsHd() ? (a) : (b))
template <typename T, std::enable_if_t<std::is_fundamental_v<std::remove_reference_t<T>>, bool> = true>
static inline constexpr auto chooseIfHd(const T a, const T b) -> T
{
	return !IsHd() ? a : b;
}
template <typename T, std::enable_if_t<!std::is_fundamental_v<std::remove_reference_t<T>>, bool> = false>
static inline constexpr auto chooseIfHd(const T& a, const T& b) -> const T&
{
	return !IsHd() ? a : b;
}
#define NRES_BOOL(a) (!IsHd() ? (a) : 0)
#define RES_DBL(a) (chooseIfHd((a), (a) * resolutionFactor))
#define RES_TRP(a) (chooseIfHd((a), (a) * 3))
#define IF_HD(a) (chooseIfHd(0, (a)))

#define RES_RBSHIFT(a, b) (RES_SCALE(RES_DESCALE((a)) >> (b)))
#define RES_RECENTER(a) (RES_RBSHIFT((a), 1))

inline auto IsDos() { return (optWindowType == uqm::WindowStyle::DOS); }
#define DOS_BOOL(a, b) (IsDos() ? (b) : (a))
// Returns the 2nd input in DOS mode, the 1st input otherwise
#define DOS_BOOL_SCL(a, b) (RES_SCALE(IsDos() ? (b) : (a)))
// Same as DOS_BOOL but scaled to HdResolutionFactor
#define DOS_NUM(a) (DOS_BOOL(0, (a)))
// Returns the input number if DOS mode is active
#define DOS_NUM_SCL(a) (RES_SCALE(DOS_NUM((a))))
// Same as DOS_NUM but scales it to HdResolutionFactor
#define NDOS_NUM(a) (DOS_BOOL((a), 0))
// Returns the input number if DOS mode is not active
#define NDOS_NUM_SCL(a) (RES_SCALE(NDOS_NUM((a))))
// Same as NDOS_NUM but scales it to HdResolutionFactor

inline auto IsPad() { return (optWindowType == uqm::WindowStyle::Console3DO); }
#define SAFE_BOOL(a, b) (IsPad() ? (b) : (a))
// Returns the 2nd input in 3DO mode, the 1st input otherwise
#define SAFE_BOOL_SCL(a, b) (RES_SCALE(SAFE_BOOL((a), (b))))
// Same as SAFE_BOOL but scaled to HdResolutionFactor
#define SAFE_NUM(a) (SAFE_BOOL(0, (a)))
// Returns the input number if 3DO mode is active
#define SAFE_NUM_SCL(a) (RES_SCALE(SAFE_NUM((a))))
// Same as SAFE_NUM but scaled it to HdResolutionFactor
#define NSAFE_NUM(a) (SAFE_BOOL((a), 0))
// Returns the input number if 3DO mode is not active
#define NSAFE_NUM_SCL(a) (RES_SCALE(NSAFE_NUM((a))))
// Same as NSAFE_NUM but scaled it to HdResolutionFactor

#define MODE_CASE(a, b, c) (IsDos() ? (a) : (IsPad() ? (b) : (c)))
#define MODE_CASE_SCL(a, b, c) RES_SCALE(MODE_CASE((a), (b), (c)))

// Margins
inline auto SafeX() { return (SAFE_NUM_SCL(16)); }
// Left and right screen margin used for 3DO mode
inline auto SafeY() { return SafeX(); }
// Top and bottom screen margin used for 3DO mode

#define SAFE_NEG(a) (SAFE_NUM(SafeX() - RES_SCALE((a))))
// Returns SafeX() minus the input number, scaled to HdResolutionFactor
#define SAFE_POS(a) (SAFE_NUM(SafeX() + RES_SCALE((a))))
// Returns SafeX() plus the input number, scaled to HdResolutionFactor

inline auto SisOrgX() { return (RES_SCALE(6) + SAFE_POS(1)); }
inline auto SisOrgY() { return (RES_SCALE(9) + SAFE_POS(1)); }

/* Status bar & play area sizes. */
inline auto StatusWidth() { return RES_SCALE(64); }
/* Width of the status "window" (the right part of the screen) */
inline auto StatusHeight() { return CanvasHeight; }
/* Height of the status "window" (the right part of the screen) */
inline auto SpaceWidth() { return (CanvasWidth - StatusWidth() - (SafeX() * 2)); }
/* Width of the space "window" (the left part of the screen) */
inline auto SpaceHeight() { return (CanvasHeight - (SafeY() * 2)); }
/* Height of the space "window" (the left part of the screen) */
inline auto SisScreenWidth() { return (SpaceWidth() - (RES_SCALE(13) + SAFE_NUM_SCL(1))); }
/* Width of the usable part of the space "window" */
inline auto SisScreenHeight() { return (SpaceHeight() - RES_SCALE(13)); }
/* Height of the usable part of the space "window" */

inline auto OrigSisScreenWidth() { return (RES_DESCALE(SisScreenWidth())); }
inline auto OrigSisScreenHeight() { return (RES_DESCALE(SisScreenHeight())); }
inline constexpr auto DosSisScreenWidth {(243)};
inline constexpr auto DosSisScreenHeight {(187)};
inline constexpr auto ThreedoSisScreenWidth {(210)};
inline constexpr auto ThreedoSisScreenHeight {(195)};
inline constexpr auto UqmSisScreenWidth {(242)};
inline constexpr auto UqmSisScreenHeight {(227)};
inline constexpr auto HdmodSisScreenWidth {(1074)};
inline constexpr auto HdmodSisScreenHeight {(924)};

#define SIS_SCREEN_DIMENSIONS                            \
	{DosSisScreenWidth, DosSisScreenHeight},             \
		{ThreedoSisScreenWidth, ThreedoSisScreenHeight}, \
		{UqmSisScreenWidth, UqmSisScreenHeight},         \
		{HdmodSisScreenWidth, HdmodSisScreenHeight},

/* Radar. */
inline auto RadarX() { return (RES_SCALE(4) + (SpaceWidth() + SafeX())); }
inline auto RadarWidth() { return (StatusWidth() - RES_SCALE(8)); }
inline auto RadarHeight() { return RES_SCALE(53); }
inline auto RadarY() { return (SisOrgY() + SisScreenHeight() - RadarHeight()); }

/* Blue boxes which display messages and the green date box. */
inline auto SisTitleBoxWidth() { return RES_SCALE(57); }
inline auto SisTitleWidth() { return (SisTitleBoxWidth() - RES_SCALE(2)); }
inline auto SisTitleHeight() { return RES_SCALE(8); }
inline auto SisSpacerBoxWidth() { return RES_SCALE(12); }

inline auto SisMessageBoxWidth() { return (SisScreenWidth() - SisTitleBoxWidth() - SisSpacerBoxWidth()); }
inline auto SisMessageWidth() { return (SisMessageBoxWidth() - RES_SCALE(2)); }
inline auto SisMessageHeight() { return SisTitleHeight(); }

inline auto StatusMessageWidth() { return (StatusWidth() - RES_SCALE(4)); }
inline auto StatusMessageHeight() { return RES_SCALE(7); }

inline auto ShipNameWidth() { return (StatusWidth() - RES_SCALE(4)); }
inline auto ShipNameHeight() { return RES_SCALE(7); }

/* A lot of other shit. */
inline constexpr auto MaxReduction {3};
inline constexpr auto MaxVisReduction {2};
inline constexpr auto ReductionShift {1};
inline constexpr auto NumViews {(MaxVisReduction + 1)};

inline constexpr auto ZoomShift {8};
inline constexpr auto MaxZoomOut {(1 << (ZoomShift + MaxReduction - 1))};

inline constexpr auto OneShift {2};
inline constexpr auto BackgroundShift {3};
inline constexpr auto ScaledOne {(1 << OneShift)};
#define DISPLAY_TO_WORLD(x) ((x) << OneShift)
#define WORLD_TO_DISPLAY(x) ((x) >> OneShift)

inline auto LogSpaceWidth() { return (DISPLAY_TO_WORLD(SpaceWidth()) << MaxReduction); }
inline auto LogSpaceHeight() { return (DISPLAY_TO_WORLD(SpaceHeight()) << MaxReduction); }
inline auto TransitionWidth() { return (DISPLAY_TO_WORLD(SpaceWidth()) << MaxVisReduction); }
inline auto TransitionHeight() { return (DISPLAY_TO_WORLD(SpaceHeight()) << MaxVisReduction); }

// JMS_GFX: Changed from int16_t to int32_t and from uint16_t to uint32_t
#define DISPLAY_ALIGN(x) ((int32_t)(x) & ~(ScaledOne - 1))
#define DISPLAY_ALIGN_X(x) ((int32_t)((uint32_t)(x) % LogSpaceWidth()) & ~(ScaledOne - 1))
#define DISPLAY_ALIGN_Y(y) ((int32_t)((uint32_t)(y) % LogSpaceHeight()) & ~(ScaledOne - 1))

inline constexpr auto MaxXUniverse {9999};
inline constexpr auto MaxYUniverse {9999};
// Due to the added rounding error correction, the maximum logical X and Y
// in Hyperspace cannot go past 999.94999, otherwise the values will be
// rounded up to 1000.0. We do not want that so we subtract half a unit.
// The Y axis is inverted with respect to the screen Y axis.
// (MaxYUniverse - 1) is really 1 for our purposes.

inline constexpr auto SphereRadiusIncrement {11};

inline constexpr auto MaxFleetStrength {(254 * SphereRadiusIncrement)};

// XXX: These corrected for the weird screen aspect ratio on DOS
//   In part because of them, hyperflight is slower vertically
inline auto UnitScreenWidth() { return RES_SCALE(63); }
inline auto UnitScreenHeight() { return RES_SCALE(50); }

// These are for determining the HyperSpace boundaries
// for culling HyperStars when they move off screen
inline auto SisWorldWidth() { return DISPLAY_TO_WORLD(SisScreenWidth()); }
inline auto SisWorldHeight() { return DISPLAY_TO_WORLD(SisScreenHeight()); }
inline auto UnitScrDiff() { return DISPLAY_TO_WORLD(UnitScreenWidth() - UnitScreenHeight() + 1); }

inline auto HsWestEdge() { return ((SisWorldWidth() + UnitScrDiff()) >> 1); }
inline auto HsEastEdge() { return ((SisWorldWidth() - UnitScrDiff()) >> 1); }
inline auto HsNorthEdge() { return ((SisWorldHeight() + UnitScrDiff()) >> 1); }
inline auto HsSouthEdge() { return ((SisWorldHeight() - UnitScrDiff()) >> 1); }

// Bug #945: Simplified, these set the speed of SIS in Hyperspace and
//   Quasispace. The ratio between UNIVERSE_UNITS_ and LOG_UNITS_ is
//   what sets the speed, and it should be 1:16 to match the original.
//   The unit factors are reduced to keep the translation math within
//   32 bits. The original math is unnecessarily complex and depends
//   on the screen resolution when it should not.
//   Using the new math will break old savegames.

inline constexpr auto UniverseUnitsX {(((MaxXUniverse + 1) >> 4))};
inline constexpr auto UniverseUnitsY {(((MaxYUniverse + 1) >> 4))};
inline auto LogUnitsX() { return ((int32_t)(UniverseUnitsX * RES_SCALE(16))); }
inline auto LogUnitsY() { return ((int32_t)(UniverseUnitsY * RES_SCALE(16))); }

#define ROUNDING_ERROR(div) ((div) >> 1)

static inline int32_t
logxToUniverse(int32_t lx)
{
	return (int32_t)((lx * UniverseUnitsX + ROUNDING_ERROR(LogUnitsX()))
					 / LogUnitsX());
}
#define LOGX_TO_UNIVERSE(lx) \
	logxToUniverse(lx)
static inline int32_t
logyToUniverse(int32_t ly)
{
	return (int32_t)(MaxYUniverse - ((ly * UniverseUnitsY + ROUNDING_ERROR(LogUnitsY())) / LogUnitsY()));
}
#define LOGY_TO_UNIVERSE(ly) \
	logyToUniverse(ly)
static inline int32_t
universeToLogx(int16_t ux)
{
	return (ux * LogUnitsX() + ROUNDING_ERROR(UniverseUnitsX))
		 / UniverseUnitsX;
}
#define UNIVERSE_TO_LOGX(ux) \
	universeToLogx(ux)
static inline int32_t
universeToLogy(int16_t uy)
{
	return ((MaxYUniverse - uy) * LogUnitsY()
			+ ROUNDING_ERROR(UniverseUnitsY))
		 / UniverseUnitsY;
}
#define UNIVERSE_TO_LOGY(uy) \
	universeToLogy(uy)

inline auto MaxXLogical() { return (UNIVERSE_TO_LOGX(MaxXUniverse + 1) - (UNIVERSE_TO_LOGX(1) >> 1) - 1L); }
inline auto MaxYLogical() { return (UNIVERSE_TO_LOGY(-1) - (UNIVERSE_TO_LOGY(MaxYUniverse - 1) >> 1) - 1L); }

// Original (and now broken) Hyperspace speed factors
// Now being utilized to load Vanilla saves properly
inline constexpr auto SectorWidth {195};
inline constexpr auto SectorHeight {25};

inline constexpr auto OldLogUnitsX {((int32_t)(8192 >> 4) * SectorWidth)};
inline constexpr auto OldLogUnitsY {((int32_t)(7680 >> 4) * SectorHeight)};
inline constexpr auto OldUniverseUnitsX {(((MaxXUniverse + 1) >> 4) * 10)};
inline constexpr auto OldUniverseUnitsY {(((MaxYUniverse + 1) >> 4))};

static inline int32_t
oldLogxToUniverse(int32_t lx)
{
	return (int32_t)((lx * OldUniverseUnitsX + ROUNDING_ERROR(OldLogUnitsX))
					 / OldLogUnitsX);
}

static inline int32_t
oldLogyToUniverse(int32_t ly)
{
	return (int32_t)(MaxYUniverse - ((ly * OldUniverseUnitsY + ROUNDING_ERROR(OldLogUnitsY)) / OldLogUnitsY));
}

static inline int32_t
inBounds(int32_t val, int32_t min, int32_t max)
{
	if (val < min)
	{
		return min;
	}
	if (val > max)
	{
		return max;
	}
	return val;
}

inline constexpr auto CircleShift {6};
inline constexpr auto FullCircle {(1 << CircleShift)};

inline constexpr auto HalfCircle {(FullCircle >> 1)};
inline constexpr auto Quadrant {(FullCircle >> 2)};
inline constexpr auto Octant {(FullCircle >> 3)};

inline constexpr auto FacingShift {4};

#define ANGLE_TO_FACING(a) (((a) + (1 << (CircleShift - FacingShift - 1))) \
							>> (CircleShift - FacingShift))
#define FACING_TO_ANGLE(f) ((f) << (CircleShift - FacingShift))

#define NORMALIZE_ANGLE(a) ((uint32_t)((a) & (FullCircle - 1)))
#define NORMALIZE_FACING(f) ((uint32_t)((f) & ((1 << FacingShift) - 1)))

#define DEGREES_TO_ANGLE(d) NORMALIZE_ANGLE((((d) % 360) * FullCircle \
											 + HalfCircle)            \
											/ 360)
#define ANGLE_TO_DEGREES(d) (NORMALIZE_ANGLE(d) * 360 / FullCircle)

inline constexpr auto SinShift {14};
inline constexpr auto SinScale {(1 << SinShift)};
#define INT_ADJUST(x) ((x) << SinShift)
#define FLT_ADJUST(x) (int16_t)((x) * SinScale)
#define UNADJUST(x) (int16_t)((x) >> SinShift)
#define ROUND(x, y) ((x) + ((x) >= 0 ? ((y) >> 1) : -((y) >> 1)))

extern int32_t sinetab[];
#define SINVAL(a) sinetab[NORMALIZE_ANGLE(a)]
#define COSVAL(a) SINVAL((a) + Quadrant)
#define SINE(a, m) ((int32_t)((((long)SINVAL(a)) * (long)(m)) >> SinShift))
#define COSINE(a, m) SINE((a) + Quadrant, m)
extern uint16_t ARCTAN(int32_t delta_x, int32_t delta_y);

#define WRAP_VAL(v, w) ((uint32_t)((v) < 0 ? ((v) + (w)) : ((v) >= (w) ? ((v) - (w)) : (v))))
#define WRAP_X(x) WRAP_VAL(x, LogSpaceWidth())
#define WRAP_Y(y) WRAP_VAL(y, LogSpaceHeight())
#define WRAP_DELTA_X(dx) ((dx) < 0 ?                                                                \
							  ((-(dx) <= LogSpaceWidth() >> 1) ? (dx) : (LogSpaceWidth() + (dx))) : \
							  (((dx) <= LogSpaceWidth() >> 1) ? (dx) : ((dx) - LogSpaceWidth())))
#define WRAP_DELTA_Y(dy) ((dy) < 0 ?                                                                  \
							  ((-(dy) <= LogSpaceHeight() >> 1) ? (dy) : (LogSpaceHeight() + (dy))) : \
							  (((dy) <= LogSpaceHeight() >> 1) ? (dy) : ((dy) - LogSpaceHeight())))

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_UNITS_H_ */
