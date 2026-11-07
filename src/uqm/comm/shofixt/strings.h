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
#ifndef UQM_COMM_SHOFIXT_STRINGS_H
#define UQM_COMM_SHOFIXT_STRINGS_H

#include "core/types/enum.h"

enum class ShofixtiPhrase
{
	NullPhrase,
	Name1,
	Name2,
	Name3,
	Name4,
	Tanaka,
	Katana,
	HostileKatana1,
	HostileKatana2,
	HostileKatana3,
	HostileKatana4,
	HostileTanaka1,
	HostileTanaka2,
	HostileTanaka3,
	HostileTanaka4,
	HostileTanaka5,
	HostileTanaka6,
	HostileTanaka7,
	HostileTanaka8,
	DontAttack,
	TypicalPloy,
	HeyStop,
	OnlyStop,
	LookYouAre,
	TooBad,
	DontKnow,
	Never,
	Look0,
	Look1,
	ForYou,
	NoBloodshed,
	YesBloodshed,
	DontWantToFight,
	MustFightYouUrquan1,
	MustFightYouUrquan2,
	MustFightYouUrquan3,
	MustFightYouUrquan4,
	NoOneInsults,
	YouLimp,
	MightyWords,
	HangYour,
	DonkeyBreath,
	Dgrunti,
	IAmCaptain,
	IAmNice,
	IAmGuy,
	SoSorry,
	MustUnderstand,
	NiceButWhatIsDonkey,
	IsDefeatTrue,
	YesAndNo,
	ButtBlasted,
	Clobbered,
	VerySadKillSelf,
	ImportantDuty,
	WhatDuty,
	NeedYouForDuty,
	OkWillBeSentry,
	DontDoIt,
	YesIDoIt,
	GoAhead,
	OnSecondThought,
	ProcreatingWildly,
	ReplenishingYourSpecies,
	HopeYouHave,
	SoundsGreatButHow,
	Females,
	Nubiles,
	RatBabes,
	LeapingHappiness,
	Bye0,
	Bye1,
	Goodbye0,
	Goodbye1,
	WhyHere0,
	WhyHere1,
	IGuard,
	WhereWorld,
	BlewItUp,
	HowSurvive,
	NotHere,
	WhatHappened,
	MetVux,
	GloryDevice,
	SwitchBroke,
	Bye,
	Goodbye,
	FriendlyHello,
	Report0,
	Report1,
	NothingNew,
	OutTakes,
};
consteval bool isArithmeticEnum(ShofixtiPhrase) { return true; }

#endif // UQM_COMM_SHOFIXT_STRINGS_H
