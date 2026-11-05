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
#include "uqm/races.h"
#include "uqm/lua/UQMLua.h"

static LOCDATA vux_desc =
	{
		VUX_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		VuxPmapAnim, /* AlienFrame */
		VuxFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* (SisTextWidth() - 16) >> 1, */  /* AlienTextWidth */
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_TOP, /* AlienTextValign */
		VuxColorMap, /* AlienColorMap */
		VuxMusic, /* AlienSong */
		{
								VuxZexPmapAnim, /* AlienAltFrame */
			NullResource,	/* AlienAltColorMap */
			VuxZexMusic,	/* AlienAltSong */
		},
		VuxConversationPhrases, /* PlayerPhrases */
		19, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				12,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				15,			/* StartIndex */
				5,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				20,			  /* StartIndex */
				14,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond / 30,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				34,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				41,			/* StartIndex */
				6,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				47,			  /* StartIndex */
				11,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				58,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				61,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				65,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				69,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				71,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				74,		  /* StartIndex */
				6,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				80,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 14),				 /* BlockMask */
			},
								{
				85,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				90,		  /* StartIndex */
				5,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 12),				 /* BlockMask */
			},
								{
				95,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 5,
				GameTicksPerSecond * 5, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				99,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 5,
				GameTicksPerSecond * 5, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				103,						 /* StartIndex */
				16,							 /* NumFrames */
				CircularAnim | AnimDisabled, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				119, /* StartIndex */
				8,	 /* NumFrames */
				CircularAnim | OneShotAnim
					| WaitTalking | AnimDisabled, /* AnimFlags */
				GameTicksPerSecond / 15,
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

static FILTER_DESC vux_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,				/* Color index */
				1,				/* Opacity index */
				-1,				/* Frame index */
				DRAW_ALPHA,		/* DrawKind*/
				SwitchOffAnims, /* Flags */
			},
								  }
};

static void ZexBeingEatenAlive(void)
{
	if (altResFlags & UseAltFrame)
	{ // make sure we're using the correct set of frames
		if (IsHd())
		{
			CommData.AlienFrame =
				SetAbsFrameIndex(CommData.AlienFrame, 126);
			EnableTalkingAnim(false);
			SwitchSequences(false);
			RunOneTimeSequence(18, 0);
		}
		else
		{
			CommData.AlienFrame =
				SetAbsFrameIndex(CommData.AlienFrame, 119);

			XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
						  c_gameSecond / 4);
		}
	}
	else
	{
		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond / 4);

		if (IsHd())
		{
			EngageFilters(&vux_filters);
		}
	}
}

