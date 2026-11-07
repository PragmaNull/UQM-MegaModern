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
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "fmv.h"
#include "globdata.h"
#include "libs/file/textfile.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/sound/sound.h"
#include "libs/sound/stream.h"
#include "libs/vidlib.h"
#include "nameref.h"
#include "options.h"
#include "resinst.h"
#include "settings.h"
#include "setup.h"
#include "sis.h"
#include "slideshow.h"
#include "sounds.h"
#include "util.h"

namespace uqm
{
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
	uqstl::array<FontRef, uqm::c_slideshowMaxFonts> Fonts;
	FrameRef Frame;
	MusicRef musicRef;
	bool Batched;
	FrameRef SisFrame;
	FrameRef RotatedFrame;
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
	uqstl::array<GfxText, uqm::c_slideshowMaxFadeInLines> TextLines;
	uint16_t LinesCount;
	uqstl::array<char, 512> Buffer;
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

	LegacyVideoRef CurVideo;
};

static bool DoPresentation(void* pIS);

static Color commandColor(const uqm::SlideCommand& cmd)
{
	return BuildColorRgba(cmd.color[0], cmd.color[1], cmd.color[2], 0xff);
}

static bool DoFadeScreen(PresentationInputState* pPIS, int msecs, ScreenFadeType FadeType)
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
	FrameRef SisFrame {};
	GfxStamp s;
	GfxRect r;
	HotSpot hs;
	int slot {};
	uint16_t piece {};
	Color SisBack;

	GfxContextRef OldContext {SetContext(OffScreenContext)};

	FrameRef SkelFrame {CaptureDrawable(LoadGraphic(SisskelMaskPmapAnimRes))};
	FrameRef ModuleFrame {CaptureDrawable(LoadGraphic(SismodsMaskPmapAnimRes))};

	GetFrameRect(SkelFrame, &r);
	SisFrame = CaptureDrawable(CreateDrawable(
		CreateFlags::WantPixmap | CreateFlags::WantAlpha, r.extent.width, r.extent.height, 1));
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
		piece = *uqm::gameState().sis().DriveSlots[slot];
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
		piece = *uqm::gameState().sis().JetSlots[slot];
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
		piece = *uqm::gameState().sis().ModuleSlots[slot];
		if (piece < EmptySlot)
		{
			s.origin.x = ModuleTopXP();
			s.origin.y = ModuleTopYP();
			s.origin.x += slot * ShipPieceOffset();
			s.frame = SetAbsFrameIndex(ModuleFrame, piece);
			if (piece >= ModuleType::BombModule0)
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

static void DoSpinText(const char* buf, int16_t x, int16_t y, FrameRef repair, bool* skip)
{
	GfxText Text;

	Text.pStr = buf;
	Text.CharCount = (uint16_t)uqm::utf8StringCount(buf);
	Text.align = TextAlign::Left;
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
		PlayMenuSound(MenuSoundEffect::Text);
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
		PlayMenuSound(MenuSoundEffect::Text);
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
	Text.align = TextAlign::Left;
	Text.baseline.y = y;
	Text.baseline.x = x;

	if (!*skip)
	{
		SetContextForeGroundColor(back);
		font_DrawText(&Text);
		PlayMenuSound(MenuSoundEffect::Text);
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
			PlayMenuSound(MenuSoundEffect::Text);
			SleepThread(c_gameSecond / 16);
		}
		SetContextForeGroundColor(front);
		DrawFilledRectangle(&sq);
		sq.corner.x += ResScale(6);

		UpdateInputState();
		if (CurrentInputState.menu[*MenuKey::MenuCancel] || (uqm::currentActivity() & ActivityFlag::CheckAbort))
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
			PlayMenuSound(MenuSoundEffect::Text);
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
		if (CurrentInputState.menu[*MenuKey::MenuCancel] || (uqm::currentActivity() & ActivityFlag::CheckAbort))
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
	auto text {uqm::readTextFile(uqm::contentDir(), name)};
	const auto commands {text ? uqm::slideshowFromJson(uqstl::move(*text)) : uqstl::nullopt};
	if (!commands)
	{
		uqm::log::warn("Could not read the slideshow '{}'.", name);
		return false;
	}
	return ShowSlidePresentation(*commands);
}

struct IntroShip
{
	SpeciesId sID;				   // The ship SPECIES_ID, a second index
	uqstl::array<char, 256> ditty; // The ditty string (file name)
	uqstl::array<char, 256> race;  // The race's name string e.g. EARTHLING
	uqstl::array<char, 256> ship;  // The ship's name string e.g. CRUISER
	uint16_t spinline;			   // The line on which race/ship strings occur
	uint16_t width;				   // The SD pixel width of the race name
};

const static uqstl::array<IntroShip, 25> ship_map {
	{
		{SpeciesId::Arilou, "arilou", "ARILOULALEELAY", " SKIFF", 2, 123},
		{SpeciesId::Chmmr, "chmmr", "CHMMR", " AVATAR", 2, 50},
		{SpeciesId::Earthling, "earthling", "EARTHLING", " CRUISER", 130, 79},
		{SpeciesId::Orz, "orz", "ORZ", " NEMESIS", 2, 25},
		{SpeciesId::Pkunk, "pkunk", "PKUNK", " FURY", 2, 43},
		{SpeciesId::Shofixti, "shofixti", "SHOFIXTI", " SCOUT", 4, 61},
		{SpeciesId::Spathi, "spathi", "SPATHI", " ELUDER", 2, 47},
		{SpeciesId::Supox, "supox", "SUPOX", " BLADE", 2, 43},
		{SpeciesId::Thraddash, "thraddash", "THRADDASH", " TORCH", 2, 87},
		{SpeciesId::Utwig, "utwig", "UTWIG", " JUGGER", 2, 43},
		{SpeciesId::Vux, "vux", "VUX", " INTRUDER", 130, 28},
		{SpeciesId::Yehat, "yehat", "YEHAT", " TERMINATOR", 130, 46},
		{SpeciesId::Melnorme, "melnorme", "MELNORME", " TRADER", 2, 77},
		{SpeciesId::Druuge, "druuge", "DRUUGE", " MAULER", 2, 53},
		{SpeciesId::Ilwrath, "ilwrath", "ILWRATH", " AVENGER", 2, 62},
		{SpeciesId::Mycon, "mycon", "MYCON", " PODSHIP", 127, 47},
		{SpeciesId::Slylandro, "slylandro", "SLYLANDRO", " PROBE", 1, 80},
		{SpeciesId::Umgah, "umgah", "UMGAH", " DRONE", 127, 50},
		{SpeciesId::UrQuan, "urquan", "UR-QUAN", "\nDREADNAUGHT", 119, 66},
		{SpeciesId::Zoqfotpik, "zoqfotpik", "ZOQ-FOT-PIK", " STINGER", 130, 92},
		{SpeciesId::Syreen, "syreen", "SYREEN", " PENETRATOR", 2, 53},
		{SpeciesId::KohrAh, "kohrah", "KOHR-AH", "", 119, 64},
		{SpeciesId::Androsynth, "androsynth", "ANDROSYNTH", " GUARDIAN", 2, 94},
		{SpeciesId::Chenjesu, "chenjesu", "CHENJESU", "    BROODHOME", 1, 71},
		{SpeciesId::Mmrnmhrm, "mmrnmhrm", "MMRNMHRM", " TRANSFORMER", 128, 84},
	 }
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
		if (!strcasecmp(str.data(), ship_map[shipID].ditty.data()))
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
	fmt::format_to_sz_n(buf, "ship.{}.ditty", ship_map[raceID].ditty.data());
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

	if (ship_map[shipID].sID == SpeciesId::KohrAh && ship_map[raceID].sID != SpeciesId::Arilou && ship_map[raceID].width > 80 && *y2 == 122)
	{
		*x2 -= ship_map[raceID].width - 80; // Shorten line for Spinning Blade
	}
	if (ship_map[shipID].sID == SpeciesId::Druuge && !IsHd() && ship_map[raceID].width > 80 && *y1 == 21)
	{ // Line for High-recoil Cannon in SD, needs to move down for wide ships
		*y1 += 10;
		*y2 += 10;
	}
SeedLineSpinPassThru:
	return;
}

static void SeedTextSpin(uqstl::span<char> buf, uqgsl::czstring str, int* x, int* y)
{
	size_t pos {0};
	if (!uqm::currentSeed().ships || !x || !y || shipID == raceID || shipID >= NumShips || raceID >= NumShips)
	{
		goto SeedTextSpinPassThru;
	}

	if (ship_map[shipID].sID == SpeciesId::Spathi && IsHd() && ship_map[raceID].sID != SpeciesId::Arilou && ship_map[raceID].width > 80 && (*x == 25 || *x == 40))
	{
		*x -= 15; // Rear-firing Missile Launch Tube, move left for wide ships
	}
	if (ship_map[shipID].sID == SpeciesId::Druuge && !IsHd() && ship_map[raceID].width > 80 && *x == 108)
	{
		*y += 10; // This is High-recoil Cannon, move down for wide ships
	}
	if (ship_map[shipID].sID == SpeciesId::Slylandro && *x == 18)
	{
		goto SeedTextSpinSkipLine; // This is 2418-B skip unless O.G.
	}
	if (ship_map[shipID].spinline != *y)
	{
		goto SeedTextSpinPassThru; // This is anything not on the spinline
	}
	if (ship_map[shipID].sID == SpeciesId::Melnorme && *x == 215)
	{
		goto SeedTextSpinPassThru; // This is Confusion Ray, on the spinline
	}
	if (ship_map[shipID].sID == SpeciesId::UrQuan && *x == 222)
	{
		goto SeedTextSpinPassThru; // This is Fusion Blast, on the spinline
	}
	if (linespun)
	{
		goto SeedTextSpinSkipLine; // Anything on the spinline we've done
	}

	linespun = true;
	uqm::utf8StringCopy(buf.subspan(pos), ship_map[raceID].race.data());
	pos += strlen(ship_map[raceID].race.data()) * sizeof(char);

	switch (ship_map[shipID].sID)
	{
		case SpeciesId::Chenjesu:
		case SpeciesId::Earthling:
		case SpeciesId::KohrAh:
		case SpeciesId::Mycon:
		case SpeciesId::Thraddash: // Right hand huggers
			if ((ship_map[shipID].sID == SpeciesId::Earthling || (ship_map[shipID].sID == SpeciesId::KohrAh && IsHd())) && ship_map[raceID].sID == SpeciesId::Arilou)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				pos -= 8 * sizeof(char);
				*x += ship_map[shipID].width - 50;
				break;
			}
			if (ship_map[shipID].sID == SpeciesId::Chenjesu && IsHd())
			{
				*x -= 10; // Better centering the gap over the broodhome
			}
			*x += ship_map[shipID].width - ship_map[raceID].width;
			break;
		case SpeciesId::Shofixti: // Right hand hugger but squeeze it a little
			*x += (ship_map[shipID].width - ship_map[raceID].width) * 4 / 5;
			break;
		case SpeciesId::Vux: // Middle but squeeze the right a little
			if (ship_map[shipID].sID == SpeciesId::Vux && ship_map[raceID].sID == SpeciesId::Arilou)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				pos -= 8 * sizeof(char);
				*x += (ship_map[shipID].width - 50) * 3 / 5;
				break;
			}
			*x += (ship_map[shipID].width - ship_map[raceID].width) * 3 / 5;
			break;
		case SpeciesId::Androsynth:
		case SpeciesId::Arilou:
		case SpeciesId::Pkunk:
		case SpeciesId::Spathi:
		case SpeciesId::Syreen:
		case SpeciesId::Utwig:
		case SpeciesId::Mmrnmhrm: // Middle-ships
			if (ship_map[shipID].sID == SpeciesId::Spathi && ship_map[raceID].sID == SpeciesId::Arilou)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				pos -= 8 * sizeof(char);
				*x += (ship_map[shipID].width - 50) / 2;
				break;
			}
			if (ship_map[shipID].sID == SpeciesId::Spathi)
			{
				*x += 2; // Better centering over (In Attack Position)
			}
			if (ship_map[shipID].sID == SpeciesId::Arilou)
			{
				*x += 4; // Better centering over (Last Reported Position)
			}
			*x += (ship_map[shipID].width - ship_map[raceID].width) / 2;
			break;
		case SpeciesId::Chmmr:
		case SpeciesId::Ilwrath:
		case SpeciesId::Melnorme:
		case SpeciesId::Orz:
		case SpeciesId::Slylandro:
		case SpeciesId::Umgah:
		case SpeciesId::UrQuan:
		case SpeciesId::Yehat:
		case SpeciesId::Zoqfotpik: // Lefties need no adjustment
			if ((ship_map[shipID].sID == SpeciesId::Chmmr || ship_map[shipID].sID == SpeciesId::Orz || ship_map[shipID].sID == SpeciesId::Zoqfotpik) && ship_map[raceID].sID == SpeciesId::Arilou)
			{ // Special case - ship doesn't have room, cuts to "ARILOU"
				pos -= 8 * sizeof(char);
				break;
			}
			if (ship_map[shipID].sID == SpeciesId::Umgah && !IsHd())
			{ // Right hand hugger but squeeze it a little
				*x += (ship_map[shipID].width - ship_map[raceID].width)
					* 4 / 5;
				break;
			}
			break;
		case SpeciesId::Druuge:
		case SpeciesId::Supox: // Lefties but line break when long
			if (ship_map[raceID].width > 80)
			{
				const char* pad = (IsHd() ? "\n   " : "\n       ");
				uqm::utf8StringCopy(buf.subspan(pos), pad);
				pos += strlen(pad) * sizeof(char);
			}
			break;
		default:
			break;
	}

	uqm::utf8StringCopy(buf.subspan(pos), ship_map[shipID].ship.data());
	return;

