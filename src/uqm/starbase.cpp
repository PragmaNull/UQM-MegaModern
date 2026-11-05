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

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "starmap.h"
#include "comm.h"
#include "gamestr.h"
#include "save.h"
#include "starbase.h"
#include "sis.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "libs/graphics/gfx_common.h"
#include "libs/time/timelib.h"
#include "core/log/log.h"
#include "planets/planets.h"
#include <math.h>

#include "util.h"

static void CleanupAfterStarBase(void);

static void
DrawBaseStateStrings(STARBASE_STATE OldState, STARBASE_STATE NewState)
{
	TEXT t;
	const uint16_t text_base_y = RES_SCALE(106 + 28);
	const uint16_t text_spacing_y = RES_SCALE(23 - 4);

	SetContext(ScreenContext);
	SetContextFont(StarConFont);
	SetContextForeGroundColor(BlackColor);

	t.baseline.x = RES_SCALE(73 - 4) + SafeX();
	t.align = ALIGN_CENTER;

	if (OldState == (STARBASE_STATE)~0)
	{
		t.baseline.y = text_base_y + SafeY();
		for (OldState = TALK_COMMANDER; OldState < DEPART_BASE; ++OldState)
		{
			if (OldState != NewState)
			{
				t.pStr = GAME_STRING(STARBASE_STRING_BASE + 1 + OldState);
				t.CharCount = (uint16_t)~0;
				font_DrawText(&t);
			}
			t.baseline.y += text_spacing_y;
		}
	}

	t.baseline.y = text_base_y + SafeY() + (text_spacing_y * OldState);
	t.pStr = GAME_STRING(STARBASE_STRING_BASE + 1 + OldState);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E));
	t.baseline.y = text_base_y + SafeY() + (text_spacing_y * NewState);
	t.pStr = GAME_STRING(STARBASE_STRING_BASE + 1 + NewState);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
}

