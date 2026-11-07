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
#include "uqm/lua/UQMLua.h"


namespace uqm
{
static ConversationData umgah_desc =
	{
		Conversation::Umgah, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		UmgahPmapAnim, /* AlienFrame */
		UmgahFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		UmgahColorMap, /* AlienColorMap */
		UmgahMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		UmgahConversationPhrases, /* PlayerPhrases */
		16, /* NumAnimations */
		{{
			/* AlienAmbientArray (ambient animations) */
			{
				5,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 5),
			},
			{
				8,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 6),
			},
			{
				11,					  /* StartIndex */
				2,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 7),
			},
			{
				13,					  /* StartIndex */
				2,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 8),
			},
			{
				15,					  /* StartIndex */
				2,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 9),
			},
			{
				17,						  /* StartIndex */
				3,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond * 7 / 60,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 0),
			},
			{
				20,						  /* StartIndex */
				3,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond * 7 / 60,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 1),
			},
			{
				23,						  /* StartIndex */
				2,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond * 7 / 60,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 2),
			},
			{
				25,						  /* StartIndex */
				2,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond * 7 / 60,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 3),
			},
			{
				27,						  /* StartIndex */
				2,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond * 7 / 60,
				0, /* FrameRate */
				GameTicksPerSecond * 3,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 4),
			},
			{
				29,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				32,					  /* StartIndex */
				3,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				35,					  /* StartIndex */
				5,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
			{
				40,					  /* StartIndex */
				6,					  /* NumFrames */
				AnimationFlags::Yoyo, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				0, /* RestartRate */
				0, /* BlockMask */
			},
			{
				46,						  /* StartIndex */
				2,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 5,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 15),				/* BlockMask */
			},
			{
				48,						  /* StartIndex */
				2,						  /* NumFrames */
				AnimationFlags::Circular, /* AnimFlags */
				GameTicksPerSecond / 5,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				(1 << 14),				/* BlockMask */
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
			1,					  /* StartIndex */
			4,					  /* NumFrames */
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

	if (PlayerSaid(R, UmgahPhrase::ByeZombie))
	{
		NPCPhrase(UmgahPhrase::GoodbyeZombie);

		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, UmgahPhrase::ByePreZombie))
	{
		NPCPhrase(UmgahPhrase::GoodbyePreZombie);
	}
	else if (PlayerSaid(R, UmgahPhrase::CanWeBeFriends))
	{
		NPCPhrase(UmgahPhrase::SureFriends);

		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_MENTIONED_TRICKS, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UmgahPhrase::EvilBlobbiesGiveUp))
	{
		NPCPhrase(UmgahPhrase::NotEvilBlobbies);

		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_EVIL_BLOBBIES, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, UmgahPhrase::EvilBlobbiesMustDie))
	{
		NPCPhrase(UmgahPhrase::OhNoWeWont);
	}
	else if (PlayerSaid(R, UmgahPhrase::Threat))
	{
		NPCPhrase(UmgahPhrase::NoThreat);
	}
	else if (PlayerSaid(R, UmgahPhrase::DontBelieve))
	{
		NPCPhrase(UmgahPhrase::ThenDie);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES, static_cast<uint32_t>(1));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(0));
	}
	else if (PlayerSaid(R, UmgahPhrase::ByeUnknown))
	{
		NPCPhrase(UmgahPhrase::GoodbyeUnknown);

		setSegue(Segue::Peace);
	}
	else if (PlayerSaid(R, UmgahPhrase::ByePostZombie))
	{
		NPCPhrase(UmgahPhrase::FunnyIdea);

		AlienTalkSegue((uint16_t)~0);
		AddEscortShips(RaceId::Umgah, difficultyCase(4, 4, 2));
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_HOSTILE, static_cast<uint32_t>(1));
	}
}

