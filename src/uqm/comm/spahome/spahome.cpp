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
#include "../spathi/resinst.h"
#include "strings.h"

#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"
#include "uqm/gameev.h"


static ConversationData spahome_desc =
	{
		Conversation::Spathi, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SpathiHomePmapAnim, /* AlienFrame */
		SpathiFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SpathiHomeColorMap, /* AlienColorMap */
		SpathiMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			SpahomeMusic, /* AlienAltSong */
		},
		SpathiHomeConversationPhrases, /* PlayerPhrases */
		14, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				1,			  /* StartIndex */
				3,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				4,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				9,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10) | (1 << 11),	/* BlockMask */
			},
								{
				13,							/* StartIndex */
				6,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 20, 0, /* FrameRate */
				GameTicksPerSecond / 20, 0, /* RestartRate */
				(1 << 4) | (1 << 5)			/* BlockMask */
			},
								{
				19,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3) | (1 << 5),	/* BlockMask */
			},
								{
				22,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 3) | (1 << 4)
					| (1 << 10), /* BlockMask */
			},
								{
				26,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10),				/* BlockMask */
			},
								{
				29,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				32,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0,					  /* RestartRate */
				(1 << 9) | (1 << 10), /* BlockMask */
			},
								{
				39,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 8) | (1 << 10),	/* BlockMask */
			},
								{
				42,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				0, /* RestartRate */
				(1 << 8) | (1 << 9)
					| (1 << 6) | (1 << 2)
					| (1 << 11) | (1 << 5), /* BlockMask */
			},
								{
				46,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 2) | (1 << 10),	 /* BlockMask */
			},
								{
				50,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0,		   /* RestartRate */
				(1 << 13), /* BlockMask */
			},
								{
				56,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
								},
		{
								/* AlienTransitionDesc - empty */
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
								/* AlienTalkDesc - empty */
			0, /* StartIndex */
			0, /* NumFrames */
			0, /* AnimFlags */
			0,
								0, /* FrameRate */
			0,
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

static AnimFilterSet spahome_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,							/* Color index */
				1,							/* Opacity index */
				-1,							/* Frame index */
				DrawKind::Alpha,			/* DrawKind*/
				SwitchOnAnims | TurnOffOft, /* Flags */
			},
								  }
};

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue::Peace);
	if (PlayerSaid(R, SpathiHomePhrase::WeAttackAgain))
	{
		NPCPhrase(SpathiHomePhrase::WeFightAgain);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::SurrenderOrDie))
	{
		NPCPhrase(SpathiHomePhrase::DefendOurselves);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WeAreVindicator))
	{
		NPCPhrase(SpathiHomePhrase::NoPassword);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::GortMerenga)
			 || PlayerSaid(R, SpathiHomePhrase::GuphFlorp)
			 || PlayerSaid(R, SpathiHomePhrase::WagnglFthagn)
			 || PlayerSaid(R, SpathiHomePhrase::Pleeese))
	{
		NPCPhrase(SpathiHomePhrase::WrongPassword);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ScrewPassword))
	{
		NPCPhrase(SpathiHomePhrase::NoPassword);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ByeNoAllyOffer))
	{
		NPCPhrase(SpathiHomePhrase::GoodbyeNoAllyOffer);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ByeAngrySpathi))
	{
		NPCPhrase(SpathiHomePhrase::GoodbyeAngrySpathi);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ByeAlly))
	{
		NPCPhrase(SpathiHomePhrase::GoodbyeAlly);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::AlreadyGotThem))
	{
		NPCPhrase(SpathiHomePhrase::EarlyBirdCheck);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_PARTY, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::TooDangerous))
	{
		NPCPhrase(SpathiHomePhrase::WeAgree);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ThinkMore))
	{
		NPCPhrase(SpathiHomePhrase::Coward);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::IAccept))
	{
		NPCPhrase(SpathiHomePhrase::AwaitReturn);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_QUEST, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::DoAsWeSay))
	{
		NPCPhrase(SpathiHomePhrase::DepartForEarth);

		SetRaceAllied(RaceId::Spathi, true);
		AddEvent(EventType::Relative, 6, 0, 0, GameEventId::SpathiShieldEvent);
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::KilledThemAll1))
	{
		NPCPhrase(SpathiHomePhrase::WillCheck1);

		if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
		{
			uqm::gameState().setFlag(uqm::GameFlag::LIED_ABOUT_CREATURES, static_cast<uint32_t>(1));
		}
		else
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_PARTY, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		}
	}
	else if (PlayerSaid(R, SpathiHomePhrase::KilledThemAll2))
	{
		NPCPhrase(SpathiHomePhrase::WillCheck2);

		if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
		{
			uqm::gameState().setFlag(uqm::GameFlag::LIED_ABOUT_CREATURES, static_cast<uint32_t>(2));
		}
		else
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_PARTY, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		}
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ByeBeforeParty))
	{
		NPCPhrase(SpathiHomePhrase::GoodbyeBeforeParty);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ByeFromParty1)
			 || PlayerSaid(R, SpathiHomePhrase::ByeFromParty2)
			 || PlayerSaid(R, SpathiHomePhrase::ByeFromParty3))
	{
		NPCPhrase(SpathiHomePhrase::GoodbyeFromParty);
	}
}

