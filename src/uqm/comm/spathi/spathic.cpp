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

#include "uqm/build.h"
#include "uqm/lua/UQMLua.h"


static ConversationData spathi_desc =
	{
		SPATHI_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SpathiPmapAnim, /* AlienFrame */
		SpathiFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SpathiColorMap, /* AlienColorMap */
		SpathiMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			FwiffoMusic,  /* AlienAltSong */
		},
		SpathiConversationPhrases, /* PlayerPhrases */
		8, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				1,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				0,
				GameTicksPerSecond, /* RestartRate */
				(1 << 1),			/* BlockMask */
			},
								{
				7,			  /* StartIndex */
				9,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0,		  /* RestartRate */
				(1 << 0), /* BlockMask */
			},
								{
				16,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 4),				 /* BlockMask */
			},
								{20,											   /* StartIndex */
			 4,												   /* NumFrames */
			 YoyoAnim,										   /* AnimFlags */
			 GameTicksPerSecond / 15, GameTicksPerSecond / 15, /* FrameRate */
			 GameTicksPerSecond / 15, GameTicksPerSecond / 15, /* FrameRate */
			 (1 << 5)},
								{
				24,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2),				/* BlockMask */
			},


								{
				34,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3),				/* BlockMask */
			},
								{
				38,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				41,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
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
#ifdef NEVER
		{
								/* AlienTalkDesc */
			29, /* StartIndex */
			5,	/* NumFrames */
			0,	/* AnimFlags */
			GameTicksPerSecond / 15,
								0, /* FrameRate */
			GameTicksPerSecond / 12,
								0, /* RestartRate */
			0, /* BlockMask */
		},
#else
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
#endif	 /* NEVER */
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static void ExitConversation(RESPONSE_REF Response)
{
	const uint8_t SpaCrew = ifEasyDifficulty(MaxCrewSize, 1);
	setSegue(Segue::Peace);

	if (PlayerSaid(Response, bye_ally_space))
	{
		NPCPhrase(GOODBYE_ALLY_SPACE);
	}
	else if (PlayerSaid(Response, bye_friendly_space))
	{
		NPCPhrase(GOODBYE_FRIENDLY_SPACE);
	}
	else if (PlayerSaid(Response, part_in_peace))
	{
		NPCPhrase(KEEP_IT_SECRET);
	}
	else if (PlayerSaid(Response, we_sorry_space))
	{
		NPCPhrase(APOLOGIZE_AT_HOMEWORLD);
	}
	else if (PlayerSaid(Response, give_info_space))
	{
		NPCPhrase(HERES_SOME_INFO);
	}
	else if (PlayerSaid(Response, bye_angry_space))
	{
		NPCPhrase(GOODBYE_ANGRY_SPACE);
	}
	else if (PlayerSaid(Response, attack_you_now))
	{
		NPCPhrase(YIPES);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(Response, we_fight_again_space))
	{
		NPCPhrase(OK_FIGHT_AGAIN_SPACE);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(Response, die_slugboy)
			 || PlayerSaid(Response, we_fight_1)
			 || PlayerSaid(Response, we_fight_2)
			 || PlayerSaid(Response, pay_for_crimes)
			 || PlayerSaid(Response, tell_me_coordinates)
			 || PlayerSaid(Response, changed_mind))
	{
		if (PlayerSaid(Response, tell_me_coordinates))
		{
			NPCPhrase(FAKE_COORDINATES);
		}
		NPCPhrase(OK_WE_FIGHT_AT_PLUTO);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(Response, join_us))
	{
		if (EscortFeasibilityStudy(SPATHI_SHIP) == 0)
		{
			NPCPhrase(TOO_SCARY);
		}
		else
		{
			NPCPhrase(WILL_JOIN);

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(SPATHI_SHIP, 1);
			/* Make the Eluder escort captained by Fwiffo alone */
			SetEscortCrewComplement(SPATHI_SHIP, SpaCrew,
									NameOffset + NumCaptainsNames);
		}
	}
	else if (PlayerSaid(Response, join_us_nomad))
	{
		if ((EscortFeasibilityStudy(SPATHI_SHIP) == 0
			 || CountEscortShips(SPATHI_SHIP) > 2))
		{
			NPCPhrase(TOO_SCARY);
		}
		else
		{
			NPCPhrase(WILL_JOIN);

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(SPATHI_SHIP, 1);
		}
	}
}

static uint8_t join_us_refusals;

static void SpathiOnPluto(RESPONSE_REF R)
{
	if (PlayerSaid(R, hi_there))
	{
		NPCPhrase(ARE_YOU_SURE);
	}
	else if (PlayerSaid(R, dont_kill))
	{
		NPCPhrase(OK_WONT);
	}
	else if (PlayerSaid(R, youre_forgiven))
	{
		NPCPhrase(THANKS_FOR_FORGIVENESS);
	}
	else if (PlayerSaid(R, you_wont_die_yet))
	{
		NPCPhrase(ETERNAL_GRATITUDE);
	}
	else if (PlayerSaid(R, you_may_live))
	{
		NPCPhrase(GEE_THANKS);
	}
	else if (PlayerSaid(R, youve_got_me_all_wrong))
	{
		NPCPhrase(SORRY_NO_COORDS);
	}
	else if (PlayerSaid(R, what_doing_on_pluto_1))
	{
		NPCPhrase(ABOUT_20_YEARS_AGO);

		DisablePhrase(what_doing_on_pluto_1);
	}
	else if (PlayerSaid(R, what_doing_on_pluto_2))
	{
		NPCPhrase(WHEN_URQUAN_ARRIVED);

		DisablePhrase(what_doing_on_pluto_2);
	}
	else if (PlayerSaid(R, what_doing_on_pluto_3))
	{
		NPCPhrase(STATIONED_ON_EARTH_MOON);

		DisablePhrase(what_doing_on_pluto_3);
	}
	else if (PlayerSaid(R, what_about_ilwrath))
	{
		NPCPhrase(ABOUT_ILWRATH);

		DisablePhrase(what_about_ilwrath);
	}
	else if (PlayerSaid(R, when_ilwrath))
	{
		NPCPhrase(THEN_ILWRATH);

		DisablePhrase(when_ilwrath);
	}
	else if (PlayerSaid(R, what_about_moonbase))
	{
		NPCPhrase(SET_UP_BASE);

		DisablePhrase(what_about_moonbase);
	}
	else if (PlayerSaid(R, what_about_other_spathi))
	{
		NPCPhrase(SPATHI_ARE);

		DisablePhrase(what_about_other_spathi);
	}
	else if (PlayerSaid(R, what_about_other_spathi))
	{
		NPCPhrase(THEN_ILWRATH);

		DisablePhrase(what_about_other_spathi);
	}
	else if (PlayerSaid(R, how_many_crew))
	{
		NPCPhrase(THOUSANDS);

		DisablePhrase(how_many_crew);
	}
	else if (PlayerSaid(R, really_thousands))
	{
		NPCPhrase(JUST_ME);

		DisablePhrase(really_thousands);
	}
	else if (PlayerSaid(R, full_of_monsters))
	{
		NPCPhrase(HOW_TRUE);

		DisablePhrase(full_of_monsters);
	}
	else if (PlayerSaid(R, what_enemy))
	{
		NPCPhrase(ENEMY_IS);

		DisablePhrase(what_enemy);
	}
	else if (PlayerSaid(R, why_you_here))
	{
		NPCPhrase(DREW_SHORT_STRAW);

		DisablePhrase(why_you_here);
	}
	else if (PlayerSaid(R, where_are_urquan))
	{
		NPCPhrase(URQUAN_LEFT);

		DisablePhrase(where_are_urquan);
	}
	else if (PlayerSaid(R, what_about_other_races))
	{
		NPCPhrase(ABOUT_OTHER_RACES);

		DisablePhrase(what_about_other_races);
	}
	else if (PlayerSaid(R, what_blaze_of_glory))
	{
		NPCPhrase(BLAZE_IS);

		DisablePhrase(what_blaze_of_glory);
	}
	else if (PlayerSaid(R, what_about_yourself))
	{
		NPCPhrase(ABOUT_MYSELF);

		DisablePhrase(what_about_yourself);
	}
	else if (PlayerSaid(R, join_us))
	{
		if (join_us_refusals == 0)
		{
			NPCPhrase(WONT_JOIN_1);
			++join_us_refusals;
		}
		else if (join_us_refusals == 1)
		{
			NPCPhrase(WONT_JOIN_2);
			++join_us_refusals;
		}
		else
		{
			NPCPhrase(WONT_JOIN_3);
		}
	}

	if (PhraseEnabled(what_doing_on_pluto_1))
	{
		Response(what_doing_on_pluto_1, SpathiOnPluto);
	}
	else if (PhraseEnabled(what_doing_on_pluto_2))
	{
		Response(what_doing_on_pluto_2, SpathiOnPluto);
	}
	else if (PhraseEnabled(what_doing_on_pluto_3))
	{
		Response(what_doing_on_pluto_3, SpathiOnPluto);
	}
	else
	{
		if (PhraseEnabled(what_about_ilwrath))
		{
			Response(what_about_ilwrath, SpathiOnPluto);
		}
		else if (PhraseEnabled(when_ilwrath))
		{
			Response(when_ilwrath, SpathiOnPluto);
		}

		if (PhraseEnabled(what_about_moonbase))
		{
			Response(what_about_moonbase, SpathiOnPluto);
		}
		else if (PhraseEnabled(what_about_other_spathi))
		{
			Response(what_about_other_spathi, SpathiOnPluto);
		}
		else
		{
			if (PhraseEnabled(how_many_crew))
			{
				Response(how_many_crew, SpathiOnPluto);
			}
			else if (PhraseEnabled(really_thousands))
			{
				Response(really_thousands, SpathiOnPluto);
			}
			else if (PhraseEnabled(full_of_monsters))
			{
				Response(full_of_monsters, SpathiOnPluto);
			}

			if (PhraseEnabled(what_enemy))
			{
				Response(what_enemy, SpathiOnPluto);
			}
			else if (PhraseEnabled(why_you_here))
			{
				Response(why_you_here, SpathiOnPluto);
			}
		}
	}
	if (PhraseEnabled(where_are_urquan))
	{
		Response(where_are_urquan, SpathiOnPluto);
	}
	else if (PhraseEnabled(what_about_other_races))
	{
		Response(what_about_other_races, SpathiOnPluto);
	}
	else if (PhraseEnabled(what_blaze_of_glory))
	{
		Response(what_blaze_of_glory, SpathiOnPluto);
	}
	else if (PhraseEnabled(what_about_yourself))
	{
		Response(what_about_yourself, SpathiOnPluto);
	}

	if (!PhraseEnabled(full_of_monsters))
	{
		Response(join_us, ExitConversation);
	}
	else
	{
		Response(join_us, SpathiOnPluto);
	}
	Response(changed_mind, ExitConversation);
}

static void SpathiMustGrovel(RESPONSE_REF R)
{
	if (PlayerSaid(R, identify))
	{
		NPCPhrase(I_FWIFFO);

		Response(do_cultural, SpathiMustGrovel);
		Response(youre_forgiven, SpathiOnPluto);
		Response(die_slugboy, ExitConversation);
	}
	else if (PlayerSaid(R, do_cultural))
	{
		NPCPhrase(WEZZY_WEZZAH);

		Response(begin_ritual, SpathiMustGrovel);
		Response(you_wont_die_yet, SpathiOnPluto);
		Response(we_fight_2, ExitConversation);
	}
	else if (PlayerSaid(R, begin_ritual))
	{
		NPCPhrase(MUST_DO_RITUAL_AT_HOME);

		Response(you_may_live, SpathiOnPluto);
		Response(pay_for_crimes, ExitConversation);
		Response(what_are_coordinates, SpathiMustGrovel);
	}
	else /* if (R == what_are_coordinates) */
	{
		NPCPhrase(COORDINATES_ARE);

		Response(youve_got_me_all_wrong, SpathiOnPluto);
		Response(tell_me_coordinates, ExitConversation);
	}
}

static void SpathiAllies(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INIT_ALLIED_HELLO_SPACE);
				break;
			case 1:
				NPCPhrase(SUBSEQUENT_ALLIED_HELLO_SPACE);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, whats_up_space_2))
	{
		NPCPhrase(GENERAL_INFO_SPACE_2);

		DisablePhrase(whats_up_space_2);
	}
	else if (PlayerSaid(R, give_us_info_from_space))
	{
		NPCPhrase(GET_INFO_FROM_SPATHIWA);

		DisablePhrase(give_us_info_from_space);
	}
	else if (PlayerSaid(R, give_us_resources_space))
	{
		NPCPhrase(GET_RESOURCES_FROM_SPATHIWA);

		DisablePhrase(give_us_resources_space);
	}
	else if (PlayerSaid(R, what_do_for_fun))
	{
		NPCPhrase(DO_THIS_FOR_FUN);

		DisablePhrase(what_do_for_fun);
	}

	if (PhraseEnabled(whats_up_space_2))
	{
		Response(whats_up_space_2, SpathiAllies);
	}
	if (PhraseEnabled(give_us_info_from_space))
	{
		Response(give_us_info_from_space, SpathiAllies);
	}
	if (PhraseEnabled(give_us_resources_space))
	{
		Response(give_us_resources_space, SpathiAllies);
	}
	if (PhraseEnabled(what_do_for_fun))
	{
		Response(what_do_for_fun, SpathiAllies);
	}
	if (getNomadMode() == uqm::NomadMode::Easy)
	{
		Response(join_us_nomad, ExitConversation);
	}
	Response(bye_ally_space, ExitConversation);
}

