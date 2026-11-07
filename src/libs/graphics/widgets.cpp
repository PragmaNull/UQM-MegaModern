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

#include "widgets.h"

#include <stdio.h>
#include <string.h>

#include "gfx_common.h"
#include "options.h"
#include "uqm/colors.h"
#include "uqm/igfxres.h"
#include "uqm/nameref.h"
#include "uqm/units.h"
#include "uqm/util.h"

Widget* widget_focus = nullptr;

/* Some basic color defines */
static constexpr auto WidgetActiveColor {uqm::MenuHighlightColor};
static constexpr auto WidgetInactiveColor {BuildColor(MakeRgb15(0x18, 0x18, 0x1F), 0x00)};

static constexpr auto WidgetInactiveSelectedColor {uqm::WhiteColor};
static constexpr auto WidgetCursorColor {uqm::BlackColor};

static constexpr auto WidgetDialogTextColor {WidgetCursorColor};
static constexpr auto WidgetBonusColor {BuildColorRgba(0xDF, 0x6A, 0x13, 0xFF)};

static constexpr auto WidgetEnabledColor {BuildColor(MakeRgb15(0x00, 0x18, 0x00), 0x00)};
static constexpr auto WidgetDisabledColor {uqm::PcmenuTopLeftBorderColor};
static constexpr auto WidgetTooltipColor {BuildColor(MakeRgb15(0xD2, 0xB4, 0x8C), 0x00)};
static constexpr auto WidgetLabelColor {BuildColorRgba(0x00, 0x77, 0x77, 0xFF)};
static constexpr auto WidgetWarningColor {BuildColorRgba(0xB1, 0x27, 0x47, 0xFF)};

static auto Onscreen() { return uqm::DosBool(13, 10); }
// The pos from the page edge where we need to start scrolling
static constexpr auto ScrollOffset {3};
static auto ScreenCenter() { return uqm::ResScale(uqm::ResDescale(CanvasWidth) / 2); }
static auto Lstep() { return uqm::ResScale(uqm::ResDescale(CanvasWidth) / 2 - 7); }
static auto Rstep() { return uqm::ResScale(uqm::ResDescale(CanvasWidth) / 2 + 7); }

static Color win_bg_clr =
	BuildColor(MakeRgb15(0x18, 0x18, 0x1F), 0x00);
static Color win_medium_clr =
	BuildColor(MakeRgb15(0x10, 0x10, 0x18), 0x00);
static Color win_dark_clr =
	BuildColor(MakeRgb15(0x08, 0x08, 0x10), 0x00);

static FontRef cur_font;

static uint16_t offset_t = 0;		   // Top widget offset
static uint16_t offset_b;			   // = Onscreen(); // Bottom widget offset
static FrameRef arrow_frame = nullptr; // Frames for additional graphics

void ResetOffset(void)
{ // To reset offsets while traversing different moves
	offset_t = 0;
	offset_b = Onscreen();
}

void LoadArrows(void)
{
	if (arrow_frame == nullptr || optRequiresReload)
	{
		// Load the different arrows depending on the resolution factor.
		arrow_frame = CaptureDrawable(uqm::LoadGraphic(uqm::MenuarrPmapAnimRes));
	}
}

void ReleaseArrows(void)
{ // Release graphics
	if (arrow_frame)
	{
		DestroyDrawable(ReleaseDrawable(arrow_frame));
		arrow_frame = nullptr;
	}
}

void DrawShadowedBox(GfxRect* r, Color bg, Color dark, Color medium)
{ // Dialog box
	uqm::DrawStarConBox(r, uqm::ResScale(2), dark, medium, true, bg, true,
						uqm::Transparent);
}

