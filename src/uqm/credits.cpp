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

#include "credits.h"

#include <math.h>

#include "colors.h"
#include "comm.h"
#include "controls.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "creditsfile.h"
#include "libs/file/textfile.h"
#include "libs/graphics/drawable.h"
#include "nameref.h"
#include "options.h"
#include "oscill.h"
#include "resinst.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"

namespace uqm
{
// Rates in pixel lines per second
static auto CreditsBaseRate() { return ResScale(9); }
static auto CreditsMaxRate() { return ResScale(130); }
// Maximum frame rate
static auto CreditsFrameRate() { return ResScale(36); }

static constexpr auto CreditsTimeout {c_gameSecond * 5};


// Positive or negative scroll rate in pixel lines per second
static int CreditsRate;

static bool OutTakesRunning;
static bool CreditsRunning;
static uqstl::vector<uqm::CreditsBlock> CreditsBlocks {};
static FrameRef CreditsBack;

// Context used for drawing to the screen
static GfxContextRef DrawContext;
// Context used for pre-rendering a credits frame
static GfxContextRef LocalContext;
// Pre-rendered frame, possibly with a cutout
static FrameRef CreditsFrame;
// Size of the credits "window" (normally screen size)
static GfxExtent CreditsExtent;

struct CreditTextFrame
{
	FrameRef frame;
	int strIndex;
};

static constexpr auto MaxCreditFrames {32};
// Circular text frame buffer for scrolling
// Text frames are generated as needed, and when a text frame scrolls
// of the screen, it is destroyed
static CreditTextFrame textFrames[MaxCreditFrames];
// Index of first active frame in the circular buffer (the first frame
// is the one on top)
static int firstFrame;
// Index of last active frame in the circular buffer + 1
static int lastFrame;
// Total height of all active frames in the circular buffer
static int totalHeight;
// Current vertical offset into the first text frame
static int curFrameOfs;

struct FontSizeDef
{
	int size;
	ResourceName res;
	FontRef font;
};

static FontSizeDef CreditsFont[] =
	{
		{13, Pt13aaFontRes, 0},
		{17, Pt17aaFontRes, 0},
		{45, Pt45aaFontRes, 0},
		{0,	0,			   0},
};


static FrameRef Credits_MakeTransFrame(int w, int h, Color TransColor)
{
	FrameRef f {CaptureDrawable(CreateDrawable(CreateFlags::WantPixmap, w, h, 1))};
	SetFrameTransparentColor(f, TransColor);

	FrameRef OldFrame {SetContextFGFrame(f)};
	SetContextBackGroundColor(TransColor);
	ClearDrawable();
	SetContextFGFrame(OldFrame);

	return f;
}

static FrameRef Credits_RenderTextFrame(GfxContextRef TempContext, int* istr, int dir,
										Color BackColor, Color ForeColor)
{
	FrameRef f {};
	FrameRef OldFrame {};
	GfxText TextLines[uqm::c_creditsMaxLines];
	int i {};
	const FontSizeDef* fdef {};
	int16_t leading {};
	GfxText t;
	GfxRect r;
	struct col_format_t
	{
		TextAlign align;
		int16_t basex;
	};
	col_format_t colfmt[uqm::c_creditsMaxColumns];

	const auto blockCount {static_cast<int>(CreditsBlocks.size())};
	if (*istr < 0 || *istr >= blockCount)
	{ // check if next one is within range
		const int next_s = *istr + dir;

		if (next_s < 0 || next_s >= blockCount)
		{
			return 0;
		}

		*istr = next_s;
	}

	const uqm::CreditsBlock& block {CreditsBlocks[*istr]};
	*istr += dir;

	// one GfxText for each cell; the baseline holds the column and the row.
	// The first cell of each line comes first, then the other cells while there is space.
	const int rows {static_cast<int>(block.lines.size())};
	int cnt {0};
	const auto addCell {[&](int row, size_t column) {
		TextLines[cnt].pStr = block.lines[row][column].c_str();
		TextLines[cnt].CharCount = ~0;
		TextLines[cnt].baseline.x = static_cast<int16_t>(column);
		TextLines[cnt].baseline.y = row + 1;
		++cnt;
	}};
	for (i = 0; i < rows; ++i)
	{
		addCell(i, 0);
	}
	for (i = 0; i < rows; ++i)
	{
		for (size_t icol {1}; icol < block.lines[i].size() && cnt < static_cast<int>(uqm::c_creditsMaxLines); ++icol)
		{
			addCell(i, icol);
		}
	}

	// init alignments
	for (auto& col : colfmt)
	{
		col.align = TextAlign::Left;
		col.basex = CreditsExtent.width / 64;
	}

	// find the right font
	for (fdef = CreditsFont; fdef->size && block.size > fdef->size; ++fdef)
		;
	if (!fdef->size)
	{
		return 0;
	}

	t.align = TextAlign::Left;
	t.baseline.x = 100; // any value will do
	t.baseline.y = 100; // any value will do
	t.pStr = " ";
	t.CharCount = 1;

	GfxContextRef OldContext {SetContext(TempContext)};

	// get font dimensions
	SetContextFont(fdef->font);
	GetContextFontLeading(&leading);
	// get left/right margin
	TextRect(&t, &r, nullptr);

	// column alignments
	for (auto&& [column, format] : uqstl::views::zip(block.columns, colfmt))
	{
		const auto basex {[&](int16_t fallback) {
			return column.x ? static_cast<int16_t>(*column.x << resolutionFactor) : fallback;
		}};
		switch (column.align)
		{
			case uqm::CreditsAlign::Left:
				format.align = TextAlign::Left;
				format.basex = basex(r.extent.width);
				break;
			case uqm::CreditsAlign::Center:
				format.align = TextAlign::Center;
				format.basex = basex(CreditsExtent.width / 2);
				break;
			case uqm::CreditsAlign::Right:
				format.align = TextAlign::Right;
				format.basex = basex(CreditsExtent.width - r.extent.width);
				break;
		}
	}

	for (i = 0; i < cnt; ++i)
	{
		// baseline contains coords in row/col quantities
		const col_format_t* fmt = colfmt + TextLines[i].baseline.x;

		TextLines[i].align = fmt->align;
		TextLines[i].baseline.x = fmt->basex;
		TextLines[i].baseline.y *= leading;
	}

	f = Credits_MakeTransFrame(CreditsExtent.width, leading * rows + (leading >> 1),
							   BackColor);
	OldFrame = SetContextFGFrame(f);
	// draw text
	SetContextForeGroundColor(ForeColor);
	for (i = 0; i < cnt; ++i)
	{
		font_DrawText(TextLines + i);
	}

	SetContextFGFrame(OldFrame);
	SetContext(OldContext);

	return f;
}

static inline int frameIndex(int index)
{
	// Make sure index is positive before %
	return (index + MaxCreditFrames) % MaxCreditFrames;
}

static void RenderCreditsScreen(GfxContextRef targetContext)
{
	GfxStamp s;

	GfxContextRef oldContext {SetContext(targetContext)};
	// draw background
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = CreditsBack;
	DrawStamp(&s);

	// draw text frames
	s.origin.y = -curFrameOfs;
	for (int i {firstFrame}; i != lastFrame; i = frameIndex(i + 1))
	{
		GfxRect fr;

		s.frame = textFrames[i].frame;
		DrawStamp(&s);
		GetFrameRect(s.frame, &fr);
		s.origin.y += fr.extent.height;
	}

	if (OutTakesRunning)
	{ // Cut out the Outtakes rect
		SetContextForeGroundColor(BrightBlueColor);
		DrawFilledRectangle(&CommWndRect);
	}

	SetContext(oldContext);
}

static void InitCredits(void)
{
	GfxRect ctxRect;

	memset(textFrames, 0, sizeof textFrames);

	LocalContext = CreateContext("Credits.LocalContext");
	DrawContext = CreateContext("Credits.DrawContext");

	FrameRef targetFrame {GetContextFGFrame()};
	GetContextClipRect(&ctxRect);
	CreditsExtent = ctxRect.extent;

	// prep our local context
	GfxContextRef oldContext {SetContext(LocalContext)};
	// Local screen copy. We draw everything to this frame, then cut
	// the Outtakes rect out and draw this frame to the screen.
	CreditsFrame = Credits_MakeTransFrame(CreditsExtent.width,
										  CreditsExtent.height, BrightBlueColor);
	SetContextFGFrame(CreditsFrame);

	// The first credits frame is fake, the height of the screen,
	// so that the credits can roll in from the bottom
	textFrames[0].frame = Credits_MakeTransFrame(1, CreditsExtent.height,
												 BrightBlueColor);
	textFrames[0].strIndex = -1;
	firstFrame = 0;
	lastFrame = firstFrame + 1;

	totalHeight = GetFrameHeight(textFrames[0].frame);
	curFrameOfs = 0;

	// We use an own screen draw context to avoid collisions
	SetContext(DrawContext);
	SetContextFGFrame(targetFrame);

	SetContext(oldContext);

	// Prepare the first screen frame
	RenderCreditsScreen(LocalContext);

	CreditsRate = CreditsBaseRate();
	CreditsRunning = true;
}

static void freeCreditTextFrame(CreditTextFrame* tf)
{
	DestroyDrawable(ReleaseDrawable(tf->frame));
	tf->frame = nullptr;
}

static void UninitCredits(void)
{
	DestroyContext(DrawContext);
	DrawContext = nullptr;
	DestroyContext(LocalContext);
	LocalContext = nullptr;

	// free remaining frames
	DestroyDrawable(ReleaseDrawable(CreditsFrame));
	CreditsFrame = nullptr;
	for (; firstFrame != lastFrame; firstFrame = frameIndex(firstFrame + 1))
	{
		freeCreditTextFrame(&textFrames[firstFrame]);
	}
}

static int calcDeficitHeight(void)
{
	int maxPos {-curFrameOfs};
	for (int i {firstFrame}; i != lastFrame; i = frameIndex(i + 1))
	{
		GfxRect fr;

		GetFrameRect(textFrames[i].frame, &fr);
		maxPos += fr.extent.height;
	}

	return CreditsExtent.height - maxPos;
}

static void processCreditsFrame(void)
{
	static TimeCount NextTime;
	const TimeCount Now = GetTimeCounter();

	if (Now >= NextTime)
	{
		GfxRect fr;
		int direction {};
		int dirstep {};
		GfxStamp s;

		int rate {abs(CreditsRate)};
		if (rate != 0)
		{
			// scroll direction; forward or backward
			direction = CreditsRate / rate;
			// step in pixels
			dirstep = (rate + CreditsFrameRate() - 1) / CreditsFrameRate();
			rate = GameTicksPerSecond * dirstep / rate;
			// step is also directional
			dirstep *= direction;
		}
		else
		{ // scroll stopped
			direction = 0;
			dirstep = 0;
			// one second interframe
			rate = GameTicksPerSecond;
		}

		NextTime = GetTimeCounter() + TimePeriod {rate};

		// draw the credits
		//  comm animations play with contexts so we need to make
		//  sure the context is not desynced
		s.origin.x = 0;
		s.origin.y = 0;
		s.frame = CreditsFrame;

		GfxContextRef OldContext {SetContext(DrawContext)};
		DrawStamp(&s);
		SetContext(OldContext);
		FlushGraphics();

		// prepare next screen frame
		const int deficitHeight {calcDeficitHeight()};
		curFrameOfs += dirstep;
		// cap scroll
		if (curFrameOfs < -(CreditsExtent.height / 20))
		{ // at the begining, deceleration
			if (CreditsRate < 0)
			{
				CreditsRate -= CreditsRate / 10 - 1;
			}
		}
		else if (deficitHeight > CreditsExtent.height / 25)
		{ // frame deficit -- credits almost over, deceleration
			if (CreditsRate > 0)
			{
				CreditsRate -= CreditsRate / 10 + 1;
			}

			CreditsRunning = (CreditsRate != 0);
		}
		else if (!CreditsRunning)
		{ // resumed
			CreditsRunning = true;
		}

		if (firstFrame != lastFrame)
		{ // clean up frames that scrolled off the screen
			if (direction > 0)
			{ // forward scroll
				GetFrameRect(textFrames[firstFrame].frame, &fr);
				if (curFrameOfs >= fr.extent.height)
				{ // past this frame already
					totalHeight -= fr.extent.height;
					freeCreditTextFrame(&textFrames[firstFrame]);
					// next frame
					firstFrame = frameIndex(firstFrame + 1);
					curFrameOfs -= fr.extent.height;
				}
			}
			else if (direction < 0)
			{ // backward scroll
				const int index = frameIndex(lastFrame - 1);

				GetFrameRect(textFrames[index].frame, &fr);
				const int framePos {totalHeight - curFrameOfs - fr.extent.height};
				if (framePos >= CreditsExtent.height)
				{ // past this frame already
					lastFrame = index;
					totalHeight -= fr.extent.height;
					freeCreditTextFrame(&textFrames[lastFrame]);
				}
			}
		}

		// render new text frames if needed
		if (direction > 0)
		{ // forward scroll
			int next_s = 0;

			// get next string
			if (firstFrame != lastFrame)
			{
				next_s = textFrames[frameIndex(lastFrame - 1)].strIndex + 1;
			}

			while (totalHeight - curFrameOfs < CreditsExtent.height
				   && next_s < static_cast<int>(CreditsBlocks.size()))
			{
				CreditTextFrame* tf = &textFrames[lastFrame];

				tf->frame = Credits_RenderTextFrame(LocalContext, &next_s,
													direction, BlackColor, CreditsTextColor);
				tf->strIndex = next_s - 1;
				if (tf->frame)
				{
					GetFrameRect(tf->frame, &fr);
					totalHeight += fr.extent.height;

					lastFrame = frameIndex(lastFrame + 1);
				}
			}
		}
		else if (direction < 0)
		{ // backward scroll
			int next_s = static_cast<int>(CreditsBlocks.size()) - 1;

			// get next string
			if (firstFrame != lastFrame)
			{
				next_s = textFrames[firstFrame].strIndex - 1;
			}

			while (curFrameOfs < 0 && next_s >= 0)
			{
				const int index = frameIndex(firstFrame - 1);
				CreditTextFrame* tf = &textFrames[index];

				tf->frame = Credits_RenderTextFrame(LocalContext, &next_s,
													direction, BlackColor, CreditsTextColor);
				tf->strIndex = next_s + 1;
				if (tf->frame)
				{
					GetFrameRect(tf->frame, &fr);
					totalHeight += fr.extent.height;

					firstFrame = index;
					curFrameOfs += fr.extent.height;
				}
			}
		}

		// draw next screen frame
		RenderCreditsScreen(LocalContext);
	}
}

static bool LoadCredits(void)
{
	const char* const path {res_GetString(CreditsFileRes)};
	const auto text {uqm::readTextFile(uqm::contentDir(), path)};
	auto blocks {text ? uqm::creditsFromJson(*text) : uqstl::nullopt};
	if (!blocks)
	{
		uqm::log::warn("Could not read the credits file '{}'.", path);
		return false;
	}
	CreditsBlocks = uqstl::move(*blocks);
	CreditsBack = CaptureDrawable(LoadGraphic(CreditsBackAnimRes));
	// load fonts
	for (FontSizeDef* fdef {CreditsFont}; fdef->size; ++fdef)
	{
		fdef->font = LoadFont(fdef->res);
	}

	return true;
}

static void FreeCredits(void)
{
	CreditsBlocks.clear();

	DestroyDrawable(ReleaseDrawable(CreditsBack));
	CreditsBack = nullptr;

	// free fonts
	for (FontSizeDef* fdef {CreditsFont}; fdef->size; ++fdef)
	{
		DestroyFont(fdef->font);
		fdef->font = nullptr;
	}
}

static constexpr auto NumOuttakes {15};

static void OutTakes(void)
{
	const static Conversation outtake_list[NumOuttakes] =
		{
			Conversation::Zoqfotpik,
			Conversation::TalkingPet,
			Conversation::Orz,
			Conversation::Utwig,
			Conversation::Thradd,
			Conversation::Supox,
			Conversation::Syreen,
			Conversation::Shofixti,
			Conversation::Pkunk,
			Conversation::Yehat,
			Conversation::Druuge,
			Conversation::Urquan,
			Conversation::Vux,
			Conversation::Blackurq,
			Conversation::Arilou};

	const bool oldsubtitles = uqm::UQMOptions::read().subtitles.value;
	int i = 0;

	// Outtakes have no voice tracks, so the subtitles are always on
	uqm::UQMOptions::edit().subtitles = true;
	sliderDisabled = true;
	oscillDisabled = true;

	for (i = 0; (i < NumOuttakes) && !(uqm::currentActivity() & ActivityFlag::CheckAbort); i++)
	{
		SetCommIntroMode(CommIntroMode::CrossfadeWindow, TimePeriod {});
		InitCommunication(outtake_list[i]);
	}

	uqm::UQMOptions::edit().subtitles = oldsubtitles;
	sliderDisabled = false;
	oscillDisabled = false;
}

struct CreditsInputState
{
	// standard state required by DoInput
	bool (*InputFunc)(void* pInputState);

