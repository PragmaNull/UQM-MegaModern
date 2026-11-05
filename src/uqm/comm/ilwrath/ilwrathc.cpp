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
#include "uqm/build.h"
#include "uqm/races.h"
#include "uqm/lua/UQMLua.h"


static LOCDATA ilwrath_desc =
	{
		ILWRATH_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		IlwrathPmapAnim, /* AlienFrame */
		IlwrathFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_MIDDLE, /* AlienTextValign */
		IlwrathColorMap, /* AlienColorMap */
		IlwrathMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		IlwrathConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				6,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				11,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				21,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
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

static void
CombatIsInevitable(RESPONSE_REF R)
{
	const bool isGTFO = PlayerSaid(R, gtfo);
	setSegue(Segue_hostile);

	if (PlayerSaid(R, you_are_weak))
	{
		NPCPhrase(STRENGTH_NOT_ALL);
	}
	else if (PlayerSaid(R, slay_by_thousands))
	{
		NPCPhrase(NO_SLAY_BY_THOUSANDS);
	}
	else if (PlayerSaid(R, ease_up))
	{
		NPCPhrase(NO_EASE_UP);
	}
	else if (PlayerSaid(R, bye_space))
	{
		NPCPhrase(GOODBYE_AND_DIE_SPACE);
	}
	else if (PlayerSaid(R, bye_homeworld))
	{
		NPCPhrase(GOODBYE_AND_DIE_HOMEWORLD);
	}
	else if (PlayerSaid(R, want_peace))
	{
		NPCPhrase(NO_PEACE);
	}
	else if (PlayerSaid(R, want_alliance))
	{
		NPCPhrase(NO_ALLIANCE);
	}
	else if (PlayerSaid(R, but_evil_is_defined))
	{
		NPCPhrase(DONT_CONFUSE_US);
	}
	else if (PlayerSaid(R, bye_gods) || isGTFO)
	{
		NPCPhrase(GOODBYE_GODS);

		GTFO = isGTFO;

		setSegue(Segue_peace);
	}
	if (PlayerSaid(R, whats_up))
	{
		NPCPhrase(GENERAL_INFO);
		Response(bye, CombatIsInevitable);
	}
	else if (PlayerSaid(R, whats_up_space_1)
			 || PlayerSaid(R, whats_up_space_2)
			 || PlayerSaid(R, whats_up_space_3)
			 || PlayerSaid(R, whats_up_space_4)
			 || PlayerSaid(R, whats_up_space_5))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_INFO);
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
				SetHomeworldKnown(ILWRATH_HOME);
				break;
			case 3:
				NPCPhrase(GENERAL_INFO_SPACE_4);
				break;
			case 4:
				NPCPhrase(GENERAL_INFO_SPACE_5);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_INFO, static_cast<uint32_t>(NumVisits));
	}
	else
	{
		if (PlayerSaid(R, bye))
		{
			NPCPhrase(GOODBYE_AND_DIE);
		}
		else if (PlayerSaid(R, where_you_come_from))
		{
			NPCPhrase(CAME_FROM);
		}
		if (PlayerSaid(R, it_will_be_a_pleasure))
		{
			NPCPhrase(WHO_BLASTS_WHO);
		}
		if (PlayerSaid(R, surrender))
		{
			NPCPhrase(NO_SURRENDER);
		}
		if (PlayerSaid(R, be_reasonable))
		{
			NPCPhrase(NOT_REASONABLE);
		}
	}
}

static void IlwrathHome(RESPONSE_REF R);

