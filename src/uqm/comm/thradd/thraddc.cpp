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

#include "../../../options.h"
#include "../commall.h"
#include "resinst.h"
#include "strings.h"
#include "uqm/build.h"
#include "uqm/gameev.h"
#include "uqm/lua/UQMLua.h"


namespace uqm
{
static ConversationData thradd_desc =
	{
		Conversation::Thradd, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		ThraddPmapAnim, /* AlienFrame */
		ThraddFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		ThraddColorMap, /* AlienColorMap */
		ThraddMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		ThraddConversationPhrases, /* PlayerPhrases */
		8, /* NumAnimations */
		{{
			/* AlienAmbientArray (ambient animations) */
			{
				8,						/* StartIndex */
				4,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 4),				 /* BlockMask */
			},
			{
				12,						  /* StartIndex */
				9,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				21,						/* StartIndex */
				6,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				27,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4),				/* BlockMask */
			},
			{
				30, /* StartIndex */
				12, /* NumFrames */
				AnimationFlags::Circular
					| AnimationFlags::WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond, /* RestartRate */
				(1 << 0) | (1 << 3) | (1 << 5),
			},
			{
				42,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4) | (1 << 6),	/* BlockMask */
			},
			{
				47,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
			{
				52,						/* StartIndex */
				4,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0, /* RestartRate */
				0, /* BlockMask */
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
			7,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
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

static ThraddashPhrase GetCultureName(void)
{
	ThraddashPhrase culture {ThraddashPhrase::NullPhrase};

	switch (uqm::gameState().flag(uqm::GameFlag::THRADD_CULTURE))
	{
		case 1:
			culture = ThraddashPhrase::Culture;
			break;
		case 2:
			culture = ThraddashPhrase::FatJerks;
			break;
		case 3:
			culture = ThraddashPhrase::SlaveEmpire;
			break;
		default:
			assert(0 && "Unknown culture");
	}

	return (culture);
}

static void PolitePhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(ThraddashPhrase::HelloPolite1);
			break;
		case 1:
			NPCPhrase(ThraddashPhrase::HelloPolite2);
			break;
		case 2:
			NPCPhrase(ThraddashPhrase::HelloPolite3);
			break;
		case 3:
			NPCPhrase(ThraddashPhrase::HelloPolite4);
			break;
	}
}

static void RhymePhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(ThraddashPhrase::HelloRhyme1);
			break;
		case 1:
			NPCPhrase(ThraddashPhrase::HelloRhyme2);
			break;
		case 2:
			NPCPhrase(ThraddashPhrase::HelloRhyme3);
			break;
		case 3:
			NPCPhrase(ThraddashPhrase::HelloRhyme4);
			break;
	}
}

static void PigLatinPhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(ThraddashPhrase::HelloPigLatin1);
			break;
		case 1:
			NPCPhrase(ThraddashPhrase::HelloPigLatin2);
			break;
		case 2:
			NPCPhrase(ThraddashPhrase::HelloPigLatin3);
			break;
		case 3:
			NPCPhrase(ThraddashPhrase::HelloPigLatin4);
			break;
	}
}

static void LikeYouPhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(ThraddashPhrase::HelloLikeYou1);
			break;
		case 1:
			NPCPhrase(ThraddashPhrase::HelloLikeYou2);
			break;
		case 2:
			NPCPhrase(ThraddashPhrase::HelloLikeYou3);
			break;
		case 3:
			NPCPhrase(ThraddashPhrase::HelloLikeYou4);
			break;
	}
}

