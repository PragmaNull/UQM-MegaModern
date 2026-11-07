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
#ifndef UQM_COMM_ILWRATH_STRINGS_H
#define UQM_COMM_ILWRATH_STRINGS_H

#include "core/types/enum.h"

namespace uqm
{
enum class IlwrathPhrase
{
	NullPhrase,
	NeverEnough,
	OkWarship,
	OkDwe,
	OkYouboo,
	OkDilrat,
	BigFun,
	FastAsCan,
	GloriousWorship,
	OnWay,
	GodsReturn1,
	GodsReturn2,
	GodsReturn3,
	JustGrunts,
	GruntsAgain,
	WhatOrders,
	WeWorship1,
	WeWorship2,
	WeWorship3,
	SubsequentChmmrHello,
	InitChmmrHello,
	OkEnoughIlwrath,
	OkEnoughGods,
	SendMessage,
	CameFrom,
	WhoBlastsWho,
	NoSurrender,
	NotReasonable,
	SubsequentHomeHello,
	GeneralInfo,
	GoodbyeAndDie,
	Deceivers,
	NoPeace,
	NoAlliance,
	IlwrathBelieve,
	OkKillThraddash,
	GoodbyeGods,
	InitHelloSpace,
	SubsequentHelloSpace1,
	SubsequentHelloSpace2,
	SubsequentHelloSpace3,
	SubsequentHelloSpace4,
	GeneralInfoSpace1,
	GeneralInfoSpace2,
	GeneralInfoSpace3,
	GeneralInfoSpace4,
	GeneralInfoSpace5,
	StrengthNotAll,
	NoSlayByThousands,
	NoEaseUp,
	GoodbyeAndDieSpace,
	InitHomeHello,
	GoodbyeAndDieHomeworld,
	SoMuchToKnow,
	LongAgo,
	KilledGoodGods,
	Channel44,
	Because44,
	WhatAboutIlwrath,
	AboutPhysio,
	AboutHistory,
	AboutCulture,
	AboutUrquan,
	UrquanTooNice,
	OfCourseWereEvil,
	DontConfuseUs,
	OnWayToThraddash,
	HappyFightingThraddash,

	SayWarship,
	SayDwe,
	SayYouboo,
	SayDillrat,
	EnoughOrders,
	OtherDivineOrders,
	WorshipUs,
	ByeGods,
	EnoughIlwrath,
	EnoughGods,
	WhereYouComeFrom,
	ItWillBeAPleasure,
	BeReasonable,
	Surrender,
	WhatsUp,
	Bye,
	WantPeace,
	WantAlliance,
	GoKillThraddash,
	WhatsUpSpace1,
	WhatsUpSpace2,
	WhatsUpSpace3,
	WhatsUpSpace4,
	WhatsUpSpace5,
	YouAreWeak,
	SlayByThousands,
	EaseUp,
	ByeSpace,
	ByeHomeworld,
	WantInfoOnGods,
	WhenStartWorship,
	AnyGoodGods,
	HowTalkWithGods,
	Why44,
	WantInfoOnIlwrath,
	WhatAboutPhysio,
	WhatAboutHistory,
	WhatAboutCulture,
	WhatAboutUrquan,
	AreYouEvil,
	ButEvilIsDefined,
	Gtfo,
};
consteval bool isArithmeticEnum(IlwrathPhrase) { return true; }

} // namespace uqm

#endif // UQM_COMM_ILWRATH_STRINGS_H