// windowRect, if not nullptr, will be filled with the dimensions of the
// window drawn.
void DrawLabelAsWindow(WidgetLabel* label, GfxRect* windowRect)
{
	const Color oldfg = SetContextForeGroundColor(WidgetDialogTextColor);
	FontRef oldfont = 0;
	FrameRef oldFontEffect = SetContextFontEffect(nullptr);
	GfxRect r;
	GfxText t;
	int i {};
	int win_w {};
	int win_h {};

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	/* Compute the dimensions of the label */
	win_h = label->height((Widget*)label) + uqm::ResScale(16);
	win_w = 0;
	for (i = 0; i < label->line_count; i++)
	{
		const size_t len = uqm::utf8StringCount(label->lines[i]);
		if (len > win_w)
		{
			win_w = uqm::ResScale(len);
		}
	}
	win_w = (win_w * 6) + uqm::ResScale(16);

	{
		const BatchScope batch {};
		r.corner.x = uqm::ResScale(
						 (uqm::ResDescale(CanvasWidth) - uqm::ResDescale(win_w)) >> 1)
				   - uqm::ResScale(2);
		r.corner.y = uqm::ResScale(
						 (uqm::ResDescale(CanvasHeight) - uqm::ResDescale(win_h)) >> 1)
				   - uqm::ResScale(2);
		r.extent.width = win_w + uqm::ResScale(4);
		r.extent.height = win_h + uqm::ResScale(4);

		DrawShadowedBox(&r, win_bg_clr, win_dark_clr, win_medium_clr);

		t.baseline.x = r.corner.x
					 + uqm::ResScale(uqm::ResDescale(r.extent.width) >> 1);
		t.baseline.y = r.corner.y + uqm::ResScale(16);
		for (i = 0; i < label->line_count; i++)
		{
			t.pStr = label->lines[i];
			t.align = TextAlign::Center;
			t.CharCount = (uint16_t)~0;
			font_DrawText(&t);
			t.baseline.y += uqm::ResScale(8);
		}
	}

	SetContextFontEffect(oldFontEffect);
	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldfg);

	if (windowRect != nullptr)
	{ // Add the outer border added by DrawShadowedBox.
		// XXX: It may be nicer to add a border size parameter to
		// DrawShadowedBox, instead of assuming 2 here.
		windowRect->corner.x = r.corner.x;
		windowRect->corner.y = r.corner.y;
		windowRect->extent.width = r.extent.width;
		windowRect->extent.height = r.extent.height;
	}
}

void Widget_SetWindowColors(Color bg, Color dark, Color medium)
{
	win_bg_clr = bg;
	win_dark_clr = dark;
	win_medium_clr = medium;
}

FontRef Widget_SetFont(FontRef newFont)
{
	FontRef oldFont = cur_font;
	cur_font = newFont;
	return oldFont;
}

static void Widget_DrawToolTips(int numlines, const char** tips)
{
	// This functions draws the text at the bottom of the screen
	// which explains what the current option does.
	GfxRect r;
	FontRef oldfont = 0;
	const FontEffectScope fontEffectScope {nullptr};
	const ForegroundColorScope colorScope {WidgetTooltipColor};
	GfxText t;
	const char* amperBang = "&!";
	const size_t abSize = strlen(amperBang);
	bool warning = false;

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	r.corner.x = uqm::ResScale(2);
	r.corner.y = uqm::ResScale(8);
	r.extent.width = CanvasWidth - uqm::ResScale(4);
	r.extent.height = CanvasHeight + uqm::ResScale(3);

	t.align = TextAlign::Center;
	t.CharCount = ~0;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y
				 + (r.extent.height - uqm::ResScale(8) - uqm::ResScale(8) * numlines);

	for (int i {0}; i < numlines; i++)
	{
		t.pStr = tips[i];
		warning = (bool)(strncmp(amperBang, t.pStr, abSize) == 0);

		if (warning)
		{
			t.pStr += abSize;
			SetContextForeGroundColor(WidgetWarningColor);
		}

		font_DrawText(&t);

		if (warning)
		{
			SetContextForeGroundColor(WidgetTooltipColor);
		}

		t.baseline.y += uqm::ResScale(9);
	}

	if (oldfont)
	{
		SetContextFont(oldfont);
	}
}

