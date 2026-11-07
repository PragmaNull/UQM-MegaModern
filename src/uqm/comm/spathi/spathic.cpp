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
#include "uqm/lua/UQMLua.h"


namespace uqm
{
static ConversationData spathi_desc =
	{
		Conversation::Spathi, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SpathiPmapAnim, /* AlienFrame */
		SpathiFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SpathiColorMap, /* AlienColorMap */
		SpathiMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			FwiffoMusic,  /* AlienAltSong */
		},
		SpathiConversationPhrases, /* PlayerPhrases */
		8, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				1,						  /* StartIndex */
				6,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				0,
				GameTicksPerSecond, /* RestartRate */
				(1 << 1),			/* BlockMask */
			},
								{
				7,						  /* StartIndex */
				9,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0,		  /* RestartRate */
				(1 << 0), /* BlockMask */
			},
								{
				16,						  /* StartIndex */
				4,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 4),				 /* BlockMask */
			},
								{20,											   /* StartIndex */
			 4,												   /* NumFrames */
			 AnimationFlags::Yoyo,							   /* AnimFlags */
			 GameTicksPerSecond / 15, GameTicksPerSecond / 15, /* FrameRate */
			 GameTicksPerSecond / 15, GameTicksPerSecond / 15, /* FrameRate */
			 (1 << 5)},
								{
				24,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2),				/* BlockMask */
			},


								{
				34,					  /* StartIndex */
				4,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3),				/* BlockMask */
			},
								{
				38,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				41,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								},
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
#ifdef NEVER
		{
								/* AlienTalkDesc */
			29,					  /* StartIndex */
			5,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			GameTicksPerSecond / 15,
								0, /* FrameRate */
			GameTicksPerSecond / 12,
								0, /* RestartRate */
			0, /* BlockMask */
		},
#else
		{
			/* AlienTalkDesc - empty */
			0,					  /* StartIndex */
			0,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			0,
			0, /* FrameRate */
			0,
			0, /* RestartRate */
			0, /* BlockMask */
		},
#endif	 /* NEVER */
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static void ExitConversation(ResponseRef Response)
{
	const uint8_t SpaCrew = ifEasyDifficulty(MaxCrewSize, 1);
	setSegue(Segue::Peace);

	if (PlayerSaid(Response, SpathiPhrase::ByeAllySpace))
	{
		NPCPhrase(SpathiPhrase::GoodbyeAllySpace);
	}
	else if (PlayerSaid(Response, SpathiPhrase::ByeFriendlySpace))
	{
		NPCPhrase(SpathiPhrase::GoodbyeFriendlySpace);
	}
	else if (PlayerSaid(Response, SpathiPhrase::PartInPeace))
	{
		NPCPhrase(SpathiPhrase::KeepItSecret);
	}
	else if (PlayerSaid(Response, SpathiPhrase::WeSorrySpace))
	{
		NPCPhrase(SpathiPhrase::ApologizeAtHomeworld);
	}
	else if (PlayerSaid(Response, SpathiPhrase::GiveInfoSpace))
	{
		NPCPhrase(SpathiPhrase::HeresSomeInfo);
	}
	else if (PlayerSaid(Response, SpathiPhrase::ByeAngrySpace))
	{
		NPCPhrase(SpathiPhrase::GoodbyeAngrySpace);
	}
	else if (PlayerSaid(Response, SpathiPhrase::AttackYouNow))
	{
		NPCPhrase(SpathiPhrase::Yipes);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(Response, SpathiPhrase::WeFightAgainSpace))
	{
		NPCPhrase(SpathiPhrase::OkFightAgainSpace);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(Response, SpathiPhrase::DieSlugboy)
			 || PlayerSaid(Response, SpathiPhrase::WeFight1)
			 || PlayerSaid(Response, SpathiPhrase::WeFight2)
			 || PlayerSaid(Response, SpathiPhrase::PayForCrimes)
			 || PlayerSaid(Response, SpathiPhrase::TellMeCoordinates)
			 || PlayerSaid(Response, SpathiPhrase::ChangedMind))
	{
		if (PlayerSaid(Response, SpathiPhrase::TellMeCoordinates))
		{
			NPCPhrase(SpathiPhrase::FakeCoordinates);
		}
		NPCPhrase(SpathiPhrase::OkWeFightAtPluto);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(Response, SpathiPhrase::JoinUs))
	{
		if (EscortFeasibilityStudy(RaceId::Spathi) == 0)
		{
			NPCPhrase(SpathiPhrase::TooScary);
		}
		else
		{
			NPCPhrase(SpathiPhrase::WillJoin);

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(RaceId::Spathi, 1);
			/* Make the Eluder escort captained by Fwiffo alone */
			SetEscortCrewComplement(RaceId::Spathi, SpaCrew,
									NameOffset + NumCaptainsNames);
		}
	}
	else if (PlayerSaid(Response, SpathiPhrase::JoinUsNomad))
	{
		if ((EscortFeasibilityStudy(RaceId::Spathi) == 0
			 || CountEscortShips(RaceId::Spathi) > 2))
		{
			NPCPhrase(SpathiPhrase::TooScary);
		}
		else
		{
			NPCPhrase(SpathiPhrase::WillJoin);

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(RaceId::Spathi, 1);
		}
	}
}

