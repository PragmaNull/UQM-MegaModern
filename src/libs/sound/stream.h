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

#ifndef STREAM_H
#define STREAM_H

#include "sndintrn.h"

int InitStreamDecoder(void);
void UninitStreamDecoder(void);

// The caller must hold the stream lock of the source.
void PlayStream(TFB_SoundSample* sample, uint32_t source, bool looping,
				bool scope, bool rewind) UQM_REQUIRES(soundSource[source].stream_mutex);
void StopStream(uint32_t source) UQM_REQUIRES(soundSource[source].stream_mutex);
void PauseStream(uint32_t source) UQM_REQUIRES(soundSource[source].stream_mutex);
void ResumeStream(uint32_t source) UQM_REQUIRES(soundSource[source].stream_mutex);
void SeekStream(uint32_t source, uint32_t pos) UQM_REQUIRES(soundSource[source].stream_mutex);
uint32_t GetStreamFrame(uint32_t source);
uint16_t GetNumTrackerPos(uint32_t source);
bool IsTracker(uint32_t source);
float GetStreamLength(uint32_t source);
uint32_t GetStreamTime(uint32_t source);
bool PlayingStream(uint32_t source);

int GraphForegroundStream(uint8_t* data, int32_t width, int32_t height,
						  bool wantSpeech);

// returns true if the fade was accepted by stream decoder
bool SetMusicStreamFade(int32_t howLong, int endVolume);

#endif
