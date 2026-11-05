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
#include "uqm/units.h"
#include "uqm/setup.h"
#include "uqm/sis.h"
// for DeltaSISGauges(), DrawLanders()
#include "libs/graphics/gfx_common.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/settings.h"
#include "uqm/nameref.h"
#include "libs/math/Random.h"
#include "uqm/random/UQMRandom.h"

static LOCDATA commander_desc =
	{
		COMMANDER_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		CommanderPmapAnim, /* AlienFrame */
		CommanderFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth(), */  /* AlienTextWidth */
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_MIDDLE, /* AlienTextValign */
		CommanderColorMap, /* AlienColorMap */
		CommanderMusic, /* AlienSong */
		{
								CommanderInitPmapAnim, /* AlienAltFrame */
			NullResource,		   /* AlienAltColorMap */
			CommanderLowpowMusic,  /* AlienAltSong */
		},
		CommanderConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				/* Blink */
				1,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Running light */
				10,			  /* StartIndex */
				30,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 2,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				/* SD "rave party" */
				1, /* StartIndex */
				3, /* NumFrames */
				RandomAnim | ColorxformAnim
					| AnimDisabled, /* AnimFlags */
				0,
				GameTicksPerSecond / 30, /* FrameRate */
				0,
				GameTicksPerSecond / 15, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				/* HdResolutionFactor static distortion */
				40, /* StartIndex */
				16, /* NumFrames */
				RandomAnim | AnimDisabled
					| ImmumeToRestart, /* AnimFlags */
				0,
				GameTicksPerSecond / 5, /* FrameRate */
				0,
				GameTicksPerSecond / 4, /* RestartRate */
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
			4, /* StartIndex */
			6, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 10,
								GameTicksPerSecond / 15, /* FrameRate */
			GameTicksPerSecond * 7 / 60,
								GameTicksPerSecond / 12, /* RestartRate */
			0,						 /* BlockMask */
		},
		nullptr, /* AlienNumberSpeech - none */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};

static FILTER_DESC commander_filters =
	{
		2, /* Number of filters */
		{
								  /* Filter array */
			{
				0,				/* Color index */
				1,				/* Opacity index */
				56,				/* Frame index */
				DRAW_MULTIPLY,	/* DrawKind*/
				FilterDisabled, /* Flags */
			},
								  {
				2,			  /* Color index */
				3,			  /* Opacity index */
				56,			  /* Frame index */
				DRAW_OVERLAY, /* DrawKind*/
				TurnOffOft,	  /* Flags */
			},
								  }
};

static void
RaveParty(void)
{
	CommData.AlienColorMapIndex = (uqm::random().next() % 3) + 1;

	if (IsHd() && IsExtended())
	{
		CommData.AlienFrame = SetAbsFrameIndex(CommData.AlienFrame,
											   (uqm::random().next() % 15) + 40);
		SwitchSequences(false);
		EnableTalkingAnim(false);
		CommData.AlienAmbientArray[3].AnimFlags &= ~AnimDisabled;
	}
	else
	{
		CommData.AlienAmbientArray[2].AnimFlags &= ~AnimDisabled;

		if (IsHd())
		{
			EngageFilters(&commander_filters);
		}
	}
}