static void CombatIsInevitable(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	setSegue(Segue_hostile);

	if (PlayerSaid(R, ok_take_beast))
	{
		NPCPhrase(FOOL_AIEE0);
		NPCPhrase(FOOL_AIEE1);

		AlienTalkSegue(1);

		ZexBeingEatenAlive();

		AlienTalkSegue((uint16_t)~0);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST_ON_SHIP, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ZEX_IS_DEAD, static_cast<uint32_t>(1));
		setSegue(Segue_peace);
	}
	else if (PlayerSaid(R, try_any_way))
	{
		NPCPhrase(NOPE);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, kill_you_squids_1)
			 || PlayerSaid(R, kill_you_squids_2)
			 || PlayerSaid(R, kill_you_squids_3)
			 || PlayerSaid(R, kill_you_squids_4))
	{
		NPCPhrase(WE_FIGHT);

		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_2) + 1;
		if (NumVisits <= 3)
		{
			uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_2, static_cast<uint32_t>(NumVisits));
		}
	}
	else if (PlayerSaid(R, cant_we_be_friends_1)
			 || PlayerSaid(R, cant_we_be_friends_2)
			 || PlayerSaid(R, cant_we_be_friends_3)
			 || PlayerSaid(R, cant_we_be_friends_4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_3);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(NEVER_UGLY_HUMANS_1);
				break;
			case 1:
				NPCPhrase(NEVER_UGLY_HUMANS_2);
				break;
			case 2:
				NPCPhrase(NEVER_UGLY_HUMANS_3);
				break;
			case 3:
				NPCPhrase(NEVER_UGLY_HUMANS_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_3, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, bye_hostile_space))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_4);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GOODBYE_AND_DIE_HOSTILE_SPACE_1);
				break;
			case 1:
				NPCPhrase(GOODBYE_AND_DIE_HOSTILE_SPACE_2);
				break;
			case 2:
				NPCPhrase(GOODBYE_AND_DIE_HOSTILE_SPACE_3);
				break;
			case 3:
				NPCPhrase(GOODBYE_AND_DIE_HOSTILE_SPACE_4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_4, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, bye_zex))
	{
		NPCPhrase(GOODBYE_ZEX);

		setSegue(Segue_peace);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_1);
		switch (NumVisits++)
		{
			case 4:
				NPCPhrase(NOT_ACCEPTED_1);
				break;
			case 5:
				NPCPhrase(NOT_ACCEPTED_2);
				break;
			case 6:
				NPCPhrase(NOT_ACCEPTED_3);
				break;
			case 7:
				NPCPhrase(NOT_ACCEPTED_4);
				break;
			case 8:
				NPCPhrase(NOT_ACCEPTED_5);
				break;
			case 9:
				NPCPhrase(NOT_ACCEPTED_6);
				break;
			case 10:
				NPCPhrase(NOT_ACCEPTED_7);
				break;
			case 11:
				NPCPhrase(NOT_ACCEPTED_8);
				break;
			case 12:
				NPCPhrase(NOT_ACCEPTED_9);
				break;
			case 13:
				NPCPhrase(TRUTH);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(NumVisits));
	}
}

