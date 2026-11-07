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
#include "uqm/units.h"
#include "uqm/setup.h"
#include "uqm/sis.h"
// for DeltaSISGauges(), DrawLanders()
#include "libs/graphics/gfx_common.h"
#include "uqm/lua/UQMLua.h"
#include "uqm/settings.h"
#include "uqm/nameref.h"
#include "libs/math/Random.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
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
								CommanderInitPmapAnim, /* AlienAltFrame */
			NullResource,		   /* AlienAltColorMap */
			CommanderLowpowMusic,  /* AlienAltSong */
		},
		CommanderConversationPhrases, /* PlayerPhrases */
		4, /* NumAnimations */
		{
								/* AlienAmbientArray (ambient animations) */
			{
				/* Blink */
				1,		  /* StartIndex */
				3,		  /* NumFrames */
				YoyoAnim, /* AnimFlags */
				GameTicksPerSecond / 15,
				0, /* FrameRate */
				0,
				GameTicksPerSecond * 8, /* RestartRate */
				0,						/* BlockMask */
			},
								{
				/* Running light */
				10,			  /* StartIndex */
				30,			  /* NumFrames */
				CircularAnim, /* AnimFlags */
				GameTicksPerSecond / 40,
				0, /* FrameRate */
				GameTicksPerSecond * 2,
				0, /* RestartRate */
				0, /* BlockMask */
			},
								{
				/* SD "rave party" */
				1, /* StartIndex */
				3, /* NumFrames */
				RandomAnim | ColorxformAnim
					| AnimDisabled, /* AnimFlags */
				0,
				GameTicksPerSecond / 30, /* FrameRate */
				0,
				GameTicksPerSecond / 15, /* RestartRate */
				0,						 /* BlockMask */
			},
								{
				/* HdResolutionFactor static distortion */
				40, /* StartIndex */
				16, /* NumFrames */
				RandomAnim | AnimDisabled
					| ImmumeToRestart, /* AnimFlags */
				0,
				GameTicksPerSecond / 5, /* FrameRate */
				0,
				GameTicksPerSecond / 4, /* RestartRate */
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
			4, /* StartIndex */
			6, /* NumFrames */
			0, /* AnimFlags */
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

static AnimFilterSet commander_filters =
	{
		2, /* Number of filters */
		{
								  /* Filter array */
			{
				0,					/* Color index */
				1,					/* Opacity index */
				56,					/* Frame index */
				DrawKind::Multiply, /* DrawKind*/
				FilterDisabled,		/* Flags */
			},
								  {
				2,				   /* Color index */
				3,				   /* Opacity index */
				56,				   /* Frame index */
				DrawKind::Overlay, /* DrawKind*/
				TurnOffOft,		   /* Flags */
			},
								  }
};

static void RaveParty(void)
{
	CommData.AlienColorMapIndex = (uqm::random().next() % 3) + 1;

	if (IsHd() && IsExtended())
	{
		CommData.AlienFrame = SetAbsFrameIndex(CommData.AlienFrame,
											   (uqm::random().next() % 15) + 40);
		SwitchSequences(false);
		EnableTalkingAnim(false);
		CommData.AlienAmbientArray[3].AnimFlags &= ~AnimDisabled;
	}
	else
	{
		CommData.AlienAmbientArray[2].AnimFlags &= ~AnimDisabled;

		if (IsHd())
		{
			EngageFilters(&commander_filters);
		}
	}
}

static void ByeBye(ResponseRef R)
{
	if (PlayerSaid(R, CommanderPhrase::OkIWillGetRadios))
	{
		NPCPhrase(CommanderPhrase::ThanksForHelping);
	}
	else if (PlayerSaid(R, CommanderPhrase::WellGoGetThemNow))
	{
		NPCPhrase(CommanderPhrase::GladWhenYouComeBack);
	}
	else if (PlayerSaid(R, CommanderPhrase::WeWillTakeCareOfBase))
	{
		NPCPhrase(CommanderPhrase::GoodLuckWithBase);

		uqm::gameState().setFlag(uqm::GameFlag::WILL_DESTROY_BASE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::TakeCareOfBaseAgain))
	{
		NPCPhrase(CommanderPhrase::GoodLuckAgain);
	}
	else if (PlayerSaid(R, CommanderPhrase::BaseWasAbandoned)
			 || PlayerSaid(R, CommanderPhrase::ILiedItWasAbandoned))
	{
		NPCPhrase(CommanderPhrase::ItWasAbandoned);
		NPCPhrase(CommanderPhrase::HereComesIlwrath);

		uqm::gameState().setFlag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::OhYesBigFight))
	{
		NPCPhrase(CommanderPhrase::ImGladYouWon);
		NPCPhrase(CommanderPhrase::HereComesIlwrath);

		uqm::gameState().setFlag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::ICantTalkAboutIt))
	{
		NPCPhrase(CommanderPhrase::ImSureItWasDifficult);
		NPCPhrase(CommanderPhrase::HereComesIlwrath);

		uqm::gameState().setFlag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::CookTheirButts)
			 || PlayerSaid(R, CommanderPhrase::OverthrowEvilAliens)
			 || PlayerSaid(R, CommanderPhrase::AnnihilateThoseMonsters))
	{
		uqm::gameState().setFlag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER, static_cast<uint32_t>(0));

		if (PlayerSaid(R, CommanderPhrase::CookTheirButts))
		{
			NPCPhrase(CommanderPhrase::CookButts);
		}
		else if (PlayerSaid(R, CommanderPhrase::OverthrowEvilAliens))
		{
			NPCPhrase(CommanderPhrase::OverthrowAliens);
		}
		else /* if (R == CommanderPhrase::AnnihilateThoseMonsters) */
		{
			NPCPhrase(CommanderPhrase::KillMonsters);
		}

		NPCPhrase(CommanderPhrase::ThisMaySeemSilly);

		Response(CommanderPhrase::Name1, ByeBye);
		Response(CommanderPhrase::Name2, ByeBye);
		Response(CommanderPhrase::Name3, ByeBye);
		Response(CommanderPhrase::Name4, ByeBye);

		uqm::gameState().setFlag(uqm::GameFlag::STARBASE_AVAILABLE, static_cast<uint32_t>(1));
	}
	else
	{
		if (PlayerSaid(R, CommanderPhrase::Name1))
		{
			NPCPhrase(CommanderPhrase::OkTheNafs);

			uqm::gameState().setFlag(uqm::GameFlag::NEW_ALLIANCE_NAME, static_cast<uint32_t>(0));
		}
		else if (PlayerSaid(R, CommanderPhrase::Name2))
		{
			NPCPhrase(CommanderPhrase::OkTheCan);

			uqm::gameState().setFlag(uqm::GameFlag::NEW_ALLIANCE_NAME, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, CommanderPhrase::Name3))
		{
			NPCPhrase(CommanderPhrase::OkTheUfw);

			uqm::gameState().setFlag(uqm::GameFlag::NEW_ALLIANCE_NAME, static_cast<uint32_t>(2));
		}
		else /* if (PlayerSaid (R, CommanderPhrase::Name4)) */
		{
			NPCPhrase(CommanderPhrase::OkTheNameIsEmpire);

			uqm::gameState().setFlag(uqm::GameFlag::NEW_ALLIANCE_NAME, static_cast<uint32_t>(3));
		}

		NPCPhrase(CommanderPhrase::StarbaseWillBeReady);
	}
}

