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

#include "vidplayer.h"
#include "options.h"
#include "vidintrn.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/tfb_draw.h"
#include "core/log/log.h"
#include "libs/sndlib.h"
#include "uqm/units.h"

// video callbacks
static void vp_BeginFrame(TFB_VideoDecoder*);
static void vp_EndFrame(TFB_VideoDecoder*);
static void* vp_GetCanvasLine(TFB_VideoDecoder*, uint32_t line);
static uint32_t vp_GetTicks(TFB_VideoDecoder*);
static bool vp_SetTimer(TFB_VideoDecoder*, uint32_t msecs);


static const TFB_VideoCallbacks vp_DecoderCBs =
	{
		vp_BeginFrame,
		vp_EndFrame,
		vp_GetCanvasLine,
		vp_GetTicks,
		vp_SetTimer};

// audio stream callbacks
static bool vp_AudioStart(TFB_SoundSample* sample);
static void vp_AudioEnd(TFB_SoundSample* sample);
static void vp_BufferTag(TFB_SoundSample* sample, TFB_SoundTag* tag);
static void vp_QueueBuffer(TFB_SoundSample* sample, audio_Object buffer);

static const TFB_SoundCallbacks vp_AudioCBs =
	{
		vp_AudioStart,
		nullptr,
		vp_AudioEnd,
		vp_BufferTag,
		vp_QueueBuffer};


bool TFB_InitVideoPlayer(void)
{
	// now just a stub
	return true;
}

void TFB_UninitVideoPlayer(void)
{
	// now just a stub
}

static inline int32_t
msecToTimeCount(int32_t msec)
{
	return msec * GameTicksPerSecond / 1000;
}

// audio-synced video playback frame function
// the frame rate and timing is dictated by the audio
static bool
processAudioSyncedFrame(VIDEO_REF vid)
{
#define MAX_FRAME_LAG 8
#define LAG_FRACTION 6
#define SYNC_BIAS 1 / 3
	int ret;
	uint32_t want_frame;
	uint32_t prev_want_frame;
	int32_t wait_msec;
	GFXCONTEXT oldContext;
	TimeCount Now = GetTimeCounter();

	if (!vid->playing)
	{
		return false;
	}

	if (Now < vid->frame_time)
	{
		return true; // not time yet
	}

	{
		const uqm::LockGuard lock {vid->guard};
		want_frame = vid->want_frame;
	}

	if (want_frame >= vid->decoder->frame_count)
	{
		vid->playing = false;
		return false;
	}

	// this works like so (audio-synced):
	//  1. you call VideoDecoder_Seek() [when necessary] and
	//     VideoDecoder_Decode()
	//  2. wait till it's time for this frame to be drawn
	//     the timeout is necessary because the audio signaling is not
	//     precise (see vp_AudioStart, vp_AudioEnd, vp_BufferTag)
	//  3. output the frame; if the audio is behind, the lag counter
	//     goes up; if the video is behind, the lag counter goes down
	//  4. set the next frame timeout; lag counter increases or
	//     decreases the timeout to allow audio or video to catch up
	//  5. on a seek operation, the audio stream is moved to the
	//     correct position and then the audio signals the frame
	//     that should be rendered
	//  The system of timeouts and lag counts should make the video
	//  *relatively* smooth
	//
	prev_want_frame = vid->cur_frame - vid->lag_cnt;
	if (want_frame > prev_want_frame - MAX_FRAME_LAG
		&& want_frame <= prev_want_frame + MAX_FRAME_LAG)
	{
		// we will draw the next frame right now, thus +1
		vid->lag_cnt = vid->cur_frame + 1 - want_frame;
	}
	else
	{ // out of sequence frame, let's get it
		vid->lag_cnt = 0;
		vid->cur_frame = VideoDecoder_SeekFrame(vid->decoder, want_frame);
		ret = VideoDecoder_Decode(vid->decoder);
		if (ret < 0)
		{ // decoder returned a failure
			vid->playing = false;
			return false;
		}
	}
	vid->cur_frame = vid->decoder->cur_frame;

	// draw the frame
	// We have the cliprect precalculated and don't need the rest
	oldContext = SetContext(nullptr);
	TFB_DrawScreen_Image(vid->frame,
						 vid->dst_rect.corner.x, vid->dst_rect.corner.y, 0, uqm::TFBScaleMode::None,
						 nullptr, DRAW_REPLACE_MODE, TFB_SCREEN_MAIN);
	SetContext(oldContext);
	FlushGraphics(); // needed to prevent half-frame updates

	// increase interframe with positive lag-count to allow audio to catch up
	// decrease interframe with negative lag-count to allow video to catch up
	wait_msec = vid->decoder->interframe_wait
			  - (int)vid->decoder->interframe_wait * SYNC_BIAS
			  + (int)vid->decoder->interframe_wait * vid->lag_cnt / LAG_FRACTION;
	vid->frame_time = Now + msecToTimeCount(wait_msec);

	ret = VideoDecoder_Decode(vid->decoder);
	if (ret < 0)
	{
		// TODO: decide what to do on error
	}

	return vid->playing;
}

