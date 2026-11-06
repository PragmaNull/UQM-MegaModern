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


static auto ElementOrgY() { return ResScale(35); }
static auto BioOrgY() { return ResScale(119); }
static auto ElementSpacingY() { return ResScale(9); }
static auto FreeOrgY() { return (ElementOrgY() + (NUM_ELEMENT_CATEGORIES * ElementSpacingY())); }

static auto ElementCol0() { return ResScale(7); }
static auto ElementCol1() { return ResScale(32); }
static auto ElementCol2() { return ResScale(58); }

static auto ElementSelOrgX() { return (ElementCol0() + ResScale(7 + 5)); }
static auto ElementSelWidth() { return (ElementCol2() - ElementSelOrgX() + ResScale(1)); }

static auto TextBaseline() { return ResScale(6); }


void ShowRemainingCapacity(void)
{
	GfxRect r;
	GfxText t;
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

	r.corner.x = ResScale(40);
	r.corner.y = FreeOrgY();

	fmt::format_to_sz_n(buf, "{}",
						GetStorageBayCapacity() - uqm::gameState().sis().TotalElementMass);
	t.baseline.x = ElementCol2() + ResScale(1);
	t.baseline.y = r.corner.y + TextBaseline();
	t.align = TextAlign::Right;
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;

	r.extent.width = t.baseline.x - r.corner.x + ResScale(1);
	r.extent.height = ElementSpacingY() - ResScale(2);

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

static void DrawElementAmount(uint16_t Element, bool selected)
{
	GfxRect r;
	GfxText t;
	char buf[40];

	r.corner.x = ElementSelOrgX();
	r.extent.width = ElementSelWidth();
	r.extent.height = ElementSpacingY() - ResScale(2);

	if (Element == NUM_ELEMENT_CATEGORIES)
	{
		r.corner.y = BioOrgY();
	}
	else
	{
		r.corner.y = ElementOrgY() + (Element * ElementSpacingY());
	}

	// draw line background
	SetContextForeGroundColor(selected ?
								  CargoSelectedBackColor :
								  CargoBackColor);
	DrawFilledRectangle(&r);

	t.align = TextAlign::Right;
	t.pStr = buf;
	t.baseline.y = r.corner.y + TextBaseline();

	if (Element == NUM_ELEMENT_CATEGORIES)
	{ // Bio
		fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().TotalBioMass);
	}
	else
	{ // Element
		// print Element's worth
		SetContextForeGroundColor(selected ?
									  CargoSelectedWorthColor :
									  CargoWorthColor);
		t.baseline.x = ElementCol1();
		fmt::format_to_sz_n(buf, "{}", uqm::gameState().game().ElementWorth[Element]);
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);

		fmt::format_to_sz_n(buf, "{}", uqm::gameState().sis().ElementAmounts[Element]);
	}

	// print the Element/bio amount
	SetContextForeGroundColor(selected ?
								  CargoSelectedAmountColor :
								  CargoAmountColor);
	t.baseline.x = ElementCol2();
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
}