static void ExitConversation(ResponseRef R)
{
	setSegue(Segue::Hostile);

	if (PlayerSaid(R, ThraddashPhrase::ByeHostile2))
	{
		NPCPhrase(ThraddashPhrase::GoodbyeHostile2);
	}
	else if (PlayerSaid(R, ThraddashPhrase::ByeHostile1))
	{
		NPCPhrase(ThraddashPhrase::GoodbyeHostile1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_5, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::Submit1))
	{
		NPCPhrase(ThraddashPhrase::NoSubmit1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::Submit2))
	{
		NPCPhrase(ThraddashPhrase::NoSubmit2);
	}
	else if (PlayerSaid(R, ThraddashPhrase::GotIdea))
	{
		NPCPhrase(ThraddashPhrase::GoodIdea);

		setSegue(Segue::Peace);
		AddEvent(EventType::Relative, 0, 0, 0, GameEventId::AdvanceThraddMission);
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, ThraddashPhrase::ByeHostileHelix))
	{
		NPCPhrase(ThraddashPhrase::GoodbyeHostileHelix);
	}
	else if (PlayerSaid(R, ThraddashPhrase::ByeAlly))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_STACK_1);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ThraddashPhrase::GoodbyeAlly1);
				break;
			case 1:
				NPCPhrase(ThraddashPhrase::GoodbyeAlly2);
				break;
			case 2:
				NPCPhrase(ThraddashPhrase::GoodbyeAlly3);
				break;
			case 3:
				NPCPhrase(ThraddashPhrase::GoodbyeAlly4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(NumVisits));
		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, ThraddashPhrase::MayILand))
	{
		NPCPhrase(ThraddashPhrase::SureLand);

		uqm::gameState().setFlag(uqm::GameFlag::HELIX_UNPROTECTED, static_cast<uint32_t>(1));
		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, ThraddashPhrase::DemandToLand))
	{
		NPCPhrase(ThraddashPhrase::NoDemand);
	}
	else if (PlayerSaid(R, ThraddashPhrase::INeedToLandLie))
	{
		NPCPhrase(ThraddashPhrase::CaughtLie);
	}
	else
	{
		if (PlayerSaid(R, ThraddashPhrase::Contemplative))
		{
			NPCPhrase(ThraddashPhrase::OkContemplative);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(0));
		}
		else if (PlayerSaid(R, ThraddashPhrase::Friendly))
		{
			NPCPhrase(ThraddashPhrase::OkFriendly);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, ThraddashPhrase::Wacky))
		{
			NPCPhrase(ThraddashPhrase::OkWacky);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(2));
		}
		else if (PlayerSaid(R, ThraddashPhrase::JustLikeUs))
		{
			NPCPhrase(ThraddashPhrase::OkJustLikeYou);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(3));
		}
		NPCPhrase(ThraddashPhrase::WorkToDo);

		setSegue(Segue::Peace);
	}
}

static void ThraddAllies(ResponseRef R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, ThraddashPhrase::WhyYouHereAlly))
	{
		NPCPhrase(ThraddashPhrase::GuardingHelixAlly);

		DisablePhrase(ThraddashPhrase::WhyYouHereAlly);
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatsHelixAlly))
	{
		NPCPhrase(ThraddashPhrase::HelixIsAlly);

		DisablePhrase(ThraddashPhrase::WhatsHelixAlly);
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatsUpAlly))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ThraddashPhrase::GeneralInfoAlly1);
				break;
			case 1:
				NPCPhrase(ThraddashPhrase::GeneralInfoAlly2);
				break;
			case 2:
				NPCPhrase(ThraddashPhrase::GeneralInfoAlly3);
				break;
			case 3:
				NPCPhrase(ThraddashPhrase::GeneralInfoAlly4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(ThraddashPhrase::WhatsUpAlly);
	}
	else if (PlayerSaid(R, ThraddashPhrase::HowGoesCulture))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_DEMEANOR);
		switch (NumVisits & ((1 << 2) - 1))
		{
			case 0:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(ThraddashPhrase::ContempGoes1);
				}
				else
				{
					NPCPhrase(ThraddashPhrase::ContempGoes2);
				}
				break;
			case 1:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(ThraddashPhrase::FriendlyGoes1);
				}
				else
				{
					NPCPhrase(ThraddashPhrase::FriendlyGoes2);
				}
				break;
			case 2:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(ThraddashPhrase::WackyGoes1);
				}
				else
				{
					NPCPhrase(ThraddashPhrase::WackyGoes2);
				}
				break;
			case 3:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(ThraddashPhrase::LikeYouGoes1);
				}
				else
				{
					NPCPhrase(ThraddashPhrase::LikeYouGoes2);
				}
				break;
		}
		NumVisits |= 1 << 2;
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(NumVisits));

		DisablePhrase(ThraddashPhrase::HowGoesCulture);
	}

	if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		if (PhraseEnabled(ThraddashPhrase::WhyYouHereAlly))
		{
			Response(ThraddashPhrase::WhyYouHereAlly, ThraddAllies);
		}
		else
		{
			if (PhraseEnabled(ThraddashPhrase::WhatsHelixAlly))
			{
				Response(ThraddashPhrase::WhatsHelixAlly, ThraddAllies);
			}
			Response(ThraddashPhrase::MayILand, ExitConversation);
		}
	}
	if (PhraseEnabled(ThraddashPhrase::WhatsUpAlly))
	{
		Response(ThraddashPhrase::WhatsUpAlly, ThraddAllies);
	}
	if (PhraseEnabled(ThraddashPhrase::HowGoesCulture))
	{
		Response(ThraddashPhrase::HowGoesCulture, ThraddAllies);
	}
	Response(ThraddashPhrase::ByeAlly, ExitConversation);
}

