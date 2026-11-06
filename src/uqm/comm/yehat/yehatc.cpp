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


static ConversationData yehat_desc =
	{
		YEHAT_CONVERSATION, /* AlienConv */
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
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_MIDDLE, /* AlienTextValign */
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
				YoyoAnim
					| WaitTalking, /* AnimFlags */
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
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond / 4,
				GameTicksPerSecond / 2, /* RestartRate */
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
				GameTicksPerSecond * 6,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2) | (1 << 14),
			},
								{
				21,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 6,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3),
			},
								{
				/* right arm-wing rising; front guy */
				26,						/* StartIndex */
				2,						/* NumFrames */
				YoyoAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 6,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 0) | (1 << 1),
			},
								{
				/* left arm-wing rising; front guy */
				28,						/* StartIndex */
				2,						/* NumFrames */
				YoyoAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 6,
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
				48,						/* StartIndex */
				4,						/* NumFrames */
				YoyoAnim | WaitTalking, /* AnimFlags */
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
	setSegue(Segue_hostile);

	if (PlayerSaid(R, bye_homeworld))
	{
		NPCPhrase(GOODBYE_AND_DIE_HOMEWORLD);
	}
	else if (PlayerSaid(R, bye_royalist))
	{
		NPCPhrase(GOODBYE_AND_DIE_ROYALIST);
	}
	else if (PlayerSaid(R, i_demand_you_ally_homeworld))
	{
		NPCPhrase(ENEMY_MUST_DIE);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_ALLY_HOME, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, bye_space))
	{
		if ((uint8_t)uqm::random().next() & 1)
		{
			NPCPhrase(GOODBYE_AND_DIE_SPACE);
		}
		else
		{
			NPCPhrase(GO_IN_PEACE);

			setSegue(Segue_peace);
		}
	}
	else if (PlayerSaid(R, not_here)
			 || PlayerSaid(R, not_send))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_VISITS))
		{
			case 0:
				NPCPhrase(JUST_A_TRICK_1);
				break;
			case 1:
				NPCPhrase(JUST_A_TRICK_2);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_VISITS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ok_send))
	{
		NPCPhrase(WE_REVOLT);

		setSegue(Segue_peace);
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

			state |= 1 << NO_HELP_FROM_PKUNK;

			uqm::gameState().setFlag(uqm::GameFlag::HM_ENCOUNTERS, static_cast<uint32_t>(state));
		}

		AddEvent(RELATIVE_EVENT, 0, 0, 0, YEHAT_REBEL_EVENT);
	}
}

static void Royalists(RESPONSE_REF R)
{
	if (PlayerSaid(R, how_is_rebellion))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ROYALIST_REBELLION_1);
				break;
			case 1:
				NPCPhrase(ROYALIST_REBELLION_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(how_is_rebellion);
	}
	else if (PlayerSaid(R, what_about_pkunk_royalist))
	{
		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
		{
			NPCPhrase(PKUNK_ABSORBED_ROYALIST);
		}
		else
		{
			NPCPhrase(HATE_PKUNK_ROYALIST);
		}

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, sorry_about_revolution))
	{
		NPCPhrase(ALL_YOUR_FAULT);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_INFO, static_cast<uint32_t>(1));
	}

	if (PhraseEnabled(how_is_rebellion))
	{
		Response(how_is_rebellion, Royalists);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS))
	{
		Response(what_about_pkunk_royalist, Royalists);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_INFO))
	{
		Response(sorry_about_revolution, Royalists);
	}
	Response(bye_royalist, ExitConversation);
}

static void StartRevolt(RESPONSE_REF R)
{
	if (PlayerSaid(R, shofixti_alive_1))
	{
		NPCPhrase(SEND_HIM_OVER_1);

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_TOLD_PKUNK, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, shofixti_alive_2))
	{
		NPCPhrase(SEND_HIM_OVER_2);
	}

	if (HaveEscortShip(SHOFIXTI_SHIP))
	{
		Response(ok_send, ExitConversation);
	}
	else
	{
		Response(not_here, ExitConversation);
	}
	Response(not_send, ExitConversation);
}

static void YehatHome(RESPONSE_REF R)
{

	SetHomeworldKnown(YEHAT_HOME);

	if (PlayerSaid(R, whats_up_homeworld))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_HOMEWORLD_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_HOMEWORLD_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_homeworld);
	}
	else if (PlayerSaid(R, at_least_help_us_homeworld))
	{
		NPCPhrase(NO_HELP_ENEMY);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_HELP_HOME, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, give_info))
	{
		NPCPhrase(NO_INFO_FOR_ENEMY);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_INFO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, what_about_pkunk_royalist))
	{
		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
		{
			NPCPhrase(PKUNK_ABSORBED_ROYALIST);
		}
		else
		{
			NPCPhrase(HATE_PKUNK_ROYALIST);
		}

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK, static_cast<uint32_t>(1));
	}

	if (PhraseEnabled(whats_up_homeworld))
	{
		Response(whats_up_homeworld, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS))
	{
		Response(what_about_pkunk_royalist, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_HELP_HOME))
	{
		Response(at_least_help_us_homeworld, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_INFO))
	{
		Response(give_info, YehatHome);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_ALLY_HOME))
	{
		Response(i_demand_you_ally_homeworld, ExitConversation);
	}
	Response(bye_homeworld, ExitConversation);
}

