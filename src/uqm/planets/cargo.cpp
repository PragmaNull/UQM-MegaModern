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

#include "../colors.h"
#include "../controls.h"
#include "../gamestr.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../sounds.h"
#include "../util.h"
#include "../sis.h"
// for ClearSISRect(), DrawStatusMessage()
#include "planets.h"
#include "libs/graphics/drawable.h"
// for GetFrameBounds()

#include "core/string/StringUtils.h"


static auto ElementOrgY() { return RES_SCALE(35); }
static auto BioOrgY() { return RES_SCALE(119); }
static auto ElementSpacingY() { return RES_SCALE(9); }
static auto FreeOrgY() { return (ElementOrgY() + (NUM_ELEMENT_CATEGORIES * ElementSpacingY())); }

static auto ElementCol0() { return RES_SCALE(7); }
static auto ElementCol1() { return RES_SCALE(32); }
static auto ElementCol2() { return RES_SCALE(58); }

static auto ElementSelOrgX() { return (ElementCol0() + RES_SCALE(7 + 5)); }
static auto ElementSelWidth() { return (ElementCol2() - ElementSelOrgX() + RES_SCALE(1)); }

static auto TextBaseline() { return RES_SCALE(6); }


void ShowRemainingCapacity(void)
{
	GFXRECT r;
	TEXT t;
	char buf[40];

	GFXCONTEXT OldContext {SetContext(StatusContext)};
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	r.corner.x = RES_SCALE(40);
	r.corner.y = FreeOrgY();

	fmt::format_to_sz_n(buf, "{}",
						GetStorageBayCapacity() - GLOBAL_SIS(TotalElementMass));
	t.baseline.x = ElementCol2() + RES_SCALE(1);
	t.baseline.y = r.corner.y + TextBaseline();
	t.align = ALIGN_RIGHT;
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;

	r.extent.width = t.baseline.x - r.corner.x + RES_SCALE(1);
	r.extent.height = ElementSpacingY() - RES_SCALE(2);

	BatchGraphics();
	// erase previous free amount
	SetContextForeGroundColor(CargoBackColor);
	DrawFilledRectangle(&r);
	// print the new free amount
	SetContextForeGroundColor(CargoWorthColor);
	font_DrawText(&t);
	UnbatchGraphics();

	SetContext(OldContext);
}

static void
DrawElementAmount(uint16_t element, bool selected)
{
	GFXRECT r;
	TEXT t;
	char buf[40];

	r.corner.x = ElementSelOrgX();
	r.extent.width = ElementSelWidth();
	r.extent.height = ElementSpacingY() - RES_SCALE(2);

	if (element == NUM_ELEMENT_CATEGORIES)
	{
		r.corner.y = BioOrgY();
	}
	else
	{
		r.corner.y = ElementOrgY() + (element * ElementSpacingY());
	}

	// draw line background
	SetContextForeGroundColor(selected ?
								  CargoSelectedBackColor :
								  CargoBackColor);
	DrawFilledRectangle(&r);

	t.align = ALIGN_RIGHT;
	t.pStr = buf;
	t.baseline.y = r.corner.y + TextBaseline();

	if (element == NUM_ELEMENT_CATEGORIES)
	{ // Bio
		fmt::format_to_sz_n(buf, "{}", GLOBAL_SIS(TotalBioMass));
	}
	else
	{ // Element
		// print element's worth
		SetContextForeGroundColor(selected ?
									  CargoSelectedWorthColor :
									  CargoWorthColor);
		t.baseline.x = ElementCol1();
		fmt::format_to_sz_n(buf, "{}", GLOBAL(ElementWorth[element]));
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);

		fmt::format_to_sz_n(buf, "{}", GLOBAL_SIS(ElementAmounts[element]));
	}

	// print the element/bio amount
	SetContextForeGroundColor(selected ?
								  CargoSelectedAmountColor :
								  CargoAmountColor);
	t.baseline.x = ElementCol2();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
}

