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

#pragma once
#ifndef UQM_STARBASE_H
#define UQM_STARBASE_H

#include "core/types/enum.h"
#include "menustat.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

enum class StarbaseMenuItem
{
	TalkCommander = 0,
	OutfitStarship,
	Shipyard,
	DepartBase
};
consteval bool isArithmeticEnum(StarbaseMenuItem) { return true; }
using StarbaseState = uint8_t;

extern void InstallBombAtEarth(void);
extern void VisitStarBase(void);
extern bool DoStarBase(MenuState* pMS);
extern bool DoOutfit(MenuState* pMS);
extern bool DoShipyard(MenuState* pMS);

extern void DrawShipPiece(FrameRef ModuleFrame, uint16_t which_piece, uint16_t which_slot, bool DrawBluePrint);

extern uint16_t WrapText(const char* pStr, uint16_t len, GfxText* tarray, int16_t field_width);
// XXX: Doesn't really belong in this file.

extern uint16_t ShipCost(uint8_t race_id);

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_STARBASE_H
