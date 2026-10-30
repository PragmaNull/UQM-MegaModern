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

#define COMM_INTERNAL
#include "comm.h"

#include "build.h"
#include "commanim.h"
#include "commglue.h"
#include "controls.h"
#include "colors.h"
#include "hyper.h"
#include "sis.h"
#include "units.h"
#include "encount.h"
#include "starmap.h"
#include "endian_uqm.h"
#include "gamestr.h"
#include "options.h"
#include "oscill.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "nameref.h"
#include "uqmdebug.h"
#include "lua/luacomm.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/sound/sound.h"
#include "libs/sound/trackplayer.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "menustat.h"
// for free_gravity_well() & load_gravity_well()
#include "cons_res.h"

#include <ctype.h>

#define MAX_RESPONSES 8

// Oscilloscope frame rate
// Should be <= COMM_ANIM_RATE
// XXX: was 32 picked experimentally?
#define OSCILLOSCOPE_RATE (GameTicksPerSecond / chooseIfHd(32, 60))

// Maximum comm animation frame rate (actual execution rate)
// A gfx frame is not always produced during an execution frame,
// and several animations are combined into one gfx frame.
// The rate was originally 120fps which allowed for more animation
// precision which is ultimately wasted on the human eye anyway.
// The highest known stable animation rate is 40fps, so that's what we use.
#define COMM_ANIM_RATE (GameTicksPerSecond / 40)

static GFXCONTEXT AnimContext;

LOCDATA CommData;
char shared_phrase_buf[2048];
FONT ComputerFont;

static bool TalkingFinished;
static CommIntroMode curIntroMode = CIM_DEFAULT;
static TimeCount fadeTime;

// Dark mode
bool IsDarkMode = false;
bool cwLock = false; // To avoid drawing over comWindow if JumpTrack() is called
uint8_t altResFlags = 0;
static uint16_t fadeIndex;

typedef struct response_entry
{
	RESPONSE_REF response_ref;
	TEXT response_text;
	RESPONSE_FUNC response_func;
	char* allocedResponse;
} RESPONSE_ENTRY;

typedef struct encounter_state
{
	bool (*InputFunc)(struct encounter_state* pES);

	uint16_t Initialized;
	TimeCount NextTime; // framerate control
	uint8_t num_responses;
	uint8_t cur_response;
	uint8_t top_response;
	RESPONSE_ENTRY response_list[MAX_RESPONSES];

	char phrase_buf[1024];
} ENCOUNTER_STATE;

// Required to set custom baseline per one sentence
typedef struct CustomBaseline
{
	uint16_t index;
	GFXPOINT baseline;
	TEXT_ALIGN align;
	struct CustomBaseline* next;

} CUSTOM_BASELINE;

static CUSTOM_BASELINE* head_node; // Head node of custom baseline list
static CUSTOM_BASELINE* cur_node;  // not null if current sentence number has been found in the list

// Used to disable/enable talking animation
// Better than current because it works with rewind
static bool haveTalkingLock = false;
static uint16_t startSentence, endSentence;

static ENCOUNTER_STATE* pCurInputState;

static bool clear_subtitles;
static bool next_page = false;
static TEXT SubtitleText;
static const char* last_subtitle;

static GFXCONTEXT TextCacheContext;
static FRAME TextCacheFrame;


GFXRECT CommWndRect = {
	// default values; actually inited by HailAlien()
	{0, 0},
	{0, 0}  //was {SIS_ORG_X, SIS_ORG_Y},
};

static void ClearSubtitles(void);
static void CheckSubtitles(bool really);
static void RedrawSubtitles(void);
static void runCommAnimFrame(void);
static bool PauseSubtitles(bool force);


/* _count_lines - sees how many lines a given input string would take to
 * display given the line wrapping information
 */
static int
_count_lines(TEXT* pText)
{
	int16_t text_width;
	const char* pStr;
	int numLines = 0;
	bool eol;

	text_width = CommData.AlienTextWidth;
	SetContextFont(CommData.AlienFont);

	pStr = pText->pStr;
	do
	{
		++numLines;
		pText->pStr = pStr;
		eol = getLineWithinWidth(pText, &pStr, text_width, (uint16_t)~0);
	} while (!eol);
	pText->pStr = pStr;

	return numLines;
}

// status == -1: draw highlighted player dialog option
// status == -2: draw non-highlighted player dialog option
// status == -4: use current context, and baseline from pTextIn
// status ==  1:  draw alien speech; subtitle cache is used
static int16_t
add_text(int status, TEXT* pTextIn)
{
	uint16_t maxchars, numchars;
	TEXT locText;
	TEXT* pText;
	int16_t leading;
	const char* pStr;
	int16_t text_width;
	int num_lines = 0;
	static int16_t last_baseline;
	bool eol;
	GFXCONTEXT OldContext = nullptr;
	GFXRECT arrow;

	BatchGraphics();

	GetFrameRect(SetAbsFrameIndex(ActivityFrame, 6), &arrow);

	maxchars = (uint16_t)~0;
	if (status == 1)
	{
		if (last_subtitle == pTextIn->pStr)
		{
			// draws cached subtitle
			STAMP s;

			s.origin.x = 0;
			s.origin.y = 0;
			s.frame = TextCacheFrame;
			DrawStamp(&s);
			UnbatchGraphics();
			return last_baseline;
		}
		else
		{
			// draw to subtitle cache; prepare first
			OldContext = SetContext(TextCacheContext);
			ClearDrawable();

			last_subtitle = pTextIn->pStr;
		}

		text_width = CommData.AlienTextWidth;
		if (CommData.AlienConv == ORZ_CONVERSATION && uqm::UQMOptions::read().orzCompFont && CommData.AlienTalkDesc.AnimFlags & PAUSE_TALKING)
		{
			SetContextFont(ComputerFont); // Orz intro
		}
		else
		{
			SetContextFont(CommData.AlienFont);
		}
		GetContextFontLeading(&leading);

		pText = pTextIn;
	}
	else if (GetContextFontLeading(&leading), status <= -4)
	{
		text_width = (int16_t)(SIS_SCREEN_WIDTH - RES_SCALE(8)
							   - (TEXT_X_OFFS << 2)
							   - (arrow.extent.width + RES_SCALE(2)));

		pText = pTextIn;
	}
	else
	{
		text_width = (int16_t)(SIS_SCREEN_WIDTH - RES_SCALE(8)
							   - (TEXT_X_OFFS << 2)
							   - (arrow.extent.width + RES_SCALE(2)));

		switch (status)
		{
			case -3:
				{
					// Unknown. Never reached; color matches the background color.
					// reused to mimic DOS
					if (!IsDarkMode)
					{
						SetContextForeGroundColor(
							BUILD_COLOR(
								MAKE_RGB15(0x00, 0x00, 0x14), 0x01));
					}
					else
					{
						SetContextForeGroundColor(BLACK_COLOR);
					}
					break;
				}
			case -2:
				{ // Not highlighted dialog options.
					if (!IsDarkMode)
					{
						SetContextForeGroundColor(
							COMM_PLAYER_TEXT_NORMAL_COLOR);
					}
					else
					{
						SetContextForeGroundColor(
							BUILD_COLOR_RGBA(0x00, 0x00, 0xAA, 0xFF));
					}
					break;
				}
			case -1:
				{ // Currently highlighted dialog option.
					if (!IsDarkMode)
					{
						SetContextForeGroundColor(
							COMM_PLAYER_TEXT_HIGHLIGHT_COLOR);
					}
					else
					{
						SetContextForeGroundColor(
							BUILD_COLOR_RGBA(0x55, 0x55, 0xFF, 0xFF));
					}
					break;
				}
		}

		maxchars = pTextIn->CharCount;
		locText = *pTextIn;
		locText.baseline.x -= RES_SCALE(8);
		locText.CharCount = (uint16_t)~0;
		locText.pStr = STR_BULLET;
		font_DrawText(&locText);

		locText = *pTextIn;
		pText = &locText;
		pText->baseline.y -= leading;
	}

	numchars = 0;
	pStr = pText->pStr;

	if (status > 0 && (CommData.AlienTextValign & (VALIGN_MIDDLE | VALIGN_BOTTOM)))
	{
		num_lines = _count_lines(pText);
		if (CommData.AlienTextValign == VALIGN_BOTTOM)
		{
			pText->baseline.y -= (leading * num_lines);
		}
		else if (CommData.AlienTextValign == VALIGN_MIDDLE)
		{
			pText->baseline.y -= ((leading * num_lines) / 2);
		}
		if (pText->baseline.y < 0)
		{
			pText->baseline.y = 0;
		}
	}

	do
	{
		pText->pStr = pStr;
		pText->baseline.y += leading;

		eol = getLineWithinWidth(pText, &pStr, text_width, maxchars);

		maxchars -= pText->CharCount;
		if (maxchars != 0)
		{
			--maxchars;
		}
		numchars += pText->CharCount;

		if (status <= 0)
		{
			// Player dialog option or (status == -4) other non-alien
			// text.
			if (pText->baseline.y < SIS_SCREEN_HEIGHT)
			{
				font_DrawText(pText);
			}

			if (status < -4 && pText->baseline.y >= -status - 10)
			{
				// Never actually reached. Status is never <-4.
				++pStr;
				break;
			}
		}
		else
		{
			// Alien speech
			if (CommData.AlienConv == ORZ_CONVERSATION && uqm::UQMOptions::read().orzCompFont)
			{
				font_DrawTracedTextAlt(pText,
									   CommData.AlienTextFColor,
									   CommData.AlienTextBColor,
									   ComputerFont, '*');
			}
			else
			{
				// Normal case : other races than Orz
				font_DrawTracedText(pText,
									CommData.AlienTextFColor,
									CommData.AlienTextBColor);
			}
		}
	} while (!eol && maxchars);
	pText->pStr = pStr;

	if (status == 1)
	{
		STAMP s;

		// We were drawing to cache -- flush to screen
		SetContext(OldContext);
		s.origin.x = s.origin.y = 0;
		s.frame = TextCacheFrame;
		DrawStamp(&s);

		last_baseline = pText->baseline.y;
	}

	UnbatchGraphics();
	return (pText->baseline.y);
}

