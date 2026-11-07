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
#ifndef UQM_LIBS_VIDEO_VIDPLAYER_H
#define UQM_LIBS_VIDEO_VIDPLAYER_H

#include "video.h"

extern bool TFB_InitVideoPlayer(void);
extern void TFB_UninitVideoPlayer(void);
extern bool TFB_PlayVideo(VIDEO_REF VidRef, uint32_t x, uint32_t y);
extern void TFB_StopVideo(VIDEO_REF VidRef);
extern bool TFB_VideoPlaying(VIDEO_REF VidRef);
extern bool TFB_ProcessVideoFrame(VIDEO_REF vid);
extern uint32_t TFB_GetVideoPosition(VIDEO_REF VidRef);
extern bool TFB_SeekVideo(VIDEO_REF VidRef, uint32_t pos);

#endif // UQM_LIBS_VIDEO_VIDPLAYER_H
