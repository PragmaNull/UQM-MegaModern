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
#include "uqm/gameev.h"


static ConversationData pkunk_desc =
	{
		Conversation::Pkunk, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		PkunkPmapAnim, /* AlienFrame */
		PkunkFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		PkunkColorMap, /* AlienColorMap */
		PkunkMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		PkunkConversationPhrases, /* PlayerPhrases */
		3, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				3, /* StartIndex */
				4, /* NumFrames */
				CircularAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				7, /* StartIndex */
				4, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2),				/* BlockMask */
			},
								{
				11, /* StartIndex */
				3,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 1),				/* BlockMask */
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
			2, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 15,
								GameTicksPerSecond / 6, /* FrameRate */
			GameTicksPerSecond / 12,
								GameTicksPerSecond / 2, /* RestartRate */
			0,						/* BlockMask */
		},
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static constexpr auto GoodReason1 {1 << 0};
static constexpr auto GoodReason2 {1 << 1};
static constexpr auto BadReason1 {1 << 2};
static constexpr auto BadReason2 {1 << 3};

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, PkunkPhrase::FriendlyByeSpace))
	{
		NPCPhrase(PkunkPhrase::FriendlyGoodbyeSpace);
	}
	else if (PlayerSaid(R, PkunkPhrase::NeutralByeSpace))
	{
		NPCPhrase(PkunkPhrase::NeutralGoodbyeSpace);
	}
	else if (PlayerSaid(R, PkunkPhrase::ByeAngry))
	{
		NPCPhrase(PkunkPhrase::GoodbyeAngry);
	}
	else if (PlayerSaid(R, PkunkPhrase::ByeFriendly))
	{
		NPCPhrase(PkunkPhrase::GoodbyeFriendly);
	}
	else if (PlayerSaid(R, PkunkPhrase::WeHereToHelp)
			 || PlayerSaid(R, PkunkPhrase::WeNeedHelp))
	{
		if (PlayerSaid(R, PkunkPhrase::WeHereToHelp))
		{
			NPCPhrase(PkunkPhrase::NeedHelp);
		}
		else
		{
			NPCPhrase(PkunkPhrase::GiveHelp);
		}
		NPCPhrase(PkunkPhrase::AlmostAlliance);

		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(0));

		AddEvent(EventType::Relative, 6, 0, 0, GameEventId::AdvancePkunkMission);

		if (EscortFeasibilityStudy(RaceId::Pkunk) == 0)
		{
			NPCPhrase(PkunkPhrase::InitNoRoom);
		}
		else
		{
			NPCPhrase(PkunkPhrase::InitShipGift);
			AlienTalkSegue((uint16_t)~0);
			if (AddEscortShips(RaceId::Pkunk, difficultyCase(4, 4, 2)))
			{
				PrepareShip(RaceId::Pkunk);
			}
		}
	}
	else if (PlayerSaid(R, PkunkPhrase::TryToBeNicer))
	{
		NPCPhrase(PkunkPhrase::CantAskForMore);
		NPCPhrase(PkunkPhrase::VisitOurHomeworld);

		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, PkunkPhrase::MustConquer)
			 || PlayerSaid(R, PkunkPhrase::Obey))
	{
		if (PlayerSaid(R, PkunkPhrase::Obey))
		{
			NPCPhrase(PkunkPhrase::NoObey);
		}
		else
		{
			NPCPhrase(PkunkPhrase::BadIdea);

			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(2));
		}
		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, PkunkPhrase::DieIdiotFools))
	{
		NPCPhrase(PkunkPhrase::VeryWell);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, PkunkPhrase::SuitYourself))
	{
		NPCPhrase(PkunkPhrase::GoodbyeMigration);
	}
	else
	{
		uint8_t ReasonMask {};

		ReasonMask = uqm::gameState().flag(uqm::GameFlag::PKUNK_REASONS);
		if (PlayerSaid(R, PkunkPhrase::GoodReason1))
		{
			NPCPhrase(PkunkPhrase::WeGoHome1);
			ReasonMask |= GoodReason1;
			AddEvent(EventType::Relative, 0, 0, 0, GameEventId::AdvancePkunkMission);
		}
		else if (PlayerSaid(R, PkunkPhrase::GoodReason2))
		{
			NPCPhrase(PkunkPhrase::WeGoHome2);
			ReasonMask |= GoodReason2;
			AddEvent(EventType::Relative, 0, 0, 0, GameEventId::AdvancePkunkMission);
		}
		else if (PlayerSaid(R, PkunkPhrase::BadReason1))
		{
			NPCPhrase(PkunkPhrase::NoGoHome1);
			ReasonMask |= BadReason1;
		}
		else if (PlayerSaid(R, PkunkPhrase::BadReason2))
		{
			NPCPhrase(PkunkPhrase::NoGoHome2);
			ReasonMask |= BadReason2;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_REASONS, static_cast<uint32_t>(ReasonMask));
	}
}

