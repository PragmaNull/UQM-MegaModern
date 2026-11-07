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
#ifndef UQM_COLORS_H
#define UQM_COLORS_H

#include "libs/gfxlib.h"

// To be used as an indicator that the actual value of the color does not
// matter, for instance in structure initialisations for fields which
// are irrelevant in the context.
inline constexpr auto UndefinedColor {BuildColor(MakeRgb15(0x00, 0x00, 0x00), 0x00)};

#if 0


#endif

inline constexpr auto NullColor {BuildColorRgba(0, 0, 0, 0)};
inline constexpr auto BlackColor {BuildColor(MakeRgb15(0x00, 0x00, 0x00), 0x00)};
inline constexpr auto LtgrayColor {BuildColor(MakeRgb15(0x14, 0x14, 0x14), 0x07)};

inline constexpr auto DkgrayColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08)};
inline constexpr auto MdkgrayColor {BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F)};
inline constexpr auto VdkgrayColor {BuildColor(MakeRgb15(0x06, 0x06, 0x06), 0x00)};
inline constexpr auto WhiteColor {BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F)};
inline constexpr auto BrightRedColor {BuildColor(MakeRgb15(0x1F, 0x00, 0x00), 0x04)};
inline constexpr auto BrightGreenColor {BuildColor(MakeRgb15(0x00, 0x1F, 0x00), 0x02)};
inline constexpr auto BrightBlueColor {BuildColor(MakeRgb15(0x00, 0x00, 0x1F), 0x01)};

/* uqm-hd */
inline constexpr auto BrightYellowColor {BuildColor(MakeRgb15(0x1F, 0x1F, 0x00), 0x01)};
inline constexpr auto DullYellowColor {BuildColor(MakeRgb15(0x0F, 0x0F, 0x00), 0x01)};
/* end uqm-hd */

inline constexpr auto StarmapFuelRangeColor {BuildColor(MakeRgb15(0x04, 0x04, 0x04), 0x22)};
inline constexpr auto StarmapSecondaryRangeColor {BuildColor(MakeRgb15(0x05, 0x05, 0x05), 0x22)};


inline constexpr auto MenuBackgroundColor {LtgrayColor};
inline constexpr auto MenuForegroundColor {DkgrayColor};
inline constexpr auto MenuTextColor {VdkgrayColor};
inline constexpr auto MenuHighlightColor {BuildColor(MakeRgb15(0x1F, 0x1F, 0x00), 0x0E)};
inline constexpr auto MenuCursorColor {WhiteColor};


inline constexpr auto HierarchyBackgroundColor {BuildColor(MakeRgb15(0x14, 0x00, 0x00), 0x04)};


// Not highlighted dialog options in comm.
inline constexpr auto CommPlayerTextNormalColor {BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03)};

// Currently highlighted dialog option in comm.
inline constexpr auto CommPlayerTextHighlightColor {BuildColor(MakeRgb15(0x1A, 0x1A, 0x1A), 0x12)};

// Background color of the area containing the player's dialog options.
inline constexpr auto CommPlayerBackgroundColor {BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)};

// "(In response to your statement)"
inline constexpr auto CommResponseIntroTextColor {BuildColor(MakeRgb15(0x0A, 0x0C, 0x1F), 0x48)};

// Your dialog option after choosing it.
inline constexpr auto CommFeedbackTextColor {BuildColor(MakeRgb15(0x12, 0x14, 0x4F), 0x44)};

// The background when reviewing the conversation history.
inline constexpr auto CommHistoryBackgroundColor {BuildColor(MakeRgb15(0x00, 0x05, 0x00), 0x00)};

// The text when reviewing the conversation history.
inline constexpr auto CommHistoryTextColor {BuildColor(MakeRgb15(0x00, 0x10, 0x00), 0x6B)};

// The text "MORE" when reviewing the conversation history.
inline constexpr auto CommMoreTextColor {BuildColor(MakeRgb15(0x00, 0x17, 0x00), 0x00)};

// Default colors for System Dialog Boxes (DrawShadowedBox)
inline constexpr auto ShadowboxBackgroundColor {MenuBackgroundColor};

inline constexpr auto ShadowboxMediumColor {BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19)};

