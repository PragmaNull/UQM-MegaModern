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

#include "melee.h"

#include "core/string/StringUtils.h"

#include "options.h"
#include "buildpick.h"
#include "meleeship.h"
#include "../battle.h"
#include "../build.h"
#include "../status.h"
#include "../colors.h"
#include "../comm.h"
// for getLineWithinWidth()
#include "../cons_res.h"
// for load_gravity_well() and free_gravity_well()
#include "../controls.h"
#include "../gamestr.h"
#include "../globdata.h"
#include "../intel.h"
#include "../master.h"
#include "../nameref.h"
#ifdef NETPLAY
#include "netplay/netconnection.h"
#include "netplay/netmelee.h"
#include "netplay/notify.h"
#include "netplay/notifyall.h"
#include "options/options.h"
#include "libs/graphics/widgets.h"
// for DrawShadowedBox()
#include "../cnctdlg.h"
// for MeleeConnectDialog()
#endif /* defined (NETPLAY) */
#include "../resinst.h"
#include "../settings.h"
#include "../setup.h"
#include "../sounds.h"
#include "../util.h"
// for DrawStarConBox()
#include "../planets/planets.h"
// for NUMBER_OF_PLANET_TYPES
#include "libs/gfxlib.h"
#include "libs/mathlib.h"
// for TFB_Random()
#include "libs/reslib.h"
#include "core/log/log.h"
#include "libs/file/textfile.h"
#include "uqm/supermelee/meleeconfig.h"
#include "libs/uio.h"


#include <assert.h>
#include <string.h>


static void StartMelee(MELEE_STATE* pMS);
#ifdef NETPLAY
static ssize_t numPlayersReady(void);
#endif /* NETPLAY */

enum
{
#ifdef NETPLAY
	NET_TOP,
#endif
	CONTROLS_TOP,
	SAVE_TOP,
	LOAD_TOP,
	START_MELEE,
	LOAD_BOT,
	SAVE_BOT,
	CONTROLS_BOT,
	//#ifdef NETPLAY
	//	NET_BOT,
	//#endif
	QUIT_BOT,
	EDIT_MELEE, // Editing a fleet or the team name
	BUILD_PICK	// Selecting a ship to add to a fleet
};

#ifdef NETPLAY
#define TOP_ENTRY NET_TOP
#else
#define TOP_ENTRY CONTROLS_TOP
#endif

// Top Melee Menu
#define MELEE_X_OFFS RES_SCALE(2 + DOS_NUM(2))
#define MELEE_Y_OFFS RES_SCALE(22 - DOS_NUM(1))
#define MELEE_BOX_WIDTH RES_SCALE(34)
#define MELEE_BOX_HEIGHT RES_SCALE(34)
#define MELEE_BOX_SPACE RES_SCALE(1)

#define MENU_X_OFFS RES_SCALE(29)

// Team names in main menu
#define TEAM_NAME_BOX_WIDTH RES_SCALE(245)
#define TEAM_NAME_BOX_HEIGHT RES_SCALE(13)
#define TEAM_NAME_BOX_X_OFFS RES_SCALE(1)
#define TEAM_NAME_BOX_Y_OFFS RES_SCALE(94)

#define INFO_ORIGIN_X RES_SCALE(4)
#define INFO_WIDTH RES_SCALE(58)
#define TEAM_INFO_ORIGIN_Y RES_SCALE(3)
#define TEAM_INFO_HEIGHT (SHIP_INFO_HEIGHT + RES_SCALE(75))
#define MODE_INFO_ORIGIN_Y (TEAM_INFO_HEIGHT + RES_SCALE(6))
#define MODE_INFO_HEIGHT ((STATUS_HEIGHT - RES_SCALE(3)) - MODE_INFO_ORIGIN_Y)
#define RACE_INFO_ORIGIN_Y (SHIP_INFO_HEIGHT + RES_SCALE(6))
#define RACE_INFO_HEIGHT ((STATUS_HEIGHT - RES_SCALE(3)) - RACE_INFO_ORIGIN_Y)

#define MELEE_STATUS_X_OFFS (RES_SCALE(1))
#define MELEE_STATUS_Y_OFFS RES_SCALE(201)
#define MELEE_STATUS_WIDTH (NUM_MELEE_COLUMNS * (MELEE_BOX_WIDTH + MELEE_BOX_SPACE))
#define MELEE_STATUS_HEIGHT RES_SCALE(38)

#define MELEE_BACKGROUND_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x14, 0x00, 0x00), 0x04)
#define MELEE_TITLE_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x1F, 0x0A, 0x0A), 0x0C)
#define MELEE_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x1F, 0x0A, 0x0A), 0x0C)
#define MELEE_TEAM_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x1F, 0x1F, 0x0A), 0x0E)

#define STATE_BACKGROUND_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x00, 0x00, 0x14), 0x01)
#define STATE_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x00, 0x14, 0x14), 0x03)
#define ACTIVE_STATE_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x0A, 0x1F, 0x1F), 0x0B)
#define UNAVAILABLE_STATE_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x0A, 0x0A, 0x1F), 0x09)
#define HI_STATE_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x0A, 0x1F, 0x1F), 0x0B)
#define HI_STATE_BACKGROUND_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x0A, 0x0A, 0x1F), 0x09)

// XXX: The following entries are unused:
#define LIST_INFO_BACKGROUND_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x14, 0x00, 0x14), 0x05)
#define LIST_INFO_TITLE_COLOR \
	WHITE_COLOR
#define LIST_INFO_TEXT_COLOR \
	LT_GRAY_COLOR
#define LIST_INFO_CURENTRY_TEXT_COLOR \
	WHITE_COLOR
#define HI_LIST_INFO_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x14, 0x00, 0x00), 0x04)
#define HI_LIST_INFO_BACKGROUND_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x1F, 0x0A, 0x1F), 0x0D)

#define TEAM_NAME_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x0F, 0x10, 0x1B), 0x00)
#define TEAM_NAME_TEXT_COLOR_DOS \
	BUILD_COLOR_RGBA(0xAA, 0x00, 0x00, 0xFF)
#define TEAM_NAME_EDIT_TEXT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x17, 0x18, 0x1D), 0x00)
#define TEAM_NAME_EDIT_TEXT_COLOR_DOS \
	BUILD_COLOR_RGBA(0xFF, 0x55, 0xFF, 0xFF)
#define TEAM_NAME_EDIT_RECT_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x14, 0x00, 0x14), 0x05)
#define TEAM_NAME_EDIT_CURS_COLOR \
	WHITE_COLOR

#define SHIPBOX_TOPLEFT_COLOR_NORMAL \
	BUILD_COLOR(MAKE_RGB15(0x00, 0x00, 0x09), 0x56)
#define SHIPBOX_BOTTOMRIGHT_COLOR_NORMAL \
	BUILD_COLOR(MAKE_RGB15(0x00, 0x00, 0x0E), 0x54)
#define SHIPBOX_INTERIOR_COLOR_NORMAL \
	BUILD_COLOR(MAKE_RGB15(0x00, 0x00, 0x0C), 0x55)

#define SHIPBOX_TOPLEFT_COLOR_HILITE \
	BUILD_COLOR(MAKE_RGB15(0x07, 0x00, 0x0C), 0x3E)
#define SHIPBOX_BOTTOMRIGHT_COLOR_HILITE \
	BUILD_COLOR(MAKE_RGB15(0x0C, 0x00, 0x14), 0x3C)
#define SHIPBOX_INTERIOR_COLOR_HILITE \
	BUILD_COLOR(MAKE_RGB15(0x0A, 0x00, 0x11), 0x3D)

#define MELEE_STATUS_COLOR \
	BUILD_COLOR(MAKE_RGB15(0x00, 0x14, 0x00), 0x02)

#define BUTTON_LABEL_COLOR \
	BUILD_SHADE_RGBA(0x8A)
#define BUTTON_LABEL_HILITE \
	BUILD_COLOR_RGBA(0xF8, 0xE0, 0x00, 0xFF)

#define BATTLE_TRACE_COLOR \
	BUILD_COLOR_RGBA(0x1F, 0x00, 0x1F, 0xFF)
#define BATTLE_TRACE_HL_COLOR \
	BUILD_COLOR_RGBA(0x3E, 0x00, 0x3E, 0xFF)

#define CONTROL_BOX_COLOR \
	BUILD_COLOR_RGBA(0x02, 0x04, 0x3E, 0xFF)
#define CONTROL_BOX_HL_COLOR \
	BUILD_COLOR_RGBA(0x04, 0x08, 0x7C, 0xFF)
#define CONTROL_TEXT_COLOR \
	BUILD_COLOR_RGBA(0x23, 0x8C, 0xD2, 0xFF)
#define CONTROL_TEXT_HL_COLOR \
	BUILD_COLOR_RGBA(0x28, 0xA0, 0xF0, 0xFF)
#define CONTROL_TEXT_TRACE_COLOR \
	BUILD_COLOR_RGBA(0x28, 0x28, 0x7C, 0xFF)
#define CONTROL_TEXT_TRACE_HL_COLOR \
	BUILD_COLOR_RGBA(0x3C, 0x3C, 0xBA, 0xFF)

#define SHIP_PICK_TEXT_SHADED_COLOR \
	BUILD_SHADE_RGBA(0x45)
#define SHIP_PICK_TEXT_COLOR \
	BUILD_SHADE_RGBA(0x74)

#define TEAM_PICK_TEXT_COLOR \
	BUILD_COLOR_RGB(0x32, 0x32, 0x9B)


// Loaded from melee/melebkgd.ani
FRAME MeleeFrame;
MELEE_STATE* pMeleeState;
FONT MicroThinFont;
FONT ButtonFont;

bool DoMelee(MELEE_STATE* pMS);
static bool DoEdit(MELEE_STATE* pMS);
static bool DoConfirmSettings(MELEE_STATE* pMS);

#define DTSHS_NORMAL 0
#define DTSHS_EDIT 1
#define DTSHS_SELECTED 2
#define DTSHS_REPAIR 4
#define DTSHS_BLOCKCUR 8
static bool DrawTeamString(MELEE_STATE* pMS, uint16_t side,
						   uint16_t HiLiteState, const char* str);
static void DrawFleetValue(MELEE_STATE* pMS, uint16_t side, uint16_t HiLiteState);

static void Melee_UpdateView_fleetValue(MELEE_STATE* pMS, uint16_t side);
static void Melee_UpdateView_ship(MELEE_STATE* pMS, uint16_t side,
								  FleetShipIndex index);
static void Melee_UpdateView_teamName(MELEE_STATE* pMS, uint16_t side);

static int
ButtonText(uint16_t which_icon)
{
	switch (which_icon)
	{
		case HUMAN_CON_TOP:
		case HUMAN_CON_TOP_HL:
		case HUMAN_CON_BOTT:
		case HUMAN_CON_BOTT_HL:
			return 10; // "HUMAN CONTROL"
		case WEAK_CYBORG_TOP:
		case WEAK_CYBORG_TOP_HL:
		case WEAK_CYBORG_BOTT:
		case WEAK_CYBORG_BOTT_HL:
			return 11; // "WEAK CYBORG"
		case GOOD_CYBORG_TOP:
		case GOOD_CYBORG_TOP_HL:
		case GOOD_CYBORG_BOTT:
		case GOOD_CYBORG_BOTT_HL:
			return 12; // "GOOD CYBORG"
		case AWES_CYBORG_TOP:
		case AWES_CYBORG_TOP_HL:
		case AWES_CYBORG_BOTT:
		case AWES_CYBORG_BOTT_HL:
			return 13; // "AWESOME CYBORG"
		case LOAD_BUTTON_TOP:
		case LOAD_BUTTON_TOP_HL:
		case LOAD_BUTTON_BOTT:
		case LOAD_BUTTON_BOTT_HL:
			return 14; // "LOAD"
		case SAVE_BUTTON_TOP:
		case SAVE_BUTTON_TOP_HL:
		case SAVE_BUTTON_BOTT:
		case SAVE_BUTTON_BOTT_HL:
			return 15; // "SAVE"
		case BATTLE_BUTTON:
		case BATTLE_BUTTON_HL:
			return 16; // "BATTLE!"
		case QUIT_BUTTON:
		case QUIT_BUTTON_HL:
			return 17; // "QUIT"
		case NETWORK_CON_TOP:
		case NETWORK_CON_TOP_HL:
		case NETWORK_CON_BOTT:
		case NETWORK_CON_BOTT_HL:
			return 18; // "NETWORK CONTROL"
		case NET_BUTTON_TOP:
		case NET_BUTTON_TOP_HL:
		case NET_BUTTON_BOTT:
		case NET_BUTTON_BOTT_HL:
			return 19; // "NET..."
		default:
			return 0;
	}
}

