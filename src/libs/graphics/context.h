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

#pragma once
#ifndef UQM_LIBS_GRAPHICS_CONTEXT_H
#define UQM_LIBS_GRAPHICS_CONTEXT_H

#include "core/types/enum.h"
#include "tfb_draw.h"

enum class FbkFlags : uint16_t
{
	None = 0,
	Dirty = 1 << 0,
	Image = 1 << 1,
};
ENUM_BITWISE_OPS(FbkFlags)

struct GraphicsContextDesc
{
	uint16_t Flags;
	// Low nibble currently unused
	// High nibble contains GraphicsStatusFlags

	Color ForeGroundColor, BackGroundColor;
	DrawMode Mode;
	FrameRef ForeGroundFrame;
	FontRef Font;

	GfxRect ClipRect;

	FrameRef FontEffect;
	tfb::Image* FontBacking;
	FbkFlags BackingFlags;

#ifdef DEBUG
	const char* name;
	GfxContextRef next;
#endif
};


extern GfxContextRef _pCurContext;
extern Primitive _locPrim;

inline Color& _get_context_fg_color() { return _pCurContext->ForeGroundColor; }
inline Color& _get_context_bg_color() { return _pCurContext->BackGroundColor; }
inline uint16_t& _get_context_flags() { return _pCurContext->Flags; }
inline FrameRef& _get_context_fg_frame() { return _pCurContext->ForeGroundFrame; }
inline FontRef& _get_context_font() { return _pCurContext->Font; }
inline FbkFlags& _get_context_fbk_flags() { return _pCurContext->BackingFlags; }
inline FrameRef& _get_context_fonteff() { return _pCurContext->FontEffect; }
inline tfb::Image*& _get_context_font_backing() { return _pCurContext->FontBacking; }
inline DrawMode& _get_context_draw_mode() { return _pCurContext->Mode; }

inline void SwitchContextDrawMode(DrawMode m) { _pCurContext->Mode = m; }
inline void SwitchContextForeGroundColor(Color c) { _pCurContext->ForeGroundColor = c; }
inline void SwitchContextBackGroundColor(Color c) { _pCurContext->BackGroundColor = c; }
inline void SetContextFlags(uint16_t f) { _pCurContext->Flags |= f; }
inline void UnsetContextFlags(uint16_t f) { _pCurContext->Flags &= ~f; }
inline void SwitchContextFGFrame(FrameRef f) { _pCurContext->ForeGroundFrame = f; }
inline void SetContextFBkFlags(FbkFlags f) { _pCurContext->BackingFlags |= f; }
inline void UnsetContextFBkFlags(FbkFlags f) { _pCurContext->BackingFlags &= ~f; }

inline void SwitchContextFont(FontRef f)
{
	_pCurContext->Font = f;
	SetContextFBkFlags(FbkFlags::Dirty);
}

inline void SwitchContextFontEffect(FrameRef f)
{
	_pCurContext->FontEffect = f;
	SetContextFBkFlags(FbkFlags::Dirty);
}


enum class GraphicsStatusFlags : uint8_t
{
	None = 0,
	GraphicsActive = 1 << 0,
	GraphicsVisible = 1 << 1,
	ContextActive = 1 << 2,
	DrawableActive = 1 << 3,
};
ENUM_BITWISE_OPS(GraphicsStatusFlags)

extern GraphicsStatusFlags _GraphicsStatusFlags;
inline void DeactivateGraphics() { _GraphicsStatusFlags &= ~GraphicsStatusFlags::GraphicsActive; }
inline void ActivateGraphics() { _GraphicsStatusFlags |= GraphicsStatusFlags::GraphicsActive; }
inline bool GraphicsActive() { return testFlag(_GraphicsStatusFlags, GraphicsStatusFlags::GraphicsActive); }
inline void DeactivateVisible() { _GraphicsStatusFlags &= ~GraphicsStatusFlags::GraphicsVisible; }
inline void ActivateVisible() { _GraphicsStatusFlags |= GraphicsStatusFlags::GraphicsVisible; }
inline void DeactivateContext() { _GraphicsStatusFlags &= ~GraphicsStatusFlags::ContextActive; }
inline void ActivateContext() { _GraphicsStatusFlags |= GraphicsStatusFlags::ContextActive; }
inline bool ContextActive() { return testFlag(_GraphicsStatusFlags, GraphicsStatusFlags::ContextActive); }
inline void DeactivateDrawable() { _GraphicsStatusFlags &= ~GraphicsStatusFlags::DrawableActive; }
inline void ActivateDrawable() { _GraphicsStatusFlags |= GraphicsStatusFlags::DrawableActive; }
inline bool DrawableActive() { return testFlag(_GraphicsStatusFlags, GraphicsStatusFlags::DrawableActive); }

inline constexpr auto SystemActive {(GraphicsStatusFlags)(GraphicsStatusFlags::ContextActive | GraphicsStatusFlags::DrawableActive)};

inline bool GraphicsSystemActive() { return (_GraphicsStatusFlags & SystemActive) == SystemActive; }
inline GraphicsStatusFlags GraphicsStatus() { return _GraphicsStatusFlags & (GraphicsStatusFlags)(GraphicsStatusFlags::GraphicsActive | GraphicsStatusFlags::GraphicsVisible); }

// pValidRect or origin may be nullptr
bool GetContextValidRect(GfxRect* pValidRect, GfxPoint* origin);
extern void FixContextFontEffect(void);

#endif // UQM_LIBS_GRAPHICS_CONTEXT_H
