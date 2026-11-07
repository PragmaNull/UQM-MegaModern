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

#include "starbase.h"

#include <math.h>

#include "build.h"
#include "colors.h"
#include "comm.h"
#include "controls.h"
#include "gamestr.h"
#include "libs/graphics/gfx_common.h"
#include "libs/time/timelib.h"
#include "nameref.h"
#include "planets/planets.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sis.h"
#include "sounds.h"
#include "starmap.h"
#include "util.h"

namespace uqm
{
static void CleanupAfterStarBase(void);

static void DrawBaseStateStrings(StarbaseState OldState, StarbaseState NewState)
{
	GfxText t;
	const uint16_t text_base_y = ResScale(106 + 28);
	const uint16_t text_spacing_y = ResScale(23 - 4);

	SetContext(ScreenContext);
	SetContextFont(StarConFont);
	SetContextForeGroundColor(BlackColor);

	t.baseline.x = ResScale(73 - 4) + SafeX();
	t.align = TextAlign::Center;

	if (OldState == (StarbaseState)~0)
	{
		t.baseline.y = text_base_y + SafeY();
		for (OldState = *StarbaseMenuItem::TalkCommander; OldState < StarbaseMenuItem::DepartBase; ++OldState)
		{
			if (OldState != NewState)
			{
				t.pStr = GameString(GameStringBase::StarbaseString + 1 + OldState);
				t.CharCount = (uint16_t)~0;
				font_DrawText(&t);
			}
			t.baseline.y += text_spacing_y;
		}
	}

	t.baseline.y = text_base_y + SafeY() + (text_spacing_y * OldState);
	t.pStr = GameString(GameStringBase::StarbaseString + 1 + OldState);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E));
	t.baseline.y = text_base_y + SafeY() + (text_spacing_y * NewState);
	t.pStr = GameString(GameStringBase::StarbaseString + 1 + NewState);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
}