static void
ByeBye(RESPONSE_REF R)
{
	if (PlayerSaid(R, ok_i_will_get_radios))
	{
		NPCPhrase(THANKS_FOR_HELPING);
	}
	else if (PlayerSaid(R, well_go_get_them_now))
	{
		NPCPhrase(GLAD_WHEN_YOU_COME_BACK);
	}
	else if (PlayerSaid(R, we_will_take_care_of_base))
	{
		NPCPhrase(GOOD_LUCK_WITH_BASE);

		SET_GAME_STATE(WILL_DESTROY_BASE, 1);
	}
	else if (PlayerSaid(R, take_care_of_base_again))
	{
		NPCPhrase(GOOD_LUCK_AGAIN);
	}
	else if (PlayerSaid(R, base_was_abandoned)
			 || PlayerSaid(R, i_lied_it_was_abandoned))
	{
		NPCPhrase(IT_WAS_ABANDONED);
		NPCPhrase(HERE_COMES_ILWRATH);

		SET_GAME_STATE(PROBE_ILWRATH_ENCOUNTER, 1);
	}
	else if (PlayerSaid(R, oh_yes_big_fight))
	{
		NPCPhrase(IM_GLAD_YOU_WON);
		NPCPhrase(HERE_COMES_ILWRATH);

		SET_GAME_STATE(PROBE_ILWRATH_ENCOUNTER, 1);
	}
	else if (PlayerSaid(R, i_cant_talk_about_it))
	{
		NPCPhrase(IM_SURE_IT_WAS_DIFFICULT);
		NPCPhrase(HERE_COMES_ILWRATH);

		SET_GAME_STATE(PROBE_ILWRATH_ENCOUNTER, 1);
	}
	else if (PlayerSaid(R, cook_their_butts)
			 || PlayerSaid(R, overthrow_evil_aliens)
			 || PlayerSaid(R, annihilate_those_monsters))
	{
		SET_GAME_STATE(PROBE_ILWRATH_ENCOUNTER, 0);

		if (PlayerSaid(R, cook_their_butts))
		{
			NPCPhrase(COOK_BUTTS);
		}
		else if (PlayerSaid(R, overthrow_evil_aliens))
		{
			NPCPhrase(OVERTHROW_ALIENS);
		}
		else /* if (R == annihilate_those_monsters) */
		{
			NPCPhrase(KILL_MONSTERS);
		}

		NPCPhrase(THIS_MAY_SEEM_SILLY);

		Response(name_1, ByeBye);
		Response(name_2, ByeBye);
		Response(name_3, ByeBye);
		Response(name_4, ByeBye);

		SET_GAME_STATE(STARBASE_AVAILABLE, 1);
	}
	else
	{
		if (PlayerSaid(R, name_1))
		{
			NPCPhrase(OK_THE_NAFS);

			SET_GAME_STATE(NEW_ALLIANCE_NAME, 0);
		}
		else if (PlayerSaid(R, name_2))
		{
			NPCPhrase(OK_THE_CAN);

			SET_GAME_STATE(NEW_ALLIANCE_NAME, 1);
		}
		else if (PlayerSaid(R, name_3))
		{
			NPCPhrase(OK_THE_UFW);

			SET_GAME_STATE(NEW_ALLIANCE_NAME, 2);
		}
		else /* if (PlayerSaid (R, name_4)) */
		{
			NPCPhrase(OK_THE_NAME_IS_EMPIRE);

			SET_GAME_STATE(NEW_ALLIANCE_NAME, 3);
		}

		NPCPhrase(STARBASE_WILL_BE_READY);
	}
}

static void GiveRadios(RESPONSE_REF R);