static void Zombies(ResponseRef R)
{
	if (uqm::gameState().flag(uqm::GameFlag::MET_NORMAL_UMGAH))
	{
		if (PlayerSaid(R, UmgahPhrase::WhatsUpZombies))
		{
			NPCPhrase(UmgahPhrase::GeneralInfoZombie);

			DisablePhrase(UmgahPhrase::WhatsUpZombies);
		}
		else if (PlayerSaid(R, UmgahPhrase::HowGoesTpet))
		{
			NPCPhrase(UmgahPhrase::WhatTpet);

			DisablePhrase(UmgahPhrase::HowGoesTpet);
		}
		else if (PlayerSaid(R, UmgahPhrase::YouToldUs))
		{
			NPCPhrase(UmgahPhrase::SadlyItDied);

			DisablePhrase(UmgahPhrase::YouToldUs);
		}

		if (PhraseEnabled(UmgahPhrase::WhatsUpZombies) && PhraseEnabled(UmgahPhrase::HowGoesTpet))
		{
			Response(UmgahPhrase::WhatsUpZombies, Zombies);
		}
		if (PhraseEnabled(UmgahPhrase::HowGoesTpet))
		{
			Response(UmgahPhrase::HowGoesTpet, Zombies);
		}
		else if (PhraseEnabled(UmgahPhrase::YouToldUs))
		{
			Response(UmgahPhrase::YouToldUs, Zombies);
		}
		else
		{
			Response(UmgahPhrase::DontBelieve, CombatIsInevitable);
		}
		if (PhraseEnabled(UmgahPhrase::WhatsUpZombies) && !PhraseEnabled(UmgahPhrase::HowGoesTpet))
		{
			Response(UmgahPhrase::WhatsUpZombies, Zombies);
		}
		Response(UmgahPhrase::Threat, CombatIsInevitable);
		Response(UmgahPhrase::ByeUnknown, CombatIsInevitable);
	}
	else
	{
		uqstl::array<UmgahPhrase, 4> pStr;

		uint8_t LastStack {0};
		pStr[0] = pStr[1] = pStr[2] = pStr[3] = UmgahPhrase::NullPhrase;
		if (PlayerSaid(R, UmgahPhrase::EvilBlobbies))
		{
			NPCPhrase(UmgahPhrase::YesVeryEvil);

			DisablePhrase(UmgahPhrase::EvilBlobbies);
			LastStack = 0;
		}
		else if (PlayerSaid(R, UmgahPhrase::WeVindicator))
		{
			NPCPhrase(UmgahPhrase::GoodForYou1);

			DisablePhrase(UmgahPhrase::WeVindicator);
			LastStack = 1;
		}
		else if (PlayerSaid(R, UmgahPhrase::ComeInPeace))
		{
			NPCPhrase(UmgahPhrase::GoodForYou2);

			DisablePhrase(UmgahPhrase::ComeInPeace);
			LastStack = 1;
		}
		else if (PlayerSaid(R, UmgahPhrase::KnowAnyJokes))
		{
			NPCPhrase(UmgahPhrase::Joke1);

			DisablePhrase(UmgahPhrase::KnowAnyJokes);
			LastStack = 2;
		}
		else if (PlayerSaid(R, UmgahPhrase::BetterJoke))
		{
			NPCPhrase(UmgahPhrase::Joke2);

			DisablePhrase(UmgahPhrase::BetterJoke);
			LastStack = 2;
		}
		else if (PlayerSaid(R, UmgahPhrase::NotVeryFunny))
		{
			NPCPhrase(UmgahPhrase::YesWeAre);

			DisablePhrase(UmgahPhrase::NotVeryFunny);
			LastStack = 2;
		}
		else if (PlayerSaid(R, UmgahPhrase::WhatAboutTpet))
		{
			NPCPhrase(UmgahPhrase::WhatTpet);

			DisablePhrase(UmgahPhrase::WhatAboutTpet);
			LastStack = 3;
		}
		else if (PlayerSaid(R, UmgahPhrase::GiveUpOrDie))
		{
			NPCPhrase(UmgahPhrase::NotGiveUp);

			setSegue(Segue::Hostile);
			return;
		}
		else if (PlayerSaid(R, UmgahPhrase::ArilouToldUs))
		{
			NPCPhrase(UmgahPhrase::ThenDie);

			setSegue(Segue::Hostile);
			uqm::gameState().setFlag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES, static_cast<uint32_t>(1));
			uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(0));
			return;
		}

		if (PhraseEnabled(UmgahPhrase::EvilBlobbies))
		{
			pStr[0] = UmgahPhrase::EvilBlobbies;
		}
		else
		{
			pStr[0] = UmgahPhrase::GiveUpOrDie;
		}

		if (PhraseEnabled(UmgahPhrase::WeVindicator))
		{
			pStr[1] = UmgahPhrase::WeVindicator;
		}
		else if (PhraseEnabled(UmgahPhrase::ComeInPeace))
		{
			pStr[1] = UmgahPhrase::ComeInPeace;
		}

		if (PhraseEnabled(UmgahPhrase::KnowAnyJokes))
		{
			pStr[2] = UmgahPhrase::KnowAnyJokes;
		}
		else if (PhraseEnabled(UmgahPhrase::BetterJoke))
		{
			pStr[2] = UmgahPhrase::BetterJoke;
		}
		else if (PhraseEnabled(UmgahPhrase::NotVeryFunny))
		{
			pStr[2] = UmgahPhrase::NotVeryFunny;
		}

		if (PhraseEnabled(UmgahPhrase::WhatAboutTpet))
		{
			pStr[3] = UmgahPhrase::WhatAboutTpet;
		}
		else
		{
			pStr[3] = UmgahPhrase::ArilouToldUs;
		}

		if (pStr[LastStack] != UmgahPhrase::NullPhrase)
		{
			Response(pStr[LastStack], Zombies);
		}
		for (uint8_t i {0}; i < 4; ++i)
		{
			if (i != LastStack && *pStr[i] != UmgahPhrase::NullPhrase)
			{
				Response(pStr[i], Zombies);
			}
		}
		Response(UmgahPhrase::ByeZombie, CombatIsInevitable);
	}
}

