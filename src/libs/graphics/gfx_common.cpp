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
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawcmd.h"
#include "libs/time/timelib.h"
#include "libs/misc.h"
#include <atomic>
// for TFB_DEBUG_HALT
#include "options.h"
#include "SDL.h"

int fs_height = 0;
int fs_width = 0;

// Actual canvas size we're plotting our graphics to
int CanvasWidth;
int CanvasHeight;

// Size of the window on screen
int WindowWidth;
int WindowHeight;

// Real unforced window size we get from options
int SavedWidth;
int SavedHeight;

int ScreenWidth;
int ScreenHeight;
int ScreenWidthActual;
int ScreenHeightActual;

int ScreenColorDepth;
uqm::GfxDriver GraphicsDriver;
int TFB_DEBUG_HALT = 0;

uqstl::atomic<int> TransitionAmount {255};

static int gscale = GSCALE_IDENTITY;
static uqm::TFBScaleMode gscale_mode = uqm::TFBScaleMode::Nearest;

void DrawFromExtraScreen(GFXRECT* r)
{
	TFB_DrawScreen_Copy(r, TFB_SCREEN_EXTRA, TFB_SCREEN_MAIN);
}

void LoadIntoExtraScreen(GFXRECT* r)
{
	TFB_DrawScreen_Copy(r, TFB_SCREEN_MAIN, TFB_SCREEN_EXTRA);
}

int SetGraphicScale(int scale)
{
	int old_scale = gscale;
	gscale = (scale ? scale : GSCALE_IDENTITY);
	return old_scale;
}

int GetGraphicScale(void)
{
	return gscale;
}

uqm::TFBScaleMode SetGraphicScaleMode(uqm::TFBScaleMode mode)
{
	uqm::TFBScaleMode old_mode = gscale_mode;
	assert(mode >= uqm::TFBScaleMode::Nearest && mode <= uqm::TFBScaleMode::Trilinear);
	gscale_mode = mode;
	return old_mode;
}

uqm::TFBScaleMode GetGraphicScaleMode(void)
{
	return gscale_mode;
}

/* Batching and Unbatching functions.  A "Batch" is a collection of
   DrawCommands that will never be flipped to the screen half-rendered.
   BatchGraphics and UnbatchGraphics function vaguely like a non-blocking
   recursive lock to do this respect. */
void BatchGraphics(void)
{
	TFB_BatchGraphics();
}

void UnbatchGraphics(void)
{
	TFB_UnbatchGraphics();
}

/* Sleeps this thread until all Draw Commands queued by that thread have
   been processed. */

void FlushGraphics(void)
{
	TFB_DrawScreen_WaitForSignal();
}


void SetTransitionSource(const GFXRECT* pRect)
{
	TFB_DrawScreen_Copy(nullptr, TFB_SCREEN_MAIN, TFB_SCREEN_TRANSITION);
	(void)pRect; /* Satisfying compiler (unused parameter) */
}

// ScreenTransition() is synchronous (does not return until transition done)
void ScreenTransition(uqm::EmulationMode transType, const GFXRECT* pRect)
{
	static constexpr TimePeriod Duration = GameTicksPerSecond * 31 / 60;
	TimeCount startTime;

	if (transType == uqm::EmulationMode::PC)
	{
		return;
	}

	TFB_UploadTransitionScreen((GFXRECT*)pRect);

	TransitionAmount = 0;
	FlushGraphics();
	startTime = GetTimeCounter();
	while (TransitionAmount < 255)
	{
		TimePeriod deltaT;
		int newAmount;

		SleepThread(getTicksForFramerate(100));

		deltaT = GetTimeCounter() - startTime;
		newAmount = deltaT * 255 / Duration;
		if (newAmount > 255)
		{
			newAmount = 255;
		}

		TransitionAmount = newAmount;
	}
}
