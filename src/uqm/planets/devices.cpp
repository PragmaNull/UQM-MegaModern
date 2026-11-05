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
#include "../gendef.h"
#include "../starmap.h"
#include "../encount.h"
#include "../gamestr.h"
#include "../controls.h"
#include "../save.h"
#include "../settings.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../state.h"
#include "../sis.h"
// for ClearSISRect()
#include "../grpinfo.h"
#include "../sounds.h"
#include "../util.h"
#include "../hyper.h"
// for SaveSisHyperState()
#include "planets.h"
// for SaveSolarSysLocation() and tests
#include "libs/strlib.h"
#include "../../options.h"
#include "libs/graphics/gfx_common.h"
// for scaling down devices in HdResolutionFactor


// If DEBUG_DEVICES is defined, the device list shown in the game will
// include the pictures of all devices defined, regardless of which
// devices the player actually possesses.
//#define DEBUG_DEVICES

static auto DeviceIconWidth() { return RES_SCALE(16); }
static auto DeviceIconHeight() { return RES_SCALE(16); }

static auto DeviceOrgY() { return RES_SCALE(33); }
static auto DeviceSpacingY() { return (DeviceIconHeight() + RES_SCALE(2)); }

static auto DeviceCol0() { return RES_SCALE(4); }
static auto DeviceCol1() { return RES_SCALE(40); }

static auto DeviceSelOrgX() { return (DeviceCol0() + DeviceIconWidth()); }
static auto DeviceSelWidth() { return (FieldWidth() + RES_SCALE(2) - DeviceSelOrgX()); }

static auto IconOfsY() { return RES_SCALE(1); }
static auto NameOfsY() { return RES_SCALE(2); }
static auto TextBaseline() { return RES_SCALE(6); }
static auto TextSpacingY() { return RES_SCALE(7); }

static auto MaxVisDevices() { return ((RES_SCALE(129) - DeviceOrgY()) / DeviceSpacingY()); }


typedef enum
{
	DEVICE_FAILURE = 0,
	DEVICE_SUCCESS,
	DEVICE_SUCCESS_NO_SOUND,
} DeviceStatus;

typedef struct
{
	uint8_t list[NUM_DEVICES];
	// List of all devices player has
	uint16_t count;
	// Number of devices in the list
	uint16_t topIndex;
	// Index of the top device displayed
} DEVICES_STATE;


#if 0
static void
EraseDevicesBackground (void)
{
	GFXRECT r;

	r.corner.x = RES_SCALE (3); 
	r.extent.width = FieldWidth() - RES_SCALE (1); 
	r.corner.y = DeviceOrgY();
	r.extent.height = MaxVisDevices() * DeviceSpacingY();
	SetContextForeGroundColor (DevicesBackColor);
	DrawFilledRectangle (&r);
}
#endif

static void
DrawDevice(uint16_t device, uint16_t pos, bool selected)
{
	GFXRECT r;
	TEXT t;

	t.align = ALIGN_CENTER;
	t.baseline.x = DeviceCol1();

	r.extent.width = DeviceSelWidth();
	r.extent.height = TextSpacingY() * 2;
	r.corner.x = DeviceSelOrgX();

	// draw line background
	r.corner.y = DeviceOrgY() + pos * DeviceSpacingY() + NameOfsY();
	SetContextForeGroundColor(selected ?
								  DevicesSelectedBackColor :
								  DevicesBackColor);
	DrawFilledRectangle(&r);

	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	// print device name
	SetContextForeGroundColor(selected ?
								  DevicesSelectedNameColor :
								  DevicesNameColor);
	t.baseline.y = r.corner.y + TextBaseline();
	t.pStr = GAME_STRING(device + DEVICE_STRING_BASE + 1);
	t.CharCount = uqm::utf8StringPos(t.pStr, ' ');
	font_DrawText(&t);
	t.baseline.y += TextSpacingY();
	t.pStr = uqm::skipUTF8Chars(t.pStr, t.CharCount + 1);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
}