void DrawShipPiece(FRAME ModuleFrame, uint16_t which_piece, uint16_t which_slot,
				   bool DrawBluePrint)
{
	Color OldColor = UndefinedColor;
	// Initialisation is just to keep the compiler silent.
	GFXRECT r;
	STAMP Side;
	STAMP Top;

	int8_t RepairSlot {0};
	switch (which_piece)
	{
		case FUSION_THRUSTER:
		case EmptySlot + 0:
			Side.origin.x = DriveSideX();
			Side.origin.y = DriveSideY();
			Top.origin.x = DriveTopX();
			Top.origin.y = DriveTopY();
			break;
		case TURNING_JETS:
		case EmptySlot + 1:
			Side.origin.x = JetSideX();
			Side.origin.y = JetSideY();
			Top.origin.x = JetTopX();
			Top.origin.y = JetTopY();
			break;
		default:
			if (which_piece < EmptySlot + 2)
			{
				RepairSlot = 1;
				if (which_piece < EmptySlot
					&& (which_slot == 0
						|| GLOBAL_SIS(ModuleSlots[which_slot - 1]) < EmptySlot))
				{
					++RepairSlot;
				}
			}
			else if (!DrawBluePrint)
			{
				if (which_slot == 0 || which_slot >= NumModuleSlots - 3)
				{
					++which_piece;
				}

				if (which_slot < NumModuleSlots - 1
					&& GLOBAL_SIS(ModuleSlots[which_slot + 1]) < EmptySlot)
				{
					RepairSlot = -1;
					if (which_piece == EmptySlot + 3
						|| which_slot + 1 == NumModuleSlots - 3)
					{
						--RepairSlot;
					}
				}
			}
			Side.origin.x = ModuleSideX();
			Side.origin.y = ModuleSideY();
			Top.origin.x = ModuleTopX();
			Top.origin.y = ModuleTopY();
			break;
	}

	Side.origin.x += which_slot * ShipPieceOffset();
	Side.frame = nullptr;
	if (RepairSlot < 0)
	{
		Side.frame = SetAbsFrameIndex(ModuleFrame,
									  ((NUM_MODULES - 1) + (6 - 2)) + (NUM_MODULES + 6)
										  - (RepairSlot + 1));
		DrawStamp(&Side);
	}
	else if (RepairSlot)
	{
		r.corner = Side.origin;
		r.extent.width = ShipPieceOffset();
		r.extent.height = RES_SCALE(1);
		OldColor = SetContextForeGroundColor(BlackColor);
		if (!IsHd())
		{
			DrawFilledRectangle(&r);
		}
		r.corner.y += RES_SCALE(23 - 1);
		if (!IsHd())
		{
			DrawFilledRectangle(&r);
		}

		r.extent.width = RES_SCALE(1);
		r.extent.height = RES_SCALE(8);
		if (RepairSlot == 2)
		{
			r.corner = Side.origin;
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}
			r.corner.y += RES_SCALE(15);
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}
		}
		if (which_slot < (NumModuleSlots - 1))
		{
			r.corner = Side.origin;
			r.corner.x += ShipPieceOffset();
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}
			r.corner.y += RES_SCALE(15);
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}
		}
	}

	if (DrawBluePrint)
	{
		if (RepairSlot)
		{
			SetContextForeGroundColor(OldColor);
		}
		Side.frame = SetAbsFrameIndex(ModuleFrame, which_piece - 1);
		DrawFilledStamp(&Side);
	}
	else
	{
		Top.origin.x += which_slot * ShipPieceOffset();
		if (RepairSlot < 0)
		{
			Top.frame = SetRelFrameIndex(Side.frame, -((NUM_MODULES - 1) + 6));
			DrawStamp(&Top);
		}
		else if (RepairSlot)
		{
			r.corner = Top.origin;
			r.extent.width = ShipPieceOffset();
			r.extent.height = RES_SCALE(1);
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}
			r.corner.y += RES_SCALE(32 - 1);
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}

			r.extent.width = RES_SCALE(1);
			r.extent.height = RES_SCALE(12);
			if (RepairSlot == 2)
			{
				r.corner = Top.origin;
				if (!IsHd())
				{
					DrawFilledRectangle(&r);
				}
				r.corner.y += RES_SCALE(20);
				if (!IsHd())
				{
					DrawFilledRectangle(&r);
				}
			}
			RepairSlot = (which_slot < NumModuleSlots - 1);
			if (RepairSlot)
			{
				r.corner = Top.origin;
				r.corner.x += ShipPieceOffset();
				if (!IsHd())
				{
					DrawFilledRectangle(&r);
				}
				r.corner.y += RES_SCALE(20);
				if (!IsHd())
				{
					DrawFilledRectangle(&r);
				}
			}
		}

		Top.frame = SetAbsFrameIndex(ModuleFrame, which_piece);
		DrawStamp(&Top);

		Side.frame = SetRelFrameIndex(Top.frame, (NUM_MODULES - 1) + 6);
		DrawStamp(&Side);

		if (which_slot == 1 && which_piece == EmptySlot + 2)
		{
			STAMP s;

			s.origin = Top.origin;
			s.origin.x -= ShipPieceOffset();
			s.frame = SetAbsFrameIndex(ModuleFrame, NUM_MODULES + 5);
			DrawStamp(&s);
			s.origin = Side.origin;
			s.origin.x -= ShipPieceOffset();
			s.frame = SetRelFrameIndex(s.frame, (NUM_MODULES - 1) + 6);
			DrawStamp(&s);
		}

		if (RepairSlot)
		{
			Top.origin.x += ShipPieceOffset();
			Side.origin.x += ShipPieceOffset();
			which_piece = GLOBAL_SIS(ModuleSlots[++which_slot]);
			if (which_piece == EmptySlot + 2
				&& which_slot >= NumModuleSlots - 3)
			{
				++which_piece;
			}

			Top.frame = SetAbsFrameIndex(ModuleFrame, which_piece);
			DrawStamp(&Top);

			Side.frame = SetRelFrameIndex(Top.frame, (NUM_MODULES - 1) + 6);
			DrawStamp(&Side);
		}
	}
}

