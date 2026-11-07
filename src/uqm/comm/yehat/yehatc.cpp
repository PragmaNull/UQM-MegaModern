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
#include "libs/mathlib.h"

#include "uqm/planets/generate/gendefault.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static ConversationData yehat_desc =
	{
		Conversation::Yehat, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		YehatPmapAnim, /* AlienFrame */
		YehatFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* (SisTextWidth() - 16) * 2 / 3, */	 /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Middle, /* AlienTextValign */
		YehatColorMap, /* AlienColorMap */
		YehatMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		YehatConversationPhrases, /* PlayerPhrases */
		15, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				/* right hand-wing tapping keyboard; front guy */
				4, /* StartIndex */
				3, /* NumFrames */
				AnimationFlags::Yoyo
					| AnimationFlags::WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond / 4,
				GameTicksPerSecond / 2, /* RestartRate */
				(1 << 6) | (1 << 7),
			},
								{
				/* left hand-wing tapping keyboard; front guy */
				7, /* StartIndex */
				3, /* NumFrames */
				AnimationFlags::Yoyo
					| AnimationFlags::WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond / 4,
				GameTicksPerSecond / 2, /* RestartRate */
				(1 << 6) | (1 << 7),
			},
								{
				10,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4) | (1 << 14),
			},
								{
				13,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),
			},
								{
				16,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 6,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2) | (1 << 14),
			},
								{
				21,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 6,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3),
			},
								{
				/* right arm-wing rising; front guy */
				26,													/* StartIndex */
				2,													/* NumFrames */
				AnimationFlags::Yoyo | AnimationFlags::WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 6,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 0) | (1 << 1),
			},
								{
				/* left arm-wing rising; front guy */
				28,													/* StartIndex */
				2,													/* NumFrames */
				AnimationFlags::Yoyo | AnimationFlags::WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 6,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 0) | (1 << 1),
			},
								{
				30,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				33,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				36,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				39,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				42,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				45,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				48,													/* StartIndex */
				4,													/* NumFrames */
				AnimationFlags::Yoyo | AnimationFlags::WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2) | (1 << 4),
			},
								},
		{
								/* AlienTransitionDesc - empty */
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

static void ExitConversation(ResponseRef R)
{
	setSegue(Segue::Hostile);

	if (PlayerSaid(R, YehatPhrase::ByeHomeworld))
	{
		NPCPhrase(YehatPhrase::GoodbyeAndDieHomeworld);
	}
	else if (PlayerSaid(R, YehatPhrase::ByeRoyalist))
	{
		NPCPhrase(YehatPhrase::GoodbyeAndDieRoyalist);
	}
	else if (PlayerSaid(R, YehatPhrase::IDemandYouAllyHomeworld))
	{
		NPCPhrase(YehatPhrase::EnemyMustDie);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_ALLY_HOME, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::ByeSpace))
	{
		if ((uint8_t)uqm::random().next() & 1)
		{
			NPCPhrase(YehatPhrase::GoodbyeAndDieSpace);
		}
		else
		{
			NPCPhrase(YehatPhrase::GoInPeace);

			setSegue(Segue::Peace);
		}
	}
	else if (PlayerSaid(R, YehatPhrase::NotHere)
			 || PlayerSaid(R, YehatPhrase::NotSend))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_VISITS))
		{
			case 0:
				NPCPhrase(YehatPhrase::JustATrick1);
				break;
			case 1:
				NPCPhrase(YehatPhrase::JustATrick2);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_VISITS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::OkSend))
	{
		NPCPhrase(YehatPhrase::WeRevolt);

		setSegue(Segue::Peace);
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_CIVIL_WAR, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_TOLD_PKUNK, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_INFO, static_cast<uint32_t>(0));

		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK) && isDifficulty(uqm::Difficulty::Hard))
		{ // absorbed before revolution
			uint16_t state {};

			state = uqm::gameState().flag(uqm::GameFlag::HM_ENCOUNTERS);

			state |= 1 << *HardModeEncounter::NoHelpFromPkunk;

			uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));
		}

		AddEvent(EventType::Relative, 0, 0, 0, GameEventId::YehatRebelEvent);
	}
}