static void
DrawCargoDisplay(void)
{
	STAMP s;
	TEXT t;
	GFXRECT r;
	int16_t cy {};

	r.corner.x = RES_SCALE(2);
	r.extent.width = FieldWidth() + RES_SCALE(1);
	r.corner.y = RES_SCALE(20);
	// XXX: Shouldn't the height be 1 less? This draws the bottom border
	//   1 pixel too low. Or if not, why do we need another box anyway?
	r.extent.height = (RES_SCALE(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, RES_SCALE(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, CargoBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(DEVICE_CARGO_FRAME);
	}

	// draw the "CARGO" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - RES_SCALE(1);
	t.baseline.y = RES_SCALE(27);
	t.align = ALIGN_CENTER;
	t.pStr = GAME_STRING(CARGO_STRING_BASE);
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(CargoSelectedAmountColor);
	font_DrawText(&t);

	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	s.frame = SetAbsFrameIndex(MiscDataFrame,
							   (NumScandotTransitions * 2) + 3);
	r.corner.x = ElementCol0();
	r.extent = GetFrameBounds(s.frame);
	s.origin.x = r.corner.x + RES_RECENTER(r.extent.height);

	cy = ElementOrgY();

	// print element column headings
	t.align = ALIGN_RIGHT;
	t.baseline.y = cy - RES_SCALE(1);
	t.CharCount = (uint16_t)~0;

	SetContextForeGroundColor(CargoWorthColor);
	t.baseline.x = ElementCol1();
	t.pStr = "$";
	font_DrawText(&t);

	t.baseline.x = ElementCol2();
	t.pStr = "#";
	font_DrawText(&t);

	// draw element icons and print amounts
	for (uint16_t i {0}; i < NUM_ELEMENT_CATEGORIES; ++i, cy += ElementSpacingY())
	{
		// erase background under an element icon
		SetContextForeGroundColor(BlackColor);
		r.corner.y = cy;
		DrawFilledRectangle(&r);

		// draw an element icon
		s.origin.y = r.corner.y + RES_RECENTER(r.extent.height);
		DrawStamp(&s);
		s.frame = SetRelFrameIndex(s.frame, 5);

		DrawElementAmount(i, false);
	}

	// erase background under the Bio icon
	SetContextForeGroundColor(BlackColor);
	r.corner.y = BioOrgY();
	DrawFilledRectangle(&r);

	// draw the Bio icon
	s.origin.y = r.corner.y + RES_RECENTER(r.extent.height);
	s.frame = SetAbsFrameIndex(s.frame, 68);
	DrawStamp(&s);

	// print the Bio amount
	DrawElementAmount(NUM_ELEMENT_CATEGORIES, false);

	// draw the line over the Bio amount
	r.corner.x = RES_SCALE(4);
	r.corner.y = BioOrgY() - RES_SCALE(2);
	r.extent.width = FieldWidth() - RES_SCALE(3);
	r.extent.height = RES_SCALE(1);
	SetContextForeGroundColor(CargoSelectedBackColor);
	DrawFilledRectangle(&r);

	// print "Free"
	t.baseline.x = RES_SCALE(5);
	t.baseline.y = FreeOrgY() + TextBaseline();
	t.align = ALIGN_LEFT;
	t.pStr = GAME_STRING(CARGO_STRING_BASE + 1);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);

	ShowRemainingCapacity();
}

void DrawCargoStrings(uint8_t OldElement, uint8_t NewElement)
{
	GFXCONTEXT OldContext {SetContext(StatusContext)};
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	BatchGraphics();

	if (OldElement > NUM_ELEMENT_CATEGORIES)
	{ // Asked for the initial display
		DrawCargoDisplay();

		// do not draw unselected again this time
		OldElement = NewElement;
	}

	if (OldElement != NewElement)
	{ // unselect the previous element
		DrawElementAmount(OldElement, false);
	}

	if (NewElement != (uint8_t)~0)
	{ // select the new element
		DrawElementAmount(NewElement, true);
	}

	UnbatchGraphics();
	SetContext(OldContext);
}

static void
DrawElementDescription(uint16_t element)
{
	DrawStatusMessage(GAME_STRING(element + (CARGO_STRING_BASE + 2)));
}

void DrawRainbowPlanet(uint16_t planet)
{
	STAMP s;
	TEXT t;
	GFXRECT r;
	char buf[40];

	GFXCONTEXT OldContext {SetContext(StatusContext)};

	r.corner.x = RES_SCALE(2);
	r.extent.width = FieldWidth() + RES_SCALE(1);
	r.corner.y = RES_SCALE(20);
	r.extent.height = (RES_SCALE(129) - r.corner.y);

	BatchGraphics();

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, RES_SCALE(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, CargoBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(DEVICE_CARGO_FRAME);
	}

	// draw the "DATALOG" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - RES_SCALE(1);
	t.baseline.y = RES_SCALE(27);
	t.align = ALIGN_CENTER;
	t.pStr = GAME_STRING(CARGO_STRING_BASE + 10); // datalog
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(CargoSelectedAmountColor);
	font_DrawText(&t);

	// rainbow world icon
	s.origin.x = RES_SCALE(1);
	s.origin.y = RES_SCALE(27);
	s.frame = SetAbsFrameIndex(MiscDataFrame, 109);
	DrawStamp(&s);

	// number of worlds
	SetContextForeGroundColor(planet != 0 ?
								  PcmenuSelectionBackgroundColor :
								  CargoBackColor);
	r.corner.x = (StatusWidth() >> 1) - RES_SCALE(6);
	r.extent.width = RES_SCALE(12);
	r.extent.height = ElementSpacingY() - RES_SCALE(2);
	r.corner.y = s.origin.y - s.frame->HotSpot.y + s.frame->Bounds.height
			   + RES_SCALE(3);
	DrawFilledRectangle(&r);
	t.pStr = buf;
	fmt::format_to_sz_n(buf, "{}", planet);
	t.CharCount = (uint16_t)~0;
	t.baseline.x = (StatusWidth() >> 1);
	t.baseline.y = r.corner.y + TextBaseline();
	SetContextForeGroundColor(planet != 0 ?
								  PcmenuSelectionTextColor :
								  PcmenuTextColor);
	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}
	font_DrawText(&t);

	// decoration text rect
	r.corner.x = (StatusWidth() >> 1) - RES_SCALE(20);
	r.extent.width = RES_SCALE(40);
	r.extent.height *= 2;
	r.corner.y += RES_SCALE(22);
	SetContextForeGroundColor(CargoBackColor);
	DrawFilledRectangle(&r);

	// decoration text
	if (planet == 0)
	{
		t.pStr = GAME_STRING(CARGO_STRING_BASE + 11); // complete
		t.CharCount = (uint16_t)~0;
		t.baseline.y += RES_SCALE(25);
		SetContextForeGroundColor(PcmenuTextColor);
		font_DrawText(&t);
	}
	else
	{
		t.pStr = GAME_STRING(CARGO_STRING_BASE + 12); // Uploading
		t.CharCount = (uint16_t)~0;
		t.baseline.y += RES_SCALE(22);
		SetContextForeGroundColor(PcmenuTextColor);
		font_DrawText(&t);
		t.pStr = GAME_STRING(CARGO_STRING_BASE + 13); // data
		t.CharCount = (uint16_t)~0;
		t.baseline.y += RES_SCALE(7);
		font_DrawText(&t);
	}

	UnbatchGraphics();
	SetContext(OldContext);
}