static void
DrawControlText(STAMP stamp, uint16_t which_icon, bool HiLite)
{
	GFXRECT r;
	TEXT t;
	FONT OldFont;
	int16_t leading;
	char buf[256];

	if (!ButtonText(which_icon))
	{
		return;
	}

	OldFont = SetContextFont(MicroThinFont);

	GetFrameRect(stamp.frame, &r);

	GetContextFontLeading(&leading);

	utf8StringCopy((char*)buf, sizeof(buf),
				   GAME_STRING(MELEE_STRING_BASE + ButtonText(which_icon)));

	t.align = ALIGN_CENTER;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + leading + RES_SCALE(4);

	{
		uqstl::vector<uqstl::string> tokens;
		uqm::tokenize(uqstl::string_view {(const char*)buf}, tokens, ' ', false);
		for (const auto& tok : tokens)
		{
			t.pStr = AlignText((const char*)tok.c_str(), &t.baseline.x);
			t.CharCount = (uint16_t)~0;

			font_DrawTracedText(&t,
								HiLite ? CONTROL_TEXT_HL_COLOR : CONTROL_TEXT_COLOR,
								HiLite ? CONTROL_TEXT_TRACE_HL_COLOR : CONTROL_TEXT_TRACE_COLOR);

			t.baseline.y += leading;
		}
	}

	SetContextFont(OldFont);
}

static void
DrawBattleText(STAMP stamp, uint16_t which_icon, bool HiLite)
{
	GFXRECT r;
	TEXT t;
	FONT OldFont;
	FRAME OldFontEffect;
	int16_t leading;
	char buf[256];

	if (!ButtonText(which_icon))
	{
		return;
	}

	OldFont = SetContextFont(LabelFont);

	GetFrameRect(stamp.frame, &r);

	GetContextFontLeading(&leading);

	utf8StringCopy((char*)buf, sizeof(buf),
				   GAME_STRING(MELEE_STRING_BASE + ButtonText(which_icon)));

	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + r.extent.height
				 - RES_SCALE(RES_DESCALE(leading) >> 1) - RES_SCALE(1);

	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	t.pStr = AlignText((const char*)buf, &t.baseline.x);

	font_DrawTracedText(&t, TRANSPARENT,
						HiLite ? BATTLE_TRACE_HL_COLOR : BATTLE_TRACE_COLOR);

	OldFontEffect = SetContextFontEffect(
		SetAbsFrameIndex(FontGradFrame, 8 + HiLite));

	font_DrawText(&t);

	SetContextFontEffect(OldFontEffect);

	SetContextFont(OldFont);
}