static void
DrawDevicesDisplay(DEVICES_STATE* devState)
{
	TEXT t;
	GFXRECT r;
	STAMP s;
	int16_t cy {};

	r.corner.x = RES_SCALE(2);
	r.corner.y = RES_SCALE(20);
	r.extent.width = FieldWidth() + RES_SCALE(1);
	// XXX: Shouldn't the height be 1 less? This draws the bottom border
	//   1 pixel too low. Or if not, why do we need another box anyway?
	r.extent.height = (RES_SCALE(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, RES_SCALE(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, DevicesBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(DEVICE_CARGO_FRAME);
	}

	// print the "DEVICES" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - RES_SCALE(1);
	t.baseline.y = r.corner.y + RES_SCALE(7);
	t.align = ALIGN_CENTER;
	t.pStr = GAME_STRING(DEVICE_STRING_BASE);
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(DevicesSelectedNameColor);
	font_DrawText(&t);

	s.origin.x = DeviceCol0();
	cy = DeviceOrgY();

	// draw device icons and print names
	for (uint16_t i {0}; i < MaxVisDevices(); ++i, cy += DeviceSpacingY())
	{
		const uint16_t devIndex = devState->topIndex + i;

		if (devIndex >= devState->count)
		{
			break;
		}

		// draw device icon
		s.origin.y = cy + IconOfsY();
		s.frame = SetAbsFrameIndex(MiscDataFrame,
								   77 + devState->list[devIndex]);

		DrawStamp(&s);

		DrawDevice(devState->list[devIndex], i, false);
	}
}

static void
DrawDevices(DEVICES_STATE* devState, uint16_t OldDevice, uint16_t NewDevice)
{
	BatchGraphics();

	SetContext(StatusContext);

	if (OldDevice > NUM_DEVICES)
	{ // Asked for the initial display or refresh
		DrawDevicesDisplay(devState);

		// do not draw unselected again this time
		OldDevice = NewDevice;
	}

	if (OldDevice != NewDevice)
	{ // unselect the previous element
		DrawDevice(devState->list[OldDevice], OldDevice - devState->topIndex,
				   false);
	}

	if (NewDevice < NUM_DEVICES)
	{ // select the new element
		DrawDevice(devState->list[NewDevice], NewDevice - devState->topIndex,
				   true);
	}

	UnbatchGraphics();
}

// Returns true if the broadcaster has been successfully activated,
// and false otherwise.
static bool
UseCaster(void)
{
	if (inHQSpace())
	{
		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) <= 1)
		{
			SET_GAME_STATE(USED_BROADCASTER, 1);
			return true;
		}
		return false;
	}

	if (uqm::lowByte(uqm::currentActivity()) != IN_INTERPLANETARY
		|| !playerInSolarSystem())
	{
		return false;
	}

	if (playerInPlanetOrbit()
		&& matchWorld(pSolarSysState, pSolarSysState->pOrbitalDesc,
					  MatchPbyte, MatchPlanet)
		&& CurStarDescPtr()->Index == CHMMR_DEFINED
		&& !GET_GAME_STATE(CHMMR_UNLEASHED))
	{
		// In orbit around the Chenjesu/Mmrnmhrm home planet.
		NextActivity |= CHECK_LOAD; /* fake a load game */
		uqm::currentActivity() |= START_ENCOUNTER;

		EncounterGroup = 0;
		PutGroupInfo(GroupsRandom, GroupSaveIp);
		ReinitQueue(&GLOBAL(ip_group_q));
		assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

		SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 7);
		SaveSolarSysLocation();
		return true;
	}

	{
		bool FoundIlwrath {};
		HIPGROUP hGroup {};

		FoundIlwrath = (CurStarDescPtr()->Index == ILWRATH_DEFINED)
					&& StartSphereTracking(ILWRATH_SHIP);
		// In the Ilwrath home system and they are alive?

		if (!FoundIlwrath && (hGroup = GetHeadLink(&GLOBAL(ip_group_q))))
		{
			// Is an Ilwrath ship in the system?

			const IP_GROUP* GroupPtr {LockIpGroup(&GLOBAL(ip_group_q), hGroup)};
			FoundIlwrath = (GroupPtr->race_id == ILWRATH_SHIP);
			UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
		}

		if (FoundIlwrath)
		{
			NextActivity |= CHECK_LOAD; /* fake a load game */
			uqm::currentActivity() |= START_ENCOUNTER;

			EncounterGroup = 0;
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&GLOBAL(ip_group_q));
			assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

			if (CurStarDescPtr()->Index == ILWRATH_DEFINED)
			{
				// Ilwrath home system.
				SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 4);
			}
			else
			{
				// Ilwrath ship.
				SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 5);
			}

			if (playerInPlanetOrbit())
			{
				SaveSolarSysLocation();
			}
			return true;
		}
	}

	return false;
}