static void Royalists(ResponseRef R)
{
	if (PlayerSaid(R, YehatPhrase::HowIsRebellion))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(YehatPhrase::RoyalistRebellion1);
				break;
			case 1:
				NPCPhrase(YehatPhrase::RoyalistRebellion2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(YehatPhrase::HowIsRebellion);
	}
	else if (PlayerSaid(R, YehatPhrase::WhatAboutPkunkRoyalist))
	{
		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
		{
			NPCPhrase(YehatPhrase::PkunkAbsorbedRoyalist);
		}
		else
		{
			NPCPhrase(YehatPhrase::HatePkunkRoyalist);
		}

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::SorryAboutRevolution))
	{
		NPCPhrase(YehatPhrase::AllYourFault);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_INFO, static_cast<uint32_t>(1));
	}

	if (PhraseEnabled(YehatPhrase::HowIsRebellion))
	{
		Response(YehatPhrase::HowIsRebellion, Royalists);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS))
	{
		Response(YehatPhrase::WhatAboutPkunkRoyalist, Royalists);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_INFO))
	{
		Response(YehatPhrase::SorryAboutRevolution, Royalists);
	}
	Response(YehatPhrase::ByeRoyalist, ExitConversation);
}

static void StartRevolt(ResponseRef R)
{
	if (PlayerSaid(R, YehatPhrase::ShofixtiAlive1))
	{
		NPCPhrase(YehatPhrase::SendHimOver1);

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_TOLD_PKUNK, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::ShofixtiAlive2))
	{
		NPCPhrase(YehatPhrase::SendHimOver2);
	}

	if (HaveEscortShip(RaceId::Shofixti))
	{
		Response(YehatPhrase::OkSend, ExitConversation);
	}
	else
	{
		Response(YehatPhrase::NotHere, ExitConversation);
	}
	Response(YehatPhrase::NotSend, ExitConversation);
}

static void YehatHome(ResponseRef R)
{

	SetHomeworldKnown(*HomeId::Yehat);

	if (PlayerSaid(R, YehatPhrase::WhatsUpHomeworld))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(YehatPhrase::GeneralInfoHomeworld1);
				break;
			case 1:
				NPCPhrase(YehatPhrase::GeneralInfoHomeworld2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(YehatPhrase::WhatsUpHomeworld);
	}
	else if (PlayerSaid(R, YehatPhrase::AtLeastHelpUsHomeworld))
	{
		NPCPhrase(YehatPhrase::NoHelpEnemy);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_HELP_HOME, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::GiveInfo))
	{
		NPCPhrase(YehatPhrase::NoInfoForEnemy);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_INFO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::WhatAboutPkunkRoyalist))
	{
		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
		{
			NPCPhrase(YehatPhrase::PkunkAbsorbedRoyalist);
		}
		else
		{
			NPCPhrase(YehatPhrase::HatePkunkRoyalist);
		}

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK, static_cast<uint32_t>(1));
	}

	if (PhraseEnabled(YehatPhrase::WhatsUpHomeworld))
	{
		Response(YehatPhrase::WhatsUpHomeworld, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS))
	{
		Response(YehatPhrase::WhatAboutPkunkRoyalist, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_HELP_HOME))
	{
		Response(YehatPhrase::AtLeastHelpUsHomeworld, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_INFO))
	{
		Response(YehatPhrase::GiveInfo, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_ALLY_HOME))
	{
		Response(YehatPhrase::IDemandYouAllyHomeworld, ExitConversation);
	}
	Response(YehatPhrase::ByeHomeworld, ExitConversation);
}

