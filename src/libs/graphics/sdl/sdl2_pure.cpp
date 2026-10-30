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

#include "pure.h"
#include "libs/graphics/bbox.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "scalers.h"
#include "uqmversion.h"
#include "png2sdl.h"
#include "options.h"


typedef struct tfb_sdl2_screeninfo_s
{
	SDL_Surface* scaled;
	SDL_Texture* texture;
	bool dirty, active;
	SDL_Rect updated;
} TFB_SDL2_SCREENINFO;

static TFB_SDL2_SCREENINFO SDL2_Screens[TFB_GFX_NUMSCREENS];

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static const char* rendererBackend = nullptr;

static TFB_ScaleFunc scaler = nullptr;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define A_MASK 0xff000000
#define B_MASK 0x00ff0000
#define G_MASK 0x0000ff00
#define R_MASK 0x000000ff
#else
#define A_MASK 0x000000ff
#define B_MASK 0x0000ff00
#define G_MASK 0x00ff0000
#define R_MASK 0xff000000
#endif

static void TFB_SDL2_Preprocess(uqm::TFBRedraw force_full_redraw,
								int transition_amount, int fade_amount);
static void TFB_SDL2_Postprocess(bool hd);
static void TFB_SDL2_UploadTransitionScreen(void);
static void TFB_SDL2_Scaled_ScreenLayer(SCREEN screen, Uint8 a,
										SDL_Rect* rect);
static void TFB_SDL2_Unscaled_ScreenLayer(SCREEN screen, Uint8 a,
										  SDL_Rect* rect);
static void TFB_SDL2_ColorLayer(Uint8 r, Uint8 g, Uint8 b, Uint8 a,
								SDL_Rect* rect);

static TFB_GRAPHICS_BACKEND sdl2_scaled_backend = {
	TFB_SDL2_Preprocess,
	TFB_SDL2_Postprocess,
	TFB_SDL2_UploadTransitionScreen,
	TFB_SDL2_Scaled_ScreenLayer,
	TFB_SDL2_ColorLayer};

static TFB_GRAPHICS_BACKEND sdl2_unscaled_backend = {
	TFB_SDL2_Preprocess,
	TFB_SDL2_Postprocess,
	TFB_SDL2_UploadTransitionScreen,
	TFB_SDL2_Unscaled_ScreenLayer,
	TFB_SDL2_ColorLayer};

static SDL_Surface*
Create_Screen(int w, int h)
{
	SDL_Surface* newsurf = TFB_CreateSurface(w, h,
											 32, R_MASK, G_MASK, B_MASK, 0);
	if (newsurf == 0)
	{
		uqm::log::error("Couldn't create screen buffers: {}",
						SDL_GetError());
	}
	return newsurf;
}

static int
ReInit_Screen(SDL_Surface** screen, int w, int h)
{
	if (*screen)
	{
		SDL_DestroySurface(*screen);
	}
	*screen = Create_Screen(w, h);

	return *screen == 0 ? -1 : 0;
}

static int
ReInit_FPS_Screen(SDL_Surface** screen, int w, int h)
{
	if (*screen)
	{
		SDL_DestroySurface(*screen);
	}
	*screen = TFB_CreateSurface(w, h, 32, R_MASK, G_MASK,
								B_MASK, A_MASK);
	if (*screen == 0)
	{
		uqm::log::error("Couldn't create screen buffers: {}",
						SDL_GetError());
	}

	return *screen == 0 ? -1 : 0;
}

// Returns the name of the render driver to use, or nullptr to let SDL choose.
static const char*
FindBestRenderDriver(void)
{
	if (!rendererBackend || !*rendererBackend)
	{
		return nullptr;
	}
	uqm::log::info("Searching for render driver \"{}\".",
				   rendererBackend);

	const int n {SDL_GetNumRenderDrivers()};
	for (int i {}; i < n; i++)
	{
		const char* name {SDL_GetRenderDriver(i)};
		if (name && !strcmp(name, rendererBackend))
		{
			return name;
		}
		uqm::log::info("Skipping render driver \"{}\"", name ? name : "");
	}
	uqm::log::info("Render driver \"{}\" not available, using system "
				   "default",
				   rendererBackend);
	return nullptr;
}

