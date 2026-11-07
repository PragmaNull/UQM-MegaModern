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
#ifndef UQM_COMM_YEHAT_STRINGS_H
#define UQM_COMM_YEHAT_STRINGS_H

#include "core/types/enum.h"

enum class YehatPhrase
{
	NullPhrase,
	HomeworldHello1,
	HomeworldHello2,
	WhatsUpHomeworld,
	GeneralInfoHomeworld1,
	GeneralInfoHomeworld2,
	IDemandYouAllyHomeworld,
	EnemyMustDie,
	AtLeastHelpUsHomeworld,
	NoHelpEnemy,
	GiveInfo,
	NoInfoForEnemy,
	WhatAboutPkunkRoyalist,
	PkunkAbsorbedRoyalist,
	HatePkunkRoyalist,
	ByeHomeworld,
	GoodbyeAndDieHomeworld,
	SpaceHello1,
	SpaceHello2,
	SpaceHello3,
	SpaceHello4,
	WhatsUpSpace1,
	GeneralInfoSpace1,
	WhatsUpSpace2,
	GeneralInfoSpace2,
	WhatsUpSpace3,
	GeneralInfoSpace3,
	WhatsUpSpace4,
	GeneralInfoSpace4,
	IDemandYouAllySpace,
	WeCannot1,
	Obligation,
	WeCannot2,
	AtLeastHelpUsSpace,
	SorryCannot,
	Dishonor,
	HeresAHint,
	WhatAboutPkunkSpace,
	PkunkAbsorbedSpace,
	HatePkunkSpace,
	ByeSpace,
	GoInPeace,
	GoodbyeAndDieSpace,
	ShofixtiAlive1,
	ShofixtiAlive2,
	SendHimOver1,
	SendHimOver2,
	NotHere,
	NotSend,
	JustATrick1,
	JustATrick2,
	OkSend,
	WeRevolt,
	RoyalistSpaceHello1,
	RoyalistSpaceHello2,
	RoyalistHomeworldHello1,
	RoyalistHomeworldHello2,
	HowIsRebellion,
	RoyalistRebellion1,
	RoyalistRebellion2,
	SorryAboutRevolution,
	AllYourFault,
	ByeRoyalist,
	GoodbyeAndDieRoyalist,
	Name1,
	Name2,
	Name3,
	Name4,
	OutTakes,
};
consteval bool isArithmeticEnum(YehatPhrase) { return true; }

#endif // UQM_COMM_YEHAT_STRINGS_H
