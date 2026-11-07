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

#include "uqm/gameev.h"
#include "uqm/build.h"
#include "uqm/races.h"
#include "uqm/lua/UQMLua.h"


static ConversationData ilwrath_desc =
	{
		Conversation::Ilwrath, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		IlwrathPmapAnim, /* AlienFrame */
		IlwrathFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Middle, /* AlienTextValign */
		IlwrathColorMap, /* AlienColorMap */
		IlwrathMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		IlwrathConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				6,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				11,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				21,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
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
			5, /* NumFrames */
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

static void CombatIsInevitable(RESPONSE_REF R)
{
	const bool isGTFO = PlayerSaid(R, IlwrathPhrase::Gtfo);
	setSegue(Segue::Hostile);

	if (PlayerSaid(R, IlwrathPhrase::YouAreWeak))
	{
		NPCPhrase(IlwrathPhrase::StrengthNotAll);
	}
	else if (PlayerSaid(R, IlwrathPhrase::SlayByThousands))
	{
		NPCPhrase(IlwrathPhrase::NoSlayByThousands);
	}
	else if (PlayerSaid(R, IlwrathPhrase::EaseUp))
	{
		NPCPhrase(IlwrathPhrase::NoEaseUp);
	}
	else if (PlayerSaid(R, IlwrathPhrase::ByeSpace))
	{
		NPCPhrase(IlwrathPhrase::GoodbyeAndDieSpace);
	}
	else if (PlayerSaid(R, IlwrathPhrase::ByeHomeworld))
	{
		NPCPhrase(IlwrathPhrase::GoodbyeAndDieHomeworld);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WantPeace))
	{
		NPCPhrase(IlwrathPhrase::NoPeace);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WantAlliance))
	{
		NPCPhrase(IlwrathPhrase::NoAlliance);
	}
	else if (PlayerSaid(R, IlwrathPhrase::ButEvilIsDefined))
	{
		NPCPhrase(IlwrathPhrase::DontConfuseUs);
	}
	else if (PlayerSaid(R, IlwrathPhrase::ByeGods) || isGTFO)
	{
		NPCPhrase(IlwrathPhrase::GoodbyeGods);

		GTFO = isGTFO;

		setSegue(Segue::Peace);
	}
	if (PlayerSaid(R, IlwrathPhrase::WhatsUp))
	{
		NPCPhrase(IlwrathPhrase::GeneralInfo);
		Response(IlwrathPhrase::Bye, CombatIsInevitable);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WhatsUpSpace1)
			 || PlayerSaid(R, IlwrathPhrase::WhatsUpSpace2)
			 || PlayerSaid(R, IlwrathPhrase::WhatsUpSpace3)
			 || PlayerSaid(R, IlwrathPhrase::WhatsUpSpace4)
			 || PlayerSaid(R, IlwrathPhrase::WhatsUpSpace5))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(IlwrathPhrase::GeneralInfoSpace1);
				break;
			case 1:
				NPCPhrase(IlwrathPhrase::GeneralInfoSpace2);
				break;
			case 2:
				NPCPhrase(IlwrathPhrase::GeneralInfoSpace3);
				SetHomeworldKnown(*HomeId::Ilwrath);
				break;
			case 3:
				NPCPhrase(IlwrathPhrase::GeneralInfoSpace4);
				break;
			case 4:
				NPCPhrase(IlwrathPhrase::GeneralInfoSpace5);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_INFO, static_cast<uint32_t>(NumVisits));
	}
	else
	{
		if (PlayerSaid(R, IlwrathPhrase::Bye))
		{
			NPCPhrase(IlwrathPhrase::GoodbyeAndDie);
		}
		else if (PlayerSaid(R, IlwrathPhrase::WhereYouComeFrom))
		{
			NPCPhrase(IlwrathPhrase::CameFrom);
		}
		if (PlayerSaid(R, IlwrathPhrase::ItWillBeAPleasure))
		{
			NPCPhrase(IlwrathPhrase::WhoBlastsWho);
		}
		if (PlayerSaid(R, IlwrathPhrase::Surrender))
		{
			NPCPhrase(IlwrathPhrase::NoSurrender);
		}
		if (PlayerSaid(R, IlwrathPhrase::BeReasonable))
		{
			NPCPhrase(IlwrathPhrase::NotReasonable);
		}
	}
}

static void IlwrathHome(RESPONSE_REF R);