static void DrawCargoDisplay(void)
{
	GfxStamp s;
	GfxText t;
	GfxRect r;
	int16_t cy {};

	r.corner.x = ResScale(2);
	r.extent.width = FieldWidth() + ResScale(1);
	r.corner.y = ResScale(20);
	// XXX: Shouldn't the height be 1 less? This draws the bottom border
	//   1 pixel too low. Or if not, why do we need another box anyway?
	r.extent.height = (ResScale(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, ResScale(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, CargoBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(DEVICE_CARGO_FRAME);
	}

	// draw the "CARGO" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - ResScale(1);
	t.baseline.y = ResScale(27);
	t.align = TextAlign::Center;
	t.pStr = GameString(CARGO_STRING_BASE);
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
	s.origin.x = r.corner.x + ResRecenter(r.extent.height);

	cy = ElementOrgY();

	// print Element column headings
	t.align = TextAlign::Right;
	t.baseline.y = cy - ResScale(1);
	t.CharCount = (uint16_t)~0;

	SetContextForeGroundColor(CargoWorthColor);
	t.baseline.x = ElementCol1();
	t.pStr = "$";
	font_DrawText(&t);

	t.baseline.x = ElementCol2();
	t.pStr = "#";
	font_DrawText(&t);

	// draw Element icons and print amounts
	for (uint16_t i {0}; i < NUM_ELEMENT_CATEGORIES; ++i, cy += ElementSpacingY())
	{
		// erase background under an Element icon
		SetContextForeGroundColor(BlackColor);
		r.corner.y = cy;
		DrawFilledRectangle(&r);

		// draw an Element icon
		s.origin.y = r.corner.y + ResRecenter(r.extent.height);
		DrawStamp(&s);
		s.frame = SetRelFrameIndex(s.frame, 5);

		DrawElementAmount(i, false);
	}

	// erase background under the Bio icon
	SetContextForeGroundColor(BlackColor);
	r.corner.y = BioOrgY();
	DrawFilledRectangle(&r);

	// draw the Bio icon
	s.origin.y = r.corner.y + ResRecenter(r.extent.height);
	s.frame = SetAbsFrameIndex(s.frame, 68);
	DrawStamp(&s);

	// print the Bio amount
	DrawElementAmount(NUM_ELEMENT_CATEGORIES, false);

	// draw the line over the Bio amount
	r.corner.x = ResScale(4);
	r.corner.y = BioOrgY() - ResScale(2);
	r.extent.width = FieldWidth() - ResScale(3);
	r.extent.height = ResScale(1);
	SetContextForeGroundColor(CargoSelectedBackColor);
	DrawFilledRectangle(&r);

	// print "Free"
	t.baseline.x = ResScale(5);
	t.baseline.y = FreeOrgY() + TextBaseline();
	t.align = TextAlign::Left;
	t.pStr = GameString(CARGO_STRING_BASE + 1);
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
	{ // unselect the previous Element
		DrawElementAmount(OldElement, false);
	}

	if (NewElement != (uint8_t)~0)
	{ // select the new Element
		DrawElementAmount(NewElement, true);
	}

	UnbatchGraphics();
	SetContext(OldContext);
}

static void DrawElementDescription(uint16_t Element)
{
	DrawStatusMessage(GameString(Element + (CARGO_STRING_BASE + 2)));
}

void DrawRainbowPlanet(uint16_t planet)
{
	GfxStamp s;
	GfxText t;
	GfxRect r;
	char buf[40];

	GFXCONTEXT OldContext {SetContext(StatusContext)};

	r.corner.x = ResScale(2);
	r.extent.width = FieldWidth() + ResScale(1);
	r.corner.y = ResScale(20);
	r.extent.height = (ResScale(129) - r.corner.y);

	BatchGraphics();

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, ResScale(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, CargoBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(DEVICE_CARGO_FRAME);
	}

	// draw the "DATALOG" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - ResScale(1);
	t.baseline.y = ResScale(27);
	t.align = TextAlign::Center;
	t.pStr = GameString(CARGO_STRING_BASE + 10); // datalog
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(CargoSelectedAmountColor);
	font_DrawText(&t);

	// rainbow world icon
	s.origin.x = ResScale(1);
	s.origin.y = ResScale(27);
	s.frame = SetAbsFrameIndex(MiscDataFrame, 109);
	DrawStamp(&s);

	// number of worlds
	SetContextForeGroundColor(planet != 0 ?
								  PcmenuSelectionBackgroundColor :
								  CargoBackColor);
	r.corner.x = (StatusWidth() >> 1) - ResScale(6);
	r.extent.width = ResScale(12);
	r.extent.height = ElementSpacingY() - ResScale(2);
	r.corner.y = s.origin.y - s.frame->HotSpot.y + s.frame->Bounds.height
			   + ResScale(3);
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
	r.corner.x = (StatusWidth() >> 1) - ResScale(20);
	r.extent.width = ResScale(40);
	r.extent.height *= 2;
	r.corner.y += ResScale(22);
	SetContextForeGroundColor(CargoBackColor);
	DrawFilledRectangle(&r);

	// decoration text
	if (planet == 0)
	{
		t.pStr = GameString(CARGO_STRING_BASE + 11); // complete
		t.CharCount = (uint16_t)~0;
		t.baseline.y += ResScale(25);
		SetContextForeGroundColor(PcmenuTextColor);
		font_DrawText(&t);
	}
	else
	{
		t.pStr = GameString(CARGO_STRING_BASE + 12); // Uploading
		t.CharCount = (uint16_t)~0;
		t.baseline.y += ResScale(22);
		SetContextForeGroundColor(PcmenuTextColor);
		font_DrawText(&t);
		t.pStr = GameString(CARGO_STRING_BASE + 13); // data
		t.CharCount = (uint16_t)~0;
		t.baseline.y += ResScale(7);
		font_DrawText(&t);
	}

	UnbatchGraphics();
	SetContext(OldContext);
}

static bool DoDiscardCargo(MenuState* pMS)
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
		if (uqm::gameState().sis().ElementAmounts[pMS->CurState])
		{
			--uqm::gameState().sis().ElementAmounts[pMS->CurState];
			DrawCargoStrings(pMS->CurState, pMS->CurState);

			--uqm::gameState().sis().TotalElementMass;
			ShowRemainingCapacity();
		}
		else
		{ // no Element left in cargo hold
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
	MenuState menuState;

	memset(&menuState, 0, sizeof menuState);

	// draw the initial cargo display
	DrawCargoStrings((uint8_t)~0, menuState.CurState);
	DrawElementDescription(menuState.CurState);

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		DrawMenuStateStrings(PM_ALT_CARGO, 0);
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	menuState.InputFunc = DoDiscardCargo;
	DoInput(&menuState, true);

	// erase the cargo display
	ClearSISRect(DrawSisDisplay);
}
