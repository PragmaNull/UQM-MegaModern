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

#include "../commall.h"
#include "resinst.h"
#include "strings.h"

#include "uqm/build.h"
#include "uqm/setup.h"
#include "uqm/sis.h"
// for DeltaSISGauges()
#include "uqm/planets/generate/gendefault.h"
#include "uqm/lua/UQMLua.h"


static ConversationData druuge_desc =
	{
		Conversation::Druuge, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		DruugePmapAnim, /* AlienFrame */
		DruugeFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Middle, /* AlienTextValign */
		DruugeColorMap, /* AlienColorMap */
		DruugeMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		DruugeConversationPhrases, /* PlayerPhrases */
		11, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */

			{
				// Biggest flame
				5,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				// Flame on the right
				9,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				// Smallest flame (behind the biggest flame)
				13,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				// Flashing eyes
				19,			  /* StartIndex */
				3,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				22,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				25,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				28,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				31,			  /* StartIndex */
				2,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				33,							/* StartIndex */
				7,							/* NumFrames */
				CircularAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond * 7,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				40,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 10,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 10,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				44,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 5,
				0, /* FrameRate */
				GameTicksPerSecond / 5,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								},
		{
								/* AlienTransitionDesc */
			0, /* StartIndex */
			0, /* NumFrames */
			0, /* AnimFlags */
			0,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			1, /* StartIndex */
			4, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 15,
								GameTicksPerSecond / 15, /* FrameRate */
			GameTicksPerSecond / 12,
								GameTicksPerSecond / 12, /* RestartRate */
			0,						 /* BlockMask */
		},
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static uint16_t SlaveryCount = 0;
static bool AttemptedSalvage = false;

static void ExitConversation(RESPONSE_REF R)
{
	if (PlayerSaid(R, DruugePhrase::Bye))
	{
		setSegue(Segue::Peace);

		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			if (SlaveryCount)
			{
				uint16_t PreviousSlaves {};

				PreviousSlaves = uqm::makeWord(
					uqm::gameState().flag(uqm::GameFlag::CREW_SOLD_TO_DRUUGE0),
					uqm::gameState().flag(uqm::GameFlag::CREW_SOLD_TO_DRUUGE1));
				SlaveryCount += PreviousSlaves;
				if (SlaveryCount > MaxSold() && PreviousSlaves <= MaxSold())
				{
					if (PreviousSlaves > MinSold())
					{
						uqm::gameState().game().CrewCost += (22 - 7);
					}
					else
					{
						uqm::gameState().game().CrewCost += 22;
					}
				}
				else if (SlaveryCount > MinSold() && PreviousSlaves <= MinSold())
				{
					uqm::gameState().game().CrewCost += 7;
				}

				uqm::gameState().setFlag(uqm::GameFlag::CREW_SOLD_TO_DRUUGE0, static_cast<uint32_t>(uqm::lowByte(SlaveryCount)));
				uqm::gameState().setFlag(uqm::GameFlag::CREW_SOLD_TO_DRUUGE1, static_cast<uint32_t>(uqm::highByte(SlaveryCount)));
			}

			switch (uqm::gameState().flag(uqm::GameFlag::DRUUGE_HOME_VISITS))
			{
				case 1:
					NPCPhrase(DruugePhrase::ByeFromTradeWorld1);
					break;
				default:
					NPCPhrase(DruugePhrase::ByeFromTradeWorld2);
					break;
			}
		}
		else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			NPCPhrase(DruugePhrase::GoodbyeFromBombPlanet);
		}
		else
		{
			NPCPhrase(DruugePhrase::GoodbyeFromSpace);
		}
	}
	else /* if (R == DruugePhrase::ThenWeTakeBomb) */
	{
		setSegue(Segue::Hostile);

		NPCPhrase(DruugePhrase::FightForBomb);
	}
}

static void TradeWorld(RESPONSE_REF R);

static constexpr auto ArtifactCrewCost {100};

static constexpr auto ShipCrewCost {100};
static constexpr auto FuelCrewCost {10};