static void GiveRadios(ResponseRef R);

static void NoRadioactives(ResponseRef R)
{
	if (PlayerSaid(R, CommanderPhrase::YesThisIsSupplyShip))
	{
		NPCPhrase(CommanderPhrase::AboutTime);

		if (uqm::gameState().sis().ElementAmounts[*MineralCategory::Radioactive])
		{
			GiveRadios(0);
		}
		else
		{
			Response(CommanderPhrase::ILied, NoRadioactives);
			Response(CommanderPhrase::PlumbOut, NoRadioactives);
		}
	}
	else
	{
		if (PlayerSaid(R, CommanderPhrase::WhereCanIGetRadios))
		{
			NPCPhrase(CommanderPhrase::RadiosOnMercury);

			DisablePhrase(CommanderPhrase::WhereCanIGetRadios);
		}
		else if (PlayerSaid(R, CommanderPhrase::NoButWellHelp))
		{
			NPCPhrase(CommanderPhrase::TheWhatFromWhere);
		}
		else if (PlayerSaid(R, CommanderPhrase::WhatSlavePlanet)
				 || PlayerSaid(R, CommanderPhrase::ILied))
		{
			NPCPhrase(CommanderPhrase::DontKnowWhoYouAre);
		}
		else if (PlayerSaid(R, CommanderPhrase::PlumbOut))
		{
			NPCPhrase(CommanderPhrase::WhatKindOfIdiot);
		}
		else if (PlayerSaid(R, CommanderPhrase::ILostMyLander))
		{
			NPCPhrase(CommanderPhrase::HereIsANewLander);
			++uqm::gameState().sis().NumLanders;
			DrawLanders();
			DeltaSISGauges(4, 0, 0);

			uqm::gameState().setFlag(uqm::GameFlag::LANDERS_LOST, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, CommanderPhrase::ILostAnotherLander))
		{
			NPCPhrase(CommanderPhrase::HereIsAnotherLander);
			++uqm::gameState().sis().NumLanders;
			DrawLanders();
			DeltaSISGauges(4, 0, 0);
		}
		else if (PlayerSaid(R, CommanderPhrase::NeedFuelMercury) || PlayerSaid(R, CommanderPhrase::NeedFuelLuna))
		{
			NPCPhrase(CommanderPhrase::GiveFuel);
			DeltaSISGauges(0, 5 * FuelTankScale, 0);

			uqm::gameState().setFlag(uqm::GameFlag::GIVEN_FUEL_BEFORE, static_cast<uint32_t>(1));
		}
		else if (PlayerSaid(R, CommanderPhrase::NeedFuelAgain))
		{
			NPCPhrase(CommanderPhrase::GiveFuelAgain);
			DeltaSISGauges(0, 5 * FuelTankScale, 0);
		}

		if (uqm::gameState().sis().ElementAmounts[*MineralCategory::Radioactive])
		{
			GiveRadios(0);
		}
		else
		{
			if (uqm::gameState().sis().NumLanders == 0
				&& uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2)
			{
				if (uqm::gameState().flag(uqm::GameFlag::LANDERS_LOST))
				{
					Response(CommanderPhrase::ILostAnotherLander, NoRadioactives);
				}
				else
				{
					Response(CommanderPhrase::ILostMyLander, NoRadioactives);
				}
			}
			if (uqm::gameState().sis().FuelOnBoard < 2 * FuelTankScale)
			{
				if (uqm::gameState().flag(uqm::GameFlag::GIVEN_FUEL_BEFORE))
				{
					Response(CommanderPhrase::NeedFuelAgain, NoRadioactives);
				}
				else
				{
					Response(CommanderPhrase::NeedFuelMercury, NoRadioactives);
				}
			}

			Response(CommanderPhrase::OkIWillGetRadios, ByeBye);
			if (PhraseEnabled(CommanderPhrase::WhereCanIGetRadios))
			{
				Response(CommanderPhrase::WhereCanIGetRadios, NoRadioactives);
			}
		}
	}
}

