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

#include "options.h"
#include "../commall.h"
#include "resinst.h"
#include "strings.h"

#include "uqm/gameev.h"
#include "uqm/races.h"
#include "uqm/lua/UQMLua.h"


static ConversationData arilou_desc =
	{
		Conversation::Arilou, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		ArilouPmapAnim, /* AlienFrame */
		ArilouFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		ArilouColorMap, /* AlienColorMap */
		ArilouMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		ArilouConversationPhrases, /* PlayerPhrases */
		20, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				4,											/* StartIndex */
				9,											/* NumFrames */
				CircularAnim,								/* AnimFlags */
				GameTicksPerSecond / 30, 0,					/* FrameRate */
				GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
				(1 << 1) | (1L << 16)},
								{13,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 0) | (1L << 16)},
								{22,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1L << 16)},
								{31,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 4)},
								{40,										 /* StartIndex */
			 10,										 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 3)},
								{50,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 7)},
								{
				59,			  /* StartIndex */
				8,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{67,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 5)},
								{76,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 9)},
								{85,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 8)},
								{
				94,			  /* StartIndex */
				9,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{103,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 13)},
								{
				112,		  /* StartIndex */
				9,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{121,										 /* StartIndex */
			 8,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 11)},
								{129,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1L << 15)},
								{138,										 /* StartIndex */
			 8,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 14)},
								{146,										 /* StartIndex */
			 9,											 /* NumFrames */
			 CircularAnim,								 /* AnimFlags */
			 GameTicksPerSecond / 30, 0,				 /* FrameRate */
			 GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
			 (1 << 0) | (1 << 1) | (1 << 2)},
								{
				/* Hands moving (right up) */
				155,						/* StartIndex */
				2,							/* NumFrames */
				CircularAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 19),				/* BlockMask */
			},
								{
				/* Hands moving (left up) */
				157,						/* StartIndex */
				2,							/* NumFrames */
				CircularAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 19),				/* BlockMask */
			},
								{
				/* Stars flashing next to the head */
				159,		  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 17) | (1 << 18),	/* BlockMask */
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

	if (PlayerSaid(R, ArilouPhrase::ByeAngrySpace))
	{
		NPCPhrase(ArilouPhrase::GoodbyeAngrySpace);
	}
	else if (PlayerSaid(R, ArilouPhrase::ByeFriendlySpace))
	{
		NPCPhrase(ArilouPhrase::GoodbyeFriendlySpace);
	}
	else if (PlayerSaid(R, ArilouPhrase::ByeFriendlyHomeworld))
	{
		NPCPhrase(ArilouPhrase::GoodbyeFrdlyHomeworld);
	}
	else if (PlayerSaid(R, ArilouPhrase::LetsFight))
	{
		NPCPhrase(ArilouPhrase::NoFight);
	}
	else if (PlayerSaid(R, ArilouPhrase::BugEyedFruitcakes))
	{
		NPCPhrase(ArilouPhrase::WeNeverFriends);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_MANNER, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ArilouPhrase::BestIfIKilledYou))
	{
		NPCPhrase(ArilouPhrase::WickedHuman);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_MANNER, static_cast<uint32_t>(2));
	}
}

