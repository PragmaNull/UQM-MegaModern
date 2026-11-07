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
#include "uqm/lua/UQMLua.h"

namespace uqm
{
static ConversationData urquan_desc =
	{
		Conversation::Urquan, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		UrquanPmapAnim, /* AlienFrame */
		UrquanFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		UrquanColorMap, /* AlienColorMap */
		UrquanMusic, /* AlienSong */
		{
								NullResource,	   /* AlienAltFrame */
			UrquanAltcolorMap, /* AlienAltColorMap */
			UrquanProbeMusic,  /* AlienAltSong */
		},
		UrquanConversationPhrases, /* PlayerPhrases */
		9, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				7,						  /* StartIndex */
				6,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				13,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				19,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				22,						  /* StartIndex */
				7,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				29,						  /* StartIndex */
				7,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				36,						  /* StartIndex */
				8,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				44,													 /* StartIndex */
				5,													 /* NumFrames */
				AnimationFlags::Circular | AnimationFlags::Disabled, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond * 5, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				49,													 /* StartIndex */
				1,													 /* NumFrames */
				AnimationFlags::Circular | AnimationFlags::Disabled, /* AnimFlags */
				0,
				0, /* FrameRate */
				0,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								},
		{
								/* AlienTransitionDesc */
			1,					  /* StartIndex */
			2,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
			GameTicksPerSecond / 30,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			2,					  /* StartIndex */
			5,					  /* NumFrames */
			AnimationFlags::None, /* AnimFlags */
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

static void CombatIsInevitable(ResponseRef R)
{
	setSegue(Segue::Hostile);

	if (PlayerSaid(R, UrquanPhrase::YouMustSurrender))
	{
		NPCPhrase(UrquanPhrase::Nope);
	}
	else if (PlayerSaid(R, UrquanPhrase::WhatsUp1)
			 || PlayerSaid(R, UrquanPhrase::WhatsUp2)
			 || PlayerSaid(R, UrquanPhrase::WhatsUp3)
			 || PlayerSaid(R, UrquanPhrase::WhatsUp4))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::URQUAN_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(UrquanPhrase::GeneralInfo1);
				break;
			case 1:
				NPCPhrase(UrquanPhrase::GeneralInfo2);
				break;
			case 2:
				NPCPhrase(UrquanPhrase::GeneralInfo3);
				break;
			case 3:
				NPCPhrase(UrquanPhrase::GeneralInfo4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_INFO, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, UrquanPhrase::IWontSurrender))
	{
		NPCPhrase(UrquanPhrase::BadChoice);
	}
	else if (PlayerSaid(R, UrquanPhrase::IWillSurrender))
	{
		NPCPhrase(UrquanPhrase::GoodChoice);

		setSegue(Segue::Defeat);
	}
	else if (PlayerSaid(R, UrquanPhrase::LikeToLeave))
	{
		NPCPhrase(UrquanPhrase::IndependenceIsBad);
	}
	else if (PlayerSaid(R, UrquanPhrase::ByeWarsOver))
	{
		NPCPhrase(UrquanPhrase::GoodbyeWarsOver);

		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, UrquanPhrase::ByeSubHypno))
	{
		NPCPhrase(UrquanPhrase::GoodbyeAndDieSubHypno);
	}
	else if (PlayerSaid(R, UrquanPhrase::ByeInitHypno))
	{
		NPCPhrase(UrquanPhrase::GoodbyeAndDieInitHypno);

		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_HYPNO_VISITS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UrquanPhrase::TerranAmphibian)
			 || PlayerSaid(R, UrquanPhrase::TalkingPetOnSteroids))
	{
		NPCPhrase(UrquanPhrase::BadNews);

		setSegue(Segue::Peace);
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_HYPNO_VISITS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UrquanPhrase::FallingAsleep)
			 || PlayerSaid(R, UrquanPhrase::SomeoneElseControlled))
	{
		NPCPhrase(UrquanPhrase::SoundsFamiliar);

		setSegue(Segue::Peace);
		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_HYPNO_VISITS, static_cast<uint32_t>(1));
	}
}

