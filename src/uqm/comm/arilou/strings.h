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

#ifndef UQM_COMM_ARILOU_STRINGS_H
#define UQM_COMM_ARILOU_STRINGS_H

#include "core/types/enum.h"

enum class ArilouPhrase
{
	NullPhrase,
	InitHello,
	ConfusedByHello,
	ConfusedResponse,
	HappyByHello,
	HappyResponse,
	MiffedByHello,
	MiffedResponse,
	FriendlySpaceHello1,
	FriendlySpaceHello2,
	FriendlySpaceHello3,
	FriendlySpaceHello4,
	FrdlyHomeworldHello1,
	FrdlyHomeworldHello2,
	FrdlyHomeworldHello3,
	FrdlyHomeworldHello4,
	WhatsUp1,
	WhatsUp2,
	GeneralInfo1,
	GeneralInfo2,
	GeneralInfo3,
	GeneralInfo4,
	WhyYouHere,
	LearnThings,
	WhatThings,
	TheseThings,
	WhyDoIt,
	DoItBecause,
	GiveMeInfo1,
	ArilouHints1,
	GiveMeInfo2,
	ArilouHints2,
	ArilouHints3,
	ArilouHints4,
	ByeFriendlySpace,
	GoodbyeFriendlySpace,
	GotPartYet1,
	GotPartYet2,
	InitAngryHwldHello,
	InvadersFromMars,
	HadOurReasons,
	BugEyedFruitcakes,
	WeNeverFriends,
	OkLetsBeFriends,
	NoAllyButMuchGive,
	WhyShouldITrust,
	TrustBecause,
	WhatAboutInterference,
	InterferenceNecessary,
	IJustLikeToLeave,
	SorryNoLeave,
	WhatAboutWar,
	AboutWar,
	WhatAboutUrquan,
	AboutUrquan,
	BestIfIKilledYou,
	WickedHuman,
	WhatDidOnEarth,
	DidThis,
	WhyDidThis,
	IdfParasites,
	TellMore,
	NotNow,
	UmgahActingWeird,
	LearnedAboutUmgah,
	WellGoCheck,
	NoNewsYet,
	UmgahUnderCompulsion,
	WhatDoNow,
	GoFindOut,
	TellArilouAboutTpet,
	BadNewsAboutTpet,
	WhatDoAboutTpet,
	DangerousButUseful,
	WhatGiveMe,
	AboutPortal,
	WhatAboutTpet,
	AboutTpet,
	AboutPortalAgain,
	PortalAgain,
	GotIt,
	CleverHuman,
	GivePortal,
	ByeFriendlyHomeworld,
	GoodbyeFrdlyHomeworld,
	HostileGoodbye1,
	HostileGoodbye2,
	HostileGoodbye3,
	HostileGoodbye4,
	AngrySpaceHello1,
	AngrySpaceHello2,
	LetsFight,
	NoFight,
	ImSorry,
	ApologizeAtHomeworld,
	ByeAngrySpace,
	GoodbyeAngrySpace,
	OutTakes,
};
template <>
inline constexpr bool IsArithmeticEnum<ArilouPhrase> = true;

#endif /* UQM_COMM_ARILOU_STRINGS_H */