static void YehatSpace(ResponseRef R)
{
	uint8_t i {};
	YehatPhrase pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = YehatPhrase::NullPhrase;
	if (PlayerSaid(R, YehatPhrase::WhatsUpSpace1)
		|| PlayerSaid(R, YehatPhrase::WhatsUpSpace2)
		|| PlayerSaid(R, YehatPhrase::WhatsUpSpace3)
		|| PlayerSaid(R, YehatPhrase::WhatsUpSpace4))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(YehatPhrase::GeneralInfoSpace1);
				break;
			case 1:
				NPCPhrase(YehatPhrase::GeneralInfoSpace2);
				SetHomeworldKnown(*HomeId::Yehat);
				break;
			case 2:
				NPCPhrase(YehatPhrase::GeneralInfoSpace3);
				break;
			case 3:
				NPCPhrase(YehatPhrase::GeneralInfoSpace4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(YehatPhrase::WhatsUpSpace1);
	}
	else if (PlayerSaid(R, YehatPhrase::IDemandYouAllySpace))
	{
		NPCPhrase(YehatPhrase::WeCannot1);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_ALLY_SPACE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::Obligation))
	{
		NPCPhrase(YehatPhrase::WeCannot2);

		setSegue(Segue::Peace);
		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_ALLY_SPACE, static_cast<uint32_t>(2));

		return;
	}
	else if (PlayerSaid(R, YehatPhrase::AtLeastHelpUsSpace))
	{
		NPCPhrase(YehatPhrase::SorryCannot);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_HELP_SPACE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatPhrase::Dishonor))
	{
		NPCPhrase(YehatPhrase::HeresAHint);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_HELP_SPACE, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, YehatPhrase::WhatAboutPkunkRoyalist))
	{
		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
		{
			NPCPhrase(YehatPhrase::PkunkAbsorbedRoyalist);
		}
		else
		{
			NPCPhrase(YehatPhrase::HatePkunkRoyalist);
		}

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK, static_cast<uint32_t>(1));
	}

	// SET_FUNCPTR (&PtrDesc, YehatSpace);
	if (PhraseEnabled(YehatPhrase::WhatsUpSpace1))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_INFO))
		{
			case 0:
				pStr[0] = YehatPhrase::WhatsUpSpace1;
				break;
			case 1:
				pStr[0] = YehatPhrase::WhatsUpSpace2;
				break;
			case 2:
				pStr[0] = YehatPhrase::WhatsUpSpace3;
				break;
			case 3:
				pStr[0] = YehatPhrase::WhatsUpSpace4;
				break;
		}
	}
	switch (uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_HELP_SPACE))
	{
		case 0:
			pStr[1] = YehatPhrase::AtLeastHelpUsSpace;
			break;
		case 1:
			pStr[1] = YehatPhrase::Dishonor;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_ALLY_SPACE))
	{
		case 0:
			{
				pStr[2] = YehatPhrase::IDemandYouAllySpace;
				break;
			}
		case 1:
			pStr[2] = YehatPhrase::Obligation;
			break;
	}

	if (pStr[LastStack] != YehatPhrase::NullPhrase)
	{
		Response(pStr[LastStack], YehatSpace);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && *pStr[i] != YehatPhrase::NullPhrase)
		{
			Response(pStr[i], YehatSpace);
		}
	}
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS))
	{
		Response(YehatPhrase::WhatAboutPkunkRoyalist, YehatSpace);
	}
	if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_VISITS))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_TOLD_PKUNK))
		{
			case 0:
				Response(YehatPhrase::ShofixtiAlive1, StartRevolt);
				break;
			case 1:
				Response(YehatPhrase::ShofixtiAlive2, StartRevolt);
				break;
		}
	}
	Response(YehatPhrase::ByeSpace, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(YehatPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::YEHAT_CIVIL_WAR))
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(YehatPhrase::RoyalistHomeworldHello1);
					break;
				case 1:
					NPCPhrase(YehatPhrase::RoyalistHomeworldHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::YEHAT_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(YehatPhrase::RoyalistSpaceHello1);
					break;
				case 1:
					NPCPhrase(YehatPhrase::RoyalistSpaceHello2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::YEHAT_VISITS, static_cast<uint32_t>(NumVisits));
		}

		Royalists((ResponseRef)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(YehatPhrase::HomeworldHello1);
				break;
			case 1:
				NPCPhrase(YehatPhrase::HomeworldHello2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		YehatHome((ResponseRef)0);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(YehatPhrase::SpaceHello1);
				break;
			case 1:
				NPCPhrase(YehatPhrase::SpaceHello2);
				break;
			case 2:
				NPCPhrase(YehatPhrase::SpaceHello3);
				break;
			case 3:
				NPCPhrase(YehatPhrase::SpaceHello4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_VISITS, static_cast<uint32_t>(NumVisits));

		YehatSpace((ResponseRef)0);
	}
}

static uint16_t uninit_yehat(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_yehat_enc(void)
{
	// nothing defined so far
}

ConversationData* init_yehat_comm(void)
{
	yehat_desc.init_encounter_func = Intro;
	yehat_desc.post_encounter_func = post_yehat_enc;
	yehat_desc.uninit_encounter_func = uninit_yehat;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	yehat_desc.AlienTextBaseline.x = SisScreenWidth() * 2 / 3;
	yehat_desc.AlienTextBaseline.y = ResScale(107 * 2 / 3);
	yehat_desc.AlienTextWidth = (SisTextWidth() - ResScale(16)) * 2 / 3;

	if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Hostile);
	}
	else
	{
		setSegue(Segue::Peace);
	}
	ConversationData* retval {&yehat_desc};

	return (retval);
}

} // namespace uqm
