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

#include "../../nameref.h"
#include "../commall.h"
#include "resinst.h"
#include "strings.h"
#include "uqm/build.h"
#include "uqm/lua/UQMLua.h"
//JMS_GFX: For LoadGraphic()

#include "uqm/setup.h"

namespace uqm
{
// for GraphicsLock


static ConversationData orz_desc =
	{
		Conversation::Orz, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		OrzPmapAnim, /* AlienFrame */
		OrzFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		OrzColorMap, /* AlienColorMap */
		OrzMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		OrzConversationPhrases, /* PlayerPhrases */
		13, /* NumAnimations */
		{{
			/* AlienAmbientArray (ambient animations) */
			{
				4,						  /* StartIndex */
				6,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				10,						  /* StartIndex */
				5,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				15,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				17,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				20,					  /* StartIndex */
				2,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 7),				/* BlockMask */
			},
			{
				22,						  /* StartIndex */
				8,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6),				/* BlockMask */
			},
			{
				30,						  /* StartIndex */
				3,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
			{
				33,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4),				/* BlockMask */
			},
			{
				36,						  /* StartIndex */
				25,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				61,						  /* StartIndex */
				15,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				76,						  /* StartIndex */
				17,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
			{
				93,						  /* StartIndex */
				25,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				118,					  /* StartIndex */
				11,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10),				/* BlockMask */
			},
		}},
		{
								/* AlienTransitionDesc */
			0,					  /* StartIndex */
			0,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			0,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			1,					  /* StartIndex */
			3,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			GameTicksPerSecond / 15,
								GameTicksPerSecond / 15, /* FrameRate */
			GameTicksPerSecond / 12,
								GameTicksPerSecond * 3 / 8, /* RestartRate */
			0,							/* BlockMask */
		},
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static AnimFilterSet orz_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,					/* Color index */
				1,					/* Opacity index */
				-1,					/* Frame index */
				DrawKind::Multiply, /* DrawKind*/
				FilterFlags::None,	/* Flags */
			},
								  }
};

static void ExitConversation(ResponseRef R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, OrzPhrase::ByeAlly))
	{
		NPCPhrase(OrzPhrase::GoodbyeAlly);
	}
	else if (PlayerSaid(R, OrzPhrase::ByeNeutral))
	{
		NPCPhrase(OrzPhrase::GoodbyeNeutral);
	}
	else if (PlayerSaid(R, OrzPhrase::ByeAngry))
	{
		NPCPhrase(OrzPhrase::GoodbyeAngry);
	}
	else if (PlayerSaid(R, OrzPhrase::ByeTaalo))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER) == 1)
		{
			NPCPhrase(OrzPhrase::AngryTaaloGoodbye);
		}
		else
		{
			NPCPhrase(OrzPhrase::FriendlyTaaloGoodbye);
		}
	}
	else if (PlayerSaid(R, OrzPhrase::Hostile2))
	{
		NPCPhrase(OrzPhrase::HostilityIsBad2);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, OrzPhrase::MayWeLand))
	{
		NPCPhrase(OrzPhrase::SureLand);

		uqm::gameState().setFlag(uqm::GameFlag::TAALO_UNPROTECTED, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, OrzPhrase::YesAlliance)
			 || PlayerSaid(R, OrzPhrase::WereSorry))
	{
		if (PlayerSaid(R, OrzPhrase::YesAlliance))
		{
			NPCPhrase(OrzPhrase::Great);
		}
		else
		{
			NPCPhrase(OrzPhrase::ApologyAccepted);
		}

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_GENERAL_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_PERSONAL_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_MANNER, static_cast<uint32_t>(3));
		SetRaceAllied(RaceId::Orz, true);
	}
	else if (PlayerSaid(R, OrzPhrase::DemandToLand))
	{
		NPCPhrase(OrzPhrase::NoDemand);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, OrzPhrase::AboutAndro3)
			 || PlayerSaid(R, OrzPhrase::MustKnowAboutAndrosyn))
	{
		// Run filter for HdResolutionFactor
		if (IsHd())
		{
			EngageFilters(&orz_filters);
		}

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond / 2);

		if (PlayerSaid(R, OrzPhrase::AboutAndro3))
		{
			NPCPhrase(OrzPhrase::BlewIt);
		}
		else
		{
			NPCPhrase(OrzPhrase::KnowTooMuch);
		}

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_MANNER, static_cast<uint32_t>(2));
		setSegue(Segue::Hostile);
		if (PlayerSaid(R, OrzPhrase::AboutAndro3))
		{
			SetRaceAllied(RaceId::Orz, false);
			RemoveEscortShips(RaceId::Orz);
		}
	}
	else /* insults */
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_PERSONAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(OrzPhrase::Insulted1);
				break;
			case 1:
				NPCPhrase(OrzPhrase::Insulted2);
				break;
			case 2:
				NPCPhrase(OrzPhrase::Insulted3);
				setSegue(Segue::Hostile);
				break;
			case 7:
				--NumVisits;
				FALLTHROUGH;
			default:
				NPCPhrase(OrzPhrase::Insulted4);
				setSegue(Segue::Hostile);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_PERSONAL_INFO, static_cast<uint32_t>(NumVisits));
	}
}