static void
rotateStarbase(MENU_STATE* pMS, FRAME AniFrame)
{
	static TimeCount NextTime {};
	const TimeCount Now = GetTimeCounter();

	if (AniFrame)
	{ // Setup the animation
		pMS->flash_frame0 = AniFrame;
		pMS->flash_rect0.corner.x = SafeX();
		pMS->flash_rect0.corner.y = SafeY();
	}

	if (Now >= NextTime || AniFrame)
	{
		STAMP s;

		NextTime = Now + (c_gameSecond / 20); // Starbase animation framerate

		s.origin = pMS->flash_rect0.corner;
		s.frame = pMS->flash_frame0;
		DrawStamp(&s);

		s.frame = IncFrameIndex(s.frame);
		if (GetFrameIndex(s.frame) == 0)
		{ // Do not redraw the base frame, animation loops to frame 1
			s.frame = IncFrameIndex(s.frame);
		}
		pMS->flash_frame0 = s.frame;
	}
}

bool DoStarBase(MENU_STATE* pMS)
{
	// XXX: This function is full of hacks and otherwise strange code

	if (uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
	{
		pMS->CurState = DEPART_BASE;
		goto ExitStarBase;
	}
	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	if (!pMS->Initialized)
	{
		LastActivity &= ~CHECK_LOAD;
		pMS->InputFunc = DoStarBase;

		SetFlashRect(nullptr, false);

		if (pMS->hMusic)
		{
			StopMusic();
			DestroyMusic(pMS->hMusic);
			pMS->hMusic = 0;
		}

		pMS->Initialized = true;

		pMS->CurFrame = CaptureDrawable(LoadGraphic(StarbaseAnimRes));
		pMS->hMusic = LoadMusicInstance(StarbaseMusicRes);

		SetContext(ScreenContext);
		SetTransitionSource(nullptr);
		BatchGraphics();
		SetContextBackGroundColor(BlackColor);
		ClearDrawable();
		rotateStarbase(pMS, pMS->CurFrame);
		DrawBaseStateStrings((STARBASE_STATE)~0, pMS->CurState);
		ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);

		PlayMusicResume(pMS->hMusic, NormalVolume);

		DrawBorderPadding(0);

		UnbatchGraphics();
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
ExitStarBase:
		DestroyDrawable(ReleaseDrawable(pMS->CurFrame));
		pMS->CurFrame = 0;

		SetMusicPosition();

		StopMusic();
		if (pMS->hMusic)
		{
			DestroyMusic(pMS->hMusic);
			pMS->hMusic = 0;
		}

		if (pMS->CurState == DEPART_BASE)
		{
			if (!(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
			{
				SET_GAME_STATE(STARBASE_VISITED, 0);
			}
			return (false);
		}

		pMS->Initialized = false;
		if (pMS->CurState == TALK_COMMANDER)
		{
			FlushInput();
			InitCommunication(COMMANDER_CONVERSATION);
			// XXX: InitCommunication() clears these flags, and we need them
			//   This marks that we are in Starbase.
			SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, (uint8_t)~0);
		}
		else
		{
			uint8_t OldState {};

			if (IsHd() && !hdFuelFrame)
			{
				hdFuelFrame =
					CaptureDrawable(LoadGraphic(FuelPmapAnimRes));
			}

			switch (OldState = pMS->CurState)
			{
				case OUTFIT_STARSHIP:
					pMS->InputFunc = DoOutfit;
					break;
				case SHIPYARD:
					pMS->InputFunc = DoShipyard;
					break;
			}

			SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
			DoInput(pMS, true);

			if (IsHd())
			{
				DestroyDrawable(ReleaseDrawable(hdFuelFrame));
				hdFuelFrame = 0;
			}

			pMS->Initialized = false;
			pMS->CurState = OldState;
			pMS->InputFunc = DoStarBase;
		}
	}
	else
	{
		STARBASE_STATE NewState {pMS->CurState};
		if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState-- == TALK_COMMANDER)
			{
				NewState = DEPART_BASE;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_RIGHT] || PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState++ == DEPART_BASE)
			{
				NewState = TALK_COMMANDER;
			}
		}

		BatchGraphics();
		SetContext(ScreenContext);

		if (NewState != pMS->CurState)
		{
			DrawBaseStateStrings(pMS->CurState, NewState);
			pMS->CurState = NewState;
		}

		rotateStarbase(pMS, nullptr);

		DrawBorderPadding(0);

		UnbatchGraphics();

		SleepThread(c_gameSecond / 30);
	}

	return (true);
}

