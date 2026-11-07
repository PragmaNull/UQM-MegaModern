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
#include "uqm/lua/UQMLua.h"
#include "uqm/build.h"
#include "uqm/setup.h"


static ConversationData syreen_desc =
	{
		Conversation::Syreen, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		SyreenPmapAnim, /* AlienFrame */
		SyreenFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		SyreenColorMap, /* AlienColorMap */
		SyreenMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		SyreenConversationPhrases, /* PlayerPhrases */
		15, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				5,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				7,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				9,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				11,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				13,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				15,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12),				/* BlockMask */
			},
								{
				17,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				19,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 13),
			},
								{
				21,			/* StartIndex */
				6,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				27,		  /* StartIndex */
				4,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 14),				/* BlockMask */
			},
								{
				31,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				37,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				41,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
								{
				44, /* StartIndex */
				4,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 6,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond,	  /* RestartRate */
				(1 << 7) | (1 << 14), /* BlockMask */
			},
								{
				48, /* StartIndex */
				3,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond * 2 / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 10,
				GameTicksPerSecond,	  /* RestartRate */
				(1 << 9) | (1 << 13), /* BlockMask */
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
			4, /* NumFrames */
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

static AnimFilterSet syreen_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,	/* Color index */
				1,	/* Opacity index */
				-1, /* Frame index */
				DrawKind::Alpha,
				/* DrawKind*/	//maybe try multiply?
				SwitchOffAnims, /* Flags */
			},
								  }
};

static void FriendlyExit(RESPONSE_REF R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, SyreenPhrase::Bye))
	{
		NPCPhrase(SyreenPhrase::Goodbye);
	}
	else if (PlayerSaid(R, SyreenPhrase::ImOnMyWay)
			 || PlayerSaid(R, SyreenPhrase::DoingThisForYou)
			 || PlayerSaid(R, SyreenPhrase::IfIDie))
	{
		NPCPhrase(SyreenPhrase::GoodLuck);
	}
	else if (PlayerSaid(R, SyreenPhrase::ByeBeforeVault))
	{
		NPCPhrase(SyreenPhrase::GoodbyeBeforeVault);
	}
	else if (PlayerSaid(R, SyreenPhrase::ByeAfterVault))
	{
		NPCPhrase(SyreenPhrase::GoodbyeAfterVault);
	}
	else if (PlayerSaid(R, SyreenPhrase::ByeBeforeAmbush))
	{
		NPCPhrase(SyreenPhrase::GoodbyeBeforeAmbush);
	}
	else if (PlayerSaid(R, SyreenPhrase::ByeAfterAmbush))
	{
		NPCPhrase(SyreenPhrase::GoodbyeAfterAmbush);
	}
	else
	{
		if (PlayerSaid(R, SyreenPhrase::HandsOff))
		{
			SetCustomBaseLine(3, MAKE_POINT(108, 81), TextAlign::Center);
			SetCustomBaseLine(4, MAKE_POINT(80, 49), TextAlign::Center);
			SetCustomBaseLine(5, MAKE_POINT(121, 27), TextAlign::Center);

			NPCPhrase(SyreenPhrase::OkWontUseHands);
		}
		else if (PlayerSaid(R, SyreenPhrase::NotMuchMoreToSay))
		{
			NPCPhrase(SyreenPhrase::ThenStopTalking);
		}

		NPCPhrase(SyreenPhrase::Later);
		NPCPhrase(SyreenPhrase::SexGoodbye);


		AlienTalkSegue(1);

		RedrawSISComWindow();

		AlienTalkSegue(2);

		CommData.AlienTextFColor = WhiteColor;
		FadeScreen(ScreenFadeType::FadeAllToBlack, TimePeriod {});

		DrawSISFrame();
		DrawSISMessage(nullptr);
		DrawSISTitle(uqm::gameState().sis().PlanetName);
		SetCommDarkMode(false);
		RedrawSISComWindow();

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 0),
					  TimePeriod {});

		BlockTalkingAnim(1, 2); // Several hours later block (New)

		if (IsHd())
		{
			SwitchSequences(true);
			EnableTalkingAnim(true);
			DisengageFilters();
		}
		FadeScreen(ScreenFadeType::FadeAllToColor, c_gameSecond / 2);
		AlienTalkSegue((uint16_t)~0);

		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HAD_SEX, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HAVING_SEX, static_cast<uint32_t>(0));
	}
}

