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

static ConversationData urquan_desc =
	{
		URQUAN_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		UrquanPmapAnim, /* AlienFrame */
		UrquanFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		UrquanColorMap, /* AlienColorMap */
		UrquanMusic, /* AlienSong */
		{
								NullResource,	   /* AlienAltFrame */
			UrquanAltcolorMap, /* AlienAltColorMap */
			UrquanProbeMusic,  /* AlienAltSong */
		},
		UrquanConversationPhrases, /* PlayerPhrases */
		9, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				7,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				13,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				19,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				22,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				29,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				36,			  /* StartIndex */
				8,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				44,							 /* StartIndex */
				5,							 /* NumFrames */
				CircularAnim | AnimDisabled, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond * 5, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				49,							 /* StartIndex */
				1,							 /* NumFrames */
				CircularAnim | AnimDisabled, /* AnimFlags */
				0,
				0, /* FrameRate */
				0,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								},
		{
								/* AlienTransitionDesc */
			1, /* StartIndex */
			2, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 30,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			2, /* StartIndex */
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
	setSegue(Segue_hostile);

	if (PlayerSaid(R, you_must_surrender))
	{
		NPCPhrase(NOPE);
	}
	else if (PlayerSaid(R, whats_up_1)
			 || PlayerSaid(R, whats_up_2)
			 || PlayerSaid(R, whats_up_3)
			 || PlayerSaid(R, whats_up_4))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::URQUAN_INFO);
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
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_INFO, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, i_wont_surrender))
	{
		NPCPhrase(BAD_CHOICE);
	}
	else if (PlayerSaid(R, i_will_surrender))
	{
		NPCPhrase(GOOD_CHOICE);

		setSegue(Segue_defeat);
	}
	else if (PlayerSaid(R, like_to_leave))
	{
		NPCPhrase(INDEPENDENCE_IS_BAD);
	}
	else if (PlayerSaid(R, bye_wars_over))
	{
		NPCPhrase(GOODBYE_WARS_OVER);

		setSegue(Segue_peace);
	}
	else if (PlayerSaid(R, bye_sub_hypno))
	{
		NPCPhrase(GOODBYE_AND_DIE_SUB_HYPNO);
	}
	else if (PlayerSaid(R, bye_init_hypno))
	{
		NPCPhrase(GOODBYE_AND_DIE_INIT_HYPNO);

		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_HYPNO_VISITS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, terran_amphibian)
			 || PlayerSaid(R, talking_pet_on_steroids))
	{
		NPCPhrase(BAD_NEWS);

		setSegue(Segue_peace);
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_HYPNO_VISITS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, falling_asleep)
			 || PlayerSaid(R, someone_else_controlled))
	{
		NPCPhrase(SOUNDS_FAMILIAR);

		setSegue(Segue_peace);
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_HYPNO_VISITS, static_cast<uint32_t>(1));
	}
}

static void DescribePet(RESPONSE_REF R)
{
	if (PlayerSaid(R, wascally_little_guy))
	{
		NPCPhrase(WHAT_IT_LOOK_LIKE);
	}
	else if (PlayerSaid(R, turd_and_toad))
	{
		NPCPhrase(WHAT_IS_TURD_AND_TOAD);

		DisablePhrase(turd_and_toad);
	}

	Response(terran_amphibian, CombatIsInevitable);
	Response(talking_pet_on_steroids, CombatIsInevitable);
	if (PhraseEnabled(turd_and_toad))
	{
		Response(turd_and_toad, DescribePet);
	}
}

static void DescribeCompulsion(RESPONSE_REF R)
{
	if (PlayerSaid(R, alien_mind_control))
	{
		NPCPhrase(WHAT_FELT_LIKE);
	}
	else if (PlayerSaid(R, before_coffee))
	{
		NPCPhrase(EXPLAIN);

		DisablePhrase(before_coffee);
	}

	Response(falling_asleep, CombatIsInevitable);
	Response(someone_else_controlled, CombatIsInevitable);
	if (PhraseEnabled(before_coffee))
	{
		Response(before_coffee, DescribeCompulsion);
	}
}

static void MentionCompulsion(RESPONSE_REF R)
{
	if (PlayerSaid(R, compulsion))
	{
		NPCPhrase(WHAT_COMPULSION);

		uqm::gameState().setFlag(uqm::GameFlag::MENTIONED_PET_COMPULSION, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, possessed_by_devil))
	{
		NPCPhrase(STUPID_DEVIL);

		DisablePhrase(possessed_by_devil);
	}
	else if (PlayerSaid(R, why_explain))
	{
		NPCPhrase(MUST_EXPLAIN);

		DisablePhrase(why_explain);
	}

	Response(wascally_little_guy, DescribePet);
	Response(alien_mind_control, DescribeCompulsion);
	if (PhraseEnabled(possessed_by_devil))
	{
		Response(possessed_by_devil, MentionCompulsion);
	}
	if (PhraseEnabled(why_explain))
	{
		Response(why_explain, MentionCompulsion);
	}
}

