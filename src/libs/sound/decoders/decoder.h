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

/* Sound file decoder for .wav, .mod, .ogg
 * API is heavily influenced by SDL_sound.
 */

#ifndef DECODER_H
#define DECODER_H

#include "port.h"
#include <cstdint>
#include "libs/io/FileSystem.h"
#include "libs/sound/audiocoredefs.h"

#ifndef OVCODEC_NONE
#ifdef _MSC_VER
#pragma comment(lib, "vorbisfile.lib")
#endif /* _MSC_VER */
#endif /* OVCODEC_NONE */

typedef struct tfb_decoderformats
{
	bool big_endian;
	bool want_big_endian;
	uint32_t mono8;
	uint32_t stereo8;
	uint32_t mono16;
	uint32_t stereo16;
} TFB_DecoderFormats;

typedef enum
{
	AUDIO_WAV,
	AUDIO_TRACKER,
	AUDIO_OGG,
	AUDIO_DUK,
	AUDIO_AIF,
	NUM_AUDIO_FILETYPES
} AUDIO_FILETYPES;

// forward-declare
typedef struct tfb_sounddecoder TFB_SoundDecoder;

#define THIS_PTR TFB_SoundDecoder*

typedef struct tfb_sounddecoderfunc
{
	const char* (*GetName)(void);
	bool (*InitModule)(AudioFlags flags, const TFB_DecoderFormats*);
	void (*TermModule)(void);
	uint32_t (*GetStructSize)(void);
	int (*GetError)(THIS_PTR);
	bool (*Init)(THIS_PTR);
	void (*Term)(THIS_PTR);
	bool (*Open)(THIS_PTR, const uqm::io::Directory& dir, const char* filename);
	void (*Close)(THIS_PTR);
	int (*Decode)(THIS_PTR, void* buf, int32_t bufsize);
	// returns <0 on error, ==0 when no more data, >0 bytes returned
	uint32_t (*Seek)(THIS_PTR, uint32_t pcm_pos);
	// returns the pcm position set
	uint32_t (*GetFrame)(THIS_PTR);

} TFB_SoundDecoderFuncs;

#undef THIS_PTR

struct tfb_sounddecoder
{
	// decoder virtual funcs - R/O
	const TFB_SoundDecoderFuncs* funcs;

	// public R/O, set by decoder
	uint32_t format;
	uint32_t frequency;
	float length; // total length in seconds
	bool is_null;
	bool need_swap;

	// public R/O, set by wrapper
	void* buffer;
	uint32_t buffer_size;
	int32_t error;
	uint32_t bytes_per_samp;

	// public R/W
	bool looping;

	// semi-private
	char* filename;
	uint32_t pos;
	uint32_t start_sample;
	uint32_t end_sample;

	uint16_t numpos;
	// for tracker modules
	uint32_t filename_hash;
	// for music resume
};

// return values
enum
{
	SOUNDDECODER_OK,
	SOUNDDECODER_ERROR,
	SOUNDDECODER_EOF,
};

typedef struct TFB_RegSoundDecoder TFB_RegSoundDecoder;

TFB_RegSoundDecoder* SoundDecoder_Register(const char* fileext,
										   TFB_SoundDecoderFuncs* decvtbl);
void SoundDecoder_Unregister(TFB_RegSoundDecoder* regdec);
const TFB_SoundDecoderFuncs* SoundDecoder_Lookup(const char* fileext);

void SoundDecoder_SwapWords(uint16_t* data, uint32_t size);
int32_t SoundDecoder_Init(AudioFlags flags, TFB_DecoderFormats* formats);
void SoundDecoder_Uninit(void);
// Opens a file for a decoder. The caller owns the result. Returns nullptr on failure.
uqgsl::owner<uqm::io::File*> SoundDecoder_OpenFile(const uqm::io::Directory& dir, const char* filename);
TFB_SoundDecoder* SoundDecoder_Load(const uqm::io::Directory& dir,
									char* filename, uint32_t buffer_size, uint32_t startTime, int32_t runTime);
uint32_t SoundDecoder_Decode(TFB_SoundDecoder* decoder);
uint32_t SoundDecoder_DecodeAll(TFB_SoundDecoder* decoder);
float SoundDecoder_GetTime(TFB_SoundDecoder* decoder);
uint32_t SoundDecoder_GetFrame(TFB_SoundDecoder* decoder);
void SoundDecoder_Seek(TFB_SoundDecoder* decoder, uint32_t msecs);
void SoundDecoder_Rewind(TFB_SoundDecoder* decoder);
void SoundDecoder_Free(TFB_SoundDecoder* decoder);
const char* SoundDecoder_GetName(TFB_SoundDecoder* decoder);

extern uint32_t crc32b(const char* str);

#endif
