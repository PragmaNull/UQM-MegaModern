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
#ifndef UQM_SHIPCONT_H
#define UQM_SHIPCONT_H

#include "menustat.h"

namespace uqm
{
#if 0 //defined(__cplusplus)
extern "C" {
#endif

inline auto FieldWidth() { return (StatusWidth() - ResScale(5)); }

extern void CargoMenu(void);
extern bool RosterMenu(void);
extern bool DevicesMenu(void);
extern bool StarMap(void);

extern void DrawCargoStrings(uint8_t OldElement, uint8_t NewElement);
extern void ShowRemainingCapacity(void);
extern void DrawRainbowPlanet(uint16_t planet);
extern bool InvokeSpawner(void);

extern int16_t InventoryDevices(uint8_t* pDeviceMap, uint16_t Size);

extern FONT ModuleFont;

#if 0 //defined(__cplusplus)
}
#endif

} // namespace uqm

#endif // UQM_SHIPCONT_H
