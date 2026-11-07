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
#ifndef UQM_COMM_COMANDR_STRINGS_H
#define UQM_COMM_COMANDR_STRINGS_H

#include "core/types/enum.h"

namespace uqm
{
enum class CommanderPhrase
{
	NullPhrase,
	GladWhenYouComeBack,
	GiveFuel,
	GiveFuelAgain,
	AreYouSupplyShip,
	DoYouHaveRadioThisTime,
	HereIsAnotherLander,
	TheWhatFromWhere,
	AboutTime,
	MessageGarbled1,
	MessageGarbled2,
	HereIsANewLander,
	ThisMaySeemSilly,
	OkTheNafs,
	OkTheCan,
	OkTheUfw,
	OkTheNameIsEmpire,
	FuelUp0,
	FuelUp1,
	WhatKindOfIdiot,
	DontKnowWhoYouAre,
	ThatsImpossible,
	AskAway,
	RadiosOnMercury,
	ThanksForHelping,
	StarbaseIs,
	HappenedToEarth,
	UrquanLeft,
	BaseOnMoon,
	AcknowledgeSecret,
	AboutBase,
	GoodLuckWithBase,
	DealtWithBaseYet,
	HereComesIlwrath,
	VeryImpressive,
	ItWasAbandoned,
	YouReallyFoughtBase,
	ImGladYouWon,
	ImSureItWasDifficult,
	ThatWasProbe,
	DeepTrouble,
	GoodNews,
	SureHope,
	AboutBaseAgain,
	CookButts,
	OverthrowAliens,
	KillMonsters,
	GoodLuckAgain,
	StarbaseWillBeReady,

	OverthrowEvilAliens,
	AnnihilateThoseMonsters,
	CookTheirButts,
	WhereGetRadios,
	WellGoGetThemNow,
	WeWillTransferNow,
	WhatWillYouGiveUs,
	BeforeRadiosWeNeedInfo,
	NoButWellHelp,
	YesThisIsSupplyShip,
	WhatSlavePlanet,
	ILied,
	PlumbOut,
	WeAreVindicator,
	FirstGiveInfo,
	WeMustGoNow,
	WhereCanIGetRadios,
	OkIWillGetRadios,
	WhatsThisStarbase,
	WhatAboutEarth,
	WhereAreUrquan,
	OurMissionWasSecret,
	WeAreHereToHelp,
	TellMeAboutBase,
	WeWillTakeCareOfBase,
	TellMeAgain,
	BaseWasAbandoned,
	WeFoughtThem,
	OhYesBigFight,
	ILiedItWasAbandoned,
	ICantTalkAboutIt,
	Name1,
	Name2,
	Name3,
	Name4,
	ILostMyLander,
	ILostAnotherLander,
	NeedFuelMercury,
	NeedFuelLuna,
	NeedFuelAgain,
	WhatWasRedThing,
	ItWentAway,
	WeDestroyedIt,
	WhatProbe,
	TakeCareOfBaseAgain,
	GoodbyeCommander,
};
consteval bool isArithmeticEnum(CommanderPhrase) { return true; }

} // namespace uqm

#endif // UQM_COMM_COMANDR_STRINGS_H
