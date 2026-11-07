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


static ConversationData supox_desc =
	{
		SUPOX_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SupoxPmapAnim, /* AlienFrame */
		SupoxFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SupoxColorMap, /* AlienColorMap */
		SupoxMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		SupoxConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				4, /* StartIndex */
				5, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				9,			  /* StartIndex */
				10,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				19,			  /* StartIndex */
				10,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				29,			  /* StartIndex */
				13,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
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

	if (PlayerSaid(R, bye_neutral))
	{
		NPCPhrase(GOODBYE_NEUTRAL);
	}
	else if (PlayerSaid(R, what_do_i_do_now))
	{
		NPCPhrase(FIX_IT);
	}
	else if (PlayerSaid(R, thanks_now_we_eat_you))
	{
		NPCPhrase(HIDEOUS_MONSTERS);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOSTILE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, bye_after_space))
	{
		NPCPhrase(GOODBYE_AFTER_SPACE);
	}
	else if (PlayerSaid(R, bye_before_space))
	{
		NPCPhrase(GOODBYE_BEFORE_SPACE);
	}
	else if (PlayerSaid(R, bye_allied_homeworld))
	{
		NPCPhrase(GOODBYE_ALLIED_HOMEWORLD);
	}
	else if (PlayerSaid(R, can_you_help))
	{
		NPCPhrase(HOW_HELP);
		if (EscortFeasibilityStudy(SUPOX_SHIP) == 0)
		{
			NPCPhrase(DONT_NEED);
		}
		else
		{
			NPCPhrase(HAVE_4_SHIPS);

			AlienTalkSegue((uint16_t)~0);
			AddEscortShips(SUPOX_SHIP, difficultyCase(4, 4, 2));
			PrepareShip(SUPOX_SHIP);
		}
	}
}

static void AlliedHome(RESPONSE_REF R);

static void AlliedHome(RESPONSE_REF R)
{
	uint8_t NumVisits {};
	uint8_t News {};

	News = uqm::gameState().flag(uqm::GameFlag::SUPOX_WAR_NEWS);
	NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
	if (PlayerSaid(R, how_went_war))
	{
		NPCPhrase(ABOUT_BATTLE);

		News |= (1 << 0);
	}
	else if (PlayerSaid(R, how_goes_war))
	{
		if (NumVisits == 1)
		{
			NPCPhrase(FLEET_ON_WAY);

			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(1));
		}
		else
		{
			switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_WAR_NEWS))
			{
				case 0:
					NPCPhrase(BATTLE_HAPPENS_1);
					News = 1;
					break;
				case 1:
					NPCPhrase(BATTLE_HAPPENS_2);
					News = 2;
					break;
			}
		}

		DisablePhrase(how_goes_war);
	}
	else if (PlayerSaid(R, learn_new_info))
	{
		if (NumVisits < 5)
		{
			NPCPhrase(NO_NEW_INFO);
		}
		else
		{
			NPCPhrase(SAMATRA);

			News |= (1 << 1);
		}

		DisablePhrase(learn_new_info);
	}
	else if (PlayerSaid(R, what_now_homeworld))
	{
		if (NumVisits < 5)
		{
			NPCPhrase(UP_TO_YOU);
		}
		else
		{
			NPCPhrase(HOPE_KILL_EACH_OTHER);
		}

		DisablePhrase(what_now_homeworld);
	}
	else if (PlayerSaid(R, how_is_ultron))
	{
		NPCPhrase(ULTRON_IS_GREAT);

		DisablePhrase(how_is_ultron);
	}
	else if (IsExtended() && PlayerSaid(R, give_info))
	{
		NPCPhrase(GOOD_HINTS);

		DisablePhrase(give_info);
	}
	uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(News));

	if (NumVisits >= 5)
	{
		if (!(News & (1 << 0)))
		{
			Response(how_went_war, AlliedHome);
		}
	}
	else if (PhraseEnabled(how_goes_war)
			 && ((NumVisits == 1 && News == 0)
				 || (NumVisits && News < 2)))
	{
		Response(how_goes_war, AlliedHome);
	}
	if (PhraseEnabled(learn_new_info))
	{
		Response(learn_new_info, AlliedHome);
	}
	if (PhraseEnabled(what_now_homeworld))
	{
		Response(what_now_homeworld, AlliedHome);
	}
	if (PhraseEnabled(how_is_ultron))
	{
		Response(how_is_ultron, AlliedHome);
	}
	if (IsExtended() && PhraseEnabled(give_info))
	{
		Response(give_info, AlliedHome);
	}
	if (IsExtended() && ShipsReady(SUPOX_SHIP))
	{
		Response(can_you_help, ExitConversation);
	}
	Response(bye_allied_homeworld, ExitConversation);
}

