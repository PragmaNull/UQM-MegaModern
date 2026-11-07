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

#include <ctype.h>
#include <string.h>

#include "../colors.h"
#include "../controls.h"
#include "../gamestr.h"
#include "../ifontres.h"
#include "../igfxres.h"
#include "../nameref.h"
#include "../setup.h"
#include "../sounds.h"
#include "../uqmdebug.h"
#include "../util.h"
#include "core/stl/optional.h"
#include "lander.h"
#include "libs/inplib.h"
#include "options.h"
#include "planets.h"
#include "scan.h"
#include "uqm/lua/UQMLua.h"


namespace uqm
{
static auto ColMultiplier() { return (isPC(uqm::UQMOptions::read().landerStyle) || IsPad() ? 7 : 6); }
static auto NumCellCols() { return (UqmMapWidth / ColMultiplier()); }
static constexpr auto NumCellRows {Sc2MapHeight / 6};


extern FrameRef SpaceJunkFrame;

static void ClearReportArea(int16_t startx)
{
	uint16_t x {};
	uint16_t y {};
	GfxRect r;
	GfxStamp s;

	if (uqm::UQMOptions::read().whichFonts == uqm::EmulationMode::PC)
	{
		s.frame = SetAbsFrameIndex(SpaceJunkFrame, 21);
	}
	else
	{
		s.frame = SetAbsFrameIndex(SpaceJunkFrame, 18);
	}
	GetFrameRect(s.frame, &r);
	const BatchScope batch {};

	SetContextBackGroundColor(BlackColor);
	ClearDrawable();
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x00, 0x07, 0x00), 0x57));

	s.origin.y = ResScale(1);
	for (y = 0; y < NumCellRows; ++y)
	{
		s.origin.x = startx;
		for (x = 0; x < NumCellCols(); ++x)
		{
			if (uqm::UQMOptions::read().whichFonts == uqm::EmulationMode::PC)
			{
				DrawStamp(&s);
			}
			else
			{
				DrawFilledStamp(&s);
			}

			s.origin.x += r.extent.width + ResScale(1);
		}
		s.origin.y += r.extent.height + ResScale(1);
	}
}

