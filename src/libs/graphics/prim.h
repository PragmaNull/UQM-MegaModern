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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef LIBS_GRAPHICS_PRIM_H_
#define LIBS_GRAPHICS_PRIM_H_

enum gfx_object
{
	POINT_PRIM = 0,
	STAMP_PRIM,
	STAMPFILL_PRIM,
	LINE_PRIM,
	TEXT_PRIM,
	RECT_PRIM,
	RECTFILL_PRIM,
	POINT_PRIM_HD,

	NUM_PRIMS
};
typedef uint8_t GRAPHICS_PRIM;

union PRIM_DESC
{
	GFXPOINT Point;
	STAMP Stamp;
	LINE Line;
	TEXT Text;
	GFXRECT Rect;
};

typedef uint32_t PRIM_LINKS;

struct PRIMITIVE
{
	PRIM_LINKS Links {};
	GRAPHICS_PRIM Type {};
	Color color {};
	PRIM_DESC Object {};
	uint8_t flags {};
};

inline constexpr auto EndOfList {((uint16_t)0xFFFF)};

/* Primitive flags */
inline constexpr auto UnscaledStamp {(1 << 0)};		// Stamp will not be scaled via smooth scaler
inline constexpr auto HyperToQuasiColor {(1 << 1)}; // Stamp be rendered with quasispace filter (for ambience in HdResolutionFactor only)
inline constexpr auto HsStarmask {(1 << 2)};

constexpr auto GetPredLink(auto l) { return uqm::lowWord(l); }
constexpr auto GetSuccLink(auto l) { return uqm::highWord(l); }

constexpr auto SetPrimLinks(auto pPrim, auto p, auto s) { return ((pPrim)->Links = uqm::makeDword(p, s)); }
constexpr auto GetPrimLinks(auto pPrim) { return ((pPrim)->Links); }
constexpr auto SetPrimType(auto pPrim, auto t) { return ((pPrim)->Type = t); }
constexpr auto GetPrimType(auto pPrim) { return ((pPrim)->Type); }
constexpr auto SetPrimColor(auto pPrim, auto c) { return ((pPrim)->color = c); }
constexpr auto GetPrimColor(auto pPrim) { return ((pPrim)->color); }
constexpr auto SetPrimFlags(auto pPrim, auto f) { return ((pPrim)->flags = f); }
constexpr auto GetPrimFlags(auto pPrim) { return ((pPrim)->flags); }

static inline void SetPrimNextLink(PRIMITIVE* pPrim, uint16_t Link)
{
	SetPrimLinks(pPrim, EndOfList, Link);
}


static inline uint16_t GetPrimNextLink(PRIMITIVE* pPrim)
{
	return GetSuccLink(GetPrimLinks(pPrim));
}


#endif /* PRIM_H */
