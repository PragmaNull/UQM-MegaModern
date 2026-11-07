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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once
#ifndef UQM_SETTINGS_H
#define UQM_SETTINGS_H

#include "libs/sndlib.h"

namespace uqm
{
extern void ToggleMusic(void);
extern void StopMusic(void);
extern void ResumeMusic(void);
extern void PauseMusic(void);
extern void PlayMusicResume(MusicRef hMusic, uint8_t Volume);
extern void ToggleSoundEffect(void);
extern void SeekMusic(uint32_t pos);

extern void PlayMusic(MusicRef musicRef, bool Continuous, uint8_t Priority);
extern void PlaySoundEffect(SoundRef S, uint16_t Channel, SoundPosition Pos,
							void* PositionalObject, uint8_t Priority);
extern void PlaySpeechEffect(SoundRef S, SoundPosition Pos, void* PositionalObject, uint8_t Priority);

} // namespace uqm

#endif // UQM_SETTINGS_H
