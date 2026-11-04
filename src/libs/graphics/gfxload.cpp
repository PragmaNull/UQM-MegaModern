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

#include <string.h>
#include <stdio.h>
#include <scn/scan.h>

#include "options.h"
#include "port.h"
#include "uqm/units.h"
#include "libs/reslib.h"
#include "libs/io/Path.h"
#include "core/log/log.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/font.h"
#include "libs/graphics/anifile.h"
#include "libs/graphics/fontfile.h"
#include "libs/file/textfile.h"
#include <fmt/format.h>


static void
process_image(FRAME FramePtr, TFB_Canvas img[], const uqm::AniFrame* ani, int cel_ct)
{
	FramePtr->Type = ROM_DRAWABLE;
	FramePtr->Index = cel_ct;

	// handle transparency cases
	if (TFB_DrawCanvas_IsPaletted(img[cel_ct]))
	{ // indexed color image
		if (ani[cel_ct].transparentColor >= 0)
		{
			TFB_DrawCanvas_SetTransparentIndex(img[cel_ct],
											   ani[cel_ct].transparentColor, false);
		}
	}
	else
	{ // special transparency cases for truecolor images
		if (ani[cel_ct].transparentColor == 0)
		{ // make RGB=0,0,0 transparent
			const Color color = {0, 0, 0, 0};
			TFB_DrawCanvas_SetTransparentColor(img[cel_ct], color, false);
		}
	}
	if (ani[cel_ct].transparentColor == uqm::c_aniTransparencyNone)
	{
		TFB_DrawCanvas_SetTransparentIndex(img[cel_ct], -1, false);
	}

	const int hx {ani[cel_ct].hotspotX};
	const int hy {ani[cel_ct].hotspotY};

	FramePtr->image = TFB_DrawImage_New(img[cel_ct]);

	TFB_Image* tfbimg {FramePtr->image};
	tfbimg->colormap_index = ani[cel_ct].colormapIndex;
	img[cel_ct] = tfbimg->NormalImg;

	FramePtr->HotSpot = MAKE_HOT_SPOT(hx, hy);
	SetFrameBounds(FramePtr, tfbimg->extent.width, tfbimg->extent.height);

#ifdef CLIPDEBUG
	{
		/* for debugging clipping:
		   draws white (or most matching color from palette) pixels to
	       every corner of the image
		 */
		Color color = {0xff, 0xff, 0xff, 0xff};
		GFXRECT r = {
			{0, 0},
			{1, 1}
		  };
		if (tfbimg->extent.width > 2 && tfbimg->extent.height > 2)
		{
			TFB_DrawImage_Rect(&r, color, tfbimg);
			r.corner.x = tfbimg->extent.width - 1;
			TFB_DrawImage_Rect(&r, color, tfbimg);
			r.corner.y = tfbimg->extent.height - 1;
			TFB_DrawImage_Rect(&r, color, tfbimg);
			r.corner.x = 0;
			TFB_DrawImage_Rect(&r, color, tfbimg);
		}
	}
#endif
}

static void
processFontChar(TFB_Char* CharPtr, TFB_Canvas canvas, FONT fontPtr)
{
	size_t dpitch {};

	TFB_DrawCanvas_GetExtent(canvas, &CharPtr->extent);

	// Currently, each font char has its own separate data
	// but that can change to common mem area
	dpitch = CharPtr->extent.width;
	uint8_t* newdata {new uint8_t[dpitch * CharPtr->extent.height]};
	TFB_DrawCanvas_GetFontCharData(canvas, newdata, dpitch);

	CharPtr->data = newdata;
	CharPtr->pitch = dpitch;
	CharPtr->disp.width = CharPtr->extent.width;
	CharPtr->disp.height = CharPtr->extent.height;
	CharPtr->HotSpot.x = 0;

	{
		// This tunes the font positioning to be about what it should
		// TODO: prolly needs a little tweaking still

		int tune_amount = 0;

		if (CharPtr->disp.height <= RES_SCALE(8))
		{
			tune_amount = RES_SCALE(1);
		}
		else if (CharPtr->disp.height == RES_SCALE(9))
		{
			tune_amount = RES_SCALE(2);
		}
		else if (CharPtr->disp.height > RES_SCALE(9))
		{
			tune_amount = RES_SCALE(3);
		}

		if (fontPtr->HaveFntData)
		{
			tune_amount = fontPtr->VertAlign;
		}

		CharPtr->HotSpot.y = CharPtr->disp.height - tune_amount;
	}
}

