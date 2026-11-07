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
#ifndef UQM_COMM_UTWIG_STRINGS_H
#define UQM_COMM_UTWIG_STRINGS_H

#include "core/types/enum.h"

enum class UtwigPhrase
{
	NullPhrase,
	NeutralSpaceHello1,
	NeutralSpaceHello2,
	HostileSpaceHello1,
	HostileSpaceHello2,
	BombWorldHello1,
	BombWorldHello2,
	HostileBombHello1,
	HostileBombHello2,
	NeutralHomeworldHello1,
	NeutralHomeworldHello2,
	NeutralHomeworldHello3,
	NeutralHomeworldHello4,
	HostileHomeworldHello1,
	HostileHomeworldHello2,
	WhyYouHere,
	WeGuardBomb,
	WhatAboutBomb,
	AboutBomb,
	GiveUsBombOrDie,
	GuardsWarn,
	DemandBomb,
	GuardsFight,
	MayWeHaveBomb,
	NoBomb,
	Please,
	SorryNoBomb,
	WhatsUpBomb,
	GeneralInfoBomb1,
	GeneralInfoBomb2,
	ByeBomb,
	GoodbyeBomb,
	HeyWaitGotUltron,
	TauntUsButWeLook,
	TrickedUs1,
	TrickedUs2,
	WeAreVindicator,
	WouldBeHappyBut,
	WhySad,
	UltronBroke,
	WhatUltron,
	GloriousUltron,
	DontBeBabies,
	MockOurPain,
	RealSorryAboutUltron,
	AppreciateSympathy,
	WhatAboutYou1,
	AboutUs1,
	WhatAboutYou2,
	AboutUs2,
	WhatAboutYou3,
	AboutUs3,
	WhatAboutUrquan1,
	AboutUrquan1,
	WhatAboutUrquan2,
	AboutUrquan2,
	GotUltron,
	DontWantToLook,
	SickTrick1,
	SickTrick2,
	ByeNeutral,
	GoodbyeNeutral,
	TooLate,
	Name1,
	Name2,
	Name3,
	Name4,
	HappyDays,
	OkAttackKohrah,
	WhatsUpAfterSpace,
	GeneralInfoAfterSpace1,
	GeneralInfoAfterSpace2,
	WhatNowAfterSpace,
	DoThisAfterSpace,
	ByeAfterSpace,
	GoodbyeAfterSpace,
	WhatsUpBeforeSpace,
	GeneralInfoBeforeSpace1,
	GeneralInfoBeforeSpace2,
	WhatNowBeforeSpace,
	DoThisBeforeSpace,
	ByeBeforeSpace,
	GoodbyeBeforeSpace,
	HowWentWar,
	AboutBattle,
	HowGoesWar,
	BattleHappens1,
	BattleHappens2,
	FleetOnWay,
	LearnNewInfo,
	NoNewInfo,
	Samatra,
	WhatNowHomeworld,
	HopeKillEachOther,
	HowIsUltron,
	UltronIsGreat,
	ByeAlliedHomeworld,
	GoodbyeAlliedHomeworld,
	AlliedHomeworldHello1,
	AlliedHomeworldHello2,
	AlliedHomeworldHello3,
	AlliedHomeworldHello4,
	HelloBeforeKohrahSpace1,
	HelloBeforeKohrahSpace2,
	HelloDuringKohrahSpace1,
	HelloDuringKohrahSpace2,
	HelloAfterKohrahSpace1,
	HelloAfterKohrahSpace2,
	UpToYou,
	CanYouHelp,
	HowHelp,
	DontNeed,
	Have4Ships,
	NoUltronAtBomb,
	OutTakes,
};
consteval bool isArithmeticEnum(UtwigPhrase) { return true; }

#endif // UQM_COMM_UTWIG_STRINGS_H
