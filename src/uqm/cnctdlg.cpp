//Copyright Michael Martin, 2006

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

#ifdef NETPLAY

#include "cnctdlg.h"
#include "controls.h"
#include "colors.h"
#include "gamestr.h"
#include "setup.h"
#include "units.h"
#include "resinst.h"
#include "nameref.h"
#include "libs/graphics/widgets.h"
#include "options/options.h"


static auto McdWidth() { return ResScale(260); }
static auto McdHeight() { return ResScale(110); }

static constexpr auto MenuFrameRate {(c_gameSecond / 20)};

struct ConnectDialogState
{
	bool (*InputFunc)(struct ConnectDialogState* pInputState);

	TimeCount NextTime;
	bool Initialized;
	int which_side;

	int confirmed;
};

static void DrawConnectDialog(void);

static WidgetMenuScreen menu;
static WidgetButton buttons[3];
static WidgetSlider slider;
static WidgetTextEntry texts[2];

static Widget* menu_widgets[] = {
	(Widget*)&buttons[1],
	(Widget*)&texts[0],
	(Widget*)&buttons[0],
	(Widget*)&slider,
	(Widget*)&texts[1],
	(Widget*)&buttons[2]};

static bool done;

/* This kind of sucks, but the Button callbacks need access to the
 * ConnectDialogState, so we need a pointer to it */

static ConnectDialogState* current_state;

static FONT PlayerFont;

static int do_connect(Widget* self, WidgetEvent event);
static int do_listen(Widget* self, WidgetEvent event);
static int do_cancel(Widget* self, WidgetEvent event);

static void MCD_DrawMenuScreen(Widget* _self, int x, int y)
{
	const WidgetMenuScreen* self = (WidgetMenuScreen*)_self;

	int widget_y {y + ResScale(8)};
	for (int widget_index {0}; widget_index < self->num_children; widget_index++)
	{
		Widget* c = self->child[widget_index];
		(*c->draw)(c, x, widget_y);
		widget_y += (*c->height)(c) + ResScale(8);
	}
}

