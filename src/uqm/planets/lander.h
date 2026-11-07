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
#ifndef UQM_PLANETS_LANDER_H
#define UQM_PLANETS_LANDER_H


#include "../element.h"
#include "core/misc/bittricks.h"
#include "core/types/enum.h"
#include "elemdata.h"
#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "libs/time/timelib.h"

namespace uqm
{
// Surface magnification shift (x4)
inline constexpr auto MagShift {2};

inline constexpr auto NumTextFrames {32};

struct LanderNodeData
{
	uint8_t NumNodesGrabbed;
	uqstl::array<uint8_t, 32> NodeType;
	uqstl::array<uint16_t, 32> NodeAmounts;
};

// XXX: This is a private type now. Move it to lander.c?
//   We may also want to merge it with LanderInputState.
struct PlanetsideDesc
{
	bool InTransit;
	// Landing on or taking of from a planet.
	// Setting it while landed will initiate takeoff.

	uint16_t ElementLevel;
	uint16_t MaxElementLevel;
	uint16_t BiologicalLevel;
	uqstl::array<uint16_t, *MineralCategory::NumElementCategories> ElementAmounts;

	uint16_t NumFrames;
	char AmountBuf[40];
	uqstl::array<GfxText, 3> MineralText;

	uqstl::array<Color, (((NumTextFrames >> 1)))> ColorCycle;

	uint8_t TectonicsChance;
	uint8_t WeatherChance;
	uint8_t FireChance;

	LanderNodeData NodeData;
};

extern MusicRef LanderMusic;

extern void PlanetSide(GfxPoint planetLoc);
extern void DoDiscoveryReport(SoundBankRef ReadOutSounds);
extern uint8_t SetPlanetMusic(uint8_t planet_type);
extern void LoadLanderData(void);
extern void FreeLanderData(void);

extern void MaskLanderGraphics(void);

extern void object_animation(Element* ElementPtr);

extern void SetLanderTakeoff(void);
extern bool KillLanderCrewSeq(uint16_t numKilled, TimePeriod period);

extern unsigned GetThermalHazardRating(int temp);

// Element.playerNr constants
enum class PlanetsidePlayer
{
	Human,
	NonPlayer,
};
consteval bool isArithmeticEnum(PlanetsidePlayer) { return true; }

} // namespace uqm

#endif // UQM_PLANETS_LANDER_H