void* _GetCelData(const ResourceFile& resource)
{
	const auto text {readResourceText(resource)};
	auto frames {text ? uqm::aniFromJson(*text) : uqstl::nullopt};
	if (!frames || frames->empty())
	{
		uqm::log::warn("Couldn't get cel data for '{}'", resource.path);
		return nullptr;
	}

	const uqstl::string_view parent {uqm::io::path::parent(resource.path)};
	const auto count {static_cast<int>(frames->size())};
	uqstl::vector<TFB_Canvas> img(frames->size());
	for (int i {0}; i < count; ++i)
	{
		const uqstl::string path {uqm::io::path::join(parent, (*frames)[i].image)};
		img[i] = TFB_DrawCanvas_LoadFromFile(resource.dir, path.c_str());
		if (img[i] == nullptr)
		{
			const char* const err {TFB_DrawCanvas_GetError()};
			uqm::log::warn("_GetCelData: Unable to load image '{}' for frame {} of '{}'.{}", path, i, resource.path,
						   err != nullptr ? fmt::format(" Gfx Driver reports: {}", err) : "");
			// An empty frame keeps the index of each later frame.
			img[i] = TFB_DrawCanvas_New_TrueColor(1, 1, true);
			(*frames)[i] = uqm::AniFrame {};
		}
	}

	const DRAWABLE Drawable {AllocDrawable(count)};
	if (Drawable == nullptr)
	{
		uqm::log::warn("Couldn't get cel data for '{}'", resource.path);
		return nullptr;
	}
	Drawable->Flags = WANT_PIXMAP;
	Drawable->MaxIndex = count - 1;
	for (int i {count - 1}; i >= 0; --i)
	{
		process_image(Drawable->Frame.data() + i, img.data(), frames->data(), i);
	}
	return Drawable;
}

bool _ReleaseCelData(void* handle)
{
	auto* DrawablePtr {static_cast<DRAWABLE>(handle)};
	if (DrawablePtr == nullptr)
	{
		return (false);
	}

	for (FRAME_DESC& frame : DrawablePtr->Frame)
	{
		if (TFB_Image * img {frame.image})
		{
			frame.image = nullptr;
			TFB_DrawScreen_DeleteImage(img);
		}
	}
	delete DrawablePtr;

	return (true);
}

typedef struct BuildCharDesc
{
	TFB_Canvas canvas;
	uqm::UniChar index;
} BuildCharDesc;

