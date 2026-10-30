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

/* SDL audio driver
 */

#include "audiodrv_sdl.h"
#include "../../sndintrn.h"
#include "core/log/log.h"
#include <stdlib.h>
#include "core/stl/vector.h"


/* SDL2 wants to talk to a specific device. We'll let SDL1 use the same
 * function names and just throw the device argument away. */
static SDL_AudioStream* stream {};
// Mixed data for the audio stream. Only the audio callback uses it.
static uqstl::vector<uint8_t> s_mixBuffer {};

static const audio_Driver mixSDL_Driver =
	{
		mixSDL_Uninit,
		mixSDL_GetError,
		AudioDriverType::MixSDL,
		{/* Errors */
		 MIX_NO_ERROR,
				  MIX_INVALID_NAME,
				  MIX_INVALID_ENUM,
				  MIX_INVALID_VALUE,
				  MIX_INVALID_OPERATION,
				  MIX_OUT_OF_MEMORY,
				  MIX_DRIVER_FAILURE,

				  /* Source properties */
		 MIX_POSITION,
				  MIX_LOOPING,
				  MIX_BUFFER,
				  MIX_GAIN,
				  MIX_SOURCE_STATE,
				  MIX_BUFFERS_QUEUED,
				  MIX_BUFFERS_PROCESSED,

				  /* Source state information */
		 MIX_INITIAL,
				  MIX_STOPPED,
				  MIX_PLAYING,
				  MIX_PAUSED,

				  /* Sound buffer properties */
		 MIX_FREQUENCY,
				  MIX_BITS,
				  MIX_CHANNELS,
				  MIX_SIZE,
				  MIX_FORMAT_MONO16,
				  MIX_FORMAT_STEREO16,
				  MIX_FORMAT_MONO8,
				  MIX_FORMAT_STEREO8},

		/* Sources */
		mixSDL_GenSources,
		mixSDL_DeleteSources,
		mixSDL_IsSource,
		mixSDL_Sourcei,
		mixSDL_Sourcef,
		mixSDL_Sourcefv,
		mixSDL_GetSourcei,
		mixSDL_GetSourcef,
		mixSDL_SourceRewind,
		mixSDL_SourcePlay,
		mixSDL_SourcePause,
		mixSDL_SourceStop,
		mixSDL_SourceQueueBuffers,
		mixSDL_SourceUnqueueBuffers,

		/* Buffers */
		mixSDL_GenBuffers,
		mixSDL_DeleteBuffers,
		mixSDL_IsBuffer,
		mixSDL_GetBufferi,
		mixSDL_BufferData
};

/* Adapted from SDL
 * This will generate "negative subscript or subscript is too large"
 * error during compile, if the size of a type is wrong
 */
#define UQM_COMPILE_TIME_ASSERT(name, x) \
	typedef int UQM_dummy_##name[(x) * 2 - 1]

UQM_COMPILE_TIME_ASSERT(mixer_Object_fits_in_audio_Object,
						sizeof(mixer_Object) <= sizeof(audio_Object));

#undef UQM_COMPILE_TIME_ASSERT

// Converts an array of n audio_Objects to an array of mixer_Objects, in place.
static void
mixSDL_ConvertObjectArrayToMixerObjects(uint32_t n, audio_Object* arr)
{
	uint32_t i;
	if (sizeof(audio_Object) == sizeof(mixer_Object))
	{
		return;
	}
	for (i = 0; i < n; i++)
	{
		((mixer_Object*)arr)[i] = arr[i];
	}
}
// Converts an array of n mixer_Objects to an array of audio_Objects, in place.
static void
mixSDL_ConvertObjectArrayFromMixerObjects(uint32_t n, audio_Object* arr)
{
	uint32_t i;
	if (sizeof(audio_Object) == sizeof(mixer_Object))
	{
		return;
	}
	i = n;
	while (i--)
	{
		arr[i] = ((mixer_Object*)arr)[i];
	}
}

static void audioCallback(void* userdata, SDL_AudioStream* audioStream, int additionalAmount, int totalAmount);

/*
 * Initialization
 */

