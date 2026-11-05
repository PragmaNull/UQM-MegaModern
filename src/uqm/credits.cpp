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

#include <math.h>

#include "credits.h"
#include "creditsfile.h"
#include "libs/file/textfile.h"
#include "core/log/log.h"

#include "core/string/StringUtils.h"

#include "controls.h"
#include "colors.h"
#include "options.h"
#include "oscill.h"
#include "comm.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "sounds.h"
#include "setup.h"
#include "libs/graphics/drawable.h"

// Rates in pixel lines per second
#define CREDITS_BASE_RATE RES_SCALE(9)
#define CREDITS_MAX_RATE RES_SCALE(130)
// Maximum frame rate
#define CREDITS_FRAME_RATE RES_SCALE(36)

#define CREDITS_TIMEOUT (c_gameSecond * 5)

#define TRANS_COLOR BRIGHT_BLUE_COLOR

// Positive or negative scroll rate in pixel lines per second
static int CreditsRate;

static bool OutTakesRunning;
static bool CreditsRunning;
static uqstl::vector<uqm::CreditsBlock> CreditsBlocks {};
static FRAME CreditsBack;

// Context used for drawing to the screen
static GFXCONTEXT DrawContext;
// Context used for pre-rendering a credits frame
static GFXCONTEXT LocalContext;
// Pre-rendered frame, possibly with a cutout
static FRAME CreditsFrame;
// Size of the credits "window" (normally screen size)
static EXTENT CreditsExtent;

typedef struct
{
	FRAME frame;
	int strIndex;
} CreditTextFrame;

#define MAX_CREDIT_FRAMES 32
// Circular text frame buffer for scrolling
// Text frames are generated as needed, and when a text frame scrolls
// of the screen, it is destroyed
static CreditTextFrame textFrames[MAX_CREDIT_FRAMES];
// Index of first active frame in the circular buffer (the first frame
// is the one on top)
static int firstFrame;
// Index of last active frame in the circular buffer + 1
static int lastFrame;
// Total height of all active frames in the circular buffer
static int totalHeight;
// Current vertical offset into the first text frame
static int curFrameOfs;

typedef struct
{
	int size;
	RESOURCE res;
	FONT font;
} FONT_SIZE_DEF;

static FONT_SIZE_DEF CreditsFont[] =
	{
		{13, PT13AA_FONT, 0},
		{17, PT17AA_FONT, 0},
		{45, PT45AA_FONT, 0},
		{0,	0,		   0},
};


static FRAME
Credits_MakeTransFrame(int w, int h, Color TransColor)
{
	FRAME f {CaptureDrawable(CreateDrawable(WantPixmap, w, h, 1))};
	SetFrameTransparentColor(f, TransColor);

	FRAME OldFrame {SetContextFGFrame(f)};
	SetContextBackGroundColor(TransColor);
	ClearDrawable();
	SetContextFGFrame(OldFrame);

	return f;
}