static void
DrawButtonText(STAMP stamp, uint16_t which_icon, bool HiLite)
{
	GFXRECT r;
	TEXT t;
	FONT OldFont;
	Color OldColor;
	char buf[256];

	if (!ButtonText(which_icon))
	{
		return;
	}

	OldFont = SetContextFont(ButtonFont);
	OldColor = SetContextForeGroundColor(
		HiLite ? BUTTON_LABEL_HILITE : BUTTON_LABEL_COLOR);

	GetFrameRect(stamp.frame, &r);

	utf8StringCopy(buf, sizeof(buf),
				   GAME_STRING(MELEE_STRING_BASE + ButtonText(which_icon)));

	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y;

	t.align = ALIGN_CENTER;
	t.CharCount = (uint16_t)~0;
	t.pStr = AlignText((const char*)buf, &t.baseline.x);

	font_DrawText(&t);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static void
DrawVerticalText(char* str, GFXPOINT point)
{
	TEXT t;
	uint16_t i;
	int16_t leading;
	size_t str_len;
	char buf[256];
	Color OldColor;

	GetContextFontLeading(&leading);

	t.baseline = point;

	utf8StringCopy(buf, sizeof(buf),
				   AlignText((const char*)AddPadd(
								 (const char*)str, &leading),
							 &t.baseline.x));

	str_len = strlen(buf);

	t.align = ALIGN_CENTER;
	t.CharCount = 1;
	t.pStr = buf;

	OldColor = GetContextForeGroundColor();

	for (i = 0; i < str_len; i++)
	{
		SetContextForeGroundColor(SHIP_PICK_TEXT_SHADED_COLOR);
		t.baseline.x -= RES_SCALE(1);
		font_DrawText(&t);

		SetContextForeGroundColor(SHIP_PICK_TEXT_COLOR);
		t.baseline.x += RES_SCALE(1);
		font_DrawText(&t);

		t.pStr = skipUTF8Chars(t.pStr, 1);
		t.baseline.y += leading;
	}

	SetContextForeGroundColor(OldColor);
}

#define SP_X_PADDING RES_SCALE(2)
#define SP_Y_PADDING RES_SCALE(3)

void DrawShipPickerText(STAMP stamp)
{
	GFXRECT r;
	FONT OldFont;
	uint16_t i;
	STAMP s;
	GFXPOINT pt;

	for (i = 0; i < 2; i++)
	{ // Check if we actually have text to print
		if (!strlen(GAME_STRING(MELEE_STRING_BASE + 23 + i)))
		{
			return;
		}
	}

	OldFont = SetContextFont(LabelFont);

	GetFrameRect(stamp.frame, &r);

	pt.x = r.corner.x + SP_X_PADDING;
	pt.y = r.corner.y + SP_Y_PADDING;

	s.frame = SetAbsFrameIndex(MeleeFrame,
							   CONFIRM_PC + (static_cast<int>(uqm::UQMOptions::read().optControllerType.value) * 4));
	s.origin = pt;

	DrawStamp(&s);

	GetFrameRect(s.frame, &r);

	pt.x = s.origin.x + (r.extent.width >> 1);
	pt.y = s.origin.y + r.extent.height + RES_SCALE(9);

	DrawVerticalText(GAME_STRING(MELEE_STRING_BASE + 23), pt);

	GetFrameRect(stamp.frame, &r);

	pt.x = r.corner.x + r.extent.width - SP_X_PADDING;
	pt.y = r.corner.y + SP_Y_PADDING;

	s.frame = SetAbsFrameIndex(MeleeFrame,
							   SPECIAL_PC + (static_cast<int>(uqm::UQMOptions::read().optControllerType.value) * 4));

	GetFrameRect(s.frame, &r);
	pt.x -= r.extent.width;
	s.origin = pt;

	DrawStamp(&s);

	pt.x = s.origin.x + (r.extent.width >> 1);
	pt.y = s.origin.y + r.extent.height + RES_SCALE(9);

	DrawVerticalText(GAME_STRING(MELEE_STRING_BASE + 24), pt);

	SetContextFont(OldFont);
}

#define TP_PADDING RES_SCALE(3)

static void
DrawTeamPickerText(STAMP stamp)
{
	GFXRECT r, text_r;
	GFXPOINT pt;
	TEXT t;
	FONT OldFont;
	Color OldColor;
	char buf[256];
	STAMP s;
	uint16_t i;
	int16_t leading;

	for (i = 0; i < 3; i++)
	{ // Check if we actually have text to print
		utf8StringCopy(buf, sizeof(buf),
					   GAME_STRING(MELEE_STRING_BASE + 20 + i));

		if (!strlen(buf))
		{
			return;
		}
	}

	OldFont = SetContextFont(LabelFont);
	OldColor = SetContextForeGroundColor(TEAM_PICK_TEXT_COLOR);

	GetFrameRect(stamp.frame, &r);
	GetContextFontLeading(&leading);

	pt.x = r.corner.x + TP_PADDING;
	pt.y = r.corner.y + r.extent.height - TP_PADDING;

	s.frame = SetAbsFrameIndex(MeleeFrame,
							   CONFIRM_PC + (static_cast<int>(uqm::UQMOptions::read().optControllerType.value) * 4));

	GetFrameRect(s.frame, &r);
	pt.y -= r.extent.height;
	s.origin = pt;

	DrawStamp(&s);

	utf8StringCopy(buf, sizeof(buf),
				   GAME_STRING(MELEE_STRING_BASE + 20));

	t.baseline.x = r.extent.width + pt.x + TP_PADDING;
	t.baseline.y = pt.y + leading - RES_SCALE(1);

	t.align = ALIGN_LEFT;
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;

	for (i = 0; i < 3; ++i)
	{
		font_DrawText(&t);
		text_r = font_GetTextRect(&t);

		if (i == 2)
		{
			break;
		}

		s.frame = SetAbsFrameIndex(MeleeFrame,
								   CANCEL_PC + i + (static_cast<int>(uqm::UQMOptions::read().optControllerType.value) * 4));
		s.origin.x = text_r.extent.width + t.baseline.x + TP_PADDING
				   + RES_SCALE(1);
		DrawStamp(&s);

		utf8StringCopy(buf, sizeof(buf),
					   GAME_STRING(MELEE_STRING_BASE + 21 + i));
		t.baseline.x = s.origin.x + r.extent.width + TP_PADDING;
		t.CharCount = (uint16_t)~0;
	}

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

static int
WhichText(uint16_t which_icon)
{
	switch (which_icon)
	{
		case HUMAN_CON_TOP:
		case HUMAN_CON_TOP_HL:
		case HUMAN_CON_BOTT:
		case HUMAN_CON_BOTT_HL:
		case WEAK_CYBORG_TOP:
		case WEAK_CYBORG_TOP_HL:
		case WEAK_CYBORG_BOTT:
		case WEAK_CYBORG_BOTT_HL:
		case GOOD_CYBORG_TOP:
		case GOOD_CYBORG_TOP_HL:
		case GOOD_CYBORG_BOTT:
		case GOOD_CYBORG_BOTT_HL:
		case AWES_CYBORG_TOP:
		case AWES_CYBORG_TOP_HL:
		case AWES_CYBORG_BOTT:
		case AWES_CYBORG_BOTT_HL:
		case NETWORK_CON_TOP:
		case NETWORK_CON_TOP_HL:
		case NETWORK_CON_BOTT:
		case NETWORK_CON_BOTT_HL:
			return 1;
		case LOAD_BUTTON_TOP:
		case LOAD_BUTTON_TOP_HL:
		case LOAD_BUTTON_BOTT:
		case LOAD_BUTTON_BOTT_HL:
		case SAVE_BUTTON_TOP:
		case SAVE_BUTTON_TOP_HL:
		case SAVE_BUTTON_BOTT:
		case SAVE_BUTTON_BOTT_HL:
		case QUIT_BUTTON:
		case QUIT_BUTTON_HL:
		case NET_BUTTON_TOP:
		case NET_BUTTON_TOP_HL:
		case NET_BUTTON_BOTT:
		case NET_BUTTON_BOTT_HL:
			return 3;
		case BATTLE_BUTTON:
		case BATTLE_BUTTON_HL:
			return 2;
		case TEAM_PICK_KB:
		case TEAM_PICK_XB:
		case TEAM_PICK_PS:
			return 4;
		default:
			return 0;
	}
}

// These icons come from ui/meleemenu.ani
void DrawMeleeIcon(uint16_t which_icon, bool HiLite)
{
	STAMP s;
	int which_text = WhichText(which_icon);
	bool NeedBatch = which_text && which_text < 3;

	if (NeedBatch)
	{
		BatchGraphics();
	}

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex(MeleeFrame, which_icon);
	DrawStamp(&s);

	if (!which_text)
	{
		return;
	}

	switch (which_text)
	{
		case 1:
			DrawControlText(s, which_icon, HiLite);
			break;
		case 2:
			DrawBattleText(s, which_icon, HiLite);
			break;
		case 3:
			DrawButtonText(s, which_icon, HiLite);
			break;
		case 4:
			DrawTeamPickerText(s);
			break;
		default:
			// should not happen
			break;
	}

	if (NeedBatch)
	{
		UnbatchGraphics();
	}
}

static FleetShipIndex
GetShipIndex(uint8_t row, uint8_t col)
{
	return row * NUM_MELEE_COLUMNS + col;
}

static uint8_t
GetShipRow(FleetShipIndex index)
{
	return index / NUM_MELEE_COLUMNS;
}

static uint8_t
GetShipColumn(int index)
{
	return index % NUM_MELEE_COLUMNS;
}

// Get the rectangle containing the ship slot for the specified side, row,
// and column.
void GetShipBox(GFXRECT* pRect, uint16_t side, uint16_t row, uint16_t col)
{
	pRect->corner.x = MELEE_X_OFFS
					+ (col * (MELEE_BOX_WIDTH + MELEE_BOX_SPACE));
	pRect->corner.y = MELEE_Y_OFFS
					+ (side * (MELEE_Y_OFFS + MELEE_BOX_SPACE + (NUM_MELEE_ROWS * (MELEE_BOX_HEIGHT + MELEE_BOX_SPACE))))
					+ (row * (MELEE_BOX_HEIGHT + MELEE_BOX_SPACE));
	pRect->extent.width = MELEE_BOX_WIDTH;
	pRect->extent.height = MELEE_BOX_HEIGHT;
}

static void
DrawShipBox(uint16_t side, FleetShipIndex index, MeleeShip ship, bool HiLite)
{
	GFXRECT r;
	uint8_t row = GetShipRow(index);
	uint8_t col = GetShipColumn(index);
	bool FilledSlot = (ship != MELEE_NONE);

	GetShipBox(&r, side, row, col);

	BatchGraphics();
	if (IS_HD)
	{ // Draw prerendered rectangles in HD
		STAMP s;
#define HD_SHIPBOX_START_INDEX (GetFrameCount(MeleeFrame) - 4)

		s.origin = r.corner;
		s.frame = SetAbsFrameIndex(MeleeFrame, HD_SHIPBOX_START_INDEX
												   + FilledSlot + (HiLite << 1));
		DrawStamp(&s);
	}
	else
	{
		if (HiLite)
		{
			DrawStarConBox(&r, 1,
						   SHIPBOX_TOPLEFT_COLOR_HILITE,
						   SHIPBOX_BOTTOMRIGHT_COLOR_HILITE,
						   FilledSlot, SHIPBOX_INTERIOR_COLOR_HILITE, false,
						   TRANSPARENT);
		}
		else
		{
			DrawStarConBox(&r, 1,
						   SHIPBOX_TOPLEFT_COLOR_NORMAL,
						   SHIPBOX_BOTTOMRIGHT_COLOR_NORMAL,
						   FilledSlot, SHIPBOX_INTERIOR_COLOR_NORMAL, false,
						   TRANSPARENT);
		}
	}

	if (FilledSlot)
	{
		STAMP s;
		s.origin.x = r.corner.x + (r.extent.width >> 1);
		s.origin.y = r.corner.y + (r.extent.height >> 1);
		s.frame = GetShipMeleeIconsFromIndex(ship);

		DrawStamp(&s);
	}
	UnbatchGraphics();
}

static void
ClearShipBox(uint16_t side, FleetShipIndex index)
{
	GFXRECT rect;
	uint8_t row = GetShipRow(index);
	uint8_t col = GetShipColumn(index);

	GetShipBox(&rect, side, row, col);
	RepairMeleeFrame(&rect);
}

static void
DrawShipBoxCurrent(MELEE_STATE* pMS, bool HiLite)
{
	FleetShipIndex slotI = GetShipIndex(pMS->row, pMS->col);
	MeleeShip ship = MeleeSetup_getShip(pMS->meleeSetup, pMS->side, slotI);
	DrawShipBox(pMS->side, slotI, ship, HiLite);
}

// Draw an image for one of the control method selection buttons.
static void
DrawControls(uint16_t which_side, bool HiLite)
{
	uint16_t which_icon;

	if (testFlag(PlayerControl[which_side], PlayerControlFlags::Network))
	{
		DrawMeleeIcon(35 + (HiLite ? 1 : 0) + 2 * (1 - which_side), HiLite);
		/* "Network Control" */
		return;
	}

	if (testFlag(PlayerControl[which_side], PlayerControlFlags::Human))
	{
		which_icon = 0;
	}
	else
	{
		switch (PlayerControl[which_side] & DifficultyFlagsMask)
		{
			case PlayerControlFlags::DifficultyStandard:
				which_icon = 1;
				break;
			case PlayerControlFlags::DifficultyGood:
				which_icon = 2;
				break;
			case PlayerControlFlags::DifficultyAwesome:
				which_icon = 3;
				break;
			default:
				// Should not happen. Satisfying compiler.
				which_icon = 0;
				break;
		}
	}

	DrawMeleeIcon(1 + (8 * (1 - which_side)) + (HiLite ? 4 : 0) + which_icon, HiLite);
}

static void
DrawTeams(void)
{
	uint16_t side;

	for (side = 0; side < NUM_SIDES; side++)
	{
		FleetShipIndex index;

		DrawControls(side, false);

		for (index = 0; index < MELEE_FLEET_SIZE; index++)
		{
			MeleeShip ship = MeleeSetup_getShip(pMeleeState->meleeSetup,
												side, index);

			if (index == TRUE_MELEE_FLEET_SIZE)
			{
				break;
			}

			DrawShipBox(side, index, ship, false);
		}

		DrawTeamString(pMeleeState, side, DTSHS_NORMAL, nullptr);
		DrawFleetValue(pMeleeState, side, DTSHS_NORMAL);
	}
}

void QuickRepair(uint16_t whichFrame, GFXRECT* pRect)
{
	GFXRECT r;
	GFXCONTEXT OldContext;
	GFXRECT OldRect;
	GFXPOINT oldOrigin;

	r.corner.x = pRect->corner.x;
	r.corner.y = pRect->corner.y;
	r.extent = pRect->extent;

	OldContext = SetContext(SpaceContext);
	GetContextClipRect(&OldRect);
	SetContextClipRect(&r);

	if (IS_PAD)
	{
		oldOrigin = SetContextOrigin(MAKE_POINT(r.corner.x, r.corner.y));
	}
	else
	{
		oldOrigin =
			SetContextOrigin(MAKE_POINT(-r.corner.x, -r.corner.y));
	}

	DrawMeleeIcon(whichFrame, false);

	SetContextOrigin(oldOrigin);
	SetContextClipRect(&OldRect);
	SetContext(OldContext);
}

static void
DrawSuperMeleeTitle(void)
{
	TEXT t;
	FONT OldFont;
	FRAME OldFontEffect;
	char* buf = GAME_STRING(MELEE_STRING_BASE + 9);

	if (strlen(buf) == 0)
	{
		return;
	}

	OldFont = SetContextFont(LoadFont(MELEE_TITLE_FONT));
	OldFontEffect = SetContextFontEffect(
		SetAbsFrameIndex(FontGradFrame, 7));

	t.baseline.x = (SCREEN_WIDTH >> 1) - (SAFE_NEG(1));
	t.baseline.y = RES_SCALE(4);
	t.align = ALIGN_CENTER;
	t.pStr = AlignText((const char*)buf, &t.baseline.x);
	t.CharCount = (uint16_t)~0;

	font_DrawText(&t);

	SetContextFont(OldFont);
	SetContextFontEffect(OldFontEffect);
}

void RepairMeleeFrame(const GFXRECT* pRect)
{
	GFXRECT r;
	GFXCONTEXT OldContext;
	GFXRECT OldRect;
	GFXPOINT oldOrigin;

	r.corner.x = pRect->corner.x + SAFE_X;
	r.corner.y = pRect->corner.y + SAFE_Y;
	r.extent = pRect->extent;
	if (r.corner.y & 1)
	{
		r.corner.y -= RES_SCALE(1);
		r.extent.height += RES_SCALE(1);
	}

	OldContext = SetContext(SpaceContext);
	GetContextClipRect(&OldRect);
	SetContextClipRect(&r);
	// Offset the origin so that we draw the correct gfx in the cliprect
	oldOrigin = SetContextOrigin(MAKE_POINT(-r.corner.x + SAFE_X,
											-r.corner.y + SAFE_Y));
	BatchGraphics();

	DrawMeleeIcon(MELEE_BACKGROUND, false); // Entire melee background

	DrawTeams();

	if (pRect->corner.x == 0 && pRect->corner.y == 0
		&& pRect->extent.width == SCREEN_WIDTH
		&& pRect->extent.height == SCREEN_HEIGHT)
	{ // Only draw these on a full screen redraw

		DrawSuperMeleeTitle();

		DrawMeleeIcon(LOAD_BUTTON_TOP, false);
		DrawMeleeIcon(SAVE_BUTTON_TOP, false);
		DrawMeleeIcon(LOAD_BUTTON_BOTT, false);
		DrawMeleeIcon(SAVE_BUTTON_BOTT, false);
		DrawMeleeIcon(QUIT_BUTTON, false);

#ifdef NETPLAY
		DrawMeleeIcon(NET_BUTTON_TOP, false);
		//DrawMeleeIcon (NET_BUTTON_BOTT, false);
#endif
		DrawMeleeIcon(BATTLE_BUTTON_HL, true);
	}

	if (pMeleeState->MeleeOption == BUILD_PICK)
	{
		DrawPickFrame(pMeleeState);
	}

	UnbatchGraphics();
	SetContextOrigin(oldOrigin);
	SetContextClipRect(&OldRect);
	SetContext(OldContext);

	DrawBorderPadding(0);
}

static void
RedrawMeleeFrame(void)
{
	GFXRECT r;

	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = SCREEN_WIDTH;
	r.extent.height = SCREEN_HEIGHT;

	RepairMeleeFrame(&r);
}

static void
GetTeamStringRect(uint16_t side, GFXRECT* r)
{
	r->corner.x = MELEE_X_OFFS - RES_SCALE(1);
	r->corner.y = (side + 1) * (MELEE_Y_OFFS + ((MELEE_BOX_HEIGHT + MELEE_BOX_SPACE) * NUM_MELEE_ROWS + RES_SCALE(2)));
	r->extent.width = NUM_MELEE_COLUMNS * (MELEE_BOX_WIDTH + MELEE_BOX_SPACE)
					- RES_SCALE(29);
	r->extent.height = RES_SCALE(13);
}

static void
GetFleetValueRect(uint16_t side, GFXRECT* r)
{
	r->corner.x = MELEE_X_OFFS
				+ NUM_MELEE_COLUMNS * (MELEE_BOX_WIDTH + MELEE_BOX_SPACE) - RES_SCALE(30);
	r->corner.y = (side + 1) * (MELEE_Y_OFFS + ((MELEE_BOX_HEIGHT + MELEE_BOX_SPACE) * NUM_MELEE_ROWS + RES_SCALE(2)));
	r->extent.width = RES_SCALE(29);
	r->extent.height = RES_SCALE(13);
}

static void
GetFullStringRect(uint16_t side, GFXRECT* r)
{
	r->extent.width = TEAM_NAME_BOX_WIDTH;
	r->extent.height = TEAM_NAME_BOX_HEIGHT;
	r->corner.x = TEAM_NAME_BOX_X_OFFS;
	r->corner.y = TEAM_NAME_BOX_Y_OFFS << side;
}

static void
DrawTeamStringsBackGround(uint16_t side)
{
	GFXRECT r;

	GetFullStringRect(side, &r);
	QuickRepair(0, &r);
}

static void
DrawFleetValue(MELEE_STATE* pMS, uint16_t side, uint16_t HiLiteState)
{
	GFXRECT r;
	TEXT rtText;
	char buf[30] {};
	uint16_t fleetValue;

	GetFleetValueRect(side, &r);

	if (HiLiteState == DTSHS_REPAIR)
	{
		RepairMeleeFrame(&r);
		return;
	}

	SetContextFont(MicroFont);

	fleetValue = MeleeSetup_getFleetValue(pMS->meleeSetup, side);
	fmt::format_to_n(buf, sizeof(buf) - 1, "{}", fleetValue);
	rtText.pStr = buf;
	rtText.align = ALIGN_RIGHT;
	rtText.CharCount = (uint16_t)~0;
	rtText.baseline.y = r.corner.y + r.extent.height - RES_SCALE(3);
	rtText.baseline.x = r.corner.x + r.extent.width;

	SetContextForeGroundColor(!(HiLiteState & DTSHS_SELECTED) ? DOS_BOOL(TEAM_NAME_TEXT_COLOR, TEAM_NAME_TEXT_COLOR_DOS) : DOS_BOOL(TEAM_NAME_EDIT_TEXT_COLOR, TEAM_NAME_EDIT_TEXT_COLOR_DOS));
	font_DrawText(&rtText);
}

// If teamName == nullptr, the team name is taken from pMS->meleeSetup
static bool
DrawTeamString(MELEE_STATE* pMS, uint16_t side, uint16_t HiLiteState,
			   const char* teamName)
{
	GFXRECT r;
	TEXT lfText;

	GetTeamStringRect(side, &r);
	if (HiLiteState == DTSHS_REPAIR)
	{
		RepairMeleeFrame(&r);
		return true;
	}

	SetContextFont(MicroFont);

	lfText.pStr = (teamName != nullptr) ? teamName :
										  MeleeSetup_getTeamName(pMS->meleeSetup, side);
	lfText.baseline.y = r.corner.y + r.extent.height - RES_SCALE(3);
	lfText.baseline.x = r.corner.x + RES_SCALE(1);
	lfText.align = ALIGN_LEFT;
	lfText.CharCount = (uint16_t)strlen(lfText.pStr);

	BatchGraphics();
	if (!(HiLiteState & DTSHS_EDIT))
	{ // normal or selected state
		SetContextForeGroundColor(!(HiLiteState & DTSHS_SELECTED) ? DOS_BOOL(TEAM_NAME_TEXT_COLOR, TEAM_NAME_TEXT_COLOR_DOS) : DOS_BOOL(TEAM_NAME_EDIT_TEXT_COLOR, TEAM_NAME_EDIT_TEXT_COLOR_DOS));
		font_DrawText(&lfText);
	}
	else
	{ // editing state
		uint16_t i;
		GFXRECT text_r;
		uint8_t char_deltas[MAX_TEAM_CHARS];
		uint8_t* pchar_deltas;

		TextRect(&lfText, &text_r, char_deltas);
#if 0
		if ((text_r.extent.width + RES_SCALE (2)) >= r.extent.width)
		{	// the text does not fit the input box size and so
			// will not fit when displayed later
			UnbatchGraphics ();
			// disallow the change
			return false;
		}
#endif

		text_r = r;
		SetContextForeGroundColor(TEAM_NAME_EDIT_RECT_COLOR);
		DrawFilledRectangle(&text_r);

		// calculate the cursor position and draw it
		pchar_deltas = char_deltas;
		for (i = pMS->CurIndex; i > 0; --i)
		{
			text_r.corner.x += (int16_t)*pchar_deltas++;
		}
		if (pMS->CurIndex < lfText.CharCount) /* cursor mid-line */
		{
			text_r.corner.x -= RES_SCALE(1);
		}

		if (HiLiteState & DTSHS_BLOCKCUR)
		{ // Use block cursor for keyboardless systems

			text_r.corner.y = r.corner.y + RES_SCALE(1);
			text_r.extent.height = r.extent.height - RES_SCALE(2);

			SetCursorFlashBlock(true);

			if (pMS->CurIndex == lfText.CharCount)
			{ // cursor at end-line -- use insertion point
				text_r.extent.width = RES_SCALE(1);
				text_r.corner.x -= IF_HD(3);
			}
			else if (pMS->CurIndex + 1 == lfText.CharCount)
			{ // extra pixel for last char margin
				text_r.extent.width = (int16_t)*pchar_deltas - IF_HD(3);
				text_r.corner.x += RES_SCALE(2);
			}
			else
			{ // normal mid-line char
				text_r.extent.width = (int16_t)*pchar_deltas;
				text_r.corner.x += RES_SCALE(2);
			}

			if (text_r.extent.width >= 200)
			{
				text_r.extent.width = RES_SCALE(1);
				text_r.corner.x -= IF_HD(3);
			}
			else
			{
				SetContextForeGroundColor(TEAM_NAME_EDIT_CURS_COLOR);
				DrawFilledRectangle(&text_r);
			}
		}
		else
		{ // Insertion point cursor
			text_r.corner.y = r.corner.y + RES_SCALE(3);
			text_r.extent.height = r.extent.height - RES_SCALE(6);
			text_r.extent.width = RES_SCALE(1);

			if (pMS->CurIndex == lfText.CharCount)
			{
				text_r.corner.x -= IF_HD(3);
			}

			SetCursorFlashBlock(false);
		}
		// position cursor within input field rect
		text_r.corner.x += RES_SCALE(1);

		SetCursorRect(&text_r, SpaceContext);

		SetContextForeGroundColor(DOS_BOOL(BLACK_COLOR,
										   TEAM_NAME_EDIT_TEXT_COLOR_DOS));
		font_DrawText(&lfText);
	}
	UnbatchGraphics();

	return true;
}

#ifdef NETPLAY
// This function is generic. It should probably be moved to elsewhere.
static void
multiLineDrawText(TEXT* textIn, GFXRECT* clipRect)
{
	GFXRECT oldRect;

	int16_t leading;
	TEXT text;
	int16_t lineWidth;

	GetContextClipRect(&oldRect);

	SetContextClipRect(clipRect);
	GetContextFontLeading(&leading);

	text = *textIn;
	text.baseline.x = RES_SCALE(1);
	text.baseline.y = 0;

	if (clipRect->extent.width <= text.baseline.x)
	{
		goto out;
	}

	lineWidth = clipRect->extent.width - text.baseline.x;

	while (*text.pStr != '\0')
	{
		const char* nextLine;

		text.baseline.y += leading;
		text.CharCount = (uint16_t)~0;
		getLineWithinWidth(&text, &nextLine, lineWidth, text.CharCount);
		// This will also fill in text->CharCount.

		font_DrawText(&text);

		text.pStr = nextLine;
	}

out:
	SetContextClipRect(&oldRect);
}

// Use an empty string to clear the status area.
static void
DrawMeleeStatusMessage(const char* message)
{
	GFXCONTEXT oldContext;
	GFXRECT r;

	oldContext = SetContext(SpaceContext);

	r.corner.x = MELEE_STATUS_X_OFFS;
	r.corner.y = MELEE_STATUS_Y_OFFS;
	r.extent.width = MELEE_STATUS_WIDTH;
	r.extent.height = MELEE_STATUS_HEIGHT;

	RepairMeleeFrame(&r);

	if (message[0] != '\0')
	{
		TEXT lfText;
		lfText.pStr = message;
		lfText.align = ALIGN_LEFT;
		lfText.CharCount = (uint16_t)~0;

		SetContextFont(MicroFont);
		SetContextForeGroundColor(MELEE_STATUS_COLOR);

		BatchGraphics();
		multiLineDrawText(&lfText, &r);
		UnbatchGraphics();
	}

	SetContext(oldContext);
}

static void
UpdateMeleeStatusMessage(ssize_t player)
{
	NetConnection* conn;

	assert(player == -1 || (player >= 0 && player < NUM_PLAYERS));

	if (player == -1)
	{
		DrawMeleeStatusMessage("");
		return;
	}

	conn = netConnections[player];
	if (conn == nullptr)
	{
		DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 0));
		// "Unconnected. Press LEFT to connect."
		return;
	}

	switch (NetConnection_getState(conn))
	{
		case NetState_unconnected:
			DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 0));
			// "Unconnected. Press LEFT to connect."
			break;
		case NetState_connecting:
			if (NetConnection_getPeerOptions(conn)->isServer)
			{
				DrawMeleeStatusMessage(
					GAME_STRING(NETMELEE_STRING_BASE + 1));
			}
			// "Awaiting incoming connection...\n"
			// "Press RIGHT to cancel."
			else
			{
				DrawMeleeStatusMessage(
					GAME_STRING(NETMELEE_STRING_BASE + 2));
			}
			// "Attempting outgoing connection...\n"
			// "Press RIGHT to cancel."
			break;
		case NetState_init:
		case NetState_inSetup:
			DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 3));
			// "Connected. Press RIGHT to disconnect."
			break;
		default:
			DrawMeleeStatusMessage("");
			break;
	}
}
#endif /* NETPLAY */

