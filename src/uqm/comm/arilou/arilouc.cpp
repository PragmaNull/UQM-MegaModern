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
		ARILOU_CONVERSATION, /* AlienConv */
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

	if (PlayerSaid(R, bye_angry_space))
	{
		NPCPhrase(GOODBYE_ANGRY_SPACE);
	}
	else if (PlayerSaid(R, bye_friendly_space))
	{
		NPCPhrase(GOODBYE_FRIENDLY_SPACE);
	}
	else if (PlayerSaid(R, bye_friendly_homeworld))
	{
		NPCPhrase(GOODBYE_FRDLY_HOMEWORLD);
	}
	else if (PlayerSaid(R, lets_fight))
	{
		NPCPhrase(NO_FIGHT);
	}
	else if (PlayerSaid(R, bug_eyed_fruitcakes))
	{
		NPCPhrase(WE_NEVER_FRIENDS);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_MANNER, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, best_if_i_killed_you))
	{
		NPCPhrase(WICKED_HUMAN);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_MANNER, static_cast<uint32_t>(2));
	}
}

static void ArilouHome(RESPONSE_REF R)
{
	uint8_t i {};
	RESPONSE_REF pStr[4];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = pStr[3] = 0;
	if (PlayerSaid(R, confused_by_hello))
	{
		NPCPhrase(CONFUSED_RESPONSE);
	}
	else if (PlayerSaid(R, happy_by_hello))
	{
		NPCPhrase(HAPPY_RESPONSE);
	}
	else if (PlayerSaid(R, miffed_by_hello))
	{
		NPCPhrase(MIFFED_RESPONSE);
	}
	else if (PlayerSaid(R, ok_lets_be_friends))
	{
		NPCPhrase(NO_ALLY_BUT_MUCH_GIVE);
	}
	else if (PlayerSaid(R, what_about_war))
	{
		NPCPhrase(ABOUT_WAR);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_1, static_cast<uint32_t>(1));
		SetHomeworldKnown(SHOFIXTI_HOME);
	}
	else if (PlayerSaid(R, what_about_urquan))
	{
		NPCPhrase(ABOUT_URQUAN);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_1, static_cast<uint32_t>(2));
		SetHomeworldKnown(SYREEN_HOME);
	}
	else if (PlayerSaid(R, tell_arilou_about_tpet))
	{
		NPCPhrase(BAD_NEWS_ABOUT_TPET);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, what_do_about_tpet))
	{
		NPCPhrase(DANGEROUS_BUT_USEFUL);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_2, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, learned_about_umgah))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_CHECKED_UMGAH) != 2)
		{
			NPCPhrase(NO_NEWS_YET);
		}
		else
		{
			NPCPhrase(UMGAH_UNDER_COMPULSION);

			LastStack = 1;

			SetHomeworldKnown(UMGAH_HOME);
		}

		DisablePhrase(learned_about_umgah);
	}
	else if (PlayerSaid(R, umgah_acting_weird))
	{
		NPCPhrase(WELL_GO_CHECK);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_CHECKED_UMGAH, static_cast<uint32_t>(1));
		AddEvent(EventType::Relative, 0, 10, 0, ARILOU_UMGAH_CHECK);
		DisablePhrase(umgah_acting_weird);
	}
	else if (PlayerSaid(R, what_do_now))
	{
		NPCPhrase(GO_FIND_OUT);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_CHECKED_UMGAH, static_cast<uint32_t>(3));
		SetHomeworldKnown(UMGAH_HOME);
	}
	else if (PlayerSaid(R, what_did_on_earth))
	{
		NPCPhrase(DID_THIS);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, why_did_this))
	{
		NPCPhrase(IDF_PARASITES);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_3, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, tell_more))
	{
		NPCPhrase(NOT_NOW);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_3, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, what_give_me))
	{
		NPCPhrase(ABOUT_PORTAL);

		LastStack = 3;
		uqm::gameState().setFlag(uqm::GameFlag::KNOW_ARILOU_WANT_WRECK, static_cast<uint32_t>(1));

		R = about_portal_again;
		DisablePhrase(what_give_me);
	}
	else if (PlayerSaid(R, what_about_tpet))
	{
		NPCPhrase(ABOUT_TPET);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_4, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, about_portal_again))
	{
		NPCPhrase(PORTAL_AGAIN);

		DisablePhrase(about_portal_again);
	}
	else if (PlayerSaid(R, got_it))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_HOME_VISITS) == 1)
		{
			NPCPhrase(CLEVER_HUMAN);
		}
		NPCPhrase(GIVE_PORTAL);

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
			pStr[0] = what_about_war;
			break;
		case 1:
			pStr[0] = what_about_urquan;
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
					pStr[1] = tell_arilou_about_tpet;
					break;
				case 1:
					pStr[1] = what_do_about_tpet;
					break;
			}
		}
		else if (uqm::gameState().flag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES))
		{
			if (!uqm::gameState().flag(uqm::GameFlag::ARILOU_CHECKED_UMGAH))
			{
				pStr[1] = umgah_acting_weird;
			}
			else if (PhraseEnabled(learned_about_umgah) && PhraseEnabled(umgah_acting_weird))
			{
				pStr[1] = learned_about_umgah;
			}
			else if (uqm::gameState().flag(uqm::GameFlag::ARILOU_CHECKED_UMGAH) == 2)
			{
				pStr[1] = what_do_now;
			}
		}
	}
	switch (uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_3))
	{
		case 0:
			pStr[2] = what_did_on_earth;
			break;
		case 1:
			pStr[2] = why_did_this;
			break;
		case 2:
			pStr[2] = tell_more;
			break;
	}
	if (!uqm::gameState().flag(uqm::GameFlag::KNOW_ARILOU_WANT_WRECK))
	{
		pStr[3] = what_give_me;
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_4))
	{
		pStr[3] = what_about_tpet;
	}

	if (pStr[LastStack])
	{
		Response(pStr[LastStack], ArilouHome);
	}
	for (i = 0; i < 4; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			Response(pStr[i], ArilouHome);
		}
	}

	if (uqm::gameState().flag(uqm::GameFlag::KNOW_ARILOU_WANT_WRECK))
	{
		if (uqm::gameState().flag(uqm::GameFlag::PORTAL_KEY_ON_SHIP))
		{
			Response(got_it, ArilouHome);
		}
		else if (PhraseEnabled(about_portal_again) && !uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER))
		{
			Response(about_portal_again, ArilouHome);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER) != 3)
	{
		Response(best_if_i_killed_you, ExitConversation);
	}
	Response(bye_friendly_homeworld, ExitConversation);
}

