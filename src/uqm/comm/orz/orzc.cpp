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
#include "../../nameref.h"
//JMS_GFX: For LoadGraphic()

#include "uqm/setup.h"
// for GraphicsLock


static LOCDATA orz_desc =
	{
		ORZ_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		OrzPmapAnim, /* AlienFrame */
		OrzFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_TOP, /* AlienTextValign */
		OrzColorMap, /* AlienColorMap */
		OrzMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		OrzConversationPhrases, /* PlayerPhrases */
		13, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				4,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				10,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				15,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				17,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 10, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				20,		  /* StartIndex */
				2,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 7),				/* BlockMask */
			},
								{
				22,			  /* StartIndex */
				8,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6),				/* BlockMask */
			},
								{
				30,			  /* StartIndex */
				3,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
								{
				33,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4),				/* BlockMask */
			},
								{
				36,			  /* StartIndex */
				25,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				61,			  /* StartIndex */
				15,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				76,			  /* StartIndex */
				17,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
								{
				93,			  /* StartIndex */
				25,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				118,		  /* StartIndex */
				11,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 60,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10),				/* BlockMask */
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
								GameTicksPerSecond / 15, /* FrameRate */
			GameTicksPerSecond / 12,
								GameTicksPerSecond * 3 / 8, /* RestartRate */
			0,							/* BlockMask */
		},
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static FILTER_DESC orz_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,			   /* Color index */
				1,			   /* Opacity index */
				-1,			   /* Frame index */
				DRAW_MULTIPLY, /* DrawKind*/
				0,			   /* Flags */
			},
								  }
};

static void
ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue_peace);

	if (PlayerSaid(R, bye_ally))
	{
		NPCPhrase(GOODBYE_ALLY);
	}
	else if (PlayerSaid(R, bye_neutral))
	{
		NPCPhrase(GOODBYE_NEUTRAL);
	}
	else if (PlayerSaid(R, bye_angry))
	{
		NPCPhrase(GOODBYE_ANGRY);
	}
	else if (PlayerSaid(R, bye_taalo))
	{
		if (uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER) == 1)
		{
			NPCPhrase(ANGRY_TAALO_GOODBYE);
		}
		else
		{
			NPCPhrase(FRIENDLY_TAALO_GOODBYE);
		}
	}
	else if (PlayerSaid(R, hostile_2))
	{
		NPCPhrase(HOSTILITY_IS_BAD_2);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, may_we_land))
	{
		NPCPhrase(SURE_LAND);

		uqm::gameState().setFlag(uqm::GameFlag::TAALO_UNPROTECTED, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, yes_alliance)
			 || PlayerSaid(R, were_sorry))
	{
		if (PlayerSaid(R, yes_alliance))
		{
			NPCPhrase(GREAT);
		}
		else
		{
			NPCPhrase(APOLOGY_ACCEPTED);
		}

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_GENERAL_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_PERSONAL_INFO, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_MANNER, static_cast<uint32_t>(3));
		SetRaceAllied(ORZ_SHIP, true);
	}
	else if (PlayerSaid(R, demand_to_land))
	{
		NPCPhrase(NO_DEMAND);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, about_andro_3)
			 || PlayerSaid(R, must_know_about_androsyn))
	{
		// Run filter for HdResolutionFactor
		if (IsHd())
		{
			EngageFilters(&orz_filters);
		}

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond / 2);

		if (PlayerSaid(R, about_andro_3))
		{
			NPCPhrase(BLEW_IT);
		}
		else
		{
			NPCPhrase(KNOW_TOO_MUCH);
		}

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_MANNER, static_cast<uint32_t>(2));
		setSegue(Segue_hostile);
		if (PlayerSaid(R, about_andro_3))
		{
			SetRaceAllied(ORZ_SHIP, false);
			RemoveEscortShips(ORZ_SHIP);
		}
	}
	else /* insults */
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_PERSONAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INSULTED_1);
				break;
			case 1:
				NPCPhrase(INSULTED_2);
				break;
			case 2:
				NPCPhrase(INSULTED_3);
				setSegue(Segue_hostile);
				break;
			case 7:
				--NumVisits;
				FALLTHROUGH;
			default:
				NPCPhrase(INSULTED_4);
				setSegue(Segue_hostile);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_PERSONAL_INFO, static_cast<uint32_t>(NumVisits));
	}
}

