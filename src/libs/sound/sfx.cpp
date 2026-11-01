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

#include "options.h"
#include "sound.h"
#include "sndintrn.h"
#include "libs/io/Path.h"
#include "libs/reslib.h"
#include "core/log/log.h"
#include "soundbank.h"


static void CheckFinishedChannels(void);

static const SoundPosition notPositional = {false, 0, 0};

void PlayChannel(uint16_t channel, SOUND snd, SoundPosition pos,
				 void* positional_object, unsigned char priority)
{
	TFB_SoundSample* const sample {snd};

	StopSource(channel);
	// all finished (stopped) channels can be cleaned up at this point
	// since this is the only func that can initiate an sfx sound
	CheckFinishedChannels();

	if (!sample)
	{
		return; // nothing to play
	}

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

static constexpr int32_t c_missingSoundMs {100};

void* _GetSoundBankData(const ResourceFile& resource)
{
	uqstl::vector<TFB_SoundSample*> samples {};

	const auto text {readResourceText(resource)};
	const auto sounds {text ? uqm::soundBankFromJson(*text) : uqstl::nullopt};
	if (!sounds)
	{
		uqm::log::warn("_GetSoundBankData(): could not read '{}'", resource.path);
		return nullptr;
	}

	// The sound file names are relative to the directory of the bank.
	const uqstl::string_view parent {uqm::io::path::parent(resource.path)};
	for (const uqstl::string& sound : *sounds)
	{
		uqstl::string filename {uqm::io::path::join(parent, sound)};
		uqm::log::debug("_GetSoundBankData(): loading {}", filename);

		// A missing sound gets a silent decoder.
		TFB_SoundDecoder* const decoder {SoundDecoder_Load(uqm::contentDir(), filename.data(), 4096, 0, -c_missingSoundMs)};
		if (!decoder)
		{
			uqm::log::warn("_GetSoundBankData(): couldn't load {}", filename);
			continue;
		}
		if (decoder->is_null)
		{
			uqm::log::warn("_GetSoundBankData(): there is no sound '{}'. The sound is silent.", filename);
		}

		// SFX samples don't have decoders, everything is pre-decoded below
		TFB_SoundSample* const sample {TFB_CreateSoundSample(nullptr, 1, nullptr)};

		// Decode everything and stash it in 1 buffer
		const uint32_t decoded_bytes {SoundDecoder_DecodeAll(decoder)};
		uqm::log::debug("_GetSoundBankData(): decoded bytes {}", decoded_bytes);

		audio_BufferData(sample->buffer[0], decoder->format, decoder->buffer, decoded_bytes, decoder->frequency);
		// just for informational purposes
		sample->length = decoder->length;

		SoundDecoder_Free(decoder);

		samples.push_back(sample);
	}

	if (samples.empty())
	{
		return nullptr; // no sounds decoded
	}

	return new uqm::SoundBank {uqstl::move(samples)};
}

bool _ReleaseSoundBankData(void* Snd)
{
	if (!Snd)
	{
		return false;
	}
	delete static_cast<uqm::SoundBank*>(Snd);
	return true;
}

bool DestroySound(SOUND_REF target)
{
	return _ReleaseSoundBankData(target);
}

namespace uqm
{

SoundBank::SoundBank(uqstl::vector<TFB_SoundSample*> samples)
	: m_samples {uqstl::move(samples)}
{
}

SoundBank::~SoundBank()
{
	for (TFB_SoundSample* const sample : m_samples)
	{
		// Check all sources and see if we are currently playing this sample
		for (int i {0}; i < NUM_SOUNDSOURCES; ++i)
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
	}
}

TFB_SoundSample* SoundBank::sample(size_t index) const
{
	return m_samples.empty() ? nullptr : m_samples[index % m_samples.size()];
}

} // namespace uqm