static void Sex(RESPONSE_REF R)
{
	if (PlayerSaid(R, SyreenPhrase::InTheSpirit))
	{
		SetCustomBaseLine(3, MAKE_POINT(98, 21), TextAlign::Center);
		SetCustomBaseLine(4, MAKE_POINT(156, 51), TextAlign::Center);
		SetCustomBaseLine(5, MAKE_POINT(169, 79), TextAlign::Center);

		NPCPhrase(SyreenPhrase::OkSpirit);
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatInMind))
	{
		SetCustomBaseLine(0, MAKE_POINT(3, 0), TextAlign::Left);
		SetCustomBaseLine(1, MAKE_POINT(3, 0), TextAlign::Left);
		SetCustomBaseLine(2, MAKE_POINT(3, 55), TextAlign::Left);
		SetCustomBaseLine(3, MAKE_POINT(125, 67), TextAlign::Center);
		SetCustomBaseLine(4, MAKE_POINT(240, 10), TextAlign::Right);
		SetCustomBaseLine(5, MAKE_POINT(240, 10), TextAlign::Right);

		NPCPhrase(SyreenPhrase::SomethingLikeThis);
	}
	else if (PlayerSaid(R, SyreenPhrase::Disease))
	{
		NPCPhrase(SyreenPhrase::JustRelax);
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatHappensIfITouchThis))
	{
		NPCPhrase(SyreenPhrase::ThisHappens);

		DisablePhrase(SyreenPhrase::WhatHappensIfITouchThis);
	}
	else if (PlayerSaid(R, SyreenPhrase::AreYouSureThisIsOk))
	{
		NPCPhrase(SyreenPhrase::YesSure);

		DisablePhrase(SyreenPhrase::AreYouSureThisIsOk);
	}
	else if (PlayerSaid(R, SyreenPhrase::BoyTheyNeverTaught))
	{
		NPCPhrase(SyreenPhrase::ThenLetMeTeach);

		DisablePhrase(SyreenPhrase::BoyTheyNeverTaught);
	}

	if (!PhraseEnabled(SyreenPhrase::WhatHappensIfITouchThis)
		&& !PhraseEnabled(SyreenPhrase::AreYouSureThisIsOk)
		&& !PhraseEnabled(SyreenPhrase::BoyTheyNeverTaught))
	{
		Response(SyreenPhrase::NotMuchMoreToSay, FriendlyExit);
	}
	else
	{
		if (PhraseEnabled(SyreenPhrase::WhatHappensIfITouchThis))
		{
			Response(SyreenPhrase::WhatHappensIfITouchThis, Sex);
		}
		if (PhraseEnabled(SyreenPhrase::AreYouSureThisIsOk))
		{
			Response(SyreenPhrase::AreYouSureThisIsOk, Sex);
		}
		if (PhraseEnabled(SyreenPhrase::BoyTheyNeverTaught))
		{
			Response(SyreenPhrase::BoyTheyNeverTaught, Sex);
		}
	}
}

