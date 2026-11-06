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
#include "../comandr/resinst.h"
#include "strings.h"
#include "options.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"
#include "uqm/setup.h"
#include "uqm/shipcont.h"
#include "uqm/sis.h"
// for DeltaSISGauges()
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/inplib.h"
#include "uqm/oscill.h"
#include "uqm/nameref.h"
#include "uqm/controls.h"
#include "uqm/random/UQMRandom.h"


static void TellMission(RESPONSE_REF R);
static void SellMinerals(RESPONSE_REF R);


static ConversationData commander_desc =
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
								CommanderPmapAnimRed, /* AlienAltFrame */
			NullResource,		  /* AlienAltColorMap */
			StarbaseAltMusic,	  /* AlienAltSong */
		},
		StarbaseConversationPhrases, /* PlayerPhrases */
		10, /* NumAnimations */
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
				/* Arc welder 0 */
				40,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Arc welder 1 */
				47,			  /* StartIndex */
				8,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Arc welder 2 */
				55,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Arc welder 3 */
				61,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Arc welder 4 */
				67,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Arc welder 5 */
				74,			  /* StartIndex */
				11,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Arc welder 6 */
				85,			  /* StartIndex */
				10,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Flagship picture */
				95, /* StartIndex */
				1,	/* NumFrames */
				0,	/* AnimFlags */
				0,
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

static uint32_t CurBulletinMask;

static uint16_t DoSellMinerals(void)
{
	uint16_t total = 0;
	bool Sleepy = true;

	FlushInput();

	for (uint16_t i {0}; i < NUM_ELEMENT_CATEGORIES; ++i)
	{
		uint16_t amount {};
		TimeCount TimeIn {};

		if (i == 0)
		{
			DrawCargoStrings((uint8_t)~0, (uint8_t)~0);
			TimeIn = GetTimeCounter() + c_gameSecond / 2;
			while (GetTimeCounter() <= TimeIn)
			{
				if (AnyButtonPress(true) || (uqm::currentActivity() & CHECK_ABORT))
				{
					Sleepy = false;
					break;
				}
				UpdateDuty(false);
			}

			if (Sleepy)
			{
				DrawCargoStrings((uint8_t)0, (uint8_t)0);
			}
		}
		else if (Sleepy)
		{
			DrawCargoStrings((uint8_t)(i - 1), (uint8_t)i);
		}

		if ((amount = uqm::gameState().sis().ElementAmounts[i]) != 0)
		{
			total += amount * uqm::gameState().game().ElementWorth[i];
			do
			{
				if (!Sleepy || AnyButtonPress(true) || (uqm::currentActivity() & CHECK_ABORT))
				{
					Sleepy = false;
					uqm::gameState().sis().ElementAmounts[i] = 0;
					uqm::gameState().sis().TotalElementMass -= amount;
					DeltaSISGauges(0, 0,
								   amount * uqm::gameState().game().ElementWorth[i]);
					break;
				}

				UpdateDuty(false);

				--uqm::gameState().sis().ElementAmounts[i];
				--uqm::gameState().sis().TotalElementMass;
				TaskSwitch();
				DrawCargoStrings((uint8_t)i, (uint8_t)i);
				ShowRemainingCapacity();
				DeltaSISGauges(0, 0, uqm::gameState().game().ElementWorth[i]);
			} while (--amount);
		}
		if (Sleepy)
		{
			TimeIn = GetTimeCounter() + (c_gameSecond / 4);
			while (GetTimeCounter() <= TimeIn)
			{
				if (AnyButtonPress(true) || (uqm::currentActivity() & CHECK_ABORT))
				{
					Sleepy = false;
					break;
				}
				UpdateDuty(false);
			}
		}
	}

	SleepThread(c_gameSecond / 2);

	ClearSISRect(DrawSisDisplay);

	return total;
}

static RESPONSE_REF EveryOnesACritic(uint16_t total)
{
	RESPONSE_REF pStr = LIGHT_LOAD_A;

	if (total < 1000)
	{
		total = uqm::gameState().flag(uqm::GameFlag::LIGHT_MINERAL_LOAD);
		switch (total++)
		{
			case 0:
				pStr = LIGHT_LOAD_A;
				break;
			case 1:
				pStr = LIGHT_LOAD_B;
				break;
			case 2:
				// There are two separate sound samples in this case.
				pStr = LIGHT_LOAD_C0;
				//pStr2 = LIGHT_LOAD_C1; Kruzen: processed outside this func now
				break;
			case 3:
				pStr = LIGHT_LOAD_D;
				break;
			case 4:
				pStr = LIGHT_LOAD_E;
				break;
			case 5:
				pStr = LIGHT_LOAD_F;
				break;
			case 6:
				--total;
				pStr = LIGHT_LOAD_G;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::LIGHT_MINERAL_LOAD, static_cast<uint32_t>(total));
	}
	else if (total < 2500)
	{
		total = uqm::gameState().flag(uqm::GameFlag::MEDIUM_MINERAL_LOAD);
		switch (total++)
		{
			case 0:
				pStr = MEDIUM_LOAD_A;
				break;
			case 1:
				pStr = MEDIUM_LOAD_B;
				break;
			case 2:
				pStr = MEDIUM_LOAD_C;
				break;
			case 3:
				pStr = MEDIUM_LOAD_D;
				break;
			case 4:
				pStr = MEDIUM_LOAD_E;
				break;
			case 5:
				pStr = MEDIUM_LOAD_F;
				break;
			case 6:
				--total;
				pStr = MEDIUM_LOAD_G;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::MEDIUM_MINERAL_LOAD, static_cast<uint32_t>(total));
	}
	else
	{
		total = uqm::gameState().flag(uqm::GameFlag::HEAVY_MINERAL_LOAD);
		switch (total++)
		{
			case 0:
				pStr = HEAVY_LOAD_A;
				break;
			case 1:
				pStr = HEAVY_LOAD_B;
				break;
			case 2:
				pStr = HEAVY_LOAD_C;
				break;
			case 3:
				pStr = HEAVY_LOAD_D;
				break;
			case 4:
				pStr = HEAVY_LOAD_E;
				break;
			case 5:
				pStr = HEAVY_LOAD_F;
				break;
			case 6:
				--total;
				pStr = HEAVY_LOAD_G;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::HEAVY_MINERAL_LOAD, static_cast<uint32_t>(total));
	}

	return pStr;
}

