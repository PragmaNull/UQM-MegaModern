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

#ifndef UQM_COMM_MYCON_STRINGS_H
#define UQM_COMM_MYCON_STRINGS_H

#include "core/types/enum.h"

enum class MyconPhrase
{
	NullPhrase,
	TellUsAboutWorld,
	ByeAndDieHomeworld,
	Ramble1,
	Ramble2,
	Ramble3,
	Ramble4,
	Ramble5,
	Ramble6,
	Ramble7,
	Ramble8,
	Ramble9,
	Ramble10,
	Ramble11,
	Ramble12,
	Ramble13,
	Ramble14,
	Ramble15,
	Ramble16,
	Ramble17,
	Ramble18,
	Ramble19,
	Ramble20,
	Ramble21,
	Ramble22,
	Ramble23,
	Ramble24,
	Ramble25,
	Ramble26,
	Ramble27,
	Ramble28,
	Ramble29,
	Ramble30,
	Ramble31,
	Ramble32,
	Question1,
	Question2,
	Question3,
	Question4,
	Question5,
	Question6,
	Question7,
	Question8,
	Question9,
	Question10,
	Question11,
	Question12,
	Question13,
	Question14,
	Question15,
	Question16,
	ByeSpace,
	ByeAndDieSpace,
	GonnaDie,
	Insult1,
	Insult2,
	Insult3,
	Insult4,
	Insult5,
	Insult6,
	Insult7,
	Insult8,
	ComeInPeace,
	HelloHomeworld1,
	HelloHomeworld2,
	HelloHomeworld3,
	HelloHomeworld4,
	HelloHomeworld5,
	HelloHomeworld6,
	HelloHomeworld7,
	HelloHomeworld8,
	HelloSpace1,
	HelloSpace2,
	HelloSpace3,
	HelloSpace4,
	HelloSpace5,
	HelloSpace6,
	HelloSpace7,
	HelloSpace8,
	LetsBeFriends,
	CameToHomeworld,
	SubmitToUs,
	ByeSunDevice,
	GoodbyeSunDevice,
	Response1,
	Response2,
	Response3,
	Clue1,
	Clue2,
	Clue3,
	WhatAboutShattered,
	AboutShattered,
	HelloSunDeviceWorld1,
	HelloSunDeviceWorld2,
	WhatsUpSunDevice,
	GeneralInfoSunDevice,
	LikeToLand,
	NeverLetLand,
	ByeHomeworld,
	IHaveACunningPlan,
	DieLiar,
	HowGoesImplanting,
	UnforseenDelays,
	DieThief,
	DieThiefAgain,
	GoodbyeAndDie,
	AmbushTail,
	RambleTail,
	WeGoToImplant,
	WontFallForTrick,
};
template <>
inline constexpr bool IsArithmeticEnum<MyconPhrase> = true;

#endif /* UQM_COMM_MYCON_STRINGS_H */