static uint8_t join_us_refusals;

static void SpathiOnPluto(ResponseRef R)
{
	if (PlayerSaid(R, SpathiPhrase::HiThere))
	{
		NPCPhrase(SpathiPhrase::AreYouSure);
	}
	else if (PlayerSaid(R, SpathiPhrase::DontKill))
	{
		NPCPhrase(SpathiPhrase::OkWont);
	}
	else if (PlayerSaid(R, SpathiPhrase::YoureForgiven))
	{
		NPCPhrase(SpathiPhrase::ThanksForForgiveness);
	}
	else if (PlayerSaid(R, SpathiPhrase::YouWontDieYet))
	{
		NPCPhrase(SpathiPhrase::EternalGratitude);
	}
	else if (PlayerSaid(R, SpathiPhrase::YouMayLive))
	{
		NPCPhrase(SpathiPhrase::GeeThanks);
	}
	else if (PlayerSaid(R, SpathiPhrase::YouveGotMeAllWrong))
	{
		NPCPhrase(SpathiPhrase::SorryNoCoords);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatDoingOnPluto1))
	{
		NPCPhrase(SpathiPhrase::About20YearsAgo);

		DisablePhrase(SpathiPhrase::WhatDoingOnPluto1);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatDoingOnPluto2))
	{
		NPCPhrase(SpathiPhrase::WhenUrquanArrived);

		DisablePhrase(SpathiPhrase::WhatDoingOnPluto2);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatDoingOnPluto3))
	{
		NPCPhrase(SpathiPhrase::StationedOnEarthMoon);

		DisablePhrase(SpathiPhrase::WhatDoingOnPluto3);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatAboutIlwrath))
	{
		NPCPhrase(SpathiPhrase::AboutIlwrath);

		DisablePhrase(SpathiPhrase::WhatAboutIlwrath);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhenIlwrath))
	{
		NPCPhrase(SpathiPhrase::ThenIlwrath);

		DisablePhrase(SpathiPhrase::WhenIlwrath);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatAboutMoonbase))
	{
		NPCPhrase(SpathiPhrase::SetUpBase);

		DisablePhrase(SpathiPhrase::WhatAboutMoonbase);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatAboutOtherSpathi))
	{
		NPCPhrase(SpathiPhrase::SpathiAre);

		DisablePhrase(SpathiPhrase::WhatAboutOtherSpathi);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatAboutOtherSpathi))
	{
		NPCPhrase(SpathiPhrase::ThenIlwrath);

		DisablePhrase(SpathiPhrase::WhatAboutOtherSpathi);
	}
	else if (PlayerSaid(R, SpathiPhrase::HowManyCrew))
	{
		NPCPhrase(SpathiPhrase::Thousands);

		DisablePhrase(SpathiPhrase::HowManyCrew);
	}
	else if (PlayerSaid(R, SpathiPhrase::ReallyThousands))
	{
		NPCPhrase(SpathiPhrase::JustMe);

		DisablePhrase(SpathiPhrase::ReallyThousands);
	}
	else if (PlayerSaid(R, SpathiPhrase::FullOfMonsters))
	{
		NPCPhrase(SpathiPhrase::HowTrue);

		DisablePhrase(SpathiPhrase::FullOfMonsters);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatEnemy))
	{
		NPCPhrase(SpathiPhrase::EnemyIs);

		DisablePhrase(SpathiPhrase::WhatEnemy);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhyYouHere))
	{
		NPCPhrase(SpathiPhrase::DrewShortStraw);

		DisablePhrase(SpathiPhrase::WhyYouHere);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhereAreUrquan))
	{
		NPCPhrase(SpathiPhrase::UrquanLeft);

		DisablePhrase(SpathiPhrase::WhereAreUrquan);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatAboutOtherRaces))
	{
		NPCPhrase(SpathiPhrase::AboutOtherRaces);

		DisablePhrase(SpathiPhrase::WhatAboutOtherRaces);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatBlazeOfGlory))
	{
		NPCPhrase(SpathiPhrase::BlazeIs);

		DisablePhrase(SpathiPhrase::WhatBlazeOfGlory);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatAboutYourself))
	{
		NPCPhrase(SpathiPhrase::AboutMyself);

		DisablePhrase(SpathiPhrase::WhatAboutYourself);
	}
	else if (PlayerSaid(R, SpathiPhrase::JoinUs))
	{
		if (join_us_refusals == 0)
		{
			NPCPhrase(SpathiPhrase::WontJoin1);
			++join_us_refusals;
		}
		else if (join_us_refusals == 1)
		{
			NPCPhrase(SpathiPhrase::WontJoin2);
			++join_us_refusals;
		}
		else
		{
			NPCPhrase(SpathiPhrase::WontJoin3);
		}
	}

	if (PhraseEnabled(SpathiPhrase::WhatDoingOnPluto1))
	{
		Response(SpathiPhrase::WhatDoingOnPluto1, SpathiOnPluto);
	}
	else if (PhraseEnabled(SpathiPhrase::WhatDoingOnPluto2))
	{
		Response(SpathiPhrase::WhatDoingOnPluto2, SpathiOnPluto);
	}
	else if (PhraseEnabled(SpathiPhrase::WhatDoingOnPluto3))
	{
		Response(SpathiPhrase::WhatDoingOnPluto3, SpathiOnPluto);
	}
	else
	{
		if (PhraseEnabled(SpathiPhrase::WhatAboutIlwrath))
		{
			Response(SpathiPhrase::WhatAboutIlwrath, SpathiOnPluto);
		}
		else if (PhraseEnabled(SpathiPhrase::WhenIlwrath))
		{
			Response(SpathiPhrase::WhenIlwrath, SpathiOnPluto);
		}

		if (PhraseEnabled(SpathiPhrase::WhatAboutMoonbase))
		{
			Response(SpathiPhrase::WhatAboutMoonbase, SpathiOnPluto);
		}
		else if (PhraseEnabled(SpathiPhrase::WhatAboutOtherSpathi))
		{
			Response(SpathiPhrase::WhatAboutOtherSpathi, SpathiOnPluto);
		}
		else
		{
			if (PhraseEnabled(SpathiPhrase::HowManyCrew))
			{
				Response(SpathiPhrase::HowManyCrew, SpathiOnPluto);
			}
			else if (PhraseEnabled(SpathiPhrase::ReallyThousands))
			{
				Response(SpathiPhrase::ReallyThousands, SpathiOnPluto);
			}
			else if (PhraseEnabled(SpathiPhrase::FullOfMonsters))
			{
				Response(SpathiPhrase::FullOfMonsters, SpathiOnPluto);
			}

			if (PhraseEnabled(SpathiPhrase::WhatEnemy))
			{
				Response(SpathiPhrase::WhatEnemy, SpathiOnPluto);
			}
			else if (PhraseEnabled(SpathiPhrase::WhyYouHere))
			{
				Response(SpathiPhrase::WhyYouHere, SpathiOnPluto);
			}
		}
	}
	if (PhraseEnabled(SpathiPhrase::WhereAreUrquan))
	{
		Response(SpathiPhrase::WhereAreUrquan, SpathiOnPluto);
	}
	else if (PhraseEnabled(SpathiPhrase::WhatAboutOtherRaces))
	{
		Response(SpathiPhrase::WhatAboutOtherRaces, SpathiOnPluto);
	}
	else if (PhraseEnabled(SpathiPhrase::WhatBlazeOfGlory))
	{
		Response(SpathiPhrase::WhatBlazeOfGlory, SpathiOnPluto);
	}
	else if (PhraseEnabled(SpathiPhrase::WhatAboutYourself))
	{
		Response(SpathiPhrase::WhatAboutYourself, SpathiOnPluto);
	}

	if (!PhraseEnabled(SpathiPhrase::FullOfMonsters))
	{
		Response(SpathiPhrase::JoinUs, ExitConversation);
	}
	else
	{
		Response(SpathiPhrase::JoinUs, SpathiOnPluto);
	}
	Response(SpathiPhrase::ChangedMind, ExitConversation);
}

