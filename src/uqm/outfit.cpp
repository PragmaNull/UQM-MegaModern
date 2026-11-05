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

#include "options.h"
#include "colors.h"
#include "controls.h"
#include "menustat.h"
#include "gameopt.h"
#include "gamestr.h"
#include "resinst.h"
#include "menustat.h"
#include "nameref.h"
#include "settings.h"
#include "starbase.h"
#include "setup.h"
#include "sis.h"
#include "units.h"
#include "sounds.h"
#include "planets/planets.h"
// for xxx_DISASTER
#include "libs/graphics/gfx_common.h"
#include "util.h"
#include "shipcont.h"

#include "core/string/StringUtils.h"

// How manyeth .png in the module.ani file is the first lander shield.
static constexpr auto ShieldLocationInModuleAni {(23 + 2 * NUM_PURCHASE_MODULES)};
static auto DosMenu() { return (uqm::UQMOptions::read().dosMenus || IsDos()); } // The DOS Menu window

enum
{
	OUTFIT_FUEL,
	OUTFIT_MODULES,
	OUTFIT_SAVELOAD,
	OUTFIT_EXIT,
	OUTFIT_DOFUEL
};

GFXPOINT lander_pos[MaxLanders];
FONT ModuleFont;

static void InitializeDOSLanderPos(void)
{ // Initialize the DOS lander icon positions
	int i {};
	const auto& temp {LanderDosPts};

	if (!IsDos())
	{
		return;
	}

	if (lander_pos[0].x != ResScale(temp[0].x))
	{
		for (i = 0; i < MaxLanders; i++)
		{
			lander_pos[i].x = ResScale(temp[i].x);
			lander_pos[i].y = ResScale(temp[i].y);
		}
	}
}

// This is all for drawing the DOS version modules menu
static auto ModuleOrgY() { return ResScale(36); }
static auto ModuleSpacingY() { return ResScale(16); }

static auto ModuleCol0() { return ResScale(5); }
static auto ModuleCol1() { return ResScale(61); }

static auto ModuleSelOrgX() { return (ModuleCol0() - ResScale(1)); }
static auto ModuleSelWidth() { return (FieldWidth() - ResScale(3)); }

static auto NameOfsY() { return ResScale(2); }
static auto TextBaseline() { return ResScale(6); }
static auto TextSpacingY() { return ResScale(7); }

static constexpr auto MaxVisModules {5};

typedef struct
{
	uint8_t list[NUM_PURCHASE_MODULES];
	// List of all modules player has
	uint16_t count;
	// Number of modules in the list
	uint16_t topIndex;
	// Index of the top module displayed
} MODULES_STATE;

MODULES_STATE ModuleState;

static void DrawModuleStatus(uint16_t index, uint16_t pos, bool selected)
{
	GFXRECT r;
	TEXT t;
	char buf[10];

	t.align = ALIGN_LEFT;
	t.baseline.x = ModuleCol0();

	r.extent.width = ModuleSelWidth();
	r.extent.height = TextSpacingY() * 2;
	r.corner.x = ModuleSelOrgX();

	// draw line background
	r.corner.y = ModuleOrgY() + pos * ModuleSpacingY() + NameOfsY();
	SetContextForeGroundColor(selected ?
								  ModuleSelectedBackColor :
								  ModuleBackColor);
	DrawFilledRectangle(&r);
	SetContextFont(TinyFont);


	if (uqm::gameState().game().ModuleCost[index])
	{ // print module name
		SetContextForeGroundColor(selected ?
									  ModuleSelectedColor :
									  ModuleNameColor);
		t.baseline.y = r.corner.y + TextBaseline();
		t.pStr = GameString(index + DosStarbaseStringBase);
		t.CharCount = uqm::utf8StringPos(t.pStr, ' ');
		font_DrawText(&t);
		t.baseline.y += TextSpacingY();
		t.pStr = uqm::skipUTF8Chars(t.pStr, t.CharCount + 1);
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);

		// print module cost
		SetContextForeGroundColor(selected ?
									  ModuleSelectedColor :
									  ModulePriceColor);
		t.align = ALIGN_RIGHT;
		t.baseline.x = ModuleCol1() - ResScale(2);
		t.baseline.y -= ResScale(3);
		fmt::format_to_sz_n(buf, "{}",
							uqm::gameState().game().ModuleCost[index] * ModuleCostScale);
		t.pStr = buf;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
	else
	{
		SetContextForeGroundColor(ModulePriceColor);
		r.corner.x += ResScale(21);
		r.corner.y += ResScale(6);
		r.extent.width >>= 2;
		r.extent.height = ResScale(2);
		DrawFilledRectangle(&r);
	}
}

static void DrawModuleDisplay(MODULES_STATE* modState)
{
	TEXT t;
	GFXRECT r;

	r.corner.x = ResScale(2);
	r.corner.y = ResScale(20);
	r.extent.width = FieldWidth() + ResScale(1);
	r.extent.height = (ResScale(129) - r.corner.y);

	if (!uqm::UQMOptions::read().customBorder && !IsHd())
	{
		DrawStarConBox(&r, ResScale(1),
					   ShadowboxMediumColor, ShadowboxDarkColor,
					   true, ModuleBackColor, false, Transparent);
	}
	else
	{
		DrawBorder(DEVICE_CARGO_FRAME);
	}

	// print the "MODULES" title
	SetContextFont(StarConFont);
	t.baseline.x = (StatusWidth() >> 1) - ResScale(1);
	t.baseline.y = r.corner.y + ResScale(7);
	t.align = ALIGN_CENTER;
	t.pStr = GameString(STARBASE_STRING_BASE + 11);
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(ModuleSelectedColor);
	font_DrawText(&t);

	// print names and costs
	for (uint16_t i {0}; i < MaxVisModules; ++i)
	{
		const uint16_t modIndex = modState->topIndex + i;

		if (modIndex >= modState->count)
		{
			break;
		}

		DrawModuleStatus(modState->list[modIndex], i, false);
	}
}

