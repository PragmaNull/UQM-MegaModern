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

#include "uqm/gameev.h"
#include "libs/mathlib.h"
#include "uqm/races.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/random/UQMRandom.h"


static LOCDATA mycon_desc =
	{
		MYCON_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		MyconPmapAnim, /* AlienFrame */
		MyconFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_TOP, /* AlienTextValign */
		MyconColorMap, /* AlienColorMap */
		MyconMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		MyconConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				12,			  /* StartIndex */
				10,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 80,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								//	{// It was 2 differtent animations that blocks each other
			//	 // but after the commit BlockMask algorithm was changed
			//	 // https://sourceforge.net/p/sc2/uqm/ci/5be8df3e5f4a957fa1ee63851e7e064731aadb20/
			//   // and these animations completely cancel each other.
			//   // Removing BlockMask messes with the animations restart rate,
			//   // so these animations are now merged together
			//		18, /* StartIndex */
			//		4, /* NumFrames */
			//		CircularAnim, /* AnimFlags */
			//		GameTicksPerSecond * 3 / 40, 0, /* FrameRate */
			//		GameTicksPerSecond * 3 / 40, 0, /* RestartRate */
			//		(1 << 0), /* BlockMask */
			//	},
			{
				22,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				28,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				33,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond * 3 / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 3 / 40,
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
			1,	/* StartIndex */
			11, /* NumFrames */
			0,	/* AnimFlags */
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

static uint8_t MadeChoice;

static void
DoRamble(RESPONSE_REF R)
{
	uint8_t Counter {};

	Counter = uqm::gameState().flag(uqm::GameFlag::MYCON_RAMBLE);
	switch (Counter++)
	{
		case 0:
			NPCPhrase(RAMBLE_1);
			break;
		case 1:
			NPCPhrase(RAMBLE_2);
			break;
		case 2:
			NPCPhrase(RAMBLE_3);
			break;
		case 3:
			NPCPhrase(RAMBLE_4);
			break;
		case 4:
			NPCPhrase(RAMBLE_5);
			break;
		case 5:
			NPCPhrase(RAMBLE_6);
			break;
		case 6:
			NPCPhrase(RAMBLE_7);
			break;
		case 7:
			NPCPhrase(RAMBLE_8);
			break;
		case 8:
			NPCPhrase(RAMBLE_9);
			break;
		case 9:
			NPCPhrase(RAMBLE_10);
			break;
		case 10:
			NPCPhrase(RAMBLE_11);
			break;
		case 11:
			NPCPhrase(RAMBLE_12);
			break;
		case 12:
			NPCPhrase(RAMBLE_13);
			break;
		case 13:
			NPCPhrase(RAMBLE_14);
			break;
		case 14:
			NPCPhrase(RAMBLE_15);
			break;
		case 15:
			NPCPhrase(RAMBLE_16);
			break;
		case 16:
			NPCPhrase(RAMBLE_17);
			break;
		case 17:
			NPCPhrase(RAMBLE_18);
			break;
		case 18:
			NPCPhrase(RAMBLE_19);
			break;
		case 19:
			NPCPhrase(RAMBLE_20);
			break;
		case 20:
			NPCPhrase(RAMBLE_21);
			break;
		case 21:
			NPCPhrase(RAMBLE_22);
			break;
		case 22:
			NPCPhrase(RAMBLE_23);
			break;
		case 23:
			NPCPhrase(RAMBLE_24);
			break;
		case 24:
			NPCPhrase(RAMBLE_25);
			if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 2)
			{
				uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
			}
			break;
		case 25:
			NPCPhrase(RAMBLE_26);
			break;
		case 26:
			NPCPhrase(RAMBLE_27);
			break;
		case 27:
			NPCPhrase(RAMBLE_28);
			break;
		case 28:
			NPCPhrase(RAMBLE_29);
			break;
		case 29:
			NPCPhrase(RAMBLE_30);
			break;
		case 30:
			NPCPhrase(RAMBLE_31);
			break;
		case 31:
			NPCPhrase(RAMBLE_32);
			Counter = 0;
			break;
	}
	uqm::gameState().setFlag(uqm::GameFlag::MYCON_RAMBLE, static_cast<uint32_t>(Counter));

	if (!(uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
	{
		if (!PlayerSaid(R, come_in_peace)
			&& !PlayerSaid(R, gonna_die))
		{
			Counter = (uqm::gameState().flag(uqm::GameFlag::MYCON_INSULTS) + 1) & 7;
			uqm::gameState().setFlag(uqm::GameFlag::MYCON_INSULTS, static_cast<uint32_t>(Counter));
			MadeChoice = 1;
		}
	}
	else if (!PlayerSaid(R, lets_be_friends)
			 && !PlayerSaid(R, came_to_homeworld)
			 && !PlayerSaid(R, submit_to_us))
	{
		Counter = (uqm::gameState().flag(uqm::GameFlag::MYCON_INFO) + 1) & 15;
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_INFO, static_cast<uint32_t>(Counter));
		MadeChoice = 1;
	}
}

static void
CombatIsInevitable(RESPONSE_REF R)
{
	setSegue(Segue_hostile);

	if (PlayerSaid(R, bye_space))
	{
		NPCPhrase(BYE_AND_DIE_SPACE);
	}
	else if (PlayerSaid(R, bye_homeworld))
	{
		NPCPhrase(BYE_AND_DIE_HOMEWORLD);
	}
	else if (PlayerSaid(R, like_to_land))
	{
		NPCPhrase(NEVER_LET_LAND);
	}
	else if (PlayerSaid(R, bye_sun_device))
	{
		NPCPhrase(GOODBYE_SUN_DEVICE);

		setSegue(Segue_peace);
	}
	else
	{
		DoRamble(R);
		if (!(uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
		{
			NPCPhrase(BYE_AND_DIE_SPACE);
		}
		else
		{
			NPCPhrase(BYE_AND_DIE_HOMEWORLD);
		}
	}
	MadeChoice = 0;
}

static void
SunDevice(RESPONSE_REF R)
{
	if (PlayerSaid(R, whats_up_sun_device))
	{
		NPCPhrase(GENERAL_INFO_SUN_DEVICE);

		DisablePhrase(whats_up_sun_device);
	}
	else if (PlayerSaid(R, how_goes_implanting))
	{
		NPCPhrase(UNFORSEEN_DELAYS);

		DisablePhrase(how_goes_implanting);
	}
	else if (PlayerSaid(R, i_have_a_cunning_plan))
	{
		NPCPhrase(WONT_FALL_FOR_TRICK);

		uqm::gameState().setFlag(uqm::GameFlag::NO_TRICK_AT_SUN, static_cast<uint32_t>(1));
	}

	if (PhraseEnabled(whats_up_sun_device))
	{
		Response(whats_up_sun_device, SunDevice);
	}
	if (uqm::gameState().flag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH))
	{
		if (PhraseEnabled(how_goes_implanting) && uqm::gameState().flag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH))
		{
			Response(how_goes_implanting, SunDevice);
		}
		Response(like_to_land, CombatIsInevitable);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::MYCON_AMBUSH)
			 && !uqm::gameState().flag(uqm::GameFlag::NO_TRICK_AT_SUN))
	{
		Response(i_have_a_cunning_plan, SunDevice);
	}
	Response(bye_sun_device, CombatIsInevitable);
}

static void
TrickMycon(RESPONSE_REF R)
{
	if (PlayerSaid(R, i_have_a_cunning_plan))
	{
		NPCPhrase(TELL_US_ABOUT_WORLD);

		DisablePhrase(i_have_a_cunning_plan);
	}
	else if (PlayerSaid(R, clue_1))
	{
		NPCPhrase(RESPONSE_1);

		DisablePhrase(clue_1);
	}
	else if (PlayerSaid(R, clue_2))
	{
		NPCPhrase(RESPONSE_2);

		DisablePhrase(clue_2);
	}
	else if (PlayerSaid(R, clue_3))
	{
		NPCPhrase(RESPONSE_3);

		DisablePhrase(clue_3);
	}

	if (PhraseEnabled(clue_1) == 0
		&& PhraseEnabled(clue_2) == 0
		&& PhraseEnabled(clue_3) == 0)
	{
		NPCPhrase(WE_GO_TO_IMPLANT);

		setSegue(Segue_peace);
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH, static_cast<uint32_t>(1));
		AddEvent(RELATIVE_EVENT, 0, 0, 0, ADVANCE_MYCON_MISSION);
	}
	else
	{
		NPCPhrase(AMBUSH_TAIL);
	}

	if (PhraseEnabled(clue_1))
	{
		Response(clue_1, TrickMycon);
	}
	if (PhraseEnabled(clue_2))
	{
		Response(clue_2, TrickMycon);
	}
	if (PhraseEnabled(clue_3))
	{
		Response(clue_3, TrickMycon);
	}
}