// XXX: this function is called when the current selection is blinking off.
static void
Deselect(uint8_t opt)
{
	switch (opt)
	{
		case START_MELEE:
			DrawMeleeIcon(BATTLE_BUTTON, false);
			// "Battle!" (not highlighted)
			break;
		case LOAD_TOP:
			DrawMeleeIcon(LOAD_BUTTON_TOP, false);
			// "Load" (top, not highlighted)
			break;
		case LOAD_BOT:
			DrawMeleeIcon(LOAD_BUTTON_BOTT, false);
			// "Load" (bottom, not highlighted)
			break;
		case SAVE_TOP:
			DrawMeleeIcon(SAVE_BUTTON_TOP, false);
			// "Save" (top, not highlighted)
			break;
		case SAVE_BOT:
			DrawMeleeIcon(SAVE_BUTTON_BOTT, false);
			// "Save" (bottom, not highlighted)
			break;
#ifdef NETPLAY
		case NET_TOP:
			DrawMeleeIcon(NET_BUTTON_TOP, false);
			// "Net..." (top, not highlighted)
			break;
			//case NET_BOT:
			//	DrawMeleeIcon (NET_BUTTON_BOTT, false);
			//		// "Net..." (bottom, not highlighted)
			//	break;
#endif
		case QUIT_BOT:
			DrawMeleeIcon(QUIT_BUTTON, false);
			// "Quit" (not highlighted)
			break;
		case CONTROLS_TOP:
		case CONTROLS_BOT:
			{
				uint16_t which_side;

				which_side = opt == CONTROLS_TOP ? 1 : 0;
				DrawControls(which_side, false);
				break;
			}
		case EDIT_MELEE:
			if (pMeleeState->InputFunc == DoEdit)
			{
				if (pMeleeState->row < NUM_MELEE_ROWS)
				{
					DrawShipBoxCurrent(pMeleeState, false);
				}
				else if (pMeleeState->CurIndex == MELEE_STATE_INDEX_DONE)
				{
					// Not currently editing the team name.
					if (IS_HD)
					{
						DrawTeamStringsBackGround(pMeleeState->side);
					}

					DrawTeamString(pMeleeState, pMeleeState->side,
								   DTSHS_NORMAL, nullptr);
					DrawFleetValue(pMeleeState, pMeleeState->side,
								   DTSHS_NORMAL);
				}
			}
			break;
		case BUILD_PICK:
			DrawPickIcon(pMeleeState->currentShip, false);
			break;
	}
}

// XXX: this function is called when the current selection is blinking off.
static void
Select(uint8_t opt)
{
	switch (opt)
	{
		case START_MELEE:
			DrawMeleeIcon(BATTLE_BUTTON_HL, true);
			/* "Battle!" (highlighted) */
			break;
		case LOAD_TOP:
			DrawMeleeIcon(LOAD_BUTTON_TOP_HL, true);
			/* "Load" (top, highlighted) */
			break;
		case LOAD_BOT:
			DrawMeleeIcon(LOAD_BUTTON_BOTT_HL, true);
			/* "Load" (bottom, highlighted) */
			break;
		case SAVE_TOP:
			DrawMeleeIcon(SAVE_BUTTON_TOP_HL, true);
			/* "Save" (top; highlighted) */
			break;
		case SAVE_BOT:
			DrawMeleeIcon(SAVE_BUTTON_BOTT_HL, true);
			/* "Save" (bottom; highlighted) */
			break;
#ifdef NETPLAY
		case NET_TOP:
			DrawMeleeIcon(NET_BUTTON_TOP_HL, true);
			/* "Net..." (top; highlighted) */
			break;
			//case NET_BOT:
			//	DrawMeleeIcon (NET_BUTTON_BOTT_HL, true);
			//		/* "Net..." (bottom; highlighted) */
			//	break;
#endif
		case QUIT_BOT:
			DrawMeleeIcon(QUIT_BUTTON_HL, true);
			/* "Quit" (highlighted) */
			break;
		case CONTROLS_TOP:
		case CONTROLS_BOT:
			{
				uint16_t which_side;

				which_side = (opt == CONTROLS_TOP) ? 1 : 0;
				DrawControls(which_side, true);
				break;
			}
		case EDIT_MELEE:
			if (pMeleeState->InputFunc == DoEdit)
			{
				if (pMeleeState->row < NUM_MELEE_ROWS)
				{
					DrawShipBoxCurrent(pMeleeState, true);
				}
				else if (pMeleeState->CurIndex == MELEE_STATE_INDEX_DONE)
				{
					// Not currently editing the team name.
					if (IS_HD)
					{
						DrawTeamStringsBackGround(pMeleeState->side);
					}

					DrawTeamString(pMeleeState, pMeleeState->side,
								   DTSHS_SELECTED, nullptr);
					DrawFleetValue(pMeleeState, pMeleeState->side,
								   DTSHS_SELECTED);
				}
			}
			break;
		case BUILD_PICK:
			DrawPickIcon(pMeleeState->currentShip, (true & is3DO(uqm::UQMOptions::read().whichMenu)));
			break;
	}
}

void Melee_flashSelection(MELEE_STATE* pMS)
{
#define FLASH_RATE (GameTicksPerSecond / 9)
#define BLINK_RATE (GameTicksPerSecond / 16)
	static TimeCount NextTime = 0;
	static bool select = false;
	TimeCount Now = GetTimeCounter();

	if (Now >= NextTime)
	{
		GFXCONTEXT OldContext;

		NextTime = Now + ((pMS->MeleeOption != BUILD_PICK || is3DO(uqm::UQMOptions::read().whichMenu)) ? FLASH_RATE : BLINK_RATE);
		select = !select;

		OldContext = SetContext(SpaceContext);
		if (select)
		{
			Select(pMS->MeleeOption);
		}
		else
		{
			Deselect(pMS->MeleeOption);
		}
		SetContext(OldContext);
	}
}

static void
InitMelee(MELEE_STATE* pMS)
{
	GFXRECT r;

	SetContext(SpaceContext);
	SetContextFGFrame(Screen);
	SetContextClipRect(nullptr);
	SetContextBackGroundColor(BLACK_COLOR);
	ClearDrawable();
	r.corner.x = SAFE_X;
	r.corner.y = SAFE_Y;
	r.extent.width = SCREEN_WIDTH - (SAFE_X << 1);
	r.extent.height = SCREEN_HEIGHT - (SAFE_Y << 1);
	SetContextClipRect(&r);

	r.corner.x = r.corner.y = 0;
	RedrawMeleeFrame();

	(void)pMS;
}

