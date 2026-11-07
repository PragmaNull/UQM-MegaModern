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

#pragma once
#ifndef UQM_LIBS_SOUND_SOUND_H
#define UQM_LIBS_SOUND_SOUND_H

#ifndef LIBS_SOUND_SOUND_H_ // try avoiding collisions on id
#define LIBS_SOUND_SOUND_H_

#include <cstdint>
#include "audiocore.h"
#include "decoders/decoder.h"
#include "libs/time/timelib.h"
#include "libs/sndlib.h"


inline constexpr auto FirstSfxSource {0};
inline constexpr auto LastSfxSource {FirstSfxSource + NumSfxChannels - 1};
inline constexpr auto MusicSource {LastSfxSource + 1};
inline constexpr auto SpeechSource {MusicSource + 1};
inline constexpr auto NumSoundsources {SpeechSource + 1};

namespace tfb
{
struct SoundTag
{
	int in_use;
	audio_Object buf_name;
	intptr_t data; // user-defined data
};

struct SoundCallbacks
{
	// return true to continue, false to abort
	bool (*OnStartStream)(SoundSample*);
	// return true to continue, false to abort
	bool (*OnEndChunk)(SoundSample*, audio_Object);
	// return true to continue, false to abort
	void (*OnEndStream)(SoundSample*);
	// tagged buffer callback
	void (*OnTaggedBuffer)(SoundSample*, SoundTag*);
	// buffer just queued
	void (*OnQueueBuffer)(SoundSample*, audio_Object);
};
} // namespace tfb


extern int musicVolume;
extern float musicVolumeScale;
extern float sfxVolumeScale;
extern float speechVolumeScale;

void StopSource(int iSource);
void CleanSource(int iSource);

void SetSFXVolume(float volume);
void SetSpeechVolume(float volume);

tfb::SoundSample* TFB_CreateSoundSample(tfb::SoundDecoder*, uint32_t num_buffers,
										const tfb::SoundCallbacks* /* can be nullptr */);
void TFB_DestroySoundSample(tfb::SoundSample*);
void TFB_SetSoundSampleData(tfb::SoundSample*, void* data);
void* TFB_GetSoundSampleData(tfb::SoundSample*);
void TFB_SetSoundSampleCallbacks(tfb::SoundSample*,
								 const tfb::SoundCallbacks* /* can be nullptr */);
tfb::SoundDecoder* TFB_GetSoundSampleDecoder(tfb::SoundSample*);

tfb::SoundTag* TFB_FindTaggedBuffer(tfb::SoundSample*, audio_Object buffer);
void TFB_ClearBufferTag(tfb::SoundTag*);
bool TFB_TagBuffer(tfb::SoundSample*, audio_Object buffer, intptr_t data);

#include "stream.h"

#endif // LIBS_SOUND_SOUND_H_

#endif // UQM_LIBS_SOUND_SOUND_H
