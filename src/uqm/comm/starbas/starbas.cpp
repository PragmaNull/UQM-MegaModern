//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS StarbasePhrase::For A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../comandr/resinst.h"
#include "../commall.h"
#include "options.h"
#include "strings.h"
#include "uqm/build.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/setup.h"
#include "uqm/shipcont.h"
#include "uqm/sis.h"
// for DeltaSISGauges()
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"
#include "uqm/controls.h"
#include "uqm/nameref.h"
#include "uqm/oscill.h"
#include "uqm/random/UQMRandom.h"


namespace uqm
{
static void TellMission(ResponseRef R);
static void SellMinerals(ResponseRef R);


static ConversationData commander_desc =
	{
		Conversation::Commander, /* AlienConv */
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
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Middle, /* AlienTextValign */
		CommanderColorMap, /* AlienColorMap */
		CommanderMusic, /* AlienSong */
		{
								CommanderPmapAnimRed, /* AlienAltFrame */
			NullResource,		  /* AlienAltColorMap */
			StarbaseAltMusic,	  /* AlienAltSong */
		},
		StarbaseConversationPhrases, /* PlayerPhrases */
		10, /* NumAnimations */
		{{
			/* AlienAmbientArray (ambient animations) */
			{
				/* Blink */
				1,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Running light */
				10,						  /* StartIndex */
				30,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 2,
				0, /* RestartRate */
				0, /* BlockMask */
			},
			{
				/* Arc welder 0 */
				40,						  /* StartIndex */
				7,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Arc welder 1 */
				47,						  /* StartIndex */
				8,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Arc welder 2 */
				55,						  /* StartIndex */
				6,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Arc welder 3 */
				61,						  /* StartIndex */
				6,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Arc welder 4 */
				67,						  /* StartIndex */
				7,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Arc welder 5 */
				74,						  /* StartIndex */
				11,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Arc welder 6 */
				85,						  /* StartIndex */
				10,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				/* Flagship picture */
				95,					  /* StartIndex */
				1,					  /* NumFrames */
				AnimationFlags::None, /* AnimFlags */
				0,
				0, /* FrameRate */
				0,
				0, /* RestartRate */
				0, /* BlockMask */
			},
		}},
		{
								/* AlienTransitionDesc */
			0,					  /* StartIndex */
			0,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			0,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			4,					  /* StartIndex */
			6,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
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

	for (uint16_t i {0}; i < MineralCategory::NumElementCategories; ++i)
	{
		uint16_t amount {};
		TimeCount TimeIn {};

		if (i == 0)
		{
			DrawCargoStrings((uint8_t)~0, (uint8_t)~0);
			TimeIn = GetTimeCounter() + c_gameSecond / 2;
			while (GetTimeCounter() <= TimeIn)
			{
				if (AnyButtonPress(true) || (uqm::currentActivity() & ActivityFlag::CheckAbort))
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
				if (!Sleepy || AnyButtonPress(true) || (uqm::currentActivity() & ActivityFlag::CheckAbort))
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
				if (AnyButtonPress(true) || (uqm::currentActivity() & ActivityFlag::CheckAbort))
				{
					Sleepy = false;
					break;
				}
				UpdateDuty(false);
			}
		}
	}

	SleepThread(c_gameSecond / 2);

	ClearSISRect(ClearSisFlags::DrawDisplay);

	return total;
}

static StarbasePhrase EveryOnesACritic(uint16_t total)
{
	StarbasePhrase pStr {StarbasePhrase::LightLoadA};

	if (total < 1000)
	{
		total = uqm::gameState().flag(uqm::GameFlag::LIGHT_MINERAL_LOAD);
		switch (total++)
		{
			case 0:
				pStr = StarbasePhrase::LightLoadA;
				break;
			case 1:
				pStr = StarbasePhrase::LightLoadB;
				break;
			case 2:
				// There are two separate sound samples in this case.
				pStr = StarbasePhrase::LightLoadC0;
				//pStr2 = StarbasePhrase::LightLoadC1; Kruzen: processed outside this func now
				break;
			case 3:
				pStr = StarbasePhrase::LightLoadD;
				break;
			case 4:
				pStr = StarbasePhrase::LightLoadE;
				break;
			case 5:
				pStr = StarbasePhrase::LightLoadF;
				break;
			case 6:
				--total;
				pStr = StarbasePhrase::LightLoadG;
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
				pStr = StarbasePhrase::MediumLoadA;
				break;
			case 1:
				pStr = StarbasePhrase::MediumLoadB;
				break;
			case 2:
				pStr = StarbasePhrase::MediumLoadC;
				break;
			case 3:
				pStr = StarbasePhrase::MediumLoadD;
				break;
			case 4:
				pStr = StarbasePhrase::MediumLoadE;
				break;
			case 5:
				pStr = StarbasePhrase::MediumLoadF;
				break;
			case 6:
				--total;
				pStr = StarbasePhrase::MediumLoadG;
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
				pStr = StarbasePhrase::HeavyLoadA;
				break;
			case 1:
				pStr = StarbasePhrase::HeavyLoadB;
				break;
			case 2:
				pStr = StarbasePhrase::HeavyLoadC;
				break;
			case 3:
				pStr = StarbasePhrase::HeavyLoadD;
				break;
			case 4:
				pStr = StarbasePhrase::HeavyLoadE;
				break;
			case 5:
				pStr = StarbasePhrase::HeavyLoadF;
				break;
			case 6:
				--total;
				pStr = StarbasePhrase::HeavyLoadG;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::HEAVY_MINERAL_LOAD, static_cast<uint32_t>(total));
	}

	return pStr;
}

static void ByeBye(ResponseRef R)
{
	(void)R; // ignored

	CurBulletinMask |= uqm::gameState().flag(uqm::GameFlag::STARBASE_BULLETS);
	uqm::gameState().setFlag(uqm::GameFlag::STARBASE_BULLETS, static_cast<uint32_t>(CurBulletinMask));

	/* if (R == goodbye_starbase_commander) */
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) >= 2)
	{
		NPCPhrase(StarbasePhrase::GoodLuckAgain);
	}
	else
	{
		StarbasePhrase pStr {};

		switch ((uint8_t)uqm::random().next() & 7)
		{
			case 0:
				pStr = StarbasePhrase::NormalGoodbyeA;
				break;
			case 1:
				pStr = StarbasePhrase::NormalGoodbyeB;
				break;
			case 2:
				pStr = StarbasePhrase::NormalGoodbyeC;
				break;
			case 3:
				pStr = StarbasePhrase::NormalGoodbyeD;
				break;
			case 4:
				pStr = StarbasePhrase::NormalGoodbyeE;
				break;
			case 5:
				pStr = StarbasePhrase::NormalGoodbyeF;
				break;
			case 6:
				pStr = StarbasePhrase::NormalGoodbyeG;
				break;
			case 7:
				pStr = StarbasePhrase::NormalGoodbyeH;
				break;
		}

		NPCPhrase(pStr);
	}
}

