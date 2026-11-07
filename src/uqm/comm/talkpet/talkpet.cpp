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
#include "uqm/battle.h"
#include "uqm/sounds.h"

static constexpr auto StrobeRate {30};
static constexpr auto StrobeLength {(GameTicksPerSecond / 3)};
static constexpr auto NumStrobes {(StrobeLength * StrobeRate / GameTicksPerSecond)};

static ConversationData talkpet_desc =
	{
		Conversation::TalkingPet, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		TalkingPetPmapAnim, /* AlienFrame */
		TalkingPetFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		TalkingPetColorMap, /* AlienColorMap */
		TalkingPetMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		TalkingPetConversationPhrases, /* PlayerPhrases */
		18, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				7, /* StartIndex */
				3, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				10, /* StartIndex */
				3,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				13,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Blink right eye */
				18,											/* StartIndex */
				3,											/* NumFrames */
				YoyoAnim,									/* AnimFlags */
				GameTicksPerSecond / 20, 0,					/* FrameRate */
				GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
				(1 << 13)									/* BlockMask */
			},
								{
				/* Blink left eye */
				21,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12) | (1 << 14),	/* BlockMask */
			},
								{
				24,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				26,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				28,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				32,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				35,			/* StartIndex */
				5,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				40,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				42, /* StartIndex */
				6,	/* NumFrames */
				CircularAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
								{
				/* Right eyebrow */
				48,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4),				/* BlockMask */
			},
								{
				/* Left eyebrow */
				50,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
								{
				52,						  /* StartIndex */
				2,						  /* NumFrames */
				RandomAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Mind control strobe (on-demand) */
				1,				/* StartIndex */
				NumStrobes * 2, /* NumFrames */
				CircularAnim | ColorxformAnim | OneShotAnim
					| AnimDisabled, /* AnimFlags */
				GameTicksPerSecond / (StrobeRate * 2),
				0, /* FrameRate */
				0,
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
			6, /* NumFrames */
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

static AnimFilterSet talkpet_filters =
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
	setSegue(Segue::Peace);
	uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(0));

	if (PlayerSaid(R, compel_urquan))
	{
		NPCPhrase(HERE_WE_GO);

		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_MESSED_UP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, wish_to_go_now))
	{
		NPCPhrase(EXCELLENT_IDEA);
	}
	else if (PlayerSaid(R, bye_onboard))
	{
		NPCPhrase(GOODBYE_ONBOARD);
	}
	else if (PlayerSaid(R, compel_that_ship))
	{
		NPCPhrase(SAVING_MY_POWER);
	}
	else if (PlayerSaid(R, ok_lets_do_it)
			 || PlayerSaid(R, want_kill_1)
			 || PlayerSaid(R, want_kill_2)
			 || PlayerSaid(R, want_kill_3))
	{
		if (PlayerSaid(R, ok_lets_do_it))
		{
			NPCPhrase(COMING_ABOARD);
		}
		else
		{
			NPCPhrase(GLAD_YOU_WONT_KILL);
		}

		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_ON_SHIP, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_2, static_cast<uint32_t>(0));
	}
}

static void MindFuckUrquan(RESPONSE_REF R)
{
	if (PlayerSaid(R, what_now))
	{
		NPCPhrase(DO_THIS);

		DisablePhrase(what_now);
	}
	else if (PlayerSaid(R, im_scared))
	{
		NPCPhrase(STUPID_FOP);

		DisablePhrase(im_scared);
	}

	if (PhraseEnabled(what_now))
	{
		Response(what_now, MindFuckUrquan);
	}
	if (PhraseEnabled(im_scared))
	{
		Response(im_scared, MindFuckUrquan);
	}
	Response(compel_urquan, ExitConversation);
}

static void PetDevice(RESPONSE_REF R);

static void MindControlStrobe(void)
{
	// Enable the one-shot strobe animation
	CommData.AlienAmbientArray[16].AnimFlags &= ~AnimDisabled;

	if (IsHd())
	{
		EngageFilters(&talkpet_filters);
	}
}

static void MindControl(RESPONSE_REF R)
{
	RESPONSE_FUNC RespFunc {};

	if (PlayerSaid(R, what_about_powers))
	{
		NPCPhrase(NOT_POWERS_BUT_FLOWERS);

		RespFunc = (RESPONSE_FUNC)MindControl;
		R = yes_flowers;
	}
	else /* if (R == yes_flowers) */
	{
		NPCPhrase(GOOD_HUMAN);

		RespFunc = (RESPONSE_FUNC)ExitConversation;
		R = wish_to_go_now;
	}

	AlienTalkSegue((uint16_t)~0);
	MindControlStrobe();

	Response(R, RespFunc);
}