inline constexpr auto ShadowboxDarkColor {BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F)};

// Alternate Shadowed Box colors

inline constexpr auto AltShadowboxBackgroundColor {BuildColor(MakeRgb15(0x0E, 0x0E, 0x0E), 0x00)};

inline constexpr auto AltShadowboxTopLeft {AltShadowboxBackgroundColor};

inline constexpr auto AltShadowboxBottomRight {VdkgrayColor};


// === SIS ===

// Left border of the "SIS" view (the part in which your ship flies).
inline constexpr auto SisLeftBorderColor {BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19)};

// Right and bottom border of the "SIS" view.
inline constexpr auto SisBottomRightBorderColor {BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F)};

// Text color of the string "CAPTAIN", when using PC fonts.
inline constexpr auto PcCaptainStringTextColor {BuildColor(MakeRgb15(0x02, 0x04, 0x1E), 0x38)};

// Text color of the string "CAPTAIN", when using PC fonts.
inline constexpr auto ThreedoCaptainStringTextColor {BuildColorRgb(0x48, 0x98, 0xC0)};

// Background color of the string "CAPTAIN", when using PC fonts.
inline constexpr auto PcCaptainStringBackgroundColor {BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)};

// Text color of the captain's name.
inline constexpr auto CaptainNameTextColor {BuildColor(MakeRgb15(0x16, 0x0B, 0x1F), 0x38)};

// Background color of the captain's name.
inline constexpr auto CaptainNameBackgroundColor {BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)};

// Text color of the flagship's name, when using 3DO fonts.
inline constexpr auto ThreedoFlagshipNameTextColor {BuildColor(MakeRgb15(0x14, 0x0A, 0x00), 0x0C)};

// Background color of the flagship's name.
inline constexpr auto FlagshipNameBackgroundColor {BlackColor};

// Text color for the message area (at the top of the screen, on the left
// hand side, containing the name of the solar system.
inline constexpr auto SisMessageTextColor {BuildColor(MakeRgb15(0x1B, 0x00, 0x1B), 0x33)};

// Color of autocompleted text after the current cursor position,
// when editing in the title area.
inline constexpr auto SisMessageExtraTextColor {BuildColor(MakeRgb15(0x12, 0x00, 0x12), 0x33)};

// Background color for the message area.
inline constexpr auto SisMessageBackgroundColor {BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)};

// Cursor color when editing in the message area.
inline constexpr auto SisMessageCursorColor {BlackColor};

inline constexpr auto SisMessageTopLeftColor {BuildColor(MakeRgb15(0x00, 0x00, 0x0E), 0x54)};

inline constexpr auto SisMessageBottomRightColor {BuildColor(MakeRgb15(0x00, 0x01, 0x1C), 0x4E)};

// Text color of the title (at the top of the screen, on the right
// hand side, containing the coordinates in HyperSpace, or the planet name
// in IP.
inline constexpr auto SisTitleTextColor {BuildColor(MakeRgb15(0x1B, 0x00, 0x1B), 0x33)};

// Background color of the title.
inline constexpr auto SisTitleBackgroundColor {SisMessageBackgroundColor};

inline constexpr auto SisTitleTopLeftColor {SisMessageTopLeftColor};

inline constexpr auto SisTitleBottomRightColor {SisMessageBottomRightColor};

// Text color of the status message, below the flagship overview, containing
// the date, RU, etc.
inline constexpr auto StatusMessageTextColor {BuildColor(MakeRgb15(0x00, 0x10, 0x00), 0x6B)};

// Text color for the status message when it's displaying a warning (yellow).
inline constexpr auto StatusMessageWarningTextColor {BuildColor(MakeRgb15(0x10, 0x10, 0x00), 0x6B)};

// Text color for the status message then it's displaying an alert (red).
inline constexpr auto StatusMessageAlertTextColor {BuildColor(MakeRgb15(0x10, 0x00, 0x00), 0x6B)};

// Background color of the status message.
inline constexpr auto StatusMessageBackgroundColor {BuildColor(MakeRgb15(0x00, 0x08, 0x00), 0x6E)};

//	== Scavenge screen ==

