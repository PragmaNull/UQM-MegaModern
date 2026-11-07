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
#ifndef UQM_LIBS_VIDEO_VIDEODEC_H
#define UQM_LIBS_VIDEO_VIDEODEC_H

#include "core/types/enum.h"
#include "libs/vidlib.h"
#include "libs/video/video.h"
#include "libs/reslib.h"
#include "core/stl/string.h"

// forward-declare
namespace tfb
{
struct VideoDecoder;

struct VideoDecoderFuncs
{
	const char* (*GetName)(void);
	bool (*InitModule)(int flags);
	void (*TermModule)(void);
	// Make and delete the decoder struct, which starts with a VideoDecoder.
	VideoDecoder* (*Create)(void);
	void (*Destroy)(VideoDecoder*);
	int (*GetError)(VideoDecoder*);
	bool (*Init)(VideoDecoder*, PixelFormat* fmt);
	void (*Term)(VideoDecoder*);
	bool (*Open)(VideoDecoder*, const uqm::io::Directory& dir, const char* filename);
	void (*Close)(VideoDecoder*);
	int (*DecodeNext)(VideoDecoder*);
	uint32_t (*SeekFrame)(VideoDecoder*, uint32_t frame);
	float (*SeekTime)(VideoDecoder*, float time);
	uint32_t (*GetFrame)(VideoDecoder*);
	float (*GetTime)(VideoDecoder*);
};
} // namespace tfb

// decoder will call these to get info
// from the player
namespace tfb
{
struct VideoCallbacks
{
	// any decoder calls these
	void (*BeginFrame)(VideoDecoder*);
	void (*EndFrame)(VideoDecoder*);
	void* (*GetCanvasLine)(VideoDecoder*, uint32_t line);
	// non-audio-driven decoders call this to figure out
	// when the next frame should be drawn
	uint32_t (*GetTicks)(VideoDecoder*);
	// non-audio-driven decoders call this to inform
	// the player when the next frame should be drawn
	bool (*SetTimer)(VideoDecoder*, uint32_t msecs);
};

struct VideoDecoder
{
	// decoder virtual funcs - R/O
	const VideoDecoderFuncs* funcs;
	// video formats - R/O
	const PixelFormat* format;
	// decoder-set data - R/O
	uint32_t w, h;
	float length; // total length in seconds
	uint32_t frame_count;
	uint32_t interframe_wait; // nominal interframe delay in msecs
	bool audio_synced;
	// decoder callbacks
	VideoCallbacks callbacks;

	// other - public
	bool looping;
	void* data; // user-defined data
	// info - public R/O
	int32_t error;
	float pos; // position in seconds
	uint32_t cur_frame;

	// semi-private
	uqstl::string filename;
};
} // namespace tfb

// return values
enum class VideoDecoderStatus
{
	Ok,
	Error,
	Eof,
};
consteval bool isArithmeticEnum(VideoDecoderStatus) { return true; }

namespace tfb
{
struct RegVideoDecoder;
} // namespace tfb

tfb::RegVideoDecoder* VideoDecoder_Register(const char* fileext,
											tfb::VideoDecoderFuncs* decvtbl);
void VideoDecoder_Unregister(tfb::RegVideoDecoder* regdec);
const tfb::VideoDecoderFuncs* VideoDecoder_Lookup(const char* fileext);

bool VideoDecoder_Init(int flags, int depth, uint32_t Rmask, uint32_t Gmask,
					   uint32_t Bmask, uint32_t Amask);
void VideoDecoder_Uninit(void);
tfb::VideoDecoder* VideoDecoder_Load(const uqm::io::Directory& dir,
									 const char* filename);
int VideoDecoder_Decode(tfb::VideoDecoder* decoder);
float VideoDecoder_Seek(tfb::VideoDecoder* decoder, float time_pos);
uint32_t VideoDecoder_SeekFrame(tfb::VideoDecoder* decoder, uint32_t frame_pos);
void VideoDecoder_Rewind(tfb::VideoDecoder* decoder);
void VideoDecoder_Free(tfb::VideoDecoder* decoder);
const char* VideoDecoder_GetName(tfb::VideoDecoder* decoder);


#endif // UQM_LIBS_VIDEO_VIDEODEC_H