static void AskAfterRadios(ResponseRef R)
{
	if (PlayerSaid(R, CommanderPhrase::ILostMyLander))
	{
		NPCPhrase(CommanderPhrase::HereIsANewLander);
		++uqm::gameState().sis().NumLanders;
		DrawLanders();
		DeltaSISGauges(4, 0, 0);

		uqm::gameState().setFlag(uqm::GameFlag::LANDERS_LOST, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::ILostAnotherLander))
	{
		NPCPhrase(CommanderPhrase::HereIsAnotherLander);
		++uqm::gameState().sis().NumLanders;
		DrawLanders();
		DeltaSISGauges(4, 0, 0);
	}
	else if (PlayerSaid(R, CommanderPhrase::NeedFuelMercury) || PlayerSaid(R, CommanderPhrase::NeedFuelLuna))
	{
		NPCPhrase(CommanderPhrase::GiveFuel);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);

		uqm::gameState().setFlag(uqm::GameFlag::GIVEN_FUEL_BEFORE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::NeedFuelAgain))
	{
		NPCPhrase(CommanderPhrase::GiveFuelAgain);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);
	}
	else if (PlayerSaid(R, CommanderPhrase::WhereGetRadios))
	{
		NPCPhrase(CommanderPhrase::RadiosOnMercury);

		DisablePhrase(CommanderPhrase::WhereGetRadios);
	}

	{
		if (uqm::gameState().sis().NumLanders == 0
			&& uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2)
		{
			if (uqm::gameState().flag(uqm::GameFlag::LANDERS_LOST))
			{
				Response(CommanderPhrase::ILostAnotherLander, AskAfterRadios);
			}
			else
			{
				Response(CommanderPhrase::ILostMyLander, AskAfterRadios);
			}
		}
		if (uqm::gameState().sis().FuelOnBoard < 2 * FuelTankScale)
		{
			if (uqm::gameState().flag(uqm::GameFlag::GIVEN_FUEL_BEFORE))
			{
				Response(CommanderPhrase::NeedFuelAgain, AskAfterRadios);
			}
			else
			{
				Response(CommanderPhrase::NeedFuelMercury, AskAfterRadios);
			}
		}
		Response(CommanderPhrase::WellGoGetThemNow, ByeBye);
		if (PhraseEnabled(CommanderPhrase::WhereGetRadios))
		{
			Response(CommanderPhrase::WhereGetRadios, AskAfterRadios);
		}
	}
}

