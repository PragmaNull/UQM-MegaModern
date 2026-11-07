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
#ifndef UQM_SOUNDS_H
#define UQM_SOUNDS_H

#include "cons_res.h"
#include "core/misc/bittricks.h"
#include "core/types/enum.h"
#include "element.h"
#include "globdata.h"
#include "libs/sndlib.h"
#include "races.h"

namespace uqm
{
enum class SoundEffect
{
	GrabCrew = 0,
	ShipExplodes,
	TargetDamagedFor1Pt,
	TargetDamagedFor23Pt,
	TargetDamagedFor45Pt,
	TargetDamagedFor6PlusPt
};
consteval bool isArithmeticEnum(SoundEffect) { return true; }

enum class MenuSoundEffect
{
	Move = 0,
	Success,
	Failure,
	Invoked,
	BubbleWarp,
	Text,
};
consteval bool isArithmeticEnum(MenuSoundEffect) { return true; }

extern SoundBankRef MenuSounds;
extern SoundBankRef GameSounds;

/* Constants for DoInput */
enum class MenuSoundFlags : uint16_t
{
	Up = 1 << 0,
	Down = 1 << 1,
	Left = 1 << 2,
	Right = 1 << 3,
	Select = 1 << 4,
	Cancel = 1 << 5,
	Special = 1 << 6,
	PageUp = 1 << 7,
	PageDown = 1 << 8,
	Delete = 1 << 9,
	Arrows = Up | Down | Left | Right,
	Page = PageUp | PageDown,
	Action = Select | Cancel,
	None = 0,
};
ENUM_BITWISE_OPS(MenuSoundFlags)

extern void SetMenuSounds(MenuSoundFlags sound_0,
						  MenuSoundFlags sound_1);
extern void GetMenuSounds(MenuSoundFlags* sound_0,
						  MenuSoundFlags* sound_1);

extern void PlaySound(SoundRef S, SoundPosition Pos,
					  Element* PositionalObject, uint8_t Priority);
extern void PlayMenuSound(MenuSoundEffect S);
extern void ProcessSound(SoundRef Sound, Element* PositionalObject);
// Removes the queued sounds, and ignores new sounds until FlushSounds().
extern void SuppressSounds(void);
extern SoundPosition CalcSoundPosition(Element* ElementPtr);
extern SoundPosition NotPositional(void);
extern void UpdateSoundPositions(void);
extern void FlushSounds(void);
extern void RemoveSoundsForObject(Element* PosObj);

} // namespace uqm

#endif // UQM_SOUNDS_H