// Scavenge screen text color tab
inline constexpr Color ScavengeTextColorTable[] {
	BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x1D),
	BuildColor(MakeRgb15(0x09, 0x09, 0x09), 0x1E),
	BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F),
	BuildColor(MakeRgb15(0x06, 0x06, 0x06), 0x20),
	BuildColor(MakeRgb15(0x05, 0x05, 0x05), 0x21),
	BuildColor(MakeRgb15(0x04, 0x04, 0x04), 0x22),
	BuildColor(MakeRgb15(0x03, 0x03, 0x03), 0x23),
};

// Scavenge screen ship fading
inline constexpr Color ScavengeScreenColorTable[] {
	BuildColor(MakeRgb15(0x07, 0x00, 0x00), 0x2F),
	BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2D),
	BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
	BuildColor(MakeRgb15(0x1F, 0x0A, 0x0A), 0x27),
	BuildColor(MakeRgb15(0x1F, 0x14, 0x14), 0x25),
	BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F),
	BuildColor(MakeRgb15(0x1F, 0x14, 0x14), 0x25),
	BuildColor(MakeRgb15(0x1F, 0x0A, 0x0A), 0x27),
	BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2A),
	BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
};

//	== Misc tables ==

// Pulsating color of the string "AUTO-PILOT"
inline constexpr Color AutopilotColorCycleTable[] {
	BuildColor(MakeRgb15(0x0A, 0x14, 0x18), 0x5B),
	BuildColor(MakeRgb15(0x06, 0x10, 0x16), 0x5C),
	BuildColor(MakeRgb15(0x03, 0x0E, 0x14), 0x5D),
	BuildColor(MakeRgb15(0x02, 0x0C, 0x11), 0x5E),
	BuildColor(MakeRgb15(0x01, 0x0B, 0x0F), 0x5F),
	BuildColor(MakeRgb15(0x01, 0x09, 0x0D), 0x60),
	BuildColor(MakeRgb15(0x00, 0x07, 0x0B), 0x61),
};

// Pulsating color of the text cursor
inline constexpr Color CursorColorCycleTable[] {
	BuildColor(MakeRgb15(0x00, 0x00, 0x00), 0x00),
	BuildColor(MakeRgb15(0x00, 0x00, 0x00), 0x00),
	BuildColor(MakeRgb15(0x16, 0x0A, 0x1F), 0x5C),
	BuildColor(MakeRgb15(0x16, 0x0A, 0x1F), 0x5C),
	BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x00),
	BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x00),
	BuildColor(MakeRgb15(0x19, 0x19, 0x19), 0x00),
	BuildColor(MakeRgb15(0x15, 0x15, 0x15), 0x00),
	BuildColor(MakeRgb15(0x11, 0x11, 0x11), 0x00),
	BuildColor(MakeRgb15(0x0C, 0x0C, 0x0C), 0x00),
	BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x00),
	BuildColor(MakeRgb15(0x04, 0x04, 0x04), 0x00),
};

// Pulsating color of the PC Menu Rectangle
inline constexpr Color PcRectColorCycleTable[] {
	BuildColorRgba(0x24, 0x00, 0x45, 0xFF),
	BuildColorRgba(0x38, 0x00, 0x65, 0xFF),
	BuildColorRgba(0x4D, 0x00, 0x8A, 0xFF),
	BuildColorRgba(0x61, 0x00, 0xAA, 0xFF),
	BuildColorRgba(0x79, 0x00, 0xCF, 0xFF),
	BuildColorRgba(0x96, 0x00, 0xFF, 0xFF),
	BuildColorRgba(0xA6, 0x34, 0xFF, 0xFF),
	BuildColorRgba(0xB6, 0x55, 0xFF, 0xFF),
};

// Pulsating color of selected ship
inline constexpr Color ShipSelectColorCycleTable[] {
	BuildColorRgba(0xFF, 0xFF, 0xFF, 0xFF),
	BuildColorRgba(0xAA, 0xAA, 0xAA, 0xFF),
	BuildColorRgba(0x55, 0x55, 0x55, 0xFF),
	BuildColorRgba(0x1F, 0x1F, 0x1F, 0xFF),
	BuildColorRgba(0x55, 0x55, 0x55, 0xFF),
	BuildColorRgba(0xAA, 0xAA, 0xAA, 0xFF),
};