static void PetInfo(RESPONSE_REF R)
{
	if (PlayerSaid(R, about_your_race))
	{
		NPCPhrase(WHAT_ABOUT_RACE);
	}
	else if (PlayerSaid(R, what_about_physiology))
	{
		NPCPhrase(NO_TALK_ABOUT_SELF);

		DisablePhrase(what_about_physiology);
	}
	else if (PlayerSaid(R, what_about_your_history))
	{
		NPCPhrase(ABOUT_HISTORY);

		DisablePhrase(what_about_your_history);
	}
	else if (PlayerSaid(R, sentient_milieu))
	{
		NPCPhrase(ABOUT_SENTIENT_MILIEU);

		DisablePhrase(sentient_milieu);
	}
	else if (PlayerSaid(R, what_about_war))
	{
		NPCPhrase(ABOUT_WAR);

		DisablePhrase(what_about_war);
	}

	if (PhraseEnabled(what_about_physiology))
	{
		Response(what_about_physiology, PetInfo);
	}
	else
	{
		Response(what_about_powers, MindControl);
	}
	if (PhraseEnabled(what_about_your_history))
	{
		Response(what_about_your_history, PetInfo);
	}
	else if (PhraseEnabled(sentient_milieu))
	{
		Response(sentient_milieu, PetInfo);
	}
	else if (PhraseEnabled(what_about_war))
	{
		Response(what_about_war, PetInfo);
	}
	Response(enough_info, PetDevice);
}

static void PetDevice(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, whats_up_onboard))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_ONBOARD_1);
				break;
			case 1:
				NPCPhrase(GENERAL_INFO_ONBOARD_2);
				break;
			case 2:
				NPCPhrase(GENERAL_INFO_ONBOARD_3);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_ONBOARD_4);
				break;
			case 4:
				NPCPhrase(GENERAL_INFO_ONBOARD_5);
				break;
			case 5:
				NPCPhrase(GENERAL_INFO_ONBOARD_6);
				break;
			case 6:
				NPCPhrase(GENERAL_INFO_ONBOARD_7);
				break;
			case 7:
				NPCPhrase(GENERAL_INFO_ONBOARD_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_onboard);
	}
	else if (PlayerSaid(R, any_suggestions))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_SUGGESTIONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SUGGESTION_1);
				break;
			case 1:
				NPCPhrase(SUGGESTION_2);
				break;
			case 2:
				NPCPhrase(SUGGESTION_3);
				break;
			case 3:
				NPCPhrase(SUGGESTION_4);
				break;
			case 4:
				NPCPhrase(SUGGESTION_5);
				break;
			case 5:
				NPCPhrase(SUGGESTION_6);
				break;
			case 6:
				NPCPhrase(SUGGESTION_7);
				break;
			case 7:
				NPCPhrase(SUGGESTION_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_SUGGESTIONS, static_cast<uint32_t>(NumVisits));

		DisablePhrase(any_suggestions);
	}
	else if (PlayerSaid(R, enough_info))
	{
		NPCPhrase(OK_ENOUGH_INFO);
	}
	else if (PlayerSaid(R, you_lied))
	{
		NPCPhrase(SO_WHAT);

		uqm::gameState().setFlag(uqm::GameFlag::DNYARRI_LIED, static_cast<uint32_t>(0));
	}

	if (uqm::gameState().flag(uqm::GameFlag::SHIP_TO_COMPEL))
	{
		Response(compel_that_ship, ExitConversation);
	}
	if (PhraseEnabled(whats_up_onboard))
	{
		Response(whats_up_onboard, PetDevice);
	}
	if (PhraseEnabled(any_suggestions))
	{
		Response(any_suggestions, PetDevice);
	}
	Response(about_your_race, PetInfo);
	if (uqm::gameState().flag(uqm::GameFlag::DNYARRI_LIED) && uqm::gameState().flag(uqm::GameFlag::LEARNED_TALKING_PET))
	{
		Response(you_lied, PetDevice);
	}
	Response(bye_onboard, ExitConversation);
}

