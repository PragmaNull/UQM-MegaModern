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
#include "libs/mathlib.h"
#include "uqm/races.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static ConversationData mycon_desc =
	{
		Conversation::Mycon, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		MyconPmapAnim, /* AlienFrame */
		MyconFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		MyconColorMap, /* AlienColorMap */
		MyconMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		MyconConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				12,			  /* StartIndex */
				10,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 80,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								//	{// It was 2 differtent animations that blocks each other
			//	 // but after the commit BlockMask algorithm was changed
			//	 // https://sourceforge.net/p/sc2/uqm/ci/5be8df3e5f4a957fa1ee63851e7e064731aadb20/
			//   // and these animations completely cancel each other.
			//   // Removing BlockMask messes with the animations restart rate,
			//   // so these animations are now merged together
			//		18, /* StartIndex */
			//		4, /* NumFrames */
			//		CircularAnim, /* AnimFlags */
			//		GameTicksPerSecond * 3 / 40, 0, /* FrameRate */
			//		GameTicksPerSecond * 3 / 40, 0, /* RestartRate */
			//		(1 << 0), /* BlockMask */
			//	},
			{
				22,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				28,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				33,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
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
			1,	/* StartIndex */
			11, /* NumFrames */
			0,	/* AnimFlags */
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

static uint8_t MadeChoice;

static void DoRamble(ResponseRef R)
{
	uint8_t Counter {};

	Counter = uqm::gameState().flag(uqm::GameFlag::MYCON_RAMBLE);
	switch (Counter++)
	{
		case 0:
			NPCPhrase(MyconPhrase::Ramble1);
			break;
		case 1:
			NPCPhrase(MyconPhrase::Ramble2);
			break;
		case 2:
			NPCPhrase(MyconPhrase::Ramble3);
			break;
		case 3:
			NPCPhrase(MyconPhrase::Ramble4);
			break;
		case 4:
			NPCPhrase(MyconPhrase::Ramble5);
			break;
		case 5:
			NPCPhrase(MyconPhrase::Ramble6);
			break;
		case 6:
			NPCPhrase(MyconPhrase::Ramble7);
			break;
		case 7:
			NPCPhrase(MyconPhrase::Ramble8);
			break;
		case 8:
			NPCPhrase(MyconPhrase::Ramble9);
			break;
		case 9:
			NPCPhrase(MyconPhrase::Ramble10);
			break;
		case 10:
			NPCPhrase(MyconPhrase::Ramble11);
			break;
		case 11:
			NPCPhrase(MyconPhrase::Ramble12);
			break;
		case 12:
			NPCPhrase(MyconPhrase::Ramble13);
			break;
		case 13:
			NPCPhrase(MyconPhrase::Ramble14);
			break;
		case 14:
			NPCPhrase(MyconPhrase::Ramble15);
			break;
		case 15:
			NPCPhrase(MyconPhrase::Ramble16);
			break;
		case 16:
			NPCPhrase(MyconPhrase::Ramble17);
			break;
		case 17:
			NPCPhrase(MyconPhrase::Ramble18);
			break;
		case 18:
			NPCPhrase(MyconPhrase::Ramble19);
			break;
		case 19:
			NPCPhrase(MyconPhrase::Ramble20);
			break;
		case 20:
			NPCPhrase(MyconPhrase::Ramble21);
			break;
		case 21:
			NPCPhrase(MyconPhrase::Ramble22);
			break;
		case 22:
			NPCPhrase(MyconPhrase::Ramble23);
			break;
		case 23:
			NPCPhrase(MyconPhrase::Ramble24);
			break;
		case 24:
			NPCPhrase(MyconPhrase::Ramble25);
			if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2)
			{
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
			}
			break;
		case 25:
			NPCPhrase(MyconPhrase::Ramble26);
			break;
		case 26:
			NPCPhrase(MyconPhrase::Ramble27);
			break;
		case 27:
			NPCPhrase(MyconPhrase::Ramble28);
			break;
		case 28:
			NPCPhrase(MyconPhrase::Ramble29);
			break;
		case 29:
			NPCPhrase(MyconPhrase::Ramble30);
			break;
		case 30:
			NPCPhrase(MyconPhrase::Ramble31);
			break;
		case 31:
			NPCPhrase(MyconPhrase::Ramble32);
			Counter = 0;
			break;
	}
	uqm::gameState().setFlag(uqm::GameFlag::MYCON_RAMBLE, static_cast<uint32_t>(Counter));

	if (!(uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
	{
		if (!PlayerSaid(R, MyconPhrase::ComeInPeace)
			&& !PlayerSaid(R, MyconPhrase::GonnaDie))
		{
			Counter = (uqm::gameState().flag(uqm::GameFlag::MYCON_INSULTS) + 1) & 7;
			uqm::gameState().setFlag(uqm::GameFlag::MYCON_INSULTS, static_cast<uint32_t>(Counter));
			MadeChoice = 1;
		}
	}
	else if (!PlayerSaid(R, MyconPhrase::LetsBeFriends)
			 && !PlayerSaid(R, MyconPhrase::CameToHomeworld)
			 && !PlayerSaid(R, MyconPhrase::SubmitToUs))
	{
		Counter = (uqm::gameState().flag(uqm::GameFlag::MYCON_INFO) + 1) & 15;
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_INFO, static_cast<uint32_t>(Counter));
		MadeChoice = 1;
	}
}

static void CombatIsInevitable(ResponseRef R)
{
	setSegue(Segue::Hostile);

	if (PlayerSaid(R, MyconPhrase::ByeSpace))
	{
		NPCPhrase(MyconPhrase::ByeAndDieSpace);
	}
	else if (PlayerSaid(R, MyconPhrase::ByeHomeworld))
	{
		NPCPhrase(MyconPhrase::ByeAndDieHomeworld);
	}
	else if (PlayerSaid(R, MyconPhrase::LikeToLand))
	{
		NPCPhrase(MyconPhrase::NeverLetLand);
	}
	else if (PlayerSaid(R, MyconPhrase::ByeSunDevice))
	{
		NPCPhrase(MyconPhrase::GoodbyeSunDevice);

		setSegue(Segue::Peace);
	}
	else
	{
		DoRamble(R);
		if (!(uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
		{
			NPCPhrase(MyconPhrase::ByeAndDieSpace);
		}
		else
		{
			NPCPhrase(MyconPhrase::ByeAndDieHomeworld);
		}
	}
	MadeChoice = 0;
}

static void SunDevice(ResponseRef R)
{
	if (PlayerSaid(R, MyconPhrase::WhatsUpSunDevice))
	{
		NPCPhrase(MyconPhrase::GeneralInfoSunDevice);

		DisablePhrase(MyconPhrase::WhatsUpSunDevice);
	}
	else if (PlayerSaid(R, MyconPhrase::HowGoesImplanting))
	{
		NPCPhrase(MyconPhrase::UnforseenDelays);

		DisablePhrase(MyconPhrase::HowGoesImplanting);
	}
	else if (PlayerSaid(R, MyconPhrase::IHaveACunningPlan))
	{
		NPCPhrase(MyconPhrase::WontFallForTrick);

		uqm::gameState().setFlag(uqm::GameFlag::NO_TRICK_AT_SUN, static_cast<uint32_t>(1));
	}

	if (PhraseEnabled(MyconPhrase::WhatsUpSunDevice))
	{
		Response(MyconPhrase::WhatsUpSunDevice, SunDevice);
	}
	if (uqm::gameState().flag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH))
	{
		if (PhraseEnabled(MyconPhrase::HowGoesImplanting) && uqm::gameState().flag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH))
		{
			Response(MyconPhrase::HowGoesImplanting, SunDevice);
		}
		Response(MyconPhrase::LikeToLand, CombatIsInevitable);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::MYCON_AMBUSH)
			 && !uqm::gameState().flag(uqm::GameFlag::NO_TRICK_AT_SUN))
	{
		Response(MyconPhrase::IHaveACunningPlan, SunDevice);
	}
	Response(MyconPhrase::ByeSunDevice, CombatIsInevitable);
}

static void TrickMycon(ResponseRef R)
{
	if (PlayerSaid(R, MyconPhrase::IHaveACunningPlan))
	{
		NPCPhrase(MyconPhrase::TellUsAboutWorld);

		DisablePhrase(MyconPhrase::IHaveACunningPlan);
	}
	else if (PlayerSaid(R, MyconPhrase::Clue1))
	{
		NPCPhrase(MyconPhrase::Response1);

		DisablePhrase(MyconPhrase::Clue1);
	}
	else if (PlayerSaid(R, MyconPhrase::Clue2))
	{
		NPCPhrase(MyconPhrase::Response2);

		DisablePhrase(MyconPhrase::Clue2);
	}
	else if (PlayerSaid(R, MyconPhrase::Clue3))
	{
		NPCPhrase(MyconPhrase::Response3);

		DisablePhrase(MyconPhrase::Clue3);
	}

	if (PhraseEnabled(MyconPhrase::Clue1) == 0
		&& PhraseEnabled(MyconPhrase::Clue2) == 0
		&& PhraseEnabled(MyconPhrase::Clue3) == 0)
	{
		NPCPhrase(MyconPhrase::WeGoToImplant);

		setSegue(Segue::Peace);
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH, static_cast<uint32_t>(1));
		AddEvent(EventType::Relative, 0, 0, 0, GameEventId::AdvanceMyconMission);
	}
	else
	{
		NPCPhrase(MyconPhrase::AmbushTail);
	}

	if (PhraseEnabled(MyconPhrase::Clue1))
	{
		Response(MyconPhrase::Clue1, TrickMycon);
	}
	if (PhraseEnabled(MyconPhrase::Clue2))
	{
		Response(MyconPhrase::Clue2, TrickMycon);
	}
	if (PhraseEnabled(MyconPhrase::Clue3))
	{
		Response(MyconPhrase::Clue3, TrickMycon);
	}
}

