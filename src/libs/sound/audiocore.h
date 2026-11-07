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

/* Audio Core API (derived from OpenAL)
 */

#pragma once
#ifndef UQM_LIBS_SOUND_AUDIOCORE_H
#define UQM_LIBS_SOUND_AUDIOCORE_H

#include <cstdint>

#include "config.h"
#include "libs/sound/audiocoredefs.h"


extern AudioDriverType snddriver;
extern AudioFlags soundflags;

struct audio_Driver
{
	/* General */
	void (*Uninitialize)(void);
	int32_t (*GetError)(void);
	AudioDriverType driverID;
	int32_t EnumLookup[*AudioEnum::Count];

	/* Sources */
	void (*GenSources)(uint32_t n, audio_Object* psrcobj);
	void (*DeleteSources)(uint32_t n, audio_Object* psrcobj);
	bool (*IsSource)(audio_Object srcobj);
	void (*Sourcei)(audio_Object srcobj, int32_t pname,
					audio_IntVal value);
	void (*Sourcef)(audio_Object srcobj, int32_t pname,
					float value);
	void (*Sourcefv)(audio_Object srcobj, int32_t pname,
					 float* value);
	void (*GetSourcei)(audio_Object srcobj, int32_t pname,
					   audio_IntVal* value);
	void (*GetSourcef)(audio_Object srcobj, int32_t pname,
					   float* value);
	void (*SourceRewind)(audio_Object srcobj);
	void (*SourcePlay)(audio_Object srcobj);
	void (*SourcePause)(audio_Object srcobj);
	void (*SourceStop)(audio_Object srcobj);
	void (*SourceQueueBuffers)(audio_Object srcobj, uint32_t n,
							   audio_Object* pbufobj);
	void (*SourceUnqueueBuffers)(audio_Object srcobj, uint32_t n,
								 audio_Object* pbufobj);

	/* Buffers */
	void (*GenBuffers)(uint32_t n, audio_Object* pbufobj);
	void (*DeleteBuffers)(uint32_t n, audio_Object* pbufobj);
	bool (*IsBuffer)(audio_Object bufobj);
	void (*GetBufferi)(audio_Object bufobj, int32_t pname,
					   audio_IntVal* value);
	void (*BufferData)(audio_Object bufobj, uint32_t format, void* data,
					   uint32_t size, uint32_t freq);
};


/* Initialization */
int32_t initAudio(AudioDriverType driver, AudioFlags flags);
void unInitAudio(void);

/* General */
int32_t audio_GetError(void);

/* Sources */
void audio_GenSources(uint32_t n, audio_Object* psrcobj);
void audio_DeleteSources(uint32_t n, audio_Object* psrcobj);
bool audio_IsSource(audio_Object srcobj);
void audio_Sourcei(audio_Object srcobj, audio_SourceProp pname,
				   audio_IntVal value);
void audio_Sourcef(audio_Object srcobj, audio_SourceProp pname,
				   float value);
void audio_Sourcefv(audio_Object srcobj, audio_SourceProp pname,
					float* value);
void audio_GetSourcei(audio_Object srcobj, audio_SourceProp pname,
					  audio_IntVal* value);
void audio_GetSourcef(audio_Object srcobj, audio_SourceProp pname,
					  float* value);
void audio_SourceRewind(audio_Object srcobj);
void audio_SourcePlay(audio_Object srcobj);
void audio_SourcePause(audio_Object srcobj);
void audio_SourceStop(audio_Object srcobj);
void audio_SourceQueueBuffers(audio_Object srcobj, uint32_t n,
							  audio_Object* pbufobj);
void audio_SourceUnqueueBuffers(audio_Object srcobj, uint32_t n,
								audio_Object* pbufobj);

/* Buffers */
void audio_GenBuffers(uint32_t n, audio_Object* pbufobj);
void audio_DeleteBuffers(uint32_t n, audio_Object* pbufobj);
bool audio_IsBuffer(audio_Object bufobj);
void audio_GetBufferi(audio_Object bufobj, audio_BufferProp pname,
					  audio_IntVal* value);
void audio_BufferData(audio_Object bufobj, uint32_t format, void* data,
					  uint32_t size, uint32_t freq);

bool audio_GetFormatInfo(uint32_t format, int* channels, int* sample_size);

#endif // UQM_LIBS_SOUND_AUDIOCORE_H