static void IlwrathGods(RESPONSE_REF R)
{
	uint8_t GodsLeft {false};
	if (PlayerSaid(R, IlwrathPhrase::WantInfoOnGods))
	{
		NPCPhrase(IlwrathPhrase::SoMuchToKnow);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WhenStartWorship))
	{
		NPCPhrase(IlwrathPhrase::LongAgo);

		DisablePhrase(IlwrathPhrase::WhenStartWorship);
	}
	else if (PlayerSaid(R, IlwrathPhrase::AnyGoodGods))
	{
		NPCPhrase(IlwrathPhrase::KilledGoodGods);

		DisablePhrase(IlwrathPhrase::AnyGoodGods);
	}
	else if (PlayerSaid(R, IlwrathPhrase::HowTalkWithGods))
	{
		NPCPhrase(IlwrathPhrase::Channel44);

		DisablePhrase(IlwrathPhrase::HowTalkWithGods);
	}
	else if (PlayerSaid(R, IlwrathPhrase::Why44))
	{
		NPCPhrase(IlwrathPhrase::Because44);

		DisablePhrase(IlwrathPhrase::Why44);
	}

	if (PhraseEnabled(IlwrathPhrase::WhenStartWorship))
	{
		Response(IlwrathPhrase::WhenStartWorship, IlwrathGods);
		GodsLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::AnyGoodGods))
	{
		Response(IlwrathPhrase::AnyGoodGods, IlwrathGods);
		GodsLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::HowTalkWithGods))
	{
		Response(IlwrathPhrase::HowTalkWithGods, IlwrathGods);
		GodsLeft = true;
	}
	else if (PhraseEnabled(IlwrathPhrase::Why44))
	{
		Response(IlwrathPhrase::Why44, IlwrathGods);
		GodsLeft = true;
	}
	Response(IlwrathPhrase::EnoughGods, IlwrathHome);

	if (!GodsLeft)
	{
		DisablePhrase(IlwrathPhrase::WantInfoOnGods);
	}
}

static void IlwrathInfo(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, IlwrathPhrase::WantInfoOnIlwrath))
	{
		NPCPhrase(IlwrathPhrase::WhatAboutIlwrath);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WhatAboutPhysio))
	{
		NPCPhrase(IlwrathPhrase::AboutPhysio);

		DisablePhrase(IlwrathPhrase::WhatAboutPhysio);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WhatAboutHistory))
	{
		NPCPhrase(IlwrathPhrase::AboutHistory);

		DisablePhrase(IlwrathPhrase::WhatAboutHistory);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WhatAboutCulture))
	{
		NPCPhrase(IlwrathPhrase::AboutCulture);

		DisablePhrase(IlwrathPhrase::WhatAboutCulture);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WhatAboutUrquan))
	{
		NPCPhrase(IlwrathPhrase::UrquanTooNice);

		DisablePhrase(IlwrathPhrase::WhatAboutUrquan);
	}
	else if (PlayerSaid(R, IlwrathPhrase::AreYouEvil))
	{
		NPCPhrase(IlwrathPhrase::OfCourseWereEvil);

		DisablePhrase(IlwrathPhrase::AreYouEvil);
	}

	if (PhraseEnabled(IlwrathPhrase::WhatAboutPhysio))
	{
		Response(IlwrathPhrase::WhatAboutPhysio, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::WhatAboutHistory))
	{
		Response(IlwrathPhrase::WhatAboutHistory, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::WhatAboutCulture))
	{
		Response(IlwrathPhrase::WhatAboutCulture, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::WhatAboutUrquan))
	{
		Response(IlwrathPhrase::WhatAboutUrquan, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::AreYouEvil))
	{
		Response(IlwrathPhrase::AreYouEvil, IlwrathInfo);
		InfoLeft = true;
	}
	else
	{
		Response(IlwrathPhrase::ButEvilIsDefined, CombatIsInevitable);
		InfoLeft = true;
	}
	Response(IlwrathPhrase::EnoughIlwrath, IlwrathHome);

	if (!InfoLeft)
	{
		DisablePhrase(IlwrathPhrase::WantInfoOnIlwrath);
	}
}

static void IlwrathHome(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(IlwrathPhrase::InitHomeHello);
				break;
			case 1:
				NPCPhrase(IlwrathPhrase::SubsequentHomeHello);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, IlwrathPhrase::EnoughGods))
	{
		NPCPhrase(IlwrathPhrase::OkEnoughGods);
	}
	else if (PlayerSaid(R, IlwrathPhrase::EnoughIlwrath))
	{
		NPCPhrase(IlwrathPhrase::OkEnoughIlwrath);
	}

	if (PhraseEnabled(IlwrathPhrase::WantInfoOnGods))
	{
		Response(IlwrathPhrase::WantInfoOnGods, IlwrathGods);
	}
	if (PhraseEnabled(IlwrathPhrase::WantInfoOnIlwrath))
	{
		Response(IlwrathPhrase::WantInfoOnIlwrath, IlwrathInfo);
	}
	Response(IlwrathPhrase::WantPeace, CombatIsInevitable);
	Response(IlwrathPhrase::WantAlliance, CombatIsInevitable);
	Response(IlwrathPhrase::ByeHomeworld, CombatIsInevitable);
}

