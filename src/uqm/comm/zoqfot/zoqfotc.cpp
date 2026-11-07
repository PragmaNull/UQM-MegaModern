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

#include "core/types/enum.h"
#include "../commall.h"
#include "resinst.h"
#include "strings.h"

#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"
#include "uqm/gameev.h"


namespace uqm
{
static auto ZoqBaseX() { return (TextXOffs() + ((SisTextWidth() >> 1) >> 1)); }
static auto ZoqBaseY() { return ResScale(24); }
static constexpr auto ZoqTalkIndex {18};
static constexpr auto ZoqTalkFrames {5};
static constexpr auto FotToZoq {23};


static auto PikBaseX() { return (SisScreenWidth() - (TextXOffs() + ((SisTextWidth() >> 1) >> 1))); }
static auto PikBaseY() { return ResScale(24); }
static constexpr auto PikTalkIndex {29};
static constexpr auto PikTalkFrames {2};
static constexpr auto FotToPik {26};

static ConversationData zoqfot_desc =
	{
		Conversation::Zoqfotpik, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		ZoqfotpikPmapAnim, /* AlienFrame */
		ZoqfotpikFont, /* AlienFont */
		UndefinedColor, /* AlienTextFColor */
		UndefinedColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0, /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Middle, /* AlienTextValign */
		ZoqfotpikColorMap, /* AlienColorMap */
		ZoqfotpikMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		ZoqfotpikConversationPhrases, /* PlayerPhrases */
		3, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				/* Eye blink */
				1,		 /* StartIndex */
				4,		 /* NumFrames */
				YoyoAnim /* AnimFlags */
					| WaitTalking,
				GameTicksPerSecond / 24,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 10, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				/* Blow smoke */
				5,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond * 7 / 120,
				0, /* FrameRate */
				GameTicksPerSecond * 2,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				/* Gulp */
				10,			  /* StartIndex */
				8,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 10, /* RestartRate */
				0,						 /* BlockMask */
			},
								},
		{
								/* AlienTransitionDesc - Move Eye */
			FotToZoq, /* StartIndex */
			3,		  /* NumFrames */
			0,		  /* AnimFlags */
			GameTicksPerSecond / 30,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			ZoqTalkIndex,  /* StartIndex */
			ZoqTalkFrames, /* NumFrames */
			0,			   /* AnimFlags */
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

enum class ZoqFotPikAlien
{
	Zoq,
	Fot,
	Pik
};
consteval bool isArithmeticEnum(ZoqFotPikAlien) { return true; }

static int LastAlien;

// Queued and executes synchronously on the Starcon2Main thread
static void SelectAlienZOQ(CallbackArg arg)
{
	if (LastAlien != ZoqFotPikAlien::Zoq)
	{
		// Transition to neutral state first if Pik was talking
		if (LastAlien != ZoqFotPikAlien::Fot)
		{
			CommData.AlienTransitionDesc.AnimFlags |= TalkDone;
		}
		LastAlien = *ZoqFotPikAlien::Zoq;
		CommData.AlienTransitionDesc.AnimFlags |= TalkIntro;
		CommData.AlienTransitionDesc.StartIndex = FotToZoq;
		CommData.AlienTalkDesc.StartIndex = ZoqTalkIndex;
		CommData.AlienTalkDesc.NumFrames = ZoqTalkFrames;
		CommData.AlienAmbientArray[1].AnimFlags &= ~WaitTalking;

		CommData.AlienTextBaseline.x = (int16_t)ZoqBaseX();
		CommData.AlienTextBaseline.y = ZoqBaseY();
		CommData.AlienTextFColor = WhiteColor;
		CommData.AlienTextBColor = BlackColor;
	}

	(void)arg; // ignored
}

// Queued and executes synchronously on the Starcon2Main thread
static void SelectAlienPIK(CallbackArg arg)
{
	if (LastAlien != ZoqFotPikAlien::Pik)
	{
		// Transition to neutral state first if Zoq was talking
		if (LastAlien != ZoqFotPikAlien::Fot)
		{
			CommData.AlienTransitionDesc.AnimFlags |= TalkDone;
		}
		LastAlien = *ZoqFotPikAlien::Pik;
		CommData.AlienTransitionDesc.AnimFlags |= TalkIntro;
		CommData.AlienTransitionDesc.StartIndex = FotToPik;
		CommData.AlienTalkDesc.StartIndex = PikTalkIndex;
		CommData.AlienTalkDesc.NumFrames = PikTalkFrames;
		CommData.AlienAmbientArray[1].AnimFlags |= WaitTalking;

		CommData.AlienTextBaseline.x = (int16_t)PikBaseX();
		CommData.AlienTextBaseline.y = PikBaseY();
		CommData.AlienTextFColor = WhiteColor;
		CommData.AlienTextBColor = BlackColor;
	}

	(void)arg; // ignored
}

static void ZFPTalkSegue(uint16_t wait_track)
{
	LastAlien = *ZoqFotPikAlien::Fot;
	SelectAlienZOQ(0);
	AlienTalkSegue(wait_track);
}

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, ZoqFotPikPhrase::ByeHomeworld))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::GoodbyeHome0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::GoodbyeHome1, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::DecideLater))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::PleaseHurry0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::PleaseHurry1, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::ValuableInfo))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::Goodbye0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Goodbye1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Goodbye2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Goodbye3, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::HowCanIHelp))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Emmissaries7, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		SetHomeworldKnown(*HomeId::Zoqfotpik);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::Sure))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::WeAlly0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAlly1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAlly2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAlly3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAlly4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAlly5, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);
		SetRaceAllied(RaceId::Zoqfotpik, true);
		AddEvent(EventType::Relative, 3, 0, 0, GameEventId::ZoqfotDistressEvent);
		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_HOME_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::AllVeryInteresting))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::SeeToldYou0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::SeeToldYou1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::SeeToldYou2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::SeeToldYou3, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_HOSTILE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_HOME_VISITS, static_cast<uint32_t>(0));
		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::Never))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::WeEnemies0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeEnemies1, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_HOSTILE, static_cast<uint32_t>(1));
		setSegue(Segue::Hostile);
	}
}