int32_t
mixSDL_Init(audio_Driver* driver, AudioFlags flags)
{
	int i;
	SDL_AudioSpec desired {};
	mixer_Quality quality;
	TFB_DecoderFormats formats =
		{
			MIX_IS_BIG_ENDIAN, MIX_WANT_BIG_ENDIAN,
			audio_FORMAT_MONO8, audio_FORMAT_STEREO8,
			audio_FORMAT_MONO16, audio_FORMAT_STEREO16};

	uqm::log::info("Initializing SDL audio subsystem.");
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
	{
		uqm::log::error("Couldn't initialize audio subsystem: {}",
						SDL_GetError());
		return -1;
	}
	uqm::log::info("SDL audio subsystem initialized.");

	if (testFlag(flags, AudioFlags::QualityHigh))
	{
		quality = MIX_QUALITY_HIGH;
		desired.freq = 44100;
	}
	else if (testFlag(flags, AudioFlags::QualityLow))
	{
		quality = MIX_QUALITY_LOW;
		desired.freq = 22050;
	}
	else
	{
		quality = MIX_QUALITY_DEFAULT;
		desired.freq = 44100;
	}

	desired.format = SDL_AUDIO_S16;
	desired.channels = 2;

	uqm::log::info("Opening SDL audio device.");
	// The stream converts the mixer output to the format of the device.
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, audioCallback, nullptr);
	if (!stream)
	{
		uqm::log::error("Unable to open audio device: {}",
						SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return -1;
	}

	{
		SDL_AudioSpec deviceSpec {};
		int deviceSamples {};
		SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &deviceSpec, &deviceSamples);
		uqm::log::info("    using {} at {} Hz 16 bit {}, "
					   "{} samples audio buffer (device: {} Hz, {} channels)",
					   SDL_GetCurrentAudioDriver(), desired.freq,
					   desired.channels > 1 ? "stereo" : "mono",
					   deviceSamples, deviceSpec.freq, deviceSpec.channels);
	}

	uqm::log::info("Initializing mixer.");
	if (!mixer_Init(desired.freq, MIX_FORMAT_MAKE(2, desired.channels),
					quality, MIX_NOFLAGS))
	{
		uqm::log::error("Mixer initialization failed: {:x}",
						mixer_GetError());
		SDL_DestroyAudioStream(stream);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return -1;
	}
	uqm::log::info("Mixer initialized.");

	uqm::log::info("Initializing sound decoders.");
	if (SoundDecoder_Init(flags, &formats))
	{
		uqm::log::error("Sound decoders initialization failed.");
		SDL_DestroyAudioStream(stream);
		mixer_Uninit();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return -1;
	}
	uqm::log::info("Sound decoders initialized.");

	*driver = mixSDL_Driver;
	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		audio_GenSources(1, &soundSource[i].handle);
	}

	if (InitStreamDecoder())
	{
		uqm::log::error("Stream decoder initialization failed.");
		// TODO: cleanup source mutexes [or is it "muti"? :) ]
		SDL_DestroyAudioStream(stream);
		SoundDecoder_Uninit();
		mixer_Uninit();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return -1;
	}

	SDL_ResumeAudioStreamDevice(stream);

	return 0;
}

void mixSDL_Uninit(void)
{
	int i;

	UninitStreamDecoder();

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		const uqm::LockGuard lock {soundSource[i].stream_mutex};
		if (soundSource[i].sample && soundSource[i].sample->decoder)
		{
			StopStream(i);
		}
		soundSource[i].sbuffer = {};

		mixSDL_DeleteSources(1, &soundSource[i].handle);
	}

	SDL_DestroyAudioStream(stream);
	mixer_Uninit();
	SoundDecoder_Uninit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

static void
audioCallback(void* userdata, SDL_AudioStream* audioStream, int additionalAmount, int totalAmount)
{
	(void)totalAmount;
	if (additionalAmount <= 0)
	{
		return;
	}
	s_mixBuffer.resize(additionalAmount);
	mixer_MixChannels(userdata, s_mixBuffer.data(), additionalAmount);
	SDL_PutAudioStreamData(audioStream, s_mixBuffer.data(), additionalAmount);
}

/*
 * General
 */

int32_t
mixSDL_GetError(void)
{
	int32_t value = mixer_GetError();
	switch (value)
	{
		case MIX_NO_ERROR:
			return audio_NO_ERROR;
		case MIX_INVALID_NAME:
			return audio_INVALID_NAME;
		case MIX_INVALID_ENUM:
			return audio_INVALID_ENUM;
		case MIX_INVALID_VALUE:
			return audio_INVALID_VALUE;
		case MIX_INVALID_OPERATION:
			return audio_INVALID_OPERATION;
		case MIX_OUT_OF_MEMORY:
			return audio_OUT_OF_MEMORY;
		default:
			uqm::log::debug("mixSDL_GetError: unknown value {:x}", value);
			return audio_DRIVER_FAILURE;
			break;
	}
}


/*
 * Sources
 */

void mixSDL_GenSources(uint32_t n, audio_Object* psrcobj)
{
	mixer_GenSources(n, (mixer_Object*)psrcobj);
	mixSDL_ConvertObjectArrayFromMixerObjects(n, psrcobj);
}

