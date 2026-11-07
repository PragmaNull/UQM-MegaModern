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
#include "uqm/hyper.h"
// for SOL_X/SOL_Y
#include "../../nameref.h"
// JMS_GFX: For LoadGraphic()
#include "../../planets/planets.h"
// For MinMoonRadius()
// JSD it needs starmap.h to find SOL
#include "../../starmap.h"


namespace uqm
{
static ConversationData chmmr_desc =
	{
		Conversation::Chmmr, /* AlienConv */
		nullptr, /* init_encounter_func */
		nullptr, /* post_encounter_func */
		nullptr, /* uninit_encounter_func */
		ChmmrPmapAnim, /* AlienFrame */
		ChmmrFont, /* AlienFont */
		WhiteColor, /* AlienTextFColor */
		BlackColor, /* AlienTextBColor */
		{0, 0}, /* AlienTextBaseline */
		0,
		/* SisTextWidth() - 16, */  /* AlienTextWidth */
		TextAlign::Center, /* AlienTextAlign */
		TextVAlign::Top, /* AlienTextValign */
		ChmmrColorMap, /* AlienColorMap */
		ChmmrMusic, /* AlienSong */
		{
								NullResource,	   /* AlienAltFrame */
			NullResource,	   /* AlienAltColorMap */
			ChmmrProcessMusic, /* AlienAltSong */
		},
		ChmmrConversationPhrases, /* PlayerPhrases */
		6, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				12,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				17,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				22,			  /* StartIndex */
				5,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				27,		  /* StartIndex */
				20,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				47,		  /* StartIndex */
				14,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				61,		  /* StartIndex */
				24,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 20,
				0, /* FrameRate */
				GameTicksPerSecond,
				GameTicksPerSecond * 3, /* RestartRate */
				0,						/* BlockMask */
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
			1,	/* StartIndex */
			11, /* NumFrames */
			0,	/* AnimFlags */
			GameTicksPerSecond / 60,
								0, /* FrameRate */
			GameTicksPerSecond / 60,
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

static AnimFilterSet chmmr_filters =
	{
		3, /* Number of filters */
		{
								  /* Filter array */
			{
				1,					 /* Color index */
				1,					 /* Opacity index */
				-1,					 /* Frame index */
				DrawKind::Grayscale, /* DrawKind*/
				0,					 /* Flags */
			},
								  {
				0,				   /* Color index */
				3,				   /* Opacity index */
				-1,				   /* Frame index */
				DrawKind::Overlay, /* DrawKind*/
				0,				   /* Flags */
			},
								  {
				0,				 /* Color index */
				2,				 /* Opacity index */
				-1,				 /* Frame index */
				DrawKind::Alpha, /* DrawKind*/
				0,				 /* Flags */
			},
								  }
};

static constexpr auto EarthIndex {2}; /* earth is 3rd planet --> 3 - 1 = 2 */

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue::Peace);

