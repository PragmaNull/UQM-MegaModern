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
#include "uqm/build.h"


static ConversationData slylandro_desc =
	{
		Conversation::SlylandroHome, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SlylandroPmapAnim, /* AlienFrame */
		SlylandroFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth(), */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SlylandroColorMap, /* AlienColorMap */
		SlylandroMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		SlylandroConversationPhrases, /* PlayerPhrases */
		13, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				0,											/* StartIndex */
				5,											/* NumFrames */
				RandomAnim | ColorxformAnim | AnimDisabled, /* AnimFlags */
				GameTicksPerSecond / 8,
				GameTicksPerSecond * 5 / 8, /* FrameRate */
				GameTicksPerSecond / 8,
				GameTicksPerSecond * 5 / 8, /* RestartRate */
				0,							/* BlockMask */
			},
								{
				1,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				6,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				11,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,		  /* StartIndex */
				6,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				22,			  /* StartIndex */
				8,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond / 15,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				30,			  /* StartIndex */
				9,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 8) | (1 << 9),	/* BlockMask */
			},
								{
				39,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				43,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6),				/* BlockMask */
			},
								{
				48,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6),				/* BlockMask */
			},
								{
				54,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
								{
				60,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
								{
				67,			  /* StartIndex */
				8,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10) | (1 << 11),	/* BlockMask */
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
								/* AlienTalkDesc - empty */
			0, /* StartIndex */
			0, /* NumFrames */
			0, /* AnimFlags */
			0,
								0, /* FrameRate */
			0,
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

static AnimFilterSet slyhome_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,				   /* Color index */
				1,				   /* Opacity index */
				-1,				   /* Frame index */
				DrawKind::Overlay, /* DrawKind*/
				0,				   /* Flags */
			},
								  }
};

static void ExitConversation(RESPONSE_REF R)
{
	(void)R; // ignored
	setSegue(Segue::Peace);

	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_HOME_VISITS))
	{
		case 1:
			NPCPhrase(GOODBYE_1);
			break;
		default:
			NPCPhrase(GOODBYE_2);
			break;
	}
}

static void HomeWorld(RESPONSE_REF R);

static void HumanInfo(RESPONSE_REF R)
{
	if (PlayerSaid(R, happy_to_tell_more))
	{
		NPCPhrase(TELL_MORE);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK4, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, would_you_like_to_know_more))
	{
		NPCPhrase(YES_TELL_MORE);
	}
	else if (PlayerSaid(R, we_come_from_earth))
	{
		NPCPhrase(OK_EARTH);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_EARTH, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, we_explore))
	{
		NPCPhrase(OK_EXPLORE);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_EXPLORE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, we_fight_urquan))
	{
		NPCPhrase(URQUAN_NICE_GUYS);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, not_same_urquan))
	{
		NPCPhrase(PERSONALITY_CHANGE);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, we_gather))
	{
		NPCPhrase(MAYBE_INTERESTED);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_GATHER, static_cast<uint32_t>(1));
	}

	uint8_t InfoLeft {false};
	if (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN) == 1)
	{
		InfoLeft = true;
		Response(not_same_urquan, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_EARTH))
	{
		InfoLeft = true;
		Response(we_come_from_earth, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_EXPLORE))
	{
		InfoLeft = true;
		Response(we_explore, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN))
	{
		InfoLeft = true;
		Response(we_fight_urquan, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_GATHER))
	{
		InfoLeft = true;
		Response(we_gather, HumanInfo);
	}

	Response(enough_about_me, HomeWorld);
	if (!InfoLeft)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK4, static_cast<uint32_t>(2));
	}
}

static void SlylandroInfo(RESPONSE_REF R)
{
	if (PlayerSaid(R, like_more_about_you))
	{
		NPCPhrase(SURE_KNOW_WHAT);
	}
	else if (PlayerSaid(R, what_about_home))
	{
		NPCPhrase(ABOUT_HOME);

		DisablePhrase(what_about_home);
	}
	else if (PlayerSaid(R, what_about_culture))
	{
		NPCPhrase(ABOUT_CULTURE);

		DisablePhrase(what_about_culture);
	}
	else if (PlayerSaid(R, what_about_history))
	{
		NPCPhrase(ABOUT_HISTORY);

		DisablePhrase(what_about_history);
	}
	else if (PlayerSaid(R, what_about_biology))
	{
		NPCPhrase(ABOUT_BIOLOGY);

		DisablePhrase(what_about_biology);
	}

	uint8_t InfoLeft {false};
	if (PhraseEnabled(what_about_home))
	{
		InfoLeft = true;
		Response(what_about_home, SlylandroInfo);
	}
	if (PhraseEnabled(what_about_culture))
	{
		InfoLeft = true;
		Response(what_about_culture, SlylandroInfo);
	}
	if (PhraseEnabled(what_about_history))
	{
		InfoLeft = true;
		Response(what_about_history, SlylandroInfo);
	}
	if (PhraseEnabled(what_about_biology))
	{
		InfoLeft = true;
		Response(what_about_biology, SlylandroInfo);
	}

	Response(enough_info, HomeWorld);
	if (!InfoLeft)
	{
		DisablePhrase(like_more_about_you);
	}
}