static void ThraddDemeanor(ResponseRef R)
{
	if (PlayerSaid(R, ThraddashPhrase::YouDecide))
	{
		NPCPhrase(ThraddashPhrase::OkCulture20);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::Fat))
	{
		NPCPhrase(ThraddashPhrase::OkFat);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ThraddashPhrase::TheSlaveEmpire))
	{
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(3));

		NPCPhrase(ThraddashPhrase::OkSlave);
	}

	NPCPhrase(ThraddashPhrase::HowShouldWeAct);
	Response(ThraddashPhrase::Contemplative, ExitConversation);
	Response(ThraddashPhrase::Friendly, ExitConversation);
	Response(ThraddashPhrase::Wacky, ExitConversation);
	Response(ThraddashPhrase::JustLikeUs, ExitConversation);
}

static void ThraddCulture(ResponseRef R)
{
	if (PlayerSaid(R, ThraddashPhrase::BePolite))
	{
		NPCPhrase(ThraddashPhrase::OkPolite);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, ThraddashPhrase::UseRhymes))
	{
		NPCPhrase(ThraddashPhrase::OkRhymes);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::SpeakPigLatin))
	{
		NPCPhrase(ThraddashPhrase::OkPigLatin);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ThraddashPhrase::JustTheWayWeDo))
	{
		NPCPhrase(ThraddashPhrase::OkWayYouDo);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(3));
	}
	NPCPhrase(ThraddashPhrase::WhatNameForCulture);

	Response(ThraddashPhrase::YouDecide, ThraddDemeanor);
	Response(ThraddashPhrase::Fat, ThraddDemeanor);
	Response(ThraddashPhrase::TheSlaveEmpire, ThraddDemeanor);
}

static void ThraddWorship(ResponseRef R)
{
	(void)R; // ignored
	uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(0));
	uqm::gameState().setFlag(uqm::GameFlag::THRADD_MANNER, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(0));
	SetRaceAllied(RaceId::Thraddash, true);

	Response(ThraddashPhrase::BePolite, ThraddCulture);
	Response(ThraddashPhrase::SpeakPigLatin, ThraddCulture);
	Response(ThraddashPhrase::UseRhymes, ThraddCulture);
	Response(ThraddashPhrase::JustTheWayWeDo, ThraddCulture);
}

