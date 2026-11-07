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
#include "uqm/gameev.h"
#include "uqm/lua/UQMLua.h"


namespace uqm
{
static ConversationData utwig_desc =
	{
		Conversation::Utwig, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		UtwigPmapAnim, /* AlienFrame */
		UtwigFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Middle, /* AlienTextValign */
		UtwigColorMap, /* AlienColorMap */
		UtwigMusic, /* AlienSong */
		{
								NullResource,	  /* AlienAltFrame */
			NullResource,	  /* AlienAltColorMap */
			UtwigUltronMusic, /* AlienAltSong */
		},
		UtwigConversationPhrases, /* PlayerPhrases */
		16, /* NumAnimations */
		{{
			/* AlienAmbientArray (ambient animations) */
			{
				4,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				7,					  /* StartIndex */
				4,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2),				/* BlockMask */
			},
			{
				11, /* StartIndex */
				2,	/* NumFrames */
				AnimationFlags::Yoyo
					| AnimationFlags::WaitTalking, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 1),				/* BlockMask */
			},
			{
				13,						  /* StartIndex */
				5,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond / 12,
				0, /* RestartRate */
				0, /* BlockMask */
			},
			{
				18,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				20,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				22,						/* StartIndex */
				3,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				25,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				27,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				30,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				32,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				34,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				36,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				38,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				40,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				42,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
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
			GameTicksPerSecond / 20,
								GameTicksPerSecond / 20, /* FrameRate */
			GameTicksPerSecond * 7 / 60,
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

static void ExitConversation(ResponseRef R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, UtwigPhrase::ByeNeutral))
	{
		NPCPhrase(UtwigPhrase::GoodbyeNeutral);
	}
	else if (PlayerSaid(R, UtwigPhrase::ByeAfterSpace))
	{
		NPCPhrase(UtwigPhrase::GoodbyeAfterSpace);
	}
	else if (PlayerSaid(R, UtwigPhrase::ByeBeforeSpace))
	{
		NPCPhrase(UtwigPhrase::GoodbyeBeforeSpace);
	}
	else if (PlayerSaid(R, UtwigPhrase::ByeAlliedHomeworld))
	{
		NPCPhrase(UtwigPhrase::GoodbyeAlliedHomeworld);
	}
	else if (PlayerSaid(R, UtwigPhrase::ByeBomb))
	{
		NPCPhrase(UtwigPhrase::GoodbyeBomb);
	}
	else if (PlayerSaid(R, UtwigPhrase::DemandBomb))
	{
		NPCPhrase(UtwigPhrase::GuardsFight);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, UtwigPhrase::GotUltron)
			 || PlayerSaid(R, UtwigPhrase::HeyWaitGotUltron))
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			NPCPhrase(UtwigPhrase::NoUltronAtBomb);

			uqm::gameState().setFlag(uqm::GameFlag::REFUSED_ULTRON_AT_BOMB, static_cast<uint32_t>(1));

			SetHomeworldKnown(*HomeId::Utwig);
		}
		else
		{
			if (PlayerSaid(R, UtwigPhrase::GotUltron))
			{
				NPCPhrase(UtwigPhrase::DontWantToLook);
			}
			else
			{
				NPCPhrase(UtwigPhrase::TauntUsButWeLook);
			}
			if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) < 4)
			{
				switch (uqm::gameState().flag(uqm::GameFlag::UTWIG_INFO))
				{
					case 0:
						if (PlayerSaid(R, UtwigPhrase::GotUltron))
						{
							NPCPhrase(UtwigPhrase::SickTrick1);
						}
						else
						{
							NPCPhrase(UtwigPhrase::TrickedUs1);

							setSegue(Segue::Hostile);
						}
						break;
					case 1:
						if (PlayerSaid(R, UtwigPhrase::GotUltron))
						{
							NPCPhrase(UtwigPhrase::SickTrick2);
						}
						else
						{
							NPCPhrase(UtwigPhrase::TrickedUs2);

							setSegue(Segue::Hostile);
						}
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_INFO, static_cast<uint32_t>(1));
			}
			else
			{
				NPCPhrase(UtwigPhrase::HappyDays);
				if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
				{
					NPCPhrase(UtwigPhrase::TooLate);
				}
				else
				{
					NPCPhrase(UtwigPhrase::OkAttackKohrah);

					AddEvent(EventType::Relative, 0, 0, 0, GameEventId::AdvanceUtwigSupoxMission);
				}

				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HAVE_ULTRON, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(5));

				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOME_VISITS, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOME_VISITS, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(0));

				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_INFO, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_INFO, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOSTILE, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOSTILE, static_cast<uint32_t>(0));

				SetRaceAllied(RaceId::Utwig, true);
				SetRaceAllied(RaceId::Supox, true);
			}
		}
	}
	else if (PlayerSaid(R, UtwigPhrase::CanYouHelp))
	{
		NPCPhrase(UtwigPhrase::HowHelp);
		if (EscortFeasibilityStudy(RaceId::Utwig) == 0)
		{
			NPCPhrase(UtwigPhrase::DontNeed);
		}
		else
		{
			NPCPhrase(UtwigPhrase::Have4Ships);

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(RaceId::Utwig, difficultyCase(4, 4, 2));
			PrepareShip(RaceId::Utwig);
		}
	}
}

