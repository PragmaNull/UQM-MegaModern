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

#include "controls.h"
#include "colors.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "gamestr.h"
#include "util.h"
#include "libs/graphics/widgets.h"
#include "libs/sound/trackplayer.h"
#include "core/log/log.h"
#include "libs/resource/stringbank.h"
#include "battle.h"
#include "comm.h"

#include <ctype.h>
#include <stdlib.h>


static auto ConfirmWinWidth() { return ResScale(84); }
static auto ConfirmWinHeight() { return ResScale(26); }

bool WarpFromMenu = false;

static void DrawConfirmationWindow(bool answer, bool confirm)
{
	const Color oldfg = SetContextForeGroundColor(ShadowboxDarkColor);
	FONT oldfont = SetContextFont(StarConFont);
	FRAME oldFontEffect = SetContextFontEffect(nullptr);
	GfxRect r;
	GfxText t;

	BatchGraphics();
	r.corner.x = (CanvasWidth - ConfirmWinWidth()) >> 1;
	r.corner.y = (CanvasHeight - ConfirmWinHeight()) >> 1;
	r.extent.width = ConfirmWinWidth();
	r.extent.height = ConfirmWinHeight();
	if (!IsHd())
	{
		DrawShadowedBox(&r, AltShadowboxBackgroundColor,
						ShadowboxDarkColor, ShadowboxMediumColor);
	}
	else
	{
		DrawRenderedBox(&r, true, AltShadowboxBackgroundColor,
						THICK_OUTER_BEVEL, false);
	}


	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + ResScale(10);
	t.pStr = GameString(QUITMENU_STRING_BASE); // "Really Quit?"
	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
	t.baseline.y += ResScale(10);
	t.baseline.x = r.corner.x + (r.extent.width >> 2);
	t.pStr = GameString(QUITMENU_STRING_BASE + 1); // "Yes"
	SetContextForeGroundColor(
		answer ? (confirm ? WhiteColor : MenuHighlightColor) :
				 BlackColor);
	font_DrawText(&t);
	t.baseline.x += (r.extent.width >> 1);
	t.pStr = GameString(QUITMENU_STRING_BASE + 2); // "No"
	SetContextForeGroundColor(
		answer ? BlackColor : MenuHighlightColor);
	font_DrawText(&t);

	UnbatchGraphics();

	SetContextFontEffect(oldFontEffect);
	SetContextFont(oldfont);
	SetContextForeGroundColor(oldfg);
}

bool DoConfirmExit(void)
{
	bool result {};

	if (PlayingTrack())
	{
		PauseTrack();
	}

	const bool FlashPaused {PauseFlash()};

	{
		GfxRect r;
		GfxStamp s;
		GfxRect ctxRect;
		GfxRect oldRect;
		bool response = false;
		Color OldColor;
		DrawMode mode;
		DrawMode oldMode;
		TimeCount deltaT;

		deltaT = GetTimeCounter();

		GFXCONTEXT oldContext {SetContext(ScreenContext)};
		GetContextClipRect(&oldRect);
		SetContextClipRect(nullptr);

		GetContextClipRect(&ctxRect);
		r.extent.width = ConfirmWinWidth() + ResScale(4);
		r.extent.height = ConfirmWinHeight() + ResScale(4);
		r.corner.x = (ctxRect.extent.width - r.extent.width) >> 1;
		r.corner.y = (ctxRect.extent.height - r.extent.height) >> 1;
		s = SaveContextFrame(&ctxRect);

		mode = MAKE_DRAW_MODE(DRAW_DESATURATE, DesatAmount);
		oldMode = SetContextDrawMode(mode);
		DrawFilledRectangle(&ctxRect);
		SetContextDrawMode(oldMode);
		OldColor = SetContextForeGroundColor(BuildColorRgba(0x00, 0x00, 0x00, 0x30));
		DrawFilledRectangle(&ctxRect);
		SetContextForeGroundColor(OldColor);

		// There was a SetSystemRect(&r) call which we don't need anymore

		DrawConfirmationWindow(response, false);
		FlushGraphics();

		FlushInput();
		bool done {false};

		const uint8_t oldVolume {GetCurrMusicVol()};
		FadeMusic(60, c_gameSecond / 2);

		while (!done)
		{
			// Forbid recursive calls or pausing here!
			ExitRequested = false;
			GamePaused = false;
			UpdateInputState();
			if (uqm::currentActivity() & CHECK_ABORT)
			{ // something else triggered an exit
				done = true;
				response = true;
			}
			else if (PulsedInputState.menu[KEY_MENU_SELECT])
			{
				done = true;
				PlayMenuSound(MENU_SOUND_SUCCESS);
			}
			else if (PulsedInputState.menu[KEY_MENU_CANCEL])
			{
				done = true;
				response = false;
			}
			else if (PulsedInputState.menu[KEY_MENU_LEFT]
					 || PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				response = !response;
				DrawConfirmationWindow(response, false);
				PlayMenuSound(MENU_SOUND_MOVE);
			}
			SleepThread(c_gameSecond / 30);
		};

		// Restore the screen under the confirmation window
		if (!response)
		{
			DrawStamp(&s);
			DeltaLastTime(GetTimeCounter() - deltaT);
			FadeMusic(oldVolume, c_gameSecond / 2);
		}
		else
		{
			DrawConfirmationWindow(true, true);
		}
		DestroyDrawable(ReleaseDrawable(s.frame));
		ClearSystemRect();
		if (response || (uqm::currentActivity() & CHECK_ABORT))
		{
			result = true;
			uqm::currentActivity() |= CHECK_ABORT;
		}
		else
		{
			result = false;
		}
		ExitRequested = false;
		GamePaused = false;
		FlushInput();
		SetContextClipRect(&oldRect);
		SetContext(oldContext);
	}

	if (FlashPaused)
	{
		ContinueFlash();
	}

	if (PlayingTrack() && !result)
	{
		ResumeTrack();
	}

	return (result);
}