static void BaseDestroyed(ResponseRef R)
{
	if (PlayerSaid(R, CommanderPhrase::WeFoughtThem))
	{
		NPCPhrase(CommanderPhrase::YouReallyFoughtBase);

		Response(CommanderPhrase::OhYesBigFight, ByeBye);
		Response(CommanderPhrase::ILiedItWasAbandoned, ByeBye);
		Response(CommanderPhrase::ICantTalkAboutIt, ByeBye);
	}
	else
	{
		if (PlayerSaid(R, CommanderPhrase::WeAreHereToHelp))
		{
			NPCPhrase(CommanderPhrase::BaseOnMoon);
		}
		else
		{
			NPCPhrase(CommanderPhrase::DealtWithBaseYet);
		}

		Response(CommanderPhrase::BaseWasAbandoned, ByeBye);
		Response(CommanderPhrase::WeFoughtThem, BaseDestroyed);
	}
}

static void TellMoonBase(ResponseRef R)
{
	if (R == 0)
	{
		NPCPhrase(CommanderPhrase::DealtWithBaseYet);
	}
	else if (PlayerSaid(R, CommanderPhrase::ILostMyLander))
	{
		NPCPhrase(CommanderPhrase::HereIsANewLander);
		++uqm::gameState().sis().NumLanders;
		DrawLanders();
		DeltaSISGauges(4, 0, 0);

		uqm::gameState().setFlag(uqm::GameFlag::LANDERS_LOST, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::ILostAnotherLander))
	{
		NPCPhrase(CommanderPhrase::HereIsAnotherLander);
		++uqm::gameState().sis().NumLanders;
		DrawLanders();
		DeltaSISGauges(4, 0, 0);
	}
	else if (PlayerSaid(R, CommanderPhrase::NeedFuelMercury) || PlayerSaid(R, CommanderPhrase::NeedFuelLuna))
	{
		NPCPhrase(CommanderPhrase::GiveFuel);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);

		uqm::gameState().setFlag(uqm::GameFlag::GIVEN_FUEL_BEFORE, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, CommanderPhrase::NeedFuelAgain))
	{
		NPCPhrase(CommanderPhrase::GiveFuelAgain);
		DeltaSISGauges(0, 5 * FuelTankScale, 0);
	}
	else if (PlayerSaid(R, CommanderPhrase::WeAreHereToHelp))
	{
		NPCPhrase(CommanderPhrase::BaseOnMoon);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::STARBASE_YACK_STACK1) == 0)
	{
		NPCPhrase(CommanderPhrase::AboutBase);

		uqm::gameState().setFlag(uqm::GameFlag::STARBASE_YACK_STACK1, static_cast<uint32_t>(1));
	}
	else
	{
		NPCPhrase(CommanderPhrase::AboutBaseAgain);
	}

	if (uqm::gameState().sis().NumLanders == 0
		&& uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 2)
	{
		if (uqm::gameState().flag(uqm::GameFlag::LANDERS_LOST))
		{
			Response(CommanderPhrase::ILostAnotherLander, TellMoonBase);
		}
		else
		{
			Response(CommanderPhrase::ILostMyLander, TellMoonBase);
		}
	}
	if (uqm::gameState().sis().FuelOnBoard < 2 * FuelTankScale)
	{
		if (uqm::gameState().flag(uqm::GameFlag::GIVEN_FUEL_BEFORE))
		{
			Response(CommanderPhrase::NeedFuelAgain, TellMoonBase);
		}
		else
		{
			Response(CommanderPhrase::NeedFuelLuna, TellMoonBase);
		}
	}
	if (uqm::gameState().flag(uqm::GameFlag::WILL_DESTROY_BASE) == 0)
	{
		Response(CommanderPhrase::WeWillTakeCareOfBase, ByeBye);
	}
	else
	{
		Response(CommanderPhrase::TakeCareOfBaseAgain, ByeBye);
	}
	if (uqm::gameState().flag(uqm::GameFlag::STARBASE_YACK_STACK1) == 0)
	{
		Response(CommanderPhrase::TellMeAboutBase, TellMoonBase);
	}
	else
	{
		Response(CommanderPhrase::TellMeAgain, TellMoonBase);
	}
}

