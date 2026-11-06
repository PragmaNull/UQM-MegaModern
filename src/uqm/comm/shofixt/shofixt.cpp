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


static ConversationData shofixti_desc =
	{
		SHOFIXTI_CONVERSATION, /* AlienConv */
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

static RESPONSE_REF shofixti_name;

static void GetShofixtiName(void)
{
	if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_KIA))
	{
		shofixti_name = katana;
	}
	else
	{
		shofixti_name = tanaka;
	}
}

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue_hostile);

	if (PlayerSaid(R, bye0))
	{
		NPCPhrase(GOODBYE);

		setSegue(Segue_peace);
	}
	else if (PlayerSaid(R, go_ahead))
	{
		NPCPhrase(ON_SECOND_THOUGHT);

		setSegue(Segue_peace);
	}
	else if (PlayerSaid(R, need_you_for_duty))
	{
		NPCPhrase(OK_WILL_BE_SENTRY);

		SetHomeworldKnown(SHOFIXTI_HOME);

		setSegue(Segue_peace);
	}
	else if (PlayerSaid(R, females)
			 || PlayerSaid(R, nubiles)
			 || PlayerSaid(R, rat_babes))
	{
		NPCPhrase(LEAPING_HAPPINESS);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_RECRUITED, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::MAIDENS_ON_SHIP, static_cast<uint32_t>(0));
		setSegue(Segue_peace);

		AddEvent(RELATIVE_EVENT, 2, 0, 0, SHOFIXTI_RETURN_EVENT);
	}
	else if (PlayerSaid(R, dont_attack))
	{
		NPCPhrase(TYPICAL_PLOY);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, hey_stop))
	{
		NPCPhrase(ONLY_STOP);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, look_you_are))
	{
		NPCPhrase(TOO_BAD);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, no_one_insults))
	{
		NPCPhrase(YOU_LIMP);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, mighty_words))
	{
		NPCPhrase(HANG_YOUR);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK2, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, dont_know))
	{
		NPCPhrase(NEVER);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, look0))
	{
		NPCPhrase(FOR_YOU);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, no_bloodshed))
	{
		NPCPhrase(YES_BLOODSHED);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, dont_want_to_fight))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK4);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(MUST_FIGHT_YOU_URQUAN_1);
				break;
			case 1:
				NPCPhrase(MUST_FIGHT_YOU_URQUAN_2);
				break;
			case 2:
				NPCPhrase(MUST_FIGHT_YOU_URQUAN_3);
				break;
			case 3:
				NPCPhrase(MUST_FIGHT_YOU_URQUAN_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK4, static_cast<uint32_t>(NumVisits));
	}
}

static void GiveMaidens(RESPONSE_REF R)
{
	if (PlayerSaid(R, important_duty))
	{
		NPCPhrase(WHAT_DUTY);

		Response(procreating_wildly, GiveMaidens);
		Response(replenishing_your_species, GiveMaidens);
		Response(hope_you_have, GiveMaidens);
	}
	else
	{
		NPCPhrase(SOUNDS_GREAT_BUT_HOW);

		Response(females, ExitConversation);
		Response(nubiles, ExitConversation);
		Response(rat_babes, ExitConversation);
	}
}

static void ConsoleShofixti(RESPONSE_REF R)
{
	if (PlayerSaid(R, dont_do_it))
	{
		NPCPhrase(YES_I_DO_IT);
		DisablePhrase(dont_do_it);
	}
	else
	{
		NPCPhrase(VERY_SAD_KILL_SELF);
	}

	if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP))
	{
		Response(important_duty, GiveMaidens);
	}
	if (PhraseEnabled(dont_do_it))
	{
		Response(dont_do_it, ConsoleShofixti);
	}
	Response(need_you_for_duty, ExitConversation);
	Response(go_ahead, ExitConversation);
}

static void ExplainDefeat(RESPONSE_REF R)
{
	if (PlayerSaid(R, i_am_nice))
	{
		NPCPhrase(MUST_UNDERSTAND);
	}
	else if (PlayerSaid(R, i_am_guy))
	{
		NPCPhrase(NICE_BUT_WHAT_IS_DONKEY);
	}
	else /* if (PlayerSaid (R, i_am_captain)) */
	{
		NPCPhrase(SO_SORRY);
	}
	NPCPhrase(IS_DEFEAT_TRUE);

	Response(yes_and_no, ConsoleShofixti);
	Response(clobbered, ConsoleShofixti);
	Response(butt_blasted, ConsoleShofixti);
}

static void RealizeMistake(RESPONSE_REF R)
{
	(void)R; // ignored
	NPCPhrase(DGRUNTI);
	uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(0));
	uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(0));
	uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK2, static_cast<uint32_t>(3));

	Response(i_am_captain, ExplainDefeat);
	Response(i_am_nice, ExplainDefeat);
	Response(i_am_guy, ExplainDefeat);
}

