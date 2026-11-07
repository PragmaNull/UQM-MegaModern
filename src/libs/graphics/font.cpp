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

#include <math.h>

#include "options.h"
#include "gfxintrn.h"
#include "tfb_prim.h"
#include "core/log/log.h"
#include "uqm/units.h"
#include "uqm/sounds.h"
#include "uqm/controls.h"

#include "uqm/setup.h"

static inline TFB_Char* getCharFrame(FontDesc* fontPtr, uqm::UniChar ch);

TFB_Char* GetFrameForFPS(uqm::UniChar ch)
{
	if (StarConFont && !(uqm::currentActivity() & CHECK_ABORT))
	{
		return getCharFrame(StarConFont, ch);
	}
	else
	{
		return nullptr;
	}
}

bool GoodToGoFPS(void)
{
	return (bool)(StarConFont && !(uqm::currentActivity() & CHECK_ABORT)
				  && !optRequiresReload);
}

void GetFontDims(int16_t* w, int16_t* h)
{
	*w = (int16_t)StarConFont->disp.width;
	*h = (int16_t)StarConFont->disp.height;
}

int GetFontCharSpace(void)
{
	return StarConFont->CharSpace;
}

FONT SetContextFont(FONT Font)
{
	FONT LastFont {_CurFontPtr};
	_CurFontPtr = Font;
	if (ContextActive())
	{
		SwitchContextFont(Font);
	}

	return (LastFont);
}

bool DestroyFont(FONT FontRef)
{
	if (FontRef == nullptr)
	{
		return (false);
	}

	if (_CurFontPtr && _CurFontPtr == FontRef)
	{
		SetContextFont((FONT) nullptr);
	}

	return (_ReleaseFontData(FontRef));
}

// Returns the GfxRect of any given GfxText
GfxRect font_GetTextRect(GfxText* lpText)
{
	GfxRect ClipRect;
	GfxPoint origin;
	GfxText text;

	memset(&ClipRect, 0, sizeof(GfxRect));

	FixContextFontEffect();
	if (!GraphicsSystemActive() || !GetContextValidRect(nullptr, &origin))
	{
		return ClipRect;
	}

	// TextRect() clobbers GfxText.CharCount so we have to make a copy
	text = *lpText;
	if (!TextRect(&text, &ClipRect, nullptr))
	{
		return ClipRect;
	}

	return ClipRect;
}

// XXX: Should be in frame.c (renamed to something decent?)
void font_DrawText(GfxText* lpText)
{
	GfxRect ClipRect;
	GfxPoint origin;
	GfxText text;

	FixContextFontEffect();
	if (!GraphicsSystemActive() || !GetContextValidRect(nullptr, &origin))
	{
		return;
	}

	// TextRect() clobbers GfxText.CharCount so we have to make a copy
	text = *lpText;
	if (!TextRect(&text, &ClipRect, nullptr))
	{
		return;
	}
	// ClipRect is relative to origin
	_text_blt(&ClipRect, &text, origin);
}

void font_DrawText_Fade(GfxText* lpText, FRAME repair, bool* skip)
{
	GfxRect ClipRect;
	GfxPoint origin;
	GfxText text;

	if (!GraphicsSystemActive() || !GetContextValidRect(nullptr, &origin))
	{
		return;
	}

	// TextRect() clobbers GfxText.CharCount so we have to make a copy
	text = *lpText;
	if (!TextRect(&text, &ClipRect, nullptr))
	{
		return;
	}
	// ClipRect is relative to origin
	_text_blt_fade(&ClipRect, &text, origin, repair, skip);
}

/* Draw the stroke by drawing the same text in the
 * background color one pixel shifted to all 4 directions.
 */
