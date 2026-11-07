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
#ifndef UQM_COMM_SLYLAND_STRINGS_H
#define UQM_COMM_SLYLAND_STRINGS_H

#include "core/types/enum.h"

namespace uqm
{
enum class SlylandroPhrase
{
	NullPhrase,
	WeComeInPeace1,
	WeComeInPeace2,
	WeComeInPeace3,
	WeComeInPeace4,
	WeComeInPeace5,
	WeComeInPeace6,
	WeComeInPeace7,
	WeComeInPeace8,
	Threat1,
	Threat2,
	Threat3,
	Threat4,
	ProgrammedToDefend1,
	ProgrammedToDefend2,
	ProgrammedToDefend3,
	ProgrammedToDefend4,
	SomethingWrong1,
	SomethingWrong2,
	SomethingWrong3,
	SomethingWrong4,
	NominalFunction1,
	NominalFunction2,
	NominalFunction3,
	NominalFunction4,
	WeAreUs1,
	WeAreUs2,
	WeAreUs3,
	WeAreUs4,
	ThisIsProbe1,
	ThisIsProbe2,
	ThisIsProbe3,
	ThisIsProbe40,
	ThisIsProbe41,
	ThisIsProbe42,
	WhyAttack1,
	WhyAttack2,
	WhyAttack3,
	WhyAttack4,
	PeacefulMission1,
	PeacefulMission2,
	PeacefulMission3,
	PeacefulMission4,
	Bye1,
	Bye2,
	Bye3,
	Bye4,
	Goodbye1,
	Goodbye2,
	Goodbye3,
	Goodbye4,
	Hostile,
	DestructSequence,
	DestructCode,
	CoordPlus,
	CoordMinus,
	CoordPoint,
	EnumerateHundred,
	EnumerateThousand,
	EnumerateZero,
	EnumerateOne,
	EnumerateTwo,
	EnumerateThree,
	EnumerateFour,
	EnumerateFive,
	EnumerateSix,
	EnumerateSeven,
	EnumerateEight,
	EnumerateNine,
	EnumerateTen,
	EnumerateEleven,
	EnumerateTwelve,
	EnumerateThirteen,
	EnumerateFourteen,
	EnumerateFifteen,
	EnumerateSixteen,
	EnumerateSeventeen,
	EnumerateEighteen,
	EnumerateNineteen,
	EnumerateTwenty,
	EnumerateThirty,
	EnumerateFourty,
	EnumerateFifty,
	EnumerateSixty,
	EnumerateSeventy,
	EnumerateEighty,
	EnumerateNinety,
};
consteval bool isArithmeticEnum(SlylandroPhrase) { return true; }

} // namespace uqm

#endif // UQM_COMM_SLYLAND_STRINGS_H