static void
NormalMycon(RESPONSE_REF R)
{
	RESPONSE_FUNC RespFunc {};

	if (PlayerSaid(R, what_about_shattered))
	{
		NPCPhrase(ABOUT_SHATTERED);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(2));
	}
	else if (R)
	{
		DoRamble(R);
		NPCPhrase(RAMBLE_TAIL);

		DisablePhrase(R);
	}

	if ((uint8_t)uqm::random().next() < 256 * 30 / 100)
	{
		RespFunc = (RESPONSE_FUNC)CombatIsInevitable;
	}
	else
	{
		RespFunc = (RESPONSE_FUNC)NormalMycon;
	}
	if (!(uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
	{
		if (PhraseEnabled(come_in_peace))
		{
			Response(come_in_peace, RespFunc);
		}
		if (PhraseEnabled(gonna_die))
		{
			Response(gonna_die, RespFunc);
		}
		if (!MadeChoice)
		{
			switch (uqm::gameState().flag(uqm::GameFlag::MYCON_INSULTS))
			{
				case 0:
					Response(insult_1, RespFunc);
					break;
				case 1:
					Response(insult_2, RespFunc);
					break;
				case 2:
					Response(insult_3, RespFunc);
					break;
				case 3:
					Response(insult_4, RespFunc);
					break;
				case 4:
					Response(insult_5, RespFunc);
					break;
				case 5:
					Response(insult_6, RespFunc);
					break;
				case 6:
					Response(insult_7, RespFunc);
					break;
				case 7:
					Response(insult_8, RespFunc);
					break;
			}
		}
		Response(bye_space, CombatIsInevitable);
	}
	else
	{
		if (!MadeChoice)
		{
			switch (uqm::gameState().flag(uqm::GameFlag::MYCON_INFO))
			{
				case 0:
					Response(question_1, RespFunc);
					break;
				case 1:
					Response(question_2, RespFunc);
					break;
				case 2:
					Response(question_3, RespFunc);
					break;
				case 3:
					Response(question_4, RespFunc);
					break;
				case 4:
					Response(question_5, RespFunc);
					break;
				case 5:
					Response(question_6, RespFunc);
					break;
				case 6:
					Response(question_7, RespFunc);
					break;
				case 7:
					Response(question_8, RespFunc);
					break;
				case 8:
					Response(question_9, RespFunc);
					break;
				case 9:
					Response(question_10, RespFunc);
					break;
				case 10:
					Response(question_11, RespFunc);
					break;
				case 11:
					Response(question_12, RespFunc);
					break;
				case 12:
					Response(question_13, RespFunc);
					break;
				case 13:
					Response(question_14, RespFunc);
					break;
				case 14:
					Response(question_15, RespFunc);
					break;
				case 15:
					Response(question_16, RespFunc);
					break;
			}
		}
		if (PhraseEnabled(lets_be_friends))
		{
			Response(lets_be_friends, RespFunc);
		}
		if (PhraseEnabled(came_to_homeworld))
		{
			Response(came_to_homeworld, RespFunc);
		}
		if (PhraseEnabled(submit_to_us))
		{
			Response(submit_to_us, RespFunc);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::MYCON_FELL_FOR_AMBUSH))
		{
			if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) == 1)
			{
				Response(what_about_shattered, NormalMycon);
			}
			if (uqm::gameState().flag(uqm::GameFlag::MYCON_AMBUSH))
			{
				Response(i_have_a_cunning_plan, TrickMycon);
			}
		}
		Response(bye_homeworld, CombatIsInevitable);
	}
}

