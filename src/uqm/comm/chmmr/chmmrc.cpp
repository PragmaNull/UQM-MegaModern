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


static ConversationData chmmr_desc =
	{
		CHMMR_CONVERSATION, /* AlienConv */
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
		ALIGN_CENTER, /* AlienTextAlign */
		VALIGN_TOP, /* AlienTextValign */
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
				1,				/* Color index */
				1,				/* Opacity index */
				-1,				/* Frame index */
				DRAW_GRAYSCALE, /* DrawKind*/
				0,				/* Flags */
			},
								  {
				0,			  /* Color index */
				3,			  /* Opacity index */
				-1,			  /* Frame index */
				DRAW_OVERLAY, /* DrawKind*/
				0,			  /* Flags */
			},
								  {
				0,			/* Color index */
				2,			/* Opacity index */
				-1,			/* Frame index */
				DRAW_ALPHA, /* DrawKind*/
				0,			/* Flags */
			},
								  }
};

static constexpr auto EarthIndex {2}; /* earth is 3rd planet --> 3 - 1 = 2 */

static void ExitConversation(RESPONSE_REF R)
{
	setSegue(Segue_peace);

	if (PlayerSaid(R, bye))
	{
		NPCPhrase(GOODBYE);
	}
	else if (PlayerSaid(R, bye_shielded))
	{
		NPCPhrase(GOODBYE_SHIELDED);
	}
	else if (PlayerSaid(R, bye_after_bomb))
	{
		NPCPhrase(GOODBYE_AFTER_BOMB);
	}
	else if (PlayerSaid(R, proceed))
	{
		int i {};

		NPCPhrase(TAKE_2_WEEKS);

		SetRaceAllied(CHMMR_SHIP, true);

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
		uqm::gameState().game().ModuleCost[PLANET_LANDER] = 0;


		/* transport player to Earth */
		// JSD replace old method of teleport to SOL with new method
		// GLOBAL_SIS (log_x) = UniverseToLogx (SOL_X);
		// GLOBAL_SIS (log_y) = UniverseToLogy (SOL_Y);
		uqm::gameState().sis().log_x = UniverseToLogx(plot_map[SOL_DEFINED].star_pt.x);
		uqm::gameState().sis().log_y = UniverseToLogy(plot_map[SOL_DEFINED].star_pt.y);
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
			uqm::gameState().game().ModuleCost[STORAGE_BAY] = 0; /* disable Storage Bay */
			for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
			{
				uqm::gameState().sis().ElementAmounts[i] = 0;
			}
			for (i = NumBombModules; i < NumModuleSlots; ++i)
			{
				uqm::gameState().sis().ModuleSlots[i] = EmptySlot + 2;
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
					   && uqm::gameState().sis().ModuleSlots[i] != EmptySlot + 2;
					 --i)
					;
				if (i == 0)
				{
					break;
				}
				/* find next module to move */
				for (m = i - 1; m >= 0
								&& uqm::gameState().sis().ModuleSlots[m] == EmptySlot + 2;
					 --m)
					;
				if (m < 0)
				{
					break;
				}

				/* move the module */
				uqm::gameState().sis().ModuleSlots[i] = uqm::gameState().sis().ModuleSlots[m];
				uqm::gameState().sis().ModuleSlots[m] = EmptySlot + 2;
			}
		}

		/* install Chmmr-supplied modules */
		for (i = 0; i < NumDriveSlots; ++i)
		{
			uqm::gameState().sis().DriveSlots[i] = FUSION_THRUSTER;
		}
		for (i = 0; i < NumJetSlots; ++i)
		{
			uqm::gameState().sis().JetSlots[i] = TURNING_JETS;
		}
		uqm::gameState().sis().ModuleSlots[0] = BOMB_MODULE_4;
		uqm::gameState().sis().ModuleSlots[1] = BOMB_MODULE_5;
		uqm::gameState().sis().ModuleSlots[2] = BOMB_MODULE_3;
		uqm::gameState().sis().ModuleSlots[3] = BOMB_MODULE_1;
		uqm::gameState().sis().ModuleSlots[4] = BOMB_MODULE_0;
		uqm::gameState().sis().ModuleSlots[5] = BOMB_MODULE_1;
		uqm::gameState().sis().ModuleSlots[6] = BOMB_MODULE_3;
		uqm::gameState().sis().ModuleSlots[7] = BOMB_MODULE_4;
		uqm::gameState().sis().ModuleSlots[8] = BOMB_MODULE_5;
		uqm::gameState().sis().ModuleSlots[9] = BOMB_MODULE_2;
	}
	else if (PlayerSaid(R, perhaps_not_install))
	{
		NPCPhrase(YES);

		NPCPhrase(GOODBYE);
	}
}