void DrawShipPiece(FrameRef ModuleFrame, uint16_t which_piece, uint16_t which_slot,
				   bool DrawBluePrint)
{
	Color OldColor = UndefinedColor;
	// Initialisation is just to keep the compiler silent.
	GfxRect r;
	GfxStamp Side;
	GfxStamp Top;

	int8_t RepairSlot {0};
	switch (static_cast<ModuleType>(which_piece))
	{
		case ModuleType::FusionThruster:
		case ModuleType::EmptyDriveSlot:
			Side.origin.x = DriveSideX();
			Side.origin.y = DriveSideY();
			Top.origin.x = DriveTopX();
			Top.origin.y = DriveTopY();
			break;
		case ModuleType::TurningJets:
		case ModuleType::EmptyJetSlot:
			Side.origin.x = JetSideX();
			Side.origin.y = JetSideY();
			Top.origin.x = JetTopX();
			Top.origin.y = JetTopY();
			break;
		default:
			if (which_piece < ModuleType::EmptyModuleSlot)
			{
				RepairSlot = 1;
				if (which_piece < EmptySlot
					&& (which_slot == 0
						|| uqm::gameState().sis().ModuleSlots[which_slot - 1] < EmptySlot))
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
					&& uqm::gameState().sis().ModuleSlots[which_slot + 1] < EmptySlot)
				{
					RepairSlot = -1;
					if (which_piece == ModuleType::EmptyLanderSlot
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
									  ((*ModuleType::NumModules - 1) + (6 - 2)) + (*ModuleType::NumModules + 6)
										  - (RepairSlot + 1));
		DrawStamp(&Side);
	}
	else if (RepairSlot)
	{
		r.corner = Side.origin;
		r.extent.width = ShipPieceOffset();
		r.extent.height = ResScale(1);
		OldColor = SetContextForeGroundColor(BlackColor);
		if (!IsHd())
		{
			DrawFilledRectangle(&r);
		}
		r.corner.y += ResScale(23 - 1);
		if (!IsHd())
		{
			DrawFilledRectangle(&r);
		}

		r.extent.width = ResScale(1);
		r.extent.height = ResScale(8);
		if (RepairSlot == 2)
		{
			r.corner = Side.origin;
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}
			r.corner.y += ResScale(15);
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
			r.corner.y += ResScale(15);
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
			Top.frame = SetRelFrameIndex(Side.frame, -((ModuleType::NumModules - 1) + 6));
			DrawStamp(&Top);
		}
		else if (RepairSlot)
		{
			r.corner = Top.origin;
			r.extent.width = ShipPieceOffset();
			r.extent.height = ResScale(1);
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}
			r.corner.y += ResScale(32 - 1);
			if (!IsHd())
			{
				DrawFilledRectangle(&r);
			}

			r.extent.width = ResScale(1);
			r.extent.height = ResScale(12);
			if (RepairSlot == 2)
			{
				r.corner = Top.origin;
				if (!IsHd())
				{
					DrawFilledRectangle(&r);
				}
				r.corner.y += ResScale(20);
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
				r.corner.y += ResScale(20);
				if (!IsHd())
				{
					DrawFilledRectangle(&r);
				}
			}
		}

		Top.frame = SetAbsFrameIndex(ModuleFrame, which_piece);
		DrawStamp(&Top);

		Side.frame = SetRelFrameIndex(Top.frame, *(ModuleType::NumModules - 1) + 6);
		DrawStamp(&Side);

		if (which_slot == 1 && which_piece == ModuleType::EmptyModuleSlot)
		{
			GfxStamp s;

			s.origin = Top.origin;
			s.origin.x -= ShipPieceOffset();
			s.frame = SetAbsFrameIndex(ModuleFrame, ModuleType::NumModules + 5);
			DrawStamp(&s);
			s.origin = Side.origin;
			s.origin.x -= ShipPieceOffset();
			s.frame = SetRelFrameIndex(s.frame, *(ModuleType::NumModules - 1) + 6);
			DrawStamp(&s);
		}

		if (RepairSlot)
		{
			Top.origin.x += ShipPieceOffset();
			Side.origin.x += ShipPieceOffset();
			which_piece = *uqm::gameState().sis().ModuleSlots[++which_slot];
			if (which_piece == ModuleType::EmptyModuleSlot
				&& which_slot >= NumModuleSlots - 3)
			{
				++which_piece;
			}

			Top.frame = SetAbsFrameIndex(ModuleFrame, which_piece);
			DrawStamp(&Top);

			Side.frame = SetRelFrameIndex(Top.frame, *(ModuleType::NumModules - 1) + 6);
			DrawStamp(&Side);
		}
	}
}

static void rotateStarbase(MenuState* pMS, FrameRef AniFrame)
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
		GfxStamp s;

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

bool DoStarBase(MenuState* pMS)
{
	// XXX: This function is full of hacks and otherwise strange code

	if (uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
	{
		pMS->CurState = *StarbaseMenuItem::DepartBase;
		goto ExitStarBase;
	}
	SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);

	if (!pMS->Initialized)
	{
		LastActivity &= ~ActivityFlag::CheckLoad;
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
		const BatchScope batch {};
		SetContextBackGroundColor(BlackColor);
		ClearDrawable();
		rotateStarbase(pMS, pMS->CurFrame);
		DrawBaseStateStrings((StarbaseState)~0, pMS->CurState);
		ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);

		PlayMusicResume(pMS->hMusic, NormalVolume);

		DrawBorderPadding(0);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuSelect])
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

		if (pMS->CurState == StarbaseMenuItem::DepartBase)
		{
			if (!(uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad)))
			{
				uqm::gameState().setFlag(uqm::GameFlag::STARBASE_VISITED, static_cast<uint32_t>(0));
			}
			return (false);
		}

		pMS->Initialized = false;
		if (pMS->CurState == StarbaseMenuItem::TalkCommander)
		{
			FlushInput();
			InitCommunication(Conversation::Commander);
			// XXX: InitCommunication() clears these flags, and we need them
			//   This marks that we are in Starbase.
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>((uint8_t)~0));
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
				case *StarbaseMenuItem::OutfitStarship:
					pMS->InputFunc = DoOutfit;
					break;
				case *StarbaseMenuItem::Shipyard:
					pMS->InputFunc = DoShipyard;
					break;
			}

			SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
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
		StarbaseState NewState {pMS->CurState};
		if (PulsedInputState.menu[*MenuKey::MenuLeft] || PulsedInputState.menu[*MenuKey::MenuUp])
		{
			if (NewState-- == StarbaseMenuItem::TalkCommander)
			{
				NewState = *StarbaseMenuItem::DepartBase;
			}
		}
		else if (PulsedInputState.menu[*MenuKey::MenuRight] || PulsedInputState.menu[*MenuKey::MenuDown])
		{
			if (NewState++ == StarbaseMenuItem::DepartBase)
			{
				NewState = *StarbaseMenuItem::TalkCommander;
			}
		}

		{
			const BatchScope batch {};
			SetContext(ScreenContext);

			if (NewState != pMS->CurState)
			{
				DrawBaseStateStrings(pMS->CurState, NewState);
				pMS->CurState = NewState;
			}

			rotateStarbase(pMS, nullptr);

			DrawBorderPadding(0);
		}

		SleepThread(c_gameSecond / 30);
	}

	return (true);
}