static void AngryHomeArilou(RESPONSE_REF R)
{
	if (PlayerSaid(R, invaders_from_mars))
	{
		NPCPhrase(HAD_OUR_REASONS);

		DisablePhrase(invaders_from_mars);
	}
	else if (PlayerSaid(R, why_should_i_trust))
	{
		NPCPhrase(TRUST_BECAUSE);

		DisablePhrase(why_should_i_trust);
	}
	else if (PlayerSaid(R, what_about_interference))
	{
		NPCPhrase(INTERFERENCE_NECESSARY);

		DisablePhrase(what_about_interference);
	}
	else if (PlayerSaid(R, i_just_like_to_leave))
	{
		NPCPhrase(SORRY_NO_LEAVE);

		DisablePhrase(i_just_like_to_leave);
	}

	if (PhraseEnabled(invaders_from_mars))
	{
		Response(invaders_from_mars, AngryHomeArilou);
	}
	else
	{
		Response(bug_eyed_fruitcakes, ExitConversation);
	}

	if (PhraseEnabled(why_should_i_trust))
	{
		Response(why_should_i_trust, AngryHomeArilou);
	}
	else if (PhraseEnabled(what_about_interference))
	{
		Response(what_about_interference, AngryHomeArilou);
	}

	Response(ok_lets_be_friends, ArilouHome);

	if (PhraseEnabled(i_just_like_to_leave))
	{
		Response(i_just_like_to_leave, AngryHomeArilou);
	}
}

static void AngrySpaceArilou(RESPONSE_REF R)
{
	if (PlayerSaid(R, im_sorry))
	{
		NPCPhrase(APOLOGIZE_AT_HOMEWORLD);

		DisablePhrase(im_sorry);
	}

	Response(lets_fight, ExitConversation);
	if (PhraseEnabled(im_sorry))
	{
		Response(im_sorry, AngrySpaceArilou);
	}
	Response(bye_angry_space, ExitConversation);
}