static void FormAlliance(RESPONSE_REF R)
{
	(void)R; // ignored
	NPCPhrase_cb(ZoqFotPikPhrase::AllyWithUs0, &SelectAlienZOQ);
	NPCPhrase_cb(ZoqFotPikPhrase::AllyWithUs1, &SelectAlienPIK);
	NPCPhrase_cb(ZoqFotPikPhrase::AllyWithUs2, &SelectAlienZOQ);
	NPCPhrase_cb(ZoqFotPikPhrase::AllyWithUs3, &SelectAlienPIK);
	NPCPhrase_cb(ZoqFotPikPhrase::AllyWithUs4, &SelectAlienZOQ);
	NPCPhrase_cb(ZoqFotPikPhrase::AllyWithUs5, &SelectAlienPIK);
	ZFPTalkSegue((uint16_t)~0);

	Response(ZoqFotPikPhrase::Sure, ExitConversation);
	Response(ZoqFotPikPhrase::Never, ExitConversation);
	Response(ZoqFotPikPhrase::DecideLater, ExitConversation);
}

static void ZoqFotIntro(RESPONSE_REF R)
{
	if (PlayerSaid(R, ZoqFotPikPhrase::WeAreVindicator))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::WeGlad0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeGlad1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::WeGlad2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeGlad3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::WeGlad4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeGlad5, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WeAreVindicator);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::YourRace))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo7, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo8, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo9, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo10, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo11, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo12, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::YearsAgo13, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::YourRace);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhereFrom))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::TraveledFar0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::TraveledFar1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::TraveledFar2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::TraveledFar3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::TraveledFar4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::TraveledFar5, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhereFrom);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhatEmergency))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack7, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack8, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack9, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack10, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::UnderAttack11, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhatEmergency);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::ToughLuck))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::NotHelpful0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::NotHelpful1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::NotHelpful2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::NotHelpful3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::NotHelpful4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::NotHelpful5, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::ToughLuck);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhatLookLike))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::LookLike0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::LookLike1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::LookLike2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::LookLike3, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhatLookLike);
	}

	if (PhraseEnabled(ZoqFotPikPhrase::YourRace)
		|| PhraseEnabled(ZoqFotPikPhrase::WhereFrom)
		|| PhraseEnabled(ZoqFotPikPhrase::WhatEmergency))
	{
		if (PhraseEnabled(ZoqFotPikPhrase::YourRace))
		{
			Response(ZoqFotPikPhrase::YourRace, ZoqFotIntro);
		}
		if (PhraseEnabled(ZoqFotPikPhrase::WhereFrom))
		{
			Response(ZoqFotPikPhrase::WhereFrom, ZoqFotIntro);
		}
		if (PhraseEnabled(ZoqFotPikPhrase::WhatEmergency))
		{
			Response(ZoqFotPikPhrase::WhatEmergency, ZoqFotIntro);
		}
	}
	else
	{
		if (PhraseEnabled(ZoqFotPikPhrase::ToughLuck))
		{
			Response(ZoqFotPikPhrase::ToughLuck, ZoqFotIntro);
		}
		if (PhraseEnabled(ZoqFotPikPhrase::WhatLookLike))
		{
			Response(ZoqFotPikPhrase::WhatLookLike, ZoqFotIntro);
		}
		Response(ZoqFotPikPhrase::AllVeryInteresting, ExitConversation);
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			Response(ZoqFotPikPhrase::HowCanIHelp, FormAlliance);
		}
		else
		{
			Response(ZoqFotPikPhrase::HowCanIHelp, ExitConversation);
		}
		Response(ZoqFotPikPhrase::ValuableInfo, ExitConversation);
	}
}