static void Buy(RESPONSE_REF R)
{
	if (PlayerSaid(R, DruugePhrase::WantToBuy)
		|| PlayerSaid(R, DruugePhrase::ImReadyToBuy))
	{
		NPCPhrase(DruugePhrase::ReadyToSell);
		if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
		{
			NPCPhrase(DruugePhrase::HaveSphere);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP))
		{
			NPCPhrase(DruugePhrase::HaveArt1);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::GLOWING_ROD_ON_SHIP))
		{
			NPCPhrase(DruugePhrase::HaveArt2);
		}
		NPCPhrase(DruugePhrase::ShipsAndFuel);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, DruugePhrase::BuyDruugeShip))
	{
		if (uqm::gameState().sis().CrewEnlisted < ShipCrewCost)
		{
			NPCPhrase(DruugePhrase::NotEnoughCrew);
		}
		else if (EscortFeasibilityStudy(RaceId::Druuge) == 0)
		{
			NPCPhrase(DruugePhrase::NotEnoughRoom);
		}
		else
		{
			DeltaSISGauges(-ShipCrewCost, 0, 0);
			SlaveryCount += ShipCrewCost;
			AddEscortShips(RaceId::Druuge, 1);

			NPCPhrase(DruugePhrase::BoughtShip);
		}
	}
	else if (PlayerSaid(R, DruugePhrase::BuyRosySphere))
	{
		if (uqm::gameState().sis().CrewEnlisted < ArtifactCrewCost)
		{
			NPCPhrase(DruugePhrase::NotEnoughCrew);
		}
		else
		{
			DeltaSISGauges(-ArtifactCrewCost, 0, 0);
			SlaveryCount += ArtifactCrewCost;
			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE, static_cast<uint32_t>(1));

			NPCPhrase(DruugePhrase::BoughtSphere);
		}
	}
	else if (PlayerSaid(R, DruugePhrase::BuyArt1))
	{
		if (uqm::gameState().sis().CrewEnlisted < ArtifactCrewCost)
		{
			NPCPhrase(DruugePhrase::NotEnoughCrew);
		}
		else
		{
			DeltaSISGauges(-ArtifactCrewCost, 0, 0);
			SlaveryCount += ArtifactCrewCost;
			uqm::gameState().setFlag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP, static_cast<uint32_t>(1));

			NPCPhrase(DruugePhrase::BoughtArt1);
		}
	}
	else if (PlayerSaid(R, DruugePhrase::BuyArt2))
	{
		if (uqm::gameState().sis().CrewEnlisted < ArtifactCrewCost)
		{
			NPCPhrase(DruugePhrase::NotEnoughCrew);
		}
		else
		{
			DeltaSISGauges(-ArtifactCrewCost, 0, 0);
			SlaveryCount += ArtifactCrewCost;
			uqm::gameState().setFlag(uqm::GameFlag::GLOWING_ROD_ON_SHIP, static_cast<uint32_t>(1));

			NPCPhrase(DruugePhrase::BoughtArt2);
		}
	}
	else if (PlayerSaid(R, DruugePhrase::BuyFuel))
	{
		if (uqm::gameState().sis().CrewEnlisted < FuelCrewCost)
		{
			NPCPhrase(DruugePhrase::NotEnoughCrew);
		}
		else
		{
			DeltaSISGauges(-FuelCrewCost,
						   FuelCrewCost * FuelTankScale, 0);
			SlaveryCount += FuelCrewCost;

			NPCPhrase(DruugePhrase::BoughtFuel);
		}
	}

	Response(DruugePhrase::BuyDruugeShip, Buy);
	if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
	{
		Response(DruugePhrase::BuyRosySphere, Buy);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP))
	{
		Response(DruugePhrase::BuyArt1, Buy);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::GLOWING_ROD_ON_SHIP))
	{
		Response(DruugePhrase::BuyArt2, Buy);
	}
	Response(DruugePhrase::BuyFuel, Buy);
	Response(DruugePhrase::DoneBuying, TradeWorld);
}

static void Sell(RESPONSE_REF R);

static RESPONSE_REF LastResponse = 0;

static void Trade(RESPONSE_REF R)
{
	if (!PlayerSaid(R, DruugePhrase::WhatsTheSphereAgain))
	{
		NPCPhrase(DruugePhrase::TradeForSphere);
		LastResponse = R;
	}
	else
	{
		NPCPhrase(DruugePhrase::SphereIs);

		DisablePhrase(DruugePhrase::WhatsTheSphereAgain);
	}

	Response(DruugePhrase::NoWay, Sell);
	Response(DruugePhrase::Way, Sell);
	if (PhraseEnabled(DruugePhrase::WhatsTheSphereAgain))
	{
		Response(DruugePhrase::WhatsTheSphereAgain, Trade);
	}
}

