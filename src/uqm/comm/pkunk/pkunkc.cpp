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


static ConversationData pkunk_desc =
	{
		PKUNK_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		PkunkPmapAnim, /* AlienFrame */
		PkunkFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		PkunkColorMap, /* AlienColorMap */
		PkunkMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		PkunkConversationPhrases, /* PlayerPhrases */
		3, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				3, /* StartIndex */
				4, /* NumFrames */
				CircularAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				7, /* StartIndex */
				4, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2),				/* BlockMask */
			},
								{
				11, /* StartIndex */
				3,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 1),				/* BlockMask */
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
			2, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 15,
								GameTicksPerSecond / 6, /* FrameRate */
			GameTicksPerSecond / 12,
								GameTicksPerSecond / 2, /* RestartRate */
			0,						/* BlockMask */
		},
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static constexpr auto GoodReason1 {(1 << 0)};
static constexpr auto GoodReason2 {(1 << 1)};
static constexpr auto BadReason1 {(1 << 2)};
static constexpr auto BadReason2 {(1 << 3)};

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, friendly_bye_space))
	{
		NPCPhrase(FRIENDLY_GOODBYE_SPACE);
	}
	else if (PlayerSaid(R, neutral_bye_space))
	{
		NPCPhrase(NEUTRAL_GOODBYE_SPACE);
	}
	else if (PlayerSaid(R, bye_angry))
	{
		NPCPhrase(GOODBYE_ANGRY);
	}
	else if (PlayerSaid(R, bye_friendly))
	{
		NPCPhrase(GOODBYE_FRIENDLY);
	}
	else if (PlayerSaid(R, we_here_to_help)
			 || PlayerSaid(R, we_need_help))
	{
		if (PlayerSaid(R, we_here_to_help))
		{
			NPCPhrase(NEED_HELP);
		}
		else
		{
			NPCPhrase(GIVE_HELP);
		}
		NPCPhrase(ALMOST_ALLIANCE);

		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(0));

		AddEvent(EventType::Relative, 6, 0, 0, ADVANCE_PKUNK_MISSION);

		if (EscortFeasibilityStudy(PKUNK_SHIP) == 0)
		{
			NPCPhrase(INIT_NO_ROOM);
		}
		else
		{
			NPCPhrase(INIT_SHIP_GIFT);
			AlienTalkSegue((uint16_t)~0);
			if (AddEscortShips(PKUNK_SHIP, difficultyCase(4, 4, 2)))
			{
				PrepareShip(PKUNK_SHIP);
			}
		}
	}
	else if (PlayerSaid(R, try_to_be_nicer))
	{
		NPCPhrase(CANT_ASK_FOR_MORE);
		NPCPhrase(VISIT_OUR_HOMEWORLD);

		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, must_conquer)
			 || PlayerSaid(R, obey))
	{
		if (PlayerSaid(R, obey))
		{
			NPCPhrase(NO_OBEY);
		}
		else
		{
			NPCPhrase(BAD_IDEA);

			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(2));
		}
		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, die_idiot_fools))
	{
		NPCPhrase(VERY_WELL);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, suit_yourself))
	{
		NPCPhrase(GOODBYE_MIGRATION);
	}
	else
	{
		uint8_t ReasonMask {};

		ReasonMask = uqm::gameState().flag(uqm::GameFlag::PKUNK_REASONS);
		if (PlayerSaid(R, good_reason_1))
		{
			NPCPhrase(WE_GO_HOME_1);
			ReasonMask |= GoodReason1;
			AddEvent(EventType::Relative, 0, 0, 0, ADVANCE_PKUNK_MISSION);
		}
		else if (PlayerSaid(R, good_reason_2))
		{
			NPCPhrase(WE_GO_HOME_2);
			ReasonMask |= GoodReason2;
			AddEvent(EventType::Relative, 0, 0, 0, ADVANCE_PKUNK_MISSION);
		}
		else if (PlayerSaid(R, bad_reason_1))
		{
			NPCPhrase(NO_GO_HOME_1);
			ReasonMask |= BadReason1;
		}
		else if (PlayerSaid(R, bad_reason_2))
		{
			NPCPhrase(NO_GO_HOME_2);
			ReasonMask |= BadReason2;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_REASONS, static_cast<uint32_t>(ReasonMask));
	}
}

static void PkunkHome(RESPONSE_REF R);

