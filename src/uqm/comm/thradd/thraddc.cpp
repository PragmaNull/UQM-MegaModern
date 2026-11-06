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
#include "../../../options.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"
#include "uqm/gameev.h"


static ConversationData thradd_desc =
	{
		THRADD_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		ThraddPmapAnim, /* AlienFrame */
		ThraddFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		ThraddColorMap, /* AlienColorMap */
		ThraddMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		ThraddConversationPhrases, /* PlayerPhrases */
		8, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				8,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 4),				 /* BlockMask */
			},
								{
				12,			  /* StartIndex */
				9,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				21,			/* StartIndex */
				6,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				27,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4),				/* BlockMask */
			},
								{
				30, /* StartIndex */
				12, /* NumFrames */
				CircularAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 12,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond, /* RestartRate */
				(1 << 0) | (1 << 3) | (1 << 5),
			},
								{
				42,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4) | (1 << 6),	/* BlockMask */
			},
								{
				47,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
								{
				52,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0, /* RestartRate */
				0, /* BlockMask */
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
			7, /* NumFrames */
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

static int GetCultureName(void)
{
	int culture = 0;

	switch (uqm::gameState().flag(uqm::GameFlag::THRADD_CULTURE))
	{
		case 1:
			culture = CULTURE;
			break;
		case 2:
			culture = FAT_JERKS;
			break;
		case 3:
			culture = SLAVE_EMPIRE;
			break;
		default:
			assert(0 && "Unknown culture");
	}

	return (culture);
}

static void PolitePhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(HELLO_POLITE_1);
			break;
		case 1:
			NPCPhrase(HELLO_POLITE_2);
			break;
		case 2:
			NPCPhrase(HELLO_POLITE_3);
			break;
		case 3:
			NPCPhrase(HELLO_POLITE_4);
			break;
	}
}

static void RhymePhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(HELLO_RHYME_1);
			break;
		case 1:
			NPCPhrase(HELLO_RHYME_2);
			break;
		case 2:
			NPCPhrase(HELLO_RHYME_3);
			break;
		case 3:
			NPCPhrase(HELLO_RHYME_4);
			break;
	}
}

static void PigLatinPhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(HELLO_PIG_LATIN_1);
			break;
		case 1:
			NPCPhrase(HELLO_PIG_LATIN_2);
			break;
		case 2:
			NPCPhrase(HELLO_PIG_LATIN_3);
			break;
		case 3:
			NPCPhrase(HELLO_PIG_LATIN_4);
			break;
	}
}

static void LikeYouPhrase(uint8_t which_phrase)
{
	switch (which_phrase)
	{
		case 0:
			NPCPhrase(HELLO_LIKE_YOU_1);
			break;
		case 1:
			NPCPhrase(HELLO_LIKE_YOU_2);
			break;
		case 2:
			NPCPhrase(HELLO_LIKE_YOU_3);
			break;
		case 3:
			NPCPhrase(HELLO_LIKE_YOU_4);
			break;
	}
}

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue_hostile);

	if (PlayerSaid(R, bye_hostile_2))
	{
		NPCPhrase(GOODBYE_HOSTILE_2);
	}
	else if (PlayerSaid(R, bye_hostile_1))
	{
		NPCPhrase(GOODBYE_HOSTILE_1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_5, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, submit_1))
	{
		NPCPhrase(NO_SUBMIT_1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, submit_2))
	{
		NPCPhrase(NO_SUBMIT_2);
	}
	else if (PlayerSaid(R, got_idea))
	{
		NPCPhrase(GOOD_IDEA);

		setSegue(Segue_peace);
		AddEvent(RELATIVE_EVENT, 0, 0, 0, ADVANCE_THRADD_MISSION);
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, bye_hostile_helix))
	{
		NPCPhrase(GOODBYE_HOSTILE_HELIX);
	}
	else if (PlayerSaid(R, bye_ally))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_STACK_1);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GOODBYE_ALLY_1);
				break;
			case 1:
				NPCPhrase(GOODBYE_ALLY_2);
				break;
			case 2:
				NPCPhrase(GOODBYE_ALLY_3);
				break;
			case 3:
				NPCPhrase(GOODBYE_ALLY_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(NumVisits));
		setSegue(Segue_peace);
	}
	else if (PlayerSaid(R, may_i_land))
	{
		NPCPhrase(SURE_LAND);

		uqm::gameState().setFlag(uqm::GameFlag::HELIX_UNPROTECTED, static_cast<uint32_t>(1));
		setSegue(Segue_peace);
	}
	else if (PlayerSaid(R, demand_to_land))
	{
		NPCPhrase(NO_DEMAND);
	}
	else if (PlayerSaid(R, i_need_to_land_lie))
	{
		NPCPhrase(CAUGHT_LIE);
	}
	else
	{
		if (PlayerSaid(R, contemplative))
		{
			NPCPhrase(OK_CONTEMPLATIVE);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(0));
		}
		else if (PlayerSaid(R, friendly))
		{
			NPCPhrase(OK_FRIENDLY);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, wacky))
		{
			NPCPhrase(OK_WACKY);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(2));
		}
		else if (PlayerSaid(R, just_like_us))
		{
			NPCPhrase(OK_JUST_LIKE_YOU);

			uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(3));
		}
		NPCPhrase(WORK_TO_DO);

		setSegue(Segue_peace);
	}
}

