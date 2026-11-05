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
#include "../spathi/resinst.h"
#include "strings.h"

#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"
#include "uqm/gameev.h"


static LOCDATA spahome_desc =
	{
		SPATHI_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SpathiHomePmapAnim, /* AlienFrame */
		SpathiFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_TOP, /* AlienTextValign */
		SpathiHomeColorMap, /* AlienColorMap */
		SpathiMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			SpahomeMusic, /* AlienAltSong */
		},
		SpathiHomeConversationPhrases, /* PlayerPhrases */
		14, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				1,			  /* StartIndex */
				3,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				4,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				9,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10) | (1 << 11),	/* BlockMask */
			},
								{
				13,							/* StartIndex */
				6,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 20, 0, /* FrameRate */
				GameTicksPerSecond / 20, 0, /* RestartRate */
				(1 << 4) | (1 << 5)			/* BlockMask */
			},
								{
				19,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3) | (1 << 5),	/* BlockMask */
			},
								{
				22,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 3) | (1 << 4)
					| (1 << 10), /* BlockMask */
			},
								{
				26,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10),				/* BlockMask */
			},
								{
				29,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				32,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0,					  /* RestartRate */
				(1 << 9) | (1 << 10), /* BlockMask */
			},
								{
				39,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 8) | (1 << 10),	/* BlockMask */
			},
								{
				42,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				0, /* RestartRate */
				(1 << 8) | (1 << 9)
					| (1 << 6) | (1 << 2)
					| (1 << 11) | (1 << 5), /* BlockMask */
			},
								{
				46,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 30, /* RestartRate */
				(1 << 2) | (1 << 10),	 /* BlockMask */
			},
								{
				50,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond / 20,
				0,		   /* RestartRate */
				(1 << 13), /* BlockMask */
			},
								{
				56,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
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

static FILTER_DESC spahome_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,							/* Color index */
				1,							/* Opacity index */
				-1,							/* Frame index */
				DRAW_ALPHA,					/* DrawKind*/
				SwitchOnAnims | TurnOffOft, /* Flags */
			},
								  }
};

static void
ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue_peace);
	if (PlayerSaid(R, we_attack_again))
	{
		NPCPhrase(WE_FIGHT_AGAIN);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, surrender_or_die))
	{
		NPCPhrase(DEFEND_OURSELVES);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, we_are_vindicator))
	{
		NPCPhrase(NO_PASSWORD);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, gort_merenga)
			 || PlayerSaid(R, guph_florp)
			 || PlayerSaid(R, wagngl_fthagn)
			 || PlayerSaid(R, pleeese))
	{
		NPCPhrase(WRONG_PASSWORD);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, screw_password))
	{
		NPCPhrase(NO_PASSWORD);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, bye_no_ally_offer))
	{
		NPCPhrase(GOODBYE_NO_ALLY_OFFER);
	}
	else if (PlayerSaid(R, bye_angry_spathi))
	{
		NPCPhrase(GOODBYE_ANGRY_SPATHI);
	}
	else if (PlayerSaid(R, bye_ally))
	{
		NPCPhrase(GOODBYE_ALLY);
	}
	else if (PlayerSaid(R, already_got_them))
	{
		NPCPhrase(EARLY_BIRD_CHECK);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_PARTY, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, too_dangerous))
	{
		NPCPhrase(WE_AGREE);
	}
	else if (PlayerSaid(R, think_more))
	{
		NPCPhrase(COWARD);
	}
	else if (PlayerSaid(R, i_accept))
	{
		NPCPhrase(AWAIT_RETURN);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_QUEST, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, do_as_we_say))
	{
		NPCPhrase(DEPART_FOR_EARTH);

		SetRaceAllied(SPATHI_SHIP, true);
		AddEvent(RELATIVE_EVENT, 6, 0, 0, SPATHI_SHIELD_EVENT);
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, killed_them_all_1))
	{
		NPCPhrase(WILL_CHECK_1);

		if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
		{
			uqm::gameState().setFlag(uqm::GameFlag::LIED_ABOUT_CREATURES, static_cast<uint32_t>(1));
		}
		else
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_PARTY, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		}
	}
	else if (PlayerSaid(R, killed_them_all_2))
	{
		NPCPhrase(WILL_CHECK_2);

		if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
		{
			uqm::gameState().setFlag(uqm::GameFlag::LIED_ABOUT_CREATURES, static_cast<uint32_t>(2));
		}
		else
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_PARTY, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		}
	}
	else if (PlayerSaid(R, bye_before_party))
	{
		NPCPhrase(GOODBYE_BEFORE_PARTY);
	}
	else if (PlayerSaid(R, bye_from_party_1)
			 || PlayerSaid(R, bye_from_party_2)
			 || PlayerSaid(R, bye_from_party_3))
	{
		NPCPhrase(GOODBYE_FROM_PARTY);
	}
}