static void ArilouHome(RESPONSE_REF R)
{
	uint8_t i {};
	ArilouPhrase pStr[4];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = pStr[3] = ArilouPhrase::NullPhrase;
	if (PlayerSaid(R, ArilouPhrase::ConfusedByHello))
	{
		NPCPhrase(ArilouPhrase::ConfusedResponse);
	}
	else if (PlayerSaid(R, ArilouPhrase::HappyByHello))
	{
		NPCPhrase(ArilouPhrase::HappyResponse);
	}
	else if (PlayerSaid(R, ArilouPhrase::MiffedByHello))
	{
		NPCPhrase(ArilouPhrase::MiffedResponse);
	}
	else if (PlayerSaid(R, ArilouPhrase::OkLetsBeFriends))
	{
		NPCPhrase(ArilouPhrase::NoAllyButMuchGive);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatAboutWar))
	{
		NPCPhrase(ArilouPhrase::AboutWar);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_1, static_cast<uint32_t>(1));
		SetHomeworldKnown(*HomeId::Shofixti);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatAboutUrquan))
	{
		NPCPhrase(ArilouPhrase::AboutUrquan);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_1, static_cast<uint32_t>(2));
		SetHomeworldKnown(*HomeId::Syreen);
	}
	else if (PlayerSaid(R, ArilouPhrase::TellArilouAboutTpet))
	{
		NPCPhrase(ArilouPhrase::BadNewsAboutTpet);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatDoAboutTpet))
	{
		NPCPhrase(ArilouPhrase::DangerousButUseful);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_2, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ArilouPhrase::LearnedAboutUmgah))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_CHECKED_UMGAH) != 2)
		{
			NPCPhrase(ArilouPhrase::NoNewsYet);
		}
		else
		{
			NPCPhrase(ArilouPhrase::UmgahUnderCompulsion);

			LastStack = 1;

			SetHomeworldKnown(*HomeId::Umgah);
		}

		DisablePhrase(ArilouPhrase::LearnedAboutUmgah);
	}
	else if (PlayerSaid(R, ArilouPhrase::UmgahActingWeird))
	{
		NPCPhrase(ArilouPhrase::WellGoCheck);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_CHECKED_UMGAH, static_cast<uint32_t>(1));
		AddEvent(EventType::Relative, 0, 10, 0, GameEventId::ArilouUmgahCheck);
		DisablePhrase(ArilouPhrase::UmgahActingWeird);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatDoNow))
	{
		NPCPhrase(ArilouPhrase::GoFindOut);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_CHECKED_UMGAH, static_cast<uint32_t>(3));
		SetHomeworldKnown(*HomeId::Umgah);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatDidOnEarth))
	{
		NPCPhrase(ArilouPhrase::DidThis);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ArilouPhrase::WhyDidThis))
	{
		NPCPhrase(ArilouPhrase::IdfParasites);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_3, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ArilouPhrase::TellMore))
	{
		NPCPhrase(ArilouPhrase::NotNow);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_3, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatGiveMe))
	{
		NPCPhrase(ArilouPhrase::AboutPortal);

		LastStack = 3;
		uqm::gameState().setFlag(uqm::GameFlag::KNOW_ARILOU_WANT_WRECK, static_cast<uint32_t>(1));

		R = *ArilouPhrase::AboutPortalAgain;
		DisablePhrase(ArilouPhrase::WhatGiveMe);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatAboutTpet))
	{
		NPCPhrase(ArilouPhrase::AboutTpet);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_4, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ArilouPhrase::AboutPortalAgain))
	{
		NPCPhrase(ArilouPhrase::PortalAgain);

		DisablePhrase(ArilouPhrase::AboutPortalAgain);
	}
	else if (PlayerSaid(R, ArilouPhrase::GotIt))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_HOME_VISITS) == 1)
		{
			NPCPhrase(ArilouPhrase::CleverHuman);
		}
		NPCPhrase(ArilouPhrase::GivePortal);

		uqm::gameState().setFlag(uqm::GameFlag::PORTAL_KEY_ON_SHIP, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PORTAL_SPAWNER, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP, static_cast<uint32_t>(1));
	}
#ifdef NEVER
	else if (PlayerSaid(R, got_tpet))
	{
		NPCPhrase(OK_GOT_TPET);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_2, static_cast<uint32_t>(1));
	}
#endif /* NEVER */

	switch (uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_1))
	{
		case 0:
			pStr[0] = ArilouPhrase::WhatAboutWar;
			break;
		case 1:
			pStr[0] = ArilouPhrase::WhatAboutUrquan;
			break;
	}
	if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET))
	{
#ifdef NEVER
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_2) == 0)
		{
			pStr[1] = got_tpet;
		}