static void ByeBye(RESPONSE_REF R)
{
	(void)R; // ignored

	CurBulletinMask |= uqm::gameState().flag(uqm::GameFlag::STARBASE_BULLETS);
	uqm::gameState().setFlag(uqm::GameFlag::STARBASE_BULLETS, static_cast<uint32_t>(CurBulletinMask));

	/* if (R == goodbye_starbase_commander) */
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) >= 2)
	{
		NPCPhrase(GOOD_LUCK_AGAIN);
	}
	else
	{
		RESPONSE_REF pStr = 0;

		switch ((uint8_t)uqm::random().next() & 7)
		{
			case 0:
				pStr = NORMAL_GOODBYE_A;
				break;
			case 1:
				pStr = NORMAL_GOODBYE_B;
				break;
			case 2:
				pStr = NORMAL_GOODBYE_C;
				break;
			case 3:
				pStr = NORMAL_GOODBYE_D;
				break;
			case 4:
				pStr = NORMAL_GOODBYE_E;
				break;
			case 5:
				pStr = NORMAL_GOODBYE_F;
				break;
			case 6:
				pStr = NORMAL_GOODBYE_G;
				break;
			case 7:
				pStr = NORMAL_GOODBYE_H;
				break;
		}

		NPCPhrase(pStr);
	}
}

static void NeedInfo(RESPONSE_REF R);
static void TellHistory(RESPONSE_REF R);
static void AlienRaces(RESPONSE_REF R);

static uint8_t stack0;
static uint8_t stack1;
static uint8_t stack2;
static uint8_t stack3;

static constexpr auto AllianceShofixti {(1 << 0)};
static constexpr auto AllianceYehat {(1 << 1)};
static constexpr auto AllianceArilou {(1 << 2)};
static constexpr auto AllianceChenjesu {(1 << 3)};
static constexpr auto AllianceMmrnmhrm {(1 << 4)};
static constexpr auto AllianceSyreen {(1 << 5)};

static void AllianceInfo(RESPONSE_REF R)
{
	static uint8_t AllianceMask = 0;

	if (PlayerSaid(R, what_about_alliance))
	{
		NPCPhrase(WHICH_ALLIANCE);
		AllianceMask = 0;
	}
	else if (PlayerSaid(R, shofixti))
	{
		NPCPhrase(ABOUT_SHOFIXTI);
		AllianceMask |= AllianceShofixti;
		SetHomeworldKnown(SHOFIXTI_HOME);
	}
	else if (PlayerSaid(R, yehat))
	{
		NPCPhrase(ABOUT_YEHAT);
		AllianceMask |= AllianceYehat;
	}
	else if (PlayerSaid(R, arilou))
	{
		NPCPhrase(ABOUT_ARILOU);
		AllianceMask |= AllianceArilou;
	}
	else if (PlayerSaid(R, chenjesu))
	{
		NPCPhrase(ABOUT_CHENJESU);
		AllianceMask |= AllianceChenjesu;
	}
	else if (PlayerSaid(R, mmrnmhrm))
	{
		NPCPhrase(ABOUT_MMRNMHRM);
		AllianceMask |= AllianceMmrnmhrm;
	}
	else if (PlayerSaid(R, syreen))
	{
		NPCPhrase(ABOUT_SYREEN);
		AllianceMask |= AllianceSyreen;
	}

	if (!(AllianceMask & AllianceShofixti))
	{
		Response(shofixti, AllianceInfo);
	}
	if (!(AllianceMask & AllianceYehat))
	{
		Response(yehat, AllianceInfo);
	}
	if (!(AllianceMask & AllianceArilou))
	{
		Response(arilou, AllianceInfo);
	}
	if (!(AllianceMask & AllianceChenjesu))
	{
		Response(chenjesu, AllianceInfo);
	}
	if (!(AllianceMask & AllianceMmrnmhrm))
	{
		Response(mmrnmhrm, AllianceInfo);
	}
	if (!(AllianceMask & AllianceSyreen))
	{
		Response(syreen, AllianceInfo);
	}
	Response(enough_alliance, AlienRaces);
}

static constexpr auto HierarchyMycon {(1 << 0)};
static constexpr auto HierarchySpathi {(1 << 1)};
static constexpr auto HierarchyUmgah {(1 << 2)};
static constexpr auto HierarchyAndrosynth {(1 << 3)};
static constexpr auto HierarchyIlwrath {(1 << 4)};
static constexpr auto HierarchyVux {(1 << 5)};
static constexpr auto HierarchyUrquan {(1 << 6)};

static void HierarchyInfo(RESPONSE_REF R)
{
	static uint8_t HierarchyMask = 0;

	if (PlayerSaid(R, what_about_hierarchy))
	{
		NPCPhrase(WHICH_HIERARCHY);
		HierarchyMask = 0;
	}
	else if (PlayerSaid(R, urquan))
	{
		NPCPhrase(ABOUT_URQUAN);
		HierarchyMask |= HierarchyUrquan;
	}
	else if (PlayerSaid(R, mycon))
	{
		NPCPhrase(ABOUT_MYCON);
		HierarchyMask |= HierarchyMycon;
	}
	else if (PlayerSaid(R, spathi))
	{
		NPCPhrase(ABOUT_SPATHI);
		HierarchyMask |= HierarchySpathi;
	}
	else if (PlayerSaid(R, umgah))
	{
		NPCPhrase(ABOUT_UMGAH);
		HierarchyMask |= HierarchyUmgah;
	}
	else if (PlayerSaid(R, androsynth))
	{
		NPCPhrase(ABOUT_ANDROSYNTH);
		HierarchyMask |= HierarchyAndrosynth;
		SetHomeworldKnown(ANDROSYNTH_HOME);
	}
	else if (PlayerSaid(R, ilwrath))
	{
		NPCPhrase(ABOUT_ILWRATH);
		HierarchyMask |= HierarchyIlwrath;
	}
	else if (PlayerSaid(R, vux))
	{
		NPCPhrase(ABOUT_VUX);
		HierarchyMask |= HierarchyVux;
	}

	if (!(HierarchyMask & HierarchyUrquan))
	{
		Response(urquan, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyMycon))
	{
		Response(mycon, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchySpathi))
	{
		Response(spathi, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyUmgah))
	{
		Response(umgah, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyAndrosynth))
	{
		Response(androsynth, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyIlwrath))
	{
		Response(ilwrath, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyVux))
	{
		Response(vux, HierarchyInfo);
	}
	Response(enough_hierarchy, AlienRaces);
}