static void NeedInfo(ResponseRef R);
static void TellHistory(ResponseRef R);
static void AlienRaces(ResponseRef R);

static uint8_t stack0;
static uint8_t stack1;
static uint8_t stack2;
static uint8_t stack3;

static constexpr auto AllianceShofixti {1 << 0};
static constexpr auto AllianceYehat {1 << 1};
static constexpr auto AllianceArilou {1 << 2};
static constexpr auto AllianceChenjesu {1 << 3};
static constexpr auto AllianceMmrnmhrm {1 << 4};
static constexpr auto AllianceSyreen {1 << 5};

static void AllianceInfo(ResponseRef R)
{
	static uint8_t AllianceMask = 0;

	if (PlayerSaid(R, StarbasePhrase::WhatAboutAlliance))
	{
		NPCPhrase(StarbasePhrase::WhichAlliance);
		AllianceMask = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::Shofixti))
	{
		NPCPhrase(StarbasePhrase::AboutShofixti);
		AllianceMask |= AllianceShofixti;
		SetHomeworldKnown(*HomeId::Shofixti);
	}
	else if (PlayerSaid(R, StarbasePhrase::Yehat))
	{
		NPCPhrase(StarbasePhrase::AboutYehat);
		AllianceMask |= AllianceYehat;
	}
	else if (PlayerSaid(R, StarbasePhrase::Arilou))
	{
		NPCPhrase(StarbasePhrase::AboutArilou);
		AllianceMask |= AllianceArilou;
	}
	else if (PlayerSaid(R, StarbasePhrase::Chenjesu))
	{
		NPCPhrase(StarbasePhrase::AboutChenjesu);
		AllianceMask |= AllianceChenjesu;
	}
	else if (PlayerSaid(R, StarbasePhrase::Mmrnmhrm))
	{
		NPCPhrase(StarbasePhrase::AboutMmrnmhrm);
		AllianceMask |= AllianceMmrnmhrm;
	}
	else if (PlayerSaid(R, StarbasePhrase::Syreen))
	{
		NPCPhrase(StarbasePhrase::AboutSyreen);
		AllianceMask |= AllianceSyreen;
	}

	if (!(AllianceMask & AllianceShofixti))
	{
		Response(StarbasePhrase::Shofixti, AllianceInfo);
	}
	if (!(AllianceMask & AllianceYehat))
	{
		Response(StarbasePhrase::Yehat, AllianceInfo);
	}
	if (!(AllianceMask & AllianceArilou))
	{
		Response(StarbasePhrase::Arilou, AllianceInfo);
	}
	if (!(AllianceMask & AllianceChenjesu))
	{
		Response(StarbasePhrase::Chenjesu, AllianceInfo);
	}
	if (!(AllianceMask & AllianceMmrnmhrm))
	{
		Response(StarbasePhrase::Mmrnmhrm, AllianceInfo);
	}
	if (!(AllianceMask & AllianceSyreen))
	{
		Response(StarbasePhrase::Syreen, AllianceInfo);
	}
	Response(StarbasePhrase::EnoughAlliance, AlienRaces);
}

static constexpr auto HierarchyMycon {1 << 0};
static constexpr auto HierarchySpathi {1 << 1};
static constexpr auto HierarchyUmgah {1 << 2};
static constexpr auto HierarchyAndrosynth {1 << 3};
static constexpr auto HierarchyIlwrath {1 << 4};
static constexpr auto HierarchyVux {1 << 5};
static constexpr auto HierarchyUrquan {1 << 6};

static void HierarchyInfo(ResponseRef R)
{
	static uint8_t HierarchyMask = 0;

	if (PlayerSaid(R, StarbasePhrase::WhatAboutHierarchy))
	{
		NPCPhrase(StarbasePhrase::WhichHierarchy);
		HierarchyMask = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::Urquan))
	{
		NPCPhrase(StarbasePhrase::AboutUrquan);
		HierarchyMask |= HierarchyUrquan;
	}
	else if (PlayerSaid(R, StarbasePhrase::Mycon))
	{
		NPCPhrase(StarbasePhrase::AboutMycon);
		HierarchyMask |= HierarchyMycon;
	}
	else if (PlayerSaid(R, StarbasePhrase::Spathi))
	{
		NPCPhrase(StarbasePhrase::AboutSpathi);
		HierarchyMask |= HierarchySpathi;
	}
	else if (PlayerSaid(R, StarbasePhrase::Umgah))
	{
		NPCPhrase(StarbasePhrase::AboutUmgah);
		HierarchyMask |= HierarchyUmgah;
	}
	else if (PlayerSaid(R, StarbasePhrase::Androsynth))
	{
		NPCPhrase(StarbasePhrase::AboutAndrosynth);
		HierarchyMask |= HierarchyAndrosynth;
		SetHomeworldKnown(*HomeId::Androsynth);
	}
	else if (PlayerSaid(R, StarbasePhrase::Ilwrath))
	{
		NPCPhrase(StarbasePhrase::AboutIlwrath);
		HierarchyMask |= HierarchyIlwrath;
	}
	else if (PlayerSaid(R, StarbasePhrase::Vux))
	{
		NPCPhrase(StarbasePhrase::AboutVux);
		HierarchyMask |= HierarchyVux;
	}

	if (!(HierarchyMask & HierarchyUrquan))
	{
		Response(StarbasePhrase::Urquan, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyMycon))
	{
		Response(StarbasePhrase::Mycon, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchySpathi))
	{
		Response(StarbasePhrase::Spathi, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyUmgah))
	{
		Response(StarbasePhrase::Umgah, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyAndrosynth))
	{
		Response(StarbasePhrase::Androsynth, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyIlwrath))
	{
		Response(StarbasePhrase::Ilwrath, HierarchyInfo);
	}
	if (!(HierarchyMask & HierarchyVux))
	{
		Response(StarbasePhrase::Vux, HierarchyInfo);
	}
	Response(StarbasePhrase::EnoughHierarchy, AlienRaces);
}