static void DoTransaction(RESPONSE_REF R)
{
	if (PlayerSaid(R, DruugePhrase::SellMaidens))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MAIDENS_ON_SHIP, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, DruugePhrase::SellFragments))
	{
		uint8_t num_frags {};

		if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE0_ON_SHIP, static_cast<uint32_t>(0));
		}
		else if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE1_ON_SHIP, static_cast<uint32_t>(0));
		}
		else if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::EGG_CASE2_ON_SHIP, static_cast<uint32_t>(0));
		}

		num_frags = uqm::gameState().flag(uqm::GameFlag::FRAGMENTS_BOUGHT) + 1;
		uqm::gameState().setFlag(uqm::GameFlag::FRAGMENTS_BOUGHT, static_cast<uint32_t>(num_frags));
	}
	else if (PlayerSaid(R, DruugePhrase::SellCaster))
	{
		uqm::gameState().setFlag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, DruugePhrase::SellSpawner))
	{
		uqm::gameState().setFlag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP, static_cast<uint32_t>(0));
	}

	if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE)
		&& uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP))
	{
		uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE, static_cast<uint32_t>(1));
	}
	else
	{
		uint8_t ship_slots {};

		if (isDifficulty(uqm::Difficulty::Hard) && !(uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << *HardModeEncounter::ReadyToBargain))
		{
			uint16_t state {};

			state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);

			state |= 1 << *HardModeEncounter::ReadyToBargain;

			uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));
		}

		uint8_t trade_gas {0};
		uint8_t ships_to_trade {0};
		ship_slots = EscortFeasibilityStudy(RaceId::Druuge);
		if (PlayerSaid(R, DruugePhrase::SellMaidens))
		{
			NPCPhrase(DruugePhrase::BoughtMaidens);
			ships_to_trade = 6;
		}
		else if (PlayerSaid(R, DruugePhrase::SellFragments))
		{
			NPCPhrase(DruugePhrase::BoughtFragments);
			ships_to_trade = 1;
		}
		else if (PlayerSaid(R, DruugePhrase::SellCaster))
		{
			NPCPhrase(DruugePhrase::BoughtCaster);
			ships_to_trade = 0;
			trade_gas = 1;
		}
		else if (PlayerSaid(R, DruugePhrase::SellSpawner))
		{
			NPCPhrase(DruugePhrase::BoughtSpawner);
			ships_to_trade = 3;
			trade_gas = 1;
		}

		NPCPhrase(DruugePhrase::YouGet);
		if (ships_to_trade)
		{
			AddEscortShips(RaceId::Druuge, ships_to_trade);

			if (ship_slots >= ships_to_trade)
			{
				NPCPhrase(DruugePhrase::DealForStatedShips);
			}
			else if (ship_slots == 0)
			{
				NPCPhrase(DruugePhrase::DealForNoShips);
			}
			else
			{
				NPCPhrase(DruugePhrase::DealForLessShips);
			}

			if (trade_gas)
			{
				NPCPhrase(DruugePhrase::YouAlsoGet);
			}
		}

		if (trade_gas)
		{
			const uint16_t HardLimit = 500;
			uint32_t FuelOnBoard {};

			FuelOnBoard = uqm::gameState().sis().FuelOnBoard;
			uint32_t capacity {FuelReserve};
			uint8_t slot {NumModuleSlots - 1};
			do
			{
				if (uqm::gameState().sis().ModuleSlots[slot] == ModuleType::FuelTank
					|| uqm::gameState().sis().ModuleSlots[slot] == ModuleType::HigheffFuelsys)
				{
					uint16_t volume {};

					volume = uqm::gameState().sis().ModuleSlots[slot] == ModuleType::FuelTank ? FuelTankCapacity : HefuelTankCapacity;
					capacity += volume;
				}
			} while (slot--);
			capacity -= FuelOnBoard;
			uint16_t f {(uint16_t)((capacity + (FuelTankScale >> 1)) / FuelTankScale)};

			if (isDifficulty(uqm::Difficulty::Hard) && f > HardLimit)
			{
				f -= (f - HardLimit);
				capacity = (f * FuelTankScale) - FuelOnBoard;
			}

			while (capacity > 0x3FFFL)
			{
				DeltaSISGauges(0, 0x3FFF, 0);
				capacity -= 0x3FFF;
			}

			DeltaSISGauges(0, capacity, 0);

			NPCPhrase(DruugePhrase::Fuel0);
			NPCNumber(f, nullptr);
			NPCPhrase(DruugePhrase::Fuel1);

			if (f >= 250)
			{
				NPCPhrase(DruugePhrase::HideousDeal);
			}
			else if (f >= 100)
			{
				NPCPhrase(DruugePhrase::BadDeal);
			}
			else if (f >= 50)
			{
				NPCPhrase(DruugePhrase::FairDeal);
			}
			else if (f >= 10)
			{
				NPCPhrase(DruugePhrase::GoodDeal);
			}
			else
			{
				NPCPhrase(DruugePhrase::FineDeal);
			}
		}
	}
}