static void NotReady(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(RETURN_WHEN_READY);
	}
	else if (PlayerSaid(R, further_assistance))
	{
		NPCPhrase(NO_FURTHER_ASSISTANCE);

		DisablePhrase(further_assistance);
	}
	else if (PlayerSaid(R, tech_help))
	{
		NPCPhrase(USE_OUR_SHIPS_BEFORE);

		SetRaceAllied(CHMMR_SHIP, true);
	}
	else if (PlayerSaid(R, where_weapon))
	{
		NPCPhrase(PRECURSOR_WEAPON);

		DisablePhrase(where_weapon);
	}
	else if (PlayerSaid(R, where_distraction))
	{
		NPCPhrase(PSYCHIC_WEAPONRY);

		DisablePhrase(where_distraction);
	}

	if (CheckAlliance(CHMMR_SHIP) != GOOD_GUY)
	{
		Response(tech_help, NotReady);
	}
	else if (PhraseEnabled(further_assistance))
	{
		Response(further_assistance, NotReady);
	}
	if (PhraseEnabled(where_weapon) && !uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
	{
		Response(where_weapon, NotReady);
	}
	if (PhraseEnabled(where_distraction) && !uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		Response(where_distraction, NotReady);
	}
	Response(bye, ExitConversation);
}

static void ImproveBomb(RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase(WE_WILL_IMPROVE_BOMB);
	}
	else if (PlayerSaid(R, what_now))
	{
		NPCPhrase(MODIFY_VESSEL);

		DisablePhrase(what_now);
	}
	else if (PlayerSaid(R, wont_hurt_my_ship))
	{
		NPCPhrase(WILL_DESTROY_IT);

		DisablePhrase(wont_hurt_my_ship);
	}
	else if (PlayerSaid(R, bummer_about_my_ship))
	{
		NPCPhrase(DEAD_SILENCE);

		DisablePhrase(bummer_about_my_ship);
	}
	else if (PlayerSaid(R, other_assistance))
	{
		NPCPhrase(USE_OUR_SHIPS_AFTER);

		SetRaceAllied(CHMMR_SHIP, true);
	}

	if (PhraseEnabled(what_now))
	{
		Response(what_now, ImproveBomb);
	}
	else if (PhraseEnabled(wont_hurt_my_ship))
	{
		Response(wont_hurt_my_ship, ImproveBomb);
	}
	else if (PhraseEnabled(bummer_about_my_ship))
	{
		Response(bummer_about_my_ship, ImproveBomb);
	}
	if (CheckAlliance(CHMMR_SHIP) != GOOD_GUY)
	{
		Response(other_assistance, ImproveBomb);
	}
	Response(proceed, ExitConversation);
	Response(perhaps_not_install, ExitConversation);
}

