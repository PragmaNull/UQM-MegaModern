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

#include "slideshow.h"
#include "libs/file/textfile.h"

static bool ShowSlidePresentation(const uqstl::vector<uqm::SlideCommand>& commands);

struct PresentationInputState
{
	/* standard state required by DoInput */
	bool (*InputFunc)(void* pInputState);

	/* Presentation state */
	TimeCount StartTime;
	TimeCount LastSyncTime;
	TimeCount TimeOut;
	int TimeOutOnSkip;
	const uqstl::vector<uqm::SlideCommand>* Commands;
	FONT Fonts[uqm::c_slideshowMaxFonts];
	FRAME Frame;
	MUSIC_REF MusicRef;
	bool Batched;
	FRAME SisFrame;
	FRAME RotatedFrame;
	int LastDrawKind;
	int LastAngle;
	size_t OperIndex;
	Color TextFadeColor;
	Color TextColor;
	Color TextBackColor;
	uqm::SlideTextAlign TextVPos;
	bool TextTraced;
	GfxRect clip_r;
	GfxRect tfade_r;
	GfxText TextLines[uqm::c_slideshowMaxFadeInLines];
	uint16_t LinesCount;
	char Buffer[512];
	int MovieFrame;
	int MovieEndFrame;
	TimePeriod InterframeDelay;

	// For DOS Spins
	GfxRect StatBox;
	uint16_t NumSpinStat;
	GfxRect GetRect;
	uint16_t CurrentFrameIndex;
	bool HaveFrame;
	bool Skip;
};

struct SpinAnimInputState
{
	/* standard state required by DoInput */
	bool (*InputFunc)(void* pInputState);

	/* Spinanim state */
	GfxStamp anim;
	TimeCount last_time;
	int debounce;
};

struct VideoInputState
{
	// standard state required by DoInput
	bool (*InputFunc)(void* pInputState);

	LEGACY_VIDEO_REF CurVideo;
};

static bool DoPresentation(void* pIS);

static Color commandColor(const uqm::SlideCommand& cmd)
{
	return BuildColorRgba(cmd.color[0], cmd.color[1], cmd.color[2], 0xff);
}

static bool DoFadeScreen(PresentationInputState* pPIS, int msecs, uint8_t FadeType)
{
	pPIS->TimeOut = FadeScreen((ScreenFadeType)FadeType, msecs * c_gameSecond / 1000)
				  + c_gameSecond / 10;
	pPIS->TimeOutOnSkip = false;
	return true;
}

static void DrawTextEffect(GfxText* pText, Color Fore, Color Back, bool traced)
{
	if (traced)
	{
		font_DrawTracedText(pText, Fore, Back);
	}
	else
	{
		SetContextForeGroundColor(Fore);
		font_DrawText(pText);
	}
}

static void Present_BatchGraphics(PresentationInputState* pPIS)
{
	if (!pPIS->Batched)
	{
		pPIS->Batched = true;
		BatchGraphics();
	}
}

static void Present_UnbatchGraphics(PresentationInputState* pPIS, bool bYield)
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