static FRAME
Credits_RenderTextFrame(GFXCONTEXT TempContext, int* istr, int dir,
						Color BackColor, Color ForeColor)
{
	FRAME f {};
	FRAME OldFrame {};
	TEXT TextLines[uqm::c_creditsMaxLines];
	int i {};
	const FONT_SIZE_DEF* fdef {};
	int16_t leading {};
	TEXT t;
	GFXRECT r;
	typedef struct
	{
		TEXT_ALIGN align;
		int16_t basex;
	} col_format_t;
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

	// one TEXT for each cell; the baseline holds the column and the row.
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
		col.align = ALIGN_LEFT;
		col.basex = CreditsExtent.width / 64;
	}

	// find the right font
	for (fdef = CreditsFont; fdef->size && block.size > fdef->size; ++fdef)
		;
	if (!fdef->size)
	{
		return 0;
	}

	t.align = ALIGN_LEFT;
	t.baseline.x = 100; // any value will do
	t.baseline.y = 100; // any value will do
	t.pStr = " ";
	t.CharCount = 1;

	GFXCONTEXT OldContext {SetContext(TempContext)};

	// get font dimensions
	SetContextFont(fdef->font);
	GetContextFontLeading(&leading);
	// get left/right margin
	TextRect(&t, &r, nullptr);

	// column alignments
	for (auto&& [column, format] : uqstl::views::zip(block.columns, colfmt))
	{
		const auto basex {[&](int16_t fallback) {
			return column.x ? static_cast<int16_t>(*column.x << RESOLUTION_FACTOR) : fallback;
		}};
		switch (column.align)
		{
			case uqm::CreditsAlign::Left:
				format.align = ALIGN_LEFT;
				format.basex = basex(r.extent.width);
				break;
			case uqm::CreditsAlign::Center:
				format.align = ALIGN_CENTER;
				format.basex = basex(CreditsExtent.width / 2);
				break;
			case uqm::CreditsAlign::Right:
				format.align = ALIGN_RIGHT;
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

static inline int
frameIndex(int index)
{
	// Make sure index is positive before %
	return (index + MAX_CREDIT_FRAMES) % MAX_CREDIT_FRAMES;
}

static void
RenderCreditsScreen(GFXCONTEXT targetContext)
{
	STAMP s;

	GFXCONTEXT oldContext {SetContext(targetContext)};
	// draw background
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = CreditsBack;
	DrawStamp(&s);

	// draw text frames
	s.origin.y = -curFrameOfs;
	for (int i {firstFrame}; i != lastFrame; i = frameIndex(i + 1))
	{
		GFXRECT fr;

		s.frame = textFrames[i].frame;
		DrawStamp(&s);
		GetFrameRect(s.frame, &fr);
		s.origin.y += fr.extent.height;
	}

	if (OutTakesRunning)
	{ // Cut out the Outtakes rect
		SetContextForeGroundColor(TRANS_COLOR);
		DrawFilledRectangle(&CommWndRect);
	}

	SetContext(oldContext);
}

static void
InitCredits(void)
{
	GFXRECT ctxRect;

	memset(textFrames, 0, sizeof textFrames);

	LocalContext = CreateContext("Credits.LocalContext");
	DrawContext = CreateContext("Credits.DrawContext");

	FRAME targetFrame {GetContextFGFrame()};
	GetContextClipRect(&ctxRect);
	CreditsExtent = ctxRect.extent;

	// prep our local context
	GFXCONTEXT oldContext {SetContext(LocalContext)};
	// Local screen copy. We draw everything to this frame, then cut
	// the Outtakes rect out and draw this frame to the screen.
	CreditsFrame = Credits_MakeTransFrame(CreditsExtent.width,
										  CreditsExtent.height, TRANS_COLOR);
	SetContextFGFrame(CreditsFrame);

	// The first credits frame is fake, the height of the screen,
	// so that the credits can roll in from the bottom
	textFrames[0].frame = Credits_MakeTransFrame(1, CreditsExtent.height,
												 TRANS_COLOR);
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

	CreditsRate = CREDITS_BASE_RATE;
	CreditsRunning = true;
}

static void
freeCreditTextFrame(CreditTextFrame* tf)
{
	DestroyDrawable(ReleaseDrawable(tf->frame));
	tf->frame = nullptr;
}

static void
UninitCredits(void)
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

static int
calcDeficitHeight(void)
{
	int maxPos {-curFrameOfs};
	for (int i {firstFrame}; i != lastFrame; i = frameIndex(i + 1))
	{
		GFXRECT fr;

		GetFrameRect(textFrames[i].frame, &fr);
		maxPos += fr.extent.height;
	}

	return CreditsExtent.height - maxPos;
}

static void
processCreditsFrame(void)
{
	static TimeCount NextTime;
	const TimeCount Now = GetTimeCounter();

	if (Now >= NextTime)
	{
		GFXRECT fr;
		int direction {};
		int dirstep {};
		STAMP s;

		int rate {abs(CreditsRate)};
		if (rate != 0)
		{
			// scroll direction; forward or backward
			direction = CreditsRate / rate;
			// step in pixels
			dirstep = (rate + CREDITS_FRAME_RATE - 1) / CREDITS_FRAME_RATE;
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

		GFXCONTEXT OldContext {SetContext(DrawContext)};
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
													direction, BLACK_COLOR, CREDITS_TEXT_COLOR);
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
													direction, BLACK_COLOR, CREDITS_TEXT_COLOR);
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

static bool
LoadCredits(void)
{
	const char* const path {res_GetString(CREDITS_FILE)};
	const auto text {uqm::readTextFile(uqm::contentDir(), path)};
	auto blocks {text ? uqm::creditsFromJson(*text) : uqstl::nullopt};
	if (!blocks)
	{
		uqm::log::warn("Could not read the credits file '{}'.", path);
		return false;
	}
	CreditsBlocks = uqstl::move(*blocks);
	CreditsBack = CaptureDrawable(LoadGraphic(CREDITS_BACK_ANIM));
	// load fonts
	for (FONT_SIZE_DEF* fdef {CreditsFont}; fdef->size; ++fdef)
	{
		fdef->font = LoadFont(fdef->res);
	}

	return true;
}

static void
FreeCredits(void)
{
	CreditsBlocks.clear();

	DestroyDrawable(ReleaseDrawable(CreditsBack));
	CreditsBack = nullptr;

	// free fonts
	for (FONT_SIZE_DEF* fdef {CreditsFont}; fdef->size; ++fdef)
	{
		DestroyFont(fdef->font);
		fdef->font = nullptr;
	}
}

static void
OutTakes(void)
{
#define NUM_OUTTAKES 15
	const static CONVERSATION outtake_list[NUM_OUTTAKES] =
		{
			ZOQFOTPIK_CONVERSATION,
			TALKING_PET_CONVERSATION,
			ORZ_CONVERSATION,
			UTWIG_CONVERSATION,
			THRADD_CONVERSATION,
			SUPOX_CONVERSATION,
			SYREEN_CONVERSATION,
			SHOFIXTI_CONVERSATION,
			PKUNK_CONVERSATION,
			YEHAT_CONVERSATION,
			DRUUGE_CONVERSATION,
			URQUAN_CONVERSATION,
			VUX_CONVERSATION,
			BLACKURQ_CONVERSATION,
			ARILOU_CONVERSATION};

	const bool oldsubtitles = uqm::UQMOptions::read().subtitles.value;
	int i = 0;

	// Outtakes have no voice tracks, so the subtitles are always on
	uqm::UQMOptions::edit().subtitles = true;
	sliderDisabled = true;
	oscillDisabled = true;

	for (i = 0; (i < NUM_OUTTAKES) && !(uqm::currentActivity() & CHECK_ABORT); i++)
	{
		SetCommIntroMode(CIM_CROSSFADE_WINDOW, TimePeriod {});
		InitCommunication(outtake_list[i]);
	}

	uqm::UQMOptions::edit().subtitles = oldsubtitles;
	sliderDisabled = false;
	oscillDisabled = false;
}

typedef struct
{
	// standard state required by DoInput
	bool (*InputFunc)(void* pInputState);

	bool AllowCancel;
	bool AllowSpeedChange;
	bool CloseWhenDone;
	TimeCount CloseTimeOut;

} CREDITS_INPUT_STATE;

static bool
DoCreditsInput(void* pIS)
{
	CREDITS_INPUT_STATE* pCIS = (CREDITS_INPUT_STATE*)pIS;

	if (CreditsRunning)
	{ // cancel timeout if resumed (or just running)
		pCIS->CloseTimeOut = TimeCount {};
	}

	if ((uqm::currentActivity() & CHECK_ABORT)
		|| (pCIS->AllowCancel && (PulsedInputState.menu[KEY_MENU_SELECT] || PulsedInputState.menu[KEY_MENU_CANCEL])))
	{ // aborted
		return false;
	}

	if (pCIS->AllowSpeedChange
		&& (PulsedInputState.menu[KEY_MENU_UP]
			|| PulsedInputState.menu[KEY_MENU_DOWN]))
	{ // speed adjustment
		int newrate = CreditsRate;
		const int step = abs(CreditsRate) / 5 + 1;

		if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			newrate += step;
		}
		else if (PulsedInputState.menu[KEY_MENU_UP])
		{
			newrate -= step;
		}
		if (newrate < -CREDITS_MAX_RATE)
		{
			newrate = -CREDITS_MAX_RATE;
		}
		else if (newrate > CREDITS_MAX_RATE)
		{
			newrate = CREDITS_MAX_RATE;
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
			pCIS->CloseTimeOut = GetTimeCounter() + CREDITS_TIMEOUT;
		}
		else if (GetTimeCounter() > pCIS->CloseTimeOut)
		{ // all done!
			return false;
		}
	}

	if (!CreditsRunning
		&& (PulsedInputState.menu[KEY_MENU_SELECT]
			|| PulsedInputState.menu[KEY_MENU_CANCEL]))
	{ // credits finished and exit requested
		return false;
	}

	SleepThread(c_gameSecond / CREDITS_FRAME_RATE);

	return true;
}

static void
on_input_frame(void)
{
	processCreditsFrame();
}

void Credits(bool WithOuttakes)
{
	CREDITS_INPUT_STATE cis;
	GFXRECT screenRect;
	STAMP s;

	MUSIC_REF hMusic {LoadMusic(CREDITS_MUSIC)};

	SetContext(ScreenContext);
	SetContextClipRect(nullptr);
	GetContextClipRect(&screenRect);
	SetContextBackGroundColor(BLACK_COLOR);
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
	FadeScreen(FadeAllToColor, c_gameSecond / 2);

	// set the position of outtakes comm
	CommWndRect.corner.x = (screenRect.extent.width - CommWndRect.extent.width)
						 / 2;
	CommWndRect.corner.y = RES_SCALE(5);

	InitCredits();
	SetInputCallback(on_input_frame);

	if (WithOuttakes)
	{
		OutTakesRunning = true;
		OutTakes();
		OutTakesRunning = false;
	}

	if (!(uqm::currentActivity() & CHECK_ABORT))
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
		SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_NONE);
		DoInput(&cis, true);
	}

	SetInputCallback(nullptr);
	FadeMusic(0, c_gameSecond / 2);
	UninitCredits();

	SetContext(ScreenContext);
	SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));
	FlushColorXForms();

	if (hMusic)
	{
		StopMusic();
		DestroyMusic(hMusic);
	}
	FadeMusic(NormalVolume, TimePeriod {});

	FreeCredits();
}
