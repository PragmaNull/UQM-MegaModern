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

GraphicsStatusFlags _GraphicsStatusFlags;
GfxContextRef _pCurContext;

#ifdef DEBUG
// We keep track of all contexts
GfxContextRef firstContext;
// The first one in the list.
GfxContextRef* contextEnd = &firstContext;
// Where to put the next context.
#endif

Primitive _locPrim;

FontRef _CurFontPtr;

static constexpr auto DefaultForeColor {BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F)};
static constexpr auto DefaultBackColor {BuildColor(MakeRgb15(0x00, 0x00, 0x00), 0x00)};

static constexpr auto DefaultDrawMode {MAKE_DRAW_MODE(DrawKind::Default, 255)};

GfxContextRef SetContext(GfxContextRef Context)
{
	GfxContextRef LastContext {_pCurContext};
	if (Context != LastContext)
	{
		if (LastContext)
		{
			UnsetContextFlags(
				uqm::makeWord(0, GraphicsStatusFlags::GraphicsActive | GraphicsStatusFlags::DrawableActive));
			SetContextFlags(
				uqm::makeWord(0, _GraphicsStatusFlags & (GraphicsStatusFlags::GraphicsActive | GraphicsStatusFlags::DrawableActive)));

			DeactivateContext();
		}

		_pCurContext = Context;
		if (_pCurContext)
		{
			ActivateContext();

			_GraphicsStatusFlags &= ~(GraphicsStatusFlags::GraphicsActive | GraphicsStatusFlags::DrawableActive);
			_GraphicsStatusFlags |= static_cast<GraphicsStatusFlags>(uqm::highByte(_get_context_flags()));

			SetPrimColor(&_locPrim, _get_context_fg_color());

			_CurFramePtr = _get_context_fg_frame();
			_CurFontPtr = _get_context_font();
		}
	}

	return (LastContext);
}

#ifdef DEBUG
GfxContextRef CreateContextAux(const char* name)
#else  /* if !defined(DEBUG) */
GfxContextRef CreateContextAux(void)
#endif /* !defined(DEBUG) */
{
	GfxContextRef NewContext {new GraphicsContextDesc {}};
	if (NewContext)
	{
		/* initialize context */
#ifdef DEBUG
		NewContext->name = name;
		NewContext->next = nullptr;
		*contextEnd = NewContext;
		contextEnd = &NewContext->next;
#endif /* DEBUG */

		NewContext->Mode = DefaultDrawMode;
		NewContext->ForeGroundColor = DefaultForeColor;
		NewContext->BackGroundColor = DefaultBackColor;
	}

	return NewContext;
}