static constexpr auto RacesAlliance {1 << 0};
static constexpr auto RacesHierarchy {1 << 1};
static constexpr auto RacesOther {1 << 2};

static void AlienRaces(ResponseRef R)
{
	static uint8_t RacesMask = 0;

	if (PlayerSaid(R, StarbasePhrase::AlienRaces))
	{
		NPCPhrase(StarbasePhrase::WhichAlien);
		RacesMask = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughAlliance))
	{
		NPCPhrase(StarbasePhrase::OkEnoughAlliance);
		RacesMask |= RacesAlliance;
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughHierarchy))
	{
		NPCPhrase(StarbasePhrase::OkEnoughHierarchy);
		RacesMask |= RacesHierarchy;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhatAboutOther))
	{
		NPCPhrase(StarbasePhrase::AboutOther);
		RacesMask |= RacesOther;
	}

	if (!(RacesMask & RacesAlliance))
	{
		Response(StarbasePhrase::WhatAboutAlliance, AllianceInfo);
	}
	if (!(RacesMask & RacesHierarchy))
	{
		Response(StarbasePhrase::WhatAboutHierarchy, HierarchyInfo);
	}
	if (!(RacesMask & RacesOther))
	{
		Response(StarbasePhrase::WhatAboutOther, AlienRaces);
	}
	Response(StarbasePhrase::EnoughAliens, TellHistory);
}

static constexpr auto WarStarted {1 << 0};
static constexpr auto WarWasLike {1 << 1};
static constexpr auto WarLost {1 << 2};
static constexpr auto WarAftermath {1 << 3};

static void WarInfo(ResponseRef R)
{
	static uint8_t WarMask = 0;

	if (PlayerSaid(R, StarbasePhrase::TheWar))
	{
		NPCPhrase(StarbasePhrase::WhichWar);
		WarMask = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhatStartedWar))
	{
		NPCPhrase(StarbasePhrase::UrquanStartedWar);
		WarMask |= WarStarted;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhatWasWarLike))
	{
		NPCPhrase(StarbasePhrase::WarWasLikeSo);
		WarMask |= WarWasLike;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhyLoseWar))
	{
		NPCPhrase(StarbasePhrase::LostWarBecause);
		WarMask |= WarLost;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhatAfterWar))
	{
		NPCPhrase(StarbasePhrase::AfterWar);
		WarMask |= WarAftermath;
	}

	if (!(WarMask & WarStarted))
	{
		Response(StarbasePhrase::WhatStartedWar, WarInfo);
	}
	if (!(WarMask & WarWasLike))
	{
		Response(StarbasePhrase::WhatWasWarLike, WarInfo);
	}
	if (!(WarMask & WarLost))
	{
		Response(StarbasePhrase::WhyLoseWar, WarInfo);
	}
	if (!(WarMask & WarAftermath))
	{
		Response(StarbasePhrase::WhatAfterWar, WarInfo);
	}
	Response(StarbasePhrase::EnoughWar, TellHistory);
}

static constexpr auto AncientPrecursors {1 << 0};
static constexpr auto AncientRaces {1 << 1};
static constexpr auto AncientEarth {1 << 2};

static void AncientHistory(ResponseRef R)
{
	static uint8_t AncientMask = 0;

	if (PlayerSaid(R, StarbasePhrase::AncientHistory))
	{
		NPCPhrase(StarbasePhrase::WhichAncient);
		AncientMask = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::Precursors))
	{
		NPCPhrase(StarbasePhrase::AboutPrecursors);
		AncientMask |= AncientPrecursors;
	}
	else if (PlayerSaid(R, StarbasePhrase::OldRaces))
	{
		NPCPhrase(StarbasePhrase::AboutOldRaces);
		AncientMask |= AncientRaces;
	}
	else if (PlayerSaid(R, StarbasePhrase::AliensOnEarth))
	{
		NPCPhrase(StarbasePhrase::AboutAliensOnEarth);
		AncientMask |= AncientEarth;
	}

	if (!(AncientMask & AncientPrecursors))
	{
		Response(StarbasePhrase::Precursors, AncientHistory);
	}
	if (!(AncientMask & AncientRaces))
	{
		Response(StarbasePhrase::OldRaces, AncientHistory);
	}
	if (!(AncientMask & AncientEarth))
	{
		Response(StarbasePhrase::AliensOnEarth, AncientHistory);
	}
	Response(StarbasePhrase::EnoughAncient, TellHistory);
}

static void TellHistory(ResponseRef R)
{
	uqstl::array<StarbasePhrase, 3> pstack;

	if (PlayerSaid(R, StarbasePhrase::History))
	{
		NPCPhrase(StarbasePhrase::WhichHistory);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughAliens))
	{
		NPCPhrase(StarbasePhrase::OkEnoughAliens);

		stack0 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughWar))
	{
		NPCPhrase(StarbasePhrase::OkEnoughWar);

		stack1 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughAncient))
	{
		NPCPhrase(StarbasePhrase::OkEnoughAncient);

		stack2 = 1;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = StarbasePhrase::AlienRaces;
			break;
		default:
			pstack[0] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = StarbasePhrase::TheWar;
			break;
		default:
			pstack[1] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = StarbasePhrase::AncientHistory;
			break;
		default:
			pstack[2] = StarbasePhrase::NullPhrase;
			break;
	}

	if (pstack[0] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[0], AlienRaces);
	}
	if (pstack[1] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[1], WarInfo);
	}
	if (pstack[2] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[2], AncientHistory);
	}
	Response(StarbasePhrase::EnoughHistory, NeedInfo);
}