static constexpr auto RacesAlliance {(1 << 0)};
static constexpr auto RacesHierarchy {(1 << 1)};
static constexpr auto RacesOther {(1 << 2)};

static void AlienRaces(RESPONSE_REF R)
{
	static uint8_t RacesMask = 0;

	if (PlayerSaid(R, alien_races))
	{
		NPCPhrase(WHICH_ALIEN);
		RacesMask = 0;
	}
	else if (PlayerSaid(R, enough_alliance))
	{
		NPCPhrase(OK_ENOUGH_ALLIANCE);
		RacesMask |= RacesAlliance;
	}
	else if (PlayerSaid(R, enough_hierarchy))
	{
		NPCPhrase(OK_ENOUGH_HIERARCHY);
		RacesMask |= RacesHierarchy;
	}
	else if (PlayerSaid(R, what_about_other))
	{
		NPCPhrase(ABOUT_OTHER);
		RacesMask |= RacesOther;
	}

	if (!(RacesMask & RacesAlliance))
	{
		Response(what_about_alliance, AllianceInfo);
	}
	if (!(RacesMask & RacesHierarchy))
	{
		Response(what_about_hierarchy, HierarchyInfo);
	}
	if (!(RacesMask & RacesOther))
	{
		Response(what_about_other, AlienRaces);
	}
	Response(enough_aliens, TellHistory);
}

static constexpr auto WarStarted {(1 << 0)};
static constexpr auto WarWasLike {(1 << 1)};
static constexpr auto WarLost {(1 << 2)};
static constexpr auto WarAftermath {(1 << 3)};

static void WarInfo(RESPONSE_REF R)
{
	static uint8_t WarMask = 0;

	if (PlayerSaid(R, the_war))
	{
		NPCPhrase(WHICH_WAR);
		WarMask = 0;
	}
	else if (PlayerSaid(R, what_started_war))
	{
		NPCPhrase(URQUAN_STARTED_WAR);
		WarMask |= WarStarted;
	}
	else if (PlayerSaid(R, what_was_war_like))
	{
		NPCPhrase(WAR_WAS_LIKE_SO);
		WarMask |= WarWasLike;
	}
	else if (PlayerSaid(R, why_lose_war))
	{
		NPCPhrase(LOST_WAR_BECAUSE);
		WarMask |= WarLost;
	}
	else if (PlayerSaid(R, what_after_war))
	{
		NPCPhrase(AFTER_WAR);
		WarMask |= WarAftermath;
	}

	if (!(WarMask & WarStarted))
	{
		Response(what_started_war, WarInfo);
	}
	if (!(WarMask & WarWasLike))
	{
		Response(what_was_war_like, WarInfo);
	}
	if (!(WarMask & WarLost))
	{
		Response(why_lose_war, WarInfo);
	}
	if (!(WarMask & WarAftermath))
	{
		Response(what_after_war, WarInfo);
	}
	Response(enough_war, TellHistory);
}

static constexpr auto AncientPrecursors {(1 << 0)};
static constexpr auto AncientRaces {(1 << 1)};
static constexpr auto AncientEarth {(1 << 2)};

static void AncientHistory(RESPONSE_REF R)
{
	static uint8_t AncientMask = 0;

	if (PlayerSaid(R, ancient_history))
	{
		NPCPhrase(WHICH_ANCIENT);
		AncientMask = 0;
	}
	else if (PlayerSaid(R, precursors))
	{
		NPCPhrase(ABOUT_PRECURSORS);
		AncientMask |= AncientPrecursors;
	}
	else if (PlayerSaid(R, old_races))
	{
		NPCPhrase(ABOUT_OLD_RACES);
		AncientMask |= AncientRaces;
	}
	else if (PlayerSaid(R, aliens_on_earth))
	{
		NPCPhrase(ABOUT_ALIENS_ON_EARTH);
		AncientMask |= AncientEarth;
	}

	if (!(AncientMask & AncientPrecursors))
	{
		Response(precursors, AncientHistory);
	}
	if (!(AncientMask & AncientRaces))
	{
		Response(old_races, AncientHistory);
	}
	if (!(AncientMask & AncientEarth))
	{
		Response(aliens_on_earth, AncientHistory);
	}
	Response(enough_ancient, TellHistory);
}

static void TellHistory(RESPONSE_REF R)
{
	RESPONSE_REF pstack[3];

	if (PlayerSaid(R, history))
	{
		NPCPhrase(WHICH_HISTORY);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
	}
	else if (PlayerSaid(R, enough_aliens))
	{
		NPCPhrase(OK_ENOUGH_ALIENS);

		stack0 = 1;
	}
	else if (PlayerSaid(R, enough_war))
	{
		NPCPhrase(OK_ENOUGH_WAR);

		stack1 = 1;
	}
	else if (PlayerSaid(R, enough_ancient))
	{
		NPCPhrase(OK_ENOUGH_ANCIENT);

		stack2 = 1;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = alien_races;
			break;
		default:
			pstack[0] = 0;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = the_war;
			break;
		default:
			pstack[1] = 0;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = ancient_history;
			break;
		default:
			pstack[2] = 0;
			break;
	}

	if (pstack[0])
	{
		Response(pstack[0], AlienRaces);
	}
	if (pstack[1])
	{
		Response(pstack[1], WarInfo);
	}
	if (pstack[2])
	{
		Response(pstack[2], AncientHistory);
	}
	Response(enough_history, NeedInfo);
}

static constexpr auto HowFindUrquan {(1 << 0)};
static constexpr auto HowFightUrquan {(1 << 1)};
static constexpr auto HowAllyAgainstUrquan {(1 << 2)};
static constexpr auto HowStrongAgainstUrquan {(1 << 3)};

