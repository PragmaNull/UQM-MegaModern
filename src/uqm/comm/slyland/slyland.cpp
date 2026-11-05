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
#include <stdlib.h>
#include "resinst.h"
#include "strings.h"

#include "options.h"
#include "uqm/battle.h"
#include "uqm/setup.h"
#include "uqm/starmap.h"


static uint16_t probe_digit_names[] =
	{
		ENUMERATE_ZERO,
		ENUMERATE_ONE,
		ENUMERATE_TWO,
		ENUMERATE_THREE,
		ENUMERATE_FOUR,
		ENUMERATE_FIVE,
		ENUMERATE_SIX,
		ENUMERATE_SEVEN,
		ENUMERATE_EIGHT,
		ENUMERATE_NINE,
};

static uint16_t probe_teen_names[] =
	{
		ENUMERATE_TEN,
		ENUMERATE_ELEVEN,
		ENUMERATE_TWELVE,
		ENUMERATE_THIRTEEN,
		ENUMERATE_FOURTEEN,
		ENUMERATE_FIFTEEN,
		ENUMERATE_SIXTEEN,
		ENUMERATE_SEVENTEEN,
		ENUMERATE_EIGHTEEN,
		ENUMERATE_NINETEEN,
};

static uint16_t probe_tens_names[] =
	{
		0, /* invalid */
		0, /* skip digit */
		ENUMERATE_TWENTY,
		ENUMERATE_THIRTY,
		ENUMERATE_FOURTY,
		ENUMERATE_FIFTY,
		ENUMERATE_SIXTY,
		ENUMERATE_SEVENTY,
		ENUMERATE_EIGHTY,
		ENUMERATE_NINETY,
};

const static NUMBER_SPEECH_DESC probe_numbers_english =
	{
		5, /* NumDigits */
		{
						  {
				/* 1000-999999 */
				1000,			   /* Divider */
				0,				   /* Subtrahend */
				nullptr,		   /* StrDigits - recurse */
				nullptr,		   /* Names - not used */
				ENUMERATE_THOUSAND /* CommonIndex */
			},
						  {
				/* 100-999 */
				100,			   /* Divider */
				0,				   /* Subtrahend */
				probe_digit_names, /* StrDigits */
				nullptr,		   /* Names - not used */
				ENUMERATE_HUNDRED  /* CommonIndex */
			},
						  {
				/* 20-99 */
				10,				  /* Divider */
				0,				  /* Subtrahend */
				probe_tens_names, /* StrDigits */
				nullptr,		  /* Names - not used */
				0				  /* CommonIndex - not used */
			},
						  {
				/* 10-19 */
				1,				  /* Divider */
				10,				  /* Subtrahend */
				probe_teen_names, /* StrDigits */
				nullptr,		  /* Names - not used */
				0				  /* CommonIndex - not used */
			},
						  {
				/* 0-9 */
				1,				   /* Divider */
				0,				   /* Subtrahend */
				probe_digit_names, /* StrDigits */
				nullptr,		   /* Names - not used */
				0				   /* CommonIndex - not used */
			}}
};

static LOCDATA slylandro_desc =
	{
		SLYLANDRO_CONVERSATION, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SlylandPmapAnim, /* AlienFrame */
		SlylandFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_TOP, /* AlienTextValign */
		SlylandColorMap, /* AlienColorMap */
		SlylandMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		SlylandConversationPhrases, /* PlayerPhrases */
		6, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				1,							/* StartIndex */
				9,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 40, 0, /* FrameRate */
				0, GameTicksPerSecond * 3,	/* RestartRate */
				(1 << 3)					/* BlockMask */
			},
								{
				10,							/* StartIndex */
				8,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 40, 0, /* FrameRate */
				0, GameTicksPerSecond * 3,	/* RestartRate */
				(1 << 4)					/* BlockMask */
			},
								{
				18,							/* StartIndex */
				8,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 40, 0, /* FrameRate */
				0, GameTicksPerSecond * 3,	/* RestartRate */
				(1 << 5)					/* BlockMask */
			},
								{
				26,							/* StartIndex */
				8,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 40, 0, /* FrameRate */
				0, GameTicksPerSecond * 3,	/* RestartRate */
				(1 << 0)					/* BlockMask */
			},
								{
				34,							/* StartIndex */
				8,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 40, 0, /* FrameRate */
				0, GameTicksPerSecond * 3,	/* RestartRate */
				(1 << 1)					/* BlockMask */
			},
								{
				42,							/* StartIndex */
				9,							/* NumFrames */
				CircularAnim,				/* AnimFlags */
				GameTicksPerSecond / 40, 0, /* FrameRate */
				0, GameTicksPerSecond * 3,	/* RestartRate */
				(1 << 2)					/* BlockMask */
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
		&probe_numbers_english, /* AlienNumberSpeech - default */
		/* Filler for loaded resources */
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
};


