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
	uint8_t chan, c;
	static uint8_t lru_channel[NUM_FX_CHANNELS] = {0, 1, 2, 3};
	static SOUND channel[NUM_FX_CHANNELS] = {0, 0, 0, 0};

	if (S == 0)
	{
		return;
	}

	for (chan = 0; chan < NUM_FX_CHANNELS; ++chan)
	{
		if (S == channel[chan])
		{
			break;
		}
	}

	if (chan == NUM_FX_CHANNELS)
	{
		for (chan = 0; chan < NUM_FX_CHANNELS; ++chan)
		{
			if (!ChannelPlaying(chan + MIN_FX_CHANNEL))
			{
				break;
			}
		}

		if (chan == NUM_FX_CHANNELS)
		{
			chan = lru_channel[0];
		}
	}

	channel[chan] = S;

	for (c = 0; c < NUM_FX_CHANNELS - 1; ++c)
	{
		if (lru_channel[c] == chan)
		{
			memmove(&lru_channel[c], &lru_channel[c + 1],
					(NUM_FX_CHANNELS - 1) - c);
			break;
		}
	}
	lru_channel[NUM_FX_CHANNELS - 1] = chan;

	PlaySoundEffect(S, chan + MIN_FX_CHANNEL, Pos, PositionalObject, Priority);
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
		GRAPHICS_PRIM objtype;

		objtype = GetPrimType(&DisplayArray[ElementPtr->PrimIndex]);
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
	uint16_t i;

	for (i = FIRST_SFX_CHANNEL; i <= LAST_SFX_CHANNEL; ++i)
	{
		ELEMENT* posobj;
		if (!ChannelPlaying(i))
		{
			continue;
		}

		posobj = (ELEMENT*)GetPositionalObject(i);
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
		SOUND* pSound;
		ELEMENT** pSoundPosObj;

		pSound = sound_buf;
		pSoundPosObj = sound_posobj;
		do
		{
			SoundPosition pos = CalcSoundPosition(*pSoundPosObj);
			PlaySound(*pSound++, pos, *pSoundPosObj++,
					  GAME_SOUND_PRIORITY);
		} while (--num_sounds);
	}
}

void RemoveSoundsForObject(ELEMENT* PosObj)
{
	int i;

	for (i = FIRST_SFX_CHANNEL; i <= LAST_SFX_CHANNEL; ++i)
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