static void SpathiAllies(RESPONSE_REF R);

static void SpathiInfo(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, SpathiHomePhrase::LikeSomeInfo))
	{
		NPCPhrase(SpathiHomePhrase::WhatAbout);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatAboutHierarchy))
	{
		NPCPhrase(SpathiHomePhrase::AboutHierarchy);

		DisablePhrase(SpathiHomePhrase::WhatAboutHierarchy);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatAboutHistory))
	{
		NPCPhrase(SpathiHomePhrase::AboutHistory);

		DisablePhrase(SpathiHomePhrase::WhatAboutHistory);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatAboutAlliance))
	{
		NPCPhrase(SpathiHomePhrase::AboutAlliance);

		DisablePhrase(SpathiHomePhrase::WhatAboutAlliance);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatAboutOther))
	{
		NPCPhrase(SpathiHomePhrase::AboutOther);

		DisablePhrase(SpathiHomePhrase::WhatAboutOther);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatAboutPrecursors))
	{
		NPCPhrase(SpathiHomePhrase::AboutPrecursors);

		DisablePhrase(SpathiHomePhrase::WhatAboutPrecursors);
	}

	if (PhraseEnabled(SpathiHomePhrase::WhatAboutHierarchy))
	{
		InfoLeft = true;
		Response(SpathiHomePhrase::WhatAboutHierarchy, SpathiInfo);
	}
	if (PhraseEnabled(SpathiHomePhrase::WhatAboutHistory))
	{
		InfoLeft = true;
		Response(SpathiHomePhrase::WhatAboutHistory, SpathiInfo);
	}
	if (PhraseEnabled(SpathiHomePhrase::WhatAboutAlliance))
	{
		InfoLeft = true;
		Response(SpathiHomePhrase::WhatAboutAlliance, SpathiInfo);
	}
	if (PhraseEnabled(SpathiHomePhrase::WhatAboutOther))
	{
		InfoLeft = true;
		Response(SpathiHomePhrase::WhatAboutOther, SpathiInfo);
	}
	if (PhraseEnabled(SpathiHomePhrase::WhatAboutPrecursors))
	{
		InfoLeft = true;
		Response(SpathiHomePhrase::WhatAboutPrecursors, SpathiInfo);
	}
	Response(SpathiHomePhrase::EnoughInfo, SpathiAllies);

	if (!InfoLeft)
	{
		DisablePhrase(SpathiHomePhrase::LikeSomeInfo);
	}
}