static void GodsSpeak(RESPONSE_REF R);

static void GodsOrder(RESPONSE_REF R)
{
	uint8_t OrdersLeft {false};
	if (PlayerSaid(R, IlwrathPhrase::OtherDivineOrders))
	{
		NPCPhrase(IlwrathPhrase::WhatOrders);
	}
	else if (PlayerSaid(R, IlwrathPhrase::SayWarship))
	{
		NPCPhrase(IlwrathPhrase::OkWarship);

		DisablePhrase(IlwrathPhrase::SayWarship);
	}
	else if (PlayerSaid(R, IlwrathPhrase::SayDwe))
	{
		NPCPhrase(IlwrathPhrase::OkDwe);

		DisablePhrase(IlwrathPhrase::SayDwe);
	}
	else if (PlayerSaid(R, IlwrathPhrase::SayYouboo))
	{
		NPCPhrase(IlwrathPhrase::OkYouboo);

		DisablePhrase(IlwrathPhrase::SayYouboo);
	}
	else if (PlayerSaid(R, IlwrathPhrase::SayDillrat))
	{
		NPCPhrase(IlwrathPhrase::OkDilrat);

		DisablePhrase(IlwrathPhrase::SayDillrat);
	}

	if (PhraseEnabled(IlwrathPhrase::SayWarship))
	{
		Response(IlwrathPhrase::SayWarship, GodsOrder);
		OrdersLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::SayDwe))
	{
		Response(IlwrathPhrase::SayDwe, GodsOrder);
		OrdersLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::SayYouboo))
	{
		Response(IlwrathPhrase::SayYouboo, GodsOrder);
		OrdersLeft = true;
	}
	if (PhraseEnabled(IlwrathPhrase::SayDillrat))
	{
		Response(IlwrathPhrase::SayDillrat, GodsOrder);
		OrdersLeft = true;
	}
	Response(IlwrathPhrase::EnoughOrders, GodsSpeak);

	if (!OrdersLeft)
	{
		DisablePhrase(IlwrathPhrase::OtherDivineOrders);
	}
}

static void GodsSpeak(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (R == 0)
	{
		if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
		{
			NPCPhrase(IlwrathPhrase::GloriousWorship);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			NPCPhrase(IlwrathPhrase::OnWay);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_GODS_SPOKEN);

			if (NumVisits > 3)
			{
				NumVisits = 0;
			}

			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(IlwrathPhrase::IlwrathBelieve);
					break;
				case 1:
					NPCPhrase(IlwrathPhrase::GodsReturn1);
					break;
				case 2:
					NPCPhrase(IlwrathPhrase::GodsReturn2);
					break;
				case 3:
					NPCPhrase(IlwrathPhrase::GodsReturn3);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_GODS_SPOKEN, static_cast<uint32_t>(NumVisits));
		}
	}
	else if (PlayerSaid(R, IlwrathPhrase::GoKillThraddash))
	{
		NPCPhrase(IlwrathPhrase::OkKillThraddash);

		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_DECEIVED, static_cast<uint32_t>(1));
		AddEvent(EventType::Relative, 0, 0, 0, GameEventId::AdvanceIlwrathMission);
	}
	else if (PlayerSaid(R, IlwrathPhrase::WorshipUs))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_WORSHIP);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(IlwrathPhrase::WeWorship1);
				break;
			case 1:
				NPCPhrase(IlwrathPhrase::WeWorship2);
				break;
			case 2:
				NPCPhrase(IlwrathPhrase::WeWorship3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_WORSHIP, static_cast<uint32_t>(NumVisits));

		DisablePhrase(IlwrathPhrase::WorshipUs);
	}
	else if (PlayerSaid(R, IlwrathPhrase::EnoughOrders))
	{
		NPCPhrase(IlwrathPhrase::NeverEnough);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
	{
		Response(IlwrathPhrase::GoKillThraddash, GodsSpeak);
	}
	if (PhraseEnabled(IlwrathPhrase::WorshipUs))
	{
		Response(IlwrathPhrase::WorshipUs, GodsSpeak);
	}
	if (PhraseEnabled(IlwrathPhrase::OtherDivineOrders))
	{
		Response(IlwrathPhrase::OtherDivineOrders, GodsOrder);
	}
	Response(IlwrathPhrase::ByeGods, CombatIsInevitable);
}