static void DrawModules(MODULES_STATE* modState, uint16_t NewItem)
{
	GFXCONTEXT OldContext = SetContext(StatusContext);

	BatchGraphics();

	DrawModuleDisplay(modState);
	DrawModuleStatus(modState->list[NewItem],
					 NewItem - modState->topIndex, true);

	UnbatchGraphics();

	SetContext(OldContext);
}

static void ManipulateModules(int16_t NewState)
{
	int16_t NewTop {};

	if (!DosMenu())
	{
		return;
	}

	MODULES_STATE* modState {&ModuleState};
	NewTop = modState->topIndex;

	if (NewState > NUM_PURCHASE_MODULES)
	{
		DrawModules(modState, NewState);
		return;
	}

	if (NewState < NewTop || NewState >= NewTop + MaxVisModules)
	{
		modState->topIndex = NewState - NewState % MaxVisModules;
	}

	DrawModules(modState, NewState);
}

int16_t InventoryModules(uint8_t* pModuleMap, uint16_t Size)
{
	int16_t ModulesOnBoard {0};
	for (uint8_t i {0}; i < NUM_PURCHASE_MODULES && Size > 0; ++i)
	{
		//uint8_t ActiveModule;

		//ActiveModule = GLOBAL (ModuleCost[i]);

		{
			*pModuleMap++ = i;
			++ModulesOnBoard;
			--Size;
		}
	}

	return ModulesOnBoard;
}

static void DrawModuleMenuText(GFXRECT* r, int Index)
{
	TEXT text;
	int16_t leading {};
	GFXRECT block;
	char buf[256];

	if (IsDos() || !strlen(GameString(TdoStarbaseStringBase + Index)))
	{
		return;
	}

	SetContextFont(ModuleFont);

	GetContextFontLeading(&leading);

	SetContextForeGroundColor(MdlRectColor);

	if (!uqm::UQMOptions::read().customBorder)
	{
		block = *r;
		block.extent.height = (leading << 1) - ResScale(1);
		DrawFilledRectangle(&block);
	}
	else
	{
		DrawBorder(TEXT_LABEL_FRAME);
	}

	text.baseline.x = r->corner.x + (r->extent.width >> 1);
	text.baseline.y = r->corner.y + leading - ResScale(1);
	const int16_t og_baseline_x {text.baseline.x};

	uqm::utf8StringCopy((char*)buf, sizeof(buf),
						GameString(TdoStarbaseStringBase + Index));

	text.align = ALIGN_CENTER;
	{
		uqstl::vector<uqstl::string> tokens;
		uqm::tokenize(uqstl::string_view {buf}, tokens, ' ', false);
		for (const auto& tok : tokens)
		{
			text.pStr = AlignText((const char*)tok.c_str(),
								  &text.baseline.x);
			text.CharCount = (uint16_t)~0;

			font_DrawShadowedText(&text, WEST_SHADOW, MdlTextColor,
								  MdlShadowColor);

			text.baseline.y += leading;
			text.baseline.x = og_baseline_x;
		}
	}
}

static void DrawModuleStrings(MENU_STATE* pMS, uint8_t NewModule)
{
	GFXRECT r;
	STAMP s;

	GFXCONTEXT OldContext {SetContext(StatusContext)};
	GetContextClipRect(&r);
	s.origin.x = RadarX() - r.corner.x;
	s.origin.y = RadarY() - r.corner.y;

	BatchGraphics();

	if (!IsDos())
	{
		r.corner.x = s.origin.x - ResScale(1);
		r.corner.y = s.origin.y - ResScale(11);
		r.extent.width = RadarWidth() + ResScale(2);
		r.extent.height = ResScale(11);
		//	ClearSISRect (ClearSisRadar); // blinks otherwise
		SetContextForeGroundColor(MenuForegroundColor);
		if (!uqm::UQMOptions::read().customBorder)
		{
			DrawFilledRectangle(&r); // drawn over anyway
		}
	}

	DrawBorder(SIS_RADAR_FRAME);

	if (IsDos())
	{
		GFXRECT dosRect;

		dosRect.corner.x = ResScale(2);
		dosRect.corner.y = RadarY() - ResScale(1);
		dosRect.extent.width = RadarWidth() + ResScale(4);
		dosRect.extent.height = RadarHeight() + ResScale(2);

		if (uqm::UQMOptions::read().customBorder)
		{
			dosRect.corner.x += ResScale(2);
			dosRect.corner.y += ResScale(2);
			dosRect.extent.width -= ResScale(4);
			dosRect.extent.height -= ResScale(4);
			SetContextForeGroundColor(BlackColor);
			DrawFilledRectangle(&dosRect);
		}
		else
		{
			if (!IsHd())
			{
				DrawStarConBox(&dosRect, 1, PcmenuTopLeftBorderColor,
							   PcmenuBottomRightBorderColor, true,
							   BlackColor, false, Transparent);
			}
			else
			{
				DrawRenderedBox(&dosRect, true, BlackColor,
								THIN_INNER_BEVEL, uqm::UQMOptions::read().customBorder);
			}
		}
	}

	if (NewModule >= EmptySlot)
	{
		r.corner = s.origin;
		r.extent.width = RadarWidth();
		r.extent.height = RadarHeight();
		SetContextForeGroundColor(BlackColor);
		DrawFilledRectangle(&r);
	}
	else if (pMS->CurFrame)
	{
		TEXT t;
		char buf[40] {};

		// Draw the module image.
		s.frame = SetAbsFrameIndex(pMS->CurFrame, NewModule);
		DrawStamp(&s);

		/// HERE!
		DrawModuleMenuText(&r, NewModule);

		// Print the module cost.
		t.baseline.x = s.origin.x + RadarWidth() - ResScale(2);
		t.baseline.y = s.origin.y + RadarHeight() - ResScale(2);
		t.align = ALIGN_RIGHT;
		t.CharCount = (uint16_t)~0;
		t.pStr = buf;
		fmt::format_to_n(buf, sizeof(buf) - 1, "{}",
						 uqm::gameState().game().ModuleCost[NewModule] * ModuleCostScale);
		if (isPC(uqm::UQMOptions::read().whichFonts))
		{
			SetContextFont(TinyFont);
		}
		else
		{
			SetContextFont(TinyFontBold);
		}

		if ((uqm::gameState().sis().ResUnits)
			>= (uint32_t)((uqm::gameState().game().ModuleCost[NewModule]
						   * ModuleCostScale)))
		{
			SetContextForeGroundColor(BrightGreenColor);
		}
		else
		{
			SetContextForeGroundColor(BrightRedColor);
		}

		if (!IsDos())
		{
			font_DrawText(&t);
		}
	}
	UnbatchGraphics();
	SetContext(OldContext);
}