void GetCustomBaseline(uint16_t i)
{
	if (head_node == nullptr)
	{
		return;
	}
	else
	{ // Set cur_node if current sentence number equals to one of list
		// indices. MUST be nullptr otherwise
		cur_node = head_node;
		while (cur_node != nullptr && cur_node->index != i)
		{
			cur_node = cur_node->next;
		}
	}
}

void CheckTalkingAnim(uint16_t i)
{
	if (haveTalkingLock) // Do not check if there is no lock
	{
		if (i >= startSentence && i <= endSentence)
		{
			CommData.AlienTalkDesc.AnimFlags &= ~WAIT_TALKING;
			EnableTalkingAnim(false);
			ShutYourMouth();
		}
		else
		{
			CommData.AlienTalkDesc.AnimFlags |= WAIT_TALKING;
			EnableTalkingAnim(true);
		}
	}
}

void BlockTalkingAnim(uint16_t trackStart, uint16_t trackEnd)
{
	if (trackStart >= trackEnd) // Fool-proof
	{
		return;
	}

	haveTalkingLock = true;
	startSentence = GetSubtitleNumberByTrack(trackStart);
	// First sentence of locked interval
	endSentence = GetSubtitleNumberByTrack(trackEnd) - 1;
	// Last sentence of locked interval
}

void ReleaseTalkingAnim(void)
{ // Called at restart and at SelectResponce ()
	if (haveTalkingLock)
	{
		haveTalkingLock = false;
		startSentence = (uint16_t)~0;
		endSentence = (uint16_t)~0;
		EnableTalkingAnim(true);
	}
}


// This function calculates how much of a string can be fitted within
// a specific width, up to a newline or terminating \0.
// pText is the text to be fitted. pText->CharCount will be set to the
// number of characters that fitted.
// startNext will be filled with the start of the first word that
// doesn't fit in one line, or if an entire line fits, to the character
// past the newline, or if the entire string fits, to the end of the
// string.
// maxWidth is the maximum number of pixels that a line may be wide
//   ASSUMPTION: there are no words in the text wider than maxWidth
// maxChars is the maximum number of characters (not bytes) that are to
// be fitted.
// true is returned if a complete line fitted
// false otherwise
bool getLineWithinWidth(TEXT* pText, const char** startNext,
						int16_t maxWidth, uint16_t maxChars)
{
	bool eol;
	// The end of the line of text has been reached.
	bool done;
	// We cannot add any more words.
	GFXRECT rect;
	uint16_t oldCount;
	const char* ptr;
	const char* wordStart;
	uqm::UniChar ch;
	uint16_t charCount;

	//GetContextClipRect (&rect);

	eol = false;
	done = false;
	oldCount = 1;
	charCount = 0;
	ch = '\0';
	ptr = pText->pStr;
	for (;;)
	{
		wordStart = ptr;

		// Scan one word.
		for (;;)
		{
			if (*ptr == '\0')
			{
				eol = true;
				done = true;
				break;
			}
			ch = uqm::getCharFromString(&ptr);
			eol = ch == '\0' || ch == '\n' || ch == '\r';
			done = eol || charCount >= maxChars;
			if (done || ch == ' ')
			{
				break;
			}
			charCount++;
		}

		oldCount = pText->CharCount;
		pText->CharCount = charCount;
		TextRect(pText, &rect, nullptr);

		if (rect.extent.width >= maxWidth)
		{
			pText->CharCount = oldCount;
			*startNext = wordStart;
			return false;
		}

		if (done)
		{
			*startNext = ptr;
			return eol;
		}
		charCount++;
		// For the space in between words.
	}
}

void DrawCommBorder(GFXRECT r)
{
	GFXRECT clipRect;
	GFXRECT oldClipRect;

	GetContextClipRect(&oldClipRect);

	// Expand the context clip-rect so that we can tweak the existing border
	clipRect = oldClipRect;
	clipRect.corner.x -= RES_SCALE(1);
	clipRect.extent.width += RES_SCALE(2);
	SetContextClipRect(&clipRect);

	// Border foreground
	SetContextForeGroundColor(MENU_FOREGROUND_COLOR);
	r.corner.y -= SLIDER_HEIGHT;
	r.extent.width = clipRect.extent.width;
	r.extent.height = SLIDER_HEIGHT;
	DrawFilledRectangle(&r);

	// Border top shadow line
	SetContextForeGroundColor(SIS_BOTTOM_RIGHT_BORDER_COLOR);
	r.extent.height = RES_SCALE(1);
	DrawFilledRectangle(&r);

	// Border bottom shadow line
	SetContextForeGroundColor(SIS_LEFT_BORDER_COLOR);
	r.corner.y += SLIDER_HEIGHT - RES_SCALE(1);
	DrawFilledRectangle(&r);

	SetContextClipRect(&oldClipRect);

	DrawBorder(SIS_COMM_FRAME);
}

static void
DrawSISComWindow(void)
{
	GFXCONTEXT OldContext;

	if (uqm::lowByte(GLOBAL(CurrentActivity)) != WON_LAST_BATTLE)
	{
		GFXRECT r;

		OldContext = SetContext(SpaceContext);

		r.corner.x = 0;
		r.corner.y = SLIDER_Y + SLIDER_HEIGHT;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;
		if (!IsDarkMode)
		{
			SetContextForeGroundColor(COMM_PLAYER_BACKGROUND_COLOR);
		}
		else
		{
			SetContextForeGroundColor(BLACK_COLOR);
		}
		DrawFilledRectangle(&r);

		if (!usingSpeech && uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::PC && !IsDarkMode)
		{
			DrawCommBorder(r);
		}

		SetContext(OldContext);
	}
}

void init_communication(void)
{
	// now a no-op
}

void uninit_communication(void)
{
	// now a no-op
}

static void
RefreshResponsesSpecial(ENCOUNTER_STATE* pES)
{ // PC style repsonses
	int16_t y;
	uint8_t response;
	int16_t leading;

	SetContext(SpaceContext);
	GetContextFontLeading(&leading);
	BatchGraphics();

	DrawSISComWindow();
	y = SLIDER_Y + SLIDER_HEIGHT + RES_SCALE(is3DO(uqm::UQMOptions::read().whichFonts));
	for (response = pES->top_response; response < pES->num_responses;
		 ++response)
	{
		pES->response_list[response].response_text.baseline.x =
			TEXT_X_OFFS + RES_SCALE(8);
		pES->response_list[response].response_text.baseline.y =
			y + leading;
		pES->response_list[response].response_text.align = ALIGN_LEFT;
		if (response == pES->cur_response)
		{
			y = add_text(-1, &pES->response_list[response].response_text);
		}
		else
		{
			y = add_text(-3, &pES->response_list[response].response_text);
		}
	}

	UnbatchGraphics();
}

static void
RefreshResponses(ENCOUNTER_STATE* pES)
{
	int16_t y;
	uint8_t response;
	int16_t leading;
	STAMP s;


	SetContext(SpaceContext);
	GetContextFontLeading(&leading);
	BatchGraphics();

	DrawSISComWindow();
	y = SLIDER_Y + SLIDER_HEIGHT + RES_SCALE(is3DO(uqm::UQMOptions::read().whichFonts));
	for (response = pES->top_response; response < pES->num_responses;
		 ++response)
	{
		pES->response_list[response].response_text.baseline.x =
			TEXT_X_OFFS + RES_SCALE(8);
		pES->response_list[response].response_text.baseline.y =
			y + leading;
		pES->response_list[response].response_text.align = ALIGN_LEFT;
		if (response == pES->cur_response)
		{
			y = add_text(-1, &pES->response_list[response].response_text);
		}
		else
		{
			y = add_text(-2, &pES->response_list[response].response_text);
		}
	}

	if (pES->top_response)
	{
		s.origin.y = SLIDER_Y + SLIDER_HEIGHT + RES_SCALE(1);
		s.frame = SetAbsFrameIndex(ActivityFrame, 6);
	}
	else if (y > SIS_SCREEN_HEIGHT)
	{
		s.origin.y = SIS_SCREEN_HEIGHT - RES_SCALE(2);
		s.frame = SetAbsFrameIndex(ActivityFrame, 7);
	}
	else
	{
		s.frame = 0;
	}
	if (s.frame)
	{
		GFXRECT r;

		GetFrameRect(s.frame, &r);
		s.origin.x = SIS_SCREEN_WIDTH - r.extent.width - RES_SCALE(1);
		DrawStamp(&s);
	}

	UnbatchGraphics();
}