static void PkunkHome(RESPONSE_REF R);

static void PkunkAngry(RESPONSE_REF R)
{
	if (PlayerSaid(R, PkunkPhrase::WhyInsults))
	{
		NPCPhrase(PkunkPhrase::ReleaseTension);

		DisablePhrase(PkunkPhrase::WhyInsults);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhatAboutYou))
	{
		NPCPhrase(PkunkPhrase::AboutUs);

		DisablePhrase(PkunkPhrase::WhatAboutYou);
	}
	else if (PlayerSaid(R, PkunkPhrase::ShouldBeFriends))
	{
		NPCPhrase(PkunkPhrase::YesFriends);

		DisablePhrase(PkunkPhrase::ShouldBeFriends);
	}

	if (PhraseEnabled(PkunkPhrase::ShouldBeFriends))
	{
		Response(PkunkPhrase::ShouldBeFriends, PkunkAngry);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			Response(PkunkPhrase::TryToBeNicer, PkunkHome);
		}
		else
		{
			Response(PkunkPhrase::TryToBeNicer, ExitConversation);
		}
	}
	Response(PkunkPhrase::DieIdiotFools, ExitConversation);
	if (PhraseEnabled(PkunkPhrase::WhyInsults))
	{
		Response(PkunkPhrase::WhyInsults, PkunkAngry);
	}
	if (PhraseEnabled(PkunkPhrase::WhatAboutYou))
	{
		Response(PkunkPhrase::WhatAboutYou, PkunkAngry);
	}
	Response(PkunkPhrase::ByeAngry, ExitConversation);
}

static void DiscussConquer(RESPONSE_REF R)
{
	if (PlayerSaid(R, PkunkPhrase::WeConquer))
	{
		NPCPhrase(PkunkPhrase::WhyConquer);

		DisablePhrase(PkunkPhrase::WeConquer);
	}
	else if (PlayerSaid(R, PkunkPhrase::ConquerBecause1))
	{
#if 0
		NPCPhrase (PkunkPhrase::NotConquer10);
		NPCPhrase (GLOBAL_ALLIANCE_NAME + PkunkPhrase::Name1);
		NPCPhrase (PkunkPhrase::NotConquer11);
		NPCPhrase (GLOBAL_ALLIANCE_NAME + PkunkPhrase::Name1);
		NPCPhrase (PkunkPhrase::NotConquer12);
#endif
		NPCPhrase(PkunkPhrase::NotConquer1);

		DisablePhrase(PkunkPhrase::ConquerBecause1);
	}
	else if (PlayerSaid(R, PkunkPhrase::ConquerBecause2))
	{
		NPCPhrase(PkunkPhrase::NotConquer2);

		DisablePhrase(PkunkPhrase::ConquerBecause2);
	}

	if (PhraseEnabled(PkunkPhrase::ConquerBecause1))
	{
#if 0
		char buf[ALLIANCE_NAME_BUFSIZE];

		GetAllianceName (buf, PkunkPhrase::Name1);
		construct_response (
				shared_phrase_buf,
				PkunkPhrase::ConquerBecause1,
				buf,
				(RESPONSE_REF)-1);
		DoResponsePhrase (PkunkPhrase::ConquerBecause1, DiscussConquer, shared_phrase_buf);
#endif
		Response(PkunkPhrase::ConquerBecause1, DiscussConquer);
	}
	if (PhraseEnabled(PkunkPhrase::ConquerBecause2))
	{
		Response(PkunkPhrase::ConquerBecause2, DiscussConquer);
	}
	Response(PkunkPhrase::MustConquer, ExitConversation);
	Response(PkunkPhrase::NoConquest, PkunkHome);
}

