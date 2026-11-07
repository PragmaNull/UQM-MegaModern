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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This code is adapted from IMG_png.c in the SDL_image library, which is
 * available under the zlib license and is (c) 1997-2019 Sam Lantinga. The
 * code itself is credited to Philippe Lavoie as the original author. It
 * also shares some heritage with libpng's own example.c file, and
 * ultimately inherits the GPL2+ license from the rest of UQM.
 *
 * Differences from SDL_image are:
 *  - It is PNG-only
 *  - It directly links the relevant version of libpng at compile time
 *  - It always uses libpng and will never forward to alternative
 *    libraries such as ImageIO.framework. This means that palette
 *    information will always be preserved, as UQM requires.
 *  - It locks the surface as the API demands rather than using
 *    volatility markers
 *  - Palette assignment is done through the API rather than by
 *    directly editing the format contents
 */

#include "png2sdl.h"

#include <fmt/base.h>
#include <png.h>

#include "core/log/log.h"
#include "core/stl/array.h"
#include "core/stl/stl.h"
#include "core/stl/vector.h"
#include "sdl_common.h"
#include "sdl_ptr.h"

/* Link function between SDL_IOStream and PNG's data source */
static void png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
	SDL_IOStream* src = (SDL_IOStream*)png_get_io_ptr(ctx);
	SDL_ReadIO(src, area, size);
}

namespace
{

// Owns a libpng read or write struct and its info struct.
template <bool Write>
class [[nodiscard]] PngStructs
{
public:
	PngStructs()
		: m_png {Write ? png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr) : png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)}
	{
		if (m_png != nullptr)
		{
			m_info = png_create_info_struct(m_png);
		}
	}

	~PngStructs()
	{
		if (m_png == nullptr)
		{
			return;
		}
		if constexpr (Write)
		{
			png_destroy_write_struct(&m_png, m_info != nullptr ? &m_info : nullptr);
		}
		else
		{
			png_destroy_read_struct(&m_png, m_info != nullptr ? &m_info : nullptr, nullptr);
		}
	}

	PngStructs(const PngStructs&) = delete;
	PngStructs& operator=(const PngStructs&) = delete;

	[[nodiscard]] png_structp png() const
	{
		return m_png;
	}

	[[nodiscard]] png_infop info() const
	{
		return m_info;
	}

private:
	png_structp m_png {};
	png_infop m_info {};
};

// The image data that the read needs after the header.
struct PngHeader
{
	png_uint_32 width;
	png_uint_32 height;
	int bitDepth;
	int colorType;
	int numChannels;
	int colorKey;
	png_color_16* transparent;
};

// These functions call setjmp. Their local variables must have trivial destructors.

// Reads the header and sets up the decode.
bool readPngHeader(png_structp png, png_infop info, SDL_IOStream* src, PngHeader& header)
{
	if (setjmp(png_jmpbuf(png)))
	{
		return false;
	}

	png_set_read_fn(png, src, png_read_data);

	png_uint_32 width {};
	png_uint_32 height {};
	int bitDepth {};
	int colorType {};
	int interlaceType {};
	png_read_info(png, info);
	png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

	/* Configure the decode based on what we know of the image
	 * already: strip 16 bit color down to 8 bit, automatically
	 * deinterlace, expand grayscale images or those with more
	 * than one transparent color or any translucent colors into
	 * full RGB or RGBA, and expand 1, 2, or 4-bpp paletted
	 * images to 8bpp. */
	png_set_strip_16(png);
	png_set_interlace_handling(png);
	png_set_packing(png);
	if (colorType == PNG_COLOR_TYPE_GRAY)
	{
		png_set_expand(png);
	}
	int colorKey {-1};
	png_color_16* transparent {};
	if (png_get_valid(png, info, PNG_INFO_tRNS))
	{
		int numTrans {};
		Uint8* trans {};
		png_get_tRNS(png, info, &trans, &numTrans, &transparent);
		if (colorType == PNG_COLOR_TYPE_PALETTE)
		{
			/* Check if all tRNS entries are opaque except one */
			int j {};
			int t {-1};
			for (j = 0; j < numTrans; j++)
			{
				if (trans[j] == 0)
				{
					if (t >= 0)
					{
						break;
					}
					t = j;
				}
				else if (trans[j] != 255)
				{
					break;
				}
			}
			if (j == numTrans)
			{
				/* exactly one transparent index */
				colorKey = t;
			}
			else
			{
				/* more than one transparent index, or translucency */
				png_set_expand(png);
			}
		}
		else
		{
			colorKey = 0; /* actual value will be set later */
		}
	}

	if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(png);
	}

	/* Register our changes with the reading machinery and refresh
	 * our ancillary data about the image */
	png_read_update_info(png, info);
	png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

	header = PngHeader {width, height, bitDepth, colorType, png_get_channels(png, info), colorKey, transparent};
	return true;
}