static void TaaloWorld(ResponseRef R)
{
	// We can only get here when ORZ_MANNER != HOSTILE (2)
	uint8_t Manner {};

	Manner = uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER);
	if (PlayerSaid(R, OrzPhrase::DemandToLand))
	{
		NPCPhrase(OrzPhrase::AskNicely);

		DisablePhrase(OrzPhrase::DemandToLand);
	}
	else if (PlayerSaid(R, OrzPhrase::WhyYouHere))
	{
		if (Manner != 1)
		{
			NPCPhrase(OrzPhrase::FriendlyExplanation);
		}
		else
		{
			NPCPhrase(OrzPhrase::AngryExplanation);
		}

		DisablePhrase(OrzPhrase::WhyYouHere);
	}
	else if (PlayerSaid(R, OrzPhrase::WhatIsThisPlace))
	{
		if (Manner != 1)
		{
			NPCPhrase(OrzPhrase::FriendlyPlace);
		}
		else
		{
			NPCPhrase(OrzPhrase::AngryPlace);
			SetHomeworldKnown(*HomeId::Orz);
		}

		DisablePhrase(OrzPhrase::WhatIsThisPlace);
	}
	else if (PlayerSaid(R, OrzPhrase::MayWeLand))
	{
		NPCPhrase(OrzPhrase::AlliesCanVisit);

		DisablePhrase(OrzPhrase::MayWeLand);
	}
	else if (PlayerSaid(R, OrzPhrase::MakeAlliance))
	{
		NPCPhrase(OrzPhrase::CantAllyHere);
		SetHomeworldKnown(*HomeId::Orz);

		DisablePhrase(OrzPhrase::MakeAlliance);
	}
	else if (PlayerSaid(R, OrzPhrase::WhyBusy))
	{
		NPCPhrase(OrzPhrase::BusyBecause);

		DisablePhrase(OrzPhrase::WhyBusy);
	}

	if (PhraseEnabled(OrzPhrase::MayWeLand))
	{
		if (Manner == 3 && CheckAlliance(RaceId::Orz) == RaceAllegiance::Good)
		{
			Response(OrzPhrase::MayWeLand, ExitConversation);
		}
		else
		{
			Response(OrzPhrase::MayWeLand, TaaloWorld);
		}
	}
	else if (PhraseEnabled(OrzPhrase::MakeAlliance))
	{
		Response(OrzPhrase::MakeAlliance, TaaloWorld);
	}
	else if (PhraseEnabled(OrzPhrase::WhyBusy))
	{
		Response(OrzPhrase::WhyBusy, TaaloWorld);
	}
	if (PhraseEnabled(OrzPhrase::DemandToLand))
	{
		if (Manner == 1)
		{
			Response(OrzPhrase::DemandToLand, ExitConversation);
		}
		else
		{
			Response(OrzPhrase::DemandToLand, TaaloWorld);
		}
	}
	if (PhraseEnabled(OrzPhrase::WhyYouHere))
	{
		Response(OrzPhrase::WhyYouHere, TaaloWorld);
	}
	if (PhraseEnabled(OrzPhrase::WhatIsThisPlace))
	{
		Response(OrzPhrase::WhatIsThisPlace, TaaloWorld);
	}
	Response(OrzPhrase::ByeTaalo, ExitConversation);
}

