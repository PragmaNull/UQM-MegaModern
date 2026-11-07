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

static ConversationData blackurq_desc =
	{
		Conversation::Blackurq, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		BlackurqPmapAnim, /* AlienFrame */
		BlackurqFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		BlackurqColorMap, /* AlienColorMap */
		BlackurqMusic, /* AlienSong */
		{
								NullResource, /* AlienAltFrame */
			NullResource, /* AlienAltColorMap */
			NullResource, /* AlienAltSong */
		},
		BlackurqConversationPhrases, /* PlayerPhrases */
		9, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				7,			  /* StartIndex */
				6,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				13,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				// New eye animation
				42,			  /* StartIndex */
				7,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				20,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				23,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				26,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				29,			/* StartIndex */
				4,			/* NumFrames */
				RandomAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond / 10,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				// "Left" extremity
				33,							/* StartIndex */
				5,							/* NumFrames */
				CircularAnim | WaitTalking, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				// "Right" extremeity
				38,			  /* StartIndex */
				4,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 10,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								},
		{
								/* AlienTransitionDesc */
			1, /* StartIndex */
			2, /* NumFrames */
			0, /* AnimFlags */
			GameTicksPerSecond / 30,
								0, /* FrameRate */
			0,
								0, /* RestartRate */
			0, /* BlockMask */
		},
		{
								/* AlienTalkDesc */
			2, /* StartIndex */
			5, /* NumFrames */
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

static void CombatIsInevitable(RESPONSE_REF R)
{
	uint8_t NumVisits {};

	setSegue(Segue::Hostile);

	if (PlayerSaid(R, KohrAhPhrase::Bye))
	{
		if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_BYES) == 0)
		{
			NPCPhrase(KohrAhPhrase::GoodbyeAndDie);
		}
		else
		{
			NPCPhrase(KohrAhPhrase::DieHuman /* GOODBYE_AND_DIE_2 */);
		}

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_BYES, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, KohrAhPhrase::GuessThatsAll))
	{
		NPCPhrase(KohrAhPhrase::ThenDie);
	}
	else if (PlayerSaid(R, KohrAhPhrase::WhatAreYouHoveringOver))
	{
		NPCPhrase(KohrAhPhrase::BonePile);

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, KohrAhPhrase::YouSureAreCreepy))
	{
		NPCPhrase(KohrAhPhrase::YesCreepy);

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, KohrAhPhrase::StopThatGrossBlinking))
	{
		NPCPhrase(KohrAhPhrase::DieHuman);

		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, KohrAhPhrase::Threat1)
			 || PlayerSaid(R, KohrAhPhrase::Threat2)
			 || PlayerSaid(R, KohrAhPhrase::Threat3)
			 || PlayerSaid(R, KohrAhPhrase::Threat4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(KohrAhPhrase::ResistanceIsUseless1);
				break;
			case 1:
				NPCPhrase(KohrAhPhrase::ResistanceIsUseless2);
				break;
			case 2:
				NPCPhrase(KohrAhPhrase::ResistanceIsUseless3);
				break;
			case 3:
				NPCPhrase(KohrAhPhrase::ResistanceIsUseless4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_REASONS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, KohrAhPhrase::Plead1)
			 || PlayerSaid(R, KohrAhPhrase::Plead2)
			 || PlayerSaid(R, KohrAhPhrase::Plead3)
			 || PlayerSaid(R, KohrAhPhrase::Plead4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(KohrAhPhrase::PleadingIsUseless1);
				break;
			case 1:
				NPCPhrase(KohrAhPhrase::PleadingIsUseless2);
				break;
			case 2:
				// This response disabled due to lack of a speech file.
				// NPCPhrase (KohrAhPhrase::PleadingIsUseless3);
				// break;
			case 3:
				NPCPhrase(KohrAhPhrase::PleadingIsUseless4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_PLEAD, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, KohrAhPhrase::WhyKillAll1)
			 || PlayerSaid(R, KohrAhPhrase::WhyKillAll2)
			 || PlayerSaid(R, KohrAhPhrase::WhyKillAll3)
			 || PlayerSaid(R, KohrAhPhrase::WhyKillAll4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(KohrAhPhrase::KillBecause1);
				break;
			case 1:
				NPCPhrase(KohrAhPhrase::KillBecause2);
				break;
			case 2:
				NPCPhrase(KohrAhPhrase::KillBecause3);
				break;
			case 3:
				NPCPhrase(KohrAhPhrase::KillBecause4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_REASONS, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, KohrAhPhrase::PleaseDontKill1)
			 || PlayerSaid(R, KohrAhPhrase::PleaseDontKill2)
			 || PlayerSaid(R, KohrAhPhrase::PleaseDontKill3)
			 || PlayerSaid(R, KohrAhPhrase::PleaseDontKill4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(KohrAhPhrase::WillKill1);
				break;
			case 1:
				NPCPhrase(KohrAhPhrase::WillKill2);
				break;
			case 2:
				NPCPhrase(KohrAhPhrase::WillKill3);
				break;
			case 3:
				NPCPhrase(KohrAhPhrase::WillKill4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_PLEAD, static_cast<uint32_t>(NumVisits));
	}
	else if (PlayerSaid(R, KohrAhPhrase::ByeFrenzy1)
			 || PlayerSaid(R, KohrAhPhrase::ByeFrenzy2)
			 || PlayerSaid(R, KohrAhPhrase::ByeFrenzy3)
			 || PlayerSaid(R, KohrAhPhrase::ByeFrenzy4))
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(KohrAhPhrase::GoodbyeAndDieFrenzy1);
				break;
			case 1:
				NPCPhrase(KohrAhPhrase::GoodbyeAndDieFrenzy2);
				break;
			case 2:
				NPCPhrase(KohrAhPhrase::GoodbyeAndDieFrenzy3);
				break;
			case 3:
				NPCPhrase(KohrAhPhrase::GoodbyeAndDieFrenzy4);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_INFO, static_cast<uint32_t>(NumVisits));
	}
}