static void Sell(RESPONSE_REF R)
{
	RESPONSE_FUNC RespFunc {};

	if (PlayerSaid(R, DruugePhrase::WantToSell))
	{
		NPCPhrase(DruugePhrase::ReadyToBuy);
	}
	else if (PlayerSaid(R, DruugePhrase::NoWay)
			 || PlayerSaid(R, DruugePhrase::Way))
	{
		if (PlayerSaid(R, DruugePhrase::NoWay))
		{
			NPCPhrase(DruugePhrase::OkRegularDeal);
		}
		else
		{
			NPCPhrase(DruugePhrase::OkHeresSphere);

			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(1));
		}

		DoTransaction(LastResponse);
	}
	else if (PlayerSaid(R, DruugePhrase::SellMaidens)
			 || PlayerSaid(R, DruugePhrase::SellFragments)
			 || PlayerSaid(R, DruugePhrase::SellCaster)
			 || PlayerSaid(R, DruugePhrase::SellSpawner))
	{
		DoTransaction(R);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
	{
		if ((isDifficulty(uqm::Difficulty::Hard) && uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << *HardModeEncounter::ReadyToBargain) || !isDifficulty(uqm::Difficulty::Hard))
		{
			RespFunc = (RESPONSE_FUNC)Trade;
		}
		else
		{
			RespFunc = (RESPONSE_FUNC)Sell;
		}
	}
	else
	{
		RespFunc = (RESPONSE_FUNC)Sell;
	}
	if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP))
	{
		Response(DruugePhrase::SellMaidens, RespFunc);
	}
	if ((uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP)
		 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP)
		 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP))
		&& uqm::gameState().flag(uqm::GameFlag::FRAGMENTS_BOUGHT) < 2)
	{
		Response(DruugePhrase::SellFragments, RespFunc);
	}
	if (uqm::gameState().flag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP))
	{
		Response(DruugePhrase::SellCaster, RespFunc);
	}
	if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP))
	{
		Response(DruugePhrase::SellSpawner, RespFunc);
	}
	Response(DruugePhrase::DoneSelling, TradeWorld);
}

static void ExplainSlaveTrade(RESPONSE_REF R)
{
	if (PlayerSaid(R, DruugePhrase::WantToBuy))
	{
		NPCPhrase(DruugePhrase::WeSellForCrew);
	}
	else if (PlayerSaid(R, DruugePhrase::IsntThisSlaveTrading))
	{
		NPCPhrase(DruugePhrase::NoSlaveTrade);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, DruugePhrase::WhatDoWithCrew))
	{
		NPCPhrase(DruugePhrase::HaveFun);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS, static_cast<uint32_t>(2));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS))
	{
		case 0:
			Response(DruugePhrase::IsntThisSlaveTrading, ExplainSlaveTrade);
			break;
		case 1:
			Response(DruugePhrase::WhatDoWithCrew, ExplainSlaveTrade);
			break;
	}
	Response(DruugePhrase::IWillNeverTradeCrew, TradeWorld);
	Response(DruugePhrase::ImReadyToBuy, Buy);
}