#endif /* NEVER */
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_VISITS))
		{
			switch (uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_2))
			{
				case 0:
					pStr[1] = ArilouPhrase::TellArilouAboutTpet;
					break;
				case 1:
					pStr[1] = ArilouPhrase::WhatDoAboutTpet;
					break;
			}
		}
		else if (uqm::gameState().flag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES))
		{
			if (!uqm::gameState().flag(uqm::GameFlag::ARILOU_CHECKED_UMGAH))
			{
				pStr[1] = ArilouPhrase::UmgahActingWeird;
			}
			else if (PhraseEnabled(ArilouPhrase::LearnedAboutUmgah) && PhraseEnabled(ArilouPhrase::UmgahActingWeird))
			{
				pStr[1] = ArilouPhrase::LearnedAboutUmgah;
			}
			else if (uqm::gameState().flag(uqm::GameFlag::ARILOU_CHECKED_UMGAH) == 2)
			{
				pStr[1] = ArilouPhrase::WhatDoNow;
			}
		}
	}
	switch (uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_3))
	{
		case 0:
			pStr[2] = ArilouPhrase::WhatDidOnEarth;
			break;
		case 1:
			pStr[2] = ArilouPhrase::WhyDidThis;
			break;
		case 2:
			pStr[2] = ArilouPhrase::TellMore;
			break;
	}
	if (!uqm::gameState().flag(uqm::GameFlag::KNOW_ARILOU_WANT_WRECK))
	{
		pStr[3] = ArilouPhrase::WhatGiveMe;
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_4))
	{
		pStr[3] = ArilouPhrase::WhatAboutTpet;
	}

	if (pStr[LastStack] != ArilouPhrase::NullPhrase)
	{
		Response(pStr[LastStack], ArilouHome);
	}
	for (i = 0; i < 4; ++i)
	{
		if (i != LastStack && *pStr[i] != ArilouPhrase::NullPhrase)
		{
			Response(pStr[i], ArilouHome);
		}
	}

	if (uqm::gameState().flag(uqm::GameFlag::KNOW_ARILOU_WANT_WRECK))
	{
		if (uqm::gameState().flag(uqm::GameFlag::PORTAL_KEY_ON_SHIP))
		{
			Response(ArilouPhrase::GotIt, ArilouHome);
		}
		else if (PhraseEnabled(ArilouPhrase::AboutPortalAgain) && !uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER))
		{
			Response(ArilouPhrase::AboutPortalAgain, ArilouHome);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER) != 3)
	{
		Response(ArilouPhrase::BestIfIKilledYou, ExitConversation);
	}
	Response(ArilouPhrase::ByeFriendlyHomeworld, ExitConversation);
}

static void AngryHomeArilou(RESPONSE_REF R)
{
	if (PlayerSaid(R, ArilouPhrase::InvadersFromMars))
	{
		NPCPhrase(ArilouPhrase::HadOurReasons);

		DisablePhrase(ArilouPhrase::InvadersFromMars);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhyShouldITrust))
	{
		NPCPhrase(ArilouPhrase::TrustBecause);

		DisablePhrase(ArilouPhrase::WhyShouldITrust);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatAboutInterference))
	{
		NPCPhrase(ArilouPhrase::InterferenceNecessary);

		DisablePhrase(ArilouPhrase::WhatAboutInterference);
	}
	else if (PlayerSaid(R, ArilouPhrase::IJustLikeToLeave))
	{
		NPCPhrase(ArilouPhrase::SorryNoLeave);

		DisablePhrase(ArilouPhrase::IJustLikeToLeave);
	}

	if (PhraseEnabled(ArilouPhrase::InvadersFromMars))
	{
		Response(ArilouPhrase::InvadersFromMars, AngryHomeArilou);
	}
	else
	{
		Response(ArilouPhrase::BugEyedFruitcakes, ExitConversation);
	}

	if (PhraseEnabled(ArilouPhrase::WhyShouldITrust))
	{
		Response(ArilouPhrase::WhyShouldITrust, AngryHomeArilou);
	}
	else if (PhraseEnabled(ArilouPhrase::WhatAboutInterference))
	{
		Response(ArilouPhrase::WhatAboutInterference, AngryHomeArilou);
	}

	Response(ArilouPhrase::OkLetsBeFriends, ArilouHome);

	if (PhraseEnabled(ArilouPhrase::IJustLikeToLeave))
	{
		Response(ArilouPhrase::IJustLikeToLeave, AngryHomeArilou);
	}
}