// audio-independent video playback frame function
// the frame rate and timing is dictated by the video decoder
static bool
processMuteFrame(VIDEO_REF vid)
{
	int ret;
	TimeCount Now = GetTimeCounter();

	if (!vid->playing)
	{
		return false;
	}

	// this works like so:
	//  1. you call VideoDecoder_Seek() [when necessary] and
	//     VideoDecoder_Decode()
	//  2. the decoder calls back vp_GetTicks() and vp_SetTimer()
	//     to figure out and tell you when to render the frame
	//     being decoded
	//  On a seek operation, the decoder should reset its internal
	//  clock and call vp_GetTicks() again
	//
	if (Now >= vid->frame_time)
	{
		GFXCONTEXT oldContext;

		vid->cur_frame = vid->decoder->cur_frame;

		// We have the cliprect precalculated and don't need the rest
		oldContext = SetContext(nullptr);
		TFB_DrawScreen_Image(vid->frame,
							 vid->dst_rect.corner.x, vid->dst_rect.corner.y, 0, uqm::TFBScaleMode::None,
							 nullptr, DRAW_REPLACE_MODE, TFB_SCREEN_MAIN);
		SetContext(oldContext);
		FlushGraphics(); // needed to prevent half-frame updates

		if (vid->cur_frame == vid->loop_frame)
		{
			VideoDecoder_SeekFrame(vid->decoder, vid->loop_to);
		}

		ret = VideoDecoder_Decode(vid->decoder);
		if (ret <= 0)
		{
			vid->playing = false;
		}
	}

	return vid->playing;
}

bool TFB_PlayVideo(VIDEO_REF vid, uint32_t x, uint32_t y)
{
	GFXRECT scrn_r;
	GFXRECT clip_r = {
		{0,			   0			  },
		{(int16_t)vid->w, (int16_t)vid->h}
	  };
	GFXRECT vid_r = {
		{0,					0					},
		{(int16_t)CanvasWidth, (int16_t)CanvasHeight}
	 };
	GFXRECT dr = {
		{(int16_t)x,		 (int16_t)y	   },
		{(int16_t)vid->w, (int16_t)vid->h}
	  };
	GFXRECT sr;
	bool loop_music = false;
	int ret;

	if (!vid)
	{
		return false;
	}

	// calculate the frame-source and screen-destination rects
	GetContextClipRect(&scrn_r);
	if (!BoxIntersect(&scrn_r, &vid_r, &scrn_r))
	{
		return false; // drawing outside visible
	}

	sr = dr;
	// JMS_GFX: Added this if-clause around the following lines to make the
	// 3DO videos work also in 1280x960.
	if (IS_HD)
	{
		sr.corner.x = -sr.corner.x;
		sr.corner.y = -sr.corner.y;
		if (!BoxIntersect(&clip_r, &sr, &sr))
		{
			return false; // drawing outside visible
		}
	}

	dr.corner.x += scrn_r.corner.x;
	dr.corner.y += scrn_r.corner.y;
	if (!BoxIntersect(&scrn_r, &dr, &vid->dst_rect))
	{
		return false; // drawing outside visible
	}

	vid->src_rect = vid->dst_rect;
	vid->src_rect.corner.x = sr.corner.x;
	vid->src_rect.corner.y = sr.corner.y;

	vid->decoder->callbacks = vp_DecoderCBs;
	vid->decoder->data = vid;

	vid->frame = TFB_DrawImage_CreateForScreen(vid->w, vid->h, false);
	vid->cur_frame = -1;
	{
		const uqm::LockGuard lock {vid->guard};
		vid->want_frame = -1;
	}

	if (!vid->hAudio)
	{
		vid->hAudio = LoadMusicFile(vid->decoder->filename.c_str());
		vid->own_audio = true;
	}

	if (vid->decoder->audio_synced)
	{
		if (!vid->hAudio)
		{
			uqm::log::warn("TFB_PlayVideo: "
						   "Cannot load sound-track for audio-synced video");
			return false;
		}

		TFB_SetSoundSampleCallbacks(*vid->hAudio, &vp_AudioCBs);
		TFB_SetSoundSampleData(*vid->hAudio, vid);
	}

	// get the first frame
	ret = VideoDecoder_Decode(vid->decoder);
	if (ret < 0)
	{
		return false;
	}

	vid->playing = true;

	loop_music = !vid->decoder->audio_synced && vid->loop_frame != VID_NO_LOOP;
	if (vid->hAudio)
	{
		PLRPlaySong(vid->hAudio, (bool)loop_music, 1);
	}

	if (vid->decoder->audio_synced)
	{
		// draw the first frame now
		vid->frame_time = GetTimeCounter();
	}

	return true;
}

