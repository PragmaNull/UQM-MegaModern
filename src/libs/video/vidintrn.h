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

#pragma once
#ifndef UQM_LIBS_VIDEO_VIDINTRN_H
#define UQM_LIBS_VIDEO_VIDINTRN_H

#include <cstdint>

#include "core/stl/string.h"
#include "libs/time/timelib.h"
#include "libs/vidlib.h"

struct LegacyVideoDesc
{
	// The audio and speech names are empty if the video has none.
	uqstl::string video, audio, speech;
	uint32_t loop;
};

struct legacy_video_ref
{
	VideoRef vidref;
	MusicRef audref;
	MusicRef speechref;
};

#endif // UQM_LIBS_VIDEO_VIDINTRN_H
