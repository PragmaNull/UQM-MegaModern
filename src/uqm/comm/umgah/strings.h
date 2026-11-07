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
#ifndef UQM_COMM_UMGAH_STRINGS_H
#define UQM_COMM_UMGAH_STRINGS_H

#include "core/types/enum.h"

namespace uqm
{
enum class UmgahPhrase
{
	NullPhrase,
	HwldPreZombieHello1,
	HwldPreZombieHello2,
	HwldPreZombieHello3,
	HwldPreZombieHello4,
	SpacePreZombieHello1,
	SpacePreZombieHello2,
	SpacePreZombieHello3,
	SpacePreZombieHello4,
	UnknownZombieHello1,
	UnknownZombieHello2,
	UnknownZombieHello3,
	UnknownZombieHello4,
	DestroyInterferer1,
	DestroyInterferer2,
	DestroyInterferer3,
	DestroyInterferer4,
	RevealedZombieHello1,
	RevealedZombieHello2,
	RevealedZombieHello3,
	RevealedZombieHello4,
	HostileHello1,
	HostileHello2,
	HostileHello3,
	HostileHello4,
	RewardAtHomeworld1,
	RewardAtHomeworld2,
	PostZombieHwldHello,
	OweMeBigTime,
	OurLargesse,
	GiveLifedata,
	Thanks,
	WhatDoWithTpet,
	TrickUrquan,
	AnyJokes,
	Sure,
	WhatBeforeTpet,
	TrkdSpathiAndIlwrath,
	WhereCaster,
	SpathiTookThem,
	SoWhatForNow,
	DoThisNow,
	ByePostZombie,
	FunnyIdea,
	WhatsUpPreZombie,
	GeneralInfoPreZombie,
	EvilBlobbiesGiveUp,
	NotEvilBlobbies,
	EvilBlobbiesMustDie,
	OhNoWeWont,
	CanWeBeFriends,
	SureFriends,
	WantToDefeatUrquan,
	FineByUs,
	ByePreZombie,
	GoodbyePreZombie,
	Threat,
	NoThreat,
	WhatsUpZombies,
	GeneralInfoZombie,
	HowGoesTpet,
	WhatTpet,
	YouToldUs,
	SadlyItDied,
	DontBelieve,
	ThenDie,
	ByeUnknown,
	GoodbyeUnknown,
	EvilBlobbies,
	YesVeryEvil,
	GiveUpOrDie,
	NotGiveUp,
	WeVindicator,
	GoodForYou1,
	ComeInPeace,
	GoodForYou2,
	KnowAnyJokes,
	Joke1,
	BetterJoke,
	Joke2,
	NotVeryFunny,
	YesWeAre,
	WhatAboutTpet,
	ArilouToldUs,
	ByeZombie,
	GoodbyeZombie,
	OutTakes,
};
consteval bool isArithmeticEnum(UmgahPhrase) { return true; }

} // namespace uqm

#endif // UQM_COMM_UMGAH_STRINGS_H