static void DefeatUrquan(RESPONSE_REF R)
{
	static uint8_t DefeatMask = 0;

	if (PlayerSaid(R, how_defeat))
	{
		NPCPhrase(DEFEAT_LIKE_SO);
		DefeatMask = 0;
	}
	else if (PlayerSaid(R, how_find_urquan))
	{
		NPCPhrase(FIND_URQUAN);
		DefeatMask |= HowFindUrquan;
	}
	else if (PlayerSaid(R, how_fight_urquan))
	{
		NPCPhrase(FIGHT_URQUAN);
		DefeatMask |= HowFightUrquan;
	}
	else if (PlayerSaid(R, how_ally))
	{
		NPCPhrase(ALLY_LIKE_SO);
		DefeatMask |= HowAllyAgainstUrquan;
	}
	else if (PlayerSaid(R, how_get_strong))
	{
		NPCPhrase(STRONG_LIKE_SO);
		DefeatMask |= HowStrongAgainstUrquan;
	}

	if (!(DefeatMask & HowFindUrquan))
	{
		Response(how_find_urquan, DefeatUrquan);
	}
	if (!(DefeatMask & HowFightUrquan))
	{
		Response(how_fight_urquan, DefeatUrquan);
	}
	if (!(DefeatMask & HowAllyAgainstUrquan))
	{
		Response(how_ally, DefeatUrquan);
	}
	if (!(DefeatMask & HowStrongAgainstUrquan))
	{
		Response(how_get_strong, DefeatUrquan);
	}
	Response(enough_defeat, TellMission);
}

static void AnalyzeCondition(void)
{
	uint8_t i {};
	uint8_t num_thrusters = 0;
	uint8_t num_jets = 0;
	uint8_t num_guns = 0;
	uint8_t num_bays = 0;
	uint8_t num_batts = 0;
	uint8_t num_track = 0;
	uint8_t num_defense = 0;
	bool HasMinimum {};

	for (i = 0; i < NumDriveSlots; ++i)
	{
		if (uqm::gameState().sis().DriveSlots[i] < EmptySlot)
		{
			++num_thrusters;
		}
	}
	for (i = 0; i < NumJetSlots; ++i)
	{
		if (uqm::gameState().sis().JetSlots[i] < EmptySlot)
		{
			++num_jets;
		}
	}
	for (i = 0; i < NumModuleSlots; ++i)
	{
		uint8_t which_piece {};

		switch (which_piece = uqm::gameState().sis().ModuleSlots[i])
		{
			case STORAGE_BAY:
				++num_bays;
				break;
			case DYNAMO_UNIT:
			case SHIVA_FURNACE:
				num_batts += 1 + (which_piece - DYNAMO_UNIT);
				break;
			case GUN_WEAPON:
			case BLASTER_WEAPON:
			case CANNON_WEAPON:
				num_guns += 1 + (which_piece - GUN_WEAPON);
				break;
			case TRACKING_SYSTEM:
				++num_track;
				break;
			case ANTIMISSILE_DEFENSE:
				++num_defense;
				break;
		}
	}
	if (num_track && num_guns)
	{
		num_guns += 2;
	}

	HasMinimum = (num_thrusters >= 7 && num_jets >= 5
				  && uqm::gameState().sis().CrewEnlisted >= CrewPodCapacity
				  && uqm::gameState().sis().FuelOnBoard >= FuelTankCapacity
				  && num_bays >= 1 && uqm::gameState().sis().NumLanders
				  && num_batts >= 1 && num_guns >= 2);
	NPCPhrase(LETS_SEE);
	if (!HasMinimum && uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2)
	{
		NPCPhrase(IMPROVE_1);
		if (num_thrusters < 7)
		{
			NPCPhrase(NEED_THRUSTERS_1);
		}
		if (num_jets < 5)
		{
			NPCPhrase(NEED_TURN_1);
		}
		if (num_guns < 2)
		{
			NPCPhrase(NEED_GUNS_1);
		}
		if (uqm::gameState().sis().CrewEnlisted < CrewPodCapacity)
		{
			NPCPhrase(NEED_CREW_1);
		}
		if (uqm::gameState().sis().FuelOnBoard < FuelTankCapacity)
		{
			NPCPhrase(NEED_FUEL_1);
		}
		if (num_bays < 1)
		{
			NPCPhrase(NEED_STORAGE_1);
		}
		if (uqm::gameState().sis().NumLanders == 0)
		{
			NPCPhrase(NEED_LANDERS_2);
		}
		if (num_batts < 1)
		{
			NPCPhrase(NEED_DYNAMOS_1);
		}

		if (uqm::gameState().sis().ResUnits >= 3000)
		{
			NPCPhrase(IMPROVE_FLAGSHIP_WITH_RU);
		}
		else
		{
			NPCPhrase(GO_GET_MINERALS);
		}
	}
	else
	{
		uint8_t num_aliens = 0;
		bool HasMaximum {};

		const uint16_t FleetStrength {CalculateEscortsWorth()};
		for (i = 0; i < NUM_AVAILABLE_RACES; ++i)
		{
			if (i != HUMAN_SHIP && CheckAlliance((RACE_ID)i) == GOOD_GUY)
			{
				++num_aliens;
			}
		}

		HasMaximum = (num_thrusters == NumDriveSlots
					  && num_jets == NumJetSlots
					  && uqm::gameState().sis().CrewEnlisted >= CrewPodCapacity * 3
					  && uqm::gameState().sis().FuelOnBoard >= FuelTankCapacity * 3
					  && uqm::gameState().sis().NumLanders >= 3
					  && num_batts >= 4 && num_guns >= 7 && num_defense >= 2);
		if (!HasMaximum && uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2)
		{
			NPCPhrase(GOT_OK_FLAGSHIP);
		}
		else
		{
			NPCPhrase(GOT_AWESOME_FLAGSHIP);
		}

		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) >= 2)
		{
			NPCPhrase(CHMMR_IMPROVED_BOMB);
			if (FleetStrength < 20000)
			{
				NPCPhrase(MUST_ACQUIRE_AWESOME_FLEET);
			}
			else
			{
				NPCPhrase(GOT_AWESOME_FLEET);
				if (!uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
				{
					NPCPhrase(MUST_ELIMINATE_URQUAN_GUARDS);
				}
				else
				{
					NPCPhrase(GO_DESTROY_SAMATRA);
				}
			}
		}
		else if (num_aliens < 2)
		{
			NPCPhrase(GO_ALLY_WITH_ALIENS);
		}
		else
		{
			NPCPhrase(MADE_SOME_ALLIES);
			if (FleetStrength < 6000)
			{
				if (uqm::gameState().sis().ResUnits >= 3000)
				{
					NPCPhrase(BUY_COMBAT_SHIPS);
				}
				else
				{
					NPCPhrase(GET_SHIPS_BY_MINING_OR_ALLIANCE);
				}
			}
			else
			{
				NPCPhrase(GOT_OK_FLEET);
				if (!HasMaximum)
				{
					NPCPhrase(IMPROVE_2);
					if (num_thrusters < NumDriveSlots)
					{
						NPCPhrase(NEED_THRUSTERS_2);
					}
					if (num_jets < NumJetSlots)
					{
						NPCPhrase(NEED_TURN_2);
					}
					if (num_guns < 7)
					{
						NPCPhrase(NEED_GUNS_2);
					}
					if (uqm::gameState().sis().CrewEnlisted < CrewPodCapacity * 3)
					{
						NPCPhrase(NEED_CREW_2);
					}
					if (uqm::gameState().sis().FuelOnBoard < FuelTankCapacity * 3)
					{
						NPCPhrase(NEED_FUEL_2);
					}
					if (uqm::gameState().sis().NumLanders < 3)
					{
						NPCPhrase(NEED_LANDERS_1);
					}
					if (num_batts < 4)
					{
						NPCPhrase(NEED_DYNAMOS_2);
					}
					if (num_defense < 2)
					{
						NPCPhrase(NEED_POINT);
					}
				}
				else if (!uqm::gameState().flag(uqm::GameFlag::AWARE_OF_SAMATRA))
				{
					NPCPhrase(GO_LEARN_ABOUT_URQUAN);
				}
				else
				{
					NPCPhrase(KNOW_ABOUT_SAMATRA);
					if (!uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB))
					{
						NPCPhrase(FIND_WAY_TO_DESTROY_SAMATRA);
					}
					else if (uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
					{
						NPCPhrase(MUST_INCREASE_BOMB_STRENGTH);
					}
				}
			}
		}
	}
}

