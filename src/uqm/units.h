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
constexpr auto ResScale(const auto& a) { return ((a) << resolutionFactor); }
constexpr auto ResDescale(const auto& a) { return ((a) >> resolutionFactor); }
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


constexpr auto ResTrp(const auto& a) { return (chooseIfHd((a), (a) * 3)); }
constexpr auto IfHd(const auto& a) { return (chooseIfHd(0, (a))); }

constexpr auto ResRbshift(const auto& a, const auto& b) { return (ResScale(ResDescale((a)) >> (b))); }
constexpr auto ResRecenter(const auto& a) { return (ResRbshift((a), 1)); }

inline auto IsDos() { return (optWindowType == uqm::WindowStyle::DOS); }
constexpr auto DosBool(const auto& a, const auto& b) { return (IsDos() ? (b) : (a)); }
// Returns the 2nd input in DOS mode, the 1st input otherwise
constexpr auto DosBoolScl(const auto& a, const auto& b) { return (ResScale(IsDos() ? (b) : (a))); }
// Same as DosBool but scaled to HdResolutionFactor
constexpr auto DosNum(const auto& a) { return (DosBool(0, (a))); }
// Returns the input number if DOS mode is active
constexpr auto DosNumScl(const auto& a) { return (ResScale(DosNum((a)))); }
// Same as DosNum but scales it to HdResolutionFactor
constexpr auto NdosNum(const auto& a) { return (DosBool((a), 0)); }
// Returns the input number if DOS mode is not active
constexpr auto NdosNumScl(const auto& a) { return (ResScale(NdosNum((a)))); }
// Same as NdosNum but scales it to HdResolutionFactor

inline auto IsPad() { return (optWindowType == uqm::WindowStyle::Console3DO); }
constexpr auto SafeBool(const auto& a, const auto& b) { return (IsPad() ? (b) : (a)); }
// Returns the 2nd input in 3DO mode, the 1st input otherwise
constexpr auto SafeBoolScl(const auto& a, const auto& b) { return (ResScale(SafeBool((a), (b)))); }
// Same as SafeBool but scaled to HdResolutionFactor
constexpr auto SafeNum(const auto& a) { return (SafeBool(0, (a))); }
// Returns the input number if 3DO mode is active
constexpr auto SafeNumScl(const auto& a) { return (ResScale(SafeNum((a)))); }
// Same as SafeNum but scaled it to HdResolutionFactor
constexpr auto NsafeNum(const auto& a) { return (SafeBool((a), 0)); }
// Returns the input number if 3DO mode is not active
constexpr auto NsafeNumScl(const auto& a) { return (ResScale(NsafeNum((a)))); }
// Same as NsafeNum but scaled it to HdResolutionFactor

constexpr auto ModeCase(const auto& a, const auto& b, const auto& c) { return (IsDos() ? (a) : (IsPad() ? (b) : (c))); }


// Margins
inline auto SafeX() { return (SafeNumScl(16)); }
// Left and right screen margin used for 3DO mode
inline auto SafeY() { return SafeX(); }
// Top and bottom screen margin used for 3DO mode

constexpr auto SafeNeg(const auto& a) { return (SafeNum(SafeX() - ResScale((a)))); }
// Returns SafeX() minus the input number, scaled to HdResolutionFactor
constexpr auto SafePos(const auto& a) { return (SafeNum(SafeX() + ResScale((a)))); }
// Returns SafeX() plus the input number, scaled to HdResolutionFactor

inline auto SisOrgX() { return (ResScale(6) + SafePos(1)); }
inline auto SisOrgY() { return (ResScale(9) + SafePos(1)); }

/* Status bar & play area sizes. */
inline auto StatusWidth() { return ResScale(64); }
/* Width of the status "window" (the right part of the screen) */
inline auto StatusHeight() { return CanvasHeight; }
/* Height of the status "window" (the right part of the screen) */
inline auto SpaceWidth() { return (CanvasWidth - StatusWidth() - (SafeX() * 2)); }
/* Width of the space "window" (the left part of the screen) */
inline auto SpaceHeight() { return (CanvasHeight - (SafeY() * 2)); }
/* Height of the space "window" (the left part of the screen) */
inline auto SisScreenWidth() { return (SpaceWidth() - (ResScale(13) + SafeNumScl(1))); }
/* Width of the usable part of the space "window" */
inline auto SisScreenHeight() { return (SpaceHeight() - ResScale(13)); }
/* Height of the usable part of the space "window" */

