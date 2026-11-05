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

#include "sound.h"
#include "sndintrn.h"

static void GetSoundBankFileData(const char* pathname, RESOURCE_DATA* resdata)
{
	resdata->ptr = LoadResourceFromPath(pathname, _GetSoundBankData);
}

static void GetMusicFileData(const char* pathname, RESOURCE_DATA* resdata)
{
	resdata->ptr = LoadResourceFromPath(pathname, _GetMusicData);
}

bool InstallAudioResTypes(void)
{
	InstallResTextTypeVectors("SNDRES", GetSoundBankFileData, _ReleaseSoundBankData);
	InstallResTextTypeVectors("MUSICRES", GetMusicFileData, _ReleaseMusicData);
	return (true);
}

uqm::SoundBankPtr uqm::loadSoundBank(RESOURCE res)
{
	void* const data {res_GetResource(res)};
	if (data)
	{
		res_DetachResource(res);
	}
	return SoundBankPtr {static_cast<SoundBank*>(data)};
}

MUSIC_REF LoadMusicInstance(RESOURCE res)
{
	void* hData {res_GetResource(res)};
	if (hData)
	{
		res_DetachResource(res);
	}

	return ((MUSIC_REF)hData);
}