static void NormalUmgah(ResponseRef R)
{
	if (PlayerSaid(R, UmgahPhrase::WhatsUpPreZombie))
	{
		NPCPhrase(UmgahPhrase::GeneralInfoPreZombie);

		DisablePhrase(UmgahPhrase::WhatsUpPreZombie);
	}
	else if (PlayerSaid(R, UmgahPhrase::WantToDefeatUrquan))
	{
		NPCPhrase(UmgahPhrase::FineByUs);

		DisablePhrase(UmgahPhrase::WantToDefeatUrquan);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::UMGAH_EVIL_BLOBBIES))
	{
		Response(UmgahPhrase::EvilBlobbiesGiveUp, CombatIsInevitable);
	}
	else
	{
		Response(UmgahPhrase::EvilBlobbiesMustDie, CombatIsInevitable);
	}
	if (PhraseEnabled(UmgahPhrase::WhatsUpPreZombie))
	{
		Response(UmgahPhrase::WhatsUpPreZombie, NormalUmgah);
	}
	if (PhraseEnabled(UmgahPhrase::WantToDefeatUrquan))
	{
		Response(UmgahPhrase::WantToDefeatUrquan, NormalUmgah);
	}
	switch (uqm::gameState().flag(uqm::GameFlag::UMGAH_MENTIONED_TRICKS))
	{
		case 0:
			Response(UmgahPhrase::CanWeBeFriends, CombatIsInevitable);
			break;
	}
	Response(UmgahPhrase::ByePreZombie, CombatIsInevitable);
}