static void Foreplay(RESPONSE_REF R)
{
	if (PlayerSaid(R, SyreenPhrase::WhatsMyReward)
		|| PlayerSaid(R, SyreenPhrase::WhatAboutUs))
	{
		if (PlayerSaid(R, SyreenPhrase::WhatsMyReward))
		{
			NPCPhrase(SyreenPhrase::HeresReward);
		}
		else
		{
			NPCPhrase(SyreenPhrase::AboutUs);
		}
		NPCPhrase(SyreenPhrase::MoreComfortable);

		cwLock = true;

		AlienTalkSegue(1);

		CommData.AlienTextFColor = BuildColorRgba(85, 255, 255, 0);
		SetCommDarkMode(true);

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 1),
					  c_gameSecond);

		if (IsHd())
		{
			EngageFilters(&syreen_filters);
		}

		/*if (!IsExtended())
			{
				CommData.AlienFrame = SetAbsFrameIndex
					(CommData.AlienFrame, 63);
				CommData.AlienTalkDesc.AnimFlags |= PauseTalking;
				RunOneTimeSequence(16, 0);
		}*/
		// For future ideas maybe

		AlienTalkSegue((uint16_t)~0);
		uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HAVING_SEX, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SyreenPhrase::WhyLightsOff))
	{
		SetCustomBaseLine(1, MAKE_POINT(60, 41), TextAlign::Center);
		SetCustomBaseLine(2, MAKE_POINT(180, 41), TextAlign::Center);
		SetCustomBaseLine(3, MAKE_POINT(120, 81), TextAlign::Center);

		NPCPhrase(SyreenPhrase::LightsOffBecause);

		DisablePhrase(SyreenPhrase::WhyLightsOff);
	}
	else if (PlayerSaid(R, SyreenPhrase::EvilMonster))
	{
		NPCPhrase(SyreenPhrase::NotEvilMonster);

		DisablePhrase(SyreenPhrase::EvilMonster);
	}

	if (PhraseEnabled(SyreenPhrase::WhyLightsOff))
	{
		Response(SyreenPhrase::WhyLightsOff, Foreplay);
	}
	else if (PhraseEnabled(SyreenPhrase::EvilMonster))
	{
		Response(SyreenPhrase::EvilMonster, Foreplay);
	}
	else
	{
		Response(SyreenPhrase::Disease, Sex);
	}
	Response(SyreenPhrase::InTheSpirit, Sex);
	Response(SyreenPhrase::WhatInMind, Sex);
	Response(SyreenPhrase::HandsOff, FriendlyExit);
}

static void AfterAmbush(RESPONSE_REF R)
{
	if (PlayerSaid(R, SyreenPhrase::WhatNowAfterAmbush))
	{
		NPCPhrase(SyreenPhrase::DoThisAfterAmbush);

		DisablePhrase(SyreenPhrase::WhatNowAfterAmbush);
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatAboutYou))
	{
		NPCPhrase(SyreenPhrase::AboutMe);

		DisablePhrase(SyreenPhrase::WhatAboutYou);
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatsUpAfterAmbush))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::SYREEN_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SyreenPhrase::GeneralInfoAfterAmbush1);
				break;
			case 1:
				NPCPhrase(SyreenPhrase::GeneralInfoAfterAmbush2);
				break;
			case 2:
				NPCPhrase(SyreenPhrase::GeneralInfoAfterAmbush3);
				break;
			case 3:
				NPCPhrase(SyreenPhrase::GeneralInfoAfterAmbush4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(SyreenPhrase::WhatsUpAfterAmbush);
	}

	if (PhraseEnabled(SyreenPhrase::WhatAboutYou))
	{
		Response(SyreenPhrase::WhatAboutYou, AfterAmbush);
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::PLAYER_HAD_SEX))
	{
		Response(SyreenPhrase::WhatAboutUs, Foreplay);
	}
	if (PhraseEnabled(SyreenPhrase::WhatNowAfterAmbush))
	{
		Response(SyreenPhrase::WhatNowAfterAmbush, AfterAmbush);
	}
	if (PhraseEnabled(SyreenPhrase::WhatsUpAfterAmbush))
	{
		Response(SyreenPhrase::WhatsUpAfterAmbush, AfterAmbush);
	}
	Response(SyreenPhrase::ByeAfterAmbush, FriendlyExit);
}

static void AmbushReady(RESPONSE_REF R)
{
	if (PlayerSaid(R, SyreenPhrase::RepeatPlan))
	{
		NPCPhrase(SyreenPhrase::OkRepeatPlan);

		DisablePhrase(SyreenPhrase::RepeatPlan);
	}

	if (PhraseEnabled(SyreenPhrase::RepeatPlan))
	{
		Response(SyreenPhrase::RepeatPlan, AmbushReady);
	}
	Response(SyreenPhrase::ByeBeforeAmbush, FriendlyExit);
}