void* _GetFontData(const uqm::io::Directory& dir, uqstl::string_view path)
{
	uqstl::vector<uqstl::string> fontDir {};
	uqstl::vector<BuildCharDesc> bcds {};
	size_t numBCDs = 0;
	uqstl::optional<uqm::io::Directory> fontDirHandle {};
	FONT fontPtr = nullptr;
	const char* cfg_name = "kerndat.fnt.json";

	if (const uqstl::optional<uqm::io::FileInfo> info {dir.stat(path)}; info && !info->isDirectory())
	{
		if (uqm::io::Result<uqm::io::Directory> archive {uqm::gameFileSystem().openArchive(dir, path)})
		{
			fontDirHandle = uqstl::move(*archive);
		}
		else
		{
			uqm::log::warn("Could not open font archive '{}': {}", path, archive.error().message());
		}
	}
	else
	{
		fontDirHandle = dir.subdir(path);
	}
	if (!fontDirHandle)
	{
		goto err;
	}

	fontDir = uqm::listFiles(*fontDirHandle, "", uqm::io::Matcher {uqm::io::Matcher::Kind::Substring, "."});
	if (fontDir.empty())
	{
		goto err;
	}

	bcds.resize(fontDir.size());

	// Load the surfaces for all dir Entries
	for (int dirEntryI {0}; dirEntryI < static_cast<int>(fontDir.size()); dirEntryI++)
	{
		unsigned int charIndex {};
		EXTENT size;

		const char* char_name = fontDir[dirEntryI].c_str();

		if (strcmp(char_name, cfg_name) == 0)
		{
			continue;
		}

		if (const auto result {scn::scan<unsigned int>(uqstl::string_view {char_name}, "{:x}.")})
		{
			charIndex = result->value();
		}
		else
		{
			continue;
		}

		if (charIndex > MAX_UNICODE)
		{
			continue;
		}

		TFB_Canvas canvas {TFB_DrawCanvas_LoadFromFile(*fontDirHandle, char_name)};
		if (canvas == nullptr)
		{
			continue;
		}

		TFB_DrawCanvas_GetExtent(canvas, &size);
		if (size.width == 0 || size.height == 0)
		{
			TFB_DrawCanvas_Delete(canvas);
			continue;
		}

		bcds[numBCDs].canvas = canvas;
		bcds[numBCDs].index = charIndex;
		numBCDs++;
	}

	fontPtr = new FONT_DESC {};
	if (fontPtr == nullptr)
	{
		goto err;
	}

	if (const auto text {uqm::readTextFile(*fontDirHandle, cfg_name)})
	{
		memset(&fontPtr->KernTab, (uint8_t)~0, sizeof(fontPtr->KernTab));
		if (const auto metrics {uqm::fontMetricsFromJson(*text)})
		{
			fontPtr->Leading = metrics->leading;
			fontPtr->CharSpace = metrics->charSpace;
			fontPtr->KernAmount = metrics->kernAmount;
			fontPtr->VertAlign = metrics->vertAlign;
			fontPtr->HaveFntData = true;
			for (const uqm::FontKern& kern : metrics->kerning)
			{
				if (kern.ch < MAX_UNICODE)
				{
					fontPtr->KernTab[kern.ch] = static_cast<uint8_t>((kern.left << 2) | kern.right);
				}
			}
		}
		else
		{
			uqm::log::warn("Could not read '{}' of font '{}'.", cfg_name, path);
		}
	}

	fontDirHandle.reset();

#if 0
	if (numBCDs == 0)
		goto err;
#endif

	// sort on the character index
	uqstl::sort(bcds.begin(), bcds.begin() + numBCDs, [](const BuildCharDesc& a, const BuildCharDesc& b) {
		return a.index < b.index;
	});

	{
		size_t startBCD = 0;
		uqm::UniChar pageStart {};
		FONT_PAGE** pageEndPtr = &fontPtr->fontPages;
		while (startBCD < numBCDs)
		{
			// Process one character page.
			pageStart = bcds[startBCD].index & CHARACTER_PAGE_MASK;

			size_t endBCD {startBCD};
			while (endBCD < numBCDs && (bcds[endBCD].index & CHARACTER_PAGE_MASK) == pageStart)
			{
				endBCD++;
			}

			{
				const int numChars = bcds[endBCD - 1].index + 1
								   - bcds[startBCD].index;
				FONT_PAGE* page = AllocFontPage(numChars);
				page->pageStart = pageStart;
				page->firstChar = bcds[startBCD].index;
				page->numChars = numChars;
				*pageEndPtr = page;
				pageEndPtr = &page->next;

				for (size_t bcdI {startBCD}; bcdI < endBCD; bcdI++)
				{
					// Process one character.
					BuildCharDesc* bcd = &bcds[bcdI];
					TFB_Char* destChar =
						&page->charDesc[bcd->index - page->firstChar];

					if (destChar->data != nullptr)
					{
						// There's already an image for this character.
						uqm::log::debug("Duplicate image for character {} "
										"for font {}.",
										(int)bcd->index,
										path);
						TFB_DrawCanvas_Delete(bcd->canvas);
						continue;
					}

					processFontChar(destChar, bcd->canvas, fontPtr);
					TFB_DrawCanvas_Delete(bcd->canvas);

					if (destChar->disp.height > fontPtr->disp.height)
					{
						fontPtr->disp.height = destChar->disp.height;
					}
					if (destChar->disp.width > fontPtr->disp.width)
					{
						fontPtr->disp.width = destChar->disp.width;
					}
				}
			}

			startBCD = endBCD;
		}
		*pageEndPtr = nullptr;
	}

	if (!fontPtr->HaveFntData)
	{
		fontPtr->Leading = fontPtr->disp.height + RES_SCALE(1);
		fontPtr->CharSpace = RES_SCALE(1);
	}

	return fontPtr;

err:
	delete fontPtr;

	for (size_t bcdI {}; bcdI < numBCDs; bcdI++)
	{
		TFB_DrawCanvas_Delete(bcds[bcdI].canvas);
	}

	fontDirHandle.reset();


	return 0;
}

bool _ReleaseFontData(void* handle)
{
	FONT font = (FONT)handle;
	if (font == nullptr)
	{
		return false;
	}

	{
		FONT_PAGE* nextPage {};

		for (FONT_PAGE* page {font->fontPages}; page != nullptr; page = nextPage)
		{
			for (size_t charI {0}; charI < page->numChars; charI++)
			{
				const TFB_Char* c = &page->charDesc[charI];

				if (c->data == nullptr)
				{
					continue;
				}

				// XXX: fix this if fonts get per-page data
				//  rather than per-char
				TFB_DrawScreen_DeleteData(c->data);
			}

			nextPage = page->next;
			FreeFontPage(page);
		}
	}

	delete font;

	return true;
}