static void TradeWorld(RESPONSE_REF R)
{
	if (PlayerSaid(R, DruugePhrase::WhatsUpAtTradeWorld))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_HOME_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(DruugePhrase::GenInfoAtTradeWorld1);
				break;
			case 1:
				NPCPhrase(DruugePhrase::GenInfoAtTradeWorld2);
				break;
			case 2:
				NPCPhrase(DruugePhrase::GenInfoAtTradeWorld3);
				if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2)
				{
					uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
				}
				break;
			case 3:
				NPCPhrase(DruugePhrase::GenInfoAtTradeWorld4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_HOME_INFO, static_cast<uint32_t>(NumVisits));
		DisablePhrase(DruugePhrase::WhatsUpAtTradeWorld);
	}
	else if (PlayerSaid(R, DruugePhrase::DoneSelling))
	{
		NPCPhrase(DruugePhrase::OkDoneSelling);
	}
	else if (PlayerSaid(R, DruugePhrase::DoneBuying))
	{
		NPCPhrase(DruugePhrase::OkDoneBuying);
	}
	else if (PlayerSaid(R, DruugePhrase::IWillNeverTradeCrew))
	{
		NPCPhrase(DruugePhrase::YourLoss);
	}

	if (PhraseEnabled(DruugePhrase::WhatsUpAtTradeWorld))
	{
		Response(DruugePhrase::WhatsUpAtTradeWorld, TradeWorld);
	}
	Response(DruugePhrase::WantToSell, Sell);
	if (uqm::gameState().flag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS) == 3)
	{
		Response(DruugePhrase::WantToBuy, Buy);
	}
	else
	{
		Response(DruugePhrase::WantToBuy, ExplainSlaveTrade);
	}
	Response(DruugePhrase::Bye, ExitConversation);
}

static void BombAmbush(RESPONSE_REF R)
{
	if (PlayerSaid(R, DruugePhrase::WhatsUpAtBombPlanet))
	{
		NPCPhrase(DruugePhrase::GenInfoAtBombPlanet);
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, DruugePhrase::WeGetBomb))
	{
		NPCPhrase(DruugePhrase::NotGetBomb);
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(3));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS))
	{
		case 1:
			Response(DruugePhrase::WhatsUpAtBombPlanet, BombAmbush);
			break;
		case 2:
			Response(DruugePhrase::WeGetBomb, BombAmbush);
			break;
		default:
			Response(DruugePhrase::ThenWeTakeBomb, ExitConversation);
			break;
	}
	Response(DruugePhrase::Bye, ExitConversation);
}