	if (PlayerSaid(R, ChmmrPhrase::Bye))
	{
		NPCPhrase(ChmmrPhrase::Goodbye);
	}
	else if (PlayerSaid(R, ChmmrPhrase::ByeShielded))
	{
		NPCPhrase(ChmmrPhrase::GoodbyeShielded);
	}
	else if (PlayerSaid(R, ChmmrPhrase::ByeAfterBomb))
	{
		NPCPhrase(ChmmrPhrase::GoodbyeAfterBomb);
	}
	else if (PlayerSaid(R, ChmmrPhrase::Proceed))
	{
		int i {};

		NPCPhrase(ChmmrPhrase::Take2Weeks);

		SetRaceAllied(RaceId::Chmmr, true);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_HOME_VISITS, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_BOMB_STATE, static_cast<uint32_t>(2));
		uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP, static_cast<uint32_t>(0));

		if (!uqm::UQMOptions::read().infiniteRU)
		{
			uqm::gameState().sis().ResUnits = 1000000L;
		}
		else
		{
			oldRU = 1000000L;
		}

		uqm::gameState().sis().NumLanders = 0;
		uqm::gameState().game().ModuleCost[*ModuleType::PlanetLander] = 0;


		/* transport player to Earth */
		// JSD replace old method of teleport to SOL with new method
		// GLOBAL_SIS (log_x) = UniverseToLogx (SOL_X);
		// GLOBAL_SIS (log_y) = UniverseToLogy (SOL_Y);
		uqm::gameState().sis().log_x = UniverseToLogx(plot_map[*PlotId::Sol].star_pt.x);
		uqm::gameState().sis().log_y = UniverseToLogy(plot_map[*PlotId::Sol].star_pt.y);
		uqm::flagship().shipFacing = 1;
		/* At Earth or at Starbase */
		uqm::flagship().ipPlanet = EarthIndex + 1;
		uqm::flagship().inOrbit = 0;
		/* XXX : this should be unhardcoded eventually */
		uqm::flagship().ipLocation.x = EarthOuterX;
		uqm::flagship().ipLocation.y = EarthOuterY;

		if (uqm::gameState().flag(uqm::GameFlag::STARBASE_AVAILABLE))
		{ /* Normal game mode - you are transported to Starbase */
			uqm::gameState().sis().FuelOnBoard = FuelReserve;
			uqm::gameState().sis().CrewEnlisted = 0;
			uqm::gameState().sis().TotalElementMass = 0;
			uqm::gameState().game().ModuleCost[*ModuleType::StorageBay] = 0; /* disable Storage Bay */
			for (i = 0; i < MineralCategory::NumElementCategories; ++i)
			{
				uqm::gameState().sis().ElementAmounts[i] = 0;
			}
			for (i = NumBombModules; i < NumModuleSlots; ++i)
			{
				uqm::gameState().sis().ModuleSlots[i] = ModuleType::EmptyModuleSlot;
			}

			/* XXX : this should be unhardcoded eventually */
			/* transport to Starbase */
			/* note: if uqm::UQMOptions::read().orbitingPlanets is enabled, this will be corrected in DoTimePassage */
			uqm::flagship().shipStamp.origin.x = ResScale(OrigSisScreenWidth() >> 1) + Cosine(HalfCircle + Quadrant, MinMoonRadius());
			uqm::flagship().shipStamp.origin.y = ResScale(OrigSisScreenHeight() >> 1) + Sine(HalfCircle + Quadrant, MinMoonRadius() >> 1);
		}
		else
		{ /* 'Beating Game Differently' mode - never visited Starbase,
			 * so you are transported to Earth */
			/* compress the layout -- move all to front */
			for (i = NumModuleSlots - 1; i > 0; --i)
			{
				int m {};

				/* find next unused slot */
				for (; i > 0
					   && uqm::gameState().sis().ModuleSlots[i] != ModuleType::EmptyModuleSlot;
					 --i)
					;
				if (i == 0)
				{
					break;
				}
				/* find next module to move */
				for (m = i - 1; m >= 0
								&& uqm::gameState().sis().ModuleSlots[m] == ModuleType::EmptyModuleSlot;
					 --m)
					;
				if (m < 0)
				{
					break;
				}

				/* move the module */
				uqm::gameState().sis().ModuleSlots[i] = uqm::gameState().sis().ModuleSlots[m];
				uqm::gameState().sis().ModuleSlots[m] = ModuleType::EmptyModuleSlot;
			}
		}

		/* install Chmmr-supplied modules */
		for (i = 0; i < NumDriveSlots; ++i)
		{
			uqm::gameState().sis().DriveSlots[i] = ModuleType::FusionThruster;
		}
		for (i = 0; i < NumJetSlots; ++i)
		{
			uqm::gameState().sis().JetSlots[i] = ModuleType::TurningJets;
		}
		uqm::gameState().sis().ModuleSlots[0] = ModuleType::BombModule4;
		uqm::gameState().sis().ModuleSlots[1] = ModuleType::BombModule5;
		uqm::gameState().sis().ModuleSlots[2] = ModuleType::BombModule3;
		uqm::gameState().sis().ModuleSlots[3] = ModuleType::BombModule1;
		uqm::gameState().sis().ModuleSlots[4] = ModuleType::BombModule0;
		uqm::gameState().sis().ModuleSlots[5] = ModuleType::BombModule1;
		uqm::gameState().sis().ModuleSlots[6] = ModuleType::BombModule3;
		uqm::gameState().sis().ModuleSlots[7] = ModuleType::BombModule4;
		uqm::gameState().sis().ModuleSlots[8] = ModuleType::BombModule5;
		uqm::gameState().sis().ModuleSlots[9] = ModuleType::BombModule2;
	}
	else if (PlayerSaid(R, ChmmrPhrase::PerhapsNotInstall))
	{
		NPCPhrase(ChmmrPhrase::Yes);

		NPCPhrase(ChmmrPhrase::Goodbye);
	}
}