static void
NoRadioactives(RESPONSE_REF R)
{
	if (PlayerSaid(R, yes_this_is_supply_ship))
	{
		NPCPhrase(ABOUT_TIME);

		if (GLOBAL_SIS(ElementAmounts[RADIOACTIVE]))
		{
			GiveRadios(0);
		}
		else
		{
			Response(i_lied, NoRadioactives);
			Response(plumb_out, NoRadioactives);
		}
	}
	else
	{
		if (PlayerSaid(R, where_can_i_get_radios))
		{
			NPCPhrase(RADIOS_ON_MERCURY);

			DISABLE_PHRASE(where_can_i_get_radios);
		}
		else if (PlayerSaid(R, no_but_well_help))
		{
			NPCPhrase(THE_WHAT_FROM_WHERE);
		}
		else if (PlayerSaid(R, what_slave_planet)
				 || PlayerSaid(R, i_lied))
		{
			NPCPhrase(DONT_KNOW_WHO_YOU_ARE);
		}
		else if (PlayerSaid(R, plumb_out))
		{
			NPCPhrase(WHAT_KIND_OF_IDIOT);
		}
		else if (PlayerSaid(R, i_lost_my_lander))
		{
			NPCPhrase(HERE_IS_A_NEW_LANDER);
			++GLOBAL_SIS(NumLanders);
			DrawLanders();
			DeltaSISGauges(4, 0, 0);

			SET_GAME_STATE(LANDERS_LOST, 1);
		}
		else if (PlayerSaid(R, i_lost_another_lander))
		{
			NPCPhrase(HERE_IS_ANOTHER_LANDER);
			++GLOBAL_SIS(NumLanders);
			DrawLanders();
			DeltaSISGauges(4, 0, 0);
		}
		else if (PlayerSaid(R, need_fuel_mercury) || PlayerSaid(R, need_fuel_luna))
		{
			NPCPhrase(GIVE_FUEL);
			DeltaSISGauges(0, 5 * FuelTankScale, 0);

			SET_GAME_STATE(GIVEN_FUEL_BEFORE, 1);
		}
		else if (PlayerSaid(R, need_fuel_again))
		{
			NPCPhrase(GIVE_FUEL_AGAIN);
			DeltaSISGauges(0, 5 * FuelTankScale, 0);
		}

		if (GLOBAL_SIS(ElementAmounts[RADIOACTIVE]))
		{
			GiveRadios(0);
		}
		else
		{
			if (GLOBAL_SIS(NumLanders) == 0
				&& GET_GAME_STATE(CHMMR_BOMB_STATE) < 2)
			{
				if (GET_GAME_STATE(LANDERS_LOST))
				{
					Response(i_lost_another_lander, NoRadioactives);
				}
				else
				{
					Response(i_lost_my_lander, NoRadioactives);
				}
			}
			if (GLOBAL_SIS(FuelOnBoard) < 2 * FuelTankScale)
			{
				if (GET_GAME_STATE(GIVEN_FUEL_BEFORE))
				{
					Response(need_fuel_again, NoRadioactives);
				}
				else
				{
					Response(need_fuel_mercury, NoRadioactives);
				}
			}

			Response(ok_i_will_get_radios, ByeBye);
			if (PhraseEnabled(where_can_i_get_radios))
			{
				Response(where_can_i_get_radios, NoRadioactives);
			}
		}
	}
}

static void
AskAfterRadios(RESPONSE_REF R)
{
	if (PlayerSaid(R, i_lost_my_lander))
	{
		NPCPhrase(HERE_IS_A_NEW_LANDER);
		++GLOBAL_SIS(NumLanders);
		DrawLanders();
		DeltaSISGauges(4, 0, 0);

		SET_GAME_STATE(LANDERS_LOST, 1);
	}
	else if (PlayerSaid(R, i_lost_another_lander))
	{
		NPCPhrase(HERE_IS_ANOTHER_LANDER);
		++GLOBAL_SIS(NumLanders);
		DrawLanders();
		DeltaSISGauges(4, 0, 0);
	}
	else if (PlayerSaid(R, need_fuel_mercury) || PlayerSaid(R, need_fuel_luna))
	{
		NPCPhrase(GIVE_FUEL);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);

		SET_GAME_STATE(GIVEN_FUEL_BEFORE, 1);
	}
	else if (PlayerSaid(R, need_fuel_again))
	{
		NPCPhrase(GIVE_FUEL_AGAIN);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);
	}
	else if (PlayerSaid(R, where_get_radios))
	{
		NPCPhrase(RADIOS_ON_MERCURY);

		DISABLE_PHRASE(where_get_radios);
	}

	{
		if (GLOBAL_SIS(NumLanders) == 0
			&& GET_GAME_STATE(CHMMR_BOMB_STATE) < 2)
		{
			if (GET_GAME_STATE(LANDERS_LOST))
			{
				Response(i_lost_another_lander, AskAfterRadios);
			}
			else
			{
				Response(i_lost_my_lander, AskAfterRadios);
			}
		}
		if (GLOBAL_SIS(FuelOnBoard) < 2 * FuelTankScale)
		{
			if (GET_GAME_STATE(GIVEN_FUEL_BEFORE))
			{
				Response(need_fuel_again, AskAfterRadios);
			}
			else
			{
				Response(need_fuel_mercury, AskAfterRadios);
			}
		}
		Response(well_go_get_them_now, ByeBye);
		if (PhraseEnabled(where_get_radios))
		{
			Response(where_get_radios, AskAfterRadios);
		}
	}
}