void DrawMeleeShipStrings(MELEE_STATE* pMS, MeleeShip NewStarShip)
{
	GFXRECT r, OldRect;
	GFXCONTEXT OldContext;

	OldContext = SetContext(StatusContext);
	GetContextClipRect(&OldRect);
	r = OldRect;
	r.corner.x -= NSAFE_NUM_SCL(3);
	r.corner.y += RES_SCALE(76);
	r.extent.height = SHIP_INFO_HEIGHT;
	SetContextClipRect(&r);
	BatchGraphics();

	if (NewStarShip == MELEE_NONE)
	{
		GFXRECT r;
		TEXT t;

		ClearShipStatus(0);

		SetContextFont(StarConFont);
		r.corner.x = RES_SCALE(3);
		r.corner.y = RES_SCALE(4);
		r.extent.width = RES_SCALE(57);
		r.extent.height = RES_SCALE(60);
		SetContextForeGroundColor(BLACK_COLOR);
		DrawRectangle(&r, IS_HD);
		t.baseline.x = STATUS_WIDTH >> 1;
		t.baseline.y = RES_SCALE(32);
		t.align = ALIGN_CENTER;
		if (pMS->row < NUM_MELEE_ROWS)
		{
			// A ship is selected (or an empty fleet position).
			t.pStr = GAME_STRING(MELEE_STRING_BASE + 0); // "Empty"
			t.CharCount = (uint16_t)~0;
			font_DrawText(&t);
			t.pStr = GAME_STRING(MELEE_STRING_BASE + 1); // "Slot"
		}
		else
		{
			// The team name is selected.
			t.pStr = GAME_STRING(MELEE_STRING_BASE + 2); // "Team"
			t.CharCount = (uint16_t)~0;
			font_DrawText(&t);
			t.pStr = GAME_STRING(MELEE_STRING_BASE + 3); // "Name"
		}
		t.baseline.y += TINY_TEXT_HEIGHT;
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
	}
	else
	{
		HMASTERSHIP hMasterShip;
		MASTER_SHIP_INFO* MasterPtr;

		hMasterShip = GetStarShipFromIndex(&master_q, NewStarShip);
		MasterPtr = LockMasterShip(&master_q, hMasterShip);

		InitShipStatus(&MasterPtr->ShipInfo, nullptr, nullptr, true);

		if (uqm::UQMOptions::read().meleeToolTips && pMS->MeleeOption == BUILD_PICK)
		{
			DrawTooltip(&MasterPtr->ShipInfo);
		}

		UnlockMasterShip(&master_q, hMasterShip);
	}

	UnbatchGraphics();
	SetContextClipRect(&OldRect);
	SetContext(OldContext);
}

// Set the currently displayed ship to the ship for the slot indicated by
// pMS->row and pMS->col.
static void
UpdateCurrentShip(MELEE_STATE* pMS)
{
	if (pMS->row == NUM_MELEE_ROWS)
	{
		// The team name is selected.
		pMS->currentShip = MELEE_NONE;
	}
	else
	{
		FleetShipIndex slotNr = GetShipIndex(pMS->row, pMS->col);
		pMS->currentShip =
			MeleeSetup_getShip(pMS->meleeSetup, pMS->side, slotNr);
	}

	DrawMeleeShipStrings(pMS, pMS->currentShip);
}

// returns (uint16_t) ~0 for an invalid ship.
uint16_t
GetShipValue(MeleeShip StarShip)
{
	uint16_t val;

	if (StarShip == MELEE_NONE)
	{
		return 0;
	}

	val = GetShipCostFromIndex(StarShip);
	if (val == 0)
	{
		val = (uint16_t)~0;
	}

	return val;
}

static void
DeleteCurrentShip(MELEE_STATE* pMS)
{
	FleetShipIndex slotI = GetShipIndex(pMS->row, pMS->col);
	Melee_LocalChange_ship(pMS, pMS->side, slotI, MELEE_NONE);
}

static bool
isShipSlotSelected(MELEE_STATE* pMS, uint16_t side, FleetShipIndex index)
{
	if (pMS->MeleeOption != EDIT_MELEE)
	{
		return false;
	}

	if (pMS->side != side)
	{
		return false;
	}

	return (index == GetShipIndex(pMS->row, pMS->col));
}

static void
AdvanceCursor(MELEE_STATE* pMS)
{
	++pMS->col;
	if (pMS->col == NUM_MELEE_COLUMNS)
	{
		++pMS->row;
		if (pMS->row < NUM_MELEE_ROWS)
		{
			pMS->col = 0;
		}
		else
		{
			pMS->col = NUM_MELEE_COLUMNS - 1;
			pMS->row = NUM_MELEE_ROWS - 1;
		}
	}
}

static bool
OnTeamNameChange(TEXTENTRY_STATE* pTES)
{
	MELEE_STATE* pMS = (MELEE_STATE*)pTES->CbParam;
	bool ret;
	uint16_t hl = DTSHS_EDIT;

	pMS->CurIndex = pTES->CursorPos;
	if (pTES->JoystickMode)
	{
		hl |= DTSHS_BLOCKCUR;
	}

	ret = DrawTeamString(pMS, pMS->side, hl, pTES->BaseStr);

	return ret;
}

static bool
TeamNameFrameCallback(TEXTENTRY_STATE* pTES)
{
#ifdef NETPLAY
	// Process incoming packets, so that remote changes are displayed
	// while we are editing the team name.
	// The team name itself isn't modified visually due to remote changes
	// while it is being edited.
	netInput();
#endif

	(void)pTES;

	return true;
	// Keep editing
}

static void
BuildPickShipPopup(MELEE_STATE* pMS)
{
	bool buildOk;

	pMS->MeleeOption = BUILD_PICK;

	buildOk = BuildPickShip(pMS);
	if (buildOk)
	{
		// A ship has been selected.
		// Add the currently selected ship to the fleet.
		FleetShipIndex index = GetShipIndex(pMS->row, pMS->col);
		Melee_LocalChange_ship(pMS, pMS->side, index, pMS->currentShip);
		AdvanceCursor(pMS);
	}

	pMS->MeleeOption = EDIT_MELEE;
	// Must set this before the call to RepairMeleeFrame(), so that
	// it will not redraw the BuildPickFrame.

	{
		GFXRECT r;

		GetBuildPickFrameRect(&r);
		RepairMeleeFrame(&r);

		if (uqm::UQMOptions::read().meleeToolTips)
		{
			GetToolTipFrameRect(&r);
			RepairMeleeFrame(&r);
		}
	}

	UpdateCurrentShip(pMS);
	pMS->InputFunc = DoEdit;
}

static bool
DoEdit(MELEE_STATE* pMS)
{
	uint32_t TimeIn = GetTimeCounter();

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false;
	}

	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT | MENU_SOUND_DELETE);
	if (!pMS->Initialized)
	{
		UpdateCurrentShip(pMS);
		pMS->Initialized = true;
		pMS->InputFunc = DoEdit;
		return true;
	}

#ifdef NETPLAY
	netInput();
#endif
	if ((pMS->row < NUM_MELEE_ROWS || pMS->currentShip == MELEE_NONE)
		&& (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| (PulsedInputState.menu[KEY_MENU_RIGHT]
				&& (pMS->col == NUM_MELEE_COLUMNS - 1
					|| pMS->row == NUM_MELEE_ROWS))))
	{
		// Done editing the teams.
		Deselect(EDIT_MELEE);
		pMS->currentShip = MELEE_NONE;
		pMS->MeleeOption = START_MELEE;
		pMS->InputFunc = DoMelee;
		pMS->LastInputTime = GetTimeCounter();
	}
	else if (pMS->row < NUM_MELEE_ROWS
			 && PulsedInputState.menu[KEY_MENU_SELECT])
	{
		// Show a popup to add a new ship to the current team.
		Select(EDIT_MELEE);
		BuildPickShipPopup(pMS);
	}
	else if (pMS->row < NUM_MELEE_ROWS
			 && PulsedInputState.menu[KEY_MENU_SPECIAL])
	{
		// TODO: this is a stub; Should we display a ship spin?
		Deselect(EDIT_MELEE);
		if (pMS->currentShip != MELEE_NONE)
		{
			// Do something with pMS->currentShip here
		}
	}
	else if (pMS->row < NUM_MELEE_ROWS && PulsedInputState.menu[KEY_MENU_DELETE])
	{
		// Remove the currently selected ship from the current team.
		Deselect(EDIT_MELEE);
		DeleteCurrentShip(pMS);
		AdvanceCursor(pMS);
		UpdateCurrentShip(pMS);
	}
	else
	{
		uint16_t side = pMS->side;
		uint16_t row = pMS->row;
		uint16_t col = pMS->col;

		if (row == NUM_MELEE_ROWS)
		{
			// Edit the name of the current team.
			if (PulsedInputState.menu[KEY_MENU_SELECT])
			{
				TEXTENTRY_STATE tes;
				char buf[MAX_TEAM_CHARS + 1] {};

				// going to enter text
				pMS->CurIndex = 0;
				DrawTeamString(pMS, pMS->side, DTSHS_EDIT, nullptr);

				uqm::strncpy_safe(buf, MeleeSetup_getTeamName(pMS->meleeSetup, pMS->side));
				buf[MAX_TEAM_CHARS] = '\0';

				tes.Initialized = false;
				tes.BaseStr = buf;
				tes.CursorPos = 0;
				tes.MaxSize = MAX_TEAM_CHARS + 1;
				tes.CbParam = pMS;
				tes.ChangeCallback = OnTeamNameChange;
				tes.FrameCallback = TeamNameFrameCallback;
				DoTextEntry(&tes);

				// done entering
				pMS->CurIndex = MELEE_STATE_INDEX_DONE;
				if (!tes.Success || !Melee_LocalChange_teamName(pMS, pMS->side, buf))
				{
					// The team name was not changed, so it was not redrawn.
					// However, because we now leave edit mode, we still
					// need to redraw.
					Melee_UpdateView_teamName(pMS, pMS->side);
				}

				return true;
			}
		}

		{
			if (PulsedInputState.menu[KEY_MENU_LEFT])
			{
				if (col > 0)
				{
					--col;
				}
			}
			else if (PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				if (col < NUM_MELEE_COLUMNS - 1)
				{
					++col;
				}
			}

			if (PulsedInputState.menu[KEY_MENU_UP])
			{
				if (row-- == 0)
				{
					if (side == 0)
					{
						row = 0;
					}
					else
					{
						row = NUM_MELEE_ROWS;
						side = side ? 0 : 1;
					}
				}
			}
			else if (PulsedInputState.menu[KEY_MENU_DOWN])
			{
				if (row++ == NUM_MELEE_ROWS)
				{
					if (side == 1)
					{
						row = NUM_MELEE_ROWS;
					}
					else
					{
						row = 0;
						side = side ? 0 : 1;
					}
				}
			}
		}

		if (col != pMS->col || row != pMS->row || side != pMS->side)
		{
			Deselect(EDIT_MELEE);
			pMS->side = side;
			pMS->row = row;
			pMS->col = col;

			UpdateCurrentShip(pMS);
		}
	}

#ifdef NETPLAY
	flushPacketQueues();
#endif

	Melee_flashSelection(pMS);

	SleepThreadUntil(TimeIn + GameTicksPerSecond / 30);

	return true;
}

#ifdef NETPLAY
// Returns -1 if a connection has been aborted.
static ssize_t
numPlayersReady(void)
{
	size_t player;
	size_t numDone;

	numDone = 0;
	for (player = 0; player < NUM_PLAYERS; player++)
	{
		if (!testFlag(PlayerControl[player], PlayerControlFlags::Network))
		{
			numDone++;
			continue;
		}

		{
			NetConnection* conn;

			conn = netConnections[player];

			if (conn == nullptr || !NetConnection_isConnected(conn))
			{
				return -1;
			}

			if (NetConnection_getState(conn) > NetState_inSetup)
			{
				numDone++;
			}
		}
	}

	return numDone;
}
#endif /* NETPLAY */

// The player has pressed "Start Game", and all Network players are
// connected. We're now just waiting for the confirmation of the other
// party.
// When the other party changes something in the settings, the confirmation
// is cancelled.
static bool
DoConfirmSettings(MELEE_STATE* pMS)
{
#ifdef NETPLAY
	ssize_t numDone;
#endif

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		// The connection is explicitely cancelled, locally.
		pMS->InputFunc = DoMelee;
#ifdef NETPLAY
		cancelConfirmations();
		DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 4));
		// "Confirmation cancelled. Press FIRE to reconfirm."
#endif
		return true;
	}

	if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_DOWN])
	{
		// The player moves the cursor; cancel the confirmation.
		pMS->InputFunc = DoMelee;
#ifdef NETPLAY
		cancelConfirmations();
		DrawMeleeStatusMessage("");
#endif
		return DoMelee(pMS);
		// Let the pressed keys take effect immediately.
	}

#ifndef NETPLAY
	pMS->InputFunc = DoMelee;
	SeedRandomNumbers();
	pMS->meleeStarted = true;
	StartMelee(pMS);
	pMS->meleeStarted = false;
	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false;
	}
	return true;