static void IlwrathSpace(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (R == 0)
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(IlwrathPhrase::InitHelloSpace);
				break;
			case 1:
				NPCPhrase(IlwrathPhrase::SubsequentHelloSpace1);
				break;
			case 2:
				NPCPhrase(IlwrathPhrase::SubsequentHelloSpace2);
				break;
			case 3:
				NPCPhrase(IlwrathPhrase::SubsequentHelloSpace3);
				break;
			case 4:
				NPCPhrase(IlwrathPhrase::SubsequentHelloSpace4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_VISITS, static_cast<uint32_t>(NumVisits));
	}

	NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_INFO);
	switch (NumVisits)
	{
		case 0:
			Response(IlwrathPhrase::WhatsUpSpace1, CombatIsInevitable);
			break;
		case 1:
			Response(IlwrathPhrase::WhatsUpSpace2, CombatIsInevitable);
			break;
		case 2:
			Response(IlwrathPhrase::WhatsUpSpace3, CombatIsInevitable);
			break;
		case 3:
			Response(IlwrathPhrase::WhatsUpSpace4, CombatIsInevitable);
			break;
		case 4:
			Response(IlwrathPhrase::WhatsUpSpace5, CombatIsInevitable);
			break;
	}
	Response(IlwrathPhrase::YouAreWeak, CombatIsInevitable);
	Response(IlwrathPhrase::SlayByThousands, CombatIsInevitable);
	Response(IlwrathPhrase::EaseUp, CombatIsInevitable);
	Response(IlwrathPhrase::ByeSpace, CombatIsInevitable);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::gameState().flag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER))
	{
		NPCPhrase(IlwrathPhrase::SendMessage);

		Response(IlwrathPhrase::WhereYouComeFrom, CombatIsInevitable);
		Response(IlwrathPhrase::ItWillBeAPleasure, CombatIsInevitable);
		Response(IlwrathPhrase::Surrender, CombatIsInevitable);
		Response(IlwrathPhrase::BeReasonable, CombatIsInevitable);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		IlwrathHome((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_CHMMR_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(IlwrathPhrase::InitChmmrHello);
				break;
			case 1:
				NPCPhrase(IlwrathPhrase::SubsequentChmmrHello);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_CHMMR_VISITS, static_cast<uint32_t>(NumVisits));

		Response(IlwrathPhrase::WhatsUp, CombatIsInevitable);
		Response(IlwrathPhrase::Bye, CombatIsInevitable);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 5))
	{
		// Communicating with an Ilwrath ship using a HyperWave Broadcaster.
		if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
		{
			NPCPhrase(IlwrathPhrase::BigFun);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			NPCPhrase(IlwrathPhrase::FastAsCan);
		}
		else
		{
			if (IsExtended())
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_GODS_SPOKEN);

				if (NumVisits < 5)
				{
					Response(IlwrathPhrase::Gtfo, CombatIsInevitable);
					Response(IlwrathPhrase::ByeGods, CombatIsInevitable);
				}

				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(IlwrathPhrase::JustGrunts);
						break;
					case 1:
						NPCPhrase(IlwrathPhrase::IlwrathBelieve);
						break;
					case 2:
						NPCPhrase(IlwrathPhrase::GodsReturn1);
						break;
					case 3:
						NPCPhrase(IlwrathPhrase::GodsReturn2);
						break;
					case 4:
						NPCPhrase(IlwrathPhrase::GodsReturn3);
						break;
					case 5:
						NPCPhrase(IlwrathPhrase::Deceivers);
						GTFO = 2;
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_GODS_SPOKEN, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				NPCPhrase(IlwrathPhrase::JustGrunts);
			}

			setSegue(Segue::Peace);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 4))
	{
		// Communicating with the Ilwrath homeworld using a
		// Hyperwave Broadcaster.
		GodsSpeak((RESPONSE_REF)0);
	}
	else
	{
		setSegue(Segue::Peace);

		if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
		{
			NPCPhrase(IlwrathPhrase::HappyFightingThraddash);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			NPCPhrase(IlwrathPhrase::OnWayToThraddash);
		}
		else
		{
			IlwrathSpace((RESPONSE_REF)0);
		}
	}
}

static uint16_t uninit_ilwrath(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_ilwrath_enc(void)
{
	// nothing defined so far
}

ConversationData* init_ilwrath_comm(void)
{
	ilwrath_desc.init_encounter_func = Intro;
	ilwrath_desc.post_encounter_func = post_ilwrath_enc;
	ilwrath_desc.uninit_encounter_func = uninit_ilwrath;

	uqm::lua().beginComm(nullptr, NullResource);

	ilwrath_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	ilwrath_desc.AlienTextBaseline.y = ResScale(64);
	ilwrath_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER)
		|| (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA)
			& ((1 << 4) | (1 << 5)))
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&ilwrath_desc};

	return (retval);
}