static void
IlwrathGods(RESPONSE_REF R)
{
	uint8_t GodsLeft {false};
	if (PlayerSaid(R, want_info_on_gods))
	{
		NPCPhrase(SO_MUCH_TO_KNOW);
	}
	else if (PlayerSaid(R, when_start_worship))
	{
		NPCPhrase(LONG_AGO);

		DISABLE_PHRASE(when_start_worship);
	}
	else if (PlayerSaid(R, any_good_gods))
	{
		NPCPhrase(KILLED_GOOD_GODS);

		DISABLE_PHRASE(any_good_gods);
	}
	else if (PlayerSaid(R, how_talk_with_gods))
	{
		NPCPhrase(CHANNEL_44);

		DISABLE_PHRASE(how_talk_with_gods);
	}
	else if (PlayerSaid(R, why_44))
	{
		NPCPhrase(BECAUSE_44);

		DISABLE_PHRASE(why_44);
	}

	if (PhraseEnabled(when_start_worship))
	{
		Response(when_start_worship, IlwrathGods);
		GodsLeft = true;
	}
	if (PhraseEnabled(any_good_gods))
	{
		Response(any_good_gods, IlwrathGods);
		GodsLeft = true;
	}
	if (PhraseEnabled(how_talk_with_gods))
	{
		Response(how_talk_with_gods, IlwrathGods);
		GodsLeft = true;
	}
	else if (PhraseEnabled(why_44))
	{
		Response(why_44, IlwrathGods);
		GodsLeft = true;
	}
	Response(enough_gods, IlwrathHome);

	if (!GodsLeft)
	{
		DISABLE_PHRASE(want_info_on_gods);
	}
}

