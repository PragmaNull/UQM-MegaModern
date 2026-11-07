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
		DRUUGE_CONVERSATION, /* AlienConv */
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
	if (PlayerSaid(R, bye))
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
					NPCPhrase(BYE_FROM_TRADE_WORLD_1);
					break;
				default:
					NPCPhrase(BYE_FROM_TRADE_WORLD_2);
					break;
			}
		}
		else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			NPCPhrase(GOODBYE_FROM_BOMB_PLANET);
		}
		else
		{
			NPCPhrase(GOODBYE_FROM_SPACE);
		}
	}
	else /* if (R == then_we_take_bomb) */
	{
		setSegue(Segue::Hostile);

		NPCPhrase(FIGHT_FOR_BOMB);
	}
}

static void TradeWorld(RESPONSE_REF R);

static constexpr auto ArtifactCrewCost {100};

static constexpr auto ShipCrewCost {100};
static constexpr auto FuelCrewCost {10};

static void Buy(RESPONSE_REF R)
{
	if (PlayerSaid(R, want_to_buy)
		|| PlayerSaid(R, im_ready_to_buy))
	{
		NPCPhrase(READY_TO_SELL);
		if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
		{
			NPCPhrase(HAVE_SPHERE);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP))
		{
			NPCPhrase(HAVE_ART_1);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::GLOWING_ROD_ON_SHIP))
		{
			NPCPhrase(HAVE_ART_2);
		}
		NPCPhrase(SHIPS_AND_FUEL);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, buy_druuge_ship))
	{
		if (uqm::gameState().sis().CrewEnlisted < ShipCrewCost)
		{
			NPCPhrase(NOT_ENOUGH_CREW);
		}
		else if (EscortFeasibilityStudy(DRUUGE_SHIP) == 0)
		{
			NPCPhrase(NOT_ENOUGH_ROOM);
		}
		else
		{
			DeltaSISGauges(-ShipCrewCost, 0, 0);
			SlaveryCount += ShipCrewCost;
			AddEscortShips(DRUUGE_SHIP, 1);

			NPCPhrase(BOUGHT_SHIP);
		}
	}
	else if (PlayerSaid(R, buy_rosy_sphere))
	{
		if (uqm::gameState().sis().CrewEnlisted < ArtifactCrewCost)
		{
			NPCPhrase(NOT_ENOUGH_CREW);
		}
		else
		{
			DeltaSISGauges(-ArtifactCrewCost, 0, 0);
			SlaveryCount += ArtifactCrewCost;
			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE, static_cast<uint32_t>(1));

			NPCPhrase(BOUGHT_SPHERE);
		}
	}
	else if (PlayerSaid(R, buy_art_1))
	{
		if (uqm::gameState().sis().CrewEnlisted < ArtifactCrewCost)
		{
			NPCPhrase(NOT_ENOUGH_CREW);
		}
		else
		{
			DeltaSISGauges(-ArtifactCrewCost, 0, 0);
			SlaveryCount += ArtifactCrewCost;
			uqm::gameState().setFlag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP, static_cast<uint32_t>(1));

			NPCPhrase(BOUGHT_ART_1);
		}
	}
	else if (PlayerSaid(R, buy_art_2))
	{
		if (uqm::gameState().sis().CrewEnlisted < ArtifactCrewCost)
		{
			NPCPhrase(NOT_ENOUGH_CREW);
		}
		else
		{
			DeltaSISGauges(-ArtifactCrewCost, 0, 0);
			SlaveryCount += ArtifactCrewCost;
			uqm::gameState().setFlag(uqm::GameFlag::GLOWING_ROD_ON_SHIP, static_cast<uint32_t>(1));

			NPCPhrase(BOUGHT_ART_2);
		}
	}
	else if (PlayerSaid(R, buy_fuel))
	{
		if (uqm::gameState().sis().CrewEnlisted < FuelCrewCost)
		{
			NPCPhrase(NOT_ENOUGH_CREW);
		}
		else
		{
			DeltaSISGauges(-FuelCrewCost,
						   FuelCrewCost * FuelTankScale, 0);
			SlaveryCount += FuelCrewCost;

			NPCPhrase(BOUGHT_FUEL);
		}
	}

	Response(buy_druuge_ship, Buy);
	if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
	{
		Response(buy_rosy_sphere, Buy);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP))
	{
		Response(buy_art_1, Buy);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::GLOWING_ROD_ON_SHIP))
	{
		Response(buy_art_2, Buy);
	}
	Response(buy_fuel, Buy);
	Response(done_buying, TradeWorld);
}