void font_DrawTracedText(GfxText* pText, Color text, Color trace)
{
	// Preserve current foreground color for full correctness
	const Color oldfg = SetContextForeGroundColor(trace);
	const uint8_t stroke = ResScale(1);
	const GfxPoint t_baseline = pText->baseline;
	GfxPoint offset;

	for (offset.x = -stroke; offset.x <= stroke; ++offset.x)
	{
		for (offset.y = -stroke; offset.y <= stroke; ++offset.y)
		{
			if (hypot(offset.x, offset.y) > stroke)
			{
				continue;
			}
			pText->baseline =
				MAKE_POINT(
					t_baseline.x + offset.x,
					t_baseline.y + offset.y);
			font_DrawText(pText);
		}
	}
	pText->baseline = t_baseline;

	SetContextForeGroundColor(text);
	font_DrawText(pText);
	SetContextForeGroundColor(oldfg);
}

// Alt stuff to handle 2 fonts at once (for Orz)
uint8_t font_DrawTextAlt(GfxText* lpText, uint8_t swap, FONT AltFontPtr, uqm::UniChar key)
{
	GfxRect ClipRect;
	GfxPoint origin;
	GfxText text;

	FixContextFontEffect();
	if (!GraphicsSystemActive() || !GetContextValidRect(nullptr, &origin))
	{
		return 0;
	}

	// TextRect() clobbers GfxText.CharCount so we have to make a copy
	text = *lpText;
	if (!TextRectAlt(&text, &ClipRect, nullptr, swap, key, AltFontPtr))
	{
		return 0;
	}
	// ClipRect is relative to origin
	return _text_blt_alt(&ClipRect, &text, origin, swap, AltFontPtr, key);
}

void font_DrawTracedTextAlt(GfxText* pText, Color text, Color trace, FONT AltFontPtr,
							uqm::UniChar key)
{
	// Preserve current foreground color for full correctness
	const Color oldfg = SetContextForeGroundColor(trace);
	const uint8_t stroke = ResScale(1);
	const GfxPoint t_baseline = pText->baseline;
	GfxPoint offset;
	static uint8_t swap = 0;

	for (offset.x = -stroke; offset.x <= stroke; ++offset.x)
	{
		for (offset.y = -stroke; offset.y <= stroke; ++offset.y)
		{
			if (hypot(offset.x, offset.y) > stroke)
			{
				continue;
			}
			pText->baseline =
				MAKE_POINT(
					t_baseline.x + offset.x,
					t_baseline.y + offset.y);
			font_DrawTextAlt(pText, swap, AltFontPtr, key);
		}
	}
	pText->baseline = t_baseline;

	SetContextForeGroundColor(text);
	swap = font_DrawTextAlt(pText, swap, AltFontPtr, key);
	SetContextForeGroundColor(oldfg);
}

void font_DrawShadowedText(GfxText* pText, ShadowAngle direction,
						   Color text_color, Color shadow_color)
{
	GfxPoint shadow_angle = {0, 0};
	Color OldColor;

	switch (direction)
	{
		case ShadowAngle::North:
			shadow_angle = MAKE_POINT(0, -1);
			break;
		case ShadowAngle::NorthEast:
			shadow_angle = MAKE_POINT(1, -1);
			break;
		case ShadowAngle::East:
			shadow_angle = MAKE_POINT(1, 0);
			break;
		case ShadowAngle::SouthEast:
			shadow_angle = MAKE_POINT(1, 1);
			break;
		case ShadowAngle::South:
			shadow_angle = MAKE_POINT(0, 1);
			break;
		case ShadowAngle::SouthWest:
			shadow_angle = MAKE_POINT(-1, 1);
			break;
		case ShadowAngle::West:
			shadow_angle = MAKE_POINT(-1, 0);
			break;
		case ShadowAngle::NorthWest:
			shadow_angle = MAKE_POINT(-1, -1);
			break;
	}

	pText->baseline.x += ResScale(shadow_angle.x);
	pText->baseline.y += ResScale(shadow_angle.y);

	OldColor = SetContextForeGroundColor(shadow_color);

	font_DrawText(pText);

	pText->baseline.x -= ResScale(shadow_angle.x);
	pText->baseline.y -= ResScale(shadow_angle.y);

	SetContextForeGroundColor(text_color);

	font_DrawText(pText);

	SetContextForeGroundColor(OldColor);
}