static DeviceStatus
InvokeDevice(uint8_t which_device)
{
	uint8_t val {};

	switch (which_device)
	{
		case ROSY_SPHERE_DEVICE:
			val = GET_GAME_STATE(ULTRON_CONDITION);
			if (val)
			{
				SET_GAME_STATE(ULTRON_CONDITION, val + 1);
				SET_GAME_STATE(ROSY_SPHERE_ON_SHIP, 0);
				SET_GAME_STATE(DISCUSSED_ULTRON, 0);
				SET_GAME_STATE(SUPOX_ULTRON_HELP, 0);
				return DEVICE_SUCCESS;
			}
			break;
		case WIMBLIS_TRIDENT_DEVICE:
			break;
		case GLOWING_ROD_DEVICE:
			break;
		case SUN_EFFICIENCY_DEVICE:
			if (uqm::lowByte(uqm::currentActivity()) == IN_INTERPLANETARY
				&& playerInPlanetOrbit())
			{
				PlayMenuSound(MENU_SOUND_INVOKED);
				SleepThreadUntil(FadeScreen(FadeAllToWhite, c_gameSecond * 1)
								 + (c_gameSecond * 2));
				if (CurStarDescPtr()->Index != CHMMR_DEFINED
					|| !matchWorld(pSolarSysState,
								   pSolarSysState->pOrbitalDesc,
								   MatchPbyte, MatchPlanet))
				{
					FadeScreen(FadeAllToColor, c_gameSecond * 2);
				}
				else
				{
					SET_GAME_STATE(CHMMR_EMERGING, 1);

					EncounterGroup = 0;
					uqm::currentActivity() |= START_ENCOUNTER;

					PutGroupInfo(GroupsRandom, GroupSaveIp);
					ReinitQueue(&GLOBAL(ip_group_q));
					assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

					CloneShipFragment(CHMMR_SHIP,
									  &GLOBAL(npc_built_ship_q), 0);
				}
				return DEVICE_SUCCESS_NO_SOUND;
			}
			break;
		case UTWIG_BOMB_DEVICE:
			SET_GAME_STATE(UTWIG_BOMB, 0);
			uqm::currentActivity() &= ~IN_BATTLE;
			GLOBAL_SIS(CrewEnlisted) = (uint16_t)~0;
			DeathBySuicide = true;
			return DEVICE_SUCCESS;
		case ULTRON_0_DEVICE:
			break;
		case ULTRON_1_DEVICE:
			break;
		case ULTRON_2_DEVICE:
			break;
		case ULTRON_3_DEVICE:
			break;
		case MAIDENS_DEVICE:
			break;
		case TALKING_PET_DEVICE:
			NextActivity |= CHECK_LOAD; /* fake a load game */
			uqm::currentActivity() |= START_ENCOUNTER;
			SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 0);
			if (inHQSpace())
			{
				if (GetHeadEncounter())
				{
					SET_GAME_STATE(SHIP_TO_COMPEL, 1);
				}
				uqm::currentActivity() &= ~IN_BATTLE;

				SaveSisHyperState();
			}
			else
			{
				EncounterGroup = 0;
				if (GetHeadLink(&GLOBAL(ip_group_q)))
				{
					SET_GAME_STATE(SHIP_TO_COMPEL, 1);

					PutGroupInfo(GroupsRandom, GroupSaveIp);
					ReinitQueue(&GLOBAL(ip_group_q));
					assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);
				}

				if (CurStarDescPtr()->Index == SAMATRA_DEFINED)
				{
					SET_GAME_STATE(READY_TO_CONFUSE_URQUAN, 1);
				}
				if (playerInPlanetOrbit())
				{
					SaveSolarSysLocation();
				}
			}
			return DEVICE_SUCCESS;
		case AQUA_HELIX_DEVICE:
			val = GET_GAME_STATE(ULTRON_CONDITION);
			if (val)
			{
				SET_GAME_STATE(ULTRON_CONDITION, val + 1);
				SET_GAME_STATE(AQUA_HELIX_ON_SHIP, 0);
				SET_GAME_STATE(DISCUSSED_ULTRON, 0);
				SET_GAME_STATE(SUPOX_ULTRON_HELP, 0);
				return DEVICE_SUCCESS;
			}
			break;
		case CLEAR_SPINDLE_DEVICE:
			val = GET_GAME_STATE(ULTRON_CONDITION);
			if (val)
			{
				SET_GAME_STATE(ULTRON_CONDITION, val + 1);
				SET_GAME_STATE(CLEAR_SPINDLE_ON_SHIP, 0);
				SET_GAME_STATE(DISCUSSED_ULTRON, 0);
				SET_GAME_STATE(SUPOX_ULTRON_HELP, 0);
				return DEVICE_SUCCESS;
			}
			break;
		case UMGAH_HYPERWAVE_DEVICE:
		case BURVIX_HYPERWAVE_DEVICE:
			if (UseCaster())
			{
				return DEVICE_SUCCESS;
			}
			break;
		case TAALO_PROTECTOR_DEVICE:
			break;
		case EGG_CASING0_DEVICE:
		case EGG_CASING1_DEVICE:
		case EGG_CASING2_DEVICE:
			break;
		case SYREEN_SHUTTLE_DEVICE:
			break;
		case VUX_BEAST_DEVICE:
			break;
		case DESTRUCT_CODE_DEVICE:
			break;
		case PORTAL_SPAWNER_DEVICE:
			{

				const uint32_t portalFuelCost {difficultyCase<uint32_t>(5, 10, 20) * FuelTankScale};

				if ((inHyperSpace() || (IsExtended() && inHQSpace()))
					&& GLOBAL_SIS(FuelOnBoard) >= portalFuelCost)
				{
					/* No DeltaSISGauges because the flagship picture
				 * is currently obscured.
				 */
					if (!uqm::UQMOptions::read().infiniteFuel
						|| (IsExtended() && !uqm::UQMOptions::read().infiniteFuel
							&& inHyperSpace()))
					{
						GLOBAL_SIS(FuelOnBoard) -= portalFuelCost;
					}

					if (IsExtended() && inHyperSpace())
					{
						SaveLastLoc(MAKE_POINT(
							LOGX_TO_UNIVERSE(GLOBAL_SIS(log_x)),
							LOGY_TO_UNIVERSE(GLOBAL_SIS(log_y))));
					}

					SET_GAME_STATE(PORTAL_COUNTER, 1);
					return DEVICE_SUCCESS;
				}
			}
			break;
		case URQUAN_WARP_DEVICE:
			break;
		case LUNAR_BASE_DEVICE:
			break;
	}

	return DEVICE_FAILURE;
}

