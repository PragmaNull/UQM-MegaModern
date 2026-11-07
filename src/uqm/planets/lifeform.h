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
#ifndef UQM_PLANETS_LIFEFORM_H
#define UQM_PLANETS_LIFEFORM_H

#include "core/misc/bittricks.h"

namespace uqm
{
inline constexpr auto BehaviorHunt {0 << 0};
inline constexpr auto BehaviorFlee {1 << 0};
inline constexpr auto BehaviorUnpredictable {2 << 0};

inline constexpr auto BehaviorMask {0x03};
inline constexpr auto BehaviorShift {0};

inline constexpr auto AwarenessLow {0 << 2};
inline constexpr auto AwarenessMedium {1 << 2};
inline constexpr auto AwarenessHigh {2 << 2};

inline constexpr auto AwarenessMask {0x0C};
inline constexpr auto AwarenessShift {BehaviorShift + 2};

inline constexpr auto SpeedMotionless {0 << 4};
inline constexpr auto SpeedSlow {1 << 4};
inline constexpr auto SpeedMedium {2 << 4};
inline constexpr auto SpeedFast {3 << 4};

inline constexpr auto SpeedMask {0x30};
inline constexpr auto SpeedShift {AwarenessShift + 2};

inline constexpr auto DangerHarmless {0 << 6};
inline constexpr auto DangerWeak {1 << 6};
inline constexpr auto DangerNormal {2 << 6};
inline constexpr auto DangerMonstrous {3 << 6};

inline constexpr auto DangerMask {0xC0};
inline constexpr auto DangerShift {SpeedShift + 2};

inline constexpr auto NumCreatureTypes {23};
inline constexpr auto NumSpecialCreatureTypes {3};
inline constexpr auto MaxLifeVariation {3};

inline constexpr auto EvilOne {NumCreatureTypes};
inline constexpr auto BrainboxBulldozer {NumCreatureTypes + 1};
inline constexpr auto ZexBeauty {NumCreatureTypes + 2};

inline constexpr auto CreatureAware {(uint8_t)(1 << 7)};

struct LifeformDesc
{
	uint8_t Attributes, ValueAndHitPoints;
};

extern LifeformDesc CreatureData[];

} // namespace uqm

#endif // UQM_PLANETS_LIFEFORM_H
