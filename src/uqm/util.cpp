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
#include "util.h"
#include "setup.h"
#include "units.h"
#include "settings.h"
#include "core/string/StringUtils.h"
#include "libs/inplib.h"
#include "libs/sound/trackplayer.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
#include "hyper.h"
#include "colors.h"
#include "sis.h"
#include "menustat.h"
#include "gamestr.h"
#include "shipcont.h"
#include "comm.h"
// JSD added to reference plot_map for SOL.
#include "starmap.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
void DrawStarConBox(GfxRect* pRect, int16_t BorderWidth, Color TopLeftColor,
					Color BottomRightColor, bool FillInterior, Color InteriorColor,
					bool CreateCorners, Color CornerColor)
{
	GfxRect locRect;
	Color oldcolor;

	BatchGraphics();

	if (FillInterior)
	{
		oldcolor = SetContextForeGroundColor(InteriorColor);
		DrawFilledRectangle(pRect);
		SetContextForeGroundColor(TopLeftColor);
	}
	else
	{
		oldcolor = SetContextForeGroundColor(TopLeftColor);
	}

	if (BorderWidth == 0)
	{
		BorderWidth = ResScale(2);
	}

	locRect.corner = pRect->corner;
	locRect.extent.width = pRect->extent.width;
	locRect.extent.height = ResScale(1);
	DrawFilledRectangle(&locRect);

	if (BorderWidth == ResScale(2))
	{
		locRect.corner.x += ResScale(1);
		locRect.corner.y += ResScale(1);
		locRect.extent.width -= ResScale(2);
		DrawFilledRectangle(&locRect);
	}

	locRect.corner = pRect->corner;
	locRect.extent.width = ResScale(1);
	locRect.extent.height = pRect->extent.height;
	DrawFilledRectangle(&locRect);

	if (BorderWidth == ResScale(2))
	{
		locRect.corner.x += ResScale(1);
		locRect.corner.y += ResScale(1);
		locRect.extent.height -= ResScale(2);
		DrawFilledRectangle(&locRect);
	}

	SetContextForeGroundColor(BottomRightColor);
	locRect.corner.x = pRect->corner.x + pRect->extent.width - ResScale(1);
	locRect.corner.y = pRect->corner.y + ResScale(1);
	locRect.extent.height = pRect->extent.height - ResScale(1);
	DrawFilledRectangle(&locRect);

	if (BorderWidth == ResScale(2))
	{
		locRect.corner.x -= ResScale(1);
		locRect.corner.y += ResScale(1);
		locRect.extent.height -= ResScale(2);
		DrawFilledRectangle(&locRect);
	}

	locRect.corner.x = pRect->corner.x;
	locRect.extent.width = pRect->extent.width;
	locRect.corner.y = pRect->corner.y + pRect->extent.height - ResScale(1);
	locRect.extent.height = ResScale(1);
	DrawFilledRectangle(&locRect);

	if (BorderWidth == ResScale(2))
	{
		locRect.corner.x += ResScale(1);
		locRect.corner.y -= ResScale(1);
		locRect.extent.width -= ResScale(2);
		DrawFilledRectangle(&locRect);
	}

	if (CreateCorners)
	{
		if (sameColor(Transparent, CornerColor)
			&& AreTheyShades(TopLeftColor, BottomRightColor))
		{
			SetContextForeGroundColor(
				CreateAvgShade(TopLeftColor, BottomRightColor));
		}
		else
		{
			SetContextForeGroundColor(CornerColor);
		}

		locRect.corner.x = pRect->corner.x;
		locRect.corner.y = pRect->corner.y + pRect->extent.height
						 - ResScale(1);
		locRect.extent.width = ResScale(1);
		locRect.extent.height = ResScale(1);
		DrawFilledRectangle(&locRect);
		locRect.corner.x = pRect->corner.x + pRect->extent.width
						 - ResScale(1);
		locRect.corner.y = pRect->corner.y;
		DrawFilledRectangle(&locRect);

		if (BorderWidth == ResScale(2))
		{
			locRect.corner.x -= ResScale(1);
			locRect.corner.y += ResScale(1);
			DrawFilledRectangle(&locRect);
			locRect.corner.x = pRect->corner.x + ResScale(1);
			locRect.corner.y = pRect->corner.y + pRect->extent.height
							 - ResScale(2);
			DrawFilledRectangle(&locRect);
		}
	}

	SetContextForeGroundColor(oldcolor);

	UnbatchGraphics();
}

