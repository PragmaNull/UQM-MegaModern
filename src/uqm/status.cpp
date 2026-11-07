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

#include "status.h"
#include "colors.h"
#include "globdata.h"
#include "races.h"
#include "ship.h"
#include "setup.h"
#include "options.h"
#include "init.h"
// for NumPlayers
#include "menustat.h"
#include "util.h"
#include "intel.h"

#include <stdio.h>
#include <string.h>


int16_t status_y_offsets[NumPlayers];


void InitStatusOffsets(void)
{
	// XXX: We have to jump through these hoops because GoodGuyYoffs() is
	//   not a constant, contrary to what its name suggests.
	status_y_offsets[0] = GoodGuyYoffs(); // bottom player
	status_y_offsets[1] = BadGuyYoffs;	  // top player
}

bool animIsInTransition(CaptainGraphics* CSPtr)
{
	return (CSPtr->thrust_offset & 1) || (CSPtr->weapon_offset & 1) || (CSPtr->special_offset & 1) || (CSPtr->tl_offset & 1) || (CSPtr->tr_offset & 1);
}

static uint8_t CalculateAnimOffsets(CaptainGraphics* CSPtr,
									STATUS_FLAGS delta_status_flags, STATUS_FLAGS cur_status_flags)
{
	uint8_t redraw_flag = 0;

	if (delta_status_flags & LeftFlag || CSPtr->tl_offset & 1)
	{
		if (!(delta_status_flags & RightFlag))
		{
			CSPtr->tr_offset = 0;

			if (cur_status_flags & LeftFlag)
			{
				CSPtr->tl_offset++;
			}
			else
			{
				CSPtr->tl_offset--;
			}

			redraw_flag |= LeftFlag;
		}
		else if (cur_status_flags & RightFlag)
		{
			CSPtr->tl_offset = 0;
			CSPtr->tr_offset = 1;

			redraw_flag |= RightFlag;
		}
		else
		{
			CSPtr->tr_offset = 0;
			CSPtr->tl_offset = 1;

			redraw_flag |= LeftFlag;
		}
	}
	else if (delta_status_flags & RightFlag || CSPtr->tr_offset & 1)
	{
		CSPtr->tl_offset = 0;

		if (cur_status_flags & RightFlag)
		{
			CSPtr->tr_offset++;
		}
		else
		{
			CSPtr->tr_offset--;
		}

		redraw_flag |= RightFlag;
	}

	if (delta_status_flags & ThrustFlag || CSPtr->thrust_offset & 1)
	{
		if (cur_status_flags & ThrustFlag)
		{
			CSPtr->thrust_offset++;
		}
		else
		{
			CSPtr->thrust_offset--;
		}

		redraw_flag |= ThrustFlag;
	}

	if (delta_status_flags & WeaponFlag || CSPtr->weapon_offset & 1)
	{
		if (cur_status_flags & WeaponFlag)
		{
			CSPtr->weapon_offset++;
		}
		else
		{
			CSPtr->weapon_offset--;
		}

		redraw_flag |= WeaponFlag;
	}

	if (delta_status_flags & SpecialFlag || CSPtr->special_offset & 1)
	{
		if (cur_status_flags & SpecialFlag)
		{
			CSPtr->special_offset++;
		}
		else
		{
			CSPtr->special_offset--;
		}

		redraw_flag |= SpecialFlag;
	}

	return redraw_flag;
}