static void
FeedbackPlayerPhrase(char* pStr)
{
	SetContext(SpaceContext);

	BatchGraphics();
	DrawSISComWindow();
	if (pStr[0])
	{
		TEXT ct;

		ct.baseline.x = RES_SCALE(ORIG_SIS_SCREEN_WIDTH >> 1);
		ct.baseline.y = SLIDER_Y + SLIDER_HEIGHT + RES_SCALE(13);
		ct.align = ALIGN_CENTER;
		ct.CharCount = (uint16_t)~0;

		if (!IsDarkMode)
		{
			ct.pStr = GAME_STRING(FEEDBACK_STRING_BASE);
			// "(In response to your statement)"
			SetContextForeGroundColor(COMM_RESPONSE_INTRO_TEXT_COLOR);
			font_DrawText(&ct);
			// Your response
			SetContextForeGroundColor(COMM_FEEDBACK_TEXT_COLOR);
		}
		else
		{
			SetContextForeGroundColor(
				BUILD_COLOR_RGBA(0x55, 0x55, 0xFF, 0xFF));
		}

		ct.baseline.y += RES_SCALE(16);
		ct.pStr = pStr;
		add_text(-4, &ct);
	}
	UnbatchGraphics();
}

static void
InitSpeechGraphics(void)
{
	if (uqm::UQMOptions::read().scopeStyle != uqm::EmulationMode::PC)
	{
		InitOscilloscope(SetAbsFrameIndex(ActivityFrame, 9));
	}
	else
	{
		InitOscilloscope(SetAbsFrameIndex(ActivityFrame, 10));
	}

	InitSlider(0, SLIDER_Y, SIS_SCREEN_WIDTH,
			   SetAbsFrameIndex(ActivityFrame, 5),
			   SetAbsFrameIndex(ActivityFrame, 2));
}

static void
UpdateSpeechGraphics(void)
{
	static TimeCount NextTime;
	GFXCONTEXT OldContext;

	if (GetTimeCounter() < NextTime)
	{
		return; // too early
	}

	NextTime = GetTimeCounter() + OSCILLOSCOPE_RATE;

	OldContext = SetContext(RadarContext);
	DrawOscilloscope();
	SetContext(SpaceContext);
	DrawSlider();
	SetContext(OldContext);
}

static void
UpdateAnimations(bool paused)
{
	static TimeCount NextTime;
	GFXCONTEXT OldContext;
	bool change;

	if (GetTimeCounter() < NextTime)
	{
		return; // too early
	}

	NextTime = GetTimeCounter() + COMM_ANIM_RATE;

	OldContext = SetContext(AnimContext);
	BatchGraphics();
	// Advance and draw ambient, transit and talk animations
	change = ProcessCommAnimations(clear_subtitles, paused);
	if (change || clear_subtitles)
	{
		RedrawSubtitles();
	}
	UnbatchGraphics();
	clear_subtitles = false;
	SetContext(OldContext);
}

void UpdateDuty(bool talk)
{
	if (talk)
	{
		if (!TalkingFinished)
		{
			setRunTalkingAnim();
		}
		CheckSubtitles(true);
		UpdateAnimations(false);
	}
	UpdateSpeechGraphics();
}

GFXRECT DarkModeRect[6];

void InitUIRects(bool state)
{
	if (state)
	{
		// Top border
		DarkModeRect[0].corner = MAKE_POINT(0, 0);
		DarkModeRect[0].extent = MAKE_EXTENT(
			SIS_ORG_X + SIS_SCREEN_WIDTH, SIS_ORG_Y);

		// Left border
		DarkModeRect[1].corner = MAKE_POINT(0, SIS_ORG_Y);
		DarkModeRect[1].extent = MAKE_EXTENT(
			SIS_ORG_X, SCREEN_HEIGHT - SIS_ORG_Y);

		// Right border
		DarkModeRect[2].corner = MAKE_POINT(
			SIS_ORG_X + SIS_SCREEN_WIDTH, 0);
		DarkModeRect[2].extent = MAKE_EXTENT(
			SCREEN_WIDTH - DarkModeRect[2].corner.x, SCREEN_HEIGHT);

		// Bottom border
		DarkModeRect[3].corner = MAKE_POINT(SIS_ORG_X,
											SIS_ORG_Y + SIS_SCREEN_HEIGHT);
		DarkModeRect[3].extent = MAKE_EXTENT(SIS_SCREEN_WIDTH,
											 SCREEN_HEIGHT - SIS_ORG_Y + SIS_SCREEN_HEIGHT);

		// Slider
		DarkModeRect[4].corner = MAKE_POINT(
			SIS_ORG_X, SIS_ORG_Y + SLIDER_Y);
		DarkModeRect[4].extent = MAKE_EXTENT(
			SIS_SCREEN_WIDTH, SLIDER_HEIGHT);

		// Com Window with player responses
		DarkModeRect[5].corner = MAKE_POINT(
			SIS_ORG_X, SIS_ORG_Y + SLIDER_Y + SLIDER_HEIGHT);
		DarkModeRect[5].extent = MAKE_EXTENT(SIS_SCREEN_WIDTH,
											 SIS_SCREEN_HEIGHT - (SLIDER_Y + SLIDER_HEIGHT));
	}
	else
	{
		memset(&DarkModeRect, 0, sizeof(DarkModeRect));
	}
}

static void
FadePlayerUI(void)
{
	if (!IsDarkMode)
	{
		return;
	}
	else
	{
		static TimeCount NextTime;
		GFXCONTEXT OldContext;
		uint8_t i;

		// emulating as close as possible PC-DOS text glow
		static const Color DarkModeFGTextColor[] = DARKMODE_FG_TABLE;
		// it was not instantanious, but also looked like fading shenanigans
		static const Color DarkModeBGTextColor[] = DARKMODE_BG_TABLE;

		if (fadeIndex > 50)
		{
			cwLock = false;
			return;
		}

		if (GetTimeCounter() < NextTime)
		{
			return;
		}

		NextTime = GetTimeCounter() + GameTicksPerSecond / 15;

		OldContext = SetContext(ScreenContext);

		SetContextForeGroundColor(
			BUILD_COLOR_RGBA(
				0x00, 0x00, 0x00, DRAW_FACTOR_1 * 0.02 * fadeIndex));

		BatchGraphics();

		for (i = 0; i < (std::size(DarkModeRect) - 1); i++)
		{
			DrawFilledRectangle(&DarkModeRect[i]);
		}

		if (fadeIndex < 16)
		{
			ClearSubtitles();
			CommData.AlienTextFColor = DarkModeFGTextColor[fadeIndex];
			CommData.AlienTextBColor = DarkModeBGTextColor[fadeIndex];
			RedrawSubtitles();
		}

		if (cwLock)
		{
			DrawFilledRectangle(&DarkModeRect[5]);
		}

		UnbatchGraphics();
		SetContext(OldContext);
		fadeIndex++;
	}
}

static void
UpdateCommGraphics(void)
{
	UpdateAnimations(false);
	UpdateSpeechGraphics();
	FadePlayerUI();
}

// Derived from INPUT_STATE_DESC
typedef struct talking_state
{
	// Fields required by DoInput()
	bool (*InputFunc)(struct talking_state*);

	TimeCount NextTime; // framerate control
	uint16_t waitTrack;
	bool rewind;
	bool seeking;
	bool ended;

} TALKING_STATE;