static void YehatSpace(RESPONSE_REF R)
{
	uint8_t i {};
	RESPONSE_REF pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PlayerSaid(R, whats_up_space_1)
		|| PlayerSaid(R, whats_up_space_2)
		|| PlayerSaid(R, whats_up_space_3)
		|| PlayerSaid(R, whats_up_space_4))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_SPACE_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_SPACE_2);
				SetHomeworldKnown(YEHAT_HOME);
				break;
			case 2:
				NPCPhrase(GENERAL_INFO_SPACE_3);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_SPACE_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_REBEL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_space_1);
	}
	else if (PlayerSaid(R, i_demand_you_ally_space))
	{
		NPCPhrase(WE_CANNOT_1);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_ALLY_SPACE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, obligation))
	{
		NPCPhrase(WE_CANNOT_2);

		setSegue(Segue_peace);
		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_ALLY_SPACE, static_cast<uint32_t>(2));

		return;
	}
	else if (PlayerSaid(R, at_least_help_us_space))
	{
		NPCPhrase(SORRY_CANNOT);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_HELP_SPACE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, dishonor))
	{
		NPCPhrase(HERES_A_HINT);

		uqm::gameState().setFlag(uqm::GameFlag::NO_YEHAT_HELP_SPACE, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, what_about_pkunk_royalist))
	{
		if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
		{
			NPCPhrase(PKUNK_ABSORBED_ROYALIST);
		}
		else
		{
			NPCPhrase(HATE_PKUNK_ROYALIST);
		}

		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK, static_cast<uint32_t>(1));
	}

	// SET_FUNCPTR (&PtrDesc, YehatSpace);
	if (PhraseEnabled(whats_up_space_1))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_INFO))
		{
			case 0:
				pStr[0] = whats_up_space_1;
				break;
			case 1:
				pStr[0] = whats_up_space_2;
				break;
			case 2:
				pStr[0] = whats_up_space_3;
				break;
			case 3:
				pStr[0] = whats_up_space_4;
				break;
		}
	}
	switch (uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_HELP_SPACE))
	{
		case 0:
			pStr[1] = at_least_help_us_space;
			break;
		case 1:
			pStr[1] = dishonor;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::NO_YEHAT_ALLY_SPACE))
	{
		case 0:
			{
				pStr[2] = i_demand_you_ally_space;
				break;
			}
		case 1:
			pStr[2] = obligation;
			break;
	}

	if (pStr[LastStack])
	{
		Response(pStr[LastStack], YehatSpace);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			Response(pStr[i], YehatSpace);
		}
	}
	if (!uqm::gameState().flag(uqm::GameFlag::YEHAT_ROYALIST_TOLD_PKUNK)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS)
		&& uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS))
	{
		Response(what_about_pkunk_royalist, YehatSpace);
	}
	if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_VISITS))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::YEHAT_REBEL_TOLD_PKUNK))
		{
			case 0:
				Response(shofixti_alive_1, StartRevolt);
				break;
			case 1:
				Response(shofixti_alive_2, StartRevolt);
				break;
		}
	}
	Response(bye_space, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue_peace);
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
					NPCPhrase(ROYALIST_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase(ROYALIST_HOMEWORLD_HELLO_2);
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
					NPCPhrase(ROYALIST_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(ROYALIST_SPACE_HELLO_2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::YEHAT_VISITS, static_cast<uint32_t>(NumVisits));
		}

		Royalists((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HOMEWORLD_HELLO_1);
				break;
			case 1:
				NPCPhrase(HOMEWORLD_HELLO_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		YehatHome((RESPONSE_REF)0);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::YEHAT_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SPACE_HELLO_1);
				break;
			case 1:
				NPCPhrase(SPACE_HELLO_2);
				break;
			case 2:
				NPCPhrase(SPACE_HELLO_3);
				break;
			case 3:
				NPCPhrase(SPACE_HELLO_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::YEHAT_VISITS, static_cast<uint32_t>(NumVisits));

		YehatSpace((RESPONSE_REF)0);
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

	if (uqm::lowByte(uqm::currentActivity()) != WON_LAST_BATTLE)
	{
		setSegue(Segue_hostile);
	}
	else
	{
		setSegue(Segue_peace);
	}
	ConversationData* retval {&yehat_desc};

	return (retval);
}