static void SyreenShuttle(RESPONSE_REF R)
{
	if (PlayerSaid(R, SyreenPhrase::WhatsNextStep))
	{
		NPCPhrase(SyreenPhrase::OpenVault);

		DisablePhrase(SyreenPhrase::WhatsNextStep);
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatDoIGetForThis))
	{
		NPCPhrase(SyreenPhrase::Gratitude);

		DisablePhrase(SyreenPhrase::WhatDoIGetForThis);
	}
	else if (PlayerSaid(R, SyreenPhrase::NotSure))
	{
		NPCPhrase(SyreenPhrase::Please);

		DisablePhrase(SyreenPhrase::NotSure);
	}
	else if (PlayerSaid(R, SyreenPhrase::WhereIsIt))
	{
		NPCPhrase(SyreenPhrase::DontKnowWhere);
		NPCPhrase(SyreenPhrase::GiveShuttle);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_SHUTTLE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP, static_cast<uint32_t>(1));

		DisablePhrase(SyreenPhrase::WhereIsIt);
	}
	else if (PlayerSaid(R, SyreenPhrase::BeenThere))
	{
		NPCPhrase(SyreenPhrase::Great);
		NPCPhrase(SyreenPhrase::GiveShuttle);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_SHUTTLE, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP, static_cast<uint32_t>(1));

		DisablePhrase(SyreenPhrase::BeenThere);
	}

	if (PhraseEnabled(SyreenPhrase::WhatsNextStep))
	{
		Response(SyreenPhrase::WhatsNextStep, SyreenShuttle);
	}
	else
	{
		if (!uqm::gameState().flag(uqm::GameFlag::KNOW_SYREEN_VAULT))
		{
			if (PhraseEnabled(SyreenPhrase::WhereIsIt))
			{
				Response(SyreenPhrase::WhereIsIt, SyreenShuttle);
			}
		}
		else
		{
			if (PhraseEnabled(SyreenPhrase::BeenThere))
			{
				Response(SyreenPhrase::BeenThere, SyreenShuttle);
			}
		}
		if (!PhraseEnabled(SyreenPhrase::WhereIsIt)
			|| !PhraseEnabled(SyreenPhrase::BeenThere))
		{
			Response(SyreenPhrase::ImOnMyWay, FriendlyExit);
			Response(SyreenPhrase::DoingThisForYou, FriendlyExit);
			Response(SyreenPhrase::IfIDie, FriendlyExit);
		}
	}
	if (PhraseEnabled(SyreenPhrase::WhatDoIGetForThis))
	{
		Response(SyreenPhrase::WhatDoIGetForThis, SyreenShuttle);
	}
	if (PhraseEnabled(SyreenPhrase::NotSure))
	{
		Response(SyreenPhrase::NotSure, SyreenShuttle);
	}
}