static void RevealSelf(ResponseRef R);

static void TellProbe(ResponseRef R)
{
	(void)R; // ignored
	NPCPhrase(CommanderPhrase::ThatWasProbe);
	DisablePhrase(CommanderPhrase::WhatWasRedThing);

	Response(CommanderPhrase::ItWentAway, RevealSelf);
	Response(CommanderPhrase::WeDestroyedIt, RevealSelf);
	Response(CommanderPhrase::WhatProbe, RevealSelf);
}

static void RevealSelf(ResponseRef R)
{
	uint8_t stack {0};
	if (PlayerSaid(R, CommanderPhrase::WeAreVindicator))
	{
		NPCPhrase(CommanderPhrase::ThatsImpossible);

		DisablePhrase(CommanderPhrase::WeAreVindicator);
	}
	else if (PlayerSaid(R, CommanderPhrase::OurMissionWasSecret))
	{
		NPCPhrase(CommanderPhrase::AcknowledgeSecret);

		DisablePhrase(CommanderPhrase::OurMissionWasSecret);
	}
	else if (PlayerSaid(R, CommanderPhrase::FirstGiveInfo))
	{
		NPCPhrase(CommanderPhrase::AskAway);

		stack = 1;
		DisablePhrase(CommanderPhrase::FirstGiveInfo);
	}
	else if (PlayerSaid(R, CommanderPhrase::WhatsThisStarbase))
	{
		NPCPhrase(CommanderPhrase::StarbaseIs);

		stack = 1;
		DisablePhrase(CommanderPhrase::WhatsThisStarbase);
	}
	else if (PlayerSaid(R, CommanderPhrase::WhatAboutEarth))
	{
		NPCPhrase(CommanderPhrase::HappenedToEarth);

		stack = 1;
		DisablePhrase(CommanderPhrase::WhatAboutEarth);
	}
	else if (PlayerSaid(R, CommanderPhrase::WhereAreUrquan))
	{
		NPCPhrase(CommanderPhrase::UrquanLeft);

		stack = 1;
		DisablePhrase(CommanderPhrase::WhereAreUrquan);
	}
	else if (PlayerSaid(R, CommanderPhrase::ItWentAway))
	{
		NPCPhrase(CommanderPhrase::DeepTrouble);
	}
	else if (PlayerSaid(R, CommanderPhrase::WeDestroyedIt))
	{
		NPCPhrase(CommanderPhrase::GoodNews);
	}
	else if (PlayerSaid(R, CommanderPhrase::WhatProbe))
	{
		NPCPhrase(CommanderPhrase::SureHope);
	}

	for (uint8_t i {0}; i < 2; ++i, stack ^= 1)
	{
		if (stack == 1)
		{
			if (PhraseEnabled(CommanderPhrase::FirstGiveInfo))
			{
				Response(CommanderPhrase::FirstGiveInfo, RevealSelf);
			}
			else if (PhraseEnabled(CommanderPhrase::WhatsThisStarbase))
			{
				Response(CommanderPhrase::WhatsThisStarbase, RevealSelf);
			}
			else if (PhraseEnabled(CommanderPhrase::WhatAboutEarth))
			{
				Response(CommanderPhrase::WhatAboutEarth, RevealSelf);
			}
			else if (PhraseEnabled(CommanderPhrase::WhereAreUrquan))
			{
				Response(CommanderPhrase::WhereAreUrquan, RevealSelf);
			}
			else if (PhraseEnabled(CommanderPhrase::WhatWasRedThing))
			{
				Response(CommanderPhrase::WhatWasRedThing, TellProbe);
			}
		}
		else
		{
			if (PhraseEnabled(CommanderPhrase::WeAreVindicator))
			{
				Response(CommanderPhrase::WeAreVindicator, RevealSelf);
			}
			else if (PhraseEnabled(CommanderPhrase::OurMissionWasSecret))
			{
				Response(CommanderPhrase::OurMissionWasSecret, RevealSelf);
			}
			else
			{
				if (uqm::gameState().flag(uqm::GameFlag::MOONBASE_DESTROYED) == 0)
				{
					Response(CommanderPhrase::WeAreHereToHelp, TellMoonBase);
				}
				else
				{
					Response(CommanderPhrase::WeAreHereToHelp, BaseDestroyed);
				}
			}
		}
	}
}

