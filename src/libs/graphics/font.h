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
#ifndef UQM_LIBS_GRAPHICS_FONT_H
#define UQM_LIBS_GRAPHICS_FONT_H

#include "core/stl/vector.h"

inline constexpr auto MaxDeltas {100};
inline constexpr auto MaxUnicode {0xFFFF};

inline constexpr auto CharacterPageMask {0xfffff800};

struct FontPage
{
	struct FontPage* next;
	uqm::UniChar pageStart;
	uqm::UniChar firstChar;
	size_t numChars;
	uqstl::vector<tfb::Char> charDesc;
};

static inline FontPage* AllocFontPage(int numChars)
{
	auto* result {new FontPage {}};
	result->charDesc.assign(numChars, tfb::Char {});
	return result;
}

static inline void FreeFontPage(FontPage* page)
{
	delete page;
}

struct FontDesc
{
	uint8_t Leading;
	FontPage* fontPages;
	GfxExtent disp;
	uint8_t CharSpace;
	uint8_t KernAmount;
	uint8_t KernTab[MaxUnicode];
	int8_t VertAlign;
	bool HaveFntData;
};


extern FontRef _CurFontPtr;

// A font is a directory of glyph images, or a zip archive of them.
extern void* _GetFontData(const uqm::io::Directory& dir, uqstl::string_view path);
extern bool _ReleaseFontData(void* handle);
extern tfb::Char* GetFrameForFPS(uqm::UniChar ch);
extern void GetFontDims(int16_t* w, int16_t* h);
extern int GetFontCharSpace(void);
extern bool GoodToGoFPS(void);

#endif // UQM_LIBS_GRAPHICS_FONT_H