static constexpr auto LostDays {14};

static void DoTimePassage(void)
{
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond * 2));
	MoveGameClockDays(LostDays);

	// JMS: Calculate flagship location in IP.
	if (uqm::UQMOptions::read().orbitingPlanets)
	{
		double newAngle {NAN};
		GfxPoint starbase_coords;

		// Calculate the starbase position on a circle with the help of sin and cos.
		newAngle = ((double)(10) + daysElapsed() * (FullCircle / 11.46)) * M_PI / 32 - M_PI / 2; // JMS: Starbase orbit values copied from gensol.c
		starbase_coords.x = (int16_t)(cos(newAngle) * MinMoonRadius());
		starbase_coords.y = (int16_t)(sin(newAngle) * MinMoonRadius());

		//log_add (log_Debug, "startangle:{} angle:{}, radius:{}, speed:{}, days:{} X:{}, y:{}", 10, newAngle, MinMoonRadius(), FullCircle / 11.46, daysElapsed(), starbase_coords.x, starbase_coords.y);

		// Translate the coordinates on a circle to an ellipse and update the ship's graphics' coordinates on the screen.
		uqm::flagship().shipStamp.origin.x = ResScale(OrigSisScreenWidth() >> 1) + starbase_coords.x;
		uqm::flagship().shipStamp.origin.y = ResScale(OrigSisScreenHeight() >> 1) + (starbase_coords.y >> 1);
	}
}