static void UmgahReward(ResponseRef R)
{
	if (PlayerSaid(R, UmgahPhrase::WhatBeforeTpet))
	{
		NPCPhrase(UmgahPhrase::TrkdSpathiAndIlwrath);

		DisablePhrase(UmgahPhrase::WhatBeforeTpet);
	}
	else if (PlayerSaid(R, UmgahPhrase::WhereCaster))
	{
		NPCPhrase(UmgahPhrase::SpathiTookThem);

		DisablePhrase(UmgahPhrase::WhereCaster);
	}
	else if (PlayerSaid(R, UmgahPhrase::OweMeBigTime))
	{
		NPCPhrase(UmgahPhrase::Thanks);

		uqm::gameState().sis().TotalBioMass += 1000 / BioCreditValueDefault;
		DisablePhrase(UmgahPhrase::OweMeBigTime);
		DisablePhrase(UmgahPhrase::OurLargesse);
	}
	else if (PlayerSaid(R, UmgahPhrase::OurLargesse))
	{
		NPCPhrase(UmgahPhrase::GiveLifedata);

		uqm::gameState().sis().TotalBioMass += 1000 / BioCreditValueDefault;
		DisablePhrase(UmgahPhrase::OurLargesse);
		DisablePhrase(UmgahPhrase::OweMeBigTime);
	}
	else if (PlayerSaid(R, UmgahPhrase::WhatDoWithTpet))
	{
		NPCPhrase(UmgahPhrase::TrickUrquan);

		DisablePhrase(UmgahPhrase::WhatDoWithTpet);
	}
	else if (PlayerSaid(R, UmgahPhrase::AnyJokes))
	{
		NPCPhrase(UmgahPhrase::Sure);

		DisablePhrase(UmgahPhrase::AnyJokes);
	}
	else if (PlayerSaid(R, UmgahPhrase::SoWhatForNow))
	{
		NPCPhrase(UmgahPhrase::DoThisNow);

		DisablePhrase(UmgahPhrase::SoWhatForNow);
	}

	if (!uqm::gameState().flag(uqm::GameFlag::MET_NORMAL_UMGAH))
	{
		if (PhraseEnabled(UmgahPhrase::WhatBeforeTpet))
		{
			Response(UmgahPhrase::WhatBeforeTpet, UmgahReward);
		}
		else if (PhraseEnabled(UmgahPhrase::WhereCaster))
		{
			Response(UmgahPhrase::WhereCaster, UmgahReward);
		}
	}
	if (PhraseEnabled(UmgahPhrase::OweMeBigTime))
	{
		Response(UmgahPhrase::OweMeBigTime, UmgahReward);
		Response(UmgahPhrase::OurLargesse, UmgahReward);
	}
	if (PhraseEnabled(UmgahPhrase::WhatDoWithTpet))
	{
		Response(UmgahPhrase::WhatDoWithTpet, UmgahReward);
	}
	else if (PhraseEnabled(UmgahPhrase::AnyJokes) && uqm::gameState().flag(uqm::GameFlag::UMGAH_MENTIONED_TRICKS) < 2)
	{
		Response(UmgahPhrase::AnyJokes, UmgahReward);
	}
	if (PhraseEnabled(UmgahPhrase::SoWhatForNow))
	{
		Response(UmgahPhrase::SoWhatForNow, UmgahReward);
	}
	Response(UmgahPhrase::ByePostZombie, CombatIsInevitable);
}