static void
IlwrathInfo(RESPONSE_REF R)
{
	uint8_t InfoLeft {false};
	if (PlayerSaid(R, want_info_on_ilwrath))
	{
		NPCPhrase(WHAT_ABOUT_ILWRATH);
	}
	else if (PlayerSaid(R, what_about_physio))
	{
		NPCPhrase(ABOUT_PHYSIO);

		DISABLE_PHRASE(what_about_physio);
	}
	else if (PlayerSaid(R, what_about_history))
	{
		NPCPhrase(ABOUT_HISTORY);

		DISABLE_PHRASE(what_about_history);
	}
	else if (PlayerSaid(R, what_about_culture))
	{
		NPCPhrase(ABOUT_CULTURE);

		DISABLE_PHRASE(what_about_culture);
	}
	else if (PlayerSaid(R, what_about_urquan))
	{
		NPCPhrase(URQUAN_TOO_NICE);

		DISABLE_PHRASE(what_about_urquan);
	}
	else if (PlayerSaid(R, are_you_evil))
	{
		NPCPhrase(OF_COURSE_WERE_EVIL);

		DISABLE_PHRASE(are_you_evil);
	}

	if (PhraseEnabled(what_about_physio))
	{
		Response(what_about_physio, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(what_about_history))
	{
		Response(what_about_history, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(what_about_culture))
	{
		Response(what_about_culture, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(what_about_urquan))
	{
		Response(what_about_urquan, IlwrathInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(are_you_evil))
	{
		Response(are_you_evil, IlwrathInfo);
		InfoLeft = true;
	}
	else
	{
		Response(but_evil_is_defined, CombatIsInevitable);
		InfoLeft = true;
	}
	Response(enough_ilwrath, IlwrathHome);

	if (!InfoLeft)
	{
		DISABLE_PHRASE(want_info_on_ilwrath);
	}
}

static void
IlwrathHome(RESPONSE_REF R)
{
	if (R == 0)
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INIT_HOME_HELLO);
				break;
			case 1:
				NPCPhrase(SUBSEQUENT_HOME_HELLO);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_HOME_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, enough_gods))
	{
		NPCPhrase(OK_ENOUGH_GODS);
	}
	else if (PlayerSaid(R, enough_ilwrath))
	{
		NPCPhrase(OK_ENOUGH_ILWRATH);
	}

	if (PhraseEnabled(want_info_on_gods))
	{
		Response(want_info_on_gods, IlwrathGods);
	}
	if (PhraseEnabled(want_info_on_ilwrath))
	{
		Response(want_info_on_ilwrath, IlwrathInfo);
	}
	Response(want_peace, CombatIsInevitable);
	Response(want_alliance, CombatIsInevitable);
	Response(bye_homeworld, CombatIsInevitable);
}

static void GodsSpeak(RESPONSE_REF R);

static void
GodsOrder(RESPONSE_REF R)
{
	uint8_t OrdersLeft {false};
	if (PlayerSaid(R, other_divine_orders))
	{
		NPCPhrase(WHAT_ORDERS);
	}
	else if (PlayerSaid(R, say_warship))
	{
		NPCPhrase(OK_WARSHIP);

		DISABLE_PHRASE(say_warship);
	}
	else if (PlayerSaid(R, say_dwe))
	{
		NPCPhrase(OK_DWE);

		DISABLE_PHRASE(say_dwe);
	}
	else if (PlayerSaid(R, say_youboo))
	{
		NPCPhrase(OK_YOUBOO);

		DISABLE_PHRASE(say_youboo);
	}
	else if (PlayerSaid(R, say_dillrat))
	{
		NPCPhrase(OK_DILRAT);

		DISABLE_PHRASE(say_dillrat);
	}

	if (PhraseEnabled(say_warship))
	{
		Response(say_warship, GodsOrder);
		OrdersLeft = true;
	}
	if (PhraseEnabled(say_dwe))
	{
		Response(say_dwe, GodsOrder);
		OrdersLeft = true;
	}
	if (PhraseEnabled(say_youboo))
	{
		Response(say_youboo, GodsOrder);
		OrdersLeft = true;
	}
	if (PhraseEnabled(say_dillrat))
	{
		Response(say_dillrat, GodsOrder);
		OrdersLeft = true;
	}
	Response(enough_orders, GodsSpeak);

	if (!OrdersLeft)
	{
		DISABLE_PHRASE(other_divine_orders);
	}
}

static void
GodsSpeak(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (R == 0)
	{
		if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
		{
			NPCPhrase(GLORIOUS_WORSHIP);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			NPCPhrase(ON_WAY);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_GODS_SPOKEN);

			if (NumVisits > 3)
			{
				NumVisits = 0;
			}

			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ILWRATH_BELIEVE);
					break;
				case 1:
					NPCPhrase(GODS_RETURN_1);
					break;
				case 2:
					NPCPhrase(GODS_RETURN_2);
					break;
				case 3:
					NPCPhrase(GODS_RETURN_3);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_GODS_SPOKEN, static_cast<uint32_t>(NumVisits));
		}
	}
	else if (PlayerSaid(R, go_kill_thraddash))
	{
		NPCPhrase(OK_KILL_THRADDASH);

		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_DECEIVED, static_cast<uint32_t>(1));
		AddEvent(RELATIVE_EVENT, 0, 0, 0, ADVANCE_ILWRATH_MISSION);
	}
	else if (PlayerSaid(R, worship_us))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_WORSHIP);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(WE_WORSHIP_1);
				break;
			case 1:
				NPCPhrase(WE_WORSHIP_2);
				break;
			case 2:
				NPCPhrase(WE_WORSHIP_3);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_WORSHIP, static_cast<uint32_t>(NumVisits));

		DISABLE_PHRASE(worship_us);
	}
	else if (PlayerSaid(R, enough_orders))
	{
		NPCPhrase(NEVER_ENOUGH);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
	{
		Response(go_kill_thraddash, GodsSpeak);
	}
	if (PhraseEnabled(worship_us))
	{
		Response(worship_us, GodsSpeak);
	}
	if (PhraseEnabled(other_divine_orders))
	{
		Response(other_divine_orders, GodsOrder);
	}
	Response(bye_gods, CombatIsInevitable);
}

static void
IlwrathSpace(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (R == 0)
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INIT_HELLO_SPACE);
				break;
			case 1:
				NPCPhrase(SUBSEQUENT_HELLO_SPACE_1);
				break;
			case 2:
				NPCPhrase(SUBSEQUENT_HELLO_SPACE_2);
				break;
			case 3:
				NPCPhrase(SUBSEQUENT_HELLO_SPACE_3);
				break;
			case 4:
				NPCPhrase(SUBSEQUENT_HELLO_SPACE_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_VISITS, static_cast<uint32_t>(NumVisits));
	}

	NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_INFO);
	switch (NumVisits)
	{
		case 0:
			Response(whats_up_space_1, CombatIsInevitable);
			break;
		case 1:
			Response(whats_up_space_2, CombatIsInevitable);
			break;
		case 2:
			Response(whats_up_space_3, CombatIsInevitable);
			break;
		case 3:
			Response(whats_up_space_4, CombatIsInevitable);
			break;
		case 4:
			Response(whats_up_space_5, CombatIsInevitable);
			break;
	}
	Response(you_are_weak, CombatIsInevitable);
	Response(slay_by_thousands, CombatIsInevitable);
	Response(ease_up, CombatIsInevitable);
	Response(bye_space, CombatIsInevitable);
}