void Widget_DrawMenuScreen(Widget* _self, int x, int y)
{ // Main menu function that draws backgroung and all widgets
	GfxRect r;
	Color title;
	Color oldtext;
	FontRef oldfont = 0;
	FrameRef oldFontEffect = SetContextFontEffect(nullptr);
	GfxText t;
	int widget_index {};
	int height {};
	int widget_y {};
	int on_screen {};

	WidgetMenuScreen* self = (WidgetMenuScreen*)_self;

	if (!offset_b)
	{
		offset_b = Onscreen();
	}

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	r.corner.x = uqm::ResScale(2);
	r.corner.y = uqm::ResScale(1);
	r.extent.width = CanvasWidth - uqm::ResScale(4);
	r.extent.height = CanvasHeight - uqm::ResScale(4);

	title = WidgetInactiveSelectedColor;

	DrawStamp(&self->bgStamp);

	oldtext = SetContextForeGroundColor(title);
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + uqm::ResScale(6);
	t.pStr = self->title;
	t.align = TextAlign::Center;
	t.CharCount = ~0;
	font_DrawText(&t);
	t.baseline.y += uqm::ResScale(8);
	t.pStr = self->subtitle;
	font_DrawText(&t);

	height = 0;
	for (widget_index = 0; widget_index
						   < self->num_children;
		 widget_index++)
	{ // Calculate overall height until we hit top limit
		// (more widgets can't fit the page)
		Widget* child = self->child[widget_index];
		if (widget_index <= Onscreen())
		{
			height += (*child->height)(child);
			height += uqm::ResScale(5); /* spacing */
		}
	}

	height -= uqm::ResScale(8);
	widget_y = (CanvasHeight - height) >> 1;

	{ // Scrolling
		if (self->highlighted > (offset_b - ScrollOffset)
			&& offset_b < self->num_children - 1)
		{
			if (self->highlighted == self->num_children - 1
				&& offset_t != self->highlighted - (Onscreen() - ScrollOffset))
			{ // smooth wrapping scroll
				offset_t += 1;
				offset_b += 1;
			}
			else
			{ // standart scroll
				offset_b = self->highlighted + ScrollOffset;

				if (offset_b > self->num_children - 1)
				{ // reached bottom widget
					offset_b = self->num_children - 1;
					// cap bottom offset - stop further scrolling
					offset_t = offset_b - Onscreen(); // cap top offset
				}
				else // still moving
				{
					offset_t = self->highlighted
							 - (Onscreen() - ScrollOffset);
				}
			}
		}
		if (self->highlighted < (offset_t + ScrollOffset) && offset_t > 0)
		{
			if (self->highlighted < offset_t && offset_t != 0)
			{ // smooth wrapping scroll
				offset_t -= 1;
				offset_b -= 1;
			}
			else
			{ // standart scroll
				offset_t = self->highlighted - ScrollOffset;
				if (offset_t > 65500)				  // unit16 overflow
				{									  // reached top widget
					offset_t = 0;					  // cap top offset - stop further scrolling
					offset_b = offset_t + Onscreen(); // cap bottom offset
				}
				else // still moving
				{
					offset_b = self->highlighted + (Onscreen() - ScrollOffset);
				}
			}
		}
		if (self->num_children > Onscreen())
		{ // Arrows (blue to the right)
			GfxStamp arr;

			arr.origin.x = uqm::ResScale(290);

			if (offset_t != 0)
			{
				arr.frame = SetAbsFrameIndex(arrow_frame, 0); // Up arrow
				arr.origin.y = uqm::ResScale(25);
				DrawStamp(&arr);
			}
			if (offset_b != self->num_children - 1)
			{
				arr.frame = SetAbsFrameIndex(arrow_frame, 1);
				// Down arrow
				arr.origin.y = uqm::ResScale(195);
				DrawStamp(&arr);
			}
		}
	}
	// Determine how much widgets we're gonna draw
	on_screen = self->num_children;

	if ((offset_b + 1) < self->num_children)
	{
		on_screen = offset_b + 1;
	}

	for (widget_index = offset_t; widget_index < on_screen; widget_index++)
	{
		Widget* c = self->child[widget_index];
		(*c->draw)(c, 0, widget_y);
		widget_y += (*c->height)(c) + uqm::ResScale(5);
	}

	SetContextFontEffect(oldFontEffect);
	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldtext);

	(void)x;
	(void)y;
}