static bool
DoTalkSegue(TALKING_STATE* pTS)
{
	bool left = false;
	bool right = false;
	uint16_t curTrack;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		pTS->ended = true;
		return false;
	}

	if (uqm::UQMOptions::read().useSpeech || uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::Console3DO || (uqm::lowByte(GLOBAL(CurrentActivity)) == WON_LAST_BATTLE))
	{
		if (PulsedInputState.menu[KEY_MENU_CANCEL])
		{
			JumpTrack();
			pTS->ended = true;
			return false;
		}

		if (uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::PC || !uqm::UQMOptions::read().useSpeech)
		{
			left = PulsedInputState.menu[KEY_MENU_LEFT] != 0;
			right = PulsedInputState.menu[KEY_MENU_RIGHT] != 0;
		}
		else if (uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::Console3DO)
		{
			left = CurrentInputState.menu[KEY_MENU_LEFT] != 0;
			right = CurrentInputState.menu[KEY_MENU_RIGHT] != 0;
		}

		curTrack = PlayingTrack();

		if (right)
		{
			SetSliderImage(SetAbsFrameIndex(ActivityFrame, 3));
			if (uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::PC || !uqm::UQMOptions::read().useSpeech)
			{
				FastForward_Page();
			}
			else if (uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::Console3DO)
			{
				FastForward_Smooth();
			}
			pTS->seeking = true;
		}
		else if (left || pTS->rewind)
		{
			pTS->rewind = false;
			SetSliderImage(SetAbsFrameIndex(ActivityFrame, 4));
			if (uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::PC || !uqm::UQMOptions::read().useSpeech)
			{
				FastReverse_Page();
			}
			else if (uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::Console3DO)
			{
				FastReverse_Smooth();
			}
			pTS->seeking = true;
		}
		else if (pTS->seeking)
		{
			// This is only done once the seeking is over (in the smooth
			// scroll case, once the user releases the seek button)
			pTS->seeking = false;
			SetSliderImage(SetAbsFrameIndex(ActivityFrame, 2));
		}
		else
		{
			// This used to have a buggy guard condition, which
			// would cause the animations to remain paused in a couple cases
			// after seeking back to the beginning.
			// Broken cases were: Syreen "several hours later" and Starbase
			// VUX Beast analysis by the scientist.

			// Kruzen - Now paused animations handled differently
			CheckSubtitles(curTrack && curTrack <= pTS->waitTrack);
		}
	}
	else
	{
		curTrack = PlayingTrack();

		if (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			FastForward_Page();
			FlushInput();
			next_page = true;
			return true;
		}
		else if (PulsedInputState.menu[KEY_MENU_SPECIAL])
		{
			JumpTrack();
			pTS->ended = true;
			return false;
		}
		else if (PauseSubtitles(next_page))
		{
			/* I would like to NOT count ellipses but whatever */
			uint32_t delay = RecalculateDelay(strlen(SubtitleText.pStr), false);
			uint8_t read_speed = speed_array[GLOBAL(glob_flags) & READ_SPEED_MASK];

			if (delay > 0 || read_speed == VERY_SLOW)
			{
				bool awake = false;
				bool block = (!wantTalkingAnim() || IsDarkMode
							  || !haveTalkingAnim());
				TimeCount TimeOut;

				PauseTrack();

				if (!block)
				{
					freezeTalkingAnim();
				}

				TimeOut = GetTimeCounter() + delay;

				while (!awake)
				{
					if (read_speed != VERY_SLOW
						&& GetTimeCounter() >= TimeOut)
					{
						awake = true;
					}

					UpdateCommGraphics();
					UpdateInputState();

					if (PulsedInputState.menu[KEY_MENU_CANCEL]
						|| PulsedInputState.menu[KEY_MENU_RIGHT]
						|| (GLOBAL(CurrentActivity) & CHECK_ABORT))
					{
						awake = true;
					}
					else if (PulsedInputState.menu[KEY_MENU_SPECIAL])
					{
						JumpTrack();
						pTS->ended = true;

						if (!block)
						{
							unFreezeTalkingAnim();
						}

						return false;
					}
				}

				if (!block)
				{
					unFreezeTalkingAnim();
				}

				ResumeTrack();
			}
		}

		next_page = false;

		CheckSubtitles(curTrack && curTrack <= pTS->waitTrack);
	}

	// XXX: When seeking, all animations (talking and ambient) stop
	// progressing. This is an original 3DO behavior, and I see no
	// reason why the animations cannot continue while seeking.
	UpdateAnimations(pTS->seeking);
	UpdateSpeechGraphics();
	FadePlayerUI();

	pTS->ended = !pTS->seeking && !curTrack;

	SleepThreadUntil(pTS->NextTime);
	// Need a high enough framerate for 3DO smooth seeking
	pTS->NextTime = GetTimeCounter() + GameTicksPerSecond / 60;

	return pTS->seeking || (curTrack && curTrack <= pTS->waitTrack);
}

static void
runCommAnimFrame(void)
{
	UpdateCommGraphics();
	SleepThread(COMM_ANIM_RATE);
}

static bool
TalkSegue(uint16_t wait_track)
{
	TALKING_STATE talkingState;

	// Transition animation to talking state, if necessary
	if (wantTalkingAnim() && haveTalkingAnim())
	{
		if (haveTransitionAnim())
		{
			setRunIntroAnim();
		}

		setRunTalkingAnim();

		// wait until the transition finishes
		while (runningIntroAnim())
		{
			runCommAnimFrame();
		}
	}

	memset(&talkingState, 0, sizeof talkingState);

	if (wait_track == 0)
	{ // Restarting with a rewind
		wait_track = WAIT_TRACK_ALL;
		talkingState.rewind = true;
	}
	else if (!PlayingTrack())
	{ // initial start of player
		PlayTrack();
	}

	// Run the talking controls
	SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_NONE);
	talkingState.InputFunc = DoTalkSegue;
	talkingState.waitTrack = wait_track;
	DoInput(&talkingState, false);

	if (talkingState.ended)
	{ // reached the end; set STOP icon
		ClearSubtitles();
		SetSliderImage(SetAbsFrameIndex(ActivityFrame, 8));
		cwLock = false; // Do not update comWindow
	}

	// transition back to silent, if necessary
	if (runningTalkingAnim())
	{
		setStopTalkingAnim();
	}

	if (!(GLOBAL(CurrentActivity) & CHECK_ABORT))
	{ // Wait until the animation task stops "talking"
		while (runningTalkingAnim())
		{
			runCommAnimFrame();
		}
	}

	return talkingState.ended;
}

static void
CommIntroTransition(void)
{
	if (curIntroMode == CIM_CROSSFADE_SCREEN)
	{
		ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);
		UnbatchGraphics();
	}
	else if (curIntroMode == CIM_CROSSFADE_SPACE)
	{
		GFXRECT r;
		r.corner.x = SIS_ORG_X;
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT;
		ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
		UnbatchGraphics();
	}
	else if (curIntroMode == CIM_CROSSFADE_WINDOW)
	{
		ScreenTransition(uqm::UQMOptions::read().scrTrans, &CommWndRect);
		UnbatchGraphics();
	}
	else if (curIntroMode == CIM_FADE_IN_SCREEN)
	{
		UnbatchGraphics();
		FadeScreen(FadeAllToColor, fadeTime);
	}
	else
	{ // Uknown transition
		// Have to unbatch anyway or no more graphics, ever
		UnbatchGraphics();
		assert(0 && "Unknown comm intro transition");
	}

	// Reset the mode for next time. Everything that needs a
	// different one will let us know.
	curIntroMode = CIM_DEFAULT;
}

void AlienTalkSegue(uint16_t wait_track)
{
	// this skips any talk segues that follow an aborted one
	if ((GLOBAL(CurrentActivity) & CHECK_ABORT) || TalkingFinished)
	{
		return;
	}

	if (!pCurInputState->Initialized)
	{
		MUSIC_REF AlienSong = CommData.AlienSong;

		InitSpeechGraphics();
		SetColorMap(GetColorMapAddress(CommData.AlienColorMap));
		SetContext(AnimContext);
		DrawAlienFrame(nullptr, 0, true);
		UpdateSpeechGraphics();
		CommIntroTransition();

		pCurInputState->Initialized = true;

		PlayMusicResume(AlienSong, BACKGROUND_VOL);

		InitCommAnimations();

		if (uqm::UQMOptions::read().scrTrans != uqm::EmulationMode::None && !uqm::UQMOptions::read().useSpeech)
		{ // short pause to compensate instant fading (conditions to be adjusted)
			// I think it is for optIPScaler
			TimeCount timeout = GetTimeCounter() + GameTicksPerSecond / 4;
			while (GetTimeCounter() < timeout)
			{
				UpdateDuty(false);
				UpdateAnimations(false);
			}
		}

		LastActivity &= ~CHECK_LOAD;
	}

	TalkingFinished = TalkSegue(wait_track);
	if (TalkingFinished && !VolasPackPresent)
	{
		FadeMusic(FOREGROUND_VOL, GameTicksPerSecond);
	}
}


typedef struct summary_state
{
	// standard state required by DoInput
	bool (*InputFunc)(struct summary_state* pSS);

	// extended state
	bool Initialized;
	bool PrintNext;
	SUBTITLE_REF NextSub;
	const char* LeftOver;

} SUMMARY_STATE;

static void
remove_char_from_string(char* str, const char c)
{ // MB: Hack for removing '$' characters from Orz dialogue when viewing
	// summary conversation - Used by DoConvSummary below
	char *pr = str, *pw = str;

	while (*pr)
	{
		*pw = *pr++;
		pw += (*pw != c);
	}
	*pw = '\0';
}