static void
SetFullscreen(uqm::GfxFlags flags, int width, int height)
{
	if (testFlag(flags, uqm::GfxFlags::Fullscreen))
	{
		SDL_SetWindowFullscreenMode(window, nullptr);
		SDL_SetWindowFullscreen(window, true);
	}
	else if (testFlag(flags, uqm::GfxFlags::ExclusiveFullscreen))
	{
		SDL_DisplayMode mode {};
		if (SDL_GetClosestFullscreenDisplayMode(SDL_GetDisplayForWindow(window), width, height, 0.0f, false, &mode))
		{
			SDL_SetWindowFullscreenMode(window, &mode);
		}
		else
		{
			uqm::log::warn("No fullscreen mode for {}x{}: {}", width, height, SDL_GetError());
			SDL_SetWindowFullscreenMode(window, nullptr);
		}
		SDL_SetWindowFullscreen(window, true);
	}
	else
	{
		SDL_SetWindowFullscreen(window, false);
	}
}

static SDL_Texture*
CreateScreenTexture(int width, int height)
{
	SDL_Texture* texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, width, height)};
	if (texture)
	{
		SDL_SetTextureScaleMode(texture, testFlag(g_gfxFlags, uqm::GfxFlagsScaleAny) ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
	}
	return texture;
}

static SDL_FRect
ToFRect(const SDL_Rect& rect)
{
	SDL_FRect result {};
	SDL_RectToFRect(&rect, &result);
	return result;
}

int TFB_Pure_ConfigureVideo(uqm::GfxDriver driver, uqm::GfxFlags flags, int width, int height,
							int togglefullscreen, unsigned int resFactor,
							unsigned int windowType)
{
	int i;
	char caption[50] {};
	GraphicsDriver = driver;
	(void)togglefullscreen; /* satisfy compiler (unused parameter) */
	(void)windowType;		/* satisfy compiler (unused parameter) */

	fmt::format_to_sz_n(caption, "The Ur-Quan Masters v{}.{}.{} {}",
						UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
						(resFactor ? "HD " UQM_EXTRA_VERSION : UQM_EXTRA_VERSION));

	if (window == nullptr)
	{
		window = SDL_CreateWindow(caption, width, height, 0);
		if (!window)
		{
			return -1;
		}
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		/* If we create the window fullscreen, it will have
		 * no icon if and when it becomes windowed. */
		SetFullscreen(flags, width, height);

		renderer = SDL_CreateRenderer(window, FindBestRenderDriver());
		if (!renderer)
		{
			return -1;
		}
		if (const char* name {SDL_GetRendererName(renderer)})
		{
			uqm::log::info("SDL renderer '{}' selected.", name);
		}
		else
		{
			uqm::log::info("SDL renderer had no name.");
		}
		SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
		for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		{
			SDL2_Screens[i].scaled = nullptr;
			SDL2_Screens[i].texture = nullptr;
			SDL2_Screens[i].dirty = true;
			SDL2_Screens[i].active = true;
			if (0 != ReInit_Screen(&SDL_Screens[i], CanvasWidth, CanvasHeight))
			{
				return -1;
			}
		}
		if (testFlag(flags, uqm::GfxFlags::ShowFPS))
		{
			if (0 != ReInit_FPS_Screen(&SDL_Screen_fps, CanvasWidth >> 4, CanvasHeight >> 4))
			{
				return -1;
			}
		}
		else
		{
			UnInit_Screen(&SDL_Screen_fps);
		}
		SDL2_Screens[1].active = false;
		SDL_Screen = SDL_Screens[0];
		TransitionScreen = SDL_Screens[2];
		format_conv_surf = TFB_CreateSurface(0, 0,
											 32, R_MASK, G_MASK, B_MASK, A_MASK);
		if (!format_conv_surf)
		{
			return -1;
		}
	}
	else
	{
		int LastScreenWidth, LastScreenHeight;
		SDL_RendererLogicalPresentation mode;
		SDL_GetRenderLogicalPresentation(renderer, &LastScreenWidth, &LastScreenHeight, &mode);
		if (LastScreenWidth != width || LastScreenHeight != height)
		{
			SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
		}
		if (SDL_Screens[0]->h != CanvasHeight)
		{
			for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
			{
				SDL2_Screens[i].dirty = true;
				if (0 != ReInit_Screen(&SDL_Screens[i], CanvasWidth, CanvasHeight))
				{
					return -1;
				}
			}
			SDL_Screen = SDL_Screens[0];
			TransitionScreen = SDL_Screens[2];
		}
		if (testFlag(flags, uqm::GfxFlags::ShowFPS))
		{
			if (0 != ReInit_FPS_Screen(&SDL_Screen_fps, CanvasWidth >> 4, CanvasHeight >> 4))
			{
				return -1;
			}
		}
		else
		{
			UnInit_Screen(&SDL_Screen_fps);
		}
		SetFullscreen(flags, width, height);
		if (!testAnyFlag(flags, uqm::GfxFlagsFullscreen))
		{
			SDL_SetWindowSize(window, width, height);
		}
		SDL_SetWindowTitle(window, caption);
	}

	if (testFlag(g_gfxFlags, uqm::GfxFlagsScaleSoftwareOnly))
	{
		for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		{
			if (!SDL2_Screens[i].active)
			{
				continue;
			}
			if (0 != ReInit_Screen(&SDL2_Screens[i].scaled, CanvasWidth * 2, CanvasHeight * 2))
			{
				return -1;
			}
			if (SDL2_Screens[i].texture)
			{
				SDL_DestroyTexture(SDL2_Screens[i].texture);
				SDL2_Screens[i].texture = nullptr;
			}
			SDL2_Screens[i].texture = CreateScreenTexture(CanvasWidth * 2, CanvasHeight * 2);
			SDL_LockSurface(SDL2_Screens[i].scaled);
			SDL_UpdateTexture(SDL2_Screens[i].texture, nullptr,
							  SDL2_Screens[i].scaled->pixels,
							  SDL2_Screens[i].scaled->pitch);
			SDL_UnlockSurface(SDL2_Screens[i].scaled);
		}
		if (testFlag(flags, uqm::GfxFlags::ShowFPS))
		{
			if (0 != ReInit_FPS_Screen(&SDL_Screen_fps, CanvasWidth * 2, CanvasHeight * 2))
			{
				return -1;
			}
		}
		else
		{
			UnInit_Screen(&SDL_Screen_fps);
		}
		scaler = Scale_PrepPlatform(flags, SDL_GetPixelFormatDetails(SDL2_Screens[0].scaled->format));
		graphics_backend = &sdl2_scaled_backend;
	}
	else
	{
		for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		{
			if (SDL2_Screens[i].scaled)
			{
				SDL_DestroySurface(SDL2_Screens[i].scaled);
				SDL2_Screens[i].scaled = nullptr;
			}
			if (SDL2_Screens[i].texture)
			{
				SDL_DestroyTexture(SDL2_Screens[i].texture);
				SDL2_Screens[i].texture = nullptr;
			}
			SDL2_Screens[i].texture = CreateScreenTexture(CanvasWidth, CanvasHeight);
			SDL_LockSurface(SDL_Screens[i]);
			SDL_UpdateTexture(SDL2_Screens[i].texture, nullptr,
							  SDL_Screens[i]->pixels, SDL_Screens[i]->pitch);
			SDL_UnlockSurface(SDL_Screens[i]);
		}
		scaler = nullptr;
		graphics_backend = &sdl2_unscaled_backend;
	}

	/* We succeeded, so alter the screen size to our new sizes */
	WindowWidth = width;
	WindowHeight = height;

	return 0;
}

