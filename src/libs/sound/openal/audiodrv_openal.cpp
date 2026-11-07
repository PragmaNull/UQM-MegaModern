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

/* OpenAL audio driver
 */

#ifdef HAVE_OPENAL


#include "audiodrv_openal.h"

#include <stdlib.h>

#include "../sndintrn.h"
#include "core/log/log.h"
#include "libs/sound/stream.h"


ALCcontext* alcContext = nullptr;
ALCdevice* alcDevice = nullptr;
uqstl::array<ALfloat, 3> defaultPos {0.0f, 0.0f, -1.0f};
uqstl::array<ALfloat, 3> listenerPos {0.0f, 0.0f, 0.0f};
uqstl::array<ALfloat, 3> listenerVel {0.0f, 0.0f, 0.0f};
uqstl::array<ALfloat, 6> listenerOri {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};

const static audio_Driver openAL_Driver =
	{
		openAL_Uninit,
		openAL_GetError,
		AudioDriverType::OpenAL,
		{/* Errors */
		 AL_FALSE,
				  AL_INVALID_NAME,
				  AL_INVALID_ENUM,
				  AL_INVALID_VALUE,
				  AL_INVALID_OPERATION,
				  AL_OUT_OF_MEMORY,
				  *AudioEnum::DriverFailure,

				  /* Source properties */
		 AL_POSITION,
				  AL_LOOPING,
				  AL_BUFFER,
				  AL_GAIN,
				  AL_SOURCE_STATE,
				  AL_BUFFERS_QUEUED,
				  AL_BUFFERS_PROCESSED,

				  /* Source state information */
		 AL_INITIAL,
				  AL_STOPPED,
				  AL_PLAYING,
				  AL_PAUSED,

				  /* Sound buffer properties */
		 AL_FREQUENCY,
				  AL_BITS,
				  AL_CHANNELS,
				  AL_SIZE,
				  AL_FORMAT_MONO16,
				  AL_FORMAT_STEREO16,
				  AL_FORMAT_MONO8,
				  AL_FORMAT_STEREO8},

		/* Sources */
		openAL_GenSources,
		openAL_DeleteSources,
		openAL_IsSource,
		openAL_Sourcei,
		openAL_Sourcef,
		openAL_Sourcefv,
		openAL_GetSourcei,
		openAL_GetSourcef,
		openAL_SourceRewind,
		openAL_SourcePlay,
		openAL_SourcePause,
		openAL_SourceStop,
		openAL_SourceQueueBuffers,
		openAL_SourceUnqueueBuffers,

		/* Buffers */
		openAL_GenBuffers,
		openAL_DeleteBuffers,
		openAL_IsBuffer,
		openAL_GetBufferi,
		openAL_BufferData
};

static_assert(sizeof(ALuint) <= sizeof(audio_Object), "ALuint must fit in audio_Object");

// Converts an array of n audio_Objects to an array of ALuints, in place.
static void openAL_ConvertObjectArrayToALuints(uint32_t n, audio_Object* arr)
{
	if (sizeof(audio_Object) == sizeof(ALuint))
	{
		return;
	}
	for (uint32_t i {0}; i < n; i++)
	{
		((ALuint*)arr)[i] = arr[i];
	}
}
// Converts an array of n ALuints to an array of audio_Objects, in place.
static void openAL_ConvertObjectArrayFromALuints(uint32_t n, audio_Object* arr)
{
	if (sizeof(audio_Object) == sizeof(ALuint))
	{
		return;
	}
	uint32_t i {n};
	while (i--)
	{
		arr[i] = ((ALuint*)arr)[i];
	}
}

/*
 * Initialization
 */

