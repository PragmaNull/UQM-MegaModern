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
#include "../yehat/resinst.h"
#include "strings.h"

#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"


namespace uqm
{
static ConversationData yehat_desc =
	{
		Conversation::YehatRebel, /* AlienConv */
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
			RebelMusic,	  /* AlienAltSong */
		},
		RebelConversationPhrases, /* PlayerPhrases */
		15, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				/* right hand-wing tapping keyboard; front guy */
				4, /* StartIndex */
				3, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6) | (1 << 7),
			},
								{
				/* left hand-wing tapping keyboard; front guy */
				7, /* StartIndex */
				3, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6) | (1 << 7),
			},
								{
				10,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4) | (1 << 14),
			},
								{
				13,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),
			},
								{
				16,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2) | (1 << 14),
			},
								{
				21,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3),
			},
								{
				/* right arm-wing rising; front guy */
				26, /* StartIndex */
				2,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 0) | (1 << 1),
			},
								{
				/* left arm-wing rising; front guy */
				28, /* StartIndex */
				2,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 0) | (1 << 1),
			},
								{
				30,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				33,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				36,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				39,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				42,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				45,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				48, /* StartIndex */
				4,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2) | (1 << 4),
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
	if (PlayerSaid(R, YehatRebelPhrase::ByeRebel))
	{
		NPCPhrase(YehatRebelPhrase::GoodbyeRebel);
	}
}

static void Rebels(RESPONSE_REF R);

