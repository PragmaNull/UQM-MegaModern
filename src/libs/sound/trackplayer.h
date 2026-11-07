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

#pragma once
#ifndef UQM_LIBS_SOUND_TRACKPLAYER_H
#define UQM_LIBS_SOUND_TRACKPLAYER_H

#include "core/misc/bittricks.h"
#include "libs/callback/callback.h"

inline constexpr auto AccelScrollSpeed {300};

inline constexpr auto VerySlow {0};
inline constexpr auto Slow {12};
inline constexpr auto ModerateSpeed {16};
inline constexpr auto Fast {24};
inline constexpr auto VeryFast {32};

static const uqstl::array<uint8_t, 5> speed_array {
	VerySlow,
	Slow,
	ModerateSpeed,
	Fast,
	VeryFast};

extern void PlayTrack(void);
extern void StopTrack(void);
extern void JumpTrack(void);
extern void PauseTrack(void);
extern void ResumeTrack(void);
extern uint16_t PlayingTrack(void);
extern uint16_t GetSubtitleNumber(const char* sub);

extern void FastReverse_Smooth(void);
extern void FastForward_Smooth(void);
extern void FastReverse_Page(void);
extern void FastForward_Page(void);

extern void SpliceTrack(const char* filespec, const char* textspec, const char* TimeStamp, CallbackFunction cb);
extern void SpliceMultiTrack(const char* const TrackNames[], const char* TrackText);

extern int GetTrackPosition(int in_units);

namespace tfb
{
struct SoundChunk;
} // namespace tfb

using SubtitleRef = struct tfb::SoundChunk*;

extern SubtitleRef GetFirstTrackSubtitle(void);
extern SubtitleRef GetNextTrackSubtitle(SubtitleRef LastRef);
extern const char* GetTrackSubtitleText(SubtitleRef SubRef);

extern const char* GetTrackSubtitle(void);
extern uint16_t GetSubtitleNumberByTrack(uint16_t track);
extern uint32_t RecalculateDelay(uint32_t numChars, bool talk);

#endif // UQM_LIBS_SOUND_TRACKPLAYER_H
