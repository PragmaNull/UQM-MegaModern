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
#include "uqm/build.h"
#include "uqm/battle.h"
#include "uqm/sounds.h"

namespace uqm
{
static constexpr auto StrobeRate {30};
static constexpr auto StrobeLength {GameTicksPerSecond / 3};
static constexpr auto NumStrobes {StrobeLength * StrobeRate / GameTicksPerSecond};

static ConversationData talkpet_desc =
	{
		Conversation::TalkingPet, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		TalkingPetPmapAnim, /* AlienFrame */
		TalkingPetFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		TalkingPetColorMap, /* AlienColorMap */
		TalkingPetMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		TalkingPetConversationPhrases, /* PlayerPhrases */
		18, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				7, /* StartIndex */
				3, /* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				10, /* StartIndex */
				3,	/* NumFrames */
				YoyoAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				13,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				16,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				GameTicksPerSecond / 15, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Blink right eye */
				18,											/* StartIndex */
				3,											/* NumFrames */
				YoyoAnim,									/* AnimFlags */
				GameTicksPerSecond / 20, 0,					/* FrameRate */
				GameTicksPerSecond, GameTicksPerSecond * 3, /* RestartRate */
				(1 << 13)									/* BlockMask */
			},
								{
				/* Blink left eye */
				21,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 12) | (1 << 14),	/* BlockMask */
			},
								{
				24,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				26,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				28,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				32,			/* StartIndex */
				3,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				35,			/* StartIndex */
				5,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				40,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				42, /* StartIndex */
				6,	/* NumFrames */
				CircularAnim
					| WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
								{
				/* Right eyebrow */
				48,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4),				/* BlockMask */
			},
								{
				/* Left eyebrow */
				50,			/* StartIndex */
				2,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),				/* BlockMask */
			},
								{
				52,						  /* StartIndex */
				2,						  /* NumFrames */
				RandomAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Mind control strobe (on-demand) */
				1,				/* StartIndex */
				NumStrobes * 2, /* NumFrames */
				CircularAnim | ColorxformAnim | OneShotAnim
					| AnimDisabled, /* AnimFlags */
				GameTicksPerSecond / (StrobeRate * 2),
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
			1, /* StartIndex */
			6, /* NumFrames */
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

static AnimFilterSet talkpet_filters =
	{
		1, /* Number of filters */
		{
								  /* Filter array */
			{
				0,				   /* Color index */
				1,				   /* Opacity index */
				-1,				   /* Frame index */
				DrawKind::Overlay, /* DrawKind*/
				0,				   /* Flags */
			},
								  }
};

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue::Peace);
	uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(0));

	if (PlayerSaid(R, TalkingPetPhrase::CompelUrquan))
	{
		NPCPhrase(TalkingPetPhrase::HereWeGo);

		uqm::gameState().setFlag(uqm::GameFlag::URQUAN_MESSED_UP, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, TalkingPetPhrase::WishToGoNow))
	{
		NPCPhrase(TalkingPetPhrase::ExcellentIdea);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::ByeOnboard))
	{
		NPCPhrase(TalkingPetPhrase::GoodbyeOnboard);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::CompelThatShip))
	{
		NPCPhrase(TalkingPetPhrase::SavingMyPower);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::OkLetsDoIt)
			 || PlayerSaid(R, TalkingPetPhrase::WantKill1)
			 || PlayerSaid(R, TalkingPetPhrase::WantKill2)
			 || PlayerSaid(R, TalkingPetPhrase::WantKill3))
	{
		if (PlayerSaid(R, TalkingPetPhrase::OkLetsDoIt))
		{
			NPCPhrase(TalkingPetPhrase::ComingAboard);
		}
		else
		{
			NPCPhrase(TalkingPetPhrase::GladYouWontKill);
		}

		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_ON_SHIP, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::ARILOU_STACK_2, static_cast<uint32_t>(0));
	}
}

static void MindFuckUrquan(RESPONSE_REF R)
{
	if (PlayerSaid(R, TalkingPetPhrase::WhatNow))
	{
		NPCPhrase(TalkingPetPhrase::DoThis);

		DisablePhrase(TalkingPetPhrase::WhatNow);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::ImScared))
	{
		NPCPhrase(TalkingPetPhrase::StupidFop);

		DisablePhrase(TalkingPetPhrase::ImScared);
	}

	if (PhraseEnabled(TalkingPetPhrase::WhatNow))
	{
		Response(TalkingPetPhrase::WhatNow, MindFuckUrquan);
	}
	if (PhraseEnabled(TalkingPetPhrase::ImScared))
	{
		Response(TalkingPetPhrase::ImScared, MindFuckUrquan);
	}
	Response(TalkingPetPhrase::CompelUrquan, ExitConversation);
}