static void Present_GenerateSIS(PresentationInputState* pPIS)
{
	const auto ModuleYofsP = [&] {
		return -(ResScale(79) + IfHd(33));
	};
	const auto DriveTopYP = [&] {
		return (DriveTopY() + ModuleYofsP());
	};
	const auto JetTopYP = [&] {
		return (JetTopY() + ModuleYofsP());
	};
	const auto ModuleTopYP = [&] {
		return (ModuleTopY() + ModuleYofsP());
	};
	const auto ModuleTopXP = [&] {
		return ModuleTopX();
	};
	FRAME SisFrame {};
	GfxStamp s;
	GfxRect r;
	HOT_SPOT hs;
	int slot {};
	uint16_t piece {};
	Color SisBack;

	GFXCONTEXT OldContext {SetContext(OffScreenContext)};

	FRAME SkelFrame {CaptureDrawable(LoadGraphic(SisskelMaskPmapAnimRes))};
	FRAME ModuleFrame {CaptureDrawable(LoadGraphic(SismodsMaskPmapAnimRes))};

	GetFrameRect(SkelFrame, &r);
	SisFrame = CaptureDrawable(CreateDrawable(
		WantPixmap | WantAlpha, r.extent.width, r.extent.height, 1));
	SetContextFGFrame(SisFrame);
	SetContextClipRect(nullptr);
	SisBack = BuildColorRgba(0, 0, 0, 0);
	SetContextBackGroundColor(SisBack);
	ClearDrawable();

	s.frame = SetAbsFrameIndex(SkelFrame, is3DO(uqm::UQMOptions::read().flagshipColor));
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp(&s);

	for (slot = 0; slot < NumDriveSlots; ++slot)
	{
		piece = uqm::gameState().sis().DriveSlots[slot];
		if (piece < EmptySlot)
		{
			s.origin.x = DriveTopX();
			s.origin.y = DriveTopYP();
			s.origin.x += slot * ShipPieceOffset();
			s.frame = SetAbsFrameIndex(ModuleFrame, piece);
			DrawStamp(&s);
		}
	}
	for (slot = 0; slot < NumJetSlots; ++slot)
	{
		piece = uqm::gameState().sis().JetSlots[slot];
		if (piece < EmptySlot)
		{
			s.origin.x = JetTopX();
			s.origin.y = JetTopYP();
			s.origin.x += slot * ShipPieceOffset();
			s.frame = SetAbsFrameIndex(ModuleFrame, piece);
			DrawStamp(&s);
		}
	}
	for (slot = 0; slot < NumModuleSlots; ++slot)
	{
		piece = uqm::gameState().sis().ModuleSlots[slot];
		if (piece < EmptySlot)
		{
			s.origin.x = ModuleTopXP();
			s.origin.y = ModuleTopYP();
			s.origin.x += slot * ShipPieceOffset();
			s.frame = SetAbsFrameIndex(ModuleFrame, piece);
			if (piece >= BOMB_MODULE_0)
			{ // Blue glow around the bomb - mimic PC-DOS pre-rendered cutscene
				const Color oldColor = SetContextForeGroundColor(
					BuildColorRgb(0x00, 0x00, 0x5F));

				s.origin.y += ResScale(2);
				DrawFilledStamp(&s);

				s.origin.y -= ResScale(4);
				DrawFilledStamp(&s);

				SetContextForeGroundColor(
					BuildColorRgb(0x00, 0x07, 0xFF));

				s.origin.y += ResScale(1);
				DrawFilledStamp(&s);

				s.origin.y += ResScale(2);
				DrawFilledStamp(&s);

				s.origin.y -= ResScale(1);
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

static void DoSpinText(const char* buf, int16_t x, int16_t y, FRAME repair, bool* skip)
{
	GfxText Text;

	Text.pStr = buf;
	Text.CharCount = (uint16_t)uqm::utf8StringCount(buf);
	Text.align = ALIGN_LEFT;
	Text.baseline.y = y;
	Text.baseline.x = x;

	font_DrawText_Fade(&Text, repair, skip);
}

static void DoSpinLine(GfxLine* l, Color front, Color back, bool* skip)
{
	if (!*skip)
	{
		SetContextForeGroundColor(back);
		DrawLine(l, ResScale(1));
		PlayMenuSound(MENU_SOUND_TEXT);
		SleepThread(c_gameSecond / 16);
	}
	SetContextForeGroundColor(front);
	DrawLine(l, ResScale(1));
}

static void DoSpinStatBox(GfxRect* r, Color front, Color back, bool* skip)
{
	if (!*skip)
	{
		DrawStarConBox(r, ResScale(1), back, back, false, BlackColor, false, BlackColor);
		PlayMenuSound(MENU_SOUND_TEXT);
		SleepThread(c_gameSecond / 16);
	}
	DrawStarConBox(r, ResScale(1), front, front, false, BlackColor, false, BlackColor);
}

static void DoSpinStat(char* buf, int16_t x, int16_t y, uint16_t filled, uint16_t empty, Color front, Color back,
					   bool* skip)
{
	GfxText Text;
	uint16_t i {};
	GfxRect sq;
	GfxPoint c;
	GfxRect chd;

	sq.corner.x = x + ResScale(63);
	sq.corner.y = y - ResScale(5);
	sq.extent.width = sq.extent.height = ResScale(5);

	chd.extent.width = chd.extent.height = 4;

	Text.pStr = buf;
	Text.CharCount = (uint16_t)uqm::utf8StringCount(buf);
	Text.align = ALIGN_LEFT;
	Text.baseline.y = y;
	Text.baseline.x = x;

	if (!*skip)
	{
		SetContextForeGroundColor(back);
		font_DrawText(&Text);
		PlayMenuSound(MENU_SOUND_TEXT);
		SleepThread(c_gameSecond / 16);
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
			SleepThread(c_gameSecond / 16);
		}
		SetContextForeGroundColor(front);
		DrawFilledRectangle(&sq);
		sq.corner.x += ResScale(6);

		UpdateInputState();
		if (CurrentInputState.menu[KEY_MENU_CANCEL] || (uqm::currentActivity() & CHECK_ABORT))
		{
			*skip = true;
		}
	}
	for (i = 0; i < empty; i++)
	{
		c.x = sq.corner.x + ResScale(2);
		c.y = sq.corner.y + ResScale(2);
		if (!*skip)
		{
			SetContextForeGroundColor(back);
			DrawStarConBox(&sq, ResScale(1), back, back, false, BlackColor, false, BlackColor);
			if (IsHd())
			{
				chd.corner = c;
				DrawFilledRectangle(&chd);
			}
			else
			{
				DrawPoint(&c);
			}
			PlayMenuSound(MENU_SOUND_TEXT);
			SleepThread(c_gameSecond / 16);
		}
		SetContextForeGroundColor(front);
		DrawStarConBox(&sq, ResScale(1), front, front, false, BlackColor, false, BlackColor);
		if (IsHd())
		{
			chd.corner = c;
			DrawFilledRectangle(&chd);
		}
		else
		{
			DrawPoint(&c);
		}
		sq.corner.x += ResScale(6);

		UpdateInputState();
		if (CurrentInputState.menu[KEY_MENU_CANCEL] || (uqm::currentActivity() & CHECK_ABORT))
		{
			*skip = true;
		}
	}
}

static void Present_DrawMovieFrame(PresentationInputState* pPIS)
{
	GfxStamp s;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex(pPIS->Frame, pPIS->MovieFrame);
	DrawStamp(&s);
}

static bool ShowPresentationFile(const char* name)
{
	const auto text {uqm::readTextFile(uqm::contentDir(), name)};
	const auto commands {text ? uqm::slideshowFromJson(*text) : uqstl::nullopt};
	if (!commands)
	{
		uqm::log::warn("Could not read the slideshow '{}'.", name);
		return false;
	}
	return ShowSlidePresentation(*commands);
}

struct IntroShip
{
	SPECIES_ID sID;	   // The ship SPECIES_ID, a second index
	char ditty[256];   // The ditty string (file name)
	char race[256];	   // The race's name string e.g. EARTHLING
	char ship[256];	   // The ship's name string e.g. CRUISER
	uint16_t spinline; // The line on which race/ship strings occur
	uint16_t width;	   // The SD pixel width of the race name
};

const static IntroShip ship_map[] = {
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

static constexpr uint16_t NumShips {static_cast<uint16_t>(uqstl::size(ship_map))};

static uint16_t shipID = NumShips;
static uint16_t raceID = NumShips;
static bool linespun = false;

static void SeedDitty(uqstl::span<char> buf, uqstl::string_view str)
{
	//TODO Nullicious: Get rid of goto :(
	if (!uqm::currentSeed().ships)
	{
		goto SeedDittyPassThru;
	}

	for (shipID = 0; shipID < NumShips; shipID++)
	{
		if (!strcasecmp(str.data(), ship_map[shipID].ditty))
		{
			break;
		}
	}
	if (shipID >= NumShips)
	{
		goto SeedDittyPassThru;
	}

	for (raceID = 0; raceID < NumShips; raceID++)
	{
		if (SeedShip(ship_map[raceID].sID, false) == ship_map[shipID].sID)
		{
			break;
		}
	}
	if (raceID >= NumShips)
	{
		goto SeedDittyPassThru;
	}

	linespun = false;
	fmt::format_to_sz_n(buf, "ship.{}.ditty", ship_map[raceID].ditty);
	return;

SeedDittyPassThru:
	shipID = raceID = NumShips;
	fmt::format_to_sz_n(buf, "ship.{}.ditty", str);
	return;
}

static void SeedLineSpin(int* x1, int* y1, int* x2, int* y2)
{
	if (!uqm::currentSeed().ships || !x1 || !x2 || !y1 || !y2 || shipID == raceID || shipID >= NumShips || raceID >= NumShips)
	{
		goto SeedLineSpinPassThru;
	}

	if (ship_map[shipID].sID == KOHR_AH_ID && ship_map[raceID].sID != ARILOU_ID && ship_map[raceID].width > 80 && *y2 == 122)
	{
		*x2 -= ship_map[raceID].width - 80; // Shorten line for Spinning Blade
	}
	if (ship_map[shipID].sID == DRUUGE_ID && !IsHd() && ship_map[raceID].width > 80 && *y1 == 21)
	{ // Line for High-recoil Cannon in SD, needs to move down for wide ships
		*y1 += 10;
		*y2 += 10;
	}
SeedLineSpinPassThru:
	return;
}

static void SeedTextSpin(char* buf, size_t size, uqgsl::czstring str, int* x, int* y)
{
	if (!uqm::currentSeed().ships || !x || !y || shipID == raceID || shipID >= NumShips || raceID >= NumShips)
	{
		goto SeedTextSpinPassThru;
	}

	if (ship_map[shipID].sID == SPATHI_ID && IsHd() && ship_map[raceID].sID != ARILOU_ID && ship_map[raceID].width > 80 && (*x == 25 || *x == 40))
	{
		*x -= 15; // Rear-firing Missile Launch Tube, move left for wide ships
	}
	if (ship_map[shipID].sID == DRUUGE_ID && !IsHd() && ship_map[raceID].width > 80 && *x == 108)
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
	uqm::utf8StringCopy(buf, size, ship_map[raceID].race);
	buf += strlen(ship_map[raceID].race) * sizeof(char);

	switch (ship_map[shipID].sID)
	{
		case CHENJESU_ID:
		case EARTHLING_ID:
		case KOHR_AH_ID:
		case MYCON_ID:
		case THRADDASH_ID: // Right hand huggers
			if ((ship_map[shipID].sID == EARTHLING_ID || (ship_map[shipID].sID == KOHR_AH_ID && IsHd())) && ship_map[raceID].sID == ARILOU_ID)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				buf -= 8 * sizeof(char);
				*x += ship_map[shipID].width - 50;
				break;
			}
			if (ship_map[shipID].sID == CHENJESU_ID && IsHd())
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
			if (ship_map[shipID].sID == UMGAH_ID && !IsHd())
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
				const char* pad = (IsHd() ? "\n   " : "\n       ");
				uqm::utf8StringCopy(buf, size, pad);
				buf += strlen(pad) * sizeof(char);
			}
			break;
		default:
			break;
	}

	uqm::utf8StringCopy(buf, size, ship_map[shipID].ship);
	return;

SeedTextSpinSkipLine:
	uqm::utf8StringCopy(buf, size, "");
	return;

SeedTextSpinPassThru:
	uqm::utf8StringCopy(buf, size, str);
	return;
}

static bool DoPresentation(void* pIS)
{
	PresentationInputState* pPIS = (PresentationInputState*)pIS;

	if (PulsedInputState.menu[KEY_MENU_CANCEL]
		|| (uqm::currentActivity() & CHECK_ABORT))
	{
		return false; /* abort requested - we are done */
	}

	if (pPIS->TimeOut != TimeCount {})
	{
		TimePeriod Delay {c_gameSecond / 84};

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
				pPIS->TimeOut = TimeCount {};
				return true;
			}
		}

		if (pPIS->TimeOutOnSkip && (PulsedInputState.menu[KEY_MENU_SELECT] || PulsedInputState.menu[KEY_MENU_SPECIAL] || PulsedInputState.menu[KEY_MENU_RIGHT]))
		{ /* skip requested - continue normal ops */
			pPIS->TimeOut = TimeCount {};
			pPIS->MovieFrame = -1; /* abort any movie in progress */
			return true;
		}

		SleepThread(Delay);
		return true;
	}

	while (pPIS->OperIndex < pPIS->Commands->size())
	{
		const uqm::SlideCommand& cmd {(*pPIS->Commands)[pPIS->OperIndex]};
		pPIS->OperIndex++;

		if ((cmd.resolution == uqm::SlideResolution::Sd && IsHd()) || (cmd.resolution == uqm::SlideResolution::Hd && !IsHd()))
		{
			continue;
		}

		switch (cmd.op)
		{
			case uqm::SlideOp::Dims:
				{ /* set dimensions */
					const int w {cmd.width << resolutionFactor};
					const int h {cmd.height << resolutionFactor};

					pPIS->clip_r.extent.width = w;
					pPIS->clip_r.extent.height = h;
					/* center on screen */
					pPIS->clip_r.corner.x = (CanvasWidth - w) / 2;
					pPIS->clip_r.corner.y = (CanvasHeight - h) / 2;
					SetContextClipRect(&pPIS->clip_r);
					break;
				}
			case uqm::SlideOp::Font:
				{ /* set and/or load a font */
					FONT* const pFont {&pPIS->Fonts[cmd.index]};

					if (!cmd.text.empty())
					{ /* asked to load a font */
						if (*pFont)
						{
							DestroyFont(*pFont);
						}
						*pFont = LoadFontFile(cmd.text.c_str());
					}

					SetContextFont(*pFont);
					break;
				}
			case uqm::SlideOp::Ani:
				{ /* set ani */
					if (pPIS->Frame)
					{
						DestroyDrawable(ReleaseDrawable(pPIS->Frame));
					}
					pPIS->Frame = CaptureDrawable(LoadGraphicFile(cmd.text.c_str()));
					break;
				}
			case uqm::SlideOp::Music:
				{ /* set music */
					if (pPIS->MusicRef)
					{
						StopMusic();
						DestroyMusic(pPIS->MusicRef);
					}
					pPIS->MusicRef = LoadMusicFile(cmd.text.c_str());
					PlayMusic(pPIS->MusicRef, false, 1);
					break;
				}
			case uqm::SlideOp::Ditty:
				{ /* set ditty */
					if (uqm::currentSeed().ships)
					{
						SeedDitty(pPIS->Buffer, cmd.text);
					}
					else
					{
						fmt::format_to_sz_n(pPIS->Buffer, "ship.{}.ditty", cmd.text);
					}

					if (pPIS->MusicRef)
					{
						StopMusic();
						DestroyMusic(pPIS->MusicRef);
					}

					pPIS->MusicRef = LoadMusicInstance(pPIS->Buffer);
					PlayMusic(pPIS->MusicRef, false, 1);
					break;
				}
			case uqm::SlideOp::Wait:
				{ /* wait */
					Present_UnbatchGraphics(pPIS, true);
					pPIS->TimeOut = GetTimeCounter() + cmd.ms * c_gameSecond / 1000;
					pPIS->TimeOutOnSkip = true;
					return true;
				}
			case uqm::SlideOp::WaitDitty:
				{ /* wait for ditty to end */
					while (PlayingStream(MusicSource))
					{
						if (CurrentInputState.menu[KEY_MENU_CANCEL]
							|| (uqm::currentActivity() & CHECK_ABORT))
						{
							StopMusic();
							pPIS->Skip = true;
							return true;
						}
						SleepThread(c_gameSecond / 10);
						UpdateInputState();
					}
					break;
				}
			case uqm::SlideOp::SpinWait:
				{ /* special wait during spin */
					if (!pPIS->Skip)
					{
						const TimeCount TimeOut {GetTimeCounter() + cmd.ms * c_gameSecond / 1000};
						while (GetTimeCounter() < TimeOut)
						{
							if (CurrentInputState.menu[KEY_MENU_CANCEL]
								|| (uqm::currentActivity() & CHECK_ABORT))
							{
								Present_BatchGraphics(pPIS);
								pPIS->Skip = true;
								return true;
							}
							SleepThread(c_gameSecond / 84);
							UpdateInputState();
						}
					}
					break;
				}
			case uqm::SlideOp::Sync:
				{ /* absolute time-sync */
					Present_UnbatchGraphics(pPIS, true);
					pPIS->LastSyncTime = pPIS->StartTime + cmd.ms * c_gameSecond / 1000;
					pPIS->TimeOut = pPIS->LastSyncTime;
					pPIS->TimeOutOnSkip = false;
					return true;
				}
			case uqm::SlideOp::Resync:
				{ /* flush and update absolute sync point */
					pPIS->LastSyncTime = pPIS->StartTime = GetTimeCounter();
					break;
				}
			case uqm::SlideOp::DeltaSync:
				{ /* delta time-sync; from the last absolute sync */
					Present_UnbatchGraphics(pPIS, true);
					pPIS->TimeOut = pPIS->LastSyncTime + cmd.ms * c_gameSecond / 1000;
					pPIS->TimeOutOnSkip = false;
					return true;
				}
			case uqm::SlideOp::BackgroundColor:
				{ /* text back color */
					SetContextBackGroundColor(commandColor(cmd));
					break;
				}
			case uqm::SlideOp::TextColor:
				{ /* text fore color */
					pPIS->TextColor = commandColor(cmd);
					break;
				}
			case uqm::SlideOp::TextBackColor:
				{ /* text back color */
					pPIS->TextBackColor = commandColor(cmd);
					break;
				}
			case uqm::SlideOp::TextFadeColor:
				{ /* text fade color */
					pPIS->TextFadeColor = commandColor(cmd);
					break;
				}
			case uqm::SlideOp::TextAlign:
				{ /* text vertical align */
					pPIS->TextVPos = cmd.align;
					break;
				}
			case uqm::SlideOp::TextEffect:
				{ /* text effect */
					pPIS->TextTraced = cmd.traced;
					break;
				}
			case uqm::SlideOp::Text:
				{ /* simple text draw */
					GfxText t;

					t.align = ALIGN_CENTER;
					t.pStr = cmd.text.c_str();
					t.CharCount = (uint16_t)~0;
					t.baseline.x = ResScale(cmd.x);
					t.baseline.y = ResScale(cmd.y);
					DrawTextEffect(&t, pPIS->TextColor, pPIS->TextBackColor,
								   pPIS->TextTraced);
					break;
				}
			case uqm::SlideOp::TextSpin:
				{ /* spin text draw */
					int x {cmd.x};
					int y {cmd.y};

					if (uqm::currentSeed().ships)
					{
						SeedTextSpin(pPIS->Buffer, sizeof(pPIS->Buffer),
									 cmd.text.c_str(), &x, &y);
					}
					else
					{
						uqm::utf8StringCopy(pPIS->Buffer, sizeof(pPIS->Buffer),
											cmd.text.c_str());
					}
					x <<= resolutionFactor;
					y <<= resolutionFactor;

					if (pPIS->HaveFrame
						&& (pPIS->GetRect.extent.width > 0
							&& pPIS->GetRect.extent.height > 0))
					{
						x += pPIS->GetRect.corner.x;
						y += pPIS->GetRect.corner.y;
					}

					SetContextForeGroundColor(pPIS->TextColor);
					SetContextBackGroundColor(pPIS->TextBackColor);
					if (uqm::currentSeed().ships && !strncmp(pPIS->Buffer, "SPATHI", 6))
					{ // Manually space the SPATHI text
						DoSpinText("SP", x,
								   y + ResScale(7),
								   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
						DoSpinText("A", x + ResScale(16),
								   y + ResScale(7),
								   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
						DoSpinText("THI", x + ResScale(25),
								   y + ResScale(7),
								   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
						if (ship_map[shipID].sID == UR_QUAN_ID)
						{ // All for a ship that won't occur anyway
							x -= ResScale(52);
							y += ResScale(11);
						}
						DoSpinText(&(pPIS->Buffer[6]), x + ResScale(52),
								   y + ResScale(7),
								   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
					}
					else
					{
						DoSpinText(pPIS->Buffer, x, y + ResScale(7),
								   SetAbsFrameIndex(pPIS->Frame, 0), &pPIS->Skip);
					}

					if (pPIS->Skip)
					{
						Present_BatchGraphics(pPIS);
					}
					break;
				}
			case uqm::SlideOp::SpinStat:
				{ /* spin stat draw */
					int16_t leading {};

					uqm::strncpy_safe(pPIS->Buffer, cmd.text);
					GetContextFontLeading(&leading);

					pPIS->NumSpinStat++;

					const int x {pPIS->StatBox.corner.x + ResScale(3)};
					const int y {pPIS->StatBox.corner.y + ResScale(1) + (leading * pPIS->NumSpinStat)};

					if (pPIS->NumSpinStat > 8)
					{
						uqm::log::warn("SPINSTAT: Number of SPINSTAT "
									   "entries exceeds max amount '{}'",
									   cmd.text);
						return false;
					}

					if (cmd.filled > 9 || (cmd.filled + cmd.empty) > 9)
					{
						char buf[sizeof(pPIS->Buffer) + 12];
						GfxText t;

						uqm::log::warn("SPINSTAT: Stats exceed max "
									   "values '{}'",
									   cmd.text);
						fmt::format_to_sz_n(buf, "{} Exceed max !", pPIS->Buffer);

						t.align = ALIGN_LEFT;
						t.pStr = buf;
						t.CharCount = (uint16_t)~0;
						t.baseline = MAKE_POINT(x, y);
						DrawTextEffect(&t,
									   BuildColorRgba(0xFF, 0x55, 0x55, 0xFF),
									   pPIS->TextBackColor, pPIS->TextTraced);
					}
					else
					{
						DoSpinStat(pPIS->Buffer,
								   x, y, cmd.filled, cmd.empty,
								   pPIS->TextColor, pPIS->TextBackColor, &pPIS->Skip);

						if (pPIS->Skip)
						{
							Present_BatchGraphics(pPIS);
						}
					}
					break;
				}
			case uqm::SlideOp::TextFadeIn:
				{ /* text fade-in */
					int16_t leading {};
					uint16_t i {};
					int16_t y {};

					pPIS->LinesCount = static_cast<uint16_t>(cmd.lines.size());
					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						pPIS->TextLines[i].pStr = cmd.lines[i].c_str();
						pPIS->TextLines[i].CharCount = (uint16_t)~0;
					}

					Present_UnbatchGraphics(pPIS, true);

					GetContextFontLeading(&leading);

					switch (pPIS->TextVPos)
					{
						case uqm::SlideTextAlign::Top:
							y = leading / pPIS->LinesCount + leading;
							break;
						case uqm::SlideTextAlign::Middle:
							y = (pPIS->clip_r.extent.height
								 - pPIS->LinesCount * leading)
							  / 2;
							break;
						default:
							y = pPIS->clip_r.extent.height - pPIS->LinesCount * leading;
					}
					pPIS->tfade_r = pPIS->clip_r;
					pPIS->tfade_r.corner.y = 0;
					pPIS->tfade_r.extent.height = CanvasHeight;
					for (i = 0; i < pPIS->LinesCount; ++i, y += leading)
					{
						pPIS->TextLines[i].align = ALIGN_CENTER;
						pPIS->TextLines[i].baseline.x = CanvasWidth / 2;
						pPIS->TextLines[i].baseline.y = y;
					}

					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines + i, pPIS->TextFadeColor,
									   pPIS->TextFadeColor, pPIS->TextTraced);
					}

					/* do transition */
					SetTransitionSource(&pPIS->tfade_r);
					BatchGraphics();
					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines + i, pPIS->TextColor,
									   pPIS->TextBackColor, pPIS->TextTraced);
					}
					ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, &pPIS->tfade_r);
					UnbatchGraphics();
					break;
				}
			case uqm::SlideOp::TextFadeOut:
				{ /* text fade-out */
					uint16_t i {};

					Present_UnbatchGraphics(pPIS, true);

					/* do transition */
					SetTransitionSource(&pPIS->tfade_r);
					BatchGraphics();
					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines + i, pPIS->TextFadeColor,
									   pPIS->TextFadeColor, pPIS->TextTraced);
					}
					ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, &pPIS->tfade_r);
					UnbatchGraphics();
					break;
				}
			case uqm::SlideOp::SaveBackground:
				{ /* save background */
					TFB_DrawScreen_Copy(&pPIS->clip_r,
										TFB_SCREEN_MAIN, TFB_SCREEN_EXTRA);
					break;
				}
			case uqm::SlideOp::RestoreBackground:
				{ /* restore background */
					TFB_DrawScreen_Copy(&pPIS->clip_r,
										TFB_SCREEN_EXTRA, TFB_SCREEN_MAIN);
					break;
				}
			case uqm::SlideOp::Draw:
			case uqm::SlideOp::DrawSis:
				{ /* draw a graphic */
					const bool drawSis {cmd.op == uqm::SlideOp::DrawSis};
					const int angle {cmd.angle};
					GfxStamp s;

					if (!drawSis)
					{ /* draw stamp by index */
						s.frame = SetAbsFrameIndex(pPIS->Frame, static_cast<uint16_t>(cmd.index));
						pPIS->CurrentFrameIndex = static_cast<uint16_t>(cmd.index);
						pPIS->HaveFrame = true;
					}
					else
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
							|| static_cast<int>(drawSis) != pPIS->LastDrawKind)
						{
							DestroyDrawable(ReleaseDrawable(pPIS->RotatedFrame));
							pPIS->RotatedFrame = CaptureDrawable(
								RotateFrame(s.frame, -angle));
							pPIS->LastAngle = angle;
							pPIS->LastDrawKind = static_cast<int>(drawSis);
						}
						s.frame = pPIS->RotatedFrame;
					}
					s.origin.x = cmd.x << resolutionFactor;
					s.origin.y = cmd.y << resolutionFactor;
					const uqm::TFBScaleMode old_mode {
						SetGraphicScaleMode(drawSis ? uqm::TFBScaleMode::Nearest : uqm::TFBScaleMode::Bilinear)};
					const int old_scale {SetGraphicScale(cmd.scale.value_or(GscaleIdentity))};
					DrawStamp(&s);
					SetGraphicScale(old_scale);
					SetGraphicScaleMode(old_mode);
					break;
				}
			case uqm::SlideOp::Batch:
				{ /* batch graphics */
					Present_BatchGraphics(pPIS);
					break;
				}
			case uqm::SlideOp::Unbatch:
				{ /* unbatch graphics */
					Present_UnbatchGraphics(pPIS, false);
					break;
				}
			case uqm::SlideOp::FadeToColor:
				{ /* fade to color */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, cmd.ms, FadeAllToColor);
				}
			case uqm::SlideOp::FadeToBlack:
				{ /* fade to black */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, cmd.ms, FadeAllToBlack);
				}
			case uqm::SlideOp::FadeToWhite:
				{ /* fade to white */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, cmd.ms, FadeAllToWhite);
				}
			case uqm::SlideOp::ClearScreen:
				{ /* clear screen */
					Present_UnbatchGraphics(pPIS, true);
					ClearScreen();
					break;
				}
			case uqm::SlideOp::Call:
				{ /* call another script */
					Present_UnbatchGraphics(pPIS, true);
					ShowPresentationFile(cmd.text.c_str());
					break;
				}
			case uqm::SlideOp::Line:
				{ /* draw simple line */
					GfxLine l;

					l.first.x = ResScale(cmd.x);
					l.first.y = ResScale(cmd.y);
					l.second.x = ResScale(cmd.x2);
					l.second.y = ResScale(cmd.y2);

					SetContextForeGroundColor(pPIS->TextColor);
					DrawLine(&l, 1);
					break;
				}
			case uqm::SlideOp::LineSpin:
				{ /* draw line for spin */
					int x1 {cmd.x};
					int y1 {cmd.y};
					int x2 {cmd.x2};
					int y2 {cmd.y2};
					if (uqm::currentSeed().ships)
					{
						SeedLineSpin(&x1, &y1, &x2, &y2);
					}
					GfxLine l;

					x1 <<= resolutionFactor;
					x2 <<= resolutionFactor;
					y1 <<= resolutionFactor;
					y2 <<= resolutionFactor;

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
					break;
				}
			case uqm::SlideOp::GetRect:
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
									   "GETRECT without drawing a frame first");
					}
					break;
				}
			case uqm::SlideOp::StatBox:
				{ /* draw stat box for spin */
					const auto StatboxWidth = [&] {
						return ResScale(122);
					};
					const auto StatboxHeight = [&] {
						return ResScale(60);
					};
					int x {cmd.x << resolutionFactor};
					int y {cmd.y << resolutionFactor};
					pPIS->NumSpinStat = 0;

					if (pPIS->HaveFrame
						&& (pPIS->GetRect.extent.width > 0
							&& pPIS->GetRect.extent.height > 0))
					{
						x += pPIS->GetRect.corner.x;
						y += pPIS->GetRect.corner.y;
					}

					pPIS->StatBox.corner = MAKE_POINT(x, y);
					pPIS->StatBox.extent =
						MAKE_EXTENT(StatboxWidth(), StatboxHeight());

					DoSpinStatBox(&pPIS->StatBox, pPIS->TextColor,
								  pPIS->TextBackColor, &pPIS->Skip);

					if (pPIS->Skip)
					{
						Present_BatchGraphics(pPIS);
					}
					break;
				}
			case uqm::SlideOp::Movie:
				{ /* play movie */
					Present_UnbatchGraphics(pPIS, true);

					pPIS->MovieFrame = cmd.index;
					pPIS->MovieEndFrame = cmd.lastIndex;
					pPIS->InterframeDelay = c_gameSecond / cmd.fps;

					pPIS->TimeOut = GetTimeCounter();
					pPIS->TimeOutOnSkip = true;
					return true;
				}
			case uqm::SlideOp::Animate:
				{ /* basic frame animation */
					GfxStamp s;
					int loops = 0;
					uint16_t index = 0;
					TimeCount Now;
					TimeCount timeout;
					TimeCount NextTime;
					const TimePeriod animation_rate {c_gameSecond / cmd.fps};

					s.origin.x = 0;
					s.origin.y = 0;

					timeout = GetTimeCounter() + uqstl::chrono::duration_cast<TimePeriod>(uqstl::chrono::milliseconds {cmd.ms});
					NextTime = GetTimeCounter() + animation_rate;

					while (cmd.loops || cmd.ms)
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

							if (index == cmd.lastIndex)
							{
								loops++;
								index = static_cast<uint16_t>(cmd.index);
							}

							if (cmd.loops > 0 && loops == cmd.loops)
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
		} // switch (cmd.op)
	}
	/* we are all done */
	return false;
}