bool readPngRows(png_structp png, png_bytep* rows)
{
	if (setjmp(png_jmpbuf(png)))
	{
		return false;
	}
	png_read_image(png, rows);
	return true;
}

} // namespace

SDL_Surface* TFB_png_to_sdl(SDL_IOStream* src)
{
	if (!src)
	{
		/* The error message has been set in SDL_IOFromFile */
		return nullptr;
	}
	const Sint64 start {SDL_TellIO(src)};

	const auto fail {[&](uqgsl::czstring error) -> SDL_Surface* {
		SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
		uqm::log::error("{}", error);
		return nullptr;
	}};

	const PngStructs<false> structs {};
	if (structs.png() == nullptr)
	{
		return fail("Couldn't allocate memory for PNG file");
	}
	if (structs.info() == nullptr)
	{
		return fail("Couldn't create image information for PNG file");
	}

	PngHeader header {};
	if (!readPngHeader(structs.png(), structs.info(), src, header))
	{
		return fail("Error reading the PNG file.");
	}

	/* Allocate the SDL surface to hold the image */
	Uint32 Rmask {};
	Uint32 Gmask {};
	Uint32 Bmask {};
	Uint32 Amask {};
	if (header.numChannels >= 3)
	{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		Rmask = 0x000000FF;
		Gmask = 0x0000FF00;
		Bmask = 0x00FF0000;
		Amask = (header.numChannels == 4) ? 0xFF000000 : 0;
#else
		const int s {(header.numChannels == 4) ? 0 : 8};
		Rmask = 0xFF000000 >> s;
		Gmask = 0x00FF0000 >> s;
		Bmask = 0x0000FF00 >> s;
		Amask = 0x000000FF >> s;
#endif
	}
	uqm::SurfacePtr surface {TFB_CreateSurface(header.width, header.height, header.bitDepth * header.numChannels, Rmask,
											   Gmask, Bmask, Amask)};
	if (!surface)
	{
		return fail(SDL_GetError());
	}

	if (header.colorKey != -1)
	{
		int colorKey {header.colorKey};
		if (header.colorType != PNG_COLOR_TYPE_PALETTE)
		{
			/* FIXME: Should these be truncated or shifted down? */
			colorKey = SDL_MapSurfaceRGB(surface.get(), static_cast<Uint8>(header.transparent->red),
										 static_cast<Uint8>(header.transparent->green),
										 static_cast<Uint8>(header.transparent->blue));
		}
		SDL_SetSurfaceColorKey(surface.get(), true, colorKey);
	}

	SDL_LockSurface(surface.get());
	/* Create the array of pointers to image data */
	uqstl::vector<png_bytep> rows(header.height);
	for (png_uint_32 row {0}; row < header.height; row++)
	{
		rows[row] = static_cast<png_bytep>(surface->pixels) + row * surface->pitch;
	}

	/* Read the entire image in one go */
	const bool rowsRead {readPngRows(structs.png(), rows.data())};
	SDL_UnlockSurface(surface.get());
	if (!rowsRead)
	{
		return fail("Error reading the PNG file.");
	}

	/* and we're done!  (png_read_end() can be omitted if no
	 * processing of post-IDAT text/time/etc. is desired)
	 * In some cases it can't read PNGs created by some popular
	 * programs (ACDSEE), we do not want to process comments, so
	 * we omit png_read_end */

	/* Load the palette, if any */
	if (SDL_GetSurfacePalette(surface.get()))
	{
		uqstl::array<SDL_Color, 256> palette {};
		int png_num_palette {};
		png_colorp png_palette {};
		png_get_PLTE(structs.png(), structs.info(), &png_palette, &png_num_palette);
		if (header.colorType == PNG_COLOR_TYPE_GRAY)
		{
			png_num_palette = 256;
			for (int i {0}; i < 256; i++)
			{
				palette[i].r = static_cast<Uint8>(i);
				palette[i].g = static_cast<Uint8>(i);
				palette[i].b = static_cast<Uint8>(i);
			}
		}
		else if (png_num_palette > 0)
		{
			for (int i {0}; i < png_num_palette; ++i)
			{
				palette[i].b = png_palette[i].blue;
				palette[i].g = png_palette[i].green;
				palette[i].r = png_palette[i].red;
			}
		}
		SDL_SetPaletteColors(SDL_GetSurfacePalette(surface.get()), palette.data(), 0, png_num_palette);
	}

	return surface.release();
}