inline auto OrigSisScreenWidth() { return (ResDescale(SisScreenWidth())); }
inline auto OrigSisScreenHeight() { return (ResDescale(SisScreenHeight())); }
inline constexpr auto DosSisScreenWidth {(243)};
inline constexpr auto DosSisScreenHeight {(187)};
inline constexpr auto ThreedoSisScreenWidth {(210)};
inline constexpr auto ThreedoSisScreenHeight {(195)};
inline constexpr auto UqmSisScreenWidth {(242)};
inline constexpr auto UqmSisScreenHeight {(227)};
inline constexpr auto HdmodSisScreenWidth {(1074)};
inline constexpr auto HdmodSisScreenHeight {(924)};

inline constexpr EXTENT SisScreenDimensions[] {
	{DosSisScreenWidth,		DosSisScreenHeight	  },
	{ThreedoSisScreenWidth, ThreedoSisScreenHeight},
	{UqmSisScreenWidth,		UqmSisScreenHeight	  },
	{HdmodSisScreenWidth,	  HdmodSisScreenHeight  },
};

/* Radar. */
inline auto RadarX() { return (ResScale(4) + (SpaceWidth() + SafeX())); }
inline auto RadarWidth() { return (StatusWidth() - ResScale(8)); }
inline auto RadarHeight() { return ResScale(53); }
inline auto RadarY() { return (SisOrgY() + SisScreenHeight() - RadarHeight()); }

/* Blue boxes which display messages and the green date box. */
inline auto SisTitleBoxWidth() { return ResScale(57); }
inline auto SisTitleWidth() { return (SisTitleBoxWidth() - ResScale(2)); }
inline auto SisTitleHeight() { return ResScale(8); }
inline auto SisSpacerBoxWidth() { return ResScale(12); }

inline auto SisMessageBoxWidth() { return (SisScreenWidth() - SisTitleBoxWidth() - SisSpacerBoxWidth()); }
inline auto SisMessageWidth() { return (SisMessageBoxWidth() - ResScale(2)); }
inline auto SisMessageHeight() { return SisTitleHeight(); }

inline auto StatusMessageWidth() { return (StatusWidth() - ResScale(4)); }
inline auto StatusMessageHeight() { return ResScale(7); }

inline auto ShipNameWidth() { return (StatusWidth() - ResScale(4)); }
inline auto ShipNameHeight() { return ResScale(7); }

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
constexpr auto DisplayToWorld(const auto& x) { return ((x) << OneShift); }
constexpr auto WorldToDisplay(const auto& x) { return ((x) >> OneShift); }

inline auto LogSpaceWidth() { return (DisplayToWorld(SpaceWidth()) << MaxReduction); }
inline auto LogSpaceHeight() { return (DisplayToWorld(SpaceHeight()) << MaxReduction); }
inline auto TransitionWidth() { return (DisplayToWorld(SpaceWidth()) << MaxVisReduction); }
inline auto TransitionHeight() { return (DisplayToWorld(SpaceHeight()) << MaxVisReduction); }

// JMS_GFX: Changed from int16_t to int32_t and from uint16_t to uint32_t
constexpr auto DisplayAlign(const auto& x) { return ((int32_t)(x) & ~(ScaledOne - 1)); }
constexpr auto DisplayAlignX(const auto& x) { return ((int32_t)((uint32_t)(x) % LogSpaceWidth()) & ~(ScaledOne - 1)); }
constexpr auto DisplayAlignY(const auto& y) { return ((int32_t)((uint32_t)(y) % LogSpaceHeight()) & ~(ScaledOne - 1)); }

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
inline auto UnitScreenWidth() { return ResScale(63); }
inline auto UnitScreenHeight() { return ResScale(50); }

// These are for determining the HyperSpace boundaries
// for culling HyperStars when they move off screen
inline auto SisWorldWidth() { return DisplayToWorld(SisScreenWidth()); }
inline auto SisWorldHeight() { return DisplayToWorld(SisScreenHeight()); }
inline auto UnitScrDiff() { return DisplayToWorld(UnitScreenWidth() - UnitScreenHeight() + 1); }

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
inline auto LogUnitsX() { return ((int32_t)(UniverseUnitsX * ResScale(16))); }
inline auto LogUnitsY() { return ((int32_t)(UniverseUnitsY * ResScale(16))); }

constexpr auto RoundingError(const auto& div) { return ((div) >> 1); }

