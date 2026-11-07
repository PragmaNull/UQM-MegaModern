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
#include "uqm/gameev.h"
#include "uqm/races.h"


namespace uqm
{
static ConversationData shofixti_desc =
	{
		Conversation::Shofixti, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		ShofixtiPmapAnim, /* AlienFrame */
		ShofixtiFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth(), */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		ShofixtiColorMap, /* AlienColorMap */
		ShofixtiMusic, /* AlienSong */
		{
								ShofixtiAltPmapAnim, /* AlienAltFrame */
			NullResource,		 /* AlienAltColorMap */
			NullResource,		 /* AlienAltSong */
		},
		ShofixtiConversationPhrases, /* PlayerPhrases */
		11, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				5,			  /* StartIndex */
				15,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond / 30,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				20,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				(GameTicksPerSecond >> 1),
				(GameTicksPerSecond >> 1) * 3, /* RestartRate */
				0,							   /* BlockMask */
			},
								{
				23,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				(GameTicksPerSecond >> 1),
				(GameTicksPerSecond >> 1) * 3, /* RestartRate */
				0,							   /* BlockMask */
			},
								{
				26,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				29,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},

								{
				33,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				39,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 7),				 /* BlockMask */
			},
								{
				46,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 6),				 /* BlockMask */
			},
								{
				52,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				56,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 10),				 /* BlockMask */
			},
								{
				63,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 20,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 9),				 /* BlockMask */
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
			4, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 20,
								0, /* FrameRate */
			GameTicksPerSecond / 15,
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

static ShofixtiPhrase shofixti_name;

static void GetShofixtiName(void)
{
	if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_KIA))
	{
		shofixti_name = ShofixtiPhrase::Katana;
	}
	else
	{
		shofixti_name = ShofixtiPhrase::Tanaka;
	}
}

static void ExitConversation(ResponseRef R)
{
	setSegue(Segue::Hostile);

	if (PlayerSaid(R, ShofixtiPhrase::Bye0))
	{
		NPCPhrase(ShofixtiPhrase::Goodbye);

		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, ShofixtiPhrase::GoAhead))
	{
		NPCPhrase(ShofixtiPhrase::OnSecondThought);

		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, ShofixtiPhrase::NeedYouForDuty))
	{
		NPCPhrase(ShofixtiPhrase::OkWillBeSentry);

		SetHomeworldKnown(*HomeId::Shofixti);

		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, ShofixtiPhrase::Females)
			 || PlayerSaid(R, ShofixtiPhrase::Nubiles)
			 || PlayerSaid(R, ShofixtiPhrase::RatBabes))
	{
		NPCPhrase(ShofixtiPhrase::LeapingHappiness);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_RECRUITED, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::MAIDENS_ON_SHIP, static_cast<uint32_t>(0));
		setSegue(Segue::Peace);

		AddEvent(EventType::Relative, 2, 0, 0, GameEventId::ShofixtiReturnEvent);
	}
	else if (PlayerSaid(R, ShofixtiPhrase::DontAttack))
	{
		NPCPhrase(ShofixtiPhrase::TypicalPloy);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::HeyStop))
	{
		NPCPhrase(ShofixtiPhrase::OnlyStop);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::LookYouAre))
	{
		NPCPhrase(ShofixtiPhrase::TooBad);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::NoOneInsults))
	{
		NPCPhrase(ShofixtiPhrase::YouLimp);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::MightyWords))
	{
		NPCPhrase(ShofixtiPhrase::HangYour);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK2, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::DontKnow))
	{
		NPCPhrase(ShofixtiPhrase::Never);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::Look0))
	{
		NPCPhrase(ShofixtiPhrase::ForYou);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::NoBloodshed))
	{
		NPCPhrase(ShofixtiPhrase::YesBloodshed);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::DontWantToFight))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK4);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ShofixtiPhrase::MustFightYouUrquan1);
				break;
			case 1:
				NPCPhrase(ShofixtiPhrase::MustFightYouUrquan2);
				break;
			case 2:
				NPCPhrase(ShofixtiPhrase::MustFightYouUrquan3);
				break;
			case 3:
				NPCPhrase(ShofixtiPhrase::MustFightYouUrquan4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK4, static_cast<uint32_t>(NumVisits));
	}
}

static void GiveMaidens(ResponseRef R)
{
	if (PlayerSaid(R, ShofixtiPhrase::ImportantDuty))
	{
		NPCPhrase(ShofixtiPhrase::WhatDuty);

		Response(ShofixtiPhrase::ProcreatingWildly, GiveMaidens);
		Response(ShofixtiPhrase::ReplenishingYourSpecies, GiveMaidens);
		Response(ShofixtiPhrase::HopeYouHave, GiveMaidens);
	}
	else
	{
		NPCPhrase(ShofixtiPhrase::SoundsGreatButHow);

		Response(ShofixtiPhrase::Females, ExitConversation);
		Response(ShofixtiPhrase::Nubiles, ExitConversation);
		Response(ShofixtiPhrase::RatBabes, ExitConversation);
	}
}

