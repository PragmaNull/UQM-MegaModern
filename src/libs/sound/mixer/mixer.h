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

/* Mixer for low-level sound output drivers
 */

#pragma once
#ifndef UQM_LIBS_SOUND_MIXER_MIXER_H
#define UQM_LIBS_SOUND_MIXER_MIXER_H

#include "core/types/enum.h"
#include "config.h"
#include <cstdint>
#include "core/stl/memory.h"

/**
 * The interface heavily influenced by OpenAL
 * to the point where you should use OpenAL's
 * documentation when programming the mixer.
 * (some source properties are not supported)
 *
 * EXCEPTION: You may not queue the same buffer
 * on more than one source
 */

/**
 * Mixer errors (see OpenAL errors)
 */
enum class MixerError
{
	NoError = 0,
	InvalidName = 0xA001U,
	InvalidEnum = 0xA002U,
	InvalidValue = 0xA003U,
	InvalidOperation = 0xA004U,
	OutOfMemory = 0xA005U,

	DriverFailure = 0xA101U
};
consteval bool isArithmeticEnum(MixerError) { return true; }

/**
 * Source properties (see OpenAL)
 */
enum class MixerSourceProp
{
	Position = 0x1004,
	Looping = 0x1007,
	Buffer = 0x1009,
	Gain = 0x100A,
	SourceState = 0x1010,

	BuffersQueued = 0x1015,
	BuffersProcessed = 0x1016

};
consteval bool isArithmeticEnum(MixerSourceProp) { return true; }

/**
 * Source state information
 */
enum class MixerSourceState
{
	Initial = 0,
	Stopped,
	Playing,
	Paused,

};
consteval bool isArithmeticEnum(MixerSourceState) { return true; }

/** 
 * Sound buffer properties
 */
enum class MixerBufferProp
{
	Frequency = 0x2001,
	Bits = 0x2002,
	Channels = 0x2003,
	Size = 0x2004,
	Data = 0x2005

};
consteval bool isArithmeticEnum(MixerBufferProp) { return true; }

/**
 * Buffer states: semi-private
 */
enum class MixerBufferState
{
	Initial = 0,
	Filled,
	Queued,
	Playing,
	Processed

};

/** Sound buffers: format specifier.
 * bits 00..07: bytes per sample
 * bits 08..15: channels
 * bits 15..31: meaningless
 */
inline constexpr auto MixFormatDummyid {0x00170000};
constexpr auto MixFormatBpc(auto f) { return ((f) & 0xff); }
constexpr auto MixFormatChans(auto f) { return (((f) >> 8) & 0xff); }
inline constexpr auto MixFormatBpcMax {2};
inline constexpr auto MixFormatChansMax {2};
constexpr auto MixFormatMake(auto b, auto c) { return (MixFormatDummyid | ((b) & 0xff) | (((c) & 0xff) << 8)); }

constexpr auto MixFormatSampsize(auto f) { return (MixFormatBpc(f) * MixFormatChans(f)); }

enum class MixerFormat
{
	Mono8 = MixFormatMake(1, 1),
	Stereo8 = MixFormatMake(1, 2),
	Mono16 = MixFormatMake(2, 1),
	Stereo16 = MixFormatMake(2, 2)

};
consteval bool isArithmeticEnum(MixerFormat) { return true; }

enum class MixerQuality
{
	Low = 0,
	Medium,
	High,
	Default = Medium,
	Count

};
consteval bool isArithmeticEnum(MixerQuality) { return true; }

enum class MixerFlags
{
	None = 0,
	FakeData = 1
};
consteval bool isFlagEnum(MixerFlags) { return true; }

/*************************************************
 *  Interface Types
 */

typedef intptr_t mixer_Object;
typedef intptr_t mixer_IntVal;

typedef struct _mixer_Source mixer_Source;