void Widget_DrawChoice(Widget* _self, int x, int y)
{ // Choice drawer
	const WidgetChoice* self = (WidgetChoice*)_self;
	Color oldtext;
	Color default_color;
	Color selected;
	Color enabled;
	Color disabled;
	Color canbe;
	FontRef oldfont = 0;
	const FontEffectScope fontEffectScope {nullptr};
	GfxText t;
	int i {};

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	default_color = WidgetInactiveSelectedColor; // White
	enabled = WidgetEnabledColor;				 // Green
	disabled = WidgetDisabledColor;				 // Grey
	selected = WidgetActiveColor;				 // Yellow
	canbe = WidgetBonusColor;					 // Orange

	t.baseline.x = x + Lstep();
	t.baseline.y = y;
	t.align = TextAlign::Right;
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
	font_DrawText(&t); // Choicer name

	{ // Choicer options
		t.baseline.x = Rstep();
		t.align = TextAlign::Left;
		for (i = 0; i < self->numopts; i++)
		{
			if (i == self->selected && widget_focus != _self)
			{ // Unfocused widget current option (White)
				t.pStr = self->options[i].optname;
				font_DrawText(&t);
			}

			if (widget_focus == _self && self->highlighted == i)
			{ // Focused widget
				if (i == self->selected)
				{ // Currently chosen option (Yellow)
					SetContextForeGroundColor(selected);
				}
				else
				{ // Available option (Orange)
					SetContextForeGroundColor(canbe);
				}
				Widget_DrawToolTips(3, self->options[i].tooltip.data());
				t.pStr = self->options[i].optname;
				font_DrawText(&t); // Draw only 1 option name at a time

				{ // Arrows around widget option
					GfxStamp arr;

					arr.origin.x = font_GetTextRect(&t).corner.x
								 - uqm::ResScale(10);
					arr.origin.y = t.baseline.y;

					// Left arrow
					arr.frame = SetAbsFrameIndex(arrow_frame, 2);
					DrawStamp(&arr);

					arr.origin.x = font_GetTextRect(&t).corner.x
								 + font_GetTextRect(&t).extent.width
								 + uqm::ResScale(10);

					// Right arrow
					arr.frame = SetAbsFrameIndex(arrow_frame, 3);
					DrawStamp(&arr);
				}

				{ // Navigation Dots
					GfxRect d;

					d.extent.width = uqm::ResScale(4);
					d.extent.height = uqm::ResScale(1);

					d.corner.x = t.baseline.x - uqm::ResScale(6);
					d.corner.y = t.baseline.y + uqm::ResScale(4);

					for (uint16_t c {0}; c < self->numopts; c++)
					{
						d.corner.x += uqm::ResScale(6);

						if (c == self->highlighted)
						{
							SetContextForeGroundColor(enabled);
						}
						else
						{
							SetContextForeGroundColor(disabled);
						}

						DrawFilledRectangle(&d);
					}
				}
			}
		}
	}
	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldtext);
}

void Widget_DrawButton(Widget* _self, int x, int y)
{ // Several buttons for navigation
	WidgetButton* self = (WidgetButton*)_self;
	Color oldtext;
	Color inactive;
	Color selected;
	FontRef oldfont = 0;
	{
		const FontEffectScope fontEffectScope {nullptr};
		GfxText t;

		if (cur_font)
		{
			oldfont = SetContextFont(cur_font);
		}

		selected = WidgetActiveColor;
		inactive = WidgetInactiveColor;
		t.baseline.x = ScreenCenter();
		t.baseline.y = y;
		t.align = TextAlign::Center;
		t.CharCount = ~0;
		t.pStr = self->name;
		if (widget_focus == _self)
		{
			Widget_DrawToolTips(3, self->tooltip.data());
			oldtext = SetContextForeGroundColor(selected);
		}
		else
		{
			oldtext = SetContextForeGroundColor(inactive);
		}
		font_DrawText(&t);
	}
	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldtext);
	(void)x;
}