static void NormalSyreen(RESPONSE_REF R)
{
	uint8_t i {};
	SyreenPhrase pStr[4];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = pStr[3] = SyreenPhrase::NullPhrase;
	if (PlayerSaid(R, SyreenPhrase::WeHereToHelp))
	{
		NPCPhrase(SyreenPhrase::NoNeedHelp);
	}
	else if (PlayerSaid(R, SyreenPhrase::WeNeedHelp))
	{
		NPCPhrase(SyreenPhrase::CantGiveHelp);
	}
	else if (PlayerSaid(R, SyreenPhrase::KnowAboutDeepChildren))
	{
		NPCPhrase(SyreenPhrase::WhatAboutDeepChildren);

		DisablePhrase(SyreenPhrase::KnowAboutDeepChildren);
	}
	else if (PlayerSaid(R, SyreenPhrase::MyconsInvolved))
	{
		NPCPhrase(SyreenPhrase::WhatProof);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_ABOUT_SHATTERED, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SyreenPhrase::HaveNoProof))
	{
		NPCPhrase(SyreenPhrase::NeedProof);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_WANT_PROOF, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, SyreenPhrase::HaveProof))
	{
		NPCPhrase(SyreenPhrase::SeeProof);

		DisablePhrase(SyreenPhrase::HaveProof);
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatDoingHere))
	{
		NPCPhrase(SyreenPhrase::OurNewWorld);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK0, static_cast<uint32_t>(1));
		LastStack = 1;
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatAboutWar))
	{
		NPCPhrase(SyreenPhrase::AboutWar);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK0, static_cast<uint32_t>(2));
		LastStack = 1;
	}
	else if (PlayerSaid(R, SyreenPhrase::HelpUs))
	{
		NPCPhrase(SyreenPhrase::WontHelp);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK0, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatAboutHistory))
	{
		NPCPhrase(SyreenPhrase::BeforeWar);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK1, static_cast<uint32_t>(1));
		LastStack = 2;
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatAboutHomeworld))
	{
		NPCPhrase(SyreenPhrase::AboutHomeworld);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK1, static_cast<uint32_t>(2));
		LastStack = 2;
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatHappened))
	{
		NPCPhrase(SyreenPhrase::DontKnowHow);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_SYREEN_WORLD_SHATTERED, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SyreenPhrase::WhatAboutOutfit))
	{
		NPCPhrase(SyreenPhrase::HopeYouLikeIt);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK2, static_cast<uint32_t>(1));
		LastStack = 3;
	}
	else if (PlayerSaid(R, SyreenPhrase::WhereMates))
	{
		NPCPhrase(SyreenPhrase::MatesKilled);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK2, static_cast<uint32_t>(2));
		LastStack = 3;
	}
	else if (PlayerSaid(R, SyreenPhrase::GetLonely))
	{
		NPCPhrase(SyreenPhrase::MakeOutAllRight);

		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_STACK2, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, SyreenPhrase::LookAtEggSacks))
	{
		NPCPhrase(SyreenPhrase::HorribleTruth);

		setSegue(Segue::Peace);
		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::SYREEN_KNOW_ABOUT_MYCON, static_cast<uint32_t>(1));

		if (!SyreenVoiceFix && usingSpeech)
		{
			SyreenShuttle((RESPONSE_REF)0);
		}
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) < 3)
	{
		if (uqm::gameState().flag(uqm::GameFlag::KNOW_ABOUT_SHATTERED) == 2
			&& uqm::gameState().flag(uqm::GameFlag::KNOW_SYREEN_WORLD_SHATTERED))
		{
			if (PhraseEnabled(SyreenPhrase::KnowAboutDeepChildren))
			{
				pStr[0] = SyreenPhrase::KnowAboutDeepChildren;
			}
			else
			{
				pStr[0] = SyreenPhrase::MyconsInvolved;
			}
		}
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP)
			|| uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP)
			|| uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP))
		{
			if (PhraseEnabled(SyreenPhrase::HaveProof))
			{
				pStr[0] = SyreenPhrase::HaveProof;
			}
			else
			{
				pStr[0] = SyreenPhrase::LookAtEggSacks;
			}
		}
		else if (!uqm::gameState().flag(uqm::GameFlag::SYREEN_WANT_PROOF))
		{
			pStr[0] = SyreenPhrase::HaveNoProof;
		}
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SYREEN_STACK0))
	{
		case 0:
			pStr[1] = SyreenPhrase::WhatDoingHere;
			break;
		case 1:
			pStr[1] = SyreenPhrase::WhatAboutWar;
			break;
		case 2:
			pStr[1] = SyreenPhrase::HelpUs;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SYREEN_STACK1))
	{
		case 0:
			pStr[2] = SyreenPhrase::WhatAboutHistory;
			break;
		case 1:
			pStr[2] = SyreenPhrase::WhatAboutHomeworld;
			break;
		case 2:
			pStr[2] = SyreenPhrase::WhatHappened;
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::SYREEN_STACK2))
	{
		case 0:
			pStr[3] = SyreenPhrase::WhatAboutOutfit;
			break;
		case 1:
			pStr[3] = SyreenPhrase::WhereMates;
			break;
		case 2:
			pStr[3] = SyreenPhrase::GetLonely;
			break;
	}
	if (pStr[LastStack] != SyreenPhrase::NullPhrase)
	{
		Response(pStr[LastStack], NormalSyreen);
	}
	for (i = 0; i < 4; ++i)
	{
		if (i != LastStack && *pStr[i] != SyreenPhrase::NullPhrase)
		{
			Response(pStr[i], NormalSyreen);
		}
	}
	Response(SyreenPhrase::Bye, FriendlyExit);
}