// Dark Mode FG text Color
inline constexpr Color DarkmodeFgTable[] {
	BuildColorRgba(0x00, 0xFF, 0x10, 0x00),
	BuildColorRgba(0x00, 0xFF, 0x20, 0x00),
	BuildColorRgba(0x00, 0xFF, 0x30, 0x00),
	BuildColorRgba(0x00, 0xFF, 0x40, 0x00),
	BuildColorRgba(0x00, 0xFF, 0x50, 0x00),
	BuildColorRgba(0x00, 0xFF, 0x60, 0x00),
	BuildColorRgba(0x08, 0xFF, 0x70, 0x00),
	BuildColorRgba(0x10, 0xFF, 0x80, 0x00),
	BuildColorRgba(0x18, 0xFF, 0x90, 0x00),
	BuildColorRgba(0x20, 0xFF, 0xA0, 0x00),
	BuildColorRgba(0x28, 0xFF, 0xB0, 0x00),
	BuildColorRgba(0x30, 0xFF, 0xC0, 0x00),
	BuildColorRgba(0x38, 0xFF, 0xD0, 0x00),
	BuildColorRgba(0x40, 0xFF, 0xE0, 0x00),
	BuildColorRgba(0x48, 0xFF, 0xF0, 0x00),
	BuildColorRgba(0x55, 0xFF, 0xFF, 0x00),
};

// Dark Mode FG text Color
inline constexpr Color DarkmodeBgTable[] {
	BuildColorRgba(0xE0, 0x9A, 0x14, 0x00),
	BuildColorRgba(0xD2, 0x87, 0x14, 0x00),
	BuildColorRgba(0xC4, 0x7E, 0x14, 0x00),
	BuildColorRgba(0xB6, 0x75, 0x14, 0x00),
	BuildColorRgba(0xA8, 0x6C, 0x14, 0x00),
	BuildColorRgba(0x9A, 0x63, 0x14, 0x00),
	BuildColorRgba(0x8C, 0x5A, 0x14, 0x00),
	BuildColorRgba(0x7E, 0x51, 0x14, 0x00),
	BuildColorRgba(0x70, 0x48, 0x07, 0x00),
	BuildColorRgba(0x62, 0x3F, 0x07, 0x00),
	BuildColorRgba(0x54, 0x36, 0x07, 0x00),
	BuildColorRgba(0x46, 0x2D, 0x00, 0x00),
	BuildColorRgba(0x38, 0x24, 0x00, 0x00),
	BuildColorRgba(0x2A, 0x1B, 0x00, 0x00),
	BuildColorRgba(0x1C, 0x12, 0x00, 0x00),
	BuildColorRgba(0x00, 0x00, 0x00, 0x00),
};

// Colors for the fuel in the fuel tanks as they are filled up,
// when viewed from the shipyard.
inline constexpr Color FuelColorTable[] {
	BuildColor(MakeRgb15(0x0F, 0x00, 0x00), 0x2D),
	BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C),
	BuildColor(MakeRgb15(0x17, 0x00, 0x00), 0x2B),
	BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2A),
	BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7F),
	BuildColor(MakeRgb15(0x1F, 0x07, 0x00), 0x7E),
	BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7D),
	BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7C),
	BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7B),
	BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7A),
	BuildColor(MakeRgb15(0x1F, 0x18, 0x00), 0x79),
};

// Colors for the crew in the crew pods as they are filled up,
// when viewed from the shipyard, when using PC fonts.
inline constexpr Color PcCrewColorTable[] {
	BuildColor(MakeRgb15(0x0A, 0x1E, 0x09), 0x65),
	BuildColor(MakeRgb15(0x00, 0x1E, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x1B, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x18, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x15, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x12, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x10, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x0D, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x0A, 0x00), 0x65),
	BuildColor(MakeRgb15(0x00, 0x07, 0x00), 0x65),
};

// Colors for the crew in the crew pods as they are filled up,
// when viewed from the shipyard, when using 3DO fonts.
inline constexpr auto ThreedoCrewColor {BuildColor(MakeRgb15(0x05, 0x10, 0x05), 0x65)};