static constexpr auto HowFindUrquan {1 << 0};
static constexpr auto HowFightUrquan {1 << 1};
static constexpr auto HowAllyAgainstUrquan {1 << 2};
static constexpr auto HowStrongAgainstUrquan {1 << 3};

static void DefeatUrquan(ResponseRef R)
{
	static uint8_t DefeatMask = 0;

	if (PlayerSaid(R, StarbasePhrase::HowDefeat))
	{
		NPCPhrase(StarbasePhrase::DefeatLikeSo);
		DefeatMask = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::HowFindUrquan))
	{
		NPCPhrase(StarbasePhrase::FindUrquan);
		DefeatMask |= HowFindUrquan;
	}
	else if (PlayerSaid(R, StarbasePhrase::HowFightUrquan))
	{
		NPCPhrase(StarbasePhrase::FightUrquan);
		DefeatMask |= HowFightUrquan;
	}
	else if (PlayerSaid(R, StarbasePhrase::HowAlly))
	{
		NPCPhrase(StarbasePhrase::AllyLikeSo);
		DefeatMask |= HowAllyAgainstUrquan;
	}
	else if (PlayerSaid(R, StarbasePhrase::HowGetStrong))
	{
		NPCPhrase(StarbasePhrase::StrongLikeSo);
		DefeatMask |= HowStrongAgainstUrquan;
	}

	if (!(DefeatMask & HowFindUrquan))
	{
		Response(StarbasePhrase::HowFindUrquan, DefeatUrquan);
	}
	if (!(DefeatMask & HowFightUrquan))
	{
		Response(StarbasePhrase::HowFightUrquan, DefeatUrquan);
	}
	if (!(DefeatMask & HowAllyAgainstUrquan))
	{
		Response(StarbasePhrase::HowAlly, DefeatUrquan);
	}
	if (!(DefeatMask & HowStrongAgainstUrquan))
	{
		Response(StarbasePhrase::HowGetStrong, DefeatUrquan);
	}
	Response(StarbasePhrase::EnoughDefeat, TellMission);
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

	for (i = 0; i < uqm::gameState().sis().DriveSlots.size(); ++i)
	{
		if (uqm::gameState().sis().DriveSlots[i] < EmptySlot)
		{
			++num_thrusters;
		}
	}
	for (i = 0; i < uqm::gameState().sis().JetSlots.size(); ++i)
	{
		if (uqm::gameState().sis().JetSlots[i] < EmptySlot)
		{
			++num_jets;
		}
	}
	for (i = 0; i < uqm::gameState().sis().ModuleSlots.size(); ++i)
	{
		ModuleType which_piece {};

		switch (which_piece = uqm::gameState().sis().ModuleSlots[i])
		{
			case ModuleType::StorageBay:
				++num_bays;
				break;
			case ModuleType::DynamoUnit:
			case ModuleType::ShivaFurnace:
				num_batts += 1 + (which_piece - ModuleType::DynamoUnit);
				break;
			case ModuleType::GunWeapon:
			case ModuleType::BlasterWeapon:
			case ModuleType::CannonWeapon:
				num_guns += 1 + (which_piece - ModuleType::GunWeapon);
				break;
			case ModuleType::TrackingSystem:
				++num_track;
				break;
			case ModuleType::AntimissileDefense:
				++num_defense;
				break;
			default:
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
	NPCPhrase(StarbasePhrase::LetsSee);
	if (!HasMinimum && uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2)
	{
		NPCPhrase(StarbasePhrase::Improve1);
		if (num_thrusters < 7)
		{
			NPCPhrase(StarbasePhrase::NeedThrusters1);
		}
		if (num_jets < 5)
		{
			NPCPhrase(StarbasePhrase::NeedTurn1);
		}
		if (num_guns < 2)
		{
			NPCPhrase(StarbasePhrase::NeedGuns1);
		}
		if (uqm::gameState().sis().CrewEnlisted < CrewPodCapacity)
		{
			NPCPhrase(StarbasePhrase::NeedCrew1);
		}
		if (uqm::gameState().sis().FuelOnBoard < FuelTankCapacity)
		{
			NPCPhrase(StarbasePhrase::NeedFuel1);
		}
		if (num_bays < 1)
		{
			NPCPhrase(StarbasePhrase::NeedStorage1);
		}
		if (uqm::gameState().sis().NumLanders == 0)
		{
			NPCPhrase(StarbasePhrase::NeedLanders2);
		}
		if (num_batts < 1)
		{
			NPCPhrase(StarbasePhrase::NeedDynamos1);
		}

		if (uqm::gameState().sis().ResUnits >= 3000)
		{
			NPCPhrase(StarbasePhrase::ImproveFlagshipWithRu);
		}
		else
		{
			NPCPhrase(StarbasePhrase::GoGetMinerals);
		}
	}
	else
	{
		uint8_t num_aliens = 0;
		bool HasMaximum {};

		const uint16_t FleetStrength {CalculateEscortsWorth()};
		for (i = 0; i < RaceId::NumAvailableRaces; ++i)
		{
			if (i != RaceId::Human && CheckAlliance((RaceId)i) == RaceAllegiance::Good)
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
			NPCPhrase(StarbasePhrase::GotOkFlagship);
		}
		else
		{
			NPCPhrase(StarbasePhrase::GotAwesomeFlagship);
		}

		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) >= 2)
		{
			NPCPhrase(StarbasePhrase::ChmmrImprovedBomb);
			if (FleetStrength < 20000)
			{
				NPCPhrase(StarbasePhrase::MustAcquireAwesomeFleet);
			}
			else
			{
				NPCPhrase(StarbasePhrase::GotAwesomeFleet);
				if (!uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
				{
					NPCPhrase(StarbasePhrase::MustEliminateUrquanGuards);
				}
				else
				{
					NPCPhrase(StarbasePhrase::GoDestroySamatra);
				}
			}
		}
		else if (num_aliens < 2)
		{
			NPCPhrase(StarbasePhrase::GoAllyWithAliens);
		}
		else
		{
			NPCPhrase(StarbasePhrase::MadeSomeAllies);
			if (FleetStrength < 6000)
			{
				if (uqm::gameState().sis().ResUnits >= 3000)
				{
					NPCPhrase(StarbasePhrase::BuyCombatShips);
				}
				else
				{
					NPCPhrase(StarbasePhrase::GetShipsByMiningOrAlliance);
				}
			}
			else
			{
				NPCPhrase(StarbasePhrase::GotOkFleet);
				if (!HasMaximum)
				{
					NPCPhrase(StarbasePhrase::Improve2);
					if (num_thrusters < NumDriveSlots)
					{
						NPCPhrase(StarbasePhrase::NeedThrusters2);
					}
					if (num_jets < NumJetSlots)
					{
						NPCPhrase(StarbasePhrase::NeedTurn2);
					}
					if (num_guns < 7)
					{
						NPCPhrase(StarbasePhrase::NeedGuns2);
					}
					if (uqm::gameState().sis().CrewEnlisted < CrewPodCapacity * 3)
					{
						NPCPhrase(StarbasePhrase::NeedCrew2);
					}
					if (uqm::gameState().sis().FuelOnBoard < FuelTankCapacity * 3)
					{
						NPCPhrase(StarbasePhrase::NeedFuel2);
					}
					if (uqm::gameState().sis().NumLanders < 3)
					{
						NPCPhrase(StarbasePhrase::NeedLanders1);
					}
					if (num_batts < 4)
					{
						NPCPhrase(StarbasePhrase::NeedDynamos2);
					}
					if (num_defense < 2)
					{
						NPCPhrase(StarbasePhrase::NeedPoint);
					}
				}
				else if (!uqm::gameState().flag(uqm::GameFlag::AWARE_OF_SAMATRA))
				{
					NPCPhrase(StarbasePhrase::GoLearnAboutUrquan);
				}
				else
				{
					NPCPhrase(StarbasePhrase::KnowAboutSamatra);
					if (!uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB))
					{
						NPCPhrase(StarbasePhrase::FindWayToDestroySamatra);
					}
					else if (uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
					{
						NPCPhrase(StarbasePhrase::MustIncreaseBombStrength);
					}
				}
			}
		}
	}
}