static void ConsoleShofixti(ResponseRef R)
{
	if (PlayerSaid(R, ShofixtiPhrase::DontDoIt))
	{
		NPCPhrase(ShofixtiPhrase::YesIDoIt);
		DisablePhrase(ShofixtiPhrase::DontDoIt);
	}
	else
	{
		NPCPhrase(ShofixtiPhrase::VerySadKillSelf);
	}

	if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP))
	{
		Response(ShofixtiPhrase::ImportantDuty, GiveMaidens);
	}
	if (PhraseEnabled(ShofixtiPhrase::DontDoIt))
	{
		Response(ShofixtiPhrase::DontDoIt, ConsoleShofixti);
	}
	Response(ShofixtiPhrase::NeedYouForDuty, ExitConversation);
	Response(ShofixtiPhrase::GoAhead, ExitConversation);
}

static void ExplainDefeat(ResponseRef R)
{
	if (PlayerSaid(R, ShofixtiPhrase::IAmNice))
	{
		NPCPhrase(ShofixtiPhrase::MustUnderstand);
	}
	else if (PlayerSaid(R, ShofixtiPhrase::IAmGuy))
	{
		NPCPhrase(ShofixtiPhrase::NiceButWhatIsDonkey);
	}
	else /* if (PlayerSaid (R, ShofixtiPhrase::IAmCaptain)) */
	{
		NPCPhrase(ShofixtiPhrase::SoSorry);
	}
	NPCPhrase(ShofixtiPhrase::IsDefeatTrue);

	Response(ShofixtiPhrase::YesAndNo, ConsoleShofixti);
	Response(ShofixtiPhrase::Clobbered, ConsoleShofixti);
	Response(ShofixtiPhrase::ButtBlasted, ConsoleShofixti);
}

static void RealizeMistake(ResponseRef R)
{
	(void)R; // ignored
	NPCPhrase(ShofixtiPhrase::Dgrunti);
	uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(0));
	uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(0));
	uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK2, static_cast<uint32_t>(3));

	Response(ShofixtiPhrase::IAmCaptain, ExplainDefeat);
	Response(ShofixtiPhrase::IAmNice, ExplainDefeat);
	Response(ShofixtiPhrase::IAmGuy, ExplainDefeat);
}

static void Hostile(ResponseRef R)
{
	(void)R; // ignored
	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK1))
	{
		case 0:
			Response(ShofixtiPhrase::DontAttack, ExitConversation);
			break;
		case 1:
			Response(ShofixtiPhrase::HeyStop, ExitConversation);
			break;
		case 2:
			Response(ShofixtiPhrase::LookYouAre, ExitConversation);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK2))
	{
		case 0:
			Response(ShofixtiPhrase::NoOneInsults, (getNomadMode() == uqm::NomadMode::Normal ? RealizeMistake : ExitConversation));
			break;
		case 1:
			Response(ShofixtiPhrase::MightyWords, ExitConversation);
			break;
		case 2:
			Response(ShofixtiPhrase::DonkeyBreath, RealizeMistake);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK3))
	{
		case 0:
			Response(ShofixtiPhrase::DontKnow, ExitConversation);
			break;
		case 1:
			{
				construct_response(
					shared_phrase_buf,
					ShofixtiPhrase::Look0,
					"",
					shofixti_name,
					"",
					ShofixtiPhrase::Look1,
					(char*)nullptr);
				DoResponsePhrase(*ShofixtiPhrase::Look0, ExitConversation, shared_phrase_buf);
				break;
			}
		case 2:
			Response(ShofixtiPhrase::LookYouAre, ExitConversation);
			break;
	}
	Response(ShofixtiPhrase::DontWantToFight, ExitConversation);
}

