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

#include "encount.h"

#include "battle.h"
#include "battlecontrols.h"
#include "build.h"
#include "colors.h"
#include "starmap.h"
#include "cons_res.h"
#include "controls.h"
#include "menustat.h"
#include "gameopt.h"
#include "gamestr.h"
#include "gendef.h"
#include "globdata.h"
#include "sis.h"
// for DrawStatusMessage(), SetStatusMessageMode()
#include "init.h"
#include "pickship.h"
#include "intel.h"
#include "nameref.h"
#include "resinst.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "util.h" // JMS: For SaveContextFrame()
#include "libs/graphics/gfx_common.h"
#include "core/log/log.h"
#include "core/string/Utf8.h"
#include "libs/mathlib.h"
#include "libs/inplib.h"
#include "core/misc/FatalExit.h"
#include "planets/solarsys.h"
#include "starbase.h"
#include "uqm/random/UQMRandom.h"


static void DrawFadeText(const char* str1, const char* str2,
						 bool fade_in, GfxRect* pRect);


static bool DoSelectAction(MenuState* pMS)
{
	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
	if (uqm::currentActivity() & CHECK_ABORT)
	{
		pMS->CurState = ATTACK + 1;
		return (false);
	}
	if (!pMS->Initialized)
	{
		pMS->Initialized = true;
		pMS->InputFunc = DoSelectAction;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case HAIL:
			case ATTACK:
				DrawMenuStateStrings(PM_CONVERSE, pMS->CurState);
				if (uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE)
				{
					pMS->CurState = HAIL;
				}
				return (false);
			case ATTACK + 1:
				// Clearing FlashRect is not necessary
				if (!GameOptions())
				{
					return false;
				}
				DrawMenuStateStrings(PM_CONVERSE, pMS->CurState);
				SetFlashRect(SfrMenu3do(), false);
				break;
			default:
				fmt::print("Unknown option: {}\n", pMS->CurState);
		}
	}
	DoMenuChooser(pMS, PM_CONVERSE);
	return (true);
}

static Queue* GetShipFragQueueForPlayer(uint16_t playerNr)
{
	if (playerNr == RpgPlayerNum)
	{
		return &uqm::gameState().game().built_ship_q;
	}
	else
	{
		return &uqm::gameState().game().npc_built_ship_q;
	}
}

void SetBattlePlanet(void)
{
	uint8_t selector {(uint8_t)((uint16_t)uqm::random().next() % NUMBER_OF_PLANET_TYPES)};

	if (IsExtended() && (selector == RAINBOW_WORLD || selector == SHATTERED_WORLD))
	{ // No rainbow or shattered worlds in hyperspace
		if (selector == RAINBOW_WORLD)
		{
			selector--;
		}
		if (selector == SHATTERED_WORLD)
		{
			selector++;
		}
	}

	// Set BATTLE_PLANET state so it can be serialized if game is saved
	uqm::gameState().setFlag(uqm::GameFlag::BATTLE_PLANET, static_cast<uint32_t>(selector));
}

