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

#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <atomic>

#include "libs/graphics/gfx_defs.h"
#include "sdl_common.h"
#include "pure.h"
#include "primitives.h"
#include "options.h"
#include "uqmversion.h"
#include "core/platform/platform.h"
#include "core/string/StringUtils.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/dcqueue.h"
#include "libs/graphics/cmap.h"
#include "libs/input/sdl/input.h"
// for ProcessInputEvent()
#include "libs/graphics/bbox.h"
#include "port.h"
#include "core/log/log.h"
#include "libs/vidlib.h"
#include "uqm/units.h"
#include "uqm/globdata.h"

#include <time.h>

SDL_Surface* SDL_Screen;
SDL_Surface* TransitionScreen;

SDL_Surface* SDL_Screens[TFB_GFX_NUMSCREENS];
SDL_Surface* SDL_Screen_fps;
bool SDL_Screen_fps_changed {};

SDL_Surface* format_conv_surf = nullptr;

SDL_Rect* pTransitionClipRect = nullptr;


static uqstl::atomic<bool> abortFlag {false};

uqm::GfxFlags g_gfxFlags {uqm::GfxFlags::None};

TFB_GRAPHICS_BACKEND* graphics_backend = nullptr;

uqstl::atomic<int> QuitPosted {0};
uqstl::atomic<int> GameActive {1}; // Track the SDL_ACTIVEEVENT state SDL_APPACTIVE

static inline bool
IsWholeScreen(GFXRECT* r)
{
	return (bool)(r->corner.x == 0 && r->corner.y == 0 && r->extent.width == CanvasWidth && r->extent.height == CanvasHeight);
}

int TFB_InitGraphics(uqm::GfxDriver driver, uqm::GfxFlags flags, const char* renderer,
					 int width, int height, unsigned int* resFactor,
					 unsigned int* windowType)
{
	int result, i;


	/* Null out screen pointers the first time */
	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		SDL_Screens[i] = nullptr;
	}

	g_gfxFlags = flags;


	result = TFB_Pure_InitGraphics(driver, flags, renderer, width,
								   height, *resFactor, *windowType);


	if (testAnyFlag(flags, uqm::GfxFlagsFullscreen))
	{
		SDL_HideCursor();
	}

	Init_DrawCommandQueue();

	TFB_DrawCanvas_Initialize();

	return result;
}

void TFB_UninitGraphics(void)
{
	int i;

	Uninit_DrawCommandQueue();

	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		UnInit_Screen(&SDL_Screens[i]);
	}

	UnInit_Screen(&SDL_Screen_fps);

	TFB_Pure_UninitGraphics();

	UnInit_Screen(&format_conv_surf);
}

void TFB_ProcessEvents()
{
	SDL_Event Event;

	while (SDL_PollEvent(&Event))
	{
		/* Run through the InputEvent filter. */
		ProcessInputEvent(&Event);
		/* Handle graphics and exposure events. */
		switch (Event.type)
		{
#if 0 /* Currently disabled in mainline */
			case SDL_ACTIVEEVENT:    /* Lose/gain visibility or focus */
				/* Up to three different state changes can occur in one event. */
				/* Here, disregard least significant change (mouse focus). */
				// This controls the automatic sleep/pause when minimized.
				// On small displays (e.g. mobile devices), APPINPUTFOCUS would
				//  be an appropriate substitution for APPACTIVE:
				// if (Event.active.state & SDL_APPINPUTFOCUS)
				if (Event.active.state & SDL_APPACTIVE)
					GameActive = Event.active.gain;
				break;
			case SDL_VIDEORESIZE:    /* User resized video mode */
				// TODO
				break;
#endif
			case SDL_EVENT_QUIT:
				QuitPosted = 1;
				break;
			case SDL_EVENT_WINDOW_EXPOSED:
				/* Screen needs to be redrawn */
				TFB_SwapBuffers(uqm::TFBRedraw::Expose);
				break;
			case SDL_EVENT_RENDER_TARGETS_RESET:
			case SDL_EVENT_RENDER_DEVICE_RESET:
				// The textures lost their contents, so upload all of the screen again.
				TFB_SwapBuffers(uqm::TFBRedraw::Yes);
				break;
			default:
				break;
		}
	}
}

static bool system_box_active = false;
static SDL_Rect system_box;