static void DrawCaptainWindowAnimation(CaptainGraphics* CSPtr, int16_t y,
									   uint8_t redraw_flag)
{
	GfxStamp Stamp;

	Stamp.origin.x = CaptainXoffs();
	Stamp.origin.y = y + CaptainYoffs();

	if (redraw_flag & LeftFlag)
	{
		Stamp.frame = SetRelFrameIndex(CSPtr->turn, 1);
		DrawStamp(&Stamp);
		Stamp.frame = IncFrameIndex(Stamp.frame);
		DrawStamp(&Stamp);

		for (uint8_t i {0}; i < CSPtr->tl_offset; i++)
		{
			Stamp.frame = IncFrameIndex(Stamp.frame);
			DrawStamp(&Stamp);
		}
	}
	else if (redraw_flag & RightFlag)
	{
		Stamp.frame = SetRelFrameIndex(CSPtr->turn, 3);
		DrawStamp(&Stamp);
		Stamp.frame = DecFrameIndex(Stamp.frame);
		DrawStamp(&Stamp);

		for (uint8_t i {0}; i < CSPtr->tr_offset; i++)
		{
			Stamp.frame = DecFrameIndex(Stamp.frame);
			DrawStamp(&Stamp);
		}
	}

	if (redraw_flag & ThrustFlag)
	{
		Stamp.frame = SetRelFrameIndex(CSPtr->thrust, CSPtr->thrust_offset);
		DrawStamp(&Stamp);
	}

	if (redraw_flag & WeaponFlag)
	{
		Stamp.frame = SetRelFrameIndex(CSPtr->weapon, CSPtr->weapon_offset);
		DrawStamp(&Stamp);
	}

	if (redraw_flag & SpecialFlag)
	{
		Stamp.frame = SetRelFrameIndex(CSPtr->special, CSPtr->special_offset);
		DrawStamp(&Stamp);
	}
}

static void DrawCaptainWindowFrame(FRAME fr, int16_t y)
{
	GfxStamp Stamp;

	Stamp.origin.x = CaptainXoffs();
	Stamp.origin.y = y + CaptainYoffs();

	Stamp.frame = fr;

	DrawStamp(&Stamp);
}

/*static void
CaptainsWindow (CaptainGraphics *CSPtr, int16_t y,
		STATUS_FLAGS delta_status_flags, STATUS_FLAGS cur_status_flags,
		uint16_t Pass)
{// Kruzen: og method. now redundant
	GfxStamp Stamp;

	Stamp.origin.x = CaptainXoffs();
	Stamp.origin.y = y + CaptainYoffs();

	if (delta_status_flags & LeftFlag)
	{
		Stamp.frame = CSPtr->turn;
		if (!(delta_status_flags & RightFlag))
		{
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 3);
			if (Pass == 2)
			{
				if (cur_status_flags & LeftFlag)
					Stamp.frame = IncFrameIndex (Stamp.frame);
				else
					Stamp.frame = DecFrameIndex (Stamp.frame);
			}
		}
		else if (cur_status_flags & RightFlag)
		{
			if (Pass == 1)
				Stamp.frame = SetRelFrameIndex (Stamp.frame, 3);
			else
				Stamp.frame = IncFrameIndex (Stamp.frame);
			DrawStamp (&Stamp);
			Stamp.frame = DecFrameIndex (Stamp.frame);
		}
		else
		{
			if (Pass == 1)
				Stamp.frame = IncFrameIndex (Stamp.frame);
			else
				Stamp.frame = SetRelFrameIndex (Stamp.frame, 3);
			DrawStamp (&Stamp);
			Stamp.frame = IncFrameIndex (Stamp.frame);
		}
		DrawStamp (&Stamp);
	}
	else if (delta_status_flags & RightFlag)
	{
		Stamp.frame = CSPtr->turn;
		Stamp.frame = IncFrameIndex (Stamp.frame);
		if (Pass == 2)
		{
			if (cur_status_flags & RightFlag)
				Stamp.frame = DecFrameIndex (Stamp.frame);
			else
				Stamp.frame = IncFrameIndex (Stamp.frame);
		}
		DrawStamp (&Stamp);
	}

	if (delta_status_flags & ThrustFlag)
	{
		Stamp.frame = CSPtr->thrust;
		if (Pass == 1)
			Stamp.frame = IncFrameIndex (Stamp.frame);
		else if (cur_status_flags & ThrustFlag)
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 2);
		DrawStamp (&Stamp);
	}
	if (delta_status_flags & WeaponFlag)
	{
		Stamp.frame = CSPtr->weapon;
		if (Pass == 1)
			Stamp.frame = IncFrameIndex (Stamp.frame);
		else if (cur_status_flags & WeaponFlag)
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 2);
		DrawStamp (&Stamp);
	}
	if (delta_status_flags & SpecialFlag)
	{
		Stamp.frame = CSPtr->special;
		if (Pass == 1)
			Stamp.frame = IncFrameIndex (Stamp.frame);
		else if (cur_status_flags & SpecialFlag)
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 2);
		DrawStamp (&Stamp);
	}
}*/

static constexpr auto MaxCrewDigits {3};

