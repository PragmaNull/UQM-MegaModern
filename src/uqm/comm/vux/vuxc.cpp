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

namespace uqm
{
static ConversationData vux_desc =
	{
		Conversation::Vux, /* AlienConv */
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
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
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
				12,						/* StartIndex */
				3,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				15,						/* StartIndex */
				5,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				20,						  /* StartIndex */
				14,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 30,
				0, /* FrameRate */
				GameTicksPerSecond / 30,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				34,						  /* StartIndex */
				7,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				41,						/* StartIndex */
				6,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				47,						  /* StartIndex */
				11,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				58,						/* StartIndex */
				3,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				61,						/* StartIndex */
				4,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				65,						/* StartIndex */
				4,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				69,						/* StartIndex */
				2,						/* NumFrames */
				AnimationFlags::Random, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				71,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				74,					  /* StartIndex */
				6,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				80,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 14),				 /* BlockMask */
			},
								{
				85,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				90,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* RestartRate */
				(1 << 12),				 /* BlockMask */
			},
								{
				95,					  /* StartIndex */
				4,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 5,
				GameTicksPerSecond * 5, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				99,					  /* StartIndex */
				4,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond * 5,
				GameTicksPerSecond * 5, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				103,												 /* StartIndex */
				16,													 /* NumFrames */
				AnimationFlags::Circular | AnimationFlags::Disabled, /* AnimFlags */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* FrameRate */
				GameTicksPerSecond / 30,
				GameTicksPerSecond / 30, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				119, /* StartIndex */
				8,	 /* NumFrames */
				AnimationFlags::Circular | AnimationFlags::OneShot
					| AnimationFlags::WaitTalking | AnimationFlags::Disabled, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				0,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								},
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
			1,					  /* StartIndex */
			11,					  /* NumFrames */
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

static AnimFilterSet vux_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,							 /* Color index */
				1,							 /* Opacity index */
				-1,							 /* Frame index */
				DrawKind::Alpha,			 /* DrawKind*/
				FilterFlags::SwitchOffAnims, /* Flags */
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
			RunOneTimeSequence(18, AnimationFlags::None);
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

static void CombatIsInevitable(ResponseRef R)
{
	uint8_t NumVisits {};

	setSegue(Segue::Hostile);

	if (PlayerSaid(R, VuxPhrase::OkTakeBeast))
	{
		NPCPhrase(VuxPhrase::FoolAiee0);
		NPCPhrase(VuxPhrase::FoolAiee1);

		AlienTalkSegue(1);

		ZexBeingEatenAlive();

		AlienTalkSegue((uint16_t)~0);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_BEAST_ON_SHIP, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ZEX_IS_DEAD, static_cast<uint32_t>(1));
		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, VuxPhrase::TryAnyWay))
	{
		NPCPhrase(VuxPhrase::Nope);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(4));
	}
	else if (PlayerSaid(R, VuxPhrase::KillYouSquids1)
			 || PlayerSaid(R, VuxPhrase::KillYouSquids2)
			 || PlayerSaid(R, VuxPhrase::KillYouSquids3)
			 || PlayerSaid(R, VuxPhrase::KillYouSquids4))
	{
		NPCPhrase(VuxPhrase::WeFight);

		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_2) + 1;
		if (NumVisits <= 3)
		{
			uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_2, static_cast<uint32_t>(NumVisits));
		}
	}
	else if (PlayerSaid(R, VuxPhrase::CantWeBeFriends1)
			 || PlayerSaid(R, VuxPhrase::CantWeBeFriends2)
			 || PlayerSaid(R, VuxPhrase::CantWeBeFriends3)
			 || PlayerSaid(R, VuxPhrase::CantWeBeFriends4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_3);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(VuxPhrase::NeverUglyHumans1);
				break;
			case 1:
				NPCPhrase(VuxPhrase::NeverUglyHumans2);
				break;
			case 2:
				NPCPhrase(VuxPhrase::NeverUglyHumans3);
				break;
			case 3:
				NPCPhrase(VuxPhrase::NeverUglyHumans4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_3, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, VuxPhrase::ByeHostileSpace))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_4);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(VuxPhrase::GoodbyeAndDieHostileSpace1);
				break;
			case 1:
				NPCPhrase(VuxPhrase::GoodbyeAndDieHostileSpace2);
				break;
			case 2:
				NPCPhrase(VuxPhrase::GoodbyeAndDieHostileSpace3);
				break;
			case 3:
				NPCPhrase(VuxPhrase::GoodbyeAndDieHostileSpace4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_4, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, VuxPhrase::ByeZex))
	{
		NPCPhrase(VuxPhrase::GoodbyeZex);

		setSegue(Segue::Peace);
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_STACK_1);
		switch (NumVisits++)
		{
			case 4:
				NPCPhrase(VuxPhrase::NotAccepted1);
				break;
			case 5:
				NPCPhrase(VuxPhrase::NotAccepted2);
				break;
			case 6:
				NPCPhrase(VuxPhrase::NotAccepted3);
				break;
			case 7:
				NPCPhrase(VuxPhrase::NotAccepted4);
				break;
			case 8:
				NPCPhrase(VuxPhrase::NotAccepted5);
				break;
			case 9:
				NPCPhrase(VuxPhrase::NotAccepted6);
				break;
			case 10:
				NPCPhrase(VuxPhrase::NotAccepted7);
				break;
			case 11:
				NPCPhrase(VuxPhrase::NotAccepted8);
				break;
			case 12:
				NPCPhrase(VuxPhrase::NotAccepted9);
				break;
			case 13:
				NPCPhrase(VuxPhrase::Truth);
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(NumVisits));
	}
}