void SetSystemRect(const GFXRECT* r)
{
	system_box_active = true;
	system_box.x = r->corner.x;
	system_box.y = r->corner.y;
	system_box.w = r->extent.width;
	system_box.h = r->extent.height;
}

void ClearSystemRect(void)
{
	system_box_active = false;
}

void TFB_SwapBuffers(uqm::TFBRedraw force_full_redraw)
{
	static int last_fade_amount = 255, last_transition_amount = 255;
	static int fade_amount = 255, transition_amount = 255;
	Uint8 sfx;

	fade_amount = GetFadeAmount();
	transition_amount = TransitionAmount;

	if (force_full_redraw == uqm::TFBRedraw::No)
	{
		if (!TFB_BBox.valid && fade_amount == 255 && transition_amount == 255 && last_fade_amount == 255 && last_transition_amount == 255)
		{
			return;
		}

		if (fade_amount != 255 || transition_amount != 255 || last_fade_amount != 255 || last_transition_amount != 255)
		{
			force_full_redraw = uqm::TFBRedraw::Fading;
		}
	}


	sfx = last_fade_amount > fade_amount ? 1 : 0;

	last_fade_amount = fade_amount;
	last_transition_amount = transition_amount;

	graphics_backend->preprocess(force_full_redraw, transition_amount,
								 fade_amount);
	graphics_backend->screen(TFB_SCREEN_MAIN, 255, nullptr);

	if (transition_amount != 255)
	{
		graphics_backend->screen(TFB_SCREEN_TRANSITION,
								 255 - transition_amount, pTransitionClipRect);
	}

	if (fade_amount != 255)
	{
#if defined(__APPLE__)
		if (fade_amount < 255)
		{
			graphics_backend->color(0, 0, 0, 255 - fade_amount, nullptr);
		}
		else
		{
			graphics_backend->color(255, 255, 255,
									fade_amount - 255, nullptr);
		}
#else
		if (isPC(uqm::UQMOptions::read().scrTrans))
		{
			if (fade_amount < 255)
			{
				graphics_backend->color(SDL_BLENDOPERATION_REV_SUBTRACT,
										sfx, 0, 255 - fade_amount, nullptr);
			}
			else
			{
				graphics_backend->color(SDL_BLENDOPERATION_ADD, 0, 0,
										fade_amount - 255, nullptr);
			}
		}
		else
		{
			if (fade_amount < 255)
			{
				graphics_backend->color(0, 0, 0, 255 - fade_amount, nullptr);
			}
			else
			{
				graphics_backend->color(255, 255, 255,
										fade_amount - 255, nullptr);
			}
		}
#endif
	}

	// After Pause/F10 exit rework we don't need it in both SDL1 and SDL2
	// TODO: delete systembox
	/*if (system_box_active)
	{
		graphics_backend->screen (TFB_SCREEN_MAIN, 255, &system_box);
	}*/

	graphics_backend->postprocess(IS_HD);
}

/* Probably ought to clean this away at some point. */
SDL_Surface*
TFB_DisplayFormatAlpha(SDL_Surface* surface)
{
	SDL_Surface* newsurf;

	// figure out what format to use (alpha/no alpha)
	const SDL_PixelFormat dstformat {SDL_GetPixelFormatDetails(surface->format)->Amask ? format_conv_surf->format : SDL_Screen->format};

	if (surface->format == dstformat)
	{
		return surface; // no conversion needed
	}

	newsurf = SDL_ConvertSurface(surface, dstformat);
	// Colorkeys and surface-level alphas cannot work at the same time,
	// so we need to disable one of them
	if (TFB_HasColorKey(surface) && newsurf && TFB_HasColorKey(newsurf) && TFB_HasSurfaceAlphaMod(newsurf))
	{
		TFB_DisableSurfaceAlphaMod(newsurf);
	}

	return newsurf;
}

// This function should only be called from the graphics thread,
// like from a TFB_DrawCommand_Callback command.
TFB_Canvas
TFB_GetScreenCanvas(SCREEN screen)
{
	return SDL_Screens[screen];
}

TFB_Canvas
TFB_GetFPSCanvas(void)
{
	return SDL_Screen_fps;
}