static void
BaseDestroyed(RESPONSE_REF R)
{
	if (PlayerSaid(R, we_fought_them))
	{
		NPCPhrase(YOU_REALLY_FOUGHT_BASE);

		Response(oh_yes_big_fight, ByeBye);
		Response(i_lied_it_was_abandoned, ByeBye);
		Response(i_cant_talk_about_it, ByeBye);
	}
	else
	{
		if (PlayerSaid(R, we_are_here_to_help))
		{
			NPCPhrase(BASE_ON_MOON);
		}
		else
		{
			NPCPhrase(DEALT_WITH_BASE_YET);
		}

		Response(base_was_abandoned, ByeBye);
		Response(we_fought_them, BaseDestroyed);
	}
}

static void
TellMoonBase(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(DEALT_WITH_BASE_YET);
	}
	else if (PlayerSaid(R, i_lost_my_lander))
	{
		NPCPhrase(HERE_IS_A_NEW_LANDER);
		++GLOBAL_SIS(NumLanders);
		DrawLanders();
		DeltaSISGauges(4, 0, 0);

		SET_GAME_STATE(LANDERS_LOST, 1);
	}
	else if (PlayerSaid(R, i_lost_another_lander))
	{
		NPCPhrase(HERE_IS_ANOTHER_LANDER);
		++GLOBAL_SIS(NumLanders);
		DrawLanders();
		DeltaSISGauges(4, 0, 0);
	}
	else if (PlayerSaid(R, need_fuel_mercury) || PlayerSaid(R, need_fuel_luna))
	{
		NPCPhrase(GIVE_FUEL);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);

		SET_GAME_STATE(GIVEN_FUEL_BEFORE, 1);
	}
	else if (PlayerSaid(R, need_fuel_again))
	{
		NPCPhrase(GIVE_FUEL_AGAIN);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);
	}
	else if (PlayerSaid(R, we_are_here_to_help))
	{
		NPCPhrase(BASE_ON_MOON);
	}
	else if (GET_GAME_STATE(STARBASE_YACK_STACK1) == 0)
	{
		NPCPhrase(ABOUT_BASE);

		SET_GAME_STATE(STARBASE_YACK_STACK1, 1);
	}
	else
	{
		NPCPhrase(ABOUT_BASE_AGAIN);
	}

	if (GLOBAL_SIS(NumLanders) == 0
		&& GET_GAME_STATE(CHMMR_BOMB_STATE) < 2)
	{
		if (GET_GAME_STATE(LANDERS_LOST))
		{
			Response(i_lost_another_lander, TellMoonBase);
		}
		else
		{
			Response(i_lost_my_lander, TellMoonBase);
		}
	}
	if (GLOBAL_SIS(FuelOnBoard) < 2 * FuelTankScale)
	{
		if (GET_GAME_STATE(GIVEN_FUEL_BEFORE))
		{
			Response(need_fuel_again, TellMoonBase);
		}
		else
		{
			Response(need_fuel_luna, TellMoonBase);
		}
	}
	if (GET_GAME_STATE(WILL_DESTROY_BASE) == 0)
	{
		Response(we_will_take_care_of_base, ByeBye);
	}
	else
	{
		Response(take_care_of_base_again, ByeBye);
	}
	if (GET_GAME_STATE(STARBASE_YACK_STACK1) == 0)
	{
		Response(tell_me_about_base, TellMoonBase);
	}
	else
	{
		Response(tell_me_again, TellMoonBase);
	}
}

static void RevealSelf(RESPONSE_REF R);