static void HelixWorld(ResponseRef R)
{
	if (PlayerSaid(R, ThraddashPhrase::WhyYouHereHostile))
	{
		NPCPhrase(ThraddashPhrase::NoneOfYourConcern);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatAboutThisWorld))
	{
		NPCPhrase(ThraddashPhrase::BlueHelix);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatsHelixHostile))
	{
		NPCPhrase(ThraddashPhrase::HelixIsHostile);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ThraddashPhrase::INeedToLandLie))
	{
		NPCPhrase(ThraddashPhrase::CaughtLie);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(1));
	}

	if (!uqm::gameState().flag(uqm::GameFlag::THRADD_CULTURE))
	{
		Response(ThraddashPhrase::WhyYouHereHostile, HelixWorld);
	}
	else
	{
		Response(ThraddashPhrase::DemandToLand, ExitConversation);
	}
	switch (uqm::gameState().flag(uqm::GameFlag::THRADD_INTRO))
	{
		case 0:
			Response(ThraddashPhrase::WhatAboutThisWorld, HelixWorld);
			break;
		case 1:
			Response(ThraddashPhrase::WhatsHelixHostile, HelixWorld);
			break;
	}
	if (!uqm::gameState().flag(uqm::GameFlag::THRADD_DEMEANOR))
	{
		Response(ThraddashPhrase::INeedToLandLie, ExitConversation);
	}
	Response(ThraddashPhrase::ByeHostileHelix, ExitConversation);
}

static void ThraddHostile(ResponseRef R)
{
	if (PlayerSaid(R, ThraddashPhrase::WhatsUpHostile1))
	{
		NPCPhrase(ThraddashPhrase::GeneralInfoHostile1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INFO, static_cast<uint32_t>(1));
		DisablePhrase(ThraddashPhrase::WhatsUpHostile2);
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatsUpHostile2))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_INFO);
		switch (NumVisits++)
		{
			case 1:
				NPCPhrase(ThraddashPhrase::GeneralInfoHostile2);
				break;
			case 2:
				NPCPhrase(ThraddashPhrase::GeneralInfoHostile3);
				break;
			case 3:
				NPCPhrase(ThraddashPhrase::GeneralInfoHostile4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(ThraddashPhrase::WhatsUpHostile2);
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatAboutYou1))
	{
		NPCPhrase(ThraddashPhrase::AboutUs1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatAboutYou2))
	{
		NPCPhrase(ThraddashPhrase::AboutUs2);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatAboutUrquan1))
	{
		NPCPhrase(ThraddashPhrase::AboutUrquan1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, ThraddashPhrase::WhatAboutUrquan2))
	{
		NPCPhrase(ThraddashPhrase::AboutUrquan2);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, ThraddashPhrase::BeFriends1))
	{
		NPCPhrase(ThraddashPhrase::NoFriends1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::BeFriends2))
	{
		NPCPhrase(ThraddashPhrase::NoFriends2);
		DisablePhrase(ThraddashPhrase::BeFriends2);
	}
	else if (PlayerSaid(R, ThraddashPhrase::HowImpressedUrquan1))
	{
		NPCPhrase(ThraddashPhrase::ImpressedLikeSo1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_4, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ThraddashPhrase::HowImpressedUrquan2))
	{
		NPCPhrase(ThraddashPhrase::ImpressedLikeSo2);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_MISSION, static_cast<uint32_t>(5));
	}

	if (uqm::gameState().flag(uqm::GameFlag::THRADD_INFO) == 0)
	{
		Response(ThraddashPhrase::WhatsUpHostile1, ThraddHostile);
	}
	else if (PhraseEnabled(ThraddashPhrase::WhatsUpHostile2))
	{
		Response(ThraddashPhrase::WhatsUpHostile2, ThraddHostile);
	}
	switch (uqm::gameState().flag(uqm::GameFlag::THRADD_STACK_1))
	{
		case 0:
			Response(ThraddashPhrase::WhatAboutYou1, ThraddHostile);
			break;
		case 1:
			Response(ThraddashPhrase::WhatAboutYou2, ThraddHostile);
			break;
		case 2:
			Response(ThraddashPhrase::WhatAboutUrquan1, ThraddHostile);
			break;
		case 3:
			Response(ThraddashPhrase::WhatAboutUrquan2, ThraddHostile);
			break;
		case 4:
			if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
			{
				Response(ThraddashPhrase::GotIdea, ExitConversation);
			}
			else
			{
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(5));
			}
			break;
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_2) == 0)
	{
		Response(ThraddashPhrase::Submit1, ExitConversation);
	}
	else
	{
		Response(ThraddashPhrase::Submit2, ExitConversation);
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_3) == 0)
	{
		Response(ThraddashPhrase::BeFriends1, ThraddHostile);
	}
	else if (PhraseEnabled(ThraddashPhrase::BeFriends2))
	{
		Response(ThraddashPhrase::BeFriends2, ThraddHostile);
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION) == 4)
	{
		if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_4) == 0)
		{
			Response(ThraddashPhrase::HowImpressedUrquan1, ThraddHostile);
		}
		else
		{
			Response(ThraddashPhrase::HowImpressedUrquan2, ThraddHostile);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_5) == 0)
	{
		Response(ThraddashPhrase::ByeHostile1, ExitConversation);
	}
	else
	{
		Response(ThraddashPhrase::ByeHostile2, ExitConversation);
	}
}