static void AlliedHome(ResponseRef R);

static void AlliedHome(ResponseRef R)
{
	uint8_t NumVisits {};
	uint8_t News {};

	News = uqm::gameState().flag(uqm::GameFlag::UTWIG_WAR_NEWS);
	NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
	if (PlayerSaid(R, UtwigPhrase::HowWentWar))
	{
		NPCPhrase(UtwigPhrase::AboutBattle);

		News |= (1 << 0);
	}
	else if (PlayerSaid(R, UtwigPhrase::HowGoesWar))
	{
		if (NumVisits == 1)
		{
			NPCPhrase(UtwigPhrase::FleetOnWay);

			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(1));
		}
		else
		{
			switch (uqm::gameState().flag(uqm::GameFlag::UTWIG_WAR_NEWS))
			{
				case 0:
					NPCPhrase(UtwigPhrase::BattleHappens1);
					News = 1;
					break;
				case 1:
					NPCPhrase(UtwigPhrase::BattleHappens2);
					News = 2;
					break;
			}
		}

		DisablePhrase(UtwigPhrase::HowGoesWar);
	}
	else if (PlayerSaid(R, UtwigPhrase::LearnNewInfo))
	{
		if (NumVisits < 5)
		{
			NPCPhrase(UtwigPhrase::NoNewInfo);
		}
		else
		{
			NPCPhrase(UtwigPhrase::Samatra);

			News |= (1 << 1);
		}

		DisablePhrase(UtwigPhrase::LearnNewInfo);
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatNowHomeworld))
	{
		if (NumVisits < 5)
		{
			NPCPhrase(UtwigPhrase::UpToYou);
		}
		else
		{
			NPCPhrase(UtwigPhrase::HopeKillEachOther);
		}

		DisablePhrase(UtwigPhrase::WhatNowHomeworld);
	}
	else if (PlayerSaid(R, UtwigPhrase::HowIsUltron))
	{
		NPCPhrase(UtwigPhrase::UltronIsGreat);

		DisablePhrase(UtwigPhrase::HowIsUltron);
	}
	uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(News));

	if (NumVisits >= 5)
	{
		if (!(News & (1 << 0)))
		{
			Response(UtwigPhrase::HowWentWar, AlliedHome);
		}
	}
	else if (PhraseEnabled(UtwigPhrase::HowGoesWar)
			 && ((NumVisits == 1 && News == 0)
				 || (NumVisits && News < 2)))
	{
		Response(UtwigPhrase::HowGoesWar, AlliedHome);
	}
	if (PhraseEnabled(UtwigPhrase::LearnNewInfo))
	{
		Response(UtwigPhrase::LearnNewInfo, AlliedHome);
	}
	if (PhraseEnabled(UtwigPhrase::WhatNowHomeworld))
	{
		Response(UtwigPhrase::WhatNowHomeworld, AlliedHome);
	}
	if (PhraseEnabled(UtwigPhrase::HowIsUltron))
	{
		Response(UtwigPhrase::HowIsUltron, AlliedHome);
	}
	if (IsExtended() && ShipsReady(RaceId::Utwig))
	{
		Response(UtwigPhrase::CanYouHelp, ExitConversation);
	}
	Response(UtwigPhrase::ByeAlliedHomeworld, ExitConversation);
}