static void Menagerie(ResponseRef R)
{
	uint8_t i {};
	uint8_t LastStack {};
	VuxPhrase pStr[3];

	if (PlayerSaid(R, VuxPhrase::IHaveBeast)
		|| PlayerSaid(R, VuxPhrase::WhyTrust1)
		|| PlayerSaid(R, VuxPhrase::WhyTrust2)
		|| PlayerSaid(R, VuxPhrase::WhyTrust3))
	{
		if (PlayerSaid(R, VuxPhrase::IHaveBeast))
		{
			NPCPhrase(VuxPhrase::GiveBeast);
		}
		else if (PlayerSaid(R, VuxPhrase::WhyTrust1))
		{
			NPCPhrase(VuxPhrase::Trust1);

			DisablePhrase(VuxPhrase::WhyTrust1);
		}
		else if (PlayerSaid(R, VuxPhrase::WhyTrust2))
		{
			NPCPhrase(VuxPhrase::Trust2);

			DisablePhrase(VuxPhrase::WhyTrust2);
		}
		else if (PlayerSaid(R, VuxPhrase::WhyTrust3))
		{
			NPCPhrase(VuxPhrase::Trust3);

			DisablePhrase(VuxPhrase::WhyTrust3);
		}

		if (PhraseEnabled(VuxPhrase::WhyTrust1))
		{
			Response(VuxPhrase::WhyTrust1, Menagerie);
		}
		else if (PhraseEnabled(VuxPhrase::WhyTrust2))
		{
			Response(VuxPhrase::WhyTrust2, Menagerie);
		}
		else if (PhraseEnabled(VuxPhrase::WhyTrust3))
		{
			Response(VuxPhrase::WhyTrust3, Menagerie);
		}
		Response(VuxPhrase::OkTakeBeast, CombatIsInevitable);
	}
	else if (PlayerSaid(R, VuxPhrase::KillYou))
	{
		NPCPhrase(VuxPhrase::FightAgain);

		setSegue(Segue::Hostile);
	}
	else if (PlayerSaid(R, VuxPhrase::Regardless))
	{
		NPCPhrase(VuxPhrase::ThenFight);

		setSegue(Segue::Hostile);
		uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_3, static_cast<uint32_t>(2));
		uqm::gameState().setFlag(uqm::GameFlag::ZEX_VISITS, static_cast<uint32_t>(0));
	}
	else
	{
		LastStack = 0;
		pStr[0] = pStr[1] = pStr[2] = VuxPhrase::NullPhrase;
		if (R == 0)
		{
			uint8_t NumVisits {};

			NumVisits = uqm::gameState().flag(uqm::GameFlag::ZEX_VISITS);
			if (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_3) >= 2)
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(VuxPhrase::FightOrTrade1);
						break;
					case 1:
						NPCPhrase(VuxPhrase::FightOrTrade2);
						--NumVisits;
						break;
				}
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase(VuxPhrase::ZexHello1);
						break;
					case 1:
						NPCPhrase(VuxPhrase::ZexHello2);
						break;
					case 2:
						NPCPhrase(VuxPhrase::ZexHello3);
						break;
					case 3:
						NPCPhrase(VuxPhrase::ZexHello4);
						--NumVisits;
						break;
				}
			}
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_VISITS, static_cast<uint32_t>(NumVisits));
		}
		else if (PlayerSaid(R, VuxPhrase::WhatYouDoHere))
		{
			NPCPhrase(VuxPhrase::MyMenagerie);

			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_1, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, VuxPhrase::WhatAboutMenagerie))
		{
			NPCPhrase(VuxPhrase::NeedNewCreature);

			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_1, static_cast<uint32_t>(2));
		}
		else if (PlayerSaid(R, VuxPhrase::WhatAboutCreature))
		{
			NPCPhrase(VuxPhrase::AboutCreature);

			uqm::gameState().setFlag(uqm::GameFlag::KNOW_ZEX_WANTS_MONSTER, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_1, static_cast<uint32_t>(3));

			R = *VuxPhrase::AboutCreatureAgain;
			DisablePhrase(VuxPhrase::WhatAboutCreature);
		}
		else if (PlayerSaid(R, VuxPhrase::AboutCreatureAgain))
		{
			NPCPhrase(VuxPhrase::CreatureAgain);

			DisablePhrase(VuxPhrase::AboutCreatureAgain);
		}
		else if (PlayerSaid(R, VuxPhrase::WhyDontYouAttack))
		{
			NPCPhrase(VuxPhrase::LikeYou);

			LastStack = 1;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_2, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, VuxPhrase::WhyLikeMe))
		{
			NPCPhrase(VuxPhrase::LikeBecause);

			LastStack = 1;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_2, static_cast<uint32_t>(2));
		}
		else if (PlayerSaid(R, VuxPhrase::AreYouAPervert))
		{
			NPCPhrase(VuxPhrase::CallMeWhatYouWish);

			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_2, static_cast<uint32_t>(3));
		}
		else if (PlayerSaid(R, VuxPhrase::TakeByForce))
		{
			NPCPhrase(VuxPhrase::PrecursorDevice);

			LastStack = 2;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_3, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, VuxPhrase::YouLied))
		{
			NPCPhrase(VuxPhrase::YupLied);

			LastStack = 2;
			uqm::gameState().setFlag(uqm::GameFlag::ZEX_STACK_3, static_cast<uint32_t>(3));
		}

		if (uqm::gameState().flag(uqm::GameFlag::KNOW_ZEX_WANTS_MONSTER)
			&& uqm::gameState().flag(uqm::GameFlag::VUX_BEAST_ON_SHIP))
		{
			pStr[0] = VuxPhrase::IHaveBeast;
		}
		else
		{
			switch (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_1))
			{
				case 0:
					pStr[0] = VuxPhrase::WhatYouDoHere;
					break;
				case 1:
					pStr[0] = VuxPhrase::WhatAboutMenagerie;
					break;
				case 2:
					pStr[0] = VuxPhrase::WhatAboutCreature;
					break;
				case 3:
					if (PhraseEnabled(VuxPhrase::AboutCreatureAgain))
					{
						pStr[0] = VuxPhrase::AboutCreatureAgain;
					}
					break;
			}
		}
		switch (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_2))
		{
			case 0:
				pStr[1] = VuxPhrase::WhyDontYouAttack;
				break;
			case 1:
				pStr[1] = VuxPhrase::WhyLikeMe;
				break;
			case 2:
				pStr[1] = VuxPhrase::AreYouAPervert;
				break;
		}
		switch (uqm::gameState().flag(uqm::GameFlag::ZEX_STACK_3))
		{
			case 0:
				pStr[2] = VuxPhrase::TakeByForce;
				break;
			case 1:
				pStr[2] = VuxPhrase::Regardless;
				break;
			case 2:
				pStr[2] = VuxPhrase::YouLied;
				break;
			case 3:
				pStr[2] = VuxPhrase::KillYou;
				break;
		}

		if (pStr[LastStack] != VuxPhrase::NullPhrase)
		{
			Response(pStr[LastStack], Menagerie);
		}
		for (i = 0; i < 3; ++i)
		{
			if (i != LastStack && *pStr[i] != VuxPhrase::NullPhrase)
			{
				Response(pStr[i], Menagerie);
			}
		}
		Response(VuxPhrase::ByeZex, CombatIsInevitable);
	}
}

