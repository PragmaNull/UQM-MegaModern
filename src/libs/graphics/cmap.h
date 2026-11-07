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
#ifndef UQM_LIBS_GRAPHICS_CMAP_H
#define UQM_LIBS_GRAPHICS_CMAP_H

#include "libs/gfxlib.h"

inline constexpr auto MaxColormaps {250};

// These are pertinent to colortable file format
// We load colormaps as binary and parse them when needed
inline constexpr auto PlutvalByteSize {3};
// Channel order in colormap tables
inline constexpr auto PlutvalRed {0};
inline constexpr auto PlutvalGreen {1};
inline constexpr auto PlutvalBlue {2};

inline constexpr auto NumberOfPlutvals {256};
// Size of the colormap in a colortable file


inline constexpr auto FadeNoIntensity {0};
inline constexpr auto FadeNormalIntensity {255};
inline constexpr auto FadeFullIntensity {510};

typedef struct NativePalette NativePalette;

struct TFB_ColorMap
{
	int index;
	// Colormap index as the game sees it
	int version;
	// Version goes up every time the colormap changes. This may
	// be due to SetColorMap() or at every transformation step
	// of XFormColorMap(). Paletted TFB_Images track the last
	// colormap version they were drawn with for optimization.
	int refcount;
	struct TFB_ColorMap* next;
	// for spares linking
	NativePalette* palette;
};

extern int GetFadeAmount(void);

extern void InitColorMaps(void);
extern void UninitColorMaps(void);

extern void GetColorMapColors(Color* colors, TFB_ColorMap*);

extern TFB_ColorMap* TFB_GetColorMap(int index);
extern void TFB_ReturnColorMap(TFB_ColorMap* map);

extern bool XFormColorMap_step(void);
extern void SetColorMapColors(Color* colors, COLORMAPPTR ColorMapPtr,
							  uint16_t from, uint16_t to);
extern Color GetColorMapColor(uint16_t ColorMapIndex, uint16_t ColorIndex);

// Native
NativePalette* AllocNativePalette(void);
void FreeNativePalette(NativePalette*);
void SetNativePaletteColor(NativePalette*, int index, Color);
Color GetNativePaletteColor(NativePalette*, int index);

#endif // UQM_LIBS_GRAPHICS_CMAP_H
