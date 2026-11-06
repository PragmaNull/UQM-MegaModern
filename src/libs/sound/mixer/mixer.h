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

#ifndef LIBS_SOUND_MIXER_MIXER_H_
#define LIBS_SOUND_MIXER_MIXER_H_

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
enum
{
	MIX_NO_ERROR = 0,
	MIX_INVALID_NAME = 0xA001U,
	MIX_INVALID_ENUM = 0xA002U,
	MIX_INVALID_VALUE = 0xA003U,
	MIX_INVALID_OPERATION = 0xA004U,
	MIX_OUT_OF_MEMORY = 0xA005U,

	MIX_DRIVER_FAILURE = 0xA101U
};

/**
 * Source properties (see OpenAL)
 */
typedef enum
{
	MIX_POSITION = 0x1004,
	MIX_LOOPING = 0x1007,
	MIX_BUFFER = 0x1009,
	MIX_GAIN = 0x100A,
	MIX_SOURCE_STATE = 0x1010,

	MIX_BUFFERS_QUEUED = 0x1015,
	MIX_BUFFERS_PROCESSED = 0x1016

} mixer_SourceProp;

/**
 * Source state information
 */
typedef enum
{
	MIX_INITIAL = 0,
	MIX_STOPPED,
	MIX_PLAYING,
	MIX_PAUSED,

} mixer_SourceState;

/** 
 * Sound buffer properties
 */
typedef enum
{
	MIX_FREQUENCY = 0x2001,
	MIX_BITS = 0x2002,
	MIX_CHANNELS = 0x2003,
	MIX_SIZE = 0x2004,
	MIX_DATA = 0x2005

} mixer_BufferProp;

/**
 * Buffer states: semi-private
 */
typedef enum
{
	MIX_BUF_INITIAL = 0,
	MIX_BUF_FILLED,
	MIX_BUF_QUEUED,
	MIX_BUF_PLAYING,
	MIX_BUF_PROCESSED

} mixer_BufferState;

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

typedef enum
{
	MIX_FORMAT_MONO8 = MixFormatMake(1, 1),
	MIX_FORMAT_STEREO8 = MixFormatMake(1, 2),
	MIX_FORMAT_MONO16 = MixFormatMake(2, 1),
	MIX_FORMAT_STEREO16 = MixFormatMake(2, 2)

} mixer_Format;

typedef enum
{
	MIX_QUALITY_LOW = 0,
	MIX_QUALITY_MEDIUM,
	MIX_QUALITY_HIGH,
	MIX_QUALITY_DEFAULT = MIX_QUALITY_MEDIUM,
	MIX_QUALITY_COUNT

} mixer_Quality;

typedef enum
{
	MIX_NOFLAGS = 0,
	MIX_FAKE_DATA = 1
} mixer_Flags;

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
	mixer_BufferState state;
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
	mixer_SourceState state;
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
uint32_t mixer_GetError(void);

bool mixer_Init(uint32_t frequency, uint32_t format, mixer_Quality quality,
				mixer_Flags flags);
void mixer_Uninit(void);
void mixer_MixChannels(void* userdata, uint8_t* stream, int32_t len);
void mixer_MixFake(void* userdata, uint8_t* stream, int32_t len);

/*************************************************
 *  Sources
 */
void mixer_GenSources(uint32_t n, mixer_Object* psrcobj);
void mixer_DeleteSources(uint32_t n, mixer_Object* psrcobj);
bool mixer_IsSource(mixer_Object srcobj);
void mixer_Sourcei(mixer_Object srcobj, mixer_SourceProp pname,
				   mixer_IntVal value);
void mixer_Sourcef(mixer_Object srcobj, mixer_SourceProp pname,
				   float value);
void mixer_Sourcefv(mixer_Object srcobj, mixer_SourceProp pname,
					float* value);
void mixer_GetSourcei(mixer_Object srcobj, mixer_SourceProp pname,
					  mixer_IntVal* value);
void mixer_GetSourcef(mixer_Object srcobj, mixer_SourceProp pname,
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
void mixer_GetBufferi(mixer_Object bufobj, mixer_BufferProp pname,
					  mixer_IntVal* value);
void mixer_BufferData(mixer_Object bufobj, uint32_t format, void* data,
					  uint32_t size, uint32_t freq);


static_assert(sizeof(mixer_Object) >= sizeof(void*), "mixer_Object must be able to hold a pointer");
static_assert(sizeof(mixer_IntVal) >= sizeof(mixer_Object), "mixer_IntVal must be able to hold a mixer_Object");

#endif /* LIBS_SOUND_MIXER_MIXER_H_ */