static bool
DoConvSummary(SUMMARY_STATE* pSS)
{
#define DELTA_Y_SUMMARY RES_SCALE(8)
#define MAX_SUMM_ROWS (SLIDER_Y / DELTA_Y_SUMMARY) - 1

	if (!pSS->Initialized)
	{
		pSS->PrintNext = true;
		pSS->NextSub = GetFirstTrackSubtitle();
		pSS->LeftOver = nullptr;
		pSS->InputFunc = DoConvSummary;
		pSS->Initialized = true;
		DoInput(pSS, false);
	}
	else if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false; // bail out
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT]
			 || PulsedInputState.menu[KEY_MENU_CANCEL]
			 || PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		if (pSS->NextSub)
		{ // we want the next page
			pSS->PrintNext = true;
		}
		else
		{ // no more, we are done
			return false;
		}
	}
	else if (pSS->PrintNext)
	{ // print the next page
		GFXRECT r;
		TEXT t;
		int row;

		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SLIDER_Y;

		SetContext(AnimContext);
		SetContextForeGroundColor(COMM_HISTORY_BACKGROUND_COLOR);
		DrawFilledRectangle(&r);

		SetContextForeGroundColor(COMM_HISTORY_TEXT_COLOR);

		r.extent.width -= RES_SCALE(2 + 2);
		t.baseline.x = RES_SCALE(2);
		t.align = ALIGN_LEFT;
		t.baseline.y = DELTA_Y_SUMMARY;

		SetContextFont(TinyFont);

		for (row = 0; row < MAX_SUMM_ROWS && pSS->NextSub;
			 ++row, pSS->NextSub = GetNextTrackSubtitle(pSS->NextSub))
		{
			const char* next = nullptr;

			if (pSS->LeftOver)
			{ // some text left from last subtitle
				t.pStr = pSS->LeftOver;
				pSS->LeftOver = nullptr;
			}
			else
			{
				t.pStr = GetTrackSubtitleText(pSS->NextSub);
				if (!t.pStr)
				{
					continue;
				}
			}

			t.CharCount = (uint16_t)~0;
			for (; row < MAX_SUMM_ROWS && !getLineWithinWidth(&t, &next, r.extent.width, (uint16_t)~0);
				 ++row)
			{
				if (CommData.AlienConv == ORZ_CONVERSATION)
				{ // MB: nasty hack: remove '$'s from conversation for
					// Orz
					char my_copy[128] {};

					uqm::strncpy_safe(my_copy, t.pStr);
					remove_char_from_string(my_copy, '$');
					t.pStr = my_copy;

					font_DrawText(&t);
				}
				else
				{
					font_DrawText(&t);
				}

				t.baseline.y += DELTA_Y_SUMMARY;
				t.pStr = next;
				t.CharCount = (uint16_t)~0;
			}

			if (row >= MAX_SUMM_ROWS)
			{ // no more space on screen, but some text left over
				// from the current subtitle
				pSS->LeftOver = next;
				break;
			}

			// this subtitle fit completely
			if (CommData.AlienConv == ORZ_CONVERSATION)
			{ // MB: nasty hack: remove '$'s from conversation for Orz
				char my_copy[128] {};

				uqm::strncpy_safe(my_copy, t.pStr);
				remove_char_from_string(my_copy, '$');
				t.pStr = my_copy;
				font_DrawText(&t);
			}
			else
			{
				font_DrawText(&t);
			}
			t.baseline.y += DELTA_Y_SUMMARY;
		}

		if (row >= MAX_SUMM_ROWS && (pSS->NextSub || pSS->LeftOver))
		{ // draw *MORE*
			TEXT mt;
			char buffer[128];

			mt.baseline.x = RES_SCALE(ORIG_SIS_SCREEN_WIDTH >> 1);
			mt.baseline.y = t.baseline.y;
			mt.align = ALIGN_CENTER;
			fmt::format_to_sz_n(buffer, "{}{}{}", // "MORE"
								STR_MIDDLE_DOT, GAME_STRING(FEEDBACK_STRING_BASE + 1),
								STR_MIDDLE_DOT);

			if (CommData.AlienConv == ORZ_CONVERSATION)
			{ // MB: nasty hack: remove '$'s from conversation for Orz
				remove_char_from_string(buffer, '$');
			}

			mt.pStr = buffer;
			SetContextForeGroundColor(COMM_MORE_TEXT_COLOR);
			font_DrawText(&mt);
		}


		pSS->PrintNext = false;
	}
	else
	{
		FlushInput();
	}
	UpdateSpeechGraphics();

	return true; // keep going
}

static void
ClearResponses(ENCOUNTER_STATE* pES)
{
	size_t responseI;

	for (responseI = 0; responseI < MAX_RESPONSES; responseI++)
	{
		RESPONSE_ENTRY* response = &pES->response_list[responseI];
		if (response->allocedResponse != nullptr)
		{
			HFree(response->allocedResponse);
			response->allocedResponse = nullptr;
		}
	}
}

// Called when the player presses the select button on a response.
static void
SelectResponse(ENCOUNTER_STATE* pES)
{
	TEXT* response_text =
		&pES->response_list[pES->cur_response].response_text;
	uqm::utf8StringCopy(pES->phrase_buf, sizeof pES->phrase_buf,
						response_text->pStr);

	if (!uqm::UQMOptions::read().useSpeech && uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::PC)
	{ // short pause after choosing response to mimic PC behaviour
		TimeCount timeout = GetTimeCounter() + GameTicksPerSecond / 2;
		RefreshResponsesSpecial(pES);
		while (GetTimeCounter() < timeout)
		{
			UpdateDuty(false);
			UpdateAnimations(false);
		}
	}
	else
	{
		FeedbackPlayerPhrase(pES->phrase_buf);
	}

	StopTrack();
	ClearSubtitles();
	SetSliderImage(SetAbsFrameIndex(ActivityFrame, 2));

	FlushCustomBaseLine();
	ReleaseTalkingAnim();

	FadeMusic(BACKGROUND_VOL, GameTicksPerSecond);

	TalkingFinished = false;
	pES->num_responses = 0;
	ClearResponses(pES);
	(*pES->response_list[pES->cur_response].response_func)(pES->response_list[pES->cur_response].response_ref);
}

// Called when the player presses the cancel button in comm.
static void
SelectConversationSummary(ENCOUNTER_STATE* pES)
{
	SUMMARY_STATE SummaryState;

	if (pES)
	{
		FeedbackPlayerPhrase(pES->phrase_buf);
	}

	SummaryState.Initialized = false;
	DoConvSummary(&SummaryState);

	if (pES)
	{
		RefreshResponses(pES);
	}
	clear_subtitles = true;
}

static void
SelectReplay(ENCOUNTER_STATE* pES)
{
	FadeMusic(BACKGROUND_VOL, GameTicksPerSecond);
	if (pES)
	{
		FeedbackPlayerPhrase(pES->phrase_buf);
	}

	TalkSegue(0);
}

static void
PlayerResponseInput(ENCOUNTER_STATE* pES)
{
	uint8_t response;

	if (pES->top_response == (uint8_t)~0)
	{
		pES->top_response = 0;
		RefreshResponses(pES);
	}

	if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		SelectResponse(pES);
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL] && uqm::lowByte(GLOBAL(CurrentActivity)) != WON_LAST_BATTLE && !IsDarkMode)
	{
		SelectConversationSummary(pES);
	}
	else
	{
		response = pES->cur_response;
		if (PulsedInputState.menu[KEY_MENU_LEFT] && (uqm::UQMOptions::read().useSpeech || uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::Console3DO))
		{
			SelectReplay(pES);

			if (!(GLOBAL(CurrentActivity) & CHECK_ABORT))
			{
				RefreshResponses(pES);
				FadeMusic(FOREGROUND_VOL, GameTicksPerSecond);
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_UP])
		{
			response = (uint8_t)((response + (uint8_t)(pES->num_responses - 1))
								 % pES->num_responses);
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			response = (uint8_t)((uint8_t)(response + 1) % pES->num_responses);
		}

		if (response != pES->cur_response)
		{
			int16_t y;

			BatchGraphics();
			add_text(-2,
					 &pES->response_list[pES->cur_response].response_text);

			pES->cur_response = response;

			y = add_text(-1,
						 &pES->response_list[pES->cur_response].response_text);
			if (response < pES->top_response)
			{
				pES->top_response = 0;
				RefreshResponses(pES);
			}
			else if (y > SIS_SCREEN_HEIGHT)
			{
				pES->top_response = response;
				RefreshResponses(pES);
			}
			else if (IS_HD)
			{
				RefreshResponses(pES);
			}

			UnbatchGraphics();
		}

		UpdateCommGraphics();
	}
}

// Derived from INPUT_STATE_DESC
typedef struct last_replay_state
{
	// Fields required by DoInput()
	bool (*InputFunc)(struct last_replay_state*);

	TimeCount NextTime; // framerate control
	TimeCount TimeOut;

} LAST_REPLAY_STATE;