static void Space(RESPONSE_REF R)
{
	if (PlayerSaid(R, DruugePhrase::WhatsUpInSpace))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_SPACE_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(DruugePhrase::GeneralInfoInSpace1);
				break;
			case 1:
				NPCPhrase(DruugePhrase::GeneralInfoInSpace2);
				break;
			case 2:
				NPCPhrase(DruugePhrase::GeneralInfoInSpace3);
				break;
			case 3:
				NPCPhrase(DruugePhrase::GeneralInfoInSpace4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_SPACE_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(DruugePhrase::WhatsUpInSpace);
	}

	if (PhraseEnabled(DruugePhrase::WhatsUpInSpace))
	{
		Response(DruugePhrase::WhatsUpInSpace, Space);
	}
	Response(DruugePhrase::Bye, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(DruugePhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::DRUUGE_MANNER))
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(DruugePhrase::HstlTradeWorldHello1);
					break;
				case 1:
					NPCPhrase(DruugePhrase::HstlTradeWorldHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(DruugePhrase::HostileSpaceHello1);
					break;
				case 1:
					NPCPhrase(DruugePhrase::HostileSpaceHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_VISITS, static_cast<uint32_t>(NumVisits));
		}

		setSegue(Segue::Hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(DruugePhrase::InitialTradeWorldHello);
					break;
				case 1:
					NPCPhrase(DruugePhrase::SsqTradeWorldHello1);
					break;
				case 2:
					NPCPhrase(DruugePhrase::SsqTradeWorldHello2);
					break;
				case 3:
					NPCPhrase(DruugePhrase::SsqTradeWorldHello3);
					break;
				case 4:
					NPCPhrase(DruugePhrase::SsqTradeWorldHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		if (uqm::gameState().flag(uqm::GameFlag::ATTACKED_DRUUGE)
			&& !uqm::gameState().flag(uqm::GameFlag::DRUUGE_DISCLAIMER))
		{
			// There is no HOSTILE_TRADE voice track that we know of
			// so this is currently disabled
			//NPCPhrase (HOSTILE_TRADE);
			uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_DISCLAIMER, static_cast<uint32_t>(1));
		}
		if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP)
			&& !uqm::gameState().flag(uqm::GameFlag::SCANNED_MAIDENS))
		{
			NPCPhrase(DruugePhrase::ScanMaidens);
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_MAIDENS, static_cast<uint32_t>(1));
		}
		if ((uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP)
			 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP)
			 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP))
			&& !uqm::gameState().flag(uqm::GameFlag::SCANNED_FRAGMENTS))
		{
			if (uqm::gameState().flag(uqm::GameFlag::FRAGMENTS_BOUGHT) < 2)
			{
				NPCPhrase(DruugePhrase::ScanFragments);
			}
			else
			{
				NPCPhrase(DruugePhrase::EnoughFragments);
			}
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_FRAGMENTS, static_cast<uint32_t>(1));
		}
		if (uqm::gameState().flag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP)
			&& !uqm::gameState().flag(uqm::GameFlag::SCANNED_CASTER))
		{
			NPCPhrase(DruugePhrase::ScanDruugeCaster);
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_CASTER, static_cast<uint32_t>(1));
		}
		if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP)
			&& !uqm::gameState().flag(uqm::GameFlag::SCANNED_SPAWNER))
		{
			NPCPhrase(DruugePhrase::ScanArilouSpawner);
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_SPAWNER, static_cast<uint32_t>(1));
		}

		TradeWorld((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		if (uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS))
		{
			NPCPhrase(DruugePhrase::SubseqBombWorldHello);
		}
		else
		{
			NPCPhrase(DruugePhrase::InitBombWorldHello);
			uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(1));
		}

		BombAmbush((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ATTACKED_DRUUGE))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(DruugePhrase::HostileSpaceHello1);
				break;
			case 1:
				NPCPhrase(DruugePhrase::HostileSpaceHello2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else
	{
		NumVisits = 0;
		if (GetHeadLink(&uqm::gameState().game().built_ship_q) == 0)
		{
			for (NumVisits = 0; NumVisits < NumModuleSlots; ++NumVisits)
			{
				const uint8_t which_module {*uqm::gameState().sis().ModuleSlots[NumVisits]};
				if (which_module >= ModuleType::GunWeapon
					&& which_module <= ModuleType::CannonWeapon)
				{
					NumVisits = 0;
					break;
				}
			}
		}

		if (NumVisits)
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_SALVAGE);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(DruugePhrase::SalvageYourShip1);
					break;
				case 1:
					NPCPhrase(DruugePhrase::SalvageYourShip2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_SALVAGE, static_cast<uint32_t>(NumVisits));

			setSegue(Segue::Hostile);
			AttemptedSalvage = true;
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(DruugePhrase::InitSpaceHello);
					SetHomeworldKnown(*HomeId::Druuge);
					break;
				case 1:
					NPCPhrase(DruugePhrase::SubsequentSpaceHello);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_VISITS, static_cast<uint32_t>(NumVisits));

			Space((RESPONSE_REF)0);
		}
	}
}

static uint16_t uninit_druuge(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_druuge_enc(void)
{
	if (getSegue() == Segue::Hostile
		&& !AttemptedSalvage
		&& !uqm::gameState().flag(uqm::GameFlag::DRUUGE_MANNER))
	{
		if (!uqm::gameState().flag(uqm::GameFlag::ATTACKED_DRUUGE))
		{
			uqm::gameState().setFlag(uqm::GameFlag::ATTACKED_DRUUGE, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_VISITS, static_cast<uint32_t>(0));
		}
	}
}

ConversationData* init_druuge_comm(void)
{
	SlaveryCount = 0;
	AttemptedSalvage = false;

	druuge_desc.init_encounter_func = Intro;
	druuge_desc.post_encounter_func = post_druuge_enc;
	druuge_desc.uninit_encounter_func = uninit_druuge;

	uqm::lua().beginComm(nullptr, NullResource);

	druuge_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	druuge_desc.AlienTextBaseline.y = ResScale(64);
	druuge_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if ((uqm::gameState().flag(uqm::GameFlag::DRUUGE_MANNER) == 0
		 && (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&druuge_desc};

	return (retval);
}