static void FriendlySpaceArilou(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, confused_by_hello))
	{
		NPCPhrase(CONFUSED_RESPONSE);
	}
	else if (PlayerSaid(R, happy_by_hello))
	{
		NPCPhrase(HAPPY_RESPONSE);
	}
	else if (PlayerSaid(R, miffed_by_hello))
	{
		NPCPhrase(MIFFED_RESPONSE);
	}
	else if (PlayerSaid(R, whats_up_1)
			 || PlayerSaid(R, whats_up_2))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_2);
				break;
			case 2:
				NPCPhrase(GENERAL_INFO_3);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_2);
	}
	else if (PlayerSaid(R, why_you_here))
	{
		NPCPhrase(LEARN_THINGS);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_5, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, what_things))
	{
		NPCPhrase(THESE_THINGS);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_5, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, why_do_it))
	{
		NPCPhrase(DO_IT_BECAUSE);

		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_5, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, give_me_info_1)
			 || PlayerSaid(R, give_me_info_2))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_HINTS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ARILOU_HINTS_1);
				break;
			case 1:
				NPCPhrase(ARILOU_HINTS_2);
				if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2)
				{
					uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
				}
				break;
			case 2:
				NPCPhrase(ARILOU_HINTS_3);
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(1));
				break;
			case 3:
				NPCPhrase(ARILOU_HINTS_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HINTS, static_cast<uint32_t>(NumVisits));

		DisablePhrase(give_me_info_2);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::ARILOU_STACK_5))
	{
		case 0:
			Response(why_you_here, FriendlySpaceArilou);
			break;
		case 1:
			Response(what_things, FriendlySpaceArilou);
			break;
		case 2:
			Response(why_do_it, FriendlySpaceArilou);
			break;
	}
	if (PhraseEnabled(whats_up_2))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_INFO) == 0)
		{
			Response(whats_up_1, FriendlySpaceArilou);
		}
		else
		{
			Response(whats_up_2, FriendlySpaceArilou);
		}
	}
	if (PhraseEnabled(give_me_info_2))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_HINTS) == 0)
		{
			Response(give_me_info_1, FriendlySpaceArilou);
		}
		else
		{
			Response(give_me_info_2, FriendlySpaceArilou);
		}
	}
	Response(bye_friendly_space, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};
	uint8_t Manner {};

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue::Peace);
		return;
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::MET_ARILOU))
	{
		RESPONSE_FUNC RespFunc {};

		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			NPCPhrase(INIT_HELLO);
			RespFunc = (RESPONSE_FUNC)FriendlySpaceArilou;
		}
		else
		{
			NPCPhrase(FRDLY_HOMEWORLD_HELLO_1);
			RespFunc = (RESPONSE_FUNC)ArilouHome;
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HOME_VISITS, static_cast<uint32_t>(1));
		}
		Response(confused_by_hello, RespFunc);
		Response(happy_by_hello, RespFunc);
		Response(miffed_by_hello, RespFunc);
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
				NPCPhrase(HOSTILE_GOODBYE_1);
				break;
			case 1:
				NPCPhrase(HOSTILE_GOODBYE_2);
				break;
			case 2:
				NPCPhrase(HOSTILE_GOODBYE_3);
				break;
			case 3:
				NPCPhrase(HOSTILE_GOODBYE_4);
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
			NPCPhrase(INIT_ANGRY_HWLD_HELLO);
			uqm::gameState().setFlag(uqm::GameFlag::ARILOU_HOME_VISITS, static_cast<uint32_t>(1));

			AngryHomeArilou((RESPONSE_REF)0);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ARILOU_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ANGRY_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(ANGRY_SPACE_HELLO_2);
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
					NPCPhrase(FRIENDLY_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(FRIENDLY_SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase(FRIENDLY_SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase(FRIENDLY_SPACE_HELLO_4);
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
						NPCPhrase(GOT_PART_YET_1);
						break;
					case 1:
						NPCPhrase(GOT_PART_YET_1);
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
						NPCPhrase(FRDLY_HOMEWORLD_HELLO_1);
						break;
					case 1:
						NPCPhrase(FRDLY_HOMEWORLD_HELLO_2);
						break;
					case 2:
						NPCPhrase(FRDLY_HOMEWORLD_HELLO_3);
						break;
					case 3:
						NPCPhrase(FRDLY_HOMEWORLD_HELLO_4);
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
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
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