static void SpathiAllies(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (R == 0)
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SpathiHomePhrase::HelloAllies1);
				break;
			case 1:
				NPCPhrase(SpathiHomePhrase::HelloAllies2);
				break;
			case 2:
				NPCPhrase(SpathiHomePhrase::HelloAllies3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatsUp))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SpathiHomePhrase::GeneralInfo1);
				break;
			case 1:
				NPCPhrase(SpathiHomePhrase::GeneralInfo2);
				break;
			case 2:
				NPCPhrase(SpathiHomePhrase::GeneralInfo3);
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(1));
				break;
			case 3:
				NPCPhrase(SpathiHomePhrase::GeneralInfo4);
				break;
			case 4:
				NPCPhrase(SpathiHomePhrase::GeneralInfo5);
				--NumVisits;
				break;
			case 5:
				NPCPhrase(SpathiHomePhrase::GeneralInfo5);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(SpathiHomePhrase::WhatsUp);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ResourcesPlease))
	{
		NPCPhrase(SpathiHomePhrase::SorryNoResources);

		DisablePhrase(SpathiHomePhrase::ResourcesPlease);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::SomethingFishy))
	{
		NPCPhrase(SpathiHomePhrase::NothingFishy);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_INFO, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::EnoughInfo))
	{
		NPCPhrase(SpathiHomePhrase::OkEnoughInfo);
	}

	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_INFO) == 4)
	{
		Response(SpathiHomePhrase::SomethingFishy, SpathiAllies);
	}
	if (PhraseEnabled(SpathiHomePhrase::WhatsUp))
	{
		Response(SpathiHomePhrase::WhatsUp, SpathiAllies);
	}
	if (PhraseEnabled(SpathiHomePhrase::ResourcesPlease))
	{
		Response(SpathiHomePhrase::ResourcesPlease, SpathiAllies);
	}
	if (PhraseEnabled(SpathiHomePhrase::LikeSomeInfo))
	{
		Response(SpathiHomePhrase::LikeSomeInfo, SpathiInfo);
	}
	Response(SpathiHomePhrase::ByeAlly, ExitConversation);
}

static void SpathiQuest(RESPONSE_REF R)
{
	if (R == 0)
	{
		if (!uqm::gameState().flag(uqm::GameFlag::LIED_ABOUT_CREATURES))
		{
			NPCPhrase(SpathiHomePhrase::HowGoEfforts);
		}
		else
		{
			NPCPhrase(SpathiHomePhrase::YouLied1);
		}
	}
	else if (PlayerSaid(R, SpathiHomePhrase::LittleMistake))
	{
		NPCPhrase(SpathiHomePhrase::BigMistake);

		DisablePhrase(SpathiHomePhrase::LittleMistake);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::TalkTest))
	{
		NPCPhrase(SpathiHomePhrase::TestAgain);

		DisablePhrase(SpathiHomePhrase::TalkTest);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ZappedAFew))
	{
		NPCPhrase(SpathiHomePhrase::YouFortunate);

		DisablePhrase(SpathiHomePhrase::ZappedAFew);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::LIED_ABOUT_CREATURES))
	{
		Response(SpathiHomePhrase::KilledThemAll1, ExitConversation);
	}
	else
	{
		if (PhraseEnabled(SpathiHomePhrase::LittleMistake))
		{
			Response(SpathiHomePhrase::LittleMistake, SpathiQuest);
		}
		Response(SpathiHomePhrase::KilledThemAll2, ExitConversation);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
	{
		if (PhraseEnabled(SpathiHomePhrase::TalkTest))
		{
			Response(SpathiHomePhrase::TalkTest, SpathiQuest);
		}
		if (PhraseEnabled(SpathiHomePhrase::ZappedAFew)
			&& uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_EXAMINED))
		{
			Response(SpathiHomePhrase::ZappedAFew, SpathiQuest);
		}
		Response(SpathiHomePhrase::ByeBeforeParty, ExitConversation);
	}
}

static void LearnQuest(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(SpathiHomePhrase::QuestAgain);

		DisablePhrase(SpathiHomePhrase::WhatTest);
		if (uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_EVIL))
		{
			DisablePhrase(SpathiHomePhrase::TellEvil);
		}
	}
	else if (PlayerSaid(R, SpathiHomePhrase::HowProve))
	{
		NPCPhrase(SpathiHomePhrase::BetterIdea);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatTest))
	{
		NPCPhrase(SpathiHomePhrase::WipeEvil);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_QUEST, static_cast<uint32_t>(1));
		DisablePhrase(SpathiHomePhrase::WhatTest);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::TellEvil))
	{
		NPCPhrase(SpathiHomePhrase::BeforeAccept);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_EVIL, static_cast<uint32_t>(1));
		DisablePhrase(SpathiHomePhrase::TellEvil);
		DisablePhrase(SpathiHomePhrase::ProveStrength);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ProveStrength))
	{
		NPCPhrase(SpathiHomePhrase::YourBehavior);

		DisablePhrase(SpathiHomePhrase::ProveStrength);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhyDontYouDoIt))
	{
		NPCPhrase(SpathiHomePhrase::WeWontBecause);

		DisablePhrase(SpathiHomePhrase::WhyDontYouDoIt);
	}

	if (PhraseEnabled(SpathiHomePhrase::WhatTest))
	{
		Response(SpathiHomePhrase::WhatTest, LearnQuest);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
	{
		Response(SpathiHomePhrase::AlreadyGotThem, ExitConversation);
	}
	else if (PhraseEnabled(SpathiHomePhrase::TellEvil))
	{
		Response(SpathiHomePhrase::TooDangerous, ExitConversation);
		Response(SpathiHomePhrase::TellEvil, LearnQuest);
	}
	else
	{
		Response(SpathiHomePhrase::TooDangerous, ExitConversation);
		Response(SpathiHomePhrase::ThinkMore, ExitConversation);
		Response(SpathiHomePhrase::IAccept, ExitConversation);
	}
	if (PhraseEnabled(SpathiHomePhrase::ProveStrength) && !uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_QUEST))
	{
		Response(SpathiHomePhrase::ProveStrength, LearnQuest);
	}
	if (PhraseEnabled(SpathiHomePhrase::WhyDontYouDoIt))
	{
		Response(SpathiHomePhrase::WhyDontYouDoIt, LearnQuest);
	}
}