static bool
DoLastReplay(LAST_REPLAY_STATE* pLRS)
{
	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false;
	}

	if (GetTimeCounter() > pLRS->TimeOut)
	{
		return false; // timed out and done
	}

	if (PulsedInputState.menu[KEY_MENU_CANCEL] && uqm::lowByte(GLOBAL(CurrentActivity)) != WON_LAST_BATTLE)
	{
		FadeMusic(BACKGROUND_VOL, GameTicksPerSecond);
		SelectConversationSummary(nullptr);
		pLRS->TimeOut = FadeMusic(0, GameTicksPerSecond * 2) + GameTicksPerSecond / 60;
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		SelectReplay(nullptr);
		pLRS->TimeOut = FadeMusic(0, GameTicksPerSecond * 2) + GameTicksPerSecond / 60;
	}

	while (GetTimeCounter() < pLRS->NextTime)
	{
		UpdateCommGraphics();
	}

	pLRS->NextTime = GetTimeCounter() + COMM_ANIM_RATE;

	return true;
}

static bool
DoCommunication(ENCOUNTER_STATE* pES)
{
	if (!IsDarkMode)
	{
		SetMenuSounds(MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
	}

	// First, finish playing all queued tracks if not done yet
	if (!TalkingFinished)
	{
		AlienTalkSegue(WAIT_TRACK_ALL);
		return true;
	}

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		UninitOscilloscope();
	}
	else if (pES->num_responses == 0)
	{
		// The player doesn't get a chance to say anything,
		// but can still review alien's last phrases.
		LAST_REPLAY_STATE replayState;

		memset(&replayState, 0, sizeof replayState);
		replayState.TimeOut = FadeMusic(0, GameTicksPerSecond * 3) + GameTicksPerSecond / 60;
		replayState.InputFunc = DoLastReplay;
		DoInput(&replayState, false);
	}
	else
	{
		PlayerResponseInput(pES);
		return true;
	}

	SetContext(SpaceContext);
	DestroyContext(AnimContext);
	AnimContext = nullptr;

	FlushColorXForms();
	ClearSubtitles();

	if (IsDarkMode)
	{
		SetCommDarkMode(false);
	}

	SetMusicPosition();
	StopMusic();
	StopSound();
	StopTrack();
	SleepThreadUntil(FadeMusic(NORMAL_VOLUME, 0) + GameTicksPerSecond / 60);

	UninitOscilloscope();

	return false;
}

void DoResponsePhrase(RESPONSE_REF R, RESPONSE_FUNC response_func,
					  char* ConstructStr)
{
	ENCOUNTER_STATE* pES = pCurInputState;
	RESPONSE_ENTRY* pEntry;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return;
	}

	if (pES->num_responses == 0)
	{
		pES->cur_response = 0;
		pES->top_response = (uint8_t)~0;
	}

	pEntry = &pES->response_list[pES->num_responses];
	pEntry->response_ref = R;
	pEntry->response_text.pStr = ConstructStr;
	if (pEntry->response_text.pStr)
	{
		pEntry->response_text.CharCount = (uint16_t)~0;
	}
	else
	{
		STRING locString;

		locString = SetAbsStringTableIndex(CommData.ConversationPhrases,
										   (uint16_t)(R - 1));
		pEntry->response_text.pStr =
			(char*)GetStringAddress(locString);

		if (luaUqm_comm_stringNeedsInterpolate(pEntry->response_text.pStr))
		{
			pEntry->allocedResponse = luaUqm_comm_stringInterpolate(
				pEntry->response_text.pStr);
			pEntry->response_text.pStr = pEntry->allocedResponse;
		}

		pEntry->response_text.CharCount = (uint16_t)~0;
	}
	pEntry->response_func = response_func;
	++pES->num_responses;
}

void SetUpCommData(void)
{ // Kruzen: Better loading alt resources
	// Loading alien frame
	if (altResFlags & USE_ALT_FRAME)
	{
		CommData.AlienFrame = CaptureDrawable(
			LoadGraphic(CommData.AltRes.AlienFrameRes));

		if (!CommData.AlienFrame) // Failed to load
		{
			CommData.AlienFrame = CaptureDrawable(
				LoadGraphic(CommData.AlienFrameRes));
			altResFlags &= ~USE_ALT_FRAME;
		}
	}
	else
	{
		CommData.AlienFrame = CaptureDrawable(
			LoadGraphic(CommData.AlienFrameRes));
	}

	// Loading alien font
	CommData.AlienFont = LoadFont(CommData.AlienFontRes);

	// Loading alien colormap
	if (altResFlags & USE_ALT_COLORMAP)
	{
		CommData.AlienColorMap = CaptureColorMap(
			LoadColorMap(CommData.AltRes.AlienColorMapRes));

		if (!CommData.AlienColorMap) // Failed to load
		{
			CommData.AlienColorMap = CaptureColorMap(
				LoadColorMap(CommData.AlienColorMapRes));
			altResFlags &= ~USE_ALT_COLORMAP;
		}
	}
	else
	{
		CommData.AlienColorMap = CaptureColorMap(
			LoadColorMap(CommData.AlienColorMapRes));
	}

	// Loading alien song
	if (altResFlags & USE_ALT_SONG)
	{
		CommData.AlienSong = LoadMusic(CommData.AltRes.AlienSongRes);

		if (!CommData.AlienSong) // Failed to load
		{
			CommData.AlienSong = LoadMusic(CommData.AlienSongRes);
			altResFlags &= ~USE_ALT_SONG;
		}
	}
	else
	{
		CommData.AlienSong = LoadMusic(CommData.AlienSongRes);
	}

	// Load alien convo lines
	CommData.ConversationPhrases = CaptureStringTable(
		LoadStringTable(CommData.ConversationPhrasesRes));
}

static void
HailAlien(void)
{
	ENCOUNTER_STATE ES;
	FONT PlayerFont, OldFont;
	Color TextBack;

	pCurInputState = &ES;
	memset(pCurInputState, 0, sizeof(*pCurInputState));

	TalkingFinished = false;

	ES.InputFunc = DoCommunication;

	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		PlayerFont = LoadFont(PLAYER_FONT);
	}
	else
	{
		PlayerFont = LoadFont(TINY_FONT_BOLD);
	}

	if (uqm::UQMOptions::read().orzCompFont)
	{
		ComputerFont = LoadFont(COMPUTER_FONT);

		if (ComputerFont == nullptr)
		{
			uqm::log::warn(
				"ComputerFont didn't load properly. "
				"Disabling Alternate Orz Font");
			uqm::UQMOptions::edit().orzCompFont = false;
		}
	}

	CommData.AlienFrame = CaptureDrawable(
		LoadGraphic(CommData.AlienFrameRes));
	CommData.AlienFont = LoadFont(CommData.AlienFontRes);
	CommData.AlienColorMap = CaptureColorMap(
		LoadColorMap(CommData.AlienColorMapRes));

	SetUpCommData();

	SubtitleText.baseline = CommData.AlienTextBaseline;
	SubtitleText.align = CommData.AlienTextAlign;


	// init subtitle cache context
	TextCacheContext = CreateContext("TextCacheContext");
	//TextCacheFrame = CaptureDrawable (
	//		CreateDrawable (WANT_PIXMAP, SIS_SCREEN_WIDTH,
	//		SIS_SCREEN_HEIGHT - SLIDER_Y - SLIDER_HEIGHT + 2, 1));
	// BW: previous lines were just a complex and wrong way of obtaining 107
	TextCacheFrame = CaptureDrawable(
		CreateDrawable(WANT_PIXMAP, SIS_SCREEN_WIDTH, SLIDER_Y, 1));
	SetContext(TextCacheContext);
	SetContextFGFrame(TextCacheFrame);
	TextBack = BUILD_COLOR(MAKE_RGB15(0x00, 0x00, 0x01), 0x00);
	// Color key for the background.
	SetContextBackGroundColor(TextBack);
	ClearDrawable();
	SetFrameTransparentColor(TextCacheFrame, TextBack);

	ES.phrase_buf[0] = '\0';

	SetContext(SpaceContext);
	OldFont = SetContextFont(PlayerFont);

	{
		GFXRECT r;

		AnimContext = CreateContext("AnimContext");
		SetContext(AnimContext);
		SetContextFGFrame(Screen);
		GetFrameRect(CommData.AlienFrame, &r);
		r.extent.width = SIS_SCREEN_WIDTH;
		CommWndRect.corner.x = SIS_ORG_X; // JMS_GFX: Added these lines because of the
		CommWndRect.corner.y = SIS_ORG_Y; // changed init of CommWndRect in the beginning of comm.c
		CommWndRect.extent = r.extent;

		SetTransitionSource(nullptr);
		BatchGraphics();
		if (uqm::lowByte(GLOBAL(CurrentActivity)) == WON_LAST_BATTLE)
		{
			// set the position of outtakes comm
			CommWndRect.corner.x = ((SCREEN_WIDTH - CommWndRect.extent.width) / 2);
			CommWndRect.corner.y = RES_SCALE(5);
			r.corner = CommWndRect.corner;
			SetContextClipRect(&r);
		}
		else
		{
			GFXPOINT Log = MAKE_POINT(LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x)),
									  LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y)));

			r.corner.x = SIS_ORG_X;
			r.corner.y = SIS_ORG_Y;
			SetContextClipRect(&r);
			CommWndRect.corner = r.corner;

			DrawSISFrame();
			// TODO: find a better way to do this, perhaps set the titles
			// forward from callers.
			if (GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) == (uint8_t)~0
				&& GET_GAME_STATE(STARBASE_AVAILABLE))
			{ // Talking to allied Starbase
				DrawSISMessage(GAME_STRING(STARBASE_STRING_BASE + 1));
				// "Starbase Commander"
				DrawSISTitle(GAME_STRING(STARBASE_STRING_BASE + 0));
				// "Starbase"
			}
			else
			{ // Default titles: star name + planet name
				DrawSISMessage(nullptr);
				// DrawSISTitle (GLOBAL_SIS (PlanetName));

				if (inHQSpace())
				{
					DrawHyperCoords(GLOBAL(ShipStamp.origin));
					if (GET_GAME_STATE(ARILOU_SPACE_SIDE) > 1
						&& GET_GAME_STATE(ARILOU_HOME_VISITS)
						&& (Log.x == ARILOU_HOME_X && Log.y == ARILOU_HOME_Y))
					{
						DrawSISMessage(GLOBAL_SIS(PlanetName));
					}
				}
				else if (GLOBAL(ip_planet) == 0)
				{
					DrawHyperCoords(CurStarDescPtr->star_pt);
				}
				else
				{
					DrawSISTitle(GLOBAL_SIS(PlanetName));
				}
			}
		}

		DrawSISComWindow();
	}


	LastActivity |= CHECK_LOAD; /* prevent spurious input */
	(*CommData.init_encounter_func)();
	DoInput(&ES, false);
	if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		(*CommData.post_encounter_func)();
	}
	(*CommData.uninit_encounter_func)();

	ClearResponses(&ES);
	SetContext(SpaceContext);
	SetContextFont(OldFont);

	DestroyStringTable(ReleaseStringTable(CommData.ConversationPhrases));
	DestroyMusic(CommData.AlienSong);
	DestroyColorMap(ReleaseColorMap(CommData.AlienColorMap));
	DestroyFont(CommData.AlienFont);
	DestroyDrawable(ReleaseDrawable(CommData.AlienFrame));

	DestroyContext(TextCacheContext);
	DestroyDrawable(ReleaseDrawable(TextCacheFrame));

	DestroyFont(PlayerFont);
	if (uqm::UQMOptions::read().orzCompFont)
	{
		DestroyFont(ComputerFont);
	}

	ReleaseTalkingAnim();
	DisengageFilters();
	altResFlags &= ~USE_ALT_ALL;

	// Some support code tests either of these to see if the
	// game is currently in comm or encounter
	CommData.ConversationPhrasesRes = 0;
	CommData.ConversationPhrases = 0;
	pCurInputState = 0;
}