static void AquaintZoqFot(RESPONSE_REF R)
{
	if (PlayerSaid(R, ZoqFotPikPhrase::WhichFot))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::HeIs7, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhichFot);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::QuietToadies))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::ToldYou7, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::QuietToadies);
	}

	if (PhraseEnabled(ZoqFotPikPhrase::WhichFot))
	{
		Response(ZoqFotPikPhrase::WhichFot, AquaintZoqFot);
	}
	if (PhraseEnabled(ZoqFotPikPhrase::WeAreVindicator))
	{
		Response(ZoqFotPikPhrase::WeAreVindicator, ZoqFotIntro);
	}
	if (PhraseEnabled(ZoqFotPikPhrase::QuietToadies))
	{
		Response(ZoqFotPikPhrase::QuietToadies, AquaintZoqFot);
	}
	Response(ZoqFotPikPhrase::AllVeryInteresting, ExitConversation);
	Response(ZoqFotPikPhrase::ValuableInfo, ExitConversation);
}

static void ZoqFotHome(RESPONSE_REF R);

static void ZoqFotInfo(RESPONSE_REF R)
{
	if (PlayerSaid(R, ZoqFotPikPhrase::WantSpecificInfo))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::WhatSpecificInfo0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WhatSpecificInfo1, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhatAboutOthers))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers7, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers8, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers9, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers10, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers11, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers12, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutOthers13, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhatAboutOthers);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhatAboutZebranky))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutZebranky7, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhatAboutZebranky);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhatAboutStinger))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::AboutStinger0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutStinger1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutStinger2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutStinger3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutStinger4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutStinger5, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhatAboutStinger);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhatAboutGuyInBack))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::AboutGuy0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutGuy1, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhatAboutGuyInBack);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::WhatAboutPast))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast7, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast8, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast9, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast10, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::AboutPast11, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		DisablePhrase(ZoqFotPikPhrase::WhatAboutPast);
	}

	uint8_t InfoLeft {false};
	if (PhraseEnabled(ZoqFotPikPhrase::WhatAboutOthers))
	{
		Response(ZoqFotPikPhrase::WhatAboutOthers, ZoqFotInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(ZoqFotPikPhrase::WhatAboutZebranky))
	{
		Response(ZoqFotPikPhrase::WhatAboutZebranky, ZoqFotInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(ZoqFotPikPhrase::WhatAboutStinger))
	{
		Response(ZoqFotPikPhrase::WhatAboutStinger, ZoqFotInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(ZoqFotPikPhrase::WhatAboutGuyInBack))
	{
		Response(ZoqFotPikPhrase::WhatAboutGuyInBack, ZoqFotInfo);
		InfoLeft = true;
	}
	if (PhraseEnabled(ZoqFotPikPhrase::WhatAboutPast))
	{
		Response(ZoqFotPikPhrase::WhatAboutPast, ZoqFotInfo);
		InfoLeft = true;
	}
	Response(ZoqFotPikPhrase::EnoughInfo, ZoqFotHome);

	if (!InfoLeft)
	{
		DisablePhrase(ZoqFotPikPhrase::WantSpecificInfo);
	}
}

static constexpr auto UtwigBuyTime {1 << 0};
static constexpr auto KohrAhWin {1 << 1};
static constexpr auto UrquanLose {1 << 2};
static constexpr auto KohrAhKill {1 << 3};
static constexpr auto KnowAll {UtwigBuyTime | KohrAhWin | UrquanLose | KohrAhKill};

static void ZoqFotHome(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, ZoqFotPikPhrase::WhatsUpHomeworld))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ZOQFOT_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo10, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo11, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo12, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo13, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				break;
			case 1:
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo20, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo21, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo22, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo23, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo24, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo25, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo26, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo27, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				break;
			case 2:
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo30, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo31, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo32, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo33, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo34, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo35, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				break;
			case 3:
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo40, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo41, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo42, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo43, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo44, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo45, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo46, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo47, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo48, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo49, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo410, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::GeneralInfo411, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(ZoqFotPikPhrase::WhatsUpHomeworld);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::AnyWarNews))
	{
		uint8_t KnowMask {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::UTWIG_SUPOX_MISSION);
		KnowMask = uqm::gameState().flag(uqm::GameFlag::ZOQFOT_KNOW_MASK);
		if (!(KnowMask & KohrAhKill) && uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
		{
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy0, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy1, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy2, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy3, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy4, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy5, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy6, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy7, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy8, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy9, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy10, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::KohrahFrenzy11, &SelectAlienPIK);
			ZFPTalkSegue((uint16_t)~0);

			KnowMask = KnowAll;
		}
		else if (!(KnowMask & UtwigBuyTime)
				 && NumVisits > 0 && NumVisits < 5)
		{
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay0, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay1, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay2, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay3, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay4, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay5, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay6, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay7, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay8, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay9, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay10, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay11, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay12, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::UtwigDelay13, &SelectAlienPIK);
			ZFPTalkSegue((uint16_t)~0);

			KnowMask |= UtwigBuyTime;
		}
		else
		{
			int16_t i {};

			i = StartYear + (YearsToKohrahVictory() + difficultyCase(2, 0, -1));
			if (NumVisits)
			{
				++i;
			}
			if ((i -= uqm::gameState().clock().year()) == 1
				&& uqm::gameState().clock().month() > 2)
			{
				i = 0;
			}
			if (!(KnowMask & UrquanLose) && i <= 0)
			{
				NPCPhrase_cb(ZoqFotPikPhrase::UrquanNearlyGone0, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::UrquanNearlyGone1, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::UrquanNearlyGone2, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::UrquanNearlyGone3, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::UrquanNearlyGone4, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::UrquanNearlyGone5, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);

				KnowMask |= KohrAhWin | UrquanLose;
			}
			else if (!(KnowMask & KohrAhWin) && i == 1)
			{
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning0, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning1, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning2, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning3, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning4, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning5, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning6, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning7, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning8, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::KohrahWinning9, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);

				KnowMask |= KohrAhWin;
			}
			else
			{
				NPCPhrase_cb(ZoqFotPikPhrase::NoWarNews0, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::NoWarNews1, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_KNOW_MASK, static_cast<uint32_t>(KnowMask));

		DisablePhrase(ZoqFotPikPhrase::AnyWarNews);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::IWantAlliance))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::Good0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Good1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Good2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Good3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Good4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Good5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Good6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Good7, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::Good8, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::Good9, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);

		SetRaceAllied(RaceId::Zoqfotpik, true);
		AddEvent(EventType::Relative, 3, 0, 0, GameEventId::ZoqfotDistressEvent);
	}
	else if (PlayerSaid(R, ZoqFotPikPhrase::EnoughInfo))
	{
		NPCPhrase_cb(ZoqFotPikPhrase::OkEnoughInfo, &SelectAlienZOQ);
		ZFPTalkSegue((uint16_t)~0);
	}

	if (PhraseEnabled(ZoqFotPikPhrase::WhatsUpHomeworld))
	{
		Response(ZoqFotPikPhrase::WhatsUpHomeworld, ZoqFotHome);
	}
	if (PhraseEnabled(ZoqFotPikPhrase::AnyWarNews))
	{
		Response(ZoqFotPikPhrase::AnyWarNews, ZoqFotHome);
	}
	if (CheckAlliance(RaceId::Zoqfotpik) != RaceAllegiance::Good)
	{
		Response(ZoqFotPikPhrase::IWantAlliance, ZoqFotHome);
	}
	else if (PhraseEnabled(ZoqFotPikPhrase::WantSpecificInfo))
	{
		Response(ZoqFotPikPhrase::WantSpecificInfo, ZoqFotInfo);
	}
	Response(ZoqFotPikPhrase::ByeHomeworld, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	SetHomeworldKnown(*HomeId::Zoqfotpik);

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes7, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes8, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes9, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes10, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes11, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes12, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::OutTakes13, &SelectAlienPIK);
		ZFPTalkSegue((uint16_t)~0);
		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_HOSTILE))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::ZOQFOT_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello10, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello11, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				break;
			case 1:
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello20, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello21, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello22, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello23, &SelectAlienPIK);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello24, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello25, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				break;
			case 2:
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello30, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello31, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				break;
			case 3:
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello40, &SelectAlienZOQ);
				NPCPhrase_cb(ZoqFotPikPhrase::HostileHello41, &SelectAlienPIK);
				ZFPTalkSegue((uint16_t)~0);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::MET_ZOQFOT))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MET_ZOQFOT, static_cast<uint32_t>(1));

		NPCPhrase_cb(ZoqFotPikPhrase::WeAre0, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAre1, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAre2, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAre3, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAre4, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAre5, &SelectAlienPIK);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAre6, &SelectAlienZOQ);
		NPCPhrase_cb(ZoqFotPikPhrase::WeAre7, &SelectAlienPIK);

		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NPCPhrase_cb(ZoqFotPikPhrase::InitHomeHello0, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::InitHomeHello1, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::InitHomeHello2, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::InitHomeHello3, &SelectAlienPIK);
		}
		else
		{
			NPCPhrase_cb(ZoqFotPikPhrase::ScoutHello0, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ScoutHello1, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::ScoutHello2, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ScoutHello3, &SelectAlienPIK);
		}

		ZFPTalkSegue((uint16_t)~0);

		AquaintZoqFot(0);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::ZOQFOT_DISTRESS))
		{
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue0, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue1, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue2, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue3, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue4, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue5, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue6, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue7, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue8, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue9, &SelectAlienPIK);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue10, &SelectAlienZOQ);
			NPCPhrase_cb(ZoqFotPikPhrase::ThanksForRescue11, &SelectAlienPIK);
			ZFPTalkSegue((uint16_t)~0);

			uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_DISTRESS, static_cast<uint32_t>(0));
			AddEscortShips(RaceId::Zoqfotpik, difficultyCase(4, 4, 2));
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::ZOQFOT_HOME_VISITS);
			if (CheckAlliance(RaceId::Zoqfotpik) != RaceAllegiance::Good)
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello10, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello11, &SelectAlienPIK);
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello12, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello13, &SelectAlienPIK);
						break;
					case 1:
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello20, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello21, &SelectAlienPIK);
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello22, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::NeutralHomeHello23, &SelectAlienPIK);
						--NumVisits;
						break;
				}
				ZFPTalkSegue((uint16_t)~0);
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello10, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello11, &SelectAlienPIK);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello12, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello13, &SelectAlienPIK);
						ZFPTalkSegue((uint16_t)~0);
						break;
					case 1:
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello20, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello21, &SelectAlienPIK);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello22, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello23, &SelectAlienPIK);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello24, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello25, &SelectAlienPIK);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello26, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello27, &SelectAlienPIK);
						ZFPTalkSegue((uint16_t)~0);
						break;
					case 2:
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello30, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello31, &SelectAlienPIK);
						ZFPTalkSegue((uint16_t)~0);
						break;
					case 3:
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello40, &SelectAlienZOQ);
						NPCPhrase_cb(ZoqFotPikPhrase::AlliedHomeHello41, &SelectAlienPIK);
						ZFPTalkSegue((uint16_t)~0);
						--NumVisits;
						break;
				}
			}
			uqm::gameState().setFlag(uqm::GameFlag::ZOQFOT_HOME_VISITS, static_cast<uint32_t>(NumVisits));
		}

		ZoqFotHome(0);
	}
}

static uint16_t uninit_zoqfot(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_zoqfot_enc(void)
{
	// nothing defined so far
}

ConversationData* init_zoqfot_comm(void)
{
	zoqfot_desc.init_encounter_func = Intro;
	zoqfot_desc.post_encounter_func = post_zoqfot_enc;
	zoqfot_desc.uninit_encounter_func = uninit_zoqfot;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	zoqfot_desc.AlienTextWidth = (SisTextWidth() >> 1) - TextXOffs();

	if (CheckAlliance(RaceId::Zoqfotpik) == RaceAllegiance::Good
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}

	ConversationData* retval {&zoqfot_desc};

	return (retval);
}

} // namespace uqm