	bool AllowCancel;
	bool AllowSpeedChange;
	bool CloseWhenDone;
	TimeCount CloseTimeOut;
};

static bool DoCreditsInput(void* pIS)
{
	CreditsInputState* pCIS = (CreditsInputState*)pIS;

	if (CreditsRunning)
	{ // cancel timeout if resumed (or just running)
		pCIS->CloseTimeOut = TimeCount {};
	}

	if ((uqm::currentActivity() & ActivityFlag::CheckAbort)
		|| (pCIS->AllowCancel && (PulsedInputState.menu[*MenuKey::MenuSelect] || PulsedInputState.menu[*MenuKey::MenuCancel])))
	{ // aborted
		return false;
	}

	if (pCIS->AllowSpeedChange
		&& (PulsedInputState.menu[*MenuKey::MenuUp]
			|| PulsedInputState.menu[*MenuKey::MenuDown]))
	{ // speed adjustment
		int newrate = CreditsRate;
		const int step = abs(CreditsRate) / 5 + 1;

		if (PulsedInputState.menu[*MenuKey::MenuDown])
		{
			newrate += step;
		}
		else if (PulsedInputState.menu[*MenuKey::MenuUp])
		{
			newrate -= step;
		}
		if (newrate < -CreditsMaxRate())
		{
			newrate = -CreditsMaxRate();
		}
		else if (newrate > CreditsMaxRate())
		{
			newrate = CreditsMaxRate();
		}

		CreditsRate = newrate;
	}

	if (!CreditsRunning)
	{ // always allow cancelling once credits run through
		pCIS->AllowCancel = true;
	}

	if (!CreditsRunning && pCIS->CloseWhenDone)
	{ // auto-close controlled by timeout
		if (pCIS->CloseTimeOut == TimeCount {})
		{ // set timeout
			pCIS->CloseTimeOut = GetTimeCounter() + CreditsTimeout;
		}
		else if (GetTimeCounter() > pCIS->CloseTimeOut)
		{ // all done!
			return false;
		}
	}

	if (!CreditsRunning
		&& (PulsedInputState.menu[*MenuKey::MenuSelect]
			|| PulsedInputState.menu[*MenuKey::MenuCancel]))
	{ // credits finished and exit requested
		return false;
	}

	SleepThread(c_gameSecond / CreditsFrameRate());

	return true;
}

static void on_input_frame(void)
{
	processCreditsFrame();
}

void Credits(bool WithOuttakes)
{
	CreditsInputState cis;
	GfxRect screenRect;
	GfxStamp s;

	MusicRef hMusic {LoadMusicInstance(CreditsMusicRes)};

	SetContext(ScreenContext);
	SetContextClipRect(nullptr);
	GetContextClipRect(&screenRect);
	SetContextBackGroundColor(BlackColor);
	ClearDrawable();

	if (!LoadCredits())
	{
		return;
	}

	// Fade in the background
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = CreditsBack;
	DrawStamp(&s);
	FadeScreen(ScreenFadeType::FadeAllToColor, c_gameSecond / 2);

	// set the position of outtakes comm
	CommWndRect.corner.x = (screenRect.extent.width - CommWndRect.extent.width)
						 / 2;
	CommWndRect.corner.y = ResScale(5);

	InitCredits();
	SetInputCallback(on_input_frame);

	if (WithOuttakes)
	{
		OutTakesRunning = true;
		OutTakes();
		OutTakesRunning = false;
	}

	if (!(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		if (hMusic)
		{
			PlayMusic(hMusic, true, 1);
		}

		// nothing to do now but wait until credits
		//  are done or canceled by user
		cis.InputFunc = DoCreditsInput;
		cis.AllowCancel = !WithOuttakes;
		cis.CloseWhenDone = !WithOuttakes;
		cis.AllowSpeedChange = true;
		SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
		DoInput(&cis, true);
	}

	SetInputCallback(nullptr);
	FadeMusic(0, c_gameSecond / 2);
	UninitCredits();

	SetContext(ScreenContext);
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
	FlushColorXForms();

	if (hMusic)
	{
		StopMusic();
		DestroyMusic(hMusic);
	}
	FadeMusic(NormalVolume, TimePeriod {});

	FreeCredits();
}

} // namespace uqm