static constexpr auto LostDays {14};

static void
DoTimePassage(void)
{
	SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond * 2));
	MoveGameClockDays(LostDays);

	// JMS: Calculate flagship location in IP.
	if (uqm::UQMOptions::read().orbitingPlanets)
	{
		double newAngle {NAN};
		GFXPOINT starbase_coords;

		// Calculate the starbase position on a circle with the help of sin and cos.
		newAngle = ((double)(10) + daysElapsed() * (FullCircle / 11.46)) * M_PI / 32 - M_PI / 2; // JMS: Starbase orbit values copied from gensol.c
		starbase_coords.x = (int16_t)(cos(newAngle) * MinMoonRadius());
		starbase_coords.y = (int16_t)(sin(newAngle) * MinMoonRadius());

		//log_add (log_Debug, "startangle:{} angle:{}, radius:{}, speed:{}, days:{} X:{}, y:{}", 10, newAngle, MinMoonRadius(), FullCircle / 11.46, daysElapsed(), starbase_coords.x, starbase_coords.y);

		// Translate the coordinates on a circle to an ellipse and update the ship's graphics' coordinates on the screen.
		uqm::flagship().shipStamp.origin.x = RES_SCALE(OrigSisScreenWidth() >> 1) + starbase_coords.x;
		uqm::flagship().shipStamp.origin.y = RES_SCALE(OrigSisScreenHeight() >> 1) + (starbase_coords.y >> 1);
	}
}

void VisitStarBase(void)
{
	MENU_STATE MenuState;
	GFXCONTEXT OldContext {};
	StatMsgMode prevMsgMode = SMM_UNDEFINED;

	// XXX: This should probably be moved out to Starcon2Main()
	if (GET_GAME_STATE(CHMMR_BOMB_STATE) == 2)
	{ // We were just transported by Chmmr to the Starbase
		// Force a reload of the SolarSys
		CurStarDescPtr() = nullptr;
		// This marks that we are in Starbase.
		SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, (uint8_t)~0);
	}

	if (!GET_GAME_STATE(STARBASE_AVAILABLE))
	{
		HSHIPFRAG hStarShip {};
		SHIP_FRAGMENT* FragPtr {};

		// Unallied Starbase conversation
		SetCommIntroMode(CIM_CROSSFADE_SCREEN, TimePeriod {});
		InitCommunication(COMMANDER_CONVERSATION);
		if (!GET_GAME_STATE(PROBE_ILWRATH_ENCOUNTER)
			|| (uqm::currentActivity() & CHECK_ABORT))
		{
			CleanupAfterStarBase();
			return;
		}

		/* Create an Ilwrath ship responding to the Ur-Quan
		 * probe's broadcast */
		hStarShip = CloneShipFragment(ILWRATH_SHIP,
									  &GLOBAL(npc_built_ship_q), 7);
		FragPtr = LockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
		/* Hack (sort of): Suppress the tally and salvage info
		 * after the battle */
		FragPtr->race_id = (uint8_t)~0;
		UnlockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);

		InitCommunication(ILWRATH_CONVERSATION);
		if (GLOBAL_SIS(CrewEnlisted) == (uint16_t)~0
			|| (uqm::currentActivity() & CHECK_ABORT))
		{
			return; // Killed by Ilwrath
		}

		// After Ilwrath battle, about-to-ally Starbase conversation
		SetCommIntroMode(CIM_CROSSFADE_SCREEN, TimePeriod {});
		InitCommunication(COMMANDER_CONVERSATION);
		if (uqm::currentActivity() & CHECK_ABORT)
		{
			return;
		}
		// XXX: InitCommunication() clears these flags, and we need them
		//   This marks that we are in Starbase.
		SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, (uint8_t)~0);
	}

	prevMsgMode = SetStatusMessageMode(SMM_RES_UNITS);

	if (GET_GAME_STATE(MOONBASE_ON_SHIP)
		|| GET_GAME_STATE(CHMMR_BOMB_STATE) == 2)
	{ // Go immediately into a conversation with the Commander when the
		// Starbase becomes available for the first time, or after Chmmr
		// install the bomb.
		DoTimePassage();
		if (GLOBAL_SIS(CrewEnlisted) == (uint16_t)~0)
		{
			return; // You are now dead! Thank you! (killed by Kohr-Ah)
		}

		SetCommIntroMode(CIM_FADE_IN_SCREEN, c_gameSecond * 2);
		InitCommunication(COMMANDER_CONVERSATION);
		if (uqm::currentActivity() & CHECK_ABORT)
		{
			return;
		}
		// XXX: InitCommunication() clears these flags, and we need them
		//   This marks that we are in Starbase.
		SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, (uint8_t)~0);
	}

	memset(&MenuState, 0, sizeof(MenuState));
	MenuState.InputFunc = DoStarBase;

	OldContext = SetContext(ScreenContext);
	DoInput(&MenuState, true);
	SetContext(OldContext);

	SetStatusMessageMode(prevMsgMode);
	CleanupAfterStarBase();
}