static void FixBug(RESPONSE_REF R)
{
	if (PlayerSaid(R, think_about_rep_priorities))
	{
		NPCPhrase(UH_OH);
	}
	else if (PlayerSaid(R, hunt_them_down))
	{
		NPCPhrase(GROW_TOO_FAST);

		DisablePhrase(hunt_them_down);
	}
	else if (PlayerSaid(R, sue_melnorme))
	{
		NPCPhrase(SIGNED_WAIVER);

		DisablePhrase(sue_melnorme);
	}
	else if (PlayerSaid(R, recall_signal))
	{
		NPCPhrase(NOT_THIS_MODEL);

		DisablePhrase(recall_signal);
	}

	if (PhraseEnabled(hunt_them_down))
	{
		Response(hunt_them_down, FixBug);
	}
	if (PhraseEnabled(sue_melnorme))
	{
		Response(sue_melnorme, FixBug);
	}
	if (PhraseEnabled(recall_signal))
	{
		Response(recall_signal, FixBug);
	}
	Response(mega_self_destruct, HomeWorld);
}

static void ProbeBug(RESPONSE_REF R)
{
	if (PlayerSaid(R, probe_has_bug))
	{
		NPCPhrase(NO_IT_DOESNT);
	}
	else if (PlayerSaid(R, tell_me_about_rep_2))
	{
		NPCPhrase(REP_NO_PROBLEM);

		DisablePhrase(tell_me_about_rep_2);
	}
	else if (PlayerSaid(R, what_about_rep_priorities))
	{
		NPCPhrase(MAXIMUM_SO_WHAT);

		DisablePhrase(what_about_rep_priorities);
	}
	else if (PlayerSaid(R, tell_me_about_attack))
	{
		NPCPhrase(ATTACK_NO_PROBLEM);

		DisablePhrase(tell_me_about_attack);
	}

	if (PhraseEnabled(tell_me_about_rep_2))
	{
		Response(tell_me_about_rep_2, ProbeBug);
	}
	else if (PhraseEnabled(what_about_rep_priorities))
	{
		Response(what_about_rep_priorities, ProbeBug);
	}
	else
	{
		Response(think_about_rep_priorities, FixBug);
	}
	if (PhraseEnabled(tell_me_about_attack))
	{
		Response(tell_me_about_attack, ProbeBug);
	}
}

static void ProbeInfo(RESPONSE_REF R);

