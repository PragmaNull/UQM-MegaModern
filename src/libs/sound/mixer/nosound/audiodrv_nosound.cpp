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

/* Nosound audio driver
 */

#include "audiodrv_nosound.h"

#include <stdlib.h>

#include "../../sndintrn.h"
#include "core/log/log.h"
#include "core/thread/Thread.h"
#include "libs/sound/stream.h"
#include "libs/time/timelib.h"


static uqstl::jthread s_playbackThread {};
static uint32_t nosound_freq = 22050;

const static audio_Driver noSound_Driver =
	{
		noSound_Uninit,
		noSound_GetError,
		AudioDriverType::NoSound,
		{/* Errors */
		 *MixerError::NoError,
				   *MixerError::InvalidName,
				   *MixerError::InvalidEnum,
				   *MixerError::InvalidValue,
				   *MixerError::InvalidOperation,
				   *MixerError::OutOfMemory,
				   *MixerError::DriverFailure,

				   /* Source properties */
		 *MixerSourceProp::Position,
				   *MixerSourceProp::Looping,
				   *MixerSourceProp::Buffer,
				   *MixerSourceProp::Gain,
				   *MixerSourceProp::SourceState,
				   *MixerSourceProp::BuffersQueued,
				   *MixerSourceProp::BuffersProcessed,

				   /* Source state information */
		 *MixerSourceState::Initial,
				   *MixerSourceState::Stopped,
				   *MixerSourceState::Playing,
				   *MixerSourceState::Paused,

				   /* Sound buffer properties */
		 *MixerBufferProp::Frequency,
				   *MixerBufferProp::Bits,
				   *MixerBufferProp::Channels,
				   *MixerBufferProp::Size,
				   *MixerFormat::Mono16,
				   *MixerFormat::Stereo16,
				   *MixerFormat::Mono8,
				   *MixerFormat::Stereo8},

		/* Sources */
		noSound_GenSources,
		noSound_DeleteSources,
		noSound_IsSource,
		noSound_Sourcei,
		noSound_Sourcef,
		noSound_Sourcefv,
		noSound_GetSourcei,
		noSound_GetSourcef,
		noSound_SourceRewind,
		noSound_SourcePlay,
		noSound_SourcePause,
		noSound_SourceStop,
		noSound_SourceQueueBuffers,
		noSound_SourceUnqueueBuffers,

		/* Buffers */
		noSound_GenBuffers,
		noSound_DeleteBuffers,
		noSound_IsBuffer,
		noSound_GetBufferi,
		noSound_BufferData
};

static_assert(sizeof(mixer_Object) <= sizeof(audio_Object), "mixer_Object must fit in audio_Object");

// Converts an array of n audio_Objects to an array of mixer_Objects, in place.
static void noSound_ConvertObjectArrayToMixerObjects(uint32_t n, audio_Object* arr)
{
	if (sizeof(audio_Object) == sizeof(mixer_Object))
	{
		return;
	}
	for (uint32_t i {0}; i < n; i++)
	{
		((mixer_Object*)arr)[i] = arr[i];
	}
}
// Converts an array of n mixer_Objects to an array of audio_Objects, in place.
static void noSound_ConvertObjectArrayFromMixerObjects(uint32_t n, audio_Object* arr)
{
	if (sizeof(audio_Object) == sizeof(mixer_Object))
	{
		return;
	}
	uint32_t i {n};
	while (i--)
	{
		arr[i] = ((mixer_Object*)arr)[i];
	}
}

/*
 * Initialization
 */

int32_t noSound_Init(audio_Driver* driver, AudioFlags flags)
{
	tfb::DecoderFormats formats =
		{
			0, 0,
			*AudioEnum::FormatMono8, *AudioEnum::FormatStereo8,
			*AudioEnum::FormatMono16, *AudioEnum::FormatStereo16};

	uqm::log::info("Using nosound audio driver.");
	uqm::log::info("Initializing mixer.");

	if (!mixer_Init(nosound_freq, MixFormatMake(1, 1),
					MixerQuality::Low, MixerFlags::FakeData))
	{
		uqm::log::error("Mixer initialization failed: {:x}",
						mixer_GetError());
		return -1;
	}
	uqm::log::info("Mixer initialized.");

	uqm::log::info("Initializing sound decoders.");
	if (SoundDecoder_Init(flags, &formats))
	{
		uqm::log::error("Sound decoders initialization failed.");
		mixer_Uninit();
		return -1;
	}
	uqm::log::info("Sound decoders initialized.");

	*driver = noSound_Driver;
	for (int i {0}; i < NumSoundsources; ++i)
	{
		audio_GenSources(1, &soundSource[i].handle);
	}

	if (InitStreamDecoder())
	{
		uqm::log::error("Stream decoder initialization failed.");
		// TODO: cleanup source mutexes [or is it "muti"? :) ]
		SoundDecoder_Uninit();
		mixer_Uninit();
		return -1;
	}

	s_playbackThread = uqm::makeThread("nosound audio playback", PlaybackTaskFunc);

	return 0;
}

void noSound_Uninit(void)
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

		noSound_DeleteSources(1, &soundSource[i].handle);
	}

	s_playbackThread = {};

	mixer_Uninit();
	SoundDecoder_Uninit();
}


/*
 * Playback task
 */

void PlaybackTaskFunc(uqstl::stop_token stopToken)
{
	const uint32_t len = 2048;
	uqstl::vector<uint8_t> stream(len);
	const TimePeriod period {uqstl::chrono::duration_cast<TimePeriod>(uqstl::chrono::duration<double> {len / (double)nosound_freq})};

	while (!stopToken.stop_requested())
	{
		const TimeCount entryTime {GetTimeCounter()};
		mixer_MixFake(nullptr, stream.data(), len);
		const TimePeriod delay {period - (GetTimeCounter() - entryTime)};
		if (delay > TimePeriod {})
		{
			HibernateThread(delay);
		}
	}
}


