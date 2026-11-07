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

// This file handles loading of teams, but the UI and the actual loading.

#define MELEESETUP_INTERNAL
#include "melee.h"

#include "../controls.h"
#include "../gameopt.h"
#include "../gamestr.h"
#include "../globdata.h"
#include "../master.h"
#include "meleesetup.h"
#include "../save.h"
#include "../setup.h"
#include "../sounds.h"
#include "options.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"


namespace uqm
{
static constexpr auto LoadTeamNameTextColor {BuildColor(MakeRgb15(0x0F, 0x10, 0x1B), 0x00)};
static constexpr auto LoadTeamNameTextColorHilite {BuildColor(MakeRgb15(0x17, 0x18, 0x1D), 0x00)};

static auto LoadMeleeBoxWidth() { return ResScale(34); }

static auto LoadMeleeBoxSpace() { return ResScale(1); }

//Team names in loadmele.c
static auto TeamNameLBoxWidth() { return ResScale(244); }
static auto TeamNameLBoxHeight() { return ResScale(10); }


static void DrawFileStrings(MeleeState* pMS);
static bool FillFileView(MeleeState* pMS);


static bool LoadTeamImage(const char* fileName, MeleeTeam* team)
{

	uqm::io::Result<uqm::io::File> stream {uqm::meleeDir().open(fileName, uqm::io::OpenMode::Read)};
	if (!stream)
	{
		return false;
	}
	return MeleeTeam_deserialize(team, *stream) != -1;
}

#if 0 /* Not used */
static void UnindexFleet (MeleeState *pMS, uint16_t index)
{
	assert (index < pMS->load.numIndices);
	pMS->load.numIndices--;
	memmove (&pMS->load.entryIndices[index],
			&pMS->load.entryIndices[index + 1],
			(pMS->load.numIndices - index) * sizeof pMS->load.entryIndices[0]);
}
#endif

static void UnindexFleets(MeleeState* pMS, uint16_t index, uint16_t count)
{
	assert(index + count <= pMS->load.numIndices);

	pMS->load.numIndices -= count;
	memmove(&pMS->load.entryIndices[index],
			&pMS->load.entryIndices[index + count],
			(pMS->load.numIndices - index) * sizeof pMS->load.entryIndices[0]);
}

static bool GetFleetByIndex(MeleeState* pMS, uint16_t index, MeleeTeam* result)
{
	if (index < pMS->load.preBuiltCount)
	{
		MeleeTeam_copy(result, pMS->load.preBuiltList[index]);
		return true;
	}

	index -= pMS->load.preBuiltCount;
	const uint16_t firstIndex {index};

	for (; index < pMS->load.numIndices; index++)
	{
		const char* const fileName {pMS->load.dirEntries[pMS->load.entryIndices[index]].c_str()};
		if (LoadTeamImage(fileName, result))
		{
			break; // Success
		}

		{
			uqm::log::warn("Warning: File '{}' is not a valid "
						   "SuperMelee team.",
						   fileName);
		}
	}

	if (index != firstIndex)
	{
		UnindexFleets(pMS, firstIndex, index - firstIndex);
	}

	return index < pMS->load.numIndices;
}

// returns (uint16_t) -1 if not found
static uint16_t GetFleetIndexByFileName(MeleeState* pMS, const char* fileName)
{
	for (uint16_t index {0}; index < pMS->load.numIndices; index++)
	{
		const char* const entryName {pMS->load.dirEntries[pMS->load.entryIndices[index]].c_str()};

		if (strcasecmp(entryName, fileName) == 0)
		{
			return pMS->load.preBuiltCount + index;
		}
	}

	return (uint16_t)-1;
}

// Auxiliary function for DrawFileStrings
// If drawShips is set the ships themselves are drawn, in addition to the
// fleet name and value; if not, only the fleet name and value are drawn.
// If highlite is set the text is drawn in the color used for highlighting.
static void DrawFileString(const MeleeTeam* team, const GfxPoint* origin,
						   bool drawShips, bool highlite)
{
	if (IsHd())
	{ // Draw the background of the text
		GfxRect r;

		r.corner.x = origin->x;
		r.corner.y = origin->y - TeamNameLBoxHeight() + ResScale(3);
		r.extent.width = TeamNameLBoxWidth();
		r.extent.height = TeamNameLBoxHeight();

		QuickRepair(30 + uqstl::to_underlying(uqm::UQMOptions::read().optControllerType.value), &r);
	}

	SetContextForeGroundColor(highlite ?
								  LoadTeamNameTextColorHilite :
								  LoadTeamNameTextColor);

	// Print the name of the fleet
	{
		GfxText Text;

		Text.baseline = *origin;
		Text.align = TextAlign::Left;
		Text.pStr = MeleeTeam_getTeamName(team);
		Text.CharCount = (uint16_t)~0;
		font_DrawText(&Text);
	}

	// Print the value of the fleet
	{
		GfxText Text;
		char buf[60] {};

		fmt::format_to_n(buf, sizeof(buf) - 1, "{}", MeleeTeam_getValue(team));
		Text.baseline = *origin;
		Text.baseline.x += NumMeleeColumns() * (LoadMeleeBoxWidth() + LoadMeleeBoxSpace())
						 - ResScale(1);
		Text.align = TextAlign::Right;
		Text.pStr = buf;
		Text.CharCount = (uint16_t)~0;
		font_DrawText(&Text);
	}

	// Draw the ships for the fleet
	if (drawShips)
	{
		GfxStamp s;

		s.origin.x = origin->x + ResScale(1);
		s.origin.y = origin->y + ResScale(4);
		for (FleetShipIndex slotI {0}; slotI < MeleeFleetSize; slotI++)
		{
			uint8_t StarShip {};

			if (slotI == TrueMeleeFleetSize())
			{
				break;
			}

			StarShip = *team->ships[slotI];
			if (StarShip != MeleeShip::None)
			{
				s.frame = GetShipIconsFromIndex(StarShip);
				DrawStamp(&s);
				s.origin.x += ResScale(17);
			}
		}
	}
}

// returns true if there are any entries in the view, in which case
// pMS->load.bot gets set to the index just past the bottom entry in the view.
// returns false if not, in which case, the entire view remains unchanged.
static bool FillFileView(MeleeState* pMS)
{
	uint16_t viewI {};

	for (viewI = 0; viewI < LoadTeamViewSize; viewI++)
	{
		const bool success = GetFleetByIndex(pMS, pMS->load.top + viewI,
											 pMS->load.view[viewI]);
		if (!success)
		{
			break;
		}
	}

	if (viewI == 0)
	{
		return false;
	}

	pMS->load.bot = pMS->load.top + viewI;
	return true;
}

static auto FileStringOriginX() { return ResScale(5 + DosNum(2)); }
static auto FileStringOriginY() { return ResScale(32 - DosNum(1)); }
static auto EntryHeight() { return ResScale(32); }

static void SelectFileString(MeleeState* pMS, bool hilite)
{
	GfxPoint origin;
	uint16_t viewI {};

	viewI = pMS->load.cur - pMS->load.top;

	GFXCONTEXT OldContext {SetContext(SpaceContext)};
	SetContextFont(MicroFont);
	BatchGraphics();

	origin.x = FileStringOriginX();
	origin.y = FileStringOriginY() + viewI * EntryHeight();
	DrawFileString(pMS->load.view[viewI], &origin, false, hilite);

	UnbatchGraphics();
	SetContext(OldContext);
}

static void DrawFileStrings(MeleeState* pMS)
{
	GfxPoint origin;

	origin.x = FileStringOriginX();
	origin.y = FileStringOriginY();

	GFXCONTEXT OldContext {SetContext(SpaceContext)};
	SetContextFont(MicroFont);
	BatchGraphics();

	DrawMeleeIcon(30 + uqstl::to_underlying(uqm::UQMOptions::read().optControllerType.value), false); /* The load team frame */

	if (FillFileView(pMS))
	{
		for (uint16_t i {pMS->load.top}; i < pMS->load.bot; i++)
		{
			DrawFileString(pMS->load.view[i - pMS->load.top], &origin,
						   true, false);
			origin.y += EntryHeight();
		}
	}

	UnbatchGraphics();
	SetContext(OldContext);
}

static void RefocusView(MeleeState* pMS, uint16_t index)
{
	assert(index < pMS->load.preBuiltCount + pMS->load.numIndices);

	pMS->load.cur = index;
	if (index <= LoadTeamViewSize / 2)
	{
		pMS->load.top = 0;
	}
	else
	{
		pMS->load.top = index - LoadTeamViewSize / 2;
	}
}

static constexpr auto FlashRate {c_gameSecond / 9};

static void flashSelectedTeam(MeleeState* pMS)
{
	static TimeCount NextTime {};
	static int hilite = 0;
	const TimeCount Now = GetTimeCounter();

	if (Now >= NextTime)
	{
		NextTime = Now + FlashRate;
		hilite ^= 1;

		GFXCONTEXT OldContext {SetContext(SpaceContext)};
		SelectFileString(pMS, hilite);
		SetContext(OldContext);
	}
}

bool DoLoadTeam(MeleeState* pMS)
{
	const TimeCount TimeIn {GetTimeCounter()};

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return false;
	}