static void SpathiMustGrovel(ResponseRef R)
{
	if (PlayerSaid(R, SpathiPhrase::Identify))
	{
		NPCPhrase(SpathiPhrase::IFwiffo);

		Response(SpathiPhrase::DoCultural, SpathiMustGrovel);
		Response(SpathiPhrase::YoureForgiven, SpathiOnPluto);
		Response(SpathiPhrase::DieSlugboy, ExitConversation);
	}
	else if (PlayerSaid(R, SpathiPhrase::DoCultural))
	{
		NPCPhrase(SpathiPhrase::WezzyWezzah);

		Response(SpathiPhrase::BeginRitual, SpathiMustGrovel);
		Response(SpathiPhrase::YouWontDieYet, SpathiOnPluto);
		Response(SpathiPhrase::WeFight2, ExitConversation);
	}
	else if (PlayerSaid(R, SpathiPhrase::BeginRitual))
	{
		NPCPhrase(SpathiPhrase::MustDoRitualAtHome);

		Response(SpathiPhrase::YouMayLive, SpathiOnPluto);
		Response(SpathiPhrase::PayForCrimes, ExitConversation);
		Response(SpathiPhrase::WhatAreCoordinates, SpathiMustGrovel);
	}
	else /* if (R == SpathiPhrase::WhatAreCoordinates) */
	{
		NPCPhrase(SpathiPhrase::CoordinatesAre);

		Response(SpathiPhrase::YouveGotMeAllWrong, SpathiOnPluto);
		Response(SpathiPhrase::TellMeCoordinates, ExitConversation);
	}
}