static void BeforeKohrAh(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, whats_up_before_space))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_BEFORE_SPACE_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_BEFORE_SPACE_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_before_space);
	}
	else if (PlayerSaid(R, what_now_before_space))
	{
		NPCPhrase(DO_THIS_BEFORE_SPACE);

		DisablePhrase(what_now_before_space);
	}

	if (PhraseEnabled(whats_up_before_space))
	{
		Response(whats_up_before_space, BeforeKohrAh);
	}
	if (PhraseEnabled(what_now_before_space))
	{
		Response(what_now_before_space, BeforeKohrAh);
	}
	Response(bye_before_space, ExitConversation);
}

static void AfterKohrAh(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, whats_up_after_space))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_AFTER_SPACE_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_AFTER_SPACE_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_after_space);
	}
	else if (PlayerSaid(R, what_now_after_space))
	{
		NPCPhrase(DO_THIS_AFTER_SPACE);

		DisablePhrase(what_now_after_space);
	}

	if (PhraseEnabled(whats_up_after_space))
	{
		Response(whats_up_after_space, AfterKohrAh);
	}
	if (PhraseEnabled(what_now_after_space))
	{
		Response(what_now_after_space, AfterKohrAh);
	}
	Response(bye_after_space, ExitConversation);
}

static void NeutralSupox(RESPONSE_REF R)
{
	uint8_t i {};
	uint8_t NumVisits {};
	RESPONSE_REF pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PlayerSaid(R, i_am))
	{
		NPCPhrase(WE_ARE_SUPOX);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(1));
		DisablePhrase(i_am);
	}
	else if (PlayerSaid(R, my_ship))
	{
		NPCPhrase(OUR_SHIP);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(2));
		DisablePhrase(my_ship);
	}
	else if (PlayerSaid(R, from_alliance))
	{
		NPCPhrase(FROM_SUPOX);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(3));
		DisablePhrase(from_alliance);
	}
	else if (PlayerSaid(R, are_you_copying))
	{
		NPCPhrase(YEAH_SORRY);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, why_copy))
	{
		NPCPhrase(SYMBIOTS);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK1, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, tell_us_of_your_species))
	{
		NPCPhrase(OUR_SPECIES);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, plants_arent_intelligent))
	{
		NPCPhrase(PROVES_WERE_SPECIAL);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_STACK2, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, anyone_around_here))
	{
		NPCPhrase(UTWIG_NEARBY);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(1));
		StartSphereTracking(UTWIG_SHIP);
	}
	else if (PlayerSaid(R, what_relation_to_utwig))
	{
		NPCPhrase(UTWIG_ALLIES);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, whats_wrong_with_utwig))
	{
		NPCPhrase(BROKE_ULTRON);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, whats_ultron))
	{
		NPCPhrase(TAKE_ULTRON);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_WAR_NEWS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(1));

		Response(what_do_i_do_now, (IsExtended() ? NeutralSupox : ExitConversation));
		Response(thanks_now_we_eat_you, ExitConversation);

		return;
	}
	else if (PlayerSaid(R, what_do_i_do_now))
	{
		NPCPhrase(FIX_IT);
	}
	else if (PlayerSaid(R, got_fixed_ultron))
	{
		NPCPhrase(GOOD_GIVE_TO_UTWIG);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, look_i_repaired_lots))
	{
		NPCPhrase(ALMOST_THERE);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, look_i_slightly_repaired))
	{
		NPCPhrase(GREAT_DO_MORE);

		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, where_get_repairs))
	{
		NPCPhrase(ANCIENT_RHYME);

		if (IsExtended())
		{
			StartSphereTracking(DRUUGE_SHIP);
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, give_info))
	{
		NPCPhrase(GOOD_HINTS);

		DisablePhrase(give_info);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_STACK2))
	{
		case 0:
			pStr[1] = tell_us_of_your_species;
			break;
		case 1:
			pStr[1] = plants_arent_intelligent;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_STACK1))
	{
		case 0:
			pStr[0] = i_am;
			pStr[1] = 0;
			break;
		case 1:
			pStr[0] = my_ship;
			pStr[1] = 0;
			break;
		case 2:
			pStr[0] = from_alliance;
			pStr[1] = 0;
			break;
		case 3:
			pStr[0] = are_you_copying;
			pStr[1] = 0;
			break;
		case 4:
			pStr[0] = why_copy;
			pStr[1] = 0;
			break;
	}
	NumVisits = uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION);
	if (NumVisits == 0)
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SUPOX_WAR_NEWS))
		{
			case 0:
				if (uqm::gameState().flag(uqm::GameFlag::UTWIG_VISITS)
					|| uqm::gameState().flag(uqm::GameFlag::UTWIG_HOME_VISITS)
					|| uqm::gameState().flag(uqm::GameFlag::BOMB_VISITS))
				{
					pStr[2] = what_relation_to_utwig;
				}
				else
				{
					pStr[2] = anyone_around_here;
				}
				break;
			case 1:
				pStr[2] = whats_wrong_with_utwig;
				break;
			case 2:
				pStr[2] = whats_ultron;
				break;
		}
	}
	if (pStr[LastStack])
	{
		Response(pStr[LastStack], NeutralSupox);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			Response(pStr[i], NeutralSupox);
		}
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SUPOX_ULTRON_HELP))
	{
		switch (NumVisits)
		{
			case 1:
				Response(where_get_repairs, NeutralSupox);
				break;
			case 2:
				Response(look_i_slightly_repaired, NeutralSupox);
				break;
			case 3:
				Response(look_i_repaired_lots, NeutralSupox);
				break;
			case 4:
				Response(got_fixed_ultron, NeutralSupox);
				break;
		}
	}
	if (IsExtended() && NumVisits && PhraseEnabled(give_info))
	{
		Response(give_info, NeutralSupox);
	}
	Response(bye_neutral, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::SUPOX_HOSTILE))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HOSTILE_SPACE_HELLO_1);
				break;
			case 1:
				NPCPhrase(HOSTILE_SPACE_HELLO_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Peace);
	}
	else if (CheckAlliance(SUPOX_SHIP) == RaceAllegiance::Good)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ALLIED_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase(ALLIED_HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase(ALLIED_HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase(ALLIED_HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			AlliedHome((RESPONSE_REF)0);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
			if (NumVisits == 1)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(HELLO_BEFORE_KOHRAH_SPACE_1);
						break;
					case 1:
						NPCPhrase(HELLO_BEFORE_KOHRAH_SPACE_2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));

				BeforeKohrAh((RESPONSE_REF)0);
			}
			else if (NumVisits < 5)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(HELLO_DURING_KOHRAH_SPACE_1);
						break;
					case 1:
						NPCPhrase(HELLO_DURING_KOHRAH_SPACE_2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(HELLO_AFTER_KOHRAH_SPACE_1);
						break;
					case 1:
						NPCPhrase(HELLO_AFTER_KOHRAH_SPACE_2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));

				AfterKohrAh((RESPONSE_REF)0);
			}
		}
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(NEUTRAL_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase(NEUTRAL_HOMEWORLD_HELLO_2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			SetHomeworldKnown(SUPOX_HOME);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SUPOX_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(NEUTRAL_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(NEUTRAL_SPACE_HELLO_2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SUPOX_VISITS, static_cast<uint32_t>(NumVisits));
		}

		NeutralSupox((RESPONSE_REF)0);
	}
}

static uint16_t uninit_supox(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_supox_enc(void)
{
	// nothing defined so far
}

ConversationData* init_supox_comm(void)
{
	supox_desc.init_encounter_func = Intro;
	supox_desc.post_encounter_func = post_supox_enc;
	supox_desc.uninit_encounter_func = uninit_supox;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	supox_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	supox_desc.AlienTextBaseline.y = 0;
	supox_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (!uqm::gameState().flag(uqm::GameFlag::SUPOX_HOSTILE)
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&supox_desc};

	return (retval);
}