static void TellMission(ResponseRef R)
{
	uqstl::array<StarbasePhrase, 4> pstack;

	if (PlayerSaid(R, StarbasePhrase::OurMission))
	{
		NPCPhrase(StarbasePhrase::WhichMission);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
		stack3 = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhereGetMinerals))
	{
		NPCPhrase(StarbasePhrase::GetMinerals);

		stack0 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhatAboutAliens))
	{
		NPCPhrase(StarbasePhrase::AboutAliens);

		stack1 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhatDoNow))
	{
		AnalyzeCondition();

		stack2 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::WhatAboutUrquan))
	{
		NPCPhrase(StarbasePhrase::MustDefeat);

		stack3 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughDefeat))
	{
		NPCPhrase(StarbasePhrase::OkEnoughDefeat);

		stack3 = 2;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = StarbasePhrase::WhereGetMinerals;
			break;
		default:
			pstack[0] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = StarbasePhrase::WhatAboutAliens;
			break;
		default:
			pstack[1] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = StarbasePhrase::WhatDoNow;
			break;
		default:
			pstack[2] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack3)
	{
		case 0:
			pstack[3] = StarbasePhrase::WhatAboutUrquan;
			break;
		case 1:
			pstack[3] = StarbasePhrase::HowDefeat;
			break;
		default:
			pstack[3] = StarbasePhrase::NullPhrase;
			break;
	}

	if (pstack[0] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[0], TellMission);
	}
	if (pstack[1] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[1], TellMission);
	}
	if (pstack[2] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[2], TellMission);
	}
	if (pstack[3] != StarbasePhrase::NullPhrase)
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

	Response(StarbasePhrase::EnoughMission, NeedInfo);
}

static void TellStarBase(ResponseRef R)
{
	uqstl::array<StarbasePhrase, 4> pstack;

	if (PlayerSaid(R, StarbasePhrase::StarbaseFunctions))
	{
		NPCPhrase(StarbasePhrase::WhichFunction);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
		stack3 = 0;
	}
	else if (PlayerSaid(R, StarbasePhrase::TellMeAboutFuel))
	{
		NPCPhrase(StarbasePhrase::AboutFuel);

		stack1 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::TellMeAboutCrew))
	{
		NPCPhrase(StarbasePhrase::AboutCrew);

		stack2 = 2;
	}
	else if (PlayerSaid(R, StarbasePhrase::TellMeAboutModules))
	{
		NPCPhrase(StarbasePhrase::AboutModules);

		stack0 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::TellMeAboutShips))
	{
		NPCPhrase(StarbasePhrase::AboutShips);

		stack2 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::TellMeAboutRu))
	{
		NPCPhrase(StarbasePhrase::AboutRu);

		stack3 = 1;
	}
	else if (PlayerSaid(R, StarbasePhrase::TellMeAboutMinerals))
	{
		NPCPhrase(StarbasePhrase::AboutMinerals);

		stack3 = 2;
	}
	else if (PlayerSaid(R, StarbasePhrase::TellMeAboutLife))
	{
		NPCPhrase(StarbasePhrase::AboutLife);

		stack3 = 3;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = StarbasePhrase::TellMeAboutModules;
			break;
		default:
			pstack[0] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = StarbasePhrase::TellMeAboutFuel;
			break;
		default:
			pstack[1] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = StarbasePhrase::TellMeAboutShips;
			break;
		case 1:
			pstack[2] = StarbasePhrase::TellMeAboutCrew;
			break;
		default:
			pstack[2] = StarbasePhrase::NullPhrase;
			break;
	}
	switch (stack3)
	{
		case 0:
			pstack[3] = StarbasePhrase::TellMeAboutRu;
			break;
		case 1:
			pstack[3] = StarbasePhrase::TellMeAboutMinerals;
			break;
		case 2:
			pstack[3] = StarbasePhrase::TellMeAboutLife;
			break;
		default:
			pstack[3] = StarbasePhrase::NullPhrase;
			break;
	}

	if (pstack[0] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[0], TellStarBase);
	}
	if (pstack[1] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[1], TellStarBase);
	}
	if (pstack[2] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[2], TellStarBase);
	}
	if (pstack[3] != StarbasePhrase::NullPhrase)
	{
		Response(pstack[3], TellStarBase);
	}

	Response(StarbasePhrase::EnoughStarbase, NeedInfo);
}

