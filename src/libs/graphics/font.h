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

#ifndef LIBS_GRAPHICS_FONT_H_
#define LIBS_GRAPHICS_FONT_H_

#include "libs/memlib.h"

#define MAX_DELTAS 100
#define MAX_UNICODE 0xFFFF

typedef struct FontPage
{
	struct FontPage* next;
	uqm::UniChar pageStart;
#define CHARACTER_PAGE_MASK 0xfffff800
	uqm::UniChar firstChar;
	size_t numChars;
	TFB_Char* charDesc;
} FONT_PAGE;

static inline FONT_PAGE*
AllocFontPage(int numChars)
{
	FONT_PAGE* result = (FONT_PAGE*)HMalloc(sizeof(FONT_PAGE));
	result->charDesc = (TFB_Char*)HCalloc(numChars * sizeof *result->charDesc);
	return result;
}

static inline void
FreeFontPage(FONT_PAGE* page)
{
	HFree(page->charDesc);
	HFree(page);
}

struct font_desc
{
	uint8_t Leading;
	FONT_PAGE* fontPages;
	EXTENT disp;
	char filename[PATH_MAX];
	uint8_t CharSpace;
	uint8_t KernAmount;
	uint8_t KernTab[MAX_UNICODE];
	int8_t VertAlign;
	bool HaveFntData;
};

#define CHAR_DESCPTR PCHAR_DESC

#define FONT_PRIORITY DEFAULT_MEM_PRIORITY

#define AllocFont(size) (FONT) HCalloc(sizeof(FONT_DESC) + (size))
#define FreeFont _ReleaseFontData

extern FONT _CurFontPtr;

// A font is a directory of glyph images, or a zip archive of them.
extern void* _GetFontData(const uqm::io::Directory& dir, uqstl::string_view path);
extern bool _ReleaseFontData(void* handle);
extern TFB_Char* GetFrameForFPS(uqm::UniChar ch);
extern void GetFontDims(int16_t* w, int16_t* h);
extern bool GoodToGoFPS(void);

#endif /* LIBS_GRAPHICS_FONT_H_ */
