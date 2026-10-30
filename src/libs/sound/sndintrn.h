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

#ifndef LIBS_SOUND_SNDINTRN_H_
#define LIBS_SOUND_SNDINTRN_H_

#include <stdio.h>
#include <cstdint>
#include "libs/reslib.h"
#include "core/stl/vector.h"
#include "core/thread/Mutex.h"

#define PAD_SCOPE_BYTES 256

extern void* _GetMusicData(const ResourceFile& resource);
extern bool _ReleaseMusicData(void* handle);

extern void* _GetSoundBankData(const ResourceFile& resource);
extern bool _ReleaseSoundBankData(void* handle);

extern char* CheckMusicResName(char* filename);

// audio data
struct tfb_soundsample
{
	TFB_SoundDecoder* decoder; // decoder to read from
	float length;			   // total length of decoder chain in seconds
	uqstl::vector<audio_Object> buffer;
	uint32_t num_buffers;
	uqstl::vector<TFB_SoundTag> buffer_tag;
	int32_t offset;				  // initial offset
	void* data;					  // user-defined data
	TFB_SoundCallbacks callbacks; // user-defined callbacks
};

// equivalent to channel in legacy sound code
typedef struct tfb_soundsource
{
	uqm::RecursiveMutex stream_mutex;
	TFB_SoundSample* sample;
	audio_Object handle;
	bool stream_should_be_playing UQM_GUARDED_BY(stream_mutex);
	int32_t start_time UQM_GUARDED_BY(stream_mutex);  // for tracks played-time math
	uint32_t pause_time UQM_GUARDED_BY(stream_mutex); // keep track for paused tracks
	void* positional_object;

	audio_Object last_q_buf UQM_GUARDED_BY(stream_mutex); // for callbacks processing

	// Cyclic waveform buffer for oscilloscope
	uqstl::vector<uint8_t> sbuffer UQM_GUARDED_BY(stream_mutex);
	uint32_t sbuf_size UQM_GUARDED_BY(stream_mutex);
	uint32_t sbuf_tail UQM_GUARDED_BY(stream_mutex);
	uint32_t sbuf_head UQM_GUARDED_BY(stream_mutex);
	uint32_t sbuf_lasttime UQM_GUARDED_BY(stream_mutex); // timestamp of the first queued buffer
} TFB_SoundSource;

extern TFB_SoundSource soundSource[];

#endif /* LIBS_SOUND_SNDINTRN_H_ */