bool InvokeSpawner(void)
{
	const DeviceStatus status = InvokeDevice(PORTAL_SPAWNER_DEVICE);
	if (status == DEVICE_FAILURE)
	{
		PlayMenuSound(MENU_SOUND_FAILURE);
	}
	else if (status == DEVICE_SUCCESS)
	{
		PlayMenuSound(MENU_SOUND_INVOKED);
	}

	return (status == DEVICE_FAILURE);
}

static bool
DoManipulateDevices(MENU_STATE* pMS)
{
	DEVICES_STATE* devState = (DEVICES_STATE*)pMS->privData;
	bool select {};
	bool cancel {};
	bool forward {};
	bool pagefwd {};
	bool pageback {};

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	const bool back {PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_LEFT]};
	forward = PulsedInputState.menu[KEY_MENU_DOWN]
		   || PulsedInputState.menu[KEY_MENU_RIGHT];
	pagefwd = PulsedInputState.menu[KEY_MENU_PAGE_DOWN];
	pageback = PulsedInputState.menu[KEY_MENU_PAGE_UP];

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
		DeviceStatus status;

		status = InvokeDevice(devState->list[pMS->CurState]);
		if (status == DEVICE_FAILURE)
		{
			PlayMenuSound(MENU_SOUND_FAILURE);
		}
		else if (status == DEVICE_SUCCESS)
		{
			PlayMenuSound(MENU_SOUND_INVOKED);
		}

		return (status == DEVICE_FAILURE);
	}
	else
	{
		int16_t NewTop {};

		NewTop = devState->topIndex;
		int16_t NewState {pMS->CurState};

		if (back)
		{
			--NewState;
		}
		else if (forward)
		{
			++NewState;
		}
		else if (pagefwd)
		{
			NewState += MaxVisDevices();
		}
		else if (pageback)
		{
			NewState -= MaxVisDevices();
		}

		if (NewState < 0)
		{
			NewState = 0;
		}
		else if (NewState >= devState->count)
		{
			NewState = devState->count - 1;
		}

		if (NewState < NewTop || NewState >= NewTop + MaxVisDevices())
		{
			NewTop = NewState - NewState % MaxVisDevices();
		}

		if (NewState != pMS->CurState)
		{
			if (NewTop != devState->topIndex)
			{ // redraw the display
				devState->topIndex = NewTop;
				DrawDevices(devState, (uint16_t)~0, NewState);
			}
			else
			{ // move selection to new device
				DrawDevices(devState, pMS->CurState, NewState);
			}
			pMS->CurState = NewState;
		}
	}

	return true;
}