static void CompelPlayer(RESPONSE_REF R)
{
	RESPONSE_REF pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PlayerSaid(R, what_are_you))
	{
		NPCPhrase(JUST_TALKING_PET);

		DisablePhrase(what_are_you);
	}
	else if (PlayerSaid(R, what_do_to_umgah))
	{
		NPCPhrase(DID_NOTHING);

		DisablePhrase(what_do_to_umgah);
		LastStack = 1;
	}
	else if (PlayerSaid(R, we_are_vindicator))
	{
		NPCPhrase(GOOD_FOR_YOU);

		DisablePhrase(we_are_vindicator);
		LastStack = 2;
	}
	else if (R != 0)
	{
		if (PlayerSaid(R, bye_at_umgah))
		{
			NPCPhrase(GOODBYE_AT_UMGAH);
		}
		else if (PlayerSaid(R, must_explain_presence))
		{
			NPCPhrase(EXPLAIN_NOTHING_MONKEY_BOY);
		}
		else if (PlayerSaid(R, umgah_zombies))
		{
			NPCPhrase(WORKS_LIKE_THIS);
		}
		else if (PlayerSaid(R, talking_pets_dumb))
		{
			NPCPhrase(OH_NO_YOU_DONT);
		}

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES, static_cast<uint32_t>(1));
		if (!uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HYPNOTIZED, static_cast<uint32_t>(1));
		}
		else
		{
			NPCPhrase(CANT_COMPEL);

			setSegue(Segue::Hostile);
		}

		return;
	}

	if (PhraseEnabled(what_are_you))
	{
		pStr[0] = what_are_you;
	}
	else
	{
		pStr[0] = talking_pets_dumb;
	}
	if (uqm::gameState().flag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES))
	{
		if (PhraseEnabled(what_do_to_umgah))
		{
			pStr[1] = what_do_to_umgah;
		}
		else
		{
			pStr[1] = umgah_zombies;
		}
	}
	if (PhraseEnabled(we_are_vindicator))
	{
		pStr[2] = we_are_vindicator;
	}
	else
	{
		pStr[2] = must_explain_presence;
	}

	if (pStr[LastStack])
	{
		Response(pStr[LastStack], CompelPlayer);
	}
	for (uint8_t i {0}; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			Response(pStr[i], CompelPlayer);
		}
	}
	Response(bye_at_umgah, CompelPlayer);
}

static void PetDeal(RESPONSE_REF R);

static void KillPet(RESPONSE_REF R)
{
	if (PlayerSaid(R, must_kill))
	{
		NPCPhrase(DONT_KILL);
		AlienTalkSegue((uint16_t)~0);

		MindControlStrobe();
	}

	Response(want_kill_1, ExitConversation);
	Response(want_kill_2, ExitConversation);
	Response(want_kill_3, ExitConversation);
}