static void TellMission(RESPONSE_REF R)
{
	RESPONSE_REF pstack[4];

	if (PlayerSaid(R, our_mission))
	{
		NPCPhrase(WHICH_MISSION);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
		stack3 = 0;
	}
	else if (PlayerSaid(R, where_get_minerals))
	{
		NPCPhrase(GET_MINERALS);

		stack0 = 1;
	}
	else if (PlayerSaid(R, what_about_aliens))
	{
		NPCPhrase(ABOUT_ALIENS);

		stack1 = 1;
	}
	else if (PlayerSaid(R, what_do_now))
	{
		AnalyzeCondition();

		stack2 = 1;
	}
	else if (PlayerSaid(R, what_about_urquan))
	{
		NPCPhrase(MUST_DEFEAT);

		stack3 = 1;
	}
	else if (PlayerSaid(R, enough_defeat))
	{
		NPCPhrase(OK_ENOUGH_DEFEAT);

		stack3 = 2;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = where_get_minerals;
			break;
		default:
			pstack[0] = 0;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = what_about_aliens;
			break;
		default:
			pstack[1] = 0;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = what_do_now;
			break;
		default:
			pstack[2] = 0;
			break;
	}
	switch (stack3)
	{
		case 0:
			pstack[3] = what_about_urquan;
			break;
		case 1:
			pstack[3] = how_defeat;
			break;
		default:
			pstack[3] = 0;
			break;
	}

	if (pstack[0])
	{
		Response(pstack[0], TellMission);
	}
	if (pstack[1])
	{
		Response(pstack[1], TellMission);
	}
	if (pstack[2])
	{
		Response(pstack[2], TellMission);
	}
	if (pstack[3])
	{
		if (stack3 == 1)
		{
			Response(pstack[3], DefeatUrquan);
		}
		else
		{
			Response(pstack[3], TellMission);
		}
	}

	Response(enough_mission, NeedInfo);
}

static void TellStarBase(RESPONSE_REF R)
{
	RESPONSE_REF pstack[4];

	if (PlayerSaid(R, starbase_functions))
	{
		NPCPhrase(WHICH_FUNCTION);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
		stack3 = 0;
	}
	else if (PlayerSaid(R, tell_me_about_fuel))
	{
		NPCPhrase(ABOUT_FUEL);

		stack1 = 1;
	}
	else if (PlayerSaid(R, tell_me_about_crew))
	{
		NPCPhrase(ABOUT_CREW);

		stack2 = 2;
	}
	else if (PlayerSaid(R, tell_me_about_modules))
	{
		NPCPhrase(ABOUT_MODULES);

		stack0 = 1;
	}
	else if (PlayerSaid(R, tell_me_about_ships))
	{
		NPCPhrase(ABOUT_SHIPS);

		stack2 = 1;
	}
	else if (PlayerSaid(R, tell_me_about_ru))
	{
		NPCPhrase(ABOUT_RU);

		stack3 = 1;
	}
	else if (PlayerSaid(R, tell_me_about_minerals))
	{
		NPCPhrase(ABOUT_MINERALS);

		stack3 = 2;
	}
	else if (PlayerSaid(R, tell_me_about_life))
	{
		NPCPhrase(ABOUT_LIFE);

		stack3 = 3;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = tell_me_about_modules;
			break;
		default:
			pstack[0] = 0;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = tell_me_about_fuel;
			break;
		default:
			pstack[1] = 0;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = tell_me_about_ships;
			break;
		case 1:
			pstack[2] = tell_me_about_crew;
			break;
		default:
			pstack[2] = 0;
			break;
	}
	switch (stack3)
	{
		case 0:
			pstack[3] = tell_me_about_ru;
			break;
		case 1:
			pstack[3] = tell_me_about_minerals;
			break;
		case 2:
			pstack[3] = tell_me_about_life;
			break;
		default:
			pstack[3] = 0;
			break;
	}

	if (pstack[0])
	{
		Response(pstack[0], TellStarBase);
	}
	if (pstack[1])
	{
		Response(pstack[1], TellStarBase);
	}
	if (pstack[2])
	{
		Response(pstack[2], TellStarBase);
	}
	if (pstack[3])
	{
		Response(pstack[3], TellStarBase);
	}

	Response(enough_starbase, NeedInfo);
}

static void NormalStarbase(RESPONSE_REF R);

static void NeedInfo(RESPONSE_REF R)
{
	if (PlayerSaid(R, need_info))
	{
		NPCPhrase(WHAT_KIND_OF_INFO);
	}
	else if (PlayerSaid(R, enough_starbase))
	{
		NPCPhrase(OK_ENOUGH_STARBASE);
	}
	else if (PlayerSaid(R, enough_history))
	{
		NPCPhrase(OK_ENOUGH_HISTORY);
	}
	else if (PlayerSaid(R, enough_mission))
	{
		NPCPhrase(OK_ENOUGH_MISSION);
	}

	Response(starbase_functions, TellStarBase);
	Response(history, TellHistory);
	Response(our_mission, TellMission);
	Response(no_need_info, NormalStarbase);
}