// Called by comm code to intialize battle fleets during Encounter
void BuildBattle(uint16_t which_player)
{
	const Queue* pQueue {};
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};
	HSTARSHIP hBuiltShip {};
	Starship* BuiltShipPtr {};

	EncounterRace = -1;

	if (GetHeadLink(&uqm::gameState().game().npc_built_ship_q) == 0)
	{
		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_SEGUE, static_cast<uint32_t>(0));
		return;
	}

	if (which_player != RpgPlayerNum)
	{ // This function is called first for the NPC character
		// and this is when a centerpiece is loaded
		switch (uqm::lowByte(uqm::currentActivity()))
		{
			case IN_LAST_BATTLE:
				load_gravity_well(SA_MATRA);
				break;
			case IN_HYPERSPACE:
				{ // If somehow BATTLE_PLANET is invalid (maybe Core game load missalign)
					if (uqm::gameState().flag(uqm::GameFlag::BATTLE_PLANET) > NUMBER_OF_PLANET_TYPES)
					{
						SetBattlePlanet();
					}

					load_gravity_well(uqm::gameState().flag(uqm::GameFlag::BATTLE_PLANET));
					break;
				}
			default:
				{
					uqm::gameState().setFlag(uqm::GameFlag::ESCAPE_COUNTER, static_cast<uint32_t>(110));

					if (IsExtended() && pSolarSysState && worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
					{ // Set gravity well to moon if Encounter takes
						// place there (Spathiwa moon, Taalo HW,
						// Utwig Bomb Loc)
						// Only if Encounter starts with moon collision
						// colliding with IP group in inner system still
						// uses main planet for gravity well

						const uint8_t selector = pSolarSysState->MoonDesc[moonIndex(
																			  pSolarSysState, pSolarSysState->pOrbitalDesc)]
													 .data_index;

						if (selector == SA_MATRA)
						{
							uqm::utf8StringCopy(uqm::gameState().sis().PlanetName,
												sizeof(uqm::gameState().sis().PlanetName),
												GameString(PLANET_NUMBER_BASE + 32));
							DrawSISTitle(
								GameString(PLANET_NUMBER_BASE + 32));
						}

						load_gravity_well(selector);
						break;
					}
					else
					{
						load_gravity_well(uqm::gameState().flag(uqm::GameFlag::BATTLE_PLANET));
						break;
					}
				}
		}
	}

	pQueue = GetShipFragQueueForPlayer(which_player);

	ReinitQueue(&race_q[which_player]);
	for (hStarShip = GetHeadLink(pQueue);
		 hStarShip != 0; hStarShip = hNextShip)
	{
		const ShipFragment* FragPtr {LockShipFrag(pQueue, hStarShip)};
		hNextShip = _GetSuccLink(FragPtr);

		hBuiltShip = Build(&race_q[which_player],
						   FragPtr->race_id == SAMATRA_SHIP ?
							   SA_MATRA_ID :
							   FragPtr->SpeciesID);
		if (hBuiltShip)
		{
			BuiltShipPtr = LockStarShip(&race_q[which_player], hBuiltShip);
			BuiltShipPtr->captains_name_index = FragPtr->captains_name_index;
			BuiltShipPtr->playerNr = which_player;
			if (FragPtr->crew_level != InfiniteFleet)
			{
				BuiltShipPtr->crew_level = FragPtr->crew_level;
			}
			else /* if infinite ships */
			{
				BuiltShipPtr->crew_level = FragPtr->max_crew;
			}
			BuiltShipPtr->max_crew = FragPtr->max_crew;
			BuiltShipPtr->race_strings = FragPtr->race_strings;
			BuiltShipPtr->icons = FragPtr->icons;
			BuiltShipPtr->index = FragPtr->index;
			BuiltShipPtr->ship_cost = 0;
			BuiltShipPtr->RaceDescPtr = 0;

			UnlockStarShip(&race_q[which_player], hBuiltShip);
		}

		UnlockShipFrag(pQueue, hStarShip);
	}

	if (which_player == RpgPlayerNum
		&& (hBuiltShip = Build(&race_q[0], SIS_SHIP_ID)))
	{
		BuiltShipPtr = LockStarShip(&race_q[0], hBuiltShip);
		BuiltShipPtr->captains_name_index = 0;
		BuiltShipPtr->playerNr = RpgPlayerNum;
		BuiltShipPtr->crew_level = 0;
		BuiltShipPtr->max_crew = 0;
		// Crew will be copied directly from
		// GLOBAL_SIS (CrewEnlisted) later.
		BuiltShipPtr->race_strings = 0;
		BuiltShipPtr->icons = 0;
		BuiltShipPtr->index = -1;
		BuiltShipPtr->ship_cost = 0;
		BuiltShipPtr->energy_counter = MaxEnergySize;
		BuiltShipPtr->RaceDescPtr = 0;
		UnlockStarShip(&race_q[0], hBuiltShip);
	}
}

bool FleetIsInfinite(uint16_t playerNr)
{
	const Queue* pQueue {GetShipFragQueueForPlayer(playerNr)};
	HSHIPFRAG hShipFrag {GetHeadLink(pQueue)};
	if (!hShipFrag)
	{ // Ship queue is empty in SuperMelee or for RPG player w/o escorts
		return false;
	}

	const ShipFragment* FragPtr {LockShipFrag(pQueue, hShipFrag)};
	const bool ret {(FragPtr->crew_level == InfiniteFleet)};
	UnlockShipFrag(pQueue, hShipFrag);

	return ret;
}