static void
TaaloWorld(RESPONSE_REF R)
{
	// We can only get here when ORZ_MANNER != HOSTILE (2)
	uint8_t Manner {};

	Manner = uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER);
	if (PlayerSaid(R, demand_to_land))
	{
		NPCPhrase(ASK_NICELY);

		DisablePhrase(demand_to_land);
	}
	else if (PlayerSaid(R, why_you_here))
	{
		if (Manner != 1)
		{
			NPCPhrase(FRIENDLY_EXPLANATION);
		}
		else
		{
			NPCPhrase(ANGRY_EXPLANATION);
		}

		DisablePhrase(why_you_here);
	}
	else if (PlayerSaid(R, what_is_this_place))
	{
		if (Manner != 1)
		{
			NPCPhrase(FRIENDLY_PLACE);
		}
		else
		{
			NPCPhrase(ANGRY_PLACE);
			SetHomeworldKnown(ORZ_HOME);
		}

		DisablePhrase(what_is_this_place);
	}
	else if (PlayerSaid(R, may_we_land))
	{
		NPCPhrase(ALLIES_CAN_VISIT);

		DisablePhrase(may_we_land);
	}
	else if (PlayerSaid(R, make_alliance))
	{
		NPCPhrase(CANT_ALLY_HERE);
		SetHomeworldKnown(ORZ_HOME);

		DisablePhrase(make_alliance);
	}
	else if (PlayerSaid(R, why_busy))
	{
		NPCPhrase(BUSY_BECAUSE);

		DisablePhrase(why_busy);
	}

	if (PhraseEnabled(may_we_land))
	{
		if (Manner == 3 && CheckAlliance(ORZ_SHIP) == GOOD_GUY)
		{
			Response(may_we_land, ExitConversation);
		}
		else
		{
			Response(may_we_land, TaaloWorld);
		}
	}
	else if (PhraseEnabled(make_alliance))
	{
		Response(make_alliance, TaaloWorld);
	}
	else if (PhraseEnabled(why_busy))
	{
		Response(why_busy, TaaloWorld);
	}
	if (PhraseEnabled(demand_to_land))
	{
		if (Manner == 1)
		{
			Response(demand_to_land, ExitConversation);
		}
		else
		{
			Response(demand_to_land, TaaloWorld);
		}
	}
	if (PhraseEnabled(why_you_here))
	{
		Response(why_you_here, TaaloWorld);
	}
	if (PhraseEnabled(what_is_this_place))
	{
		Response(what_is_this_place, TaaloWorld);
	}
	Response(bye_taalo, ExitConversation);
}

static void
OrzAllied(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, whats_up_ally))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_GENERAL_INFO);
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
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_GENERAL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_ally);
	}
	else if (PlayerSaid(R, more_about_you))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_PERSONAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ABOUT_US_1);
				break;
			case 1:
				NPCPhrase(ABOUT_US_2);
				break;
			case 2:
				NPCPhrase(ABOUT_US_3);
				break;
			case 3:
				NPCPhrase(ABOUT_US_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_PERSONAL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(more_about_you);
	}
	else if (PlayerSaid(R, about_andro_1))
	{
		NPCPhrase(FORGET_ANDRO_1);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, about_andro_2))
	{
		NPCPhrase(FORGET_ANDRO_2);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(2));
	}

	if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 0)
	{
		Response(about_andro_1, OrzAllied);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 1)
	{
		Response(about_andro_2, OrzAllied);
	}
	else
	{
		Response(about_andro_3, ExitConversation);
	}
	if (PhraseEnabled(whats_up_ally))
	{
		Response(whats_up_ally, OrzAllied);
	}
	if (PhraseEnabled(more_about_you))
	{
		Response(more_about_you, OrzAllied);
	}
	Response(bye_ally, ExitConversation);
}

static void OrzNeutral(RESPONSE_REF R);

static void
WhereAndrosyn(RESPONSE_REF R)
{
	(void)R; // ignored
	NPCPhrase(DISEMBLE_ABOUT_ANDROSYN);
	DisablePhrase(where_androsyn);

	Response(must_know_about_androsyn, ExitConversation);
	Response(dont_really_care, OrzNeutral);
}

