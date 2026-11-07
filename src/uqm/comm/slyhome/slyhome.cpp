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
#include "uqm/build.h"
#include "uqm/gameev.h"
#include "uqm/lua/UQMLua.h"


namespace uqm
{
static ConversationData slylandro_desc =
	{
		Conversation::SlylandroHome, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SlylandroPmapAnim, /* AlienFrame */
		SlylandroFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth(), */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SlylandroColorMap, /* AlienColorMap */
		SlylandroMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		SlylandroConversationPhrases, /* PlayerPhrases */
		13, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				0,																				/* StartIndex */
				5,																				/* NumFrames */
				AnimationFlags::Random | AnimationFlags::ColorXform | AnimationFlags::Disabled, /* AnimFlags */
				GameTicksPerSecond / 8,
				GameTicksPerSecond * 5 / 8, /* FrameRate */
				GameTicksPerSecond / 8,
				GameTicksPerSecond * 5 / 8, /* RestartRate */
				0,							/* BlockMask */
			},
								{
				1,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				6,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				11,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,					  /* StartIndex */
				6,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				22,						  /* StartIndex */
				8,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond / 15,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				30,						  /* StartIndex */
				9,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 8) | (1 << 9),	/* BlockMask */
			},
								{
				39,						  /* StartIndex */
				4,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				43,						  /* StartIndex */
				5,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6),				/* BlockMask */
			},
								{
				48,						  /* StartIndex */
				6,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6),				/* BlockMask */
			},
								{
				54,						  /* StartIndex */
				6,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
								{
				60,						  /* StartIndex */
				7,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
								{
				67,						  /* StartIndex */
				8,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 10) | (1 << 11),	/* BlockMask */
			},
								},
		{
								/* AlienTransitionDesc - empty */
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
								/* AlienTalkDesc - empty */
			0,					  /* StartIndex */
			0,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			0,
								0, /* FrameRate */
			0,
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

static AnimFilterSet slyhome_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,				   /* Color index */
				1,				   /* Opacity index */
				-1,				   /* Frame index */
				DrawKind::Overlay, /* DrawKind*/
				FilterFlags::None, /* Flags */
			},
								  }
};

static void ExitConversation(ResponseRef R)
{
	(void)R; // ignored
	setSegue(Segue::Peace);

	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_HOME_VISITS))
	{
		case 1:
			NPCPhrase(SlylandroHomePhrase::Goodbye1);
			break;
		default:
			NPCPhrase(SlylandroHomePhrase::Goodbye2);
			break;
	}
}

static void HomeWorld(ResponseRef R);

static void HumanInfo(ResponseRef R)
{
	if (PlayerSaid(R, SlylandroHomePhrase::HappyToTellMore))
	{
		NPCPhrase(SlylandroHomePhrase::TellMore);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK4, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WouldYouLikeToKnowMore))
	{
		NPCPhrase(SlylandroHomePhrase::YesTellMore);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WeComeFromEarth))
	{
		NPCPhrase(SlylandroHomePhrase::OkEarth);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_EARTH, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WeExplore))
	{
		NPCPhrase(SlylandroHomePhrase::OkExplore);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_EXPLORE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WeFightUrquan))
	{
		NPCPhrase(SlylandroHomePhrase::UrquanNiceGuys);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::NotSameUrquan))
	{
		NPCPhrase(SlylandroHomePhrase::PersonalityChange);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WeGather))
	{
		NPCPhrase(SlylandroHomePhrase::MaybeInterested);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_GATHER, static_cast<uint32_t>(1));
	}

	uint8_t InfoLeft {false};
	if (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN) == 1)
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::NotSameUrquan, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_EARTH))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WeComeFromEarth, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_EXPLORE))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WeExplore, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_URQUAN))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WeFightUrquan, HumanInfo);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_GATHER))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WeGather, HumanInfo);
	}

	Response(SlylandroHomePhrase::EnoughAboutMe, HomeWorld);
	if (!InfoLeft)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK4, static_cast<uint32_t>(2));
	}
}