static void RedistributeFuel(void)
{
	const GFXCONTEXT OldContext = SetContext(SpaceContext);

	BatchGraphics();

	DrawFuelInFTanks(true);

	UnbatchGraphics();
	SetContext(OldContext);
}

static void DrawEscapePodText(GFXRECT rect)
{
	TEXT text;
	Color OldColor;
	int16_t leading {};
	GFXRECT block;
	char buf[256];

	if (!strlen(GameString(EndStarbaseStringBase + 1)))
	{
		return;
	}

	FONT OldFont {SetContextFont(SquareFont)};
	OldColor = SetContextForeGroundColor(BlackColor);

	block = rect;
	block.corner.x += DosBoolScl(171, 9);
	block.corner.y += ResScale(38);
	block.extent.width = ResScale(36);
	block.extent.height = ResScale(6);
	DrawFilledRectangle(&block);

	block.corner.x += ResScale(9);
	block.corner.y += block.extent.height;
	block.extent.width = ResScale(19);
	DrawFilledRectangle(&block);

	GetContextFontLeading(&leading);

	text.baseline = rect.corner;
	text.baseline.x += DosBoolScl(189, 27);
	text.baseline.y += ResScale(39);
	text.align = ALIGN_CENTER;

	const int16_t og_baseline_x {text.baseline.x};

	uqm::utf8StringCopy((char*)buf, sizeof(buf),
						GameString(EndStarbaseStringBase + 1));

	text.align = ALIGN_CENTER;
	SetContextForeGroundColor(LanderPodTextColor);

	{
		uqstl::vector<uqstl::string> tokens;
		uqm::tokenize(uqstl::string_view {buf}, tokens, ' ', false);
		for (const auto& tok : tokens)
		{
			text.pStr = AlignText((const char*)tok.c_str(),
								  &text.baseline.x);
			text.CharCount = (uint16_t)~0;

			font_DrawText(&text);

			text.baseline.y += leading;
			text.baseline.x = og_baseline_x;
		}
	}

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static void DrawNoLandersText(GFXRECT rect)
{
	TEXT text;
	Color OldColor;
	GFXRECT block;

	if (IsDos() || !strlen(GameString(EndStarbaseStringBase)))
	{
		return;
	}

	FONT OldFont {SetContextFont(SquareFont)};
	OldColor = SetContextForeGroundColor(BlackColor);

	block = rect;
	block.corner.y += ResScale(9);
	block.extent.width = ResScale(162) - IfHd(3);
	block.extent.height = ResScale(20);
	DrawFilledRectangle(&block);

	text.baseline = rect.corner;
	text.baseline.x += ResScale(81);
	text.baseline.y += ResScale(17);
	text.align = ALIGN_CENTER;
	text.pStr = AlignText(
		(const char*)GameString(EndStarbaseStringBase),
		&text.baseline.x);
	text.CharCount = (uint16_t)~0;

	SetContextForeGroundColor(LanderPodTextColor);
	font_DrawText(&text);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static auto LanderX() { return (ResScale(24) - SafePad()); }
static auto LanderY() { return ResScale(67); }
static auto LanderWidth() { return ResScale(15); }

static void DisplayLanders(MENU_STATE* pMS)
{
	STAMP s;

	s.frame = pMS->ModuleFrame;
	if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 3)
	{
		GFXRECT rect;
		s.origin.x = -SafeX();
		s.origin.y = 0;
		s.frame = SetAbsFrameIndex(pMS->ModuleFrame,
								   ShieldLocationInModuleAni + 4);
		DrawStamp(&s);

		GetFrameRect(s.frame, &rect);
		rect.corner.x += s.origin.x;

		DrawNoLandersText(rect);
		DrawEscapePodText(rect);
	}
	else
	{
		uint16_t i {};

		if (!IsDos())
		{
			s.origin.x = LanderX();
			s.origin.y = LanderY();
			for (i = 0; i < uqm::gameState().sis().NumLanders; ++i)
			{
				DrawStamp(&s);
				s.origin.x += LanderWidth();
			}

			SetContextForeGroundColor(BlackColor);
			for (; i < MaxLanders; ++i)
			{
				DrawFilledStamp(&s);
				s.origin.x += LanderWidth();
			}
		}
		else
		{
			for (i = 0; i < uqm::gameState().sis().NumLanders; ++i)
			{
				s.origin = lander_pos[i];
				DrawStamp(&s);
			}

			SetContextForeGroundColor(BlackColor);
			for (; i < MaxLanders; ++i)
			{
				s.origin = lander_pos[i];
				DrawFilledStamp(&s);
			}
		}
	}
}

static bool DoInstallModule(MENU_STATE* pMS)
{
	uint8_t new_slot_piece {};
	uint8_t old_slot_piece {};
	int16_t LastItem {};
	bool select {};
	bool cancel {};

	if (uqm::currentActivity() & CHECK_ABORT)
	{
		pMS->InputFunc = DoOutfit;
		return (true);
	}

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	const bool motion {PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_RIGHT] || PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_DOWN]};

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	int16_t FirstItem {0};
	uint8_t NewState {pMS->CurState};
	switch (NewState)
	{
		case PLANET_LANDER:
		case EmptySlot + 3:
			old_slot_piece = pMS->delta_item < uqm::gameState().sis().NumLanders ? PLANET_LANDER : (EmptySlot + 3);
			LastItem = MaxLanders - 1;
			break;
		case FUSION_THRUSTER:
		case EmptySlot + 0:
			old_slot_piece = uqm::gameState().sis().DriveSlots[pMS->delta_item];
			LastItem = NumDriveSlots - 1;
			break;
		case TURNING_JETS:
		case EmptySlot + 1:
			old_slot_piece = uqm::gameState().sis().JetSlots[pMS->delta_item];
			LastItem = NumJetSlots - 1;
			break;
		default:
			old_slot_piece = uqm::gameState().sis().ModuleSlots[pMS->delta_item];
			if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 3)
			{
				FirstItem = NumBombModules;
			}
			LastItem = NumModuleSlots - 1;
			break;
	}

	if (NewState < CREW_POD)
	{
		FirstItem = LastItem = NewState;
	}
	else if (NewState < EmptySlot)
	{
		FirstItem = CREW_POD, LastItem = NUM_PURCHASE_MODULES - 1;
	}

	if (!pMS->Initialized)
	{
		new_slot_piece = old_slot_piece;
		pMS->Initialized = true;

		pMS->InputFunc = DoInstallModule;


		SetContext(SpaceContext);
		ClearSISRect(ClearSisRadar);
		SetFlashRect(nullptr, false);
		goto InitFlash;
	}
	else if (select || cancel)
	{
		new_slot_piece = pMS->CurState;
		if (select)
		{
			if (new_slot_piece < EmptySlot)
			{
				if (uqm::gameState().sis().ResUnits < (uint32_t)(uqm::gameState().game().ModuleCost[new_slot_piece]
																 * ModuleCostScale))
				{ // not enough RUs to build
					PlayMenuSound(MENU_SOUND_FAILURE);
					return (true);
				}
			}
			else if (new_slot_piece == EmptySlot + 2)
			{
				if (old_slot_piece == CREW_POD)
				{
					if (uqm::gameState().sis().CrewEnlisted
						> CrewPodCapacity
							  * (CountSISPieces(CREW_POD) - 1))
					{ // crew pod still needed for crew recruited
						PlayMenuSound(MENU_SOUND_FAILURE);
						return (true);
					}
				}
				else if (old_slot_piece == FUEL_TANK
						 || old_slot_piece == HIGHEFF_FUELSYS)
				{
					uint32_t volume {};

					volume = (uint32_t)CountSISPieces(FUEL_TANK)
							   * FuelTankCapacity
						   + (uint32_t)CountSISPieces(HIGHEFF_FUELSYS)
								 * HefuelTankCapacity;
					volume -= (old_slot_piece == FUEL_TANK ? FuelTankCapacity : HefuelTankCapacity);
					if (uqm::gameState().sis().FuelOnBoard > volume + FuelReserve)
					{ // fuel tank still needed for the fuel on board
						if (!uqm::UQMOptions::read().infiniteFuel)
						{
							PlayMenuSound(MENU_SOUND_FAILURE);
							return (true);
						}
						else
						{
							if (old_slot_piece == FUEL_TANK)
							{
								DeltaSISGauges(0, -FuelTankCapacity, 0);
							}
							else
							{
								DeltaSISGauges(0, -HefuelTankCapacity, 0);
							}
						}
					}
				}
				else if (old_slot_piece == STORAGE_BAY)
				{
					if (uqm::gameState().sis().TotalElementMass
						> StorageBayCapacity
							  * (CountSISPieces(STORAGE_BAY) - 1))
					{ // storage bay still needed for the cargo
						PlayMenuSound(MENU_SOUND_FAILURE);
						return (true);
					}
				}
			}
		}

		SetContext(SpaceContext);

		SetFlashRect(nullptr, false);

		if (select)
		{
			if (new_slot_piece >= EmptySlot
				&& old_slot_piece >= EmptySlot)
			{
				new_slot_piece -= EmptySlot - 1;
				if (new_slot_piece > CREW_POD)
				{
					new_slot_piece = PLANET_LANDER;
				}
			}
			else
			{
				switch (pMS->CurState)
				{
					case PLANET_LANDER:
						++uqm::gameState().sis().NumLanders;
						break;
					case EmptySlot + 3:
						--uqm::gameState().sis().NumLanders;
						break;
					case FUSION_THRUSTER:
					case EmptySlot + 0:
						uqm::gameState().sis().DriveSlots[pMS->delta_item] =
							new_slot_piece;
						break;
					case TURNING_JETS:
					case EmptySlot + 1:
						uqm::gameState().sis().JetSlots[pMS->delta_item] =
							new_slot_piece;
						break;
					default:
						uqm::gameState().sis().ModuleSlots[pMS->delta_item] =
							new_slot_piece;
						break;
				}

				if (new_slot_piece < EmptySlot)
				{
					DeltaSISGauges(UndefinedDelta, UndefinedDelta,
								   -(uqm::gameState().game().ModuleCost[new_slot_piece]
									 * ModuleCostScale));
				}
				else /* if (old_slot_piece < EmptySlot) */
				{
					DeltaSISGauges(UndefinedDelta, UndefinedDelta,
								   uqm::gameState().game().ModuleCost[old_slot_piece]
									   * ModuleCostScale);
				}

				if (pMS->CurState == PLANET_LANDER || pMS->CurState == EmptySlot + 3)
				{
					DisplayLanders(pMS);
				}
				else
				{
					DrawShipPiece(pMS->ModuleFrame, new_slot_piece,
								  pMS->delta_item, false);

					if (new_slot_piece > TURNING_JETS
						&& old_slot_piece > TURNING_JETS)
					{
						RedistributeFuel();
					}

					DrawFlagshipStats();
				}
			}

			cancel = false;
		}

		if (pMS->CurState < EmptySlot)
		{
			pMS->CurState += EmptySlot - 1;
			if ((pMS->CurState == EmptySlot - 1) && pMS->delta_item + 1 > uqm::gameState().sis().NumLanders)
			{
				new_slot_piece = old_slot_piece;
			}
			if (pMS->CurState < EmptySlot)
			{
				pMS->CurState = EmptySlot + 3;
			}
			else if (pMS->CurState > EmptySlot + 2)
			{
				pMS->CurState = EmptySlot + 2;
			}
			if (cancel)
			{
				new_slot_piece = pMS->CurState;
			}
			goto InitFlash;
		}
		else if (!cancel)
		{
			if ((new_slot_piece == EmptySlot + 3) && (old_slot_piece == PLANET_LANDER) && (pMS->delta_item < uqm::gameState().sis().NumLanders))
			{
				new_slot_piece = old_slot_piece;
			}
			else
			{
				pMS->CurState = new_slot_piece;
			}
			goto InitFlash;
		}
		else
		{
			SetContext(StatusContext);
			DrawMenuStateStrings(PM_FUEL, pMS->CurState = OUTFIT_MODULES);
			SetFlashRect(SfrMenu3do(), false);

			pMS->InputFunc = DoOutfit;
			ClearSISRect(DrawSisDisplay);
		}
	}
	else if (motion)
	{
		int16_t NewItem; // NOLINT(cppcoreguidelines-init-variables)

		NewItem = NewState < EmptySlot ? pMS->CurState : pMS->delta_item;
		do
		{
			if (NewState >= EmptySlot
				&& (PulsedInputState.menu[KEY_MENU_UP]
					|| PulsedInputState.menu[KEY_MENU_DOWN]))
			{
				if (PulsedInputState.menu[KEY_MENU_UP])
				{
					if (NewState-- == EmptySlot)
					{
						NewState = EmptySlot + 3;
					}
				}
				else
				{
					if (NewState++ == EmptySlot + 3)
					{
						NewState = EmptySlot;
					}
				}
				NewItem = 0;
				if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) == 3)
				{
					if (NewState == EmptySlot + 3)
					{
						NewState = PulsedInputState.menu[KEY_MENU_UP] ?
									   EmptySlot + 2 :
									   EmptySlot;
					}
					if (NewState == EmptySlot + 2)
					{
						NewItem = NumBombModules;
					}
				}
				pMS->delta_item = NewItem;
			}
			else if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_UP])
			{
				if (NewItem-- == FirstItem)
				{
					NewItem = LastItem;
				}
			}
			else if (PulsedInputState.menu[KEY_MENU_RIGHT] || PulsedInputState.menu[KEY_MENU_DOWN])
			{
				if (NewItem++ == LastItem)
				{
					NewItem = FirstItem;
				}
			}
		} while (NewState < EmptySlot
				 && (uqm::gameState().game().ModuleCost[NewItem] == 0
					 || (NewItem >= GUN_WEAPON && NewItem <= CANNON_WEAPON
						 && pMS->delta_item > 0 && pMS->delta_item < 13)));

		if (NewState < EmptySlot)
		{
			if (NewItem != pMS->CurState)
			{
				pMS->CurState = NewItem;
				PreUpdateFlashRect();
				DrawModuleStrings(pMS, NewItem);
				ManipulateModules(NewItem);
				PostUpdateFlashRect();
			}
		}
		else if (NewItem != pMS->delta_item || NewState != pMS->CurState)
		{
			int16_t w; // NOLINT(cppcoreguidelines-init-variables)

			switch (NewState)
			{
				case PLANET_LANDER:
				case EmptySlot + 3:
					new_slot_piece = NewItem < uqm::gameState().sis().NumLanders ? PLANET_LANDER : (EmptySlot + 3);
					break;
				case FUSION_THRUSTER:
				case EmptySlot + 0:
					new_slot_piece = uqm::gameState().sis().DriveSlots[NewItem];
					break;
				case TURNING_JETS:
				case EmptySlot + 1:
					new_slot_piece = uqm::gameState().sis().JetSlots[NewItem];
					break;
				default:
					new_slot_piece = uqm::gameState().sis().ModuleSlots[NewItem];
					break;
			}

			SetContext(SpaceContext);

			if (NewState == pMS->CurState)
			{
				if (NewState == PLANET_LANDER
					|| NewState == EmptySlot + 3)
				{
					w = LanderWidth();
				}
				else
				{
					w = ShipPieceOffset();
				}

				w *= (NewItem - pMS->delta_item);
				if (IsDos() && (NewState == PLANET_LANDER || NewState == EmptySlot + 3))
				{
					pMS->flash_rect0.corner.x = lander_pos[NewItem].x
											  - ResScale(1);
					pMS->flash_rect0.corner.y = lander_pos[NewItem].y
											  - ResScale(1);
				}
				else
				{
					pMS->flash_rect0.corner.x += w;
				}
				pMS->flash_rect1.corner.x += w;
				//pMS->flash_rect2.corner.x += w;
				pMS->delta_item = NewItem;
			}
			else
			{
				pMS->CurState = NewState;
InitFlash:
				w = ShipPieceOffset();
				switch (pMS->CurState)
				{
					case PLANET_LANDER:
					case EmptySlot + 3:
						pMS->flash_rect0.corner.x =
							DosBool(LanderX(), ResScale(LanderDosX))
							- ResScale(1);
						pMS->flash_rect0.corner.y =
							DosBool(LanderY(), ResScale(LanderDosY))
							- ResScale(1);
						pMS->flash_rect0.extent.width =
							ResScale(11 + 2);
						pMS->flash_rect0.extent.height =
							ResScale(13 + 2);

						w = LanderWidth();
						break;
					case FUSION_THRUSTER:
					case EmptySlot + 0:
						pMS->flash_rect0.corner.x =
							DriveTopX() - ResScale(1);
						pMS->flash_rect0.corner.y =
							DriveTopY() - ResScale(1);
						pMS->flash_rect0.extent.width = ResScale(8);
						pMS->flash_rect0.extent.height = ResScale(6);

						pMS->flash_rect1.corner.x =
							DriveSideX() - ResScale(1) + IfHd(5);
						pMS->flash_rect1.corner.y =
							DriveSideY() - ResScale(1) - IfHd(10);
						pMS->flash_rect1.extent.width =
							ResScale(12) - IfHd(10);
						pMS->flash_rect1.extent.height =
							ResScale(7) + IfHd(4);

						//pMS->flash_rect2 = pMS->flash_rect0;
						//pMS->flash_rect2.corner.y += ResScale (90);
						break;
					case TURNING_JETS:
					case EmptySlot + 1:
						pMS->flash_rect0.corner.x =
							JetTopX() - ResScale(1) - IfHd(6);
						pMS->flash_rect0.corner.y =
							JetTopY() - ResScale(1);
						pMS->flash_rect0.extent.width = ResScale(9);
						pMS->flash_rect0.extent.height = ResScale(10);

						pMS->flash_rect1.corner.x =
							JetSideX() - ResScale(1) - IfHd(6);
						pMS->flash_rect1.corner.y =
							JetSideY() - ResScale(1) + IfHd(4);
						pMS->flash_rect1.extent.width = ResScale(7);
						pMS->flash_rect1.extent.height = ResScale(4);

						//pMS->flash_rect2 = pMS->flash_rect0;
						//pMS->flash_rect2.corner.y += ResScale (70);
						break;
					default:
						pMS->flash_rect0.corner.x =
							ModuleTopX() - ResScale(1);
						pMS->flash_rect0.corner.y =
							ModuleTopY() - ResScale(1);
						pMS->flash_rect0.extent.width =
							ShipPieceOffset() + ResScale(2)
							+ ResScale(uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
						pMS->flash_rect0.extent.height = ResScale(34);

						pMS->flash_rect1.corner.x =
							ModuleSideX() - ResScale(1);
						pMS->flash_rect1.corner.y =
							ModuleSideY() - ResScale(1);
						pMS->flash_rect1.extent.width =
							ShipPieceOffset() + ResScale(2)
							+ ResScale(uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
						pMS->flash_rect1.extent.height = ResScale(25);
						break;
				}

				w *= pMS->delta_item;
				if (IsDos() && (NewState == PLANET_LANDER || NewState == EmptySlot + 3))
				{
					pMS->flash_rect0.corner.x =
						lander_pos[pMS->delta_item].x - ResScale(1);
					pMS->flash_rect0.corner.y =
						lander_pos[pMS->delta_item].y - ResScale(1);
				}
				else
				{
					pMS->flash_rect0.corner.x += w;
				}
				pMS->flash_rect1.corner.x += w;
				//pMS->flash_rect2.corner.x += w;
			}

			DrawModuleStrings(pMS, new_slot_piece);
			ManipulateModules(new_slot_piece);
			if (pMS->CurState < EmptySlot)
			{ // flash with PC menus too
				SetFlashRect(DosBool(SfrMenuAny(), SfrMenuNon()), false);
			}
			else
			{
				if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC)
				{
					switch (pMS->CurState)
					{
						case EmptySlot + 3:
							{ // lander
								DumpAdditionalRect();
								break;
							}
						case EmptySlot + 0:
						case EmptySlot + 1:
							{ // thruster and jets
								SetAdditionalRect(&pMS->flash_rect1, true);
								// SetAdditionalRect (&pMS->flash_rect2, 2);
								break;
							}
						default:
							{ // everything else
								DumpAdditionalRect();
								SetAdditionalRect(&pMS->flash_rect1, true);
								break;
							}
					}
				}

				if (IsDos() && (pMS->CurState == EmptySlot + 3 || pMS->CurState == PLANET_LANDER)
					&& is3DO(uqm::UQMOptions::read().whichMenu))
				{
					SetFlashRect(&pMS->flash_rect0, true);
				}
				else
				{
					SetFlashRect(&pMS->flash_rect0, uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC);
				}
			}
		}
	}

	return (true);
}