static void AngrySpaceArilou(RESPONSE_REF R)
{
	if (PlayerSaid(R, ArilouPhrase::ImSorry))
	{
		NPCPhrase(ArilouPhrase::ApologizeAtHomeworld);

		DisablePhrase(ArilouPhrase::ImSorry);
	}

	Response(ArilouPhrase::LetsFight, ExitConversation);
	if (PhraseEnabled(ArilouPhrase::ImSorry))
	{
		Response(ArilouPhrase::ImSorry, AngrySpaceArilou);
	}
	Response(ArilouPhrase::ByeAngrySpace, ExitConversation);
}

static void FriendlySpaceArilou(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, ArilouPhrase::ConfusedByHello))
	{
		NPCPhrase(ArilouPhrase::ConfusedResponse);
	}
	else if (PlayerSaid(R, ArilouPhrase::HappyByHello))
	{
		NPCPhrase(ArilouPhrase::HappyResponse);
	}
	else if (PlayerSaid(R, ArilouPhrase::MiffedByHello))
	{
		NPCPhrase(ArilouPhrase::MiffedResponse);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatsUp1)
			 || PlayerSaid(R, ArilouPhrase::WhatsUp2))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ArilouPhrase::GeneralInfo1);
				break;
			case 1:
				NPCPhrase(ArilouPhrase::GeneralInfo2);
				break;
			case 2:
				NPCPhrase(ArilouPhrase::GeneralInfo3);
				break;
			case 3:
				NPCPhrase(ArilouPhrase::GeneralInfo4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(ArilouPhrase::WhatsUp2);
	}
	else if (PlayerSaid(R, ArilouPhrase::WhyYouHere))
	{
		NPCPhrase(ArilouPhrase::LearnThings);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_5, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ArilouPhrase::WhatThings))
	{
		NPCPhrase(ArilouPhrase::TheseThings);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_5, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ArilouPhrase::WhyDoIt))
	{
		NPCPhrase(ArilouPhrase::DoItBecause);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_5, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, ArilouPhrase::GiveMeInfo1)
			 || PlayerSaid(R, ArilouPhrase::GiveMeInfo2))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_HINTS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ArilouPhrase::ArilouHints1);
				break;
			case 1:
				NPCPhrase(ArilouPhrase::ArilouHints2);
				if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2)
				{
					uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
				}
				break;
			case 2:
				NPCPhrase(ArilouPhrase::ArilouHints3);
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(1));
				break;
			case 3:
				NPCPhrase(ArilouPhrase::ArilouHints4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HINTS, static_cast<uint32_t>(NumVisits));

		DisablePhrase(ArilouPhrase::GiveMeInfo2);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_5))
	{
		case 0:
			Response(ArilouPhrase::WhyYouHere, FriendlySpaceArilou);
			break;
		case 1:
			Response(ArilouPhrase::WhatThings, FriendlySpaceArilou);
			break;
		case 2:
			Response(ArilouPhrase::WhyDoIt, FriendlySpaceArilou);
			break;
	}
	if (PhraseEnabled(ArilouPhrase::WhatsUp2))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_INFO) == 0)
		{
			Response(ArilouPhrase::WhatsUp1, FriendlySpaceArilou);
		}
		else
		{
			Response(ArilouPhrase::WhatsUp2, FriendlySpaceArilou);
		}
	}
	if (PhraseEnabled(ArilouPhrase::GiveMeInfo2))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_HINTS) == 0)
		{
			Response(ArilouPhrase::GiveMeInfo1, FriendlySpaceArilou);
		}
		else
		{
			Response(ArilouPhrase::GiveMeInfo2, FriendlySpaceArilou);
		}
	}
	Response(ArilouPhrase::ByeFriendlySpace, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};
	uint8_t Manner {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(ArilouPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::MET_ARILOU))
	{
		RESPONSE_FUNC RespFunc {};

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			NPCPhrase(ArilouPhrase::InitHello);
			RespFunc = (RESPONSE_FUNC)FriendlySpaceArilou;
		}
		else
		{
			NPCPhrase(ArilouPhrase::FrdlyHomeworldHello1);
			RespFunc = (RESPONSE_FUNC)ArilouHome;
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HOME_VISITS, static_cast<uint32_t>(1));
		}
		Response(ArilouPhrase::ConfusedByHello, RespFunc);
		Response(ArilouPhrase::HappyByHello, RespFunc);
		Response(ArilouPhrase::MiffedByHello, RespFunc);
		uqm::gameState().setFlag(uqm::GameFlag::MET_ARILOU, static_cast<uint32_t>(1));
		return;
	}

	Manner = uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER);
	if (Manner == 2)
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ArilouPhrase::HostileGoodbye1);
				break;
			case 1:
				NPCPhrase(ArilouPhrase::HostileGoodbye2);
				break;
			case 2:
				NPCPhrase(ArilouPhrase::HostileGoodbye3);
				break;
			case 3:
				NPCPhrase(ArilouPhrase::HostileGoodbye4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Peace);
	}
	else if (Manner == 1)
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) > 1)
		{
			NPCPhrase(ArilouPhrase::InitAngryHwldHello);
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HOME_VISITS, static_cast<uint32_t>(1));

			AngryHomeArilou((RESPONSE_REF)0);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ArilouPhrase::AngrySpaceHello1);
					break;
				case 1:
					NPCPhrase(ArilouPhrase::AngrySpaceHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_VISITS, static_cast<uint32_t>(NumVisits));

			AngrySpaceArilou((RESPONSE_REF)0);
		}
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ArilouPhrase::FriendlySpaceHello1);
					break;
				case 1:
					NPCPhrase(ArilouPhrase::FriendlySpaceHello2);
					break;
				case 2:
					NPCPhrase(ArilouPhrase::FriendlySpaceHello3);
					break;
				case 3:
					NPCPhrase(ArilouPhrase::FriendlySpaceHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_VISITS, static_cast<uint32_t>(NumVisits));

			FriendlySpaceArilou((RESPONSE_REF)0);
		}
		else
		{
			if (!uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER)
				&& uqm::gameState().flag(uqm::GameFlag::KNOW_ARILOU_WANT_WRECK))
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::NO_PORTAL_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ArilouPhrase::GotPartYet1);
						break;
					case 1:
						NPCPhrase(ArilouPhrase::GotPartYet1);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::NO_PORTAL_VISITS, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_HOME_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ArilouPhrase::FrdlyHomeworldHello1);
						break;
					case 1:
						NPCPhrase(ArilouPhrase::FrdlyHomeworldHello2);
						break;
					case 2:
						NPCPhrase(ArilouPhrase::FrdlyHomeworldHello3);
						break;
					case 3:
						NPCPhrase(ArilouPhrase::FrdlyHomeworldHello4);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HOME_VISITS, static_cast<uint32_t>(NumVisits));
			}

			ArilouHome((RESPONSE_REF)0);
		}
	}
}

static uint16_t uninit_arilou(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_arilou_enc(void)
{
	uint8_t Manner {};

	if (getSegue() == Segue::Hostile
		&& (Manner = uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HOME_VISITS, static_cast<uint32_t>(0));
		}
	}

	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) > 1
		&& uqm::gameState().flag(uqm::GameFlag::ARILOU_HOME_VISITS) <= 1)
	{
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_HOME_VISITS, static_cast<uint32_t>(0));

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER) < 2)
		{
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_MANNER, static_cast<uint32_t>(3));
		}
	}
}

ConversationData* init_arilou_comm(void)
{
	arilou_desc.init_encounter_func = Intro;
	arilou_desc.post_encounter_func = post_arilou_enc;
	arilou_desc.uninit_encounter_func = uninit_arilou;

	uqm::lua().beginComm(nullptr, NullResource);

	arilou_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	arilou_desc.AlienTextBaseline.y = 0;
	arilou_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) > 1
		|| uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER) == 3
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&arilou_desc};

	return (retval);
}