static void SlylandroInfo(ResponseRef R)
{
	if (PlayerSaid(R, SlylandroHomePhrase::LikeMoreAboutYou))
	{
		NPCPhrase(SlylandroHomePhrase::SureKnowWhat);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatAboutHome))
	{
		NPCPhrase(SlylandroHomePhrase::AboutHome);

		DisablePhrase(SlylandroHomePhrase::WhatAboutHome);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatAboutCulture))
	{
		NPCPhrase(SlylandroHomePhrase::AboutCulture);

		DisablePhrase(SlylandroHomePhrase::WhatAboutCulture);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatAboutHistory))
	{
		NPCPhrase(SlylandroHomePhrase::AboutHistory);

		DisablePhrase(SlylandroHomePhrase::WhatAboutHistory);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatAboutBiology))
	{
		NPCPhrase(SlylandroHomePhrase::AboutBiology);

		DisablePhrase(SlylandroHomePhrase::WhatAboutBiology);
	}

	uint8_t InfoLeft {false};
	if (PhraseEnabled(SlylandroHomePhrase::WhatAboutHome))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WhatAboutHome, SlylandroInfo);
	}
	if (PhraseEnabled(SlylandroHomePhrase::WhatAboutCulture))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WhatAboutCulture, SlylandroInfo);
	}
	if (PhraseEnabled(SlylandroHomePhrase::WhatAboutHistory))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WhatAboutHistory, SlylandroInfo);
	}
	if (PhraseEnabled(SlylandroHomePhrase::WhatAboutBiology))
	{
		InfoLeft = true;
		Response(SlylandroHomePhrase::WhatAboutBiology, SlylandroInfo);
	}

	Response(SlylandroHomePhrase::EnoughInfo, HomeWorld);
	if (!InfoLeft)
	{
		DisablePhrase(SlylandroHomePhrase::LikeMoreAboutYou);
	}
}

static void FixBug(ResponseRef R)
{
	if (PlayerSaid(R, SlylandroHomePhrase::ThinkAboutRepPriorities))
	{
		NPCPhrase(SlylandroHomePhrase::UhOh);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::HuntThemDown))
	{
		NPCPhrase(SlylandroHomePhrase::GrowTooFast);

		DisablePhrase(SlylandroHomePhrase::HuntThemDown);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::SueMelnorme))
	{
		NPCPhrase(SlylandroHomePhrase::SignedWaiver);

		DisablePhrase(SlylandroHomePhrase::SueMelnorme);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::RecallSignal))
	{
		NPCPhrase(SlylandroHomePhrase::NotThisModel);

		DisablePhrase(SlylandroHomePhrase::RecallSignal);
	}

	if (PhraseEnabled(SlylandroHomePhrase::HuntThemDown))
	{
		Response(SlylandroHomePhrase::HuntThemDown, FixBug);
	}
	if (PhraseEnabled(SlylandroHomePhrase::SueMelnorme))
	{
		Response(SlylandroHomePhrase::SueMelnorme, FixBug);
	}
	if (PhraseEnabled(SlylandroHomePhrase::RecallSignal))
	{
		Response(SlylandroHomePhrase::RecallSignal, FixBug);
	}
	Response(SlylandroHomePhrase::MegaSelfDestruct, HomeWorld);
}

static void ProbeBug(ResponseRef R)
{
	if (PlayerSaid(R, SlylandroHomePhrase::ProbeHasBug))
	{
		NPCPhrase(SlylandroHomePhrase::NoItDoesnt);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::TellMeAboutRep2))
	{
		NPCPhrase(SlylandroHomePhrase::RepNoProblem);

		DisablePhrase(SlylandroHomePhrase::TellMeAboutRep2);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatAboutRepPriorities))
	{
		NPCPhrase(SlylandroHomePhrase::MaximumSoWhat);

		DisablePhrase(SlylandroHomePhrase::WhatAboutRepPriorities);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::TellMeAboutAttack))
	{
		NPCPhrase(SlylandroHomePhrase::AttackNoProblem);

		DisablePhrase(SlylandroHomePhrase::TellMeAboutAttack);
	}

	if (PhraseEnabled(SlylandroHomePhrase::TellMeAboutRep2))
	{
		Response(SlylandroHomePhrase::TellMeAboutRep2, ProbeBug);
	}
	else if (PhraseEnabled(SlylandroHomePhrase::WhatAboutRepPriorities))
	{
		Response(SlylandroHomePhrase::WhatAboutRepPriorities, ProbeBug);
	}
	else
	{
		Response(SlylandroHomePhrase::ThinkAboutRepPriorities, FixBug);
	}
	if (PhraseEnabled(SlylandroHomePhrase::TellMeAboutAttack))
	{
		Response(SlylandroHomePhrase::TellMeAboutAttack, ProbeBug);
	}
}

static void ProbeInfo(ResponseRef R);