static void
TellProbe(RESPONSE_REF R)
{
	(void)R; // ignored
	NPCPhrase(THAT_WAS_PROBE);
	DISABLE_PHRASE(what_was_red_thing);

	Response(it_went_away, RevealSelf);
	Response(we_destroyed_it, RevealSelf);
	Response(what_probe, RevealSelf);
}

static void
RevealSelf(RESPONSE_REF R)
{
	uint8_t stack {0};
	if (PlayerSaid(R, we_are_vindicator))
	{
		NPCPhrase(THATS_IMPOSSIBLE);

		DISABLE_PHRASE(we_are_vindicator);
	}
	else if (PlayerSaid(R, our_mission_was_secret))
	{
		NPCPhrase(ACKNOWLEDGE_SECRET);

		DISABLE_PHRASE(our_mission_was_secret);
	}
	else if (PlayerSaid(R, first_give_info))
	{
		NPCPhrase(ASK_AWAY);

		stack = 1;
		DISABLE_PHRASE(first_give_info);
	}
	else if (PlayerSaid(R, whats_this_starbase))
	{
		NPCPhrase(STARBASE_IS);

		stack = 1;
		DISABLE_PHRASE(whats_this_starbase);
	}
	else if (PlayerSaid(R, what_about_earth))
	{
		NPCPhrase(HAPPENED_TO_EARTH);

		stack = 1;
		DISABLE_PHRASE(what_about_earth);
	}
	else if (PlayerSaid(R, where_are_urquan))
	{
		NPCPhrase(URQUAN_LEFT);

		stack = 1;
		DISABLE_PHRASE(where_are_urquan);
	}
	else if (PlayerSaid(R, it_went_away))
	{
		NPCPhrase(DEEP_TROUBLE);
	}
	else if (PlayerSaid(R, we_destroyed_it))
	{
		NPCPhrase(GOOD_NEWS);
	}
	else if (PlayerSaid(R, what_probe))
	{
		NPCPhrase(SURE_HOPE);
	}

	for (uint8_t i {0}; i < 2; ++i, stack ^= 1)
	{
		if (stack == 1)
		{
			if (PhraseEnabled(first_give_info))
			{
				Response(first_give_info, RevealSelf);
			}
			else if (PhraseEnabled(whats_this_starbase))
			{
				Response(whats_this_starbase, RevealSelf);
			}
			else if (PhraseEnabled(what_about_earth))
			{
				Response(what_about_earth, RevealSelf);
			}
			else if (PhraseEnabled(where_are_urquan))
			{
				Response(where_are_urquan, RevealSelf);
			}
			else if (PhraseEnabled(what_was_red_thing))
			{
				Response(what_was_red_thing, TellProbe);
			}
		}
		else
		{
			if (PhraseEnabled(we_are_vindicator))
			{
				Response(we_are_vindicator, RevealSelf);
			}
			else if (PhraseEnabled(our_mission_was_secret))
			{
				Response(our_mission_was_secret, RevealSelf);
			}
			else
			{
				if (GET_GAME_STATE(MOONBASE_DESTROYED) == 0)
				{
					Response(we_are_here_to_help, TellMoonBase);
				}
				else
				{
					Response(we_are_here_to_help, BaseDestroyed);
				}
			}
		}
	}
}

