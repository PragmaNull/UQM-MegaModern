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
#include "../encount.h"
#include "../gamestr.h"
#include "../gendef.h"
#include "../save.h"
#include "../settings.h"
#include "../setup.h"
#include "../shipcont.h"
#include "../sis.h"
#include "../starmap.h"
#include "../state.h"
// for ClearSISRect()
#include "../grpinfo.h"
#include "../hyper.h"
#include "../sounds.h"
#include "../util.h"
// for SaveSisHyperState()
#include "planets.h"
// for SaveSolarSysLocation() and tests
#include "../../options.h"
#include "libs/graphics/gfx_common.h"
#include "libs/strlib.h"

namespace uqm
{
// for scaling down devices in HdResolutionFactor


// If DEBUG_DEVICES is defined, the device list shown in the game will
// include the pictures of all devices defined, regardless of which
// devices the player actually possesses.
//#define DEBUG_DEVICES

static auto DeviceIconWidth() { return ResScale(16); }
static auto DeviceIconHeight() { return ResScale(16); }

static auto DeviceOrgY() { return ResScale(33); }
static auto DeviceSpacingY() { return (DeviceIconHeight() + ResScale(2)); }

static auto DeviceCol0() { return ResScale(4); }
static auto DeviceCol1() { return ResScale(40); }

static auto DeviceSelOrgX() { return (DeviceCol0() + DeviceIconWidth()); }
static auto DeviceSelWidth() { return (FieldWidth() + ResScale(2) - DeviceSelOrgX()); }

static auto IconOfsY() { return ResScale(1); }
static auto NameOfsY() { return ResScale(2); }
static auto TextBaseline() { return ResScale(6); }
static auto TextSpacingY() { return ResScale(7); }

static auto MaxVisDevices() { return ((ResScale(129) - DeviceOrgY()) / DeviceSpacingY()); }


enum class DeviceStatus
{
	Failure = 0,
	Success,
	SuccessNoSound,
};

struct DevicesMenuState
{
	uqstl::array<uint8_t, *Device::NumDevices> list;
	// List of all devices player has
	uint16_t count;
	// Number of devices in the list
	uint16_t topIndex;
	// Index of the top device displayed
};


#if 0
static void EraseDevicesBackground (void)
{
	GfxRect r;

	r.corner.x = ResScale (3); 
	r.extent.width = FieldWidth() - ResScale (1); 
	r.corner.y = DeviceOrgY();
	r.extent.height = MaxVisDevices() * DeviceSpacingY();
	SetContextForeGroundColor (DevicesBackColor);
	DrawFilledRectangle (&r);
}
#endif

static void DrawDevice(uint16_t device, uint16_t pos, bool selected)
{
	GfxRect r;
	GfxText t;

	t.align = TextAlign::Center;
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
	t.pStr = GameString(device + GameStringBase::DeviceString + 1);
	t.CharCount = uqm::utf8StringPos(t.pStr, ' ');
	font_DrawText(&t);
	t.baseline.y += TextSpacingY();
	t.pStr = uqm::skipUTF8Chars(t.pStr, t.CharCount + 1);
	t.CharCount = (uint16_t)~0;
	font_DrawText(&t);
}

static void DrawDevicesDisplay(DevicesMenuState* devState)
{
	GfxText t;
	GfxRect r;
	GfxStamp s;
	int16_t cy {};

	r.corner.x = ResScale(2);
	r.corner.y = ResScale(20);
	r.extent.width = FieldWidth() + ResScale(1);
	// XXX: Shouldn't the height be 1 less? This draws the bottom border
	//   1 pixel too low. Or if not, why do we need another box anyway?
	r.extent.height = (ResScale(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, ResScale(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, DevicesBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(BorderFrameIndex::DeviceCargo);
	}

	// print the "DEVICES" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - ResScale(1);
	t.baseline.y = r.corner.y + ResScale(7);
	t.align = TextAlign::Center;
	t.pStr = GameString(GameStringBase::DeviceString);
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

static void DrawDevices(DevicesMenuState* devState, uint16_t OldDevice, uint16_t NewDevice)
{
	const BatchScope batch {};

	SetContext(StatusContext);

	if (OldDevice > Device::NumDevices)
	{ // Asked for the initial display or refresh
		DrawDevicesDisplay(devState);

		// do not draw unselected again this time
		OldDevice = NewDevice;
	}

	if (OldDevice != NewDevice)
	{ // unselect the previous Element
		DrawDevice(devState->list[OldDevice], OldDevice - devState->topIndex,
				   false);
	}

	if (NewDevice < Device::NumDevices)
	{ // select the new Element
		DrawDevice(devState->list[NewDevice], NewDevice - devState->topIndex,
				   true);
	}
}

// Returns true if the broadcaster has been successfully activated,
// and false otherwise.
static bool UseCaster(void)
{
	if (inHQSpace())
	{
		if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
		{
			uqm::gameState().setFlag(uqm::GameFlag::USED_BROADCASTER, static_cast<uint32_t>(1));
			return true;
		}
		return false;
	}

	if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::InInterplanetary
		|| !playerInSolarSystem())
	{
		return false;
	}

	if (playerInPlanetOrbit()
		&& matchWorld(pSolarSysState, pSolarSysState->pOrbitalDesc,
					  MatchPbyte, MatchPlanet)
		&& CurStarDescPtr()->Index == PlotId::Chmmr
		&& !uqm::gameState().flag(uqm::GameFlag::CHMMR_UNLEASHED))
	{
		// In orbit around the Chenjesu/Mmrnmhrm home planet.
		NextActivity |= ActivityFlag::CheckLoad; /* fake a load game */
		uqm::currentActivity() |= ActivityFlag::StartEncounter;

		EncounterGroup = 0;
		PutGroupInfo(GroupsRandom, GroupSaveIp);
		ReinitQueue(&uqm::gameState().game().ip_group_q);
		assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

		uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 7));
		SaveSolarSysLocation();
		return true;
	}

	{
		bool FoundIlwrath {};
		IpGroupHandle hGroup {};

		FoundIlwrath = (CurStarDescPtr()->Index == PlotId::Ilwrath)
					&& StartSphereTracking(RaceId::Ilwrath);
		// In the Ilwrath home system and they are alive?

		if (!FoundIlwrath && (hGroup = GetHeadLink(&uqm::gameState().game().ip_group_q)))
		{
			// Is an Ilwrath ship in the system?

			const InterplanetaryGroup* GroupPtr {LockIpGroup(&uqm::gameState().game().ip_group_q, hGroup)};
			FoundIlwrath = (GroupPtr->race_id == RaceId::Ilwrath);
			UnlockIpGroup(&uqm::gameState().game().ip_group_q, hGroup);
		}

		if (FoundIlwrath)
		{
			NextActivity |= ActivityFlag::CheckLoad; /* fake a load game */
			uqm::currentActivity() |= ActivityFlag::StartEncounter;

			EncounterGroup = 0;
			PutGroupInfo(GroupsRandom, GroupSaveIp);
			ReinitQueue(&uqm::gameState().game().ip_group_q);
			assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

			if (CurStarDescPtr()->Index == PlotId::Ilwrath)
			{
				// Ilwrath home system.
				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 4));
			}
			else
			{
				// Ilwrath ship.
				uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(1 << 5));
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

static DeviceStatus InvokeDevice(uint8_t which_device)
{
	uint8_t val {};

	switch (static_cast<Device>(which_device))
	{
		case Device::RosySphere:
			val = uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION);
			if (val)
			{
				uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(val + 1));
				uqm::gameState().setFlag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(0));
				return DeviceStatus::Success;
			}
			break;
		case Device::WimblisTrident:
			break;
		case Device::GlowingRod:
			break;
		case Device::SunEfficiency:
			if (uqm::activityKind(uqm::currentActivity()) == ActivityKind::InInterplanetary
				&& playerInPlanetOrbit())
			{
				PlayMenuSound(MenuSoundEffect::Invoked);
				SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToWhite, c_gameSecond * 1)
								 + (c_gameSecond * 2));
				if (CurStarDescPtr()->Index != PlotId::Chmmr
					|| !matchWorld(pSolarSysState,
								   pSolarSysState->pOrbitalDesc,
								   MatchPbyte, MatchPlanet))
				{
					FadeScreen(ScreenFadeType::FadeAllToColor, c_gameSecond * 2);
				}
				else
				{
					uqm::gameState().setFlag(uqm::GameFlag::CHMMR_EMERGING, static_cast<uint32_t>(1));

					EncounterGroup = 0;
					uqm::currentActivity() |= ActivityFlag::StartEncounter;

					PutGroupInfo(GroupsRandom, GroupSaveIp);
					ReinitQueue(&uqm::gameState().game().ip_group_q);
					assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

					CloneShipFragment(RaceId::Chmmr,
									  &uqm::gameState().game().npc_built_ship_q, 0);
				}
				return DeviceStatus::SuccessNoSound;
			}
			break;
		case Device::UtwigBomb:
			uqm::gameState().setFlag(uqm::GameFlag::UTWIG_BOMB, static_cast<uint32_t>(0));
			uqm::currentActivity() &= ~ActivityFlag::InBattle;
			uqm::gameState().sis().CrewEnlisted = (uint16_t)~0;
			DeathBySuicide = true;
			return DeviceStatus::Success;
		case Device::Ultron0:
			break;
		case Device::Ultron1:
			break;
		case Device::Ultron2:
			break;
		case Device::Ultron3:
			break;
		case Device::Maidens:
			break;
		case Device::TalkingPet:
			NextActivity |= ActivityFlag::CheckLoad; /* fake a load game */
			uqm::currentActivity() |= ActivityFlag::StartEncounter;
			uqm::gameState().setFlag(uqm::GameFlag::GLOBAL_FLAGS_AND_DATA, static_cast<uint32_t>(0));
			if (inHQSpace())
			{
				if (GetHeadEncounter())
				{
					uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(1));
				}
				uqm::currentActivity() &= ~ActivityFlag::InBattle;

				SaveSisHyperState();
			}
			else
			{
				EncounterGroup = 0;
				if (GetHeadLink(&uqm::gameState().game().ip_group_q))
				{
					uqm::gameState().setFlag(uqm::GameFlag::SHIP_TO_COMPEL, static_cast<uint32_t>(1));

					PutGroupInfo(GroupsRandom, GroupSaveIp);
					ReinitQueue(&uqm::gameState().game().ip_group_q);
					assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);
				}

				if (CurStarDescPtr()->Index == PlotId::Samatra)
				{
					uqm::gameState().setFlag(uqm::GameFlag::READY_TO_CONFUSE_URQUAN, static_cast<uint32_t>(1));
				}
				if (playerInPlanetOrbit())
				{
					SaveSolarSysLocation();
				}
			}
			return DeviceStatus::Success;
		case Device::AquaHelix:
			val = uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION);
			if (val)
			{
				uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(val + 1));
				uqm::gameState().setFlag(uqm::GameFlag::AQUA_HELIX_ON_SHIP, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(0));
				return DeviceStatus::Success;
			}
			break;
		case Device::ClearSpindle:
			val = uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION);
			if (val)
			{
				uqm::gameState().setFlag(uqm::GameFlag::ULTRON_CONDITION, static_cast<uint32_t>(val + 1));
				uqm::gameState().setFlag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::DISCUSSED_ULTRON, static_cast<uint32_t>(0));
				uqm::gameState().setFlag(uqm::GameFlag::SUPOX_ULTRON_HELP, static_cast<uint32_t>(0));
				return DeviceStatus::Success;
			}
			break;
		case Device::UmgahHyperwave:
		case Device::BurvixHyperwave:
			if (UseCaster())
			{
				return DeviceStatus::Success;
			}
			break;
		case Device::TaaloProtector:
			break;
		case Device::EggCasing0:
		case Device::EggCasing1:
		case Device::EggCasing2:
			break;
		case Device::SyreenShuttle:
			break;
		case Device::VuxBeast:
			break;
		case Device::DestructCode:
			break;
		case Device::PortalSpawner:
			{

				const uint32_t portalFuelCost {difficultyCase<uint32_t>(5, 10, 20) * FuelTankScale};

				if ((inHyperSpace() || (IsExtended() && inHQSpace()))
					&& uqm::gameState().sis().FuelOnBoard >= portalFuelCost)
				{
					/* No DeltaSISGauges because the flagship picture
				 * is currently obscured.
				 */
					if (!uqm::UQMOptions::read().infiniteFuel
						|| (IsExtended() && !uqm::UQMOptions::read().infiniteFuel
							&& inHyperSpace()))
					{
						uqm::gameState().sis().FuelOnBoard -= portalFuelCost;
					}

					if (IsExtended() && inHyperSpace())
					{
						SaveLastLoc(MAKE_POINT(
							LogxToUniverse(uqm::gameState().sis().log_x),
							LogyToUniverse(uqm::gameState().sis().log_y)));
					}

					uqm::gameState().setFlag(uqm::GameFlag::PORTAL_COUNTER, static_cast<uint32_t>(1));
					return DeviceStatus::Success;
				}
			}
			break;
		case Device::UrquanWarp:
			break;
		case Device::LunarBase:
			break;
		default:
			break;
	}

	return DeviceStatus::Failure;
}