#else
	closeDisconnectedConnections();
	netInput();
	SleepThread(GameTicksPerSecond / 120);

	numDone = numPlayersReady();
	if (numDone == -1)
	{
		// Connection aborted
		cancelConfirmations();
		flushPacketQueues();
		pMS->InputFunc = DoMelee;
		return true;
	}
	else if (numDone != NUM_SIDES)
	{
		// Still waiting for some confirmation.
		return true;
	}

	// All sides have confirmed.

	// Send our own prefered frame delay.
	Netplay_NotifyAll_inputDelay(uqm::UQMOptions::getInstance().getNetplayOptions().getFrameInputDelay());

	// Synchronise the RNGs:
	{
		uint16_t player;

		for (player = 0; player < NUM_PLAYERS; player++)
		{
			NetConnection* conn;

			if (!testFlag(PlayerControl[player], PlayerControlFlags::Network))
			{
				continue;
			}

			conn = netConnections[player];
			assert(conn != nullptr);

			if (!NetConnection_isConnected(conn))
			{
				continue;
			}

			if (NetConnection_getDiscriminant(conn))
			{
				Netplay_Notify_seedRandom(conn, SeedRandomNumbers());
			}
		}
		flushPacketQueues();
	}

	{
		// One side will send the seed followed by 'Done' and wait
		// for the other side to report 'Done'.
		// The other side will report 'Done' and will wait for the other
		// side to report 'Done', but before the reception of 'Done'
		// it will have received the seed.
		bool allOk = negotiateReadyConnections(true, NetState_interBattle);
		if (!allOk)
		{
			return false;
		}
	}

	// The maximum value for all connections is used.
	{
		bool ok = setupInputDelay(uqm::UQMOptions::getInstance().getNetplayOptions().getFrameInputDelay());
		if (!ok)
		{
			return false;
		}
	}

	pMS->InputFunc = DoMelee;

	StartMelee(pMS);
	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false;
	}

	return true;
#endif /* defined (NETPLAY) */
}

static void
LoadMeleeInfo(MELEE_STATE* pMS)
{
	BuildPickMeleeFrame();
	MeleeFrame = CaptureDrawable(LoadGraphic(MELEE_SCREEN_PMAP_ANIM));
	BuildBuildPickFrame();
	MicroThinFont = LoadFont(MICRO_THIN_FONT);
	ButtonFont = LoadFont(BUTTON_FONT);
	FontGradFrame = CaptureDrawable(
		LoadGraphic(FONTGRAD_PMAP_ANIM));

	InitSpace();

	LoadTeamList(pMS);
}

static void
FreeMeleeInfo(MELEE_STATE* pMS)
{
	DestroyDirEntryTable(ReleaseDirEntryTable(pMS->load.dirEntries));
	pMS->load.dirEntries = 0;

	if (pMS->hMusic)
	{
		DestroyMusic(pMS->hMusic);
		pMS->hMusic = 0;
	}

	UninitSpace();

	DestroyPickMeleeFrame();
	DestroyDrawable(ReleaseDrawable(MeleeFrame));
	MeleeFrame = 0;

	DestroyBuildPickFrame();
	DestroyFont(MicroThinFont);
	DestroyFont(ButtonFont);
	DestroyDrawable(ReleaseDrawable(FontGradFrame));
	FontGradFrame = 0;

#ifdef NETPLAY
	closeAllConnections();
	// Clear the input delay in case we will go into the full game later.
	// Must be done after the net connections are closed.
	setupInputDelay(0);
#endif
}

static void
BuildAndDrawShipList(MELEE_STATE* pMS)
{
	FillPickMeleeFrame(pMS->meleeSetup);
	// XXX TODO: This also builds the race_q for each player.
	// This should be split off.
}

static void
StartMelee(MELEE_STATE* pMS)
{
	{
		FadeMusic(0, GameTicksPerSecond / 2);
		SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2)
						 + GameTicksPerSecond / 60);
		FlushColorXForms();

		SetMusicPosition();
		StopMusic();
	}
	FadeMusic(NORMAL_VOLUME, 0);
	if (pMS->hMusic)
	{
		DestroyMusic(pMS->hMusic);
		pMS->hMusic = 0;
	}

	do
	{
		if (!SetPlayerInputAll())
		{
			break;
		}
		BuildAndDrawShipList(pMS);

		WaitForSoundEnd(TFBSOUND_WAIT_ALL);

		load_gravity_well((uint8_t)((uint16_t)TFB_Random() % NUMBER_OF_PLANET_TYPES));
		Battle(nullptr);
		free_gravity_well();
		ClearPlayerInputAll();

		if (GLOBAL(CurrentActivity) & CHECK_ABORT)
		{
			return;
		}

		SleepThreadUntil(FadeScreen(FadeAllToBlack, GameTicksPerSecond / 2)
						 + GameTicksPerSecond / 60);
		FlushColorXForms();

	} while (0 /* !(GLOBAL (CurrentActivity) & CHECK_ABORT) */);
	GLOBAL(CurrentActivity) = SUPER_MELEE;

	pMS->Initialized = false;
}

static void
StartMeleeButtonPressed(MELEE_STATE* pMS)
{
	// Either fleet must at least have one ship.
	if (MeleeSetup_getFleetValue(pMS->meleeSetup, 0) == 0 || MeleeSetup_getFleetValue(pMS->meleeSetup, 1) == 0)
	{
		PlayMenuSound(MENU_SOUND_FAILURE);
		return;
	}

#ifdef NETPLAY
	if (testFlag(PlayerControl[0] & PlayerControl[1], PlayerControlFlags::Network))
	{
		DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 32));
		// "Only one side at a time can be network controlled."
		return;
	}

	if ((testFlag(PlayerControl[0], PlayerControlFlags::Network) && testFlag(PlayerControl[1], ComputerControlFlags))
		|| (testFlag(PlayerControl[1], PlayerControlFlags::Network) && testFlag(PlayerControl[0], ComputerControlFlags)))
	{
		DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 33));
		// "Netplay with a computer-controlled side is currently
		// not possible."
		return;
	}

	// Check whether all network parties are ready;
	{
		uint16_t player;
		bool netReady = true;

		// We collect all error conditions, instead of only reporting
		// the first one.
		for (player = 0; player < NUM_PLAYERS; player++)
		{
			NetConnection* conn;

			if (!testFlag(PlayerControl[player], PlayerControlFlags::Network))
			{
				continue;
			}

			conn = netConnections[player];
			if (conn == nullptr || !NetConnection_isConnected(conn))
			{
				// Connection for player not established.
				netReady = false;
				if (player == 0)
				{
					DrawMeleeStatusMessage(
						GAME_STRING(NETMELEE_STRING_BASE + 5));
				}
				// "Connection for bottom player not "
				// "established."
				else
				{
					DrawMeleeStatusMessage(
						GAME_STRING(NETMELEE_STRING_BASE + 6));
				}
				// "Connection for top player not "
				// "established."
			}
			else if (NetConnection_getState(conn) != NetState_inSetup)
			{
				// This side may be in the setup, but the network connection
				// is not in a state that setup information can be sent.
				netReady = false;
				if (player == 0)
				{
					DrawMeleeStatusMessage(
						GAME_STRING(NETMELEE_STRING_BASE + 14));
				}
				// "Connection for bottom player not ready."
				else
				{
					DrawMeleeStatusMessage(
						GAME_STRING(NETMELEE_STRING_BASE + 15));
				}
				// "Connection for top player not ready."
			}
		}
		if (!netReady)
		{
			PlayMenuSound(MENU_SOUND_FAILURE);
			return;
		}

		if (numPlayersReady() != NUM_PLAYERS)
		{
			DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 7));
		}
		// "Waiting for remote confirmation."
		confirmConnections();
	}
#endif

	pMS->InputFunc = DoConfirmSettings;
}

#ifdef NETPLAY

static bool
DoConnectingDialog(MELEE_STATE* pMS)
{
	uint32_t TimeIn = GetTimeCounter();
	uint16_t which_side = (pMS->MeleeOption == NET_TOP) ? 1 : 0;
	NetConnection* conn;

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false;
	}

	SetMenuSounds(MENU_SOUND_NONE, MENU_SOUND_NONE);
	if (!pMS->Initialized)
	{
		GFXRECT r;
		FONT oldfont;
		Color oldcolor;
		TEXT t;

		// Build a network connection.
		if (netConnections[which_side] != nullptr)
		{
			closePlayerNetworkConnection(which_side);
		}

		pMS->Initialized = true;
		conn = openPlayerNetworkConnection(which_side, pMS);
		pMS->InputFunc = DoConnectingDialog;

		/* Draw the dialog box here */
		oldfont = SetContextFont(StarConFont);
		oldcolor = SetContextForeGroundColor(BLACK_COLOR);
		BatchGraphics();
		r.extent.width = RES_SCALE(200);
		r.extent.height = RES_SCALE(30);
		r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
		r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
		DrawShadowedBox(&r, SHADOWBOX_BACKGROUND_COLOR,
						SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

		if (NetConnection_getPeerOptions(conn)->isServer)
		{
			t.pStr = GAME_STRING(NETMELEE_STRING_BASE + 1);
			/* "Awaiting incoming connection */
		}
		else
		{
			t.pStr = GAME_STRING(NETMELEE_STRING_BASE + 2);
			/* "Awaiting outgoing connection */
		}
		t.baseline.y = r.corner.y + RES_SCALE(10);
		t.baseline.x = SCREEN_WIDTH >> 1;
		t.align = ALIGN_CENTER;
		t.CharCount = ~0;
		font_DrawText(&t);

		t.pStr = GAME_STRING(NETMELEE_STRING_BASE + 18);
		/* "Press SPACE to cancel" */
		t.baseline.y += RES_SCALE(16);
		font_DrawText(&t);

		// Restore original graphics
		SetContextFont(oldfont);
		SetContextForeGroundColor(oldcolor);
		UnbatchGraphics();
	}

	netInput();

	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		// Terminate a network connection.
		if (netConnections[which_side] != nullptr)
		{
			closePlayerNetworkConnection(which_side);
			UpdateMeleeStatusMessage(which_side);
		}
		RedrawMeleeFrame();
		pMS->InputFunc = DoMelee;
		pMS->LastInputTime = GetTimeCounter();

		flushPacketQueues();

		return true;
	}

	conn = netConnections[which_side];
	if (conn != nullptr)
	{
		NetState status = NetConnection_getState(conn);
		if ((status == NetState_init) || (status == NetState_inSetup))
		{
			/* Connection complete! */
			PlayerControl[which_side] = PlayerControlFlags::Network | PlayerControlFlags::DifficultyStandard;
			DrawControls(which_side, true);

			RedrawMeleeFrame();

			UpdateMeleeStatusMessage(which_side);
			pMS->InputFunc = DoMelee;
			pMS->LastInputTime = GetTimeCounter();
			Deselect(pMS->MeleeOption);
			pMS->MeleeOption = START_MELEE;
		}
	}

	flushPacketQueues();

	SleepThreadUntil(TimeIn + GameTicksPerSecond / 30);

	return true;
}

/* Check for disconnects, and revert to human control if there is one */
static void
check_for_disconnects(MELEE_STATE* pMS)
{
	uint16_t player;

	for (player = 0; player < NUM_PLAYERS; player++)
	{
		NetConnection* conn;

		if (!testFlag(PlayerControl[player], PlayerControlFlags::Network))
		{
			continue;
		}

		conn = netConnections[player];
		if (conn == nullptr || !NetConnection_isConnected(conn))
		{
			PlayerControl[player] = PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard;
			DrawControls(player, false);
			uqm::log::info("Player {} has disconnected; shifting controls", player);
		}
	}

	(void)pMS;
}

#endif

static void
nextControlType(uint16_t which_side)
{
	const PlayerControlFlags human {PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard};
	const PlayerControlFlags current {PlayerControl[which_side]};
	PlayerControlFlags& control {PlayerControl[which_side]};
	if (current == human)
	{
		control = ComputerControlFlags | PlayerControlFlags::DifficultyStandard;
	}
	else if (current == (ComputerControlFlags | PlayerControlFlags::DifficultyStandard))
	{
		control = ComputerControlFlags | PlayerControlFlags::DifficultyGood;
	}
	else if (current == (ComputerControlFlags | PlayerControlFlags::DifficultyGood))
	{
		control = ComputerControlFlags | PlayerControlFlags::DifficultyAwesome;
	}
	else if (current == (ComputerControlFlags | PlayerControlFlags::DifficultyAwesome))
	{
		control = human;
	}
#ifdef NETPLAY
	else if (current == (PlayerControlFlags::Network | PlayerControlFlags::DifficultyStandard))
	{
		if (netConnections[which_side] != nullptr)
		{
			closePlayerNetworkConnection(which_side);
		}
		UpdateMeleeStatusMessage(-1);
		control = human;
	}
#endif /* NETPLAY */
	else
	{
		uqm::log::error("Error: Bad control type ({}) in "
						"nextControlType().\n",
						current);
		control = human;
	}

	DrawControls(which_side, true);
}

static MELEE_OPTIONS
MeleeOptionDown(MELEE_OPTIONS current)
{
	if (current == QUIT_BOT)
	{
		return TOP_ENTRY;
	}
	return current + 1;
}