static void OrzAllied(ResponseRef R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, OrzPhrase::WhatsUpAlly))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_GENERAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(OrzPhrase::GeneralInfoAlly1);
				break;
			case 1:
				NPCPhrase(OrzPhrase::GeneralInfoAlly2);
				break;
			case 2:
				NPCPhrase(OrzPhrase::GeneralInfoAlly3);
				break;
			case 3:
				NPCPhrase(OrzPhrase::GeneralInfoAlly4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_GENERAL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(OrzPhrase::WhatsUpAlly);
	}
	else if (PlayerSaid(R, OrzPhrase::MoreAboutYou))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_PERSONAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(OrzPhrase::AboutUs1);
				break;
			case 1:
				NPCPhrase(OrzPhrase::AboutUs2);
				break;
			case 2:
				NPCPhrase(OrzPhrase::AboutUs3);
				break;
			case 3:
				NPCPhrase(OrzPhrase::AboutUs4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_PERSONAL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(OrzPhrase::MoreAboutYou);
	}
	else if (PlayerSaid(R, OrzPhrase::AboutAndro1))
	{
		NPCPhrase(OrzPhrase::ForgetAndro1);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, OrzPhrase::AboutAndro2))
	{
		NPCPhrase(OrzPhrase::ForgetAndro2);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(2));
	}

	if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 0)
	{
		Response(OrzPhrase::AboutAndro1, OrzAllied);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 1)
	{
		Response(OrzPhrase::AboutAndro2, OrzAllied);
	}
	else
	{
		Response(OrzPhrase::AboutAndro3, ExitConversation);
	}
	if (PhraseEnabled(OrzPhrase::WhatsUpAlly))
	{
		Response(OrzPhrase::WhatsUpAlly, OrzAllied);
	}
	if (PhraseEnabled(OrzPhrase::MoreAboutYou))
	{
		Response(OrzPhrase::MoreAboutYou, OrzAllied);
	}
	Response(OrzPhrase::ByeAlly, ExitConversation);
}

static void OrzNeutral(ResponseRef R);

static void WhereAndrosyn(ResponseRef R)
{
	(void)R; // ignored
	NPCPhrase(OrzPhrase::DisembleAboutAndrosyn);
	DisablePhrase(OrzPhrase::WhereAndrosyn);

	Response(OrzPhrase::MustKnowAboutAndrosyn, ExitConversation);
	Response(OrzPhrase::DontReallyCare, OrzNeutral);
}

static void OfferAlliance(ResponseRef R)
{
	if (PlayerSaid(R, OrzPhrase::SeemLikeNiceGuys))
	{
		NPCPhrase(OrzPhrase::AreNiceWantAlly);
	}
	else if (PlayerSaid(R, OrzPhrase::TalkAboutAlliance))
	{
		NPCPhrase(OrzPhrase::OkTalkAlliance);
	}
	else if (PlayerSaid(R, OrzPhrase::WhySoTrusting))
	{
		NPCPhrase(OrzPhrase::TrustingBecause);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_STACK1, static_cast<uint32_t>(1));
	}

	Response(OrzPhrase::NoAlliance, OrzNeutral);
	Response(OrzPhrase::DecideLater, OrzNeutral);
	if (uqm::gameState().flag(uqm::GameFlag::ORZ_STACK1) == 0)
	{
		Response(OrzPhrase::WhySoTrusting, OfferAlliance);
	}
	Response(OrzPhrase::YesAlliance, ExitConversation);
}

