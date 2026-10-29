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

#include <math.h>
#include <scn/scan.h>

#include "options.h"
#include "sound.h"
#include "sndintrn.h"
#include "libs/io/Path.h"
#include "libs/reslib.h"
#include "core/log/log.h"
#include "libs/strlib.h"
// for GetStringAddress()
#include "libs/strings/strintrn.h"
// for AllocStringTable(), FreeStringTable()
#include "libs/memlib.h"


static void CheckFinishedChannels(void);

static const SoundPosition notPositional = {false, 0, 0};

void PlayChannel(uint16_t channel, SOUND snd, SoundPosition pos,
				 void* positional_object, unsigned char priority)
{
	SOUNDPTR snd_ptr = GetSoundAddress(snd);
	TFB_SoundSample* sample;

	StopSource(channel);
	// all finished (stopped) channels can be cleaned up at this point
	// since this is the only func that can initiate an sfx sound
	CheckFinishedChannels();

	if (!snd_ptr)
	{
		return; // nothing to play
	}

	sample = *(TFB_SoundSample**)snd_ptr;

	soundSource[channel].sample = sample;
	soundSource[channel].positional_object = positional_object;

	UpdateSoundPosition(channel, uqm::UQMOptions::read().stereoSFX ? pos : notPositional);

	audio_Sourcei(soundSource[channel].handle, audio_BUFFER,
				  sample->buffer[0]);
	audio_SourcePlay(soundSource[channel].handle);
	(void)priority;
}

void StopChannel(uint16_t channel, uint8_t Priority)
{
	StopSource(channel);
	(void)Priority; // ignored
}

static void
CheckFinishedChannels(void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		audio_IntVal state;

		audio_GetSourcei(soundSource[i].handle, audio_SOURCE_STATE,
						 &state);
		if (state == audio_STOPPED)
		{
			CleanSource(i);
			// and if it failed... we still dont care
			audio_GetError();
		}
	}
}

bool ChannelPlaying(uint16_t WhichChannel)
{
	audio_IntVal state;

	audio_GetSourcei(soundSource[WhichChannel].handle,
					 audio_SOURCE_STATE, &state);
	if (state == audio_PLAYING)
	{
		return true;
	}
	return false;
}

void* GetPositionalObject(uint16_t channel)
{
	return soundSource[channel].positional_object;
}

void SetPositionalObject(uint16_t channel, void* positional_object)
{
	soundSource[channel].positional_object = positional_object;
}

void UpdateSoundPosition(uint16_t channel, SoundPosition pos)
{
	const float ATTENUATION = 160.0f;
	const float MIN_DISTANCE = 0.5f;
	float fpos[3];

	if (pos.positional)
	{
		float dist;

		fpos[0] = pos.x / ATTENUATION;
		fpos[1] = 0.0f;
		fpos[2] = pos.y / ATTENUATION;
		dist = (float)sqrt(fpos[0] * fpos[0] + fpos[2] * fpos[2]);
		if (dist < MIN_DISTANCE)
		{ // object is too close to listener
			// move it away along the same vector
			float scale = MIN_DISTANCE / dist;
			fpos[0] *= scale;
			fpos[2] *= scale;
		}

		audio_Sourcefv(soundSource[channel].handle, audio_POSITION, fpos);
		//log_add (log_Debug, "UpdateSoundPosition(): channel {}, pos {} {}, posobj {:x}",
		//		channel, pos.x, pos.y, (unsigned int)soundSource[channel].positional_object);
	}
	else
	{
		fpos[0] = fpos[1] = 0.0f;
		fpos[2] = -1.0f;
		audio_Sourcefv(soundSource[channel].handle, audio_POSITION, fpos);
	}
}

void SetChannelVolume(uint16_t channel, uint16_t volume, uint8_t priority)
// I wonder what this whole priority business is...
// I can probably ignore it.
{
	audio_Sourcef(soundSource[channel].handle, audio_GAIN,
				  (volume / (float)MAX_VOLUME) * sfxVolumeScale);
	(void)priority; // ignored
}

