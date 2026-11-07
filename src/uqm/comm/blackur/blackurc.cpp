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

static ConversationData blackurq_desc =
	{
		BLACKURQ_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		BlackurqPmapAnim, /* AlienFrame */
		BlackurqFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		BlackurqColorMap, /* AlienColorMap */
		BlackurqMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		BlackurqConversationPhrases, /* PlayerPhrases */
		9, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				7,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				13,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				// New eye animation
				42,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				20,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				23,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				26,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				29,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				// "Left" extremity
				33,							/* StartIndex */
				5,							/* NumFrames */
				CircularAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				// "Right" extremeity
				38,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
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
	uint8_t NumVisits {};

	setSegue(Segue::Hostile);

	if (PlayerSaid(R, bye))
	{
		if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_BYES) == 0)
		{
			NPCPhrase(GOODBYE_AND_DIE);
		}
		else
		{
			NPCPhrase(DIE_HUMAN /* GOODBYE_AND_DIE_2 */);
		}

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_BYES, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, guess_thats_all))
	{
		NPCPhrase(THEN_DIE);
	}
	else if (PlayerSaid(R, what_are_you_hovering_over))
	{
		NPCPhrase(BONE_PILE);

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, you_sure_are_creepy))
	{
		NPCPhrase(YES_CREEPY);

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, stop_that_gross_blinking))
	{
		NPCPhrase(DIE_HUMAN);

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, threat_1)
			 || PlayerSaid(R, threat_2)
			 || PlayerSaid(R, threat_3)
			 || PlayerSaid(R, threat_4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(RESISTANCE_IS_USELESS_1);
				break;
			case 1:
				NPCPhrase(RESISTANCE_IS_USELESS_2);
				break;
			case 2:
				NPCPhrase(RESISTANCE_IS_USELESS_3);
				break;
			case 3:
				NPCPhrase(RESISTANCE_IS_USELESS_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_REASONS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, plead_1)
			 || PlayerSaid(R, plead_2)
			 || PlayerSaid(R, plead_3)
			 || PlayerSaid(R, plead_4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(PLEADING_IS_USELESS_1);
				break;
			case 1:
				NPCPhrase(PLEADING_IS_USELESS_2);
				break;
			case 2:
				// This response disabled due to lack of a speech file.
				// NPCPhrase (PLEADING_IS_USELESS_3);
				// break;
			case 3:
				NPCPhrase(PLEADING_IS_USELESS_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_PLEAD, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, why_kill_all_1)
			 || PlayerSaid(R, why_kill_all_2)
			 || PlayerSaid(R, why_kill_all_3)
			 || PlayerSaid(R, why_kill_all_4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(KILL_BECAUSE_1);
				break;
			case 1:
				NPCPhrase(KILL_BECAUSE_2);
				break;
			case 2:
				NPCPhrase(KILL_BECAUSE_3);
				break;
			case 3:
				NPCPhrase(KILL_BECAUSE_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_REASONS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, please_dont_kill_1)
			 || PlayerSaid(R, please_dont_kill_2)
			 || PlayerSaid(R, please_dont_kill_3)
			 || PlayerSaid(R, please_dont_kill_4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(WILL_KILL_1);
				break;
			case 1:
				NPCPhrase(WILL_KILL_2);
				break;
			case 2:
				NPCPhrase(WILL_KILL_3);
				break;
			case 3:
				NPCPhrase(WILL_KILL_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_PLEAD, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, bye_frenzy_1)
			 || PlayerSaid(R, bye_frenzy_2)
			 || PlayerSaid(R, bye_frenzy_3)
			 || PlayerSaid(R, bye_frenzy_4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GOODBYE_AND_DIE_FRENZY_1);
				break;
			case 1:
				NPCPhrase(GOODBYE_AND_DIE_FRENZY_2);
				break;
			case 2:
				NPCPhrase(GOODBYE_AND_DIE_FRENZY_3);
				break;
			case 3:
				NPCPhrase(GOODBYE_AND_DIE_FRENZY_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(NumVisits));
	}
}

static void Frenzy(RESPONSE_REF R)
{
	(void)R; // ignored
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS))
	{
		case 0:
			Response(why_kill_all_1, CombatIsInevitable);
			break;
		case 1:
			Response(why_kill_all_2, CombatIsInevitable);
			break;
		case 2:
			Response(why_kill_all_3, CombatIsInevitable);
			break;
		case 3:
			Response(why_kill_all_4, CombatIsInevitable);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD))
	{
		case 0:
			Response(please_dont_kill_1, CombatIsInevitable);
			break;
		case 1:
			Response(please_dont_kill_2, CombatIsInevitable);
			break;
		case 2:
			Response(please_dont_kill_3, CombatIsInevitable);
			break;
		case 3:
			Response(please_dont_kill_4, CombatIsInevitable);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_INFO))
	{
		case 0:
			Response(bye_frenzy_1, CombatIsInevitable);
			break;
		case 1:
			Response(bye_frenzy_2, CombatIsInevitable);
			break;
		case 2:
			Response(bye_frenzy_3, CombatIsInevitable);
			break;
		case 3:
			Response(bye_frenzy_4, CombatIsInevitable);
			break;
	}
}