static void ChangeFuelQuantity(void)
{
	int32_t incr = 0; // Fuel increment in fuel points (not units).
	const int32_t maxFit =
		GetFuelTankCapacity() - (int32_t)uqm::gameState().sis().FuelOnBoard;
	const int32_t minFit = -(int32_t)uqm::gameState().sis().FuelOnBoard;

	if (PulsedInputState.menu[KEY_MENU_UP])
	{
		incr = FuelTankScale; // +1 Unit
	}
	else if (PulsedInputState.menu[KEY_MENU_DOWN])
	{
		incr = -FuelTankScale; // -1 Unit
	}
	else if (PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		incr = (FuelTankScale * 10); // +1 Bar
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		incr = -(FuelTankScale * 10); // -1 Bar
	}
	else if (PulsedInputState.menu[KEY_MENU_ZOOM_IN])
	{
		incr = maxFit; // Fill to max
	}
	else if (PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
	{
		incr = minFit; // Empty the tanks
	}
	else
	{
		return;
	}

	if (PulsedInputState.menu[KEY_MENU_ZOOM_IN]
		|| PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
	{
		PlayMenuSound(MENU_SOUND_INVOKED);
	}

	// Clamp incr to what we can afford/hold/have.
	{
		const int32_t maxAfford = uqm::gameState().sis().ResUnits / uqm::gameState().game().FuelCost;

		if (incr > maxFit)
		{
			incr = maxFit; // All we can hold.
		}

		if (incr > maxAfford * FuelTankScale)
		{
			incr = maxAfford * FuelTankScale; // All we can afford.
		}

		if (incr < minFit)
		{
			incr = minFit; // All we have.
		}
	}

	if (!incr)
	{
		// No more room, not enough RUs, or no fuel left to drain.
		PlayMenuSound(MENU_SOUND_FAILURE);
	}
	else
	{
		const int cost = (incr / FuelTankScale) * uqm::gameState().game().FuelCost;
		PreUpdateFlashRect();
		DeltaSISGauges(0, incr, -cost);
		PostUpdateFlashRect();
		RedistributeFuel();
	}

	{ // Make fuel gauge flash.
		GFXRECT r;
		GFXCONTEXT oldContext = SetContext(StatusContext);
		GetGaugeRect(&r, false);
		SetFlashRect(&r, false);
		SetContext(oldContext);
	}
}

static void onNamingDone(void)
{
	// In case player just named a ship, redraw it
	DrawFlagshipName(false, false);
}

bool DoOutfit(MENU_STATE* pMS)
{
	if (uqm::currentActivity() & CHECK_ABORT)
	{
		goto ExitOutfit;
	}

	OutfitOrShipyard = 2;

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoOutfit;
		pMS->Initialized = true;
		InitializeDOSLanderPos();

		SetNamingCallback(onNamingDone);

		{
			uint16_t num_frames {};
			STAMP s;

			if (!IsDos())
			{
				ModuleFont = LoadFont(ModuleFontRes);
			}

			pMS->CurFrame = CaptureDrawable(
				LoadGraphic(ModulesPmapAnimRes));
			pMS->hMusic = LoadMusicInstance(OutfitMusicRes);
			pMS->CurState = OUTFIT_FUEL;
			pMS->ModuleFrame = CaptureDrawable(
				LoadGraphic(SismodsMaskPmapAnimRes));
			s.origin.x = IsPad() ? (-SafeX() + ResScale(3)) : 0;
			s.origin.y = 0;
			s.frame = CaptureDrawable(
				LoadGraphic(OutfitPmapAnimRes));

			if (uqm::UQMOptions::read().flagshipColor == uqm::EmulationMode::Console3DO)
			{
				s.frame = SetAbsFrameIndex(s.frame, 1);
			}
			else
			{
				s.frame = SetAbsFrameIndex(s.frame, 0);
			}

			SetTransitionSource(nullptr);
			BatchGraphics();
			DrawSISFrame();
			DrawSISMessage(GameString(STARBASE_STRING_BASE + 2));
			DrawSISTitle(GameString(STARBASE_STRING_BASE));

			SetContext(SpaceContext);

			DrawStamp(&s);
			DestroyDrawable(ReleaseDrawable(s.frame));

			for (num_frames = 0; num_frames < NumDriveSlots;
				 ++num_frames)
			{
				const uint8_t which_piece {uqm::gameState().sis().DriveSlots[num_frames]};
				if (which_piece < EmptySlot)
				{
					DrawShipPiece(pMS->ModuleFrame, which_piece,
								  num_frames, false);
				}
			}
			for (num_frames = 0; num_frames < NumJetSlots;
				 ++num_frames)
			{
				const uint8_t which_piece {uqm::gameState().sis().JetSlots[num_frames]};
				if (which_piece < EmptySlot)
				{
					DrawShipPiece(pMS->ModuleFrame, which_piece,
								  num_frames, false);
				}
			}
			for (num_frames = 0; num_frames < NumModuleSlots;
				 ++num_frames)
			{
				const uint8_t which_piece {uqm::gameState().sis().ModuleSlots[num_frames]};
				if (which_piece < EmptySlot)
				{
					DrawShipPiece(pMS->ModuleFrame, which_piece,
								  num_frames, false);
				}
			}
			RedistributeFuel();
			DisplayLanders(pMS);
			if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) < 3)
			{
				uint8_t ShieldFlags {};

				ShieldFlags = uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS);

				s.frame = SetAbsFrameIndex(pMS->ModuleFrame,
										   ShieldLocationInModuleAni);
				if (ShieldFlags & (1 << EARTHQUAKE_DISASTER))
				{
					DrawStamp(&s);
				}
				s.frame = IncFrameIndex(s.frame);
				if (ShieldFlags & (1 << BIOLOGICAL_DISASTER))
				{
					DrawStamp(&s);
				}
				s.frame = IncFrameIndex(s.frame);
				if (ShieldFlags & (1 << LIGHTNING_DISASTER))
				{
					DrawStamp(&s);
				}
				s.frame = IncFrameIndex(s.frame);
				if (ShieldFlags & (1 << LAVASPOT_DISASTER))
				{
					DrawStamp(&s);
				}
			}

			DrawMenuStateStrings(PM_FUEL, pMS->CurState);
			DrawFlagshipName(false, false);

			DrawFlagshipStats();

			ScreenTransition(uqm::UQMOptions::read().scrTrans, nullptr);

			PlayMusicResume(pMS->hMusic, NormalVolume);

			UnbatchGraphics();

			SetFlashRect(SfrMenu3do(), false);

			uqm::gameState().sis().FuelOnBoard =
				(uqm::gameState().sis().FuelOnBoard
				 + (FuelTankScale >> 1))
				/ FuelTankScale;
			uqm::gameState().sis().FuelOnBoard *= FuelTankScale;
		}

		SetContext(StatusContext);
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL]
			 || (PulsedInputState.menu[KEY_MENU_SELECT]
				 && pMS->CurState == OUTFIT_EXIT))
	{
		if (pMS->CurState == OUTFIT_DOFUEL)
		{
			pMS->CurState = OUTFIT_FUEL;
			DrawMenuStateStrings(PM_FUEL, pMS->CurState);
			SetFlashRect(SfrMenu3do(), false);
		}
		else
		{
ExitOutfit:
			if (pMS->CurState < OUTFIT_EXIT)
			{
				DrawMenuStateStrings(PM_FUEL, OUTFIT_EXIT);
			}
			DestroyDrawable(ReleaseDrawable(pMS->CurFrame));
			pMS->CurFrame = 0;
			DestroyDrawable(ReleaseDrawable(pMS->ModuleFrame));
			pMS->ModuleFrame = 0;

			// Release Fonts
			if (!IsDos())
			{
				DestroyFont(ModuleFont);
			}

			SetMusicPosition();

			SetNamingCallback(nullptr);

			return (false);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case OUTFIT_FUEL:
				{
					GFXRECT r;
					DrawMenuStateStrings(PM_FUEL, pMS->CurState);
					pMS->CurState = OUTFIT_DOFUEL;
					SetContext(StatusContext);
					GetGaugeRect(&r, false);
					SetFlashRect(&r, false);
					break;
				}
			case OUTFIT_DOFUEL:
				pMS->CurState = OUTFIT_FUEL;
				DrawMenuStateStrings(PM_FUEL, pMS->CurState);
				SetFlashRect(SfrMenu3do(), false);
				break;
			case OUTFIT_MODULES:

				if (DosMenu())
				{
					memset(&ModuleState, 0, sizeof ModuleState);
					ModuleState.count = InventoryModules(ModuleState.list,
														 NUM_PURCHASE_MODULES);
				}

				DrawMenuStateStrings(PM_FUEL, pMS->CurState);
				pMS->CurState = EmptySlot + 2;
				if (uqm::gameState().flag(uqm::GameFlag::CHMMR_BOMB_STATE) != 3)
				{
					pMS->delta_item = 0;
				}
				else
				{
					pMS->delta_item = NumBombModules;
				}
				pMS->first_item.y = 0;
				pMS->Initialized = 0;
				DoInstallModule(pMS);
				break;
			case OUTFIT_SAVELOAD:
				// Clearing FlashRect is not necessary
				if (!GameOptions())
				{
					goto ExitOutfit;
				}
				DrawMenuStateStrings(PM_FUEL, pMS->CurState);
				SetFlashRect(SfrMenu3do(), false);
				break;
		}
	}
	else
	{
		switch (pMS->CurState)
		{
			case OUTFIT_DOFUEL:
				SetMenuSounds(MenuSoundArrows,
							  MenuSoundPage | MenuSoundAction);
				break;
			default:
				SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
				break;
		}

		if (pMS->CurState == OUTFIT_DOFUEL)
		{
			if (!uqm::UQMOptions::read().infiniteFuel)
			{
				ChangeFuelQuantity();
			}

			SleepThread(c_gameSecond / 30);
		}
		else
		{
			DoMenuChooser(pMS, PM_FUEL);
		}
	}

	if (uqm::UQMOptions::read().infiniteFuel)
	{
		DeltaSISGauges(0, GetFuelTankCapacity(), 0);
		RedistributeFuel();
	}
	if (uqm::UQMOptions::read().infiniteRU)
	{
		uqm::gameState().sis().ResUnits = 1000000L;
	}

	return (true);
}