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

#include "video.h"

#include "vidintrn.h"
#include "options.h"
#include "vidplayer.h"
#include "libs/sndlib.h"
#include "uqm/units.h"


static constexpr VIDEO_REF NullVideoRef {nullptr};
static VIDEO_REF _cur_video = NullVideoRef;
static MUSIC_REF _cur_speech = 0;

bool InitVideoPlayer(bool useCDROM)
//useCDROM doesn't really apply to us
{
	TFB_PixelFormat fmt;

	TFB_DrawCanvas_GetScreenFormat(&fmt);
	if (!VideoDecoder_Init(0, fmt.BitsPerPixel, fmt.Rmask,
						   fmt.Gmask, fmt.Bmask, 0))
	{
		return false;
	}

	return (bool)TFB_InitVideoPlayer();

	(void)useCDROM; /* dodge compiler warning */
}

void UninitVideoPlayer(void)
{
	TFB_UninitVideoPlayer();
	VideoDecoder_Uninit();
}

void VidStop(void)
{
	if (_cur_speech)
	{
		snd_StopSpeech();
	}
	if (_cur_video)
	{
		TFB_StopVideo(_cur_video);
	}
	_cur_speech = 0;
	_cur_video = NullVideoRef;
}

VIDEO_REF VidPlaying(void)
// this should just probably return bool
{
	if (!_cur_video)
	{
		return NullVideoRef;
	}

	if (TFB_VideoPlaying(_cur_video))
	{
		return _cur_video;
	}

	return NullVideoRef;
}

bool VidProcessFrame(void)
{
	if (!_cur_video)
	{
		return false;
	}
	return (bool)TFB_ProcessVideoFrame(_cur_video);
}

// return current video position in milliseconds
uint32_t VidGetPosition(void)
{
	if (!VidPlaying())
	{
		return 0;
	}
	return TFB_GetVideoPosition(_cur_video);
}

bool VidSeek(uint32_t pos)
// pos in milliseconds
{
	if (!VidPlaying())
	{
		return false;
	}
	return (bool)TFB_SeekVideo(_cur_video, pos);
}

VIDEO_TYPE VidPlayEx(VIDEO_REF vid, MUSIC_REF AudRef, MUSIC_REF SpeechRef,
					 uint32_t LoopFrame)
{
	VIDEO_TYPE ret;

	if (!vid)
	{
		return NO_FMV;
	}

	if (AudRef)
	{
		if (vid->hAudio)
		{
			DestroyMusic(vid->hAudio);
		}
		vid->hAudio = AudRef;
		vid->decoder->audio_synced = false;
	}

	vid->loop_frame = LoopFrame;
	vid->loop_to = 0;

	if (_cur_speech)
	{
		snd_StopSpeech();
	}
	if (_cur_video)
	{
		TFB_StopVideo(_cur_video);
	}
	_cur_speech = 0;
	_cur_video = NullVideoRef;

	// play video in the center of the screen
	if (TFB_PlayVideo(vid, (CanvasWidth - vid->w) / 2,
					  (CanvasHeight - vid->h) / 2))
	{
		_cur_video = vid;
		ret = SOFTWARE_FMV;
		if (SpeechRef)
		{
			snd_PlaySpeech(SpeechRef);
			_cur_speech = SpeechRef;
		}
	}
	else
	{
		ret = NO_FMV;
	}

	return ret;
}

VIDEO_TYPE VidPlay(VIDEO_REF VidRef)
{
	return VidPlayEx(VidRef, 0, 0, VidNoLoop);
}

VIDEO_REF _init_video_file(const char* pStr)
{
	TFB_VideoDecoder* dec {VideoDecoder_Load(uqm::contentDir(), pStr)};
	if (!dec)
	{
		return NullVideoRef;
	}

	TFB_VideoClip* vid {new TFB_VideoClip {}};
	vid->decoder = dec;
	vid->length = dec->length;
	vid->w = vid->decoder->w - IfHd(4);
	vid->h = vid->decoder->h + IfHd(7);

	return (VIDEO_REF)vid;
}

bool DestroyVideo(VIDEO_REF vid)
{
	if (!vid)
	{
		return false;
	}

	// just some armouring; should already be stopped
	TFB_StopVideo(vid);

	VideoDecoder_Free(vid->decoder);
	delete vid;

	return true;
}