static bool
DoDiscardCargo(MENU_STATE* pMS)
{
	uint8_t NewState {};
	bool select {};
	bool cancel {};

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	const bool back {PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_LEFT]};
	const bool forward {PulsedInputState.menu[KEY_MENU_DOWN] || PulsedInputState.menu[KEY_MENU_RIGHT]};

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		return false;
	}

	if (cancel)
	{
		return false;
	}
	else if (select)
	{
		if (GLOBAL_SIS(ElementAmounts[pMS->CurState]))
		{
			--GLOBAL_SIS(ElementAmounts[pMS->CurState]);
			DrawCargoStrings(pMS->CurState, pMS->CurState);

			--GLOBAL_SIS(TotalElementMass);
			ShowRemainingCapacity();
		}
		else
		{ // no element left in cargo hold
			PlayMenuSound(MENU_SOUND_FAILURE);
		}
	}
	else
	{
		NewState = pMS->CurState;
		if (back)
		{
			if (NewState == 0)
			{
				NewState += NUM_ELEMENT_CATEGORIES;
			}
			--NewState;
		}
		else if (forward)
		{
			++NewState;
			if (NewState == NUM_ELEMENT_CATEGORIES)
			{
				NewState = 0;
			}
		}

		if (NewState != pMS->CurState)
		{
			DrawCargoStrings(pMS->CurState, NewState);
			DrawElementDescription(NewState);
			pMS->CurState = NewState;
		}
	}

	return (true);
}

void CargoMenu(void)
{
	MENU_STATE MenuState;

	memset(&MenuState, 0, sizeof MenuState);

	// draw the initial cargo display
	DrawCargoStrings((uint8_t)~0, MenuState.CurState);
	DrawElementDescription(MenuState.CurState);

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		DrawMenuStateStrings(PM_ALT_CARGO, 0);
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	MenuState.InputFunc = DoDiscardCargo;
	DoInput(&MenuState, true);

	// erase the cargo display
	ClearSISRect(DrawSisDisplay);
}