void UpdateShipFragCrew(Starship* StarShipPtr)
{
	HSHIPFRAG hShipFrag {};
	HSHIPFRAG hNextFrag {};
	ShipFragment* frag {};
	HSTARSHIP hStarShip {};
	HSTARSHIP hNextShip {};
	const Starship* ship {};

	const Queue* frag_q {GetShipFragQueueForPlayer(StarShipPtr->playerNr)};
	const Queue* ship_q {&race_q[StarShipPtr->playerNr]};

	// Find a ShipFragment that corresponds to the given Starship
	// The ships and fragments are in the same order in two queues
	// XXX: It would probably be simpler to keep HSHIPFRAG in Starship struct
	for (hShipFrag = GetHeadLink(frag_q), hStarShip = GetHeadLink(ship_q);
		 hShipFrag != 0 && hStarShip != 0;
		 hShipFrag = hNextFrag, hStarShip = hNextShip)
	{
		ship = LockStarShip(ship_q, hStarShip);
		hNextShip = _GetSuccLink(ship);
		frag = LockShipFrag(frag_q, hShipFrag);
		hNextFrag = _GetSuccLink(frag);

		if (ship == StarShipPtr)
		{
			assert(frag->crew_level != InfiniteFleet);

			// Record crew left after the battle */
			frag->crew_level = ship->crew_level;

			UnlockShipFrag(frag_q, hShipFrag);
			UnlockStarShip(ship_q, hStarShip);
			break;
		}

		UnlockShipFrag(frag_q, hShipFrag);
		UnlockStarShip(ship_q, hStarShip);
	}
}

/*
 * Encountering an alien.
 * Draws the Encounter screen, plays the red alert music, and
 * waits for a decision of the player on how to handle the situation.
 * Returns either HAIL or ATTACK.
 */
uint16_t InitEncounter(void)
{
	uint16_t i {};
	GfxStamp s;
	GfxText t;
	extern FRAME planet[];


	SetContext(SpaceContext);
	SetContextFont(MicroFont);

	MUSIC_REF MR {LoadMusicInstance(RedalertMusicRes)};
	PlayMusic(MR, false, 1);
	FRAME SegueFrame {GetStarBackGround(true)};
	WaitForSoundEnd(TfbsoundWaitAll);
	StopMusic();
	DestroyMusic(MR);
	s.origin.x = s.origin.y = 0;

	SetTransitionSource(nullptr);
	BatchGraphics();

	SetContextBackGroundColor(BlackColor);
	ClearDrawable();
	s.frame = SegueFrame;
	DrawStamp(&s);

	//    t.baseline.x = SisScreenWidth() >> 1;
	t.baseline.x = ResScale(OrigSisScreenWidth() >> 1);
	t.baseline.y = ResScale(10);
	t.align = TextAlign::Center;

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01));
	if (inHQSpace())
	{
		t.pStr = GameString(ENCOUNTER_STRING_BASE + 0);
		// "ENCOUNTER IN"
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += ResScale(12);
		t.pStr = GameString(ENCOUNTER_STRING_BASE + 1);
		// "DEEP SPACE"
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
	else
	{
		char buf[256];

		t.pStr = GameString(ENCOUNTER_STRING_BASE + 2);
		// "ENCOUNTER AT"
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += ResScale(12);
		GetClusterName(CurStarDescPtr(), buf);
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += ResScale(12);
		t.pStr = uqm::gameState().sis().PlanetName;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
	DrawSISMessage(nullptr);

	s.origin.x = ResScale(OrigSisScreenWidth() >> 1);
	s.origin.y = SisScreenHeight() >> 1;
	s.frame = planet[0];
	DrawStamp(&s);

	if (uqm::lowByte(uqm::currentActivity()) != IN_LAST_BATTLE)
	{
		HSHIPFRAG hStarShip {};
		HSHIPFRAG hNextShip {};
		const GfxPoint display_pt[] =
			{
				{10,	 51 },
				{-10, 51 },
				{33,	 40 },
				{-33, 40 },
				{49,	 18 },
				{-49, 18 },
				{52,	 -6 },
				{-52, -6 },
				{44,	 -27},
				{-44, -27},
		};
		constexpr auto NumDisplayPts {(std::size(display_pt))};

		for (hStarShip = GetHeadLink(&uqm::gameState().game().npc_built_ship_q), i = 0;
			 hStarShip && i < 60; hStarShip = hNextShip, ++i)
		{
			GfxRect r;

			const ShipFragment* FragPtr {LockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip)};
			if (FragPtr->crew_level != InfiniteFleet)
			{
				hNextShip = _GetSuccLink(FragPtr);
			}
			else /* if infinite ships */
			{
				hNextShip = hStarShip;
			}

			s.origin = display_pt[i % NumDisplayPts];
			if (i >= NumDisplayPts)
			{
				uint16_t radius {};

				radius = square_root((long)s.origin.x * s.origin.x
									 + (long)s.origin.y * s.origin.y)
					   + ((i / NumDisplayPts) * 18);

				radius <<= resolutionFactor;

				const uint16_t angle {ARCTAN(s.origin.x, s.origin.y)};
				s.origin.x = (Cosine(angle, radius));
				s.origin.y = (Sine(angle, radius));
			}
			else
			{
				s.origin.x <<= resolutionFactor;
				s.origin.y <<= resolutionFactor;
			}
			s.frame = SetAbsFrameIndex(FragPtr->icons, 0);
			GetFrameRect(s.frame, &r);
			s.origin.x += ResScale(OrigSisScreenWidth() >> 1) - (r.extent.width >> 1);
			s.origin.y += ResScale(OrigSisScreenHeight() >> 1) - (r.extent.height >> 1);
			DrawStamp(&s);

			UnlockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip);
		}
	}

	UnbatchGraphics();
	DestroyDrawable(ReleaseDrawable(SegueFrame));
	ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);


	{
		MenuState menuState;

		menuState.InputFunc = DoSelectAction;
		menuState.Initialized = false;

		DrawMenuStateStrings(PM_CONVERSE, menuState.CurState = HAIL);
		SetFlashRect(SfrMenu3do(), false);

		DoInput(&menuState, true);

		SetFlashRect(nullptr, false);

		return (menuState.CurState);
	}
}

