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

#include "tfb_draw.h"

typedef uint16_t FBK_FLAGS;
inline constexpr auto FbkDirty {1 << 0};
inline constexpr auto FbkImage {1 << 1};

struct GraphicsContextDesc
{
	uint16_t Flags;
	// Low nibble currently unused
	// High nibble contains GRAPHICS_STATUS

	Color ForeGroundColor, BackGroundColor;
	DrawMode Mode;
	FRAME ForeGroundFrame;
	FONT Font;

	GfxRect ClipRect;

	FRAME FontEffect;
	tfb::Image* FontBacking;
	FBK_FLAGS BackingFlags;

#ifdef DEBUG
	const char* name;
	GFXCONTEXT next;
#endif
};


extern GFXCONTEXT _pCurContext;
extern Primitive _locPrim;

inline Color& _get_context_fg_color() { return _pCurContext->ForeGroundColor; }
inline Color& _get_context_bg_color() { return _pCurContext->BackGroundColor; }
inline uint16_t& _get_context_flags() { return _pCurContext->Flags; }
inline FRAME& _get_context_fg_frame() { return _pCurContext->ForeGroundFrame; }
inline FONT& _get_context_font() { return _pCurContext->Font; }
inline FBK_FLAGS& _get_context_fbk_flags() { return _pCurContext->BackingFlags; }
inline FRAME& _get_context_fonteff() { return _pCurContext->FontEffect; }
inline tfb::Image*& _get_context_font_backing() { return _pCurContext->FontBacking; }
inline DrawMode& _get_context_draw_mode() { return _pCurContext->Mode; }

inline void SwitchContextDrawMode(DrawMode m) { _pCurContext->Mode = m; }
inline void SwitchContextForeGroundColor(Color c) { _pCurContext->ForeGroundColor = c; }
inline void SwitchContextBackGroundColor(Color c) { _pCurContext->BackGroundColor = c; }
inline void SetContextFlags(uint16_t f) { _pCurContext->Flags |= f; }
inline void UnsetContextFlags(uint16_t f) { _pCurContext->Flags &= ~f; }
inline void SwitchContextFGFrame(FRAME f) { _pCurContext->ForeGroundFrame = f; }
inline void SetContextFBkFlags(FBK_FLAGS f) { _pCurContext->BackingFlags |= f; }
inline void UnsetContextFBkFlags(FBK_FLAGS f) { _pCurContext->BackingFlags &= ~f; }

inline void SwitchContextFont(FONT f)
{
	_pCurContext->Font = f;
	SetContextFBkFlags(FbkDirty);
}

inline void SwitchContextFontEffect(FRAME f)
{
	_pCurContext->FontEffect = f;
	SetContextFBkFlags(FbkDirty);
}

typedef uint8_t GRAPHICS_STATUS;

extern GRAPHICS_STATUS _GraphicsStatusFlags;
inline constexpr auto GraphicsActiveFlag {(GRAPHICS_STATUS)(1 << 0)};
inline constexpr auto GraphicsVisible {(GRAPHICS_STATUS)(1 << 1)};
inline constexpr auto ContextActiveFlag {(GRAPHICS_STATUS)(1 << 2)};
inline constexpr auto DrawableActiveFlag {(GRAPHICS_STATUS)(1 << 3)};
inline void DeactivateGraphics() { _GraphicsStatusFlags &= ~GraphicsActiveFlag; }
inline void ActivateGraphics() { _GraphicsStatusFlags |= GraphicsActiveFlag; }
inline bool GraphicsActive() { return (_GraphicsStatusFlags & GraphicsActiveFlag) != 0; }
inline void DeactivateVisible() { _GraphicsStatusFlags &= ~GraphicsVisible; }
inline void ActivateVisible() { _GraphicsStatusFlags |= GraphicsVisible; }
inline void DeactivateContext() { _GraphicsStatusFlags &= ~ContextActiveFlag; }
inline void ActivateContext() { _GraphicsStatusFlags |= ContextActiveFlag; }
inline bool ContextActive() { return (_GraphicsStatusFlags & ContextActiveFlag) != 0; }
inline void DeactivateDrawable() { _GraphicsStatusFlags &= ~DrawableActiveFlag; }
inline void ActivateDrawable() { _GraphicsStatusFlags |= DrawableActiveFlag; }
inline bool DrawableActive() { return (_GraphicsStatusFlags & DrawableActiveFlag) != 0; }

inline constexpr auto SystemActive {(GRAPHICS_STATUS)(ContextActiveFlag | DrawableActiveFlag)};

inline bool GraphicsSystemActive() { return (_GraphicsStatusFlags & SystemActive) == SystemActive; }
inline GRAPHICS_STATUS GraphicsStatus() { return _GraphicsStatusFlags & (GRAPHICS_STATUS)(GraphicsActiveFlag | GraphicsVisible); }

// pValidRect or origin may be nullptr
bool GetContextValidRect(GfxRect* pValidRect, GfxPoint* origin);
extern void FixContextFontEffect(void);

#endif // UQM_LIBS_GRAPHICS_CONTEXT_H
