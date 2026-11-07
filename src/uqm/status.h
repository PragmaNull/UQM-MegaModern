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
#ifndef UQM_STATUS_H
#define UQM_STATUS_H

#include "races.h"
#include "options/options.h"
#include "globdata.h"

namespace uqm
{
inline auto ShipInfoHeight() { return ResScale(65); }

} // namespace uqm

#include "core/misc/bittricks.h"

namespace uqm
{
constexpr auto MenuBool(const auto& a, const auto& b) { return (isPC(uqm::UQMOptions::read().whichMenu) ? (b) : (a)); }


inline auto CrewXoffs() { return ResScale(4); }
inline auto EnergyXoffs() { return ResScale(52); }
inline auto GaugeYoffs() { return (ShipInfoHeight() - ResScale(10)); }
inline auto UnitWidth() { return ResScale(2); }
inline auto UnitHeight() { return ResScale(1); }
inline auto StatWidth() { return (UnitWidth() * 2 + ResScale(3)); }
//(ResScale (1) + UnitWidth() + ResScale (1) + UnitWidth() + ResScale (1))

inline auto CaptainWidth() { return ResScale(55); }
inline auto CaptainHeight() { return ResScale(30); }
inline auto CaptainXoffs() { return ResScale(ResDescale(StatusWidth() - CaptainWidth()) >> 1); }
inline auto CaptainYoffs() { return (ShipInfoHeight() + ResScale(MenuBool(DosBool(4, 1), 1))); }

inline auto ShipStatusHeight() { return ((StatusHeight() >> 1) - SafeX()); }
inline constexpr auto BadGuyYoffs {0};
inline auto GoodGuyYoffs() { return ShipStatusHeight(); }

inline auto StarconTextHeight() { return ResScale(7); }
inline auto TinyTextHeight() { return ResScale(9); }
inline auto BattleCrewX() { return ResScale(8); }
inline auto BattleCrewY() { return (ResScale(64) - SafeY()); }

extern int16_t status_y_offsets[];

extern void InitStatusOffsets(void);

extern void DrawCrewFuelString(int16_t y, int16_t state);
extern void ClearShipStatus(int16_t y);
extern void OutlineShipStatus(int16_t y);
extern void InitShipStatus(ShipInfoDesc* ShipInfoPtr, Starship* StarShipPtr, GfxRect* pClipRect, bool inMeleeMenu);
// StarShipPtr or pClipRect can be nullptr
extern void DeltaStatistics(ShipInfoDesc* ShipInfoPtr, int16_t y_offs,
							int16_t crew_delta, int16_t energy_delta);
extern void DrawBattleCrewAmount(ShipInfoDesc* ShipInfoPtr, int16_t y_offs);

extern void DrawCaptainsWindow(Starship* StarShipPtr);
extern bool DeltaEnergy(Element* ElementPtr, int16_t energy_delta);
extern bool DeltaCrew(Element* ElementPtr, int16_t crew_delta);

extern void PreProcessStatus(Element* ShipPtr);
extern void PostProcessStatus(Element* ShipPtr);

} // namespace uqm

#endif // UQM_STATUS_H