static void InitialSyreen(RESPONSE_REF R)
{
	if (PlayerSaid(R, SyreenPhrase::WeAreViceSquad))
	{
		NPCPhrase(SyreenPhrase::OkVice);
		NPCPhrase(SyreenPhrase::HowCanYouBeHere);
	}
	else if (PlayerSaid(R, SyreenPhrase::WeAreTheOneForYouBaby))
	{
		NPCPhrase(SyreenPhrase::MaybeCaptain);
		NPCPhrase(SyreenPhrase::HowCanYouBeHere);
	}
	else if (PlayerSaid(R, SyreenPhrase::WeAreVindicator))
	{
		NPCPhrase(SyreenPhrase::WelcomeVindicator);
		NPCPhrase(SyreenPhrase::HowCanYouBeHere);
	}
	else if (PlayerSaid(R, SyreenPhrase::WeAreImpressed))
	{
		NPCPhrase(SyreenPhrase::SoAmICaptain);
		NPCPhrase(SyreenPhrase::HowCanYouBeHere);
	}
	else if (PlayerSaid(R, SyreenPhrase::INeedYou))
	{
		NPCPhrase(SyreenPhrase::OkNeed);

		DisablePhrase(SyreenPhrase::INeedYou);
		DisablePhrase(SyreenPhrase::INeedTouchOVision);
	}
	else if (PlayerSaid(R, SyreenPhrase::INeedTouchOVision))
	{
		NPCPhrase(SyreenPhrase::TouchOVision);

		DisablePhrase(SyreenPhrase::INeedYou);
		DisablePhrase(SyreenPhrase::INeedTouchOVision);
	}

	Response(SyreenPhrase::WeHereToHelp, NormalSyreen);
	Response(SyreenPhrase::WeNeedHelp, NormalSyreen);
	if (PhraseEnabled(SyreenPhrase::INeedYou))
	{
		Response(SyreenPhrase::INeedYou, InitialSyreen);
	}
	if (PhraseEnabled(SyreenPhrase::INeedTouchOVision))
	{
		Response(SyreenPhrase::INeedTouchOVision, InitialSyreen);
	}
}

static void PlanAmbush(RESPONSE_REF R)
{
	(void)R; // ignored
	NPCPhrase(SyreenPhrase::OkFoundVault);

	uqm::gameState().setFlag(uqm::GameFlag::MYCON_AMBUSH, static_cast<uint32_t>(1));
	// This is redundant but left here for clarity
	uqm::gameState().setFlag(uqm::GameFlag::SYREEN_HOME_VISITS, static_cast<uint32_t>(0));

	// Send ambush fleet to Organon.  EncounterPercent for the
	// Syreen is 0, so this is purely decorative.
	if (IsExtended())
	{
		HFLEETINFO hSyreen =
			GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, *RaceId::Syreen);
		FleetInfo* SyreenPtr =
			LockFleetInfo(&uqm::gameState().game().avail_race_q, hSyreen);

		if (SyreenPtr)
		{
			SyreenPtr->actual_strength = 300 / SphereRadiusIncrement * 2;
			SyreenPtr->loc = SeedFleetLocation(SyreenPtr, plot_map, *PlotId::Home);
			StartSphereTracking(RaceId::Syreen);
			const GfxPoint loc = SeedFleetLocation(SyreenPtr, plot_map,
												   *PlotId::MyconTrap);
			SetRaceDest(*RaceId::Syreen, loc.x, loc.y, 15, (uint8_t)~0);
		}

		UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hSyreen);
	}

	Response(SyreenPhrase::WhatsMyReward, Foreplay);
	Response(SyreenPhrase::ByeAfterVault, FriendlyExit);
}