struct mixer_Buffer
{
	uint32_t magic;
	bool locked;
	MixerBufferState state;
	uqstl::unique_ptr<uint8_t[]> data;
	uint32_t size;
	uint32_t sampsize;
	uint32_t high;
	uint32_t low;
	float (*Resample)(mixer_Source* src, bool left);
	/* original buffer values for OpenAL compat */
	void* orgdata;
	uint32_t orgfreq;
	uint32_t orgsize;
	uint32_t orgchannels;
	uint32_t orgchansize;
	/* next buffer in chain */
	struct mixer_Buffer* next;
};

inline constexpr auto mixer_bufMagic {0x4258494DU}; /* MIXB in LSB */

struct _mixer_Source
{
	uint32_t magic;
	bool locked;
	MixerSourceState state;
	bool looping;
	float gain;
	float leftGain;
	float rightGain;
	uint32_t cqueued;
	uint32_t cprocessed;
	mixer_Buffer* firstqueued; /* first buf in the queue */
	mixer_Buffer* nextqueued;  /* next to play, or 0 */
	mixer_Buffer* prevqueued;  /* previously played */
	mixer_Buffer* lastqueued;  /* last in queue */
	uint32_t pos;			   /* position in current buffer */
	uint32_t count;			   /* fractional part of pos */

	float samplecache;
};

inline constexpr auto mixer_srcMagic {0x5358494DU}; /* MIXS in LSB */

/*************************************************
 *  General interface
 */
MixerError mixer_GetError(void);

bool mixer_Init(uint32_t frequency, uint32_t format, MixerQuality quality,
				MixerFlags flags);
void mixer_Uninit(void);
void mixer_MixChannels(void* userdata, uint8_t* stream, int32_t len);
void mixer_MixFake(void* userdata, uint8_t* stream, int32_t len);

/*************************************************
 *  Sources
 */
void mixer_GenSources(uint32_t n, mixer_Object* psrcobj);
void mixer_DeleteSources(uint32_t n, mixer_Object* psrcobj);
bool mixer_IsSource(mixer_Object srcobj);
void mixer_Sourcei(mixer_Object srcobj, MixerSourceProp pname,
				   mixer_IntVal value);
void mixer_Sourcef(mixer_Object srcobj, MixerSourceProp pname,
				   float value);
void mixer_Sourcefv(mixer_Object srcobj, MixerSourceProp pname,
					float* value);
void mixer_GetSourcei(mixer_Object srcobj, MixerSourceProp pname,
					  mixer_IntVal* value);
void mixer_GetSourcef(mixer_Object srcobj, MixerSourceProp pname,
					  float* value);
void mixer_SourceRewind(mixer_Object srcobj);
void mixer_SourcePlay(mixer_Object srcobj);
void mixer_SourcePause(mixer_Object srcobj);
void mixer_SourceStop(mixer_Object srcobj);
void mixer_SourceQueueBuffers(mixer_Object srcobj, uint32_t n,
							  mixer_Object* pbufobj);
void mixer_SourceUnqueueBuffers(mixer_Object srcobj, uint32_t n,
								mixer_Object* pbufobj);

/*************************************************
 *  Buffers
 */
void mixer_GenBuffers(uint32_t n, mixer_Object* pbufobj);
void mixer_DeleteBuffers(uint32_t n, mixer_Object* pbufobj);
bool mixer_IsBuffer(mixer_Object bufobj);
void mixer_GetBufferi(mixer_Object bufobj, MixerBufferProp pname,
					  mixer_IntVal* value);
void mixer_BufferData(mixer_Object bufobj, uint32_t format, void* data,
					  uint32_t size, uint32_t freq);


static_assert(sizeof(mixer_Object) >= sizeof(void*), "mixer_Object must be able to hold a pointer");
static_assert(sizeof(mixer_IntVal) >= sizeof(mixer_Object), "mixer_IntVal must be able to hold a mixer_Object");

#endif // UQM_LIBS_SOUND_MIXER_MIXER_H
