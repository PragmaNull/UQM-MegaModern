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

#ifndef LIBS_VIDEO_VIDEO_H_
#define LIBS_VIDEO_VIDEO_H_

#include "libs/vidlib.h"
#include "libs/sndlib.h"
#include "libs/graphics/tfb_draw.h"
#include <cstdint>
#include "videodec.h"
#include "libs/sound/sound.h"
#include "core/thread/Mutex.h"


typedef struct tfb_videoclip
{
	TFB_VideoDecoder* decoder; // decoder to read from
	float length;			   // total length of clip seconds
	uint32_t w, h;

	// video player data
	GFXRECT dst_rect; // destination screen rect
	GFXRECT src_rect; // source rect
	MUSIC_REF hAudio;
	uint32_t frame_time; // time when next frame should be rendered
	TFB_Image* frame;	 // frame preped and optimized for rendering
	uint32_t cur_frame;	 // index of frame currently displayed
	bool playing;
	bool own_audio;
	uint32_t loop_frame; // frame index to loop from
	uint32_t loop_to;	 // frame index to loop to

	uqm::Mutex guard;
	uint32_t want_frame UQM_GUARDED_BY(guard); // audio-signaled desired frame index
	int lag_cnt;							   // N of frames video is behind or ahead of audio

	void* data; // user-defined data

} TFB_VideoClip;

extern VIDEO_REF _init_video_file(const char* pStr);

#endif // LIBS_VIDEO_VIDEO_H_
