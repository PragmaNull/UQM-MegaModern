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

#ifndef UQM_STATUS_H_INCL_
#define UQM_STATUS_H_INCL_

#include "races.h"
#include "options/options.h"
#include "globdata.h"

inline auto ShipInfoHeight() { return RES_SCALE(65); }
#include "core/misc/bittricks.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

#define MENU_BOOL(a, b) (isPC(uqm::UQMOptions::read().whichMenu) ? (b) : (a))
#define FONT_BOOL(a, b) (isPC(uqm::UQMOptions::read().whichFonts) ? (b) : (a))

inline auto CrewXoffs() { return RES_SCALE(4); }
inline auto EnergyXoffs() { return RES_SCALE(52); }
inline auto GaugeYoffs() { return (ShipInfoHeight() - RES_SCALE(10)); }
inline auto UnitWidth() { return RES_SCALE(2); }
inline auto UnitHeight() { return RES_SCALE(1); }
inline auto StatWidth() { return (UnitWidth() * 2 + RES_SCALE(3)); }
//(RES_SCALE (1) + UnitWidth() + RES_SCALE (1) + UnitWidth() + RES_SCALE (1))

inline auto CaptainWidth() { return RES_SCALE(55); }
inline auto CaptainHeight() { return RES_SCALE(30); }
inline auto CaptainXoffs() { return RES_SCALE(RES_DESCALE(StatusWidth() - CaptainWidth()) >> 1); }
inline auto CaptainYoffs() { return (ShipInfoHeight() + RES_SCALE(MENU_BOOL(DOS_BOOL(4, 1), 1))); }

inline auto ShipStatusHeight() { return ((StatusHeight() >> 1) - SafeX()); }
inline constexpr auto BadGuyYoffs {0};
inline auto GoodGuyYoffs() { return ShipStatusHeight(); }

inline auto StarconTextHeight() { return RES_SCALE(7); }
inline auto TinyTextHeight() { return RES_SCALE(9); }
inline auto BattleCrewX() { return RES_SCALE(8); }
inline auto BattleCrewY() { return (RES_SCALE(64) - SafeY()); }

extern int16_t status_y_offsets[];

extern void InitStatusOffsets(void);

extern void DrawCrewFuelString(int16_t y, int16_t state);
extern void ClearShipStatus(int16_t y);
extern void OutlineShipStatus(int16_t y);
extern void InitShipStatus(SHIP_INFO* ShipInfoPtr, STARSHIP* StarShipPtr, GFXRECT* pClipRect, bool inMeleeMenu);
// StarShipPtr or pClipRect can be nullptr
extern void DeltaStatistics(SHIP_INFO* ShipInfoPtr, int16_t y_offs,
							int16_t crew_delta, int16_t energy_delta);
extern void DrawBattleCrewAmount(SHIP_INFO* ShipInfoPtr, int16_t y_offs);

extern void DrawCaptainsWindow(STARSHIP* StarShipPtr);
extern bool DeltaEnergy(ELEMENT* ElementPtr, int16_t energy_delta);
extern bool DeltaCrew(ELEMENT* ElementPtr, int16_t crew_delta);

extern void PreProcessStatus(ELEMENT* ShipPtr);
extern void PostProcessStatus(ELEMENT* ShipPtr);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_STATUS_H_INCL_ */