static void NotReady(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(ChmmrPhrase::ReturnWhenReady);
	}
	else if (PlayerSaid(R, ChmmrPhrase::FurtherAssistance))
	{
		NPCPhrase(ChmmrPhrase::NoFurtherAssistance);

		DisablePhrase(ChmmrPhrase::FurtherAssistance);
	}
	else if (PlayerSaid(R, ChmmrPhrase::TechHelp))
	{
		NPCPhrase(ChmmrPhrase::UseOurShipsBefore);

		SetRaceAllied(RaceId::Chmmr, true);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhereWeapon))
	{
		NPCPhrase(ChmmrPhrase::PrecursorWeapon);

		DisablePhrase(ChmmrPhrase::WhereWeapon);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhereDistraction))
	{
		NPCPhrase(ChmmrPhrase::PsychicWeaponry);

		DisablePhrase(ChmmrPhrase::WhereDistraction);
	}

	if (CheckAlliance(RaceId::Chmmr) != RaceAllegiance::Good)
	{
		Response(ChmmrPhrase::TechHelp, NotReady);
	}
	else if (PhraseEnabled(ChmmrPhrase::FurtherAssistance))
	{
		Response(ChmmrPhrase::FurtherAssistance, NotReady);
	}
	if (PhraseEnabled(ChmmrPhrase::WhereWeapon) && !uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
	{
		Response(ChmmrPhrase::WhereWeapon, NotReady);
	}
	if (PhraseEnabled(ChmmrPhrase::WhereDistraction) && !uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		Response(ChmmrPhrase::WhereDistraction, NotReady);
	}
	Response(ChmmrPhrase::Bye, ExitConversation);
}

static void ImproveBomb(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(ChmmrPhrase::WeWillImproveBomb);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhatNow))
	{
		NPCPhrase(ChmmrPhrase::ModifyVessel);

		DisablePhrase(ChmmrPhrase::WhatNow);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WontHurtMyShip))
	{
		NPCPhrase(ChmmrPhrase::WillDestroyIt);

		DisablePhrase(ChmmrPhrase::WontHurtMyShip);
	}
	else if (PlayerSaid(R, ChmmrPhrase::BummerAboutMyShip))
	{
		NPCPhrase(ChmmrPhrase::DeadSilence);

		DisablePhrase(ChmmrPhrase::BummerAboutMyShip);
	}
	else if (PlayerSaid(R, ChmmrPhrase::OtherAssistance))
	{
		NPCPhrase(ChmmrPhrase::UseOurShipsAfter);

		SetRaceAllied(RaceId::Chmmr, true);
	}

	if (PhraseEnabled(ChmmrPhrase::WhatNow))
	{
		Response(ChmmrPhrase::WhatNow, ImproveBomb);
	}
	else if (PhraseEnabled(ChmmrPhrase::WontHurtMyShip))
	{
		Response(ChmmrPhrase::WontHurtMyShip, ImproveBomb);
	}
	else if (PhraseEnabled(ChmmrPhrase::BummerAboutMyShip))
	{
		Response(ChmmrPhrase::BummerAboutMyShip, ImproveBomb);
	}
	if (CheckAlliance(RaceId::Chmmr) != RaceAllegiance::Good)
	{
		Response(ChmmrPhrase::OtherAssistance, ImproveBomb);
	}
	Response(ChmmrPhrase::Proceed, ExitConversation);
	Response(ChmmrPhrase::PerhapsNotInstall, ExitConversation);
}

