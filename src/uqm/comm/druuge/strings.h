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
#ifndef UQM_COMM_DRUUGE_STRINGS_H
#define UQM_COMM_DRUUGE_STRINGS_H

#include "core/types/enum.h"

enum class DruugePhrase
{
	NullPhrase,
	AmbushIsFirstHello,
	InitBombWorldHello,
	SubseqBombWorldHello,
	WhatsUpAtBombPlanet,
	GenInfoAtBombPlanet,
	WeGetBomb,
	NotGetBomb,
	ThenWeTakeBomb,
	FightForBomb,
	GoodbyeFromBombPlanet,
	NotEnoughRoom,
	TradeForSphere,
	NoWay,
	OkRegularDeal,
	Way,
	OkHeresSphere,
	WhatsTheSphereAgain,
	SphereIs,
	WeSellForCrew,
	IWillNeverTradeCrew,
	YourLoss,
	IsntThisSlaveTrading,
	NoSlaveTrade,
	WhatDoWithCrew,
	HaveFun,
	ImReadyToBuy,
	ThisForSale,
	HaveSphere,
	HaveArt2,
	HaveArt1,
	ShipsAndFuel,
	BoughtShip,
	BoughtFuel,
	BoughtArt2,
	BoughtArt1,
	BoughtSphere,
	RepeatWhatToSell,
	InitSpaceHello,
	SubsequentSpaceHello,
	WhatsUpInSpace,
	GeneralInfoInSpace1,
	GeneralInfoInSpace2,
	GeneralInfoInSpace3,
	GeneralInfoInSpace4,
	GoodbyeFromSpace,
	HstlTradeWorldHello1,
	HstlTradeWorldHello2,
	HostileSpaceHello1,
	HostileSpaceHello2,
	InitialTradeWorldHello,
	SsqTradeWorldHello1,
	SsqTradeWorldHello2,
	SsqTradeWorldHello3,
	SsqTradeWorldHello4,
	WhatsUpAtTradeWorld,
	GenInfoAtTradeWorld1,
	GenInfoAtTradeWorld2,
	GenInfoAtTradeWorld3,
	GenInfoAtTradeWorld4,
	ScanMaidens,
	ScanFragments,
	ScanDruugeCaster,
	ScanArilouSpawner,
	EnoughFragments,
	ReadyToBuy,
	ReadyToSell,
	ByeFromTradeWorld1,
	ByeFromTradeWorld2,
	NotEnoughCrew,
	ExchangeMade,
	OkDoneBuying,
	OkDoneSelling,
	Bye,
	WantToSell,
	WantToBuy,
	BuyDruugeShip,
	BuyFuel,
	BuyArt1,
	BuyArt2,
	BuyRosySphere,
	DoneBuying,
	DoneSelling,
	SellMaidens,
	SellCaster,
	SellFragments,
	SellSpawner,
	BoughtMaidens,
	BoughtFragments,
	BoughtCaster,
	YouGet,
	YouAlsoGet,
	BoughtSpawner,
	SalvageYourShip1,
	SalvageYourShip2,
	DealForStatedShips,
	DealForLessShips,
	DealForNoShips,
	Fuel0,
	Fuel1,
	HideousDeal,
	BadDeal,
	FairDeal,
	GoodDeal,
	FineDeal,
	OutTakes,
};
template <>
inline constexpr bool IsArithmeticEnum<DruugePhrase> = true;

#endif // UQM_COMM_DRUUGE_STRINGS_H