void TFB_Pure_GetRendererBackends(uqstl::vector<uqstl::string>& backends)
{
	const int numDrivers = SDL_GetNumRenderDrivers();

	for (int i {0}; i < numDrivers; ++i)
	{
		if (const char* name {SDL_GetRenderDriver(i)})
		{
			backends.emplace_back(name);
		}
	}
}

int TFB_Pure_InitGraphics(uqm::GfxDriver driver, uqm::GfxFlags flags, const char* renderer,
						  int width, int height, unsigned int resFactor,
						  unsigned int windowType)
{
	uqm::log::info("Initializing SDL.");
	uqm::log::info("SDL initialized.");
	uqm::log::info("Initializing Screen.");

	(void)windowType;

	rendererBackend = renderer;

	if (TFB_Pure_ConfigureVideo(driver, flags, width, height, 0,
								resFactor, windowType))
	{
		uqm::log::critical("Could not initialize video: {}",
						   SDL_GetError());
		exit(EXIT_FAILURE);
	}

	/* Initialize scalers (let them precompute whatever) */
	Scale_Init();

	return 0;
}

void TFB_Pure_UninitGraphics(void)
{
	if (renderer)
	{
		SDL_DestroyRenderer(renderer);
	}
	if (window)
	{
		SDL_DestroyWindow(window);
	}
}

static void
TFB_SDL2_UploadTransitionScreen(void)
{
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.x = 0;
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.y = 0;
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.w = CanvasWidth;
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.h = CanvasHeight;
	SDL2_Screens[TFB_SCREEN_TRANSITION].dirty = true;
}