static void Sell(RESPONSE_REF R);

static RESPONSE_REF LastResponse = 0;

static void Trade(RESPONSE_REF R)
{
	if (!PlayerSaid(R, whats_the_sphere_again))
	{
		NPCPhrase(TRADE_FOR_SPHERE);
		LastResponse = R;
	}
	else
	{
		NPCPhrase(SPHERE_IS);

		DisablePhrase(whats_the_sphere_again);
	}

	Response(no_way, Sell);
	Response(way, Sell);
	if (PhraseEnabled(whats_the_sphere_again))
	{
		Response(whats_the_sphere_again, Trade);
	}
}

static void DoTransaction(RESPONSE_REF R)
{
	if (PlayerSaid(R, sell_maidens))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MAIDENS_ON_SHIP, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, sell_fragments))
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
	else if (PlayerSaid(R, sell_caster))
	{
		uqm::gameState().setFlag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, sell_spawner))
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

		if (isDifficulty(uqm::Difficulty::Hard) && !(uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << READY_TO_BARGAIN))
		{
			uint16_t state {};

			state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);

			state |= 1 << READY_TO_BARGAIN;

			uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));
		}

		uint8_t trade_gas {0};
		uint8_t ships_to_trade {0};
		ship_slots = EscortFeasibilityStudy(DRUUGE_SHIP);
		if (PlayerSaid(R, sell_maidens))
		{
			NPCPhrase(BOUGHT_MAIDENS);
			ships_to_trade = 6;
		}
		else if (PlayerSaid(R, sell_fragments))
		{
			NPCPhrase(BOUGHT_FRAGMENTS);
			ships_to_trade = 1;
		}
		else if (PlayerSaid(R, sell_caster))
		{
			NPCPhrase(BOUGHT_CASTER);
			ships_to_trade = 0;
			trade_gas = 1;
		}
		else if (PlayerSaid(R, sell_spawner))
		{
			NPCPhrase(BOUGHT_SPAWNER);
			ships_to_trade = 3;
			trade_gas = 1;
		}

		NPCPhrase(YOU_GET);
		if (ships_to_trade)
		{
			AddEscortShips(DRUUGE_SHIP, ships_to_trade);

			if (ship_slots >= ships_to_trade)
			{
				NPCPhrase(DEAL_FOR_STATED_SHIPS);
			}
			else if (ship_slots == 0)
			{
				NPCPhrase(DEAL_FOR_NO_SHIPS);
			}
			else
			{
				NPCPhrase(DEAL_FOR_LESS_SHIPS);
			}

			if (trade_gas)
			{
				NPCPhrase(YOU_ALSO_GET);
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
				if (uqm::gameState().sis().ModuleSlots[slot] == FUEL_TANK
					|| uqm::gameState().sis().ModuleSlots[slot] == HIGHEFF_FUELSYS)
				{
					uint16_t volume {};

					volume = uqm::gameState().sis().ModuleSlots[slot] == FUEL_TANK ? FuelTankCapacity : HefuelTankCapacity;
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

			NPCPhrase(FUEL0);
			NPCNumber(f, nullptr);
			NPCPhrase(FUEL1);

			if (f >= 250)
			{
				NPCPhrase(HIDEOUS_DEAL);
			}
			else if (f >= 100)
			{
				NPCPhrase(BAD_DEAL);
			}
			else if (f >= 50)
			{
				NPCPhrase(FAIR_DEAL);
			}
			else if (f >= 10)
			{
				NPCPhrase(GOOD_DEAL);
			}
			else
			{
				NPCPhrase(FINE_DEAL);
			}
		}
	}
}

