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

#pragma once
#ifndef UQM_MENUSTAT_H
#define UQM_MENUSTAT_H

#include "core/types/enum.h"
#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "flash.h"

namespace uqm
{
struct MenuState
{
	// Standard field required by DoInput()
	bool (*InputFunc)(struct MenuState* pMS);

	int16_t Initialized;

	uint8_t CurState;
	FrameRef CurFrame;
	ColorMapRef HangarColorMap;
	GfxPoint first_item;
	int16_t delta_item;

	FrameRef ModuleFrame;
	GfxRect flash_rect0, flash_rect1, flash_rect2;
	FrameRef flash_frame0, flash_frame1;
	FlashContext* flashContext;

	MusicRef hMusic;

	// For private use by various menus
	// Usually, a menu associates its internal data struct using this
	void* privData;
};

// XXX: Should probably go to menu.h (does not yet exist)
enum class MenuString
{
	Scan = 0,
	Starmap,
	Devices,
	Cargo,
	Roster,
	GameMenu,
	Navigate,

	MinScan,
	EneScan,
	BioScan,
	ExitScan,
	AutoScan,
	LaunchLander,

	SaveGame,
	LoadGame,
	QuitGame,
	ChangeSettings,
	ExitGameMenu,

	Converse,
	Attack,
	EncounterGameMenu,

	Fuel,
	Module,
	OutfitGameMenu,
	ExitOutfit,

	Crew,
	ShipyardGameMenu,
	ExitShipyard,

	SoundOn,
	SoundOff,
	MusicOn,
	MusicOff,
	CyborgOff,
	CyborgNormal,
	CyborgDouble,
	CyborgSuper,

	ReadVerySlow,
	ReadSlow,
	ReadModerate,
	ReadFast,
	ReadVeryFast,

	ChangeCaptain,
	ChangeShip,
	ExitSettings,

	AltScan,
	AltStarmap,
	AltManifest,
	AltGameMenu,
	AltNavigate,

	AltCargo,
	AltDevices,
	AltRoster,
	AltExitManifest,

	AltMscan,
	AltEscan,
	AltBscan,
	AltAscan,
	AltDispatch,
	AltExitScan,
};
consteval bool isArithmeticEnum(MenuString) { return true; }

enum class GameMenuString
{
	GameMenu = 5,
	ExitMenu = 10,
	Converse = 17,
	Attack,
	Fuel,
	Module,
	Crew,
	SndOn,
	SndOff,
	MusOn,
	MusOff,
	Combat,
	Reading,
	ChangeCap,
	ChangeSis,
};
consteval bool isArithmeticEnum(GameMenuString) { return true; }

extern bool DoMenuChooser(MenuState* pMS, MenuString BaseState);
extern void DrawMenuStateStrings(MenuString beg_index, int16_t NewState);
extern void DrawMineralHelpers(void);
enum class BorderFrameIndex;
extern void DrawBorder(BorderFrameIndex Visible);

} // namespace uqm

#endif // UQM_MENUSTAT_H