static void OfferAlliance(RESPONSE_REF R)
{
	if (PlayerSaid(R, PkunkPhrase::WeAreVindicator))
	{
		NPCPhrase(PkunkPhrase::WhyYouHere);
	}
	else if (PlayerSaid(R, PkunkPhrase::ExploringUniverse))
	{
		NPCPhrase(PkunkPhrase::SenseDeeperConflict);

		DisablePhrase(PkunkPhrase::ExploringUniverse);
	}
	else if (PlayerSaid(R, PkunkPhrase::FunCruise))
	{
		NPCPhrase(PkunkPhrase::Repress);

		DisablePhrase(PkunkPhrase::FunCruise);
	}

	Response(PkunkPhrase::WeHereToHelp, ExitConversation);
	Response(PkunkPhrase::WeNeedHelp, ExitConversation);
	if (PhraseEnabled(PkunkPhrase::ExploringUniverse))
	{
		Response(PkunkPhrase::ExploringUniverse, OfferAlliance);
	}
	if (PhraseEnabled(PkunkPhrase::FunCruise))
	{
		Response(PkunkPhrase::FunCruise, OfferAlliance);
	}
}

static void AboutPkunk(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, PkunkPhrase::WhatAboutYou))
	{
		NPCPhrase(PkunkPhrase::AboutUs);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhatAboutHistory))
	{
		NPCPhrase(PkunkPhrase::AboutHistory);

		DisablePhrase(PkunkPhrase::WhatAboutHistory);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhatAboutYehat))
	{
		NPCPhrase(PkunkPhrase::AboutYehat);

		DisablePhrase(PkunkPhrase::WhatAboutYehat);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhatAboutCulture))
	{
		NPCPhrase(PkunkPhrase::AboutCulture);

		DisablePhrase(PkunkPhrase::WhatAboutCulture);
	}
	else if (PlayerSaid(R, PkunkPhrase::ElaborateCulture))
	{
		NPCPhrase(PkunkPhrase::OkElaborateCulture);

		DisablePhrase(PkunkPhrase::ElaborateCulture);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhatAboutFuture))
	{
		NPCPhrase(PkunkPhrase::AboutFuture);

		DisablePhrase(PkunkPhrase::WhatAboutFuture);
	}

	if (PhraseEnabled(PkunkPhrase::WhatAboutHistory))
	{
		Response(PkunkPhrase::WhatAboutHistory, AboutPkunk);
		InfoLeft = true;
	}
	else if (PhraseEnabled(PkunkPhrase::WhatAboutYehat))
	{
		Response(PkunkPhrase::WhatAboutYehat, AboutPkunk);
		InfoLeft = true;
	}
	if (PhraseEnabled(PkunkPhrase::WhatAboutCulture))
	{
		Response(PkunkPhrase::WhatAboutCulture, AboutPkunk);
		InfoLeft = true;
	}
	else if (PhraseEnabled(PkunkPhrase::ElaborateCulture))
	{
		Response(PkunkPhrase::ElaborateCulture, AboutPkunk);
		InfoLeft = true;
	}
	if (PhraseEnabled(PkunkPhrase::WhatAboutFuture))
	{
		Response(PkunkPhrase::WhatAboutFuture, AboutPkunk);
		InfoLeft = true;
	}
	Response(PkunkPhrase::EnoughAboutYou, PkunkHome);

	if (!InfoLeft)
	{
		DisablePhrase(PkunkPhrase::WhatAboutYou);
	}
}