static void UrquanHypno(RESPONSE_REF R)
{
	if (uqm::gameState().flag(uqm::GameFlag::URQUAN_HYPNO_VISITS) == 0)
	{
		if (R == 0)
		{
			NPCPhrase(INIT_URQUAN_WAKE_UP);
		}
		else if (PlayerSaid(R, where_am_i))
		{
			NPCPhrase(YOU_ARE_HERE);

			DisablePhrase(where_am_i);
		}
		else if (PlayerSaid(R, why_does_my_head_hurt))
		{
			NPCPhrase(HURTS_BECAUSE);

			DisablePhrase(why_does_my_head_hurt);
		}
		else if (PlayerSaid(R, what_about_2_weeks))
		{
			NPCPhrase(ABOUT_2_WEEKS);

			DisablePhrase(what_about_2_weeks);
		}

		if (PhraseEnabled(where_am_i))
		{
			Response(where_am_i, UrquanHypno);
		}
		if (PhraseEnabled(why_does_my_head_hurt))
		{
			Response(why_does_my_head_hurt, UrquanHypno);
		}
		if (PhraseEnabled(what_about_2_weeks))
		{
			Response(what_about_2_weeks, UrquanHypno);
		}
		Response(compulsion, MentionCompulsion);
		Response(bye_init_hypno, CombatIsInevitable);
	}
	else
	{
		if (R == 0)
		{
			NPCPhrase(SUBSEQUENT_URQUAN_WAKE_UP);
		}
		else if (PlayerSaid(R, uh_oh))
		{
			NPCPhrase(NO_UH_OH);

			DisablePhrase(uh_oh);
		}
		else if (PlayerSaid(R, stop_meeting))
		{
			NPCPhrase(NO_STOP_MEETING);

			DisablePhrase(stop_meeting);
		}

		if (PhraseEnabled(uh_oh))
		{
			Response(uh_oh, UrquanHypno);
		}
		if (PhraseEnabled(stop_meeting))
		{
			Response(stop_meeting, UrquanHypno);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::MENTIONED_PET_COMPULSION))
		{
			Response(compulsion, MentionCompulsion);
		}
		Response(bye_sub_hypno, CombatIsInevitable);
	}
}

static void NormalUrquan(RESPONSE_REF R)
{
	if (PlayerSaid(R, i_surrender))
	{
		NPCPhrase(DISOBEDIENCE_PUNISHED);

		Response(i_wont_surrender, CombatIsInevitable);
		Response(i_will_surrender, CombatIsInevitable);
	}
	else
	{
		if (PlayerSaid(R, key_phrase))
		{
			NPCPhrase(URQUAN_STORY);

			uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(2));
		}

		Response(you_must_surrender, CombatIsInevitable);
		if (uqm::gameState().flag(uqm::GameFlag::KNOW_URQUAN_STORY) == 1)
		{
			Response(key_phrase, NormalUrquan);
		}
		switch (uqm::gameState().flag(uqm::GameFlag::URQUAN_INFO))
		{
			case 0:
				Response(whats_up_1, CombatIsInevitable);
				break;
			case 1:
				Response(whats_up_2, CombatIsInevitable);
				break;
			case 2:
				Response(whats_up_3, CombatIsInevitable);
				break;
			case 3:
				Response(whats_up_4, CombatIsInevitable);
				break;
		}
		Response(i_surrender, NormalUrquan);
		Response(like_to_leave, CombatIsInevitable);
	}
}

static void LoserUrquan(RESPONSE_REF R)
{
	if (PlayerSaid(R, why_flee))
	{
		NPCPhrase(FLEE_BECAUSE);

		DisablePhrase(why_flee);
	}
	else if (PlayerSaid(R, what_happens_now))
	{
		NPCPhrase(HAPPENS_NOW);

		DisablePhrase(what_happens_now);
	}
	else if (PlayerSaid(R, what_about_you))
	{
		NPCPhrase(ABOUT_US);

		DisablePhrase(what_about_you);
	}

	if (PhraseEnabled(why_flee))
	{
		Response(why_flee, LoserUrquan);
	}
	if (PhraseEnabled(what_happens_now))
	{
		Response(what_happens_now, LoserUrquan);
	}
	if (PhraseEnabled(what_about_you))
	{
		Response(what_about_you, LoserUrquan);
	}
	Response(bye_wars_over, CombatIsInevitable);
}