static void SpathiAllies(RESPONSE_REF R);

static void
SpathiInfo(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, like_some_info))
	{
		NPCPhrase(WHAT_ABOUT);
	}
	else if (PlayerSaid(R, what_about_hierarchy))
	{
		NPCPhrase(ABOUT_HIERARCHY);

		DISABLE_PHRASE(what_about_hierarchy);
	}
	else if (PlayerSaid(R, what_about_history))
	{
		NPCPhrase(ABOUT_HISTORY);

		DISABLE_PHRASE(what_about_history);
	}
	else if (PlayerSaid(R, what_about_alliance))
	{
		NPCPhrase(ABOUT_ALLIANCE);

		DISABLE_PHRASE(what_about_alliance);
	}
	else if (PlayerSaid(R, what_about_other))
	{
		NPCPhrase(ABOUT_OTHER);

		DISABLE_PHRASE(what_about_other);
	}
	else if (PlayerSaid(R, what_about_precursors))
	{
		NPCPhrase(ABOUT_PRECURSORS);

		DISABLE_PHRASE(what_about_precursors);
	}

	if (PhraseEnabled(what_about_hierarchy))
	{
		InfoLeft = true;
		Response(what_about_hierarchy, SpathiInfo);
	}
	if (PhraseEnabled(what_about_history))
	{
		InfoLeft = true;
		Response(what_about_history, SpathiInfo);
	}
	if (PhraseEnabled(what_about_alliance))
	{
		InfoLeft = true;
		Response(what_about_alliance, SpathiInfo);
	}
	if (PhraseEnabled(what_about_other))
	{
		InfoLeft = true;
		Response(what_about_other, SpathiInfo);
	}
	if (PhraseEnabled(what_about_precursors))
	{
		InfoLeft = true;
		Response(what_about_precursors, SpathiInfo);
	}
	Response(enough_info, SpathiAllies);

	if (!InfoLeft)
	{
		DISABLE_PHRASE(like_some_info);
	}
}

static void
SpathiAllies(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (R == 0)
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HELLO_ALLIES_1);
				break;
			case 1:
				NPCPhrase(HELLO_ALLIES_2);
				break;
			case 2:
				NPCPhrase(HELLO_ALLIES_3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, whats_up))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_INFO);
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
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(1));
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(1));
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_4);
				break;
			case 4:
				NPCPhrase(GENERAL_INFO_5);
				--NumVisits;
				break;
			case 5:
				NPCPhrase(GENERAL_INFO_5);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_INFO, static_cast<uint32_t>(NumVisits));

		DISABLE_PHRASE(whats_up);
	}
	else if (PlayerSaid(R, resources_please))
	{
		NPCPhrase(SORRY_NO_RESOURCES);

		DISABLE_PHRASE(resources_please);
	}
	else if (PlayerSaid(R, something_fishy))
	{
		NPCPhrase(NOTHING_FISHY);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_INFO, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, enough_info))
	{
		NPCPhrase(OK_ENOUGH_INFO);
	}

	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_INFO) == 4)
	{
		Response(something_fishy, SpathiAllies);
	}
	if (PhraseEnabled(whats_up))
	{
		Response(whats_up, SpathiAllies);
	}
	if (PhraseEnabled(resources_please))
	{
		Response(resources_please, SpathiAllies);
	}
	if (PhraseEnabled(like_some_info))
	{
		Response(like_some_info, SpathiInfo);
	}
	Response(bye_ally, ExitConversation);
}