static void AboutIlwrath(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, PkunkPhrase::WhatAboutIlwrath))
	{
		NPCPhrase(PkunkPhrase::AboutIlwrath);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhyIlwrathFight))
	{
		NPCPhrase(PkunkPhrase::IlwrathFightBecause);

		DisablePhrase(PkunkPhrase::WhyIlwrathFight);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhenFightStart))
	{
		NPCPhrase(PkunkPhrase::FightStartWhen);

		DisablePhrase(PkunkPhrase::WhenFightStart);
	}
	else if (PlayerSaid(R, PkunkPhrase::HowGoesFight))
	{
		NPCPhrase(PkunkPhrase::FightGoes);

		DisablePhrase(PkunkPhrase::HowGoesFight);
	}
	else if (PlayerSaid(R, PkunkPhrase::HowStopFight))
	{
		NPCPhrase(PkunkPhrase::StopFightLikeSo);

		DisablePhrase(PkunkPhrase::HowStopFight);
	}

	if (PhraseEnabled(PkunkPhrase::WhyIlwrathFight))
	{
		Response(PkunkPhrase::WhyIlwrathFight, AboutIlwrath);
		InfoLeft = true;
	}
	if (PhraseEnabled(PkunkPhrase::WhenFightStart))
	{
		Response(PkunkPhrase::WhenFightStart, AboutIlwrath);
		InfoLeft = true;
	}
	if (PhraseEnabled(PkunkPhrase::HowGoesFight))
	{
		Response(PkunkPhrase::HowGoesFight, AboutIlwrath);
		InfoLeft = true;
	}
	if (PhraseEnabled(PkunkPhrase::HowStopFight))
	{
		Response(PkunkPhrase::HowStopFight, AboutIlwrath);
		InfoLeft = true;
	}
	Response(PkunkPhrase::EnoughIlwrath, PkunkHome);

	if (!InfoLeft)
	{
		DisablePhrase(PkunkPhrase::WhatAboutIlwrath);
	}
}