static void SpathiAllies(ResponseRef R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SpathiPhrase::InitAlliedHelloSpace);
				break;
			case 1:
				NPCPhrase(SpathiPhrase::SubsequentAlliedHelloSpace);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatsUpSpace2))
	{
		NPCPhrase(SpathiPhrase::GeneralInfoSpace2);

		DisablePhrase(SpathiPhrase::WhatsUpSpace2);
	}
	else if (PlayerSaid(R, SpathiPhrase::GiveUsInfoFromSpace))
	{
		NPCPhrase(SpathiPhrase::GetInfoFromSpathiwa);

		DisablePhrase(SpathiPhrase::GiveUsInfoFromSpace);
	}
	else if (PlayerSaid(R, SpathiPhrase::GiveUsResourcesSpace))
	{
		NPCPhrase(SpathiPhrase::GetResourcesFromSpathiwa);

		DisablePhrase(SpathiPhrase::GiveUsResourcesSpace);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatDoForFun))
	{
		NPCPhrase(SpathiPhrase::DoThisForFun);

		DisablePhrase(SpathiPhrase::WhatDoForFun);
	}

	if (PhraseEnabled(SpathiPhrase::WhatsUpSpace2))
	{
		Response(SpathiPhrase::WhatsUpSpace2, SpathiAllies);
	}
	if (PhraseEnabled(SpathiPhrase::GiveUsInfoFromSpace))
	{
		Response(SpathiPhrase::GiveUsInfoFromSpace, SpathiAllies);
	}
	if (PhraseEnabled(SpathiPhrase::GiveUsResourcesSpace))
	{
		Response(SpathiPhrase::GiveUsResourcesSpace, SpathiAllies);
	}
	if (PhraseEnabled(SpathiPhrase::WhatDoForFun))
	{
		Response(SpathiPhrase::WhatDoForFun, SpathiAllies);
	}
	if (getNomadMode() == uqm::NomadMode::Easy)
	{
		Response(SpathiPhrase::JoinUsNomad, ExitConversation);
	}
	Response(SpathiPhrase::ByeAllySpace, ExitConversation);
}