static void NormalMycon(ResponseRef R)
{
	ResponseFunc RespFunc {};

	if (PlayerSaid(R, MyconPhrase::WhatAboutShattered))
	{
		NPCPhrase(MyconPhrase::AboutShattered);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
	}
	else if (R)
	{
		DoRamble(R);
		NPCPhrase(MyconPhrase::RambleTail);

		DisablePhrase(R);
	}

	if ((uint8_t)uqm::random().next() < 256 * 30 / 100)
	{
		RespFunc = (ResponseFunc)CombatIsInevitable;
	}
	else
	{
		RespFunc = (ResponseFunc)NormalMycon;
	}
	if (!(uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
	{
		if (PhraseEnabled(MyconPhrase::ComeInPeace))
		{
			Response(MyconPhrase::ComeInPeace, RespFunc);
		}
		if (PhraseEnabled(MyconPhrase::GonnaDie))
		{
			Response(MyconPhrase::GonnaDie, RespFunc);
		}
		if (!MadeChoice)
		{
			switch (uqm::gameState().flag(uqm::GameFlag::MYCON_INSULTS))
			{
				case 0:
					Response(MyconPhrase::Insult1, RespFunc);
					break;
				case 1:
					Response(MyconPhrase::Insult2, RespFunc);
					break;
				case 2:
					Response(MyconPhrase::Insult3, RespFunc);
					break;
				case 3:
					Response(MyconPhrase::Insult4, RespFunc);
					break;
				case 4:
					Response(MyconPhrase::Insult5, RespFunc);
					break;
				case 5:
					Response(MyconPhrase::Insult6, RespFunc);
					break;
				case 6:
					Response(MyconPhrase::Insult7, RespFunc);
					break;
				case 7:
					Response(MyconPhrase::Insult8, RespFunc);
					break;
			}
		}
		Response(MyconPhrase::ByeSpace, CombatIsInevitable);
	}
	else
	{
		if (!MadeChoice)
		{
			switch (uqm::gameState().flag(uqm::GameFlag::MYCON_INFO))
			{
				case 0:
					Response(MyconPhrase::Question1, RespFunc);
					break;
				case 1:
					Response(MyconPhrase::Question2, RespFunc);
					break;
				case 2:
					Response(MyconPhrase::Question3, RespFunc);
					break;
				case 3:
					Response(MyconPhrase::Question4, RespFunc);
					break;
				case 4:
					Response(MyconPhrase::Question5, RespFunc);
					break;
				case 5:
					Response(MyconPhrase::Question6, RespFunc);
					break;
				case 6:
					Response(MyconPhrase::Question7, RespFunc);
					break;
				case 7:
					Response(MyconPhrase::Question8, RespFunc);
					break;
				case 8:
					Response(MyconPhrase::Question9, RespFunc);
					break;
				case 9:
					Response(MyconPhrase::Question10, RespFunc);
					break;
				case 10:
					Response(MyconPhrase::Question11, RespFunc);
					break;
				case 11:
					Response(MyconPhrase::Question12, RespFunc);
					break;
				case 12:
					Response(MyconPhrase::Question13, RespFunc);
					break;
				case 13:
					Response(MyconPhrase::Question14, RespFunc);
					break;
				case 14:
					Response(MyconPhrase::Question15, RespFunc);
					break;
				case 15:
					Response(MyconPhrase::Question16, RespFunc);
					break;
			}
		}
		if (PhraseEnabled(MyconPhrase::LetsBeFriends))
		{
			Response(MyconPhrase::LetsBeFriends, RespFunc);
		}
		if (PhraseEnabled(MyconPhrase::CameToHomeworld))
		{
			Response(MyconPhrase::CameToHomeworld, RespFunc);
		}
		if (PhraseEnabled(MyconPhrase::SubmitToUs))
		{
			Response(MyconPhrase::SubmitToUs, RespFunc);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH))
		{
			if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) == 1)
			{
				Response(MyconPhrase::WhatAboutShattered, NormalMycon);
			}
			if (uqm::gameState().flag(uqm::GameFlag::MYCON_AMBUSH))
			{
				Response(MyconPhrase::IHaveACunningPlan, TrickMycon);
			}
		}
		Response(MyconPhrase::ByeHomeworld, CombatIsInevitable);
	}
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(MyconPhrase::DieThief);
				break;
			case 1:
				NPCPhrase(MyconPhrase::DieThiefAgain);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::MYCON_KNOW_AMBUSH))
	{
		NPCPhrase(MyconPhrase::DieLiar);

		setSegue(Segue::Hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_SUN_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(MyconPhrase::HelloSunDeviceWorld1);
				SetHomeworldKnown(*HomeId::Mycon);
				break;
			case 1:
				NPCPhrase(MyconPhrase::HelloSunDeviceWorld2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_SUN_VISITS, static_cast<uint32_t>(NumVisits));

		SunDevice((ResponseRef)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(MyconPhrase::HelloHomeworld1);
				break;
			case 1:
				NPCPhrase(MyconPhrase::HelloHomeworld2);
				break;
			case 2:
				NPCPhrase(MyconPhrase::HelloHomeworld3);
				break;
			case 3:
				NPCPhrase(MyconPhrase::HelloHomeworld4);
				break;
			case 4:
				NPCPhrase(MyconPhrase::HelloHomeworld5);
				break;
			case 5:
				NPCPhrase(MyconPhrase::HelloHomeworld6);
				break;
			case 6:
				NPCPhrase(MyconPhrase::HelloHomeworld7);
				break;
			case 7:
				NPCPhrase(MyconPhrase::HelloHomeworld8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		NormalMycon((ResponseRef)0);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(MyconPhrase::HelloSpace1);
				SetHomeworldKnown(*HomeId::Mycon);
				break;
			case 1:
				NPCPhrase(MyconPhrase::HelloSpace2);
				break;
			case 2:
				NPCPhrase(MyconPhrase::HelloSpace3);
				break;
			case 3:
				NPCPhrase(MyconPhrase::HelloSpace4);
				break;
			case 4:
				NPCPhrase(MyconPhrase::HelloSpace5);
				break;
			case 5:
				NPCPhrase(MyconPhrase::HelloSpace6);
				break;
			case 6:
				NPCPhrase(MyconPhrase::HelloSpace7);
				break;
			case 7:
				NPCPhrase(MyconPhrase::HelloSpace8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_VISITS, static_cast<uint32_t>(NumVisits));

		NormalMycon((ResponseRef)0);
	}
}

static uint16_t uninit_mycon(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_mycon_enc(void)
{
	// nothing defined so far
}

ConversationData* init_mycon_comm(void)
{
	mycon_desc.init_encounter_func = Intro;
	mycon_desc.post_encounter_func = post_mycon_enc;
	mycon_desc.uninit_encounter_func = uninit_mycon;

	uqm::lua().beginComm(nullptr, NullResource);

	mycon_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	mycon_desc.AlienTextBaseline.y = 0;
	mycon_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	MadeChoice = 0;

	if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Hostile);
	}
	else
	{
		setSegue(Segue::Peace);
	}
	ConversationData* retval {&mycon_desc};

	return (retval);
}

} // namespace uqm