static void ChmmrFree(RESPONSE_REF R)
{
	if (R == 0
		|| PlayerSaid(R, i_am_captain)
		|| PlayerSaid(R, i_am_savior)
		|| PlayerSaid(R, i_am_silly))
	{
		NPCPhrase(WHY_HAVE_YOU_FREED_US);
		AlienTalkSegue((uint16_t)~0);
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_EMERGING, static_cast<uint32_t>(0));

		Response(serious_1, ChmmrFree);
		Response(serious_2, ChmmrFree);
		Response(silly, ChmmrFree);
	}
	else
	{
		NPCPhrase(WILL_HELP_ANALYZE_LOGS);

		if (uqm::gameState().flag(uqm::GameFlag::AWARE_OF_SAMATRA))
		{
			NPCPhrase(YOU_KNOW_SAMATRA);
		}
		else
		{
			NPCPhrase(DONT_KNOW_ABOUT_SAMATRA);

			uqm::gameState().setFlag(uqm::GameFlag::AWARE_OF_SAMATRA, static_cast<uint32_t>(1));
		}

		if (uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
		{
			NPCPhrase(HAVE_TALKING_PET);
		}
		else
		{
			NPCPhrase(NEED_DISTRACTION);
		}

		if (uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
		{
			NPCPhrase(HAVE_BOMB);
		}
		else
		{
			NPCPhrase(NEED_WEAPON);
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
	if (PlayerSaid(R, need_advice))
	{
		NPCPhrase(WHAT_ADVICE);
	}
	else if (PlayerSaid(R, how_defeat_urquan))
	{
		NPCPhrase(DEFEAT_LIKE_SO);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_BOMB_STATE, static_cast<uint32_t>(1));
		DisablePhrase(how_defeat_urquan);
	}
	else if (PlayerSaid(R, what_about_tpet))
	{
		NPCPhrase(SCARY_BUT_USEFUL);

		DisablePhrase(what_about_tpet);
	}
	else if (PlayerSaid(R, what_about_bomb))
	{
		NPCPhrase(ABOUT_BOMB);

		DisablePhrase(what_about_bomb);
	}
	else if (PlayerSaid(R, what_about_sun_device))
	{
		NPCPhrase(ABOUT_SUN_DEVICE);

		DisablePhrase(what_about_sun_device);
	}
	else if (PlayerSaid(R, what_about_samatra))
	{
		NPCPhrase(ABOUT_SAMATRA);

		DisablePhrase(what_about_samatra);
	}

	uint8_t AdviceLeft {0};
	if (PhraseEnabled(how_defeat_urquan))
	{
		Response(how_defeat_urquan, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(what_about_tpet) && uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP))
	{
		Response(what_about_tpet, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(what_about_bomb) && uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP))
	{
		Response(what_about_bomb, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(what_about_sun_device) && uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE_ON_SHIP))
	{
		Response(what_about_sun_device, ChmmrAdvice);
		AdviceLeft = true;
	}
	if (PhraseEnabled(what_about_samatra) && uqm::gameState().flag(uqm::GameFlag::AWARE_OF_SAMATRA))
	{
		Response(what_about_samatra, ChmmrAdvice);
		AdviceLeft = true;
	}
	Response(enough_advice, ChmmrShielded);

	if (!AdviceLeft)
	{
		DisablePhrase(need_advice);
	}
}

static void ChmmrShielded(RESPONSE_REF R)
{
	if (PlayerSaid(R, find_out_whats_up))
	{
		NPCPhrase(HYBRID_PROCESS);

		DisablePhrase(find_out_whats_up);
	}
	else if (PlayerSaid(R, need_help))
	{
		NPCPhrase(CANT_HELP);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(1));
	}
	else if (PlayerSaid(R, why_no_help))
	{
		NPCPhrase(LONG_TIME);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(2));
	}
	else if (PlayerSaid(R, what_if_more_energy))
	{
		NPCPhrase(DANGER_TO_US);

		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(3));
	}
	else if (PlayerSaid(R, enough_advice))
	{
		NPCPhrase(OK_ENOUGH_ADVICE);
	}

	switch (uqm::gameState().flag(uqm::GameFlag::CHMMR_STACK))
	{
		case 0:
			Response(need_help, ChmmrShielded);
			break;
		case 1:
			Response(why_no_help, ChmmrShielded);
			break;
		case 2:
			Response(what_if_more_energy, ChmmrShielded);
			break;
	}
	if (PhraseEnabled(find_out_whats_up))
	{
		Response(find_out_whats_up, ChmmrShielded);
	}
	if (PhraseEnabled(need_advice))
	{
		Response(need_advice, ChmmrAdvice);
	}
	Response(bye_shielded, ExitConversation);
}