static bool ShowSlidePresentation(const uqstl::vector<uqm::SlideCommand>& commands)
{
	GfxRect OldRect;
	PresentationInputState pis;

	memset(&pis, 0, sizeof(pis));
	pis.Commands = &commands;
	pis.OperIndex = 0;

	GFXCONTEXT OldContext {SetContext(ScreenContext)};
	GetContextClipRect(&OldRect);
	FONT OldFont {SetContextFont(nullptr)};
	SetContextBackGroundColor(BlackColor);

	SetMenuSounds(MenuSoundNone, MenuSoundNone);
	pis.InputFunc = DoPresentation;
	pis.LastDrawKind = -1;
	pis.TextVPos = uqm::SlideTextAlign::Bottom;
	pis.MovieFrame = -1;
	pis.StartTime = GetTimeCounter();
	pis.LastSyncTime = pis.StartTime;
	DoInput(&pis, true);

	if (pis.MusicRef && PlayingStream(MusicSource))
	{
		SleepThreadUntil(FadeMusic(0, c_gameSecond));
		StopMusic();
		FadeMusic(NormalVolume, TimePeriod {});
	}

	DestroyMusic(pis.MusicRef);
	DestroyDrawable(ReleaseDrawable(pis.RotatedFrame));
	DestroyDrawable(ReleaseDrawable(pis.Frame));
	for (int i {0}; i < uqm::c_slideshowMaxFonts; ++i)
	{
		DestroyFont(pis.Fonts[i]);
	}

	SetContextFont(OldFont);
	SetContextClipRect(&OldRect);
	SetContext(OldContext);

	return true;
}

