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

#ifndef LIBS_VIDEO_VIDEODEC_H_
#define LIBS_VIDEO_VIDEODEC_H_

#include "core/types/enum.h"
#include "libs/vidlib.h"
#include "libs/video/video.h"
#include "libs/reslib.h"
#include "core/stl/string.h"

// forward-declare
typedef struct tfb_videodecoder TFB_VideoDecoder;


struct TFB_VideoDecoderFuncs
{
	const char* (*GetName)(void);
	bool (*InitModule)(int flags);
	void (*TermModule)(void);
	// Make and delete the decoder struct, which starts with a TFB_VideoDecoder.
	TFB_VideoDecoder* (*Create)(void);
	void (*Destroy)(TFB_VideoDecoder*);
	int (*GetError)(TFB_VideoDecoder*);
	bool (*Init)(TFB_VideoDecoder*, TFB_PixelFormat* fmt);
	void (*Term)(TFB_VideoDecoder*);
	bool (*Open)(TFB_VideoDecoder*, const uqm::io::Directory& dir, const char* filename);
	void (*Close)(TFB_VideoDecoder*);
	int (*DecodeNext)(TFB_VideoDecoder*);
	uint32_t (*SeekFrame)(TFB_VideoDecoder*, uint32_t frame);
	float (*SeekTime)(TFB_VideoDecoder*, float time);
	uint32_t (*GetFrame)(TFB_VideoDecoder*);
	float (*GetTime)(TFB_VideoDecoder*);
};

// decoder will call these to get info
// from the player
struct TFB_VideoCallbacks
{
	// any decoder calls these
	void (*BeginFrame)(TFB_VideoDecoder*);
	void (*EndFrame)(TFB_VideoDecoder*);
	void* (*GetCanvasLine)(TFB_VideoDecoder*, uint32_t line);
	// non-audio-driven decoders call this to figure out
	// when the next frame should be drawn
	uint32_t (*GetTicks)(TFB_VideoDecoder*);
	// non-audio-driven decoders call this to inform
	// the player when the next frame should be drawn
	bool (*SetTimer)(TFB_VideoDecoder*, uint32_t msecs);
};


struct tfb_videodecoder
{
	// decoder virtual funcs - R/O
	const TFB_VideoDecoderFuncs* funcs;
	// video formats - R/O
	const TFB_PixelFormat* format;
	// decoder-set data - R/O
	uint32_t w, h;
	float length; // total length in seconds
	uint32_t frame_count;
	uint32_t interframe_wait; // nominal interframe delay in msecs
	bool audio_synced;
	// decoder callbacks
	TFB_VideoCallbacks callbacks;

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

// return values
enum class VideoDecoderStatus
{
	Ok,
	Error,
	Eof,
};
template <>
inline constexpr bool IsArithmeticEnum<VideoDecoderStatus> = true;

typedef struct TFB_RegVideoDecoder TFB_RegVideoDecoder;

TFB_RegVideoDecoder* VideoDecoder_Register(const char* fileext,
										   TFB_VideoDecoderFuncs* decvtbl);
void VideoDecoder_Unregister(TFB_RegVideoDecoder* regdec);
const TFB_VideoDecoderFuncs* VideoDecoder_Lookup(const char* fileext);

bool VideoDecoder_Init(int flags, int depth, uint32_t Rmask, uint32_t Gmask,
					   uint32_t Bmask, uint32_t Amask);
void VideoDecoder_Uninit(void);
TFB_VideoDecoder* VideoDecoder_Load(const uqm::io::Directory& dir,
									const char* filename);
int VideoDecoder_Decode(TFB_VideoDecoder* decoder);
float VideoDecoder_Seek(TFB_VideoDecoder* decoder, float time_pos);
uint32_t VideoDecoder_SeekFrame(TFB_VideoDecoder* decoder, uint32_t frame_pos);
void VideoDecoder_Rewind(TFB_VideoDecoder* decoder);
void VideoDecoder_Free(TFB_VideoDecoder* decoder);
const char* VideoDecoder_GetName(TFB_VideoDecoder* decoder);


#endif // LIBS_VIDEO_VIDEODEC_H_