void SetCommIntroMode(CommIntroMode newMode, TimeCount howLong)
{
	curIntroMode = newMode;
	fadeTime = howLong;
}

uint16_t
InitCommunication(CONVERSATION which_comm)
{
	uint16_t status;
	LOCDATA* LocDataPtr;

#ifdef DEBUG
	if (disableInteractivity)
	{
		return 0;
	}
#endif


	if (LastActivity & CHECK_LOAD)
	{
		LastActivity &= ~CHECK_LOAD;
		if (which_comm != COMMANDER_CONVERSATION)
		{
			if (uqm::lowByte(LastActivity) == 0)
			{
				DrawSISFrame();
			}
			else
			{
				ClearSISRect(DRAW_SIS_DISPLAY);
				RepairSISBorder();
			}
			DrawSISMessage(nullptr);
			if (inHQSpace())
			{
				DrawHyperCoords(GLOBAL(ShipStamp.origin));
			}
			else if (GLOBAL(ip_planet) == 0)
			{
				DrawHyperCoords(CurStarDescPtr->star_pt);
			}
			else
			{ // to fix moon suffix on load
				if (pSolarSysState && uqm::lowByte(GLOBAL(CurrentActivity)) != IN_LAST_BATTLE
					&& worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
				{
					if (!(GetNamedPlanetaryBody()) && isPC(uqm::UQMOptions::read().whichFonts)
						&& (pSolarSysState->pOrbitalDesc->data_index != HIERARCHY_STARBASE
							&& pSolarSysState->pOrbitalDesc->data_index != DESTROYED_STARBASE
							&& pSolarSysState->pOrbitalDesc->data_index != PRECURSOR_STARBASE))
					{
						fmt::format_to_sz_n((GLOBAL_SIS(PlanetName)) + strlen(GLOBAL_SIS(PlanetName)),
											4, "-%c%c", 'A' + moonIndex(pSolarSysState, pSolarSysState->pOrbitalDesc), '\0');
					}
				}

				DrawSISTitle(GLOBAL_SIS(PlanetName));
			}
		}
	}


	if (which_comm == URQUAN_DRONE_CONVERSATION)
	{
		status = URQUAN_DRONE_SHIP;
		which_comm = URQUAN_CONVERSATION;
	}
	else
	{
		if (which_comm == YEHAT_REBEL_CONVERSATION)
		{
			status = YEHAT_REBEL_SHIP;
			which_comm = YEHAT_CONVERSATION;
		}
		else
		{
			uint16_t commToShip[] = {
				RACE_SHIP_FOR_COMM};
			status = commToShip[which_comm];
			if (status >= YEHAT_REBEL_SHIP)
			{
				/* conversation exception, set to self */
				status = HUMAN_SHIP;
			}
		}
		StartSphereTracking((RACE_ID)status);

		if (which_comm == ORZ_CONVERSATION
			|| (which_comm == TALKING_PET_CONVERSATION
				&& (!GET_GAME_STATE(TALKING_PET_ON_SHIP)
					|| uqm::lowByte(GLOBAL(CurrentActivity)) == IN_LAST_BATTLE))
			|| (which_comm != CHMMR_CONVERSATION
				&& which_comm != SYREEN_CONVERSATION)) //&& CheckAlliance (status) == BAD_GUY))
		{
			BuildBattle(NPC_PLAYER_NUM);
		}
	}

	LocDataPtr = init_race(
		status != YEHAT_REBEL_SHIP ? which_comm :
									 YEHAT_REBEL_CONVERSATION);
	if (LocDataPtr)
	{ // We make a copy here
		CommData = *LocDataPtr;
	}

	if (GET_GAME_STATE(BATTLE_SEGUE) == 0)
	{
		// Not offered the chance to attack.
		status = HAIL;
	}
	else if ((status = InitEncounter()) == HAIL && LocDataPtr)
	{
		// The player chose to talk.
		SET_GAME_STATE(BATTLE_SEGUE, 0);
	}
	else
	{
		// The player chose to attack.
		status = ATTACK;
		SET_GAME_STATE(BATTLE_SEGUE, 1);
	}

	if (status == HAIL)
	{
		HailAlien();
	}
	else if (LocDataPtr)
	{ // only when comm initied successfully
		if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
		{
			(*CommData.post_encounter_func)(); // process states
		}

		(*CommData.uninit_encounter_func)(); // cleanup
	}

	status = 0;
	if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		// The Sa-Matra battle is skipped when Cyborg is enabled.
		// Most likely because the Cyborg is too dumb to know what
		// to do in this battle.
		if (uqm::lowByte(GLOBAL(CurrentActivity)) == IN_LAST_BATTLE
			&& (GLOBAL(glob_flags) & CYBORG_ENABLED))
		{
			ReinitQueue(&GLOBAL(npc_built_ship_q));
		}

		// Clear the location flags
		SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 0);
		status = (GET_GAME_STATE(BATTLE_SEGUE)
				  && GetHeadLink(&GLOBAL(npc_built_ship_q)));
		if (status)
		{
			// Start combat
			if (EXTENDED && GET_GAME_STATE(URQUAN_PROTECTING_SAMATRA))
			{ // reload GW to planet in penultimate battle
				free_gravity_well();
				load_gravity_well(GET_GAME_STATE(BATTLE_PLANET));
			}
			BuildBattle(RPG_PLAYER_NUM);
			EncounterBattle();
		}
		else
		{
			SET_GAME_STATE(BATTLE_SEGUE, 0);
		}
	}

	UninitEncounter();

	return (status);
}