void Widget_DrawLabel(Widget* _self, int x, int y)
{ // Special labels
	const WidgetLabel* self = (WidgetLabel*)_self;
	const Color oldtext = SetContextForeGroundColor(WidgetLabelColor);
	FontRef oldfont = 0;
	FrameRef oldFontEffect = SetContextFontEffect(nullptr);
	GfxText t;
	GfxRect r;
	const char* amperScore = "&_";
	const char* amperBang = "&!";
	const size_t asSize = strlen(amperScore);
	bool underline = false;
	bool warning = false;

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	t.baseline.x = ScreenCenter();
	t.baseline.y = y;
	t.align = TextAlign::Center;
	t.CharCount = ~0;

	for (int i {0}; i < self->line_count; i++)
	{
		t.pStr = self->lines[i];
		underline = (bool)(strncmp(amperScore, t.pStr, asSize) == 0);
		warning = (bool)(strncmp(amperBang, t.pStr, asSize) == 0);

		if (underline)
		{
			t.pStr += asSize;
		}

		if (warning)
		{
			t.pStr += asSize;
			SetContextForeGroundColor(WidgetWarningColor);
		}

		font_DrawText(&t);
		t.baseline.y += uqm::ResScale(10);

		if (warning)
		{
			SetContextForeGroundColor(WidgetLabelColor);
		}

		if (underline)
		{ // Underline labels
			r = font_GetTextRect(&t);
			r.corner.y += uqm::ResScale(1);
			r.extent.height = uqm::ResScale(1);
			DrawFilledRectangle(&r);
		}
	}

	SetContextFontEffect(oldFontEffect);
	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldtext);

	(void)x;
}

void Widget_DrawSlider(Widget* _self, int x, int y)
{ // SFX slider
	WidgetSlider* self = (WidgetSlider*)_self;
	Color oldtext;
	Color default_color;
	Color selected;
	FontRef oldfont = 0;
	const FontEffectScope fontEffectScope {nullptr};
	GfxText t;
	GfxRect r;
	const int tick = (CanvasWidth - x) / 8;

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	default_color = WidgetInactiveSelectedColor;
	selected = WidgetActiveColor;

	t.baseline.x = x + Lstep();
	t.baseline.y = y;
	t.align = TextAlign::Right;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		Widget_DrawToolTips(3, self->tooltip.data());
		oldtext = SetContextForeGroundColor(selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor(default_color);
	}
	font_DrawText(&t);

	// Slider Bar
	r.corner.x = Rstep();
	r.corner.y = t.baseline.y - uqm::ResScale(4);
	r.extent.height = uqm::ResScale(2);
	r.extent.width = 2 * tick;
	const int slider_width {r.corner.x + r.extent.width};
	DrawFilledRectangle(&r);

	// Slider Indicator
	r.extent.width = uqm::ResScale(3);
	r.extent.height = uqm::ResScale(8);
	r.corner.y = t.baseline.y - uqm::ResScale(7);
	r.corner.x = Rstep() + (2 * tick * (self->value - self->min) / (self->max - self->min)) - (uqm::ResScale(3) >> 1);
	DrawFilledRectangle(&r);

	//printf("%d + 3 * %d - 22 * %d\n", Rstep(), tick, tick);
	(*self->draw_value)(self, slider_width, t.baseline.y);

	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldtext);
}

void Widget_Slider_DrawValue(WidgetSlider* self, int x, int y)
{ // Number from 0 to 100 near slider
	GfxText t;
	uqstl::array<char, 16> buffer {};

	fmt::format_to_n(buffer.data(), sizeof(buffer) - 1, "{}", self->value);

	t.baseline.x = x + uqm::ResScale(14);
	t.baseline.y = y;
	t.align = TextAlign::Left;
	t.CharCount = ~0;
	t.pStr = buffer.data();

	font_DrawText(&t);
}

void Widget_Slider_DrawPercentageValue(WidgetSlider* self, int x, int y)
{ // Number from 0% to 100% near slider
	GfxText t;
	uqstl::array<char, 16> buffer {};

	fmt::format_to_n(buffer.data(), sizeof(buffer) - 1, "{}%", self->value);

	t.baseline.x = x + uqm::ResScale(14);
	t.baseline.y = y;
	t.align = TextAlign::Left;
	t.CharCount = ~0;
	t.pStr = buffer.data();

	font_DrawText(&t);
}