static void PkunkHome(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, PkunkPhrase::NoConquest))
	{
		NPCPhrase(PkunkPhrase::GoodIdea);
	}
	else if (PlayerSaid(R, PkunkPhrase::EnoughIlwrath))
	{
		NPCPhrase(PkunkPhrase::OkEnoughIlwrath);
	}
	else if (PlayerSaid(R, PkunkPhrase::EnoughAboutYou))
	{
		NPCPhrase(PkunkPhrase::OkEnoughAboutUs);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhereFleet1)
			 || PlayerSaid(R, PkunkPhrase::WhereFleet2)
			 || PlayerSaid(R, PkunkPhrase::WhereFleet3))
	{
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SWITCH, static_cast<uint32_t>(1));
		if (!(uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION) & 1))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_RETURN);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PkunkPhrase::ReturningFromYehat1);
					break;
				case 1:
					NPCPhrase(PkunkPhrase::ReturningFromYehat2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_RETURN, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_MIGRATE);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PkunkPhrase::MigratingHomeworld1);
					break;
				case 1:
					NPCPhrase(PkunkPhrase::MigratingHomeworld2);
					break;
				case 2:
					NPCPhrase(PkunkPhrase::MigratingHomeworld3);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MIGRATE, static_cast<uint32_t>(NumVisits));
		}

		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_FLEET) + 1;
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_FLEET, static_cast<uint32_t>(NumVisits));

		DisablePhrase(PkunkPhrase::WhereFleet1);
	}
	else if (PlayerSaid(R, PkunkPhrase::AmWorried1)
			 || PlayerSaid(R, PkunkPhrase::AmWorried2)
			 || PlayerSaid(R, PkunkPhrase::AmWorried3))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_WORRY);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PkunkPhrase::DontWorry1);
				break;
			case 1:
				NPCPhrase(PkunkPhrase::DontWorry2);
				break;
			case 2:
				NPCPhrase(PkunkPhrase::DontWorry3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_WORRY, static_cast<uint32_t>(NumVisits));

		DisablePhrase(PkunkPhrase::AmWorried1);
	}
	else if (PlayerSaid(R, PkunkPhrase::TryToBeNicer))
	{
		NPCPhrase(PkunkPhrase::CantAskForMore);
		if (!uqm::gameState().flag(uqm::GameFlag::CLEAR_SPINDLE))
		{
			NPCPhrase(PkunkPhrase::GiveSpindle);

			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP, static_cast<uint32_t>(1));
		}
		NPCPhrase(PkunkPhrase::CanBeFriends);
		SetHomeworldKnown(*HomeId::Pkunk);

		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, PkunkPhrase::WhatAboutIlwrath))
	{
		NPCPhrase(PkunkPhrase::AboutIlwrath /* ILWRATH_GONE */);

		DisablePhrase(PkunkPhrase::WhatAboutIlwrath);
	}

	if (PhraseEnabled(PkunkPhrase::WeConquer) && uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 0)
	{
		Response(PkunkPhrase::WeConquer, DiscussConquer);
	}
	if (uqm::gameState().flag(uqm::GameFlag::PKUNK_ON_THE_MOVE))
	{
		if (PhraseEnabled(PkunkPhrase::WhereFleet1) && !uqm::gameState().flag(uqm::GameFlag::PKUNK_SWITCH))
		{
			switch (uqm::gameState().flag(uqm::GameFlag::PKUNK_FLEET))
			{
				case 0:
					Response(PkunkPhrase::WhereFleet1, PkunkHome);
					break;
				case 1:
					Response(PkunkPhrase::WhereFleet2, PkunkHome);
					break;
				case 2:
					Response(PkunkPhrase::WhereFleet3, PkunkHome);
					break;
			}
		}
		else if (!PhraseEnabled(PkunkPhrase::WhereFleet1)
				 && PhraseEnabled(PkunkPhrase::AmWorried1)
				 && (uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION) & 1))
		{
			switch (uqm::gameState().flag(uqm::GameFlag::PKUNK_WORRY))
			{
				case 0:
					Response(PkunkPhrase::AmWorried1, PkunkHome);
					break;
				case 1:
					Response(PkunkPhrase::AmWorried2, PkunkHome);
					break;
				case 2:
					Response(PkunkPhrase::AmWorried3, PkunkHome);
					break;
			}
		}
	}
	if (!uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_MONTH))
	{
		Response(PkunkPhrase::WeAreVindicator, OfferAlliance);
	}
	if (PhraseEnabled(PkunkPhrase::WhatAboutYou))
	{
		Response(PkunkPhrase::WhatAboutYou, AboutPkunk);
	}
	if (PhraseEnabled(PkunkPhrase::WhatAboutIlwrath))
	{
		if (!uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			Response(PkunkPhrase::WhatAboutIlwrath, AboutIlwrath);
		}
		else
		{
			Response(PkunkPhrase::WhatAboutIlwrath, PkunkHome);
		}
	}
	Response(PkunkPhrase::ByeFriendly, ExitConversation);
}