static void KohrAhStory(RESPONSE_REF R)
{
	if (PlayerSaid(R, key_phrase))
	{
		NPCPhrase(RESPONSE_TO_KEY_PHRASE);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, why_do_you_destroy))
	{
		NPCPhrase(WE_WERE_SLAVES);

		DisablePhrase(why_do_you_destroy);
	}
	else if (PlayerSaid(R, relationship_with_urquan))
	{
		NPCPhrase(WE_ARE_URQUAN_TOO);

		DisablePhrase(relationship_with_urquan);
	}
	else if (PlayerSaid(R, what_about_culture))
	{
		NPCPhrase(BONE_GARDENS);

		DisablePhrase(what_about_culture);
	}
	else if (PlayerSaid(R, how_leave_me_alone))
	{
		NPCPhrase(YOU_DIE);

		DisablePhrase(how_leave_me_alone);
	}

	if (PhraseEnabled(why_do_you_destroy))
	{
		Response(why_do_you_destroy, KohrAhStory);
	}
	if (PhraseEnabled(relationship_with_urquan))
	{
		Response(relationship_with_urquan, KohrAhStory);
	}
	if (PhraseEnabled(what_about_culture))
	{
		Response(what_about_culture, KohrAhStory);
	}
	if (PhraseEnabled(how_leave_me_alone))
	{
		Response(how_leave_me_alone, KohrAhStory);
	}
	Response(guess_thats_all, CombatIsInevitable);
}

static void DieHuman(RESPONSE_REF R)
{
	(void)R; // ignored
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS))
	{
		case 0:
			Response(threat_1, CombatIsInevitable);
			break;
		case 1:
			Response(threat_2, CombatIsInevitable);
			break;
		case 2:
			Response(threat_3, CombatIsInevitable);
			break;
		case 3:
			Response(threat_4, CombatIsInevitable);
			break;
	}
	if (uqm::gameState().flag(uqm::GameFlag::KNOW_KOHR_AH_STORY) == 1)
	{
		Response(key_phrase, KohrAhStory);
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_INFO))
	{
		case 0:
			Response(what_are_you_hovering_over, CombatIsInevitable);
			break;
		case 1:
			Response(you_sure_are_creepy, CombatIsInevitable);
			break;
		case 2:
			Response(stop_that_gross_blinking, CombatIsInevitable);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD))
	{
		case 0:
			Response(plead_1, CombatIsInevitable);
			break;
		case 1:
			Response(plead_2, CombatIsInevitable);
			break;
		case 2:
			// This response disabled due to lack of a speech file.
			// Response (plead_3, CombatIsInevitable);
			// break;
		case 3:
			Response(plead_4, CombatIsInevitable);
			break;
	}
	Response(bye, CombatIsInevitable);
}

static void Intro(void)
{
	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_KILLED_ALL))
	{
		NPCPhrase(GAME_OVER_DUDE);

		setSegue(Segue::Peace);
		return;
	}

	if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_SENSES_EVIL)
		&& uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		NPCPhrase(SENSE_EVIL);
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_SENSES_EVIL, static_cast<uint32_t>(1));
	}

	const uint32_t GrpOffs {uqm::gameState().flag(uqm::GameFlag::SAMATRA_GRPOFFS)};
	if (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
		&& uqm::gameState().game().BattleGroupRef
		&& uqm::gameState().game().BattleGroupRef == GrpOffs)
	{
		NPCPhrase(HELLO_SAMATRA);

		uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
		setSegue(Segue::Hostile);
	}
	else
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_VISITS);
		if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(WE_KILL_ALL_1);
					break;
				case 1:
					NPCPhrase(WE_KILL_ALL_2);
					break;
				case 2:
					NPCPhrase(WE_KILL_ALL_3);
					break;
				case 3:
					NPCPhrase(WE_KILL_ALL_4);
					--NumVisits;
					break;
			}

			Frenzy((RESPONSE_REF)0);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(HELLO_AND_DIE_1);
					break;
				case 1:
					NPCPhrase(HELLO_AND_DIE_2);
					break;
				case 2:
					NPCPhrase(HELLO_AND_DIE_3);
					break;
				case 3:
					NPCPhrase(HELLO_AND_DIE_4);
					--NumVisits;
					break;
			}

			DieHuman((RESPONSE_REF)0);
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_VISITS, static_cast<uint32_t>(NumVisits));
	}
}

static uint16_t uninit_blackurq(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_blackurq_enc(void)
{
	// nothing defined so far
}

ConversationData* init_blackurq_comm(void)
{
	blackurq_desc.init_encounter_func = Intro;
	blackurq_desc.post_encounter_func = post_blackurq_enc;
	blackurq_desc.uninit_encounter_func = uninit_blackurq;

	uqm::lua().beginComm(nullptr, NullResource);

	blackurq_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	blackurq_desc.AlienTextBaseline.y = 0;
	blackurq_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_KILLED_ALL)
		&& uqm::lowByte(uqm::currentActivity()) != WON_LAST_BATTLE)
	{
		setSegue(Segue::Hostile);
	}
	else
	{
		setSegue(Segue::Peace);
	}
	ConversationData* retval {&blackurq_desc};

	return (retval);
}
