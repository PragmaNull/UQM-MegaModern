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

#include "colors.h"
#include "globdata.h"
#include "options.h"
#include "status.h"
#include "setup.h"
#include "libs/gfxlib.h"
#include "menustat.h"
#include "util.h"

static constexpr auto LowFuelColor {BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E)};

void DrawCrewFuelString(int16_t y, int16_t state)
{
	STAMP Stamp;

	Stamp.origin.y = y + GaugeYoffs() + StarconTextHeight();
	if (state == 0)
	{
		Stamp.origin.x = CrewXoffs() + RES_SCALE(RES_DESCALE(StatWidth()) >> 1) + RES_SCALE(5);
		if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
		{
			Stamp.frame = SetAbsFrameIndex(StatusFrame, 4);
		}
		else
		{
			Stamp.frame = SetAbsFrameIndex(StatusFrame, 0);
		}
		DrawStamp(&Stamp);
	}

	Stamp.origin.x = EnergyXoffs() + RES_SCALE(RES_DESCALE(StatWidth()) >> 1) - RES_SCALE(4);
	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		Stamp.frame = SetAbsFrameIndex(StatusFrame, 5);
	}
	else
	{
		Stamp.frame = SetAbsFrameIndex(StatusFrame, 1);
	}
	if (state >= 0)
	{
		if (IsHd())
		{
			GFXRECT r;
			GetFrameRect(Stamp.frame, &r);
			r.corner.x += Stamp.origin.x;
			r.corner.y += Stamp.origin.y;
			SetContextForeGroundColor(
				BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08));
			DrawFilledRectangle(&r);
		}

		DrawStamp(&Stamp);
	}
	else
	{
		SetContextForeGroundColor(LowFuelColor);
		DrawFilledStamp(&Stamp);
	}
}

static void
DrawShipNameString(const char* pStr, uint16_t CharCount, int16_t y)
{
	TEXT Text;

	FONT OldFont {SetContextFont(StarConFont)};

	Text.pStr = pStr;
	Text.CharCount = CharCount;
	Text.align = ALIGN_CENTER;

	Text.baseline.y = StarconTextHeight() + RES_SCALE(3) + y;
	Text.baseline.x = StatusWidth() >> 1;

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19));
	font_DrawText(&Text);
	Text.baseline.y -= RES_SCALE(1);
	SetContextForeGroundColor(BlackColor);
	font_DrawText(&Text);

	SetContextFont(OldFont);
}

void ClearShipStatus(int16_t y)
{
	GFXRECT r;

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08));
	r.corner.x = RES_SCALE(2);
	r.corner.y = RES_SCALE(3) + y;
	r.extent.width = StatusWidth() - RES_SCALE(4);
	r.extent.height = ShipInfoHeight() - RES_SCALE(3);
	DrawFilledRectangle(&r);
}

void OutlineShipStatus(int16_t y)
{
	GFXRECT r;

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F));
	r.corner.x = 0;
	r.corner.y = RES_SCALE(1) + y;
	r.extent.width = StatusWidth();
	r.extent.height = RES_SCALE(1);
	DrawFilledRectangle(&r);
	r.corner.y += RES_SCALE(1);
	r.extent.width -= RES_SCALE(1);
	DrawFilledRectangle(&r);
	r.extent.width = RES_SCALE(1);
	r.extent.height = ShipInfoHeight() - RES_SCALE(2);
	DrawFilledRectangle(&r);
	r.corner.x += RES_SCALE(1);
	DrawFilledRectangle(&r);

	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19));
	r.corner.x = StatusWidth() - RES_SCALE(1);
	DrawFilledRectangle(&r);
	r.corner.x = StatusWidth() - RES_SCALE(2);
	r.corner.y += RES_SCALE(1);
	r.extent.height -= RES_SCALE(1);
	DrawFilledRectangle(&r);

	SetContextForeGroundColor(BlackColor);
	r.corner.x = 0;
	r.corner.y = y;
	r.extent.width = StatusWidth();
	r.extent.height = RES_SCALE(1);
	DrawFilledRectangle(&r);
}