/*
 * General
 */

int32_t noSound_GetError(void)
{
	const MixerError value {mixer_GetError()};
	switch (value)
	{
		case MixerError::NoError:
			return *AudioEnum::NoError;
		case MixerError::InvalidName:
			return *AudioEnum::InvalidName;
		case MixerError::InvalidEnum:
			return *AudioEnum::InvalidEnum;
		case MixerError::InvalidValue:
			return *AudioEnum::InvalidValue;
		case MixerError::InvalidOperation:
			return *AudioEnum::InvalidOperation;
		case MixerError::OutOfMemory:
			return *AudioEnum::OutOfMemory;
		default:
			uqm::log::debug("noSound_GetError: unknown value {:x}",
							value);
			return *AudioEnum::DriverFailure;
			break;
	}
}


/*
 * Sources
 */

void noSound_GenSources(uint32_t n, audio_Object* psrcobj)
{
	mixer_GenSources(n, (mixer_Object*)psrcobj);
	noSound_ConvertObjectArrayFromMixerObjects(n, psrcobj);
}

void noSound_DeleteSources(uint32_t n, audio_Object* psrcobj)
{
	noSound_ConvertObjectArrayToMixerObjects(n, psrcobj);
	mixer_DeleteSources(n, (mixer_Object*)psrcobj);
	noSound_ConvertObjectArrayFromMixerObjects(n, psrcobj);
}

bool noSound_IsSource(audio_Object srcobj)
{
	return mixer_IsSource((mixer_Object)srcobj);
}

void noSound_Sourcei(audio_Object srcobj, int32_t pname,
					 audio_IntVal value)

{
	mixer_Sourcei((mixer_Object)srcobj, (MixerSourceProp)pname,
				  (mixer_IntVal)value);
}

void noSound_Sourcef(audio_Object srcobj, int32_t pname,
					 float value)
{
	mixer_Sourcef((mixer_Object)srcobj, (MixerSourceProp)pname, value);
}

void noSound_Sourcefv(audio_Object srcobj, int32_t pname,
					  float* value)
{
	mixer_Sourcefv((mixer_Object)srcobj, (MixerSourceProp)pname, value);
}

void noSound_GetSourcei(audio_Object srcobj, int32_t pname,
						audio_IntVal* value)
{
	mixer_IntVal temp = *value;
	mixer_GetSourcei((mixer_Object)srcobj, (MixerSourceProp)pname,
					 &temp);
	*value = temp;
	if (pname == MixerSourceProp::SourceState)
	{
		switch (static_cast<MixerSourceState>(*value))
		{
			case MixerSourceState::Initial:
				*value = *AudioEnum::Initial;
				break;
			case MixerSourceState::Stopped:
				*value = *AudioEnum::Stopped;
				break;
			case MixerSourceState::Playing:
				*value = *AudioEnum::Playing;
				break;
			case MixerSourceState::Paused:
				*value = *AudioEnum::Paused;
				break;
			default:
				uqm::log::debug("noSound_GetSourcei(): unknown value %lx",
								(long int)*value);
				*value = *AudioEnum::DriverFailure;
		}
	}
}

void noSound_GetSourcef(audio_Object srcobj, int32_t pname,
						float* value)
{
	mixer_GetSourcef((mixer_Object)srcobj, (MixerSourceProp)pname, value);
}

void noSound_SourceRewind(audio_Object srcobj)
{
	mixer_SourceRewind((mixer_Object)srcobj);
}

void noSound_SourcePlay(audio_Object srcobj)
{
	mixer_SourcePlay((mixer_Object)srcobj);
}

void noSound_SourcePause(audio_Object srcobj)
{
	mixer_SourcePause((mixer_Object)srcobj);
}

void noSound_SourceStop(audio_Object srcobj)
{
	mixer_SourceStop((mixer_Object)srcobj);
}

void noSound_SourceQueueBuffers(audio_Object srcobj, uint32_t n,
								audio_Object* pbufobj)
{
	noSound_ConvertObjectArrayToMixerObjects(n, pbufobj);
	mixer_SourceQueueBuffers((mixer_Object)srcobj, n,
							 (mixer_Object*)pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}

void noSound_SourceUnqueueBuffers(audio_Object srcobj, uint32_t n,
								  audio_Object* pbufobj)
{
	noSound_ConvertObjectArrayToMixerObjects(n, pbufobj);
	mixer_SourceUnqueueBuffers((mixer_Object)srcobj, n,
							   (mixer_Object*)pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}


/*
 * Buffers
 */

void noSound_GenBuffers(uint32_t n, audio_Object* pbufobj)
{
	mixer_GenBuffers(n, (mixer_Object*)pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}

void noSound_DeleteBuffers(uint32_t n, audio_Object* pbufobj)
{
	noSound_ConvertObjectArrayToMixerObjects(n, pbufobj);
	mixer_DeleteBuffers(n, (mixer_Object*)pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects(n, pbufobj);
}

bool noSound_IsBuffer(audio_Object bufobj)
{
	return mixer_IsBuffer((mixer_Object)bufobj);
}

void noSound_GetBufferi(audio_Object bufobj, int32_t pname,
						audio_IntVal* value)
{
	mixer_IntVal temp = *value;
	mixer_GetBufferi((mixer_Object)bufobj, (MixerBufferProp)pname,
					 &temp);
	*value = temp;
}

void noSound_BufferData(audio_Object bufobj, uint32_t format, void* data,
						uint32_t size, uint32_t freq)
{
	mixer_BufferData((mixer_Object)bufobj, format, data, size, freq);
}