	SetMenuSounds(MenuSoundUp | MenuSoundDown | MenuSoundPage,
				  MenuSoundSelect);

	if (!pMS->Initialized)
	{
		DrawFileStrings(pMS);
		SelectFileString(pMS, true);
		pMS->Initialized = true;
		pMS->InputFunc = DoLoadTeam;
		return true;
	}

	if (PulsedInputState.menu[*MenuKey::MenuSelect] || PulsedInputState.menu[*MenuKey::MenuCancel])
	{
		if (PulsedInputState.menu[*MenuKey::MenuSelect])
		{
			// Copy the selected fleet to the player.
			Melee_LocalChange_team(pMS, pMS->side,
								   pMS->load.view[pMS->load.cur - pMS->load.top]);
		}

		pMS->InputFunc = DoMelee;
		pMS->LastInputTime = GetTimeCounter();
		{
			GfxRect r;

			GetFrameRect(SetAbsFrameIndex(MeleeFrame, 30), &r);
			RepairMeleeFrame(&r);
		}
		return true;
	}

	{
		uint16_t newTop = pMS->load.top;
		uint16_t newIndex = pMS->load.cur;

		if (PulsedInputState.menu[*MenuKey::MenuUp])
		{
			if (newIndex > 0)
			{
				newIndex--;
				if (newIndex < newTop)
				{
					newTop = (newTop < LoadTeamViewSize) ?
								 0 :
								 newTop - LoadTeamViewSize;
				}
			}
		}
		else if (PulsedInputState.menu[*MenuKey::MenuDown])
		{
			const uint16_t numEntries = pMS->load.numIndices + pMS->load.preBuiltCount;
			if (newIndex + 1 < numEntries)
			{
				newIndex++;
				if (newIndex >= pMS->load.bot)
				{
					newTop = pMS->load.bot;
				}
			}
		}
		else if (PulsedInputState.menu[*MenuKey::MenuPageUp])
		{
			newIndex = (newIndex < LoadTeamViewSize) ?
						   0 :
						   newIndex - LoadTeamViewSize;
			newTop = (newTop < LoadTeamViewSize) ?
						 0 :
						 newTop - LoadTeamViewSize;
		}
		else if (PulsedInputState.menu[*MenuKey::MenuPageDown])
		{
			const uint16_t numEntries = pMS->load.numIndices + pMS->load.preBuiltCount;
			if (newIndex + LoadTeamViewSize < numEntries)
			{
				newIndex += LoadTeamViewSize;
				newTop += LoadTeamViewSize;
			}
			else
			{
				newIndex = numEntries - 1;
				if (newTop + LoadTeamViewSize < numEntries && numEntries > LoadTeamViewSize)
				{
					newTop = numEntries - LoadTeamViewSize;
				}
			}
		}

		if (newIndex != pMS->load.cur)
		{
			// The cursor has been moved.
			if (newTop == pMS->load.top)
			{
				// The view itself hasn't changed.
				SelectFileString(pMS, false);
			}
			else
			{
				// The view is changed.
				pMS->load.top = newTop;
				DrawFileStrings(pMS);
			}
			pMS->load.cur = newIndex;
		}
	}