/* SDL to PNG
 * Based entirely on driedfruit's SDL_SavePNG
 * A libpng-based SDL_Surface writer.
 * https://github.com/driedfruit/SDL_SavePNG/
 */

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
static constexpr auto AMask {0xff000000};
static constexpr auto BMask {0x00ff0000};
static constexpr auto GMask {0x0000ff00};
static constexpr auto RMask {0x000000ff};
#else
static constexpr auto AMask {0x000000ff};
static constexpr auto BMask {0x0000ff00};
static constexpr auto GMask {0x00ff0000};
static constexpr auto RMask {0xff000000};
#endif

static void png_write_data(png_structp ctx, png_bytep area, png_size_t size)
{
	SDL_IOStream* rw = (SDL_IOStream*)png_get_io_ptr(ctx);
	SDL_WriteIO(rw, area, size);
}

SDL_Surface* SDL_PNGFormatAlpha(SDL_Surface* src)
{
	SDL_Surface* surf {};
	SDL_Rect rect = {0};

	/* NO-OP for images < 32bpp and 32bpp images that already have Alpha channel */
	if (SDL_GetPixelFormatDetails(src->format)->bits_per_pixel <= 24 || SDL_GetPixelFormatDetails(src->format)->Amask)
	{
		src->refcount++;
		return src;
	}

	/* Convert 32bpp alpha-less image to 24bpp alpha-less image */
	rect.w = src->w;
	rect.h = src->h;
	surf = TFB_CreateSurface(src->w, src->h, 24,
							 SDL_GetPixelFormatDetails(src->format)->Rmask, SDL_GetPixelFormatDetails(src->format)->Gmask, SDL_GetPixelFormatDetails(src->format)->Bmask, 0);
	SDL_BlitSurfaceUnchecked(src, &rect, surf, &rect);

	return surf;
}

namespace
{

// Writes the image. Calls setjmp, so its local variables must have trivial destructors.
bool writePng(png_structp png, png_infop info, SDL_Surface* surface, SDL_IOStream* dst,
			  uqstl::span<const png_color> palette, png_bytep* rows)
{
	if (setjmp(png_jmpbuf(png)))
	{
		return false;
	}

	/* Set up the output control */
	png_set_write_fn(png, dst, png_write_data, nullptr);

	/* Prepare chunks */
	const SDL_PixelFormatDetails* const format {SDL_GetPixelFormatDetails(surface->format)};
	int colortype {PNG_COLOR_MASK_COLOR};
	if (!palette.empty())
	{
		colortype |= PNG_COLOR_MASK_PALETTE;
		png_set_PLTE(png, info, palette.data(), static_cast<int>(palette.size()));
	}
	else if (format->bytes_per_pixel > 3 || format->Amask)
	{
		colortype |= PNG_COLOR_MASK_ALPHA;
	}

	/* Set the PNG header info */
	png_set_IHDR(png, info, surface->w, surface->h, 8, colortype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);

	png_set_packing(png);

	/* Allow BGR surfaces */
	if (format->Rmask == BMask && format->Gmask == GMask && format->Bmask == RMask)
	{
		png_set_bgr(png);
	}

	/* Write everything */
	png_write_info(png, info);
	png_write_image(png, rows);
	png_write_end(png, info);
	return true;
}

} // namespace

int TFB_sdl_to_png(SDL_Surface* surface, SDL_IOStream* dst)
{
	const auto fail {[](uqgsl::czstring error) {
		uqm::log::error("{}", error);
		return -1;
	}};

	/* Initialize and do basic error checking */
	if (!dst)
	{
		return fail("Argument 2 to TFB_sdl_to_png can't be nullptr");
	}
	if (!surface)
	{
		return fail("Argument 1 to TFB_sdl_to_png can't be nullptr");
	}

	const PngStructs<true> structs {};
	if (structs.png() == nullptr)
	{
		return fail("Couldn't allocate memory for PNG file");
	}
	if (structs.info() == nullptr)
	{
		return fail("Couldn't create image information for PNG file");
	}

	uqstl::vector<png_color> palette {};
	const SDL_PixelFormatDetails* const format {SDL_GetPixelFormatDetails(surface->format)};
	if (const SDL_Palette* const pal {SDL_GetSurfacePalette(surface)};
		pal != nullptr && format->bytes_per_pixel > 0 && format->bytes_per_pixel <= 8)
	{
		for (const SDL_Color& color : uqstl::span {pal->colors, static_cast<size_t>(pal->ncolors)})
		{
			palette.push_back({color.r, color.g, color.b});
		}
	}

	/* Create the array of pointers to image data */
	uqstl::vector<png_bytep> rows(static_cast<size_t>(surface->h));
	for (int i {0}; i < surface->h; i++)
	{
		rows[i] = static_cast<png_bytep>(surface->pixels) + i * surface->pitch;
	}

	if (!writePng(structs.png(), structs.info(), surface, dst, palette, rows.data()))
	{
		return fail("Error writing the PNG file.");
	}
	return 0;
}
