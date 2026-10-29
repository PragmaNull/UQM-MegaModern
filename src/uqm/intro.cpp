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
#include "options.h"
#include "settings.h"
#include "globdata.h"
#include "sis.h"
#include "setup.h"
#include "sounds.h"
#include "colors.h"
#include "fmv.h"
#include "resinst.h"
#include "nameref.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/sound/sound.h"
#include "libs/vidlib.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "libs/inplib.h"
#include "util.h"
#include "build.h"

#include <ctype.h>
#include <scn/scan.h>

static bool ShowSlidePresentation(STRING PresStr);

typedef struct
{
	/* standard state required by DoInput */
	bool (*InputFunc)(void* pInputState);

	/* Presentation state */
	TimeCount StartTime;
	TimeCount LastSyncTime;
	TimeCount TimeOut;
	int TimeOutOnSkip;
	STRING SlideShow;
#define MAX_FONTS 5
	FONT Fonts[MAX_FONTS];
	FRAME Frame;
	MUSIC_REF MusicRef;
	bool Batched;
	FRAME SisFrame;
	FRAME RotatedFrame;
	int LastDrawKind;
	int LastAngle;
	uint16_t OperIndex;
	Color TextFadeColor;
	Color TextColor;
	Color TextBackColor;
	int TextVPos;
	int TextEffect;
	GFXRECT clip_r;
	GFXRECT tfade_r;
#define MAX_TEXT_LINES 15
	TEXT TextLines[MAX_TEXT_LINES];
	uint16_t LinesCount;
	char Buffer[512];
	int MovieFrame;
	int MovieEndFrame;
	int InterframeDelay;

	// For DOS Spins
	GFXRECT StatBox;
	uint16_t NumSpinStat;
	GFXRECT GetRect;
	uint16_t CurrentFrameIndex;
	bool HaveFrame;
	bool Skip;

} PRESENTATION_INPUT_STATE;

typedef struct
{
	/* standard state required by DoInput */
	bool (*InputFunc)(void* pInputState);

	/* Spinanim state */
	STAMP anim;
	TimeCount last_time;
	int debounce;
} SPINANIM_INPUT_STATE;

typedef struct
{
	// standard state required by DoInput
	bool (*InputFunc)(void* pInputState);

	LEGACY_VIDEO_REF CurVideo;

} VIDEO_INPUT_STATE;

static bool DoPresentation(void* pIS);

static bool
ParseColorString(const uqstl::string_view src, Color* pColor)
{
	const auto clrResult {scn::scan<unsigned>(src, "{:x}")};
	if (!clrResult)
	{
		return false;
	}
	const unsigned clr = clrResult->value();

	*pColor = BUILD_COLOR_RGBA(
		(clr >> 16) & 0xff, (clr >> 8) & 0xff, clr & 0xff, 0xff);
	return true;
}

static bool
DoFadeScreen(PRESENTATION_INPUT_STATE* pPIS, const uqstl::string_view src, uint8_t FadeType)
{
	if (const auto result {scn::scan_value<int>(src)})
	{
		const int msecs = result->value();
		pPIS->TimeOut = FadeScreen((ScreenFadeType)FadeType, msecs * GameTicksPerSecond / 1000)
					  + GameTicksPerSecond / 10;
		pPIS->TimeOutOnSkip = false;
	}
	return true;
}

static void
DrawTextEffect(TEXT* pText, Color Fore, Color Back, int Effect)
{
	if (Effect == 'T')
	{
		font_DrawTracedText(pText, Fore, Back);
	}
	else
	{
		SetContextForeGroundColor(Fore);
		font_DrawText(pText);
	}
}

static uint16_t
ParseTextLines(TEXT* Lines, uint16_t MaxLines, char* Buffer)
{
	uint16_t i;
	const char* pEnd = Buffer + strlen(Buffer);

	for (i = 0; i < MaxLines && Buffer < pEnd; ++i, ++Lines)
	{
		char* pTerm = strchr(Buffer, '\n');
		if (!pTerm)
		{
			pTerm = Buffer + strlen(Buffer);
		}
		*pTerm = '\0'; /* terminate string */
		Lines->pStr = Buffer;
		Lines->CharCount = ~0;
		Buffer = pTerm + 1;
	}
	return i;
}

static void
Present_BatchGraphics(PRESENTATION_INPUT_STATE* pPIS)
{
	if (!pPIS->Batched)
	{
		pPIS->Batched = true;
		BatchGraphics();
	}
}

static void
Present_UnbatchGraphics(PRESENTATION_INPUT_STATE* pPIS, bool bYield)
{
	if (pPIS->Batched)
	{
		UnbatchGraphics();
		pPIS->Batched = false;
		if (bYield)
		{
			TaskSwitch();
		}
	}
}

static void
Present_GenerateSIS(PRESENTATION_INPUT_STATE* pPIS)
{
#define MODULE_YOFS_P -(RES_SCALE(79) + IF_HD(33))
#define DRIVE_TOP_Y_P (DRIVE_TOP_Y + MODULE_YOFS_P)
#define JET_TOP_Y_P (JET_TOP_Y + MODULE_YOFS_P)
#define MODULE_TOP_Y_P (MODULE_TOP_Y + MODULE_YOFS_P)
#define MODULE_TOP_X_P MODULE_TOP_X
	GFXCONTEXT OldContext;
	FRAME SisFrame;
	FRAME ModuleFrame;
	FRAME SkelFrame;
	STAMP s;
	GFXRECT r;
	HOT_SPOT hs;
	int slot;
	uint16_t piece;
	Color SisBack;

	OldContext = SetContext(OffScreenContext);

	SkelFrame = CaptureDrawable(LoadGraphic(SISSKEL_MASK_PMAP_ANIM));
	ModuleFrame = CaptureDrawable(LoadGraphic(SISMODS_MASK_PMAP_ANIM));

	GetFrameRect(SkelFrame, &r);
	SisFrame = CaptureDrawable(CreateDrawable(
		WANT_PIXMAP | WANT_ALPHA, r.extent.width, r.extent.height, 1));
	SetContextFGFrame(SisFrame);
	SetContextClipRect(nullptr);
	SisBack = BUILD_COLOR_RGBA(0, 0, 0, 0);
	SetContextBackGroundColor(SisBack);
	ClearDrawable();

	s.frame = SetAbsFrameIndex(SkelFrame, is3DO(uqm::UQMOptions::read().flagshipColor));
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp(&s);

	for (slot = 0; slot < NUM_DRIVE_SLOTS; ++slot)
	{
		piece = GLOBAL_SIS(DriveSlots[slot]);
		if (piece < EMPTY_SLOT)
		{
			s.origin.x = DRIVE_TOP_X;
			s.origin.y = DRIVE_TOP_Y_P;
			s.origin.x += slot * SHIP_PIECE_OFFSET;
			s.frame = SetAbsFrameIndex(ModuleFrame, piece);
			DrawStamp(&s);
		}
	}
	for (slot = 0; slot < NUM_JET_SLOTS; ++slot)
	{
		piece = GLOBAL_SIS(JetSlots[slot]);
		if (piece < EMPTY_SLOT)
		{
			s.origin.x = JET_TOP_X;
			s.origin.y = JET_TOP_Y_P;
			s.origin.x += slot * SHIP_PIECE_OFFSET;
			s.frame = SetAbsFrameIndex(ModuleFrame, piece);
			DrawStamp(&s);
		}
	}
	for (slot = 0; slot < NUM_MODULE_SLOTS; ++slot)
	{
		piece = GLOBAL_SIS(ModuleSlots[slot]);
		if (piece < EMPTY_SLOT)
		{
			s.origin.x = MODULE_TOP_X_P;
			s.origin.y = MODULE_TOP_Y_P;
			s.origin.x += slot * SHIP_PIECE_OFFSET;
			s.frame = SetAbsFrameIndex(ModuleFrame, piece);
			if (piece >= BOMB_MODULE_0)
			{ // Blue glow around the bomb - mimic PC-DOS pre-rendered cutscene
				Color oldColor = SetContextForeGroundColor(
					BUILD_COLOR_RGB(0x00, 0x00, 0x5F));

				s.origin.y += RES_SCALE(2);
				DrawFilledStamp(&s);

				s.origin.y -= RES_SCALE(4);
				DrawFilledStamp(&s);

				SetContextForeGroundColor(
					BUILD_COLOR_RGB(0x00, 0x07, 0xFF));

				s.origin.y += RES_SCALE(1);
				DrawFilledStamp(&s);

				s.origin.y += RES_SCALE(2);
				DrawFilledStamp(&s);

				s.origin.y -= RES_SCALE(1);
				SetContextForeGroundColor(oldColor);
			}
			DrawStamp(&s);
		}
	}

	DestroyDrawable(ReleaseDrawable(SkelFrame));
	DestroyDrawable(ReleaseDrawable(ModuleFrame));

	hs.x = r.extent.width / 2;
	hs.y = r.extent.height / 2;
	SetFrameHot(SisFrame, hs);

	SetContext(OldContext);
	FlushGraphics();

	pPIS->SisFrame = SisFrame;
}