static void DescribePet(ResponseRef R)
{
	if (PlayerSaid(R, UrquanPhrase::WascallyLittleGuy))
	{
		NPCPhrase(UrquanPhrase::WhatItLookLike);
	}
	else if (PlayerSaid(R, UrquanPhrase::TurdAndToad))
	{
		NPCPhrase(UrquanPhrase::WhatIsTurdAndToad);

		DisablePhrase(UrquanPhrase::TurdAndToad);
	}

	Response(UrquanPhrase::TerranAmphibian, CombatIsInevitable);
	Response(UrquanPhrase::TalkingPetOnSteroids, CombatIsInevitable);
	if (PhraseEnabled(UrquanPhrase::TurdAndToad))
	{
		Response(UrquanPhrase::TurdAndToad, DescribePet);
	}
}

static void DescribeCompulsion(ResponseRef R)
{
	if (PlayerSaid(R, UrquanPhrase::AlienMindControl))
	{
		NPCPhrase(UrquanPhrase::WhatFeltLike);
	}
	else if (PlayerSaid(R, UrquanPhrase::BeforeCoffee))
	{
		NPCPhrase(UrquanPhrase::Explain);

		DisablePhrase(UrquanPhrase::BeforeCoffee);
	}

	Response(UrquanPhrase::FallingAsleep, CombatIsInevitable);
	Response(UrquanPhrase::SomeoneElseControlled, CombatIsInevitable);
	if (PhraseEnabled(UrquanPhrase::BeforeCoffee))
	{
		Response(UrquanPhrase::BeforeCoffee, DescribeCompulsion);
	}
}

static void MentionCompulsion(ResponseRef R)
{
	if (PlayerSaid(R, UrquanPhrase::Compulsion))
	{
		NPCPhrase(UrquanPhrase::WhatCompulsion);

		uqm::gameState().setFlag(uqm::GameFlag::MENTIONED_PET_COMPULSION, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UrquanPhrase::PossessedByDevil))
	{
		NPCPhrase(UrquanPhrase::StupidDevil);

		DisablePhrase(UrquanPhrase::PossessedByDevil);
	}
	else if (PlayerSaid(R, UrquanPhrase::WhyExplain))
	{
		NPCPhrase(UrquanPhrase::MustExplain);

		DisablePhrase(UrquanPhrase::WhyExplain);
	}

	Response(UrquanPhrase::WascallyLittleGuy, DescribePet);
	Response(UrquanPhrase::AlienMindControl, DescribeCompulsion);
	if (PhraseEnabled(UrquanPhrase::PossessedByDevil))
	{
		Response(UrquanPhrase::PossessedByDevil, MentionCompulsion);
	}
	if (PhraseEnabled(UrquanPhrase::WhyExplain))
	{
		Response(UrquanPhrase::WhyExplain, MentionCompulsion);
	}
}

static void UrquanHypno(ResponseRef R)
{
	if (uqm::gameState().flag(uqm::GameFlag::URQUAN_HYPNO_VISITS) == 0)
	{
		if (R == 0)
		{
			NPCPhrase(UrquanPhrase::InitUrquanWakeUp);
		}
		else if (PlayerSaid(R, UrquanPhrase::WhereAmI))
		{
			NPCPhrase(UrquanPhrase::YouAreHere);

			DisablePhrase(UrquanPhrase::WhereAmI);
		}
		else if (PlayerSaid(R, UrquanPhrase::WhyDoesMyHeadHurt))
		{
			NPCPhrase(UrquanPhrase::HurtsBecause);

			DisablePhrase(UrquanPhrase::WhyDoesMyHeadHurt);
		}
		else if (PlayerSaid(R, UrquanPhrase::WhatAbout2Weeks))
		{
			NPCPhrase(UrquanPhrase::About2Weeks);

			DisablePhrase(UrquanPhrase::WhatAbout2Weeks);
		}

		if (PhraseEnabled(UrquanPhrase::WhereAmI))
		{
			Response(UrquanPhrase::WhereAmI, UrquanHypno);
		}
		if (PhraseEnabled(UrquanPhrase::WhyDoesMyHeadHurt))
		{
			Response(UrquanPhrase::WhyDoesMyHeadHurt, UrquanHypno);
		}
		if (PhraseEnabled(UrquanPhrase::WhatAbout2Weeks))
		{
			Response(UrquanPhrase::WhatAbout2Weeks, UrquanHypno);
		}
		Response(UrquanPhrase::Compulsion, MentionCompulsion);
		Response(UrquanPhrase::ByeInitHypno, CombatIsInevitable);
	}
	else
	{
		if (R == 0)
		{
			NPCPhrase(UrquanPhrase::SubsequentUrquanWakeUp);
		}
		else if (PlayerSaid(R, UrquanPhrase::UhOh))
		{
			NPCPhrase(UrquanPhrase::NoUhOh);

			DisablePhrase(UrquanPhrase::UhOh);
		}
		else if (PlayerSaid(R, UrquanPhrase::StopMeeting))
		{
			NPCPhrase(UrquanPhrase::NoStopMeeting);

			DisablePhrase(UrquanPhrase::StopMeeting);
		}

		if (PhraseEnabled(UrquanPhrase::UhOh))
		{
			Response(UrquanPhrase::UhOh, UrquanHypno);
		}
		if (PhraseEnabled(UrquanPhrase::StopMeeting))
		{
			Response(UrquanPhrase::StopMeeting, UrquanHypno);
		}
		if (!uqm::gameState().flag(uqm::GameFlag::MENTIONED_PET_COMPULSION))
		{
			Response(UrquanPhrase::Compulsion, MentionCompulsion);
		}
		Response(UrquanPhrase::ByeSubHypno, CombatIsInevitable);
	}
}