//static GfxStamp
//SetTextFrameRect (const char* str1, const char* str2, GfxRect* pRect)
//{
//	GfxStamp s;
//	GfxText t1, t2;
//	GfxRect r1, r2, res;
//
//	t1.baseline.x = pRect->corner.x + ResScale(100);
//	t1.baseline.y = pRect->corner.y + ResScale(45);
//	t1.align = ALIGN_CENTER;
//	t1.pStr = str1;
//	t1.CharCount = (uint16_t)~0;
//	t2 = t1;
//	t2.baseline.y += ResScale(11);
//	t2.pStr = str2;
//
//	TextRect (&t1, &r1, nullptr);
//	TextRect (&t2, &r2, nullptr);
//
//	// Take the closest X to the origin point.
//	res.corner.x = (r1.corner.x < r2.corner.x ? r1.corner.x : r2.corner.x);
//	// r1 always on top
//	res.corner.y = r1.corner.y;
//
//	// Take the widest extent
//	res.extent.width = (r1.extent.width > r2.extent.width ? r1.extent.width : r2.extent.width);
//	// Height is the gap between texts plus height of the second one
//	res.extent.height = r2.corner.y - r1.corner.y + r2.extent.height;
//
//	s = SaveContextFrame(&res);
//
//	return s;
//}

static void DrawFadeText(const char* str1, const char* str2, bool fade_in,
						 GfxRect* pRect)
{
	TimeCount TimeIn;
	GfxText t1;
	GfxText t2;
	GfxRect r1;
	GfxRect r2;
	const auto& fade_cycle {ScavengeTextColorTable};
	constexpr auto NumFades {(std::size(fade_cycle))};

	t1.baseline.x = pRect->corner.x + ResScale(100);
	t1.baseline.y = pRect->corner.y + ResScale(45);
	t1.align = TextAlign::Center;
	t1.pStr = str1;
	t1.CharCount = (uint16_t)~0;
	t2 = t1;
	t2.baseline.y += ResScale(11);
	t2.pStr = str2;

	TextRect(&t1, &r1, nullptr);
	TextRect(&t2, &r2, nullptr);

	FlushInput();
	TimeIn = GetTimeCounter();

	for (int16_t i {0}; i < (int16_t)NumFades; ++i)
	{
		if (AnyButtonPress(true))
		{
			i = NumFades - 1;
		}

		if (IsHd())
		{
			RepairPickFrame(&r1, 1);
			RepairPickFrame(&r2, 1);
		}

		SetContextForeGroundColor(
			fade_cycle[fade_in ? i : (int16_t)(NumFades - i - 1)]);
		font_DrawText(&t1);
		font_DrawText(&t2);
		SleepThreadUntil(TimeIn + (c_gameSecond / 20));
		TimeIn = GetTimeCounter();
	}
	if (!fade_in)
	{ // Clear the remaining text
		if (IsHd())
		{
			RepairPickFrame(&r1, 1);
			RepairPickFrame(&r2, 1);
		}
		else
		{
			SetContextForeGroundColor(BuildShadeRgba(0x50));
			DrawFilledRectangle(&r1);
			DrawFilledRectangle(&r2);
		}
	}
}

