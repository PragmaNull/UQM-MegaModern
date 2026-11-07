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
#ifndef UQM_COMM_URQUAN_STRINGS_H
#define UQM_COMM_URQUAN_STRINGS_H

#include "core/types/enum.h"

enum class UrquanPhrase
{
	NullPhrase,
	HelloSamatra,
	SenseEvil,
	InitUrquanWakeUp,
	WhereAmI,
	YouAreHere,
	WhyDoesMyHeadHurt,
	HurtsBecause,
	WhatAbout2Weeks,
	About2Weeks,
	Compulsion,
	WhatCompulsion,
	WascallyLittleGuy,
	WhatItLookLike,
	TerranAmphibian,
	TalkingPetOnSteroids,
	BadNews,
	TurdAndToad,
	WhatIsTurdAndToad,
	AlienMindControl,
	WhatFeltLike,
	PossessedByDevil,
	StupidDevil,
	FallingAsleep,
	SomeoneElseControlled,
	SoundsFamiliar,
	BeforeCoffee,
	Explain,
	WhyExplain,
	MustExplain,
	ByeInitHypno,
	GoodbyeAndDieInitHypno,
	SubsequentUrquanWakeUp,
	UhOh,
	NoUhOh,
	StopMeeting,
	NoStopMeeting,
	ByeSubHypno,
	GoodbyeAndDieSubHypno,
	CaughtYa,
	InitFleeHuman,
	SubsequentFleeHuman,
	WhyFlee,
	FleeBecause,
	WhatHappensNow,
	HappensNow,
	WhatAboutYou,
	AboutUs,
	ByeWarsOver,
	GoodbyeWarsOver,
	SendMessage,
	InitHello,
	SubsequentHello1,
	SubsequentHello2,
	SubsequentHello3,
	SubsequentHello4,
	YouMustSurrender,
	Nope,
	ISurrender,
	DisobediencePunished,
	IWontSurrender,
	BadChoice,
	IWillSurrender,
	GoodChoice,
	KeyPhrase,
	UrquanStory,
	LikeToLeave,
	IndependenceIsBad,
	WhatsUp1,
	GeneralInfo1,
	WhatsUp2,
	GeneralInfo2,
	WhatsUp3,
	GeneralInfo3,
	WhatsUp4,
	GeneralInfo4,
	OutTakes,
};
consteval bool isArithmeticEnum(UrquanPhrase) { return true; }

#endif // UQM_COMM_URQUAN_STRINGS_H