bool GetContextFontLeading(int16_t* pheight)
{
	if (_CurFontPtr != 0)
	{
		*pheight = (int16_t)_CurFontPtr->Leading;
		return (true);
	}

	*pheight = 0;
	return (false);
}

bool GetContextFontDispHeight(int16_t* pheight)
{
	if (_CurFontPtr != 0)
	{
		*pheight = (int16_t)_CurFontPtr->disp.height;
		return (true);
	}

	*pheight = 0;
	return (false);
}

bool GetContextFontDispWidth(int16_t* pwidth)
{
	if (_CurFontPtr != 0)
	{
		*pwidth = (int16_t)_CurFontPtr->disp.width;
		return (true);
	}

	*pwidth = 0;
	return (false);
}

bool TextRect(GfxText* lpText, GfxRect* pRect, uint8_t* pdelta)
{
	uint8_t char_delta_array[MaxDeltas];

	FONT FontPtr {_CurFontPtr};
	if (FontPtr != 0 && lpText->CharCount != 0)
	{
		int16_t top_y {};
		int16_t bot_y {};
		int16_t width {};
		uqm::UniChar next_ch = 0;
		const char* pStr {};

		uint16_t num_chars {lpText->CharCount};
		/* At this point lpText->CharCount contains the *maximum* number of
		 * characters that lpText->pStr may contain.
		 * After the while loop below, it will contain the actual number.
		 */
		if (pdelta == 0)
		{
			pdelta = char_delta_array;
			if (num_chars > MaxDeltas)
			{
				num_chars = MaxDeltas;
				lpText->CharCount = MaxDeltas;
			}
		}

		top_y = 0;
		bot_y = 0;
		width = 0;
		pStr = lpText->pStr;
		if (num_chars > 0)
		{
			next_ch = uqm::getCharFromString(&pStr);
			if (next_ch == '\0')
			{
				num_chars = 0;
			}
		}
		while (num_chars--)
		{
			const int16_t last_width {width};

			const uqm::UniChar ch {next_ch};
			if (num_chars > 0)
			{
				next_ch = uqm::getCharFromString(&pStr);
				if (next_ch == '\0')
				{
					lpText->CharCount -= num_chars;
					num_chars = 0;
				}
			}

			const TFB_Char* charFrame {getCharFrame(FontPtr, ch)};
			if (charFrame != nullptr && charFrame->disp.width)
			{
				int16_t y {};

				y = -charFrame->HotSpot.y;
				if (y < top_y)
				{
					top_y = y;
				}
				y += charFrame->disp.height;
				if (y > bot_y)
				{
					bot_y = y;
				}

				width += charFrame->disp.width + FontPtr->CharSpace;

				if (num_chars && next_ch < MaxUnicode
					&& FontPtr->KernTab[ch] != (uint8_t)~0
					&& !(FontPtr->KernTab[ch]
						 & (FontPtr->KernTab[next_ch] >> 2)))
				{
					width -= FontPtr->KernAmount;

					//printf ("%c: %d -- %d :%c\n", ch, FontPtr->KernTab[ch],
					//		FontPtr->KernTab[next_ch] >> 2, next_ch);
				}
			}

			*pdelta++ = (uint8_t)(width - last_width);
		}

		if (width > 0 && (bot_y -= top_y) > 0)
		{
			/* subtract off character spacing */
			if (pdelta[-1] > 0)
			{
				--pdelta[-1];
				width -= FontPtr->CharSpace;
			}

			if (lpText->align == TextAlign::Left)
			{
				pRect->corner.x = 0;
			}
			else if (lpText->align == TextAlign::Center)
			{
				pRect->corner.x = -ResScale((ResDescale(width) >> 1));
			}
			else
			{
				pRect->corner.x = -width;
			}
			pRect->corner.y = top_y;
			pRect->extent.width = width;
			pRect->extent.height = bot_y;

			pRect->corner.x += lpText->baseline.x;
			pRect->corner.y += lpText->baseline.y;

			return (true);
		}
	}

	pRect->corner = lpText->baseline;
	pRect->extent.width = 0;
	pRect->extent.height = 0;

	return (false);
}