void RaceCommunication(void)
{
	uint16_t i, status;
	HSHIPFRAG hStarShip;
	SHIP_FRAGMENT* FragPtr;
	HENCOUNTER hEncounter = 0;
	CONVERSATION RaceComm[] =
		{
			RACE_COMMUNICATION};

	if (uqm::lowByte(GLOBAL(CurrentActivity)) == IN_LAST_BATTLE)
	{
		/* Going into talking pet conversation */
		ReinitQueue(&GLOBAL(npc_built_ship_q));
		CloneShipFragment(SAMATRA_SHIP, &GLOBAL(npc_built_ship_q), 0);
		InitCommunication(TALKING_PET_CONVERSATION);
		if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
			&& GLOBAL_SIS(CrewEnlisted) != (uint16_t)~0)
		{
			GLOBAL(CurrentActivity) = WON_LAST_BATTLE;
		}
		return;
	}
	else if (NextActivity & CHECK_LOAD)
	{
		uint8_t ec;

		ec = GET_GAME_STATE(ESCAPE_COUNTER);

		if (GET_GAME_STATE(FOUND_PLUTO_SPATHI) == 1)
		{
			InitCommunication(SPATHI_CONVERSATION);
		}
		else if (GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) == 0)
		{
			InitCommunication(TALKING_PET_CONVERSATION);
		}
		else if (GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) & ((1 << 4) | (1 << 5)))
		{
			// Communicate with the Ilwrath using a Hyperwave Broadcaster.
			InitCommunication(ILWRATH_CONVERSATION);
		}
		else
		{
			InitCommunication(CHMMR_CONVERSATION);
		}
		if (GLOBAL_SIS(CrewEnlisted) != (uint16_t)~0)
		{
			NextActivity = GLOBAL(CurrentActivity) & ~START_ENCOUNTER;
			if (uqm::lowByte(NextActivity) == IN_INTERPLANETARY)
			{
				NextActivity |= START_INTERPLANETARY;
			}
			GLOBAL(CurrentActivity) |= CHECK_LOAD; /* fake a load game */
		}

		SET_GAME_STATE(ESCAPE_COUNTER, ec);
		return;
	}
	else if (inHQSpace())
	{
		ReinitQueue(&GLOBAL(npc_built_ship_q));
		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) >= 2)
		{
			InitCommunication(ARILOU_CONVERSATION);
			return;
		}
		else
		{
			/* Encounter with a black globe in HS, prepare enemy ship list */
			ENCOUNTER* EncounterPtr;

			// The encounter globe that the flagship collided with is moved
			// to the head of the queue in hyper.c:cleanup_hyperspace()
			hEncounter = GetHeadEncounter();
			LockEncounter(hEncounter, &EncounterPtr);

			for (i = 0; i < EncounterPtr->num_ships; ++i)
			{
				CloneShipFragment((RACE_ID)EncounterPtr->race_id,
								  &GLOBAL(npc_built_ship_q),
								  EncounterPtr->ShipList[i].crew_level);
			}

			// XXX: Bug: CurStarDescPtr was abused to point within
			//    an ENCOUNTER struct, which is immediately unlocked
			//CurStarDescPtr = (STAR_DESC*)&EncounterPtr->SD;
			UnlockEncounter(hEncounter);
		}
	}

	// First ship in the npc queue defines which alien race
	// the player will be talking to
	hStarShip = GetHeadLink(&GLOBAL(npc_built_ship_q));
	FragPtr = LockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
	i = FragPtr->race_id;
	UnlockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);

	status = InitCommunication(RaceComm[i]);

	if (GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
	{
		return;
	}

	if (i == CHMMR_SHIP)
	{
		ReinitQueue(&GLOBAL(npc_built_ship_q));
	}

	if (uqm::lowByte(GLOBAL(CurrentActivity)) == IN_INTERPLANETARY)
	{
		/* if used destruct code in interplanetary */
		if (i == SLYLANDRO_SHIP && status == 0)
		{
			ReinitQueue(&GLOBAL(npc_built_ship_q));
		}
	}
	else if (hEncounter)
	{
		/* Update HSpace encounter info, ships lefts, etc. */
		uint8_t i, NumShips;
		ENCOUNTER* EncounterPtr;

		LockEncounter(hEncounter, &EncounterPtr);

		NumShips = CountLinks(&GLOBAL(npc_built_ship_q));
		EncounterPtr->num_ships = NumShips;
		EncounterPtr->flags |= ENCOUNTER_REFORMING;
		if (status == 0)
		{
			EncounterPtr->flags |= ONE_SHOT_ENCOUNTER;
		}

		for (i = 0; i < NumShips; ++i)
		{
			HSHIPFRAG hStarShip;
			SHIP_FRAGMENT* FragPtr;
			BRIEF_SHIP_INFO* BSIPtr;

			hStarShip = GetStarShipFromIndex(&GLOBAL(npc_built_ship_q), i);
			FragPtr = LockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
			BSIPtr = &EncounterPtr->ShipList[i];
			BSIPtr->race_id = FragPtr->race_id;
			BSIPtr->crew_level = FragPtr->crew_level;
			BSIPtr->max_crew = FragPtr->max_crew;
			BSIPtr->max_energy = FragPtr->max_energy;
			UnlockShipFrag(&GLOBAL(npc_built_ship_q), hStarShip);
		}

		UnlockEncounter(hEncounter);
		ReinitQueue(&GLOBAL(npc_built_ship_q));
	}
}

static void
RedrawSubtitles(void)
{
	TEXT t;

	if (!uqm::UQMOptions::read().subtitles)
	{
		return;
	}

	if (SubtitleText.pStr)
	{
		t = SubtitleText;

		if (cur_node != nullptr)
		{ // If cur_node exist - use custom baseline
			t.baseline = cur_node->baseline;
			t.align = cur_node->align;
		}

		add_text(1, &t);
	}
}

static void
ClearSubtitles(void)
{
	clear_subtitles = true;
	last_subtitle = nullptr;
	SubtitleText.pStr = nullptr;
	SubtitleText.CharCount = 0;
}

static bool
PauseSubtitles(bool force)
{
	const char* pStr;
	static uint16_t num = 0;

	pStr = GetTrackSubtitle();

	if (GetSubtitleNumber(pStr) == 0 || SubtitleText.pStr == nullptr)
	{
		num = GetSubtitleNumber(pStr);
		return false;
	}

	if (num != GetSubtitleNumber(pStr) && !force)
	{
		num = GetSubtitleNumber(pStr);
		return true;
	}

	num = GetSubtitleNumber(pStr);
	return false;
}

static void
CheckSubtitles(bool really)
{
	const char* pStr;
	GFXPOINT baseline;
	TEXT_ALIGN align;
	uint16_t num;

	if (!really)
	{ // New check - blocks subtitle change on switching tracks so any code
		// that was waiting through AlienTalkSegue(x) would be completed first
		next_page = true;
		return;
	}

	pStr = GetTrackSubtitle();
	baseline = CommData.AlienTextBaseline;
	align = CommData.AlienTextAlign;

	if (pStr != SubtitleText.pStr || SubtitleText.baseline.x != baseline.x || SubtitleText.baseline.y != baseline.y || SubtitleText.align != align)
	{ // Subtitles changed
		num = GetSubtitleNumber(pStr);
		GetCustomBaseline(num);
		CheckTalkingAnim(num);
		clear_subtitles = true;
		// Baseline may be updated by the ZFP
		SubtitleText.baseline = baseline;
		SubtitleText.align = align;
		// Make a note in the logs if the update was multiframe
		if (SubtitleText.pStr == pStr)
		{
			uqm::log::warn(
				"Dialog text and location changed out of sync");
		}

		SubtitleText.pStr = pStr;
		// may have been cleared too
		if (pStr)
		{
			SubtitleText.CharCount = (uint16_t)~0;
		}
		else
		{
			SubtitleText.CharCount = 0;
		}
	}
}

void EnableTalkingAnim(bool enable)
{
	if (enable)
	{
		CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
	}
	else
	{
		CommData.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
	}
}

void SetCommDarkMode(bool state)
{ // Set dark mode (Black UI during Talana sex scene)
	IsDarkMode = state;
	oscillDisabled = state;
	sliderDisabled = state;
	InitUIRects(state);
	fadeIndex = state;
	if (!state)
	{
		ReleaseTalkingAnim();
	}
}

void RedrawSISComWindow(void)
{ // To call from outside
	DrawSISComWindow();
}

void SetCustomBaseLine(uint16_t sentence, GFXPOINT bl, TEXT_ALIGN align)
{ // Add custom baseline to the list
	CUSTOM_BASELINE *cur, *sPtr;

	cur = (CUSTOM_BASELINE*)HCalloc(sizeof(*cur));
	cur->index = sentence;
	cur->baseline.x = RES_SCALE(bl.x);
	cur->baseline.y = RES_SCALE(bl.y);
	cur->align = align;

	if (head_node == nullptr)
	{
		head_node = cur;
	}
	else
	{
		sPtr = head_node;
		while (sPtr->next != nullptr)
		{
			sPtr = sPtr->next;
		}
		sPtr->next = cur;
	}
}

void FlushCustomBaseLine(void)
{ // Free List, called at SelectResponce and Race uninit func (syreen.c for now)
	CUSTOM_BASELINE *cur, *next;

	if (head_node == nullptr)
	{
		return;
	}

	cur = head_node;
	while (cur != nullptr)
	{
		next = cur->next;
		HFree(cur);
		cur = next;
	}
	head_node = nullptr;
	cur_node = nullptr;
}