void InitShipStatus(SHIP_INFO* SIPtr, STARSHIP* StarShipPtr, GFXRECT* pClipRect, bool inMeleeMenu)
{
	GFXRECT r;
	int16_t y = 0; // default, for Melee menu
	STAMP Stamp;
	GFXRECT oldClipRect;
	GFXPOINT oldOrigin = {0, 0};

	if (StarShipPtr) // set during battle
	{
		assert(StarShipPtr->playerNr >= 0);
		y = status_y_offsets[StarShipPtr->playerNr];

		{
			CAPTAIN_STUFF* cs = &StarShipPtr->RaceDescPtr->ship_data.captain_control;

			cs->tl_offset = 0;
			cs->tr_offset = 0;
			cs->thrust_offset = 0;
			cs->weapon_offset = 0;
			cs->special_offset = 0;
		}
	}

	GFXCONTEXT OldContext {SetContext(StatusContext)};
	if (pClipRect)
	{
		GetContextClipRect(&oldClipRect);
		r = oldClipRect;
		r.corner.x += pClipRect->corner.x;
		r.corner.y += (pClipRect->corner.y & ~1);
		r.extent = pClipRect->extent;
		r.extent.height += pClipRect->corner.y & 1;
		SetContextClipRect(&r);
		// Offset the origin so that we draw into the cliprect
		oldOrigin = SetContextOrigin(MAKE_POINT(-pClipRect->corner.x,
												-(pClipRect->corner.y & ~1)));
	}

	BatchGraphics();

	if (!inMeleeMenu)
	{
		OutlineShipStatus(y);
	}
	ClearShipStatus(y);

	Stamp.origin.x = (StatusWidth() >> 1);
	Stamp.origin.y = RES_SCALE(31) + y;
	Stamp.frame = IncFrameIndex(SIPtr->icons);
	DrawStamp(&Stamp);

	{
		int16_t crew_height {}, energy_height {};

		if (!IsHd())
		{
			crew_height = ((uqstl::min<uint16_t>(SIPtr->max_crew, MaxCrewSize) + 1) & ~1) + 1;
		}
		else
		{
			crew_height = ((uqstl::min<uint16_t>(SIPtr->max_crew, MaxCrewSize) + 1) * RES_SCALE(1));
		}
		energy_height = (((SIPtr->max_energy + 1) >> 1) << RES_TRP(1)) + RES_SCALE(1);

		if (!IsHd())
		{
			// Dark gray line on the bottom of crew box
			SetContextForeGroundColor(
				BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F));
			r.corner.x = CrewXoffs() - RES_SCALE(1);
			r.corner.y = GaugeYoffs() + RES_SCALE(1) + y;
			r.extent.width = StatWidth() + RES_SCALE(2);
			r.extent.height = RES_SCALE(1);
			DrawFilledRectangle(&r);
			// Dark gray line on the right of crew box
			r.corner.x = EnergyXoffs() - RES_SCALE(1);
			DrawFilledRectangle(&r);
			// Dark gray line on the right of energy box
			r.corner.x = EnergyXoffs() + StatWidth();
			r.corner.y -= energy_height;
			r.extent.width = RES_SCALE(1);
			r.extent.height = energy_height;
			DrawFilledRectangle(&r);
			// Dark gray line on the right of crew box
			r.corner.x = CrewXoffs() + StatWidth();
			r.corner.y = (GaugeYoffs() + RES_SCALE(1) + y) - crew_height;
			r.extent.width = RES_SCALE(1);
			r.extent.height = crew_height;
			DrawFilledRectangle(&r);

			// Light gray line on the top of crew box
			SetContextForeGroundColor(
				BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19));
			r.corner.x = CrewXoffs() - RES_SCALE(1);
			r.corner.y = GaugeYoffs() - crew_height + y;
			r.extent.width = StatWidth() + RES_SCALE(2);
			r.extent.height = RES_SCALE(1);
			DrawFilledRectangle(&r);
			// Light gray line on the top of energy box
			r.corner.x = EnergyXoffs() - RES_SCALE(1);
			r.corner.y = GaugeYoffs() - energy_height + y;
			DrawFilledRectangle(&r);
			// Light gray line on the left of energy box
			r.extent.width = RES_SCALE(1);
			r.extent.height = energy_height + RES_SCALE(1);
			DrawFilledRectangle(&r);
			// Light gray line on the left of crew box
			r.corner.x = CrewXoffs() - RES_SCALE(1);
			r.corner.y = GaugeYoffs() - crew_height + y;
			r.extent.height = crew_height + RES_SCALE(1);
			DrawFilledRectangle(&r);

			SetContextForeGroundColor(BlackColor);

			// Black rectangle behind green crew boxes
			r.extent.width = StatWidth();
			r.corner.x = CrewXoffs();
			r.extent.height = crew_height;
			r.corner.y = y - r.extent.height + GaugeYoffs() + RES_SCALE(1);
			DrawFilledRectangle(&r);

			// Black rectangle behind red energy boxes
			r.extent.width = StatWidth();
			r.corner.x = EnergyXoffs();
			r.extent.height = energy_height;
			r.corner.y = y - r.extent.height + GaugeYoffs() + RES_SCALE(1);
			DrawFilledRectangle(&r);
		}
		else
		{
			r.corner.x = CrewXoffs() - 4;
			r.corner.y = GaugeYoffs() - crew_height + y;
			r.extent.width = StatWidth() + 8;
			r.extent.height = crew_height + 8;
			DrawRenderedBox(&r, true, BlackColor, THIN_INNER_BEVEL,
							false);

			r.corner.x = EnergyXoffs() - 4;
			r.corner.y = GaugeYoffs() - energy_height + y;
			r.extent.width = StatWidth() + 8;
			r.extent.height = energy_height + 8;
			DrawRenderedBox(&r, true, BlackColor, THIN_INNER_BEVEL,
							false);
		}
	}

	if (!StarShipPtr || StarShipPtr->captains_name_index)
	{ // Any regular ship. SIS and Sa-Matra are separate.
		// This includes Melee menu.
		DrawCrewFuelString(y, 0);

		DrawShipNameString(SIPtr->race_strings->text(1), (uint16_t)~0, y);

		{
			char buf[30] {};
			TEXT Text;

			FONT OldFont {SetContextFont(MENU_BOOL(TinyFontBold, TinyFont))};

			if (!StarShipPtr)
			{ // In Melee menu
				fmt::format_to_n(buf, sizeof(buf) - 1, "{}", SIPtr->ship_cost);
				Text.pStr = buf;
				Text.CharCount = (uint16_t)~0;
			}
			else
			{
				uqm::utf8StringCopy(buf, sizeof(buf),
									SIPtr->race_strings->text(StarShipPtr->captains_name_index));

				if (is3DO(uqm::UQMOptions::read().whichMenu))
				{
					replaceChar(buf, UnicharSpace, UnicharTab);
				}

				Text.pStr = buf;
				Text.CharCount = (uint16_t)uqm::utf8StringCount(buf);
			}
			Text.align = ALIGN_CENTER;

			Text.baseline.x = StatusWidth() >> 1;
			Text.baseline.y = y + GaugeYoffs() + RES_SCALE(3)
							+ RES_SCALE(MENU_BOOL(6, 0));

			SetContextForeGroundColor(BlackColor);
			font_DrawText(&Text);

			SetContextFont(OldFont);
		}
	}
	else if (StarShipPtr->playerNr == RpgPlayerNum)
	{ // This is SIS
		DrawCrewFuelString(y, 0);
		DrawShipNameString(GLOBAL_SIS(ShipName), (uint16_t)~0, y);
	}

	{
		int16_t crew_delta {};

		crew_delta = SIPtr->crew_level;
		const int16_t energy_delta {SIPtr->energy_level};
		// DeltaStatistics() below will add specified values to these
		SIPtr->crew_level = 0;
		SIPtr->energy_level = 0;
		DeltaStatistics(SIPtr, y, crew_delta, energy_delta);
	}

	UnbatchGraphics();

	if (pClipRect)
	{
		SetContextOrigin(oldOrigin);
		SetContextClipRect(&oldClipRect);
	}

	SetContext(OldContext);
}

