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

#pragma once
#ifndef UQM_LIBS_GRAPHICS_SDL_PNG2SDL_H
#define UQM_LIBS_GRAPHICS_SDL_PNG2SDL_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>


SDL_Surface* TFB_png_to_sdl(SDL_IOStream* src);

extern int TFB_sdl_to_png(SDL_Surface* surface, SDL_IOStream* dst);
extern SDL_Surface* SDL_PNGFormatAlpha(SDL_Surface* src);

#endif // UQM_LIBS_GRAPHICS_SDL_PNG2SDL_H