bool InvokeSpawner(void)
{
	const DeviceStatus status = InvokeDevice(*Device::PortalSpawner);
	if (status == DeviceStatus::Failure)
	{
		PlayMenuSound(MenuSoundEffect::Failure);
	}
	else if (status == DeviceStatus::Success)
	{
		PlayMenuSound(MenuSoundEffect::Invoked);
	}

	return (status == DeviceStatus::Failure);
}

static bool DoManipulateDevices(MenuState* pMS)
{
	DevicesMenuState* devState = (DevicesMenuState*)pMS->privData;
	bool select {};
	bool cancel {};
	bool forward {};
	bool pagefwd {};
	bool pageback {};

	select = PulsedInputState.menu[*MenuKey::MenuSelect];
	cancel = PulsedInputState.menu[*MenuKey::MenuCancel];
	const bool back {PulsedInputState.menu[*MenuKey::MenuUp] || PulsedInputState.menu[*MenuKey::MenuLeft]};
	forward = PulsedInputState.menu[*MenuKey::MenuDown]
		   || PulsedInputState.menu[*MenuKey::MenuRight];
	pagefwd = PulsedInputState.menu[*MenuKey::MenuPageDown];
	pageback = PulsedInputState.menu[*MenuKey::MenuPageUp];

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
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
		if (status == DeviceStatus::Failure)
		{
			PlayMenuSound(MenuSoundEffect::Failure);
		}
		else if (status == DeviceStatus::Success)
		{
			PlayMenuSound(MenuSoundEffect::Invoked);
		}

		return (status == DeviceStatus::Failure);
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

int16_t InventoryDevices(uqstl::span<uint8_t> pDeviceMap)
{
	int16_t DevicesOnBoard {0};
	for (Device i {}; i < Device::NumDevices && static_cast<size_t>(DevicesOnBoard) < pDeviceMap.size(); ++i)
	{
		uint8_t DeviceState {0};
		switch (i)
		{
			case Device::RosySphere:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::ROSY_SPHERE_ON_SHIP);
				break;
			case Device::WimblisTrident:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::WIMBLIS_TRIDENT_ON_SHIP);
				break;
			case Device::GlowingRod:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::GLOWING_ROD_ON_SHIP);
				break;
			case Device::SunEfficiency:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::SUN_DEVICE_ON_SHIP);
				break;
			case Device::UtwigBomb:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::UTWIG_BOMB_ON_SHIP);
				break;
			case Device::Ultron0:
				DeviceState = (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 1);
				break;
			case Device::Ultron1:
				DeviceState = (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 2);
				break;
			case Device::Ultron2:
				DeviceState = (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 3);
				break;
			case Device::Ultron3:
				DeviceState = (uqm::gameState().flag(uqm::GameFlag::ULTRON_CONDITION) == 4);
				break;
			case Device::Maidens:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::MAIDENS_ON_SHIP);
				break;
			case Device::TalkingPet:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::TALKING_PET_ON_SHIP);
				break;
			case Device::AquaHelix:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::AQUA_HELIX_ON_SHIP);
				break;
			case Device::ClearSpindle:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::CLEAR_SPINDLE_ON_SHIP);
				break;
			case Device::UmgahHyperwave:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::UMGAH_BROADCASTERS_ON_SHIP);
				break;
			case Device::TaaloProtector:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::TAALO_PROTECTOR_ON_SHIP);
				break;
			case Device::EggCasing0:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::EGG_CASE0_ON_SHIP);
				break;
			case Device::EggCasing1:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::EGG_CASE1_ON_SHIP);
				break;
			case Device::EggCasing2:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::EGG_CASE2_ON_SHIP);
				break;
			case Device::SyreenShuttle:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::SYREEN_SHUTTLE_ON_SHIP);
				break;
			case Device::VuxBeast:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::VUX_BEAST_ON_SHIP);
				break;
			case Device::DestructCode:
#ifdef NEVER
				DeviceState = uqm::gameState().flag(uqm::GameFlag::DESTRUCT_CODE_ON_SHIP);
#endif /* NEVER */
				break;
			case Device::PortalSpawner:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::PORTAL_SPAWNER_ON_SHIP);
				break;
			case Device::UrquanWarp:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::PORTAL_KEY_ON_SHIP);
				break;
			case Device::BurvixHyperwave:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::BURV_BROADCASTERS_ON_SHIP);
				break;
			case Device::LunarBase:
				DeviceState = uqm::gameState().flag(uqm::GameFlag::MOONBASE_ON_SHIP);
				break;
			default:
				break;
		}

#ifndef DEBUG_DEVICES
		if (DeviceState)
#endif /* DEBUG_DEVICES */
		{
			pDeviceMap[DevicesOnBoard] = *i;
			++DevicesOnBoard;
		}
	}

	return DevicesOnBoard;
}

bool DevicesMenu(void)
{
	MenuState menuState;
	DevicesMenuState DevicesState;

	memset(&menuState, 0, sizeof menuState);
	menuState.privData = &DevicesState;

	memset(&DevicesState, 0, sizeof DevicesState);

	DevicesState.count = InventoryDevices({DevicesState.list.data(), *Device::NumDevices});
	if (!DevicesState.count)
	{
		return false;
	}

	DrawDevices(&DevicesState, (uint16_t)~0, menuState.CurState);

	SetMenuSounds(MenuSoundFlags::Arrows | MenuSoundFlags::Page,
				  MenuSoundFlags::Select);

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
	{
		DrawMenuStateStrings(MenuString::AltCargo, 1);
	}

	menuState.InputFunc = DoManipulateDevices;
	DoInput(&menuState, true);

	SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);

	if (uqm::gameState().sis().CrewEnlisted != (uint16_t)~0
		&& !(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		ClearSISRect(ClearSisFlags::DrawDisplay);

		if (!uqm::gameState().flag(uqm::GameFlag::PORTAL_COUNTER)
			&& !(uqm::currentActivity() & ActivityFlag::StartEncounter)
			&& uqm::gameState().sis().CrewEnlisted != (uint16_t)~0)
		{
			return true;
		}
	}

	return false;
}

} // namespace uqm