static void ProbeFunction(ResponseRef R)
{
	SlylandroHomePhrase pStr[2];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = SlylandroHomePhrase::NullPhrase;
	if (PlayerSaid(R, SlylandroHomePhrase::TalkMoreProbeAttack))
	{
		NPCPhrase(SlylandroHomePhrase::NoProblemButSure);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::TellMeAboutBasics))
	{
		NPCPhrase(SlylandroHomePhrase::BasicCommands);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_PROGRAM, static_cast<uint32_t>(1));
		DisablePhrase(SlylandroHomePhrase::TellBasicsAgain);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::TellBasicsAgain))
	{
		NPCPhrase(SlylandroHomePhrase::OkBasicsAgain);

		DisablePhrase(SlylandroHomePhrase::TellBasicsAgain);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatEffect))
	{
		NPCPhrase(SlylandroHomePhrase::AffectsBehavior);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_EFFECTS, static_cast<uint32_t>(1));
		DisablePhrase(SlylandroHomePhrase::WhatEffect);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::TellMeAboutRep1))
	{
		NPCPhrase(SlylandroHomePhrase::AboutRep);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK8, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatSetPriority))
	{
		NPCPhrase(SlylandroHomePhrase::Maximum);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_PRIORITY, static_cast<uint32_t>(1));
		DisablePhrase(SlylandroHomePhrase::WhatSetPriority);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::HowDoesProbeDefend))
	{
		NPCPhrase(SlylandroHomePhrase::OnlySelfDefense);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK9, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::CombatBehavior))
	{
		NPCPhrase(SlylandroHomePhrase::MissileBatteries);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK9, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatMissileBatteries))
	{
		NPCPhrase(SlylandroHomePhrase::LightningOnlyForHarvesting);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK9, static_cast<uint32_t>(3));
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK9))
	{
		case 0:
			pStr[0] = SlylandroHomePhrase::HowDoesProbeDefend;
			break;
		case 1:
			pStr[0] = SlylandroHomePhrase::CombatBehavior;
			break;
		case 2:
			pStr[0] = SlylandroHomePhrase::WhatMissileBatteries;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK8))
	{
		case 2:
			pStr[1] = SlylandroHomePhrase::TellMeAboutRep1;
			break;
		case 3:
			if (PhraseEnabled(SlylandroHomePhrase::WhatSetPriority))
			{
				pStr[1] = SlylandroHomePhrase::WhatSetPriority;
			}
			break;
	}

	if (LastStack && *pStr[LastStack - 1])
	{
		Response(pStr[LastStack - 1], ProbeFunction);
	}
	if (!uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_PROGRAM))
	{
		Response(SlylandroHomePhrase::TellMeAboutBasics, ProbeFunction);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_PRIORITY))
		{
			if (uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_EFFECTS))
			{
				Response(SlylandroHomePhrase::ProbeHasBug, ProbeBug);
			}
			if (PhraseEnabled(SlylandroHomePhrase::WhatEffect))
			{
				Response(SlylandroHomePhrase::WhatEffect, ProbeFunction);
			}
		}
		if (PhraseEnabled(SlylandroHomePhrase::TellBasicsAgain))
		{
			Response(SlylandroHomePhrase::TellBasicsAgain, ProbeFunction);
		}
	}
	if (LastStack == 0)
	{
		do
		{
			if (pStr[LastStack] != SlylandroHomePhrase::NullPhrase)
			{
				Response(pStr[LastStack], ProbeFunction);
			}
		} while (++LastStack < 2);
	}
	else
	{
		LastStack = (LastStack - 1) ^ 1;
		if (pStr[LastStack] != SlylandroHomePhrase::NullPhrase)
		{
			Response(pStr[LastStack], ProbeFunction);
		}
	}

	Response(SlylandroHomePhrase::EnoughProblem, ProbeInfo);
}

