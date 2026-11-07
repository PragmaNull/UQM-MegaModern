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
#ifndef UQM_COMM_SLYHOME_STRINGS_H
#define UQM_COMM_SLYHOME_STRINGS_H

#include "core/types/enum.h"

namespace uqm
{
enum class SlylandroHomePhrase
{
	NullPhrase,
	Hello1,
	Hello2,
	Hello3,
	Hello4,
	RecallProgram1,
	WeAreUs,
	TerriblyExciting,
	HappyToTellMore,
	TellMore,
	WouldYouLikeToKnowMore,
	YesTellMore,
	WeComeFromEarth,
	OkEarth,
	WeExplore,
	OkExplore,
	WeFightUrquan,
	UrquanNiceGuys,
	NotSameUrquan,
	PersonalityChange,
	WeGather,
	MaybeInterested,
	EnoughAboutMe,
	OkEnoughYou,
	WhatOtherVisitors,
	Visitors,
	AnyOtherVisitors,
	LongAgo,
	WhatAboutSentientMilieu,
	MetTaaloTheyAreFrom,
	WhoElse,
	Precursors,
	PrecursorsYow,
	AboutPrecursors,
	MustKnowMore,
	AllWeKnow,
	WhoAreYou,
	WeAreSly,
	LikeMoreAboutYou,
	SureKnowWhat,
	WhatAboutHome,
	AboutHome,
	WhatAboutCulture,
	AboutCulture,
	WhatAboutHistory,
	AboutHistory,
	WhatAboutBiology,
	AboutBiology,
	EnoughInfo,
	OkEnoughInfo,
	WhereAreYou,
	DownHere,
	ThatsImpossible1,
	NoItsNot1,
	ThatsImpossible2,
	NoItsNot2,
	Bye,
	Goodbye1,
	Goodbye2,
	WhatAreProbes,
	ProbesAre,
	KnowMoreProbe,
	OkWhatMoreProbe,
	WhereProbesFrom,
	ProbesFromMelnorme,
	WhySell,
	SellForInfo,
	HowLongAgo,
	FiftyThousandRotations,
	WhatsProbesMission,
	SeekOutNewLife,
	IfOnlyOne,
	TheyReplicate,
	EnoughProbe,
	OkEnoughProbe,
	WhyProbeAlwaysAttack,
	OnlyDefend,
	TalkMoreProbeAttack,
	NoProblemButSure,
	TellMeAboutBasics,
	BasicCommands,
	TellBasicsAgain,
	OkBasicsAgain,
	WhatEffect,
	AffectsBehavior,
	HowDoesProbeDefend,
	OnlySelfDefense,
	CombatBehavior,
	MissileBatteries,
	WhatMissileBatteries,
	LightningOnlyForHarvesting,
	TellMeAboutRep1,
	AboutRep,
	WhatSetPriority,
	Maximum,
	EnoughProblem,
	OkEnoughProblem,
	ProbeHasBug,
	NoItDoesnt,
	TellMeAboutAttack,
	AttackNoProblem,
	TellMeAboutRep2,
	RepNoProblem,
	WhatAboutRepPriorities,
	MaximumSoWhat,
	ThinkAboutRepPriorities,
	UhOh,
	HuntThemDown,
	GrowTooFast,
	SueMelnorme,
	SignedWaiver,
	RecallSignal,
	NotThisModel,
	MegaSelfDestruct,
	WhyYesThereIs,
};
consteval bool isArithmeticEnum(SlylandroHomePhrase) { return true; }

} // namespace uqm

#endif // UQM_COMM_SLYHOME_STRINGS_H