static void NormalVux(ResponseRef R)
{
	if (PlayerSaid(R, VuxPhrase::WhySoMean))
	{
		NPCPhrase(VuxPhrase::UrquanSlaves);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, VuxPhrase::DeeperReason))
	{
		NPCPhrase(VuxPhrase::OldInsult);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, VuxPhrase::IfWeApologize))
	{
		NPCPhrase(VuxPhrase::ProbablyNot);

		uqm::gameState().setFlag(uqm::GameFlag::VUX_STACK_1, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, VuxPhrase::WhatsUpHostile))
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(VuxPhrase::GeneralInfoHostile1);
				break;
			case 1:
				NPCPhrase(VuxPhrase::GeneralInfoHostile2);
				break;
			case 2:
				NPCPhrase(VuxPhrase::GeneralInfoHostile3);
				break;
			case 3:
				NPCPhrase(VuxPhrase::GeneralInfoHostile4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::VUX_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(VuxPhrase::WhatsUpHostile);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_1))
	{
		case 0:
			Response(VuxPhrase::WhySoMean, NormalVux);
			break;
		case 1:
			Response(VuxPhrase::DeeperReason, NormalVux);
			break;
		case 2:
			Response(VuxPhrase::IfWeApologize, NormalVux);
			break;
		case 3:
			Response(VuxPhrase::TryAnyWay, CombatIsInevitable);
			break;
		case 4:
			Response(VuxPhrase::Apology1, CombatIsInevitable);
			break;
		case 5:
			Response(VuxPhrase::Apology2, CombatIsInevitable);
			break;
		case 6:
			Response(VuxPhrase::Apology3, CombatIsInevitable);
			break;
		case 7:
			Response(VuxPhrase::Apology4, CombatIsInevitable);
			break;
		case 8:
			Response(VuxPhrase::Apology5, CombatIsInevitable);
			break;
		case 9:
			Response(VuxPhrase::Apology6, CombatIsInevitable);
			break;
		case 10:
			Response(VuxPhrase::Apology7, CombatIsInevitable);
			break;
		case 11:
			Response(VuxPhrase::Apology8, CombatIsInevitable);
			break;
		case 12:
			Response(VuxPhrase::Apology9, CombatIsInevitable);
			break;
		case 13:
			Response(VuxPhrase::Apology10, CombatIsInevitable);
			break;
	}

	switch (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_2))
	{
		case 0:
			Response(VuxPhrase::KillYouSquids1, CombatIsInevitable);
			break;
		case 1:
			Response(VuxPhrase::KillYouSquids2, CombatIsInevitable);
			break;
		case 2:
			Response(VuxPhrase::KillYouSquids3, CombatIsInevitable);
			break;
		case 3:
			Response(VuxPhrase::KillYouSquids4, CombatIsInevitable);
			break;
	}

	if (PhraseEnabled(VuxPhrase::WhatsUpHostile))
	{
		Response(VuxPhrase::WhatsUpHostile, NormalVux);
	}

	if (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_1) > 13)
	{
		switch (uqm::gameState().flag(uqm::GameFlag::VUX_STACK_3))
		{
			case 0:
				Response(VuxPhrase::CantWeBeFriends1, CombatIsInevitable);
				break;
			case 1:
				Response(VuxPhrase::CantWeBeFriends2, CombatIsInevitable);
				break;
			case 2:
				Response(VuxPhrase::CantWeBeFriends3, CombatIsInevitable);
				break;
			case 3:
				Response(VuxPhrase::CantWeBeFriends4, CombatIsInevitable);
				break;
		}
	}

	Response(VuxPhrase::ByeHostileSpace, CombatIsInevitable);
}

static void Intro(void)
{
	if (altResFlags & UseAltFrame)
	{
		CommData.AlienAmbientArray[17].AnimFlags &= ~AnimationFlags::Disabled;
	}

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(VuxPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		Menagerie((ResponseRef)0);
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
					NPCPhrase(VuxPhrase::HomeworldHello1);
					break;
				case 1:
					NPCPhrase(VuxPhrase::HomeworldHello2);
					break;
				case 2:
					NPCPhrase(VuxPhrase::HomeworldHello3);
					break;
				case 3:
					NPCPhrase(VuxPhrase::HomeworldHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::VUX_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			SetHomeworldKnown(*HomeId::Vux);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::VUX_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(VuxPhrase::SpaceHello1);
					break;
				case 1:
					NPCPhrase(VuxPhrase::SpaceHello2);
					break;
				case 2:
					NPCPhrase(VuxPhrase::SpaceHello3);
					break;
				case 3:
					NPCPhrase(VuxPhrase::SpaceHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::VUX_VISITS, static_cast<uint32_t>(NumVisits));
		}

		NormalVux((ResponseRef)0);
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

ConversationData* init_vux_comm(void)
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
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&vux_desc};

	return (retval);
}

} // namespace uqm
