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

#include "gfxintrn.h"

GRAPHICS_STATUS _GraphicsStatusFlags;
GFXCONTEXT _pCurContext;

#ifdef DEBUG
// We keep track of all contexts
GFXCONTEXT firstContext;
// The first one in the list.
GFXCONTEXT* contextEnd = &firstContext;
// Where to put the next context.
#endif

PRIMITIVE _locPrim;

FONT _CurFontPtr;

#define DEFAULT_FORE_COLOR BUILD_COLOR(MAKE_RGB15(0x1F, 0x1F, 0x1F), 0x0F)
#define DEFAULT_BACK_COLOR BUILD_COLOR(MAKE_RGB15(0x00, 0x00, 0x00), 0x00)

#define DEFAULT_DRAW_MODE MAKE_DRAW_MODE(DRAW_DEFAULT, 255)

GFXCONTEXT
SetContext(GFXCONTEXT Context)
{
	GFXCONTEXT LastContext;

	LastContext = _pCurContext;
	if (Context != LastContext)
	{
		if (LastContext)
		{
			UnsetContextFlags(
				uqm::makeWord(0, GRAPHICS_ACTIVE | DRAWABLE_ACTIVE));
			SetContextFlags(
				uqm::makeWord(0, _GraphicsStatusFlags
									 & (GRAPHICS_ACTIVE | DRAWABLE_ACTIVE)));

			DeactivateContext();
		}

		_pCurContext = Context;
		if (_pCurContext)
		{
			ActivateContext();

			_GraphicsStatusFlags &= ~(GRAPHICS_ACTIVE | DRAWABLE_ACTIVE);
			_GraphicsStatusFlags |= uqm::highByte(_get_context_flags());

			SetPrimColor(&_locPrim, _get_context_fg_color());

			_CurFramePtr = _get_context_fg_frame();
			_CurFontPtr = _get_context_font();
		}
	}

	return (LastContext);
}

#ifdef DEBUG
GFXCONTEXT
CreateContextAux(const char* name)
#else  /* if !defined(DEBUG) */
GFXCONTEXT
CreateContextAux(void)
#endif /* !defined(DEBUG) */
{
	GFXCONTEXT NewContext;

	NewContext = (GFXCONTEXT)AllocContext();
	if (NewContext)
	{
		/* initialize context */
#ifdef DEBUG
		NewContext->name = name;
		NewContext->next = nullptr;
		*contextEnd = NewContext;
		contextEnd = &NewContext->next;
#endif /* DEBUG */

		NewContext->Mode = DEFAULT_DRAW_MODE;
		NewContext->ForeGroundColor = DEFAULT_FORE_COLOR;
		NewContext->BackGroundColor = DEFAULT_BACK_COLOR;
	}

	return NewContext;
}

#ifdef DEBUG
// Loop through the list of context to the pointer which points to the
// specified context. This is either 'firstContext' or the address of
// the 'next' field of some other context.
static GFXCONTEXT*
FindContextPtr(GFXCONTEXT context)
{
	GFXCONTEXT* ptr;

	for (ptr = &firstContext; *ptr != nullptr; ptr = &(*ptr)->next)
	{
		if (*ptr == context)
		{
			break;
		}
	}
	return ptr;
}
#endif /* DEBUG */

bool DestroyContext(GFXCONTEXT ContextRef)
{
	TFB_Image* img;

	if (ContextRef == 0)
	{
		return (false);
	}

	if (_pCurContext && _pCurContext == ContextRef)
	{
		SetContext((GFXCONTEXT)0);
	}

#ifdef DEBUG
	// Unlink the context.
	{
		GFXCONTEXT* contextPtr = FindContextPtr(ContextRef);
		if (contextEnd == &ContextRef->next)
		{
			contextEnd = contextPtr;
		}
		*contextPtr = ContextRef->next;
	}
#endif /* DEBUG */

	img = ContextRef->FontBacking;
	if (img)
	{
		TFB_DrawImage_Delete(img);
	}

	FreeContext(ContextRef);
	return true;
}

Color SetContextForeGroundColor(Color color)
{
	Color oldColor;

	if (!ContextActive())
	{
		return DEFAULT_FORE_COLOR;
	}

	oldColor = _get_context_fg_color();
	if (!sameColor(oldColor, color))
	{
		SwitchContextForeGroundColor(color);

		if (!(_get_context_fbk_flags() & FBK_IMAGE))
		{
			SetContextFBkFlags(FBK_DIRTY);
		}
	}
	SetPrimColor(&_locPrim, color);

	return (oldColor);
}

Color GetContextForeGroundColor(void)
{
	if (!ContextActive())
	{
		return DEFAULT_FORE_COLOR;
	}

	return _get_context_fg_color();
}

Color SetContextBackGroundColor(Color color)
{
	Color oldColor;

	if (!ContextActive())
	{
		return DEFAULT_BACK_COLOR;
	}

	oldColor = _get_context_bg_color();
	if (!sameColor(oldColor, color))
	{
		SwitchContextBackGroundColor(color);
	}

	return oldColor;
}

Color GetContextBackGroundColor(void)
{
	if (!ContextActive())
	{
		return DEFAULT_BACK_COLOR;
	}

	return _get_context_bg_color();
}