// Colors for the minerals in the storage bays as they are filled up,
// when viewed from the shipyard.
inline constexpr Color StorageBayColorTable[] {
	BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F),
	BuildColor(MakeRgb15(0x1C, 0x1C, 0x1C), 0x11),
	BuildColor(MakeRgb15(0x18, 0x18, 0x18), 0x13),
	BuildColor(MakeRgb15(0x15, 0x15, 0x15), 0x15),
	BuildColor(MakeRgb15(0x12, 0x12, 0x12), 0x17),
	BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x19),
	BuildColor(MakeRgb15(0x0D, 0x0D, 0x0D), 0x1B),
	BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x1D),
	BuildColor(MakeRgb15(0x08, 0x08, 0x08), 0x1F),
	BuildColor(MakeRgb15(0x05, 0x05, 0x05), 0x21),
};

// Color of the storage bay indicator, as shown beneath the flagship,
// for the parts which are full.
inline constexpr auto StorageBayFullColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08)};

// Color of the storage bay indicator, as shown beneath the flagship,
// for the parts which are empty.
inline constexpr auto StorageBayEmptyColor {BuildColor(MakeRgb15(0x06, 0x06, 0x06), 0x20)};

// Colors of the oscilloscope lines
inline constexpr auto ScopeColor3do {BuildColorRgba(0x83, 0x4A, 0xFF, 0xFF)};
inline constexpr auto ScopeColorPc {BuildColorRgba(0x79, 0x00, 0xCE, 0xFF)};

// Flagship blueprint color
inline constexpr auto BlueprintColor {BuildColor(MakeRgb15(0x00, 0x00, 0x16), 0x01)};

inline constexpr auto FullCrewColor {BuildColor(MakeRgb15(0x00, 0x16, 0x00), 0x01)};
inline constexpr auto HalfCrewColor {BuildColor(MakeRgb15(0x16, 0x16, 0x00), 0x01)};
inline constexpr auto LowCrewColor {BuildColor(MakeRgb15(0x16, 0x00, 0x00), 0x01)};

// Colors for the Lander damage cycle
inline constexpr Color LanderDamageColorTable[] {
	BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F),
	BuildColor(MakeRgb15(0x1B, 0x00, 0x00), 0x2A),
	BuildColor(MakeRgb15(0x1F, 0x07, 0x00), 0x7E),
	BuildColor(MakeRgb15(0x1F, 0x0E, 0x00), 0x7C),
	BuildColor(MakeRgb15(0x1F, 0x15, 0x00), 0x7A),
	BuildColor(MakeRgb15(0x1F, 0x1C, 0x00), 0x78),
	BuildColor(MakeRgb15(0x1F, 0x1F, 0x0A), 0x0E),
};

// Colors for the lightning color cycle
inline constexpr Color LightningColorTable[] {
	BuildColor(MakeRgb15(0x11, 0x11, 0x11), 0x18),
	BuildColor(MakeRgb15(0x13, 0x13, 0x13), 0x17),
	BuildColor(MakeRgb15(0x15, 0x15, 0x15), 0x15),
	BuildColor(MakeRgb15(0x17, 0x17, 0x17), 0x14),
	BuildColor(MakeRgb15(0x19, 0x19, 0x19), 0x13),
	BuildColor(MakeRgb15(0x1B, 0x1B, 0x1B), 0x12),
	BuildColor(MakeRgb15(0x1D, 0x1D, 0x1D), 0x10),
	BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0f),
};

// === PC Menus ===

// Background color of the PC-style menus.
inline constexpr auto PcmenuBackgroundColor {BuildColor(MakeRgb15(0x00, 0x00, 0x15), 0x00)};

// Color of the top and left segments of the border around PC-style menus.
inline constexpr auto PcmenuTopLeftBorderColor {BuildColor(MakeRgb15(0x0F, 0x0F, 0x0F), 0x00)};

// Color of the bottom and right segments of the border around PC-style menus.
inline constexpr auto PcmenuBottomRightBorderColor {BuildColor(MakeRgb15(0x06, 0x06, 0x06), 0x00)};

// Text color of an unselected menu item.
inline constexpr auto PcmenuTextColor {BuildColor(MakeRgb15(0x00, 0x15, 0x15), 0x00)};