static void
TFB_SDL2_UpdateTexture(SDL_Texture* dest, SDL_Surface* src, SDL_Rect* rect)
{
	char* srcBytes;
	SDL_LockSurface(src);
	srcBytes = (char*)src->pixels;
	if (rect)
	{
		/* SDL2 screen surfaces are always 32bpp */
		srcBytes += (src->pitch * rect->y) + (rect->x * 4);
	}
	/* 2020-08-02: At time of writing, the documentation for
	 * SDL_UpdateTexture states this: "If the texture is intended to be
	 * updated often, it is preferred to create the texture as streaming
	 * and use [SDL_LockTexture and SDL_UnlockTexture]." Unfortunately,
	 * SDL_LockTexture will corrupt driver-space memory in the 32-bit
	 * Direct3D 9 driver on Intel Integrated graphics chips, resulting
	 * in an immediate crash with no detectable errors from the API up
	 * to that point.
	 *
	 * We also cannot simply forbid the Direct3D driver outright, because
	 * pre-Windows 10 machines appear to fail to initialize D3D11 even
	 * while claiming to support it.
	 *
	 * These bugs may be fixed in the future, but in the meantime we
	 * rely on this allegedly slower but definitely more reliable
	 * function. */
	SDL_UpdateTexture(dest, rect, srcBytes, src->pitch);
	SDL_UnlockSurface(src);
}

static void
TFB_SDL2_ScanLines(bool hd)
{
	int y;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 64);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	for (y = 0; y < WindowHeight; y += 2)
	{
		SDL_RenderLine(renderer, 0.0f, static_cast<float>(y), static_cast<float>(WindowWidth - 1), static_cast<float>(y));
	}

	(void)hd; // satisfy compiler (unused parameter)
}

static void
TFB_SDL2_FPS(void)
{
	SDL_Texture* texture;
	SDL_Rect r, t;
	r.x = r.y = t.x = t.y = 0;
	r.w = CanvasWidth >> 4;
	r.h = CanvasHeight >> 4;
	t.w = WindowWidth >> 4;
	t.h = WindowHeight >> 4;
	texture = SDL_CreateTextureFromSurface(renderer, SDL_Screen_fps);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	const SDL_FRect src {ToFRect(r)};
	const SDL_FRect dst {ToFRect(t)};
	SDL_RenderTexture(renderer, texture, &src, &dst);
	SDL_DestroyTexture(texture);
	texture = nullptr;
}