void DrawRenderedBox(GfxRect* r, bool filled, Color fill_color, BevelLevel type,
					 int custom)
{
	int i {};
	GfxStamp stamp;
	int16_t columns = r->extent.width;
	int16_t rows = r->extent.height;

	if (!r->extent.width || !r->extent.height)
	{
		return;
	}

	BatchGraphics();

	if (filled)
	{
		const Color OldColor = SetContextForeGroundColor(fill_color);

		if (type == BevelLevel::SpecialBevel)
		{
			r->corner.x += ResScale(4);
			r->corner.y += ResScale(4);
			r->extent.width -= ResScale(8);
			r->extent.height -= ResScale(8);
		}

		DrawFilledRectangle(r);

		if (type == BevelLevel::SpecialBevel)
		{
			r->corner.x -= ResScale(4);
			r->corner.y -= ResScale(4);
			r->extent.width += ResScale(8);
			r->extent.height += ResScale(8);
		}

		SetContextForeGroundColor(OldColor);
	}

	stamp.frame = custom ? CustBevelFrame : DefBevelFrame;

	if (type == BevelLevel::SpecialBevel)
	{
		columns -= ResScale(8);
		rows -= ResScale(8);
	}

	for (i = 0; i < columns; i++)
	{ // Draw top and bottom borders
		stamp.frame = SetAbsFrameIndex(stamp.frame, type + 4);
		stamp.origin.x = r->corner.x + i;
		stamp.origin.y = r->corner.y;
		DrawStamp(&stamp);
		stamp.frame = SetAbsFrameIndex(stamp.frame, type + 6);
		stamp.origin.y = r->corner.y + r->extent.height;
		DrawStamp(&stamp);
	}

	for (i = 0; i < rows; i++)
	{ // Draw right and left borders
		stamp.frame = SetAbsFrameIndex(stamp.frame, type + 5);
		stamp.origin.x = r->corner.x + r->extent.width;
		stamp.origin.y = r->corner.y + i;
		DrawStamp(&stamp);
		stamp.frame = SetAbsFrameIndex(stamp.frame, type + 7);
		stamp.origin.x = r->corner.x;
		DrawStamp(&stamp);
	}

	// Draw corners clockwise from the top-left
	stamp.frame = SetAbsFrameIndex(stamp.frame, type);
	stamp.origin.x = r->corner.x;
	stamp.origin.y = r->corner.y;
	DrawStamp(&stamp);

	stamp.frame = SetAbsFrameIndex(stamp.frame, type + 1);
	stamp.origin.x = r->corner.x + r->extent.width;
	DrawStamp(&stamp);

	stamp.frame = SetAbsFrameIndex(stamp.frame, type + 2);
	stamp.origin.y = r->corner.y + r->extent.height;
	DrawStamp(&stamp);

	stamp.frame = SetAbsFrameIndex(stamp.frame, type + 3);
	stamp.origin.x = r->corner.x;
	DrawStamp(&stamp);

	UnbatchGraphics();
}

void DrawBorderPadding(uint32_t videoWidth)
{
	GfxRect r;
	const uint16_t safe_x =
		(videoWidth && videoWidth < 280 ? SafeNeg(4) * 2 : SafeX());

	if (!safe_x)
	{
		return;
	}

	GfxContextRef OldContext {SetContext(ScreenContext)};

	if (videoWidth)
	{
		SetContextForeGroundColor(BuildShadeRgba(0x0C));
	}
	else
	{
		SetContextForeGroundColor(BlackColor);
	}

	// Top bar
	r.corner = MAKE_POINT(0, 0);
	r.extent.width = CanvasWidth;
	r.extent.height = SafeY();
	DrawFilledRectangle(&r);

	// Right bar
	r.corner.x = r.extent.width - safe_x;
	r.extent.width = safe_x;
	r.extent.height = CanvasHeight;
	DrawFilledRectangle(&r);

	// Bottom bar
	r.corner.x = 0;
	r.corner.y = CanvasHeight - SafeY();
	r.extent.width = CanvasWidth;
	r.extent.height = SafeY();
	DrawFilledRectangle(&r);

	// Left bar
	r.corner = MAKE_POINT(0, 0);
	r.extent.width = safe_x;
	r.extent.height = CanvasHeight;
	DrawFilledRectangle(&r);

	SetContext(OldContext);
}