static void Frenzy(RESPONSE_REF R)
{
	(void)R; // ignored
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS))
	{
		case 0:
			Response(KohrAhPhrase::WhyKillAll1, CombatIsInevitable);
			break;
		case 1:
			Response(KohrAhPhrase::WhyKillAll2, CombatIsInevitable);
			break;
		case 2:
			Response(KohrAhPhrase::WhyKillAll3, CombatIsInevitable);
			break;
		case 3:
			Response(KohrAhPhrase::WhyKillAll4, CombatIsInevitable);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD))
	{
		case 0:
			Response(KohrAhPhrase::PleaseDontKill1, CombatIsInevitable);
			break;
		case 1:
			Response(KohrAhPhrase::PleaseDontKill2, CombatIsInevitable);
			break;
		case 2:
			Response(KohrAhPhrase::PleaseDontKill3, CombatIsInevitable);
			break;
		case 3:
			Response(KohrAhPhrase::PleaseDontKill4, CombatIsInevitable);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_INFO))
	{
		case 0:
			Response(KohrAhPhrase::ByeFrenzy1, CombatIsInevitable);
			break;
		case 1:
			Response(KohrAhPhrase::ByeFrenzy2, CombatIsInevitable);
			break;
		case 2:
			Response(KohrAhPhrase::ByeFrenzy3, CombatIsInevitable);
			break;
		case 3:
			Response(KohrAhPhrase::ByeFrenzy4, CombatIsInevitable);
			break;
	}
}

static void KohrAhStory(RESPONSE_REF R)
{
	if (PlayerSaid(R, KohrAhPhrase::KeyPhrase))
	{
		NPCPhrase(KohrAhPhrase::ResponseToKeyPhrase);

		uqm::gameState().setFlag(uqm::GameFlag::KNOW_KOHR_AH_STORY, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, KohrAhPhrase::WhyDoYouDestroy))
	{
		NPCPhrase(KohrAhPhrase::WeWereSlaves);

		DisablePhrase(KohrAhPhrase::WhyDoYouDestroy);
	}
	else if (PlayerSaid(R, KohrAhPhrase::RelationshipWithUrquan))
	{
		NPCPhrase(KohrAhPhrase::WeAreUrquanToo);

		DisablePhrase(KohrAhPhrase::RelationshipWithUrquan);
	}
	else if (PlayerSaid(R, KohrAhPhrase::WhatAboutCulture))
	{
		NPCPhrase(KohrAhPhrase::BoneGardens);

		DisablePhrase(KohrAhPhrase::WhatAboutCulture);
	}
	else if (PlayerSaid(R, KohrAhPhrase::HowLeaveMeAlone))
	{
		NPCPhrase(KohrAhPhrase::YouDie);

		DisablePhrase(KohrAhPhrase::HowLeaveMeAlone);
	}

	if (PhraseEnabled(KohrAhPhrase::WhyDoYouDestroy))
	{
		Response(KohrAhPhrase::WhyDoYouDestroy, KohrAhStory);
	}
	if (PhraseEnabled(KohrAhPhrase::RelationshipWithUrquan))
	{
		Response(KohrAhPhrase::RelationshipWithUrquan, KohrAhStory);
	}
	if (PhraseEnabled(KohrAhPhrase::WhatAboutCulture))
	{
		Response(KohrAhPhrase::WhatAboutCulture, KohrAhStory);
	}
	if (PhraseEnabled(KohrAhPhrase::HowLeaveMeAlone))
	{
		Response(KohrAhPhrase::HowLeaveMeAlone, KohrAhStory);
	}
	Response(KohrAhPhrase::GuessThatsAll, CombatIsInevitable);
}