static void Intro(void)
{
	uint8_t NumVisits {};


	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(UmgahPhrase::OutTakes);
		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_SEGUE, static_cast<uint32_t>(0));
		return;
	}
	else if (uqm::gameState().flag(uqm::GameFlag::UMGAH_HOSTILE))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::UMGAH_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(UmgahPhrase::HostileHello1);
				break;
			case 1:
				NPCPhrase(UmgahPhrase::HostileHello2);
				break;
			case 2:
				NPCPhrase(UmgahPhrase::HostileHello3);
				break;
			case 3:
				NPCPhrase(UmgahPhrase::HostileHello4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(NumVisits));

		setSegue(Segue::Hostile);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::UMGAH_VISITS);
		if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_VISITS))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UmgahPhrase::DestroyInterferer1);
					break;
				case 1:
					NPCPhrase(UmgahPhrase::DestroyInterferer2);
					break;
				case 2:
					NPCPhrase(UmgahPhrase::DestroyInterferer3);
					break;
				case 3:
					NPCPhrase(UmgahPhrase::DestroyInterferer4);
					--NumVisits;
					break;
			}

			setSegue(Segue::Hostile);
		}
		else if (uqm::gameState().flag(uqm::GameFlag::KNOW_UMGAH_ZOMBIES))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UmgahPhrase::RevealedZombieHello1);
					break;
				case 1:
					NPCPhrase(UmgahPhrase::RevealedZombieHello2);
					break;
				case 2:
					NPCPhrase(UmgahPhrase::RevealedZombieHello3);
					break;
				case 3:
					NPCPhrase(UmgahPhrase::RevealedZombieHello4);
					--NumVisits;
					break;
			}

			setSegue(Segue::Hostile);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UmgahPhrase::UnknownZombieHello1);
					break;
				case 1:
					NPCPhrase(UmgahPhrase::UnknownZombieHello2);
					break;
				case 2:
					NPCPhrase(UmgahPhrase::UnknownZombieHello3);
					break;
				case 3:
					NPCPhrase(UmgahPhrase::UnknownZombieHello4);
					--NumVisits;
					break;
			}

			Zombies((ResponseRef)0);
		}
		uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(NumVisits));
	}
	else if (!uqm::gameState().flag(uqm::GameFlag::TALKING_PET))
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UMGAH_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UmgahPhrase::HwldPreZombieHello1);
					break;
				case 1:
					NPCPhrase(UmgahPhrase::HwldPreZombieHello2);
					break;
				case 2:
					NPCPhrase(UmgahPhrase::HwldPreZombieHello3);
					break;
				case 3:
					NPCPhrase(UmgahPhrase::HwldPreZombieHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UMGAH_HOME_VISITS, static_cast<uint32_t>(NumVisits));

			SetHomeworldKnown(*HomeId::Umgah);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UMGAH_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UmgahPhrase::SpacePreZombieHello1);
					break;
				case 1:
					NPCPhrase(UmgahPhrase::SpacePreZombieHello2);
					break;
				case 2:
					NPCPhrase(UmgahPhrase::SpacePreZombieHello3);
					break;
				case 3:
					NPCPhrase(UmgahPhrase::SpacePreZombieHello4);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(NumVisits));
		}

		NormalUmgah((ResponseRef)0);
	}
	else
	{
		if (uqm::gameState().flag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NPCPhrase(UmgahPhrase::PostZombieHwldHello);

			SetHomeworldKnown(*HomeId::Umgah);
			UmgahReward((ResponseRef)0);
		}
		else
		{
			NumVisits = uqm::gameState().flag(uqm::GameFlag::UMGAH_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(UmgahPhrase::RewardAtHomeworld1);
					break;
				case 1:
					NPCPhrase(UmgahPhrase::RewardAtHomeworld2);
					--NumVisits;
					break;
			}
			uqm::gameState().setFlag(uqm::GameFlag::UMGAH_VISITS, static_cast<uint32_t>(NumVisits));

			setSegue(Segue::Peace);
		}
	}
}

static uint16_t uninit_umgah(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_umgah_enc(void)
{
	if (!uqm::gameState().flag(uqm::GameFlag::UMGAH_ZOMBIE_BLOBBIES))
	{
		uqm::gameState().setFlag(uqm::GameFlag::MET_NORMAL_UMGAH, static_cast<uint32_t>(1));
	}
}

ConversationData* init_umgah_comm(void)
{
	umgah_desc.init_encounter_func = Intro;
	umgah_desc.post_encounter_func = post_umgah_enc;
	umgah_desc.uninit_encounter_func = uninit_umgah;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	umgah_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	umgah_desc.AlienTextBaseline.y = 0;
	umgah_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if ((uqm::gameState().flag(uqm::GameFlag::TALKING_PET) && !uqm::gameState().flag(uqm::GameFlag::UMGAH_HOSTILE))
		|| uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Peace);
	}
	else
	{
		setSegue(Segue::Hostile);
	}
	ConversationData* retval {&umgah_desc};

	return (retval);
}

} // namespace uqm
