// Copyright Michael Martin, 2004.

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
#ifndef UQM_LIBS_GRAPHICS_WIDGETS_H
#define UQM_LIBS_GRAPHICS_WIDGETS_H

#include "core/types/enum.h"
#include "core/stl/stl.h"
#include "libs/gfxlib.h"

enum class WidgetEvent
{
	Up,
	Down,
	Left,
	Right,
	Select,
	Cancel,
	Delete,
	NumWidgetEvents
};
template <>
inline constexpr bool IsArithmeticEnum<WidgetEvent> = true;

enum class WidgetType
{
	MenuScreen,
	Choice,
	Button,
	Label,
	Slider,
	TextEntry,
	ControlEntry,
	NumWidgetTypes
};

inline constexpr auto WidgetTextentryWidth {32};
inline constexpr auto WidgetControlentryWidth {16};

struct Widget
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
};

struct WidgetMenuScreen
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
	const char* title;
	const char* subtitle;
	GfxStamp bgStamp;
	int num_children;
	struct Widget** child;
	int highlighted;
};

struct ChoiceOption
{
	const char* optname;
	const char* tooltip[3];
};

struct WidgetChoice
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
	const char* category;
	int numopts;
	ChoiceOption* options;
	int selected, highlighted, choice_num;
	void (*onChange)(struct WidgetChoice* self, int oldval);
};

struct WidgetButton
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
	const char* name;
	const char* tooltip[3];
};

struct WidgetLabel
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
	int line_count;
	const char** lines;
};

struct WidgetSlider
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
	void (*draw_value)(struct WidgetSlider* self, int x, int y);
	int min, max, step;
	int value;
	const char* category;
	const char* tooltip[3];
	void (*onChange)(struct WidgetSlider* self);
};

enum class TextEntryWidgetState
{
	Normal = 0,
	Editing,
	BlockCursor,

};
template <>
inline constexpr bool IsFlagEnum<TextEntryWidgetState> = true;

inline TextEntryWidgetState operator~(TextEntryWidgetState rhs)
{
	return static_cast<TextEntryWidgetState>(~static_cast<uint32_t>(rhs));
}
inline TextEntryWidgetState operator|=(TextEntryWidgetState lhs, TextEntryWidgetState rhs)
{
	return static_cast<TextEntryWidgetState>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
inline TextEntryWidgetState operator&=(TextEntryWidgetState lhs, TextEntryWidgetState rhs)
{
	return static_cast<TextEntryWidgetState>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}


struct WidgetTextEntry
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
	int (*handleEventSelect)(struct WidgetTextEntry* self);
	// handleEventSelect is an overridable callback event
	// called by the default handleEvent implementation
	// can be nullptr, in which case SELECT is ignored
	void (*onChange)(struct WidgetTextEntry* self);
	const char* category;
	char value[WidgetTextentryWidth];
	int maxlen;
	TextEntryWidgetState state;
	int cursor_pos;
	const char* tooltip[3];
};

struct WidgetControlEntry
{
	WidgetType tag;
	struct Widget* parent;
	int (*handleEvent)(struct Widget* self, WidgetEvent event);
	int (*receiveFocus)(struct Widget* self, WidgetEvent event);
	void (*draw)(struct Widget* self, int x, int y);
	int (*height)(struct Widget* self);
	int (*width)(struct Widget* self);
	void (*onChange)(struct WidgetControlEntry* self);
	void (*onDelete)(struct WidgetControlEntry* self);
	const char* category;
	int controlindex;
	int highlighted;
	char controlname[2][WidgetControlentryWidth];
};

void DrawShadowedBox(GfxRect* r, Color bg, Color dark, Color medium);
void DrawLabelAsWindow(WidgetLabel* label, GfxRect* windowRect);
void Widget_SetWindowColors(Color bg, Color dark, Color medium);
FONT Widget_SetFont(FONT newFont);


int Widget_Event(WidgetEvent event);

/* Methods for filling in widgets with */

int Widget_ReceiveFocusMenuScreen(Widget* _self, WidgetEvent event);
int Widget_ReceiveFocusChoice(Widget* _self, WidgetEvent event);
int Widget_ReceiveFocusSimple(Widget* _self, WidgetEvent event);
int Widget_ReceiveFocusControlEntry(Widget* _self, WidgetEvent event);
int Widget_ReceiveFocusRefuseFocus(Widget* _self, WidgetEvent event);

int Widget_HandleEventMenuScreen(Widget* _self, WidgetEvent event);
int Widget_HandleEventChoice(Widget* _self, WidgetEvent event);
int Widget_HandleEventSlider(Widget* _self, WidgetEvent event);
int Widget_HandleEventTextEntry(Widget* _self, WidgetEvent event);
int Widget_HandleEventControlEntry(Widget* _self, WidgetEvent event);
int Widget_HandleEventIgnoreAll(Widget* _self, WidgetEvent event);

int Widget_HeightChoice(Widget* _self);
int Widget_HeightFullScreen(Widget* _self);
int Widget_HeightOneLine(Widget* _self);
int Widget_HeightLabel(Widget* _self);

int Widget_WidthFullScreen(Widget* _self);

void Widget_DrawMenuScreen(Widget* _self, int x, int y);
void Widget_DrawChoice(Widget* _self, int x, int y);
void Widget_DrawButton(Widget* _self, int x, int y);
void Widget_DrawLabel(Widget* _self, int x, int y);
void Widget_DrawSlider(Widget* _self, int x, int y);
void Widget_DrawTextEntry(Widget* _self, int x, int y);
void Widget_DrawControlEntry(Widget* _self, int x, int y);

void Widget_Slider_DrawValue(WidgetSlider* self, int x, int y);
void Widget_Slider_DrawPercentageValue(WidgetSlider* self, int x, int y);

void ResetOffset(void);
void LoadArrows(void);
void ReleaseArrows(void);

/* Other implementations will need these values */
extern Widget* widget_focus;

#endif // UQM_LIBS_GRAPHICS_WIDGETS_H