static void SpathiFriendly(ResponseRef R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SpathiPhrase::InitFriendlyHelloSpace);
				break;
			case 1:
				NPCPhrase(SpathiPhrase::SubsequentFriendlyHelloSpace);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, SpathiPhrase::SinceFriendlyGiveStuff))
	{
		NPCPhrase(SpathiPhrase::GiveAdvice);

		DisablePhrase(SpathiPhrase::SinceFriendlyGiveStuff);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatsUpSpace1))
	{
		NPCPhrase(SpathiPhrase::GeneralInfoSpace1);

		DisablePhrase(SpathiPhrase::WhatsUpSpace1);
	}

	if (PhraseEnabled(SpathiPhrase::WhatsUpSpace1))
	{
		Response(SpathiPhrase::WhatsUpSpace1, SpathiFriendly);
	}
	if (PhraseEnabled(SpathiPhrase::SinceFriendlyGiveStuff))
	{
		Response(SpathiPhrase::SinceFriendlyGiveStuff, SpathiFriendly);
	}
	Response(SpathiPhrase::ByeFriendlySpace, ExitConversation);
}

static void SpathiNeutral(ResponseRef R);

static void SpathiBefriend(ResponseRef R)
{
	SpathiPhrase pStr[2];

	uint8_t InfoLeft {false};
	uint8_t LastStack {0};
	pStr[0] = pStr[1] = SpathiPhrase::NullPhrase;
	if (PlayerSaid(R, SpathiPhrase::ComeInPeace))
	{
		NPCPhrase(SpathiPhrase::AgainstNature);
	}
	else if (PlayerSaid(R, SpathiPhrase::LookingForAFewGoodSquids))
	{
		NPCPhrase(SpathiPhrase::UrquanSlaves);

		DisablePhrase(SpathiPhrase::LookingForAFewGoodSquids);
	}
	else if (PlayerSaid(R, SpathiPhrase::WhySlaves))
	{
		NPCPhrase(SpathiPhrase::UmgahTrick);

		DisablePhrase(SpathiPhrase::WhySlaves);
	}
	else if (PlayerSaid(R, SpathiPhrase::TellUsAboutYou))
	{
		NPCPhrase(SpathiPhrase::AboutUs);

		DisablePhrase(SpathiPhrase::TellUsAboutYou);
		LastStack = 1;
	}
	else if (PlayerSaid(R, SpathiPhrase::WhatYouReallyWant))
	{
		NPCPhrase(SpathiPhrase::WantThis);

		DisablePhrase(SpathiPhrase::WhatYouReallyWant);
	}
	else if (PlayerSaid(R, SpathiPhrase::HowAboutAlliance))
	{
		NPCPhrase(SpathiPhrase::Sure);

		DisablePhrase(SpathiPhrase::HowAboutAlliance);
	}

	if (PhraseEnabled(SpathiPhrase::LookingForAFewGoodSquids))
	{
		pStr[0] = SpathiPhrase::LookingForAFewGoodSquids;
	}
	else if (PhraseEnabled(SpathiPhrase::WhySlaves))
	{
		pStr[0] = SpathiPhrase::WhySlaves;
	}
	if (PhraseEnabled(SpathiPhrase::TellUsAboutYou))
	{
		pStr[1] = SpathiPhrase::TellUsAboutYou;
	}
	else if (PhraseEnabled(SpathiPhrase::WhatYouReallyWant))
	{
		pStr[1] = SpathiPhrase::WhatYouReallyWant;
	}
	if (pStr[LastStack] != SpathiPhrase::NullPhrase)
	{
		InfoLeft = true;
		Response(pStr[LastStack], SpathiBefriend);
	}
	LastStack ^= 1;
	if (pStr[LastStack] != SpathiPhrase::NullPhrase)
	{
		InfoLeft = true;
		Response(pStr[LastStack], SpathiBefriend);
	}
	if (PhraseEnabled(SpathiPhrase::HowAboutAlliance))
	{
		InfoLeft = true;
		Response(SpathiPhrase::HowAboutAlliance, SpathiBefriend);
	}

	if (!InfoLeft)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK1, static_cast<uint32_t>(1));
		SpathiNeutral(R);
	}
}