static void ProbeInfo(ResponseRef R)
{
	uint8_t i {};
	uint8_t InfoLeft {};
	SlylandroHomePhrase pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = SlylandroHomePhrase::NullPhrase;
	if (PlayerSaid(R, SlylandroHomePhrase::WhatAreProbes))
	{
		NPCPhrase(SlylandroHomePhrase::ProbesAre);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK5, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::KnowMoreProbe))
	{
		NPCPhrase(SlylandroHomePhrase::OkWhatMoreProbe);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhyProbeAlwaysAttack))
	{
		NPCPhrase(SlylandroHomePhrase::OnlyDefend);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK6, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::TalkMoreProbeAttack))
	{
		ProbeFunction(R);
		return;
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhereProbesFrom))
	{
		NPCPhrase(SlylandroHomePhrase::ProbesFromMelnorme);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK7, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhySell))
	{
		NPCPhrase(SlylandroHomePhrase::SellForInfo);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK7, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::HowLongAgo))
	{
		NPCPhrase(SlylandroHomePhrase::FiftyThousandRotations);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK7, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatsProbesMission))
	{
		NPCPhrase(SlylandroHomePhrase::SeekOutNewLife);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK8, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::IfOnlyOne))
	{
		NPCPhrase(SlylandroHomePhrase::TheyReplicate);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK8, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::EnoughProblem))
	{
		NPCPhrase(SlylandroHomePhrase::OkEnoughProblem);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN)
		&& uqm::gameState().flag(uqm::GameFlag::PROBE_EXHIBITED_BUG))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK6))
		{
			case 0:
				pStr[0] = SlylandroHomePhrase::WhyProbeAlwaysAttack;
				break;
			case 1:
				pStr[0] = SlylandroHomePhrase::TalkMoreProbeAttack;
				break;
		}
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK7))
	{
		case 0:
			pStr[1] = SlylandroHomePhrase::WhereProbesFrom;
			break;
		case 1:
			pStr[1] = SlylandroHomePhrase::WhySell;
			break;
		case 2:
			pStr[1] = SlylandroHomePhrase::HowLongAgo;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK8))
	{
		case 0:
			pStr[2] = SlylandroHomePhrase::WhatsProbesMission;
			break;
		case 1:
			pStr[2] = SlylandroHomePhrase::IfOnlyOne;
			break;
	}

	InfoLeft = false;
	if (pStr[LastStack] != SlylandroHomePhrase::NullPhrase)
	{
		InfoLeft = true;
		Response(pStr[LastStack], ProbeInfo);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i] != SlylandroHomePhrase::NullPhrase)
		{
			InfoLeft = true;
			Response(pStr[i], ProbeInfo);
		}
	}

	Response(SlylandroHomePhrase::EnoughProbe, HomeWorld);
	if (!InfoLeft)
	{
		DisablePhrase(SlylandroHomePhrase::KnowMoreProbe);
	}
}