void Widget_DrawTextEntry(Widget* _self, int x, int y)
{ // Custom seed and key layout name
	WidgetTextEntry* self = (WidgetTextEntry*)_self;
	Color oldtext;
	Color inactive;
	Color default_color;
	Color selected;
	Color editing;
	FontRef oldfont = 0;
	FrameRef oldFontEffect = SetContextFontEffect(nullptr);
	GfxText t;

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	default_color = WidgetInactiveSelectedColor;
	selected = WidgetActiveColor;
	inactive = WidgetInactiveColor;
	editing = WidgetLabelColor;

	BatchGraphics();

	t.baseline.x = x + Lstep();
	t.baseline.y = y;
	t.align = TextAlign::Right;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		Widget_DrawToolTips(3, self->tooltip.data());
		oldtext = SetContextForeGroundColor(selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor(default_color);
	}
	font_DrawText(&t);

	t.baseline.x -= t.baseline.x;

	/* Force string termination */
	self->value[WidgetTextentryWidth - uqm::ResScale(1)] = 0;

	t.baseline.y = y;
	t.CharCount = (uint16_t)uqm::utf8StringCount(self->value.data());
	t.pStr = self->value.data();

	if (!any(self->state, TextEntryWidgetState::Editing))
	{ // normal or selected state
		t.baseline.x = Rstep();
		t.align = TextAlign::Left;

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
		uqstl::array<uint8_t, WidgetTextentryWidth> char_deltas;
		GfxRect r;
		int16_t leading {};

		t.baseline.x = Rstep();
		t.align = TextAlign::Left;

		// calc background box dimensions
		// XXX: this may need some tuning, especially if a
		//   different font is used. The font 'leading' values
		//   are not what they should be.
		const auto BoxVertOffset {uqm::ResScale(2)};
		GetContextFontLeading(&leading);
		r.corner.x = t.baseline.x - uqm::ResScale(1);
		r.corner.y = t.baseline.y - leading + BoxVertOffset;
		r.extent.width = CanvasWidth - r.corner.x - uqm::ResScale(10);
		r.extent.height = leading - uqm::ResScale(1);

		TextRect(&t, &text_r, char_deltas.data());
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
		const uint8_t* pchar_deltas {char_deltas.data()};
		for (i = self->cursor_pos; i > 0; --i)
		{
			r.corner.x += (int16_t)*pchar_deltas++;
		}
		if (self->cursor_pos < t.CharCount) /* cursor mid-line */
		{
			r.corner.x -= uqm::ResScale(1);
		}

		if (any(self->state, TextEntryWidgetState::BlockCursor))
		{ // Use block cursor for keyboardless systems

			r.corner.y = r.corner.y;
			r.extent.height = r.extent.height;

			if (self->cursor_pos == t.CharCount)
			{ // cursor at end-line -- use insertion point
				r.extent.width = uqm::ResScale(1);
				r.corner.x -= uqm::IfHd(3);
			}
			else if (self->cursor_pos + 1 == t.CharCount)
			{ // extra pixel for last char margin
				r.extent.width = (int16_t)*pchar_deltas - uqm::IfHd(3);
				r.corner.x += uqm::ResScale(1);
			}
			else
			{ // normal mid-line char
				r.extent.width = (int16_t)*pchar_deltas;
				r.corner.x += uqm::ResScale(1);
			}
		}
		else
		{ // Insertion point cursor
			r.corner.y = r.corner.y + uqm::ResScale(1);
			r.extent.height = r.extent.height - uqm::ResScale(2);
			r.extent.width = uqm::ResScale(1);

			if (self->cursor_pos == t.CharCount)
			{
				r.corner.x -= uqm::IfHd(3);
			}
		}
		// position cursor within input field rect
		r.corner.x += uqm::ResScale(1);
		SetContextForeGroundColor(WidgetCursorColor);
		DrawFilledRectangle(&r);

		SetContextForeGroundColor(editing);
		font_DrawText(&t);
	}

	UnbatchGraphics();
	SetContextFontEffect(oldFontEffect);
	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldtext);
}