int32_t openAL_Init(audio_Driver* driver, AudioFlags flags)
{
	tfb::DecoderFormats formats =
		{
			uqstl::endian::native == uqstl::endian::big, uqstl::endian::native == uqstl::endian::big,
			*AudioEnum::FormatMono8, *AudioEnum::FormatStereo8,
			*AudioEnum::FormatMono16, *AudioEnum::FormatStereo16};

	uqm::log::info("Initializing OpenAL.");
	alcDevice = alcOpenDevice(nullptr);

	if (!alcDevice)
	{
		uqm::log::error("Couldn't initialize OpenAL: {}",
						alcGetError(nullptr));
		return -1;
	}

	*driver = openAL_Driver;

	alcContext = alcCreateContext(alcDevice, nullptr);
	if (!alcContext)
	{
		uqm::log::error("Couldn't create OpenAL context: {}",
						alcGetError(alcDevice));
		alcCloseDevice(alcDevice);
		alcDevice = nullptr;
		return -1;
	}

	alcMakeContextCurrent(alcContext);

	uqm::log::info("OpenAL initialized.\n"
				   "    version:     {}\n"
				   "    vendor:      {}\n"
				   "    renderer:    {}\n"
				   "    device:      {}",
				   alGetString(AL_VERSION), alGetString(AL_VENDOR),
				   alGetString(AL_RENDERER),
				   alcGetString(alcDevice, ALC_DEFAULT_DEVICE_SPECIFIER));
	//log_add (log_Info, "    extensions:  {}", alGetString (AL_EXTENSIONS));

	uqm::log::info("Initializing sound decoders.");
	if (SoundDecoder_Init(flags, &formats))
	{
		uqm::log::error("Sound decoders initialization failed.");
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(alcContext);
		alcContext = nullptr;
		alcCloseDevice(alcDevice);
		alcDevice = nullptr;
		return -1;
	}
	uqm::log::info("Sound decoders initialized.");

	alListenerfv(AL_POSITION, listenerPos.data());
	alListenerfv(AL_VELOCITY, listenerVel.data());
	alListenerfv(AL_ORIENTATION, listenerOri.data());

	for (auto& source : soundSource)
	{
		uqstl::array<float, 3> zero {0.0f, 0.0f, 0.0f};

		audio_GenSources(1, &source.handle);
		alSourcei(source.handle, AL_LOOPING, AL_FALSE);
		alSourcefv(source.handle, AL_POSITION, defaultPos.data());
		alSourcefv(source.handle, AL_VELOCITY, zero.data());
		alSourcefv(source.handle, AL_DIRECTION, zero.data());
	}

	if (InitStreamDecoder())
	{
		uqm::log::error("Stream decoder initialization failed.");
		// TODO: cleanup source mutexes [or is it "muti"? :) ]
		SoundDecoder_Uninit();
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(alcContext);
		alcContext = nullptr;
		alcCloseDevice(alcDevice);
		alcDevice = nullptr;
		return -1;
	}

	alDistanceModel(AL_INVERSE_DISTANCE);

	(void)driver; // eat compiler warning

	return 0;
}

void openAL_Uninit(void)
{
	UninitStreamDecoder();

	for (int i {0}; i < NumSoundsources; ++i)
	{
		const uqm::LockGuard lock {soundSource[i].stream_mutex};
		if (soundSource[i].sample && soundSource[i].sample->decoder)
		{
			StopStream(i);
		}
		soundSource[i].sbuffer = {};
	}

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(alcContext);
	alcContext = nullptr;
	alcCloseDevice(alcDevice);
	alcDevice = nullptr;

	SoundDecoder_Uninit();
}


/*
 * General
 */

int32_t openAL_GetError(void)
{
	ALint value = alGetError();
	switch (value)
	{
		case AL_FALSE:
			return *AudioEnum::NoError;
		case AL_INVALID_NAME:
			return *AudioEnum::InvalidName;
		case AL_INVALID_ENUM:
			return *AudioEnum::InvalidEnum;
		case AL_INVALID_VALUE:
			return *AudioEnum::InvalidValue;
		case AL_INVALID_OPERATION:
			return *AudioEnum::InvalidOperation;
		case AL_OUT_OF_MEMORY:
			return *AudioEnum::OutOfMemory;
		default:
			uqm::log::debug("openAL_GetError: unknown value {:x}", value);
			return *AudioEnum::DriverFailure;
			break;
	}
}


/*
 * Sources
 */

void openAL_GenSources(uint32_t n, audio_Object* psrcobj)
{
	alGenSources((ALsizei)n, (ALuint*)psrcobj);
	openAL_ConvertObjectArrayFromALuints(n, psrcobj);
}

void openAL_DeleteSources(uint32_t n, audio_Object* psrcobj)
{
	openAL_ConvertObjectArrayToALuints(n, psrcobj);
	alDeleteSources((ALsizei)n, (ALuint*)psrcobj);
	openAL_ConvertObjectArrayFromALuints(n, psrcobj);
}