// Text color of an selected menu item.
inline constexpr auto PcmenuSelectionTextColor {BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B)};

// Background color of a selected menu item.
inline constexpr auto PcmenuSelectionBackgroundColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)};

// === 3DO menus ===
inline constexpr auto ThreedomenuTextColor {BuildColor(MakeRgb15(0x00, 0x1F, 0x00), 0x00)};


// === Credits ===

inline constexpr auto CreditsTextColor {WhiteColor};

// === Main Menu ===

inline constexpr auto MainMenuTextColor {BuildColorRgb(0x00, 0x73, 0x00)};

// === Cargo menu ===

inline constexpr auto CargoBackColor {BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)};

inline constexpr auto CargoWorthColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)};

inline constexpr auto CargoAmountColor {BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03)};

inline constexpr auto CargoSelectedBackColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)};

inline constexpr auto CargoSelectedWorthColor {BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03)};

inline constexpr auto CargoSelectedAmountColor {BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B)};


// === Devices menu ===

inline constexpr auto DevicesBackColor {BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)};

inline constexpr auto DevicesNameColor {BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x03)};

inline constexpr auto DevicesSelectedBackColor {BuildColor(MakeRgb15(0x0A, 0x0A, 0x1F), 0x09)};

inline constexpr auto DevicesSelectedNameColor {BuildColor(MakeRgb15(0x0A, 0x1F, 0x1F), 0x0B)};

// === Modules Menu ===

inline constexpr auto ModuleBackColor {BuildColorRgba(0x00, 0x00, 0xAA, 0xFF)};

inline constexpr auto ModulePriceColor {BuildColorRgba(0x55, 0x55, 0xFF, 0xFF)};

inline constexpr auto ModuleSelectedBackColor {ModulePriceColor};

inline constexpr auto ModuleNameColor {BuildColorRgba(0x00, 0xAA, 0xAA, 0xFF)};

inline constexpr auto ModuleSelectedColor {BuildColorRgba(0x55, 0xFF, 0xFF, 0xFF)};

// === Roster menu ===

inline constexpr auto RosterModifyShipColor {WhiteColor};

// === No Room For Landers & Escape Pod text ===

inline constexpr auto LanderPodTextColor {BuildColorRgb(0x00, 0xF8, 0x00)};

// === Scan menu and general ===

inline constexpr auto ScanPcTitleColor {BuildColor(MakeRgb15(0x00, 0x00, 0x15), 0x3B)};

inline constexpr auto ScanInfoColor {BuildColor(MakeRgb15(0x0F, 0x00, 0x19), 0x3B)};

inline constexpr auto ScanMineralTextColor {BuildColor(MakeRgb15(0x13, 0x00, 0x00), 0x2C)};

inline constexpr auto ScanEnergyTextColor {BuildColor(MakeRgb15(0x0C, 0x0C, 0x0C), 0x1C)};

inline constexpr auto ScanBiologicalTextColor {BuildColor(MakeRgb15(0x00, 0x0E, 0x00), 0x6C)};


inline constexpr auto ScanPcTitleColor6014 {BuildColor(MakeRgb15(0x00, 0x14, 0x14), 0x3B)};

inline constexpr auto OutlineColor {(BuildColorRgba(0x00, 0x00, 0x00, 0x66))};

// === Super-Melee ToolTips ===

inline constexpr auto TooltipColorNameBack {BuildShadeRgba(0x2F)};
inline constexpr auto TooltipColorNameFront {BuildShadeRgba(0xC7)};
inline constexpr auto TooltipColorDescFront {BuildShadeRgba(0x81)};

// === Load/Save Menu ===

inline constexpr auto SaveUnselectedColor {BuildColor(MakeRgb15(0x00, 0x00, 0x14), 0x01)};
inline constexpr auto SaveSelectedColor {BuildColor(MakeRgb15(0x1B, 0x00, 0x1B), 0x33)};

inline constexpr auto SummTextColor {BuildColorRgba(0xBA, 0x00, 0xBA, 0xFF)};
inline constexpr auto SummValueColor {BuildColor(MakeRgb15(0x10, 0x00, 0x10), 0x01)};
inline constexpr auto LoadSaveLabelColor {BuildColorRgba(0x7B, 0x83, 0xDE, 0xFF)};
inline constexpr auto BombPodTextColor {BuildColorRgba(0x00, 0xE0, 0x00, 0xFF)};