static void PetDevice(RESPONSE_REF R);

static void MindControlStrobe(void)
{
	// Enable the one-shot strobe animation
	CommData.AlienAmbientArray[16].AnimFlags &= ~AnimDisabled;

	if (IsHd())
	{
		EngageFilters(&talkpet_filters);
	}
}

static void MindControl(RESPONSE_REF R)
{
	RESPONSE_FUNC RespFunc {};

	if (PlayerSaid(R, TalkingPetPhrase::WhatAboutPowers))
	{
		NPCPhrase(TalkingPetPhrase::NotPowersButFlowers);

		RespFunc = (RESPONSE_FUNC)MindControl;
		R = *TalkingPetPhrase::YesFlowers;
	}
	else /* if (R == TalkingPetPhrase::YesFlowers) */
	{
		NPCPhrase(TalkingPetPhrase::GoodHuman);

		RespFunc = (RESPONSE_FUNC)ExitConversation;
		R = *TalkingPetPhrase::WishToGoNow;
	}

	AlienTalkSegue((uint16_t)~0);
	MindControlStrobe();

	Response(R, RespFunc);
}

static void PetInfo(RESPONSE_REF R)
{
	if (PlayerSaid(R, TalkingPetPhrase::AboutYourRace))
	{
		NPCPhrase(TalkingPetPhrase::WhatAboutRace);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::WhatAboutPhysiology))
	{
		NPCPhrase(TalkingPetPhrase::NoTalkAboutSelf);

		DisablePhrase(TalkingPetPhrase::WhatAboutPhysiology);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::WhatAboutYourHistory))
	{
		NPCPhrase(TalkingPetPhrase::AboutHistory);

		DisablePhrase(TalkingPetPhrase::WhatAboutYourHistory);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::SentientMilieu))
	{
		NPCPhrase(TalkingPetPhrase::AboutSentientMilieu);

		DisablePhrase(TalkingPetPhrase::SentientMilieu);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::WhatAboutWar))
	{
		NPCPhrase(TalkingPetPhrase::AboutWar);

		DisablePhrase(TalkingPetPhrase::WhatAboutWar);
	}

	if (PhraseEnabled(TalkingPetPhrase::WhatAboutPhysiology))
	{
		Response(TalkingPetPhrase::WhatAboutPhysiology, PetInfo);
	}
	else
	{
		Response(TalkingPetPhrase::WhatAboutPowers, MindControl);
	}
	if (PhraseEnabled(TalkingPetPhrase::WhatAboutYourHistory))
	{
		Response(TalkingPetPhrase::WhatAboutYourHistory, PetInfo);
	}
	else if (PhraseEnabled(TalkingPetPhrase::SentientMilieu))
	{
		Response(TalkingPetPhrase::SentientMilieu, PetInfo);
	}
	else if (PhraseEnabled(TalkingPetPhrase::WhatAboutWar))
	{
		Response(TalkingPetPhrase::WhatAboutWar, PetInfo);
	}
	Response(TalkingPetPhrase::EnoughInfo, PetDevice);
}