void _text_blt(GfxRect* pClipRect, GfxText* TextPtr, GfxPoint ctxOrigin)
{
	const char* pStr {};
	GfxPoint origin;
	const DrawMode mode = _get_context_draw_mode();

	FONT FontPtr {_CurFontPtr};
	if (FontPtr == nullptr)
	{
		return;
	}
	TFB_Image* backing {_get_context_font_backing()};
	if (!backing)
	{
		return;
	}

	origin.x = pClipRect->corner.x;
	origin.y = TextPtr->baseline.y;
	uint16_t num_chars {TextPtr->CharCount};
	if (num_chars == 0)
	{
		return;
	}

	pStr = TextPtr->pStr;

	uqm::UniChar next_ch {uqm::getCharFromString(&pStr)};
	if (next_ch == '\0')
	{
		num_chars = 0;
	}
	while (num_chars--)
	{
		const uqm::UniChar ch {next_ch};
		if (num_chars > 0)
		{
			next_ch = uqm::getCharFromString(&pStr);
			if (next_ch == '\0')
			{
				num_chars = 0;
			}
		}

		TFB_Char* fontChar {getCharFrame(FontPtr, ch)};
		if (fontChar != nullptr && fontChar->disp.width)
		{
			GfxRect r;

			r.corner.x = origin.x - fontChar->HotSpot.x;
			r.corner.y = origin.y - fontChar->HotSpot.y;
			r.extent.width = fontChar->disp.width;
			r.extent.height = fontChar->disp.height;
			if (BoxIntersect(&r, pClipRect, &r))
			{
				TFB_Prim_FontChar(origin, fontChar, backing, mode,
								  ctxOrigin);
			}

			origin.x += fontChar->disp.width + FontPtr->CharSpace;

			if (num_chars && next_ch < MaxUnicode
				&& FontPtr->KernTab[ch] != (uint8_t)~0
				&& !(FontPtr->KernTab[ch]
					 & (FontPtr->KernTab[next_ch] >> 2)))
			{
				origin.x -= FontPtr->KernAmount;

				//printf ("%c: %d -- %d :%c\n", ch, FontPtr->KernTab[ch],
				//		FontPtr->KernTab[next_ch] >> 2, next_ch);
			}
		}
	}
}