static constexpr auto CrewUnitColor {BuildColor(MakeRgb15(0x00, 0x14, 0x00), 0x02)};

static constexpr auto RobotUnitColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08)};
static constexpr auto PlayerUnitColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)};
static constexpr auto FuelUnitColor {BuildColor(MakeRgb15(0x14, 0x00, 0x00), 0x04)};

// Pre: -crew_delta <= ShipInfoPtr->crew_level
//      crew_delta <= ShipInfoPtr->max_crew - ShipInfoPtr->crew_level
void DeltaStatistics(SHIP_INFO* ShipInfoPtr, int16_t y_offs,
					 int16_t crew_delta, int16_t energy_delta)
{
	int16_t y {};
	GFXRECT r;

	if (crew_delta == 0 && energy_delta == 0)
	{
		return;
	}

	const int16_t x {0};
	// Y coordinates for the crew and energy rectangles
	y = GaugeYoffs() + y_offs;

	r.extent.width = UnitWidth();
	r.extent.height = UnitHeight();

	if (crew_delta != 0)
	{
		uint16_t oldNumBlocks {};
		uint16_t newNumBlocks {};
		uint16_t blockI {};
		uint16_t newCrewLevel {};

		oldNumBlocks = uqstl::min<uint16_t>(ShipInfoPtr->crew_level, MaxCrewSize);
		newCrewLevel = ShipInfoPtr->crew_level + crew_delta;
		newNumBlocks = uqstl::min<uint16_t>(newCrewLevel, MaxCrewSize);

		if (crew_delta > 0)
		{
			r.corner.y = (y + RES_SCALE(1)) - (((oldNumBlocks + 1) >> 1) * (UnitHeight() + RES_SCALE(1)));
			SetContextForeGroundColor(
				(ShipInfoPtr->ship_flags & CrewImmune) ?
					RobotUnitColor :
					CrewUnitColor);
			for (blockI = oldNumBlocks; blockI < newNumBlocks; blockI++)
			{
				r.corner.x = x + (CrewXoffs() + RES_SCALE(1));
				if (!(blockI & 1))
				{
					r.corner.x += UnitWidth() + RES_SCALE(1);
					r.corner.y -= UnitHeight() + RES_SCALE(1);
				}
				DrawFilledRectangle(&r);
			}
		}
		else /* crew_delta < 0 */
		{
			SetContextForeGroundColor(BlackColor);
			r.corner.y = (y + RES_SCALE(1)) - (((oldNumBlocks + 2) >> 1) * (UnitHeight() + RES_SCALE(1)));
			for (blockI = oldNumBlocks; blockI > newNumBlocks; blockI--)
			{
				r.corner.x = x + (CrewXoffs() + RES_SCALE(1) + UnitWidth() + RES_SCALE(1));
				if (!(blockI & 1))
				{
					r.corner.x -= UnitWidth() + RES_SCALE(1);
					r.corner.y += UnitHeight() + RES_SCALE(1);
				}
				DrawFilledRectangle(&r);
			}
		}

		if (ShipInfoPtr->ship_flags & PlayerCaptain)
		{
			if (((ShipInfoPtr->crew_level > MaxCrewSize) != (newCrewLevel > MaxCrewSize) || ShipInfoPtr->crew_level == 0) && newCrewLevel != 0)
			{
				// The block indicating the captain needs to change color.
				SetContextForeGroundColor(
					(newCrewLevel > MaxCrewSize) ?
						CrewUnitColor :
						PlayerUnitColor);
				r.corner.x = x + (CrewXoffs() + RES_SCALE(1)) + (UnitWidth() + RES_SCALE(1));
				r.corner.y = y - UnitHeight();
				DrawFilledRectangle(&r);
			}
		}

		ShipInfoPtr->crew_level = newCrewLevel;
		if (ShipInfoPtr->max_crew > MaxCrewSize || ShipInfoPtr->ship_flags & PlayerCaptain)
		{
			// All crew doesn't fit in the graphics; print a number.
			// Always print a number for the SIS in the full game.
			y_offs += SafeX();
			DrawBattleCrewAmount(ShipInfoPtr, y_offs);
		}
	}

	if (energy_delta != 0)
	{
		if (energy_delta > 0)
		{
			SetContextForeGroundColor(FuelUnitColor);
			r.corner.y = (y + RES_SCALE(1)) - (((ShipInfoPtr->energy_level + 1) >> 1) * (UnitHeight() + RES_SCALE(1)));
			do
			{
				r.corner.x = x + (EnergyXoffs() + RES_SCALE(1));
				if (!(ShipInfoPtr->energy_level & 1))
				{
					r.corner.x += UnitWidth() + RES_SCALE(1);
					r.corner.y -= UnitHeight() + RES_SCALE(1);
				}
				DrawFilledRectangle(&r);
				++ShipInfoPtr->energy_level;
			} while (--energy_delta);
		}
		else
		{
			SetContextForeGroundColor(BlackColor);
			r.corner.y = (y + RES_SCALE(1)) - (((ShipInfoPtr->energy_level + 2) >> 1) * (UnitHeight() + RES_SCALE(1)));
			do
			{
				r.corner.x = x + (EnergyXoffs() + RES_SCALE(1) + UnitWidth() + RES_SCALE(1));
				if (!(ShipInfoPtr->energy_level & 1))
				{
					r.corner.x -= UnitWidth() + RES_SCALE(1);
					r.corner.y += UnitHeight() + RES_SCALE(1);
				}
				DrawFilledRectangle(&r);
				--ShipInfoPtr->energy_level;
			} while (++energy_delta);
		}
	}
}