int16_t
InventoryDevices(uint8_t* pDeviceMap, uint16_t Size)
{
	int16_t DevicesOnBoard {0};
	for (uint8_t i {0}; i < NUM_DEVICES && Size > 0; ++i)
	{
		uint8_t DeviceState {0};
		switch (i)
		{
			case ROSY_SPHERE_DEVICE:
				DeviceState = GET_GAME_STATE(ROSY_SPHERE_ON_SHIP);
				break;
			case WIMBLIS_TRIDENT_DEVICE:
				DeviceState = GET_GAME_STATE(WIMBLIS_TRIDENT_ON_SHIP);
				break;
			case GLOWING_ROD_DEVICE:
				DeviceState = GET_GAME_STATE(GLOWING_ROD_ON_SHIP);
				break;
			case SUN_EFFICIENCY_DEVICE:
				DeviceState = GET_GAME_STATE(SUN_DEVICE_ON_SHIP);
				break;
			case UTWIG_BOMB_DEVICE:
				DeviceState = GET_GAME_STATE(UTWIG_BOMB_ON_SHIP);
				break;
			case ULTRON_0_DEVICE:
				DeviceState = (GET_GAME_STATE(ULTRON_CONDITION) == 1);
				break;
			case ULTRON_1_DEVICE:
				DeviceState = (GET_GAME_STATE(ULTRON_CONDITION) == 2);
				break;
			case ULTRON_2_DEVICE:
				DeviceState = (GET_GAME_STATE(ULTRON_CONDITION) == 3);
				break;
			case ULTRON_3_DEVICE:
				DeviceState = (GET_GAME_STATE(ULTRON_CONDITION) == 4);
				break;
			case MAIDENS_DEVICE:
				DeviceState = GET_GAME_STATE(MAIDENS_ON_SHIP);
				break;
			case TALKING_PET_DEVICE:
				DeviceState = GET_GAME_STATE(TALKING_PET_ON_SHIP);
				break;
			case AQUA_HELIX_DEVICE:
				DeviceState = GET_GAME_STATE(AQUA_HELIX_ON_SHIP);
				break;
			case CLEAR_SPINDLE_DEVICE:
				DeviceState = GET_GAME_STATE(CLEAR_SPINDLE_ON_SHIP);
				break;
			case UMGAH_HYPERWAVE_DEVICE:
				DeviceState = GET_GAME_STATE(UMGAH_BROADCASTERS_ON_SHIP);
				break;
			case TAALO_PROTECTOR_DEVICE:
				DeviceState = GET_GAME_STATE(TAALO_PROTECTOR_ON_SHIP);
				break;
			case EGG_CASING0_DEVICE:
				DeviceState = GET_GAME_STATE(EGG_CASE0_ON_SHIP);
				break;
			case EGG_CASING1_DEVICE:
				DeviceState = GET_GAME_STATE(EGG_CASE1_ON_SHIP);
				break;
			case EGG_CASING2_DEVICE:
				DeviceState = GET_GAME_STATE(EGG_CASE2_ON_SHIP);
				break;
			case SYREEN_SHUTTLE_DEVICE:
				DeviceState = GET_GAME_STATE(SYREEN_SHUTTLE_ON_SHIP);
				break;
			case VUX_BEAST_DEVICE:
				DeviceState = GET_GAME_STATE(VUX_BEAST_ON_SHIP);
				break;
			case DESTRUCT_CODE_DEVICE:
#ifdef NEVER
				DeviceState = GET_GAME_STATE(DESTRUCT_CODE_ON_SHIP);
#endif /* NEVER */
				break;
			case PORTAL_SPAWNER_DEVICE:
				DeviceState = GET_GAME_STATE(PORTAL_SPAWNER_ON_SHIP);
				break;
			case URQUAN_WARP_DEVICE:
				DeviceState = GET_GAME_STATE(PORTAL_KEY_ON_SHIP);
				break;
			case BURVIX_HYPERWAVE_DEVICE:
				DeviceState = GET_GAME_STATE(BURV_BROADCASTERS_ON_SHIP);
				break;
			case LUNAR_BASE_DEVICE:
				DeviceState = GET_GAME_STATE(MOONBASE_ON_SHIP);
				break;
		}

#ifndef DEBUG_DEVICES
		if (DeviceState)
#endif /* DEBUG_DEVICES */
		{
			*pDeviceMap++ = i;
			++DevicesOnBoard;
			--Size;
		}
	}

	return DevicesOnBoard;
}