static void HomeWorld(ResponseRef R)
{
	uint8_t i {};
	SlylandroHomePhrase pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = SlylandroHomePhrase::NullPhrase;
	if (PlayerSaid(R, SlylandroHomePhrase::WeAreUs))
	{
		NPCPhrase(SlylandroHomePhrase::TerriblyExciting);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(1));
		DisablePhrase(SlylandroHomePhrase::WeAreUs);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatOtherVisitors))
	{
		NPCPhrase(SlylandroHomePhrase::Visitors);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_KNOWS_PROBE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::AnyOtherVisitors))
	{
		NPCPhrase(SlylandroHomePhrase::LongAgo);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhatAboutSentientMilieu))
	{
		NPCPhrase(SlylandroHomePhrase::MetTaaloTheyAreFrom);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhoElse))
	{
		NPCPhrase(SlylandroHomePhrase::Precursors);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(5));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::PrecursorsYow))
	{
		NPCPhrase(SlylandroHomePhrase::AboutPrecursors);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(6));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::MustKnowMore))
	{
		NPCPhrase(SlylandroHomePhrase::AllWeKnow);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK1, static_cast<uint32_t>(7));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhoAreYou))
	{
		NPCPhrase(SlylandroHomePhrase::WeAreSly);

		LastStack = 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK2, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::WhereAreYou))
	{
		NPCPhrase(SlylandroHomePhrase::DownHere);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK3, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::ThatsImpossible1))
	{
		NPCPhrase(SlylandroHomePhrase::NoItsNot1);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK3, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::ThatsImpossible2))
	{
		NPCPhrase(SlylandroHomePhrase::NoItsNot2);

		LastStack = 2;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_STACK3, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::LikeMoreAboutYou))
	{
		SlylandroInfo(R);
		return;
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::EnoughAboutMe))
	{
		NPCPhrase(SlylandroHomePhrase::OkEnoughYou);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::EnoughInfo))
	{
		NPCPhrase(SlylandroHomePhrase::OkEnoughInfo);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::EnoughProbe))
	{
		NPCPhrase(SlylandroHomePhrase::OkEnoughProbe);
	}
	else if (PlayerSaid(R, SlylandroHomePhrase::MegaSelfDestruct))
	{
		NPCPhrase(SlylandroHomePhrase::WhyYesThereIs);

		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP, static_cast<uint32_t>(1));
		i = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_MULTIPLIER) + 1;
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_MULTIPLIER, static_cast<uint32_t>(i));
		AddEvent(EventType::Relative, 0, 0, 0, GameEventId::SlylandroRampDown);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK1))
	{
		case 0:
			pStr[0] = SlylandroHomePhrase::WeAreUs;
			break;
		case 1:
			pStr[0] = SlylandroHomePhrase::WhatOtherVisitors;
			break;
		case 2:
			pStr[0] = SlylandroHomePhrase::AnyOtherVisitors;
			break;
		case 3:
			pStr[0] = SlylandroHomePhrase::WhatAboutSentientMilieu;
			break;
		case 4:
			pStr[0] = SlylandroHomePhrase::WhoElse;
			break;
		case 5:
			pStr[0] = SlylandroHomePhrase::PrecursorsYow;
			break;
		case 6:
			pStr[0] = SlylandroHomePhrase::MustKnowMore;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK2))
	{
		case 0:
			pStr[1] = SlylandroHomePhrase::WhoAreYou;
			break;
		case 1:
			if (PhraseEnabled(SlylandroHomePhrase::LikeMoreAboutYou))
			{
				pStr[1] = SlylandroHomePhrase::LikeMoreAboutYou;
			}
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK3))
	{
		case 0:
			pStr[2] = SlylandroHomePhrase::WhereAreYou;
			break;
		case 1:
			pStr[2] = SlylandroHomePhrase::ThatsImpossible1;
			break;
		case 2:
			pStr[2] = SlylandroHomePhrase::ThatsImpossible2;
			break;
	}

	if (pStr[LastStack] != SlylandroHomePhrase::NullPhrase)
	{
		Response(pStr[LastStack], HomeWorld);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i] != SlylandroHomePhrase::NullPhrase)
		{
			Response(pStr[i], HomeWorld);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK1))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK4))
		{
			case 0:
				Response(SlylandroHomePhrase::HappyToTellMore, HumanInfo);
				break;
			case 1:
				Response(SlylandroHomePhrase::WouldYouLikeToKnowMore, HumanInfo);
				break;
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::PLAYER_KNOWS_PROBE)
		&& !uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN))
	{
		switch (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_STACK5))
		{
			case 0:
				Response(SlylandroHomePhrase::WhatAreProbes, ProbeInfo);
				break;
			case 1:
				if (PhraseEnabled(SlylandroHomePhrase::KnowMoreProbe))
				{
					Response(SlylandroHomePhrase::KnowMoreProbe, ProbeInfo);
				}
				break;
		}
	}
	Response(SlylandroHomePhrase::Bye, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};


	if (IsHd())
	{
		EngageFilters(&slyhome_filters);
	}

	CommData.AlienAmbientArray[0].AnimFlags &= ~AnimationFlags::Disabled;

	if (uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_KNOW_BROKEN)
		&& (NumVisits = uqm::gameState().flag(uqm::GameFlag::RECALL_VISITS)) == 0)
	{
		NPCPhrase(SlylandroHomePhrase::RecallProgram1);
		++NumVisits;
		uqm::gameState().setFlag(uqm::GameFlag::RECALL_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::SLYLANDRO_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SlylandroHomePhrase::Hello1);
				break;
			case 1:
				NPCPhrase(SlylandroHomePhrase::Hello2);
				break;
			case 2:
				NPCPhrase(SlylandroHomePhrase::Hello3);
				break;
			case 3:
				NPCPhrase(SlylandroHomePhrase::Hello4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SLYLANDRO_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		SetHomeworldKnown(*HomeId::Slylandro);
	}

	HomeWorld((ResponseRef)0);
}

static uint16_t uninit_slylandro(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_slylandro_enc(void)
{
	// nothing defined so far
}

ConversationData* init_slylandro_comm(void)
{
	slylandro_desc.init_encounter_func = Intro;
	slylandro_desc.post_encounter_func = post_slylandro_enc;
	slylandro_desc.uninit_encounter_func = uninit_slylandro;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	slylandro_desc.AlienTextBaseline.x =
		TextXOffs() + (SisTextWidth() >> 1);
	slylandro_desc.AlienTextBaseline.y = 0;
	slylandro_desc.AlienTextWidth = SisTextWidth();

	setSegue(Segue::Peace);
	ConversationData* retval {&slylandro_desc};

	return (retval);
}

} // namespace uqm
