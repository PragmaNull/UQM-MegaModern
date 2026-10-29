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

#include <string.h>
#include "libs/file.h"
#include "options.h"
#include "sound.h"
#include "sndintrn.h"
#include "libs/reslib.h"
#include "core/log/log.h"
#include "libs/memlib.h"
#include "uqm/globdata.h"
#include "uqm/setup.h"
#include "libs/mathlib.h"


static MUSIC_REF curMusicRef;
static MUSIC_REF curSpeechRef;
static MUSIC_POSITION resumeMusicArray[PATH_MAX];

void PLRPlaySong(MUSIC_REF MusicRef, bool Continuous, uint8_t Priority)
{
	TFB_SoundSample** pmus = MusicRef;

	if (pmus)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		// Always scope the music data, we may need it
		PlayStream((*pmus), MUSIC_SOURCE, Continuous, true, true);
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);

		curMusicRef = MusicRef;
	}

	(void)Priority; /* Satisfy compiler because of unused variable */
}

void PLRStop(MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		StopStream(MUSIC_SOURCE);
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);

		curMusicRef = 0;
	}
}

bool PLRPlaying(MUSIC_REF MusicRef)
{
	if (curMusicRef && (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0))
	{
		bool playing;

		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		playing = PlayingStream(MUSIC_SOURCE);
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);

		return playing;
	}

	return false;
}

void PLRSeek(MUSIC_REF MusicRef, uint32_t pos)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		SeekStream(MUSIC_SOURCE, pos);
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
	}
}

void PLRPause(MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		PauseStream(MUSIC_SOURCE);
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
	}
}

static uint32_t
get_current_music_pos(MUSIC_REF MusicRef)
{
	uint32_t pos = 0;
	float length = 0.0f;
	//char *filename;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return 0;
	}

	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);

		//filename = soundSource[MUSIC_SOURCE].sample->decoder->filename;

		if (IsTracker(MUSIC_SOURCE))
		{
			length = (float)GetNumTrackerPos(MUSIC_SOURCE);
			pos = GetStreamFrame(MUSIC_SOURCE);
		}
		else
		{
			length = GetStreamLength(MUSIC_SOURCE);
			pos = GetStreamTime(MUSIC_SOURCE);
		}

		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
	}
	else
	{
		return 0;
	}

	if (pos > (uint32_t)length)
	{
		pos = 0;
	}

	return pos;
}

uint32_t
PLRGetPos(void)
{
	return curMusicRef != 0 ? get_current_music_pos(curMusicRef) : 0;
}

static char*
get_current_music_filename(MUSIC_REF MusicRef)
{
	char* filename;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return 0;
	}

	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		filename = soundSource[MUSIC_SOURCE].sample->decoder->filename;
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);

		return filename;
	}
	else
	{
		return 0;
	}
}

char* PLRGetFilename(void)
{
	return curMusicRef != 0 ? get_current_music_filename(curMusicRef) : 0;
}

void PLRResume(MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		ResumeStream(MUSIC_SOURCE);
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
	}
}

static uint32_t
get_current_music_filename_hash(MUSIC_REF MusicRef)
{
	uint32_t filename_hash;

	if (GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return 0;
	}

	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		filename_hash = soundSource[MUSIC_SOURCE].sample->decoder->filename_hash;
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);

		return filename_hash;
	}
	else
	{
		return 0;
	}
}

uint32_t
PLRGetFilenameHash(void)
{
	return curMusicRef != 0 ? get_current_music_filename_hash(curMusicRef) : 0;
}

void snd_PlaySpeech(MUSIC_REF SpeechRef)
{
	TFB_SoundSample** pmus = SpeechRef;

	if (pmus)
	{
		LockMutex(soundSource[SPEECH_SOURCE].stream_mutex);
		// Do not need to scope the music-as-speech as of now
		PlayStream(*pmus, SPEECH_SOURCE, false, false, true);
		UnlockMutex(soundSource[SPEECH_SOURCE].stream_mutex);

		curSpeechRef = SpeechRef;
	}
}

void snd_StopSpeech(void)
{
	if (!curSpeechRef)
	{
		return;
	}

	LockMutex(soundSource[SPEECH_SOURCE].stream_mutex);
	StopStream(SPEECH_SOURCE);
	UnlockMutex(soundSource[SPEECH_SOURCE].stream_mutex);

	curSpeechRef = 0;
}

bool DestroyMusic(MUSIC_REF MusicRef)
{
	return _ReleaseMusicData(MusicRef);
}

void SetMusicVolume(uint16_t Volume)
{
	float f = (Volume / (float)MAX_VOLUME) * musicVolumeScale;
	musicVolume = Volume;
	audio_Sourcef(soundSource[MUSIC_SOURCE].handle, audio_GAIN, f);
}

char* CheckMusicResName(char* fileName)
{
	if (!uqm::contentDir().exists(fileName))
	{
		uqm::log::warn("Requested track '{}' not found.", fileName);
	}
	return fileName;
}