static void ProbeFunction(RESPONSE_REF R)
{
	RESPONSE_REF pStr[2];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = 0;
	if (PlayerSaid(R, talk_more_probe_attack))
	{
		NPCPhrase(NO_PROBLEM_BUT_SURE);
	}
	else if (PlayerSaid(R, tell_me_about_basics))
	{
		NPCPhrase(BASIC_COMMANDS);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_PROGRAM, static_cast<uint32_t>(1));
		DisablePhrase(tell_basics_again);
	}
	else if (PlayerSaid(R, tell_basics_again))
	{
		NPCPhrase(OK_BASICS_AGAIN);

		DisablePhrase(tell_basics_again);
	}
	else if (PlayerSaid(R, what_effect))
	{
		NPCPhrase(AFFECTS_BEHAVIOR);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_EFFECTS, static_cast<uint32_t>(1));
		DisablePhrase(what_effect);
	}
	else if (PlayerSaid(R, tell_me_about_rep_1))
	{
		NPCPhrase(ABOUT_REP);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK8, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, what_set_priority))
	{
		NPCPhrase(MAXIMUM);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_PRIORITY, static_cast<uint32_t>(1));
		DisablePhrase(what_set_priority);
	}
	else if (PlayerSaid(R, how_does_probe_defend))
	{
		NPCPhrase(ONLY_SELF_DEFENSE);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK9, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, combat_behavior))
	{
		NPCPhrase(MISSILE_BATTERIES);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK9, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, what_missile_batteries))
	{
		NPCPhrase(LIGHTNING_ONLY_FOR_HARVESTING);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK9, static_cast<uint32_t>(3));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK9))
	{
		case 0:
			pStr[0] = how_does_probe_defend;
			break;
		case 1:
			pStr[0] = combat_behavior;
			break;
		case 2:
			pStr[0] = what_missile_batteries;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK8))
	{
		case 2:
			pStr[1] = tell_me_about_rep_1;
			break;
		case 3:
			if (PhraseEnabled(what_set_priority))
			{
				pStr[1] = what_set_priority;
			}
			break;
	}

	if (LastStack && pStr[LastStack - 1])
	{
		Response(pStr[LastStack - 1], ProbeFunction);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_PROGRAM))
	{
		Response(tell_me_about_basics, ProbeFunction);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_PRIORITY))
		{
			if (uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_EFFECTS))
			{
				Response(probe_has_bug, ProbeBug);
			}
			if (PhraseEnabled(what_effect))
			{
				Response(what_effect, ProbeFunction);
			}
		}
		if (PhraseEnabled(tell_basics_again))
		{
			Response(tell_basics_again, ProbeFunction);
		}
	}
	if (LastStack == 0)
	{
		do
		{
			if (pStr[LastStack])
			{
				Response(pStr[LastStack], ProbeFunction);
			}
		} while (++LastStack < 2);
	}
	else
	{
		LastStack = (LastStack - 1) ^ 1;
		if (pStr[LastStack])
		{
			Response(pStr[LastStack], ProbeFunction);
		}
	}

	Response(enough_problem, ProbeInfo);
}

static void ProbeInfo(RESPONSE_REF R)
{
	uint8_t i {};
	uint8_t InfoLeft {};
	RESPONSE_REF pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PlayerSaid(R, what_are_probes))
	{
		NPCPhrase(PROBES_ARE);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK5, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, know_more_probe))
	{
		NPCPhrase(OK_WHAT_MORE_PROBE);
	}
	else if (PlayerSaid(R, why_probe_always_attack))
	{
		NPCPhrase(ONLY_DEFEND);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK6, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, talk_more_probe_attack))
	{
		ProbeFunction(R);
		return;
	}
	else if (PlayerSaid(R, where_probes_from))
	{
		NPCPhrase(PROBES_FROM_MELNORME);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK7, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, why_sell))
	{
		NPCPhrase(SELL_FOR_INFO);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK7, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, how_long_ago))
	{
		NPCPhrase(FIFTY_THOUSAND_ROTATIONS);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK7, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, whats_probes_mission))
	{
		NPCPhrase(SEEK_OUT_NEW_LIFE);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK8, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, if_only_one))
	{
		NPCPhrase(THEY_REPLICATE);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK8, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, enough_problem))
	{
		NPCPhrase(OK_ENOUGH_PROBLEM);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN)
		&& uqm::gameState().flag(uqm::GameFlag::PROBE_EXHIBITED_BUG))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK6))
		{
			case 0:
				pStr[0] = why_probe_always_attack;
				break;
			case 1:
				pStr[0] = talk_more_probe_attack;
				break;
		}
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK7))
	{
		case 0:
			pStr[1] = where_probes_from;
			break;
		case 1:
			pStr[1] = why_sell;
			break;
		case 2:
			pStr[1] = how_long_ago;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK8))
	{
		case 0:
			pStr[2] = whats_probes_mission;
			break;
		case 1:
			pStr[2] = if_only_one;
			break;
	}

	InfoLeft = false;
	if (pStr[LastStack])
	{
		InfoLeft = true;
		Response(pStr[LastStack], ProbeInfo);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			InfoLeft = true;
			Response(pStr[i], ProbeInfo);
		}
	}

	Response(enough_probe, HomeWorld);
	if (!InfoLeft)
	{
		DisablePhrase(know_more_probe);
	}
}