static void Menagerie(RESPONSE_REF R)
{
	uint8_t i {};
	uint8_t LastStack {};
	RESPONSE_REF pStr[3];

	if (PlayerSaid(R, i_have_beast)
		|| PlayerSaid(R, why_trust_1)
		|| PlayerSaid(R, why_trust_2)
		|| PlayerSaid(R, why_trust_3))
	{
		if (PlayerSaid(R, i_have_beast))
		{
			NPCPhrase(GIVE_BEAST);
		}
		else if (PlayerSaid(R, why_trust_1))
		{
			NPCPhrase(TRUST_1);

			DisablePhrase(why_trust_1);
		}
		else if (PlayerSaid(R, why_trust_2))
		{
			NPCPhrase(TRUST_2);

			DisablePhrase(why_trust_2);
		}
		else if (PlayerSaid(R, why_trust_3))
		{
			NPCPhrase(TRUST_3);

			DisablePhrase(why_trust_3);
		}

		if (PhraseEnabled(why_trust_1))
		{
			Response(why_trust_1, Menagerie);
		}
		else if (PhraseEnabled(why_trust_2))
		{
			Response(why_trust_2, Menagerie);
		}
		else if (PhraseEnabled(why_trust_3))
		{
			Response(why_trust_3, Menagerie);
		}
		Response(ok_take_beast, CombatIsInevitable);
	}
	else if (PlayerSaid(R, kill_you))
	{
		NPCPhrase(FIGHT_AGAIN);

		setSegue(Segue_hostile);
	}
	else if (PlayerSaid(R, regardless))
	{
		NPCPhrase(THEN_FIGHT);

		setSegue(Segue_hostile);
		uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_3, static_cast<uint32_t>(2));
		uqm::gameState().setFlag(uqm::GameFlag::ZEX_VISITS, static_cast<uint32_t>(0));
	}
	else
	{
		LastStack = 0;
		pStr[0] = pStr[1] = pStr[2] = 0;
		if (R == 0)
		{
			uint8_t NumVisits {};

			NumVisits = uqm::gameState().flag(uqm::GameFlag::ZEX_VISITS);
			if (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_3) >= 2)
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(FIGHT_OR_TRADE_1);
						break;
					case 1:
						NPCPhrase(FIGHT_OR_TRADE_2);
						--NumVisits;
						break;
				}
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(ZEX_HELLO_1);
						break;
					case 1:
						NPCPhrase(ZEX_HELLO_2);
						break;
					case 2:
						NPCPhrase(ZEX_HELLO_3);
						break;
					case 3:
						NPCPhrase(ZEX_HELLO_4);
						--NumVisits;
						break;
				}
			}
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else if (PlayerSaid(R, what_you_do_here))
		{
			NPCPhrase(MY_MENAGERIE);

			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_1, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, what_about_menagerie))
		{
			NPCPhrase(NEED_NEW_CREATURE);

			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_1, static_cast<uint32_t>(2));
		}
		else if (PlayerSaid(R, what_about_creature))
		{
			NPCPhrase(ABOUT_CREATURE);

			uqm::gameState().setFlag(uqm::GameFlag::KNOW_ZEX_WANTS_MONSTER, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_1, static_cast<uint32_t>(3));

			R = about_creature_again;
			DisablePhrase(what_about_creature);
		}
		else if (PlayerSaid(R, about_creature_again))
		{
			NPCPhrase(CREATURE_AGAIN);

			DisablePhrase(about_creature_again);
		}
		else if (PlayerSaid(R, why_dont_you_attack))
		{
			NPCPhrase(LIKE_YOU);

			LastStack = 1;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_2, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, why_like_me))
		{
			NPCPhrase(LIKE_BECAUSE);

			LastStack = 1;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_2, static_cast<uint32_t>(2));
		}
		else if (PlayerSaid(R, are_you_a_pervert))
		{
			NPCPhrase(CALL_ME_WHAT_YOU_WISH);

			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_2, static_cast<uint32_t>(3));
		}
		else if (PlayerSaid(R, take_by_force))
		{
			NPCPhrase(PRECURSOR_DEVICE);

			LastStack = 2;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_3, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, you_lied))
		{
			NPCPhrase(YUP_LIED);

			LastStack = 2;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_3, static_cast<uint32_t>(3));
		}

		if (uqm::gameState().flag(uqm::GameFlag::KNOW_ZEX_WANTS_MONSTER)
			&& uqm::gameState().flag(uqm::GameFlag::VUX_BEAST_ON_SHIP))
		{
			pStr[0] = i_have_beast;
		}
		else
		{
			switch (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_1))
			{
				case 0:
					pStr[0] = what_you_do_here;
					break;
				case 1:
					pStr[0] = what_about_menagerie;
					break;
				case 2:
					pStr[0] = what_about_creature;
					break;
				case 3:
					if (PhraseEnabled(about_creature_again))
					{
						pStr[0] = about_creature_again;
					}
					break;
			}
		}
		switch (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_2))
		{
			case 0:
				pStr[1] = why_dont_you_attack;
				break;
			case 1:
				pStr[1] = why_like_me;
				break;
			case 2:
				pStr[1] = are_you_a_pervert;
				break;
		}
		switch (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_3))
		{
			case 0:
				pStr[2] = take_by_force;
				break;
			case 1:
				pStr[2] = regardless;
				break;
			case 2:
				pStr[2] = you_lied;
				break;
			case 3:
				pStr[2] = kill_you;
				break;
		}

		if (pStr[LastStack])
		{
			Response(pStr[LastStack], Menagerie);
		}
		for (i = 0; i < 3; ++i)
		{
			if (i != LastStack && pStr[i])
			{
				Response(pStr[i], Menagerie);
			}
		}
		Response(bye_zex, CombatIsInevitable);
	}
}