static inline int32_t logxToUniverse(int32_t lx)
{
	return (int32_t)((lx * UniverseUnitsX + RoundingError(LogUnitsX()))
					 / LogUnitsX());
}
constexpr auto LogxToUniverse(const auto& lx) { return logxToUniverse(lx); }
static inline int32_t logyToUniverse(int32_t ly)
{
	return (int32_t)(MaxYUniverse - ((ly * UniverseUnitsY + RoundingError(LogUnitsY())) / LogUnitsY()));
}
constexpr auto LogyToUniverse(const auto& ly) { return logyToUniverse(ly); }
static inline int32_t universeToLogx(int16_t ux)
{
	return (ux * LogUnitsX() + RoundingError(UniverseUnitsX))
		 / UniverseUnitsX;
}
constexpr auto UniverseToLogx(const auto& ux) { return universeToLogx(ux); }
static inline int32_t universeToLogy(int16_t uy)
{
	return ((MaxYUniverse - uy) * LogUnitsY()
			+ RoundingError(UniverseUnitsY))
		 / UniverseUnitsY;
}
constexpr auto UniverseToLogy(const auto& uy) { return universeToLogy(uy); }

inline auto MaxXLogical() { return (UniverseToLogx(MaxXUniverse + 1) - (UniverseToLogx(1) >> 1) - 1L); }
inline auto MaxYLogical() { return (UniverseToLogy(-1) - (UniverseToLogy(MaxYUniverse - 1) >> 1) - 1L); }

// Original (and now broken) Hyperspace speed factors
// Now being utilized to load Vanilla saves properly
inline constexpr auto SectorWidth {195};
inline constexpr auto SectorHeight {25};

inline constexpr auto OldLogUnitsX {((int32_t)(8192 >> 4) * SectorWidth)};
inline constexpr auto OldLogUnitsY {((int32_t)(7680 >> 4) * SectorHeight)};
inline constexpr auto OldUniverseUnitsX {(((MaxXUniverse + 1) >> 4) * 10)};
inline constexpr auto OldUniverseUnitsY {(((MaxYUniverse + 1) >> 4))};

static inline int32_t oldLogxToUniverse(int32_t lx)
{
	return (int32_t)((lx * OldUniverseUnitsX + RoundingError(OldLogUnitsX))
					 / OldLogUnitsX);
}

static inline int32_t oldLogyToUniverse(int32_t ly)
{
	return (int32_t)(MaxYUniverse - ((ly * OldUniverseUnitsY + RoundingError(OldLogUnitsY)) / OldLogUnitsY));
}

static inline int32_t inBounds(int32_t val, int32_t min, int32_t max)
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

constexpr auto AngleToFacing(const auto& a) { return (((a) + (1 << (CircleShift - FacingShift - 1))) >> (CircleShift - FacingShift)); }
constexpr auto FacingToAngle(const auto& f) { return ((f) << (CircleShift - FacingShift)); }

constexpr auto NormalizeAngle(const auto& a) { return ((uint32_t)((a) & (FullCircle - 1))); }
constexpr auto NormalizeFacing(const auto& f) { return ((uint32_t)((f) & ((1 << FacingShift) - 1))); }


inline constexpr auto SinShift {14};
inline constexpr auto SinScale {(1 << SinShift)};

constexpr auto FltAdjust(const auto& x) { return (int16_t)((x)*SinScale); }


extern int32_t sinetab[];
constexpr auto Sinval(const auto& a) { return sinetab[NormalizeAngle(a)]; }

constexpr auto Sine(const auto& a, const auto& m) { return ((int32_t)((((long)Sinval(a)) * (long)(m)) >> SinShift)); }
constexpr auto Cosine(const auto& a, const auto& m) { return Sine((a) + Quadrant, m); }
extern uint16_t ARCTAN(int32_t delta_x, int32_t delta_y);

constexpr auto WrapVal(const auto& v, const auto& w) { return ((uint32_t)((v) < 0 ? ((v) + (w)) : ((v) >= (w) ? ((v) - (w)) : (v)))); }
constexpr auto WrapX(const auto& x) { return WrapVal(x, LogSpaceWidth()); }
constexpr auto WrapY(const auto& y) { return WrapVal(y, LogSpaceHeight()); }
constexpr auto WrapDeltaX(const auto& dx) { return ((dx) < 0 ? ((-(dx) <= LogSpaceWidth() >> 1) ? (dx) : (LogSpaceWidth() + (dx))) : (((dx) <= LogSpaceWidth() >> 1) ? (dx) : ((dx)-LogSpaceWidth()))); }
constexpr auto WrapDeltaY(const auto& dy) { return ((dy) < 0 ? ((-(dy) <= LogSpaceHeight() >> 1) ? (dy) : (LogSpaceHeight() + (dy))) : (((dy) <= LogSpaceHeight() >> 1) ? (dy) : ((dy)-LogSpaceHeight()))); }

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_UNITS_H_ */