void _text_blt_fade(GfxRect* pClipRect, GfxText* TextPtr, GfxPoint ctxOrigin, FRAME repair, bool* skip)
{
	const char* pStr {};
	GfxPoint origin;
	int16_t leading {};
	TFB_Image* b_first {};
	TFB_Image* b_second {};
	TFB_Image* b_clear {};
	const DrawMode mode = _get_context_draw_mode();

	FONT FontPtr {_CurFontPtr};
	if (FontPtr != nullptr)
	{
		GfxRect r;
		int16_t w {};
		int16_t h {};

		if (!GetContextFontDispHeight(&h) || !GetContextFontDispWidth(&w))
		{
			return;
		}

		b_first = TFB_DrawImage_CreateForScreen(w, h, true);
		b_second = TFB_DrawImage_CreateForScreen(w, h, true);
		b_clear = TFB_DrawImage_CreateForScreen(w, h, true);

		r.corner.x = r.corner.y = 0;
		r.extent.width = w;
		r.extent.height = h;

		TFB_DrawImage_Rect(&r, _get_context_bg_color(), DrawReplaceMode, b_first);
		TFB_DrawImage_Rect(&r, _get_context_fg_color(), DrawReplaceMode, b_second);
	}
	else
	{
		return;
	}

	origin.x = pClipRect->corner.x;
	origin.y = TextPtr->baseline.y;
	GetContextFontLeading(&leading);
	uint16_t num_chars {TextPtr->CharCount};
	if (num_chars == 0)
	{
		return;
	}

	pStr = TextPtr->pStr;

	uqm::UniChar next_ch {uqm::getCharFromString(&pStr)};
	if (next_ch == '\0')
	{
		num_chars = 0;
	}
	while (num_chars--)
	{
		TFB_Char* fontChar {};

		while (next_ch == ' ')
		{
			fontChar = getCharFrame(FontPtr, next_ch);
			origin.x += fontChar->disp.width + FontPtr->CharSpace;
			next_ch = uqm::getCharFromString(&pStr);
			num_chars--;
		}
		const uqm::UniChar ch {next_ch};
		if (num_chars > 0)
		{
			next_ch = uqm::getCharFromString(&pStr);
			if (next_ch == '\0')
			{
				num_chars = 0;
			}
		}

		fontChar = getCharFrame(FontPtr, ch);
		if (fontChar != nullptr && fontChar->disp.width)
		{
			GfxRect r;

			r.corner.x = origin.x - fontChar->HotSpot.x;
			r.corner.y = origin.y - fontChar->HotSpot.y;
			r.extent.width = fontChar->disp.width;
			r.extent.height = fontChar->disp.height;
			if (BoxIntersect(&r, pClipRect, &r))
			{
				if (!*skip)
				{
					TFB_Prim_FontChar(origin, fontChar, b_first, mode,
									  ctxOrigin);
					PlayMenuSound(MenuSoundEffect::Text);

					SleepThread(getTicksForFramerate(16));
				}
				BatchGraphics();
				if (repair && !*skip)
				{
					TFB_DrawImage_Image(repair->image, -r.corner.x, -r.corner.y,
										0, uqm::TFBScaleMode::None, nullptr, DrawReplaceMode, b_clear);
					TFB_Prim_FontChar(origin, fontChar, b_clear, MAKE_DRAW_MODE(DrawKind::Grayscale, 0xff),
									  ctxOrigin);
				}
				TFB_Prim_FontChar(origin, fontChar, b_second, mode,
								  ctxOrigin);
				UnbatchGraphics();
			}

			if (next_ch == '\n' || next_ch == '\r')
			{
				origin.x = pClipRect->corner.x;
				origin.y += leading;
				pClipRect->extent.height += leading;
				next_ch = uqm::getCharFromString(&pStr);
				num_chars--;
			}
			else if (next_ch != '\0')
			{
				origin.x += fontChar->disp.width + FontPtr->CharSpace;

				if (num_chars && next_ch < MaxUnicode
					&& FontPtr->KernTab[ch] != (uint8_t)~0
					&& !(FontPtr->KernTab[ch]
						 & (FontPtr->KernTab[next_ch] >> 2)))
				{
					origin.x -= FontPtr->KernAmount;

					//printf ("%c: %d -- %d :%c\n", ch, FontPtr->KernTab[ch],
					//		FontPtr->KernTab[next_ch] >> 2, next_ch);
				}
			}
		}
		UpdateInputState();
		if (CurrentInputState.menu[*MenuKey::MenuCancel] || (uqm::currentActivity() & CHECK_ABORT))
		{
			*skip = true;
		}
	}
	if (b_first)
	{
		TFB_DrawScreen_DeleteImage(b_first);
	}
	if (b_second)
	{
		TFB_DrawScreen_DeleteImage(b_second);
	}
	if (b_clear)
	{
		TFB_DrawScreen_DeleteImage(b_clear);
	}
}