void DrawBattleCrewAmount(ShipInfoDesc* ShipInfoPtr, int16_t y_offs)
{
	GfxRect r;
	GfxText t;
	char buf[40] {};

	t.baseline.x = BattleCrewX() + ResScale(MenuBool(2, 4));
	t.baseline.y = BattleCrewY() + y_offs;
	t.align = MenuBool(TextAlign::Left, TextAlign::Center);
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	r.corner.x = t.baseline.x - ResScale(MenuBool(2, 9));
	r.corner.y = t.baseline.y - ResScale(5);
	r.extent.width = ResScale(6 * MaxCrewDigits + 6);
	r.extent.height = ResScale(5);

	fmt::format_to_n(buf, sizeof(buf) - 1, "{}", ShipInfoPtr->crew_level);
	SetContextFont(StarConFont);

	SetContextForeGroundColor(MenuForegroundColor);
	DrawFilledRectangle(&r);
	SetContextForeGroundColor(BlackColor);
	font_DrawText(&t);
}

void DrawCaptainsWindow(Starship* StarShipPtr)
{
	int16_t y {};
	GfxRect r;
	GfxStamp s;

	RaceDesc* RDPtr {StarShipPtr->RaceDescPtr};
	FRAME Frame {RDPtr->ship_data.captain_control.background};
	if (Frame)
	{
		Frame = SetAbsFrameIndex(Frame, 0);
		RDPtr->ship_data.captain_control.background = Frame;
		Frame = SetRelFrameIndex(Frame, 1);
		RDPtr->ship_data.captain_control.turn = Frame;
		Frame = SetRelFrameIndex(Frame, 5);
		RDPtr->ship_data.captain_control.thrust = Frame;
		Frame = SetRelFrameIndex(Frame, 3);
		RDPtr->ship_data.captain_control.weapon = Frame;
		Frame = SetRelFrameIndex(Frame, 3);
		RDPtr->ship_data.captain_control.special = Frame;
	}

	BatchGraphics();

	// Grey area under and around captain's window.
	assert(StarShipPtr->playerNr >= 0);
	int16_t y_offs {status_y_offsets[StarShipPtr->playerNr]};

	r.corner.x = CaptainXoffs() - ResScale(2);
	r.corner.y = y_offs + ShipInfoHeight();
	r.extent.width = StatusWidth() - CaptainXoffs() + IfHd(4);
	r.extent.height = ShipStatusHeight() - CaptainYoffs()
					+ ResScale(MenuBool(NdosNum(2), 1));
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08));
	DrawFilledRectangle(&r);

	// Left border of the status panel.
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F));
	r.corner.x = ResScale(1);
	r.corner.y = y_offs + ShipInfoHeight();
	r.extent.width = ResScale(1);
	r.extent.height = (ShipStatusHeight() - ShipInfoHeight() - ResScale(2));
	DrawFilledRectangle(&r);
	r.corner.x = 0;
	r.extent.height += ResScale(1);
	DrawFilledRectangle(&r);

	// Lower and right border of the status panel.
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19));
	r.corner.x = StatusWidth() - ResScale(1);
	r.corner.y = y_offs + ShipInfoHeight();
	r.extent.width = ResScale(1);
	r.extent.height = ShipStatusHeight() - ShipInfoHeight();
	DrawFilledRectangle(&r);
	r.corner.x = StatusWidth() - ResScale(2);
	DrawFilledRectangle(&r);
	r.corner.x = ResScale(1);
	r.extent.width = StatusWidth() - ResScale(2);
	r.corner.y = y_offs + (ShipStatusHeight() - ResScale(2));
	r.extent.height = ResScale(1);
	DrawFilledRectangle(&r);
	r.corner.x = 0;
	r.extent.width += ResScale(1);
	r.corner.y += ResScale(1);
	DrawFilledRectangle(&r);

	y = y_offs + CaptainYoffs();

	if (!IsHd())
	{
		// Actual Captain's Window
		// Darker grey rectangle at bottom and right of captain's window
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F));
		r.corner.x = CaptainWidth() + CaptainXoffs();
		r.corner.y = y;
		r.extent.width = ResScale(1);
		r.extent.height = CaptainHeight();
		DrawFilledRectangle(&r);
		r.corner.x = CaptainXoffs() - ResScale(1);
		r.corner.y += CaptainHeight();
		r.extent.width = CaptainWidth() + ResScale(2);
		r.extent.height = ResScale(1);
		DrawFilledRectangle(&r);

		// Light grey rectangle at top and left of captains window
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19));
		r.corner.x = CaptainXoffs() - ResScale(1);
		r.extent.width = CaptainWidth() + ResScale(2);
		r.corner.y = y - ResScale(1);
		r.extent.height = ResScale(1);
		DrawFilledRectangle(&r);
		r.corner.x = CaptainXoffs() - ResScale(1);
		r.extent.width = ResScale(1);
		r.corner.y = y;
		r.extent.height = CaptainHeight();
		DrawFilledRectangle(&r);
	}
	else
	{
		r.corner.x = CaptainXoffs() - 4;
		r.corner.y = y - 4;
		r.extent.width = CaptainWidth() + 8;
		r.extent.height = CaptainHeight() + 8;
		DrawRenderedBox(&r, true, BlackColor, BevelLevel::ThinInnerBevel, false);
	}

	s.frame = RDPtr->ship_data.captain_control.background;
	s.origin.x = CaptainXoffs();
	s.origin.y = y;
	DrawStamp(&s);

	if (StarShipPtr->captains_name_index == 0
		&& StarShipPtr->playerNr == RpgPlayerNum)
	{ // This is SIS
		GfxText t;

		t.baseline.x = StatusWidth() >> 1;
		t.baseline.y = y + ResScale(6);
		t.align = TextAlign::Center;
		t.pStr = uqm::gameState().sis().CommanderName;
		t.CharCount = (uint16_t)~0;
		SetContextForeGroundColor(
			BuildColor(MakeRgb15(0x00, 0x14, 0x00), 0x02));
		SetContextFont(MenuBool(TinyFontBold, TinyFont));
		font_DrawText(&t);
	}
	if (RDPtr->ship_info.max_crew > MaxCrewSize || RDPtr->ship_info.ship_flags & PlayerCaptain)
	{
		// All crew doesn't fit in the graphics; print a number.
		// Always print a number for the SIS in the full game.
		y_offs += SafeX();
		DrawBattleCrewAmount(&RDPtr->ship_info, y_offs);
	}

	UnbatchGraphics();
}

