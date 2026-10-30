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

#include "sdl_common.h"
#include "pure.h"
#include "primitives.h"
#include "options.h"
#include "uqmversion.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/dcqueue.h"
#include "libs/graphics/cmap.h"
#include "libs/graphics/gfx_defs.h"
#include "libs/input/sdl/input.h"
// for ProcessInputEvent()
#include "libs/graphics/bbox.h"
#include "port.h"
#include "core/log/log.h"
#include "libs/vidlib.h"


static void TFB_PreQuit(void);

void TFB_PreInit(void)
{
	const int linked {SDL_GetVersion()};
	uqm::log::info("Initializing base SDL functionality.");
	uqm::log::info("Using SDL version {}.{}.{} (compiled with {}.{}.{})",
				   SDL_VERSIONNUM_MAJOR(linked), SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked),
				   SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		uqm::log::critical("Could not initialize SDL: {}.", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	atexit(TFB_PreQuit);
}

static void
TFB_PreQuit(void)
{
	// Release the renderer and its GPU objects while SDL still runs.
	TFB_Pure_UninitGraphics();
	SDL_Quit();
}

int TFB_ReInitGraphics(uqm::GfxDriver driver, uqm::GfxFlags flags, int width, int height,
					   unsigned int* resFactor, unsigned int* windowType)
{
	int result;
	int togglefullscreen = 0;

	if (!testAnyFlag(g_gfxFlags, uqm::GfxFlagsFullscreen)
		&& driver == GraphicsDriver && width == WindowWidth && height == WindowHeight)
	{
		togglefullscreen = 1;
	}

	g_gfxFlags = flags;

	result = TFB_Pure_ConfigureVideo(uqm::GfxDriver::SDL_Pure, flags,
									 width, height, togglefullscreen, *resFactor, *windowType);

	if (testAnyFlag(flags, uqm::GfxFlagsFullscreen))
	{
		SDL_HideCursor();
	}
	else
	{
		SDL_ShowCursor();
	}

	return result;
}

bool TFB_SetGamma(float gamma)
{
	return TFB_SDL2_GammaCorrection(gamma);
	//log_add (log_Warning, "Custom gamma correction is not available in the SDL2 engine.");
	//(void) gamma; /* satisfy compiler (unused parameter) */
	//return false;
}

int TFB_HasSurfaceAlphaMod(SDL_Surface* surface)
{
	SDL_BlendMode blend_mode;
	if (!surface)
	{
		return 0;
	}
	if (!SDL_GetSurfaceBlendMode(surface, &blend_mode))
	{
		return 0;
	}
	return blend_mode == SDL_BLENDMODE_BLEND;
}

int TFB_GetSurfaceAlphaMod(SDL_Surface* surface, Uint8* alpha)
{
	SDL_BlendMode blend_mode;
	if (!surface || !alpha)
	{
		return -1;
	}
	if (SDL_GetSurfaceBlendMode(surface, &blend_mode))
	{
		if (blend_mode == SDL_BLENDMODE_BLEND)
		{
			return SDL_GetSurfaceAlphaMod(surface, alpha) ? 0 : -1;
		}
	}
	*alpha = 255;
	return 0;
}

int TFB_SetSurfaceAlphaMod(SDL_Surface* surface, Uint8 alpha)
{
	if (!surface)
	{
		return -1;
	}
	return SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND) && SDL_SetSurfaceAlphaMod(surface, alpha) ? 0 : -1;
}

int TFB_DisableSurfaceAlphaMod(SDL_Surface* surface)
{
	if (!surface)
	{
		return -1;
	}
	SDL_SetSurfaceAlphaMod(surface, 255);
	return SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE) ? 0 : -1;
}

int TFB_GetColorKey(SDL_Surface* surface, Uint32* key)
{
	if (!surface || !key)
	{
		return -1;
	}
	return SDL_GetSurfaceColorKey(surface, key) ? 0 : -1;
}

int TFB_SetColorKey(SDL_Surface* surface, Uint32 key, int rleaccel)
{
	if (!surface)
	{
		return -1;
	}
	SDL_SetSurfaceRLE(surface, rleaccel != 0);
	return SDL_SetSurfaceColorKey(surface, true, key) ? 0 : -1;
}

int TFB_DisableColorKey(SDL_Surface* surface)
{
	if (!surface)
	{
		return -1;
	}
	return SDL_SetSurfaceColorKey(surface, false, 0) ? 0 : -1;
}

int TFB_SetColors(SDL_Surface* surface, SDL_Color* colors, int firstcolor, int ncolors)
{
	SDL_Palette* palette {surface ? SDL_GetSurfacePalette(surface) : nullptr};
	if (!colors || !palette)
	{
		return 0;
	}
	return SDL_SetPaletteColors(palette, colors, firstcolor, ncolors) ? 1 : 0;
}

int TFB_SupportsHardwareScaling(void)
{
	return 1;
}
