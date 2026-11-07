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

#ifndef UQM_COMM_SUPOX_STRINGS_H
#define UQM_COMM_SUPOX_STRINGS_H

#include "core/types/enum.h"

enum class SupoxPhrase
{
	NullPhrase,
	NeutralSpaceHello1,
	NeutralSpaceHello2,
	NeutralHomeworldHello1,
	NeutralHomeworldHello2,
	HostileSpaceHello1,
	HostileSpaceHello2,
	AlliedHomeworldHello1,
	AlliedHomeworldHello2,
	AlliedHomeworldHello3,
	AlliedHomeworldHello4,
	IAm,
	WeAreSupox,
	MyShip,
	OurShip,
	FromAlliance,
	FromSupox,
	AreYouCopying,
	YeahSorry,
	WhyCopy,
	Symbiots,
	TellUsOfYourSpecies,
	OurSpecies,
	PlantsArentIntelligent,
	ProvesWereSpecial,
	AnyoneAroundHere,
	UtwigNearby,
	WhatRelationToUtwig,
	UtwigAllies,
	WhatsWrongWithUtwig,
	BrokeUltron,
	WhatsUltron,
	TakeUltron,
	WhatDoIDoNow,
	FixIt,
	ThanksNowWeEatYou,
	HideousMonsters,
	GotFixedUltron,
	GoodGiveToUtwig,
	LookIRepairedLots,
	AlmostThere,
	LookISlightlyRepaired,
	GreatDoMore,
	WhereGetRepairs,
	AncientRhyme,
	ByeNeutral,
	GoodbyeNeutral,
	AboutBattle,
	HelloBeforeKohrahSpace1,
	HelloBeforeKohrahSpace2,
	HelloDuringKohrahSpace1,
	HelloDuringKohrahSpace2,
	HelloAfterKohrahSpace1,
	HelloAfterKohrahSpace2,
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
	HowGoesWar,
	BattleHappens1,
	BattleHappens2,
	FleetOnWay,
	LearnNewInfo,
	NoNewInfo,
	Samatra,
	WhatNowHomeworld,
	HopeKillEachOther,
	UpToYou,
	CanYouHelp,
	HowHelp,
	DontNeed,
	Have4Ships,
	GiveInfo,
	GoodHints,
	HowIsUltron,
	UltronIsGreat,
	ByeAlliedHomeworld,
	GoodbyeAlliedHomeworld,
	Name1,
	Name2,
	Name3,
	Name4,
	OutTakes,
};
template <>
inline constexpr bool IsArithmeticEnum<SupoxPhrase> = true;

#endif /* UQM_COMM_SUPOX_STRINGS_H */
