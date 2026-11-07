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

#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"


namespace uqm
{
static ConversationData supox_desc =
	{
		Conversation::Supox, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SupoxPmapAnim, /* AlienFrame */
		SupoxFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SupoxColorMap, /* AlienColorMap */
		SupoxMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		SupoxConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				4, /* StartIndex */
				5, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				9,			  /* StartIndex */
				10,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				19,			  /* StartIndex */
				10,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				29,			  /* StartIndex */
				13,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
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
			3, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 15,
								0, /* FrameRate */
			GameTicksPerSecond / 12,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, SupoxPhrase::ByeNeutral))
	{
		NPCPhrase(SupoxPhrase::GoodbyeNeutral);
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatDoIDoNow))
	{
		NPCPhrase(SupoxPhrase::FixIt);
	}
	else if (PlayerSaid(R, SupoxPhrase::ThanksNowWeEatYou))
	{
		NPCPhrase(SupoxPhrase::HideousMonsters);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOSTILE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, SupoxPhrase::ByeAfterSpace))
	{
		NPCPhrase(SupoxPhrase::GoodbyeAfterSpace);
	}
	else if (PlayerSaid(R, SupoxPhrase::ByeBeforeSpace))
	{
		NPCPhrase(SupoxPhrase::GoodbyeBeforeSpace);
	}
	else if (PlayerSaid(R, SupoxPhrase::ByeAlliedHomeworld))
	{
		NPCPhrase(SupoxPhrase::GoodbyeAlliedHomeworld);
	}
	else if (PlayerSaid(R, SupoxPhrase::CanYouHelp))
	{
		NPCPhrase(SupoxPhrase::HowHelp);
		if (EscortFeasibilityStudy(RaceId::Supox) == 0)
		{
			NPCPhrase(SupoxPhrase::DontNeed);
		}
		else
		{
			NPCPhrase(SupoxPhrase::Have4Ships);

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(RaceId::Supox, difficultyCase(4, 4, 2));
			PrepareShip(RaceId::Supox);
		}
	}
}

static void AlliedHome(RESPONSE_REF R);

static void AlliedHome(RESPONSE_REF R)
{
	uint8_t NumVisits {};
	uint8_t News {};

	News = uqm::gameState().flag(uqm::GameFlag::SUPOX_WAR_NEWS);
	NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
	if (PlayerSaid(R, SupoxPhrase::HowWentWar))
	{
		NPCPhrase(SupoxPhrase::AboutBattle);

		News |= (1 << 0);
	}
	else if (PlayerSaid(R, SupoxPhrase::HowGoesWar))
	{
		if (NumVisits == 1)
		{
			NPCPhrase(SupoxPhrase::FleetOnWay);

			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(1));
		}
		else
		{
			switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_WAR_NEWS))
			{
				case 0:
					NPCPhrase(SupoxPhrase::BattleHappens1);
					News = 1;
					break;
				case 1:
					NPCPhrase(SupoxPhrase::BattleHappens2);
					News = 2;
					break;
			}
		}

		DisablePhrase(SupoxPhrase::HowGoesWar);
	}
	else if (PlayerSaid(R, SupoxPhrase::LearnNewInfo))
	{
		if (NumVisits < 5)
		{
			NPCPhrase(SupoxPhrase::NoNewInfo);
		}
		else
		{
			NPCPhrase(SupoxPhrase::Samatra);

			News |= (1 << 1);
		}

		DisablePhrase(SupoxPhrase::LearnNewInfo);
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatNowHomeworld))
	{
		if (NumVisits < 5)
		{
			NPCPhrase(SupoxPhrase::UpToYou);
		}
		else
		{
			NPCPhrase(SupoxPhrase::HopeKillEachOther);
		}

		DisablePhrase(SupoxPhrase::WhatNowHomeworld);
	}
	else if (PlayerSaid(R, SupoxPhrase::HowIsUltron))
	{
		NPCPhrase(SupoxPhrase::UltronIsGreat);

		DisablePhrase(SupoxPhrase::HowIsUltron);
	}
	else if (IsExtended() && PlayerSaid(R, SupoxPhrase::GiveInfo))
	{
		NPCPhrase(SupoxPhrase::GoodHints);

		DisablePhrase(SupoxPhrase::GiveInfo);
	}
	uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(News));

	if (NumVisits >= 5)
	{
		if (!(News & (1 << 0)))
		{
			Response(SupoxPhrase::HowWentWar, AlliedHome);
		}
	}
	else if (PhraseEnabled(SupoxPhrase::HowGoesWar)
			 && ((NumVisits == 1 && News == 0)
				 || (NumVisits && News < 2)))
	{
		Response(SupoxPhrase::HowGoesWar, AlliedHome);
	}
	if (PhraseEnabled(SupoxPhrase::LearnNewInfo))
	{
		Response(SupoxPhrase::LearnNewInfo, AlliedHome);
	}
	if (PhraseEnabled(SupoxPhrase::WhatNowHomeworld))
	{
		Response(SupoxPhrase::WhatNowHomeworld, AlliedHome);
	}
	if (PhraseEnabled(SupoxPhrase::HowIsUltron))
	{
		Response(SupoxPhrase::HowIsUltron, AlliedHome);
	}
	if (IsExtended() && PhraseEnabled(SupoxPhrase::GiveInfo))
	{
		Response(SupoxPhrase::GiveInfo, AlliedHome);
	}
	if (IsExtended() && ShipsReady(RaceId::Supox))
	{
		Response(SupoxPhrase::CanYouHelp, ExitConversation);
	}
	Response(SupoxPhrase::ByeAlliedHomeworld, ExitConversation);
}