static void SpathiFriendly(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INIT_FRIENDLY_HELLO_SPACE);
				break;
			case 1:
				NPCPhrase(SUBSEQUENT_FRIENDLY_HELLO_SPACE);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, since_friendly_give_stuff))
	{
		NPCPhrase(GIVE_ADVICE);

		DisablePhrase(since_friendly_give_stuff);
	}
	else if (PlayerSaid(R, whats_up_space_1))
	{
		NPCPhrase(GENERAL_INFO_SPACE_1);

		DisablePhrase(whats_up_space_1);
	}

	if (PhraseEnabled(whats_up_space_1))
	{
		Response(whats_up_space_1, SpathiFriendly);
	}
	if (PhraseEnabled(since_friendly_give_stuff))
	{
		Response(since_friendly_give_stuff, SpathiFriendly);
	}
	Response(bye_friendly_space, ExitConversation);
}

static void SpathiNeutral(RESPONSE_REF R);

static void SpathiBefriend(RESPONSE_REF R)
{
	RESPONSE_REF pStr[2];

	uint8_t InfoLeft {false};
	uint8_t LastStack {0};
	pStr[0] = pStr[1] = 0;
	if (PlayerSaid(R, come_in_peace))
	{
		NPCPhrase(AGAINST_NATURE);
	}
	else if (PlayerSaid(R, looking_for_a_few_good_squids))
	{
		NPCPhrase(URQUAN_SLAVES);

		DisablePhrase(looking_for_a_few_good_squids);
	}
	else if (PlayerSaid(R, why_slaves))
	{
		NPCPhrase(UMGAH_TRICK);

		DisablePhrase(why_slaves);
	}
	else if (PlayerSaid(R, tell_us_about_you))
	{
		NPCPhrase(ABOUT_US);

		DisablePhrase(tell_us_about_you);
		LastStack = 1;
	}
	else if (PlayerSaid(R, what_you_really_want))
	{
		NPCPhrase(WANT_THIS);

		DisablePhrase(what_you_really_want);
	}
	else if (PlayerSaid(R, how_about_alliance))
	{
		NPCPhrase(SURE);

		DisablePhrase(how_about_alliance);
	}

	if (PhraseEnabled(looking_for_a_few_good_squids))
	{
		pStr[0] = looking_for_a_few_good_squids;
	}
	else if (PhraseEnabled(why_slaves))
	{
		pStr[0] = why_slaves;
	}
	if (PhraseEnabled(tell_us_about_you))
	{
		pStr[1] = tell_us_about_you;
	}
	else if (PhraseEnabled(what_you_really_want))
	{
		pStr[1] = what_you_really_want;
	}
	if (pStr[LastStack])
	{
		InfoLeft = true;
		Response(pStr[LastStack], SpathiBefriend);
	}
	LastStack ^= 1;
	if (pStr[LastStack])
	{
		InfoLeft = true;
		Response(pStr[LastStack], SpathiBefriend);
	}
	if (PhraseEnabled(how_about_alliance))
	{
		InfoLeft = true;
		Response(how_about_alliance, SpathiBefriend);
	}

	if (!InfoLeft)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK1, static_cast<uint32_t>(1));
		SpathiNeutral(R);
	}
}