static void BeforeKohrAh(ResponseRef R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, UtwigPhrase::WhatsUpBeforeSpace))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(UtwigPhrase::GeneralInfoBeforeSpace1);
				break;
			case 1:
				NPCPhrase(UtwigPhrase::GeneralInfoBeforeSpace2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(UtwigPhrase::WhatsUpBeforeSpace);
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatNowBeforeSpace))
	{
		NPCPhrase(UtwigPhrase::DoThisBeforeSpace);

		DisablePhrase(UtwigPhrase::WhatNowBeforeSpace);
	}

	if (PhraseEnabled(UtwigPhrase::WhatsUpBeforeSpace))
	{
		Response(UtwigPhrase::WhatsUpBeforeSpace, BeforeKohrAh);
	}
	if (PhraseEnabled(UtwigPhrase::WhatNowBeforeSpace))
	{
		Response(UtwigPhrase::WhatNowBeforeSpace, BeforeKohrAh);
	}
	Response(UtwigPhrase::ByeBeforeSpace, ExitConversation);
}

static void AfterKohrAh(ResponseRef R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, UtwigPhrase::WhatsUpAfterSpace))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(UtwigPhrase::GeneralInfoAfterSpace1);
				break;
			case 1:
				NPCPhrase(UtwigPhrase::GeneralInfoAfterSpace2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(UtwigPhrase::WhatsUpAfterSpace);
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatNowAfterSpace))
	{
		NPCPhrase(UtwigPhrase::DoThisAfterSpace);

		DisablePhrase(UtwigPhrase::WhatNowAfterSpace);
	}

	if (PhraseEnabled(UtwigPhrase::WhatsUpAfterSpace))
	{
		Response(UtwigPhrase::WhatsUpAfterSpace, AfterKohrAh);
	}
	if (PhraseEnabled(UtwigPhrase::WhatNowAfterSpace))
	{
		Response(UtwigPhrase::WhatNowAfterSpace, AfterKohrAh);
	}
	Response(UtwigPhrase::ByeAfterSpace, ExitConversation);
}

static void NeutralUtwig(ResponseRef R)
{
	uint8_t i {};
	uqstl::array<UtwigPhrase, 4> pStr;

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = pStr[3] = UtwigPhrase::NullPhrase;
	if (PlayerSaid(R, UtwigPhrase::WeAreVindicator))
	{
		NPCPhrase(UtwigPhrase::WouldBeHappyBut);

		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_STACK1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UtwigPhrase::WhySad))
	{
		NPCPhrase(UtwigPhrase::UltronBroke);

		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatUltron))
	{
		NPCPhrase(UtwigPhrase::GloriousUltron);

		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, UtwigPhrase::DontBeBabies))
	{
		NPCPhrase(UtwigPhrase::MockOurPain);

		setSegue(Segue::Hostile);
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_STACK1, static_cast<uint32_t>(4));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOSTILE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(0));
		return;
	}
	else if (PlayerSaid(R, UtwigPhrase::RealSorryAboutUltron))
	{
		NPCPhrase(UtwigPhrase::AppreciateSympathy);

		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_STACK1, static_cast<uint32_t>(4));
		return;
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatAboutYou1))
	{
		NPCPhrase(UtwigPhrase::AboutUs1);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatAboutYou2))
	{
		NPCPhrase(UtwigPhrase::AboutUs2);

		LastStack = 2;
		StartSphereTracking(RaceId::Supox);
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatAboutYou3))
	{
		NPCPhrase(UtwigPhrase::AboutUs3);

		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_WAR_NEWS, static_cast<uint32_t>(3));
		SetHomeworldKnown(*HomeId::Supox);
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatAboutUrquan1))
	{
		NPCPhrase(UtwigPhrase::AboutUrquan1);

		LastStack = 3;
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatAboutUrquan2))
	{
		NPCPhrase(UtwigPhrase::AboutUrquan2);

		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_STACK2, static_cast<uint32_t>(2));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::UTWIG_STACK1))
	{
		case 0:
			pStr[0] = UtwigPhrase::WeAreVindicator;
			break;
		case 1:
			pStr[0] = UtwigPhrase::WhySad;
			break;
		case 2:
			pStr[0] = UtwigPhrase::WhatUltron;
			break;
		case 3:
			pStr[0] = UtwigPhrase::DontBeBabies;
			pStr[1] = UtwigPhrase::RealSorryAboutUltron;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::UTWIG_WAR_NEWS))
	{
		case 0:
			pStr[2] = UtwigPhrase::WhatAboutYou1;
			break;
		case 1:
			pStr[2] = UtwigPhrase::WhatAboutYou2;
			break;
		case 2:
			pStr[2] = UtwigPhrase::WhatAboutYou3;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::UTWIG_STACK2))
	{
		case 0:
			pStr[2] = UtwigPhrase::WhatAboutUrquan1;
			break;
		case 1:
			pStr[2] = UtwigPhrase::WhatAboutUrquan2;
			break;
	}

	if (pStr[LastStack] != UtwigPhrase::NullPhrase)
	{
		Response(pStr[LastStack], NeutralUtwig);
	}
	for (i = 0; i < 4; ++i)
	{
		if (i != LastStack && pStr[i] != UtwigPhrase::NullPhrase)
		{
			Response(pStr[i], NeutralUtwig);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION))
	{
		Response(UtwigPhrase::GotUltron, ExitConversation);
	}
	Response(UtwigPhrase::ByeNeutral, ExitConversation);
}