static void
TFB_SDL2_Preprocess(uqm::TFBRedraw force_full_redraw, int transition_amount,
					int fade_amount)
{
	(void)transition_amount;
	(void)fade_amount;

	if (force_full_redraw == uqm::TFBRedraw::Yes)
	{
		SDL2_Screens[TFB_SCREEN_MAIN].updated.x = 0;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.y = 0;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.w = CanvasWidth;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.h = CanvasHeight;
		SDL2_Screens[TFB_SCREEN_MAIN].dirty = true;
	}
	else if (TFB_BBox.valid)
	{
		SDL2_Screens[TFB_SCREEN_MAIN].updated.x = TFB_BBox.region.corner.x;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.y = TFB_BBox.region.corner.y;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.w =
			TFB_BBox.region.extent.width;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.h =
			TFB_BBox.region.extent.height;
		SDL2_Screens[TFB_SCREEN_MAIN].dirty = true;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
}

static void
TFB_SDL2_Unscaled_ScreenLayer(SCREEN screen, Uint8 a, SDL_Rect* rect)
{
	SDL_Texture* texture = SDL2_Screens[screen].texture;
	if (SDL2_Screens[screen].dirty)
	{
		TFB_SDL2_UpdateTexture(texture, SDL_Screens[screen],
							   &SDL2_Screens[screen].updated);
	}
	if (a == 255)
	{
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
	}
	else
	{
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(texture, a);
	}

	if (rect)
	{
		const SDL_FRect frect {ToFRect(*rect)};
		SDL_RenderTexture(renderer, texture, &frect, &frect);
	}
	else
	{
		SDL_RenderTexture(renderer, texture, nullptr, nullptr);
	}
}

static void
TFB_SDL2_Scaled_ScreenLayer(SCREEN screen, Uint8 a, SDL_Rect* rect)
{
	SDL_Texture* texture = SDL2_Screens[screen].texture;
	SDL_FRect srcRect, dstRect;
	SDL_FRect* pSrcRect = nullptr;
	SDL_FRect* pDstRect = nullptr;
	if (SDL2_Screens[screen].dirty)
	{
		SDL_Surface* src = SDL2_Screens[screen].scaled;
		SDL_Rect scaled_update = SDL2_Screens[screen].updated;
		scaler(SDL_Screens[screen], src, &SDL2_Screens[screen].updated);
		scaled_update.x *= 2;
		scaled_update.y *= 2;
		scaled_update.w *= 2;
		scaled_update.h *= 2;
		TFB_SDL2_UpdateTexture(texture, src, &scaled_update);
	}
	if (a == 255)
	{
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
	}
	else
	{
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(texture, a);
	}
	/* The texture has twice the resolution when scaled, but the
	 * screen's logical resolution has not changed, so the clip
	 * rectangle does not need to be scaled. The *source* clip
	 * rect, however, must be scaled to match. */
	if (rect)
	{
		dstRect = ToFRect(*rect);
		srcRect = dstRect;
		srcRect.x *= 2;
		srcRect.y *= 2;
		srcRect.w *= 2;
		srcRect.h *= 2;
		pSrcRect = &srcRect;
		pDstRect = &dstRect;
	}
	SDL_RenderTexture(renderer, texture, pSrcRect, pDstRect);
}

static void
FillRect(const SDL_Rect* rect)
{
	if (rect)
	{
		const SDL_FRect frect {ToFRect(*rect)};
		SDL_RenderFillRect(renderer, &frect);
	}
	else
	{
		SDL_RenderFillRect(renderer, nullptr);
	}
}

static void
TFB_SDL2_ColorLayer(Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect* rect)
{
	if (r == 0 || r == 255)
	{
		SDL_SetRenderDrawBlendMode(renderer, a == 255 ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);

		SDL_SetRenderDrawColor(renderer, r, g, b, a);
		FillRect(rect);
	}
	else
	{
		if (g == 0)
		{
			SDL_SetRenderDrawBlendMode(renderer,
									   SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,
																  SDL_BLENDFACTOR_ONE, (SDL_BlendOperation)r, SDL_BLENDFACTOR_ONE,
																  SDL_BLENDFACTOR_ONE,
																  SDL_BLENDOPERATION_MAXIMUM));

			SDL_SetRenderDrawColor(renderer, a, a, a, 255);
			FillRect(rect);
		}
		else
		{
			Uint8 c = a > 175 ? 175 : a;
			SDL_SetRenderDrawBlendMode(renderer,
									   SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,
																  SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
																  SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE,
																  SDL_BLENDOPERATION_MAXIMUM));
			SDL_SetRenderDrawColor(renderer, c, c, c, 255);
			FillRect(rect);

			SDL_SetRenderDrawBlendMode(renderer,
									   SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,
																  SDL_BLENDFACTOR_ONE,
																  SDL_BLENDOPERATION_REV_SUBTRACT,
																  SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE,
																  SDL_BLENDOPERATION_MAXIMUM));
			SDL_SetRenderDrawColor(renderer, a, a, a, 255);
			FillRect(rect);
		}
	}
}

static void
TFB_SDL2_Postprocess(bool hd)
{
	if (testFlag(g_gfxFlags, uqm::GfxFlags::Scanlines))
	{
		TFB_SDL2_ScanLines(hd);
	}

	if (testFlag(g_gfxFlags, uqm::GfxFlags::ShowFPS))
	{
		TFB_SDL2_FPS();
	}

	SDL_RenderPresent(renderer);
}

bool TFB_SDL2_GammaCorrection(float gamma)
{
	// SDL3 has no window gamma.
	(void)gamma;
	return false;
}

bool TFB_SDL_ScreenShot(const char* path)
{
	bool successful = false;

	SDL_Surface* rendered {SDL_RenderReadPixels(renderer, nullptr)};
	if (!rendered)
	{
		uqm::log::error("Cannot read the screen for the screenshot: {}", SDL_GetError());
		return false;
	}
	SDL_Surface* tmp {SDL_ConvertSurface(rendered, SDL_PIXELFORMAT_RGBA32)};
	SDL_DestroySurface(rendered);
	if (!tmp)
	{
		uqm::log::error("Cannot convert the screenshot: {}", SDL_GetError());
		return false;
	}

	SDL_LockSurface(tmp);
	if (SDL_IOStream* file {SDL_IOFromFile(path, "wb")}; file == nullptr)
	{
		uqm::log::error("Cannot open '{}' for the screenshot: {}", path, SDL_GetError());
	}
	else if (TFB_sdl_to_png(tmp, file, 1) == 0)
	{
		successful = true;
	}

	if (successful && CopySurfaceToClipboard(tmp) != 0)
	{
		uqm::log::warn("Failed to copy PNG to clipboard\n");
	}

	SDL_UnlockSurface(tmp);
	SDL_DestroySurface(tmp);

	return successful;
}

SDL_Window* TFB_GetSDLWindow(void)
{
	return window;
}

void TFB_SDL2_GetDisplaySize(SDL_Rect* bounds)
{
	if (!SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), bounds))
	{
		fmt::print("{}\n", SDL_GetError());
	}
}
