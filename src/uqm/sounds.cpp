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

#include "sounds.h"

#include "settings.h"
#include "units.h"

namespace uqm
{
SoundBankRef MenuSounds;
SoundBankRef GameSounds;

static constexpr auto MaxSounds {8};
static uint8_t num_sounds = 0;
static uqstl::array<SoundRef, MaxSounds> sound_buf;
static uqstl::array<Element*, MaxSounds> sound_posobj;

void PlaySound(SoundRef S, SoundPosition Pos, Element* PositionalObject,
			   uint8_t Priority)
{
	uint8_t chan {};
	static uqstl::array<uint8_t, NumFxChannels> lru_channel {0, 1, 2, 3};
	static uqstl::array<SoundRef, NumFxChannels> channel {0, 0, 0, 0}; // NOLINT(misc-const-correctness)

	if (S == 0)
	{
		return;
	}

	for (chan = 0; chan < channel.size(); ++chan)
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

void PlayMenuSound(MenuSoundEffect S)
{
	PlaySoundEffect(GetSound(MenuSounds, S),
					0, NotPositional(), nullptr,
					GameSoundPriority);
}

void SuppressSounds(void)
{
	memset(sound_buf.data(), 0, sizeof(sound_buf));
	memset(sound_posobj.data(), 0, sizeof(sound_posobj));
	num_sounds = MaxSounds;
}

void ProcessSound(SoundRef Sound, Element* PositionalObject)
{
	if (num_sounds < MaxSounds)
	{
		sound_buf[num_sounds] = Sound;
		sound_posobj[num_sounds++] = PositionalObject;
	}
}

SoundPosition CalcSoundPosition(Element* ElementPtr)
{
	SoundPosition pos;

	if (ElementPtr == nullptr)
	{
		pos.x = pos.y = 0;
		pos.positional = false;
	}
	else
	{
		const PrimitiveType objtype {GetPrimType(&DisplayArray[ElementPtr->PrimIndex])};
		if (objtype == PrimitiveType::Line)
		{
			pos.x = DisplayArray[ElementPtr->PrimIndex].Object.Line.first.x;
			pos.y = DisplayArray[ElementPtr->PrimIndex].Object.Line.first.y;
		}
		else
		{
			pos.x = DisplayArray[ElementPtr->PrimIndex].Object.Point.x;
			pos.y = DisplayArray[ElementPtr->PrimIndex].Object.Point.y;
		}

		pos.x -= (SpaceWidth() >> 1);
		pos.y -= (SpaceHeight() >> 1);
		pos.x = ResDescale(pos.x);
		pos.y = ResDescale(pos.y);
		pos.positional = true;
	}

	return pos;
}

SoundPosition NotPositional(void)
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

		Element* posobj {(Element*)GetPositionalObject(i)};
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
		const SoundRef* pSound {sound_buf.data()};
		Element* const* pSoundPosObj {sound_posobj.data()};
		do
		{
			const SoundPosition pos = CalcSoundPosition(*pSoundPosObj);
			PlaySound(*pSound++, pos, *pSoundPosObj++,
					  GameSoundPriority);
		} while (--num_sounds);
	}
}

void RemoveSoundsForObject(Element* PosObj)
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

} // namespace uqm