inline constexpr auto LabelLineTopColor {BuildColorRgba(0x06, 0x0C, 0xBA, 0xFF)};
inline constexpr auto LabelLineMidColor {BuildColorRgba(0x05, 0x0A, 0x9B, 0xFF)};
inline constexpr auto LabelLineBotColor {BuildColorRgba(0x03, 0x06, 0x5D, 0xFF)};

inline constexpr auto NoLimitStroke3Color {BuildColorRgba(0x02, 0x04, 0x3E, 0xFF)};
inline constexpr auto NoLimitStroke2Color {BuildColorRgba(0x03, 0x06, 0x5D, 0xFF)};
inline constexpr auto NoLimitStroke1Color {BuildColorRgba(0x04, 0x08, 0x7C, 0xFF)};
inline constexpr auto NoLimitTextColor {BuildColorRgba(0x08, 0x10, 0xF8, 0xFF)};

inline constexpr auto EmptySlotStroke3Color {BuildColorRgba(0x1F, 0x00, 0x1F, 0xFF)};
inline constexpr auto EmptySlotStroke2Color {BuildColorRgba(0x3E, 0x00, 0x3E, 0xFF)};
inline constexpr auto EmptySlotStroke1Color {BuildColorRgba(0x5D, 0x00, 0x5D, 0xFF)};

inline constexpr Color CargoColorTable[] {
	BuildColor(MakeRgb15(0x02, 0x0E, 0x13), 0x00),
	BuildColor(MakeRgb15(0x19, 0x00, 0x00), 0x00),
	BuildColor(MakeRgb15(0x10, 0x10, 0x10), 0x00),
	BuildColor(MakeRgb15(0x03, 0x05, 0x1E), 0x00),
	BuildColor(MakeRgb15(0x00, 0x18, 0x00), 0x00),
	BuildColor(MakeRgb15(0x1B, 0x1B, 0x00), 0x00),
	BuildColor(MakeRgb15(0x1E, 0x0D, 0x00), 0x00),
	BuildColor(MakeRgb15(0x14, 0x00, 0x14), 0x05),
	BuildColor(MakeRgb15(0x0F, 0x00, 0x19), 0x00),
};

// === Hyperspace Star Colors ===
inline constexpr auto BlueStarColor {BuildColorRgb(0x48, 0x60, 0xF0)};
inline constexpr auto GreenStarColor {BuildColorRgb(0x00, 0xC8, 0x00)};
inline constexpr auto OrangeStarColor {BuildColorRgb(0xFF, 0x8A, 0x00)};
inline constexpr auto RedStarColor {BuildColorRgb(0xF8, 0x20, 0x20)};
inline constexpr auto YellowStarColor {BuildColorRgb(0xDE, 0xC8, 0x10)};

// === Play Menu ===

inline constexpr auto PmRectColor {DkgrayColor};
inline constexpr auto PmTextColor {BuildShadeRgba(0x74)};
inline constexpr auto PmShadowColor {BuildShadeRgba(0x31)};

inline constexpr auto MdlRectColor {PmRectColor};
inline constexpr auto MdlTextColor {PmTextColor};
inline constexpr auto MdlShadowColor {BuildShadeRgba(0x2E)};

inline constexpr auto ShpRectColor {PmRectColor};
inline constexpr auto ShpTextColor {BuildShadeRgba(0x78)};
inline constexpr auto ShpShadowColor {BuildShadeRgba(0x4B)};

// === Lander Status ===

inline constexpr auto LandStatMinColor {BuildColorRgb(0x7C, 0x00, 0x00)};
inline constexpr auto LandStatDatColor {BuildColorRgb(0x00, 0x00, 0x90)};

inline constexpr auto ScanMineralTintColor {BrightRedColor};
inline constexpr auto ScanEnergyTintColor {WhiteColor};
inline constexpr auto ScanBiologicalTintColor {BrightGreenColor};

// Transparent
inline constexpr auto Transparent {BuildColorRgba(0x50, 0x50, 0x50, 0x00)};

#endif // UQM_COLORS_H