void* _GetSoundBankData(const ResourceFile& resource)
{
	uqm::io::File* const fp {&resource.file};
	int snd_ct, n;
	char CurrentLine[1024] {};
	char filename[1024] {};
#define MAX_FX 256
	TFB_SoundSample* sndfx[MAX_FX];
	STRING_TABLE Snd;
	STRING str;
	int i;

	// The sound file names are relative to the directory of the bank.
	{
		const uqstl::string_view parent {uqm::io::path::parent(resource.path)};
		n = parent.empty() ? 0 : static_cast<int>(parent.size()) + 1;
		uqm::strncpy_safe(filename, {resource.path.data(), static_cast<uint32_t>(n)});
	}

	snd_ct = 0;
	while (fp->readLine(CurrentLine) && snd_ct < MAX_FX)
	{
		TFB_SoundSample* sample;
		TFB_SoundDecoder* decoder;
		uint32_t decoded_bytes;

		if (const auto result {scn::scan<std::string>(CurrentLine, "{}")})
		{
			const std::string parsed {result->value()};
			uqm::strncpy_safe({&filename[n], static_cast<uint32_t>(sizeof(filename) - n)}, parsed);
		}
		else
		{

			uqm::log::warn("_GetSoundBankData: bad line: '{}'",
						   CurrentLine);
			continue;
		}

		uqm::log::debug("_GetSoundBankData(): loading {}", filename);

		decoder = SoundDecoder_Load(uqm::contentDir(), filename, 4096, 0, 0);
		if (!decoder)
		{
			uqm::log::warn("_GetSoundBankData(): couldn't load {}",
						   filename);
			continue;
		}

		// SFX samples don't have decoders, everything is pre-decoded below
		sample = TFB_CreateSoundSample(nullptr, 1, nullptr);

		// Decode everything and stash it in 1 buffer
		decoded_bytes = SoundDecoder_DecodeAll(decoder);
		uqm::log::debug("_GetSoundBankData(): decoded bytes {}",
						decoded_bytes);

		audio_BufferData(sample->buffer[0], decoder->format,
						 decoder->buffer, decoded_bytes, decoder->frequency);
		// just for informational purposes
		sample->length = decoder->length;

		SoundDecoder_Free(decoder);

		sndfx[snd_ct] = sample;
		++snd_ct;
	}

	if (!snd_ct)
	{
		return nullptr; // no sounds decoded
	}

	Snd = AllocStringTable(snd_ct, 0);
	if (!Snd)
	{ // Oops, have to delete everything now
		while (snd_ct--)
		{
			TFB_DestroySoundSample(sndfx[snd_ct]);
		}

		return nullptr;
	}

	// Populate the STRING_TABLE with ptrs to sample
	for (i = 0, str = Snd->strings; i < snd_ct; ++i, ++str)
	{
		TFB_SoundSample** target = (TFB_SoundSample**)HMalloc(sizeof(sndfx[0]));
		*target = sndfx[i];
		str->data = (STRINGPTR)target;
		str->length = sizeof(sndfx[0]);
	}

	return Snd;
}

bool _ReleaseSoundBankData(void* Snd)
{
	STRING_TABLE fxTab = (STRING_TABLE)Snd;
	int index;

	if (!fxTab)
	{
		return false;
	}

	for (index = 0; index < fxTab->size; ++index)
	{
		int i;
		void** sptr = (void**)fxTab->strings[index].data;
		TFB_SoundSample* sample = (TFB_SoundSample*)*sptr;

		// Check all sources and see if we are currently playing this sample
		for (i = 0; i < NUM_SOUNDSOURCES; ++i)
		{
			if (soundSource[i].sample == sample)
			{ // Playing this sample. Have to stop it.
				StopSource(i);
				soundSource[i].sample = nullptr;
			}
		}

		if (sample->decoder)
		{
			SoundDecoder_Free(sample->decoder);
		}
		sample->decoder = nullptr;
		TFB_DestroySoundSample(sample);
		// sptr will be deleted by FreeStringTable() below
	}

	FreeStringTable(fxTab);

	return true;
}

bool DestroySound(SOUND_REF target)
{
	return _ReleaseSoundBankData(target);
}

// The type conversions are implicit and will generate errors
// or warnings if types change imcompatibly
SOUNDPTR
GetSoundAddress(SOUND sound)
{
	return GetStringAddress(sound);
}