static void Intro(void)
{
	if (IsExtended())
	{
		CommData.AlienAmbientArray[7].AnimFlags &= ~AnimDisabled;
		CommData.AlienAmbientArray[8].AnimFlags &= ~CircularAnim;
	}

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue_peace);
		return;
	}

	uint32_t GrpOffs {uqm::gameState().flag(uqm::GameFlag::URQUAN_PROBE_GRPOFFS)};
	if (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
		&& uqm::gameState().game().BattleGroupRef
		&& uqm::gameState().game().BattleGroupRef == GrpOffs)
	{
		NPCPhrase(SEND_MESSAGE);
		uqm::gameState().setFlag(uqm::GameFlag::PROBE_MESSAGE_DELIVERED, static_cast<uint32_t>(1));
	}
	else if (uqm::gameState().flag(uqm::GameFlag::PLAYER_HYPNOTIZED))
	{
		SetCommIntroMode(CIM_FADE_IN_SCREEN, c_gameSecond * 5);
		UrquanHypno((RESPONSE_REF)0);
	}
	else
	{
		uint8_t NumVisits {};

		if (!uqm::gameState().flag(uqm::GameFlag::URQUAN_SENSES_EVIL)
			&& uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
		{
			NPCPhrase(SENSE_EVIL);
			uqm::gameState().setFlag(uqm::GameFlag::URQUAN_SENSES_EVIL, static_cast<uint32_t>(1));
		}

		GrpOffs = uqm::gameState().flag(uqm::GameFlag::COLONY_GRPOFFS);
		if (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
			&& uqm::gameState().game().BattleGroupRef
			&& uqm::gameState().game().BattleGroupRef == GrpOffs)
		{
			NPCPhrase(CAUGHT_YA);

			setSegue(Segue_hostile);
			return;
		}

		GrpOffs = uqm::gameState().flag(uqm::GameFlag::SAMATRA_GRPOFFS);
		if (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
			&& uqm::gameState().game().BattleGroupRef
			&& uqm::gameState().game().BattleGroupRef == GrpOffs)
		{
			NPCPhrase(HELLO_SAMATRA);

			uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
			setSegue(Segue_hostile);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::URQUAN_VISITS);
			if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(INIT_HELLO);
						break;
					case 1:
						NPCPhrase(SUBSEQUENT_HELLO_1);
						break;
					case 2:
						NPCPhrase(SUBSEQUENT_HELLO_2);
						break;
					case 3:
						NPCPhrase(SUBSEQUENT_HELLO_3);
						break;
					case 4:
						NPCPhrase(SUBSEQUENT_HELLO_4);
						--NumVisits;
						break;
				}

				NormalUrquan((RESPONSE_REF)0);
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(INIT_FLEE_HUMAN);
						LoserUrquan((RESPONSE_REF)0);
						break;
					case 1:
						NPCPhrase(SUBSEQUENT_FLEE_HUMAN);
						setSegue(Segue_peace);
						--NumVisits;
						break;
				}
			}
			uqm::gameState().setFlag(uqm::GameFlag::URQUAN_VISITS, static_cast<uint32_t>(NumVisits));
		}
	}
}

static uint16_t uninit_urquan(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_urquan_enc(void)
{
	uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HYPNOTIZED, static_cast<uint32_t>(0));
}

ConversationData* init_urquan_comm(void)
{
	urquan_desc.init_encounter_func = Intro;
	urquan_desc.post_encounter_func = post_urquan_enc;
	urquan_desc.uninit_encounter_func = uninit_urquan;

	uqm::lua().beginComm(nullptr, NullResource);

	urquan_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	urquan_desc.AlienTextBaseline.y = 0;
	urquan_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	const uint32_t GrpOffs {uqm::gameState().flag(uqm::GameFlag::URQUAN_PROBE_GRPOFFS)};

	// use alternate "Probe" track if available
	if ((uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
		 && uqm::gameState().game().BattleGroupRef
		 && uqm::gameState().game().BattleGroupRef == GrpOffs))
	{
		altResFlags |= UseAltSong;
	}

	if (IsExtended())
	{
		altResFlags |= UseAltColormap;
	}

	if (uqm::gameState().flag(uqm::GameFlag::PLAYER_HYPNOTIZED)
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE
		|| (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
			&& uqm::gameState().game().BattleGroupRef
			&& uqm::gameState().game().BattleGroupRef == GrpOffs))
	{
		setSegue(Segue_peace);
	}
	else
	{
		setSegue(Segue_hostile);
	}
	ConversationData* retval {&urquan_desc};

	return (retval);
}
