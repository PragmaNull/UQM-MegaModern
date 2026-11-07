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

#ifndef UQM_COMM_VUX_STRINGS_H
#define UQM_COMM_VUX_STRINGS_H

#include "core/types/enum.h"

enum class VuxPhrase
{
	NullPhrase,
	ZexHello1,
	ZexHello2,
	ZexHello3,
	ZexHello4,
	FightOrTrade1,
	FightOrTrade2,
	WhatYouDoHere,
	MyMenagerie,
	WhatAboutMenagerie,
	NeedNewCreature,
	WhatAboutCreature,
	AboutCreature,
	AboutCreatureAgain,
	CreatureAgain,
	IHaveBeast,
	GiveBeast,
	OkTakeBeast,
	FoolAiee0,
	FoolAiee1,
	WhyTrust1,
	Trust1,
	WhyTrust2,
	Trust2,
	WhyTrust3,
	Trust3,
	WhyDontYouAttack,
	LikeYou,
	WhyLikeMe,
	LikeBecause,
	AreYouAPervert,
	CallMeWhatYouWish,
	TakeByForce,
	PrecursorDevice,
	Regardless,
	ThenFight,
	YouLied,
	YupLied,
	KillYou,
	FightAgain,
	ByeZex,
	GoodbyeZex,
	HomeworldHello1,
	HomeworldHello2,
	HomeworldHello3,
	HomeworldHello4,
	SpaceHello1,
	SpaceHello2,
	SpaceHello3,
	SpaceHello4,
	KillYouSquids1,
	KillYouSquids2,
	KillYouSquids3,
	KillYouSquids4,
	WeFight,
	WhySoMean,
	UrquanSlaves,
	DeeperReason,
	OldInsult,
	IfWeApologize,
	ProbablyNot,
	TryAnyWay,
	Nope,
	ApologizeInSpace,
	Apology1,
	NotAccepted1,
	Apology2,
	NotAccepted2,
	Apology3,
	NotAccepted3,
	Apology4,
	NotAccepted4,
	Apology5,
	NotAccepted5,
	Apology6,
	NotAccepted6,
	Apology7,
	NotAccepted7,
	Apology8,
	NotAccepted8,
	Apology9,
	NotAccepted9,
	Apology10,
	Truth,
	WhatsUpHostile,
	GeneralInfoHostile1,
	GeneralInfoHostile2,
	GeneralInfoHostile3,
	GeneralInfoHostile4,
	CantWeBeFriends1,
	NeverUglyHumans1,
	CantWeBeFriends2,
	NeverUglyHumans2,
	CantWeBeFriends3,
	NeverUglyHumans3,
	CantWeBeFriends4,
	NeverUglyHumans4,
	ByeHostileSpace,
	GoodbyeAndDieHostileSpace1,
	GoodbyeAndDieHostileSpace2,
	GoodbyeAndDieHostileSpace3,
	GoodbyeAndDieHostileSpace4,
	OutTakes,
};
template <>
inline constexpr bool IsArithmeticEnum<VuxPhrase> = true;

#endif /* UQM_COMM_VUX_STRINGS_H */