static void PkunkAngry(RESPONSE_REF R)
{
	if (PlayerSaid(R, why_insults))
	{
		NPCPhrase(RELEASE_TENSION);

		DisablePhrase(why_insults);
	}
	else if (PlayerSaid(R, what_about_you))
	{
		NPCPhrase(ABOUT_US);

		DisablePhrase(what_about_you);
	}
	else if (PlayerSaid(R, should_be_friends))
	{
		NPCPhrase(YES_FRIENDS);

		DisablePhrase(should_be_friends);
	}

	if (PhraseEnabled(should_be_friends))
	{
		Response(should_be_friends, PkunkAngry);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			Response(try_to_be_nicer, PkunkHome);
		}
		else
		{
			Response(try_to_be_nicer, ExitConversation);
		}
	}
	Response(die_idiot_fools, ExitConversation);
	if (PhraseEnabled(why_insults))
	{
		Response(why_insults, PkunkAngry);
	}
	if (PhraseEnabled(what_about_you))
	{
		Response(what_about_you, PkunkAngry);
	}
	Response(bye_angry, ExitConversation);
}

static void DiscussConquer(RESPONSE_REF R)
{
	if (PlayerSaid(R, we_conquer))
	{
		NPCPhrase(WHY_CONQUER);

		DisablePhrase(we_conquer);
	}
	else if (PlayerSaid(R, conquer_because_1))
	{
#if 0
		NPCPhrase (NOT_CONQUER_10);
		NPCPhrase (GLOBAL_ALLIANCE_NAME + name_1);
		NPCPhrase (NOT_CONQUER_11);
		NPCPhrase (GLOBAL_ALLIANCE_NAME + name_1);
		NPCPhrase (NOT_CONQUER_12);
#endif
		NPCPhrase(NOT_CONQUER_1);

		DisablePhrase(conquer_because_1);
	}
	else if (PlayerSaid(R, conquer_because_2))
	{
		NPCPhrase(NOT_CONQUER_2);

		DisablePhrase(conquer_because_2);
	}

	if (PhraseEnabled(conquer_because_1))
	{
#if 0
		char buf[ALLIANCE_NAME_BUFSIZE];

		GetAllianceName (buf, name_1);
		construct_response (
				shared_phrase_buf,
				conquer_because_1,
				buf,
				(RESPONSE_REF)-1);
		DoResponsePhrase (conquer_because_1, DiscussConquer, shared_phrase_buf);
#endif
		Response(conquer_because_1, DiscussConquer);
	}
	if (PhraseEnabled(conquer_because_2))
	{
		Response(conquer_because_2, DiscussConquer);
	}
	Response(must_conquer, ExitConversation);
	Response(no_conquest, PkunkHome);
}

static void OfferAlliance(RESPONSE_REF R)
{
	if (PlayerSaid(R, we_are_vindicator))
	{
		NPCPhrase(WHY_YOU_HERE);
	}
	else if (PlayerSaid(R, exploring_universe))
	{
		NPCPhrase(SENSE_DEEPER_CONFLICT);

		DisablePhrase(exploring_universe);
	}
	else if (PlayerSaid(R, fun_cruise))
	{
		NPCPhrase(REPRESS);

		DisablePhrase(fun_cruise);
	}

	Response(we_here_to_help, ExitConversation);
	Response(we_need_help, ExitConversation);
	if (PhraseEnabled(exploring_universe))
	{
		Response(exploring_universe, OfferAlliance);
	}
	if (PhraseEnabled(fun_cruise))
	{
		Response(fun_cruise, OfferAlliance);
	}
}

