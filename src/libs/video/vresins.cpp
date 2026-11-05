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
#include <fmt/format.h>


static bool FreeLegacyVideoData(void* data)
{
	delete static_cast<LEGACY_VIDEO>(data);
	return data != nullptr;
}

static void GetLegacyVideoData(const ResourceArg& arg, RESOURCE_DATA* resdata)
{
	const char* const video {resourceStringField(arg, "video")};
	if (video == nullptr)
	{
		uqm::log::warn("Warning: A 3DOVID resource has no video.");
		resdata->ptr = nullptr;
		return;
	}
	const char* const audio {resourceStringField(arg, "audio")};
	const char* const speech {resourceStringField(arg, "speech")};
	const uqstl::optional<int64_t> loop {resourceIntField(arg, "loop")};
	const uint32_t loopFrame {loop ? static_cast<uint32_t>(*loop) : VidNoLoop};

	uqm::log::info("\t'{}' -- video, audio '{}', speech '{}', loop frame {}", video, audio != nullptr ? audio : "", speech != nullptr ? speech : "",
				   loop ? fmt::format("{}", loopFrame) : "none");

	resdata->ptr = new LEGACY_VIDEO_DESC {video, audio != nullptr ? audio : "", speech != nullptr ? speech : "", loopFrame};
}

bool InstallVideoResType(void)
{
	InstallResTypeVectors("3DOVID", GetLegacyVideoData, FreeLegacyVideoData);
	return true;
}

LEGACY_VIDEO LoadLegacyVideoInstance(RESOURCE res)
{
	void* data {res_GetResource(res)};
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
