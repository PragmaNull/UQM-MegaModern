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
#ifndef UQM_COMM_CHMMR_STRINGS_H
#define UQM_COMM_CHMMR_STRINGS_H

#include "core/types/enum.h"

namespace uqm
{
enum class ChmmrPhrase
{
	NullPhrase,
	WhyYouHere1,
	WhyYouHere2,
	WhyYouHere3,
	WhyYouHere4,
	FindOutWhatsUp,
	HybridProcess,
	NeedHelp,
	CantHelp,
	WhyNoHelp,
	LongTime,
	WhatIfMoreEnergy,
	DangerToUs,
	NeedAdvice,
	WhatAdvice,
	HowDefeatUrquan,
	DefeatLikeSo,
	WhatAboutTpet,
	ScaryButUseful,
	WhatAboutBomb,
	AboutBomb,
	WhatAboutSunDevice,
	AboutSunDevice,
	WhatAboutSamatra,
	AboutSamatra,
	EnoughAdvice,
	OkEnoughAdvice,
	ByeShielded,
	GoodbyeShielded,
	WeAreFree,
	WhoAreYou,
	IAmCaptain,
	IAmSavior,
	IAmSilly,
	WhyHaveYouFreedUs,
	Serious1,
	Serious2,
	Silly,
	WillHelpAnalyzeLogs,
	YouKnowSamatra,
	DontKnowAboutSamatra,
	NeedDistraction,
	HaveTalkingPet,
	NeedWeapon,
	HaveBomb,
	ReturnWhenReady,
	YouAreReady,
	FurtherAssistance,
	NoFurtherAssistance,
	TechHelp,
	UseOurShipsBefore,
	WhereWeapon,
	PrecursorWeapon,
	WhereDistraction,
	PsychicWeaponry,
	WhatNow,
	WeWillImproveBomb,
	ModifyVessel,
	WontHurtMyShip,
	WillDestroyIt,
	BummerAboutMyShip,
	DeadSilence,
	OtherAssistance,
	UseOurShipsAfter,
	Proceed,
	Take2Weeks,
	HelloAfterBomb1,
	HelloAfterBomb2,
	WhatsUpAfterBomb,
	GeneralInfoAfterBomb1,
	GeneralInfoAfterBomb2,
	WhatDoAfterBomb,
	DoAfterBomb,
	ByeAfterBomb,
	GoodbyeAfterBomb,
	Bye,
	Goodbye,
	PerhapsNotInstall,
	Yes,
};
consteval bool isArithmeticEnum(ChmmrPhrase) { return true; }

} // namespace uqm

#endif // UQM_COMM_CHMMR_STRINGS_H