static void HomeWorld(RESPONSE_REF R)
{
	uint8_t i {};
	RESPONSE_REF pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PlayerSaid(R, we_are_us))
	{
		NPCPhrase(TERRIBLY_EXCITING);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(1));
		DisablePhrase(we_are_us);
	}
	else if (PlayerSaid(R, what_other_visitors))
	{
		NPCPhrase(VISITORS);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_PROBE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, any_other_visitors))
	{
		NPCPhrase(LONG_AGO);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, what_about_sentient_milieu))
	{
		NPCPhrase(MET_TAALO_THEY_ARE_FROM);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, who_else))
	{
		NPCPhrase(PRECURSORS);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, precursors_yow))
	{
		NPCPhrase(ABOUT_PRECURSORS);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(6));
	}
	else if (PlayerSaid(R, must_know_more))
	{
		NPCPhrase(ALL_WE_KNOW);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(7));
	}
	else if (PlayerSaid(R, who_are_you))
	{
		NPCPhrase(WE_ARE_SLY);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, where_are_you))
	{
		NPCPhrase(DOWN_HERE);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, thats_impossible_1))
	{
		NPCPhrase(NO_ITS_NOT_1);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK3, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, thats_impossible_2))
	{
		NPCPhrase(NO_ITS_NOT_2);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK3, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, like_more_about_you))
	{
		SlylandroInfo(R);
		return;
	}
	else if (PlayerSaid(R, enough_about_me))
	{
		NPCPhrase(OK_ENOUGH_YOU);
	}
	else if (PlayerSaid(R, enough_info))
	{
		NPCPhrase(OK_ENOUGH_INFO);
	}
	else if (PlayerSaid(R, enough_probe))
	{
		NPCPhrase(OK_ENOUGH_PROBE);
	}
	else if (PlayerSaid(R, mega_self_destruct))
	{
		NPCPhrase(WHY_YES_THERE_IS);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP, static_cast<uint32_t>(1));
		i = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_MULTIPLIER) + 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_MULTIPLIER, static_cast<uint32_t>(i));
		AddEvent(EventType::Relative, 0, 0, 0, GameEventId::SlylandroRampDown);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK1))
	{
		case 0:
			pStr[0] = we_are_us;
			break;
		case 1:
			pStr[0] = what_other_visitors;
			break;
		case 2:
			pStr[0] = any_other_visitors;
			break;
		case 3:
			pStr[0] = what_about_sentient_milieu;
			break;
		case 4:
			pStr[0] = who_else;
			break;
		case 5:
			pStr[0] = precursors_yow;
			break;
		case 6:
			pStr[0] = must_know_more;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK2))
	{
		case 0:
			pStr[1] = who_are_you;
			break;
		case 1:
			if (PhraseEnabled(like_more_about_you))
			{
				pStr[1] = like_more_about_you;
			}
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK3))
	{
		case 0:
			pStr[2] = where_are_you;
			break;
		case 1:
			pStr[2] = thats_impossible_1;
			break;
		case 2:
			pStr[2] = thats_impossible_2;
			break;
	}

	if (pStr[LastStack])
	{
		Response(pStr[LastStack], HomeWorld);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			Response(pStr[i], HomeWorld);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK1))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK4))
		{
			case 0:
				Response(happy_to_tell_more, HumanInfo);
				break;
			case 1:
				Response(would_you_like_to_know_more, HumanInfo);
				break;
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_PROBE)
		&& !uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK5))
		{
			case 0:
				Response(what_are_probes, ProbeInfo);
				break;
			case 1:
				if (PhraseEnabled(know_more_probe))
				{
					Response(know_more_probe, ProbeInfo);
				}
				break;
		}
	}
	Response(bye, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};


	if (IsHd())
	{
		EngageFilters(&slyhome_filters);
	}

	CommData.AlienAmbientArray[0].AnimFlags &= ~AnimDisabled;

	if (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN)
		&& (NumVisits = uqm::gameState().flag(uqm::GameFlag::RECALL_VISITS)) == 0)
	{
		NPCPhrase(RECALL_PROGRAM_1);
		++NumVisits;
		uqm::gameState().setFlag(uqm::GameFlag::RECALL_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HELLO_1);
				break;
			case 1:
				NPCPhrase(HELLO_2);
				break;
			case 2:
				NPCPhrase(HELLO_3);
				break;
			case 3:
				NPCPhrase(HELLO_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		SetHomeworldKnown(*HomeId::Slylandro);
	}

	HomeWorld((RESPONSE_REF)0);
}

static uint16_t uninit_slylandro(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_slylandro_enc(void)
{
	// nothing defined so far
}

ConversationData* init_slylandro_comm(void)
{
	slylandro_desc.init_encounter_func = Intro;
	slylandro_desc.post_encounter_func = post_slylandro_enc;
	slylandro_desc.uninit_encounter_func = uninit_slylandro;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	slylandro_desc.AlienTextBaseline.x =
		TextXOffs() + (SisTextWidth() >> 1);
	slylandro_desc.AlienTextBaseline.y = 0;
	slylandro_desc.AlienTextWidth = SisTextWidth();

	setSegue(Segue::Peace);
	ConversationData* retval {&slylandro_desc};

	return (retval);
}