static void
DoSpinText(const char* buf, int16_t x, int16_t y, FRAME repair, bool* skip)
{
	TEXT Text;

	Text.pStr = buf;
	Text.CharCount = (uint16_t)utf8StringCount(buf);
	Text.align = ALIGN_LEFT;
	Text.baseline.y = y;
	Text.baseline.x = x;

	font_DrawText_Fade(&Text, repair, skip);
}

static void
DoSpinLine(LINE* l, Color front, Color back, bool* skip)
{
	if (!*skip)
	{
		SetContextForeGroundColor(back);
		DrawLine(l, RES_SCALE(1));
		PlayMenuSound(MENU_SOUND_TEXT);
		SleepThread(GameTicksPerSecond / 16);
	}
	SetContextForeGroundColor(front);
	DrawLine(l, RES_SCALE(1));
}

static void
DoSpinStatBox(GFXRECT* r, Color front, Color back, bool* skip)
{
	if (!*skip)
	{
		DrawStarConBox(r, RES_SCALE(1), back, back, false, BLACK_COLOR, false, BLACK_COLOR);
		PlayMenuSound(MENU_SOUND_TEXT);
		SleepThread(GameTicksPerSecond / 16);
	}
	DrawStarConBox(r, RES_SCALE(1), front, front, false, BLACK_COLOR, false, BLACK_COLOR);
}

static void
DoSpinStat(char* buf, int16_t x, int16_t y, uint16_t filled, uint16_t empty, Color front, Color back,
		   bool* skip)
{
	TEXT Text;
	uint16_t i;
	GFXRECT sq;
	GFXPOINT c;
	GFXRECT chd;

	sq.corner.x = x + RES_SCALE(63);
	sq.corner.y = y - RES_SCALE(5);
	sq.extent.width = sq.extent.height = RES_SCALE(5);

	chd.extent.width = chd.extent.height = 4;

	Text.pStr = buf;
	Text.CharCount = (uint16_t)utf8StringCount(buf);
	Text.align = ALIGN_LEFT;
	Text.baseline.y = y;
	Text.baseline.x = x;

	if (!*skip)
	{
		SetContextForeGroundColor(back);
		font_DrawText(&Text);
		PlayMenuSound(MENU_SOUND_TEXT);
		SleepThread(GameTicksPerSecond / 16);
	}
	SetContextForeGroundColor(front);
	font_DrawText(&Text);

	for (i = 0; i < filled; i++)
	{
		if (!*skip)
		{
			SetContextForeGroundColor(back);
			DrawFilledRectangle(&sq);
			PlayMenuSound(MENU_SOUND_TEXT);
			SleepThread(GameTicksPerSecond / 16);
		}
		SetContextForeGroundColor(front);
		DrawFilledRectangle(&sq);
		sq.corner.x += RES_SCALE(6);

		UpdateInputState();
		if (CurrentInputState.menu[KEY_MENU_CANCEL] || (GLOBAL(CurrentActivity) & CHECK_ABORT))
		{
			*skip = true;
		}
	}
	for (i = 0; i < empty; i++)
	{
		c.x = sq.corner.x + RES_SCALE(2);
		c.y = sq.corner.y + RES_SCALE(2);
		if (!*skip)
		{
			SetContextForeGroundColor(back);
			DrawStarConBox(&sq, RES_SCALE(1), back, back, false, BLACK_COLOR, false, BLACK_COLOR);
			if (IS_HD)
			{
				chd.corner = c;
				DrawFilledRectangle(&chd);
			}
			else
			{
				DrawPoint(&c);
			}
			PlayMenuSound(MENU_SOUND_TEXT);
			SleepThread(GameTicksPerSecond / 16);
		}
		SetContextForeGroundColor(front);
		DrawStarConBox(&sq, RES_SCALE(1), front, front, false, BLACK_COLOR, false, BLACK_COLOR);
		if (IS_HD)
		{
			chd.corner = c;
			DrawFilledRectangle(&chd);
		}
		else
		{
			DrawPoint(&c);
		}
		sq.corner.x += RES_SCALE(6);

		UpdateInputState();
		if (CurrentInputState.menu[KEY_MENU_CANCEL] || (GLOBAL(CurrentActivity) & CHECK_ABORT))
		{
			*skip = true;
		}
	}
}

static void
Present_DrawMovieFrame(PRESENTATION_INPUT_STATE* pPIS)
{
	STAMP s;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex(pPIS->Frame, pPIS->MovieFrame);
	DrawStamp(&s);
}

static bool
ShowPresentationFile(const char* name)
{
	STRING pres = CaptureStringTable(LoadStringTableFile(uqm::contentDir(), name));
	bool result = ShowSlidePresentation(pres);
	DestroyStringTable(ReleaseStringTable(pres));
	return result;
}

typedef struct
{
	SPECIES_ID sID;	   // The ship SPECIES_ID, a second index
	char ditty[256];   // The ditty string (file name)
	char race[256];	   // The race's name string e.g. EARTHLING
	char ship[256];	   // The ship's name string e.g. CRUISER
	uint16_t spinline; // The line on which race/ship strings occur
	uint16_t width;	   // The SD pixel width of the race name
} SHIPMAP;