static void PetDevice(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	if (PlayerSaid(R, TalkingPetPhrase::WhatsUpOnboard))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard1);
				break;
			case 1:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard2);
				break;
			case 2:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard3);
				break;
			case 3:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard4);
				break;
			case 4:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard5);
				break;
			case 5:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard6);
				break;
			case 6:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard7);
				break;
			case 7:
				NPCPhrase(TalkingPetPhrase::GeneralInfoOnboard8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_INFO, static_cast<uint32_t>(NumVisits));

		DisablePhrase(TalkingPetPhrase::WhatsUpOnboard);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::AnySuggestions))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_SUGGESTIONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(TalkingPetPhrase::Suggestion1);
				break;
			case 1:
				NPCPhrase(TalkingPetPhrase::Suggestion2);
				break;
			case 2:
				NPCPhrase(TalkingPetPhrase::Suggestion3);
				break;
			case 3:
				NPCPhrase(TalkingPetPhrase::Suggestion4);
				break;
			case 4:
				NPCPhrase(TalkingPetPhrase::Suggestion5);
				break;
			case 5:
				NPCPhrase(TalkingPetPhrase::Suggestion6);
				break;
			case 6:
				NPCPhrase(TalkingPetPhrase::Suggestion7);
				break;
			case 7:
				NPCPhrase(TalkingPetPhrase::Suggestion8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_SUGGESTIONS, static_cast<uint32_t>(NumVisits));

		DisablePhrase(TalkingPetPhrase::AnySuggestions);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::EnoughInfo))
	{
		NPCPhrase(TalkingPetPhrase::OkEnoughInfo);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::YouLied))
	{
		NPCPhrase(TalkingPetPhrase::SoWhat);

		uqm::gameState().setFlag(uqm::GameFlag::DNYARRI_LIED, static_cast<uint32_t>(0));
	}

	if (uqm::gameState().flag(uqm::GameFlag::SHIP_TO_COMPEL))
	{
		Response(TalkingPetPhrase::CompelThatShip, ExitConversation);
	}
	if (PhraseEnabled(TalkingPetPhrase::WhatsUpOnboard))
	{
		Response(TalkingPetPhrase::WhatsUpOnboard, PetDevice);
	}
	if (PhraseEnabled(TalkingPetPhrase::AnySuggestions))
	{
		Response(TalkingPetPhrase::AnySuggestions, PetDevice);
	}
	Response(TalkingPetPhrase::AboutYourRace, PetInfo);
	if (uqm::gameState().flag(uqm::GameFlag::DNYARRI_LIED) && uqm::gameState().flag(uqm::GameFlag::LEARNED_TALKING_PET))
	{
		Response(TalkingPetPhrase::YouLied, PetDevice);
	}
	Response(TalkingPetPhrase::ByeOnboard, ExitConversation);
}

static void CompelPlayer(RESPONSE_REF R)
{
	TalkingPetPhrase pStr[3];

	uint8_t LastStack {0};
	pStr[0] = pStr[1] = pStr[2] = TalkingPetPhrase::NullPhrase;
	if (PlayerSaid(R, TalkingPetPhrase::WhatAreYou))
	{
		NPCPhrase(TalkingPetPhrase::JustTalkingPet);

		DisablePhrase(TalkingPetPhrase::WhatAreYou);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::WhatDoToUmgah))
	{
		NPCPhrase(TalkingPetPhrase::DidNothing);

		DisablePhrase(TalkingPetPhrase::WhatDoToUmgah);
		LastStack = 1;
	}
	else if (PlayerSaid(R, TalkingPetPhrase::WeAreVindicator))
	{
		NPCPhrase(TalkingPetPhrase::GoodForYou);

		DisablePhrase(TalkingPetPhrase::WeAreVindicator);
		LastStack = 2;
	}
	else if (R != 0)
	{
		if (PlayerSaid(R, TalkingPetPhrase::ByeAtUmgah))
		{
			NPCPhrase(TalkingPetPhrase::GoodbyeAtUmgah);
		}
		else if (PlayerSaid(R, TalkingPetPhrase::MustExplainPresence))
		{
			NPCPhrase(TalkingPetPhrase::ExplainNothingMonkeyBoy);
		}
		else if (PlayerSaid(R, TalkingPetPhrase::UmgahZombies))
		{
			NPCPhrase(TalkingPetPhrase::WorksLikeThis);
		}
		else if (PlayerSaid(R, TalkingPetPhrase::TalkingPetsDumb))
		{
			NPCPhrase(TalkingPetPhrase::OhNoYouDont);
		}

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES, static_cast<uint32_t>(1));
		if (!uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HYPNOTIZED, static_cast<uint32_t>(1));
		}
		else
		{
			NPCPhrase(TalkingPetPhrase::CantCompel);

			setSegue(Segue::Hostile);
		}

		return;
	}

	if (PhraseEnabled(TalkingPetPhrase::WhatAreYou))
	{
		pStr[0] = TalkingPetPhrase::WhatAreYou;
	}
	else
	{
		pStr[0] = TalkingPetPhrase::TalkingPetsDumb;
	}
	if (uqm::gameState().flag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES))
	{
		if (PhraseEnabled(TalkingPetPhrase::WhatDoToUmgah))
		{
			pStr[1] = TalkingPetPhrase::WhatDoToUmgah;
		}
		else
		{
			pStr[1] = TalkingPetPhrase::UmgahZombies;
		}
	}
	if (PhraseEnabled(TalkingPetPhrase::WeAreVindicator))
	{
		pStr[2] = TalkingPetPhrase::WeAreVindicator;
	}
	else
	{
		pStr[2] = TalkingPetPhrase::MustExplainPresence;
	}

	if (pStr[LastStack] != TalkingPetPhrase::NullPhrase)
	{
		Response(pStr[LastStack], CompelPlayer);
	}
	for (uint8_t i {0}; i < 3; ++i)
	{
		if (i != LastStack && *pStr[i] != TalkingPetPhrase::NullPhrase)
		{
			Response(pStr[i], CompelPlayer);
		}
	}
	Response(TalkingPetPhrase::ByeAtUmgah, CompelPlayer);
}