static void DieHuman(RESPONSE_REF R)
{
	(void)R; // ignored
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_REASONS))
	{
		case 0:
			Response(KohrAhPhrase::Threat1, CombatIsInevitable);
			break;
		case 1:
			Response(KohrAhPhrase::Threat2, CombatIsInevitable);
			break;
		case 2:
			Response(KohrAhPhrase::Threat3, CombatIsInevitable);
			break;
		case 3:
			Response(KohrAhPhrase::Threat4, CombatIsInevitable);
			break;
	}
	if (uqm::gameState().flag(uqm::GameFlag::KNOW_KOHR_AH_STORY) == 1)
	{
		Response(KohrAhPhrase::KeyPhrase, KohrAhStory);
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_INFO))
	{
		case 0:
			Response(KohrAhPhrase::WhatAreYouHoveringOver, CombatIsInevitable);
			break;
		case 1:
			Response(KohrAhPhrase::YouSureAreCreepy, CombatIsInevitable);
			break;
		case 2:
			Response(KohrAhPhrase::StopThatGrossBlinking, CombatIsInevitable);
			break;
	}
	switch (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_PLEAD))
	{
		case 0:
			Response(KohrAhPhrase::Plead1, CombatIsInevitable);
			break;
		case 1:
			Response(KohrAhPhrase::Plead2, CombatIsInevitable);
			break;
		case 2:
			// This response disabled due to lack of a speech file.
			// Response (KohrAhPhrase::Plead3, CombatIsInevitable);
			// break;
		case 3:
			Response(KohrAhPhrase::Plead4, CombatIsInevitable);
			break;
	}
	Response(KohrAhPhrase::Bye, CombatIsInevitable);
}

static void Intro(void)
{
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::WonLastBattle)
	{
		NPCPhrase(KohrAhPhrase::OutTakes);

		setSegue(Segue::Peace);
		return;
	}

	if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_KILLED_ALL))
	{
		NPCPhrase(KohrAhPhrase::GameOverDude);

		setSegue(Segue::Peace);
		return;
	}

	if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_SENSES_EVIL)
		&& uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		NPCPhrase(KohrAhPhrase::SenseEvil);
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_SENSES_EVIL, static_cast<uint32_t>(1));
	}

	const uint32_t GrpOffs {uqm::gameState().flag(uqm::GameFlag::SAMATRA_GRPOFFS)};
	if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
		&& uqm::gameState().game().BattleGroupRef
		&& uqm::gameState().game().BattleGroupRef == GrpOffs)
	{
		NPCPhrase(KohrAhPhrase::HelloSamatra);

		uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
		setSegue(Segue::Hostile);
	}
	else
	{
		uint8_t NumVisits {};

		NumVisits = uqm::gameState().flag(uqm::GameFlag::KOHR_AH_VISITS);
		if (uqm::gameState().flag(uqm::GameFlag::KOHR_AH_FRENZY))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(KohrAhPhrase::WeKillAll1);
					break;
				case 1:
					NPCPhrase(KohrAhPhrase::WeKillAll2);
					break;
				case 2:
					NPCPhrase(KohrAhPhrase::WeKillAll3);
					break;
				case 3:
					NPCPhrase(KohrAhPhrase::WeKillAll4);
					--NumVisits;
					break;
			}

			Frenzy((RESPONSE_REF)0);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(KohrAhPhrase::HelloAndDie1);
					break;
				case 1:
					NPCPhrase(KohrAhPhrase::HelloAndDie2);
					break;
				case 2:
					NPCPhrase(KohrAhPhrase::HelloAndDie3);
					break;
				case 3:
					NPCPhrase(KohrAhPhrase::HelloAndDie4);
					--NumVisits;
					break;
			}

			DieHuman((RESPONSE_REF)0);
		}
		uqm::gameState().setFlag(uqm::GameFlag::KOHR_AH_VISITS, static_cast<uint32_t>(NumVisits));
	}
}

static uint16_t uninit_blackurq(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_blackurq_enc(void)
{
	// nothing defined so far
}

ConversationData* init_blackurq_comm(void)
{
	blackurq_desc.init_encounter_func = Intro;
	blackurq_desc.post_encounter_func = post_blackurq_enc;
	blackurq_desc.uninit_encounter_func = uninit_blackurq;

	uqm::lua().beginComm(nullptr, NullResource);

	blackurq_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	blackurq_desc.AlienTextBaseline.y = 0;
	blackurq_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	if (!uqm::gameState().flag(uqm::GameFlag::KOHR_AH_KILLED_ALL)
		&& uqm::activityKind(uqm::currentActivity()) != ActivityKind::WonLastBattle)
	{
		setSegue(Segue::Hostile);
	}
	else
	{
		setSegue(Segue::Peace);
	}
	ConversationData* retval {&blackurq_desc};

	return (retval);
}