static const SHIPMAP ship_map[] = {
	{ARILOU_ID,		"arilou",	  "ARILOULALEELAY", " SKIFF",		  2,	 123},
	{CHMMR_ID,	   "chmmr",		"CHMMR",			 " AVATAR",		2,   50 },
	{EARTHLING_ID,  "earthling",	"EARTHLING",		 " CRUISER",		 130, 79 },
	{ORZ_ID,		 "orz",		"ORZ",			   " NEMESIS",	   2,	  25 },
	{PKUNK_ID,	   "pkunk",		"PKUNK",			 " FURY",		  2,	 43 },
	{SHOFIXTI_ID,	  "shofixti",	  "SHOFIXTI",		  " SCOUT",		4,   61 },
	{SPATHI_ID,		"spathi",	  "SPATHI",			" ELUDER",	   2,	  47 },
	{SUPOX_ID,	   "supox",		"SUPOX",			 " BLADE",		   2,	  43 },
	{THRADDASH_ID,  "thraddash",	"THRADDASH",		 " TORCH",		   2,	  87 },
	{UTWIG_ID,	   "utwig",		"UTWIG",			 " JUGGER",		2,   43 },
	{VUX_ID,		 "vux",		"VUX",			   " INTRUDER",		130, 28 },
	{YEHAT_ID,	   "yehat",		"YEHAT",			 " TERMINATOR",	130, 46 },
	{MELNORME_ID,	  "melnorme",	  "MELNORME",		  " TRADER",		 2,	77 },
	{DRUUGE_ID,		"druuge",	  "DRUUGE",			" MAULER",	   2,	  53 },
	{ILWRATH_ID,	 "ilwrath",	"ILWRATH",		   " AVENGER",	   2,	  62 },
	{MYCON_ID,	   "mycon",		"MYCON",			 " PODSHIP",		 127, 47 },
	{SLYLANDRO_ID,  "slylandro",	"SLYLANDRO",		 " PROBE",		   1,	  80 },
	{UMGAH_ID,	   "umgah",		"UMGAH",			 " DRONE",		   127, 50 },
	{UR_QUAN_ID,	 "urquan",	   "UR-QUAN",		  "\nDREADNAUGHT", 119, 66 },
	{ZOQFOTPIK_ID,  "zoqfotpik",	"ZOQ-FOT-PIK",	   " STINGER",	   130, 92 },
	{SYREEN_ID,		"syreen",	  "SYREEN",			" PENETRATOR",   2,	  53 },
	{KOHR_AH_ID,	 "kohrah",	   "KOHR-AH",		  "",			  119, 64 },
	{ANDROSYNTH_ID, "androsynth", "ANDROSYNTH",		" GUARDIAN",	 2,	94 },
	{CHENJESU_ID,	  "chenjesu",	  "CHENJESU",		  "    BROODHOME", 1,	  71 },
	{MMRNMHRM_ID,	  "mmrnmhrm",	  "MMRNMHRM",		  " TRANSFORMER",  128, 84 },
};

#define NUM_SHIPS (sizeof(ship_map) / sizeof(SHIPMAP))

static uint16_t shipID = NUM_SHIPS;
static uint16_t raceID = NUM_SHIPS;
static bool linespun = false;

static void
SeedDitty(uqstl::span<char> buf, uqstl::string_view str)
{
	//TODO Nullicious: Get rid of goto :(
	if (!uqm::UQMOptions::read().shipSeed)
	{
		goto SeedDittyPassThru;
	}

	for (shipID = 0; shipID < NUM_SHIPS; shipID++)
	{
		if (!strcasecmp(str.data(), ship_map[shipID].ditty))
		{
			break;
		}
	}
	if (shipID >= NUM_SHIPS)
	{
		goto SeedDittyPassThru;
	}

	for (raceID = 0; raceID < NUM_SHIPS; raceID++)
	{
		if (SeedShip(ship_map[raceID].sID, false) == ship_map[shipID].sID)
		{
			break;
		}
	}
	if (raceID >= NUM_SHIPS)
	{
		goto SeedDittyPassThru;
	}

	linespun = false;
	fmt::format_to_sz_n(buf, "ship.{}.ditty", ship_map[raceID].ditty);
	return;

SeedDittyPassThru:
	shipID = raceID = NUM_SHIPS;
	fmt::format_to_sz_n(buf, "ship.{}.ditty", str);
	return;
}

static void
SeedLineSpin(int* x1, int* y1, int* x2, int* y2)
{
	if (!uqm::UQMOptions::read().shipSeed || !x1 || !x2 || !y1 || !y2 || shipID == raceID || shipID >= NUM_SHIPS || raceID >= NUM_SHIPS)
	{
		goto SeedLineSpinPassThru;
	}

	if (ship_map[shipID].sID == KOHR_AH_ID && ship_map[raceID].sID != ARILOU_ID && ship_map[raceID].width > 80 && *y2 == 122)
	{
		*x2 -= ship_map[raceID].width - 80; // Shorten line for Spinning Blade
	}
	if (ship_map[shipID].sID == DRUUGE_ID && !IS_HD && ship_map[raceID].width > 80 && *y1 == 21)
	{ // Line for High-recoil Cannon in SD, needs to move down for wide ships
		*y1 += 10;
		*y2 += 10;
	}
SeedLineSpinPassThru:
	return;
}

