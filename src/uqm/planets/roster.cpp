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

#include "../build.h"
#include "../colors.h"
#include "../controls.h"
#include "../races.h"
#include "../units.h"
#include "../util.h" // JMS: For SaveContextFrame()
#include "../sis.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../sounds.h"
#include "port.h"
#include "libs/gfxlib.h"
#include "core/log/log.h"
#include "libs/time/timelib.h"

#include <stdlib.h>

// JMS_GFX: These exist to prevent the leftover red borders of the rostered ships in hi-res.
static GfxRect savedShipFrame_r;
static GfxStamp savedShipFrame;

// Ship icon positions in status display around the flagship
const auto& ship_pos {SupportShipPts};

struct RosterState
{
	// Ship icon positions split into (lower half) left and right (upper)
	// and sorted in the Y coord. These are used for navigation around the
	// escort positions.
	GfxPoint shipPos[MaxBuiltShips];
	uint16_t count;
	// Number of ships

	GfxPoint curShipPt;
	// Location of the currently selected escort
	FRAME curShipFrame;
	// Icon of the currently selected escort
	bool modifyingCrew;
	// true when in crew modification "sub-menu". This is simple
	// enough that it does not require a real sub-menu.
};

static ShipFragment* LockSupportShip(RosterState*, HSHIPFRAG* phFrag);

static void drawSupportShip(RosterState* rosterState, bool filled, bool saveFrame)
{
	GfxStamp s;

	if (!rosterState->curShipFrame)
	{
		return;
	}

	s.origin.x = ResScale(rosterState->curShipPt.x);
	s.origin.y = ResScale(rosterState->curShipPt.y);
	s.frame = rosterState->curShipFrame;

	if (saveFrame)
	{
		savedShipFrame_r.corner.x = s.origin.x;
		savedShipFrame_r.corner.y = s.origin.y;
		savedShipFrame_r.extent.width = ResScale(16);
		savedShipFrame_r.extent.height = ResScale(16);

		savedShipFrame = SaveContextFrame(&savedShipFrame_r);

		uqm::log::debug("Saved x:{}, y:{}", savedShipFrame_r.corner.x, savedShipFrame_r.corner.y);
	}
	else
	{
		if (filled)
		{
			DrawFilledStamp(&s);
		}
		else
		{
			DrawStamp(&s);
		}
	}
}

static void getSupportShipIcon(RosterState* rosterState)
{
	HSHIPFRAG hShipFrag {};

	rosterState->curShipFrame = nullptr;
	const ShipFragment* ShipFragPtr {LockSupportShip(rosterState, &hShipFrag)};
	if (!ShipFragPtr)
	{
		return;
	}

	rosterState->curShipFrame = SetAbsFrameIndex(ShipFragPtr->icons, 2);
	UnlockShipFrag(&uqm::gameState().game().built_ship_q, hShipFrag);
}

static void flashSupportShip(RosterState* rosterState, bool saveFrame)
{
	static Color c = BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x24);
	static TimeCount NextTime {};

	drawSupportShip(rosterState, true, saveFrame);

	if (GetTimeCounter() >= NextTime)
	{
		NextTime = GetTimeCounter() + (c_gameSecond / 15);

		/* The commented code out code is the old code before the switch
		 * to 24-bits colors. The current code produces very slightly
		 * different colors due to rounding errors, but the old code wasn't
		 * original anyhow, and you can't tell the difference visually.
		 * - SvdB
		if (c >= BuildColor (MakeRgb15 (0x1F, 0x19, 0x19), 0x24))
			c = BuildColor (MakeRgb15 (0x1F, 0x00, 0x00), 0x24);
		else
			c += BuildColor (MakeRgb15 (0x00, 0x02, 0x02), 0x00);
		*/

		if (c.g >= Cc5to8(0x19))
		{
			c = BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x24);
		}
		else
		{
			c.g += Cc5to8(0x02);
			c.b += Cc5to8(0x02);
		}
		SetContextForeGroundColor(c);

		drawSupportShip(rosterState, true, false);
	}
}

static ShipFragment* LockSupportShip(RosterState* rosterState, HSHIPFRAG* phFrag)
{
	const GfxPoint* pship_pos {};
	HSHIPFRAG hStarShip {};
	HSHIPFRAG hNextShip {};

	// Lookup the current escort's location in the unsorted points list
	// to find the original escort index
	for (hStarShip = GetHeadLink(&uqm::gameState().game().built_ship_q),
		pship_pos = ship_pos;
		 hStarShip; hStarShip = hNextShip, ++pship_pos)
	{
		ShipFragment* StarShipPtr {LockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip)};

		if (pointsEqual(*pship_pos, rosterState->curShipPt))
		{
			*phFrag = hStarShip;
			return StarShipPtr;
		}

		hNextShip = _GetSuccLink(StarShipPtr);
		UnlockShipFrag(&uqm::gameState().game().built_ship_q, hStarShip);
	}

	return nullptr;
}