static void PetDeal(RESPONSE_REF R);

static void KillPet(RESPONSE_REF R)
{
	if (PlayerSaid(R, TalkingPetPhrase::MustKill))
	{
		NPCPhrase(TalkingPetPhrase::DontKill);
		AlienTalkSegue((uint16_t)~0);

		MindControlStrobe();
	}

	Response(TalkingPetPhrase::WantKill1, ExitConversation);
	Response(TalkingPetPhrase::WantKill2, ExitConversation);
	Response(TalkingPetPhrase::WantKill3, ExitConversation);
}

static void PetDeal(RESPONSE_REF R)
{
	if (PlayerSaid(R, TalkingPetPhrase::WhatKindOfDeal))
	{
		NPCPhrase(TalkingPetPhrase::HelpDefeatUrquan);

		DisablePhrase(TalkingPetPhrase::WhatKindOfDeal);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::HowTrust))
	{
		NPCPhrase(TalkingPetPhrase::Trust);

		DisablePhrase(TalkingPetPhrase::HowTrust);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::BonelessDweeb))
	{
		NPCPhrase(TalkingPetPhrase::YourBonelessDweeb);

		DisablePhrase(TalkingPetPhrase::BonelessDweeb);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::WhatAreYouReally))
	{
		NPCPhrase(TalkingPetPhrase::PoorDnyarri);

		DisablePhrase(TalkingPetPhrase::WhatAreYouReally);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::HardToBelieve))
	{
		NPCPhrase(TalkingPetPhrase::ItsTrue);

		uqm::gameState().setFlag(uqm::GameFlag::DNYARRI_LIED, static_cast<uint32_t>(1));
		DisablePhrase(TalkingPetPhrase::HardToBelieve);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::Bullshit))
	{
		NPCPhrase(TalkingPetPhrase::WorthATry);

		DisablePhrase(TalkingPetPhrase::Bullshit);
	}
	else if (PlayerSaid(R, TalkingPetPhrase::KillYou))
	{
		NPCPhrase(TalkingPetPhrase::PleaseDont);

		DisablePhrase(TalkingPetPhrase::KillYou);
	}

	if (PhraseEnabled(TalkingPetPhrase::WhatKindOfDeal))
	{
		Response(TalkingPetPhrase::WhatKindOfDeal, PetDeal);
	}
	else
	{
		if (PhraseEnabled(TalkingPetPhrase::HowTrust))
		{
			Response(TalkingPetPhrase::HowTrust, PetDeal);
		}
		else if (PhraseEnabled(TalkingPetPhrase::BonelessDweeb))
		{
			Response(TalkingPetPhrase::BonelessDweeb, PetDeal);
		}
		Response(TalkingPetPhrase::OkLetsDoIt, ExitConversation);
	}
	if (PhraseEnabled(TalkingPetPhrase::WhatAreYouReally))
	{
		Response(TalkingPetPhrase::WhatAreYouReally, PetDeal);
	}
	else
	{
		if (PhraseEnabled(TalkingPetPhrase::HardToBelieve) && !uqm::gameState().flag(uqm::GameFlag::LEARNED_TALKING_PET))
		{
			Response(TalkingPetPhrase::HardToBelieve, PetDeal);
		}
		else if (PhraseEnabled(TalkingPetPhrase::Bullshit) && uqm::gameState().flag(uqm::GameFlag::LEARNED_TALKING_PET))
		{
			Response(TalkingPetPhrase::Bullshit, PetDeal);
		}
	}
	if (PhraseEnabled(TalkingPetPhrase::KillYou))
	{
		Response(TalkingPetPhrase::KillYou, PetDeal);
	}
	else if (PhraseEnabled(TalkingPetPhrase::MustKill))
	{
		Response(TalkingPetPhrase::MustKill, KillPet);
	}
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(TalkingPetPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InLastBattle)
	{
		uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(0));
		setSegue(Segue::Hostile);
		if (!(uqm::globalFlags() & CyborgEnabled))
		{
			NPCPhrase(TalkingPetPhrase::HumanPepTalk);
		}
		else
		{
			NPCPhrase(TalkingPetPhrase::CyborgPepTalk);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::READY_TO_CONFUSE_URQUAN))
	{
		uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::READY_TO_CONFUSE_URQUAN, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) != 3)
		{
			NPCPhrase(TalkingPetPhrase::HaventGotEverything);
			if (!uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
			{
				NPCPhrase(TalkingPetPhrase::NeedBomb);
			}
			else
			{
				NPCPhrase(TalkingPetPhrase::SoupUpBomb);
			}

			setSegue(Segue::Peace);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::URQUAN_MESSED_UP))
		{
			NPCPhrase(TalkingPetPhrase::HelloAfterCompelUrquan);

			setSegue(Segue::Peace);
		}
		else
		{
			NPCPhrase(TalkingPetPhrase::ISenseMySlaves);

			MindFuckUrquan((RESPONSE_REF)0);
		}
	}
	else if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_VISITS);

		// You can acquire the Talking Pet without having Taalo Shield after
		// the Kohr-Ah wipe out the Umgah. In that case, the Pet will join
		// you willingly, but his complaints about the Taalo shield as in
		// TalkingPetPhrase::HelloAsDevice1 do not make any sense.
		if (!uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP) && NumVisits == 0)
		{
			++NumVisits; // skip TalkingPetPhrase::HelloAsDevice1
		}

		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice1);
				break;
			case 1:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice2);
				break;
			case 2:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice3);
				break;
			case 3:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice4);
				break;
			case 4:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice5);
				break;
			case 5:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice6);
				break;
			case 6:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice7);
				break;
			case 7:
				NPCPhrase(TalkingPetPhrase::HelloAsDevice8);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_VISITS, static_cast<uint32_t>(NumVisits));

		PetDevice((RESPONSE_REF)0);
	}
	else if (GetHeadLink(&uqm::gameState().game().npc_built_ship_q))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(0));
				NPCPhrase(TalkingPetPhrase::HelloAtUmgah);
				break;
			case 1:
				NPCPhrase(TalkingPetPhrase::HypnotizeAgain1);
				break;
			case 2:
				NPCPhrase(TalkingPetPhrase::HypnotizeAgain2);
				break;
			case 3:
				NPCPhrase(TalkingPetPhrase::HypnotizeAgain3);
				break;
			case 4:
				NPCPhrase(TalkingPetPhrase::HypnotizeAgain4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::TALKING_PET_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		SetHomeworldKnown(*HomeId::Umgah);
		if (NumVisits == 1)
		{
			CompelPlayer((RESPONSE_REF)0);
		}
		else if (!uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP))
		{
			uqm::gameState().setFlag(uqm::GameFlag::PLAYER_HYPNOTIZED, static_cast<uint32_t>(1));
			setSegue(Segue::Peace);
		}
		else
		{
			NPCPhrase(TalkingPetPhrase::CantCompel);

			setSegue(Segue::Hostile);
		}
	}
	else
	{
		if (StartSphereTracking(RaceId::Umgah))
		{
			NPCPhrase(TalkingPetPhrase::LetsMakeADeal);
		}
		else
		{
			NPCPhrase(TalkingPetPhrase::UmgahAllGone);

			if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_HOME_VISITS) == 0
				|| !uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP))
			{ // The TalkingPetPhrase::HowTrust-TalkingPetPhrase::Trust exchange only makes sense when the
				// player visited the Talking Pet before so he tried to
				// kill the player *and* the player has the Taalo shield.
				DisablePhrase(TalkingPetPhrase::HowTrust);
			}
		}

		PetDeal((RESPONSE_REF)0);
	}
}

static uint16_t uninit_talkpet(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_talkpet_enc(void)
{
	// nothing defined so far
}

ConversationData* init_talkpet_comm(void)
{
	talkpet_desc.init_encounter_func = Intro;
	talkpet_desc.post_encounter_func = post_talkpet_enc;
	talkpet_desc.uninit_encounter_func = uninit_talkpet;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	talkpet_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	talkpet_desc.AlienTextBaseline.y = 0;
	talkpet_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::InLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}

	ConversationData* retval {&talkpet_desc};

	return (retval);
}

} // namespace uqm