bool DeltaEnergy(Element* ElementPtr, int16_t energy_delta)
{
	Starship* StarShipPtr {};

	bool retval {true};

	if (antiCheat(ElementPtr, false, uqm::GodModeFlags::InfiniteBattery))
	{
		return retval;
	}

	GetElementStarShip(ElementPtr, &StarShipPtr);
	ShipInfoDesc* ShipInfoPtr {&StarShipPtr->RaceDescPtr->ship_info};
	if (energy_delta >= 0)
	{
		if ((uint8_t)(ShipInfoPtr->energy_level + (uint8_t)energy_delta) > ShipInfoPtr->max_energy)
		{
			energy_delta = ShipInfoPtr->max_energy
						 - ShipInfoPtr->energy_level;
		}
	}
	else
	{
		if ((uint8_t)-energy_delta > ShipInfoPtr->energy_level)
		{
			retval = false;
		}
	}

	if (!retval)
	{
		StarShipPtr->cur_status_flags |= LowOnEnergy;
	}
	else
	{
		StarShipPtr->cur_status_flags &= ~LowOnEnergy;
		StarShipPtr->energy_counter =
			StarShipPtr->RaceDescPtr->characteristics.energy_wait;

		DeltaStatistics(ShipInfoPtr, status_y_offsets[StarShipPtr->playerNr],
						0, energy_delta);
	}

	return (retval);
}

bool DeltaCrew(Element* ElementPtr, int16_t crew_delta)
{
	bool retval {};
	Starship* StarShipPtr {};
	ShipInfoDesc* ShipInfoPtr {};

	if (uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE
		&& ElementPtr->playerNr == NpcPlayerNum)
	{
		return (true); /* Samatra can't be crew-modified */
	}

	retval = true;
	GetElementStarShip(ElementPtr, &StarShipPtr);
	ShipInfoPtr = &StarShipPtr->RaceDescPtr->ship_info;
	if (crew_delta > 0)
	{
		ElementPtr->crew_level += crew_delta;
		if (ElementPtr->crew_level > ShipInfoPtr->max_crew)
		{
			crew_delta = ShipInfoPtr->max_crew - ShipInfoPtr->crew_level;
			ElementPtr->crew_level = ShipInfoPtr->max_crew;
		}
	}
	else if (crew_delta < 0)
	{
		if (ElementPtr->crew_level > (uint16_t)-crew_delta)
		{
			ElementPtr->crew_level += crew_delta;
		}
		else
		{
			crew_delta = -(int16_t)ElementPtr->crew_level;
			ElementPtr->crew_level = 0;
			retval = false;
		}
	}

	DeltaStatistics(ShipInfoPtr, status_y_offsets[StarShipPtr->playerNr],
					crew_delta, 0);

	return (retval);
}