static void NormalUrquan(ResponseRef R)
{
	if (PlayerSaid(R, UrquanPhrase::ISurrender))
	{
		NPCPhrase(UrquanPhrase::DisobediencePunished);

		Response(UrquanPhrase::IWontSurrender, CombatIsInevitable);
		Response(UrquanPhrase::IWillSurrender, CombatIsInevitable);
	}
	else
	{
		if (PlayerSaid(R, UrquanPhrase::KeyPhrase))
		{
			NPCPhrase(UrquanPhrase::UrquanStory);

			uqm::gameState().setFlag(uqm::GameFlag::KNOW_URQUAN_STORY, static_cast<uint32_t>(2));
		}

		Response(UrquanPhrase::YouMustSurrender, CombatIsInevitable);
		if (uqm::gameState().flag(uqm::GameFlag::KNOW_URQUAN_STORY) == 1)
		{
			Response(UrquanPhrase::KeyPhrase, NormalUrquan);
		}
		switch (uqm::gameState().flag(uqm::GameFlag::URQUAN_INFO))
		{
			case 0:
				Response(UrquanPhrase::WhatsUp1, CombatIsInevitable);
				break;
			case 1:
				Response(UrquanPhrase::WhatsUp2, CombatIsInevitable);
				break;
			case 2:
				Response(UrquanPhrase::WhatsUp3, CombatIsInevitable);
				break;
			case 3:
				Response(UrquanPhrase::WhatsUp4, CombatIsInevitable);
				break;
		}
		Response(UrquanPhrase::ISurrender, NormalUrquan);
		Response(UrquanPhrase::LikeToLeave, CombatIsInevitable);
	}
}

static void LoserUrquan(ResponseRef R)
{
	if (PlayerSaid(R, UrquanPhrase::WhyFlee))
	{
		NPCPhrase(UrquanPhrase::FleeBecause);

		DisablePhrase(UrquanPhrase::WhyFlee);
	}
	else if (PlayerSaid(R, UrquanPhrase::WhatHappensNow))
	{
		NPCPhrase(UrquanPhrase::HappensNow);

		DisablePhrase(UrquanPhrase::WhatHappensNow);
	}
	else if (PlayerSaid(R, UrquanPhrase::WhatAboutYou))
	{
		NPCPhrase(UrquanPhrase::AboutUs);

		DisablePhrase(UrquanPhrase::WhatAboutYou);
	}

	if (PhraseEnabled(UrquanPhrase::WhyFlee))
	{
		Response(UrquanPhrase::WhyFlee, LoserUrquan);
	}
	if (PhraseEnabled(UrquanPhrase::WhatHappensNow))
	{
		Response(UrquanPhrase::WhatHappensNow, LoserUrquan);
	}
	if (PhraseEnabled(UrquanPhrase::WhatAboutYou))
	{
		Response(UrquanPhrase::WhatAboutYou, LoserUrquan);
	}
	Response(UrquanPhrase::ByeWarsOver, CombatIsInevitable);
}