void mixSDL_DeleteSources(uint32_t n, audio_Object* psrcobj)
{
	mixSDL_ConvertObjectArrayToMixerObjects(n, psrcobj);
	mixer_DeleteSources(n, (mixer_Object*)psrcobj);
	mixSDL_ConvertObjectArrayFromMixerObjects(n, psrcobj);
}

bool mixSDL_IsSource(audio_Object srcobj)
{
	return mixer_IsSource((mixer_Object)srcobj);
}

void mixSDL_Sourcei(audio_Object srcobj, audio_SourceProp pname,
					audio_IntVal value)

{
	mixer_Sourcei((mixer_Object)srcobj, (mixer_SourceProp)pname,
				  (mixer_IntVal)value);
}

void mixSDL_Sourcef(audio_Object srcobj, audio_SourceProp pname,
					float value)
{
	mixer_Sourcef((mixer_Object)srcobj, (mixer_SourceProp)pname, value);
}

void mixSDL_Sourcefv(audio_Object srcobj, audio_SourceProp pname,
					 float* value)
{
	mixer_Sourcefv((mixer_Object)srcobj, (mixer_SourceProp)pname, value);
}

void mixSDL_GetSourcei(audio_Object srcobj, audio_SourceProp pname,
					   audio_IntVal* value)
{
	mixer_IntVal temp = *value;
	mixer_GetSourcei((mixer_Object)srcobj, (mixer_SourceProp)pname,
					 &temp);
	*value = temp;
	if (pname == MIX_SOURCE_STATE)
	{
		switch (*value)
		{
			case MIX_INITIAL:
				*value = audio_INITIAL;
				break;
			case MIX_STOPPED:
				*value = audio_STOPPED;
				break;
			case MIX_PLAYING:
				*value = audio_PLAYING;
				break;
			case MIX_PAUSED:
				*value = audio_PAUSED;
				break;
			default:
				*value = audio_DRIVER_FAILURE;
		}
	}
}

void mixSDL_GetSourcef(audio_Object srcobj, audio_SourceProp pname,
					   float* value)
{
	mixer_GetSourcef((mixer_Object)srcobj, (mixer_SourceProp)pname, value);
}

void mixSDL_SourceRewind(audio_Object srcobj)
{
	mixer_SourceRewind((mixer_Object)srcobj);
}

void mixSDL_SourcePlay(audio_Object srcobj)
{
	mixer_SourcePlay((mixer_Object)srcobj);
}

void mixSDL_SourcePause(audio_Object srcobj)
{
	mixer_SourcePause((mixer_Object)srcobj);
}

void mixSDL_SourceStop(audio_Object srcobj)
{
	mixer_SourceStop((mixer_Object)srcobj);
}

void mixSDL_SourceQueueBuffers(audio_Object srcobj, uint32_t n,
							   audio_Object* pbufobj)
{
	mixSDL_ConvertObjectArrayToMixerObjects(n, pbufobj);
	mixer_SourceQueueBuffers((mixer_Object)srcobj, n,
							 (mixer_Object*)pbufobj);
	mixSDL_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}

void mixSDL_SourceUnqueueBuffers(audio_Object srcobj, uint32_t n,
								 audio_Object* pbufobj)
{
	mixSDL_ConvertObjectArrayToMixerObjects(n, pbufobj);
	mixer_SourceUnqueueBuffers((mixer_Object)srcobj, n,
							   (mixer_Object*)pbufobj);
	mixSDL_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}


/*
 * Buffers
 */

void mixSDL_GenBuffers(uint32_t n, audio_Object* pbufobj)
{
	mixer_GenBuffers(n, (mixer_Object*)pbufobj);
	mixSDL_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}

void mixSDL_DeleteBuffers(uint32_t n, audio_Object* pbufobj)
{
	mixSDL_ConvertObjectArrayToMixerObjects(n, pbufobj);
	mixer_DeleteBuffers(n, (mixer_Object*)pbufobj);
	mixSDL_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}

bool mixSDL_IsBuffer(audio_Object bufobj)
{
	return mixer_IsBuffer((mixer_Object)bufobj);
}

void mixSDL_GetBufferi(audio_Object bufobj, audio_BufferProp pname,
					   audio_IntVal* value)
{
	mixer_IntVal temp = *value;
	mixer_GetBufferi((mixer_Object)bufobj, (mixer_BufferProp)pname,
					 &temp);
	*value = temp;
}

void mixSDL_BufferData(audio_Object bufobj, uint32_t format, void* data,
					   uint32_t size, uint32_t freq)
{
	mixer_BufferData((mixer_Object)bufobj, format, data, size, freq);
}