static MELEE_OPTIONS
MeleeOptionUp(MELEE_OPTIONS current)
{
	if (current == TOP_ENTRY)
	{
		return QUIT_BOT;
	}
	return current - 1;
}

static void
MeleeOptionSelect(MELEE_STATE* pMS)
{
	switch (pMS->MeleeOption)
	{
		case START_MELEE:
			StartMeleeButtonPressed(pMS);
			break;
		case LOAD_TOP:
		case LOAD_BOT:
			pMS->Initialized = false;
			pMS->side = pMS->MeleeOption == LOAD_TOP ? 0 : 1;
			DoLoadTeam(pMS);
			break;
		case SAVE_TOP:
		case SAVE_BOT:
			pMS->side = pMS->MeleeOption == SAVE_TOP ? 0 : 1;
			if (MeleeSetup_getFleetValue(pMS->meleeSetup, pMS->side) > 0)
			{
				DoSaveTeam(pMS);
			}
			else
			{
				PlayMenuSound(MENU_SOUND_FAILURE);
			}
			break;
		case QUIT_BOT:
			GLOBAL(CurrentActivity) |= CHECK_ABORT;
			break;
#ifdef NETPLAY
		case NET_TOP:
			//case NET_BOT:
			{
				uint16_t which_side;
				bool confirmed;

				which_side = pMS->MeleeOption == NET_TOP ? 1 : 0;
				confirmed = MeleeConnectDialog(which_side);
				RedrawMeleeFrame();
				pMS->LastInputTime = GetTimeCounter();
				if (confirmed)
				{
					pMS->Initialized = false;
					pMS->InputFunc = DoConnectingDialog;
				}
				break;
			}
#endif /* NETPLAY */
		case CONTROLS_TOP:
		case CONTROLS_BOT:
			{
				uint16_t which_side = (pMS->MeleeOption == CONTROLS_TOP) ? 1 : 0;
				nextControlType(which_side);
				break;
			}
	}
}

bool DoMelee(MELEE_STATE* pMS)
{
	uint32_t TimeIn = GetTimeCounter();
	bool force_select = false;

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false;
	}

	SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	if (!pMS->Initialized)
	{
		if (pMS->hMusic)
		{
			StopMusic();
			DestroyMusic(pMS->hMusic);
			pMS->hMusic = 0;
		}
		pMS->hMusic = 0;
		pMS->Initialized = true;

		pMS->MeleeOption = START_MELEE;

		if (uqm::UQMOptions::read().mainMenuMusic)
		{
			pMS->hMusic = LoadMusic(MELEE_MUSIC);

			PlayMusicResume(pMS->hMusic, NORMAL_VOLUME);
		}

		InitMelee(pMS);

		FadeScreen(FadeAllToColor, GameTicksPerSecond / 2);
		pMS->LastInputTime = GetTimeCounter();
		return true;
	}

#ifdef NETPLAY
	netInput();
#endif

	if (PulsedInputState.menu[KEY_MENU_CANCEL] || PulsedInputState.menu[KEY_MENU_LEFT])
	{
		// Start editing the teams.
		pMS->LastInputTime = GetTimeCounter();
		Deselect(pMS->MeleeOption);
		pMS->MeleeOption = EDIT_MELEE;
		pMS->Initialized = false;
		if (PulsedInputState.menu[KEY_MENU_CANCEL])
		{
			pMS->side = 0;
			pMS->row = 0;
			pMS->col = 0;
		}
		else
		{
			pMS->side = 0;
			pMS->row = NUM_MELEE_ROWS - 1;
			pMS->col = NUM_MELEE_COLUMNS - 1;
		}
		DoEdit(pMS);
	}
	else
	{
		MELEE_OPTIONS NewMeleeOption;

		NewMeleeOption = pMS->MeleeOption;
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			pMS->LastInputTime = GetTimeCounter();
			NewMeleeOption = MeleeOptionUp(pMS->MeleeOption);
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			pMS->LastInputTime = GetTimeCounter();
			NewMeleeOption = MeleeOptionDown(pMS->MeleeOption);
		}

		if (testFlag((PlayerControl[0] & PlayerControl[1]), PlayerControlFlags::Psytron)
			&& GetTimeCounter() - pMS->LastInputTime > GameTicksPerSecond * 10)
		{
			force_select = true;
			NewMeleeOption = START_MELEE;
		}

		if (NewMeleeOption != pMS->MeleeOption)
		{
#ifdef NETPLAY
			if (pMS->MeleeOption == CONTROLS_TOP || pMS->MeleeOption == CONTROLS_BOT)
			{
				UpdateMeleeStatusMessage(-1);
			}
#endif
			Deselect(pMS->MeleeOption);
			pMS->MeleeOption = NewMeleeOption;
			Select(pMS->MeleeOption);
#ifdef NETPLAY
			if (NewMeleeOption == CONTROLS_TOP || NewMeleeOption == CONTROLS_BOT)
			{
				uint16_t side = (NewMeleeOption == CONTROLS_TOP) ? 1 : 0;
				if (testFlag(PlayerControl[side], PlayerControlFlags::Network))
				{
					UpdateMeleeStatusMessage(side);
				}
				else
				{
					UpdateMeleeStatusMessage(-1);
				}
			}
#endif
		}

		if (PulsedInputState.menu[KEY_MENU_SELECT] || force_select)
		{
			MeleeOptionSelect(pMS);
			if (GLOBAL(CurrentActivity) & CHECK_ABORT)
			{
				return false;
			}
		}
	}

#ifdef NETPLAY
	flushPacketQueues();

	check_for_disconnects(pMS);
#endif

	Melee_flashSelection(pMS);

	SleepThreadUntil(TimeIn + GameTicksPerSecond / 30);

	return true;
}

static constexpr const char* c_meleeConfigFile {"melee.json"};

static int
LoadMeleeConfig(MELEE_STATE* pMS)
{
	const auto json {uqm::readTextFile(configDir, c_meleeConfigFile)};
	if (!json)
	{
		return -1;
	}

	const auto sides {uqm::meleeConfigFromJson(*json)};
	if (!sides || sides->size() != NUM_SIDES)
	{
		uqm::moveToBadFile(configDir, c_meleeConfigFile);
		return -1;
	}

	for (uint16_t side = 0; side < NUM_SIDES; side++)
	{
		const uqm::MeleeSideConfig& config {(*sides)[side]};

		/* Do not allow netplay mode at the start. */
		PlayerControl[side] = config.control;
		if (PlayerControl[side] == PlayerControlFlags::None || testFlag(PlayerControl[side], PlayerControlFlags::Network))
		{
			PlayerControl[side] = PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard;
		}

		MeleeSetup_setTeamName(pMS->meleeSetup, side, config.teamName.c_str());
		for (FleetShipIndex slot = 0; slot < MELEE_FLEET_SIZE; slot++)
		{
			const MeleeShip ship {slot < config.ships.size() ? config.ships[slot] : MELEE_NONE};
			MeleeSetup_setShip(pMS->meleeSetup, side, slot, ship);
		}
	}
	return 0;
}

static int
WriteMeleeConfig(MELEE_STATE* pMS)
{
	uqstl::vector<uqm::MeleeSideConfig> sides(NUM_SIDES);
	for (uint16_t side = 0; side < NUM_SIDES; side++)
	{
		uqm::MeleeSideConfig& config {sides[side]};
		config.control = PlayerControl[side];
		config.teamName = MeleeSetup_getTeamName(pMS->meleeSetup, side);
		for (FleetShipIndex slot = 0; slot < MELEE_FLEET_SIZE; slot++)
		{
			config.ships.push_back(MeleeSetup_getShip(pMS->meleeSetup, side, slot));
		}
	}

	if (!uqm::writeTextFile(configDir, c_meleeConfigFile, uqm::meleeConfigToJson(sides)))
	{
		uqm::log::error("Could not write {}.", c_meleeConfigFile);
		return -1;
	}
	return 0;
}

void Melee(void)
{
	InitGlobData();
	{
		MELEE_STATE MenuState;

		pMeleeState = &MenuState;
		memset(pMeleeState, 0, sizeof(*pMeleeState));

		MenuState.InputFunc = DoMelee;
		MenuState.Initialized = false;

		MenuState.meleeSetup = MeleeSetup_new();

		MenuState.randomContext = RandomContext_New();
		RandomContext_SeedRandom(MenuState.randomContext,
								 GetTimeCounter());
		// Using the current time still leaves the random state a bit
		// predictable, but it is good enough.

#ifdef NETPLAY
		{
			uint16_t player;
			for (player = 0; player < NUM_PLAYERS; player++)
			{
				netConnections[player] = nullptr;
			}
		}
#endif

		MenuState.currentShip = MELEE_NONE;
		MenuState.CurIndex = MELEE_STATE_INDEX_DONE;
		InitMeleeLoadState(&MenuState);

		GLOBAL(CurrentActivity) = SUPER_MELEE;

		GameSounds = CaptureSound(LoadSound(GAME_SOUNDS));
		LoadMeleeInfo(&MenuState);
		if (LoadMeleeConfig(&MenuState) == -1)
		{ // This sets the default controls on the Super Melee screen
			PlayerControl[0] = PlayerControlFlags::Human | PlayerControlFlags::DifficultyStandard;
			Melee_LocalChange_team(&MenuState, 0,
								   MenuState.load.preBuiltList[0]);
			PlayerControl[1] = ComputerControlFlags | PlayerControlFlags::DifficultyStandard;
			Melee_LocalChange_team(&MenuState, 1,
								   MenuState.load.preBuiltList[1]);
		}

		MenuState.side = 0;
		SetMenuSounds(MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
		DoInput(&MenuState, true);

		StopMusic();
		WaitForSoundEnd(TFBSOUND_WAIT_ALL);

		WriteMeleeConfig(&MenuState);
		FreeMeleeInfo(&MenuState);
		DestroySound(ReleaseSound(GameSounds));
		GameSounds = 0;

		UninitMeleeLoadState(&MenuState);

		RandomContext_Delete(MenuState.randomContext);

		MeleeSetup_delete(MenuState.meleeSetup);

		FlushInput();
	}
}

#ifdef NETPLAY
void updateRandomSeed(MELEE_STATE* pMS, uint16_t side, uint32_t seed)
{
	TFB_SeedRandom(seed);
	(void)pMS;
	(void)side;
}

// The remote player has done something which invalidates our confirmation.
void confirmationCancelled(MELEE_STATE* pMS, uint16_t side)
{
	if (side == 0)
	{
		DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 16));
	}
	// "Bottom player changed something -- need to reconfirm."
	else
	{
		DrawMeleeStatusMessage(GAME_STRING(NETMELEE_STRING_BASE + 17));
	}
	// "Top player changed something -- need to reconfirm."

	if (pMS->InputFunc == DoConfirmSettings)
	{
		pMS->InputFunc = DoMelee;
	}
}

static void
connectionFeedback(NetConnection* conn, const char* str, bool forcePopup)
{
	struct battlestate_struct* bs = NetMelee_getBattleState(conn);

	if (bs == nullptr && !forcePopup)
	{
		// bs == nullptr means the game has not started yet.
		DrawMeleeStatusMessage(str);
	}
	else
	{
		DoPopupWindow(str);
	}
}

void connectedFeedback(NetConnection* conn)
{
	if (NetConnection_getPlayerNr(conn) == 0)
	{
		connectionFeedback(conn, GAME_STRING(NETMELEE_STRING_BASE + 8),
						   false);
	}
	// "Bottom player is connected."
	else
	{
		connectionFeedback(conn, GAME_STRING(NETMELEE_STRING_BASE + 9),
						   false);
	}
	// "Top player is connected."

	PlayMenuSound(MENU_SOUND_INVOKED);
}

static const char*
abortReasonString(NetplayAbortReason reason)
{
	switch (reason)
	{
		case AbortReason_unspecified:
			return GAME_STRING(NETMELEE_STRING_BASE + 25);
			// "Disconnect for an unspecified reason.'
		case AbortReason_versionMismatch:
			return GAME_STRING(NETMELEE_STRING_BASE + 26);
			// "Connection aborted due to version mismatch."
		case AbortReason_invalidHash:
			return GAME_STRING(NETMELEE_STRING_BASE + 27);
			// "Connection aborted because the remote side sent a "
			// "fake signature."
		case AbortReason_protocolError:
			return GAME_STRING(NETMELEE_STRING_BASE + 28);
			// "Connection aborted due to an internal protocol "
			// "error."
	}

	return nullptr;
	// Should not happen.
}

void abortFeedback(NetConnection* conn, NetplayAbortReason reason)
{
	const char* msg;

	msg = abortReasonString(reason);
	if (msg != nullptr)
	{
		connectionFeedback(conn, msg, true);
	}
}