static void ThraddAllies(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, why_you_here_ally))
	{
		NPCPhrase(GUARDING_HELIX_ALLY);

		DisablePhrase(why_you_here_ally);
	}
	else if (PlayerSaid(R, whats_helix_ally))
	{
		NPCPhrase(HELIX_IS_ALLY);

		DisablePhrase(whats_helix_ally);
	}
	else if (PlayerSaid(R, whats_up_ally))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_ALLY_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_ALLY_2);
				break;
			case 2:
				NPCPhrase(GENERAL_INFO_ALLY_3);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_ALLY_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_ally);
	}
	else if (PlayerSaid(R, how_goes_culture))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_DEMEANOR);
		switch (NumVisits & ((1 << 2) - 1))
		{
			case 0:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(CONTEMP_GOES_1);
				}
				else
				{
					NPCPhrase(CONTEMP_GOES_2);
				}
				break;
			case 1:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(FRIENDLY_GOES_1);
				}
				else
				{
					NPCPhrase(FRIENDLY_GOES_2);
				}
				break;
			case 2:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(WACKY_GOES_1);
				}
				else
				{
					NPCPhrase(WACKY_GOES_2);
				}
				break;
			case 3:
				if (!(NumVisits & ~((1 << 2) - 1)))
				{
					NPCPhrase(LIKE_YOU_GOES_1);
				}
				else
				{
					NPCPhrase(LIKE_YOU_GOES_2);
				}
				break;
		}
		NumVisits |= 1 << 2;
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(NumVisits));

		DisablePhrase(how_goes_culture);
	}

	if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		if (PhraseEnabled(why_you_here_ally))
		{
			Response(why_you_here_ally, ThraddAllies);
		}
		else
		{
			if (PhraseEnabled(whats_helix_ally))
			{
				Response(whats_helix_ally, ThraddAllies);
			}
			Response(may_i_land, ExitConversation);
		}
	}
	if (PhraseEnabled(whats_up_ally))
	{
		Response(whats_up_ally, ThraddAllies);
	}
	if (PhraseEnabled(how_goes_culture))
	{
		Response(how_goes_culture, ThraddAllies);
	}
	Response(bye_ally, ExitConversation);
}

static void ThraddDemeanor(RESPONSE_REF R)
{
	if (PlayerSaid(R, you_decide))
	{
		NPCPhrase(OK_CULTURE_20);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, fat))
	{
		NPCPhrase(OK_FAT);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, the_slave_empire))
	{
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(3));

		NPCPhrase(OK_SLAVE);
	}

	NPCPhrase(HOW_SHOULD_WE_ACT);
	Response(contemplative, ExitConversation);
	Response(friendly, ExitConversation);
	Response(wacky, ExitConversation);
	Response(just_like_us, ExitConversation);
}