static void Friendly(ResponseRef R)
{
	uint8_t i {};
	struct
	{
		ShofixtiPhrase pStr;
		char* c_buf;
	} Resp[3];
	static char buf0[80];
	static char buf1[80];

	uint8_t LastStack {0};
	memset(Resp, 0, sizeof(Resp));
	if (PlayerSaid(R, ShofixtiPhrase::Report0))
	{
		NPCPhrase(ShofixtiPhrase::NothingNew);

		DisablePhrase(ShofixtiPhrase::Report0);
	}
	else if (PlayerSaid(R, ShofixtiPhrase::WhyHere0))
	{
		NPCPhrase(ShofixtiPhrase::IGuard);

		SetHomeworldKnown(*HomeId::Shofixti);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::WhatHappened))
	{
		NPCPhrase(ShofixtiPhrase::MetVux);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::GloryDevice))
	{
		NPCPhrase(ShofixtiPhrase::SwitchBroke);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::WhereWorld))
	{
		NPCPhrase(ShofixtiPhrase::BlewItUp);

		SetHomeworldKnown(*HomeId::Shofixti);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ShofixtiPhrase::HowSurvive))
	{
		NPCPhrase(ShofixtiPhrase::NotHere);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(2));
	}

	if (PhraseEnabled(ShofixtiPhrase::Report0))
	{
		construct_response(
			buf0,
			ShofixtiPhrase::Report0,
			"",
			shofixti_name,
			"",
			ShofixtiPhrase::Report1,
			(char*)nullptr);
		Resp[0].pStr = ShofixtiPhrase::Report0;
		Resp[0].c_buf = buf0;
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK1))
	{
		case 0:
			construct_response(
				buf1,
				ShofixtiPhrase::WhyHere0,
				"",
				shofixti_name,
				"",
				ShofixtiPhrase::WhyHere1,
				(char*)nullptr);
			Resp[1].pStr = ShofixtiPhrase::WhyHere0;
			Resp[1].c_buf = buf1;
			break;
		case 1:
			Resp[1].pStr = ShofixtiPhrase::WhatHappened;
			break;
		case 2:
			Resp[1].pStr = ShofixtiPhrase::GloryDevice;
			break;
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK3))
	{
		case 0:
			Resp[2].pStr = ShofixtiPhrase::WhereWorld;
			break;
		case 1:
			Resp[2].pStr = ShofixtiPhrase::HowSurvive;
			break;
	}

	if (Resp[LastStack].pStr != ShofixtiPhrase::NullPhrase)
	{
		DoResponsePhrase(*Resp[LastStack].pStr, Friendly, Resp[LastStack].c_buf);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && *Resp[i].pStr != ShofixtiPhrase::NullPhrase)
		{
			DoResponsePhrase(*Resp[i].pStr, Friendly, Resp[i].c_buf);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP))
	{
		Response(ShofixtiPhrase::ImportantDuty, GiveMaidens);
	}

	construct_response(
		shared_phrase_buf,
		ShofixtiPhrase::Bye0,
		"",
		shofixti_name,
		"",
		ShofixtiPhrase::Bye1,
		(char*)nullptr);
	DoResponsePhrase(*ShofixtiPhrase::Bye0, ExitConversation, shared_phrase_buf);
}

static void Intro(void)
{
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(ShofixtiPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	GetShofixtiName();

	if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK2) > 2)
	{
		NPCPhrase(ShofixtiPhrase::FriendlyHello);

		Friendly((ResponseRef)0);
	}
	else
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_VISITS);
		if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_KIA))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ShofixtiPhrase::HostileKatana1);
					break;
				case 1:
					NPCPhrase(ShofixtiPhrase::HostileKatana2);
					break;
				case 2:
					NPCPhrase(ShofixtiPhrase::HostileKatana3);
					break;
				case 3:
					NPCPhrase(ShofixtiPhrase::HostileKatana4);
					--NumVisits;
					break;
			}
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ShofixtiPhrase::HostileTanaka1);
					break;
				case 1:
					NPCPhrase(ShofixtiPhrase::HostileTanaka2);
					break;
				case 2:
					NPCPhrase(ShofixtiPhrase::HostileTanaka3);
					break;
				case 3:
					NPCPhrase(ShofixtiPhrase::HostileTanaka4);
					break;
				case 4:
					NPCPhrase(ShofixtiPhrase::HostileTanaka5);
					break;
				case 5:
					NPCPhrase(ShofixtiPhrase::HostileTanaka6);
					break;
				case 6:
					NPCPhrase(ShofixtiPhrase::HostileTanaka7);
					break;
				case 7:
					NPCPhrase(ShofixtiPhrase::HostileTanaka8);
					--NumVisits;
					break;
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_VISITS, static_cast<uint32_t>(NumVisits));

		Hostile((ResponseRef)0);
	}
}

static uint16_t uninit_shofixti(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_shofixti_enc(void)
{
	// nothing defined so far
}

ConversationData* init_shofixti_comm(void)
{
	shofixti_desc.init_encounter_func = Intro;
	shofixti_desc.post_encounter_func = post_shofixti_enc;
	shofixti_desc.uninit_encounter_func = uninit_shofixti;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.
	if (IsExtended())
	{
		altResFlags |= UseAltFrame;
	}

	shofixti_desc.AlienTextBaseline.x =
		TextXOffs() + (SisTextWidth() >> 1);
	shofixti_desc.AlienTextBaseline.y = 0;
	shofixti_desc.AlienTextWidth = SisTextWidth();

	setSegue(Segue::Peace);

	ConversationData* retval {&shofixti_desc};

	return (retval);
}

} // namespace uqm