	flashSelectedTeam(pMS);

	SleepThreadUntil(TimeIn + c_gameSecond / 30);

	return true;
}

static void SelectTeamByFileName(MeleeState* pMS, const char* fileName)
{
	const uint16_t index = GetFleetIndexByFileName(pMS, fileName);
	if (index == (uint16_t)-1)
	{
		return;
	}

	RefocusView(pMS, index);
}

void LoadTeamList(MeleeState* pMS)
{
	pMS->load.dirEntries = uqm::listFiles(uqm::meleeDir(), "", uqm::io::Matcher {uqm::io::Matcher::Kind::Suffix, ".mle"});

	pMS->load.numIndices = static_cast<uint16_t>(pMS->load.dirEntries.size());
	pMS->load.entryIndices.assign(pMS->load.numIndices, 0);
	for (uint16_t i {0}; i < pMS->load.numIndices; i++)
	{
		pMS->load.entryIndices[i] = i;
	}
}

bool DoSaveTeam(MeleeState* pMS)
{
	GfxStamp MsgStamp;
	char file[NAME_MAX];
	bool saveOk = false;
	bool opened {false};

	fmt::format_to_sz_n(file, "{}.mle",
						MeleeSetup_getTeamName(pMS->meleeSetup, pMS->side));

	GFXCONTEXT OldContext {SetContext(ScreenContext)};
	ConfirmSaveLoad(&MsgStamp);
	// Show the "Saving . . ." message.

	if (uqm::io::Result<uqm::io::File> stream {uqm::meleeDir().open(file, uqm::io::OpenMode::Write | uqm::io::OpenMode::Create | uqm::io::OpenMode::Truncate)})
	{
		opened = true;
		saveOk = MeleeTeam_serialize(&pMS->meleeSetup->teams[pMS->side], *stream) == 0 && stream->flush();
		stream->close();

		if (!saveOk)
		{
			(void)uqm::meleeDir().remove(file);
		}
	}

	pMS->load.top = 0;
	pMS->load.cur = 0;

	// Undo the screen damage done by the "Saving . . ." message.
	DrawStamp(&MsgStamp);
	DestroyDrawable(ReleaseDrawable(MsgStamp.frame));
	SetContext(OldContext);

	if (!saveOk)
	{
		SaveProblem();
	}

	// Update the team list; a previously existing team may have been
	// deleted when save failed.
	LoadTeamList(pMS);
	SelectTeamByFileName(pMS, file);

	return opened;
}