static void PkunkFriendlySpace(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, PkunkPhrase::WhatsUpSpace))
	{
		if (ShipsReady(RaceId::Pkunk) && uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 3)
		{
			NPCPhrase(PkunkPhrase::ShipsAtHome);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_INFO);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace1);
					break;
				case 1:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace2);
					break;
				case 2:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace3);
					break;
				case 3:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace4);
					break;
				case 4:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace5);
					break;
				case 5:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace6);
					break;
				case 6:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace7);
					break;
				case 7:
					NPCPhrase(PkunkPhrase::GeneralInfoSpace8);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(NumVisits));
		}

		DisablePhrase(PkunkPhrase::WhatsUpSpace);
	}
	else if (PlayerSaid(R, PkunkPhrase::HowGoesWar))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_WAR);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PkunkPhrase::WarGoes1);
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(1));
				break;
			case 1:
				NPCPhrase(PkunkPhrase::WarGoes2);
				break;
			case 2:
				NPCPhrase(PkunkPhrase::WarGoes3);
				break;
			case 3:
				NPCPhrase(PkunkPhrase::WarGoes4);
				uqm::gameState().setFlag(uqm::GameFlag::PKUNK_DONE_WAR, static_cast<uint32_t>(1));
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_WAR, static_cast<uint32_t>(NumVisits));

		DisablePhrase(PkunkPhrase::HowGoesWar);
	}
	else if (PlayerSaid(R, PkunkPhrase::TellMyFortune))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_FORTUNE);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PkunkPhrase::FortuneIs1);
				break;
			case 1:
				NPCPhrase(PkunkPhrase::FortuneIs2);
				break;
			case 2:
				NPCPhrase(PkunkPhrase::FortuneIs3);
				break;
			case 3:
				NPCPhrase(PkunkPhrase::FortuneIs4);
				break;
			case 4:
				NPCPhrase(PkunkPhrase::FortuneIs5);
				break;
			case 5:
				NPCPhrase(PkunkPhrase::FortuneIs6);
				break;
			case 6:
				NPCPhrase(PkunkPhrase::FortuneIs7);
				break;
			case 7:
				NPCPhrase(PkunkPhrase::FortuneIs8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_FORTUNE, static_cast<uint32_t>(NumVisits));

		DisablePhrase(PkunkPhrase::TellMyFortune);
	}

	if (PhraseEnabled(PkunkPhrase::WhatsUpSpace))
	{
		Response(PkunkPhrase::WhatsUpSpace, PkunkFriendlySpace);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::PKUNK_DONE_WAR) && PhraseEnabled(PkunkPhrase::HowGoesWar))
	{
		Response(PkunkPhrase::HowGoesWar, PkunkFriendlySpace);
	}
	if (PhraseEnabled(PkunkPhrase::TellMyFortune))
	{
		Response(PkunkPhrase::TellMyFortune, PkunkFriendlySpace);
	}
	Response(PkunkPhrase::FriendlyByeSpace, ExitConversation);
}

static void PkunkNeutralSpace(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, PkunkPhrase::FormAlliance))
	{
		NPCPhrase(PkunkPhrase::GoToHomeworld);
		SetHomeworldKnown(*HomeId::Pkunk);

		DisablePhrase(PkunkPhrase::FormAlliance);
	}
	else if (PlayerSaid(R, PkunkPhrase::CanYouHelp))
	{
		NPCPhrase(PkunkPhrase::GoToHomeworldAgain);
		SetHomeworldKnown(*HomeId::Pkunk);

		DisablePhrase(PkunkPhrase::CanYouHelp);
	}
	else if (PlayerSaid(R, PkunkPhrase::HostileGreeting))
	{
		NPCPhrase(PkunkPhrase::DontBeHostile);

		DisablePhrase(PkunkPhrase::HostileGreeting);
	}
	else if (PlayerSaid(R, PkunkPhrase::WhatsUpNeutral))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PkunkPhrase::GeneralInfoSpace1);
				break;
			case 1:
				NPCPhrase(PkunkPhrase::GeneralInfoSpace2);
				break;
			case 2:
				NPCPhrase(PkunkPhrase::GeneralInfoSpace6 /* was 3 */);
				break;
			case 3:
				NPCPhrase(PkunkPhrase::GeneralInfoSpace7 /* was 4 */);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(PkunkPhrase::WhatsUpNeutral);
	}

	if (PhraseEnabled(PkunkPhrase::FormAlliance))
	{
		Response(PkunkPhrase::FormAlliance, PkunkNeutralSpace);
	}
	else if (PhraseEnabled(PkunkPhrase::CanYouHelp))
	{
		Response(PkunkPhrase::CanYouHelp, PkunkNeutralSpace);
	}
	if (PhraseEnabled(PkunkPhrase::HostileGreeting))
	{
		Response(PkunkPhrase::HostileGreeting, PkunkNeutralSpace);
	}
	else
	{
		Response(PkunkPhrase::Obey, ExitConversation);
	}
	if (PhraseEnabled(PkunkPhrase::WhatsUpNeutral))
	{
		Response(PkunkPhrase::WhatsUpNeutral, PkunkNeutralSpace);
	}
	Response(PkunkPhrase::NeutralByeSpace, ExitConversation);
}