static void AboutPkunk(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, what_about_you))
	{
		NPCPhrase(ABOUT_US);
	}
	else if (PlayerSaid(R, what_about_history))
	{
		NPCPhrase(ABOUT_HISTORY);

		DisablePhrase(what_about_history);
	}
	else if (PlayerSaid(R, what_about_yehat))
	{
		NPCPhrase(ABOUT_YEHAT);

		DisablePhrase(what_about_yehat);
	}
	else if (PlayerSaid(R, what_about_culture))
	{
		NPCPhrase(ABOUT_CULTURE);

		DisablePhrase(what_about_culture);
	}
	else if (PlayerSaid(R, elaborate_culture))
	{
		NPCPhrase(OK_ELABORATE_CULTURE);

		DisablePhrase(elaborate_culture);
	}
	else if (PlayerSaid(R, what_about_future))
	{
		NPCPhrase(ABOUT_FUTURE);

		DisablePhrase(what_about_future);
	}

	if (PhraseEnabled(what_about_history))
	{
		Response(what_about_history, AboutPkunk);
		InfoLeft = true;
	}
	else if (PhraseEnabled(what_about_yehat))
	{
		Response(what_about_yehat, AboutPkunk);
		InfoLeft = true;
	}
	if (PhraseEnabled(what_about_culture))
	{
		Response(what_about_culture, AboutPkunk);
		InfoLeft = true;
	}
	else if (PhraseEnabled(elaborate_culture))
	{
		Response(elaborate_culture, AboutPkunk);
		InfoLeft = true;
	}
	if (PhraseEnabled(what_about_future))
	{
		Response(what_about_future, AboutPkunk);
		InfoLeft = true;
	}
	Response(enough_about_you, PkunkHome);

	if (!InfoLeft)
	{
		DisablePhrase(what_about_you);
	}
}

static void AboutIlwrath(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, what_about_ilwrath))
	{
		NPCPhrase(ABOUT_ILWRATH);
	}
	else if (PlayerSaid(R, why_ilwrath_fight))
	{
		NPCPhrase(ILWRATH_FIGHT_BECAUSE);

		DisablePhrase(why_ilwrath_fight);
	}
	else if (PlayerSaid(R, when_fight_start))
	{
		NPCPhrase(FIGHT_START_WHEN);

		DisablePhrase(when_fight_start);
	}
	else if (PlayerSaid(R, how_goes_fight))
	{
		NPCPhrase(FIGHT_GOES);

		DisablePhrase(how_goes_fight);
	}
	else if (PlayerSaid(R, how_stop_fight))
	{
		NPCPhrase(STOP_FIGHT_LIKE_SO);

		DisablePhrase(how_stop_fight);
	}

	if (PhraseEnabled(why_ilwrath_fight))
	{
		Response(why_ilwrath_fight, AboutIlwrath);
		InfoLeft = true;
	}
	if (PhraseEnabled(when_fight_start))
	{
		Response(when_fight_start, AboutIlwrath);
		InfoLeft = true;
	}
	if (PhraseEnabled(how_goes_fight))
	{
		Response(how_goes_fight, AboutIlwrath);
		InfoLeft = true;
	}
	if (PhraseEnabled(how_stop_fight))
	{
		Response(how_stop_fight, AboutIlwrath);
		InfoLeft = true;
	}
	Response(enough_ilwrath, PkunkHome);

	if (!InfoLeft)
	{
		DisablePhrase(what_about_ilwrath);
	}
}