void TFB_BlitSurface(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst,
					 SDL_Rect* dstrect, int blend_numer, int blend_denom)
{
	bool has_colorkey;
	int x, y, x1, y1, x2, y2, dst_x2, dst_y2, nr, ng, nb;
	int srcx, srcy, w, h;
	Uint8 sr, sg, sb, dr, dg, db;
	Uint32 src_pixval, dst_pixval, colorkey;
	GetPixelFn src_getpix, dst_getpix;
	PutPixelFn putpix;
	SDL_Rect fulldst;

	if (blend_numer == blend_denom)
	{
		// normal blit: dst = src

		// log_add (log_Debug, "normal blit\n");
		SDL_BlitSurface(src, srcrect, dst, dstrect);
		return;
	}

	// NOTE: following clipping code is copied from SDL-1.2.4 sources

	// If the destination rectangle is nullptr, use the entire dest surface
	if (dstrect == nullptr)
	{
		fulldst.x = fulldst.y = 0;
		dstrect = &fulldst;
	}

	// clip the source rectangle to the source surface
	if (srcrect)
	{
		int maxw, maxh;

		srcx = srcrect->x;
		w = srcrect->w;
		if (srcx < 0)
		{
			w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->w - srcx;
		if (maxw < w)
		{
			w = maxw;
		}

		srcy = srcrect->y;
		h = srcrect->h;
		if (srcy < 0)
		{
			h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->h - srcy;
		if (maxh < h)
		{
			h = maxh;
		}
	}
	else
	{
		srcx = 0;
		srcy = 0;
		w = src->w;
		h = src->h;
	}

	// clip the destination rectangle against the clip rectangle
	{
		SDL_Rect clipRect;
		SDL_GetSurfaceClipRect(dst, &clipRect);
		SDL_Rect* clip = &clipRect;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if (dx > 0)
		{
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if (dx > 0)
		{
			w -= dx;
		}

		dy = clip->y - dstrect->y;
		if (dy > 0)
		{
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if (dy > 0)
		{
			h -= dy;
		}
	}

	dstrect->w = w;
	dstrect->h = h;

	if (w <= 0 || h <= 0)
	{
		return;
	}

	x1 = srcx;
	y1 = srcy;
	x2 = srcx + w;
	y2 = srcy + h;

	if (TFB_GetColorKey(src, &colorkey) < 0)
	{
		has_colorkey = false;
		colorkey = 0; /* Satisfying compiler */
	}
	else
	{
		has_colorkey = true;
	}

	src_getpix = getpixel_for(src);
	dst_getpix = getpixel_for(dst);
	putpix = putpixel_for(dst);

	if (blend_denom < 0)
	{
		// additive blit: dst = src + dst
#if 0
		log_add (log_Debug, "additive blit {} {}, src {} {} {} {} dst {} {},"
				" srcbpp {}", blend_numer, blend_denom, x1, y1, x2, y2,
				dstrect->x, dstrect->y, SDL_GetPixelFormatDetails(src->format)->bits_per_pixel);
#endif
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix(src, x, y);

				if (has_colorkey && src_pixval == colorkey)
				{
					continue;
				}

				dst_pixval = dst_getpix(dst, dst_x2, dst_y2);

				SDL_GetRGB(src_pixval, SDL_GetPixelFormatDetails(src->format), SDL_GetSurfacePalette(src), &sr, &sg, &sb);
				SDL_GetRGB(dst_pixval, SDL_GetPixelFormatDetails(dst->format), SDL_GetSurfacePalette(dst), &dr, &dg, &db);

				nr = sr + dr;
				ng = sg + dg;
				nb = sb + db;

				if (nr > 255)
				{
					nr = 255;
				}
				if (ng > 255)
				{
					ng = 255;
				}
				if (nb > 255)
				{
					nb = 255;
				}

				putpix(dst, dst_x2, dst_y2,
					   SDL_MapSurfaceRGB(dst, nr, ng, nb));
			}
		}
	}
	else if (blend_numer < 0)
	{
		// subtractive blit: dst = src - dst
#if 0
		log_add (log_Debug, "subtractive blit {} {}, src {} {} {} {}"
				" dst {} {}, srcbpp {}", blend_numer, blend_denom,
					x1, y1, x2, y2, dstrect->x, dstrect->y,
					SDL_GetPixelFormatDetails(src->format)->bits_per_pixel);
#endif
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix(src, x, y);

				if (has_colorkey && src_pixval == colorkey)
				{
					continue;
				}

				dst_pixval = dst_getpix(dst, dst_x2, dst_y2);

				SDL_GetRGB(src_pixval, SDL_GetPixelFormatDetails(src->format), SDL_GetSurfacePalette(src), &sr, &sg, &sb);
				SDL_GetRGB(dst_pixval, SDL_GetPixelFormatDetails(dst->format), SDL_GetSurfacePalette(dst), &dr, &dg, &db);

				nr = sr - dr;
				ng = sg - dg;
				nb = sb - db;

				if (nr < 0)
				{
					nr = 0;
				}
				if (ng < 0)
				{
					ng = 0;
				}
				if (nb < 0)
				{
					nb = 0;
				}

				putpix(dst, dst_x2, dst_y2,
					   SDL_MapSurfaceRGB(dst, nr, ng, nb));
			}
		}
	}
	else
	{
		// modulated blit: dst = src * (blend_numer / blend_denom)

		float f = blend_numer / (float)blend_denom;
#if 0
		log_add (log_Debug, "modulated blit {} {}, f {}, src {} {} {} {}"
				" dst {} {}, srcbpp {}\n", blend_numer, blend_denom, f,
				x1, y1, x2, y2, dstrect->x, dstrect->y,
				SDL_GetPixelFormatDetails(src->format)->bits_per_pixel);
#endif
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix(src, x, y);

				if (has_colorkey && src_pixval == colorkey)
				{
					continue;
				}

				SDL_GetRGB(src_pixval, SDL_GetPixelFormatDetails(src->format), SDL_GetSurfacePalette(src), &sr, &sg, &sb);

				nr = (int)(sr * f);
				ng = (int)(sg * f);
				nb = (int)(sb * f);

				if (nr > 255)
				{
					nr = 255;
				}
				if (ng > 255)
				{
					ng = 255;
				}
				if (nb > 255)
				{
					nb = 255;
				}

				putpix(dst, dst_x2, dst_y2,
					   SDL_MapSurfaceRGB(dst, nr, ng, nb));
			}
		}
	}
}