static void
OfferAlliance(RESPONSE_REF R)
{
	if (PlayerSaid(R, seem_like_nice_guys))
	{
		NPCPhrase(ARE_NICE_WANT_ALLY);
	}
	else if (PlayerSaid(R, talk_about_alliance))
	{
		NPCPhrase(OK_TALK_ALLIANCE);
	}
	else if (PlayerSaid(R, why_so_trusting))
	{
		NPCPhrase(TRUSTING_BECAUSE);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_STACK1, static_cast<uint32_t>(1));
	}

	Response(no_alliance, OrzNeutral);
	Response(decide_later, OrzNeutral);
	if (uqm::gameState().flag(uqm::GameFlag::ORZ_STACK1) == 0)
	{
		Response(why_so_trusting, OfferAlliance);
	}
	Response(yes_alliance, ExitConversation);
}

static void
OrzNeutral(RESPONSE_REF R)
{
	RESPONSE_REF pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PlayerSaid(R, hostile_1))
	{
		NPCPhrase(HOSTILITY_IS_BAD_1);

		DisablePhrase(hostile_1);
		LastStack = 2;
	}
	else if (PlayerSaid(R, we_are_vindicator))
	{
		NPCPhrase(NICE_TO_MEET_YOU);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_STACK0, static_cast<uint32_t>(1));
		LastStack = 1;
	}
	else if (PlayerSaid(R, who_you))
	{
		NPCPhrase(WE_ARE_ORZ);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, why_here))
	{
		NPCPhrase(HERE_BECAUSE);

		uqm::gameState().setFlag(uqm::GameFlag::ORZ_ANDRO_STATE, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, no_alliance))
	{
		NPCPhrase(MAYBE_LATER);

		DisablePhrase(talk_about_alliance);
		uqm::gameState().setFlag(uqm::GameFlag::REFUSED_ORZ_ALLIANCE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, decide_later))
	{
		NPCPhrase(OK_LATER);

		DisablePhrase(talk_about_alliance);
		uqm::gameState().setFlag(uqm::GameFlag::REFUSED_ORZ_ALLIANCE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, dont_really_care))
	{
		NPCPhrase(YOU_ARE_OUR_FRIENDS);
	}
	else if (PlayerSaid(R, where_androsyn))
	{
		WhereAndrosyn(R);
		return;
	}
	else if (PlayerSaid(R, talk_about_alliance)
			 || PlayerSaid(R, seem_like_nice_guys))
	{
		OfferAlliance(R);
		return;
	}
	else if (PlayerSaid(R, hostile_2))
	{
		ExitConversation(R);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 0)
	{
		pStr[0] = who_you;
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 1)
	{
		pStr[0] = why_here;
	}
	else if (PhraseEnabled(where_androsyn) && uqm::gameState().flag(uqm::GameFlag::ORZ_ANDRO_STATE) == 2)
	{
		pStr[0] = where_androsyn;
	}
	if (uqm::gameState().flag(uqm::GameFlag::REFUSED_ORZ_ALLIANCE))
	{
		if (PhraseEnabled(talk_about_alliance))
		{
			pStr[1] = talk_about_alliance;
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::ORZ_STACK0) == 0)
	{
		pStr[1] = we_are_vindicator;
	}
	else
	{
		pStr[1] = seem_like_nice_guys;
	}
	if (PhraseEnabled(hostile_1))
	{
		pStr[2] = hostile_1;
	}
	else
	{
		pStr[2] = hostile_2;
	}

	if (pStr[LastStack])
	{
		Response(pStr[LastStack], OrzNeutral);
	}

	for (uint8_t i {0}; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			Response(pStr[i], OrzNeutral);
		}
	}
	Response(bye_neutral, ExitConversation);
}