static uint32_t random_ramble[] =
	{
		1,
		15,
		48,
		312,
		7,
		4000,
		668,
		46,
		4,
		8,
		15,
		16,
		23,
		42,
		998,
		80556,
		1337};


static RESPONSE_REF threat,
	something_wrong,
	we_are_us,
	why_attack,
	bye;

static void sayCoord(int coord)
{
	const int ac {abs(coord)};

	NPCPhrase_splice(coord < 0 ? COORD_MINUS : COORD_PLUS);
	NPCNumber(ac / 10, "{:03}");
	NPCPhrase_splice(COORD_POINT);
	NPCNumber(ac % 10, nullptr);
}

static void CombatIsInevitable(RESPONSE_REF R)
{
	if (R == 0)
	{
		if (uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP))
		{
			Response(destruct_code, CombatIsInevitable);
		}
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_THREAT))
		{
			case 0:
				threat = threat_1;
				break;
			case 1:
				threat = threat_2;
				break;
			case 2:
				threat = threat_3;
				break;
			default:
				threat = threat_4;
				break;
		}
		Response(threat, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_WRONG))
		{
			case 0:
				something_wrong = something_wrong_1;
				break;
			case 1:
				something_wrong = something_wrong_2;
				break;
			case 2:
				something_wrong = something_wrong_3;
				break;
			default:
				something_wrong = something_wrong_4;
				break;
		}
		Response(something_wrong, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_ID))
		{
			case 0:
				we_are_us = we_are_us_1;
				break;
			case 1:
				we_are_us = we_are_us_2;
				break;
			case 2:
				we_are_us = we_are_us_3;
				break;
			default:
				we_are_us = we_are_us_4;
				break;
		}
		Response(we_are_us, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_INFO))
		{
			case 0:
				why_attack = why_attack_1;
				break;
			case 1:
				why_attack = why_attack_2;
				break;
			case 2:
				why_attack = why_attack_3;
				break;
			default:
				why_attack = why_attack_4;
				break;
		}
		Response(why_attack, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_EXIT))
		{
			case 0:
				bye = bye_1;
				break;
			case 1:
				bye = bye_2;
				break;
			case 2:
				bye = bye_3;
				break;
			default:
				bye = bye_4;
				break;
		}
		Response(bye, CombatIsInevitable);
	}
	else if (PlayerSaid(R, destruct_code))
	{
		NPCPhrase(DESTRUCT_SEQUENCE);
		setSegue(Segue_victory);
	}
	else
	{
		uint8_t NumVisits {};

		if (PlayerSaid(R, threat))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_THREAT);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PROGRAMMED_TO_DEFEND_1);
					break;
				case 1:
					NPCPhrase(PROGRAMMED_TO_DEFEND_2);
					break;
				case 2:
					NPCPhrase(PROGRAMMED_TO_DEFEND_3);
					break;
				case 3:
					NPCPhrase(PROGRAMMED_TO_DEFEND_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_THREAT, static_cast<uint32_t>(NumVisits));
		}
		else if (PlayerSaid(R, something_wrong))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_WRONG);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(NOMINAL_FUNCTION_1);
					break;
				case 1:
					NPCPhrase(NOMINAL_FUNCTION_2);
					break;
				case 2:
					NPCPhrase(NOMINAL_FUNCTION_3);
					break;
				case 3:
					NPCPhrase(NOMINAL_FUNCTION_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_WRONG, static_cast<uint32_t>(NumVisits));
		}
		else if (PlayerSaid(R, we_are_us))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_ID);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(THIS_IS_PROBE_1);
					break;
				case 1:
					NPCPhrase(THIS_IS_PROBE_2);
					break;
				case 2:
					NPCPhrase(THIS_IS_PROBE_3);
					break;
				case 3:
					{
						int16_t dx {};
						int16_t dy {};

						// Probe's coordinate system is +/-{Y,X} relative to home
						// If homeworld X or Y are < 5000, we will flip the sign
						// (and always say Y before X)
						dx = (plot_map[SLYLANDRO_DEFINED].star_pt.x - LogxToUniverse(uqm::gameState().sis().log_x)) * (plot_map[SLYLANDRO_DEFINED].star_pt.x > 5000 ? 1 : -1);
						dy = (plot_map[SLYLANDRO_DEFINED].star_pt.y - LogyToUniverse(uqm::gameState().sis().log_y)) * (plot_map[SLYLANDRO_DEFINED].star_pt.y > 5000 ? 1 : -1);

						NPCPhrase(THIS_IS_PROBE_40);
						sayCoord(dy);
						NPCPhrase_splice(THIS_IS_PROBE_41);
						sayCoord(dx);
						NPCPhrase(THIS_IS_PROBE_42);

						--NumVisits;
						break;
					}
			}
			uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_ID, static_cast<uint32_t>(NumVisits));
		}
		else if (PlayerSaid(R, why_attack))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_INFO);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(PEACEFUL_MISSION_1);
					break;
				case 1:
					NPCPhrase(PEACEFUL_MISSION_2);
					break;
				case 2:
					NPCPhrase(PEACEFUL_MISSION_3);
					break;
				case 3:
					NPCPhrase(PEACEFUL_MISSION_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_INFO, static_cast<uint32_t>(NumVisits));
		}
		else if (PlayerSaid(R, bye))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_EXIT);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(GOODBYE_1);
					break;
				case 1:
					NPCPhrase(GOODBYE_2);
					break;
				case 2:
					NPCPhrase(GOODBYE_3);
					break;
				case 3:
					NPCPhrase(GOODBYE_4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_EXIT, static_cast<uint32_t>(NumVisits));
		}

		NPCPhrase(HOSTILE);

		uqm::gameState().setFlag(uqm::GameFlag::PROBE_EXHIBITED_BUG, static_cast<uint32_t>(1));
		setSegue(Segue_hostile);
	}
}