static void SpathiAntagonize(RESPONSE_REF R)
{
	if (PlayerSaid(R, prepare_to_die))
	{
		NPCPhrase(ALWAYS_PREPARED);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, heard_youre_cowards))
	{
		NPCPhrase(DARN_TOOTIN);

		DisablePhrase(heard_youre_cowards);
	}
	else if (PlayerSaid(R, wanna_fight))
	{
		NPCPhrase(YES_WE_DO);

		DisablePhrase(wanna_fight);
	}
	else if (PlayerSaid(R, so_lets_fight))
	{
		NPCPhrase(OK_LETS_FIGHT);

		DisablePhrase(so_lets_fight);
	}
	else if (PlayerSaid(R, so_lets_fight_already))
	{
		NPCPhrase(DONT_REALLY_WANT_TO_FIGHT);

		DisablePhrase(so_lets_fight_already);
	}

	if (PhraseEnabled(wanna_fight))
	{
		Response(wanna_fight, SpathiAntagonize);
	}
	else if (PhraseEnabled(so_lets_fight))
	{
		Response(so_lets_fight, SpathiAntagonize);
	}
	else if (PhraseEnabled(so_lets_fight_already))
	{
		Response(so_lets_fight_already, SpathiAntagonize);
	}
	if (PhraseEnabled(heard_youre_cowards))
	{
		Response(heard_youre_cowards, SpathiAntagonize);
	}
	Response(attack_you_now, ExitConversation);
}