static void
SpathiQuest(RESPONSE_REF R)
{
	if (R == 0)
	{
		if (!uqm::gameState().flag(uqm::GameFlag::LIED_ABOUT_CREATURES))
		{
			NPCPhrase(HOW_GO_EFFORTS);
		}
		else
		{
			NPCPhrase(YOU_LIED_1);
		}
	}
	else if (PlayerSaid(R, little_mistake))
	{
		NPCPhrase(BIG_MISTAKE);

		DISABLE_PHRASE(little_mistake);
	}
	else if (PlayerSaid(R, talk_test))
	{
		NPCPhrase(TEST_AGAIN);

		DISABLE_PHRASE(talk_test);
	}
	else if (PlayerSaid(R, zapped_a_few))
	{
		NPCPhrase(YOU_FORTUNATE);

		DISABLE_PHRASE(zapped_a_few);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::LIED_ABOUT_CREATURES))
	{
		Response(killed_them_all_1, ExitConversation);
	}
	else
	{
		if (PhraseEnabled(little_mistake))
		{
			Response(little_mistake, SpathiQuest);
		}
		Response(killed_them_all_2, ExitConversation);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
	{
		if (PhraseEnabled(talk_test))
		{
			Response(talk_test, SpathiQuest);
		}
		if (PhraseEnabled(zapped_a_few)
			&& uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_EXAMINED))
		{
			Response(zapped_a_few, SpathiQuest);
		}
		Response(bye_before_party, ExitConversation);
	}
}

static void
LearnQuest(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(QUEST_AGAIN);

		DISABLE_PHRASE(what_test);
		if (uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_EVIL))
		{
			DISABLE_PHRASE(tell_evil);
		}
	}
	else if (PlayerSaid(R, how_prove))
	{
		NPCPhrase(BETTER_IDEA);
	}
	else if (PlayerSaid(R, what_test))
	{
		NPCPhrase(WIPE_EVIL);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_QUEST, static_cast<uint32_t>(1));
		DISABLE_PHRASE(what_test);
	}
	else if (PlayerSaid(R, tell_evil))
	{
		NPCPhrase(BEFORE_ACCEPT);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_EVIL, static_cast<uint32_t>(1));
		DISABLE_PHRASE(tell_evil);
		DISABLE_PHRASE(prove_strength);
	}
	else if (PlayerSaid(R, prove_strength))
	{
		NPCPhrase(YOUR_BEHAVIOR);

		DISABLE_PHRASE(prove_strength);
	}
	else if (PlayerSaid(R, why_dont_you_do_it))
	{
		NPCPhrase(WE_WONT_BECAUSE);

		DISABLE_PHRASE(why_dont_you_do_it);
	}

	if (PhraseEnabled(what_test))
	{
		Response(what_test, LearnQuest);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_CREATURES_ELIMINATED))
	{
		Response(already_got_them, ExitConversation);
	}
	else if (PhraseEnabled(tell_evil))
	{
		Response(too_dangerous, ExitConversation);
		Response(tell_evil, LearnQuest);
	}
	else
	{
		Response(too_dangerous, ExitConversation);
		Response(think_more, ExitConversation);
		Response(i_accept, ExitConversation);
	}
	if (PhraseEnabled(prove_strength) && !uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_QUEST))
	{
		Response(prove_strength, LearnQuest);
	}
	if (PhraseEnabled(why_dont_you_do_it))
	{
		Response(why_dont_you_do_it, LearnQuest);
	}
}

static void
AllianceOffer(RESPONSE_REF R)
{
	if (PlayerSaid(R, misunderstanding))
	{
		NPCPhrase(JUST_MISUNDERSTANDING);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond / 4);

		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(3));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, we_come_in_peace))
	{
		NPCPhrase(OF_COURSE);
	}
	else if (PlayerSaid(R, hand_in_friendship))
	{
		NPCPhrase(TOO_AFRAID);

		DISABLE_PHRASE(hand_in_friendship);
	}
	else if (PlayerSaid(R, stronger))
	{
		NPCPhrase(YOURE_NOT);

		DISABLE_PHRASE(stronger);
	}
	else if (PlayerSaid(R, yes_we_are))
	{
		NPCPhrase(NO_YOURE_NOT);

		DISABLE_PHRASE(yes_we_are);
	}
	else if (PlayerSaid(R, share_info))
	{
		NPCPhrase(NO_INFO);

		DISABLE_PHRASE(share_info);
	}
	else if (PlayerSaid(R, give_us_resources))
	{
		NPCPhrase(NO_RESOURCES);

		DISABLE_PHRASE(give_us_resources);
	}

	if (PhraseEnabled(hand_in_friendship))
	{
		Response(hand_in_friendship, AllianceOffer);
	}
	else if (PhraseEnabled(stronger))
	{
		Response(stronger, AllianceOffer);
	}
	else if (PhraseEnabled(yes_we_are))
	{
		Response(yes_we_are, AllianceOffer);
	}
	else
	{
		Response(how_prove, LearnQuest);
	}
	if (PhraseEnabled(share_info))
	{
		Response(share_info, AllianceOffer);
	}
	if (PhraseEnabled(give_us_resources))
	{
		Response(give_us_resources, AllianceOffer);
	}
}