DrawMode
SetContextDrawMode(DrawMode mode)
{
	DrawMode oldMode;

	if (!ContextActive())
	{
		return DEFAULT_DRAW_MODE;
	}

	oldMode = _get_context_draw_mode();
	SwitchContextDrawMode(mode);

	return oldMode;
}

DrawMode
GetContextDrawMode(void)
{
	if (!ContextActive())
	{
		return DEFAULT_DRAW_MODE;
	}

	return _get_context_draw_mode();
}

// Returns a rect based at 0,0 and the size of context foreground frame
static inline GFXRECT
_get_context_fg_rect(void)
{
	GFXRECT r = {
		{0, 0},
		{0, 0}
	  };
	if (_CurFramePtr)
	{
		r.extent = GetFrameBounds(_CurFramePtr);
	}
	return r;
}

bool SetContextClipRect(GFXRECT* lpRect)
{
	if (!ContextActive())
	{
		return (false);
	}

	if (lpRect)
	{
		if (rectsEqual(*lpRect, _get_context_fg_rect()))
		{ // Cliprect is undefined to mirror GetContextClipRect()
			_pCurContext->ClipRect.extent.width = 0;
		}
		else
		{ // We have a cliprect
			_pCurContext->ClipRect = *lpRect;
		}
	}
	else
	{ // Set cliprect as undefined
		_pCurContext->ClipRect.extent.width = 0;
	}

	return true;
}

bool GetContextClipRect(GFXRECT* lpRect)
{
	if (!ContextActive())
	{
		return (false);
	}

	*lpRect = _pCurContext->ClipRect;
	if (!_pCurContext->ClipRect.extent.width)
	{ // Though the cliprect is undefined, drawing will be clipped
		// to the extent of the foreground frame
		*lpRect = _get_context_fg_rect();
	}

	return (bool)(_pCurContext->ClipRect.extent.width != 0);
}

GFXPOINT
SetContextOrigin(GFXPOINT orgOffset)
{
	// XXX: This is a hack, kind of. But that's what the original did.
	return SetFrameHot(_CurFramePtr, orgOffset);
}

FRAME
SetContextFontEffect(FRAME EffectFrame)
{
	FRAME LastEffect;

	if (!ContextActive())
	{
		return (nullptr);
	}

	LastEffect = _get_context_fonteff();
	if (EffectFrame != LastEffect)
	{
		SwitchContextFontEffect(EffectFrame);

		if (EffectFrame != 0)
		{
			SetContextFBkFlags(FBK_IMAGE);
		}
		else
		{
			UnsetContextFBkFlags(FBK_IMAGE);
		}
	}

	return LastEffect;
}

void FixContextFontEffect(void)
{
	int16_t w, h;
	TFB_Image* img;

	if (!ContextActive() || (_get_context_font_backing() != 0 && !(_get_context_fbk_flags() & FBK_DIRTY)))
	{
		return;
	}

	if (!GetContextFontDispHeight(&h) || !GetContextFontDispWidth(&w))
	{
		return;
	}

	img = _pCurContext->FontBacking;
	if (img)
	{
		TFB_DrawScreen_DeleteImage(img);
	}

	img = TFB_DrawImage_CreateForScreen(w, h, true);
	if (_get_context_fbk_flags() & FBK_IMAGE)
	{ // image pattern backing
		FRAME EffectFrame = _get_context_fonteff();

		TFB_DrawImage_Image(EffectFrame->image,
							-EffectFrame->HotSpot.x, -EffectFrame->HotSpot.y,
							0, uqm::TFBScaleMode::None, nullptr, DRAW_REPLACE_MODE, img);
	}
	else
	{ // solid color backing
		GFXRECT r = {
			{0, 0},
			{w, h}
		  };
		Color color = _get_context_fg_color();

		TFB_DrawImage_Rect(&r, color, DRAW_REPLACE_MODE, img);
	}

	_pCurContext->FontBacking = img;
	UnsetContextFBkFlags(FBK_DIRTY);
}

// 'area' may be nullptr to copy the entire GFXCONTEXT cliprect
// 'area' is relative to the GFXCONTEXT cliprect
DRAWABLE
CopyContextRect(const GFXRECT* area)
{
	GFXRECT clipRect;
	GFXRECT fgRect;
	GFXRECT r;

	if (!ContextActive() || !_CurFramePtr)
	{
		return nullptr;
	}

	fgRect = _get_context_fg_rect();
	GetContextClipRect(&clipRect);
	r = clipRect;
	if (area)
	{ // a portion of the context
		r.corner.x += area->corner.x;
		r.corner.y += area->corner.y;
		r.extent = area->extent;
	}
	// TODO: Should this take GFXCONTEXT origin into account too?
	// validate the rect
	if (!BoxIntersect(&r, &fgRect, &r))
	{
		return nullptr;
	}

	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
	{
		return LoadDisplayPixmap(&r, nullptr);
	}
	else
	{
		return CopyFrameRect(_CurFramePtr, &r);
	}
}

#ifdef DEBUG
const char*
GetContextName(GFXCONTEXT context)
{
	return context->name;
}

GFXCONTEXT
GetFirstContext(void)
{
	return firstContext;
}

GFXCONTEXT
GetNextContext(GFXCONTEXT context)
{
	return context->next;
}
#endif /* DEBUG */
