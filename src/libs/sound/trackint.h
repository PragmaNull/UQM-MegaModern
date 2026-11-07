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

#pragma once
#ifndef UQM_LIBS_SOUND_TRACKINT_H
#define UQM_LIBS_SOUND_TRACKINT_H

#include "libs/callback/callback.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"

namespace tfb
{
struct SoundChunk
{
	SoundDecoder* decoder;			 // decoder for this chunk
	float start_time;					 // relative time from track start
	int tag_me;							 // set for chunks with subtitles
	uint32_t track_num;					 // logical track #, comm code needs this
	uqstl::optional<uqstl::string> text; // subtitle text
	CallbackFunction callback;			 // comm callback, executed on chunk start
	struct SoundChunk* next;
};

struct SoundChunk;
} // namespace tfb

tfb::SoundChunk* create_SoundChunk(tfb::SoundDecoder* decoder, float start_time);
void destroy_SoundChunk_list(tfb::SoundChunk* chain);
tfb::SoundChunk* find_next_page(tfb::SoundChunk* cur);
tfb::SoundChunk* find_prev_page(tfb::SoundChunk* cur);


#endif // UQM_LIBS_SOUND_TRACKINT_H