static void AfterBomb(RESPONSE_REF R)
{
	if (PlayerSaid(R, whats_up_after_bomb))
	{
		if (uqm::gameState().flag(uqm::GameFlag::CHMMR_STACK))
		{
			NPCPhrase(GENERAL_INFO_AFTER_BOMB_2);
		}
		else
		{
			NPCPhrase(GENERAL_INFO_AFTER_BOMB_1);

			uqm::gameState().setFlag(uqm::GameFlag::CHMMR_STACK, static_cast<uint32_t>(1));
		}

		DisablePhrase(whats_up_after_bomb);
	}
	else if (PlayerSaid(R, what_do_after_bomb))
	{
		NPCPhrase(DO_AFTER_BOMB);

		DisablePhrase(what_do_after_bomb);
	}

	if (PhraseEnabled(whats_up_after_bomb))
	{
		Response(whats_up_after_bomb, AfterBomb);
	}
	if (PhraseEnabled(what_do_after_bomb))
	{
		Response(what_do_after_bomb, AfterBomb);
	}
	Response(bye_after_bomb, ExitConversation);
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
				NPCPhrase(HELLO_AFTER_BOMB_1);
				break;
			case 1:
				NPCPhrase(HELLO_AFTER_BOMB_2);
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
			NPCPhrase(YOU_ARE_READY);

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
					NPCPhrase(WHY_YOU_HERE_1);
					break;
				case 1:
					NPCPhrase(WHY_YOU_HERE_2);
					break;
				case 2:
					NPCPhrase(WHY_YOU_HERE_3);
					break;
				case 3:
					NPCPhrase(WHY_YOU_HERE_4);
					--NumVisits;
					break;
			}

			ChmmrShielded((RESPONSE_REF)0);
		}
		else
		{
			SetCommIntroMode(CIM_FADE_IN_SCREEN, c_gameSecond * 2);
			NPCPhrase(WE_ARE_FREE);

			if (NumVisits)
			{
				ChmmrFree((RESPONSE_REF)0);
				NumVisits = 0;
			}
			else
			{
				NPCPhrase(WHO_ARE_YOU);

				Response(i_am_captain, ChmmrFree);
				Response(i_am_savior, ChmmrFree);
				Response(i_am_silly, ChmmrFree);
			}

			uqm::gameState().setFlag(uqm::GameFlag::CHMMR_UNLEASHED, static_cast<uint32_t>(1));

			// Setup a Chmmr sphere-of-influence, now that they're out
			// of their shell.  EncounterPercent for the Chmmr is 0,
			// so this is purely decorative.
			if (IsExtended())
			{
				HFLEETINFO hChmmr = GetStarShipFromIndex(
					&uqm::gameState().game().avail_race_q, CHMMR_SHIP);
				FleetInfo* ChmmrPtr = LockFleetInfo(
					&uqm::gameState().game().avail_race_q, hChmmr);

				if (ChmmrPtr)
				{
					ChmmrPtr->actual_strength =
						986 / SphereRadiusIncrement * 2;
					ChmmrPtr->loc = SeedFleetLocation(ChmmrPtr, plot_map,
													  HOME);
					StartSphereTracking(CHMMR_SHIP);
				}
				UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hChmmr);
			}
		}
		uqm::gameState().setFlag(uqm::GameFlag::CHMMR_HOME_VISITS, static_cast<uint32_t>(NumVisits));

		SetHomeworldKnown(CHMMR_HOME);
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

	setSegue(Segue_peace);
	ConversationData* retval {&chmmr_desc};

	return (retval);
}
