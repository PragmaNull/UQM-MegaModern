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

#pragma once
#ifndef UQM_LIBS_SOUND_DECODERS_DECODER_H
#define UQM_LIBS_SOUND_DECODERS_DECODER_H

#include <cstdint>

#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "core/types/enum.h"
#include "libs/io/FileSystem.h"
#include "libs/mathlib.h"
#include "libs/sound/audiocoredefs.h"
#include "port.h"

#ifndef OVCODEC_NONE
#ifdef _MSC_VER
#pragma comment(lib, "vorbisfile.lib")
#endif /* _MSC_VER */
#endif /* OVCODEC_NONE */

namespace tfb
{
struct DecoderFormats
{
	bool big_endian;
	bool want_big_endian;
	uint32_t mono8;
	uint32_t stereo8;
	uint32_t mono16;
	uint32_t stereo16;
};
} // namespace tfb

enum class AudioFileType
{
	Wav,
	Tracker,
	Ogg,
	Duk,
	Aif,
	Count
};
consteval bool isArithmeticEnum(AudioFileType) { return true; }

// forward-declare
namespace tfb
{
struct SoundDecoder;

struct SoundDecoderFuncs
{
	const char* (*GetName)(void);
	bool (*InitModule)(AudioFlags flags, const DecoderFormats*);
	void (*TermModule)(void);
	// Make and delete the decoder struct, which starts with a SoundDecoder.
	SoundDecoder* (*Create)(void);
	void (*Destroy)(SoundDecoder*);
	int (*GetError)(SoundDecoder*);
	bool (*Init)(SoundDecoder*);
	void (*Term)(SoundDecoder*);
	bool (*Open)(SoundDecoder*, const uqm::io::Directory& dir, const char* filename);
	void (*Close)(SoundDecoder*);
	int (*Decode)(SoundDecoder*, void* buf, int32_t bufsize);
	// returns <0 on error, ==0 when no more data, >0 bytes returned
	uint32_t (*Seek)(SoundDecoder*, uint32_t pcm_pos);
	// returns the pcm position set
	uint32_t (*GetFrame)(SoundDecoder*);
};

struct SoundDecoder
{
	// decoder virtual funcs - R/O
	const SoundDecoderFuncs* funcs;

	// public R/O, set by decoder
	uint32_t format;
	uint32_t frequency;
	float length; // total length in seconds
	bool is_null;
	bool need_swap;

	// public R/O, set by wrapper
	// The decoded data. 'buffer' points into 'storage'.
	uqstl::vector<uint8_t> storage;
	void* buffer;
	uint32_t buffer_size;
	int32_t error;
	uint32_t bytes_per_samp;

	// public R/W
	bool looping;

	// semi-private
	uqstl::string filename;
	// Deletes the decoder. The functions can change to the buffer decoder after creation.
	void (*destroy)(SoundDecoder*);
	// The read position in 'storage' after the decoder switched to the buffer decoder.
	uint32_t buffered_max_pcm;
	uint32_t buffered_cur_pcm;
	uint32_t pos;
	uint32_t start_sample;
	uint32_t end_sample;

	uint16_t numpos;
	// for tracker modules
	uint32_t filename_hash;
	// for music resume
};
} // namespace tfb

// return values
enum class SoundDecoderStatus
{
	Ok,
	Error,
	Eof,
};
consteval bool isArithmeticEnum(SoundDecoderStatus) { return true; }

namespace tfb
{
struct RegSoundDecoder;
} // namespace tfb

tfb::RegSoundDecoder* SoundDecoder_Register(const char* fileext,
											tfb::SoundDecoderFuncs* decvtbl);
void SoundDecoder_Unregister(tfb::RegSoundDecoder* regdec);
const tfb::SoundDecoderFuncs* SoundDecoder_Lookup(const char* fileext);

void SoundDecoder_SwapWords(uint16_t* data, uint32_t size);
int32_t SoundDecoder_Init(AudioFlags flags, tfb::DecoderFormats* formats);
void SoundDecoder_Uninit(void);
tfb::SoundDecoder* SoundDecoder_Load(const uqm::io::Directory& dir,
									 const char* filename, uint32_t buffer_size, uint32_t startTime, int32_t runTime);
uint32_t SoundDecoder_Decode(tfb::SoundDecoder* decoder);
uint32_t SoundDecoder_DecodeAll(tfb::SoundDecoder* decoder);
float SoundDecoder_GetTime(tfb::SoundDecoder* decoder);
uint32_t SoundDecoder_GetFrame(tfb::SoundDecoder* decoder);
void SoundDecoder_Seek(tfb::SoundDecoder* decoder, uint32_t msecs);
void SoundDecoder_Rewind(tfb::SoundDecoder* decoder);
void SoundDecoder_Free(tfb::SoundDecoder* decoder);
const char* SoundDecoder_GetName(tfb::SoundDecoder* decoder);

#endif // UQM_LIBS_SOUND_DECODERS_DECODER_H