static void
CleanupAfterStarBase(void)
{
	if (!(uqm::currentActivity() & (CHECK_LOAD | CHECK_ABORT)))
	{
		// Mark as not in Starbase anymore
		SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 0);
		// Fake a load so Starcon2Main takes us to IP
		uqm::currentActivity() = CHECK_LOAD;
		NextActivity = uqm::makeWord(IN_INTERPLANETARY, 0)
					 | START_INTERPLANETARY;
	}
}

void InstallBombAtEarth(void)
{
	DoTimePassage();

	uqm::flagship().shipStamp.origin.x = RES_SCALE(OrigSisScreenWidth() >> 1);
	uqm::flagship().shipStamp.origin.y = RES_SCALE(OrigSisScreenHeight() >> 1);

	SetContext(ScreenContext);
	SetTransitionSource(nullptr);
	SetContextBackGroundColor(BlackColor);
	ClearDrawable();

	SleepThreadUntil(FadeScreen(FadeAllToColor, TimePeriod {}));

	SET_GAME_STATE(CHMMR_BOMB_STATE, 3); /* bomb processed */
	uqm::currentActivity() = CHECK_LOAD; /* fake a load game */
	NextActivity = uqm::makeWord(IN_INTERPLANETARY, 0) | START_INTERPLANETARY;
	CurStarDescPtr() = 0; /* force SolarSys reload */
}

// XXX: Doesn't really belong in this file.
uint16_t
WrapText(const char* pStr, uint16_t len, TEXT* tarray, int16_t field_width)
{
	uint16_t num_lines {0};
	do
	{
		GFXRECT r;
		uint16_t OldCount {};

		tarray->align = ALIGN_LEFT; /* set alignment to something */
		tarray->pStr = pStr;
		tarray->CharCount = 1;
		++num_lines;

		do
		{
			OldCount = tarray->CharCount;
			while (*++pStr != ' ' && (uint16_t)(pStr - tarray->pStr) < len)
				;
			tarray->CharCount = pStr - tarray->pStr;
			TextRect(tarray, &r, nullptr);
		} while (tarray->CharCount < len && r.extent.width < field_width);

		if (r.extent.width >= field_width)
		{
			if ((tarray->CharCount = OldCount) == 1)
			{
				do
				{
					++tarray->CharCount;
					TextRect(tarray, &r, nullptr);
				} while (r.extent.width < field_width);
				--tarray->CharCount;
			}
		}

		pStr = tarray->pStr + tarray->CharCount;
		len -= tarray->CharCount;
		++tarray;

		if (len && (r.extent.width < field_width || OldCount > 1))
		{
			++pStr; /* skip white space */
			--len;
		}

	} while (len);

	return (num_lines);
}