static void AllianceOffer(RESPONSE_REF R)
{
	if (PlayerSaid(R, SpathiHomePhrase::Misunderstanding))
	{
		NPCPhrase(SpathiHomePhrase::JustMisunderstanding);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond / 4);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WeComeInPeace))
	{
		NPCPhrase(SpathiHomePhrase::OfCourse);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::HandInFriendship))
	{
		NPCPhrase(SpathiHomePhrase::TooAfraid);

		DisablePhrase(SpathiHomePhrase::HandInFriendship);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::Stronger))
	{
		NPCPhrase(SpathiHomePhrase::YoureNot);

		DisablePhrase(SpathiHomePhrase::Stronger);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::YesWeAre))
	{
		NPCPhrase(SpathiHomePhrase::NoYoureNot);

		DisablePhrase(SpathiHomePhrase::YesWeAre);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ShareInfo))
	{
		NPCPhrase(SpathiHomePhrase::NoInfo);

		DisablePhrase(SpathiHomePhrase::ShareInfo);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::GiveUsResources))
	{
		NPCPhrase(SpathiHomePhrase::NoResources);

		DisablePhrase(SpathiHomePhrase::GiveUsResources);
	}

	if (PhraseEnabled(SpathiHomePhrase::HandInFriendship))
	{
		Response(SpathiHomePhrase::HandInFriendship, AllianceOffer);
	}
	else if (PhraseEnabled(SpathiHomePhrase::Stronger))
	{
		Response(SpathiHomePhrase::Stronger, AllianceOffer);
	}
	else if (PhraseEnabled(SpathiHomePhrase::YesWeAre))
	{
		Response(SpathiHomePhrase::YesWeAre, AllianceOffer);
	}
	else
	{
		Response(SpathiHomePhrase::HowProve, LearnQuest);
	}
	if (PhraseEnabled(SpathiHomePhrase::ShareInfo))
	{
		Response(SpathiHomePhrase::ShareInfo, AllianceOffer);
	}
	if (PhraseEnabled(SpathiHomePhrase::GiveUsResources))
	{
		Response(SpathiHomePhrase::GiveUsResources, AllianceOffer);
	}
}

static void SpathiAngry(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(SpathiHomePhrase::MeanGuysReturn);

		Response(SpathiHomePhrase::WeApologize, SpathiAngry);
	}
	else /* if (R == SpathiHomePhrase::WeApologize) */
	{
		NPCPhrase(SpathiHomePhrase::DontBelieve);

		Response(SpathiHomePhrase::Misunderstanding, AllianceOffer);
	}

	Response(SpathiHomePhrase::WeAttackAgain, ExitConversation);
	Response(SpathiHomePhrase::ByeAngrySpathi, ExitConversation);
}