static void NormalVux(RESPONSE_REF R)
{
	if (PlayerSaid(R, why_so_mean))
	{
		NPCPhrase(URQUAN_SLAVES);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, deeper_reason))
	{
		NPCPhrase(OLD_INSULT);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, if_we_apologize))
	{
		NPCPhrase(PROBABLY_NOT);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, whats_up_hostile))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(GENERAL_INFO_HOSTILE_1);
				break;
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
		uqm::gameState().setFlag(uqm::GameFlag::VUX_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(whats_up_hostile);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_1))
	{
		case 0:
			Response(why_so_mean, NormalVux);
			break;
		case 1:
			Response(deeper_reason, NormalVux);
			break;
		case 2:
			Response(if_we_apologize, NormalVux);
			break;
		case 3:
			Response(try_any_way, CombatIsInevitable);
			break;
		case 4:
			Response(apology_1, CombatIsInevitable);
			break;
		case 5:
			Response(apology_2, CombatIsInevitable);
			break;
		case 6:
			Response(apology_3, CombatIsInevitable);
			break;
		case 7:
			Response(apology_4, CombatIsInevitable);
			break;
		case 8:
			Response(apology_5, CombatIsInevitable);
			break;
		case 9:
			Response(apology_6, CombatIsInevitable);
			break;
		case 10:
			Response(apology_7, CombatIsInevitable);
			break;
		case 11:
			Response(apology_8, CombatIsInevitable);
			break;
		case 12:
			Response(apology_9, CombatIsInevitable);
			break;
		case 13:
			Response(apology_10, CombatIsInevitable);
			break;
	}

	switch (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_2))
	{
		case 0:
			Response(kill_you_squids_1, CombatIsInevitable);
			break;
		case 1:
			Response(kill_you_squids_2, CombatIsInevitable);
			break;
		case 2:
			Response(kill_you_squids_3, CombatIsInevitable);
			break;
		case 3:
			Response(kill_you_squids_4, CombatIsInevitable);
			break;
	}

	if (PhraseEnabled(whats_up_hostile))
	{
		Response(whats_up_hostile, NormalVux);
	}

	if (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_1) > 13)
	{
		switch (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_3))
		{
			case 0:
				Response(cant_we_be_friends_1, CombatIsInevitable);
				break;
			case 1:
				Response(cant_we_be_friends_2, CombatIsInevitable);
				break;
			case 2:
				Response(cant_we_be_friends_3, CombatIsInevitable);
				break;
			case 3:
				Response(cant_we_be_friends_4, CombatIsInevitable);
				break;
		}
	}

	Response(bye_hostile_space, CombatIsInevitable);
}

static void Intro(void)
{
	if (altResFlags & UseAltFrame)
	{
		CommData.AlienAmbientArray[17].AnimFlags &= ~AnimDisabled;
	}

	if (uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		NPCPhrase(OUT_TAKES);

		setSegue(Segue_peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		Menagerie((RESPONSE_REF)0);
	}
	else
	{
		uint8_t NumVisits {};

		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase(HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase(HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase(HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::VUX_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			SetHomeworldKnown(VUX_HOME);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase(SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase(SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase(SPACE_HELLO_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::VUX_VISITS, static_cast<uint32_t>(NumVisits));
		}

		NormalVux((RESPONSE_REF)0);
	}
}

static uint16_t uninit_vux(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_vux_enc(void)
{
	// nothing defined so far
}

LOCDATA* init_vux_comm(void)
{
	vux_desc.init_encounter_func = Intro;
	vux_desc.post_encounter_func = post_vux_enc;
	vux_desc.uninit_encounter_func = uninit_vux;

	uqm::lua().beginComm(nullptr, NullResource);

	vux_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1)
								 + (SisTextWidth() >> 2) - SafeNumScl(4);
	vux_desc.AlienTextBaseline.y = 0;
	vux_desc.AlienTextWidth = ((SisTextWidth() - ResScale(16)) >> 1)
							+ SafeX();

	// use alternate "ZEX" track and frame if available
	if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		altResFlags |= UseAltSong;

		if (IsExtended())
		{
			altResFlags |= UseAltFrame;
		}
	}

	if ((uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		setSegue(Segue_peace);
	}
	else
	{
		setSegue(Segue_hostile);
	}
	LOCDATA* retval {&vux_desc};

	return (retval);
}