static constexpr auto PrebuiltCount {15};

static void InitPreBuilt(MeleeState* pMS)
{
	MeleeTeam* const* list {};

	pMS->load.preBuiltList.assign(PrebuiltCount, nullptr);
	pMS->load.preBuiltCount = PrebuiltCount;

	{
		for (size_t fleetI {0}; fleetI < pMS->load.preBuiltCount; fleetI++)
		{
			pMS->load.preBuiltList[fleetI] = MeleeTeam_new();
		}
	}

	list = pMS->load.preBuiltList.data();

	{
		/* "Balanced Team 1" */
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, GameString(GameStringBase::MeleeString + 4));
		MeleeTeam_setShip(*list, i++, MeleeShip::Androsynth);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Druuge);
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		MeleeTeam_setShip(*list, i++, MeleeShip::Melnorme);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		MeleeTeam_setShip(*list, i++, MeleeShip::Spathi);
		MeleeTeam_setShip(*list, i++, MeleeShip::Syreen);
		MeleeTeam_setShip(*list, i++, MeleeShip::Utwig);
		list++;
	}

	{
		/* "Balanced Team 2" */
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, GameString(GameStringBase::MeleeString + 5));
		MeleeTeam_setShip(*list, i++, MeleeShip::Arilou);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chenjesu);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mycon);
		MeleeTeam_setShip(*list, i++, MeleeShip::Yehat);
		MeleeTeam_setShip(*list, i++, MeleeShip::Pkunk);
		MeleeTeam_setShip(*list, i++, MeleeShip::Supox);
		MeleeTeam_setShip(*list, i++, MeleeShip::Thraddash);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		MeleeTeam_setShip(*list, i++, MeleeShip::Shofixti);
		list++;
	}

	{
		/* "200 points" */
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, GameString(GameStringBase::MeleeString + 6));
		MeleeTeam_setShip(*list, i++, MeleeShip::Androsynth);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Druuge);
		MeleeTeam_setShip(*list, i++, MeleeShip::Melnorme);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::Supox);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		MeleeTeam_setShip(*list, i++, MeleeShip::Spathi);
		MeleeTeam_setShip(*list, i++, MeleeShip::Ilwrath);
		MeleeTeam_setShip(*list, i++, MeleeShip::Vux);
		list++;
	}

	{
		/* "Behemoth Zenith" */
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, GameString(GameStringBase::MeleeString + 7));
		MeleeTeam_setShip(*list, i++, MeleeShip::Chenjesu);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chenjesu);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		MeleeTeam_setShip(*list, i++, MeleeShip::Utwig);
		MeleeTeam_setShip(*list, i++, MeleeShip::Utwig);
		list++;
	}

	{
		/* "The Peeled Eyes" */
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, GameString(GameStringBase::MeleeString + 8));
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chenjesu);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mycon);
		MeleeTeam_setShip(*list, i++, MeleeShip::Syreen);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		MeleeTeam_setShip(*list, i++, MeleeShip::Shofixti);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::Melnorme);
		MeleeTeam_setShip(*list, i++, MeleeShip::Druuge);
		MeleeTeam_setShip(*list, i++, MeleeShip::Pkunk);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "Ford's Fighters");
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		MeleeTeam_setShip(*list, i++, MeleeShip::Melnorme);
		MeleeTeam_setShip(*list, i++, MeleeShip::Supox);
		MeleeTeam_setShip(*list, i++, MeleeShip::Utwig);
		MeleeTeam_setShip(*list, i++, MeleeShip::Umgah);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "Leyland's Lashers");
		MeleeTeam_setShip(*list, i++, MeleeShip::Androsynth);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mycon);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "The Gregorizers 200");
		MeleeTeam_setShip(*list, i++, MeleeShip::Androsynth);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Druuge);
		MeleeTeam_setShip(*list, i++, MeleeShip::Melnorme);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::Supox);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		MeleeTeam_setShip(*list, i++, MeleeShip::Pkunk);
		MeleeTeam_setShip(*list, i++, MeleeShip::Spathi);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "300 point Armada!");
		MeleeTeam_setShip(*list, i++, MeleeShip::Androsynth);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chenjesu);
		MeleeTeam_setShip(*list, i++, MeleeShip::Druuge);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::Melnorme);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mycon);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		MeleeTeam_setShip(*list, i++, MeleeShip::Pkunk);
		MeleeTeam_setShip(*list, i++, MeleeShip::Spathi);
		MeleeTeam_setShip(*list, i++, MeleeShip::Supox);
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		MeleeTeam_setShip(*list, i++, MeleeShip::Yehat);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "Little Dudes with Attitudes");
		MeleeTeam_setShip(*list, i++, MeleeShip::Umgah);
		MeleeTeam_setShip(*list, i++, MeleeShip::Thraddash);
		MeleeTeam_setShip(*list, i++, MeleeShip::Shofixti);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::Vux);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "New Alliance Ships");
		MeleeTeam_setShip(*list, i++, MeleeShip::Arilou);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		MeleeTeam_setShip(*list, i++, MeleeShip::Pkunk);
		MeleeTeam_setShip(*list, i++, MeleeShip::Shofixti);
		MeleeTeam_setShip(*list, i++, MeleeShip::Supox);
		MeleeTeam_setShip(*list, i++, MeleeShip::Syreen);
		MeleeTeam_setShip(*list, i++, MeleeShip::Utwig);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		MeleeTeam_setShip(*list, i++, MeleeShip::Yehat);
		MeleeTeam_setShip(*list, i++, MeleeShip::Druuge);
		MeleeTeam_setShip(*list, i++, MeleeShip::Thraddash);
		MeleeTeam_setShip(*list, i++, MeleeShip::Spathi);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "Old Alliance Ships");
		MeleeTeam_setShip(*list, i++, MeleeShip::Arilou);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chenjesu);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mmrnmhrm);
		MeleeTeam_setShip(*list, i++, MeleeShip::Shofixti);
		MeleeTeam_setShip(*list, i++, MeleeShip::Syreen);
		MeleeTeam_setShip(*list, i++, MeleeShip::Yehat);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "Old Hierarchy Ships");
		MeleeTeam_setShip(*list, i++, MeleeShip::Androsynth);
		MeleeTeam_setShip(*list, i++, MeleeShip::Ilwrath);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mycon);
		MeleeTeam_setShip(*list, i++, MeleeShip::Spathi);
		MeleeTeam_setShip(*list, i++, MeleeShip::Umgah);
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		MeleeTeam_setShip(*list, i++, MeleeShip::Vux);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "Star Control 1");
		MeleeTeam_setShip(*list, i++, MeleeShip::Androsynth);
		MeleeTeam_setShip(*list, i++, MeleeShip::Arilou);
		MeleeTeam_setShip(*list, i++, MeleeShip::Chenjesu);
		MeleeTeam_setShip(*list, i++, MeleeShip::Earthling);
		MeleeTeam_setShip(*list, i++, MeleeShip::Ilwrath);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mmrnmhrm);
		MeleeTeam_setShip(*list, i++, MeleeShip::Mycon);
		MeleeTeam_setShip(*list, i++, MeleeShip::Shofixti);
		MeleeTeam_setShip(*list, i++, MeleeShip::Spathi);
		MeleeTeam_setShip(*list, i++, MeleeShip::Syreen);
		MeleeTeam_setShip(*list, i++, MeleeShip::Umgah);
		MeleeTeam_setShip(*list, i++, MeleeShip::Urquan);
		MeleeTeam_setShip(*list, i++, MeleeShip::Vux);
		MeleeTeam_setShip(*list, i++, MeleeShip::Yehat);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName(*list, "Star Control 2");
		MeleeTeam_setShip(*list, i++, MeleeShip::Chmmr);
		MeleeTeam_setShip(*list, i++, MeleeShip::Druuge);
		MeleeTeam_setShip(*list, i++, MeleeShip::KohrAh);
		MeleeTeam_setShip(*list, i++, MeleeShip::Melnorme);
		MeleeTeam_setShip(*list, i++, MeleeShip::Orz);
		MeleeTeam_setShip(*list, i++, MeleeShip::Pkunk);
		MeleeTeam_setShip(*list, i++, MeleeShip::Slylandro);
		MeleeTeam_setShip(*list, i++, MeleeShip::Supox);
		MeleeTeam_setShip(*list, i++, MeleeShip::Thraddash);
		MeleeTeam_setShip(*list, i++, MeleeShip::Utwig);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		MeleeTeam_setShip(*list, i++, MeleeShip::Zoqfotpik);
		list++;
	}

	assert(list == pMS->load.preBuiltList.data() + pMS->load.preBuiltCount);
}

static void UninitPreBuilt(MeleeState* pMS)
{
	for (size_t fleetI {0}; fleetI < pMS->load.preBuiltCount; fleetI++)
	{
		MeleeTeam_delete(pMS->load.preBuiltList[fleetI]);
	}
	pMS->load.preBuiltList = {};
	pMS->load.preBuiltCount = 0;
}

static void InitLoadView(MeleeState* pMS)
{
	MeleeTeam** view = pMS->load.view;

	for (size_t viewI {0}; viewI < LoadTeamViewSize; viewI++)
	{
		view[viewI] = MeleeTeam_new();
	}
}

static void UninitLoadView(MeleeState* pMS)
{
	MeleeTeam* const* view = pMS->load.view;

	for (size_t viewI {0}; viewI < LoadTeamViewSize; viewI++)
	{
		MeleeTeam_delete(view[viewI]);
	}
}

void InitMeleeLoadState(MeleeState* pMS)
{
	pMS->load.entryIndices = {};
	InitPreBuilt(pMS);
	InitLoadView(pMS);
}

void UninitMeleeLoadState(MeleeState* pMS)
{
	UninitLoadView(pMS);
	UninitPreBuilt(pMS);
	pMS->load.entryIndices = {};
}

} // namespace uqm