static void PkunkHome(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, no_conquest))
	{
		NPCPhrase(GOOD_IDEA);
	}
	else if (PlayerSaid(R, enough_ilwrath))
	{
		NPCPhrase(OK_ENOUGH_ILWRATH);
	}
	else if (PlayerSaid(R, enough_about_you))
	{
		NPCPhrase(OK_ENOUGH_ABOUT_US);
	}
	else if (PlayerSaid(R, where_fleet_1)
			 || PlayerSaid(R, where_fleet_2)
			 || PlayerSaid(R, where_fleet_3))
	{
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SWITCH, static_cast<uint32_t>(1));
		if (!(uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION) & 1))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_RETURN);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(RETURNING_FROM_YEHAT_1);
					break;
				case 1:
					NPCPhrase(RETURNING_FROM_YEHAT_2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_RETURN, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_MIGRATE);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(MIGRATING_HOMEWORLD_1);
					break;
				case 1:
					NPCPhrase(MIGRATING_HOMEWORLD_2);
					break;
				case 2:
					NPCPhrase(MIGRATING_HOMEWORLD_3);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MIGRATE, static_cast<uint32_t>(NumVisits));
		}

		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_FLEET) + 1;
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_FLEET, static_cast<uint32_t>(NumVisits));

		DisablePhrase(where_fleet_1);
	}
	else if (PlayerSaid(R, am_worried_1)
			 || PlayerSaid(R, am_worried_2)
			 || PlayerSaid(R, am_worried_3))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_WORRY);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(DONT_WORRY_1);
				break;
			case 1:
				NPCPhrase(DONT_WORRY_2);
				break;
			case 2:
				NPCPhrase(DONT_WORRY_3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_WORRY, static_cast<uint32_t>(NumVisits));

		DisablePhrase(am_worried_1);
	}
	else if (PlayerSaid(R, try_to_be_nicer))
	{
		NPCPhrase(CANT_ASK_FOR_MORE);
		if (!uqm::gameState().flag(uqm::GameFlag::CLEAR_SPINDLE))
		{
			NPCPhrase(GIVE_SPINDLE);

			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP, static_cast<uint32_t>(1));
		}
		NPCPhrase(CAN_BE_FRIENDS);
		SetHomeworldKnown(PKUNK_HOME);

		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, what_about_ilwrath))
	{
		NPCPhrase(ABOUT_ILWRATH /* ILWRATH_GONE */);

		DisablePhrase(what_about_ilwrath);
	}

	if (PhraseEnabled(we_conquer) && uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 0)
	{
		Response(we_conquer, DiscussConquer);
	}
	if (uqm::gameState().flag(uqm::GameFlag::PKUNK_ON_THE_MOVE))
	{
		if (PhraseEnabled(where_fleet_1) && !uqm::gameState().flag(uqm::GameFlag::PKUNK_SWITCH))
		{
			switch (uqm::gameState().flag(uqm::GameFlag::PKUNK_FLEET))
			{
				case 0:
					Response(where_fleet_1, PkunkHome);
					break;
				case 1:
					Response(where_fleet_2, PkunkHome);
					break;
				case 2:
					Response(where_fleet_3, PkunkHome);
					break;
			}
		}
		else if (!PhraseEnabled(where_fleet_1)
				 && PhraseEnabled(am_worried_1)
				 && (uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION) & 1))
		{
			switch (uqm::gameState().flag(uqm::GameFlag::PKUNK_WORRY))
			{
				case 0:
					Response(am_worried_1, PkunkHome);
					break;
				case 1:
					Response(am_worried_2, PkunkHome);
					break;
				case 2:
					Response(am_worried_3, PkunkHome);
					break;
			}
		}
	}
	if (!uqm::gameState().flag(uqm::GameFlag::PKUNK_SHIP_MONTH))
	{
		Response(we_are_vindicator, OfferAlliance);
	}
	if (PhraseEnabled(what_about_you))
	{
		Response(what_about_you, AboutPkunk);
	}
	if (PhraseEnabled(what_about_ilwrath))
	{
		if (!uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			Response(what_about_ilwrath, AboutIlwrath);
		}
		else
		{
			Response(what_about_ilwrath, PkunkHome);
		}
	}
	Response(bye_friendly, ExitConversation);
}

static void PkunkFriendlySpace(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, whats_up_space))
	{
		if (ShipsReady(PKUNK_SHIP) && uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 3)
		{
			NPCPhrase(SHIPS_AT_HOME);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_INFO);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(GENERAL_INFO_SPACE_1);
					break;
				case 1:
					NPCPhrase(GENERAL_INFO_SPACE_2);
					break;
				case 2:
					NPCPhrase(GENERAL_INFO_SPACE_3);
					break;
				case 3:
					NPCPhrase(GENERAL_INFO_SPACE_4);
					break;
				case 4:
					NPCPhrase(GENERAL_INFO_SPACE_5);
					break;
				case 5:
					NPCPhrase(GENERAL_INFO_SPACE_6);
					break;
				case 6:
					NPCPhrase(GENERAL_INFO_SPACE_7);
					break;
				case 7:
					NPCPhrase(GENERAL_INFO_SPACE_8);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(NumVisits));
		}

		DisablePhrase(whats_up_space);
	}
	else if (PlayerSaid(R, how_goes_war))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_WAR);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(WAR_GOES_1);
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(1));
				break;
			case 1:
				NPCPhrase(WAR_GOES_2);
				break;
			case 2:
				NPCPhrase(WAR_GOES_3);
				break;
			case 3:
				NPCPhrase(WAR_GOES_4);
				uqm::gameState().setFlag(uqm::GameFlag::PKUNK_DONE_WAR, static_cast<uint32_t>(1));
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_WAR, static_cast<uint32_t>(NumVisits));

		DisablePhrase(how_goes_war);
	}
	else if (PlayerSaid(R, tell_my_fortune))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_FORTUNE);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(FORTUNE_IS_1);
				break;
			case 1:
				NPCPhrase(FORTUNE_IS_2);
				break;
			case 2:
				NPCPhrase(FORTUNE_IS_3);
				break;
			case 3:
				NPCPhrase(FORTUNE_IS_4);
				break;
			case 4:
				NPCPhrase(FORTUNE_IS_5);
				break;
			case 5:
				NPCPhrase(FORTUNE_IS_6);
				break;
			case 6:
				NPCPhrase(FORTUNE_IS_7);
				break;
			case 7:
				NPCPhrase(FORTUNE_IS_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_FORTUNE, static_cast<uint32_t>(NumVisits));

		DisablePhrase(tell_my_fortune);
	}

	if (PhraseEnabled(whats_up_space))
	{
		Response(whats_up_space, PkunkFriendlySpace);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::PKUNK_DONE_WAR) && PhraseEnabled(how_goes_war))
	{
		Response(how_goes_war, PkunkFriendlySpace);
	}
	if (PhraseEnabled(tell_my_fortune))
	{
		Response(tell_my_fortune, PkunkFriendlySpace);
	}
	Response(friendly_bye_space, ExitConversation);
}