static void Hostile(RESPONSE_REF R)
{
	(void)R; // ignored
	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK1))
	{
		case 0:
			Response(dont_attack, ExitConversation);
			break;
		case 1:
			Response(hey_stop, ExitConversation);
			break;
		case 2:
			Response(look_you_are, ExitConversation);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK2))
	{
		case 0:
			Response(no_one_insults, (getNomadMode() == uqm::NomadMode::Normal ? RealizeMistake : ExitConversation));
			break;
		case 1:
			Response(mighty_words, ExitConversation);
			break;
		case 2:
			Response(donkey_breath, RealizeMistake);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK3))
	{
		case 0:
			Response(dont_know, ExitConversation);
			break;
		case 1:
			{
				construct_response(
					shared_phrase_buf,
					look0,
					"",
					shofixti_name,
					"",
					look1,
					(char*)nullptr);
				DoResponsePhrase(look0, ExitConversation, shared_phrase_buf);
				break;
			}
		case 2:
			Response(look_you_are, ExitConversation);
			break;
	}
	Response(dont_want_to_fight, ExitConversation);
}

static void Friendly(RESPONSE_REF R)
{
	uint8_t i {};
	struct
	{
		RESPONSE_REF pStr;
		char* c_buf;
	} Resp[3];
	static char buf0[80];
	static char buf1[80];

	uint8_t LastStack {0};
	memset(Resp, 0, sizeof(Resp));
	if (PlayerSaid(R, report0))
	{
		NPCPhrase(NOTHING_NEW);

		DisablePhrase(report0);
	}
	else if (PlayerSaid(R, why_here0))
	{
		NPCPhrase(I_GUARD);

		SetHomeworldKnown(SHOFIXTI_HOME);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, what_happened))
	{
		NPCPhrase(MET_VUX);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, glory_device))
	{
		NPCPhrase(SWITCH_BROKE);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, where_world))
	{
		NPCPhrase(BLEW_IT_UP);

		SetHomeworldKnown(SHOFIXTI_HOME);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, how_survive))
	{
		NPCPhrase(NOT_HERE);

		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_STACK3, static_cast<uint32_t>(2));
	}

	if (PhraseEnabled(report0))
	{
		construct_response(
			buf0,
			report0,
			"",
			shofixti_name,
			"",
			report1,
			(char*)nullptr);
		Resp[0].pStr = report0;
		Resp[0].c_buf = buf0;
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK1))
	{
		case 0:
			construct_response(
				buf1,
				why_here0,
				"",
				shofixti_name,
				"",
				why_here1,
				(char*)nullptr);
			Resp[1].pStr = why_here0;
			Resp[1].c_buf = buf1;
			break;
		case 1:
			Resp[1].pStr = what_happened;
			break;
		case 2:
			Resp[1].pStr = glory_device;
			break;
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK3))
	{
		case 0:
			Resp[2].pStr = where_world;
			break;
		case 1:
			Resp[2].pStr = how_survive;
			break;
	}

	if (Resp[LastStack].pStr)
	{
		DoResponsePhrase(Resp[LastStack].pStr, Friendly, Resp[LastStack].c_buf);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && Resp[i].pStr)
		{
			DoResponsePhrase(Resp[i].pStr, Friendly, Resp[i].c_buf);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP))
	{
		Response(important_duty, GiveMaidens);
	}

	construct_response(
		shared_phrase_buf,
		bye0,
		"",
		shofixti_name,
		"",
		bye1,
		(char*)nullptr);
	DoResponsePhrase(bye0, ExitConversation, shared_phrase_buf);
}

static void Intro(void)
{
	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue_peace);
		return;
	}

	GetShofixtiName();

	if (uqm::gameState().flag(uqm::GameFlag::SHOFIXTI_STACK2) > 2)
	{
		NPCPhrase(FRIENDLY_HELLO);

		Friendly((RESPONSE_REF)0);
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
					NPCPhrase(HOSTILE_KATANA_1);
					break;
				case 1:
					NPCPhrase(HOSTILE_KATANA_2);
					break;
				case 2:
					NPCPhrase(HOSTILE_KATANA_3);
					break;
				case 3:
					NPCPhrase(HOSTILE_KATANA_4);
					--NumVisits;
					break;
			}
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(HOSTILE_TANAKA_1);
					break;
				case 1:
					NPCPhrase(HOSTILE_TANAKA_2);
					break;
				case 2:
					NPCPhrase(HOSTILE_TANAKA_3);
					break;
				case 3:
					NPCPhrase(HOSTILE_TANAKA_4);
					break;
				case 4:
					NPCPhrase(HOSTILE_TANAKA_5);
					break;
				case 5:
					NPCPhrase(HOSTILE_TANAKA_6);
					break;
				case 6:
					NPCPhrase(HOSTILE_TANAKA_7);
					break;
				case 7:
					NPCPhrase(HOSTILE_TANAKA_8);
					--NumVisits;
					break;
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::SHOFIXTI_VISITS, static_cast<uint32_t>(NumVisits));

		Hostile((RESPONSE_REF)0);
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

	setSegue(Segue_peace);

	ConversationData* retval {&shofixti_desc};

	return (retval);
}