static void ThraddCulture(RESPONSE_REF R)
{
	if (PlayerSaid(R, be_polite))
	{
		NPCPhrase(OK_POLITE);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, use_rhymes))
	{
		NPCPhrase(OK_RHYMES);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, speak_pig_latin))
	{
		NPCPhrase(OK_PIG_LATIN);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, just_the_way_we_do))
	{
		NPCPhrase(OK_WAY_YOU_DO);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(3));
	}
	NPCPhrase(WHAT_NAME_FOR_CULTURE);

	Response(you_decide, ThraddDemeanor);
	Response(fat, ThraddDemeanor);
	Response(the_slave_empire, ThraddDemeanor);
}

static void ThraddWorship(RESPONSE_REF R)
{
	(void)R; // ignored
	uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(0));
	uqm::gameState().setFlag(uqm::GameFlag::THRADD_MANNER, static_cast<uint32_t>(1));
	uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(0));
	SetRaceAllied(THRADDASH_SHIP, true);

	Response(be_polite, ThraddCulture);
	Response(speak_pig_latin, ThraddCulture);
	Response(use_rhymes, ThraddCulture);
	Response(just_the_way_we_do, ThraddCulture);
}

static void HelixWorld(RESPONSE_REF R)
{
	if (PlayerSaid(R, why_you_here_hostile))
	{
		NPCPhrase(NONE_OF_YOUR_CONCERN);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_CULTURE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, what_about_this_world))
	{
		NPCPhrase(BLUE_HELIX);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, whats_helix_hostile))
	{
		NPCPhrase(HELIX_IS_HOSTILE);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INTRO, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, i_need_to_land_lie))
	{
		NPCPhrase(CAUGHT_LIE);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_DEMEANOR, static_cast<uint32_t>(1));
	}

	if (!uqm::gameState().flag(uqm::GameFlag::THRADD_CULTURE))
	{
		Response(why_you_here_hostile, HelixWorld);
	}
	else
	{
		Response(demand_to_land, ExitConversation);
	}
	switch (uqm::gameState().flag(uqm::GameFlag::THRADD_INTRO))
	{
		case 0:
			Response(what_about_this_world, HelixWorld);
			break;
		case 1:
			Response(whats_helix_hostile, HelixWorld);
			break;
	}
	if (!uqm::gameState().flag(uqm::GameFlag::THRADD_DEMEANOR))
	{
		Response(i_need_to_land_lie, ExitConversation);
	}
	Response(bye_hostile_helix, ExitConversation);
}

static void ThraddHostile(RESPONSE_REF R)
{
	if (PlayerSaid(R, whats_up_hostile_1))
	{
		NPCPhrase(GENERAL_INFO_HOSTILE_1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INFO, static_cast<uint32_t>(1));
		DisablePhrase(whats_up_hostile_2);
	}
	else if (PlayerSaid(R, whats_up_hostile_2))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_INFO);
		switch (NumVisits++)
		{
			case 1:
				NPCPhrase(GENERAL_INFO_HOSTILE_2);
				break;
			case 2:
				NPCPhrase(GENERAL_INFO_HOSTILE_3);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_HOSTILE_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::THRADD_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_hostile_2);
	}
	else if (PlayerSaid(R, what_about_you_1))
	{
		NPCPhrase(ABOUT_US_1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, what_about_you_2))
	{
		NPCPhrase(ABOUT_US_2);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, what_about_urquan_1))
	{
		NPCPhrase(ABOUT_URQUAN_1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, what_about_urquan_2))
	{
		NPCPhrase(ABOUT_URQUAN_2);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, be_friends_1))
	{
		NPCPhrase(NO_FRIENDS_1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, be_friends_2))
	{
		NPCPhrase(NO_FRIENDS_2);
		DisablePhrase(be_friends_2);
	}
	else if (PlayerSaid(R, how_impressed_urquan_1))
	{
		NPCPhrase(IMPRESSED_LIKE_SO_1);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOSTILE_STACK_4, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, how_impressed_urquan_2))
	{
		NPCPhrase(IMPRESSED_LIKE_SO_2);

		uqm::gameState().setFlag(uqm::GameFlag::THRADD_MISSION, static_cast<uint32_t>(5));
	}

	if (uqm::gameState().flag(uqm::GameFlag::THRADD_INFO) == 0)
	{
		Response(whats_up_hostile_1, ThraddHostile);
	}
	else if (PhraseEnabled(whats_up_hostile_2))
	{
		Response(whats_up_hostile_2, ThraddHostile);
	}
	switch (uqm::gameState().flag(uqm::GameFlag::THRADD_STACK_1))
	{
		case 0:
			Response(what_about_you_1, ThraddHostile);
			break;
		case 1:
			Response(what_about_you_2, ThraddHostile);
			break;
		case 2:
			Response(what_about_urquan_1, ThraddHostile);
			break;
		case 3:
			Response(what_about_urquan_2, ThraddHostile);
			break;
		case 4:
			if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
			{
				Response(got_idea, ExitConversation);
			}
			else
			{
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_STACK_1, static_cast<uint32_t>(5));
			}
			break;
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_2) == 0)
	{
		Response(submit_1, ExitConversation);
	}
	else
	{
		Response(submit_2, ExitConversation);
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_3) == 0)
	{
		Response(be_friends_1, ThraddHostile);
	}
	else if (PhraseEnabled(be_friends_2))
	{
		Response(be_friends_2, ThraddHostile);
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION) == 4)
	{
		if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_4) == 0)
		{
			Response(how_impressed_urquan_1, ThraddHostile);
		}
		else
		{
			Response(how_impressed_urquan_2, ThraddHostile);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::THRADD_HOSTILE_STACK_5) == 0)
	{
		Response(bye_hostile_1, ExitConversation);
	}
	else
	{
		Response(bye_hostile_2, ExitConversation);
	}
}

