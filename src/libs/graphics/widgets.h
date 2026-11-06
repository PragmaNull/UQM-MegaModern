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

#ifndef LIBS_GRAPHICS_WIDGETS_H_
#define LIBS_GRAPHICS_WIDGETS_H_

#include "core/stl/stl.h"
#include "libs/gfxlib.h"

enum
{
	WIDGET_EVENT_UP,
	WIDGET_EVENT_DOWN,
	WIDGET_EVENT_LEFT,
	WIDGET_EVENT_RIGHT,
	WIDGET_EVENT_SELECT,
	WIDGET_EVENT_CANCEL,
	WIDGET_EVENT_DELETE,
	NUM_WIDGET_EVENTS
};

typedef enum
{
	WIDGET_TYPE_MENU_SCREEN,
	WIDGET_TYPE_CHOICE,
	WIDGET_TYPE_BUTTON,
	WIDGET_TYPE_LABEL,
	WIDGET_TYPE_SLIDER,
	WIDGET_TYPE_TEXTENTRY,
	WIDGET_TYPE_CONTROLENTRY,
	NUM_WIDGET_TYPES
} WIDGET_TYPE;

inline constexpr auto WidgetTextentryWidth {32};
inline constexpr auto WidgetControlentryWidth {16};

struct WIDGET
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
};

struct WIDGET_MENU_SCREEN
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
	const char* title;
	const char* subtitle;
	STAMP bgStamp;
	int num_children;
	struct WIDGET** child;
	int highlighted;
};

struct CHOICE_OPTION
{
	const char* optname;
	const char* tooltip[3];
};

struct WIDGET_CHOICE
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
	const char* category;
	int numopts;
	CHOICE_OPTION* options;
	int selected, highlighted, choice_num;
	void (*onChange)(struct WIDGET_CHOICE* self, int oldval);
};

struct WIDGET_BUTTON
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
	const char* name;
	const char* tooltip[3];
};

struct WIDGET_LABEL
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
	int line_count;
	const char** lines;
};

struct WIDGET_SLIDER
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
	void (*draw_value)(struct WIDGET_SLIDER* self, int x, int y);
	int min, max, step;
	int value;
	const char* category;
	const char* tooltip[3];
	void (*onChange)(struct WIDGET_SLIDER* self);
};

typedef enum
{
	WTE_NORMAL = 0,
	WTE_EDITING,
	WTE_BLOCKCUR,

} WIDGET_TEXTENTRY_STATE;

inline WIDGET_TEXTENTRY_STATE operator~(WIDGET_TEXTENTRY_STATE rhs)
{
	return static_cast<WIDGET_TEXTENTRY_STATE>(~static_cast<uint32_t>(rhs));
}
inline WIDGET_TEXTENTRY_STATE operator|=(WIDGET_TEXTENTRY_STATE lhs, WIDGET_TEXTENTRY_STATE rhs)
{
	return static_cast<WIDGET_TEXTENTRY_STATE>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
inline WIDGET_TEXTENTRY_STATE operator&=(WIDGET_TEXTENTRY_STATE lhs, WIDGET_TEXTENTRY_STATE rhs)
{
	return static_cast<WIDGET_TEXTENTRY_STATE>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}


struct WIDGET_TEXTENTRY
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
	int (*handleEventSelect)(struct WIDGET_TEXTENTRY* self);
	// handleEventSelect is an overridable callback event
	// called by the default handleEvent implementation
	// can be nullptr, in which case SELECT is ignored
	void (*onChange)(struct WIDGET_TEXTENTRY* self);
	const char* category;
	char value[WidgetTextentryWidth];
	int maxlen;
	WIDGET_TEXTENTRY_STATE state;
	int cursor_pos;
	const char* tooltip[3];
};

struct WIDGET_CONTROLENTRY
{
	WIDGET_TYPE tag;
	struct WIDGET* parent;
	int (*handleEvent)(struct WIDGET* self, int event);
	int (*receiveFocus)(struct WIDGET* self, int event);
	void (*draw)(struct WIDGET* self, int x, int y);
	int (*height)(struct WIDGET* self);
	int (*width)(struct WIDGET* self);
	void (*onChange)(struct WIDGET_CONTROLENTRY* self);
	void (*onDelete)(struct WIDGET_CONTROLENTRY* self);
	const char* category;
	int controlindex;
	int highlighted;
	char controlname[2][WidgetControlentryWidth];
};

void DrawShadowedBox(GFXRECT* r, Color bg, Color dark, Color medium);
void DrawLabelAsWindow(WIDGET_LABEL* label, GFXRECT* windowRect);
void Widget_SetWindowColors(Color bg, Color dark, Color medium);
FONT Widget_SetFont(FONT newFont);


int Widget_Event(int event);

/* Methods for filling in widgets with */

int Widget_ReceiveFocusMenuScreen(WIDGET* _self, int event);
int Widget_ReceiveFocusChoice(WIDGET* _self, int event);
int Widget_ReceiveFocusSimple(WIDGET* _self, int event);
int Widget_ReceiveFocusControlEntry(WIDGET* _self, int event);
int Widget_ReceiveFocusRefuseFocus(WIDGET* _self, int event);

int Widget_HandleEventMenuScreen(WIDGET* _self, int event);
int Widget_HandleEventChoice(WIDGET* _self, int event);
int Widget_HandleEventSlider(WIDGET* _self, int event);
int Widget_HandleEventTextEntry(WIDGET* _self, int event);
int Widget_HandleEventControlEntry(WIDGET* _self, int event);
int Widget_HandleEventIgnoreAll(WIDGET* _self, int event);

int Widget_HeightChoice(WIDGET* _self);
int Widget_HeightFullScreen(WIDGET* _self);
int Widget_HeightOneLine(WIDGET* _self);
int Widget_HeightLabel(WIDGET* _self);

int Widget_WidthFullScreen(WIDGET* _self);

void Widget_DrawMenuScreen(WIDGET* _self, int x, int y);
void Widget_DrawChoice(WIDGET* _self, int x, int y);
void Widget_DrawButton(WIDGET* _self, int x, int y);
void Widget_DrawLabel(WIDGET* _self, int x, int y);
void Widget_DrawSlider(WIDGET* _self, int x, int y);
void Widget_DrawTextEntry(WIDGET* _self, int x, int y);
void Widget_DrawControlEntry(WIDGET* _self, int x, int y);

void Widget_Slider_DrawValue(WIDGET_SLIDER* self, int x, int y);
void Widget_Slider_DrawPercentageValue(WIDGET_SLIDER* self, int x, int y);

void ResetOffset(void);
void LoadArrows(void);
void ReleaseArrows(void);

/* Other implementations will need these values */
extern WIDGET* widget_focus;

#endif /* LIBS_GRAPHICS_WIDGETS_H_ */
