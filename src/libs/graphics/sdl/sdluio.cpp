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

#include "sdluio.h"

#include "port.h"
#include "libs/io/FileSystem.h"
#include "SDL.h"
#include "SDL_error.h"
#include "SDL_rwops.h"
#include "png2sdl.h"

SDL_Surface*
sdluio_loadImage(const uqm::io::Directory& dir, const char* fileName)
{
	uqstl::error_code error {};
	uqstl::optional<uqm::io::File> file {dir.open(fileName, uqm::io::OpenMode::Read, error)};
	if (!file)
	{
		SDL_SetError("Couldn't open '%s': %s", fileName, error.message().c_str());
		return nullptr;
	}

	uqstl::vector<uqstl::byte> data(static_cast<size_t>(file->size()));
	if (file->read(data) != data.size())
	{
		SDL_SetError("Couldn't read '%s'", fileName);
		return nullptr;
	}

	SDL_RWops* const rwops {SDL_RWFromConstMem(data.data(), static_cast<int>(data.size()))};
	if (rwops == nullptr)
	{
		return nullptr;
	}
	SDL_Surface* const result {TFB_png_to_sdl(rwops)};
	SDL_RWclose(rwops);
	return result;
}