void UnInit_Screen(SDL_Surface** screen)
{
	if (*screen == nullptr)
	{
		return;
	}

	SDL_DestroySurface(*screen);
	*screen = nullptr;
}

void TFB_UploadTransitionScreen(GFXRECT* pRect)
{
	graphics_backend->uploadTransitionScreen();

	(void)pRect; /*satisfy compiler (unused parameter)*/
}

int TFB_HasColorKey(SDL_Surface* surface)
{
	Uint32 key;
	return TFB_GetColorKey(surface, &key) == 0;
}

SDL_Surface* TFB_CreateSurface(int width, int height, SDL_PixelFormat format)
{
	SDL_Surface* surface {SDL_CreateSurface(width, height, format)};
	if (surface && SDL_ISPIXELFORMAT_INDEXED(format) && !SDL_CreateSurfacePalette(surface))
	{
		SDL_DestroySurface(surface);
		return nullptr;
	}
	return surface;
}

SDL_Surface* TFB_CreateSurface(int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	return TFB_CreateSurface(width, height, SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask));
}

void TFB_ScreenShot(void)
{
	const uqstl::string shotDirName = uqm::getEnvironmentValue("UQM_SCR_SHOT_DIR");
	auto now = std::chrono::system_clock::now();
	auto fullPath = fmt::format("{}{:%Y-%m-%d %H-%M-%S} v{}.{}.{} {}.png", shotDirName, now,
								UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
								chooseIfHd<const char*>(UQM_EXTRA_VERSION, "HD " UQM_EXTRA_VERSION));

	if (std::filesystem::exists(shotDirName))
	{
		if (TFB_SDL_ScreenShot(fullPath.c_str()))
		{
			uqm::log::info("Screenshot saved at path, '{}'", fullPath);
		}
		else
		{
			uqm::log::error("Screenshot not saved due to an error");
		}
	}
	else
	{
		uqm::log::error("Screenshot not saved because the directory '{}' does not exist", shotDirName);
	}
}

void TFB_ClearFPSCanvas(void)
{
	SDL_FillSurfaceRect(SDL_Screen_fps, nullptr, 0x00000000);
	SDL_Screen_fps_changed = true;
}

void TFB_GetScreenSize(int16_t* width, int16_t* height)
{
	SDL_Rect bounds;

	TFB_SDL2_GetDisplaySize(&bounds);

	*width = bounds.w;
	*height = bounds.h;
}