static const char*
resetReasonString(NetplayResetReason reason)
{
	switch (reason)
	{
		case ResetReason_unspecified:
			return GAME_STRING(NETMELEE_STRING_BASE + 29);
			// "Game aborted for an unspecified reason."
		case ResetReason_syncLoss:
			return GAME_STRING(NETMELEE_STRING_BASE + 30);
			// "Game aborted due to loss of synchronisation."
		case ResetReason_manualReset:
			return GAME_STRING(NETMELEE_STRING_BASE + 31);
			// "Game aborted by the remote player."
	}

	return nullptr;
	// Should not happen.
}

void resetFeedback(NetConnection* conn, NetplayResetReason reason,
				   bool byRemote)
{
	const char* msg;

	flushPacketQueues();
	// If the local side queued a reset packet as a result of a
	// remote reset, that packet will not have been sent yet.
	// We flush the queue now, so that the remote side won't be
	// waiting for the reset packet while this side is waiting
	// for an acknowledgement of the feedback message.

	if (reason == ResetReason_manualReset && !byRemote)
	{
		// No message needed, the player initiated the reset.
		return;
	}

	msg = resetReasonString(reason);
	if (msg != nullptr)
	{
		connectionFeedback(conn, msg, false);
	}

	// End supermelee. This must not be done before connectionFeedback(),
	// otherwise the message will immediately disappear.
	GLOBAL(CurrentActivity) |= CHECK_ABORT;
}

void errorFeedback(NetConnection* conn)
{
	if (NetConnection_getPlayerNr(conn) == 0)
	{
		connectionFeedback(conn, GAME_STRING(NETMELEE_STRING_BASE + 10),
						   false);
	}
	// "Bottom player: connection failed."
	else
	{
		connectionFeedback(conn, GAME_STRING(NETMELEE_STRING_BASE + 11),
						   false);
	}
	// "Top player: connection failed."
}

void closeFeedback(NetConnection* conn)
{
	if (NetConnection_getPlayerNr(conn) == 0)
	{
		connectionFeedback(conn, GAME_STRING(NETMELEE_STRING_BASE + 12),
						   false);
	}
	// "Bottom player: connection closed."
	else
	{
		connectionFeedback(conn, GAME_STRING(NETMELEE_STRING_BASE + 13),
						   false);
	}
	// "Top player: connection closed."
}

#endif /* NETPLAY */


///////////////////////////////////////////////////////////////////////////

// Melee_UpdateView_xxx() functions are called when some value in the
// supermelee fleet setup screen needs to be updated visually.

static void
Melee_UpdateView_fleetValue(MELEE_STATE* pMS, uint16_t side)
{
	if (pMS->meleeStarted)
	{
		return;
	}

	DrawFleetValue(pMS, side, DTSHS_REPAIR);
	// BUG: The fleet value is always drawn as deselected.
}

static void
Melee_UpdateView_ship(MELEE_STATE* pMS, uint16_t side, FleetShipIndex index)
{
	MeleeShip ship;

	if (pMS->meleeStarted)
	{
		return;
	}

	ship = MeleeSetup_getShip(pMS->meleeSetup, side, index);

	if (ship == MELEE_NONE)
	{
		ClearShipBox(side, index);
	}
	else
	{
		DrawShipBox(side, index, ship, false);
	}
}

static void
Melee_UpdateView_teamName(MELEE_STATE* pMS, uint16_t side)
{
	if (pMS->meleeStarted)
	{
		return;
	}

	DrawTeamString(pMS, side, DTSHS_REPAIR, nullptr);
}

///////////////////////////////////////////////////////////////////////////

// Melee_Change_xxx() functions are helper functions, called when some value
// in the supermelee fleet setup screen has changed, eithed because of a
// local change, or a remote change.

static bool
Melee_Change_ship(MELEE_STATE* pMS, uint16_t side, FleetShipIndex index,
				  MeleeShip ship)
{
	if (!MeleeSetup_setShip(pMS->meleeSetup, side, index, ship))
	{
		// No change.
		Melee_UpdateView_ship(pMS, side, index);
		return false;
	}

	// Update the view
	Melee_UpdateView_ship(pMS, side, index);
	Melee_UpdateView_fleetValue(pMS, side);

	// If the modified slot is currently selected, display the new ship icon
	// on the right of the screen.
	if (isShipSlotSelected(pMS, side, index))
	{
		pMS->currentShip = ship;
		DrawMeleeShipStrings(pMS, ship);
	}

	return true;
}

// Pre: 'name' is '\0'-terminated
static bool
Melee_Change_teamName(MELEE_STATE* pMS, uint16_t side, const char* name)
{
	MeleeSetup* setup = pMS->meleeSetup;

	if (!MeleeSetup_setTeamName(setup, side, name))
	{
		// No change.
		return false;
	}

	if (pMS->row != NUM_MELEE_ROWS || pMS->side != side || pMeleeState->CurIndex == MELEE_STATE_INDEX_DONE)
	{
		// The team name is not currently being edited, so we can
		// update it on screen. If it was edited, then this function
		// will be called again after it is done.
		Melee_UpdateView_teamName(pMS, side);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////

// Melee_LocalChange_xxx() functions are called when some value in the
// supermelee fleet setup screen has changed because of a local action.
// The behavior of these functions (and the comments therein) follow the
// description in doc/devel/netplay/protocol.

bool Melee_LocalChange_ship(MELEE_STATE* pMS, uint16_t side, FleetShipIndex index,
							MeleeShip ship)
{
	if (!Melee_Change_ship(pMS, side, index, ship))
	{
		return false;
	}

#ifdef NETPLAY
	{
		MeleeSetup* setup = pMS->meleeSetup;

		MeleeShip sentShip = MeleeSetup_getSentShip(setup, side, index);
		if (sentShip == MELEE_UNSET)
		{
			// State 1.
			// Notify network connections of the change.
			Netplay_NotifyAll_setShip(pMS, side, index);
			MeleeSetup_setSentShip(setup, side, index, ship);
		}
	}
#endif /* NETPLAY */

	return true;
}


// Pre: 'name' is '\0'-terminated
bool Melee_LocalChange_teamName(MELEE_STATE* pMS, uint16_t side, const char* name)
{
	if (!Melee_Change_teamName(pMS, side, name))
	{
		return false;
	}

#ifdef NETPLAY
	{
		MeleeSetup* setup = pMS->meleeSetup;

		const char* sentName = MeleeSetup_getSentTeamName(setup, side);
		if (sentName == nullptr)
		{
			// State 1.
			// Notify network connections of the change.
			Netplay_NotifyAll_setTeamName(pMS, side);
			MeleeSetup_setSentTeamName(setup, side, name);
		}
	}
#endif /* NETPLAY */

	return true;
}

bool Melee_LocalChange_fleet(MELEE_STATE* pMS, size_t teamNr,
							 const MeleeShip* fleet)
{
	FleetShipIndex slotI;
	bool changed = false;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		if (Melee_LocalChange_ship(
				pMS, (uint16_t)teamNr, slotI, fleet[slotI]))
		{
			changed = true;
		}
	}
	return changed;
}

bool Melee_LocalChange_team(MELEE_STATE* pMS, size_t teamNr,
							const MeleeTeam* team)
{
	const MeleeShip* fleet = MeleeTeam_getFleet(team);
	const char* name = MeleeTeam_getTeamName(team);
	bool changed = false;

	if (Melee_LocalChange_fleet(pMS, teamNr, fleet))
	{
		changed = true;
	}
	if (Melee_LocalChange_teamName(pMS, (uint16_t)teamNr, name))
	{
		changed = true;
	}

	return changed;
}

///////////////////////////////////////////////////////////////////////////

#ifdef NETPLAY

// Send the entire team to the remote side. Used when the connection has
// just been established, or after the setup menu is reentered after battle.
void Melee_bootstrapSyncTeam(MELEE_STATE* meleeState, size_t teamNr)
{
	MeleeSetup* setup = meleeState->meleeSetup;
	FleetShipIndex slotI;
	const char* teamName;

	// Send the current fleet.
	Netplay_NotifyAll_setFleet(meleeState, teamNr);

	// Update the last sent fleet.
	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		MeleeShip ship = MeleeSetup_getShip(setup, teamNr, slotI);
		assert(MeleeSetup_getSentShip(setup, teamNr, slotI) == MELEE_UNSET);
		MeleeSetup_setSentShip(setup, teamNr, slotI, ship);
	}

	// Send the current team name.
	Netplay_NotifyAll_setTeamName(meleeState, teamNr);

	// Update the last sent team name.
	teamName = MeleeSetup_getTeamName(setup, teamNr);
	MeleeSetup_setSentTeamName(setup, teamNr, teamName);
}

///////////////////////////////////////////////////////////////////////////

// Melee_RemoteChange_xxx() functions are called when some value in the
// supermelee fleet setup screen has changed remotely.
// The behavior of these functions (and the comments therein) follow the
// description in doc/devel/netplay/protocol.

void Melee_RemoteChange_ship(MELEE_STATE* pMS, NetConnection* conn, uint16_t side,
							 FleetShipIndex index, MeleeShip ship)
{
	MeleeSetup* setup = pMS->meleeSetup;

	MeleeShip sentShip = MeleeSetup_getSentShip(setup, side, index);
	MeleeShip currentShip;

	if (sentShip == MELEE_UNSET)
	{
		// State 1

		// Change the ship locally.
		Melee_Change_ship(pMS, side, index, ship);

		// Notify the remote side.
		Netplay_NotifyAll_setShip(pMS, side, index);

		// A packet has now been received and sent. End of turn.
		return;
	}

	// A packet has been sent and received. End of turn.
	MeleeSetup_setSentShip(setup, side, index, MELEE_UNSET);

	if (ship != sentShip)
	{
		// Rule 2c or 3d. The value which we sent is different from the value
		// which the opponent sent. We need a tie-breaker to determine which
		// value prevails.
		if (NetConnection_getPlayerNr(conn) != side)
		{
			// Rule 2c+ or 3d+
			// We win the tie-breaker. The value which we sent prevails.
		}
		else
		{
			// Rule 2c- or 3d-.
			// We lose the tie-breaker. We adopt the remote value.
			Melee_Change_ship(pMS, side, index, ship);
			return;
		}
	}
	/*
	else
	{
		// Rule 2b or 3c. The value which we sent is the value which
		// the opponent sent. This confirms the value.
	}
	*/

	// Rule 2b, 2c+, 3c, or 3d+. The value which we sent is confirmed.

	currentShip = MeleeSetup_getShip(setup, side, index);
	if (currentShip != sentShip)
	{
		// Rule 3c or 3d+. We had a local change which was yet
		// unreported.

		// Notify the remote side and keep track of what we sent.
		Netplay_NotifyAll_setShip(pMS, side, index);
		MeleeSetup_setSentShip(setup, side, index, ship);
	}
}

void Melee_RemoteChange_teamName(MELEE_STATE* pMS, NetConnection* conn,
								 uint16_t side, const char* newName)
{
	MeleeSetup* setup = pMS->meleeSetup;

	const char* sentName = MeleeSetup_getSentTeamName(setup, side);
	const char* currentName;

	if (sentName == nullptr)
	{
		// State 1

		// Change the team name locally.
		Melee_Change_teamName(pMS, side, newName);

		// Notify the remote side.
		Netplay_NotifyAll_setTeamName(pMS, side);

		// A packet has now been received and sent. End of turn.
		// The sent team name is still unset, so we don't have to reset it.
		return;
	}

	if (strcmp(newName, sentName) == 0)
	{
		// Rule 2c or 3d. The value which we sent is different from the value
		// which the opponent sent. We need a tie-breaker to determine which
		// value prevails.
		if (NetConnection_getPlayerNr(conn) != side)
		{
			// Rule 2c+ or 3d+
			// We win the tie-breaker. The value which we sent prevails.
		}
		else
		{
			// Rule 2c- or 3d-.
			// We lose the tie-breaker. We adopt the remote value.
			Melee_Change_teamName(pMS, side, newName);
			MeleeSetup_setSentTeamName(setup, side, nullptr);
			return;
		}
	}
	/*
	else
	{
		// Rule 2b or 3c. The value which we sent is the value which
		// the opponent sent. This confirms the value.
	}
	*/

	// Rule 2b, 2c+, 3c, or 3d+. The value which we sent is confirmed.

	currentName = MeleeSetup_getTeamName(setup, side);
	if (strcmp(currentName, sentName) != 0)
	{
		// Rule 3c or 3d+. We had a local change which was yet
		// unreported.

		// A packet has been sent and received, which ends the turn.
		// We don't bother clearing the sent team name, as we're going
		// to send a new packet immediately.

		// Notify the remote side and keep track of what we sent.
		Netplay_NotifyAll_setTeamName(pMS, side);

		// Update the last sent message.
		MeleeSetup_setSentTeamName(setup, side, newName);
	}
	else
	{
		// A packet has been sent and received. End of turn.
		MeleeSetup_setSentTeamName(setup, side, nullptr);
	}
}

///////////////////////////////////////////////////////////////////////////

#endif /* NETPLAY */