static void NormalStarbase(ResponseRef R);

static void NeedInfo(ResponseRef R)
{
	if (PlayerSaid(R, StarbasePhrase::NeedInfo))
	{
		NPCPhrase(StarbasePhrase::WhatKindOfInfo);
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughStarbase))
	{
		NPCPhrase(StarbasePhrase::OkEnoughStarbase);
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughHistory))
	{
		NPCPhrase(StarbasePhrase::OkEnoughHistory);
	}
	else if (PlayerSaid(R, StarbasePhrase::EnoughMission))
	{
		NPCPhrase(StarbasePhrase::OkEnoughMission);
	}

	Response(StarbasePhrase::StarbaseFunctions, TellStarBase);
	Response(StarbasePhrase::History, TellHistory);
	Response(StarbasePhrase::OurMission, TellMission);
	Response(StarbasePhrase::NoNeedInfo, NormalStarbase);
}

static bool DiscussDevices(bool TalkAbout)
{
	//bool Undiscussed; unused

	if (TalkAbout)
	{
		NPCPhrase(StarbasePhrase::DeviceHead);
	}
	uint16_t PhraseIndex {2};

	uint16_t VuxBeastIndex {0};
	//Undiscussed = false; unused
	for (Device i {}; i < Device::NumDevices; ++i)
	{
		StarbasePhrase pStr {0};
		switch (i)
		{
			case Device::RosySphere:
				if (uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ROSY_SPHERE))
				{
					pStr = StarbasePhrase::AboutSphere;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ROSY_SPHERE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::WimblisTrident:
				if (uqm::gameState().flag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_WIMBLIS_TRIDENT))
				{
					pStr = StarbasePhrase::AboutWimblisTrident;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_WIMBLIS_TRIDENT, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::GlowingRod:
				if (uqm::gameState().flag(uqm::GameFlag::GLOWING_ROD_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_GLOWING_ROD))
				{
					pStr = StarbasePhrase::AboutGlowingRod;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_GLOWING_ROD, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::SunEfficiency:
				if (uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_SUN_EFFICIENCY))
				{
					pStr = StarbasePhrase::AboutSun;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_SUN_EFFICIENCY, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::UtwigBomb:
				if (uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_UTWIG_BOMB))
				{
					pStr = StarbasePhrase::AboutBomb;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_UTWIG_BOMB, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::Ultron0:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 1
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = StarbasePhrase::AboutUltron0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::Ultron1:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 2
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = StarbasePhrase::AboutUltron1;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::Ultron2:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 3
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = StarbasePhrase::AboutUltron2;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::Ultron3:
				if (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 4
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_ULTRON))
				{
					pStr = StarbasePhrase::AboutUltron3;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::Maidens:
				if (uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_MAIDENS))
				{
					pStr = StarbasePhrase::AboutMaidens;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_MAIDENS, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::TalkingPet:
				if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_TALKING_PET))
				{
					pStr = StarbasePhrase::AboutTalkpet;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_TALKING_PET, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::AquaHelix:
				if (uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_AQUA_HELIX))
				{
					pStr = StarbasePhrase::AboutHelix;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_AQUA_HELIX, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::ClearSpindle:
				if (uqm::gameState().flag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_CLEAR_SPINDLE))
				{
					pStr = StarbasePhrase::AboutSpindle;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_CLEAR_SPINDLE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::UmgahHyperwave:
				if (uqm::gameState().flag(uqm::GameFlag::UMGAH_BROADCASTERS_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_UMGAH_HYPERWAVE))
				{
					pStr = StarbasePhrase::AboutUcaster;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_UMGAH_HYPERWAVE, static_cast<uint32_t>(TalkAbout));
				}
				break;
#ifdef NEVER
			case Device::DataPlate1:
				if (uqm::gameState().flag(uqm::GameFlag::DATA_PLATE_1_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DATA_PLATE_1))
				{
					pStr = ABOUT_DATAPLATE_1;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DATA_PLATE_1, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::DataPlate2:
				if (uqm::gameState().flag(uqm::GameFlag::DATA_PLATE_2_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DATA_PLATE_2))
				{
					pStr = ABOUT_DATAPLATE_2;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DATA_PLATE_2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::DataPlate3:
				if (uqm::gameState().flag(uqm::GameFlag::DATA_PLATE_3_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DATA_PLATE_3))
				{
					pStr = ABOUT_DATAPLATE_3;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DATA_PLATE_3, static_cast<uint32_t>(TalkAbout));
				}
				break;
#endif /* NEVER */
			case Device::TaaloProtector:
				if (uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_TAALO_PROTECTOR))
				{
					pStr = StarbasePhrase::AboutShield;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_TAALO_PROTECTOR, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::EggCasing0:
				if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_EGG_CASING0))
				{
					pStr = StarbasePhrase::AboutEggcase0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING0, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING1, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::EggCasing1:
				if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_EGG_CASING1))
				{
					pStr = StarbasePhrase::AboutEggcase0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING0, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING1, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::EggCasing2:
				if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_EGG_CASING2))
				{
					pStr = StarbasePhrase::AboutEggcase0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING0, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING1, static_cast<uint32_t>(TalkAbout));
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_EGG_CASING2, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::SyreenShuttle:
				if (uqm::gameState().flag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_SYREEN_SHUTTLE))
				{
					pStr = StarbasePhrase::AboutShuttle;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_SYREEN_SHUTTLE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::VuxBeast:
				if (uqm::gameState().flag(uqm::GameFlag::VUX_BEAST_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_VUX_BEAST))
				{
					pStr = StarbasePhrase::AboutVuxbeast0;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_VUX_BEAST, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::DestructCode:
				if (uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_DESTRUCT_CODE))
				{
					pStr = StarbasePhrase::AboutDestruct;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_DESTRUCT_CODE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::PortalSpawner:
				if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_PORTAL_SPAWNER))
				{
					pStr = StarbasePhrase::AboutPortal;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_PORTAL_SPAWNER, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::UrquanWarp:
				if (uqm::gameState().flag(uqm::GameFlag::PORTAL_KEY_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_URQUAN_WARP))
				{
					pStr = StarbasePhrase::AboutWarppod;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_URQUAN_WARP, static_cast<uint32_t>(TalkAbout));
				}
				break;
			case Device::BurvixHyperwave:
				if (uqm::gameState().flag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP)
					&& !uqm::gameState().flag(uqm::GameFlag::DISCUSSED_BURVIX_HYPERWAVE))
				{
					pStr = StarbasePhrase::AboutBcaster;
					uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_BURVIX_HYPERWAVE, static_cast<uint32_t>(TalkAbout));
				}
				break;
			default:
				break;
		}

		if (pStr != StarbasePhrase::NullPhrase)
		{
			if (TalkAbout)
			{
				if (PhraseIndex > 2)
				{
					NPCPhrase(StarbasePhrase::BetweenDevices);
				}
				NPCPhrase(pStr);
				if (pStr == StarbasePhrase::AboutVuxbeast0)
				{
					VuxBeastIndex = ++PhraseIndex;
					NPCPhrase(StarbasePhrase::AboutVuxbeast1);
				}
			}
			PhraseIndex += 2;
		}
	}

	if (TalkAbout)
	{
		NPCPhrase(StarbasePhrase::DeviceTail);

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
	const uqstl::array<uint8_t, 12> days_in_month {
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

	StarbasePhrase pIntro {};
	for (uint8_t b0 {0}; b0 < 32; ++b0)
	{
		if (!(BulletinMask & (1L << b0)))
		{
			StarbasePhrase pStr {0};
			switch (b0)
			{
				case 0:
					if (CheckAlliance(RaceId::Spathi) == RaceAllegiance::Good)
					{
						pStr = StarbasePhrase::StarbaseBulletin1;
					}
					break;
				case 1:
					if (CheckAlliance(RaceId::Zoqfotpik) == RaceAllegiance::Good)
					{
						pStr = StarbasePhrase::StarbaseBulletin2;
					}
					break;
				case 2:
					if (CheckAlliance(RaceId::Supox) == RaceAllegiance::Good)
					{
						pStr = StarbasePhrase::StarbaseBulletin3;
					}
					break;
				case 3:
					if (CheckAlliance(RaceId::Utwig) == RaceAllegiance::Good)
					{
						pStr = StarbasePhrase::StarbaseBulletin4;
					}
					break;
				case 4:
					if (CheckAlliance(RaceId::Orz) == RaceAllegiance::Good)
					{
						pStr = StarbasePhrase::StarbaseBulletin5;
					}
					break;
				case 5:
					if (uqm::gameState().flag(uqm::GameFlag::ARILOU_MANNER) == 2)
					{
						BulletinMask |= 1L << b0;
					}
					else if (uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER)
							 && (Repeat || EscortFeasibilityStudy(RaceId::Arilou)))
					{
						pStr = StarbasePhrase::StarbaseBulletin6;
						if (!Repeat)
						{
							AddEscortShips(RaceId::Arilou, NumGiftArilous);
						}
					}
					break;
				case 6:
					if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS) == 1)
					{
						pStr = StarbasePhrase::StarbaseBulletin7;
					}
					break;
				case 7:
					if (uqm::gameState().flag(uqm::GameFlag::MET_MELNORME))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(ifEasyDifficulty(1, 3), 0))
					{
						pStr = StarbasePhrase::StarbaseBulletin8;
					}
					break;
				case 8:
					if (uqm::gameState().flag(uqm::GameFlag::MET_MELNORME))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(ifEasyDifficulty(3, 6), 0))
					{
						pStr = StarbasePhrase::StarbaseBulletin9;
					}
					break;
				case 9:
					if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(0, 7))
					{
						pStr = StarbasePhrase::StarbaseBulletin10;
					}
					break;
				case 10:
					if (uqm::gameState().flag(uqm::GameFlag::SPATHI_SHIELDED_SELVES))
					{
						pStr = StarbasePhrase::StarbaseBulletin11;
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
						pStr = StarbasePhrase::StarbaseBulletin12;
					}
					break;
				case 12:
					if (CheckAlliance(RaceId::Chmmr) == RaceAllegiance::Good)
					{
						pStr = StarbasePhrase::StarbaseBulletin13;
					}
					break;
				case 13:
					if (CheckAlliance(RaceId::Shofixti) == RaceAllegiance::Good)
					{
						pStr = StarbasePhrase::StarbaseBulletin14;
					}
					break;
				case 14:
					if (uqm::gameState().flag(uqm::GameFlag::PKUNK_MISSION))
					{
						pStr = StarbasePhrase::StarbaseBulletin15;
					}
					break;
				case 15:
					if (uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP))
					{
						BulletinMask |= 1L << b0;
					}
					else if (CheckTiming(7, 0))
					{
						pStr = StarbasePhrase::StarbaseBulletin16;
					}
					break;
				case 16:
					break;
				case 17:
					if (uqm::gameState().flag(uqm::GameFlag::YEHAT_ABSORBED_PKUNK))
					{
						pStr = StarbasePhrase::StarbaseBulletin18;
					}
					break;
				case 18:
					if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 2)
					{
						pStr = StarbasePhrase::StarbaseBulletin19;
					}
					break;
				case 19:
					break;
				case 20:
					break;
				case 21:
					if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS) == 2)
					{
						pStr = StarbasePhrase::StarbaseBulletin22;
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
							pStr = StarbasePhrase::StarbaseBulletin27;
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
							pStr = StarbasePhrase::StarbaseBulletin28;
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
							pStr = StarbasePhrase::StarbaseBulletin29;
						}
						break;
					}
				case 29:
					if (CrewSold > MaxSold())
					{
						pStr = StarbasePhrase::StarbaseBulletin30;
					}
					break;
				case 30:
					break;
				case 31:
					break;
			}

			if (pStr != StarbasePhrase::NullPhrase)
			{
				if (pIntro != StarbasePhrase::NullPhrase)
				{
					NPCPhrase(StarbasePhrase::BetweenBulletins);
				}
				else if (Repeat)
				{
					pIntro = StarbasePhrase::BeforeWeGoOn1;
				}
				else
				{
					switch ((uint8_t)uqm::random().next() % 7)
					{
						case 0:
							pIntro = StarbasePhrase::BeforeWeGoOn1;
							break;
						case 1:
							pIntro = StarbasePhrase::BeforeWeGoOn2;
							break;
						case 2:
							pIntro = StarbasePhrase::BeforeWeGoOn3;
							break;
						case 3:
							pIntro = StarbasePhrase::BeforeWeGoOn4;
							break;
						case 4:
							pIntro = StarbasePhrase::BeforeWeGoOn5;
							break;
						case 5:
							pIntro = StarbasePhrase::BeforeWeGoOn6;
							break;
						default:
							pIntro = StarbasePhrase::BeforeWeGoOn7;
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
		NPCPhrase(StarbasePhrase::ReturnHello);
	}
	else if (!Repeat)
	{
		uqm::gameState().setFlag(uqm::GameFlag::STARBASE_BULLETS, static_cast<uint32_t>(BulletinMask));
	}
}

static void NormalStarbase(ResponseRef R)
{
	if (PlayerSaid(R, StarbasePhrase::NoNeedInfo))
	{
		NPCPhrase(StarbasePhrase::OkNoNeedInfo);
	}
	else if (PlayerSaid(R, StarbasePhrase::NewDevices))
	{
		DiscussDevices(true);
	}
	else if (PlayerSaid(R, StarbasePhrase::RepeatBulletins))
	{
		CheckBulletins(true);
	}
	else if (R == 0)
	{
		if (uqm::gameState().flag(uqm::GameFlag::MOONBASE_ON_SHIP))
		{
			NPCPhrase(StarbasePhrase::StarbaseIsReady);
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
			StarbasePhrase pStr {};

			switch ((uint8_t)uqm::random().next() & 7)
			{
				case 0:
					pStr = StarbasePhrase::NormalHelloA;
					break;
				case 1:
					pStr = StarbasePhrase::NormalHelloB;
					break;
				case 2:
					pStr = StarbasePhrase::NormalHelloC;
					break;
				case 3:
					pStr = StarbasePhrase::NormalHelloD;
					break;
				case 4:
					pStr = StarbasePhrase::NormalHelloE;
					break;
				case 5:
					pStr = StarbasePhrase::NormalHelloF;
					break;
				case 6:
					pStr = StarbasePhrase::NormalHelloG;
					break;
				case 7:
					pStr = StarbasePhrase::NormalHelloH;
					break;
			}
			NPCPhrase(pStr);
			CheckBulletins(false);
		}

		uqm::gameState().setFlag(uqm::GameFlag::STARBASE_VISITED, static_cast<uint32_t>(1));
	}

	if (uqm::gameState().sis().TotalElementMass)
	{
		Response(StarbasePhrase::HaveMinerals, SellMinerals);
	}
	if (DiscussDevices(false))
	{
		Response(StarbasePhrase::NewDevices, NormalStarbase);
	}
	if (CurBulletinMask)
	{
		Response(StarbasePhrase::RepeatBulletins, NormalStarbase);
	}
	Response(StarbasePhrase::NeedInfo, NeedInfo);
	Response(StarbasePhrase::GoodbyeCommander, ByeBye);
}

static void SellMinerals(ResponseRef R)
{
	uint16_t total = 0;
	StarbasePhrase pStr {};

	if (uqm::UQMOptions::read().useSpeech)
	{
		total = DoSellMinerals();

		pStr = EveryOnesACritic(total);

		NPCPhrase(pStr);
		if (pStr == StarbasePhrase::LightLoadC0)
		{
			NPCPhrase(StarbasePhrase::LightLoadC1);
		}
	}
	else
	{
		uqstl::array<uint16_t, *MineralCategory::NumElementCategories> amount;
		uqstl::array<uint16_t, *MineralCategory::NumElementCategories> Ru;
		uint16_t seg = 0;
		uint16_t count = 0;

		NPCPhrase(StarbasePhrase::CargoList);
		for (uint16_t i {0}; i < MineralCategory::NumElementCategories; ++i)
		{
			if ((amount[seg] = uqm::gameState().sis().ElementAmounts[i]) != 0)
			{
				Ru[seg] = amount[seg] * uqm::gameState().game().ElementWorth[i];
				if (count > 0)
				{
					NPCPhrase(StarbasePhrase::Ellipses);
				}
				else
				{
					NPCPhrase(StarbasePhrase::Blank);
				}
				NPCNumber(amount[seg], nullptr);
				NPCPhrase_splice(StarbasePhrase::KilotonsOf);
				NPCPhrase_splice(StarbasePhrase::Commonr + i);
				NPCPhrase_splice(StarbasePhrase::For);
				NPCNumber(Ru[seg], nullptr);
				NPCPhrase(StarbasePhrase::Resunits);
				total += Ru[seg];
				seg++;
				uqm::gameState().sis().ElementAmounts[i] = 0;
				++count;
			}
		}

		pStr = EveryOnesACritic(total);

		NPCPhrase(pStr);
		if (pStr == StarbasePhrase::LightLoadC0)
		{
			NPCPhrase(StarbasePhrase::LightLoadC1);
		}

		for (uint16_t sseg {1}; sseg < seg + 1; sseg++)
		{
			AlienTalkSegue(sseg);
			uqm::gameState().sis().TotalElementMass -= amount[sseg - 1];
			DeltaSISGauges(0, 0, Ru[sseg - 1]);
			ClearSISRect(ClearSisFlags::DrawDisplay);
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
	altResFlags |= AltResFlags::Song;

	if (is3DO(uqm::UQMOptions::read().flagshipColor))
	{
		altResFlags |= AltResFlags::Frame;
	}

	CurBulletinMask = 0;
	setSegue(Segue::Peace);
	ConversationData* retval {&commander_desc};

	return (retval);
}

} // namespace uqm
