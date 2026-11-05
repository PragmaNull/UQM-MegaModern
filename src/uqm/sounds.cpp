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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "settings.h"
#include "sounds.h"
#include "units.h"
#include "options.h"

SOUND_REF MenuSounds;
SOUND_REF GameSounds;

#define MAX_SOUNDS 8
static uint8_t num_sounds = 0;
static SOUND sound_buf[MAX_SOUNDS];
static ELEMENT* sound_posobj[MAX_SOUNDS];

void PlaySound(SOUND S, SoundPosition Pos, ELEMENT* PositionalObject,
			   uint8_t Priority)
{
	uint8_t chan {};
	static uint8_t lru_channel[NumFxChannels] = {0, 1, 2, 3};
	static SOUND channel[NumFxChannels] = {0, 0, 0, 0}; // NOLINT(misc-const-correctness)

	if (S == 0)
	{
		return;
	}

	for (chan = 0; chan < NumFxChannels; ++chan)
	{
		if (S == channel[chan])
		{
			break;
		}
	}

	if (chan == NumFxChannels)
	{
		for (chan = 0; chan < NumFxChannels; ++chan)
		{
			if (!ChannelPlaying(chan + MinFxChannel))
			{
				break;
			}
		}

		if (chan == NumFxChannels)
		{
			chan = lru_channel[0];
		}
	}

	channel[chan] = S;

	for (uint8_t c {0}; c < NumFxChannels - 1; ++c)
	{
		if (lru_channel[c] == chan)
		{
			memmove(&lru_channel[c], &lru_channel[c + 1],
					(NumFxChannels - 1) - c);
			break;
		}
	}
	lru_channel[NumFxChannels - 1] = chan;

	PlaySoundEffect(S, chan + MinFxChannel, Pos, PositionalObject, Priority);
}

void PlayMenuSound(MENU_SOUND_EFFECT S)
{
	PlaySoundEffect(GetSound(MenuSounds, S),
					0, NotPositional(), nullptr,
					GAME_SOUND_PRIORITY);
}

void SuppressSounds(void)
{
	memset(sound_buf, 0, sizeof(sound_buf));
	memset(sound_posobj, 0, sizeof(sound_posobj));
	num_sounds = MAX_SOUNDS;
}

void ProcessSound(SOUND Sound, ELEMENT* PositionalObject)
{
	if (num_sounds < MAX_SOUNDS)
	{
		sound_buf[num_sounds] = Sound;
		sound_posobj[num_sounds++] = PositionalObject;
	}
}

SoundPosition
CalcSoundPosition(ELEMENT* ElementPtr)
{
	SoundPosition pos;

	if (ElementPtr == nullptr)
	{
		pos.x = pos.y = 0;
		pos.positional = false;
	}
	else
	{
		const GRAPHICS_PRIM objtype {GetPrimType(&DisplayArray[ElementPtr->PrimIndex])};
		if (objtype == LINE_PRIM)
		{
			pos.x = DisplayArray[ElementPtr->PrimIndex].Object.Line.first.x;
			pos.y = DisplayArray[ElementPtr->PrimIndex].Object.Line.first.y;
		}
		else
		{
			pos.x = DisplayArray[ElementPtr->PrimIndex].Object.Point.x;
			pos.y = DisplayArray[ElementPtr->PrimIndex].Object.Point.y;
		}

		pos.x -= (SPACE_WIDTH >> 1);
		pos.y -= (SPACE_HEIGHT >> 1);
		pos.x = RES_DESCALE(pos.x);
		pos.y = RES_DESCALE(pos.y);
		pos.positional = true;
	}

	return pos;
}

SoundPosition
NotPositional(void)
{
	return CalcSoundPosition(nullptr);
}

/* Updates positional sound effects */
void UpdateSoundPositions(void)
{
	for (uint16_t i {FirstSfxChannel}; i <= LastSfxChannel; ++i)
	{
		if (!ChannelPlaying(i))
		{
			continue;
		}

		ELEMENT* posobj {(ELEMENT*)GetPositionalObject(i)};
		if (posobj != nullptr)
		{
			SoundPosition pos;
			pos = CalcSoundPosition(posobj);
			if (pos.positional)
			{
				UpdateSoundPosition(i, pos);
			}
		}
	}
}

void FlushSounds(void)
{
	if (num_sounds > 0)
	{
		const SOUND* pSound {sound_buf};
		ELEMENT* const* pSoundPosObj {sound_posobj};
		do
		{
			const SoundPosition pos = CalcSoundPosition(*pSoundPosObj);
			PlaySound(*pSound++, pos, *pSoundPosObj++,
					  GAME_SOUND_PRIORITY);
		} while (--num_sounds);
	}
}

void RemoveSoundsForObject(ELEMENT* PosObj)
{
	int i {};

	for (i = FirstSfxChannel; i <= LastSfxChannel; ++i)
	{
		if (GetPositionalObject(i) == PosObj)
		{
			SetPositionalObject(i, nullptr);
		}
	}

	for (i = 0; i < num_sounds; ++i)
	{
		if (sound_posobj[i] == PosObj)
		{
			sound_posobj[i] = nullptr;
		}
	}
}