static bool DiscussDevices(bool TalkAbout)
{
	//bool Undiscussed; unused

	if (TalkAbout)
	{
		NPCPhrase(DEVICE_HEAD);
	}
	uint16_t PhraseIndex {2};

	uint16_t VuxBeastIndex {0};
	//Undiscussed = false; unused
	for (uint16_t i {0}; i < NUM_DEVICES; ++i)
	{
		RESPONSE_REF pStr {0};
		switch (i)
		{
			case ROSY_SPHERE_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ROSY_SPHERE))
				{
					pStr = ABOUT_SPHERE;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ROSY_SPHERE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case WIMBLIS_TRIDENT_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_WIMBLIS_TRIDENT))
				{
					pStr = ABOUT_WIMBLIS_TRIDENT;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_WIMBLIS_TRIDENT, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case GLOWING_ROD_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::GLOWING_ROD_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_GLOWING_ROD))
				{
					pStr = ABOUT_GLOWING_ROD;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_GLOWING_ROD, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case SUN_EFFICIENCY_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_SUN_EFFICIENCY))
				{
					pStr = ABOUT_SUN;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_SUN_EFFICIENCY, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case UTWIG_BOMB_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_UTWIG_BOMB))
				{
					pStr = ABOUT_BOMB;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_UTWIG_BOMB, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case ULTRON_0_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 1
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case ULTRON_1_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 2
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_1;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case ULTRON_2_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 3
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_2;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case ULTRON_3_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 4
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_3;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case MAIDENS_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_MAIDENS))
				{
					pStr = ABOUT_MAIDENS;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_MAIDENS, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case TALKING_PET_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_TALKING_PET))
				{
					pStr = ABOUT_TALKPET;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_TALKING_PET, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case AQUA_HELIX_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_AQUA_HELIX))
				{
					pStr = ABOUT_HELIX;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_AQUA_HELIX, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case CLEAR_SPINDLE_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_CLEAR_SPINDLE))
				{
					pStr = ABOUT_SPINDLE;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_CLEAR_SPINDLE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case UMGAH_HYPERWAVE_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::UMGAH_BROADCASTERS_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_UMGAH_HYPERWAVE))
				{
					pStr = ABOUT_UCASTER;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_UMGAH_HYPERWAVE, static_cast<uint32_t>(TalkAbout));
				}
				break;
#ifdef NEVER
			case DATA_PLATE_1_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::DATA_PLATE_1_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DATA_PLATE_1))
				{
					pStr = ABOUT_DATAPLATE_1;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DATA_PLATE_1, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case DATA_PLATE_2_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::DATA_PLATE_2_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DATA_PLATE_2))
				{
					pStr = ABOUT_DATAPLATE_2;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DATA_PLATE_2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case DATA_PLATE_3_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::DATA_PLATE_3_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DATA_PLATE_3))
				{
					pStr = ABOUT_DATAPLATE_3;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DATA_PLATE_3, static_cast<uint32_t>(TalkAbout));
				}
				break;
#endif /* NEVER */
			case TAALO_PROTECTOR_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_TAALO_PROTECTOR))
				{
					pStr = ABOUT_SHIELD;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_TAALO_PROTECTOR, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case EGG_CASING0_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_EGG_CASING0))
				{
					pStr = ABOUT_EGGCASE_0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING0, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING1, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case EGG_CASING1_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_EGG_CASING1))
				{
					pStr = ABOUT_EGGCASE_0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING0, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING1, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case EGG_CASING2_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_EGG_CASING2))
				{
					pStr = ABOUT_EGGCASE_0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING0, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING1, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case SYREEN_SHUTTLE_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_SYREEN_SHUTTLE))
				{
					pStr = ABOUT_SHUTTLE;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_SYREEN_SHUTTLE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case VUX_BEAST_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::VUX_BEAST_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_VUX_BEAST))
				{
					pStr = ABOUT_VUXBEAST0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_VUX_BEAST, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case DESTRUCT_CODE_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DESTRUCT_CODE))
				{
					pStr = ABOUT_DESTRUCT;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DESTRUCT_CODE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case PORTAL_SPAWNER_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_PORTAL_SPAWNER))
				{
					pStr = ABOUT_PORTAL;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_PORTAL_SPAWNER, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case URQUAN_WARP_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::PORTAL_KEY_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_URQUAN_WARP))
				{
					pStr = ABOUT_WARPPOD;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_URQUAN_WARP, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case BURVIX_HYPERWAVE_DEVICE:
				if (uqm::gameState().flag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_BURVIX_HYPERWAVE))
				{
					pStr = ABOUT_BCASTER;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_BURVIX_HYPERWAVE, static_cast<uint32_t>(TalkAbout));
				}
				break;
		}

		if (pStr)
		{
			if (TalkAbout)
			{
				if (PhraseIndex > 2)
				{
					NPCPhrase(BETWEEN_DEVICES);
				}
				NPCPhrase(pStr);
				if (pStr == ABOUT_VUXBEAST0)
				{
					VuxBeastIndex = ++PhraseIndex;
					NPCPhrase(ABOUT_VUXBEAST1);
				}
			}
			PhraseIndex += 2;
		}
	}

	if (TalkAbout)
	{
		NPCPhrase(DEVICE_TAIL);

		if (VuxBeastIndex)
		{
			// Run all tracks upto the Vux Beast scientist's report
			// Then disable Commander's speech animation and run the report
			BlockTalkingAnim(VuxBeastIndex - 1, VuxBeastIndex);
			AlienTalkSegue((uint16_t)~0);
		}
	}

	return (PhraseIndex > 2);
}

static bool CheckTiming(uint16_t month_index, uint16_t day_index)
{
	uint16_t mi {};
	const uint8_t days_in_month[12] =
		{
			31,
			28,
			31,
			30,
			31,
			30,
			31,
			31,
			30,
			31,
			30,
			31,
		};

	mi = uqm::gameState().flag(uqm::GameFlag::STARBASE_MONTH);
	uint16_t year_index {StartYear};

	day_index += uqm::gameState().flag(uqm::GameFlag::STARBASE_DAY);
	while (day_index > days_in_month[mi - 1])
	{
		day_index -= days_in_month[mi - 1];
		if (++mi > 12)
		{
			mi = 1;
			++year_index;
		}
	}

	month_index += mi;
	year_index += (month_index - 1) / 12;
	month_index = ((month_index - 1) % 12) + 1;

	return (year_index < uqm::gameState().clock().year()
			|| (year_index == uqm::gameState().clock().year()
				&& (month_index < uqm::gameState().clock().month()
					|| (month_index == uqm::gameState().clock().month()
						&& day_index < uqm::gameState().clock().day()))));
}

