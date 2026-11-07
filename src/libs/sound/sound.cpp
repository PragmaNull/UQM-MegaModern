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

#include "sound.h"

#include "libs/inplib.h"
#include "libs/sound/stream.h"
#include "sndintrn.h"


int musicVolume = NormalVolume;
float musicVolumeScale;
float sfxVolumeScale;
float speechVolumeScale;
uqstl::array<tfb::SoundSource, NumSoundsources> soundSource;


void StopSound(void)
{
	for (int i {FirstSfxSource}; i <= LastSfxSource; ++i)
	{
		StopSource(i);
	}
}

void CleanSource(int iSource)
{
	audio_IntVal processed {};

	soundSource[iSource].positional_object = nullptr;
	audio_GetSourcei(soundSource[iSource].handle,
					 AudioEnum::BuffersProcessed, &processed);
	if (processed != 0)
	{
		uqstl::vector<audio_Object> bufs(processed);
		audio_SourceUnqueueBuffers(soundSource[iSource].handle,
								   processed, bufs.data());
	}
	// set the source state to 'initial'
	audio_SourceRewind(soundSource[iSource].handle);
}

void StopSource(int iSource)
{
	audio_SourceStop(soundSource[iSource].handle);
	CleanSource(iSource);
}

bool SoundPlaying(void)
{
	for (int i {0}; i < NumSoundsources; ++i)
	{
		const tfb::SoundSample* sample {};
		sample = soundSource[i].sample;
		if (sample && sample->decoder)
		{
			soundSource[i].stream_mutex.lock();
			const bool result {PlayingStream(i)};
			soundSource[i].stream_mutex.unlock();
			if (result)
			{
				return true;
			}
		}
		else
		{
			audio_IntVal state {};
			audio_GetSourcei(soundSource[i].handle, AudioEnum::SourceState, &state);
			if (state == AudioEnum::Playing)
			{
				return true;
			}
		}
	}

	return false;
}

// for now just spin in a sleep() loop
// perhaps later change to condvar implementation
void WaitForSoundEnd(uint16_t Channel)
{
	while (Channel == TfbsoundWaitAll ?
			   SoundPlaying() :
			   ChannelPlaying(Channel))
	{
		SleepThread(getTicksForFramerate(20));
		if (QuitPosted) // Don't make users wait for sounds to end
		{
			break;
		}
	}
}


// Status: Ignored
bool InitSound(int argc, char* argv[])
{
	/* Quell compiler warnings */
	(void)argc;
	(void)argv;
	return true;
}

// Status: Ignored
void UninitSound(void)
{
}

void SetSFXVolume(float volume)
{
	for (int i {FirstSfxSource}; i <= LastSfxSource; ++i)
	{
		audio_Sourcef(soundSource[i].handle, AudioEnum::Gain, volume);
	}
}

void SetSpeechVolume(float volume)
{
	audio_Sourcef(soundSource[SpeechSource].handle, AudioEnum::Gain, volume);
}

TimeCount FadeMusic(uint8_t end_vol, TimePeriod TimeInterval)
{
	if (QuitPosted || TimeInterval < TimePeriod {}) // Don't make users wait for fades
	{
		TimeInterval = TimePeriod {};
	}

	if (!SetMusicStreamFade(TimeInterval, end_vol))
	{ // fade rejected, maybe due to TimeInterval==0
		SetMusicVolume(end_vol);
		return GetTimeCounter();
	}
	else
	{
		return GetTimeCounter() + TimeInterval + TimePeriod {1};
	}
}

uint8_t GetCurrMusicVol(void)
{
	return (uint8_t)musicVolume;
}