static void Sell(RESPONSE_REF R)
{
	RESPONSE_FUNC RespFunc {};

	if (PlayerSaid(R, want_to_sell))
	{
		NPCPhrase(READY_TO_BUY);
	}
	else if (PlayerSaid(R, no_way)
			 || PlayerSaid(R, way))
	{
		if (PlayerSaid(R, no_way))
		{
			NPCPhrase(OK_REGULAR_DEAL);
		}
		else
		{
			NPCPhrase(OK_HERES_SPHERE);

			uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(1));
		}

		DoTransaction(LastResponse);
	}
	else if (PlayerSaid(R, sell_maidens)
			 || PlayerSaid(R, sell_fragments)
			 || PlayerSaid(R, sell_caster)
			 || PlayerSaid(R, sell_spawner))
	{
		DoTransaction(R);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE))
	{
		if ((isDifficulty(uqm::Difficulty::Hard) && uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS) & 1 << READY_TO_BARGAIN) || !isDifficulty(uqm::Difficulty::Hard))
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
		Response(sell_maidens, RespFunc);
	}
	if ((uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP)
		 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP)
		 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP))
		&& uqm::gameState().flag(uqm::GameFlag::FRAGMENTS_BOUGHT) < 2)
	{
		Response(sell_fragments, RespFunc);
	}
	if (uqm::gameState().flag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP))
	{
		Response(sell_caster, RespFunc);
	}
	if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP))
	{
		Response(sell_spawner, RespFunc);
	}
	Response(done_selling, TradeWorld);
}

static void ExplainSlaveTrade(RESPONSE_REF R)
{
	if (PlayerSaid(R, want_to_buy))
	{
		NPCPhrase(WE_SELL_FOR_CREW);
	}
	else if (PlayerSaid(R, isnt_this_slave_trading))
	{
		NPCPhrase(NO_SLAVE_TRADE);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, what_do_with_crew))
	{
		NPCPhrase(HAVE_FUN);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS, static_cast<uint32_t>(2));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS))
	{
		case 0:
			Response(isnt_this_slave_trading, ExplainSlaveTrade);
			break;
		case 1:
			Response(what_do_with_crew, ExplainSlaveTrade);
			break;
	}
	Response(i_will_never_trade_crew, TradeWorld);
	Response(im_ready_to_buy, Buy);
}

static void TradeWorld(RESPONSE_REF R)
{
	if (PlayerSaid(R, whats_up_at_trade_world))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_HOME_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GEN_INFO_AT_TRADE_WORLD_1);
				break;
			case 1:
				NPCPhrase(GEN_INFO_AT_TRADE_WORLD_2);
				break;
			case 2:
				NPCPhrase(GEN_INFO_AT_TRADE_WORLD_3);
				if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2)
				{
					uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
				}
				break;
			case 3:
				NPCPhrase(GEN_INFO_AT_TRADE_WORLD_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_HOME_INFO, static_cast<uint32_t>(NumVisits));
		DisablePhrase(whats_up_at_trade_world);
	}
	else if (PlayerSaid(R, done_selling))
	{
		NPCPhrase(OK_DONE_SELLING);
	}
	else if (PlayerSaid(R, done_buying))
	{
		NPCPhrase(OK_DONE_BUYING);
	}
	else if (PlayerSaid(R, i_will_never_trade_crew))
	{
		NPCPhrase(YOUR_LOSS);
	}

	if (PhraseEnabled(whats_up_at_trade_world))
	{
		Response(whats_up_at_trade_world, TradeWorld);
	}
	Response(want_to_sell, Sell);
	if (uqm::gameState().flag(uqm::GameFlag::KNOW_DRUUGE_SLAVERS) == 3)
	{
		Response(want_to_buy, Buy);
	}
	else
	{
		Response(want_to_buy, ExplainSlaveTrade);
	}
	Response(bye, ExitConversation);
}