void* _GetMusicData(const ResourceFile& resource)
{
	MUSIC_REF h;
	TFB_SoundSample* sample;
	TFB_SoundDecoder* decoder;
	char filename[256] {};

	uqm::strncpy_safe(filename, {resource.path.data(), static_cast<uint32_t>(resource.path.size())});
	CheckMusicResName(filename);

	uqm::log::debug("_GetMusicData(): loading {}", filename);
	decoder = SoundDecoder_Load(uqm::contentDir(), filename, 4096, 0, 0);
	if (!decoder)
	{
		uqm::log::warn("_GetMusicData(): couldn't load {}", filename);
		return nullptr;
	}

	h = (MUSIC_REF)AllocMusicData(sizeof(void*));
	if (!h)
	{
		SoundDecoder_Free(decoder);
		return nullptr;
	}

	sample = TFB_CreateSoundSample(decoder, 64, nullptr);
	*h = sample;

	uqm::log::info("    decoder: {}, rate {} format {:x}",
				   SoundDecoder_GetName(sample->decoder),
				   sample->decoder->frequency, sample->decoder->format);

	return (h);
}

bool _ReleaseMusicData(void* data)
{
	TFB_SoundSample** pmus = (TFB_SoundSample**)data;
	TFB_SoundSample* sample;

	if (pmus == nullptr)
	{
		return (false);
	}

	sample = *pmus;
	assert(sample != 0);
	if (sample->decoder)
	{
		TFB_SoundDecoder* decoder = sample->decoder;
		LockMutex(soundSource[MUSIC_SOURCE].stream_mutex);
		if (soundSource[MUSIC_SOURCE].sample == sample)
		{ // Currently playing this sample! Not good.
			StopStream(MUSIC_SOURCE);
		}
		UnlockMutex(soundSource[MUSIC_SOURCE].stream_mutex);

		sample->decoder = nullptr;
		SoundDecoder_Free(decoder);
	}
	TFB_DestroySoundSample(sample);
	FreeMusicData(data);

	return (true);
}

// For music resume option
void SetMusicPosition(void)
{
	MUSIC_POSITION temp;
	int i;

	if (!uqm::UQMOptions::read().musicResume || GLOBAL(CurrentActivity) & CHECK_ABORT
		|| !PLRPlaying((MUSIC_REF)~0))
	{
		return;
	}

	temp.filename_hash = PLRGetFilenameHash();
	if (!temp.filename_hash)
	{
		return;
	}

	temp.position = PLRGetPos();
	temp.last_played = GetTimeCounter();

	for (i = 0; i < PATH_MAX; ++i)
	{
		// print_mp_array (resumeMusicArray, 9); // For debugging purposes

		if (resumeMusicArray[i].filename_hash == temp.filename_hash)
		{
			// Update existing entry
			resumeMusicArray[i].position = temp.position;
			resumeMusicArray[i].last_played = temp.last_played;
			return;
		}

		if (resumeMusicArray[i].filename_hash == 0)
		{
			// Found empty slot - insert and return
			resumeMusicArray[i].filename_hash = temp.filename_hash;
			resumeMusicArray[i].position = temp.position;
			resumeMusicArray[i].last_played = temp.last_played;
			return;
		}
	}
}

uint32_t
GetMusicPosition()
{
	uint32_t filename_hash;
	int i;

	if (!uqm::UQMOptions::read().musicResume || GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return 0;
	}

	filename_hash = PLRGetFilenameHash();
	if (!filename_hash)
	{
		return 0;
	}

	for (i = 0; i < PATH_MAX; ++i)
	{
		if (!resumeMusicArray[i].filename_hash)
		{
			return 0;
		}

		if (resumeMusicArray[i].filename_hash == filename_hash)
		{
			return resumeMusicArray[i].position;
		}
	}

	return 0; // Shouldn't happen, music array completely full
}

#define FIVE_MINUTES (1000 * 300)

bool OkayToResume(void)
{
	TimeCount TimeIn, difference;
	uint32_t filename_hash;
	int i;

	if (!uqm::UQMOptions::read().musicResume || GLOBAL(CurrentActivity) & CHECK_ABORT)
	{
		return false;
	}

	filename_hash = PLRGetFilenameHash();
	if (!filename_hash)
	{
		return false;
	}

	for (i = 0; i < PATH_MAX; ++i)
	{
		if (resumeMusicArray[i].filename_hash == filename_hash)
		{
			break;
		}
	}

	if (i == PATH_MAX)
	{
		return false;
	}

	if (!resumeMusicArray[i].last_played
		|| !resumeMusicArray[i].position)
	{
		return false;
	}

	TimeIn = GetTimeCounter();
	difference = TimeIn - resumeMusicArray[i].last_played;

	if (uqm::UQMOptions::read().musicResume == 2 || (difference < FIVE_MINUTES))
	{
		return true;
	}

	return false;
}

void ResetMusicResume(void)
{
	memset(&resumeMusicArray, 0, sizeof(resumeMusicArray));
}
