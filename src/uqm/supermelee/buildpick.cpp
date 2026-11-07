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

#include "buildpick.h"

#include "core/string/StringUtils.h"

#include "../controls.h"
#include "../colors.h"
#include "../fmv.h"
#include "../master.h"
#include "../setup.h"
#include "../sounds.h"
#include "libs/gfxlib.h"
#include "../gamestr.h"

namespace uqm
{
static FrameRef BuildPickFrame;

static constexpr auto TtipFrameOffset {43};
static constexpr auto BuildpickFrameOffset {27};

void BuildBuildPickFrame(void)
{
	GfxStamp s;
	GfxRect r;
	GfxContextRef OldContext = SetContext(OffScreenContext);

	// create team building ship selection box
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex(MeleeFrame,
							   BuildpickFrameOffset + uqstl::to_underlying(uqm::UQMOptions::read().optControllerType.value));
	// 5x5 grid of ships to pick from
	GetFrameRect(s.frame, &r);

	BuildPickFrame = CaptureDrawable(CreateDrawable(
		CreateFlags::WantPixmap, r.extent.width, r.extent.height, 1));
	SetContextFGFrame(BuildPickFrame);
	SetFrameHot(s.frame, MAKE_HOT_SPOT(0, 0));
	DrawStamp(&s);

	for (uint16_t i {0}; i < NumPickCols * NumPickRows; ++i)
	{
		DrawPickIcon((MeleeShip)i, true);
	}

	DrawShipPickerText(s);

	SetContext(OldContext);
}

void DestroyBuildPickFrame(void)
{
	DestroyDrawable(ReleaseDrawable(BuildPickFrame));
	BuildPickFrame = 0;
}

Color GetShipFlashColor(void)
{
	if (is3DO(uqm::UQMOptions::read().whichMenu))
	{
		return BlackColor;
	}
	else
	{
		static uint8_t cycle_index = 0;

		const auto& cycle_tab {ShipSelectColorCycleTable};
		const size_t cycleCount = std::size(cycle_tab);

		cycle_index = (cycle_index + 1) % cycleCount;

		return cycle_tab[cycle_index];
	}
}

// Draw part of the frame underneath the ship (removes artifacts in HdResolutionFactor)
void RepairBuildPickFrame(GfxRect* pRect, GfxPoint* origin)
{
	GfxRect r;
	GfxRect OldRect;
	GfxPoint oldOrigin;
	GfxStamp s;

	r.corner.x = origin->x;
	r.corner.y = origin->y;
	r.extent.width = r.extent.height = ResScale(16);

	GfxContextRef OldContext {SetContext(SpaceContext)};
	GetContextClipRect(&OldRect);
	SetContextClipRect(&r);
	if (IsPad())
	{
		oldOrigin = SetContextOrigin(MAKE_POINT(r.corner.x, r.corner.y));
	}
	else
	{
		oldOrigin =
			SetContextOrigin(MAKE_POINT(-r.corner.x, -r.corner.y));
	}

	s.origin.x = pRect->corner.x;
	s.origin.y = pRect->corner.y;
	s.frame = SetAbsFrameIndex(MeleeFrame,
							   BuildpickFrameOffset + uqstl::to_underlying(uqm::UQMOptions::read().optControllerType.value));

	DrawStamp(&s);

	SetContextOrigin(oldOrigin);
	SetContextClipRect(&OldRect);
	SetContext(OldContext);
}

// Draw a ship icon in the ship selection popup.
void DrawPickIcon(MeleeShip ship, bool DrawErase)
{
	GfxStamp s;
	GfxRect r;

	GetFrameRect(BuildPickFrame, &r);

	s.origin.x = r.corner.x + ResScale(20) + (*ship % NumPickCols) * ResScale(18);
	s.origin.y = r.corner.y + ResScale(5) + (*ship / NumPickCols) * ResScale(18);

	s.frame = GetShipIconsFromIndex(*ship);

	// Draw a rectangle below ship to remove artifacts in HdResolutionFactor
	if (IsHd())
	{
		RepairBuildPickFrame(&r, &s.origin);
	}

	if (DrawErase)
	{ // draw icon
		DrawStamp(&s);
	}
	else
	{ // erase icon
		Color OldColor;

		OldColor = SetContextForeGroundColor(GetShipFlashColor());
		DrawFilledStamp(&s);
		SetContextForeGroundColor(OldColor);
	}
}

static void DrawTooltipBox(void)
{
	GfxStamp s;

	s.origin.x = s.origin.y = 0;
	s.frame = SetAbsFrameIndex(MeleeFrame, TtipFrameOffset);

	DrawStamp(&s);
}

void GetToolTipFrameRect(GfxRect* r)
{
	GetFrameRect(SetAbsFrameIndex(MeleeFrame, TtipFrameOffset), r);
}

static constexpr auto RaceNameOffset {0};
static constexpr auto RaceShipOffset {3};

void DrawTooltip(ShipInfoDesc* SIPtr)
{
	char buf[PATH_MAX] {};
	GfxText Text;
	Color oldColor;
	GfxRect r;

	GetToolTipFrameRect(&r);

	fmt::format_to_n(buf, sizeof(buf) - 1, "{} {}",
					 SIPtr->race_strings->text(RaceNameOffset),
					 SIPtr->race_strings->text(RaceShipOffset));

	Text.pStr = buf;
	Text.CharCount = (uint16_t)uqm::utf8StringCount(buf);
	Text.align = TextAlign::Center;
	Text.baseline.y = r.corner.y + ResScale(8) + ResScale(1);
	Text.baseline.x = r.corner.x + (r.extent.width >> 1) + ResScale(1);

	GfxContextRef oldContext {SetContext(SpaceContext)};
	DrawTooltipBox();

	FontRef oldFont {SetContextFont(StarConFont)};
	oldColor = SetContextForeGroundColor(TooltipColorNameBack);
	font_DrawText(&Text);

	SetContextForeGroundColor(TooltipColorNameFront);
	Text.baseline.x -= ResScale(1);
	Text.baseline.y -= ResScale(1);
	font_DrawText(&Text);

	SetContextForeGroundColor(TooltipColorDescFront);

	uqm::utf8StringCopy(buf, sizeof buf,
						SIPtr->race_strings->text(SIPtr->race_strings->size() - 1));
	Text.baseline.y += ResScale(2);

	{
		uqstl::vector<uqstl::string> tokens;
		uqm::tokenize(uqstl::string_view {(const char*)buf}, tokens, '\n', false);
		for (const auto& tok : tokens)
		{
			Text.pStr = (const char*)tok.c_str();
			Text.CharCount = (uint16_t)uqm::utf8StringCount(Text.pStr);
			Text.baseline.y += ResScale(9);
			font_DrawText(&Text);
		}
	}

	SetContextForeGroundColor(oldColor);
	SetContextFont(oldFont);
	SetContext(oldContext);
}

void DrawPickFrame(MeleeState* pMS)
{
	GfxRect r;
	GfxRect r0;
	GfxRect r1;
	GfxRect ship_r;
	GfxStamp s;

	GetShipBox(&r0, 0, 0, 0),
		GetShipBox(&r1, 1, NumMeleeRows - 1, NumMeleeColumns() - 1),
		BoxUnion(&r0, &r1, &ship_r);

	s.frame = SetAbsFrameIndex(BuildPickFrame, 0);
	GetFrameRect(s.frame, &r);
	r.corner.x = -(ship_r.corner.x
				   + ((ship_r.extent.width - r.extent.width) >> 1));
	if (pMS->side)
	{
		r.corner.y = -ship_r.corner.y;
	}
	else
	{
		r.corner.y = -(ship_r.corner.y
					   + (ship_r.extent.height - r.extent.height));
	}
	SetFrameHot(s.frame, MAKE_HOT_SPOT(r.corner.x, r.corner.y));
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp(&s);
	DrawMeleeShipStrings(pMS, pMS->currentShip);

	if (isPC(uqm::UQMOptions::read().whichMenu))
	{ // if PC menu is selected - draw flash on the current ship as soon
		// as we pop up otherwise for 1 frame it will look like nothing is
		// selected
		DrawPickIcon(pMS->currentShip, false);
	}
}

void GetBuildPickFrameRect(GfxRect* r)
{
	GetFrameRect(BuildPickFrame, r);
}

static bool DoPickShip(MeleeState* pMS)
{
	const TimeCount TimeIn {GetTimeCounter()};

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		pMS->buildPickConfirmed = false;
		return false;
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	if (PulsedInputState.menu[*MenuKey::MenuSelect] || PulsedInputState.menu[*MenuKey::MenuCancel])
	{
		// Confirm selection or cancel.
		pMS->buildPickConfirmed = !PulsedInputState.menu[*MenuKey::MenuCancel];
		return false;
	}

	if (PulsedInputState.menu[*MenuKey::MenuSpecial]
		&& (pMS->currentShip != MeleeShip::None))
	{
		if (isPC(uqm::UQMOptions::read().whichIntro))
		{
			PlayMenuSound(MenuSoundEffect::Success);
		}
		// Show ship spin video.
		DoShipSpin(*pMS->currentShip, pMS->hMusic);

		return true;
	}

	{
		MeleeShip newSelectedShip;

		newSelectedShip = pMS->currentShip;

		if (PulsedInputState.menu[*MenuKey::MenuLeft])
		{
			if (*newSelectedShip % NumPickCols == 0)
			{
				newSelectedShip += NumPickCols;
			}
			--newSelectedShip;
		}
		else if (PulsedInputState.menu[*MenuKey::MenuRight])
		{
			++newSelectedShip;
			if (*newSelectedShip % NumPickCols == 0)
			{
				newSelectedShip -= NumPickCols;
			}
		}

		if (PulsedInputState.menu[*MenuKey::MenuUp])
		{
			if (newSelectedShip >= NumPickCols)
			{
				newSelectedShip -= NumPickCols;
			}
			else
			{
				newSelectedShip += NumPickCols * (NumPickRows - 1);
			}
		}
		else if (PulsedInputState.menu[*MenuKey::MenuDown])
		{
			if (newSelectedShip < NumPickCols * (NumPickRows - 1))
			{
				newSelectedShip += NumPickCols;
			}
			else
			{
				newSelectedShip -= NumPickCols * (NumPickRows - 1);
			}
		}

		if (newSelectedShip != pMS->currentShip)
		{
			// A new ship has been selected.
			DrawPickIcon(pMS->currentShip, true);
			pMS->currentShip = newSelectedShip;
			DrawMeleeShipStrings(pMS, newSelectedShip);
		}
	}

	Melee_flashSelection(pMS);

	SleepThreadUntil(TimeIn + c_gameSecond / 30);

	return true;
}

// Returns true if a ship has been selected, or false if the operation has
// been cancelled or if the general abort key was pressed (in which case
// 'uqm::currentActivity() & CHECK_ABORT' is true as usual.
// If a ship was selected, pMS->currentShip is set to the selected ship.
bool BuildPickShip(MeleeState* pMS)
{
	FlushInput();

	if (pMS->currentShip == MeleeShip::None)
	{
		pMS->currentShip = (MeleeShip)0;
	}

	DrawPickFrame(pMS);

	pMS->InputFunc = DoPickShip;
	DoInput(pMS, false);

	return pMS->buildPickConfirmed;
}

} // namespace uqm