void Widget_DrawControlEntry(Widget* _self, int x, int y)
{ // mappable key name in controls setup
	WidgetControlEntry* self = (WidgetControlEntry*)_self;
	Color oldtext;
	Color default_color;
	Color selected;
	FontRef oldfont = 0;
	const FontEffectScope fontEffectScope {nullptr};
	GfxText t;

	if (cur_font)
	{
		oldfont = SetContextFont(cur_font);
	}

	default_color = WidgetInactiveSelectedColor;
	selected = WidgetActiveColor;

	t.baseline.x = x + uqm::ResScale(16);
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
	font_DrawText(&t); // Control Name E.G. Up, Down, Weapon, Thrust

	t.baseline.x -= t.baseline.x;

	const int home_x {t.baseline.x + (CanvasWidth / 3)};
	const int home_y {t.baseline.y};
	t.align = TextAlign::Left;
	for (int i {0}; i < 2; i++)
	{
		t.baseline.x = home_x + ((i % 3) * (CanvasWidth / 3));
		t.baseline.y = home_y + uqm::ResScale(8 * (i / 3));
		t.pStr = self->controlname[i].data();
		if (!t.pStr[0])
		{
			t.pStr = "---";
		}
		if ((widget_focus == _self) && (self->highlighted == i))
		{
			SetContextForeGroundColor(selected);
		}
		else
		{
			SetContextForeGroundColor(default_color);
		}
		font_DrawText(&t);
	}
	if (oldfont)
	{
		SetContextFont(oldfont);
	}
	SetContextForeGroundColor(oldtext);
}

int Widget_HeightChoice(Widget* _self)
{
	(void)_self;
	return uqm::ResScale(8); //effectively 1 column
}

int Widget_HeightFullScreen(Widget* _self)
{
	(void)_self;
	return CanvasHeight;
}

int Widget_HeightOneLine(Widget* _self)
{
	(void)_self;
	return uqm::ResScale(8);
}

int Widget_HeightLabel(Widget* _self)
{
	const WidgetLabel* self = (WidgetLabel*)_self;
	return self->line_count * uqm::ResScale(8);
}

int Widget_WidthFullScreen(Widget* _self)
{
	(void)_self;
	return CanvasWidth;
}

int Widget_ReceiveFocusSimple(Widget* _self, WidgetEvent event)
{
	widget_focus = _self;
	(void)event;
	return true;
}

int Widget_ReceiveFocusChoice(Widget* _self, WidgetEvent event)
{
	WidgetChoice* self = (WidgetChoice*)_self;
	widget_focus = _self;
	self->highlighted = self->selected;
	(void)event;
	return true;
}

int Widget_ReceiveFocusControlEntry(Widget* _self, WidgetEvent event)
{
	WidgetControlEntry* self = (WidgetControlEntry*)_self;
	int oldval = 0;
	if (widget_focus->tag == WidgetType::ControlEntry)
	{
		oldval = ((WidgetControlEntry*)widget_focus)->highlighted;
	}
	widget_focus = _self;
	self->highlighted = oldval;
	(void)event;
	return true;
}

int Widget_ReceiveFocusMenuScreen(Widget* _self, WidgetEvent event)
{
	WidgetMenuScreen* self = (WidgetMenuScreen*)_self;
	int x {};
	int last_x {};
	int dx {};
	for (x = 0; x < self->num_children; x++)
	{
		self->child[x]->parent = _self;
	}
	if (event == WidgetEvent::Up)
	{
		x = self->num_children - 1;
		dx = -1;
		last_x = -1;
	}
	else if (event == WidgetEvent::Down)
	{
		x = 0;
		dx = 1;
		last_x = self->num_children;
	}
	else
	{
		/* Leave highlighted value the same */
		Widget* child = self->child[self->highlighted];
		child->receiveFocus(child, event);
		return true;
	}
	for (; x != last_x; x += dx)
	{
		Widget* child = self->child[x];
		if ((*child->receiveFocus)(child, event))
		{
			self->highlighted = x;
			return true;
		}
	}
	return false;
}