static constexpr auto NumGiftArilous {3};

static void CheckBulletins(bool Repeat)
{
	uint32_t BulletinMask {};
	const uint16_t CrewSold = uqm::makeWord(
		uqm::gameState().flag(uqm::GameFlag::CREW_SOLD_TO_DRUUGE0),
		uqm::gameState().flag(uqm::GameFlag::CREW_SOLD_TO_DRUUGE1));

	if (Repeat)
	{
		BulletinMask = CurBulletinMask ^ 0xFFFFFFFFL;
	}
	else
	{
		BulletinMask = uqm::gameState().flag(uqm::GameFlag::STARBASE_BULLETS);
	}

	RESPONSE_REF pIntro {0};
	for (uint8_t b0 {0}; b0 < 32; ++b0)
	{
		if (!(BulletinMask & (1L << b0)))
		{
			RESPONSE_REF pStr {0};
			switch (b0)
			{
				case 0:
					if (CheckAlliance(SPATHI_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_1;
					}
					break;
				case 1:
					if (CheckAlliance(ZOQFOTPIK_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_2;
					}
					break;
				case 2:
					if (CheckAlliance(SUPOX_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_3;
					}
					break;
				case 3:
					if (CheckAlliance(UTWIG_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_4;
					}
					break;
				case 4:
					if (CheckAlliance(ORZ_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_5;
					}
					break;
				case 5:
					if (uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER) == 2)
					{
						BulletinMask |= 1L << b0;
					}
					else if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER)
							 && (Repeat || EscortFeasibilityStudy(ARILOU_SHIP)))
					{
						pStr = STARBASE_BULLETIN_6;
						if (!Repeat)
						{
							AddEscortShips(ARILOU_SHIP, NumGiftArilous);
						}
					}
					break;
				case 6:
					if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS) == 1)
					{
						pStr = STARBASE_BULLETIN_7;
					}
					break;
				case 7:
					if (uqm::gameState().flag(uqm::GameFlag::MET_MELNORME))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(ifEasyDifficulty(1, 3), 0))
					{
						pStr = STARBASE_BULLETIN_8;
					}
					break;
				case 8:
					if (uqm::gameState().flag(uqm::GameFlag::MET_MELNORME))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(ifEasyDifficulty(3, 6), 0))
					{
						pStr = STARBASE_BULLETIN_9;
					}
					break;
				case 9:
					if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(0, 7))
					{
						pStr = STARBASE_BULLETIN_10;
					}
					break;
				case 10:
					if (uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES))
					{
						pStr = STARBASE_BULLETIN_11;
					}
					break;
				case 11:
					if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_HOME_VISITS)
						|| uqm::gameState().flag(uqm::GameFlag::ZOQFOT_GRPOFFS))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(0, ifEasyDifficulty(21, 42)))
					{
						pStr = STARBASE_BULLETIN_12;
					}
					break;
				case 12:
					if (CheckAlliance(CHMMR_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_13;
					}
					break;
				case 13:
					if (CheckAlliance(SHOFIXTI_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_14;
					}
					break;
				case 14:
					if (uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION))
					{
						pStr = STARBASE_BULLETIN_15;
					}
					break;
				case 15:
					if (uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(7, 0))
					{
						pStr = STARBASE_BULLETIN_16;
					}
					break;
				case 16:
					break;
				case 17:
					if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
					{
						pStr = STARBASE_BULLETIN_18;
					}
					break;
				case 18:
					if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 2)
					{
						pStr = STARBASE_BULLETIN_19;
					}
					break;
				case 19:
					break;
				case 20:
					break;
				case 21:
					if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS) == 2)
					{
						pStr = STARBASE_BULLETIN_22;
					}
					break;
				case 22:
					break;
				case 23:
					break;
				case 24:
					break;
				case 25:
					break;
				case 26:
					{
						const uint16_t crew_sold = CrewSold;

						if (crew_sold > MinSold())
						{
							BulletinMask |= 1L << b0;
						}
						else if (crew_sold)
						{
							pStr = STARBASE_BULLETIN_27;
						}
						break;
					}
				case 27:
					{
						const uint16_t crew_sold = CrewSold;

						if (crew_sold > MaxSold())
						{
							BulletinMask |= 1L << b0;
						}
						else if (crew_sold > MinSold())
						{
							pStr = STARBASE_BULLETIN_28;
						}
						break;
					}
				case 28:
					{
						uint16_t crew_bought {};

						crew_bought = uqm::makeWord(
							uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED0),
							uqm::gameState().flag(uqm::GameFlag::CREW_PURCHASED1));
						if (crew_bought >= CrewExpenseThreshold)
						{
							pStr = STARBASE_BULLETIN_29;
						}
						break;
					}
				case 29:
					if (CrewSold > MaxSold())
					{
						pStr = STARBASE_BULLETIN_30;
					}
					break;
				case 30:
					break;
				case 31:
					break;
			}

			if (pStr)
			{
				if (pIntro)
				{
					NPCPhrase(BETWEEN_BULLETINS);
				}
				else if (Repeat)
				{
					pIntro = BEFORE_WE_GO_ON_1;
				}
				else
				{
					switch ((uint8_t)uqm::random().next() % 7)
					{
						case 0:
							pIntro = BEFORE_WE_GO_ON_1;
							break;
						case 1:
							pIntro = BEFORE_WE_GO_ON_2;
							break;
						case 2:
							pIntro = BEFORE_WE_GO_ON_3;
							break;
						case 3:
							pIntro = BEFORE_WE_GO_ON_4;
							break;
						case 4:
							pIntro = BEFORE_WE_GO_ON_5;
							break;
						case 5:
							pIntro = BEFORE_WE_GO_ON_6;
							break;
						default:
							pIntro = BEFORE_WE_GO_ON_7;
							break;
					}

					NPCPhrase(pIntro);
				}

				NPCPhrase(pStr);
				CurBulletinMask |= 1L << b0;
			}
		}
	}

	if (pIntro == 0 && uqm::gameState().flag(uqm::GameFlag::STARBASE_VISITED))
	{
		NPCPhrase(RETURN_HELLO);
	}
	else if (!Repeat)
	{
		uqm::gameState().setFlag(uqm::GameFlag::STARBASE_BULLETS, static_cast<uint32_t>(BulletinMask));
	}
}

