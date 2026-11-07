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

#pragma once
#ifndef UQM_LIBS_SNDLIB_H
#define UQM_LIBS_SNDLIB_H


#include "core/types/enum.h"
#include <fmt/base.h>
#include "port.h"
#include "libs/time/timelib.h"
#include "libs/strlib.h"
#include "core/stl/memory.h"
#include "core/stl/vector.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

namespace tfb
{
struct SoundSample;
} // namespace tfb

namespace uqm
{

// The sound samples of a sound bank resource.
class SoundBank
{
public:
	explicit SoundBank(uqstl::vector<tfb::SoundSample*> samples);
	// Stops the sources that play the samples, and destroys the samples.
	~SoundBank();
	SoundBank(const SoundBank&) = delete;
	SoundBank& operator=(const SoundBank&) = delete;

	[[nodiscard]] size_t size() const noexcept
	{
		return m_samples.size();
	}
	// The sample 'index'. The index wraps at the number of samples.
	[[nodiscard]] tfb::SoundSample* sample(size_t index) const;

private:
	uqstl::vector<tfb::SoundSample*> m_samples {};
};

} // namespace uqm

typedef uqm::SoundBank* SOUND_REF;
typedef tfb::SoundSample* SOUND;

struct SoundPosition
{
	bool positional;
	int x, y;
};

// The sound 'index' of 'bank', or nullptr if there is no bank.
inline SOUND GetSound(SOUND_REF bank, size_t index)
{
	return bank != nullptr ? bank->sample(index) : nullptr;
}

inline SOUND GetSound(SOUND_REF bank, ArithmeticEnum auto index)
{
	return GetSound(bank, static_cast<size_t>(*index));
}

namespace uqm
{
using SoundBankPtr = uqstl::unique_ptr<SoundBank>;
// Returns an empty pointer if the resource is not a sound bank.
[[nodiscard]] SoundBankPtr loadSoundBank(RESOURCE res);
} // namespace uqm
typedef tfb::SoundSample** MUSIC_REF;

extern bool InitSound(int argc, char* argv[]);
extern void UninitSound(void);
extern MUSIC_REF LoadMusicFile(const char* pStr);
extern bool InstallAudioResTypes(void);
extern MUSIC_REF LoadMusicInstance(RESOURCE res);
extern bool DestroyMusic(MUSIC_REF MusicRef);

inline constexpr auto MaxChannels {8};
inline constexpr auto MaxVolume {255};
inline constexpr auto NormalVolume {160};
inline constexpr auto MuteVolume {0};

inline constexpr auto FirstSfxChannel {0};
inline constexpr auto MinFxChannel {1};
inline constexpr auto NumFxChannels {4};
inline constexpr auto LastSfxChannel {MinFxChannel + NumFxChannels - 1};
inline constexpr auto NumSfxChannels {MinFxChannel + NumFxChannels};

extern void PLRPlaySong(MUSIC_REF MusicRef, bool Continuous, uint8_t Priority);
extern void PLRStop(MUSIC_REF MusicRef);
extern bool PLRPlaying(MUSIC_REF MusicRef);
extern void PLRSeek(MUSIC_REF MusicRef, uint32_t pos);
extern void PLRPause(MUSIC_REF MusicRef);
extern void PLRResume(MUSIC_REF MusicRef);
extern void snd_PlaySpeech(MUSIC_REF SpeechRef);
extern void snd_StopSpeech(void);
extern void PlayChannel(uint16_t channel, SOUND snd, SoundPosition pos,
						void* positional_object, unsigned char priority);
extern bool ChannelPlaying(uint16_t Channel);
extern void* GetPositionalObject(uint16_t channel);
extern void SetPositionalObject(uint16_t channel, void* positional_object);
extern void UpdateSoundPosition(uint16_t channel, SoundPosition pos);
extern void StopChannel(uint16_t Channel, uint8_t Priority);
extern void SetMusicVolume(uint16_t Volume);
extern void SetChannelVolume(uint16_t Channel, uint16_t Volume, uint8_t Priority);

extern void StopSound(void);
extern bool SoundPlaying(void);

extern uint32_t PLRGetPos(void);

extern void WaitForSoundEnd(uint16_t Channel);
inline constexpr auto TfbsoundWaitAll {(uint16_t)~0};

extern TimeCount FadeMusic(uint8_t end_vol, TimePeriod TimeInterval);
extern uint8_t GetCurrMusicVol(void);

// For music resume option
struct MusicPosition
{
	uint32_t filename_hash;
	uint32_t position;
	TimeCount last_played;
};

extern void SetMusicPosition(void);
extern bool OkayToResume(void);
extern uint32_t GetMusicPosition(void);
extern void ResetMusicResume(void);

static inline void print_mp(const MusicPosition mp)
{
	fmt::print("filename_hash: {:#X}, position: {}, last_played: {}\n",
			   mp.filename_hash, mp.position, mp.last_played.time_since_epoch().count());
}

static inline void print_mp_array(const MusicPosition mp_array[], const uint16_t num_items)
{
	//system ("cls");
	fmt::print("--------------------\n\n");
	for (uint16_t i {num_items}; i > 0; --i)
	{
		fmt::print("Index {} -> ", i);
		print_mp(mp_array[i]);
	}
	fmt::print("Index 0 -> ");
	print_mp(mp_array[0]);
	fmt::print("\n--------------------\n\n");
}

#if 0 //defined(__cplusplus)
}
#endif

#endif // UQM_LIBS_SNDLIB_H