static void MCD_DrawButton(Widget* _self, int x, int y)
{
	const WidgetButton* self = (WidgetButton*)_self;
	Color oldtext;
	Color inactive;
	Color selected;
	FONT oldfont = SetContextFont(StarConFont);
	FRAME oldFontEffect = SetContextFontEffect(nullptr);
	GfxText t;

	selected = MenuHighlightColor;
	inactive = MenuTextColor;

	t.baseline.x = ResScale(160);
	t.baseline.y = y;
	t.align = TextAlign::Center;
	t.CharCount = ~0;
	t.pStr = self->name;
	if (widget_focus == _self)
	{
		oldtext = SetContextForeGroundColor(selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor(inactive);
	}
	font_DrawText(&t);
	SetContextFontEffect(oldFontEffect);
	SetContextFont(oldfont);
	SetContextForeGroundColor(oldtext);
	(void)x;
}

static void MCD_DrawSlider(Widget* _self, int x, int y)
{
	WidgetSlider* self = (WidgetSlider*)_self;
	Color oldtext;
	Color default_color;
	Color selected;
	FONT oldfont = SetContextFont(PlayerFont);
	FRAME oldFontEffect = SetContextFontEffect(nullptr);
	GfxText t;
	GfxRect r;
	const int tick = ResScale(ResDescale(McdWidth()) / 8);

	default_color = MenuTextColor;
	selected = MenuHighlightColor;

	t.baseline.x = x;
	t.baseline.y = y;
	t.align = TextAlign::Left;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		oldtext = SetContextForeGroundColor(selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor(default_color);
	}
	font_DrawText(&t);

	r.corner.x = t.baseline.x + 3 * tick;
	r.corner.y = t.baseline.y - ResScale(4);
	r.extent.height = ResScale(2);
	r.extent.width = 3 * tick;
	DrawFilledRectangle(&r);

	r.extent.width = ResScale(3);
	r.extent.height = ResScale(8);
	r.corner.y = t.baseline.y - ResScale(7);
	r.corner.x = t.baseline.x + 3 * tick + (3 * tick * (self->value - self->min) / (self->max - self->min))
			   - ResScale(1);
	DrawFilledRectangle(&r);

	(*self->draw_value)(self, t.baseline.x + 7 * tick, t.baseline.y);

	SetContextFontEffect(oldFontEffect);
	SetContextFont(oldfont);
	SetContextForeGroundColor(oldtext);
}

static void MCD_DrawTextEntry(Widget* _self, int x, int y)
{
	WidgetTextEntry* self = (WidgetTextEntry*)_self;
	Color oldtext;
	Color inactive;
	Color default_color;
	Color selected;
	FONT oldfont = SetContextFont(PlayerFont);
	FRAME oldFontEffect = SetContextFontEffect(nullptr);
	GfxText t;

	default_color = MenuTextColor;
	selected = MenuHighlightColor;
	inactive = MenuTextColor;

	BatchGraphics();

	t.baseline.x = x;
	t.baseline.y = y;
	t.align = TextAlign::Left;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		oldtext = SetContextForeGroundColor(selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor(default_color);
	}
	font_DrawText(&t);

	/* Force string termination */
	self->value[WidgetTextentryWidth - 1] = 0;

	t.baseline.y = y;
	t.CharCount = (uint16_t)uqm::utf8StringCount(self->value);
	t.pStr = self->value;

	if (!any(self->state, TextEntryWidgetState::Editing))
	{ // normal or selected state
		t.baseline.x = ResScale(160);
		t.align = TextAlign::Center;

		if (widget_focus == _self)
		{
			oldtext = SetContextForeGroundColor(selected);
		}
		else
		{
			oldtext = SetContextForeGroundColor(inactive);
		}
		font_DrawText(&t);
	}
	else
	{ // editing state
		uint16_t i {};
		GfxRect text_r;
		uint8_t char_deltas[WidgetTextentryWidth];
		GfxRect r;
		int16_t leading {};

		t.baseline.x = x + (ResScale(90));
		t.align = TextAlign::Left;

		// calc background box dimensions
		// XXX: this may need some tuning, especially if a
		//   different font is used. The font 'leading' values
		//   are not what they should be.
		const auto BoxVertOffset {ResScale(2)};
		GetContextFontLeading(&leading);
		r.corner.x = t.baseline.x - ResScale(1);
		r.corner.y = t.baseline.y - leading + BoxVertOffset;
		r.extent.width = McdWidth() - r.corner.x - ResScale(10);
		r.extent.height = leading - ResScale(1);

		TextRect(&t, &text_r, char_deltas);
#if 0
		// XXX: this should potentially be used in ChangeCallback
		if ((text_r.extent.width + 2) >= r.extent.width)
		{	// the text does not fit the input box size and so
			// will not fit when displayed later
			UnbatchGraphics ();
			// disallow the change
			return (false);
		}
#endif

		oldtext = SetContextForeGroundColor(selected);
		DrawFilledRectangle(&r);

		// calculate the cursor position and draw it
		const uint8_t* pchar_deltas {char_deltas};
		for (i = self->cursor_pos; i > 0; --i)
		{
			r.corner.x += (int16_t)*pchar_deltas++;
		}
		if (self->cursor_pos < t.CharCount) /* cursor mid-line */
		{
			r.corner.x -= ResScale(1);
		}

		if (any(self->state, TextEntryWidgetState::BlockCursor))
		{ // Use block cursor for keyboardless systems

			r.corner.y = r.corner.y;
			r.extent.height = r.extent.height;

			if (self->cursor_pos == t.CharCount)
			{ // cursor at end-line -- use insertion point
				r.extent.width = ResScale(1);
				r.corner.x -= IfHd(3);
			}
			else if (self->cursor_pos + 1 == t.CharCount)
			{ // extra pixel for last char margin
				r.extent.width = (int16_t)*pchar_deltas - IfHd(3);
				r.corner.x += ResScale(1);
			}
			else
			{ // normal mid-line char
				r.extent.width = (int16_t)*pchar_deltas;
				r.corner.x += ResScale(1);
			}
		}
		else
		{ // Insertion point cursor
			r.corner.y = r.corner.y + ResScale(1);
			r.extent.height = r.extent.height - ResScale(2);
			r.extent.width = ResScale(1);

			if (self->cursor_pos == t.CharCount)
			{
				text_r.corner.x -= IfHd(3);
			}
		}
		// position cursor within input field rect
		r.corner.x += ResScale(1);
		SetContextForeGroundColor(MenuCursorColor);
		DrawFilledRectangle(&r);

		SetContextForeGroundColor(inactive);
		font_DrawText(&t);
	}

	UnbatchGraphics();
	SetContextFontEffect(oldFontEffect);
	SetContextFont(oldfont);
	SetContextForeGroundColor(oldtext);
}

/* Text entry stuff, mostly C&Ped from setupmenu.c.  Could use some
 * refactoring, as redraw_menu () is the only real change. */

static bool OnTextEntryChange(TextEntryState* pTES)
{
	WidgetTextEntry* widget = (WidgetTextEntry*)pTES->CbParam;

	widget->cursor_pos = pTES->CursorPos;
	if (pTES->JoystickMode)
	{
		widget->state |= TextEntryWidgetState::BlockCursor;
	}
	else
	{
		widget->state &= ~TextEntryWidgetState::BlockCursor;
	}

	// XXX TODO: Here, we can examine the text entered so far
	// to make sure it fits on the screen, for example,
	// and return false to disallow the last change

	return true; // allow change
}

static bool OnTextEntryFrame(TextEntryState* pTES)
{
	DrawConnectDialog();

	SleepThreadUntil(pTES->NextTime);
	pTES->NextTime = GetTimeCounter() + MenuFrameRate;

	(void)pTES;	 // satisfying compiler
	return true; // continue
}

static int OnTextEntryEvent(WidgetTextEntry* widget)
{ // Going to edit the text
	TextEntryState tes;
	char revert_buf[256];

	// position cursor at the end of text
	widget->cursor_pos = uqm::utf8StringCount(widget->value);
	widget->state = TextEntryWidgetState::Editing;
	DrawConnectDialog();

	// make a backup copy for revert on cancel
	uqm::utf8StringCopy(revert_buf, sizeof(revert_buf), widget->value);

	// text entry setup
	tes.Initialized = false;
	tes.NextTime = GetTimeCounter() + MenuFrameRate;
	tes.BaseStr = widget->value;
	tes.MaxSize = widget->maxlen;
	tes.CursorPos = widget->cursor_pos;
	tes.CbParam = widget;
	tes.ChangeCallback = OnTextEntryChange;
	tes.FrameCallback = OnTextEntryFrame;

	// SetMenuSounds (0, MenuSoundSelect);
	if (!DoTextEntry(&tes))
	{ // editing failed (canceled) -- revert the changes
		uqm::utf8StringCopy(widget->value, widget->maxlen, revert_buf);
	}
	else
	{
		if (widget->onChange)
		{
			(*(widget->onChange))(widget);
		}
	}

	widget->state = TextEntryWidgetState::Normal;
	DrawConnectDialog();

	return true; // event handled
}

/* Button response routines */

static int do_connect(Widget* self, WidgetEvent event)
{
	if (event == WidgetEvent::Select)
	{
		/* These assignments are safe exactly because texts[] is file-scope) */
		auto& netplayOptions {uqm::UQMOptions::getInstance().getNetplayOptions()};
		auto& peerData {netplayOptions.editPeers()[current_state->which_side]};
		peerData.address.host = texts[0].value;
		peerData.address.port = std::stoul(texts[1].value);
		peerData.isServer = false;
		current_state->confirmed = true;
		netplayOptions.setFrameInputDelay(slider.value);
		done = true;
	}
	(void)self;
	return false;
}

static int do_listen(Widget* self, WidgetEvent event)
{
	if (event == WidgetEvent::Select)
	{
		/* These assignments are safe exactly because texts[] is file-scope) */
		auto& netplayOptions {uqm::UQMOptions::getInstance().getNetplayOptions()};
		auto& peerData {netplayOptions.editPeers()[current_state->which_side]};
		peerData.address.port = std::stoul(texts[1].value);
		peerData.isServer = true;
		netplayOptions.setFrameInputDelay(slider.value);
		current_state->confirmed = true;
		done = true;
	}
	(void)self;
	return false;
}

static int do_cancel(Widget* self, WidgetEvent event)
{
	if (event == WidgetEvent::Select)
	{
		current_state->confirmed = false;
		done = true;
	}
	(void)self;
	return false;
}


static void CreateWidgets(void)
{
	int i {};

	done = false;

	for (i = 0; i < 3; i++)
	{
		buttons[i].tag = WidgetType::Button;
		buttons[i].parent = nullptr;
		buttons[i].receiveFocus = Widget_ReceiveFocusSimple;
		buttons[i].draw = MCD_DrawButton;
		buttons[i].height = Widget_HeightOneLine;
		buttons[i].width = Widget_WidthFullScreen;
	}
	buttons[0].name = GameString(GameStringBase::NetmeleeString + 19);
	// "Connect to remote host"
	buttons[1].name = GameString(GameStringBase::NetmeleeString + 20);
	// "Wait for incoming connection"
	buttons[2].name = GameString(GameStringBase::NetmeleeString + 21);
	// "Cancel"

	buttons[0].handleEvent = do_connect;
	buttons[1].handleEvent = do_listen;
	buttons[2].handleEvent = do_cancel;

	menu.tag = WidgetType::MenuScreen;
	menu.parent = nullptr;
	menu.receiveFocus = Widget_ReceiveFocusMenuScreen;
	menu.draw = MCD_DrawMenuScreen;
	menu.height = Widget_HeightFullScreen;
	menu.width = Widget_WidthFullScreen;
	menu.num_children = 6;
	menu.child = menu_widgets;
	menu.handleEvent = Widget_HandleEventMenuScreen;

	auto& netplayOptions {uqm::UQMOptions::getInstance().getNetplayOptions()};

	slider.tag = WidgetType::Slider;
	slider.parent = nullptr;
	slider.handleEvent = Widget_HandleEventSlider;
	slider.receiveFocus = Widget_ReceiveFocusSimple;
	slider.draw = MCD_DrawSlider;
	slider.height = Widget_HeightOneLine;
	slider.width = Widget_WidthFullScreen;
	slider.draw_value = Widget_Slider_DrawValue;
	slider.min = 0;
	slider.max = netplayOptions.getMaxFrameInputDelay();
	slider.step = 1;
	slider.value = netplayOptions.getFrameInputDelay();
	slider.category = GameString(GameStringBase::NetmeleeString + 24);
	// "Net Delay"

	for (i = 0; i < 2; i++)
	{
		texts[i].tag = WidgetType::TextEntry;
		texts[i].parent = nullptr;
		texts[i].handleEvent = Widget_HandleEventTextEntry;
		texts[i].receiveFocus = Widget_ReceiveFocusSimple;
		texts[i].draw = MCD_DrawTextEntry;
		texts[i].height = Widget_HeightOneLine;
		texts[i].width = Widget_WidthFullScreen;
		texts[i].handleEventSelect = OnTextEntryEvent;
		texts[i].maxlen = WidgetTextentryWidth - ResScale(1);
		texts[i].state = TextEntryWidgetState::Normal;
		texts[i].cursor_pos = 0;
	}

	texts[0].category = GameString(GameStringBase::NetmeleeString + 22);
	// "Host"
	texts[1].category = GameString(GameStringBase::NetmeleeString + 23);
	// "Port"

	/* We sometimes assign to these internals; cannot strncpy over self! */
	auto& peerData {netplayOptions.editPeers()[current_state->which_side]};
	if (texts[0].value != peerData.address.host)
	{
		memset(texts[0].value, 0, sizeof(texts[0].value));
		const uqstl::string_view toCopy {peerData.address.host.c_str(), std::min(peerData.address.host.size(), static_cast<size_t>(texts[0].maxlen))};
		uqm::strncpy_safe(texts[0].value, toCopy);
	}
	fmt::format_to_sz_n(texts[1].value, std::min(sizeof(texts[1].value), static_cast<size_t>(texts[1].maxlen)), "{}", peerData.address.port);

	menu.receiveFocus((Widget*)&menu, WidgetEvent::Down);
}

static void DrawConnectDialog(void)
{
	GfxRect r;

	r.extent.width = McdWidth();
	r.extent.height = McdHeight();
	r.corner.x = (CanvasWidth - r.extent.width) >> 1;
	r.corner.y = (CanvasHeight - r.extent.height) >> 1;


	DrawShadowedBox(&r, ShadowboxBackgroundColor,
					ShadowboxDarkColor, ShadowboxMediumColor);

	menu.draw((Widget*)&menu,
			  r.corner.x + ResScale(10), r.corner.y + ResScale(10));
}

static bool DoMeleeConnectDialog(ConnectDialogState* state)
{
	bool changed {};

	/* Cancel any presses of the Pause key. */
	GamePaused = false;

	if (!state->Initialized)
	{
		state->Initialized = true;
		SetDefaultMenuRepeatDelay();
		state->NextTime = GetTimeCounter();
		/* Prepare widgets, draw stuff, etc. */
		CreateWidgets();
		DrawConnectDialog();
	}

	changed = true;

	if (PulsedInputState.menu[*MenuKey::MenuUp])
	{
		Widget_Event(WidgetEvent::Up);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuDown])
	{
		Widget_Event(WidgetEvent::Down);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuLeft])
	{
		Widget_Event(WidgetEvent::Left);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuRight])
	{
		Widget_Event(WidgetEvent::Right);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuSelect])
	{
		Widget_Event(WidgetEvent::Select);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuCancel])
	{
		Widget_Event(WidgetEvent::Cancel);
	}
	else if (PulsedInputState.menu[*MenuKey::MenuDelete])
	{
		Widget_Event(WidgetEvent::Delete);
	}
	else
	{
		changed = false;
	}

	if (changed)
	{
		DrawConnectDialog();
	}

	SleepThreadUntil(state->NextTime + MenuFrameRate);
	state->NextTime = GetTimeCounter();
	return !((uqm::currentActivity() & ActivityFlag::CheckAbort) || done);
}

bool MeleeConnectDialog(int side)
{
	ConnectDialogState state;

	PlayerFont = LoadFont(PlayerFontRes);

	state.Initialized = false;
	state.which_side = side;
	state.InputFunc = DoMeleeConnectDialog;
	state.confirmed = true;

	current_state = &state;

	DoInput(&state, true);

	current_state = nullptr;

	DestroyFont(PlayerFont);

	return state.confirmed;
}

#endif /* NETPLAY */