void PreProcessStatus(Element* ShipPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->captains_name_index
		|| StarShipPtr->playerNr == RpgPlayerNum)
	{ // All except Sa-Matra, no captain's window there
		STATUS_FLAGS old_status_flags {};
		STATUS_FLAGS cur_status_flags {};
		uint8_t redraw_flags = 0;

		cur_status_flags = StarShipPtr->cur_status_flags;
		old_status_flags = StarShipPtr->old_status_flags;
		old_status_flags ^= cur_status_flags;

		CaptainGraphics* CSPtr {&StarShipPtr->RaceDescPtr->ship_data.captain_control};
		old_status_flags &= (LeftFlag | RightFlag | ThrustFlag | WeaponFlag | SpecialFlag);
		if (old_status_flags || animIsInTransition(CSPtr))
		{
			assert(StarShipPtr->playerNr >= 0);
			redraw_flags = CalculateAnimOffsets(CSPtr, old_status_flags, cur_status_flags);

			if (redraw_flags)
			{
				DrawCaptainWindowAnimation(CSPtr, status_y_offsets[StarShipPtr->playerNr], redraw_flags);
			}
		}
	}
}

void PostProcessStatus(Element* ShipPtr)
{
	Starship* StarShipPtr {};

	GetElementStarShip(ShipPtr, &StarShipPtr);
	if (StarShipPtr->captains_name_index
		|| StarShipPtr->playerNr == RpgPlayerNum)
	{ // All except Sa-Matra, no captain's window there
		STATUS_FLAGS cur_status_flags {};
		STATUS_FLAGS old_status_flags {};


		cur_status_flags = StarShipPtr->cur_status_flags;

		assert(StarShipPtr->playerNr >= 0);
		const int16_t y {status_y_offsets[StarShipPtr->playerNr]};

		if (ShipPtr->crew_level == 0)
		{
			StarShipPtr->cur_status_flags &=
				~(LeftFlag | RightFlag | ThrustFlag | WeaponFlag | SpecialFlag);

			if (StarShipPtr->RaceDescPtr->ship_info.crew_level == 0 && ShipPtr->state_flags & FiniteLife)
			{ // Kruzen: FiniteLife check was added in case if crew-level == 0
				// Is triggered by the ship itself (shofixti glory device) and
				// avoiding drawing black area
				uint8_t i {};
				uint8_t j {};
				Color c;
				GfxRect r;

				i = (uint8_t)(NumExplosionFrames * 3 - 1) - ShipPtr->life_span;

				if (cur_status_flags & ShofixtiExplosion && i == 0)
				{ // Special case: Instead of first orange rectangle we will draw the last
					// frame from captain avatars (used for shofixti glory device)
					const CaptainGraphics* CSPtr {&StarShipPtr->RaceDescPtr->ship_data.captain_control};

					cur_status_flags &= ~ShofixtiExplosion;
					DrawCaptainWindowFrame(DecFrameIndex(CSPtr->background), y);
				}
				else
				{
					if (i <= 4)
					{
						const static Color flash_tab0[] =
							{
								BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2D),
								BuildColor(MakeRgb15(0x1F, 0x19, 0x19), 0x24),
								BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7B),
								BuildColor(MakeRgb15(0x1F, 0x1C, 0x00), 0x78),
								BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F),
							};

						c = flash_tab0[i];
						r.corner.x = CaptainXoffs();
						r.corner.y = y + CaptainYoffs();
						r.extent.width = CaptainWidth();
						r.extent.height = CaptainHeight();
					}
					else
					{
						SetContextForeGroundColor(BlackColor);
						i -= 5;
						if (i <= 14)
						{
							const static Color flash_tab1[] =
								{
									BuildColor(MakeRgb15(0x1E, 0x1F, 0x12), 0x70),
									BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E),
									BuildColor(MakeRgb15(0x1F, 0x1F, 0x00), 0x71),
									BuildColor(MakeRgb15(0x1F, 0x1C, 0x00), 0x78),
									BuildColor(MakeRgb15(0x1F, 0x18, 0x00), 0x79),
									BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7A),
									BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7B),
									BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7C),
									BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7D),
									BuildColor(MakeRgb15(0x1F, 0x07, 0x00), 0x7E),
									BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7F),
									BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2A),
									BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
									BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C),
									BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2D),
								};

							c = flash_tab1[i];
							r.corner.x = CaptainXoffs() + ResScale(i);
							r.corner.y = y + CaptainYoffs() + ResScale(i);
							r.extent.width = CaptainWidth() - ResScale((i << 1));
							r.extent.height = CaptainHeight() - ResScale((i << 1));

							if (r.extent.height == ResScale(2))
							{
								r.extent.height += ResScale(1);
							}

							for (j = 0; j < ResScale(1); j++)
							{
								DrawRectangle(&r, false);
								++r.corner.x;
								++r.corner.y;
								r.extent.width -= 2;
								r.extent.height -= 2;
							}
						}
						else if ((i -= 15) <= 4)
						{
							r.corner.y = y + (CaptainYoffs() + ResScale(15));
							r.extent.width = ResScale(i + 1);
							r.extent.height = ResScale(1);
							switch (i)
							{
								case 0:
									r.corner.x = CaptainXoffs() + ResScale(15);
									i = CaptainWidth() - ResScale((15 + 1) << 1);
									c = BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C);
									break;
								case 1:
									r.corner.x = CaptainXoffs() + ResScale(16);
									i = CaptainWidth() - ResScale((17 + 1) << 1);
									c = BuildColor(MakeRgb15(0x07, 0x00, 0x00), 0x2F);
									break;
								case 2:
									r.corner.x = CaptainXoffs() + ResScale(18);
									i = CaptainWidth() - ResScale((20 + 1) << 1);
									c = BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2A);
									break;
								case 3:
									r.corner.x = CaptainXoffs() + ResScale(21);
									i = CaptainWidth() - ResScale((24 + 1) << 1);
									c = BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x29);
									break;
								case 4:
									r.corner.x = CaptainXoffs() + ResScale(25);
									i = ResScale(1);
									r.extent.width = ResScale(2);
									c = BuildColor(MakeRgb15(0x1F, 0x50, 0x05), 0x28);
									break;
								default:
									// Should not happen.
									c = UndefinedColor; // Keeping compiler quiet.
									break;
							}
							DrawFilledRectangle(&r);
							r.corner.x += i + r.extent.width;
							DrawFilledRectangle(&r);
							r.corner.x -= i;
							r.extent.width = i;
						}
						else
						{
							if ((i -= 5) > 2)
							{
								c = BlackColor;
							}
							else
							{
								const static Color flash_tab2[] =
									{
										BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
										BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2D),
										BuildColor(MakeRgb15(0x0B, 0x00, 0x00), 0x2E),
									};

								c = flash_tab2[i];
							}
							r.corner.x = CaptainXoffs()
									   + (CaptainWidth() >> 1) - IfHd(2);
							r.corner.y = y + CaptainYoffs()
									   + ((CaptainHeight() + ResScale(1)) >> 1) - IfHd(2);
							r.extent.width = ResScale(1);
							r.extent.height = ResScale(1);
						}
					}
					SetContextForeGroundColor(c);
					DrawFilledRectangle(&r);
				}
			}
		}

		old_status_flags = StarShipPtr->old_status_flags;
		old_status_flags = (old_status_flags ^ cur_status_flags) & (LeftFlag | RightFlag | ThrustFlag | WeaponFlag | SpecialFlag | LowOnEnergy);

		if (old_status_flags)
		{
			if (old_status_flags & LowOnEnergy)
			{
				if (!(cur_status_flags & LowOnEnergy))
				{
					DrawCrewFuelString(y, 1);
				}
				else
				{
					DrawCrewFuelString(y, -1);
				}
			}
		}

		StarShipPtr->old_status_flags = cur_status_flags;
	}
}