static void GiveRadios(ResponseRef R)
{
	if (PlayerSaid(R, CommanderPhrase::WeWillTransferNow))
	{
		uqm::gameState().setFlag(uqm::GameFlag::RADIOACTIVES_PROVIDED, static_cast<uint32_t>(1));

		NPCPhrase(CommanderPhrase::FuelUp0);
		NPCPhrase(CommanderPhrase::FuelUp1);
		AlienTalkSegue(1);

		CommData.AlienAmbientArray[2].AnimFlags |= AnimDisabled;

		if (IsHd() && IsExtended())
		{ // Disable noisy static animation in hi-res.
			CommData.AlienFrame =
				SetAbsFrameIndex(CommData.AlienFrame, 0);

			CommData.AlienAmbientArray[0].AnimFlags &= ~AnimDisabled;
			CommData.AlienAmbientArray[1].AnimFlags &= ~AnimDisabled;
			CommData.AlienAmbientArray[3].AnimFlags |= AnimDisabled;
			EnableTalkingAnim(true);

			EngageFilters(&commander_filters);
			FilterData.FilterArray[0].Flags = TurnOffOft;
			FilterData.FilterArray[1].Flags |= FilterDisabled;
		}

		if (IsHd() && !IsExtended())
		{
			FilterData.FilterArray[0].Flags = TurnOffOft;
			FilterData.FilterArray[1].Flags = FilterDisabled;
		}

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, 0),
					  c_gameSecond / 2);

		if (altResFlags & UseAltSong)
		{
			const uint32_t MusicPos = PLRGetPos();
			StopMusic();
			CommData.AlienSong = LoadMusicInstance(CommData.AlienSongRes);

			SetMusicVolume(MuteVolume);
			PlayMusic(CommData.AlienSong, true, 1);
			SeekMusic(MusicPos);
			FadeMusic(NormalVolume, c_gameSecond);

			altResFlags &= ~UseAltSong;
		}

		AlienTalkSegue((uint16_t)~0);

		RevealSelf(0);
	}
	else
	{
		if (PlayerSaid(R, CommanderPhrase::WhatWillYouGiveUs))
		{
			NPCPhrase(CommanderPhrase::MessageGarbled1);
		}
		else if (PlayerSaid(R, CommanderPhrase::BeforeRadiosWeNeedInfo))
		{
			NPCPhrase(CommanderPhrase::MessageGarbled2);
		}

		Response(CommanderPhrase::WeWillTransferNow, GiveRadios);
		Response(CommanderPhrase::WhatWillYouGiveUs, GiveRadios);
		Response(CommanderPhrase::BeforeRadiosWeNeedInfo, GiveRadios);
	}
}

