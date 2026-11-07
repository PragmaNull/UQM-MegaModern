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

#ifndef UQM_COMM_TALKPET_STRINGS_H
#define UQM_COMM_TALKPET_STRINGS_H

#include "core/types/enum.h"

enum class TalkingPetPhrase
{
	NullPhrase,
	HelloAtUmgah,
	WhatAreYou,
	JustTalkingPet,
	TalkingPetsDumb,
	OhNoYouDont,
	WhatDoToUmgah,
	DidNothing,
	UmgahZombies,
	WorksLikeThis,
	WeAreVindicator,
	GoodForYou,
	MustExplainPresence,
	ExplainNothingMonkeyBoy,
	ByeAtUmgah,
	GoodbyeAtUmgah,
	HypnotizeAgain1,
	HypnotizeAgain2,
	HypnotizeAgain3,
	HypnotizeAgain4,
	HypnoTail,
	CantCompel,
	LetsMakeADeal,
	WhatKindOfDeal,
	HelpDefeatUrquan,
	OkLetsDoIt,
	ComingAboard,
	HowTrust,
	Trust,
	BonelessDweeb,
	YourBonelessDweeb,
	WhatAreYouReally,
	PoorDnyarri,
	HardToBelieve,
	ItsTrue,
	Bullshit,
	WorthATry,
	KillYou,
	PleaseDont,
	MustKill,
	DontKill,
	WantKill1,
	WantKill2,
	WantKill3,
	GladYouWontKill,
	WhatsUpOnboard,
	GeneralInfoOnboard1,
	GeneralInfoOnboard2,
	GeneralInfoOnboard3,
	GeneralInfoOnboard4,
	GeneralInfoOnboard5,
	GeneralInfoOnboard6,
	GeneralInfoOnboard7,
	GeneralInfoOnboard8,
	HelloAsDevice1,
	HelloAsDevice2,
	HelloAsDevice3,
	HelloAsDevice4,
	HelloAsDevice5,
	HelloAsDevice6,
	HelloAsDevice7,
	HelloAsDevice8,
	CyborgPepTalk,
	HumanPepTalk,
	ISenseMySlaves,
	HaventGotEverything,
	NeedBomb,
	SoupUpBomb,
	SoupUpFleet,
	SoupUpFlagship,
	ComebackWhenReady,
	WhatNow,
	DoThis,
	CompelUrquan,
	HereWeGo,
	ImScared,
	StupidFop,
	CompelThatShip,
	SavingMyPower,
	AnySuggestions,
	Suggestion1,
	Suggestion2,
	Suggestion3,
	Suggestion4,
	Suggestion5,
	Suggestion6,
	Suggestion7,
	Suggestion8,
	AboutYourRace,
	WhatAboutRace,
	YouLied,
	SoWhat,
	ByeOnboard,
	GoodbyeOnboard,
	WhatAboutPhysiology,
	NoTalkAboutSelf,
	WhatAboutPowers,
	NotPowersButFlowers,
	YesFlowers,
	GoodHuman,
	WishToGoNow,
	ExcellentIdea,
	WhatAboutYourHistory,
	AboutHistory,
	SentientMilieu,
	AboutSentientMilieu,
	WhatAboutWar,
	AboutWar,
	EnoughInfo,
	OkEnoughInfo,
	UmgahAllGone,
	HelloAfterCompelUrquan,
	OutTakes,
};
template <>
inline constexpr bool IsArithmeticEnum<TalkingPetPhrase> = true;

#endif /* UQM_COMM_TALKPET_STRINGS_H */