static void ClearRectBack(GfxRect* pRect)
{
	if (IsHd())
	{
		RepairPickFrame(pRect, 1);
	}
	else
	{
		DrawFilledRectangle(pRect);
	}
}

static constexpr auto MaxDeadDisplayed {5};

uint16_t UninitEncounter(void)
{
	uint16_t ships_killed {0};

	free_gravity_well();

	if ((uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
		|| uqm::gameState().sis().CrewEnlisted == (uint16_t)~0
		|| uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE
		|| uqm::lowByte(uqm::currentActivity()) == WON_LAST_BATTLE)
	{
		goto ExitUninitEncounter;
	}

	if (uqm::gameState().flag(uqm::GameFlag::BATTLE_SEGUE) == 0)
	{
		ReinitQueue(&race_q[0]);
		ReinitQueue(&race_q[1]);
	}
	else
	{
		bool Sleepy {};
		int16_t VictoryState {};
		int16_t i {};
		uint16_t RecycleAmount = 0;
		GfxRect r;
		GfxRect scavenge_r = {
			{0, 0},
			{0, 0}
		  };
		GfxText t;
		GfxStamp ship_s;
		const char* str1 = nullptr;
		const char* str2 = nullptr;
		StatMsgMode prevMsgMode = SMM_UNDEFINED;
		char buf[80] {};
		const ShipFragment* FragPtr {};
		const auto& fade_ship_cycle {ScavengeScreenColorTable};
		constexpr auto NumShipFades {(std::size(fade_ship_cycle))};

		uqm::gameState().setFlag(uqm::GameFlag::BATTLE_SEGUE, static_cast<uint32_t>(0));
		uqm::gameState().setFlag(uqm::GameFlag::BOMB_CARRIER, static_cast<uint32_t>(0));

		VictoryState = (battle_counter[1] || !battle_counter[0]
						|| uqm::gameState().flag(uqm::GameFlag::URQUAN_PROTECTING_SAMATRA)) ?
						   0 :
						   1;

		HSHIPFRAG hStarShip {GetHeadLink(&uqm::gameState().game().npc_built_ship_q)};
		FragPtr = LockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip);
		EncounterRace = FragPtr->race_id;
		if (GetStarShipFromIndex(&uqm::gameState().game().avail_race_q, EncounterRace) == 0)
		{
			/* Suppress the final tally and salvage info */
			VictoryState = -1;
			InitSISContexts();
		}
		UnlockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip);

		if (VictoryState > 0) // Set RU only when we get them
		{
			prevMsgMode = SetStatusMessageMode(SMM_RES_UNITS);
		}

		ship_s.origin.x = 0;
		ship_s.origin.y = 0;
		Sleepy = true;
		for (i = 0; i < NumSides; ++i)
		{
			Queue* pQueue {};
			HSHIPFRAG hNextShip {};

			if (i == 0)
			{
				pQueue = &uqm::gameState().game().built_ship_q;
			}
			else
			{
				if (VictoryState < 0)
				{
					VictoryState = 0;
				}
				else
				{
					DrawSISFrame();
					DrawSISMessage(nullptr);
					if (inHQSpace())
					{
						DrawHyperCoords(uqm::flagship().shipStamp.origin);
					}
					else if (uqm::flagship().ipPlanet == 0)
					{
						DrawHyperCoords(CurStarDescPtr()->star_pt);
					}
					else
					{
						DrawSISTitle(uqm::gameState().sis().PlanetName);
					}

					SetContext(SpaceContext);
					if (VictoryState)
					{
						InitPickFrame();
						DrawArmadaPickShip(true, &scavenge_r);
					}
				}
				pQueue = &uqm::gameState().game().npc_built_ship_q;
			}

			ReinitQueue(&race_q[(NumSides - 1) - i]);

			for (hStarShip = GetHeadLink(pQueue); hStarShip;
				 hStarShip = hNextShip)
			{
				FragPtr = LockShipFrag(pQueue, hStarShip);
				hNextShip = _GetSuccLink(FragPtr);

				if (FragPtr->crew_level == 0
					|| (VictoryState && i == NumSides - 1))
				{
					if (i == NumSides - 1)
					{
						++ships_killed;
						if (VictoryState)
						{
							uint16_t j {};

							if (ships_killed == 1)
							{
								RecycleAmount = 0;

								DrawStatusMessage(nullptr);

								ship_s.origin.x = scavenge_r.corner.x + ResScale(32);
								ship_s.origin.y = scavenge_r.corner.y + ResScale(56);
								ship_s.frame = IncFrameIndex(FragPtr->icons);
								DrawStamp(&ship_s);
								SetContextForeGroundColor(MdkgrayColor);

								SetContextFont(isPC(uqm::UQMOptions::read().whichFonts) ? TinyFont : TinyFontBold);

								uqm::utf8StringCopy(buf, sizeof buf,
													uqm::toUpperUtf8(FragPtr->race_strings->text(0)).c_str());

								t.baseline.x = scavenge_r.corner.x + ResScale(100);
								t.baseline.y = scavenge_r.corner.y + ResScale(68);
								t.align = TextAlign::Center;
								t.pStr = buf;
								t.CharCount = (uint16_t)~0;
								font_DrawText(&t);
								t.baseline.y += ResScale(6);
								t.pStr = GameString(
									ENCOUNTER_STRING_BASE + 3);
								// "BATTLE GROUP"
								t.CharCount = (uint16_t)~0;
								font_DrawText(&t);

								ship_s.frame = FragPtr->icons;

								SetContextFont(MicroFont);

								str1 = GameString(
									ENCOUNTER_STRING_BASE + 4);
								// "Enemy Ships"
								str2 = GameString(
									ENCOUNTER_STRING_BASE + 5);
								// "Destroyed"

								DrawFadeText(str1, str2, true, &scavenge_r);
							}

							r.corner.y = scavenge_r.corner.y + ResScale(9);
							r.extent.height = ResScale(22);

							SetContextForeGroundColor(BlackColor);

							r.extent.width = ResScale(34);
							r.corner.x = scavenge_r.corner.x + scavenge_r.extent.width
									   - (ResScale(10) + r.extent.width);
							BatchGraphics(); // to avoid blinking text

							// Back of the RU counter
							ClearRectBack(&r);

							/* collect bounty ResUnits */
							j = ShipCost(EncounterRace) >> 3;
							if (EncounterRace == SLYLANDRO_SHIP)
							{
								j = 550;
							}
							if (EncounterRace == MELNORME_SHIP)
							{
								j = j * 2;
							}

							RecycleAmount += j;
							fmt::format_to_n(buf, sizeof(buf) - 1, "{}", RecycleAmount);
							t.baseline.x =
								r.corner.x + r.extent.width
								- ResScale(1);
							t.baseline.y = r.corner.y + ResScale(14);
							t.align = TextAlign::Right;
							t.pStr = buf;
							t.CharCount = (uint16_t)~0;
							SetContextForeGroundColor(
								BuildColor(
									MakeRgb15(0x00, 0x00, 0x18), 0x50));
							font_DrawText(&t);
							UnbatchGraphics();
							DeltaSISGauges(0, 0, j);

							if ((VictoryState++ - 1) % MaxDeadDisplayed)
							{
								ship_s.origin.x += ResScale(17);
							}
							else
							{
								const GfxRect textRect = font_GetTextRect(&t);
								SetContextForeGroundColor(BlackColor);

								r.corner.x =
									scavenge_r.corner.x
									+ ResScale(10);
								r.extent.width =
									ResScale(132)
									- (textRect.extent.width
									   + ResScale(1));

								// Pause between erasing
								if (Sleepy)
								{
									SleepThread(c_gameSecond / 15);
								}

								// MaxDeadDisplayed reached - erase
								ClearRectBack(&r);

								ship_s.origin.x =
									r.corner.x + ResScale(2);
								ship_s.origin.y =
									scavenge_r.corner.y
									+ ResScale(12);
							}

							if (Sleepy)
							{
								TimeCount Time = GetTimeCounter();
								for (j = 0; j < NumShipFades; ++j)
								{
									Sleepy = (bool)!AnyButtonPress(true) && !(uqm::currentActivity() & CHECK_ABORT);

									if (IsHd())
									{ // Back of the current ship
										GfxPoint p;
										GfxRect ship_r;

										GetFrameRect(ship_s.frame, &ship_r);
										p = GetFrameHot(ship_s.frame);

										ship_r.corner = ship_s.origin;
										ship_r.corner.x -= p.x;
										ship_r.corner.y -= p.y;
										RepairPickFrame(&ship_r, 1);
									}

									if (!Sleepy)
									{
										break;
									}

									SetContextForeGroundColor(fade_ship_cycle[j]);
									DrawFilledStamp(&ship_s);

									SleepThreadUntil(Time + (c_gameSecond / 15));
									Time = GetTimeCounter();
								}
							}

							DrawStamp(&ship_s);
						}
					}

					UnlockShipFrag(pQueue, hStarShip);
					RemoveQueue(pQueue, hStarShip);
					FreeShipFrag(pQueue, hStarShip);

					continue;
				}

				UnlockShipFrag(pQueue, hStarShip);
			}
		}
		if (prevMsgMode) // Set Status back only when we get RU
		{
			SetStatusMessageMode(prevMsgMode);
		}

		if (VictoryState)
		{
#ifdef NEVER
			DestroyDrawable(ReleaseDrawable(s.frame));
#endif /* NEVER */

			WaitForAnyButton(true, c_gameSecond * 3, false);
			if (!CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_ESCAPE])
			{
				DrawFadeText(str1, str2, false, &scavenge_r);
				if (!CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_ESCAPE])
				{
					SetContextForeGroundColor(BlackColor);
					r.corner.x = scavenge_r.corner.x + ResScale(10);
					r.extent.width = ResScale(132);

					// The whole stat stripe
					ClearRectBack(&r);

					fmt::format_to_n(buf, sizeof(buf) - 1, "{} {}", RecycleAmount,
									 GameString(STATUS_STRING_BASE + 1)); // "RU"
					t.baseline.x = r.corner.x + (r.extent.width >> 1);
					t.baseline.y = r.corner.y + ResScale(14);
					t.align = TextAlign::Center;
					t.pStr = buf;
					t.CharCount = (uint16_t)~0;
					SetContextForeGroundColor(
						BuildColor(MakeRgb15(0x00, 0x00, 0x18), 0x50));
					font_DrawText(&t);


					str1 = GameString(ENCOUNTER_STRING_BASE + 6);
					// "Debris"
					str2 = GameString(ENCOUNTER_STRING_BASE + 7);
					// "Scavenged"

					DrawFadeText(str1, str2, true, &scavenge_r);
					WaitForAnyButton(true, c_gameSecond * 2, false);
					if (!CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_ESCAPE])
					{
						DrawFadeText(str1, str2, false, &scavenge_r);
					}
				}
			}
			DrawStatusMessage(nullptr);
			DestroyPickFrame();
		}

		if (ships_killed && EncounterRace == THRADDASH_SHIP
			&& !uqm::gameState().flag(uqm::GameFlag::THRADD_MANNER))
		{
			if ((ships_killed += uqm::gameState().flag(uqm::GameFlag::THRADDASH_BODY_COUNT)) > THRADDASH_BODY_THRESHOLD)
			{
				ships_killed = THRADDASH_BODY_THRESHOLD;
			}
			uqm::gameState().setFlag(uqm::GameFlag::THRADDASH_BODY_COUNT, static_cast<uint32_t>(ships_killed));
		}

		if (uqm::UQMOptions::read().slaughterMode && CheckSphereTracking((RACE_ID)EncounterRace)
			&& ships_killed)
		{
			HFLEETINFO hEncounter {};
			FleetInfo* EncounterPtr {};
			bool isBanned = false;
			const uint8_t bannedShip[7] =
				{
					PKUNK_SHIP,
					SHOFIXTI_SHIP,
					THRADDASH_SHIP,
					YEHAT_SHIP,
					MELNORME_SHIP,
					ILWRATH_SHIP,
					SLYLANDRO_SHIP};

			for (uint8_t j {0}; j < std::size(bannedShip); j++)
			{
				if (bannedShip[j] == EncounterRace)
				{
					isBanned = true;
					break;
				}
			}

			if (!isBanned)
			{
				hEncounter = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
												  EncounterRace);
				EncounterPtr =
					LockFleetInfo(&uqm::gameState().game().avail_race_q, hEncounter);

				if (EncounterPtr->actual_strength > 0)
				{
					int16_t actualStrength = EncounterPtr->actual_strength;

					actualStrength -= ships_killed;

					if (actualStrength <= 0)
					{
						EncounterPtr->actual_strength = 0;
						EncounterPtr->allied_state = DEAD_GUY;
					}
					else
					{
						EncounterPtr->actual_strength = actualStrength;
					}
				}

				UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hEncounter);
			}
		}
	}