static void ChmmrFree(RESPONSE_REF R)
{
	if (R == 0
		|| PlayerSaid(R, ChmmrPhrase::IAmCaptain)
		|| PlayerSaid(R, ChmmrPhrase::IAmSavior)
		|| PlayerSaid(R, ChmmrPhrase::IAmSilly))
	{
		NPCPhrase(ChmmrPhrase::WhyHaveYouFreedUs);
		AlienTalkSegue((uint16_t)~0);
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_EMERGING, static_cast<uint32_t>(0));

		Response(ChmmrPhrase::Serious1, ChmmrFree);
		Response(ChmmrPhrase::Serious2, ChmmrFree);
		Response(ChmmrPhrase::Silly, ChmmrFree);
	}
	else
	{
		NPCPhrase(ChmmrPhrase::WillHelpAnalyzeLogs);

		if (uqm::gameState().flag(uqm::GameFlag::AWARE_OF_SAMATRA))
		{
			NPCPhrase(ChmmrPhrase::YouKnowSamatra);
		}
		else
		{
			NPCPhrase(ChmmrPhrase::DontKnowAboutSamatra);

			uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
		}

		if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
		{
			NPCPhrase(ChmmrPhrase::HaveTalkingPet);
		}
		else
		{
			NPCPhrase(ChmmrPhrase::NeedDistraction);
		}

		if (uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
		{
			NPCPhrase(ChmmrPhrase::HaveBomb);
		}
		else
		{
			NPCPhrase(ChmmrPhrase::NeedWeapon);
		}

		if (!uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP)
			|| !uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
		{
			NotReady((RESPONSE_REF)0);
		}
		else
		{
			ImproveBomb((RESPONSE_REF)0);
		}
	}
}

static void ChmmrShielded(RESPONSE_REF R);