static void Intro(void)
{
	uint8_t NumVisits {};
	HFLEETINFO hThradd = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, THRADDASH_SHIP);
	const FleetInfo* ThraddPtr = LockFleetInfo(&uqm::gameState().game().avail_race_q, hThradd);

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue_peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX) && !(IsExtended() && ThraddPtr->allied_state == GOOD_GUY))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::HELIX_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(DIE_THIEF_1);
				break;
			case 1:
				NPCPhrase(DIE_THIEF_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::HELIX_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue_hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
	{
		if (isDifficulty(uqm::Difficulty::Hard) && uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			RESPONSE_REF pStr0 {};
			RESPONSE_REF pStr1 {};

			if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER))
			{
				pStr0 = WELCOME_HELIX0;
				pStr1 = WELCOME_HELIX1;

				NPCPhrase(pStr0);
				NPCPhrase_splice(GetCultureName());
				NPCPhrase(pStr1);

				ThraddAllies((RESPONSE_REF)0);
			}
			else
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::HELIX_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(HOSTILE_HELIX_HELLO_1);
						break;
					case 1:
						NPCPhrase(HOSTILE_HELIX_HELLO_2);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::HELIX_VISITS, static_cast<uint32_t>(NumVisits));

				HelixWorld((RESPONSE_REF)0);
			}
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_VISITS);
			NumVisits = NumVisits > 1 ? 0 : NumVisits;

			if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER))
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(HAVING_FUN_WITH_ILWRATH_1);
						break;
					case 1:
						NPCPhrase(HAVING_FUN_WITH_ILWRATH_2);
						--NumVisits;
						break;
				}
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(GO_AWAY_FIGHTING_ILWRATH_1);
						break;
					case 1:
						NPCPhrase(GO_AWAY_FIGHTING_ILWRATH_2);
						--NumVisits;
						break;
				}
			}
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(NumVisits));

			setSegue(Segue_peace);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER))
	{
		RESPONSE_REF pStr0 {};
		RESPONSE_REF pStr1 {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_VISITS);
		switch (uqm::gameState().flag(uqm::GameFlag::THRADD_INTRO))
		{
			case 0:
				PolitePhrase(NumVisits);
				break;
			case 1:
				RhymePhrase(NumVisits);
				break;
			case 2:
				PigLatinPhrase(NumVisits);
				break;
			case 3:
				LikeYouPhrase(NumVisits);
				break;
		}
		if (++NumVisits < 4)
		{
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(NumVisits));
		}

		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			pStr0 = WELCOME_HELIX0;
			pStr1 = WELCOME_HELIX1;
		}
		else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			pStr0 = WELCOME_HOMEWORLD0;
			pStr1 = WELCOME_HOMEWORLD1;
			SetHomeworldKnown(THRADDASH_HOME);
		}
		else
		{
			pStr0 = WELCOME_SPACE0;
			pStr1 = WELCOME_SPACE1;
		}
		NPCPhrase(pStr0);
		NPCPhrase_splice(GetCultureName());
		NPCPhrase(pStr1);

		ThraddAllies((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::HELIX_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HOSTILE_HELIX_HELLO_1);
				break;
			case 1:
				NPCPhrase(HOSTILE_HELIX_HELLO_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::HELIX_VISITS, static_cast<uint32_t>(NumVisits));

		HelixWorld((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::THRADDASH_BODY_COUNT)
			 >= (uint8_t)THRADDASH_BODY_THRESHOLD)
	{
		NPCPhrase(AMAZING_PERFORMANCE);

		ThraddWorship((RESPONSE_REF)0);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADDASH_BODY_COUNT);
		if (NumVisits >= 16
			&& uqm::gameState().flag(uqm::GameFlag::THRADD_BODY_LEVEL) == 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_BODY_LEVEL, static_cast<uint32_t>(2));
			NPCPhrase(IMPRESSIVE_PERFORMANCE);
		}
		else if (NumVisits >= 8
				 && uqm::gameState().flag(uqm::GameFlag::THRADD_BODY_LEVEL) == 0)
		{
			uqm::gameState().setFlag(uqm::GameFlag::THRADD_BODY_LEVEL, static_cast<uint32_t>(1));
			NPCPhrase(ADEQUATE_PERFORMANCE);
		}

		{
			if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_HOME_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(HOSTILE_HOMEWORLD_HELLO_1);
						break;
					case 1:
						NPCPhrase(HOSTILE_HOMEWORLD_HELLO_2);
						break;
					case 2:
						NPCPhrase(HOSTILE_HOMEWORLD_HELLO_3);
						break;
					case 3:
						NPCPhrase(HOSTILE_HOMEWORLD_HELLO_4);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_HOME_VISITS, static_cast<uint32_t>(NumVisits));
				SetHomeworldKnown(THRADDASH_HOME);
			}
			else if ((NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION)) == 0
					 || NumVisits > 3)
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::THRADD_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(HOSTILE_SPACE_HELLO_1);
						break;
					case 1:
						NPCPhrase(HOSTILE_SPACE_HELLO_2);
						break;
					case 2:
						NPCPhrase(HOSTILE_SPACE_HELLO_3);
						break;
					case 3:
						NPCPhrase(HOSTILE_SPACE_HELLO_4);
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_VISITS, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				switch (NumVisits)
				{
					case 1:
						if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION_VISITS) == 0)
						{
							NPCPhrase(WE_GO_TO_IMPRESS_URQUAN_1);
						}
						else
						{
							NPCPhrase(WE_GO_TO_IMPRESS_URQUAN_2);
						}
						break;
					case 2:
						if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION_VISITS) == 0)
						{
							NPCPhrase(WE_IMPRESSING_URQUAN_1);
						}
						else
						{
							NPCPhrase(WE_IMPRESSING_URQUAN_2);
						}
						break;
					case 3:
						if (uqm::gameState().flag(uqm::GameFlag::THRADD_MISSION_VISITS) == 0)
						{
							NPCPhrase(WE_IMPRESSED_URQUAN_1);
						}
						else
						{
							NPCPhrase(WE_IMPRESSED_URQUAN_2);
						}
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::THRADD_MISSION_VISITS, static_cast<uint32_t>(1));
			}

			ThraddHostile((RESPONSE_REF)0);
		}
	}
}

static uint16_t uninit_thradd(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_thradd_enc(void)
{
	// nothing defined so far
}

ConversationData* init_thradd_comm(void)
{
	thradd_desc.init_encounter_func = Intro;
	thradd_desc.post_encounter_func = post_thradd_enc;
	thradd_desc.uninit_encounter_func = uninit_thradd;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	thradd_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	thradd_desc.AlienTextBaseline.y = 0;
	thradd_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER)
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		setSegue(Segue_peace);
	}
	else
	{
		setSegue(Segue_hostile);
	}
	ConversationData* retval {&thradd_desc};

	return (retval);
}