static void
SeedTextSpin(char* buf, size_t size, uqgsl::czstring str, int* x, int* y)
{
	if (!uqm::UQMOptions::read().shipSeed || !x || !y || shipID == raceID || shipID >= NUM_SHIPS || raceID >= NUM_SHIPS)
	{
		goto SeedTextSpinPassThru;
	}

	if (ship_map[shipID].sID == SPATHI_ID && IS_HD && ship_map[raceID].sID != ARILOU_ID && ship_map[raceID].width > 80 && (*x == 25 || *x == 40))
	{
		*x -= 15; // Rear-firing Missile Launch Tube, move left for wide ships
	}
	if (ship_map[shipID].sID == DRUUGE_ID && !IS_HD && ship_map[raceID].width > 80 && *x == 108)
	{
		*y += 10; // This is High-recoil Cannon, move down for wide ships
	}
	if (ship_map[shipID].sID == SLYLANDRO_ID && *x == 18)
	{
		goto SeedTextSpinSkipLine; // This is 2418-B skip unless O.G.
	}
	if (ship_map[shipID].spinline != *y)
	{
		goto SeedTextSpinPassThru; // This is anything not on the spinline
	}
	if (ship_map[shipID].sID == MELNORME_ID && *x == 215)
	{
		goto SeedTextSpinPassThru; // This is Confusion Ray, on the spinline
	}
	if (ship_map[shipID].sID == UR_QUAN_ID && *x == 222)
	{
		goto SeedTextSpinPassThru; // This is Fusion Blast, on the spinline
	}
	if (linespun)
	{
		goto SeedTextSpinSkipLine; // Anything on the spinline we've done
	}

	linespun = true;
	utf8StringCopy(buf, size, ship_map[raceID].race);
	buf += strlen(ship_map[raceID].race) * sizeof(char);

	switch (ship_map[shipID].sID)
	{
		case CHENJESU_ID:
		case EARTHLING_ID:
		case KOHR_AH_ID:
		case MYCON_ID:
		case THRADDASH_ID: // Right hand huggers
			if ((ship_map[shipID].sID == EARTHLING_ID || (ship_map[shipID].sID == KOHR_AH_ID && IS_HD)) && ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof(char);
				*x += ship_map[shipID].width - 50;
				break;
			}
			if (ship_map[shipID].sID == CHENJESU_ID && IS_HD)
			{
				*x -= 10; // Better centering the gap over the broodhome
			}
			*x += ship_map[shipID].width - ship_map[raceID].width;
			break;
		case SHOFIXTI_ID: // Right hand hugger but squeeze it a little
			*x += (ship_map[shipID].width - ship_map[raceID].width) * 4 / 5;
			break;
		case VUX_ID: // Middle but squeeze the right a little
			if (ship_map[shipID].sID == VUX_ID && ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof(char);
				*x += (ship_map[shipID].width - 50) * 3 / 5;
				break;
			}
			*x += (ship_map[shipID].width - ship_map[raceID].width) * 3 / 5;
			break;
		case ANDROSYNTH_ID:
		case ARILOU_ID:
		case PKUNK_ID:
		case SPATHI_ID:
		case SYREEN_ID:
		case UTWIG_ID:
		case MMRNMHRM_ID: // Middle-ships
			if (ship_map[shipID].sID == SPATHI_ID && ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof(char);
				*x += (ship_map[shipID].width - 50) / 2;
				break;
			}
			if (ship_map[shipID].sID == SPATHI_ID)
			{
				*x += 2; // Better centering over (In Attack Position)
			}
			if (ship_map[shipID].sID == ARILOU_ID)
			{
				*x += 4; // Better centering over (Last Reported Position)
			}
			*x += (ship_map[shipID].width - ship_map[raceID].width) / 2;
			break;
		case CHMMR_ID:
		case ILWRATH_ID:
		case MELNORME_ID:
		case ORZ_ID:
		case SLYLANDRO_ID:
		case UMGAH_ID:
		case UR_QUAN_ID:
		case YEHAT_ID:
		case ZOQFOTPIK_ID: // Lefties need no adjustment
			if ((ship_map[shipID].sID == CHMMR_ID || ship_map[shipID].sID == ORZ_ID || ship_map[shipID].sID == ZOQFOTPIK_ID) && ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof(char);
				break;
			}
			if (ship_map[shipID].sID == UMGAH_ID && !IS_HD)
			{ // Right hand hugger but squeeze it a little
				*x += (ship_map[shipID].width - ship_map[raceID].width)
					* 4 / 5;
				break;
			}
			break;
		case DRUUGE_ID:
		case SUPOX_ID: // Lefties but line break when long
			if (ship_map[raceID].width > 80)
			{
				const char* pad = (IS_HD ? "\n   " : "\n       ");
				utf8StringCopy(buf, size, pad);
				buf += strlen(pad) * sizeof(char);
			}
			break;
		default:
			break;
	}

	utf8StringCopy(buf, size, ship_map[shipID].ship);
	return;

SeedTextSpinSkipLine:
	utf8StringCopy(buf, size, "");
	return;

SeedTextSpinPassThru:
	utf8StringCopy(buf, size, str);
	return;
}

uqstl::string_view scanUntilNewLine(uqstl::string_view src)
{
	// advance past whitespace.
	const auto start {src.find_first_not_of(" \t")};
	if (start == uqstl::string_view::npos)
	{
		return {};
	}
	const auto end {src.find('\n', start)};
	return src.substr(start, end);
}