static void BombWorld(ResponseRef R)
{
	uqstl::array<UtwigPhrase, 2> pStr;

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = UtwigPhrase::NullPhrase;
	if (PlayerSaid(R, UtwigPhrase::WhyYouHere))
	{
		NPCPhrase(UtwigPhrase::WeGuardBomb);

		uqm::gameState().setFlag(uqm::GameFlag::BOMB_STACK1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatAboutBomb))
	{
		NPCPhrase(UtwigPhrase::AboutBomb);

		uqm::gameState().setFlag(uqm::GameFlag::BOMB_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, UtwigPhrase::GiveUsBombOrDie))
	{
		NPCPhrase(UtwigPhrase::GuardsWarn);

		uqm::gameState().setFlag(uqm::GameFlag::BOMB_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, UtwigPhrase::DemandBomb))
	{
		NPCPhrase(UtwigPhrase::GuardsFight);

		setSegue(Segue::Hostile);
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOSTILE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(0));
		return;
	}
	else if (PlayerSaid(R, UtwigPhrase::MayWeHaveBomb))
	{
		NPCPhrase(UtwigPhrase::NoBomb);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UtwigPhrase::Please))
	{
		NPCPhrase(UtwigPhrase::SorryNoBomb);

		uqm::gameState().setFlag(uqm::GameFlag::BOMB_STACK2, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, UtwigPhrase::WhatsUpBomb))
	{
		if (uqm::gameState().flag(uqm::GameFlag::BOMB_INFO))
		{
			NPCPhrase(UtwigPhrase::GeneralInfoBomb2);
		}
		else
		{
			NPCPhrase(UtwigPhrase::GeneralInfoBomb1);

			uqm::gameState().setFlag(uqm::GameFlag::BOMB_INFO, static_cast<uint32_t>(1));
		}

		DisablePhrase(UtwigPhrase::WhatsUpBomb);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::BOMB_STACK2))
	{
		case 0:
			pStr[1] = UtwigPhrase::MayWeHaveBomb;
			break;
		case 1:
			pStr[1] = UtwigPhrase::Please;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::BOMB_STACK1))
	{
		case 0:
			pStr[0] = UtwigPhrase::WhyYouHere;
			pStr[1] = UtwigPhrase::NullPhrase;
			break;
		case 1:
			pStr[0] = UtwigPhrase::WhatAboutBomb;
			pStr[1] = UtwigPhrase::NullPhrase;
			break;
		case 2:
			pStr[0] = UtwigPhrase::GiveUsBombOrDie;
			break;
		case 3:
			pStr[0] = UtwigPhrase::DemandBomb;
			break;
	}

	if (pStr[LastStack] != UtwigPhrase::NullPhrase)
	{
		Response(pStr[LastStack], BombWorld);
	}
	LastStack ^= 1;
	if (pStr[LastStack] != UtwigPhrase::NullPhrase)
	{
		Response(pStr[LastStack], BombWorld);
	}

	if (PhraseEnabled(UtwigPhrase::WhatsUpBomb) && (uqm::gameState().flag(uqm::GameFlag::BOMB_STACK1) > 1))
	{
		Response(UtwigPhrase::WhatsUpBomb, BombWorld);
	}

	if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION)
		&& !uqm::gameState().flag(uqm::GameFlag::REFUSED_ULTRON_AT_BOMB))
	{
		Response(UtwigPhrase::GotUltron, ExitConversation);
	}

	if (uqm::gameState().flag(uqm::GameFlag::BOMB_INFO))
	{
		Response(UtwigPhrase::ByeBomb, ExitConversation);
	}
	else
	{
		Response(UtwigPhrase::ByeNeutral, ExitConversation);
	}
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(UtwigPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::UTWIG_HOSTILE))
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UtwigPhrase::HostileBombHello1);
					break;
				case 1:
					NPCPhrase(UtwigPhrase::HostileBombHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UtwigPhrase::HostileHomeworldHello1);
					break;
				case 1:
					NPCPhrase(UtwigPhrase::HostileHomeworldHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UtwigPhrase::HostileSpaceHello1);
					break;
				case 1:
					NPCPhrase(UtwigPhrase::HostileSpaceHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(NumVisits));
		}

		if (!uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION)
			|| (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6)))
		{
			setSegue(Segue::Hostile);
		}
		else
		{
			Response(UtwigPhrase::HeyWaitGotUltron, ExitConversation);
		}
	}
	else if (CheckAlliance(RaceId::Utwig) == RaceAllegiance::Good)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UtwigPhrase::AlliedHomeworldHello1);
					break;
				case 1:
					NPCPhrase(UtwigPhrase::AlliedHomeworldHello2);
					break;
				case 2:
					NPCPhrase(UtwigPhrase::AlliedHomeworldHello3);
					break;
				case 3:
					NPCPhrase(UtwigPhrase::AlliedHomeworldHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			AlliedHome((ResponseRef)0);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
			if (NumVisits == 1)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(UtwigPhrase::HelloBeforeKohrahSpace1);
						break;
					case 1:
						NPCPhrase(UtwigPhrase::HelloBeforeKohrahSpace2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(NumVisits));

				BeforeKohrAh((ResponseRef)0);
			}
			else if (NumVisits < 5)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(UtwigPhrase::HelloDuringKohrahSpace1);
						break;
					case 1:
						NPCPhrase(UtwigPhrase::HelloDuringKohrahSpace2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(UtwigPhrase::HelloAfterKohrahSpace1);
						break;
					case 1:
						NPCPhrase(UtwigPhrase::HelloAfterKohrahSpace2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(NumVisits));

				AfterKohrAh((ResponseRef)0);
			}
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(UtwigPhrase::BombWorldHello1);
				break;
			case 1:
				NPCPhrase(UtwigPhrase::BombWorldHello2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_VISITS, static_cast<uint32_t>(NumVisits));

		BombWorld((ResponseRef)0);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UtwigPhrase::NeutralHomeworldHello1);
					break;
				case 1:
					NPCPhrase(UtwigPhrase::NeutralHomeworldHello2);
					break;
				case 2:
					NPCPhrase(UtwigPhrase::NeutralHomeworldHello3);
					break;
				case 3:
					NPCPhrase(UtwigPhrase::NeutralHomeworldHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UtwigPhrase::NeutralSpaceHello1);
					break;
				case 1:
					NPCPhrase(UtwigPhrase::NeutralSpaceHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_VISITS, static_cast<uint32_t>(NumVisits));
		}

		NeutralUtwig((ResponseRef)0);
	}
}

static uint16_t uninit_utwig(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_utwig_enc(void)
{
	// nothing defined so far
}

ConversationData* init_utwig_comm(void)
{
	utwig_desc.init_encounter_func = Intro;
	utwig_desc.post_encounter_func = post_utwig_enc;
	utwig_desc.uninit_encounter_func = uninit_utwig;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	utwig_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	utwig_desc.AlienTextBaseline.y = ResScale(64);
	utwig_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	// use alternate 'Happy Utwig!' track
	if (uqm::gameState().flag(uqm::GameFlag::UTWIG_HAVE_ULTRON))
	{
		altResFlags |= AltResFlags::Song;
	}


	if (uqm::gameState().flag(uqm::GameFlag::UTWIG_HAVE_ULTRON)
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&utwig_desc};

	return (retval);
}

} // namespace uqm