static void SpathiNeutral(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INIT_NEUTRAL_HELLO_SPACE);
				break;
			case 1:
				NPCPhrase(SUBSEQUENT_NEUTRAL_HELLO_SPACE);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, look_weird))
	{
		NPCPhrase(YOU_LOOK_WEIRD);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK0, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, no_look_really_weird))
	{
		NPCPhrase(NO_YOU_LOOK_REALLY_WEIRD);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_STACK0, static_cast<uint32_t>(2));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SPATHI_STACK0))
	{
		case 0:
			Response(look_weird, SpathiNeutral);
			break;
		case 1:
			Response(no_look_really_weird, SpathiNeutral);
			break;
	}
	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_STACK1) == 0)
	{
		Response(come_in_peace, SpathiBefriend);
	}
	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_STACK2) == 0)
	{
		Response(prepare_to_die, SpathiAntagonize);
	}
	else
	{
		Response(attack_you_now, ExitConversation);
	}
	Response(part_in_peace, ExitConversation);
}

static void Intro(void)
{
	uint8_t Manner {};

	Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER);
	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1)
	{
		join_us_refusals = 0;

		NPCPhrase(SORRY_ABOUT_THAT);

		/* if already know password from Melnorme,
			 * but haven't visited Spathiwa yet . . .
			 */
		if (uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) == 7)
		{
			uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_PASSWORD, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		}
		SetHomeworldKnown(SPATHI_HOME);

		Response(identify, SpathiMustGrovel);
		Response(hi_there, SpathiOnPluto);
		Response(dont_kill, SpathiOnPluto);
		Response(we_fight_1, ExitConversation);
	}
	else if (Manner == 2)
	{
		NPCPhrase(HATE_YOU_FOREVER_SPACE);
		setSegue(Segue::Hostile);
	}
	else if (Manner == 1)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INIT_ANGRY_HELLO_SPACE);
				break;
			case 1:
				NPCPhrase(SUBSEQUENT_ANGRY_HELLO_SPACE);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(NumVisits));

		Response(give_info_space, ExitConversation);
		Response(we_sorry_space, ExitConversation);
		Response(we_fight_again_space, ExitConversation);
		Response(bye_angry_space, ExitConversation);
	}
	else if (CheckAlliance(SPATHI_SHIP) == RaceAllegiance::Good)
	{
		SpathiAllies((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_QUEST))
	{
		SpathiFriendly((RESPONSE_REF)0);
	}
	else
	{
		SpathiNeutral((RESPONSE_REF)0);
	}
}

static uint16_t uninit_spathi(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_spathi_enc(void)
{
	uint8_t Manner {};

	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1)
	{
		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(2));
	}
	else if (getSegue() == Segue::Hostile
			 && (Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			/* if don't know about Spathi via Melnorme . . . */
			if (uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) != 7)
			{
				uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
			}
		}
	}
}

ConversationData* init_spathi_comm(void)
{
	spathi_desc.init_encounter_func = Intro;
	spathi_desc.post_encounter_func = post_spathi_enc;
	spathi_desc.uninit_encounter_func = uninit_spathi;

	uqm::lua().beginComm(nullptr, NullResource);

	spathi_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	spathi_desc.AlienTextBaseline.y = 0;
	spathi_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	// use alternate Fwiffo track if available
	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1)
	{
		altResFlags |= UseAltSong;
	}

	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1
		|| uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER) == 3
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&spathi_desc};

	return (retval);
}