static void
OrzAngry(RESPONSE_REF R)
{
	if (PlayerSaid(R, whats_up_angry))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_GENERAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_ANGRY_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_ANGRY_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_GENERAL_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_angry);
	}

	if (PhraseEnabled(whats_up_angry))
	{
		Response(whats_up_angry, OrzAngry);
	}
	Response(were_sorry, ExitConversation);
	switch (uqm::gameState().flag(uqm::GameFlag::ORZ_PERSONAL_INFO))
	{
		case 0:
			Response(insult_1, ExitConversation);
			break;
		case 1:
			Response(insult_2, ExitConversation);
			break;
		case 2:
			Response(insult_3, ExitConversation);
			break;
		case 3:
			Response(insult_4, ExitConversation);
			break;
		case 4:
			Response(insult_5, ExitConversation);
			break;
		case 5:
			Response(insult_6, ExitConversation);
			break;
		case 6:
			Response(insult_7, ExitConversation);
			break;
		case 7:
			Response(insult_8, ExitConversation);
			break;
	}
	Response(bye_angry, ExitConversation);
}

static void
Intro(void)
{
	uint8_t NumVisits {};
	uint8_t Manner {};

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue_peace);
		return;
	}

	if (!uqm::gameState().flag(uqm::GameFlag::MET_ORZ_BEFORE))
	{
		NPCPhrase(INIT_HELLO);
	}

	Manner = uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER);
	if (Manner == 2)
	{
		// JMS_GFX: Use separate red angry graphics in hires instead of colormap transform.
		if (IsHd())
		{
			CommData.AlienFrameRes = OrzAngryPmapAnim;
			CommData.AlienFrame = CaptureDrawable(
				LoadGraphic(CommData.AlienFrameRes));
		}
		else
		{
			CommData.AlienColorMapIndex = 1;
		}

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HOSTILE_HELLO_1);
				break;
			case 1:
				NPCPhrase(HOSTILE_HELLO_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue_hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TAALO_VISITS);
		if (Manner != 1)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(FRIENDLY_ALLIED_TAALO_HELLO_1);
					break;
				case 1:
					NPCPhrase(FRIENDLY_ALLIED_TAALO_HELLO_2);
					--NumVisits;
					break;
			}
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ANGRY_TAALO_HELLO_1);
					break;
				case 1:
					NPCPhrase(ANGRY_TAALO_HELLO_2);
					--NumVisits;
					break;
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::TAALO_VISITS, static_cast<uint32_t>(NumVisits));

		TaaloWorld((RESPONSE_REF)0);
	}
	else if (Manner == 3 && CheckAlliance(ORZ_SHIP) == GOOD_GUY)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_HOME_VISITS);
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
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ALLIED_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(ALLIED_SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase(ALLIED_SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase(ALLIED_SPACE_HELLO_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));
		}

		OrzAllied((RESPONSE_REF)0);
	}
	else if (Manner != 1)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_HOME_VISITS);
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
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
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
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));
		}

		OrzNeutral((RESPONSE_REF)0);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ANGRY_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase(ANGRY_HOMEWORLD_HELLO_2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ORZ_VISITS);
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
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(NumVisits));
		}

		OrzAngry((RESPONSE_REF)0);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::MET_ORZ_BEFORE))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MET_ORZ_BEFORE, static_cast<uint32_t>(1));

		// Disable talking anim and run the computer report
		BlockTalkingAnim(0, 1);
	}
}

static uint16_t
uninit_orz(void)
{
	uqm::lua().endComm();
	return (0);
}

static void
post_orz_enc(void)
{
	uint8_t Manner {};

	if (getSegue() == Segue_hostile
		&& (Manner = uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER)) != 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::ORZ_MANNER, static_cast<uint32_t>(1));
		if (Manner != 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::ORZ_HOME_VISITS, static_cast<uint32_t>(0));
			uqm::gameState().setFlag(uqm::GameFlag::TAALO_VISITS, static_cast<uint32_t>(0));
		}
	}
}

LOCDATA*
init_orz_comm(void)
{
	orz_desc.init_encounter_func = Intro;
	orz_desc.post_encounter_func = post_orz_enc;
	orz_desc.uninit_encounter_func = uninit_orz;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	orz_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	orz_desc.AlienTextBaseline.y = 0;
	orz_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::ORZ_MANNER) == 3
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		setSegue(Segue_peace);
	}
	else
	{
		setSegue(Segue_hostile);
	}
	LOCDATA* retval {&orz_desc};

	return (retval);
}
