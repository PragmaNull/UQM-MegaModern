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
#ifndef UQM_COMM_BLACKUR_STRINGS_H
#define UQM_COMM_BLACKUR_STRINGS_H

#include "core/types/enum.h"

enum class KohrAhPhrase
{
	NullPhrase,
	SenseEvil,
	HelloAndDie1,
	HelloAndDie2,
	HelloAndDie3,
	HelloAndDie4,
	HelloAndDie5,
	HelloAndDie6,
	HelloAndDie7,
	HelloAndDie8,
	HelloSamatra,
	WeKillAll1,
	WeKillAll2,
	WeKillAll3,
	WeKillAll4,
	WhyKillAll1,
	WhyKillAll2,
	WhyKillAll3,
	WhyKillAll4,
	KillBecause1,
	KillBecause2,
	KillBecause3,
	KillBecause4,
	PleaseDontKill1,
	WillKill1,
	PleaseDontKill2,
	WillKill2,
	PleaseDontKill3,
	WillKill3,
	PleaseDontKill4,
	WillKill4,
	ByeFrenzy1,
	ByeFrenzy2,
	ByeFrenzy3,
	ByeFrenzy4,
	GoodbyeAndDieFrenzy1,
	GoodbyeAndDieFrenzy2,
	GoodbyeAndDieFrenzy3,
	GoodbyeAndDieFrenzy4,
	Threat1,
	ResistanceIsUseless1,
	Threat2,
	ResistanceIsUseless2,
	Threat3,
	ResistanceIsUseless3,
	Threat4,
	ResistanceIsUseless4,
	KeyPhrase,
	ResponseToKeyPhrase,
	WhyDoYouDestroy,
	WeWereSlaves,
	RelationshipWithUrquan,
	WeAreUrquanToo,
	WhatAboutCulture,
	BoneGardens,
	HowLeaveMeAlone,
	YouDie,
	GuessThatsAll,
	ThenDie,
	WhatAreYouHoveringOver,
	BonePile,
	YouSureAreCreepy,
	YesCreepy,
	StopThatGrossBlinking,
	DieHuman,
	Plead1,
	PleadingIsUseless1,
	Plead2,
	PleadingIsUseless2,
	Plead3,
	PleadingIsUseless3,
	Plead4,
	PleadingIsUseless4,
	Bye,
	GoodbyeAndDie,
	GameOverDude,
	OutTakes,
};
template <>
inline constexpr bool IsArithmeticEnum<KohrAhPhrase> = true;

#endif // UQM_COMM_BLACKUR_STRINGS_H
