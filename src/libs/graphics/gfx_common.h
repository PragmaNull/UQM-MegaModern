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
#ifndef UQM_LIBS_GRAPHICS_GFX_COMMON_H
#define UQM_LIBS_GRAPHICS_GFX_COMMON_H

#include <stdio.h>
#include <stdlib.h>

#include "core/stl/atomic.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_defs.h"
#include "options/OptionDefs.h" // EmulationMode


// The following functions are driver-defined
void TFB_PreInit(void);
int TFB_InitGraphics(uqm::GfxDriver driver, uqm::GfxFlags flags, const char* renderer,
					 int width, int height, unsigned int* resFactor,
					 unsigned int* windowType);
int TFB_ReInitGraphics(uqm::GfxDriver driver, uqm::GfxFlags flags, int width, int height,
					   unsigned int* resFactor, unsigned int* windowType);
void TFB_UninitGraphics(void);
void TFB_ProcessEvents(void);
bool TFB_SetGamma(float gamma);
void TFB_UploadTransitionScreen(GfxRect* pRect);
int TFB_SupportsHardwareScaling(void);
// This function should not be called directly
void TFB_SwapBuffers(tfb::Redraw force_full_redraw);
// True if the ImGui overlay shows, and the screen must update each frame.
bool TFB_ImGuiVisible(void);

inline constexpr auto GscaleIdentity {256};

void LoadIntoExtraScreen(GfxRect* r);
void DrawFromExtraScreen(GfxRect* r);
int SetGraphicScale(int scale);
int GetGraphicScale(void);
tfb::ScaleMode SetGraphicScaleMode(tfb::ScaleMode mode);
tfb::ScaleMode GetGraphicScaleMode();
void SetTransitionSource(const GfxRect* pRect);
void ScreenTransition(uqm::EmulationMode transition, const GfxRect* pRect);

void TFB_FlushGraphics(void);		  // Only call from main thread!!
void TFB_PurgeDanglingGraphics(void); // Only call from main thread as part of shutdown.


void TFB_ScreenShot(void);
void TFB_ClearFPSCanvas(void);
void TFB_GetScreenSize(int16_t* width, int16_t* height);

// The flag variable itself
extern uqm::GfxFlags g_gfxFlags;
// Set to stop all drawing, for debugging.
extern int TFB_DEBUG_HALT;

// TODO: there should be accessor functions for these
extern uqstl::atomic<int> TransitionAmount;

extern float FrameRate;
extern int FrameRateTickBase;


extern int fs_height;
extern int fs_width;

extern int CanvasWidth;
extern int CanvasHeight;
extern int WindowWidth;
extern int WindowHeight;
extern int SavedWidth;
extern int SavedHeight;
extern int ScreenWidth;
extern int ScreenHeight;
extern int ScreenWidthActual;
extern int ScreenHeightActual;
extern int ScreenColorDepth;
extern uqm::GfxDriver GraphicsDriver;

#endif // UQM_LIBS_GRAPHICS_GFX_COMMON_H