static void ChmmrAdvice(RESPONSE_REF R)
{
	if (PlayerSaid(R, ChmmrPhrase::NeedAdvice))
	{
		NPCPhrase(ChmmrPhrase::WhatAdvice);
	}
	else if (PlayerSaid(R, ChmmrPhrase::HowDefeatUrquan))
	{
		NPCPhrase(ChmmrPhrase::DefeatLikeSo);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_BOMB_STATE, static_cast<uint32_t>(1));
		DisablePhrase(ChmmrPhrase::HowDefeatUrquan);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhatAboutTpet))
	{
		NPCPhrase(ChmmrPhrase::ScaryButUseful);

		DisablePhrase(ChmmrPhrase::WhatAboutTpet);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhatAboutBomb))
	{
		NPCPhrase(ChmmrPhrase::AboutBomb);

		DisablePhrase(ChmmrPhrase::WhatAboutBomb);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhatAboutSunDevice))
	{
		NPCPhrase(ChmmrPhrase::AboutSunDevice);

		DisablePhrase(ChmmrPhrase::WhatAboutSunDevice);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhatAboutSamatra))
	{
		NPCPhrase(ChmmrPhrase::AboutSamatra);

		DisablePhrase(ChmmrPhrase::WhatAboutSamatra);
	}

	uint8_t AdviceLeft {0};
	if (PhraseEnabled(ChmmrPhrase::HowDefeatUrquan))
	{
		Response(ChmmrPhrase::HowDefeatUrquan, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(ChmmrPhrase::WhatAboutTpet) && uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		Response(ChmmrPhrase::WhatAboutTpet, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(ChmmrPhrase::WhatAboutBomb) && uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
	{
		Response(ChmmrPhrase::WhatAboutBomb, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(ChmmrPhrase::WhatAboutSunDevice) && uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE_ON_SHIP))
	{
		Response(ChmmrPhrase::WhatAboutSunDevice, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(ChmmrPhrase::WhatAboutSamatra) && uqm::gameState().flag(uqm::GameFlag::AWARE_OF_SAMATRA))
	{
		Response(ChmmrPhrase::WhatAboutSamatra, ChmmrAdvice);
		AdviceLeft = true;
	}
	Response(ChmmrPhrase::EnoughAdvice, ChmmrShielded);

	if (!AdviceLeft)
	{
		DisablePhrase(ChmmrPhrase::NeedAdvice);
	}
}

static void ChmmrShielded(RESPONSE_REF R)
{
	if (PlayerSaid(R, ChmmrPhrase::FindOutWhatsUp))
	{
		NPCPhrase(ChmmrPhrase::HybridProcess);

		DisablePhrase(ChmmrPhrase::FindOutWhatsUp);
	}
	else if (PlayerSaid(R, ChmmrPhrase::NeedHelp))
	{
		NPCPhrase(ChmmrPhrase::CantHelp);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhyNoHelp))
	{
		NPCPhrase(ChmmrPhrase::LongTime);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhatIfMoreEnergy))
	{
		NPCPhrase(ChmmrPhrase::DangerToUs);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, ChmmrPhrase::EnoughAdvice))
	{
		NPCPhrase(ChmmrPhrase::OkEnoughAdvice);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::CHMMR_STACK))
	{
		case 0:
			Response(ChmmrPhrase::NeedHelp, ChmmrShielded);
			break;
		case 1:
			Response(ChmmrPhrase::WhyNoHelp, ChmmrShielded);
			break;
		case 2:
			Response(ChmmrPhrase::WhatIfMoreEnergy, ChmmrShielded);
			break;
	}
	if (PhraseEnabled(ChmmrPhrase::FindOutWhatsUp))
	{
		Response(ChmmrPhrase::FindOutWhatsUp, ChmmrShielded);
	}
	if (PhraseEnabled(ChmmrPhrase::NeedAdvice))
	{
		Response(ChmmrPhrase::NeedAdvice, ChmmrAdvice);
	}
	Response(ChmmrPhrase::ByeShielded, ExitConversation);
}

static void AfterBomb(RESPONSE_REF R)
{
	if (PlayerSaid(R, ChmmrPhrase::WhatsUpAfterBomb))
	{
		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_STACK))
		{
			NPCPhrase(ChmmrPhrase::GeneralInfoAfterBomb2);
		}
		else
		{
			NPCPhrase(ChmmrPhrase::GeneralInfoAfterBomb1);

			uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(1));
		}

		DisablePhrase(ChmmrPhrase::WhatsUpAfterBomb);
	}
	else if (PlayerSaid(R, ChmmrPhrase::WhatDoAfterBomb))
	{
		NPCPhrase(ChmmrPhrase::DoAfterBomb);

		DisablePhrase(ChmmrPhrase::WhatDoAfterBomb);
	}

	if (PhraseEnabled(ChmmrPhrase::WhatsUpAfterBomb))
	{
		Response(ChmmrPhrase::WhatsUpAfterBomb, AfterBomb);
	}
	if (PhraseEnabled(ChmmrPhrase::WhatDoAfterBomb))
	{
		Response(ChmmrPhrase::WhatDoAfterBomb, AfterBomb);
	}
	Response(ChmmrPhrase::ByeAfterBomb, ExitConversation);
}