static bool DoVideoInput(void* pIS)
{
	const VideoInputState* pVIS = (VideoInputState*)pIS;

	if (!PlayingLegacyVideo(pVIS->CurVideo))
	{ // Video probably finished
		return false;
	}

	if (PulsedInputState.menu[KEY_MENU_SELECT]
		|| PulsedInputState.menu[KEY_MENU_CANCEL]
		|| PulsedInputState.menu[KEY_MENU_SPECIAL]
		|| (uqm::currentActivity() & CHECK_ABORT))
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

		SleepThread(c_gameSecond / 40);
	}

	return true;
}

static void FadeClearScreen(void)
{
	SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));

	// clear the screen with black
	SetContext(ScreenContext);
	SetContextBackGroundColor(BlackColor);
	ClearDrawable();

	FadeScreen(FadeAllToColor, TimePeriod {});
}

static bool ShowLegacyVideo(LEGACY_VIDEO vid)
{
	VideoInputState vis;

	FadeClearScreen();

	LEGACY_VIDEO_REF ref {PlayLegacyVideo(vid)};
	if (!ref)
	{
		return false;
	}

	vis.InputFunc = DoVideoInput;
	vis.CurVideo = ref;
	SetMenuSounds(MenuSoundNone, MenuSoundNone);
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
	if (!strcmp(resType, "STRING"))
	{
		return ShowPresentationFile(res_GetString(res));
	}
	else if (!strcmp(resType, "3DOVID"))
	{
		LEGACY_VIDEO vid = LoadLegacyVideoInstance(res);
		const bool result = ShowLegacyVideo(vid);
		DestroyLegacyVideo(vid);
		return result;
	}

	if (!strcmp(resType, "STRTAB"))
	{
		uqm::log::warn("The slideshow '{}' is a string table. Convert it with scripts/convert_slideshows.py.", res);
		return false;
	}
	uqm::log::warn("Tried to present '{}', of non-presentable type '{}'", res, resType);
	return false;
}
