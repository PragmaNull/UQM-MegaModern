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

#ifndef LIBS_GRAPHICS_SDL_SDLUIO_H_
#define LIBS_GRAPHICS_SDL_SDLUIO_H_

#include "libs/io/FileSystem.h"
#include <SDL.h>

SDL_Surface* sdluio_loadImage(const uqm::io::Directory& dir, const char* fileName);

#endif /* LIBS_GRAPHICS_SDL_SDLUIO_H_ */