static void NormalStarbase(RESPONSE_REF R)
{
	if (PlayerSaid(R, no_need_info))
	{
		NPCPhrase(OK_NO_NEED_INFO);
	}
	else if (PlayerSaid(R, new_devices))
	{
		DiscussDevices(true);
	}
	else if (PlayerSaid(R, repeat_bulletins))
	{
		CheckBulletins(true);
	}
	else if (R == 0)
	{
		if (uqm::gameState().flag(uqm::GameFlag::MOONBASE_ON_SHIP))
		{
			NPCPhrase(STARBASE_IS_READY);
			DeltaSISGauges(0, 0, 2500);
			if (uqm::UQMOptions::read().infiniteRU)
			{
				oldRU = 2500;
			}
			uqm::gameState().setFlag(uqm::GameFlag::STARBASE_MONTH, static_cast<uint32_t>(uqm::gameState().clock().month()));
			uqm::gameState().setFlag(uqm::GameFlag::STARBASE_DAY, static_cast<uint32_t>(uqm::gameState().clock().day()));
		}
		else if (uqm::gameState().flag(uqm::GameFlag::STARBASE_VISITED))
		{
			CheckBulletins(false);
		}
		else
		{
			// XXX TODO: This can be simplified now.
			RESPONSE_REF pStr = 0;

			switch ((uint8_t)uqm::random().next() & 7)
			{
				case 0:
					pStr = NORMAL_HELLO_A;
					break;
				case 1:
					pStr = NORMAL_HELLO_B;
					break;
				case 2:
					pStr = NORMAL_HELLO_C;
					break;
				case 3:
					pStr = NORMAL_HELLO_D;
					break;
				case 4:
					pStr = NORMAL_HELLO_E;
					break;
				case 5:
					pStr = NORMAL_HELLO_F;
					break;
				case 6:
					pStr = NORMAL_HELLO_G;
					break;
				case 7:
					pStr = NORMAL_HELLO_H;
					break;
			}
			NPCPhrase(pStr);
			CheckBulletins(false);
		}

		uqm::gameState().setFlag(uqm::GameFlag::STARBASE_VISITED, static_cast<uint32_t>(1));
	}

	if (uqm::gameState().sis().TotalElementMass)
	{
		Response(have_minerals, SellMinerals);
	}
	if (DiscussDevices(false))
	{
		Response(new_devices, NormalStarbase);
	}
	if (CurBulletinMask)
	{
		Response(repeat_bulletins, NormalStarbase);
	}
	Response(need_info, NeedInfo);
	Response(goodbye_commander, ByeBye);
}

static void SellMinerals(RESPONSE_REF R)
{
	uint16_t total = 0;
	RESPONSE_REF pStr = 0;

	if (uqm::UQMOptions::read().useSpeech)
	{
		total = DoSellMinerals();

		pStr = EveryOnesACritic(total);

		NPCPhrase(pStr);
		if (pStr == LIGHT_LOAD_C0)
		{
			NPCPhrase(LIGHT_LOAD_C1);
		}
	}
	else
	{
		uint16_t amount[NUM_ELEMENT_CATEGORIES];
		uint16_t Ru[NUM_ELEMENT_CATEGORIES];
		uint16_t seg = 0;
		uint16_t count = 0;

		NPCPhrase(CARGO_LIST);
		for (uint16_t i {0}; i < NUM_ELEMENT_CATEGORIES; ++i)
		{
			if ((amount[seg] = uqm::gameState().sis().ElementAmounts[i]) != 0)
			{
				Ru[seg] = amount[seg] * uqm::gameState().game().ElementWorth[i];
				if (count > 0)
				{
					NPCPhrase(ELLIPSES);
				}
				else
				{
					NPCPhrase(BLANK);
				}
				NPCNumber(amount[seg], nullptr);
				NPCPhrase_splice(KILOTONS_OF);
				NPCPhrase_splice(COMMONR + i);
				NPCPhrase_splice(FOR);
				NPCNumber(Ru[seg], nullptr);
				NPCPhrase(RESUNITS);
				total += Ru[seg];
				seg++;
				uqm::gameState().sis().ElementAmounts[i] = 0;
				++count;
			}
		}

		pStr = EveryOnesACritic(total);

		NPCPhrase(pStr);
		if (pStr == LIGHT_LOAD_C0)
		{
			NPCPhrase(LIGHT_LOAD_C1);
		}

		for (uint16_t sseg {1}; sseg < seg + 1; sseg++)
		{
			AlienTalkSegue(sseg);
			uqm::gameState().sis().TotalElementMass -= amount[sseg - 1];
			DeltaSISGauges(0, 0, Ru[sseg - 1]);
			ClearSISRect(DrawSisDisplay);
		}
	}

	NormalStarbase(R);
}

static void Intro(void)
{
	if (IsHd()) // To smooth out HdResolutionFactor blink animation
	{
		CommData.AlienAmbientArray[0].BaseFrameRate = GameTicksPerSecond / 40;
	}

	NormalStarbase(0);
}

static uint16_t uninit_starbase(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_starbase_enc(void)
{
	uqm::gameState().setFlag(uqm::GameFlag::MOONBASE_ON_SHIP, static_cast<uint32_t>(0));
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 2)
	{
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_BOMB_STATE, static_cast<uint32_t>(3));
	}
}

ConversationData* init_starbase_comm()
{
	commander_desc.init_encounter_func = Intro;
	commander_desc.post_encounter_func = post_starbase_enc;
	commander_desc.uninit_encounter_func = uninit_starbase;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	commander_desc.AlienTextWidth = ResScale(143);
	commander_desc.AlienTextBaseline.x = ResScale(164)
									   - (SafeNeg(2) * 2);
	commander_desc.AlienTextBaseline.y = ResScale(20);

	// use alternate Starbase track if available
	altResFlags |= UseAltSong;

	if (is3DO(uqm::UQMOptions::read().flagshipColor))
	{
		altResFlags |= UseAltFrame;
	}

	CurBulletinMask = 0;
	setSegue(Segue_peace);
	ConversationData* retval {&commander_desc};

	return (retval);
}