void TFB_StopVideo(VIDEO_REF vid)
{
	if (!vid)
	{
		return;
	}

	vid->playing = false;

	if (vid->hAudio)
	{
		PLRStop(vid->hAudio);
		if (vid->own_audio)
		{
			DestroyMusic(vid->hAudio);
			vid->hAudio = 0;
			vid->own_audio = false;
		}
	}
	if (vid->frame)
	{
		TFB_DrawScreen_DeleteImage(vid->frame);
		vid->frame = nullptr;
	}
}

bool TFB_VideoPlaying(VIDEO_REF vid)
{
	if (!vid)
	{
		return false;
	}

	return vid->playing;
}

bool TFB_ProcessVideoFrame(VIDEO_REF vid)
{
	if (!vid)
	{
		return false;
	}

	if (vid->decoder->audio_synced)
	{
		return processAudioSyncedFrame(vid);
	}
	else
	{
		return processMuteFrame(vid);
	}
}

uint32_t
TFB_GetVideoPosition(VIDEO_REF vid)
{
	uint32_t pos;

	if (!TFB_VideoPlaying(vid))
	{
		return 0;
	}

	{
		const uqm::LockGuard lock {vid->guard};
		pos = (uint32_t)(vid->decoder->pos * 1000);
	}

	return pos;
}

bool TFB_SeekVideo(VIDEO_REF vid, uint32_t pos)
{
	if (!TFB_VideoPlaying(vid))
	{
		return false;
	}

	if (vid->decoder->audio_synced)
	{
		PLRSeek(vid->hAudio, pos);
		TaskSwitch();
		return true;
	}
	else
	{ // TODO: Non-a/s decoder seeking is not supported yet
		//   Decide what to do with these. Seeking this kind of
		//   video is trivial, but we may not want to do it.
		//   The only non-a/s videos right now are ship spins.
		return false;
	}
}

static void
vp_BeginFrame(TFB_VideoDecoder* decoder)
{
	TFB_VideoClip* vid = (TFB_VideoClip*)decoder->data;

	if (vid)
	{
		TFB_DrawCanvas_Lock(vid->frame->NormalImg);
	}
}

static void
vp_EndFrame(TFB_VideoDecoder* decoder)
{
	TFB_VideoClip* vid = (TFB_VideoClip*)decoder->data;

	if (vid)
	{
		TFB_DrawCanvas_Unlock(vid->frame->NormalImg);
	}
}

static void*
vp_GetCanvasLine(TFB_VideoDecoder* decoder, uint32_t line)
{
	TFB_VideoClip* vid = (TFB_VideoClip*)decoder->data;

	if (!vid)
	{
		return nullptr;
	}

	return TFB_DrawCanvas_GetLine(vid->frame->NormalImg, line);
}

static uint32_t
vp_GetTicks(TFB_VideoDecoder* decoder)
{
	uint32_t ctr = GetTimeCounter();
	return (ctr / GameTicksPerSecond) * 1000 + ((ctr % GameTicksPerSecond) * 1000) / GameTicksPerSecond;

	(void)decoder; // gobble up compiler warning
}

static bool
vp_SetTimer(TFB_VideoDecoder* decoder, uint32_t msecs)
{
	TFB_VideoClip* vid = (TFB_VideoClip*)decoder->data;

	if (!vid)
	{
		return false;
	}

	// time when next frame should be displayed
	vid->frame_time = GetTimeCounter() + msecs * GameTicksPerSecond / 1000;
	return true;
}

static bool
vp_AudioStart(TFB_SoundSample* sample)
{
	TFB_VideoClip* vid = (TFB_VideoClip*)TFB_GetSoundSampleData(sample);
	TFB_SoundDecoder* decoder;

	assert(sizeof(intptr_t) >= sizeof(vid));
	assert(vid != nullptr);

	decoder = TFB_GetSoundSampleDecoder(sample);

	{
		const uqm::LockGuard lock {vid->guard};
		vid->want_frame = SoundDecoder_GetFrame(decoder);
	}

	return true;
}

static void
vp_AudioEnd(TFB_SoundSample* sample)
{
	TFB_VideoClip* vid = (TFB_VideoClip*)TFB_GetSoundSampleData(sample);

	assert(vid != nullptr);

	{
		const uqm::LockGuard lock {vid->guard};
		vid->want_frame = vid->decoder->frame_count; // end it
	}
}

static void
vp_BufferTag(TFB_SoundSample* sample, TFB_SoundTag* tag)
{
	TFB_VideoClip* vid = (TFB_VideoClip*)TFB_GetSoundSampleData(sample);
	uint32_t frame = (uint32_t)tag->data;

	assert(sizeof(tag->data) >= sizeof(frame));
	assert(vid != nullptr);

	{
		const uqm::LockGuard lock {vid->guard};
		vid->want_frame = frame; // let it go!
	}
}

static void
vp_QueueBuffer(TFB_SoundSample* sample, audio_Object buffer)
{
	//TFB_VideoClip* vid = (TFB_VideoClip*) TFB_GetSoundSampleData (sample);
	TFB_SoundDecoder* decoder = TFB_GetSoundSampleDecoder(sample);

	TFB_TagBuffer(sample, buffer,
				  (intptr_t)SoundDecoder_GetFrame(decoder));
}