struct PopupState
{
	// standard state required by DoInput
	bool (*InputFunc)(struct PopupState* self);
};

static bool DoPopup(struct PopupState* self)
{
	(void)self;
	SleepThread(c_gameSecond / 20);
	return !(PulsedInputState.menu[KEY_MENU_SELECT] || PulsedInputState.menu[KEY_MENU_CANCEL] || (uqm::currentActivity() & CHECK_ABORT));
}

void DoPopupWindow(const char* msg)
{
	stringbank* bank = StringBank_Create();
	const char* lines[30];
	WidgetLabel label;
	GfxStamp s;
	GfxRect oldRect;
	GfxRect windowRect;
	PopupState state;
	MENU_SOUND_FLAGS s0 {};
	MENU_SOUND_FLAGS s1 {};

	if (!bank)
	{
		uqm::log::critical("FATAL: Memory exhaustion when preparing popup"
						   " window");
		exit(EXIT_FAILURE);
	}

	label.tag = WIDGET_TYPE_LABEL;
	label.parent = nullptr;
	label.handleEvent = Widget_HandleEventIgnoreAll;
	label.receiveFocus = Widget_ReceiveFocusRefuseFocus;
	label.draw = Widget_DrawLabel;
	label.height = Widget_HeightLabel;
	label.width = Widget_WidthFullScreen;
	label.line_count = SplitString(msg, '\n', 30, lines, bank);
	label.lines = lines;

	const bool FlashPaused {PauseFlash()};

	GFXCONTEXT oldContext {SetContext(ScreenContext)};
	GetContextClipRect(&oldRect);
	SetContextClipRect(nullptr);

	// TODO: Maybe DrawLabelAsWindow() should return a saved GfxStamp?
	//   We do not know the dimensions here, and so save the whole context
	s = SaveContextFrame(nullptr);

	Widget_SetFont(StarConFont);
	Widget_SetWindowColors(ShadowboxBackgroundColor,
						   ShadowboxDarkColor, ShadowboxMediumColor);
	DrawLabelAsWindow(&label, &windowRect);
	// There was a SetSystemRect(&windowRect) call which we don't need anymore

	GetMenuSounds(&s0, &s1);
	SetMenuSounds(MenuSoundNone, MenuSoundNone);
	InputFrameCallback* oldCallback {SetInputCallback(nullptr)};

	state.InputFunc = DoPopup;
	DoInput(&state, true);

	SetInputCallback(oldCallback);
	ClearSystemRect();
	DrawStamp(&s);
	DestroyDrawable(ReleaseDrawable(s.frame));
	SetContextClipRect(&oldRect);
	SetContext(oldContext);

	if (FlashPaused)
	{
		ContinueFlash();
	}

	SetMenuSounds(s0, s1);
	StringBank_Free(bank);
}
