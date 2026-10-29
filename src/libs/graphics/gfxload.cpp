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
// for _cur_resfile_name
#include "core/log/log.h"
#include "libs/memlib.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/font.h"
typedef struct anidata
{
	int transparent_color;
	int colormap_index;
	int hotspot_x;
	int hotspot_y;
} AniData;


static void
process_image(FRAME FramePtr, TFB_Canvas img[], AniData* ani, int cel_ct)
{
	TFB_Image* tfbimg;
	int hx, hy;

	FramePtr->Type = ROM_DRAWABLE;
	FramePtr->Index = cel_ct;

	// handle transparency cases
	if (TFB_DrawCanvas_IsPaletted(img[cel_ct]))
	{ // indexed color image
		if (ani[cel_ct].transparent_color >= 0)
		{
			TFB_DrawCanvas_SetTransparentIndex(img[cel_ct],
											   ani[cel_ct].transparent_color, false);
		}
	}
	else
	{ // special transparency cases for truecolor images
		if (ani[cel_ct].transparent_color == 0)
		{ // make RGB=0,0,0 transparent
			Color color = {0, 0, 0, 0};
			TFB_DrawCanvas_SetTransparentColor(img[cel_ct], color, false);
		}
	}
	if (ani[cel_ct].transparent_color == -1)
	{ // enforce -1 to mean 'no transparency'
		TFB_DrawCanvas_SetTransparentIndex(img[cel_ct], -1, false);
		// set transparent_color == -2 to use PNG tRNS transparency
	}

	hx = ani[cel_ct].hotspot_x;
	hy = ani[cel_ct].hotspot_y;

	FramePtr->image = TFB_DrawImage_New(img[cel_ct]);

	tfbimg = FramePtr->image;
	tfbimg->colormap_index = ani[cel_ct].colormap_index;
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
	uint8_t* newdata;
	size_t dpitch;

	TFB_DrawCanvas_GetExtent(canvas, &CharPtr->extent);

	// Currently, each font char has its own separate data
	// but that can change to common mem area
	dpitch = CharPtr->extent.width;
	newdata = (uint8_t*)HMalloc(dpitch * CharPtr->extent.height * sizeof(uint8_t));
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

void* _GetCelData(uqm::io::File* fp, uint32_t length)
{
	if (fp == nullptr)
	{
		return nullptr;
	}

	int cel_total, cel_index, n;
	uint32_t opos;
	char CurrentLine[1024], filename[PATH_MAX];
	TFB_Canvas* img;
	AniData* ani;
	DRAWABLE Drawable;
	uqstl::optional<uqm::io::Mount> aniMount {};
	uqstl::optional<uqm::io::Directory> aniDir {};
	uqstl::optional<uqm::io::File> zippedAniFile {};
	uqm::io::File* aniFile {fp};

	opos = fp->tell();

	{
		const char *s1, *s2;
		char aniDirName[PATH_MAX] {};
		const char* aniFileName;
		uint8_t buf[4] = {0, 0, 0, 0};
		uint32_t header;

		if (_cur_resfile_name == 0
			|| (((s2 = 0), (s1 = strrchr(_cur_resfile_name, '/')) == 0)
				&& (s2 = strrchr(_cur_resfile_name, '\\')) == 0))
		{
			n = 0;
		}
		else
		{
			if (s2 > s1)
			{
				s1 = s2;
			}
			n = s1 - _cur_resfile_name + 1;
		}

		ReadResFile(buf, 4, 1, *fp);
		header = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
		if (_cur_resfile_name && header == 0x04034b50)
		{
			// zipped ani file
			if (n)
			{
				uqm::strncpy_safe(aniDirName, {_cur_resfile_name, static_cast<uint32_t>(n - 1)});
				aniFileName = _cur_resfile_name + n;
			}
			else
			{
				uqm::strncpy_safe(aniDirName, ".");
				aniFileName = _cur_resfile_name;
			}
			aniDir = g_fileSystem->root().subdir(aniDirName);
			if (aniDir)
			{
				uqstl::error_code error {};
				aniMount = g_fileSystem->mountArchive(aniDirName, *aniDir, aniFileName, uqm::io::MountPosition::Top, nullptr, error);
				zippedAniFile = res_OpenResFile(*aniDir, aniFileName, uqm::io::OpenMode::Read);
			}
			if (!zippedAniFile)
			{
				uqm::log::warn("Couldn't open zipped '{}'", _cur_resfile_name);
				return nullptr;
			}
			aniFile = &*zippedAniFile;
			opos = 0;
			n = 0;
		}
		else
		{
			// unpacked ani file
			uqm::strncpy_safe(filename, {_cur_resfile_name, static_cast<uint32_t>(n)});
			aniFile = fp;
			aniDir = *contentDir;
		}
	}

	cel_total = 0;
	aniFile->seek(opos, uqm::io::SeekFrom::Start);
	while (aniFile->readLine(CurrentLine))
	{
		++cel_total;
	}

	img = (TFB_Canvas*)HMalloc(sizeof(TFB_Canvas) * cel_total);
	ani = (AniData*)HMalloc(sizeof(AniData) * cel_total);
	if (!img || !ani)
	{
		uqm::log::warn("Couldn't allocate space for '{}'", _cur_resfile_name);
		HFree(img);
		HFree(ani);
		return nullptr;
	}

	cel_index = 0;
	aniFile->seek(opos, uqm::io::SeekFrom::Start);
	while (aniFile->readLine(CurrentLine) && cel_index < cel_total)
	{
		if (auto result {scn::scan<uqstl::string, int, int, int, int>(CurrentLine, "{} {} {} {} {}")})
		{
			const auto& [filename_str, transparent_color, colormap_index, hotspot_x, hotspot_y] = result->values();
			uqm::strncpy_safe({&filename[n], sizeof(filename) - n}, filename_str);
			ani[cel_index].transparent_color = transparent_color;
			ani[cel_index].colormap_index = colormap_index;
			ani[cel_index].hotspot_x = hotspot_x;
			ani[cel_index].hotspot_y = hotspot_y;
		}
		else
		{
			break;
		}

		img[cel_index] = TFB_DrawCanvas_LoadFromFile(*aniDir, filename);
		if (img[cel_index] == nullptr)
		{
			const char* err;

			err = TFB_DrawCanvas_GetError();
			uqm::log::warn("_GetCelData: Unable to load image!");
			if (err != nullptr)
			{
				uqm::log::warn("Gfx Driver reports: {}", err);
			}
		}
		else
		{
			++cel_index;
		}

		if ((int)aniFile->tell() - (int)opos >= (int)length)
		{
			break;
		}
	}

	Drawable = nullptr;
	if (cel_index && (Drawable = AllocDrawable(cel_index)))
	{
		if (!Drawable)
		{
			while (cel_index--)
			{
				TFB_DrawCanvas_Delete(img[cel_index]);
			}

			HFree(Drawable);
			Drawable = nullptr;
		}
		else
		{
			FRAME FramePtr;

			Drawable->Flags = WANT_PIXMAP;
			Drawable->MaxIndex = cel_index - 1;

			FramePtr = &Drawable->Frame[cel_index];
			while (--FramePtr, cel_index--)
			{
				process_image(FramePtr, img, ani, cel_index);
			}
		}
	}

	if (Drawable == nullptr)
	{
		uqm::log::warn("Couldn't get cel data for '{}'",
					   _cur_resfile_name);
	}

	HFree(img);
	HFree(ani);
	return Drawable;
}

bool _ReleaseCelData(void* handle)
{
	DRAWABLE DrawablePtr;
	int cel_ct;
	FRAME FramePtr = nullptr;

	if ((DrawablePtr = (DRAWABLE)handle) == 0)
	{
		return (false);
	}

	cel_ct = DrawablePtr->MaxIndex + 1;
	FramePtr = DrawablePtr->Frame;

	HFree(handle);
	if (FramePtr)
	{
		int i;
		for (i = 0; i < cel_ct; i++)
		{
			TFB_Image* img = FramePtr[i].image;
			if (img)
			{
				FramePtr[i].image = nullptr;
				TFB_DrawScreen_DeleteImage(img);
			}
		}
		HFree(FramePtr);
	}

	return (true);
}

typedef struct BuildCharDesc
{
	TFB_Canvas canvas;
	UniChar index;
} BuildCharDesc;

static int
compareBCDIndex(const void* arg1, const void* arg2)
{
	const BuildCharDesc* bcd1 = (const BuildCharDesc*)arg1;
	const BuildCharDesc* bcd2 = (const BuildCharDesc*)arg2;

	return (int)bcd1->index - (int)bcd2->index;
}

void* _GetFontData(uqm::io::File* fp, uint32_t length)
{
	uint16_t numDirEntries;
	DIRENTRY fontDir = nullptr;
	BuildCharDesc* bcds = nullptr;
	size_t numBCDs = 0;
	int dirEntryI;
	uqstl::optional<uqm::io::Directory> fontDirHandle {};
	uqstl::optional<uqm::io::Mount> fontMount {};
	FONT fontPtr = nullptr;
	uqstl::optional<uqm::io::File> cfgFile {};
	const char* cfg_name = "kerndat.fnt";

	if (_cur_resfile_name == 0)
	{
		goto err;
	}

	if (fp != nullptr)
	{
		// font is zipped instead of being in a directory

		const char *s1, *s2;
		int n;
		const char* fontZipName;
		char fontDirName[PATH_MAX] {};

		if ((((s2 = 0), (s1 = strrchr(_cur_resfile_name, '/')) == 0)
			 && (s2 = strrchr(_cur_resfile_name, '\\')) == 0))
		{
			uqm::strncpy_safe(fontDirName, ".");
			fontZipName = _cur_resfile_name;
		}
		else
		{
			if (s2 > s1)
			{
				s1 = s2;
			}
			n = s1 - _cur_resfile_name + 1;
			uqm::strncpy_safe(fontDirName, {_cur_resfile_name, static_cast<uint32_t>(n - 1)});
			fontZipName = _cur_resfile_name + n;
		}

		if (const uqstl::optional<uqm::io::Directory> fontParent {g_fileSystem->root().subdir(fontDirName)})
		{
			uqstl::error_code error {};
			fontMount = g_fileSystem->mountArchive(_cur_resfile_name, *fontParent, fontZipName, uqm::io::MountPosition::Top, nullptr, error);
			if (!fontMount)
			{
				uqm::log::warn("Could not mount font archive '{}': {}", _cur_resfile_name, error.message());
			}
		}
	}

	fontDir = CaptureDirEntryTable(LoadDirEntryTable(*contentDir, _cur_resfile_name, uqm::io::Matcher {uqm::io::Matcher::Kind::Substring, "."}));
	if (fontDir == 0)
	{
		goto err;
	}
	numDirEntries = GetDirEntryTableCount(fontDir);

	fontDirHandle = contentDir->subdir(_cur_resfile_name);
	if (!fontDirHandle)
	{
		goto err;
	}

	bcds = (BuildCharDesc*)HMalloc(numDirEntries * sizeof(BuildCharDesc));
	if (bcds == nullptr)
	{
		goto err;
	}

	// Load the surfaces for all dir Entries
	for (dirEntryI = 0; dirEntryI < numDirEntries; dirEntryI++)
	{
		unsigned int charIndex {};
		TFB_Canvas canvas;
		EXTENT size;

		const char* char_name = GetDirEntryAddress(SetAbsDirEntryTableIndex(
			fontDir, dirEntryI));

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

		canvas = TFB_DrawCanvas_LoadFromFile(*fontDirHandle, char_name);
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

	fontPtr = AllocFont(0);
	if (fontPtr == nullptr)
	{
		goto err;
	}

	cfgFile = res_OpenResFile(*fontDirHandle, cfg_name, uqm::io::OpenMode::Read | uqm::io::OpenMode::Text);

	if (cfgFile)
	{
		char CurrentLine[PATH_MAX];
		int cel_index = 0;
		int cel_total = 0;
		uint32_t opos = 0;

		memset(&fontPtr->KernTab, (uint8_t)~0, sizeof(fontPtr->KernTab));

		cfgFile->seek(opos, uqm::io::SeekFrom::Start);
		while (cfgFile->readLine(CurrentLine))
		{
			++cel_total;
			if (cel_total == MAX_UNICODE)
			{
				break;
			}
		}

		cfgFile->seek(opos, uqm::io::SeekFrom::Start);
		while (cfgFile->readLine(CurrentLine) && cel_index < cel_total)
		{
			if (cel_index > 0)
			{
				if (const auto result {scn::scan<int32_t, int32_t, int32_t>(CurrentLine, "{:x} {} {}")})
				{
					auto [KernChar, kernLBits, kernRBits] = result->values();
					if (kernLBits > 3 || kernLBits < 0)
					{
						kernLBits = 3;
					}
					if (kernRBits > 3 || kernRBits < 0)
					{
						kernRBits = 3;
					}

					fontPtr->KernTab[KernChar] =
						(kernLBits << 2) | kernRBits;
				}
			}
			else
			{
				if (const auto result {scn::scan<uqstl::string, uint8_t, uint8_t, uint8_t, int8_t>(CurrentLine, "{} {} {} {} {}")})
				{
					auto [filename_str, Leading, CharSpace, KernAmount, VertAlign] = result->values();
					uqm::strncpy_safe(fontPtr->filename, filename_str);
					fontPtr->Leading = Leading;
					fontPtr->CharSpace = CharSpace;
					fontPtr->KernAmount = KernAmount;
					fontPtr->VertAlign = VertAlign;

					fontPtr->HaveFntData = true;
				}
				else
				{
					break;
				}
			}

			++cel_index;
			if (cel_index == MAX_UNICODE)
			{
				break;
			}

			if ((int)cfgFile->tell() - (int)opos
				>= (int)LengthResFile(*cfgFile))
			{
				break;
			}
		}
		cfgFile.reset();
	}

	fontDirHandle.reset();
	DestroyDirEntryTable(ReleaseDirEntryTable(fontDir));
	fontMount.reset();

#if 0
	if (numBCDs == 0)
		goto err;
#endif

	// sort on the character index
	qsort(bcds, numBCDs, sizeof(BuildCharDesc), compareBCDIndex);

	{
		size_t startBCD = 0;
		UniChar pageStart;
		FONT_PAGE** pageEndPtr = &fontPtr->fontPages;
		while (startBCD < numBCDs)
		{
			// Process one character page.
			size_t endBCD;
			pageStart = bcds[startBCD].index & CHARACTER_PAGE_MASK;

			endBCD = startBCD;
			while (endBCD < numBCDs && (bcds[endBCD].index & CHARACTER_PAGE_MASK) == pageStart)
			{
				endBCD++;
			}

			{
				size_t bcdI;
				int numChars = bcds[endBCD - 1].index + 1
							 - bcds[startBCD].index;
				FONT_PAGE* page = AllocFontPage(numChars);
				page->pageStart = pageStart;
				page->firstChar = bcds[startBCD].index;
				page->numChars = numChars;
				*pageEndPtr = page;
				pageEndPtr = &page->next;

				for (bcdI = startBCD; bcdI < endBCD; bcdI++)
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
										_cur_resfile_name);
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

	HFree(bcds);

	(void)fp;	  /* Satisfying compiler (unused parameter) */
	(void)length; /* Satisfying compiler (unused parameter) */
	return fontPtr;

err:
	if (fontPtr != 0)
	{
		HFree(fontPtr);
	}

	if (bcds != nullptr)
	{
		size_t bcdI;
		for (bcdI = 0; bcdI < numBCDs; bcdI++)
		{
			TFB_DrawCanvas_Delete(bcds[bcdI].canvas);
		}
		HFree(bcds);
	}

	fontDirHandle.reset();

	if (fontDir != 0)
	{
		DestroyDirEntryTable(ReleaseDirEntryTable(fontDir));
	}

	fontMount.reset();

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
		FONT_PAGE* page;
		FONT_PAGE* nextPage;

		for (page = font->fontPages; page != nullptr; page = nextPage)
		{
			size_t charI;
			for (charI = 0; charI < page->numChars; charI++)
			{
				TFB_Char* c = &page->charDesc[charI];

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

	HFree(font);

	return true;
}