void DrawRadarBorder(void)
{
	GfxRect r;

	if (IsPad())
	{
		return;
	}

	GfxContextRef OldContext {SetContext(StatusContext)};

	r.corner.x = ResScale(4) - ResScale(1);
	r.corner.y = RadarY() - ResScale(1);
	r.extent.width = RadarWidth() + ResScale(2);
	r.extent.height = RadarHeight() + ResScale(2);

	if (IsHd() || uqm::UQMOptions::read().customBorder)
	{
		DrawRenderedBox(&r, false, NullColor, BevelLevel::ThinInnerBevel,
						uqm::UQMOptions::read().customBorder);
	}
	else
	{
		DrawStarConBox(&r, ResScale(1), AltShadowboxTopLeft,
					   AltShadowboxBottomRight, false, Transparent, false,
					   Transparent);
	}

	SetContext(OldContext);
}

uint32_t SeedRandomNumbers(void)
{
	const auto cur_time {static_cast<uint32_t>(GetTimeCounter().time_since_epoch().count())};
	uqm::random().reseed(cur_time);

	return (cur_time);
}

GfxStamp SaveContextFrame(const GfxRect* saveRect)
{
	GfxStamp s;

	if (saveRect)
	{ // a portion of the context
		s.origin = saveRect->corner;
	}
	else
	{ // the entire context
		s.origin.x = 0;
		s.origin.y = 0;
	}

	s.frame = CaptureDrawable(CopyContextRect(saveRect));

	return s;
}

static void DrawPauseText(GfxRect* rect)
{
	GfxText text;
	Color OldColor;
	GfxRect block;

	if (!strlen(GameString(GameStringBase::QuitmenuString + 4)))
	{
		return;
	}

	FontRef OldFont {SetContextFont(DosBool(LabelFont, StarConFont))};
	OldColor = SetContextForeGroundColor(
		BuildShadeRgba(DosBool(0x74, 0x6B)));

	// Blank out the text
	block = *rect;
	block.extent.width -= ResScale(4);
	block.extent.height -= ResScale(4);
	block.corner.x += ResScale(2);
	block.corner.y += ResScale(2);
	DrawFilledRectangle(&block);

	SetContextForeGroundColor(
		DosBool(ShadowboxDarkColor, WhiteColor));

	text.baseline = rect->corner;
	text.baseline.x += rect->extent.width >> 1;
	text.baseline.y += ResScale(10);
	text.align = TextAlign::Center;
	text.pStr = AlignText(
		(const char*)GameString(GameStringBase::QuitmenuString + 4),
		&text.baseline.x);
	text.CharCount = (uint16_t)~0;

	font_DrawText(&text);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

bool PauseGame(void)
{
	GfxRect r;
	GfxStamp s;
	GfxStamp saveStamp;
	GfxRect ctxRect;
	GfxPoint OldOrigin;
	GfxRect OldRect;
	Color OldColor;
	DrawMode mode;
	DrawMode oldMode;
	TimeCount deltaT;

	if (ActivityFrame == 0
		|| (uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckPause))
		|| (LastActivity & (ActivityFlag::CheckLoad | ActivityFlag::CheckRestart)))
	{
		return (false);
	}

	uqm::currentActivity() |= ActivityFlag::CheckPause;

	if (PlayingTrack())
	{
		PauseTrack();
	}

	deltaT = GetTimeCounter();

	GfxContextRef OldContext {SetContext(ScreenContext)};
	OldOrigin = SetContextOrigin(MAKE_POINT(0, 0));
	GetContextClipRect(&OldRect);
	SetContextClipRect(nullptr);

	GetContextClipRect(&ctxRect);
	GetFrameRect(ActivityFrame, &r);
	r.corner.x = (ctxRect.extent.width - r.extent.width) >> 1;
	r.corner.y = (ctxRect.extent.height - r.extent.height) >> 1;
	saveStamp = SaveContextFrame(&ctxRect);

	mode = MAKE_DRAW_MODE(DrawKind::Desaturate, DesatAmount);
	oldMode = SetContextDrawMode(mode);
	DrawFilledRectangle(&ctxRect);
	SetContextDrawMode(oldMode);
	OldColor = SetContextForeGroundColor(
		BuildColorRgba(0x00, 0x00, 0x00, 0x30));
	DrawFilledRectangle(&ctxRect);
	SetContextForeGroundColor(OldColor);

	s.origin = r.corner;
	s.frame = ActivityFrame;
	// There was a SetSystemRect(&r) call which we don't need anymore
	DrawStamp(&s);
	DrawPauseText(&r);

	const uint8_t oldVolume {GetCurrMusicVol()};
	FadeMusic(60, c_gameSecond / 2);

	FlushGraphics();

	while (ImmediateInputState.menu[*MenuKey::Pause] && GamePaused)
	{
		BeginInputFrame();
		TaskSwitch();
	}

	while (!ImmediateInputState.menu[*MenuKey::Pause] && GamePaused)
	{
		BeginInputFrame();
		TaskSwitch();
	}

	while (ImmediateInputState.menu[*MenuKey::Pause] && GamePaused)
	{
		BeginInputFrame();
		TaskSwitch();
	}

	GamePaused = false;

	DrawStamp(&saveStamp);
	DestroyDrawable(ReleaseDrawable(saveStamp.frame));
	ClearSystemRect();

	FadeMusic(oldVolume, c_gameSecond / 2);

	SetContextClipRect(&OldRect);
	SetContextOrigin(OldOrigin);
	SetContext(OldContext);

	WaitForNoInput(c_gameSecond / 4, true);

	DeltaLastTime(GetTimeCounter() - deltaT);

	if (PlayingTrack())
	{
		ResumeTrack();
	}


	TaskSwitch();
	uqm::currentActivity() &= ~ActivityFlag::CheckPause;
	return (true);
}

