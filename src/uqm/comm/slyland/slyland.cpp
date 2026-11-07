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


namespace uqm
{
static uint16_t probe_digit_names[] =
	{
		*SlylandroPhrase::EnumerateZero,
		*SlylandroPhrase::EnumerateOne,
		*SlylandroPhrase::EnumerateTwo,
		*SlylandroPhrase::EnumerateThree,
		*SlylandroPhrase::EnumerateFour,
		*SlylandroPhrase::EnumerateFive,
		*SlylandroPhrase::EnumerateSix,
		*SlylandroPhrase::EnumerateSeven,
		*SlylandroPhrase::EnumerateEight,
		*SlylandroPhrase::EnumerateNine,
};

static uint16_t probe_teen_names[] =
	{
		*SlylandroPhrase::EnumerateTen,
		*SlylandroPhrase::EnumerateEleven,
		*SlylandroPhrase::EnumerateTwelve,
		*SlylandroPhrase::EnumerateThirteen,
		*SlylandroPhrase::EnumerateFourteen,
		*SlylandroPhrase::EnumerateFifteen,
		*SlylandroPhrase::EnumerateSixteen,
		*SlylandroPhrase::EnumerateSeventeen,
		*SlylandroPhrase::EnumerateEighteen,
		*SlylandroPhrase::EnumerateNineteen,
};

static uint16_t probe_tens_names[] =
	{
		0, /* invalid */
		0, /* skip digit */
		*SlylandroPhrase::EnumerateTwenty,
		*SlylandroPhrase::EnumerateThirty,
		*SlylandroPhrase::EnumerateFourty,
		*SlylandroPhrase::EnumerateFifty,
		*SlylandroPhrase::EnumerateSixty,
		*SlylandroPhrase::EnumerateSeventy,
		*SlylandroPhrase::EnumerateEighty,
		*SlylandroPhrase::EnumerateNinety,
};

const static NumberSpeechDesc probe_numbers_english =
	{
		5, /* NumDigits */
		{
						  {
				/* 1000-999999 */
				1000,								/* Divider */
				0,									/* Subtrahend */
				nullptr,							/* StrDigits - recurse */
				nullptr,							/* Names - not used */
				*SlylandroPhrase::EnumerateThousand /* CommonIndex */
			},
						  {
				/* 100-999 */
				100,							   /* Divider */
				0,								   /* Subtrahend */
				probe_digit_names,				   /* StrDigits */
				nullptr,						   /* Names - not used */
				*SlylandroPhrase::EnumerateHundred /* CommonIndex */
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

static ConversationData slylandro_desc =
	{
		Conversation::Slylandro, /* AlienConv */
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
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
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


static SlylandroPhrase threat,
	something_wrong,
	we_are_us,
	why_attack,
	bye;

static void sayCoord(int coord)
{
	const int ac {abs(coord)};

	NPCPhrase_splice(coord < 0 ? SlylandroPhrase::CoordMinus : SlylandroPhrase::CoordPlus);
	NPCNumber(ac / 10, "{:03}");
	NPCPhrase_splice(SlylandroPhrase::CoordPoint);
	NPCNumber(ac % 10, nullptr);
}

static void CombatIsInevitable(ResponseRef R)
{
	if (R == 0)
	{
		if (uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP))
		{
			Response(SlylandroPhrase::DestructCode, CombatIsInevitable);
		}
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_THREAT))
		{
			case 0:
				threat = SlylandroPhrase::Threat1;
				break;
			case 1:
				threat = SlylandroPhrase::Threat2;
				break;
			case 2:
				threat = SlylandroPhrase::Threat3;
				break;
			default:
				threat = SlylandroPhrase::Threat4;
				break;
		}
		Response(threat, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_WRONG))
		{
			case 0:
				something_wrong = SlylandroPhrase::SomethingWrong1;
				break;
			case 1:
				something_wrong = SlylandroPhrase::SomethingWrong2;
				break;
			case 2:
				something_wrong = SlylandroPhrase::SomethingWrong3;
				break;
			default:
				something_wrong = SlylandroPhrase::SomethingWrong4;
				break;
		}
		Response(something_wrong, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_ID))
		{
			case 0:
				we_are_us = SlylandroPhrase::WeAreUs1;
				break;
			case 1:
				we_are_us = SlylandroPhrase::WeAreUs2;
				break;
			case 2:
				we_are_us = SlylandroPhrase::WeAreUs3;
				break;
			default:
				we_are_us = SlylandroPhrase::WeAreUs4;
				break;
		}
		Response(we_are_us, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_INFO))
		{
			case 0:
				why_attack = SlylandroPhrase::WhyAttack1;
				break;
			case 1:
				why_attack = SlylandroPhrase::WhyAttack2;
				break;
			case 2:
				why_attack = SlylandroPhrase::WhyAttack3;
				break;
			default:
				why_attack = SlylandroPhrase::WhyAttack4;
				break;
		}
		Response(why_attack, CombatIsInevitable);
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_EXIT))
		{
			case 0:
				bye = SlylandroPhrase::Bye1;
				break;
			case 1:
				bye = SlylandroPhrase::Bye2;
				break;
			case 2:
				bye = SlylandroPhrase::Bye3;
				break;
			default:
				bye = SlylandroPhrase::Bye4;
				break;
		}
		Response(bye, CombatIsInevitable);
	}
	else if (PlayerSaid(R, SlylandroPhrase::DestructCode))
	{
		NPCPhrase(SlylandroPhrase::DestructSequence);
		setSegue(Segue::Victory);
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
					NPCPhrase(SlylandroPhrase::ProgrammedToDefend1);
					break;
				case 1:
					NPCPhrase(SlylandroPhrase::ProgrammedToDefend2);
					break;
				case 2:
					NPCPhrase(SlylandroPhrase::ProgrammedToDefend3);
					break;
				case 3:
					NPCPhrase(SlylandroPhrase::ProgrammedToDefend4);
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
					NPCPhrase(SlylandroPhrase::NominalFunction1);
					break;
				case 1:
					NPCPhrase(SlylandroPhrase::NominalFunction2);
					break;
				case 2:
					NPCPhrase(SlylandroPhrase::NominalFunction3);
					break;
				case 3:
					NPCPhrase(SlylandroPhrase::NominalFunction4);
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
					NPCPhrase(SlylandroPhrase::ThisIsProbe1);
					break;
				case 1:
					NPCPhrase(SlylandroPhrase::ThisIsProbe2);
					break;
				case 2:
					NPCPhrase(SlylandroPhrase::ThisIsProbe3);
					break;
				case 3:
					{
						int16_t dx {};
						int16_t dy {};

						// Probe's coordinate system is +/-{Y,X} relative to home
						// If homeworld X or Y are < 5000, we will flip the sign
						// (and always say Y before X)
						dx = (plot_map[*PlotId::Slylandro].star_pt.x - LogxToUniverse(uqm::gameState().sis().log_x)) * (plot_map[*PlotId::Slylandro].star_pt.x > 5000 ? 1 : -1);
						dy = (plot_map[*PlotId::Slylandro].star_pt.y - LogyToUniverse(uqm::gameState().sis().log_y)) * (plot_map[*PlotId::Slylandro].star_pt.y > 5000 ? 1 : -1);

						NPCPhrase(SlylandroPhrase::ThisIsProbe40);
						sayCoord(dy);
						NPCPhrase_splice(SlylandroPhrase::ThisIsProbe41);
						sayCoord(dx);
						NPCPhrase(SlylandroPhrase::ThisIsProbe42);

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
					NPCPhrase(SlylandroPhrase::PeacefulMission1);
					break;
				case 1:
					NPCPhrase(SlylandroPhrase::PeacefulMission2);
					break;
				case 2:
					NPCPhrase(SlylandroPhrase::PeacefulMission3);
					break;
				case 3:
					NPCPhrase(SlylandroPhrase::PeacefulMission4);
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
					NPCPhrase(SlylandroPhrase::Goodbye1);
					break;
				case 1:
					NPCPhrase(SlylandroPhrase::Goodbye2);
					break;
				case 2:
					NPCPhrase(SlylandroPhrase::Goodbye3);
					break;
				case 3:
					NPCPhrase(SlylandroPhrase::Goodbye4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_EXIT, static_cast<uint32_t>(NumVisits));
		}

		NPCPhrase(SlylandroPhrase::Hostile);

		uqm::gameState().setFlag(uqm::GameFlag::PROBE_EXHIBITED_BUG, static_cast<uint32_t>(1));
		setSegue(Segue::Hostile);
	}
}