static void SpathiAntagonize(ResponseRef R)
{
	if (PlayerSaid(R, SpathiPhrase::PrepareToDie))
	{
		NPCPhrase(SpathiPhrase::AlwaysPrepared);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SpathiPhrase::HeardYoureCowards))
	{
		NPCPhrase(SpathiPhrase::DarnTootin);

		DisablePhrase(SpathiPhrase::HeardYoureCowards);
	}
	else if (PlayerSaid(R, SpathiPhrase::WannaFight))
	{
		NPCPhrase(SpathiPhrase::YesWeDo);

		DisablePhrase(SpathiPhrase::WannaFight);
	}
	else if (PlayerSaid(R, SpathiPhrase::SoLetsFight))
	{
		NPCPhrase(SpathiPhrase::OkLetsFight);

		DisablePhrase(SpathiPhrase::SoLetsFight);
	}
	else if (PlayerSaid(R, SpathiPhrase::SoLetsFightAlready))
	{
		NPCPhrase(SpathiPhrase::DontReallyWantToFight);

		DisablePhrase(SpathiPhrase::SoLetsFightAlready);
	}

	if (PhraseEnabled(SpathiPhrase::WannaFight))
	{
		Response(SpathiPhrase::WannaFight, SpathiAntagonize);
	}
	else if (PhraseEnabled(SpathiPhrase::SoLetsFight))
	{
		Response(SpathiPhrase::SoLetsFight, SpathiAntagonize);
	}
	else if (PhraseEnabled(SpathiPhrase::SoLetsFightAlready))
	{
		Response(SpathiPhrase::SoLetsFightAlready, SpathiAntagonize);
	}
	if (PhraseEnabled(SpathiPhrase::HeardYoureCowards))
	{
		Response(SpathiPhrase::HeardYoureCowards, SpathiAntagonize);
	}
	Response(SpathiPhrase::AttackYouNow, ExitConversation);
}