static void
SpathiAngry(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(MEAN_GUYS_RETURN);

		Response(we_apologize, SpathiAngry);
	}
	else /* if (R == we_apologize) */
	{
		NPCPhrase(DONT_BELIEVE);

		Response(misunderstanding, AllianceOffer);
	}

	Response(we_attack_again, ExitConversation);
	Response(bye_angry_spathi, ExitConversation);
}

static void
SpathiParty(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(MUST_PARTY_1);
				break;
			case 1:
				NPCPhrase(MUST_PARTY_2);
				break;
			case 2:
				NPCPhrase(MUST_PARTY_3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, deals_a_deal))
	{
		NPCPhrase(WAIT_A_WHILE);

		DISABLE_PHRASE(deals_a_deal);
	}
	else if (PlayerSaid(R, how_long))
	{
		NPCPhrase(TEN_YEARS);

		DISABLE_PHRASE(how_long);
	}
	else if (PlayerSaid(R, reneging))
	{
		NPCPhrase(ADULT_VIEW);

		DISABLE_PHRASE(reneging);
	}
	else if (PlayerSaid(R, return_beasts))
	{
		NPCPhrase(WHAT_RELATIONSHIP);

		DISABLE_PHRASE(return_beasts);
	}
	else if (PlayerSaid(R, minds_and_might))
	{
		NPCPhrase(HUH);

		DISABLE_PHRASE(minds_and_might);
	}
	else if (PlayerSaid(R, fellowship))
	{
		NPCPhrase(WHAT);

		DISABLE_PHRASE(fellowship);
	}

	if (PhraseEnabled(deals_a_deal))
	{
		Response(deals_a_deal, SpathiParty);
	}
	else if (PhraseEnabled(how_long))
	{
		Response(how_long, SpathiParty);
	}
	else if (PhraseEnabled(reneging))
	{
		Response(reneging, SpathiParty);
	}
	else if (PhraseEnabled(return_beasts))
	{
		Response(return_beasts, SpathiParty);
	}
	else
	{
		if (PhraseEnabled(minds_and_might))
		{
			Response(minds_and_might, SpathiParty);
		}
		if (PhraseEnabled(fellowship))
		{
			Response(fellowship, SpathiParty);
		}
		Response(do_as_we_say, ExitConversation);

		return;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) - 1)
	{
		case 0:
			Response(bye_from_party_1, ExitConversation);
			break;
		case 1:
			Response(bye_from_party_2, ExitConversation);
			break;
		default:
			Response(bye_from_party_3, ExitConversation);
			break;
	}
}

static void
SpathiCouncil(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(HELLO_AGAIN);
	}
	else if (PlayerSaid(R, good_password))
	{
		NPCPhrase(YES_GOOD_PASSWORD);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond / 4);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_SPATHI_PASSWORD, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, we_come_in_peace))
	{
		NPCPhrase(KILLED_SPATHI);

		DISABLE_PHRASE(we_come_in_peace);
	}
	else if (PlayerSaid(R, spathi_on_pluto))
	{
		NPCPhrase(WHERE_SPATHI);

		DISABLE_PHRASE(spathi_on_pluto);
	}
	else if (PlayerSaid(R, hostage))
	{
		NPCPhrase(GUN_TO_HEAD);

		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(3));
		DISABLE_PHRASE(hostage);
	}
	else if (PlayerSaid(R, killed_fwiffo))
	{
		NPCPhrase(POOR_FWIFFO);

		uqm::gameState().setFlag(uqm::GameFlag::FOUND_PLUTO_SPATHI, static_cast<uint32_t>(3));
		DISABLE_PHRASE(killed_fwiffo);
	}
	else if (PlayerSaid(R, fwiffo_fine))
	{
		NPCPhrase(NOT_LIKELY);

		R = killed_fwiffo;
		DISABLE_PHRASE(fwiffo_fine);
	}
	else if (PlayerSaid(R, surrender))
	{
		NPCPhrase(NO_SURRENDER);

		DISABLE_PHRASE(surrender);
	}

	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER) == 0)
	{
		Response(we_come_in_peace, AllianceOffer);
	}
	else if (PhraseEnabled(we_come_in_peace))
	{
		Response(we_come_in_peace, SpathiCouncil);
	}
	else
	{
		Response(misunderstanding, AllianceOffer);
	}
	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
		&& uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) < 3)
	{
		if (PhraseEnabled(spathi_on_pluto))
		{
			Response(spathi_on_pluto, SpathiCouncil);
		}
		else if (HaveEscortShip(SPATHI_SHIP))
		{
			if (PhraseEnabled(hostage))
			{
				Response(hostage, SpathiCouncil);
			}
		}
		else if (PhraseEnabled(killed_fwiffo))
		{
			Response(killed_fwiffo, SpathiCouncil);
			if (PhraseEnabled(fwiffo_fine))
			{
				Response(fwiffo_fine, SpathiCouncil);
			}
		}
	}
	if (PhraseEnabled(surrender))
	{
		Response(surrender, SpathiCouncil);
	}
	else
	{
		Response(surrender_or_die, ExitConversation);
	}
	Response(bye_no_ally_offer, ExitConversation);
}