static void PkunkMigrate(RESPONSE_REF R)
{
	uint8_t ReasonMask {};
	(void)R; // ignored

	ReasonMask = uqm::gameState().flag(uqm::GameFlag::PKUNK_REASONS);
	if (!(ReasonMask & GoodReason1))
	{
		Response(PkunkPhrase::GoodReason1, ExitConversation);
	}
	if (!(ReasonMask & BadReason1))
	{
		Response(PkunkPhrase::BadReason1, ExitConversation);
	}
	if (!(ReasonMask & GoodReason2))
	{
		Response(PkunkPhrase::GoodReason2, ExitConversation);
	}
	if (!(ReasonMask & BadReason2))
	{
		Response(PkunkPhrase::BadReason2, ExitConversation);
	}
	Response(PkunkPhrase::SuitYourself, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};
	uint8_t Manner {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(PkunkPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	Manner = uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER);
	if (Manner == 2)
	{
		// Irreparably Pissed off the Pkunk.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PkunkPhrase::HateYouForever1);
				break;
			case 1:
				NPCPhrase(PkunkPhrase::HateYouForever2);
				break;
			case 2:
				NPCPhrase(PkunkPhrase::HateYouForever3);
				break;
			case 3:
				NPCPhrase(PkunkPhrase::HateYouForever4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else if (Manner == 1)
	{
		// Bad relations with the Pkunk, but not irreparably.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PkunkPhrase::SpiritualProblems1);
				break;
			case 1:
				NPCPhrase(PkunkPhrase::SpiritualProblems2);
				break;
			case 2:
				NPCPhrase(PkunkPhrase::SpiritualProblems3);
				break;
			case 3:
				NPCPhrase(PkunkPhrase::SpiritualProblems4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(NumVisits));

		PkunkAngry((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		// Encountering the Pkunk at their home world.
		if (!uqm::gameState().flag(uqm::GameFlag::CLEAR_SPINDLE))
		{
			NPCPhrase(PkunkPhrase::GiveSpindle);

			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP, static_cast<uint32_t>(1));
		}
		else if (!uqm::gameState().flag(uqm::GameFlag::PKUNK_SENSE_VICTOR)
				 && uqm::gameState().clock().year() > StartYear
				 && !uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
		{
			NPCPhrase(PkunkPhrase::SenseKohrahVictory);

			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SENSE_VICTOR, static_cast<uint32_t>(1));
		}

		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS);
		if (Manner == 0)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PkunkPhrase::NeutralHomeworldHello1);
					break;
				case 1:
					NPCPhrase(PkunkPhrase::NeutralHomeworldHello2);
					break;
				case 2:
					NPCPhrase(PkunkPhrase::NeutralHomeworldHello3);
					break;
				case 3:
					NPCPhrase(PkunkPhrase::NeutralHomeworldHello4);
					--NumVisits;
					break;
			}
		}
		else
		{
			if (NumVisits && ShipsReady(RaceId::Pkunk) && uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 3)
			{
				if (EscortFeasibilityStudy(RaceId::Pkunk) == 0)
				{
					NPCPhrase(PkunkPhrase::NoRoom);
				}
				else
				{
					NPCPhrase(PkunkPhrase::ShipGift);
					if (AddEscortShips(RaceId::Pkunk, difficultyCase(4, 4, 2)))
					{
						PrepareShip(RaceId::Pkunk);
					}
				}
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello1);
						break;
					case 1:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello2);
						break;
					case 2:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello3);
						break;
					case 3:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello4);
						break;
					case 4:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello5);
						break;
					case 5:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello6);
						break;
					case 6:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello7);
						break;
					case 7:
						NPCPhrase(PkunkPhrase::FriendlyHomeworldHello8);
						--NumVisits;
						break;
				}
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		PkunkHome((RESPONSE_REF)0);
	}
	else if ((NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION)) == 0
			 || !(NumVisits & 1))
	{
		// Encountering a Pkunk ship in space, while they are not
		// migrating.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS);
		if (Manner == 3)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello1);
					break;
				case 1:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello2);
					break;
				case 2:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello3);
					break;
				case 3:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello4);
					break;
				case 4:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello5);
					break;
				case 5:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello6);
					break;
				case 6:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello7);
					break;
				case 7:
					NPCPhrase(PkunkPhrase::FriendlySpaceHello8);
					--NumVisits;
					break;
			}

			PkunkFriendlySpace((RESPONSE_REF)0);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PkunkPhrase::NeutralSpaceHello1);
					break;
				case 1:
					NPCPhrase(PkunkPhrase::NeutralSpaceHello2);
					break;
				case 2:
					NPCPhrase(PkunkPhrase::NeutralSpaceHello3);
					break;
				case 3:
					NPCPhrase(PkunkPhrase::NeutralSpaceHello4);
					SetHomeworldKnown(*HomeId::Pkunk);
					--NumVisits;
					break;
			}

			PkunkNeutralSpace((RESPONSE_REF)0);
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else
	{
		// Encountering a Pkunk ship in space, while they are
		// migrating.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_MIGRATE_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PkunkPhrase::MigratingSpace1);
				break;
			case 1:
				NPCPhrase(PkunkPhrase::MigratingSpace2);
				break;
			case 2:
				NPCPhrase(PkunkPhrase::MigratingSpace3);
				break;
			case 3:
				NPCPhrase(PkunkPhrase::MigratingSpace4);
				break;
			case 4:
				NPCPhrase(PkunkPhrase::MigratingSpace5);
				break;
			case 5:
				NPCPhrase(PkunkPhrase::MigratingSpace6);
				break;
			case 6:
				NPCPhrase(PkunkPhrase::MigratingSpace7);
				break;
			case 7:
				NPCPhrase(PkunkPhrase::MigratingSpace8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MIGRATE_VISITS, static_cast<uint32_t>(NumVisits));

		PkunkMigrate((RESPONSE_REF)0);
	}
}

// Called after combat or communications
static uint16_t uninit_pkunk(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_pkunk_enc(void)
{
	uint8_t Manner {};

	if (getSegue() == Segue::Hostile
		&& (Manner = uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
		}
	}
}

ConversationData* init_pkunk_comm(void)
{
	pkunk_desc.init_encounter_func = Intro;
	pkunk_desc.post_encounter_func = post_pkunk_enc;
	pkunk_desc.uninit_encounter_func = uninit_pkunk;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	pkunk_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	pkunk_desc.AlienTextBaseline.y = 0;
	pkunk_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 3
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		// Enter communications immediately.
		setSegue(Segue::Peace);
	}
	else
	{
		// Ask the player whether to attack or talk.
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&pkunk_desc};

	return (retval);
}