static void
Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::gameState().flag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER))
	{
		NPCPhrase(SEND_MESSAGE);

		Response(where_you_come_from, CombatIsInevitable);
		Response(it_will_be_a_pleasure, CombatIsInevitable);
		Response(surrender, CombatIsInevitable);
		Response(be_reasonable, CombatIsInevitable);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		IlwrathHome((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_CHMMR_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(INIT_CHMMR_HELLO);
				break;
			case 1:
				NPCPhrase(SUBSEQUENT_CHMMR_HELLO);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_CHMMR_VISITS, static_cast<uint32_t>(NumVisits));

		Response(whats_up, CombatIsInevitable);
		Response(bye, CombatIsInevitable);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 5))
	{
		// Communicating with an Ilwrath ship using a HyperWave Broadcaster.
		if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
		{
			NPCPhrase(BIG_FUN);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			NPCPhrase(FAST_AS_CAN);
		}
		else
		{
			if (IsExtended())
			{
				NumVisits = uqm::gameState().flag(uqm::GameFlag::ILWRATH_GODS_SPOKEN);

				if (NumVisits < 5)
				{
					Response(gtfo, CombatIsInevitable);
					Response(bye_gods, CombatIsInevitable);
				}

				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(JUST_GRUNTS);
						break;
					case 1:
						NPCPhrase(ILWRATH_BELIEVE);
						break;
					case 2:
						NPCPhrase(GODS_RETURN_1);
						break;
					case 3:
						NPCPhrase(GODS_RETURN_2);
						break;
					case 4:
						NPCPhrase(GODS_RETURN_3);
						break;
					case 5:
						NPCPhrase(DECEIVERS);
						GTFO = 2;
						--NumVisits;
						break;
				}
				uqm::gameState().setFlag(uqm::GameFlag::ILWRATH_GODS_SPOKEN, static_cast<uint32_t>(NumVisits));
			}
			else
			{
				NPCPhrase(JUST_GRUNTS);
			}

			setSegue(Segue_peace);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 4))
	{
		// Communicating with the Ilwrath homeworld using a
		// Hyperwave Broadcaster.
		GodsSpeak((RESPONSE_REF)0);
	}
	else
	{
		setSegue(Segue_peace);

		if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_FIGHT_THRADDASH))
		{
			NPCPhrase(HAPPY_FIGHTING_THRADDASH);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::ILWRATH_DECEIVED))
		{
			NPCPhrase(ON_WAY_TO_THRADDASH);
		}
		else
		{
			IlwrathSpace((RESPONSE_REF)0);
		}
	}
}

static uint16_t
uninit_ilwrath(void)
{
	uqm::lua().endComm();
	return (0);
}

static void
post_ilwrath_enc(void)
{
	// nothing defined so far
}

LOCDATA*
init_ilwrath_comm(void)
{
	ilwrath_desc.init_encounter_func = Intro;
	ilwrath_desc.post_encounter_func = post_ilwrath_enc;
	ilwrath_desc.uninit_encounter_func = uninit_ilwrath;

	uqm::lua().beginComm(nullptr, NullResource);

	ilwrath_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	ilwrath_desc.AlienTextBaseline.y = ResScale(64);
	ilwrath_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::gameState().flag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER)
		|| (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA)
			& ((1 << 4) | (1 << 5)))
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		setSegue(Segue_peace);
	}
	else
	{
		setSegue(Segue_hostile);
	}
	LOCDATA* retval {&ilwrath_desc};

	return (retval);
}