SeedTextSpinSkipLine:
	uqm::utf8StringCopy(buf.subspan(pos), "");
	return;

SeedTextSpinPassThru:
	uqm::utf8StringCopy(buf.subspan(pos), str);
	return;
}

static bool DoPresentation(void* pIS)
{
	PresentationInputState* pPIS = (PresentationInputState*)pIS;

	if (PulsedInputState.menu[*MenuKey::MenuCancel]
		|| (uqm::currentActivity() & ActivityFlag::CheckAbort))
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

		if (pPIS->TimeOutOnSkip && (PulsedInputState.menu[*MenuKey::MenuSelect] || PulsedInputState.menu[*MenuKey::MenuSpecial] || PulsedInputState.menu[*MenuKey::MenuRight]))
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
					FontRef* const pFont {&pPIS->Fonts[cmd.index]};

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
					if (pPIS->musicRef)
					{
						StopMusic();
						DestroyMusic(pPIS->musicRef);
					}
					pPIS->musicRef = LoadMusicFile(cmd.text.c_str());
					PlayMusic(pPIS->musicRef, false, 1);
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

					if (pPIS->musicRef)
					{
						StopMusic();
						DestroyMusic(pPIS->musicRef);
					}

					pPIS->musicRef = LoadMusicInstance(pPIS->Buffer.data());
					PlayMusic(pPIS->musicRef, false, 1);
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
						if (CurrentInputState.menu[*MenuKey::MenuCancel]
							|| (uqm::currentActivity() & ActivityFlag::CheckAbort))
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
							if (CurrentInputState.menu[*MenuKey::MenuCancel]
								|| (uqm::currentActivity() & ActivityFlag::CheckAbort))
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

					t.align = TextAlign::Center;
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
						SeedTextSpin(pPIS->Buffer,
									 cmd.text.c_str(), &x, &y);
					}
					else
					{
						uqm::utf8StringCopy(pPIS->Buffer,
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
					if (uqm::currentSeed().ships && !strncmp(pPIS->Buffer.data(), "SPATHI", 6))
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
						if (ship_map[shipID].sID == SpeciesId::UrQuan)
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
						DoSpinText(pPIS->Buffer.data(), x, y + ResScale(7),
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
						uqstl::array<char, sizeof(pPIS->Buffer) + 12> buf;
						GfxText t;

						uqm::log::warn("SPINSTAT: Stats exceed max "
									   "values '{}'",
									   cmd.text);
						fmt::format_to_sz_n(buf, "{} Exceed max !", pPIS->Buffer.data());

						t.align = TextAlign::Left;
						t.pStr = buf.data();
						t.CharCount = (uint16_t)~0;
						t.baseline = MAKE_POINT(x, y);
						DrawTextEffect(&t,
									   BuildColorRgba(0xFF, 0x55, 0x55, 0xFF),
									   pPIS->TextBackColor, pPIS->TextTraced);
					}
					else
					{
						DoSpinStat(pPIS->Buffer.data(),
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
						pPIS->TextLines[i].align = TextAlign::Center;
						pPIS->TextLines[i].baseline.x = CanvasWidth / 2;
						pPIS->TextLines[i].baseline.y = y;
					}

					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines.data() + i, pPIS->TextFadeColor,
									   pPIS->TextFadeColor, pPIS->TextTraced);
					}

					/* do transition */
					SetTransitionSource(&pPIS->tfade_r);
					BatchGraphics();
					for (i = 0; i < pPIS->LinesCount; ++i)
					{
						DrawTextEffect(pPIS->TextLines.data() + i, pPIS->TextColor,
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
						DrawTextEffect(pPIS->TextLines.data() + i, pPIS->TextFadeColor,
									   pPIS->TextFadeColor, pPIS->TextTraced);
					}
					ScreenTransition(uqm::EmulationMode::PC | uqm::EmulationMode::Console3DO, &pPIS->tfade_r);
					UnbatchGraphics();
					break;
				}
			case uqm::SlideOp::SaveBackground:
				{ /* save background */
					TFB_DrawScreen_Copy(&pPIS->clip_r,
										ScreenBuffer::Main, ScreenBuffer::Extra);
					break;
				}
			case uqm::SlideOp::RestoreBackground:
				{ /* restore background */
					TFB_DrawScreen_Copy(&pPIS->clip_r,
										ScreenBuffer::Extra, ScreenBuffer::Main);
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
					const tfb::ScaleMode old_mode {
						SetGraphicScaleMode(drawSis ? tfb::ScaleMode::Nearest : tfb::ScaleMode::Bilinear)};
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
					return DoFadeScreen(pPIS, cmd.ms, ScreenFadeType::FadeAllToColor);
				}
			case uqm::SlideOp::FadeToBlack:
				{ /* fade to black */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, cmd.ms, ScreenFadeType::FadeAllToBlack);
				}
			case uqm::SlideOp::FadeToWhite:
				{ /* fade to white */
					Present_UnbatchGraphics(pPIS, true);
					return DoFadeScreen(pPIS, cmd.ms, ScreenFadeType::FadeAllToWhite);
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
				{ /* Get currently drawn FrameRef rect */
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

	GfxContextRef OldContext {SetContext(ScreenContext)};
	GetContextClipRect(&OldRect);
	FontRef OldFont {SetContextFont(nullptr)};
	SetContextBackGroundColor(BlackColor);

	SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
	pis.InputFunc = DoPresentation;
	pis.LastDrawKind = -1;
	pis.TextVPos = uqm::SlideTextAlign::Bottom;
	pis.MovieFrame = -1;
	pis.StartTime = GetTimeCounter();
	pis.LastSyncTime = pis.StartTime;
	DoInput(&pis, true);

	if (pis.musicRef && PlayingStream(MusicSource))
	{
		SleepThreadUntil(FadeMusic(0, c_gameSecond));
		StopMusic();
		FadeMusic(NormalVolume, TimePeriod {});
	}

	DestroyMusic(pis.musicRef);
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

	if (PulsedInputState.menu[*MenuKey::MenuSelect]
		|| PulsedInputState.menu[*MenuKey::MenuCancel]
		|| PulsedInputState.menu[*MenuKey::MenuSpecial]
		|| (uqm::currentActivity() & ActivityFlag::CheckAbort))
	{ // abort movie
		return false;
	}
	else if (PulsedInputState.menu[*MenuKey::MenuLeft]
			 || PulsedInputState.menu[*MenuKey::MenuRight])
	{
		int32_t newpos = VidGetPosition();
		if (PulsedInputState.menu[*MenuKey::MenuLeft])
		{
			newpos -= 2000;
		}
		else if (PulsedInputState.menu[*MenuKey::MenuRight])
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
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));

	// clear the screen with black
	SetContext(ScreenContext);
	SetContextBackGroundColor(BlackColor);
	ClearDrawable();

	FadeScreen(ScreenFadeType::FadeAllToColor, TimePeriod {});
}

static bool ShowLegacyVideo(LegacyVideo vid)
{
	VideoInputState vis;

	FadeClearScreen();

	LegacyVideoRef ref {PlayLegacyVideo(vid)};
	if (!ref)
	{
		return false;
	}

	vis.InputFunc = DoVideoInput;
	vis.CurVideo = ref;
	SetMenuSounds(MenuSoundFlags::None, MenuSoundFlags::None);
	DoInput(&vis, true);

	StopLegacyVideo(ref);
	FadeClearScreen();

	return true;
}

bool ShowPresentation(ResourceName res)
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
		LegacyVideo vid = LoadLegacyVideoInstance(res);
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

} // namespace uqm