static void
SpathiPassword(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			default:
				NPCPhrase(WHAT_IS_PASSWORD);
				NumVisits = 1;
				break;
			case 1:
				NPCPhrase(WHAT_IS_PASSWORD_AGAIN);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, what_do_i_get))
	{
		NPCPhrase(YOU_GET_TO_LIVE);

		DISABLE_PHRASE(what_do_i_get);
	}

	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
		|| uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_PASSWORD))
	{
		Response(good_password, SpathiCouncil);
		if (PhraseEnabled(what_do_i_get))
		{
			Response(what_do_i_get, SpathiPassword);
		}
	}
	else
	{
		Response(we_are_vindicator, ExitConversation);
		Response(gort_merenga, ExitConversation);
		Response(guph_florp, ExitConversation);
		Response(wagngl_fthagn, ExitConversation);
		Response(pleeese, ExitConversation);
		Response(screw_password, ExitConversation);
	}
}

static void
Intro(void)
{
	uint8_t Manner {};

	if (IsHd())
	{
		SwitchSequences(false);
		EngageFilters(&spahome_filters);
	}

	Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER);
	if (Manner == 2)
	{
		NPCPhrase(HATE_YOU_FOREVER);
		setSegue(Segue_hostile);
	}
	else if (Manner == 1
			 && uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_PASSWORD)
			 && (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
				 || uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) != 7))
	{
		SpathiAngry((RESPONSE_REF)0);
	}
	else if (CheckAlliance(SPATHI_SHIP) == GOOD_GUY)
	{
		CommData.AlienColorMapIndex = 1;

		SpathiAllies((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_PARTY))
	{
		CommData.AlienColorMapIndex = 1;

		SpathiParty((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::SPATHI_QUEST))
	{
		if (uqm::gameState().flag(uqm::GameFlag::LIED_ABOUT_CREATURES) < 2)
		{
			CommData.AlienColorMapIndex = 1;

			SpathiQuest((RESPONSE_REF)0);
		}
		else
		{
			NPCPhrase(YOU_LIED_2);

			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(2));
			setSegue(Segue_hostile);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_QUEST))
	{
		CommData.AlienColorMapIndex = 1;

		LearnQuest((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::KNOW_SPATHI_PASSWORD)
			 && (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI)
				 || uqm::gameState().flag(uqm::GameFlag::SPATHI_HOME_VISITS) != 7))
	{
		CommData.AlienColorMapIndex = 1;

		SpathiCouncil((RESPONSE_REF)0);
	}
	else
	{
		SpathiPassword((RESPONSE_REF)0);
	}
}

static uint16_t
uninit_spahome(void)
{
	uqm::lua().endComm();
	return (0);
}

static void
post_spahome_enc(void)
{
	uint8_t Manner {};

	if (getSegue() == Segue_hostile
		&& (Manner = uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SPATHI_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::SPATHI_HOME_VISITS, static_cast<uint32_t>(0));
		}
	}
}

LOCDATA*
init_spahome_comm()
{
	spahome_desc.init_encounter_func = Intro;
	spahome_desc.post_encounter_func = post_spahome_enc;
	spahome_desc.uninit_encounter_func = uninit_spahome;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	spahome_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	spahome_desc.AlienTextBaseline.y = 0;
	spahome_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	// use alternate "Safe Ones" track if available
	altResFlags |= UseAltSong;

	if (uqm::gameState().flag(uqm::GameFlag::SPATHI_MANNER) == 3)
	{
		setSegue(Segue_peace);
	}
	else
	{
		setSegue(Segue_hostile);
	}

	LOCDATA* retval {&spahome_desc};

	return (retval);
}