int Widget_ReceiveFocusRefuseFocus(Widget* self, WidgetEvent event)
{
	(void)self;
	(void)event;
	return false;
}

int Widget_HandleEventIgnoreAll(Widget* self, WidgetEvent event)
{
	(void)event;
	(void)self;
	return false;
}

int Widget_HandleEventChoice(Widget* _self, WidgetEvent event)
{
	WidgetChoice* self = (WidgetChoice*)_self;
	switch (event)
	{
		case WidgetEvent::Left:
			self->highlighted -= 1;
			if (self->highlighted < 0)
			{
				self->highlighted = self->numopts - 1;
			}
			return true;
		case WidgetEvent::Right:
			self->highlighted += 1;
			if (self->highlighted >= self->numopts)
			{
				self->highlighted = 0;
			}
			return true;
		case WidgetEvent::Select:
			{
				const int oldval = self->selected;
				self->selected = self->highlighted;
				if (self->onChange)
				{
					(*(self->onChange))(self, oldval);
				}
				return true;
			}
		default:
			return false;
	}
}

int Widget_HandleEventSlider(Widget* _self, WidgetEvent event)
{
	WidgetSlider* self = (WidgetSlider*)_self;
	switch (event)
	{
		case WidgetEvent::Left:
			self->value -= self->step;
			if (self->value < self->min)
			{
				self->value = self->min;
			}
			if (self->onChange)
			{
				(*(self->onChange))(self);
			}
			return true;
		case WidgetEvent::Right:
			self->value += self->step;
			if (self->value > self->max)
			{
				self->value = self->max;
			}
			if (self->onChange)
			{
				(*(self->onChange))(self);
			}
			return true;
		default:
			return false;
	}
}

int Widget_HandleEventMenuScreen(Widget* _self, WidgetEvent event)
{
	WidgetMenuScreen* self = (WidgetMenuScreen*)_self;
	int x {};
	int last_x {};
	int dx {};
	switch (event)
	{
		case WidgetEvent::Up:
			dx = -1;
			break;
		case WidgetEvent::Down:
			dx = 1;
			break;
		case WidgetEvent::Cancel:
			/* On cancel, shift focus to last Element and send a SELECT. */
			self->highlighted = self->num_children - 1;
			widget_focus = self->child[self->highlighted];
			return (widget_focus->handleEvent)(widget_focus, WidgetEvent::Select);
		default:
			return false;
	}
	last_x = self->highlighted;
	x = self->highlighted + dx;
	while (x != last_x)
	{
		Widget* child {};
		if (x == -1)
		{
			x = self->num_children - 1;
		}
		if (x == self->num_children)
		{
			x = 0;
		}
		child = self->child[x];
		if ((*child->receiveFocus)(child, event))
		{
			self->highlighted = x;
			return true;
		}
		x += dx;
	}
	return false;
}

int Widget_HandleEventTextEntry(Widget* _self, WidgetEvent event)
{
	WidgetTextEntry* self = (WidgetTextEntry*)_self;
	if (event == WidgetEvent::Select)
	{
		if (!self->handleEventSelect)
		{
			return false;
		}
		return (*self->handleEventSelect)(self);
	}
	return false;
}

int Widget_HandleEventControlEntry(Widget* _self, WidgetEvent event)
{
	WidgetControlEntry* self = (WidgetControlEntry*)_self;
	if (event == WidgetEvent::Select)
	{
		if (self->onChange)
		{
			(self->onChange)(self);
			return true;
		}
	}
	if (event == WidgetEvent::Delete)
	{
		if (self->onDelete)
		{
			(self->onDelete)(self);
			return true;
		}
	}
	if ((event == WidgetEvent::Right) || (event == WidgetEvent::Left))
	{
		self->highlighted = 1 - self->highlighted;
		return true;
	}
	return false;
}

int Widget_Event(WidgetEvent event)
{
	Widget* widget = widget_focus;
	while (widget != nullptr)
	{
		if ((*widget->handleEvent)(widget, event))
		{
			return true;
		}
		widget = widget->parent;
	}
	return false;
}