// Waits for a button to be pressed
// Returns true if the wait succeeded (found input)
//    false if timed out or game aborted
bool WaitForAnyButtonUntil(bool newButton, TimeCount timeOut,
						   bool resetInput)
{
	if (newButton && !WaitForNoInputUntil(timeOut, false))
	{
		return false;
	}

	bool buttonPressed {AnyButtonPress(true)};
	while (!buttonPressed
		   && GetTimeCounter() < timeOut
		   && !(uqm::currentActivity() & ActivityFlag::CheckAbort)
		   && !QuitPosted)
	{
		SleepThread(c_gameSecond / 40);
		buttonPressed = AnyButtonPress(true);
	}

	if (resetInput)
	{
		FlushInput();
	}

	return buttonPressed;
}

// Waits for action button to be pressed
// Returns true if the wait succeeded (found input)
//    false if timed out or game aborted
bool WaitForActButtonUntil(bool newButton, TimeCount timeOut,
						   bool resetInput)
{
	if (newButton && !WaitForNoInputUntil(timeOut, false))
	{
		return false;
	}

	bool buttonPressed {ActKeysPress()};
	while (!buttonPressed
		   && GetTimeCounter() < timeOut
		   && !(uqm::currentActivity() & ActivityFlag::CheckAbort)
		   && !QuitPosted)
	{
		SleepThread(c_gameSecond / 40);
		buttonPressed = ActKeysPress();
	}

	if (resetInput)
	{
		FlushInput();
	}

	return buttonPressed;
}

bool WaitForAnyButton(bool newButton, TimePeriod duration, bool resetInput)
{
	const TimeCount timeOut {duration == WaitInfinite ? TimeCount::max() : GetTimeCounter() + duration};
	return WaitForAnyButtonUntil(newButton, timeOut, resetInput);
}

bool WaitForActButton(bool newButton, TimePeriod duration, bool resetInput)
{
	const TimeCount timeOut {duration == WaitInfinite ? TimeCount::max() : GetTimeCounter() + duration};
	return WaitForActButtonUntil(newButton, timeOut, resetInput);
}