static void
Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(DIE_THIEF);
				break;
			case 1:
				NPCPhrase(DIE_THIEF_AGAIN);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue_hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::MYCON_KNOW_AMBUSH))
	{
		NPCPhrase(DIE_LIAR);

		setSegue(Segue_hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_SUN_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HELLO_SUN_DEVICE_WORLD_1);
				SetHomeworldKnown(MYCON_HOME);
				break;
			case 1:
				NPCPhrase(HELLO_SUN_DEVICE_WORLD_2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_SUN_VISITS, static_cast<uint32_t>(NumVisits));

		SunDevice((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HELLO_HOMEWORLD_1);
				break;
			case 1:
				NPCPhrase(HELLO_HOMEWORLD_2);
				break;
			case 2:
				NPCPhrase(HELLO_HOMEWORLD_3);
				break;
			case 3:
				NPCPhrase(HELLO_HOMEWORLD_4);
				break;
			case 4:
				NPCPhrase(HELLO_HOMEWORLD_5);
				break;
			case 5:
				NPCPhrase(HELLO_HOMEWORLD_6);
				break;
			case 6:
				NPCPhrase(HELLO_HOMEWORLD_7);
				break;
			case 7:
				NPCPhrase(HELLO_HOMEWORLD_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		NormalMycon((RESPONSE_REF)0);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::MYCON_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(HELLO_SPACE_1);
				SetHomeworldKnown(MYCON_HOME);
				break;
			case 1:
				NPCPhrase(HELLO_SPACE_2);
				break;
			case 2:
				NPCPhrase(HELLO_SPACE_3);
				break;
			case 3:
				NPCPhrase(HELLO_SPACE_4);
				break;
			case 4:
				NPCPhrase(HELLO_SPACE_5);
				break;
			case 5:
				NPCPhrase(HELLO_SPACE_6);
				break;
			case 6:
				NPCPhrase(HELLO_SPACE_7);
				break;
			case 7:
				NPCPhrase(HELLO_SPACE_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MYCON_VISITS, static_cast<uint32_t>(NumVisits));

		NormalMycon((RESPONSE_REF)0);
	}
}

static uint16_t
uninit_mycon(void)
{
	uqm::lua().endComm();
	return (0);
}

static void
post_mycon_enc(void)
{
	// nothing defined so far
}

LOCDATA*
init_mycon_comm(void)
{
	mycon_desc.init_encounter_func = Intro;
	mycon_desc.post_encounter_func = post_mycon_enc;
	mycon_desc.uninit_encounter_func = uninit_mycon;

	uqm::lua().beginComm(nullptr, NullResource);

	mycon_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	mycon_desc.AlienTextBaseline.y = 0;
	mycon_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	MadeChoice = 0;

	if (uqm::lowByte(uqm::currentActivity()) != WON_LAST_BATTLE)
	{
		setSegue(Segue_hostile);
	}
	else
	{
		setSegue(Segue_peace);
	}
	LOCDATA* retval {&mycon_desc};

	return (retval);
}
