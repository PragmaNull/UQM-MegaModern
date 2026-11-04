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

#define END_OF_LIST ((uint16_t)0xFFFF)

/* Primitive flags */
#define UNSCALED_STAMP (1 << 0)		  // Stamp will not be scaled via smooth scaler
#define HYPER_TO_QUASI_COLOR (1 << 1) // Stamp be rendered with quasispace filter (for ambience in HD only)
#define HS_STARMASK (1 << 2)

#define GetPredLink(l) uqm::lowWord(l)
#define GetSuccLink(l) uqm::highWord(l)
#define MakeLinks uqm::makeDword
#define SetPrimLinks(pPrim, p, s) ((pPrim)->Links = MakeLinks(p, s))
#define GetPrimLinks(pPrim) ((pPrim)->Links)
#define SetPrimType(pPrim, t) ((pPrim)->Type = t)
#define GetPrimType(pPrim) ((pPrim)->Type)
#define SetPrimColor(pPrim, c) ((pPrim)->color = c)
#define GetPrimColor(pPrim) ((pPrim)->color)
#define SetPrimFlags(pPrim, f) ((pPrim)->flags = f)
#define GetPrimFlags(pPrim) ((pPrim)->flags)

static inline void
SetPrimNextLink(PRIMITIVE* pPrim, uint16_t Link)
{
	SetPrimLinks(pPrim, END_OF_LIST, Link);
}


static inline uint16_t
GetPrimNextLink(PRIMITIVE* pPrim)
{
	return GetSuccLink(GetPrimLinks(pPrim));
}


#endif /* PRIM_H */