static void flashSupportShipCrew(void)
{
	GfxRect r;

	SetContext(StatusContext);
	GetStatusMessageRect(&r);
	SetFlashRect(&r, false);
}

static bool DeltaSupportCrew(RosterState* rosterState, int16_t crew_delta)
{
	bool ret = false;
	char buf[40] {};
	HFLEETINFO hTemplate {};
	HSHIPFRAG hShipFrag {};

	ShipFragment* StarShipPtr {LockSupportShip(rosterState, &hShipFrag)};
	if (!StarShipPtr)
	{
		return false;
	}

	hTemplate = GetStarShipFromIndex(&uqm::gameState().game().avail_race_q,
									 StarShipPtr->race_id);
	FleetInfo* TemplatePtr {LockFleetInfo(&uqm::gameState().game().avail_race_q, hTemplate)};

	StarShipPtr->crew_level += crew_delta;

	if (StarShipPtr->crew_level == 0)
	{
		StarShipPtr->crew_level = 1;
	}
	else if (StarShipPtr->crew_level > TemplatePtr->crew_level && crew_delta > 0)
	{
		StarShipPtr->crew_level -= crew_delta;
	}
	else
	{
		if (StarShipPtr->crew_level >= TemplatePtr->crew_level)
		{
			fmt::format_to_n(buf, sizeof(buf) - 1, "{}", StarShipPtr->crew_level);
		}
		else
		{
			fmt::format_to_n(buf, sizeof(buf) - 1, "{}/{}",
							 StarShipPtr->crew_level,
							 TemplatePtr->crew_level);
		}

		PreUpdateFlashRect();
		DrawStatusMessage(buf);
		PostUpdateFlashRect();
		DeltaSISGauges(-crew_delta, 0, 0);
		if (crew_delta)
		{
			flashSupportShipCrew();
		}
		ret = true;
	}

	UnlockFleetInfo(&uqm::gameState().game().avail_race_q, hTemplate);
	UnlockShipFrag(&uqm::gameState().game().built_ship_q, hShipFrag);

	return ret;
}

static void drawModifiedSupportShip(RosterState* rosterState)
{
	SetContext(StatusContext);
	SetContextForeGroundColor(RosterModifyShipColor);
	drawSupportShip(rosterState, true, false);
}

static void selectSupportShip(RosterState* rosterState, uint16_t shipIndex)
{
	rosterState->curShipPt = rosterState->shipPos[shipIndex];
	getSupportShipIcon(rosterState);
	DeltaSupportCrew(rosterState, 0);
}