static void Intro(void)
{
	uint8_t NumVisits {};

	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) >= 2)
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::CHMMR_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase(ChmmrPhrase::HelloAfterBomb1);
				break;
			case 1:
				NPCPhrase(ChmmrPhrase::HelloAfterBomb2);
				--NumVisits;
				break;
		}
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		AfterBomb((RESPONSE_REF)0);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::CHMMR_UNLEASHED))
	{
		if (!uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP)
			|| !uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
		{
			NotReady((RESPONSE_REF)0);
		}
		else
		{
			NPCPhrase(ChmmrPhrase::YouAreReady);

			ImproveBomb((RESPONSE_REF)0);
		}
	}
	else
	{
		NumVisits = uqm::gameState().flag(uqm::GameFlag::CHMMR_HOME_VISITS);
		if (!uqm::gameState().flag(uqm::GameFlag::CHMMR_EMERGING))
		{
			// Run filter for HdResolutionFactor
			if (IsHd())
			{
				EngageFilters(&chmmr_filters);
			}

			CommData.AlienColorMapIndex = 1;

			switch (NumVisits++)
			{
				case 0:
					NPCPhrase(ChmmrPhrase::WhyYouHere1);
					break;
				case 1:
					NPCPhrase(ChmmrPhrase::WhyYouHere2);
					break;
				case 2:
					NPCPhrase(ChmmrPhrase::WhyYouHere3);
					break;
				case 3:
					NPCPhrase(ChmmrPhrase::WhyYouHere4);
					--NumVisits;
					break;
			}

			ChmmrShielded((RESPONSE_REF)0);
		}
		else
		{
			SetCommIntroMode(CommIntroMode::FadeInScreen, c_gameSecond * 2);
			NPCPhrase(ChmmrPhrase::WeAreFree);

			if (NumVisits)
			{
				ChmmrFree((RESPONSE_REF)0);
				NumVisits = 0;
			}
			else
			{
				NPCPhrase(ChmmrPhrase::WhoAreYou);

				Response(ChmmrPhrase::IAmCaptain, ChmmrFree);
				Response(ChmmrPhrase::IAmSavior, ChmmrFree);
				Response(ChmmrPhrase::IAmSilly, ChmmrFree);
			}

			uqm::gameState().setFlag(uqm::GameFlag::CHMMR_UNLEASHED, static_cast<uint32_t>(1));

			// Setup a Chmmr sphere-of-influence, now that they're out
			// of their shell.  EncounterPercent for the Chmmr is 0,
			// so this is purely decorative.
			if (IsExtended())
			{
				HFLEETINFO hChmmr = GetStarShipFromIndex(
					&uqm::gameState().game().avail_race_q, *RaceId::Chmmr);
				FleetInfo* ChmmrPtr = LockFleetInfo(
					&uqm::gameState().game().avail_race_q, hChmmr);

				if (ChmmrPtr)
				{
					ChmmrPtr->actual_strength =
						986 / SphereRadiusIncrement * 2;
					ChmmrPtr->loc = SeedFleetLocation(ChmmrPtr, plot_map,
													  *PlotId::Home);
					StartSphereTracking(RaceId::Chmmr);
				}
				UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hChmmr);
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		SetHomeworldKnown(*HomeId::Chmmr);
	}
}

static uint16_t uninit_chmmr(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_chmmr_enc(void)
{
	// nothing defined so far
}

ConversationData* init_chmmr_comm(void)
{
	chmmr_desc.init_encounter_func = Intro;
	chmmr_desc.post_encounter_func = post_chmmr_enc;
	chmmr_desc.uninit_encounter_func = uninit_chmmr;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	// use alternate "Process Complete" track if available
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_UNLEASHED)
		|| uqm::gameState().flag(uqm::GameFlag::CHMMR_EMERGING))
	{
		altResFlags |= UseAltSong;
	}

	chmmr_desc.AlienTextBaseline.x = TextXOffs() + (SisTextWidth() >> 1);
	chmmr_desc.AlienTextBaseline.y = 0;
	chmmr_desc.AlienTextWidth = SisTextWidth() - ResScale(16);

	setSegue(Segue::Peace);
	ConversationData* retval {&chmmr_desc};

	return (retval);
}

} // namespace uqm