static void SpathiParty(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SpathiHomePhrase::MustParty1);
				break;
			case 1:
				NPCPhrase(SpathiHomePhrase::MustParty2);
				break;
			case 2:
				NPCPhrase(SpathiHomePhrase::MustParty3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::DealsADeal))
	{
		NPCPhrase(SpathiHomePhrase::WaitAWhile);

		DisablePhrase(SpathiHomePhrase::DealsADeal);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::HowLong))
	{
		NPCPhrase(SpathiHomePhrase::TenYears);

		DisablePhrase(SpathiHomePhrase::HowLong);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::Reneging))
	{
		NPCPhrase(SpathiHomePhrase::AdultView);

		DisablePhrase(SpathiHomePhrase::Reneging);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::ReturnBeasts))
	{
		NPCPhrase(SpathiHomePhrase::WhatRelationship);

		DisablePhrase(SpathiHomePhrase::ReturnBeasts);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::MindsAndMight))
	{
		NPCPhrase(SpathiHomePhrase::Huh);

		DisablePhrase(SpathiHomePhrase::MindsAndMight);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::Fellowship))
	{
		NPCPhrase(SpathiHomePhrase::What);

		DisablePhrase(SpathiHomePhrase::Fellowship);
	}

	if (PhraseEnabled(SpathiHomePhrase::DealsADeal))
	{
		Response(SpathiHomePhrase::DealsADeal, SpathiParty);
	}
	else if (PhraseEnabled(SpathiHomePhrase::HowLong))
	{
		Response(SpathiHomePhrase::HowLong, SpathiParty);
	}
	else if (PhraseEnabled(SpathiHomePhrase::Reneging))
	{
		Response(SpathiHomePhrase::Reneging, SpathiParty);
	}
	else if (PhraseEnabled(SpathiHomePhrase::ReturnBeasts))
	{
		Response(SpathiHomePhrase::ReturnBeasts, SpathiParty);
	}
	else
	{
		if (PhraseEnabled(SpathiHomePhrase::MindsAndMight))
		{
			Response(SpathiHomePhrase::MindsAndMight, SpathiParty);
		}
		if (PhraseEnabled(SpathiHomePhrase::Fellowship))
		{
			Response(SpathiHomePhrase::Fellowship, SpathiParty);
		}
		Response(SpathiHomePhrase::DoAsWeSay, ExitConversation);

		return;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) - 1)
	{
		case 0:
			Response(SpathiHomePhrase::ByeFromParty1, ExitConversation);
			break;
		case 1:
			Response(SpathiHomePhrase::ByeFromParty2, ExitConversation);
			break;
		default:
			Response(SpathiHomePhrase::ByeFromParty3, ExitConversation);
			break;
	}
}

static void SpathiCouncil(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(SpathiHomePhrase::HelloAgain);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::GoodPassword))
	{
		NPCPhrase(SpathiHomePhrase::YesGoodPassword);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond / 4);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_PASSWORD, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WeComeInPeace))
	{
		NPCPhrase(SpathiHomePhrase::KilledSpathi);

		DisablePhrase(SpathiHomePhrase::WeComeInPeace);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::SpathiOnPluto))
	{
		NPCPhrase(SpathiHomePhrase::WhereSpathi);

		DisablePhrase(SpathiHomePhrase::SpathiOnPluto);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::Hostage))
	{
		NPCPhrase(SpathiHomePhrase::GunToHead);

		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(3));
		DisablePhrase(SpathiHomePhrase::Hostage);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::KilledFwiffo))
	{
		NPCPhrase(SpathiHomePhrase::PoorFwiffo);

		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(3));
		DisablePhrase(SpathiHomePhrase::KilledFwiffo);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::FwiffoFine))
	{
		NPCPhrase(SpathiHomePhrase::NotLikely);

		R = *SpathiHomePhrase::KilledFwiffo;
		DisablePhrase(SpathiHomePhrase::FwiffoFine);
	}
	else if (PlayerSaid(R, SpathiHomePhrase::Surrender))
	{
		NPCPhrase(SpathiHomePhrase::NoSurrender);

		DisablePhrase(SpathiHomePhrase::Surrender);
	}

	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER) == 0)
	{
		Response(SpathiHomePhrase::WeComeInPeace, AllianceOffer);
	}
	else if (PhraseEnabled(SpathiHomePhrase::WeComeInPeace))
	{
		Response(SpathiHomePhrase::WeComeInPeace, SpathiCouncil);
	}
	else
	{
		Response(SpathiHomePhrase::Misunderstanding, AllianceOffer);
	}
	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
		&& uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) < 3)
	{
		if (PhraseEnabled(SpathiHomePhrase::SpathiOnPluto))
		{
			Response(SpathiHomePhrase::SpathiOnPluto, SpathiCouncil);
		}
		else if (HaveEscortShip(RaceId::Spathi))
		{
			if (PhraseEnabled(SpathiHomePhrase::Hostage))
			{
				Response(SpathiHomePhrase::Hostage, SpathiCouncil);
			}
		}
		else if (PhraseEnabled(SpathiHomePhrase::KilledFwiffo))
		{
			Response(SpathiHomePhrase::KilledFwiffo, SpathiCouncil);
			if (PhraseEnabled(SpathiHomePhrase::FwiffoFine))
			{
				Response(SpathiHomePhrase::FwiffoFine, SpathiCouncil);
			}
		}
	}
	if (PhraseEnabled(SpathiHomePhrase::Surrender))
	{
		Response(SpathiHomePhrase::Surrender, SpathiCouncil);
	}
	else
	{
		Response(SpathiHomePhrase::SurrenderOrDie, ExitConversation);
	}
	Response(SpathiHomePhrase::ByeNoAllyOffer, ExitConversation);
}