static void PetDeal(RESPONSE_REF R)
{
	if (PlayerSaid(R, what_kind_of_deal))
	{
		NPCPhrase(HELP_DEFEAT_URQUAN);

		DisablePhrase(what_kind_of_deal);
	}
	else if (PlayerSaid(R, how_trust))
	{
		NPCPhrase(TRUST);

		DisablePhrase(how_trust);
	}
	else if (PlayerSaid(R, boneless_dweeb))
	{
		NPCPhrase(YOUR_BONELESS_DWEEB);

		DisablePhrase(boneless_dweeb);
	}
	else if (PlayerSaid(R, what_are_you_really))
	{
		NPCPhrase(POOR_DNYARRI);

		DisablePhrase(what_are_you_really);
	}
	else if (PlayerSaid(R, hard_to_believe))
	{
		NPCPhrase(ITS_TRUE);

		uqm::gameState().setFlag(uqm::GameFlag::DNYARRI_LIED, static_cast<uint32_t>(1));
		DisablePhrase(hard_to_believe);
	}
	else if (PlayerSaid(R, bullshit))
	{
		NPCPhrase(WORTH_A_TRY);

		DisablePhrase(bullshit);
	}
	else if (PlayerSaid(R, kill_you))
	{
		NPCPhrase(PLEASE_DONT);

		DisablePhrase(kill_you);
	}

	if (PhraseEnabled(what_kind_of_deal))
	{
		Response(what_kind_of_deal, PetDeal);
	}
	else
	{
		if (PhraseEnabled(how_trust))
		{
			Response(how_trust, PetDeal);
		}
		else if (PhraseEnabled(boneless_dweeb))
		{
			Response(boneless_dweeb, PetDeal);
		}
		Response(ok_lets_do_it, ExitConversation);
	}
	if (PhraseEnabled(what_are_you_really))
	{
		Response(what_are_you_really, PetDeal);
	}
	else
	{
		if (PhraseEnabled(hard_to_believe) && !uqm::gameState().flag(uqm::GameFlag::LEARNED_TALKING_PET))
		{
			Response(hard_to_believe, PetDeal);
		}
		else if (PhraseEnabled(bullshit) && uqm::gameState().flag(uqm::GameFlag::LEARNED_TALKING_PET))
		{
			Response(bullshit, PetDeal);
		}
	}
	if (PhraseEnabled(kill_you))
	{
		Response(kill_you, PetDeal);
	}
	else if (PhraseEnabled(must_kill))
	{
		Response(must_kill, KillPet);
	}
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(0));
		setSegue(Segue::Hostile);
		if (!(uqm::globalFlags() & CyborgEnabled))
		{
			NPCPhrase(HUMAN_PEP_TALK);
		}
		else
		{
			NPCPhrase(CYBORG_PEP_TALK);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::READY_TO_CONFUSE_URQUAN))
	{
		uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::READY_TO_CONFUSE_URQUAN, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) != 3)
		{
			NPCPhrase(HAVENT_GOT_EVERYTHING);
			if (!uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
			{
				NPCPhrase(NEED_BOMB);
			}
			else
			{
				NPCPhrase(SOUP_UP_BOMB);
			}

			setSegue(Segue::Peace);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::URQUAN_MESSED_UP))
		{
			NPCPhrase(HELLO_AFTER_COMPEL_URQUAN);

			setSegue(Segue::Peace);
		}
		else
		{
			NPCPhrase(I_SENSE_MY_SLAVES);

			MindFuckUrquan((RESPONSE_REF)0);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_VISITS);

		// You can acquire the Talking Pet without having Taalo Shield after
		// the Kohr-Ah wipe out the Umgah. In that case, the Pet will join
		// you willingly, but his complaints about the Taalo shield as in
		// HELLO_AS_DEVICE_1 do not make any sense.
		if (!uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP) && NumVisits == 0)
		{
			++NumVisits; // skip HELLO_AS_DEVICE_1
		}

		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HELLO_AS_DEVICE_1);
				break;
			case 1:
				NPCPhrase(HELLO_AS_DEVICE_2);
				break;
			case 2:
				NPCPhrase(HELLO_AS_DEVICE_3);
				break;
			case 3:
				NPCPhrase(HELLO_AS_DEVICE_4);
				break;
			case 4:
				NPCPhrase(HELLO_AS_DEVICE_5);
				break;
			case 5:
				NPCPhrase(HELLO_AS_DEVICE_6);
				break;
			case 6:
				NPCPhrase(HELLO_AS_DEVICE_7);
				break;
			case 7:
				NPCPhrase(HELLO_AS_DEVICE_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_VISITS, static_cast<uint32_t>(NumVisits));

		PetDevice((RESPONSE_REF)0);
	}
	else if (GetHeadLink(&uqm::gameState().game().npc_built_ship_q))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(0));
				NPCPhrase(HELLO_AT_UMGAH);
				break;
			case 1:
				NPCPhrase(HYPNOTIZE_AGAIN_1);
				break;
			case 2:
				NPCPhrase(HYPNOTIZE_AGAIN_2);
				break;
			case 3:
				NPCPhrase(HYPNOTIZE_AGAIN_3);
				break;
			case 4:
				NPCPhrase(HYPNOTIZE_AGAIN_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		SetHomeworldKnown(*HomeId::Umgah);
		if (NumVisits == 1)
		{
			CompelPlayer((RESPONSE_REF)0);
		}
		else if (!uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HYPNOTIZED, static_cast<uint32_t>(1));
			setSegue(Segue::Peace);
		}
		else
		{
			NPCPhrase(CANT_COMPEL);

			setSegue(Segue::Hostile);
		}
	}
	else
	{
		if (StartSphereTracking(RaceId::Umgah))
		{
			NPCPhrase(LETS_MAKE_A_DEAL);
		}
		else
		{
			NPCPhrase(UMGAH_ALL_GONE);

			if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_HOME_VISITS) == 0
				|| !uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP))
			{ // The how_trust-TRUST exchange only makes sense when the
				// player visited the Talking Pet before so he tried to
				// kill the player *and* the player has the Taalo shield.
				DisablePhrase(how_trust);
			}
		}

		PetDeal((RESPONSE_REF)0);
	}
}

static uint16_t uninit_talkpet(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_talkpet_enc(void)
{
	// nothing defined so far
}

ConversationData* init_talkpet_comm(void)
{
	talkpet_desc.init_encounter_func = Intro;
	talkpet_desc.post_encounter_func = post_talkpet_enc;
	talkpet_desc.uninit_encounter_func = uninit_talkpet;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	talkpet_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	talkpet_desc.AlienTextBaseline.y = 0;
	talkpet_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::InLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}

	ConversationData* retval {&talkpet_desc};

	return (retval);
}