static void Intro(void)
{
	if (IsExtended())
	{
		CommData.AlienAmbientArray[7].AnimFlags &= ~AnimationFlags::Disabled;
		CommData.AlienAmbientArray[8].AnimFlags &= ~AnimationFlags::Circular;
	}

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(UrquanPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	uint32_t GrpOffs {uqm::gameState().flag(uqm::GameFlag::URQUAN_PROBE_GRPOFFS)};
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		&& uqm::gameState().game().BattleGroupRef
		&& uqm::gameState().game().BattleGroupRef == GrpOffs)
	{
		NPCPhrase(UrquanPhrase::SendMessage);
		uqm::gameState().setFlag(uqm::GameFlag::PROBE_MESSAGE_DELIVERED, static_cast<uint32_t>(1));
	}
	else if (uqm::gameState().flag(uqm::GameFlag::PLAYER_HYPNOTIZED))
	{
		SetCommIntroMode(CommIntroMode::FadeInScreen, c_gameSecond * 5);
		UrquanHypno((ResponseRef)0);
	}
	else
	{
		uint8_t NumVisits {};

		if (!uqm::gameState().flag(uqm::GameFlag::URQUAN_SENSES_EVIL)
			&& uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
		{
			NPCPhrase(UrquanPhrase::SenseEvil);
			uqm::gameState().setFlag(uqm::GameFlag::URQUAN_SENSES_EVIL, static_cast<uint32_t>(1));
		}

		GrpOffs = uqm::gameState().flag(uqm::GameFlag::COLONY_GRPOFFS);
		if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
			&& uqm::gameState().game().BattleGroupRef
			&& uqm::gameState().game().BattleGroupRef == GrpOffs)
		{
			NPCPhrase(UrquanPhrase::CaughtYa);

			setSegue(Segue::Hostile);
			return;
		}

		GrpOffs = uqm::gameState().flag(uqm::GameFlag::SAMATRA_GRPOFFS);
		if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
			&& uqm::gameState().game().BattleGroupRef
			&& uqm::gameState().game().BattleGroupRef == GrpOffs)
		{
			NPCPhrase(UrquanPhrase::HelloSamatra);

			uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
			setSegue(Segue::Hostile);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::URQUAN_VISITS);
			if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(UrquanPhrase::InitHello);
						break;
					case 1:
						NPCPhrase(UrquanPhrase::SubsequentHello1);
						break;
					case 2:
						NPCPhrase(UrquanPhrase::SubsequentHello2);
						break;
					case 3:
						NPCPhrase(UrquanPhrase::SubsequentHello3);
						break;
					case 4:
						NPCPhrase(UrquanPhrase::SubsequentHello4);
						--NumVisits;
						break;
				}

				NormalUrquan((ResponseRef)0);
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(UrquanPhrase::InitFleeHuman);
						LoserUrquan((ResponseRef)0);
						break;
					case 1:
						NPCPhrase(UrquanPhrase::SubsequentFleeHuman);
						setSegue(Segue::Peace);
						--NumVisits;
						break;
				}
			}
			uqm::gameState().setFlag(uqm::GameFlag::URQUAN_VISITS, static_cast<uint32_t>(NumVisits));
		}
	}
}

static uint16_t uninit_urquan(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_urquan_enc(void)
{
	uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HYPNOTIZED, static_cast<uint32_t>(0));
}

ConversationData* init_urquan_comm(void)
{
	urquan_desc.init_encounter_func = Intro;
	urquan_desc.post_encounter_func = post_urquan_enc;
	urquan_desc.uninit_encounter_func = uninit_urquan;

	uqm::lua().beginComm(nullptr, NullResource);

	urquan_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	urquan_desc.AlienTextBaseline.y = 0;
	urquan_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	const uint32_t GrpOffs {uqm::gameState().flag(uqm::GameFlag::URQUAN_PROBE_GRPOFFS)};

	// use alternate "Probe" track if available
	if ((uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		 && uqm::gameState().game().BattleGroupRef
		 && uqm::gameState().game().BattleGroupRef == GrpOffs))
	{
		altResFlags |= AltResFlags::Song;
	}

	if (IsExtended())
	{
		altResFlags |= AltResFlags::Colormap;
	}

	if (uqm::gameState().flag(uqm::GameFlag::PLAYER_HYPNOTIZED)
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle
		|| (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
			&& uqm::gameState().game().BattleGroupRef
			&& uqm::gameState().game().BattleGroupRef == GrpOffs))
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&urquan_desc};

	return (retval);
}

} // namespace uqm