static bool DoModifyRoster(MenuState* pMS)
{
	RosterState* rosterState = (RosterState*)pMS->privData;
	bool select {};
	bool cancel {};
	bool up {};
	bool down {};
	bool pgup {};
	bool pgdn {};

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		return false;
	}

	select = PulsedInputState.menu[*MenuKey::MenuSelect];
	cancel = PulsedInputState.menu[*MenuKey::MenuCancel];
	up = PulsedInputState.menu[*MenuKey::MenuUp];
	down = PulsedInputState.menu[*MenuKey::MenuDown];
	// Left or right produces the same effect because there are 2 columns
	const bool horiz {PulsedInputState.menu[*MenuKey::MenuLeft] || PulsedInputState.menu[*MenuKey::MenuRight]};
	pgup = PulsedInputState.menu[*MenuKey::MenuZoomIn];
	pgdn = PulsedInputState.menu[*MenuKey::MenuZoomOut];

	if (cancel && !rosterState->modifyingCrew)
	{
		return false;
	}
	else if (select || cancel)
	{
		rosterState->modifyingCrew ^= true;
		if (!rosterState->modifyingCrew)
		{
			SetFlashRect(nullptr, false);
			SetMenuSounds(MenuSoundArrows | MenuSoundPage, MenuSoundSelect);
		}
		else
		{
			drawModifiedSupportShip(rosterState);
			flashSupportShipCrew();
			SetMenuSounds(MenuSoundUp | MenuSoundDown | MenuSoundPage,
						  MenuSoundAction);
		}
	}
	else if (rosterState->modifyingCrew)
	{
		uint16_t loop {};
		uint16_t DoLoop = 0;
		bool failed = false;

		if (up || pgup)
		{
			if (uqm::gameState().sis().CrewEnlisted)
			{
				DoLoop = pgup ? 10 : 1;
			}
			else
			{
				failed = true;
			}
		}
		else if (down || pgdn)
		{
			if (uqm::gameState().sis().CrewEnlisted < GetCrewPodCapacity())
			{
				DoLoop = pgdn ? 10 : 1;
			}
			else
			{
				failed = true;
			}
		}

		if (DoLoop != 0)
		{
			for (loop = 0; loop < DoLoop; loop++)
			{
				failed = !DeltaSupportCrew(rosterState, (down || pgdn) ? -1 : 1);
			}
		}

		if (failed)
		{ // not enough room or crew
			PlayMenuSound(MenuSoundEffect::Failure);
		}
	}
	else
	{
		const GfxPoint* pship_pos = rosterState->shipPos;
		const uint16_t top_right = (rosterState->count + 1) >> 1;

		uint16_t NewState {pMS->CurState};

		if (rosterState->count < 2)
		{
			// no navigation allowed
		}
		else if (horiz)
		{
			if (NewState == top_right - 1)
			{
				NewState = rosterState->count - 1;
			}
			else if (NewState >= top_right)
			{
				NewState -= top_right;
				if (pship_pos[NewState].y < pship_pos[pMS->CurState].y)
				{
					++NewState;
				}
			}
			else
			{
				NewState += top_right;
				if (NewState != top_right
					&& pship_pos[NewState].y > pship_pos[pMS->CurState].y)
				{
					--NewState;
				}
			}
		}
		else if (down)
		{
			++NewState;
			if (NewState == rosterState->count)
			{
				NewState = top_right;
			}
			else if (NewState == top_right)
			{
				NewState = 0;
			}
		}
		else if (up)
		{
			if (NewState == 0)
			{
				NewState = top_right - 1;
			}
			else if (NewState == top_right)
			{
				NewState = rosterState->count - 1;
			}
			else
			{
				--NewState;
			}
		}

		BatchGraphics();
		SetContext(StatusContext);

		if (NewState != pMS->CurState)
		{
			// Draw the previous escort in unselected state.
			// JMS_GFX: In HdResolutionFactor mode we draw the rectangle of screen
			// we captured earlier.
			if (IsHd())
			{
				DrawStamp(&savedShipFrame);
			}
			else // In 1x mode we just draw the icon.
			{
				drawSupportShip(rosterState, false, false);
			}
			// Select the new one
			selectSupportShip(rosterState, NewState);
			pMS->CurState = NewState;

			// JMS_GFX: In HdResolutionFactor mode we now have to capture the
			// location of this new rectangle.
			if (IsHd())
			{
				flashSupportShip(rosterState, true);
			}
			else
			{
				flashSupportShip(rosterState, false);
			}
		}
		else
		{
			flashSupportShip(rosterState, false);
		}

		UnbatchGraphics();
	}

	return true;
}

bool RosterMenu(void)
{
	MenuState menuState;
	RosterState rosterState;

	memset(&menuState, 0, sizeof menuState);
	menuState.privData = &rosterState;

	memset(&rosterState, 0, sizeof rosterState);

	rosterState.count = CountLinks(&uqm::gameState().game().built_ship_q);
	if (!rosterState.count)
	{
		return false;
	}

	// Get the escort positions we will use and sort on X then Y
	assert(sizeof(rosterState.shipPos) == sizeof(ship_pos));
	memcpy(rosterState.shipPos, ship_pos, sizeof(ship_pos));
	uqstl::sort(rosterState.shipPos, rosterState.shipPos + rosterState.count, [](const GfxPoint& a, const GfxPoint& b) {
		// Ships on the left in the lower half, and ordered on Y
		return a.x != b.x ? a.x < b.x : a.y < b.y;
	});

	SetContext(StatusContext);
	selectSupportShip(&rosterState, menuState.CurState);

	// JMS_GFX: Remember the location of the first ship to be able to erase
	// the red junk from around it after rostering.
	if (IsHd())
	{
		drawSupportShip(&rosterState, true, true);
	}

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		DrawMenuStateStrings(MenuString::AltCargo, 2);
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	menuState.InputFunc = DoModifyRoster;
	DoInput(&menuState, true);

	SetContext(StatusContext);

	// Draw the last escort in unselected state.
	// JMS_GFX: In HdResolutionFactor mode we draw the rectangle of screen
	// we captured earlier.
	if (IsHd())
	{
		DrawStamp(&savedShipFrame);
	}
	else // In Original mode we just draw the icon.
	{
		drawSupportShip(&rosterState, false, false);
	}

	DrawStatusMessage(nullptr);

	return true;
}