ExitUninitEncounter:

	return (ships_killed);
}

void EncounterBattle(void)
{
	extern uint16_t nth_frame;
	InputContext* savedPlayerInput = nullptr;


	uqm::gameState().setFlag(uqm::GameFlag::BATTLE_SEGUE, static_cast<uint32_t>(1));

	const ACTIVITY OldActivity {uqm::currentActivity()};
	if (uqm::lowByte(OldActivity) == IN_LAST_BATTLE)
	{
		uqm::currentActivity() = uqm::makeWord(IN_LAST_BATTLE, 0);
	}
	else
	{
		uqm::currentActivity() = uqm::makeWord(IN_ENCOUNTER, 0);
	}

	//    FreeSC2Data ();
	//    DestroyFont (ReleaseFont (MicroFont));
	WaitForSoundEnd(TfbsoundWaitAll);
	//    DestroySound (ReleaseSound (MenuSounds));

	if (uqm::globalFlags() & CyborgEnabled)
	{
		uint8_t cur_speed {};

		cur_speed = (uint8_t)(uqm::globalFlags() & CombatSpeedMask)
				 >> CombatSpeedShift;
		if (cur_speed == 1)
		{
			cur_speed = 0; /* normal speed */
		}
		else if (cur_speed == 2)
		{
			++cur_speed; /* 4x speed, 3 of 4 frames skipped */
		}
		else /* if (cur_speed == 3) */
		{
			cur_speed = (uint8_t)~0; /* maximum speed - no rendering */
		}
		nth_frame = uqm::makeWord(1, cur_speed);
		PlayerControl[0] = PlayerControlFlags::Cyborg | PlayerControlFlags::DifficultyAwesome;
		savedPlayerInput = PlayerInput[0];
		PlayerInput[0] = nullptr;
		if (!SetPlayerInput(0))
		{
			uqm::log::critical("Could not set cyborg player input.");
			uqm::fatalExit(); // Does not return;
		}
	}

	if (isDifficulty(uqm::Difficulty::Easy))
	{
		PlayerControl[1] = PlayerControlFlags::Cyborg | PlayerControlFlags::DifficultyAwesome;
	}

	// PlayerControl[1] = PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard; // Yes, you can make Adventure mode 2-player

	GameSounds = uqm::loadSoundBank(GameSoundsRes).release();

	Battle(nullptr);

	delete GameSounds;
	GameSounds = 0;

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		uqm::gameState().sis().CrewEnlisted = (uint16_t)~0;
	}

	if (uqm::globalFlags() & CyborgEnabled)
	{
		nth_frame = uqm::makeWord(0, 0);
		PlayerControl[0] = PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard;
		ClearPlayerInput(0);
		PlayerInput[0] = savedPlayerInput;
	}

	//    MicroFont = CaptureFont (
	// LoadFont (MicroFontRes)
	// );
	//    MenuSounds = CaptureSound (LoadSound (MenuSoundsRes));
	//    LoadSC2Data ();

	uqm::currentActivity() = OldActivity;
}