static void PkunkNeutralSpace(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, form_alliance))
	{
		NPCPhrase(GO_TO_HOMEWORLD);
		SetHomeworldKnown(PKUNK_HOME);

		DisablePhrase(form_alliance);
	}
	else if (PlayerSaid(R, can_you_help))
	{
		NPCPhrase(GO_TO_HOMEWORLD_AGAIN);
		SetHomeworldKnown(PKUNK_HOME);

		DisablePhrase(can_you_help);
	}
	else if (PlayerSaid(R, hostile_greeting))
	{
		NPCPhrase(DONT_BE_HOSTILE);

		DisablePhrase(hostile_greeting);
	}
	else if (PlayerSaid(R, whats_up_neutral))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_SPACE_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_SPACE_2);
				break;
			case 2:
				NPCPhrase(GENERAL_INFO_SPACE_6 /* was 3 */);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_SPACE_7 /* was 4 */);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_neutral);
	}

	if (PhraseEnabled(form_alliance))
	{
		Response(form_alliance, PkunkNeutralSpace);
	}
	else if (PhraseEnabled(can_you_help))
	{
		Response(can_you_help, PkunkNeutralSpace);
	}
	if (PhraseEnabled(hostile_greeting))
	{
		Response(hostile_greeting, PkunkNeutralSpace);
	}
	else
	{
		Response(obey, ExitConversation);
	}
	if (PhraseEnabled(whats_up_neutral))
	{
		Response(whats_up_neutral, PkunkNeutralSpace);
	}
	Response(neutral_bye_space, ExitConversation);
}