bool openAL_IsSource(audio_Object srcobj)
{
	return alIsSource((ALuint)srcobj);
}

void openAL_Sourcei(audio_Object srcobj, int32_t pname,
					audio_IntVal value)

{
	alSourcei((ALuint)srcobj, (ALenum)pname, (ALint)value);
}

void openAL_Sourcef(audio_Object srcobj, int32_t pname,
					float value)
{
	alSourcef((ALuint)srcobj, (ALenum)pname, (ALfloat)value);
}

void openAL_Sourcefv(audio_Object srcobj, int32_t pname,
					 float* value)
{
	alSourcefv((ALuint)srcobj, (ALenum)pname, (ALfloat*)value);
}

void openAL_GetSourcei(audio_Object srcobj, int32_t pname,
					   audio_IntVal* value)
{
	ALint temp = *value;
	alGetSourcei((ALuint)srcobj, (ALenum)pname, &temp);
	*value = temp;
	if (pname == AL_SOURCE_STATE)
	{
		switch (*value)
		{
			case AL_INITIAL:
				*value = *AudioEnum::Initial;
				break;
			case AL_STOPPED:
				*value = *AudioEnum::Stopped;
				break;
			case AL_PLAYING:
				*value = *AudioEnum::Playing;
				break;
			case AL_PAUSED:
				*value = *AudioEnum::Paused;
				break;
			default:
				uqm::log::debug("openAL_GetSourcei(): unknown value {:x}",
								*value);
				*value = *AudioEnum::DriverFailure;
		}
	}
}

void openAL_GetSourcef(audio_Object srcobj, int32_t pname,
					   float* value)
{
	alGetSourcef((ALuint)srcobj, (ALenum)pname, (ALfloat*)value);
}

void openAL_SourceRewind(audio_Object srcobj)
{
	alSourceRewind((ALuint)srcobj);
}

void openAL_SourcePlay(audio_Object srcobj)
{
	alSourcePlay((ALuint)srcobj);
}

void openAL_SourcePause(audio_Object srcobj)
{
	alSourcePause((ALuint)srcobj);
}

void openAL_SourceStop(audio_Object srcobj)
{
	alSourceStop((ALuint)srcobj);
}

void openAL_SourceQueueBuffers(audio_Object srcobj, uint32_t n,
							   audio_Object* pbufobj)
{
	openAL_ConvertObjectArrayToALuints(n, pbufobj);
	alSourceQueueBuffers((ALuint)srcobj, (ALsizei)n, (ALuint*)pbufobj);
	openAL_ConvertObjectArrayFromALuints(n, pbufobj);
}

void openAL_SourceUnqueueBuffers(audio_Object srcobj, uint32_t n,
								 audio_Object* pbufobj)
{
	openAL_ConvertObjectArrayToALuints(n, pbufobj);
	alSourceUnqueueBuffers((ALuint)srcobj, (ALsizei)n, (ALuint*)pbufobj);
	openAL_ConvertObjectArrayFromALuints(n, pbufobj);
}


/*
 * Buffers
 */

void openAL_GenBuffers(uint32_t n, audio_Object* pbufobj)
{
	alGenBuffers((ALsizei)n, (ALuint*)pbufobj);
	openAL_ConvertObjectArrayFromALuints(n, pbufobj);
}

void openAL_DeleteBuffers(uint32_t n, audio_Object* pbufobj)
{
	openAL_ConvertObjectArrayToALuints(n, pbufobj);
	alDeleteBuffers((ALsizei)n, (ALuint*)pbufobj);
	openAL_ConvertObjectArrayFromALuints(n, pbufobj);
}

bool openAL_IsBuffer(audio_Object bufobj)
{
	return alIsBuffer((ALuint)bufobj);
}

void openAL_GetBufferi(audio_Object bufobj, int32_t pname,
					   audio_IntVal* value)
{
	ALint temp = *value;
	alGetBufferi((ALuint)bufobj, (ALenum)pname, &temp);
	*value = temp;
}

void openAL_BufferData(audio_Object bufobj, uint32_t format, uqstl::span<const uint8_t> data, uint32_t freq)
{
	alBufferData(static_cast<ALuint>(bufobj), static_cast<ALenum>(format), data.data(), static_cast<ALsizei>(data.size()), static_cast<ALsizei>(freq));
}

#endif