bool TextRectAlt(GfxText* lpText, GfxRect* pRect, uint8_t* pdelta, uint8_t swap,
				 uqm::UniChar key, FONT AltFontPtr)
{
	uint8_t char_delta_array[MaxDeltas];

	FONT FontPtr {_CurFontPtr};
	if (FontPtr != 0 && lpText->CharCount != 0)
	{
		int16_t top_y {};
		int16_t bot_y {};
		int16_t width {};
		uqm::UniChar next_ch = 0;
		const char* pStr {};

		uint16_t num_chars {lpText->CharCount};
		/* At this point lpText->CharCount contains the *maximum* number of
		 * characters that lpText->pStr may contain.
		 * After the while loop below, it will contain the actual number.
		 */
		if (pdelta == 0)
		{
			pdelta = char_delta_array;
			if (num_chars > MaxDeltas)
			{
				num_chars = MaxDeltas;
				lpText->CharCount = MaxDeltas;
			}
		}

		top_y = 0;
		bot_y = 0;
		width = 0;
		pStr = lpText->pStr;
		if (num_chars > 0)
		{
			next_ch = uqm::getCharFromString(&pStr);
			if (next_ch == '\0')
			{
				num_chars = 0;
			}
		}
		while (num_chars--)
		{
			const TFB_Char* charFrame {};

			const int16_t last_width {width};

			const uqm::UniChar ch {next_ch};
			if (num_chars > 0)
			{
				next_ch = uqm::getCharFromString(&pStr);
				if (next_ch == '\0')
				{
					lpText->CharCount -= num_chars;
					num_chars = 0;
				}
			}

			if (ch == key)
			{
				charFrame = getCharFrame(AltFontPtr, ch);
				swap ^= 1; // switch current font
				FontPtr = swap ? AltFontPtr : _CurFontPtr;
			}
			else
			{
				FontPtr = swap ? AltFontPtr : _CurFontPtr;
				charFrame = getCharFrame(FontPtr, ch);
			}

			if (charFrame != nullptr && charFrame->disp.width)
			{
				int16_t y {};

				y = -charFrame->HotSpot.y;
				if (y < top_y)
				{
					top_y = y;
				}
				y += charFrame->disp.height;
				if (y > bot_y)
				{
					bot_y = y;
				}

				width += charFrame->disp.width + FontPtr->CharSpace;

				if (num_chars && next_ch < MaxUnicode
					&& FontPtr->KernTab[ch] != (uint8_t)~0
					&& !(FontPtr->KernTab[ch]
						 & (FontPtr->KernTab[next_ch] >> 2)))
				{
					width -= FontPtr->KernAmount;

					//printf ("%c: %d -- %d :%c\n", ch, FontPtr->KernTab[ch],
					//		FontPtr->KernTab[next_ch] >> 2, next_ch);
				}
			}

			*pdelta++ = (uint8_t)(width - last_width);
		}

		if (width > 0 && (bot_y -= top_y) > 0)
		{
			/* subtract off character spacing */
			if (pdelta[-1] > 0)
			{
				--pdelta[-1];
				width -= FontPtr->CharSpace;
			}

			if (lpText->align == TextAlign::Left)
			{
				pRect->corner.x = 0;
			}
			else if (lpText->align == TextAlign::Center)
			{
				pRect->corner.x = -ResScale((ResDescale(width) >> 1));
			}
			else
			{
				pRect->corner.x = -width;
			}
			pRect->corner.y = top_y;
			pRect->extent.width = width;
			pRect->extent.height = bot_y;

			pRect->corner.x += lpText->baseline.x;
			pRect->corner.y += lpText->baseline.y;

			return (true);
		}
	}

	pRect->corner = lpText->baseline;
	pRect->extent.width = 0;
	pRect->extent.height = 0;

	return (false);
}