static void Intro(void)
{
	if (IsHd()) // To smooth out HdResolutionFactor blink animation
	{
		CommData.AlienAmbientArray[0].BaseFrameRate = GameTicksPerSecond / 40;
	}

	if (uqm::gameState().flag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER))
	{
		NPCPhrase(CommanderPhrase::VeryImpressive);

		Response(CommanderPhrase::CookTheirButts, ByeBye);
		Response(CommanderPhrase::OverthrowEvilAliens, ByeBye);
		Response(CommanderPhrase::AnnihilateThoseMonsters, ByeBye);
	}
	else if (uqm::gameState().flag(uqm::GameFlag::STARBASE_VISITED))
	{
		if (uqm::gameState().flag(uqm::GameFlag::RADIOACTIVES_PROVIDED))
		{
			if (uqm::gameState().flag(uqm::GameFlag::MOONBASE_DESTROYED) == 0)
			{
				TellMoonBase(0);
			}
			else
			{
				BaseDestroyed(0);
			}
		}
		else
		{
			RaveParty();

			NPCPhrase(CommanderPhrase::DoYouHaveRadioThisTime);

			if (uqm::gameState().sis().ElementAmounts[*MineralCategory::Radioactive])
			{
				GiveRadios(0);
			}
			else
			{
				AskAfterRadios(0);
			}
		}
	}
	else /* first visit */
	{
		RaveParty();

		uqm::gameState().setFlag(uqm::GameFlag::STARBASE_VISITED, static_cast<uint32_t>(1));

		NPCPhrase(CommanderPhrase::AreYouSupplyShip);
		Response(CommanderPhrase::NoButWellHelp, NoRadioactives);
		Response(CommanderPhrase::YesThisIsSupplyShip, NoRadioactives);
		Response(CommanderPhrase::WhatSlavePlanet, NoRadioactives);
	}
}

static uint16_t uninit_commander(void)
{
	uqm::lua().endComm();
	return (0);
}

static void post_commander_enc(void)
{
	// nothing defined so far
}

ConversationData* init_commander_comm()
{
	commander_desc.init_encounter_func = Intro;
	commander_desc.post_encounter_func = post_commander_enc;
	commander_desc.uninit_encounter_func = uninit_commander;

	uqm::lua().beginComm(nullptr, NullResource);
	// Initialise Lua for string interpolation. This will be
	// generalised in the future.

	if (!uqm::gameState().flag(uqm::GameFlag::RADIOACTIVES_PROVIDED))
	{
		altResFlags |= UseAltSong;
	}

	if (IsHd())
	{
		altResFlags |= UseAltFrame;
	}


	commander_desc.AlienTextWidth = ResScale(143);
	commander_desc.AlienTextBaseline.x = ResScale(164)
									   - (SafeNeg(2) * 2);
	commander_desc.AlienTextBaseline.y = ResScale(20);

	setSegue(Segue::Peace);
	ConversationData* retval {&commander_desc};

	return (retval);
}

} // namespace uqm