static void RebelInfo(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, YehatRebelPhrase::GiveInfoRebels))
	{
		NPCPhrase(YehatRebelPhrase::WhatInfo);
	}
	else if (PlayerSaid(R, YehatRebelPhrase::WhatAboutUrquan))
	{
		NPCPhrase(YehatRebelPhrase::AboutUrquan);

		DisablePhrase(YehatRebelPhrase::WhatAboutUrquan);
	}
	else if (PlayerSaid(R, YehatRebelPhrase::WhatAboutRoyalty))
	{
		NPCPhrase(YehatRebelPhrase::AboutRoyalty);

		DisablePhrase(YehatRebelPhrase::WhatAboutRoyalty);
	}
	else if (PlayerSaid(R, YehatRebelPhrase::WhatAboutWar))
	{
		NPCPhrase(YehatRebelPhrase::AboutWar);
		SetHomeworldKnown(*HomeId::Shofixti);

		DisablePhrase(YehatRebelPhrase::WhatAboutWar);
	}
	else if (PlayerSaid(R, YehatRebelPhrase::WhatAboutVux))
	{
		NPCPhrase(YehatRebelPhrase::AboutVux);

		DisablePhrase(YehatRebelPhrase::WhatAboutVux);
	}
	else if (PlayerSaid(R, YehatRebelPhrase::WhatAboutClue))
	{
		NPCPhrase(YehatRebelPhrase::AboutClue);

		DisablePhrase(YehatRebelPhrase::WhatAboutClue);
	}

	if (PhraseEnabled(YehatRebelPhrase::WhatAboutUrquan))
	{
		Response(YehatRebelPhrase::WhatAboutUrquan, RebelInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(YehatRebelPhrase::WhatAboutRoyalty))
	{
		Response(YehatRebelPhrase::WhatAboutRoyalty, RebelInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(YehatRebelPhrase::WhatAboutWar))
	{
		Response(YehatRebelPhrase::WhatAboutWar, RebelInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(YehatRebelPhrase::WhatAboutVux))
	{
		Response(YehatRebelPhrase::WhatAboutVux, RebelInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(YehatRebelPhrase::WhatAboutClue))
	{
		Response(YehatRebelPhrase::WhatAboutClue, RebelInfo);
		InfoLeft = true;
	}
	Response(YehatRebelPhrase::EnoughInfo, Rebels);

	if (!InfoLeft)
	{
		DisablePhrase(YehatRebelPhrase::GiveInfoRebels);
	}
}

static void Rebels(RESPONSE_REF R)
{
	uint16_t NumVisits {};

	if (PlayerSaid(R, YehatRebelPhrase::HowGoesRevolution))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(YehatRebelPhrase::RebelRevolution1);
				break;
			case 1:
				NPCPhrase(YehatRebelPhrase::RebelRevolution2);
				break;
			case 2:
				NPCPhrase(YehatRebelPhrase::RebelRevolution3);
				break;
			case 3:
				NPCPhrase(YehatRebelPhrase::RebelRevolution4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(YehatRebelPhrase::HowGoesRevolution);
	}
	else if (PlayerSaid(R, YehatRebelPhrase::AnyShips))
	{
		if (!ShipsReady(RaceId::Yehat))
		{
			NPCPhrase(YehatRebelPhrase::NoShipsYet);
		}
		else if ((NumVisits = EscortFeasibilityStudy(RaceId::Yehat)) == 0)
		{
			NPCPhrase(YehatRebelPhrase::NoRoom);
		}
		else
		{
			const int numYehatShips {difficultyCase(4, 4, 2)};

			if (NumVisits < numYehatShips)
			{
				NPCPhrase(YehatRebelPhrase::HaveFewShips);
			}
			else
			{
				NumVisits = numYehatShips;
				NPCPhrase(YehatRebelPhrase::HaveAllShips);
			}

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(RaceId::Yehat, NumVisits);
			PrepareShip(RaceId::Yehat);
		}

		DisablePhrase(YehatRebelPhrase::AnyShips);
	}
	else if (PlayerSaid(R, YehatRebelPhrase::WhatAboutPkunkRebel))
	{
		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
		{
			NPCPhrase(YehatRebelPhrase::PkunkAbsorbedRebel);
		}
		else
		{
			NPCPhrase(YehatRebelPhrase::HatePkunkRebel);
		}

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_TOLD_PKUNK, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, YehatRebelPhrase::EnoughInfo))
	{
		NPCPhrase(YehatRebelPhrase::OkEnoughInfo);
	}

	if (PhraseEnabled(YehatRebelPhrase::HowGoesRevolution))
	{
		Response(YehatRebelPhrase::HowGoesRevolution, Rebels);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_TOLD_PKUNK)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS))
	{
		Response(YehatRebelPhrase::WhatAboutPkunkRebel, Rebels);
	}
	if (PhraseEnabled(YehatRebelPhrase::AnyShips))
	{
		Response(YehatRebelPhrase::AnyShips, Rebels);
	}
	if (PhraseEnabled(YehatRebelPhrase::GiveInfoRebels))
	{
		Response(YehatRebelPhrase::GiveInfoRebels, RebelInfo);
	}
	Response(YehatRebelPhrase::ByeRebel, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	setSegue(Segue::Peace);
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
	{
		NPCPhrase(YehatRebelPhrase::YehatCavalry);
		AlienTalkSegue((uint16_t)~0);

		NumVisits = (uint8_t)EscortFeasibilityStudy(RaceId::YehatRebel);
		if (NumVisits > 8)
		{
			NumVisits = difficultyCase(8, 8, 4);
		}
		AddEscortShips(RaceId::YehatRebel, NumVisits - (NumVisits >> 1));
		AddEscortShips(RaceId::Pkunk, NumVisits >> 1);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(YehatRebelPhrase::RebelHello1);
				break;
			case 1:
				NPCPhrase(YehatRebelPhrase::RebelHello2);
				break;
			case 2:
				NPCPhrase(YehatRebelPhrase::RebelHello3);
				break;
			case 3:
				NPCPhrase(YehatRebelPhrase::RebelHello4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_VISITS, static_cast<uint32_t>(NumVisits));

		Rebels((RESPONSE_REF)0);
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

ConversationData* init_rebel_yehat_comm(void)
{
	yehat_desc.init_encounter_func = Intro;
	yehat_desc.post_encounter_func = post_yehat_enc;
	yehat_desc.uninit_encounter_func = uninit_yehat;

	// Initialise Lua for string interpolation. This will be
	// generalised in the future.
	uqm::lua().beginComm(nullptr, NullResource);

	yehat_desc.AlienTextBaseline.x = SisScreenWidth() * 2 / 3;
	yehat_desc.AlienTextBaseline.y = ResScale(107 * 2 / 3);
	yehat_desc.AlienTextWidth = (SisTextWidth() - ResScale(16)) * 2 / 3;

	// use alternate "Rebels" track if available
	altResFlags |= UseAltSong;

	setSegue(Segue::Peace);
	ConversationData* retval {&yehat_desc};

	return (retval);
}

} // namespace uqm