uint8_t _text_blt_alt(GfxRect* pClipRect, GfxText* TextPtr, GfxPoint ctxOrigin, uint8_t swap,
					  FONT AltFontPtr, uqm::UniChar key)
{ // Kruzen: To create text using 2 fonts (Orz case)
	// Safest way to do so without going too deep into
	// original code
	// Warning: GetTextRect doesn't quite work since it looks
	// for chars only in 1 font
	const char* pStr {};
	GfxPoint origin;
	TFB_Image* ext {};
	const DrawMode mode = _get_context_draw_mode();
	uint8_t leading_step {};

	FONT FontPtr {_CurFontPtr};
	if (FontPtr == nullptr)
	{
		return 0;
	}
	TFB_Image* stock {_get_context_font_backing()};
	if (!stock)
	{
		return 0;
	}

	if (AltFontPtr != nullptr)
	{ // Local backing needed for alt font
		// Create one
		GfxRect r;
		const Color color = _get_context_fg_color();
		const int16_t w {(int16_t)AltFontPtr->disp.width};
		const int16_t h {(int16_t)AltFontPtr->disp.height};
		if (w == 0 || h == 0)
		{
			return 0;
		}

		ext = TFB_DrawImage_CreateForScreen(w, h, true);

		r.corner = MAKE_HOT_SPOT(0, 0);
		r.extent = MAKE_EXTENT(w, h);

		TFB_DrawImage_Rect(&r, color, DrawReplaceMode, ext);
	}
	else
	{
		return 0;
	}

	origin.x = pClipRect->corner.x;
	origin.y = TextPtr->baseline.y;
	uint16_t num_chars {TextPtr->CharCount};
	if (num_chars == 0)
	{
		return 0;
	}

	leading_step = AltFontPtr->Leading - _CurFontPtr->Leading;
	pStr = TextPtr->pStr;

	uqm::UniChar next_ch {uqm::getCharFromString(&pStr)};
	if (next_ch == '\0')
	{
		num_chars = 0;
	}
	TFB_Image* backing {stock};
	while (num_chars--)
	{
		TFB_Char* fontChar {};

		const uqm::UniChar ch {next_ch};
		if (num_chars > 0)
		{
			next_ch = uqm::getCharFromString(&pStr);
			if (next_ch == '\0')
			{
				num_chars = 0;
			}
		}

		if (ch == key)
		{
			fontChar = getCharFrame(AltFontPtr, ch);
			backing = ext;
			swap ^= 1; // switch current font
			FontPtr = swap ? AltFontPtr : _CurFontPtr;
			origin.y = TextPtr->baseline.y + leading_step;
		}
		else
		{
			FontPtr = swap ? AltFontPtr : _CurFontPtr;
			fontChar = getCharFrame(FontPtr, ch);
			backing = swap ? ext : stock;
			origin.y = TextPtr->baseline.y + (swap ? leading_step : 0);
		}

		if (fontChar != nullptr && fontChar->disp.width)
		{
			GfxRect r;

			r.corner.x = origin.x - fontChar->HotSpot.x;
			r.corner.y = origin.y - fontChar->HotSpot.y;
			r.extent.width = fontChar->disp.width;
			r.extent.height = fontChar->disp.height;
			if (BoxIntersect(&r, pClipRect, &r))
			{
				TFB_Prim_FontChar(origin, fontChar, backing, mode,
								  ctxOrigin);
			}

			origin.x += fontChar->disp.width + FontPtr->CharSpace;

			if (num_chars && next_ch < MaxUnicode
				&& FontPtr->KernTab[ch] != (uint8_t)~0
				&& !(FontPtr->KernTab[ch]
					 & (FontPtr->KernTab[next_ch] >> 2)))
			{
				origin.x -= FontPtr->KernAmount;

				//printf ("%c: {} -- {} :%c\n", ch, FontPtr->KernTab[ch],
				//		FontPtr->KernTab[next_ch] >> 2, next_ch);
			}
		}
	}

	if (ext)
	{
		TFB_DrawImage_Delete(ext);
	}

	return swap;
}

static inline TFB_Char* getCharFrame(FontDesc* fontPtr, uqm::UniChar ch)
{
	const uqm::UniChar pageStart = ch & CharacterPageMask;

	FontPage* page = fontPtr->fontPages;
	for (;;)
	{
		if (page == nullptr)
		{
			return nullptr;
		}

		if (page->pageStart == pageStart)
		{
			break;
		}

		page = page->next;
	}

	const size_t charIndex {ch - page->firstChar};
	if (ch >= page->firstChar && charIndex < page->numChars
		&& page->charDesc[charIndex].data)
	{
		return &page->charDesc[charIndex];
	}
	else
	{
		//log_add (log_Debug, "Character {} not present", (unsigned int) ch);
		return nullptr;
	}
}