bool DevicesMenu(void)
{
	MENU_STATE MenuState;
	DEVICES_STATE DevicesState;

	memset(&MenuState, 0, sizeof MenuState);
	MenuState.privData = &DevicesState;

	memset(&DevicesState, 0, sizeof DevicesState);

	DevicesState.count = InventoryDevices(DevicesState.list, NUM_DEVICES);
	if (!DevicesState.count)
	{
		return false;
	}

	DrawDevices(&DevicesState, (uint16_t)~0, MenuState.CurState);

	SetMenuSounds(MenuSoundArrows | MenuSoundPage,
				  MenuSoundSelect);

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		DrawMenuStateStrings(PM_ALT_CARGO, 1);
	}

	MenuState.InputFunc = DoManipulateDevices;
	DoInput(&MenuState, true);

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	if (GLOBAL_SIS(CrewEnlisted) != (uint16_t)~0
		&& !(uqm::currentActivity() & CHECK_ABORT))
	{
		ClearSISRect(DrawSisDisplay);

		if (!GET_GAME_STATE(PORTAL_COUNTER)
			&& !(uqm::currentActivity() & START_ENCOUNTER)
			&& GLOBAL_SIS(CrewEnlisted) != (uint16_t)~0)
		{
			return true;
		}
	}

	return false;
}