#ifdef DEBUG
// Loop through the list of context to the pointer which points to the
// specified context. This is either 'firstContext' or the address of
// the 'next' field of some other context.
static GfxContextRef* FindContextPtr(GfxContextRef context)
{
	GfxContextRef* ptr;

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

bool DestroyContext(GfxContextRef ContextRef)
{
	if (ContextRef == 0)
	{
		return (false);
	}

	if (_pCurContext && _pCurContext == ContextRef)
	{
		SetContext((GfxContextRef)0);
	}

#ifdef DEBUG
	// Unlink the context.
	{
		GfxContextRef* contextPtr = FindContextPtr(ContextRef);
		if (contextEnd == &ContextRef->next)
		{
			contextEnd = contextPtr;
		}
		*contextPtr = ContextRef->next;
	}
#endif /* DEBUG */

	tfb::Image* img {ContextRef->FontBacking};
	if (img)
	{
		TFB_DrawImage_Delete(img);
	}

	delete ContextRef;
	return true;
}

Color SetContextForeGroundColor(Color color)
{
	Color oldColor;

	if (!ContextActive())
	{
		return DefaultForeColor;
	}

	oldColor = _get_context_fg_color();
	if (!sameColor(oldColor, color))
	{
		SwitchContextForeGroundColor(color);

		if (!(testFlag(_get_context_fbk_flags(), FbkFlags::Image)))
		{
			SetContextFBkFlags(FbkFlags::Dirty);
		}
	}
	SetPrimColor(&_locPrim, color);

	return (oldColor);
}

Color GetContextForeGroundColor(void)
{
	if (!ContextActive())
	{
		return DefaultForeColor;
	}

	return _get_context_fg_color();
}

Color SetContextBackGroundColor(Color color)
{
	Color oldColor;

	if (!ContextActive())
	{
		return DefaultBackColor;
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
		return DefaultBackColor;
	}

	return _get_context_bg_color();
}

DrawMode SetContextDrawMode(DrawMode mode)
{
	DrawMode oldMode;

	if (!ContextActive())
	{
		return DefaultDrawMode;
	}

	oldMode = _get_context_draw_mode();
	SwitchContextDrawMode(mode);

	return oldMode;
}

DrawMode GetContextDrawMode(void)
{
	if (!ContextActive())
	{
		return DefaultDrawMode;
	}

	return _get_context_draw_mode();
}

// Returns a rect based at 0,0 and the size of context foreground frame
static inline GfxRect _get_context_fg_rect(void)
{
	GfxRect r = {
		{0, 0},
		{0, 0}
	  };
	if (_CurFramePtr)
	{
		r.extent = GetFrameBounds(_CurFramePtr);
	}
	return r;
}

bool SetContextClipRect(GfxRect* lpRect)
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

bool GetContextClipRect(GfxRect* lpRect)
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

GfxPoint SetContextOrigin(GfxPoint orgOffset)
{
	// XXX: This is a hack, kind of. But that's what the original did.
	return SetFrameHot(_CurFramePtr, orgOffset);
}

FrameRef SetContextFontEffect(FrameRef EffectFrame)
{
	if (!ContextActive())
	{
		return (nullptr);
	}

	FrameRef LastEffect {_get_context_fonteff()};
	if (EffectFrame != LastEffect)
	{
		SwitchContextFontEffect(EffectFrame);

		if (EffectFrame != 0)
		{
			SetContextFBkFlags(FbkFlags::Image);
		}
		else
		{
			UnsetContextFBkFlags(FbkFlags::Image);
		}
	}

	return LastEffect;
}

void FixContextFontEffect(void)
{
	int16_t w {};
	int16_t h {};

	if (!ContextActive() || (_get_context_font_backing() != 0 && !(testFlag(_get_context_fbk_flags(), FbkFlags::Dirty))))
	{
		return;
	}

	if (!GetContextFontDispHeight(&h) || !GetContextFontDispWidth(&w))
	{
		return;
	}

	tfb::Image* img {_pCurContext->FontBacking};
	if (img)
	{
		TFB_DrawScreen_DeleteImage(img);
	}

	img = TFB_DrawImage_CreateForScreen(w, h, true);
	if (testFlag(_get_context_fbk_flags(), FbkFlags::Image))
	{ // image pattern backing
		FrameRef EffectFrame = _get_context_fonteff();

		TFB_DrawImage_Image(EffectFrame->image,
							-EffectFrame->hotSpot.x, -EffectFrame->hotSpot.y,
							0, tfb::ScaleMode::None, nullptr, DrawReplaceMode, img);
	}
	else
	{ // solid color backing
		GfxRect r = {
			{0, 0},
			{w, h}
		  };
		const Color color = _get_context_fg_color();

		TFB_DrawImage_Rect(&r, color, DrawReplaceMode, img);
	}

	_pCurContext->FontBacking = img;
	UnsetContextFBkFlags(FbkFlags::Dirty);
}

// 'area' may be nullptr to copy the entire GfxContextRef cliprect
// 'area' is relative to the GfxContextRef cliprect
DrawableRef CopyContextRect(const GfxRect* area)
{
	GfxRect clipRect;
	GfxRect fgRect;
	GfxRect r;

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
	// TODO: Should this take GfxContextRef origin into account too?
	// validate the rect
	if (BoxIntersect(&r, &fgRect, &r) == IntersectCode::None)
	{
		return nullptr;
	}

	if (_CurFramePtr->Type == ScreenDrawable)
	{
		return LoadDisplayPixmap(&r, nullptr);
	}
	else
	{
		return CopyFrameRect(_CurFramePtr, &r);
	}
}

#ifdef DEBUG
const char* GetContextName(GfxContextRef context)
{
	return context->name;
}

GfxContextRef GetFirstContext(void)
{
	return firstContext;
}

GfxContextRef GetNextContext(GfxContextRef context)
{
	return context->next;
}
#endif /* DEBUG */