static void SpathiPassword(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			default:
				NPCPhrase(SpathiHomePhrase::WhatIsPassword);
				NumVisits = 1;
				break;
			case 1:
				NPCPhrase(SpathiHomePhrase::WhatIsPasswordAgain);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, SpathiHomePhrase::WhatDoIGet))
	{
		NPCPhrase(SpathiHomePhrase::YouGetToLive);

		DisablePhrase(SpathiHomePhrase::WhatDoIGet);
	}

	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
		|| uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_PASSWORD))
	{
		Response(SpathiHomePhrase::GoodPassword, SpathiCouncil);
		if (PhraseEnabled(SpathiHomePhrase::WhatDoIGet))
		{
			Response(SpathiHomePhrase::WhatDoIGet, SpathiPassword);
		}
	}
	else
	{
		Response(SpathiHomePhrase::WeAreVindicator, ExitConversation);
		Response(SpathiHomePhrase::GortMerenga, ExitConversation);
		Response(SpathiHomePhrase::GuphFlorp, ExitConversation);
		Response(SpathiHomePhrase::WagnglFthagn, ExitConversation);
		Response(SpathiHomePhrase::Pleeese, ExitConversation);
		Response(SpathiHomePhrase::ScrewPassword, ExitConversation);
	}
}

static void Intro(void)
{
	uint8_t Manner {};

	if (IsHd())
	{
		SwitchSequences(false);
		EngageFilters(&spahome_filters);
	}

	Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER);
	if (Manner == 2)
	{
		NPCPhrase(SpathiHomePhrase::HateYouForever);
		setSegue(Segue::Hostile);
	}
	else if (Manner == 1
			 && uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_PASSWORD)
			 && (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
				 || uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) != 7))
	{
		SpathiAngry((RESPONSE_REF)0);
	}
	else if (CheckAlliance(RaceId::Spathi) == RaceAllegiance::Good)
	{
		CommData.AlienColorMapIndex = 1;

		SpathiAllies((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_PARTY))
	{
		CommData.AlienColorMapIndex = 1;

		SpathiParty((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_QUEST))
	{
		if (uqm::gameState().flag(uqm::GameFlag::LIED_ABOUT_CREATURES) < 2)
		{
			CommData.AlienColorMapIndex = 1;

			SpathiQuest((RESPONSE_REF)0);
		}
		else
		{
			NPCPhrase(SpathiHomePhrase::YouLied2);

			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(2));
			setSegue(Segue::Hostile);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_QUEST))
	{
		CommData.AlienColorMapIndex = 1;

		LearnQuest((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_PASSWORD)
			 && (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
				 || uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) != 7))
	{
		CommData.AlienColorMapIndex = 1;

		SpathiCouncil((RESPONSE_REF)0);
	}
	else
	{
		SpathiPassword((RESPONSE_REF)0);
	}
}

static uint16_t uninit_spahome(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_spahome_enc(void)
{
	uint8_t Manner {};

	if (getSegue() == Segue::Hostile
		&& (Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		}
	}
}

ConversationData* init_spahome_comm()
{
	spahome_desc.init_encounter_func = Intro;
	spahome_desc.post_encounter_func = post_spahome_enc;
	spahome_desc.uninit_encounter_func = uninit_spahome;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	spahome_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	spahome_desc.AlienTextBaseline.y = 0;
	spahome_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	// use alternate "Safe Ones" track if available
	altResFlags |= UseAltSong;

	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER) == 3)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}

	ConversationData* retval {&spahome_desc};

	return (retval);
}
