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

#include "settings.h"

#include "globdata.h"
#include "core/misc/bittricks.h"
#include "setup.h"


namespace uqm
{
static MusicRef LastMusicRef;
static bool LastContinuous;
static uint8_t LastPriority;

void ToggleMusic(void)
{
	uqm::globalFlags() ^= MusicDisabled;
	if (LastPriority <= 1)
	{
		if (uqm::globalFlags() & MusicDisabled)
		{
			PLRStop(LastMusicRef);
		}
		else if (LastMusicRef)
		{
			PLRPlaySong(LastMusicRef, LastContinuous, LastPriority);
		}
	}
}

void PlayMusic(MusicRef musicRef, bool Continuous, uint8_t Priority)
{
	LastMusicRef = musicRef;
	LastContinuous = Continuous;
	LastPriority = Priority;

	if (
#ifdef NEVER
		Priority > 1
		||
#endif /* NEVER */
		!(uqm::globalFlags() & MusicDisabled))
	{
		PLRPlaySong(musicRef, Continuous, Priority);
	}
}

void SeekMusic(uint32_t pos)
{
	PLRSeek(LastMusicRef, pos);
}

void StopMusic(void)
{
	PLRStop(LastMusicRef);
	LastMusicRef = 0;
}

void ResumeMusic(void)
{
	PLRResume(LastMusicRef);
}

void PauseMusic(void)
{
	PLRPause(LastMusicRef);
}

void PlayMusicResume(MusicRef musicRef, uint8_t Volume)
{
	SetMusicVolume(MuteVolume);
	PlayMusic(musicRef, true, 1);

	if (OkayToResume())
	{
		SeekMusic(GetMusicPosition());
		FadeMusic(Volume, c_gameSecond * 2);
	}
	else
	{
		SetMusicVolume(Volume);
	}
}

void ToggleSoundEffect(void)
{
	uqm::globalFlags() ^= SoundDisabled;
}

void PlaySoundEffect(SoundRef S, uint16_t Channel, SoundPosition Pos,
					 void* PositionalObject, uint8_t Priority)
{
	if (!(uqm::globalFlags() & SoundDisabled))
	{
		SetChannelVolume(Channel, MaxVolume >> 1, Priority);
		//SetChannelRate (Channel, GetSampleRate (S), Priority);
		PlayChannel(Channel, S, Pos, PositionalObject, Priority);
	}
}

void PlaySpeechEffect(SoundRef S, SoundPosition Pos, void* PositionalObject, uint8_t Priority)
{
	if (!(uqm::globalFlags() & SoundDisabled))
	{
		PlayChannel(6, S, Pos, PositionalObject, Priority);
	}
}

} // namespace uqm