static void BombAmbush(RESPONSE_REF R)
{
	if (PlayerSaid(R, whats_up_at_bomb_planet))
	{
		NPCPhrase(GEN_INFO_AT_BOMB_PLANET);
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, we_get_bomb))
	{
		NPCPhrase(NOT_GET_BOMB);
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(3));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS))
	{
		case 1:
			Response(whats_up_at_bomb_planet, BombAmbush);
			break;
		case 2:
			Response(we_get_bomb, BombAmbush);
			break;
		default:
			Response(then_we_take_bomb, ExitConversation);
			break;
	}
	Response(bye, ExitConversation);
}

static void Space(RESPONSE_REF R)
{
	if (PlayerSaid(R, whats_up_in_space))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::DRUUGE_SPACE_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_IN_SPACE_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_IN_SPACE_2);
				break;
			case 2:
				NPCPhrase(GENERAL_INFO_IN_SPACE_3);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_IN_SPACE_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::DRUUGE_SPACE_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_in_space);
	}

	if (PhraseEnabled(whats_up_in_space))
	{
		Response(whats_up_in_space, Space);
	}
	Response(bye, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

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
					NPCPhrase(HSTL_TRADE_WORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase(HSTL_TRADE_WORLD_HELLO_2);
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
					NPCPhrase(HOSTILE_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(HOSTILE_SPACE_HELLO_2);
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
					NPCPhrase(INITIAL_TRADE_WORLD_HELLO);
					break;
				case 1:
					NPCPhrase(SSQ_TRADE_WORLD_HELLO_1);
					break;
				case 2:
					NPCPhrase(SSQ_TRADE_WORLD_HELLO_2);
					break;
				case 3:
					NPCPhrase(SSQ_TRADE_WORLD_HELLO_3);
					break;
				case 4:
					NPCPhrase(SSQ_TRADE_WORLD_HELLO_4);
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
			NPCPhrase(SCAN_MAIDENS);
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_MAIDENS, static_cast<uint32_t>(1));
		}
		if ((uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP)
			 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP)
			 || uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP))
			&& !uqm::gameState().flag(uqm::GameFlag::SCANNED_FRAGMENTS))
		{
			if (uqm::gameState().flag(uqm::GameFlag::FRAGMENTS_BOUGHT) < 2)
			{
				NPCPhrase(SCAN_FRAGMENTS);
			}
			else
			{
				NPCPhrase(ENOUGH_FRAGMENTS);
			}
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_FRAGMENTS, static_cast<uint32_t>(1));
		}
		if (uqm::gameState().flag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP)
			&& !uqm::gameState().flag(uqm::GameFlag::SCANNED_CASTER))
		{
			NPCPhrase(SCAN_DRUUGE_CASTER);
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_CASTER, static_cast<uint32_t>(1));
		}
		if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP)
			&& !uqm::gameState().flag(uqm::GameFlag::SCANNED_SPAWNER))
		{
			NPCPhrase(SCAN_ARILOU_SPAWNER);
			uqm::gameState().setFlag(uqm::GameFlag::SCANNED_SPAWNER, static_cast<uint32_t>(1));
		}

		TradeWorld((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		if (uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS))
		{
			NPCPhrase(SUBSEQ_BOMB_WORLD_HELLO);
		}
		else
		{
			NPCPhrase(INIT_BOMB_WORLD_HELLO);
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
				NPCPhrase(HOSTILE_SPACE_HELLO_1);
				break;
			case 1:
				NPCPhrase(HOSTILE_SPACE_HELLO_2);
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
				const uint8_t which_module {uqm::gameState().sis().ModuleSlots[NumVisits]};
				if (which_module >= GUN_WEAPON
					&& which_module <= CANNON_WEAPON)
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
					NPCPhrase(SALVAGE_YOUR_SHIP_1);
					break;
				case 1:
					NPCPhrase(SALVAGE_YOUR_SHIP_2);
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
					NPCPhrase(INIT_SPACE_HELLO);
					SetHomeworldKnown(DRUUGE_HOME);
					break;
				case 1:
					NPCPhrase(SUBSEQUENT_SPACE_HELLO);
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
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
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
