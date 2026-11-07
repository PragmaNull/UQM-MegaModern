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
#ifndef UQM_LIBS_VIDLIB_H
#define UQM_LIBS_VIDLIB_H

#include "core/misc/bittricks.h"
#include "libs/sndlib.h"
#include "libs/reslib.h"

enum class VideoType
{
	None = 0,
	Hardware,
	Software
};

namespace tfb
{
struct VideoClip;
} // namespace tfb

using VideoRef = struct tfb::VideoClip*;
using LegacyVideo = struct LegacyVideoDesc*;
using LegacyVideoRef = struct legacy_video_ref*;

extern bool InstallVideoResType(void);

extern bool InitVideoPlayer(bool UseCDROM);
extern void UninitVideoPlayer(void);

extern VideoRef LoadVideoFile(const char* pStr);
extern bool DestroyVideo(VideoRef VidRef);
extern VideoType VidPlay(VideoRef VidRef);
extern VideoType VidPlayEx(VideoRef VidRef, MusicRef AudRef,
						   MusicRef SpeechRef, uint32_t LoopFrame);
inline constexpr auto VidNoLoop {0U - 1};
extern void VidStop(void);
extern VideoRef VidPlaying(void);
extern bool VidProcessFrame(void);
extern uint32_t VidGetPosition(void); // position in milliseconds
extern bool VidSeek(uint32_t pos);	  // position in milliseconds

extern LegacyVideo LoadLegacyVideoInstance(RESOURCE res);
extern bool DestroyLegacyVideo(LegacyVideo vid);
extern LegacyVideoRef PlayLegacyVideo(LegacyVideo vid);
extern void StopLegacyVideo(LegacyVideoRef ref);
extern bool PlayingLegacyVideo(LegacyVideoRef ref);

#endif // UQM_LIBS_VIDLIB_H