static bool
DoPresentation(void* pIS)
{
	PRESENTATION_INPUT_STATE* pPIS = (PRESENTATION_INPUT_STATE*)pIS;

	if (PulsedInputState.menu[KEY_MENU_CANCEL]
		|| (GLOBAL(CurrentActivity) & CHECK_ABORT))
	{
		return false; /* abort requested - we are done */
	}

	if (pPIS->TimeOut)
	{
		TimeCount Delay = GameTicksPerSecond / 84;

		if (GetTimeCounter() >= pPIS->TimeOut)
		{
			if (pPIS->MovieFrame >= 0)
			{ /* Movie mode */
				Present_DrawMovieFrame(pPIS);
				++pPIS->MovieFrame;
				if (pPIS->MovieFrame > pPIS->MovieEndFrame)
				{
					pPIS->MovieFrame = -1; /* movie is done */
				}
				Delay = pPIS->InterframeDelay;
			}
			else
			{ /* time elapsed - continue normal ops */
				pPIS->TimeOut = 0;
				return true;
			}
		}

		if (pPIS->TimeOutOnSkip && (PulsedInputState.menu[KEY_MENU_SELECT] || PulsedInputState.menu[KEY_MENU_SPECIAL] || PulsedInputState.menu[KEY_MENU_RIGHT]))
		{ /* skip requested - continue normal ops */
			pPIS->TimeOut = 0;
			pPIS->MovieFrame = -1; /* abort any movie in progress */
			return true;
		}

		SleepThread(Delay);
		return true;
	}

	while (pPIS->OperIndex < GetStringTableCount(pPIS->SlideShow))
	{
		const char* pStr {GetStringAddress(pPIS->SlideShow)};
		pPIS->OperIndex++;
		pPIS->SlideShow = SetRelStringTableIndex(pPIS->SlideShow, 1);

		if (!pStr)
		{
			continue;
		}

		std::string_view strView {pStr};
		if (strView.empty())
		{
			continue;
		}
		const auto opcodeResult {scn::scan_value<std::string>(strView)};
		if (!opcodeResult)
		{
			continue;
		}
		const std::string& opcodeStr = opcodeResult->value();
		const auto opCodeHash {uqm::hashQuick64CaseInsensitive(opcodeStr)};

		strView = {opcodeResult->range().data(), opcodeResult->range().size()};

		// eat whitespace
		if (!strView.empty())
		{
			strView = strView.substr(strView.find_first_not_of(" \t"));
		}

		switch (opCodeHash)
		{
			case uqm::hashQuick64CaseInsensitive("DIMS"):
				{ /* set dimensions */
					if (const auto result {scn::scan<int, int>(strView, "{} {}")})
					{
						auto [w, h] = result->values();
						w <<= RESOLUTION_FACTOR;
						h <<= RESOLUTION_FACTOR;

						pPIS->clip_r.extent.width = w;
						pPIS->clip_r.extent.height = h;
						/* center on screen */
						pPIS->clip_r.corner.x = (SCREEN_WIDTH - w) / 2;
						pPIS->clip_r.corner.y = (SCREEN_HEIGHT - h) / 2;
						SetContextClipRect(&pPIS->clip_r);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("FONT"):
			case uqm::hashQuick64CaseInsensitive("FONT1X"):
			case uqm::hashQuick64CaseInsensitive("FONT4X"):
				{ /* set and/or load a font */
					if (opCodeHash == uqm::hashQuick64CaseInsensitive("FONT1X") && IS_HD)
					{
						break;
					}
					if (opCodeHash == uqm::hashQuick64CaseInsensitive("FONT4X") && !IS_HD)
					{
						break;
					}

					int index;
					FONT* pFont;

					assert(sizeof(pPIS->Buffer) >= 256);

					pPIS->Buffer[0] = '\0';
					const auto indexResult {scn::scan_value<int>(strView)};
					if (!indexResult || indexResult->value() < 0 || indexResult->value() >= MAX_FONTS)
					{
						uqm::log::warn("Bad FONT command '{}'", strView);
						continue;
					}
					index = indexResult->value();
					{
						const auto next {scanUntilNewLine({indexResult->range().data(), indexResult->range().size()})};
						uqm::strncpy_safe(pPIS->Buffer, next);
					}
					pFont = &pPIS->Fonts[index];

					if (pPIS->Buffer[0])
					{ /* asked to load a font */
						if (*pFont)
						{
							DestroyFont(*pFont);
						}
						*pFont = LoadFontFile(pPIS->Buffer);
					}

					SetContextFont(*pFont);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("ANI"):
			case uqm::hashQuick64CaseInsensitive("ANI1X"):
			case uqm::hashQuick64CaseInsensitive("ANI4X"):
				{ /* set ani */
					if (opCodeHash == uqm::hashQuick64CaseInsensitive("ANI1X") && IS_HD)
					{
						break;
					}
					if (opCodeHash == uqm::hashQuick64CaseInsensitive("ANI4X") && !IS_HD)
					{
						break;
					}

					uqm::strncpy_safe(pPIS->Buffer, strView);
					if (pPIS->Frame)
					{
						DestroyDrawable(ReleaseDrawable(pPIS->Frame));
					}
					pPIS->Frame = CaptureDrawable(LoadGraphicFile(pPIS->Buffer));
					break;
				}
			case uqm::hashQuick64CaseInsensitive("MUSIC"):
				{ /* set music */
					uqm::strncpy_safe(pPIS->Buffer, strView);
					if (pPIS->MusicRef)
					{
						StopMusic();
						DestroyMusic(pPIS->MusicRef);
					}
					pPIS->MusicRef = LoadMusicFile(pPIS->Buffer);
					PlayMusic(pPIS->MusicRef, false, 1);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("DITTY"):
				{ /* set ditty */
					if (uqm::UQMOptions::read().shipSeed)
					{
						SeedDitty(pPIS->Buffer, strView);
					}
					else
					{
						fmt::format_to_sz_n(pPIS->Buffer, "ship.{}.ditty", strView);
					}

					if (pPIS->MusicRef)
					{
						StopMusic();
						DestroyMusic(pPIS->MusicRef);
					}

					pPIS->MusicRef = LoadMusic(pPIS->Buffer);
					PlayMusic(pPIS->MusicRef, false, 1);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("WAIT"):
				{ /* wait */
					Present_UnbatchGraphics(pPIS, true);
					if (const auto result {scn::scan_value<int>(strView)})
					{
						const int msecs = result->value();
						pPIS->TimeOut = GetTimeCounter()
									  + msecs * GameTicksPerSecond / 1000;
						pPIS->TimeOutOnSkip = true;
						return true;
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("WAITDITTY"):
				{ /* wait for ditty to end */
					while (PlayingStream(MUSIC_SOURCE))
					{
						if (CurrentInputState.menu[KEY_MENU_CANCEL]
							|| (GLOBAL(CurrentActivity) & CHECK_ABORT))
						{
							StopMusic();
							pPIS->Skip = true;
							return true;
						}
						SleepThread(GameTicksPerSecond / 10);
						UpdateInputState();
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("SPINWAIT"):
				{ /* special wait during spin */
					TimeCount TimeOut;
					if (const auto result {scn::scan_value<int>(strView)}; result && !pPIS->Skip)
					{
						const int msecs = result->value();
						TimeOut = GetTimeCounter()
								+ msecs * GameTicksPerSecond / 1000;
						while (GetTimeCounter() < TimeOut)
						{
							if (CurrentInputState.menu[KEY_MENU_CANCEL]
								|| (GLOBAL(CurrentActivity) & CHECK_ABORT))
							{
								Present_BatchGraphics(pPIS);
								pPIS->Skip = true;
								return true;
							}
							SleepThread(GameTicksPerSecond / 84);
							UpdateInputState();
						}
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("SYNC"):
				{ /* absolute time-sync */
					Present_UnbatchGraphics(pPIS, true);
					if (const auto result {scn::scan_value<int>(strView)})
					{
						const int msecs = result->value();
						pPIS->LastSyncTime = pPIS->StartTime
										   + msecs * GameTicksPerSecond / 1000;
						pPIS->TimeOut = pPIS->LastSyncTime;
						pPIS->TimeOutOnSkip = false;
						return true;
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("RESYNC"):
				{ /* flush and update absolute sync point */
					pPIS->LastSyncTime = pPIS->StartTime = GetTimeCounter();
					break;
				}
			case uqm::hashQuick64CaseInsensitive("DSYNC"):
				{ /* delta time-sync; from the last absolute sync */
					Present_UnbatchGraphics(pPIS, true);
					if (const auto result {scn::scan_value<int>(strView)})
					{
						const int msecs = result->value();
						pPIS->TimeOut = pPIS->LastSyncTime
									  + msecs * GameTicksPerSecond / 1000;
						pPIS->TimeOutOnSkip = false;
						return true;
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("BGC"):
				{ /* text back color */
					Color temp;
					ParseColorString(strView, &temp);
					SetContextBackGroundColor(temp);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TC"):
				{ /* text fore color */
					ParseColorString(strView, &pPIS->TextColor);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TBC"):
				{ /* text back color */
					ParseColorString(strView, &pPIS->TextBackColor);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TFC"):
				{ /* text fade color */
					ParseColorString(strView, &pPIS->TextFadeColor);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TVA"):
				{ /* text vertical align */
					pPIS->TextVPos = toupper(strView.front());
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TE"):
				{ /* text effect */
					pPIS->TextEffect = toupper(strView.front());
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TEXT"):
				{ /* simple text draw */
					static_assert(sizeof(pPIS->Buffer) >= 256);

					if (const auto xyResult {scn::scan<int, int>(strView, "{} {}")})
					{
						const auto [x, y] = xyResult->values();

						const auto next {scanUntilNewLine({xyResult->range().data(), xyResult->range().size()})};
						uqm::strncpy_safe(pPIS->Buffer, next);

						TEXT t;

						t.align = ALIGN_CENTER;
						t.pStr = pPIS->Buffer;
						t.CharCount = (uint16_t)~0;
						t.baseline.x = RES_SCALE(x);
						t.baseline.y = RES_SCALE(y);
						DrawTextEffect(&t, pPIS->TextColor, pPIS->TextBackColor,
									   pPIS->TextEffect);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TEXTSPIN"):
				{ /* spin text draw */
					int x = 0, y = 0;

					assert(sizeof(pPIS->Buffer) >= 256);

					if (const auto xyResult {scn::scan<int, int>(strView, "{} {}")})
					{
						const auto [xi, yi] = xyResult->values();
						x = xi;
						y = yi;
						const char* textStart = xyResult->range().data();
						if (uqm::UQMOptions::read().shipSeed)
						{
							SeedTextSpin(pPIS->Buffer, sizeof(pPIS->Buffer),
										 textStart, &x, &y);
						}
						else
						{
							utf8StringCopy(pPIS->Buffer, sizeof(pPIS->Buffer),
										   textStart);
						}
						x <<= RESOLUTION_FACTOR;
						y <<= RESOLUTION_FACTOR;

						if (pPIS->HaveFrame
							&& (pPIS->GetRect.extent.width > 0
								&& pPIS->GetRect.extent.height > 0))
						{
							x += pPIS->GetRect.corner.x;
							y += pPIS->GetRect.corner.y;
						}

						SetContextForeGroundColor(pPIS->TextColor);
						SetContextBackGroundColor(pPIS->TextBackColor);
						if (uqm::UQMOptions::read().shipSeed && !strncmp(pPIS->Buffer, "SPATHI", 6))
						{ // Manually space the SPATHI text
							DoSpinText("SP", x,
									   y + RES_SCALE(7),
									   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
							DoSpinText("A", x + RES_SCALE(16),
									   y + RES_SCALE(7),
									   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
							DoSpinText("THI", x + RES_SCALE(25),
									   y + RES_SCALE(7),
									   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
							if (ship_map[shipID].sID == UR_QUAN_ID)
							{ // All for a ship that won't occur anyway
								x -= RES_SCALE(52);
								y += RES_SCALE(11);
							}
							DoSpinText(&(pPIS->Buffer[6]), x + RES_SCALE(52),
									   y + RES_SCALE(7),
									   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
						}
						else
						{
							DoSpinText(pPIS->Buffer, x, y + RES_SCALE(7),
									   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
						}

						if (pPIS->Skip)
						{
							Present_BatchGraphics(pPIS);
						}
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("SPINSTAT"):
				{ /* spin stat draw */
					int x, y, f = 0, e = 0;
					int16_t leading;

					assert(sizeof(pPIS->Buffer) >= 256);

					bool spinstatOk = false;
					if (const auto feResult {scn::scan<int, int>(strView, "{} {}")})
					{
						const auto [fi, ei] = feResult->values();
						f = fi;
						e = ei;

						const auto next {scanUntilNewLine({feResult->range().data(), feResult->range().size()})};
						uqm::strncpy_safe(pPIS->Buffer, next);
						spinstatOk = true;
					}
					if (spinstatOk)
					{
						GetContextFontLeading(&leading);

						pPIS->NumSpinStat++;

						x = pPIS->StatBox.corner.x + RES_SCALE(3);
						y = pPIS->StatBox.corner.y + RES_SCALE(1)
						  + (leading * pPIS->NumSpinStat);

						if (pPIS->NumSpinStat > 8)
						{
							uqm::log::warn("SPINSTAT: Number of SPINSTAT "
										   "entries exceeds max amount '{}'",
										   strView);
							return false;
						}

						if (f > 9 || (f + e) > 9)
						{
							char buf[sizeof(pPIS->Buffer) + 12];
							TEXT t;

							uqm::log::warn("SPINSTAT: Stats exceed max "
										   "values '{}'",
										   strView);
							fmt::format_to_sz_n(buf, "{} Exceed max !", pPIS->Buffer);

							t.align = ALIGN_LEFT;
							t.pStr = buf;
							t.CharCount = (uint16_t)~0;
							t.baseline = MAKE_POINT(x, y);
							DrawTextEffect(&t,
										   BUILD_COLOR_RGBA(0xFF, 0x55, 0x55, 0xFF),
										   pPIS->TextBackColor, pPIS->TextEffect);
						}
						else
						{
							DoSpinStat(pPIS->Buffer,
									   x, y, f, e,
									   pPIS->TextColor, pPIS->TextBackColor, &pPIS->Skip);

							if (pPIS->Skip)
							{
								Present_BatchGraphics(pPIS);
							}
						}
					}
					else
					{
						uqm::log::warn("Bad SPINSTAT command '{}'", strView);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TFI"):
				{ /* text fade-in */
					int16_t leading;
					uint16_t i;
					int16_t y;

					uqm::strncpy_safe(pPIS->Buffer, strView);
					pPIS->LinesCount = ParseTextLines(pPIS->TextLines,
													  MAX_TEXT_LINES, pPIS->Buffer);

					Present_UnbatchGraphics(pPIS, true);

					GetContextFontLeading(&leading);

					switch (pPIS->TextVPos)
					{
						case 'T': /* top */
							y = leading / pPIS->LinesCount + leading;
							break;
						case 'M': /* middle */
							y = (pPIS->clip_r.extent.height
								 - pPIS->LinesCount * leading)
							  / 2;
							break;
						default: /* bottom */
							y = pPIS->clip_r.extent.height - pPIS->LinesCount * leading;
					}
					pPIS->tfade_r = pPIS->clip_r;
					pPIS->tfade_r.corner.y = 0;
					pPIS->tfade_r.extent.height = SCREEN_HEIGHT;
					for (i = 0; i < pPIS->LinesCount; ++i, y += leading)
					{
						pPIS->TextLines[i].align = ALIGN_CENTER;
						pPIS->TextLines[i].baseline.x = SCREEN_WIDTH / 2;
						pPIS->TextLines[i].baseline.y = y;
					}

					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines + i, pPIS->TextFadeColor,
									   pPIS->TextFadeColor, pPIS->TextEffect);
					}

					/* do transition */
					SetTransitionSource(&pPIS->tfade_r);
					BatchGraphics();
					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines + i, pPIS->TextColor,
									   pPIS->TextBackColor, pPIS->TextEffect);
					}
					ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, &pPIS->tfade_r);
					UnbatchGraphics();
					break;
				}
			case uqm::hashQuick64CaseInsensitive("TFO"):
				{ /* text fade-out */
					uint16_t i;

					Present_UnbatchGraphics(pPIS, true);

					/* do transition */
					SetTransitionSource(&pPIS->tfade_r);
					BatchGraphics();
					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines + i, pPIS->TextFadeColor,
									   pPIS->TextFadeColor, pPIS->TextEffect);
					}
					ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, &pPIS->tfade_r);
					UnbatchGraphics();
					break;
				}
			case uqm::hashQuick64CaseInsensitive("SAVEBG"):
				{ /* save background */
					TFB_DrawScreen_Copy(&pPIS->clip_r,
										TFB_SCREEN_MAIN, TFB_SCREEN_EXTRA);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("RESTBG"):
				{ /* restore background */
					TFB_DrawScreen_Copy(&pPIS->clip_r,
										TFB_SCREEN_EXTRA, TFB_SCREEN_MAIN);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("DRAW"):
				{ /* draw a graphic */
#define PRES_DRAW_INDEX 0
#define PRES_DRAW_SIS 1
					int cargs = 0;
					int draw_what = PRES_DRAW_INDEX;
					int index = 0;
					int x = 0, y = 0;
					int scale = GSCALE_IDENTITY;
					int angle = 0;
					uqm::TFBScaleMode scale_mode {};
					STAMP s;

					if (const auto firstWordResult {scn::scan_value<std::string>(strView)})
					{
						uqstl::string firstWord = firstWordResult->value();
						for (char& c : firstWord)
						{
							c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
						}

						if (firstWord == "SIS")
						{
							draw_what = PRES_DRAW_SIS;
							scale_mode = uqm::TFBScaleMode::Nearest;

							const uqstl::string_view argPtr {
								firstWordResult->range().data(), firstWordResult->range().size()};
							if (const auto r4 {scn::scan<uqstl::string, int, int, int, int>(argPtr, "{} {} {} {} {}")})
							{
								const auto [_, xi, yi, si, ai] = r4->values();
								x = xi;
								y = yi;
								scale = si;
								angle = ai;
								cargs = 5;
							}
							else if (const auto r3 {scn::scan<uqstl::string, int, int, int>(argPtr, "{} {} {} {}")})
							{
								const auto [_, xi, yi, si] = r3->values();
								x = xi;
								y = yi;
								scale = si;
								cargs = 4;
							}
							else if (const auto r2 {scn::scan<uqstl::string, int, int>(argPtr, "{} {} {}")})
							{
								const auto [_, xi, yi] = r2->values();
								x = xi;
								y = yi;
								cargs = 3;
							}
							else if (const auto r1 {scn::scan<uqstl::string, int>(argPtr, "{} {}")})
							{
								x = uqstl::get<1>(r1->values());
								cargs = 2;
							}
						}
						else
						{
							draw_what = PRES_DRAW_INDEX;
							scale_mode = uqm::TFBScaleMode::Bilinear;
							if (const auto r5 {scn::scan<int, int, int, int, int>(strView, "{} {} {} {} {}")})
							{
								const auto [i, xi, yi, si, ai] = r5->values();
								index = i;
								x = xi;
								y = yi;
								scale = si;
								angle = ai;
								cargs = 5;
							}
							else if (const auto r4 {scn::scan<int, int, int, int>(strView, "{} {} {} {}")})
							{
								const auto [i, xi, yi, si] = r4->values();
								index = i;
								x = xi;
								y = yi;
								scale = si;
								cargs = 4;
							}
							else if (const auto r3 {scn::scan<int, int, int>(strView, "{} {} {}")})
							{
								const auto [i, xi, yi] = r3->values();
								index = i;
								x = xi;
								y = yi;
								cargs = 3;
							}
							else if (const auto r2 {scn::scan<int, int>(strView, "{} {}")})
							{
								const auto [i, xi] = r2->values();
								index = i;
								x = xi;
								cargs = 2;
							}
							else if (const auto r1 {scn::scan_value<int>(strView)})
							{
								index = r1->value();
								cargs = 1;
							}
						}
					}

					if (cargs < 1)
					{
						uqm::log::warn("Bad DRAW command '{}'", strView);
						pPIS->HaveFrame = false;
						break;
					}
					if (cargs < 5)
					{
						angle = 0;
					}
					if (cargs < 4)
					{
						scale = GSCALE_IDENTITY;
					}
					if (cargs < 3)
					{
						x = 0;
						y = 0;
					}

					x <<= RESOLUTION_FACTOR;
					y <<= RESOLUTION_FACTOR;

					s.frame = nullptr;
					if (draw_what == PRES_DRAW_INDEX)
					{ /* draw stamp by index */
						s.frame = SetAbsFrameIndex(pPIS->Frame, (uint16_t)index);
						pPIS->CurrentFrameIndex = (uint16_t)index;
						pPIS->HaveFrame = true;
					}
					else if (draw_what == PRES_DRAW_SIS)
					{ /* draw dynamic SIS image with player's modules */
						if (!pPIS->SisFrame)
						{
							Present_GenerateSIS(pPIS);
						}

						s.frame = SetAbsFrameIndex(pPIS->SisFrame, 0);
					}
					if (angle != 0)
					{
						if (angle != pPIS->LastAngle
							|| draw_what != pPIS->LastDrawKind)
						{
							DestroyDrawable(ReleaseDrawable(pPIS->RotatedFrame));
							pPIS->RotatedFrame = CaptureDrawable(
								RotateFrame(s.frame, -angle));
							pPIS->LastAngle = angle;
							pPIS->LastDrawKind = draw_what;
						}
						s.frame = pPIS->RotatedFrame;
					}
					s.origin.x = x;
					s.origin.y = y;
					const uqm::TFBScaleMode old_mode = SetGraphicScaleMode(scale_mode);
					const int old_scale = SetGraphicScale(scale);
					DrawStamp(&s);
					SetGraphicScale(old_scale);
					SetGraphicScaleMode(old_mode);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("BATCH"):
				{ /* batch graphics */
					Present_BatchGraphics(pPIS);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("UNBATCH"):
				{ /* unbatch graphics */
					Present_UnbatchGraphics(pPIS, false);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("FTC"):
				{ /* fade to color */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, strView, FadeAllToColor);
				}
			case uqm::hashQuick64CaseInsensitive("FTB"):
				{ /* fade to black */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, strView, FadeAllToBlack);
				}
			case uqm::hashQuick64CaseInsensitive("FTW"):
				{ /* fade to white */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, strView, FadeAllToWhite);
				}
			case uqm::hashQuick64CaseInsensitive("CLS"):
				{ /* clear screen */
					Present_UnbatchGraphics(pPIS, true);
					ClearScreen();
					break;
				}
			case uqm::hashQuick64CaseInsensitive("CALL"):
				{ /* call another script */
					Present_UnbatchGraphics(pPIS, true);
					uqm::strncpy_safe(pPIS->Buffer, strView);
					ShowPresentationFile(pPIS->Buffer);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("LINE"):
				{ /* draw simple line */
					if (const auto result {scn::scan<int, int, int, int>(strView, "{} {} {} {}")})
					{
						const auto [x1, y1, x2, y2] = result->values();
						LINE l;

						l.first.x = RES_SCALE(x1);
						l.first.y = RES_SCALE(y1);
						l.second.x = RES_SCALE(x2);
						l.second.y = RES_SCALE(y2);

						SetContextForeGroundColor(pPIS->TextColor);
						DrawLine(&l, 1);
					}
					else
					{
						uqm::log::warn("Bad LINE command '{}'", strView);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("LINESPIN"):
				{ /* draw line for spin */
					if (const auto result {scn::scan<int, int, int, int>(strView, "{} {} {} {}")})
					{
						auto [x1, y1, x2, y2] = result->values();
						if (uqm::UQMOptions::read().shipSeed)
						{
							SeedLineSpin(&x1, &y1, &x2, &y2);
						}
						LINE l;

						x1 <<= RESOLUTION_FACTOR;
						x2 <<= RESOLUTION_FACTOR;
						y1 <<= RESOLUTION_FACTOR;
						y2 <<= RESOLUTION_FACTOR;

						if (pPIS->HaveFrame
							&& (pPIS->GetRect.extent.width > 0
								&& pPIS->GetRect.extent.height > 0))
						{
							x1 += pPIS->GetRect.corner.x;
							x2 += pPIS->GetRect.corner.x;
							y1 += pPIS->GetRect.corner.y;
							y2 += pPIS->GetRect.corner.y;
						}

						l.first.x = x1;
						l.first.y = y1;
						l.second.x = x2;
						l.second.y = y2;

						DoSpinLine(&l, pPIS->TextColor, pPIS->TextBackColor, &pPIS->Skip);

						if (pPIS->Skip)
						{
							Present_BatchGraphics(pPIS);
						}
					}
					else
					{
						uqm::log::warn("Bad LINESPIN command '{}'", strView);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("GETRECT"):
				{ /* Get currently drawn FRAME rect */
					if (pPIS->HaveFrame)
					{
						GetFrameRect(SetAbsFrameIndex(
										 pPIS->Frame, pPIS->CurrentFrameIndex),
									 &pPIS->GetRect);
					}
					else
					{
						uqm::log::warn("Bad GETRECT command, can not use "
									   "GETRECT without drawing a frame first '{}'",
									   strView);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("STATBOX"):
				{ /* draw stat box for spin */
#define STATBOX_WIDTH RES_SCALE(122)
#define STATBOX_HEIGHT RES_SCALE(60)
					if (const auto result {scn::scan<int, int>(strView, "{} {}")})
					{
						auto [x, y] = result->values();
						pPIS->NumSpinStat = 0;

						x <<= RESOLUTION_FACTOR;
						y <<= RESOLUTION_FACTOR;

						if (pPIS->HaveFrame
							&& (pPIS->GetRect.extent.width > 0
								&& pPIS->GetRect.extent.height > 0))
						{
							x += pPIS->GetRect.corner.x;
							y += pPIS->GetRect.corner.y;
						}

						pPIS->StatBox.corner = MAKE_POINT(x, y);
						pPIS->StatBox.extent =
							MAKE_EXTENT(STATBOX_WIDTH, STATBOX_HEIGHT);

						DoSpinStatBox(&pPIS->StatBox, pPIS->TextColor,
									  pPIS->TextBackColor, &pPIS->Skip);

						if (pPIS->Skip)
						{
							Present_BatchGraphics(pPIS);
						}
					}
					else
					{
						uqm::log::warn("Bad STATBOX command '{}'", strView);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("MOVIE"):
				{ /* play movie */
					if (const auto result {scn::scan<int, int, int>(strView, "{} {} {}")})
					{
						const auto [fps, from, to] = result->values();
						if (fps > 0 && from >= 0 && to >= 0 && to >= from)
						{
							Present_UnbatchGraphics(pPIS, true);

							pPIS->MovieFrame = from;
							pPIS->MovieEndFrame = to;
							pPIS->InterframeDelay = GameTicksPerSecond / fps;

							pPIS->TimeOut = GetTimeCounter();
							pPIS->TimeOutOnSkip = true;
							return true;
						}
					}
					uqm::log::warn("Bad MOVIE command '{}'", strView);
					break;
				}
			case uqm::hashQuick64CaseInsensitive("ANIMATE"):
				{ /* basic frame animation */
					if (const auto result {scn::scan<int, int, int, int, int>(strView, "{} {} {} {} {}")})
					{
						const auto [first_frame, last_frame, num_loops, milliseconds, fps] = result->values();
						STAMP s;
						int loops = 0;
						uint16_t index = 0;
						TimeCount Now, timeout, NextTime;
						int animation_rate = GameTicksPerSecond / fps;

						s.origin.x = 0;
						s.origin.y = 0;

						timeout = GetTimeCounter() + milliseconds;
						NextTime = GetTimeCounter() + animation_rate;

						while (num_loops || milliseconds)
						{
							Now = GetTimeCounter();

							if (ActKeysPress())
							{
								break;
							}

							if (Now >= NextTime)
							{
								s.frame = SetAbsFrameIndex(pPIS->Frame, index);
								DrawStamp(&s);
								index++;

								if (index == last_frame)
								{
									loops++;
									index = first_frame;
								}

								if (num_loops > 0 && loops == num_loops)
								{
									break;
								}

								if (Now >= timeout)
								{
									break;
								}

								NextTime = Now + animation_rate;
							}
						}
						return true;
					}
					else
					{
						uqm::log::warn("Bad ANIMATION command '{}'", strView);
					}
					break;
				}
			case uqm::hashQuick64CaseInsensitive("NOOP"):
				{ /* no operation - must be a comment in script */
					break;
				}
		} // switch (opCodeHash)
	}
	/* we are all done */
	return false;
}

static bool
ShowSlidePresentation(STRING PresStr)
{
	GFXCONTEXT OldContext;
	FONT OldFont;
	GFXRECT OldRect;
	PRESENTATION_INPUT_STATE pis;
	int i;

	memset(&pis, 0, sizeof(pis));
	pis.SlideShow = PresStr;
	if (!pis.SlideShow)
	{
		return false;
	}
	pis.SlideShow = SetAbsStringTableIndex(pis.SlideShow, 0);
	pis.OperIndex = 0;

	OldContext = SetContext(ScreenContext);
	GetContextClipRect(&OldRect);
	OldFont = SetContextFont(nullptr);
	SetContextBackGroundColor(BLACK_COLOR);

	SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_NONE);
	pis.InputFunc = DoPresentation;
	pis.LastDrawKind = -1;
	pis.TextVPos = 'B';
	pis.MovieFrame = -1;
	pis.StartTime = GetTimeCounter();
	pis.LastSyncTime = pis.StartTime;
	DoInput(&pis, true);

	if (pis.MusicRef && PlayingStream(MUSIC_SOURCE))
	{
		SleepThreadUntil(FadeMusic(0, GameTicksPerSecond));
		StopMusic();
		FadeMusic(NORMAL_VOLUME, 0);
	}

	DestroyMusic(pis.MusicRef);
	DestroyDrawable(ReleaseDrawable(pis.RotatedFrame));
	DestroyDrawable(ReleaseDrawable(pis.Frame));
	for (i = 0; i < MAX_FONTS; ++i)
	{
		DestroyFont(pis.Fonts[i]);
	}

	SetContextFont(OldFont);
	SetContextClipRect(&OldRect);
	SetContext(OldContext);

	return true;
}

static bool
DoVideoInput(void* pIS)
{
	VIDEO_INPUT_STATE* pVIS = (VIDEO_INPUT_STATE*)pIS;

	if (!PlayingLegacyVideo(pVIS->CurVideo))
	{ // Video probably finished
		return false;
	}

	if (PulsedInputState.menu[KEY_MENU_SELECT]
		|| PulsedInputState.menu[KEY_MENU_CANCEL]
		|| PulsedInputState.menu[KEY_MENU_SPECIAL]
		|| (GLOBAL(CurrentActivity) & CHECK_ABORT))
	{ // abort movie
		return false;
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT]
			 || PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		int32_t newpos = VidGetPosition();
		if (PulsedInputState.menu[KEY_MENU_LEFT])
		{
			newpos -= 2000;
		}
		else if (PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			newpos += 1000;
		}
		if (newpos < 0)
		{
			newpos = 0;
		}

		VidSeek(newpos);
	}
	else
	{
		if (!VidProcessFrame())
		{
			return false;
		}

		SleepThread(GameTicksPerSecond / 40);
	}

	return true;
}

static void
FadeClearScreen(void)
{
	SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2));

	// clear the screen with black
	SetContext(ScreenContext);
	SetContextBackGroundColor(BLACK_COLOR);
	ClearDrawable();

	FadeScreen(FadeAllToColor, 0);
}

static bool
ShowLegacyVideo(LEGACY_VIDEO vid)
{
	VIDEO_INPUT_STATE vis;
	LEGACY_VIDEO_REF ref;

	FadeClearScreen();

	ref = PlayLegacyVideo(vid);
	if (!ref)
	{
		return false;
	}

	vis.InputFunc = DoVideoInput;
	vis.CurVideo = ref;
	SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_NONE);
	DoInput(&vis, true);

	StopLegacyVideo(ref);
	FadeClearScreen();

	return true;
}

bool ShowPresentation(RESOURCE res)
{
	const char* resType = res_GetResourceType(res);
	if (!resType)
	{
		return false;
	}
	if (!strcmp(resType, "STRTAB"))
	{
		STRING pres = CaptureStringTable(LoadStringTable(res));
		bool result = ShowSlidePresentation(pres);
		DestroyStringTable(ReleaseStringTable(pres));
		return result;
	}
	else if (!strcmp(resType, "3DOVID"))
	{
		LEGACY_VIDEO vid = LoadLegacyVideoInstance(res);
		bool result = ShowLegacyVideo(vid);
		DestroyLegacyVideo(vid);
		return result;
	}

	uqm::log::warn("Tried to present '{}', of non-presentable type '{}'", res, resType);
	return false;
}