void VisitStarBase(void)
{
	MenuState menuState;
	StatMsgMode prevMsgMode = StatMsgMode::Undefined;

	// XXX: This should probably be moved out to Starcon2Main()
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 2)
	{ // We were just transported by Chmmr to the Starbase
		// Force a reload of the SolarSys
		CurStarDescPtr() = nullptr;
		// This marks that we are in Starbase.
		uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>((uint8_t)~0));
	}

	if (!uqm::gameState().flag(uqm::GameFlag::STARBASE_AVAILABLE))
	{
		ShipFragmentHandle hStarShip {};
		ShipFragment* FragPtr {};

		// Unallied Starbase conversation
		SetCommIntroMode(CommIntroMode::CrossfadeScreen, TimePeriod {});
		InitCommunication(Conversation::Commander);
		if (!uqm::gameState().flag(uqm::GameFlag::PROBE_ILWRATH_ENCOUNTER)
			|| (uqm::currentActivity() & ActivityFlag::CheckAbort))
		{
			CleanupAfterStarBase();
			return;
		}

		/* Create an Ilwrath ship responding to the Ur-Quan
		 * probe's broadcast */
		hStarShip = CloneShipFragment(RaceId::Ilwrath,
									  &uqm::gameState().game().npc_built_ship_q, 7);
		FragPtr = LockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip);
		/* Hack (sort of): Suppress the tally and salvage info
		 * after the battle */
		FragPtr->race_id = (uint8_t)~0;
		UnlockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip);

		InitCommunication(Conversation::Ilwrath);
		if (uqm::gameState().sis().CrewEnlisted == (uint16_t)~0
			|| (uqm::currentActivity() & ActivityFlag::CheckAbort))
		{
			return; // Killed by Ilwrath
		}

		// After Ilwrath battle, about-to-ally Starbase conversation
		SetCommIntroMode(CommIntroMode::CrossfadeScreen, TimePeriod {});
		InitCommunication(Conversation::Commander);
		if (uqm::currentActivity() & ActivityFlag::CheckAbort)
		{
			return;
		}
		// XXX: InitCommunication() clears these flags, and we need them
		//   This marks that we are in Starbase.
		uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>((uint8_t)~0));
	}

	prevMsgMode = SetStatusMessageMode(StatMsgMode::ResUnits);

	if (uqm::gameState().flag(uqm::GameFlag::MOONBASE_ON_SHIP)
		|| uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 2)
	{ // Go immediately into a conversation with the Commander when the
		// Starbase becomes available for the first time, or after Chmmr
		// install the bomb.
		DoTimePassage();
		if (uqm::gameState().sis().CrewEnlisted == (uint16_t)~0)
		{
			return; // You are now dead! Thank you! (killed by Kohr-Ah)
		}

		SetCommIntroMode(CommIntroMode::FadeInScreen, c_gameSecond * 2);
		InitCommunication(Conversation::Commander);
		if (uqm::currentActivity() & ActivityFlag::CheckAbort)
		{
			return;
		}
		// XXX: InitCommunication() clears these flags, and we need them
		//   This marks that we are in Starbase.
		uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>((uint8_t)~0));
	}

	memset(&menuState, 0, sizeof(menuState));
	menuState.InputFunc = DoStarBase;

	{
		const ContextScope contextScope {ScreenContext};
		DoInput(&menuState, true);
	}

	SetStatusMessageMode(prevMsgMode);
	CleanupAfterStarBase();
}

static void CleanupAfterStarBase(void)
{
	if (!(uqm::currentActivity() & (ActivityFlag::CheckLoad | ActivityFlag::CheckAbort)))
	{
		// Mark as not in Starbase anymore
		uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(0));
		// Fake a load so Starcon2Main takes us to IP
		uqm::currentActivity() = *ActivityFlag::CheckLoad;
		NextActivity = uqm::makeWord(ActivityKind::InInterplanetary, 0)
					 | ActivityFlag::StartInterplanetary;
	}
}

void InstallBombAtEarth(void)
{
	DoTimePassage();

	uqm::flagship().shipStamp.origin.x = ResScale(OrigSisScreenWidth() >> 1);
	uqm::flagship().shipStamp.origin.y = ResScale(OrigSisScreenHeight() >> 1);

	SetContext(ScreenContext);
	SetTransitionSource(nullptr);
	SetContextBackGroundColor(BlackColor);
	ClearDrawable();

	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToColor, TimePeriod {}));

	uqm::gameState().setFlag(uqm::GameFlag::CHMMR_BOMB_STATE, static_cast<uint32_t>(3)); /* bomb processed */
	uqm::currentActivity() = *ActivityFlag::CheckLoad;									 /* fake a load game */
	NextActivity = uqm::makeWord(ActivityKind::InInterplanetary, 0) | ActivityFlag::StartInterplanetary;
	CurStarDescPtr() = 0; /* force SolarSys reload */
}

// XXX: Doesn't really belong in this file.
uint16_t WrapText(uqstl::string_view text, GfxText* tarray, int16_t field_width)
{
	const char* pStr {text.data()};
	uint16_t len {static_cast<uint16_t>(text.size())};
	uint16_t num_lines {0};
	do
	{
		GfxRect r;
		uint16_t OldCount {};

		tarray->align = TextAlign::Left; /* set alignment to something */
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

} // namespace uqm