static void BeforeKohrAh(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, SupoxPhrase::WhatsUpBeforeSpace))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SupoxPhrase::GeneralInfoBeforeSpace1);
				break;
			case 1:
				NPCPhrase(SupoxPhrase::GeneralInfoBeforeSpace2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(SupoxPhrase::WhatsUpBeforeSpace);
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatNowBeforeSpace))
	{
		NPCPhrase(SupoxPhrase::DoThisBeforeSpace);

		DisablePhrase(SupoxPhrase::WhatNowBeforeSpace);
	}

	if (PhraseEnabled(SupoxPhrase::WhatsUpBeforeSpace))
	{
		Response(SupoxPhrase::WhatsUpBeforeSpace, BeforeKohrAh);
	}
	if (PhraseEnabled(SupoxPhrase::WhatNowBeforeSpace))
	{
		Response(SupoxPhrase::WhatNowBeforeSpace, BeforeKohrAh);
	}
	Response(SupoxPhrase::ByeBeforeSpace, ExitConversation);
}

static void AfterKohrAh(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, SupoxPhrase::WhatsUpAfterSpace))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SupoxPhrase::GeneralInfoAfterSpace1);
				break;
			case 1:
				NPCPhrase(SupoxPhrase::GeneralInfoAfterSpace2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(SupoxPhrase::WhatsUpAfterSpace);
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatNowAfterSpace))
	{
		NPCPhrase(SupoxPhrase::DoThisAfterSpace);

		DisablePhrase(SupoxPhrase::WhatNowAfterSpace);
	}

	if (PhraseEnabled(SupoxPhrase::WhatsUpAfterSpace))
	{
		Response(SupoxPhrase::WhatsUpAfterSpace, AfterKohrAh);
	}
	if (PhraseEnabled(SupoxPhrase::WhatNowAfterSpace))
	{
		Response(SupoxPhrase::WhatNowAfterSpace, AfterKohrAh);
	}
	Response(SupoxPhrase::ByeAfterSpace, ExitConversation);
}