static void Intro(void)
{
	uint8_t NumVisits {};
	FleetInfoHandle hThradd = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Thraddash);
	const FleetInfo* ThraddPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hThradd);

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(ThraddashPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX) && !(IsExtended() && ThraddPtr->allied_state == RaceAllegiance::Good))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::HELIX_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ThraddashPhrase::DieThief1);
				break;
			case 1:
				NPCPhrase(ThraddashPhrase::DieThief2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::HELIX_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
	{
		if (isDifficulty(uqm::Difficulty::Hard) && uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			ThraddashPhrase pStr0 {};
			ThraddashPhrase pStr1 {};

			if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER))
			{
				pStr0 = ThraddashPhrase::WelcomeHelix0;
				pStr1 = ThraddashPhrase::WelcomeHelix1;

				NPCPhrase(pStr0);
				NPCPhrase_splice(GetCultureName());
				NPCPhrase(pStr1);

				ThraddAllies((ResponseRef)0);
			}
			else
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::HELIX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ThraddashPhrase::HostileHelixHello1);
						break;
					case 1:
						NPCPhrase(ThraddashPhrase::HostileHelixHello2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::HELIX_VISITS, static_cast<uint32_t>(NumVisits));

				HelixWorld((ResponseRef)0);
			}
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_VISITS);
			NumVisits = NumVisits > 1 ? 0 : NumVisits;

			if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER))
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ThraddashPhrase::HavingFunWithIlwrath1);
						break;
					case 1:
						NPCPhrase(ThraddashPhrase::HavingFunWithIlwrath2);
						--NumVisits;
						break;
				}
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ThraddashPhrase::GoAwayFightingIlwrath1);
						break;
					case 1:
						NPCPhrase(ThraddashPhrase::GoAwayFightingIlwrath2);
						--NumVisits;
						break;
				}
			}
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(NumVisits));

			setSegue(Segue::Peace);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER))
	{
		ThraddashPhrase pStr0 {};
		ThraddashPhrase pStr1 {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_VISITS);
		switch (uqm::gameState().flag(uqm::GameFlag::THRADD_INTRO))
		{
			case 0:
				PolitePhrase(NumVisits);
				break;
			case 1:
				RhymePhrase(NumVisits);
				break;
			case 2:
				PigLatinPhrase(NumVisits);
				break;
			case 3:
				LikeYouPhrase(NumVisits);
				break;
		}
		if (++NumVisits < 4)
		{
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(NumVisits));
		}

		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			pStr0 = ThraddashPhrase::WelcomeHelix0;
			pStr1 = ThraddashPhrase::WelcomeHelix1;
		}
		else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			pStr0 = ThraddashPhrase::WelcomeHomeworld0;
			pStr1 = ThraddashPhrase::WelcomeHomeworld1;
			SetHomeworldKnown(*HomeId::Thraddash);
		}
		else
		{
			pStr0 = ThraddashPhrase::WelcomeSpace0;
			pStr1 = ThraddashPhrase::WelcomeSpace1;
		}
		NPCPhrase(pStr0);
		NPCPhrase_splice(GetCultureName());
		NPCPhrase(pStr1);

		ThraddAllies((ResponseRef)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::HELIX_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ThraddashPhrase::HostileHelixHello1);
				break;
			case 1:
				NPCPhrase(ThraddashPhrase::HostileHelixHello2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::HELIX_VISITS, static_cast<uint32_t>(NumVisits));

		HelixWorld((ResponseRef)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::THRADDASH_BODY_COUNT)
			 >= (uint8_t)THRADDASH_BODY_THRESHOLD)
	{
		NPCPhrase(ThraddashPhrase::AmazingPerformance);

		ThraddWorship((ResponseRef)0);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADDASH_BODY_COUNT);
		if (NumVisits >= 16
			&& uqm::gameState().flag(uqm::GameFlag::THRADD_BODY_LEVEL) == 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_BODY_LEVEL, static_cast<uint32_t>(2));
			NPCPhrase(ThraddashPhrase::ImpressivePerformance);
		}
		else if (NumVisits >= 8
				 && uqm::gameState().flag(uqm::GameFlag::THRADD_BODY_LEVEL) == 0)
		{
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_BODY_LEVEL, static_cast<uint32_t>(1));
			NPCPhrase(ThraddashPhrase::AdequatePerformance);
		}

		{
			if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_HOME_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ThraddashPhrase::HostileHomeworldHello1);
						break;
					case 1:
						NPCPhrase(ThraddashPhrase::HostileHomeworldHello2);
						break;
					case 2:
						NPCPhrase(ThraddashPhrase::HostileHomeworldHello3);
						break;
					case 3:
						NPCPhrase(ThraddashPhrase::HostileHomeworldHello4);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOME_VISITS, static_cast<uint32_t>(NumVisits));
				SetHomeworldKnown(*HomeId::Thraddash);
			}
			else if ((NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION)) == 0
					 || NumVisits > 3)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ThraddashPhrase::HostileSpaceHello1);
						break;
					case 1:
						NPCPhrase(ThraddashPhrase::HostileSpaceHello2);
						break;
					case 2:
						NPCPhrase(ThraddashPhrase::HostileSpaceHello3);
						break;
					case 3:
						NPCPhrase(ThraddashPhrase::HostileSpaceHello4);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				switch (NumVisits)
				{
					case 1:
						if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION_VISITS) == 0)
						{
							NPCPhrase(ThraddashPhrase::WeGoToImpressUrquan1);
						}
						else
						{
							NPCPhrase(ThraddashPhrase::WeGoToImpressUrquan2);
						}
						break;
					case 2:
						if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION_VISITS) == 0)
						{
							NPCPhrase(ThraddashPhrase::WeImpressingUrquan1);
						}
						else
						{
							NPCPhrase(ThraddashPhrase::WeImpressingUrquan2);
						}
						break;
					case 3:
						if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION_VISITS) == 0)
						{
							NPCPhrase(ThraddashPhrase::WeImpressedUrquan1);
						}
						else
						{
							NPCPhrase(ThraddashPhrase::WeImpressedUrquan2);
						}
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_MISSION_VISITS, static_cast<uint32_t>(1));
			}

			ThraddHostile((ResponseRef)0);
		}
	}
}

static uint16_t uninit_thradd(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_thradd_enc(void)
{
	// nothing defined so far
}

ConversationData* init_thradd_comm(void)
{
	thradd_desc.init_encounter_func = Intro;
	thradd_desc.post_encounter_func = post_thradd_enc;
	thradd_desc.uninit_encounter_func = uninit_thradd;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	thradd_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	thradd_desc.AlienTextBaseline.y = 0;
	thradd_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER)
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&thradd_desc};

	return (retval);
}

} // namespace uqm