static void OrzNeutral(ResponseRef R)
{
	uqstl::array<OrzPhrase, 3> pStr;

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = OrzPhrase::NullPhrase;
	if (PlayerSaid(R, OrzPhrase::Hostile1))
	{
		NPCPhrase(OrzPhrase::HostilityIsBad1);

		DisablePhrase(OrzPhrase::Hostile1);
		LastStack = 2;
	}
	else if (PlayerSaid(R, OrzPhrase::WeAreVindicator))
	{
		NPCPhrase(OrzPhrase::NiceToMeetYou);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_STACK0, static_cast<uint32_t>(1));
		LastStack = 1;
	}
	else if (PlayerSaid(R, OrzPhrase::WhoYou))
	{
		NPCPhrase(OrzPhrase::WeAreOrz);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, OrzPhrase::WhyHere))
	{
		NPCPhrase(OrzPhrase::HereBecause);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, OrzPhrase::NoAlliance))
	{
		NPCPhrase(OrzPhrase::MaybeLater);

		DisablePhrase(OrzPhrase::TalkAboutAlliance);
		uqm::gameState().setFlag(uqm::GameFlag::REFUSED_ORZ_ALLIANCE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, OrzPhrase::DecideLater))
	{
		NPCPhrase(OrzPhrase::OkLater);

		DisablePhrase(OrzPhrase::TalkAboutAlliance);
		uqm::gameState().setFlag(uqm::GameFlag::REFUSED_ORZ_ALLIANCE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, OrzPhrase::DontReallyCare))
	{
		NPCPhrase(OrzPhrase::YouAreOurFriends);
	}
	else if (PlayerSaid(R, OrzPhrase::WhereAndrosyn))
	{
		WhereAndrosyn(R);
		return;
	}
	else if (PlayerSaid(R, OrzPhrase::TalkAboutAlliance)
			 || PlayerSaid(R, OrzPhrase::SeemLikeNiceGuys))
	{
		OfferAlliance(R);
		return;
	}
	else if (PlayerSaid(R, OrzPhrase::Hostile2))
	{
		ExitConversation(R);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 0)
	{
		pStr[0] = OrzPhrase::WhoYou;
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 1)
	{
		pStr[0] = OrzPhrase::WhyHere;
	}
	else if (PhraseEnabled(OrzPhrase::WhereAndrosyn) && uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 2)
	{
		pStr[0] = OrzPhrase::WhereAndrosyn;
	}
	if (uqm::gameState().flag(uqm::GameFlag::REFUSED_ORZ_ALLIANCE))
	{
		if (PhraseEnabled(OrzPhrase::TalkAboutAlliance))
		{
			pStr[1] = OrzPhrase::TalkAboutAlliance;
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ORZ_STACK0) == 0)
	{
		pStr[1] = OrzPhrase::WeAreVindicator;
	}
	else
	{
		pStr[1] = OrzPhrase::SeemLikeNiceGuys;
	}
	if (PhraseEnabled(OrzPhrase::Hostile1))
	{
		pStr[2] = OrzPhrase::Hostile1;
	}
	else
	{
		pStr[2] = OrzPhrase::Hostile2;
	}

	if (pStr[LastStack] != OrzPhrase::NullPhrase)
	{
		Response(pStr[LastStack], OrzNeutral);
	}

	for (uint8_t i {0}; i < 3; ++i)
	{
		if (i != LastStack && *pStr[i] != OrzPhrase::NullPhrase)
		{
			Response(pStr[i], OrzNeutral);
		}
	}
	Response(OrzPhrase::ByeNeutral, ExitConversation);
}