static void PkunkMigrate(RESPONSE_REF R)
{
	uint8_t ReasonMask {};
	(void)R; // ignored

	ReasonMask = uqm::gameState().flag(uqm::GameFlag::PKUNK_REASONS);
	if (!(ReasonMask & GoodReason1))
	{
		Response(good_reason_1, ExitConversation);
	}
	if (!(ReasonMask & BadReason1))
	{
		Response(bad_reason_1, ExitConversation);
	}
	if (!(ReasonMask & GoodReason2))
	{
		Response(good_reason_2, ExitConversation);
	}
	if (!(ReasonMask & BadReason2))
	{
		Response(bad_reason_2, ExitConversation);
	}
	Response(suit_yourself, ExitConversation);
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

	Manner = uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER);
	if (Manner == 2)
	{
		// Irreparably Pissed off the Pkunk.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HATE_YOU_FOREVER_1);
				break;
			case 1:
				NPCPhrase(HATE_YOU_FOREVER_2);
				break;
			case 2:
				NPCPhrase(HATE_YOU_FOREVER_3);
				break;
			case 3:
				NPCPhrase(HATE_YOU_FOREVER_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else if (Manner == 1)
	{
		// Bad relations with the Pkunk, but not irreparably.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SPIRITUAL_PROBLEMS_1);
				break;
			case 1:
				NPCPhrase(SPIRITUAL_PROBLEMS_2);
				break;
			case 2:
				NPCPhrase(SPIRITUAL_PROBLEMS_3);
				break;
			case 3:
				NPCPhrase(SPIRITUAL_PROBLEMS_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(NumVisits));

		PkunkAngry((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		// Encountering the Pkunk at their home world.
		if (!uqm::gameState().flag(uqm::GameFlag::CLEAR_SPINDLE))
		{
			NPCPhrase(GIVE_SPINDLE);

			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP, static_cast<uint32_t>(1));
		}
		else if (!uqm::gameState().flag(uqm::GameFlag::PKUNK_SENSE_VICTOR)
				 && uqm::gameState().clock().year() > StartYear
				 && !uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
		{
			NPCPhrase(SENSE_KOHRAH_VICTORY);

			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_SENSE_VICTOR, static_cast<uint32_t>(1));
		}

		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_HOME_VISITS);
		if (Manner == 0)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(NEUTRAL_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase(NEUTRAL_HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase(NEUTRAL_HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase(NEUTRAL_HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
		}
		else
		{
			if (NumVisits && ShipsReady(PKUNK_SHIP) && uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 3)
			{
				if (EscortFeasibilityStudy(PKUNK_SHIP) == 0)
				{
					NPCPhrase(NO_ROOM);
				}
				else
				{
					NPCPhrase(SHIP_GIFT);
					if (AddEscortShips(PKUNK_SHIP, difficultyCase(4, 4, 2)))
					{
						PrepareShip(PKUNK_SHIP);
					}
				}
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_1);
						break;
					case 1:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_2);
						break;
					case 2:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_3);
						break;
					case 3:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_4);
						break;
					case 4:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_5);
						break;
					case 5:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_6);
						break;
					case 6:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_7);
						break;
					case 7:
						NPCPhrase(FRIENDLY_HOMEWORLD_HELLO_8);
						--NumVisits;
						break;
				}
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		PkunkHome((RESPONSE_REF)0);
	}
	else if ((NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION)) == 0
			 || !(NumVisits & 1))
	{
		// Encountering a Pkunk ship in space, while they are not
		// migrating.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_VISITS);
		if (Manner == 3)
		{
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
					break;
				case 4:
					NPCPhrase(FRIENDLY_SPACE_HELLO_5);
					break;
				case 5:
					NPCPhrase(FRIENDLY_SPACE_HELLO_6);
					break;
				case 6:
					NPCPhrase(FRIENDLY_SPACE_HELLO_7);
					break;
				case 7:
					NPCPhrase(FRIENDLY_SPACE_HELLO_8);
					--NumVisits;
					break;
			}

			PkunkFriendlySpace((RESPONSE_REF)0);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(NEUTRAL_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(NEUTRAL_SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase(NEUTRAL_SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase(NEUTRAL_SPACE_HELLO_4);
					SetHomeworldKnown(PKUNK_HOME);
					--NumVisits;
					break;
			}

			PkunkNeutralSpace((RESPONSE_REF)0);
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else
	{
		// Encountering a Pkunk ship in space, while they are
		// migrating.
		NumVisits = uqm::gameState().flag(uqm::GameFlag::PKUNK_MIGRATE_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(MIGRATING_SPACE_1);
				break;
			case 1:
				NPCPhrase(MIGRATING_SPACE_2);
				break;
			case 2:
				NPCPhrase(MIGRATING_SPACE_3);
				break;
			case 3:
				NPCPhrase(MIGRATING_SPACE_4);
				break;
			case 4:
				NPCPhrase(MIGRATING_SPACE_5);
				break;
			case 5:
				NPCPhrase(MIGRATING_SPACE_6);
				break;
			case 6:
				NPCPhrase(MIGRATING_SPACE_7);
				break;
			case 7:
				NPCPhrase(MIGRATING_SPACE_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MIGRATE_VISITS, static_cast<uint32_t>(NumVisits));

		PkunkMigrate((RESPONSE_REF)0);
	}
}

// Called after combat or communications
static uint16_t uninit_pkunk(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_pkunk_enc(void)
{
	uint8_t Manner {};

	if (getSegue() == Segue::Hostile
		&& (Manner = uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::PKUNK_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::PKUNK_HOME_VISITS, static_cast<uint32_t>(0));
		}
	}
}

ConversationData* init_pkunk_comm(void)
{
	pkunk_desc.init_encounter_func = Intro;
	pkunk_desc.post_encounter_func = post_pkunk_enc;
	pkunk_desc.uninit_encounter_func = uninit_pkunk;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	pkunk_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	pkunk_desc.AlienTextBaseline.y = 0;
	pkunk_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::PKUNK_MANNER) == 3
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		// Enter communications immediately.
		setSegue(Segue::Peace);
	}
	else
	{
		// Ask the player whether to attack or talk.
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&pkunk_desc};

	return (retval);
}