static void Intro(void)
{
	if (isDifficulty(uqm::Difficulty::Hard) && uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NPCPhrase(COORD_POINT);
		for (uint16_t i {0}; i < 16; i++)
		{
			NPCNumber(random_ramble[i], "{} ");
		}
		NPCNumber(random_ramble[16], "{}");
		NPCPhrase(COORD_POINT);

		NPCPhrase(HOSTILE);

		uqm::gameState().setFlag(uqm::GameFlag::PROBE_EXHIBITED_BUG, static_cast<uint32_t>(1));
		setSegue(Segue_hostile);
	}
	else
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(WE_COME_IN_PEACE_1);
				break;
			case 1:
				NPCPhrase(WE_COME_IN_PEACE_2);
				break;
			case 2:
				NPCPhrase(WE_COME_IN_PEACE_3);
				break;
			case 3:
				NPCPhrase(WE_COME_IN_PEACE_4);
				break;
			case 4:
				NPCPhrase(WE_COME_IN_PEACE_5);
				break;
			case 5:
				NPCPhrase(WE_COME_IN_PEACE_6);
				break;
			case 6:
				NPCPhrase(WE_COME_IN_PEACE_7);
				break;
			case 7:
				NPCPhrase(WE_COME_IN_PEACE_8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_VISITS, static_cast<uint32_t>(NumVisits));

		CombatIsInevitable((RESPONSE_REF)0);
	}
}

static uint16_t uninit_slyland(void)
{
	return (0);
}

static void post_slyland_enc(void)
{
	// nothing defined so far
}

LOCDATA* init_slyland_comm(void)
{
	slylandro_desc.init_encounter_func = Intro;
	slylandro_desc.post_encounter_func = post_slyland_enc;
	slylandro_desc.uninit_encounter_func = uninit_slyland;

	slylandro_desc.AlienTextBaseline.x =
		TextXOffs() + (SisTextWidth() >> 1);
	slylandro_desc.AlienTextBaseline.y = 0;
	slylandro_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	setSegue(Segue_hostile);
	LOCDATA* retval {&slylandro_desc};

	return (retval);
}
