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
#ifndef UQM_COMM_ORZ_STRINGS_H
#define UQM_COMM_ORZ_STRINGS_H

#include "core/types/enum.h"

enum class OrzPhrase
{
	NullPhrase,
	InitHello,
	WhoYou,
	WeAreOrz,
	WhyHere,
	HereBecause,
	AlliedHomeworldHello1,
	AlliedHomeworldHello2,
	AlliedHomeworldHello3,
	AlliedHomeworldHello4,
	AlliedSpaceHello1,
	AlliedSpaceHello2,
	AlliedSpaceHello3,
	AlliedSpaceHello4,
	WhatsUpAlly,
	GeneralInfoAlly1,
	GeneralInfoAlly2,
	GeneralInfoAlly3,
	GeneralInfoAlly4,
	MoreAboutYou,
	AboutUs1,
	AboutUs2,
	AboutUs3,
	AboutUs4,
	WhereAndrosyn,
	DisembleAboutAndrosyn,
	MustKnowAboutAndrosyn,
	KnowTooMuch,
	DontReallyCare,
	YouAreOurFriends,
	AboutAndro1,
	ForgetAndro1,
	AboutAndro2,
	ForgetAndro2,
	AboutAndro3,
	BlewIt,
	NeutralHomeworldHello1,
	NeutralHomeworldHello2,
	NeutralHomeworldHello3,
	NeutralHomeworldHello4,
	NeutralSpaceHello1,
	NeutralSpaceHello2,
	NeutralSpaceHello3,
	NeutralSpaceHello4,
	Hostile1,
	HostilityIsBad1,
	Hostile2,
	HostilityIsBad2,
	WeAreVindicator,
	NiceToMeetYou,
	SeemLikeNiceGuys,
	AreNiceWantAlly,
	TalkAboutAlliance,
	OkTalkAlliance,
	YesAlliance,
	Great,
	NoAlliance,
	MaybeLater,
	DecideLater,
	OkLater,
	WhySoTrusting,
	TrustingBecause,
	ByeNeutral,
	GoodbyeNeutral,
	AngrySpaceHello1,
	AngrySpaceHello2,
	AngryHomeworldHello1,
	AngryHomeworldHello2,
	WhatsUpAngry,
	GeneralInfoAngry1,
	GeneralInfoAngry2,
	WereSorry,
	ApologyAccepted,
	Insult1,
	Insult2,
	Insult3,
	Insult4,
	Insult5,
	Insult6,
	Insult7,
	Insult8,
	Insulted1,
	Insulted2,
	Insulted3,
	Insulted4,
	ByeAngry,
	GoodbyeAngry,
	AngryTaaloHello1,
	AngryTaaloHello2,
	FriendlyAlliedTaaloHello1,
	FriendlyAlliedTaaloHello2,
	DemandToLand,
	NoDemand,
	AskNicely,
	WhyYouHere,
	AngryExplanation,
	FriendlyExplanation,
	WhatIsThisPlace,
	FriendlyPlace,
	AngryPlace,
	MayWeLand,
	SureLand,
	AlliesCanVisit,
	MakeAlliance,
	CantAllyHere,
	WhyBusy,
	BusyBecause,
	ByeTaalo,
	ByeAlly,
	GoodbyeAlly,
	FriendlyTaaloGoodbye,
	AngryTaaloGoodbye,
	HostileHello1,
	HostileHello2,
	OutTakes,
};
template <>
inline constexpr bool IsArithmeticEnum<OrzPhrase> = true;

#endif // UQM_COMM_ORZ_STRINGS_H