static void SyreenVault(RESPONSE_REF R)
{
	if (PlayerSaid(R, SyreenPhrase::VaultHint))
	{
		NPCPhrase(SyreenPhrase::OkHint);

		DisablePhrase(SyreenPhrase::VaultHint);
	}

	if (PhraseEnabled(SyreenPhrase::VaultHint))
	{
		Response(SyreenPhrase::VaultHint, SyreenVault);
	}
	Response(SyreenPhrase::ByeBeforeVault, FriendlyExit);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	SetHomeworldKnown(*HomeId::Syreen);

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(SyreenPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	NumVisits = uqm::gameState().flag(uqm::GameFlag::SYREEN_HOME_VISITS);
	if (uqm::gameState().flag(uqm::GameFlag::MYCON_KNOW_AMBUSH))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SyreenPhrase::HelloAfterAmbush1);
				SetRaceAllied(RaceId::Syreen, true);
				break;
			case 1:
				NPCPhrase(SyreenPhrase::HelloAfterAmbush2);
				break;
			case 2:
				NPCPhrase(SyreenPhrase::HelloAfterAmbush3);
				break;
			case 3:
				NPCPhrase(SyreenPhrase::HelloAfterAmbush3);
				--NumVisits;
				break;
		}

		AfterAmbush((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::MYCON_AMBUSH))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SyreenPhrase::ReadyForAmbush);
				--NumVisits;
				break;
		}

		AmbushReady((RESPONSE_REF)0);
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::SYREEN_KNOW_ABOUT_MYCON))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SyreenPhrase::HelloBeforeAmbush1);
				break;
			case 1:
				NPCPhrase(SyreenPhrase::HelloBeforeAmbush2);
				break;
			case 2:
				NPCPhrase(SyreenPhrase::HelloBeforeAmbush3);
				break;
			case 3:
				NPCPhrase(SyreenPhrase::HelloBeforeAmbush4);
				--NumVisits;
				break;
		}

		if (NumVisits > 1)
		{
			NormalSyreen((RESPONSE_REF)0);
		}
		else
		{
			Response(SyreenPhrase::WeAreViceSquad, InitialSyreen);
			Response(SyreenPhrase::WeAreTheOneForYouBaby, InitialSyreen);
			Response(SyreenPhrase::WeAreVindicator, InitialSyreen);
			Response(SyreenPhrase::WeAreImpressed, InitialSyreen);
		}
	}

	else if (!uqm::gameState().flag(uqm::GameFlag::SYREEN_SHUTTLE) && (SyreenVoiceFix || !usingSpeech))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SyreenPhrase::MustAct);
				--NumVisits;
				break;
		}

		SyreenShuttle((RESPONSE_REF)0);
	}

	else if (uqm::gameState().flag(uqm::GameFlag::SHIP_VAULT_UNLOCKED))
	{
		PlanAmbush((RESPONSE_REF)0);
		// XXX: PlanAmbush() sets SYREEN_HOME_VISITS=0, but then this value
		// is immediately reset to NumVisits just below. The intent was to
		// reset the HELLO stack so that is what we will do here as well.
		// Note that it is completely redundant because genvault.c resets
		// SYREEN_HOME_VISITS when it sets SHIP_VAULT_UNLOCKED=1.
		NumVisits = 0;
	}
	else
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(SyreenPhrase::FoundVaultYet1);
				break;
			case 1:
				NPCPhrase(SyreenPhrase::FoundVaultYet2);
				--NumVisits;
				break;
		}

		SyreenVault((RESPONSE_REF)0);
	}
	uqm::gameState().setFlag(uqm::GameFlag::SYREEN_HOME_VISITS, static_cast<uint32_t>(NumVisits));
}

static uint16_t uninit_syreen(void)
{
	FlushCustomBaseLine();
	uqm::lua().endComm();
	return (0);
}

static void post_syreen_enc(void)
{
	// nothing defined so far
}

ConversationData* init_syreen_comm(void)
{
	syreen_desc.init_encounter_func = Intro;
	syreen_desc.post_encounter_func = post_syreen_enc;
	syreen_desc.uninit_encounter_func = uninit_syreen;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	syreen_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	syreen_desc.AlienTextBaseline.y = 0;
	syreen_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	setSegue(Segue::Peace);
	ConversationData* retval {&syreen_desc};

	return (retval);
}
