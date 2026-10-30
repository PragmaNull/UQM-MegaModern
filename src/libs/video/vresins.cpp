// Copyright 2008 Michael Martin

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

#include <stdlib.h>
#include <string.h>
#include "vidintrn.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"


static bool
FreeLegacyVideoData(void* data)
{
	delete static_cast<LEGACY_VIDEO>(data);
	return data != nullptr;
}

static void
GetLegacyVideoData(const char* path, RESOURCE_DATA* resdata)
{
	char paths[1024] {};
	char *audio_path, *speech_path, *loop_str;
	uint32_t LoopFrame = VID_NO_LOOP;

	/* Parse out the video components. */
	uqm::strncpy_safe(paths, path);
	audio_path = strchr(paths, ':');
	if (audio_path == nullptr)
	{
		speech_path = nullptr;
		loop_str = nullptr;
	}
	else
	{
		*audio_path = '\0';
		audio_path++;

		speech_path = strchr(audio_path, ':');
		if (speech_path == nullptr)
		{
			loop_str = nullptr;
		}
		else
		{
			*speech_path = '\0';
			speech_path++;

			loop_str = strchr(speech_path, ':');
			if (loop_str != nullptr)
			{
				*loop_str = '\0';
				loop_str++;
			}
		}
	}

	uqm::log::info("\t'{}' -- video", paths);
	if (audio_path)
	{
		uqm::log::info("\t'{}' -- audio", audio_path);
	}
	else
	{
		uqm::log::info("\tNo associated audio");
	}
	if (speech_path)
	{
		uqm::log::info("\t'{}' -- speech path", speech_path);
	}
	else
	{
		uqm::log::info("\tNo associated speech");
	}
	if (loop_str)
	{
		char* end;
		LoopFrame = strtol(loop_str, &end, 10);
		// We allow whitespace at the end, but nothing printable.
		if (*end > 32)
		{
			uqm::log::warn("Warning: Unparsable loop frame '{}'. Disabling loop.", loop_str);
			LoopFrame = VID_NO_LOOP;
		}
		uqm::log::info("\tLoop frame is {}", LoopFrame);
	}
	else
	{
		uqm::log::info("\tNo specified loop frame");
	}

	resdata->ptr = new LEGACY_VIDEO_DESC {paths, audio_path ? audio_path : "", speech_path ? speech_path : "", LoopFrame};
}

bool InstallVideoResType(void)
{
	InstallResTypeVectors("3DOVID", GetLegacyVideoData, FreeLegacyVideoData, nullptr);
	return true;
}

LEGACY_VIDEO
LoadLegacyVideoInstance(RESOURCE res)
{
	void* data;

	data = res_GetResource(res);
	if (data)
	{
		res_DetachResource(res);
	}

	return (LEGACY_VIDEO)data;
}

bool DestroyLegacyVideo(LEGACY_VIDEO vid)
{
	return FreeLegacyVideoData(vid);
}