static void
GiveRadios(RESPONSE_REF R)
{
	if (PlayerSaid(R, we_will_transfer_now))
	{
		SET_GAME_STATE(RADIOACTIVES_PROVIDED, 1);

		NPCPhrase(FUEL_UP0);
		NPCPhrase(FUEL_UP1);
		AlienTalkSegue(1);

		CommData.AlienAmbientArray[2].AnimFlags |= AnimDisabled;

		if (IsHd() && IsExtended())
		{ // Disable noisy static animation in hi-res.
			CommData.AlienFrame =
				SetAbsFrameIndex(CommData.AlienFrame, 0);

			CommData.AlienAmbientArray[0].AnimFlags &= ~AnimDisabled;
			CommData.AlienAmbientArray[1].AnimFlags &= ~AnimDisabled;
			CommData.AlienAmbientArray[3].AnimFlags |= AnimDisabled;
			EnableTalkingAnim(true);

			EngageFilters(&commander_filters);
			FilterData.FilterArray[0].Flags = TurnOffOft;
			FilterData.FilterArray[1].Flags |= FilterDisabled;
		}

		if (IsHd() && !IsExtended())
		{
			FilterData.FilterArray[0].Flags = TurnOffOft;
			FilterData.FilterArray[1].Flags = FilterDisabled;
		}

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 0),
					  c_gameSecond / 2);

		if (altResFlags & UseAltSong)
		{
			const uint32_t MusicPos = PLRGetPos();
			StopMusic();
			CommData.AlienSong = LoadMusicInstance(CommData.AlienSongRes);

			SetMusicVolume(MuteVolume);
			PlayMusic(CommData.AlienSong, true, 1);
			SeekMusic(MusicPos);
			FadeMusic(NormalVolume, c_gameSecond);

			altResFlags &= ~UseAltSong;
		}

		AlienTalkSegue((uint16_t)~0);

		RevealSelf(0);
	}
	else
	{
		if (PlayerSaid(R, what_will_you_give_us))
		{
			NPCPhrase(MESSAGE_GARBLED_1);
		}
		else if (PlayerSaid(R, before_radios_we_need_info))
		{
			NPCPhrase(MESSAGE_GARBLED_2);
		}

		Response(we_will_transfer_now, GiveRadios);
		Response(what_will_you_give_us, GiveRadios);
		Response(before_radios_we_need_info, GiveRadios);
	}
}

static void
Intro(void)
{
	if (IsHd()) // To smooth out HdResolutionFactor blink animation
	{
		CommData.AlienAmbientArray[0].BaseFrameRate = GameTicksPerSecond / 40;
	}

	if (GET_GAME_STATE(PROBE_ILWRATH_ENCOUNTER))
	{
		NPCPhrase(VERY_IMPRESSIVE);

		Response(cook_their_butts, ByeBye);
		Response(overthrow_evil_aliens, ByeBye);
		Response(annihilate_those_monsters, ByeBye);
	}
	else if (GET_GAME_STATE(STARBASE_VISITED))
	{
		if (GET_GAME_STATE(RADIOACTIVES_PROVIDED))
		{
			if (GET_GAME_STATE(MOONBASE_DESTROYED) == 0)
			{
				TellMoonBase(0);
			}
			else
			{
				BaseDestroyed(0);
			}
		}
		else
		{
			RaveParty();

			NPCPhrase(DO_YOU_HAVE_RADIO_THIS_TIME);

			if (GLOBAL_SIS(ElementAmounts[RADIOACTIVE]))
			{
				GiveRadios(0);
			}
			else
			{
				AskAfterRadios(0);
			}
		}
	}
	else /* first visit */
	{
		RaveParty();

		SET_GAME_STATE(STARBASE_VISITED, 1);

		NPCPhrase(ARE_YOU_SUPPLY_SHIP);
		Response(no_but_well_help, NoRadioactives);
		Response(yes_this_is_supply_ship, NoRadioactives);
		Response(what_slave_planet, NoRadioactives);
	}
}

static uint16_t
uninit_commander(void)
{
	uqm::lua().endComm();
	return (0);
}

static void
post_commander_enc(void)
{
	// nothing defined so far
}

LOCDATA*
init_commander_comm()
{
	commander_desc.init_encounter_func = Intro;
	commander_desc.post_encounter_func = post_commander_enc;
	commander_desc.uninit_encounter_func = uninit_commander;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	if (!GET_GAME_STATE(RADIOACTIVES_PROVIDED))
	{
		altResFlags |= UseAltSong;
	}

	if (IsHd())
	{
		altResFlags |= UseAltFrame;
	}


	commander_desc.AlienTextWidth = ResScale(143);
	commander_desc.AlienTextBaseline.x = ResScale(164)
									   - (SafeNeg(2) * 2);
	commander_desc.AlienTextBaseline.y = ResScale(20);

	setSegue(Segue_peace);
	LOCDATA* retval {&commander_desc};

	return (retval);
}