static void MakeReport(SoundBankRef ReadOutSounds, uqstl::string_view text)
{
	const char* pStr {text.data()};
	uint16_t StrLen {static_cast<uint16_t>(text.size())};
	int end_page_len {};
	uqstl::array<char, 200> end_page_buf {};
	uqm::UniChar last_c = 0;
	GfxRect r;
	GfxRect contextRect;
	GfxText t;
	Color fgcolor;
	int16_t startx {};

	int16_t total_lines = -1;
	int16_t curr_line = -1;
	uint16_t first_line_length = 0;

	fmt::format_to_n(end_page_buf.data(), sizeof(end_page_buf) - 1, "{}\n",
					 GameString(static_cast<int>(GameStringBase::ScanString) + static_cast<int>(ScanType::Count)));
	end_page_len = uqm::utf8StringCount(end_page_buf.data());

	GetFrameRect(SetAbsFrameIndex(SpaceJunkFrame, 18), &r);
	GetContextClipRect(&contextRect);

	t.align = TextAlign::Left;
	t.CharCount = 1;
	t.pStr = pStr;
	bool Sleepy {true};

	FlushInput();

	t.baseline.y = r.extent.height + ResScale(1);
	// Text vertical alignment
	uint16_t row_cells {0};
	fgcolor = BuildColor(MakeRgb15(0x00, 0x1F, 0x00), 0xFF);

	if (contextRect.extent.width < ScaledMapWidth())
	{
		startx = ResScale(1); // Special case if we're in space
	}
	else
	{ // In DOS version first cell is 3p away from the edge of the
		// context, and 2 in UQM
		startx = ResScale(ResDescale(r.extent.width) >> 1)
			   + (isPC(uqm::UQMOptions::read().landerStyle) ? ResScale(1) : 0);
	}

	startx += SafeNumScl(1);

	if (StrLen)
	{
		ClearReportArea(startx);
		SetContextForeGroundColor(fgcolor);

		if (is3DO(uqm::UQMOptions::read().landerStyle))
		{
			const char* pCurrStr {t.pStr};
			total_lines = 0;
			curr_line = 0;

			for (uint16_t length {StrLen}; length > 0; length--)
			{
				if (uqm::getCharFromString(&pCurrStr) == '\n')
				{
					total_lines++;
				}

				if (total_lines == 0)
				{
					first_line_length++;
				}
			}
		}
	}

	while (StrLen)
	{
		const char* pNextStr {};
		uint16_t lf_pos {};

		const char* pLastStr {t.pStr};

		// scan for LFs in the remaining string
		// trailing LF will be ignored
		for (lf_pos = StrLen, pNextStr = t.pStr;
			 lf_pos && uqm::getCharFromString(&pNextStr) != '\n';
			 --lf_pos)
			;

		uint16_t col_cells {0};
		// check if the remaining text fits on current screen
		if (row_cells == NumCellRows - 1
			&& (StrLen > NumCellCols() || lf_pos > 1))
		{
			col_cells = (NumCellCols() >> 1) - (end_page_len >> 1);
			t.pStr = end_page_buf.data();
			StrLen += end_page_len;

			// We're adding lines - compensate for it
			if (curr_line >= 0)
			{
				curr_line--;
			}
		}
		if (total_lines >= 0)
		{
			if (curr_line == 0)
			{
				col_cells = (NumCellCols() - first_line_length) >> 1;
			}

			if (curr_line == total_lines)
			{
				col_cells = ((NumCellCols() - (StrLen - 2)) >> 1) - 2;
			}
		}

		t.baseline.x = startx + (col_cells * (r.extent.width + ResScale(1)));

		do
		{
			const char* pStr {};
			uqm::UniChar c {};

			pStr = t.pStr;
			pNextStr = t.pStr;
			while (uqm::isGraphChar(uqm::getCharFromString(&pNextStr)))
			{
				pStr = pNextStr;
			}

			uint16_t word_chars {(uint16_t)uqm::utf8StringCount(uqstl::string_view {t.pStr, static_cast<size_t>(pStr - t.pStr)})};
			if ((col_cells += word_chars) <= NumCellCols())
			{
				TimeCount TimeOut;

				if (StrLen -= word_chars)
				{
					--StrLen;
				}
				TimeOut = GetTimeCounter();
				while (word_chars--)
				{
					pNextStr = t.pStr;
					c = uqm::getCharFromString(&pNextStr);

					if (uqm::currentActivity() & ActivityFlag::CheckAbort)
					{
						return;
					}
					if (!Sleepy)
					{
						font_DrawText(&t);
					}
					else
					{
						uint8_t scale = 0;
						font_DrawText(&t);

						if (CurrentInputState.menu[*MenuKey::MenuRight])
						{
							scale = 2;
						}

						PlaySound(GetSound(ReadOutSounds, 0), NotPositional(), nullptr,
								  GameSoundPriority);

						switch (c)
						{
							case ',':
								TimeOut += c_gameSecond / (4 << scale);
								break;
							case '.':
								TimeOut += c_gameSecond / (2 << scale);
								break;
							case '!':
								if (last_c != '!' && last_c != ' ')
								{
									TimeOut += c_gameSecond / (2 << scale);
								}
								else
								{
									TimeOut += c_gameSecond / (20 << scale);
								}
								break;
							case '?':
								if (last_c != '?' && last_c != ' ')
								{
									TimeOut += c_gameSecond / (2 << scale);
								}
								else
								{
									TimeOut += c_gameSecond / (20 << scale);
								}
								break;
							default:
								TimeOut += c_gameSecond / (20 << scale);
								break;
						}

						last_c = c;

						if (word_chars == 0)
						{
							TimeOut += c_gameSecond / (20 << scale);
						}

						if (WaitForActButtonUntil(true, TimeOut, false))
						{
							Sleepy = false;
							// We draw the whole thing at once after this
							BatchGraphics();
						}
					}
					t.pStr = pNextStr;
					t.baseline.x += r.extent.width + ResScale(1);
					// Text spacing
				}

				++col_cells;
				last_c = uqm::getCharFromString(&t.pStr);
				t.baseline.x += r.extent.width + ResScale(1);
				// Space spacing

				if (curr_line >= 0 && last_c == '\n')
				{
					curr_line++;
				}
			}
		} while (col_cells <= NumCellCols() && last_c != '\n' && StrLen);

		t.baseline.y += r.extent.height + ResScale(1);
		// Text vertical spacing
		if (++row_cells == NumCellRows || StrLen == 0)
		{
			t.pStr = pLastStr;
			if (!Sleepy)
			{
				UnbatchGraphics();
			}

			if (!WaitForActButton(true, WaitInfinite, false))
			{
				break;
			}

			t.baseline.y = r.extent.height + ResScale(1);
			// Text vertical alignment
			row_cells = 0;
			if (StrLen)
			{
				if (!Sleepy)
				{
					BatchGraphics();
				}
				ClearReportArea(startx);
				SetContextForeGroundColor(fgcolor);
			}
		}
	}
}

void DoDiscoveryReport(SoundBankRef ReadOutSounds)
{
	bool ownContext {};
	GfxStamp saveStamp;

#ifdef DEBUG
	if (disableInteractivity)
	{
		return;
	}
#endif

	GfxContextRef context {GetScanContext(&ownContext)};
	{
		const ContextScope contextScope {context};
		saveStamp = SaveContextFrame(nullptr);
		{
			FrameRef OldFontEffect {};

			const FontScope fontScope {pSolarSysState->SysInfo.PlanetInfo.LanderFont};
			if (uqm::UQMOptions::read().whichFonts == uqm::EmulationMode::PC)
			{
				OldFontEffect = SetContextFontEffect(
					pSolarSysState->SysInfo.PlanetInfo.LanderFontEff);
			}
			else
			{
				OldFontEffect = SetContextFontEffect(nullptr);
			}

			uqm::lua().beginComm(nullptr, NullResource);
			uqstl::late_constructed<uqstl::string> interpolatedReport {};
			const PlanetData& planetInfo {pSolarSysState->SysInfo.PlanetInfo};
			uqstl::string_view report {planetInfo.DiscoveryStrings->text(planetInfo.DiscoveryIndex)};
			if (uqm::UQMLua::needsInterpolate(report.data()))
			{
				interpolatedReport.emplace(uqm::lua().interpolate(report));
				report = *interpolatedReport;
			}
			MakeReport(ReadOutSounds, report);

			uqm::lua().endComm();

			SetContextFontEffect(OldFontEffect);
		}
		// Restore previous screen
		DrawStamp(&saveStamp);
	}
	// TODO: Make GfxContextRef ref-counted
	if (ownContext)
	{
		DestroyScanContext();
	}

	DestroyDrawable(ReleaseDrawable(saveStamp.frame));

	WaitForNoInput(WaitInfinite, true);
}

} // namespace uqm