static void OrzAngry(ResponseRef R)
{
	if (PlayerSaid(R, OrzPhrase::WhatsUpAngry))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_GENERAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(OrzPhrase::GeneralInfoAngry1);
				break;
			case 1:
				NPCPhrase(OrzPhrase::GeneralInfoAngry2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_GENERAL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(OrzPhrase::WhatsUpAngry);
	}

	if (PhraseEnabled(OrzPhrase::WhatsUpAngry))
	{
		Response(OrzPhrase::WhatsUpAngry, OrzAngry);
	}
	Response(OrzPhrase::WereSorry, ExitConversation);
	switch (uqm::gameState().flag(uqm::GameFlag::ORZ_PERSONAL_INFO))
	{
		case 0:
			Response(OrzPhrase::Insult1, ExitConversation);
			break;
		case 1:
			Response(OrzPhrase::Insult2, ExitConversation);
			break;
		case 2:
			Response(OrzPhrase::Insult3, ExitConversation);
			break;
		case 3:
			Response(OrzPhrase::Insult4, ExitConversation);
			break;
		case 4:
			Response(OrzPhrase::Insult5, ExitConversation);
			break;
		case 5:
			Response(OrzPhrase::Insult6, ExitConversation);
			break;
		case 6:
			Response(OrzPhrase::Insult7, ExitConversation);
			break;
		case 7:
			Response(OrzPhrase::Insult8, ExitConversation);
			break;
	}
	Response(OrzPhrase::ByeAngry, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};
	uint8_t Manner {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(OrzPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (!uqm::gameState().flag(uqm::GameFlag::MET_ORZ_BEFORE))
	{
		NPCPhrase(OrzPhrase::InitHello);
	}

	Manner = uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER);
	if (Manner == 2)
	{
		// JMS_GFX: Use separate red angry graphics in hires instead of colormap transform.
		if (IsHd())
		{
			CommData.AlienFrameRes = OrzAngryPmapAnim;
			CommData.AlienFrame = CaptureDrawable(
				LoadGraphic(CommData.AlienFrameRes));
		}
		else
		{
			CommData.AlienColorMapIndex = 1;
		}

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(OrzPhrase::HostileHello1);
				break;
			case 1:
				NPCPhrase(OrzPhrase::HostileHello2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TAALO_VISITS);
		if (Manner != 1)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::FriendlyAlliedTaaloHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::FriendlyAlliedTaaloHello2);
					--NumVisits;
					break;
			}
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::AngryTaaloHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::AngryTaaloHello2);
					--NumVisits;
					break;
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::TAALO_VISITS, static_cast<uint32_t>(NumVisits));

		TaaloWorld((ResponseRef)0);
	}
	else if (Manner == 3 && CheckAlliance(RaceId::Orz) == RaceAllegiance::Good)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::AlliedHomeworldHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::AlliedHomeworldHello2);
					break;
				case 2:
					NPCPhrase(OrzPhrase::AlliedHomeworldHello3);
					break;
				case 3:
					NPCPhrase(OrzPhrase::AlliedHomeworldHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::AlliedSpaceHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::AlliedSpaceHello2);
					break;
				case 2:
					NPCPhrase(OrzPhrase::AlliedSpaceHello3);
					break;
				case 3:
					NPCPhrase(OrzPhrase::AlliedSpaceHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));
		}

		OrzAllied((ResponseRef)0);
	}
	else if (Manner != 1)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::NeutralHomeworldHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::NeutralHomeworldHello2);
					break;
				case 2:
					NPCPhrase(OrzPhrase::NeutralHomeworldHello3);
					break;
				case 3:
					NPCPhrase(OrzPhrase::NeutralHomeworldHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::NeutralSpaceHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::NeutralSpaceHello2);
					break;
				case 2:
					NPCPhrase(OrzPhrase::NeutralSpaceHello3);
					break;
				case 3:
					NPCPhrase(OrzPhrase::NeutralSpaceHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));
		}

		OrzNeutral((ResponseRef)0);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::AngryHomeworldHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::AngryHomeworldHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(OrzPhrase::AngrySpaceHello1);
					break;
				case 1:
					NPCPhrase(OrzPhrase::AngrySpaceHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));
		}

		OrzAngry((ResponseRef)0);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::MET_ORZ_BEFORE))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MET_ORZ_BEFORE, static_cast<uint32_t>(1));

		// Disable talking anim and run the computer report
		BlockTalkingAnim(0, 1);
	}
}

static uint16_t uninit_orz(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_orz_enc(void)
{
	uint8_t Manner {};

	if (getSegue() == Segue::Hostile
		&& (Manner = uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::TAALO_VISITS, static_cast<uint32_t>(0));
		}
	}
}

ConversationData* init_orz_comm(void)
{
	orz_desc.init_encounter_func = Intro;
	orz_desc.post_encounter_func = post_orz_enc;
	orz_desc.uninit_encounter_func = uninit_orz;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	orz_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	orz_desc.AlienTextBaseline.y = 0;
	orz_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER) == 3
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&orz_desc};

	return (retval);
}

} // namespace uqm