// Returns true if the wait succeeded (found no input)
//    false if timed out or game aborted
bool WaitForNoInputUntil(TimeCount timeOut, bool resetInput)
{
	bool buttonPressed {AnyButtonPress(true)};
	while (buttonPressed
		   && GetTimeCounter() < timeOut
		   && !(uqm::currentActivity() & ActivityFlag::CheckAbort)
		   && !QuitPosted)
	{
		SleepThread(c_gameSecond / 40);
		buttonPressed = AnyButtonPress(true);
	}

	if (resetInput)
	{
		FlushInput();
	}

	return !buttonPressed;
}

bool WaitForNoInput(TimePeriod duration, bool resetInput)
{
	const TimeCount timeOut {duration == WaitInfinite ? TimeCount::max() : GetTimeCounter() + duration};
	return WaitForNoInputUntil(timeOut, resetInput);
}

// Stops game clock and music thread and minimizes interrupts/cycles
//  based on value of global GameActive variable
// See similar sleep state for main thread in uqm.c:main()
void SleepGame(void)
{
	if (QuitPosted)
	{
		return; // Do not sleep the game when already asked to quit
	}

	uqm::log::debug("Game is going to sleep");

	if (PlayingTrack())
	{
		PauseTrack();
	}
	PauseMusic();


	while (!GameActive && !QuitPosted)
	{
		SleepThread(c_gameSecond / 2);
	}

	uqm::log::debug("Game is waking up");

	WaitForNoInput(c_gameSecond / 10, true);

	ResumeMusic();

	if (PlayingTrack())
	{
		ResumeTrack();
	}


	TaskSwitch();
}

/* Returns the fuel requirement to get to Sol (in fuel units * 100)
 */
uint32_t get_fuel_to_sol(void)
{
	GfxPoint pt;

	pt.x = LogxToUniverse(uqm::gameState().sis().log_x);
	pt.y = LogyToUniverse(uqm::gameState().sis().log_y);

	// JSD Replace old hard coded SOL with plot based SOL
	//pt.x -= SOL_X;
	//pt.y -= SOL_Y;
	pt.x -= plot_map[*PlotId::Sol].star_pt.x;
	pt.y -= plot_map[*PlotId::Sol].star_pt.y;

	const uint32_t f {(uint32_t)((long)pt.x * pt.x + (long)pt.y * pt.y)};
	if (f == 0 || uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) > 1)
	{
		return 0;
	}
	else
	{
		return (square_root(f) + (FuelTankScale / 20));
	}
}

void DrawFlagStatDisplay(const char* str)
{
	GfxText t;
	GfxRect r;

	r.corner.x = ResScale(2);
	r.corner.y = ResScale(20);
	r.extent.width = FieldWidth() + ResScale(1);
	r.extent.height = (ResScale(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, ResScale(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, ModuleBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(BorderFrameIndex::DeviceCargo);
	}

	// print the "str" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - ResScale(1);
	t.baseline.y = r.corner.y + ResScale(7);
	t.align = TextAlign::Center;
	t.pStr = str;
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(ModuleSelectedColor);
	font_DrawText(&t);
}

void formatFuelValue(uint32_t coarseFuel, uqstl::span<char> buf)
{
	double dblFuelOnBoard = (double)coarseFuel / FuelTankScale;

	if (!uqm::UQMOptions::read().infiniteFuel)
	{
		if (!uqm::UQMOptions::read().wholeFuel)
		{
			fmt::format_to_sz_n(buf, "{}", coarseFuel);
		}
		else if (dblFuelOnBoard > 999.99)
		{
			fmt::format_to_sz_n(buf, "{:.1}", dblFuelOnBoard);
		}
		else
		{
			fmt::format_to_sz_n(buf, "{:.2}", dblFuelOnBoard);
		}
	}
	else
	{
		fmt::format_to_sz_n(buf, "{}", StrInfinitySign);
	}
}

char* WholeFuelValue(void)
{
	static char buf[8] {};
	const uint32_t CoarseFuel = uqm::gameState().sis().FuelOnBoard;

	formatFuelValue(CoarseFuel, {buf, sizeof(buf)});

	return buf;
}

} // namespace uqm