static void Intro(void)
{
	if (isDifficulty(uqm::Difficulty::Hard) && uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NPCPhrase(SlylandroPhrase::CoordPoint);
		for (uint16_t i {0}; i < 16; i++)
		{
			NPCNumber(random_ramble[i], "{} ");
		}
		NPCNumber(random_ramble[16], "{}");
		NPCPhrase(SlylandroPhrase::CoordPoint);

		NPCPhrase(SlylandroPhrase::Hostile);

		uqm::gameState().setFlag(uqm::GameFlag::PROBE_EXHIBITED_BUG, static_cast<uint32_t>(1));
		setSegue(Segue::Hostile);
	}
	else
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_PROBE_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SlylandroPhrase::WeComeInPeace1);
				break;
			case 1:
				NPCPhrase(SlylandroPhrase::WeComeInPeace2);
				break;
			case 2:
				NPCPhrase(SlylandroPhrase::WeComeInPeace3);
				break;
			case 3:
				NPCPhrase(SlylandroPhrase::WeComeInPeace4);
				break;
			case 4:
				NPCPhrase(SlylandroPhrase::WeComeInPeace5);
				break;
			case 5:
				NPCPhrase(SlylandroPhrase::WeComeInPeace6);
				break;
			case 6:
				NPCPhrase(SlylandroPhrase::WeComeInPeace7);
				break;
			case 7:
				NPCPhrase(SlylandroPhrase::WeComeInPeace8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_PROBE_VISITS, static_cast<uint32_t>(NumVisits));

		CombatIsInevitable((ResponseRef)0);
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

ConversationData* init_slyland_comm(void)
{
	slylandro_desc.init_encounter_func = Intro;
	slylandro_desc.post_encounter_func = post_slyland_enc;
	slylandro_desc.uninit_encounter_func = uninit_slyland;

	slylandro_desc.AlienTextBaseline.x =
		TextXOffs() + (SisTextWidth() >> 1);
	slylandro_desc.AlienTextBaseline.y = 0;
	slylandro_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	setSegue(Segue::Hostile);
	ConversationData* retval {&slylandro_desc};

	return (retval);
}

} // namespace uqm