static void SpathiNeutral(ResponseRef R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SpathiPhrase::InitNeutralHelloSpace);
				break;
			case 1:
				NPCPhrase(SpathiPhrase::SubsequentNeutralHelloSpace);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, SpathiPhrase::LookWeird))
	{
		NPCPhrase(SpathiPhrase::YouLookWeird);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK0, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SpathiPhrase::NoLookReallyWeird))
	{
		NPCPhrase(SpathiPhrase::NoYouLookReallyWeird);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK0, static_cast<uint32_t>(2));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SPATHI_STACK0))
	{
		case 0:
			Response(SpathiPhrase::LookWeird, SpathiNeutral);
			break;
		case 1:
			Response(SpathiPhrase::NoLookReallyWeird, SpathiNeutral);
			break;
	}
	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_STACK1) == 0)
	{
		Response(SpathiPhrase::ComeInPeace, SpathiBefriend);
	}
	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_STACK2) == 0)
	{
		Response(SpathiPhrase::PrepareToDie, SpathiAntagonize);
	}
	else
	{
		Response(SpathiPhrase::AttackYouNow, ExitConversation);
	}
	Response(SpathiPhrase::PartInPeace, ExitConversation);
}

static void Intro(void)
{
	uint8_t Manner {};

	Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER);
	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1)
	{
		join_us_refusals = 0;

		NPCPhrase(SpathiPhrase::SorryAboutThat);

		/* if already know password from Melnorme,
			 * but haven't visited Spathiwa yet . . .
			 */
		if (uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) == 7)
		{
			uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_PASSWORD, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		}
		SetHomeworldKnown(*HomeId::Spathi);

		Response(SpathiPhrase::Identify, SpathiMustGrovel);
		Response(SpathiPhrase::HiThere, SpathiOnPluto);
		Response(SpathiPhrase::DontKill, SpathiOnPluto);
		Response(SpathiPhrase::WeFight1, ExitConversation);
	}
	else if (Manner == 2)
	{
		NPCPhrase(SpathiPhrase::HateYouForeverSpace);
		setSegue(Segue::Hostile);
	}
	else if (Manner == 1)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SpathiPhrase::InitAngryHelloSpace);
				break;
			case 1:
				NPCPhrase(SpathiPhrase::SubsequentAngryHelloSpace);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));

		Response(SpathiPhrase::GiveInfoSpace, ExitConversation);
		Response(SpathiPhrase::WeSorrySpace, ExitConversation);
		Response(SpathiPhrase::WeFightAgainSpace, ExitConversation);
		Response(SpathiPhrase::ByeAngrySpace, ExitConversation);
	}
	else if (CheckAlliance(RaceId::Spathi) == RaceAllegiance::Good)
	{
		SpathiAllies((ResponseRef)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_QUEST))
	{
		SpathiFriendly((ResponseRef)0);
	}
	else
	{
		SpathiNeutral((ResponseRef)0);
	}
}

static uint16_t uninit_spathi(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_spathi_enc(void)
{
	uint8_t Manner {};

	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1)
	{
		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(2));
	}
	else if (getSegue() == Segue::Hostile
			 && (Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			/* if don't know about Spathi via Melnorme . . . */
			if (uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) != 7)
			{
				uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
			}
		}
	}
}

ConversationData* init_spathi_comm(void)
{
	spathi_desc.init_encounter_func = Intro;
	spathi_desc.post_encounter_func = post_spathi_enc;
	spathi_desc.uninit_encounter_func = uninit_spathi;

	uqm::lua().beginComm(nullptr, NullResource);

	spathi_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	spathi_desc.AlienTextBaseline.y = 0;
	spathi_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	// use alternate Fwiffo track if available
	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1)
	{
		altResFlags |= UseAltSong;
	}

	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1
		|| uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER) == 3
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&spathi_desc};

	return (retval);
}

} // namespace uqm
