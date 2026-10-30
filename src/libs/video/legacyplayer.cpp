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

#include "vidintrn.h"
#include "video.h"
#include "vidplayer.h"
#include "libs/memlib.h"
#include "uqm/util.h"

LEGACY_VIDEO_REF
PlayLegacyVideo(LEGACY_VIDEO vid)
{
	const char *name, *audname, *speechname;
	uint32_t loopframe;
	LEGACY_VIDEO_REF ref;
	VIDEO_TYPE type;

	if (!vid)
	{
		return nullptr;
	}
	name = vid->video.c_str();
	audname = vid->audio.empty() ? nullptr : vid->audio.c_str();
	speechname = vid->speech.empty() ? nullptr : vid->speech.c_str();
	loopframe = vid->loop;

	ref = new legacy_video_ref {};
	ref->vidref = LoadVideoFile(name);
	if (!ref->vidref)
	{
		delete ref;
		return nullptr;
	}

	DrawBorderPadding(ref->vidref->w);

	if (audname)
	{
		ref->audref = LoadMusicFile(audname);
	}
	if (speechname)
	{
		ref->speechref = LoadMusicFile(speechname);
	}

	type = VidPlayEx(ref->vidref, ref->audref, ref->speechref, loopframe);
	if (type == NO_FMV)
	{ // Video failed to start
		StopLegacyVideo(ref);
		return nullptr;
	}

	return ref;
}

void StopLegacyVideo(LEGACY_VIDEO_REF ref)
{
	if (!ref)
	{
		return;
	}
	VidStop();

	DestroyVideo(ref->vidref);
	if (ref->speechref)
	{
		DestroyMusic(ref->speechref);
	}
	if (ref->audref)
	{
		DestroyMusic(ref->audref);
	}

	delete ref;
}

bool PlayingLegacyVideo(LEGACY_VIDEO_REF ref)
{
	if (!ref)
	{
		return false;
	}
	return (bool)TFB_VideoPlaying(ref->vidref);
}