static void NeutralSupox(RESPONSE_REF R)
{
	uint8_t i {};
	uint8_t NumVisits {};
	SupoxPhrase pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = SupoxPhrase::NullPhrase;
	if (PlayerSaid(R, SupoxPhrase::IAm))
	{
		NPCPhrase(SupoxPhrase::WeAreSupox);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(1));
		DisablePhrase(SupoxPhrase::IAm);
	}
	else if (PlayerSaid(R, SupoxPhrase::MyShip))
	{
		NPCPhrase(SupoxPhrase::OurShip);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(2));
		DisablePhrase(SupoxPhrase::MyShip);
	}
	else if (PlayerSaid(R, SupoxPhrase::FromAlliance))
	{
		NPCPhrase(SupoxPhrase::FromSupox);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(3));
		DisablePhrase(SupoxPhrase::FromAlliance);
	}
	else if (PlayerSaid(R, SupoxPhrase::AreYouCopying))
	{
		NPCPhrase(SupoxPhrase::YeahSorry);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, SupoxPhrase::WhyCopy))
	{
		NPCPhrase(SupoxPhrase::Symbiots);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, SupoxPhrase::TellUsOfYourSpecies))
	{
		NPCPhrase(SupoxPhrase::OurSpecies);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SupoxPhrase::PlantsArentIntelligent))
	{
		NPCPhrase(SupoxPhrase::ProvesWereSpecial);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK2, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SupoxPhrase::AnyoneAroundHere))
	{
		NPCPhrase(SupoxPhrase::UtwigNearby);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(1));
		StartSphereTracking(RaceId::Utwig);
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatRelationToUtwig))
	{
		NPCPhrase(SupoxPhrase::UtwigAllies);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatsWrongWithUtwig))
	{
		NPCPhrase(SupoxPhrase::BrokeUltron);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatsUltron))
	{
		NPCPhrase(SupoxPhrase::TakeUltron);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(1));

		Response(SupoxPhrase::WhatDoIDoNow, (IsExtended() ? NeutralSupox : ExitConversation));
		Response(SupoxPhrase::ThanksNowWeEatYou, ExitConversation);

		return;
	}
	else if (PlayerSaid(R, SupoxPhrase::WhatDoIDoNow))
	{
		NPCPhrase(SupoxPhrase::FixIt);
	}
	else if (PlayerSaid(R, SupoxPhrase::GotFixedUltron))
	{
		NPCPhrase(SupoxPhrase::GoodGiveToUtwig);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SupoxPhrase::LookIRepairedLots))
	{
		NPCPhrase(SupoxPhrase::AlmostThere);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SupoxPhrase::LookISlightlyRepaired))
	{
		NPCPhrase(SupoxPhrase::GreatDoMore);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SupoxPhrase::WhereGetRepairs))
	{
		NPCPhrase(SupoxPhrase::AncientRhyme);

		if (IsExtended())
		{
			StartSphereTracking(RaceId::Druuge);
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SupoxPhrase::GiveInfo))
	{
		NPCPhrase(SupoxPhrase::GoodHints);

		DisablePhrase(SupoxPhrase::GiveInfo);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_STACK2))
	{
		case 0:
			pStr[1] = SupoxPhrase::TellUsOfYourSpecies;
			break;
		case 1:
			pStr[1] = SupoxPhrase::PlantsArentIntelligent;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_STACK1))
	{
		case 0:
			pStr[0] = SupoxPhrase::IAm;
			pStr[1] = SupoxPhrase::NullPhrase;
			break;
		case 1:
			pStr[0] = SupoxPhrase::MyShip;
			pStr[1] = SupoxPhrase::NullPhrase;
			break;
		case 2:
			pStr[0] = SupoxPhrase::FromAlliance;
			pStr[1] = SupoxPhrase::NullPhrase;
			break;
		case 3:
			pStr[0] = SupoxPhrase::AreYouCopying;
			pStr[1] = SupoxPhrase::NullPhrase;
			break;
		case 4:
			pStr[0] = SupoxPhrase::WhyCopy;
			pStr[1] = SupoxPhrase::NullPhrase;
			break;
	}
	NumVisits = uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION);
	if (NumVisits == 0)
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_WAR_NEWS))
		{
			case 0:
				if (uqm::gameState().flag(uqm::GameFlag::UTWIG_VISITS)
					|| uqm::gameState().flag(uqm::GameFlag::UTWIG_HOME_VISITS)
					|| uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS))
				{
					pStr[2] = SupoxPhrase::WhatRelationToUtwig;
				}
				else
				{
					pStr[2] = SupoxPhrase::AnyoneAroundHere;
				}
				break;
			case 1:
				pStr[2] = SupoxPhrase::WhatsWrongWithUtwig;
				break;
			case 2:
				pStr[2] = SupoxPhrase::WhatsUltron;
				break;
		}
	}
	if (pStr[LastStack] != SupoxPhrase::NullPhrase)
	{
		Response(pStr[LastStack], NeutralSupox);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && *pStr[i] != SupoxPhrase::NullPhrase)
		{
			Response(pStr[i], NeutralSupox);
		}
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SUPOX_ULTRON_HELP))
	{
		switch (NumVisits)
		{
			case 1:
				Response(SupoxPhrase::WhereGetRepairs, NeutralSupox);
				break;
			case 2:
				Response(SupoxPhrase::LookISlightlyRepaired, NeutralSupox);
				break;
			case 3:
				Response(SupoxPhrase::LookIRepairedLots, NeutralSupox);
				break;
			case 4:
				Response(SupoxPhrase::GotFixedUltron, NeutralSupox);
				break;
		}
	}
	if (IsExtended() && NumVisits && PhraseEnabled(SupoxPhrase::GiveInfo))
	{
		Response(SupoxPhrase::GiveInfo, NeutralSupox);
	}
	Response(SupoxPhrase::ByeNeutral, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(SupoxPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::SUPOX_HOSTILE))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SupoxPhrase::HostileSpaceHello1);
				break;
			case 1:
				NPCPhrase(SupoxPhrase::HostileSpaceHello2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Peace);
	}
	else if (CheckAlliance(RaceId::Supox) == RaceAllegiance::Good)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(SupoxPhrase::AlliedHomeworldHello1);
					break;
				case 1:
					NPCPhrase(SupoxPhrase::AlliedHomeworldHello2);
					break;
				case 2:
					NPCPhrase(SupoxPhrase::AlliedHomeworldHello3);
					break;
				case 3:
					NPCPhrase(SupoxPhrase::AlliedHomeworldHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			AlliedHome((RESPONSE_REF)0);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
			if (NumVisits == 1)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(SupoxPhrase::HelloBeforeKohrahSpace1);
						break;
					case 1:
						NPCPhrase(SupoxPhrase::HelloBeforeKohrahSpace2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));

				BeforeKohrAh((RESPONSE_REF)0);
			}
			else if (NumVisits < 5)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(SupoxPhrase::HelloDuringKohrahSpace1);
						break;
					case 1:
						NPCPhrase(SupoxPhrase::HelloDuringKohrahSpace2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(SupoxPhrase::HelloAfterKohrahSpace1);
						break;
					case 1:
						NPCPhrase(SupoxPhrase::HelloAfterKohrahSpace2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));

				AfterKohrAh((RESPONSE_REF)0);
			}
		}
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(SupoxPhrase::NeutralHomeworldHello1);
					break;
				case 1:
					NPCPhrase(SupoxPhrase::NeutralHomeworldHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			SetHomeworldKnown(*HomeId::Supox);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(SupoxPhrase::NeutralSpaceHello1);
					break;
				case 1:
					NPCPhrase(SupoxPhrase::NeutralSpaceHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));
		}

		NeutralSupox((RESPONSE_REF)0);
	}
}

static uint16_t uninit_supox(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_supox_enc(void)
{
	// nothing defined so far
}

ConversationData* init_supox_comm(void)
{
	supox_desc.init_encounter_func = Intro;
	supox_desc.post_encounter_func = post_supox_enc;
	supox_desc.uninit_encounter_func = uninit_supox;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	supox_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	supox_desc.AlienTextBaseline.y = 0;
	supox_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (!uqm::gameState().flag(uqm::GameFlag::SUPOX_HOSTILE)
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&supox_desc};

	return (retval);
}

} // namespace uqm
