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

#ifndef UQM_COMM_H_
#define UQM_COMM_H_

#include "commglue.h"
#include "globdata.h"
#include "units.h"
#include "setup.h"
#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"
// for CONVERSATION

#if 0 //defined(__cplusplus)
extern "C" {
#endif

#ifdef COMM_INTERNAL

inline auto SliderY() { return (ResScale(107)); }
inline auto SliderHeight() { return ResScale((!usingSpeech && isPC(uqm::UQMOptions::read().smoothScroll)) ? 4 : DosBool(15, 4)); }

#include "commanim.h"


extern LOCDATA CommData;

static inline bool haveTalkingAnim(void)
{
	return CommData.AlienTalkDesc.NumFrames > 0;
}

static inline bool haveTransitionAnim(void)
{
	return CommData.AlienTransitionDesc.NumFrames > 0;
}

static inline bool wantTalkingAnim(void)
{
	return !(CommData.AlienTalkDesc.AnimFlags & PauseTalking);
}

static inline void setRunTalkingAnim(void)
{
	CommData.AlienTalkDesc.AnimFlags |= WaitTalking;
}

static inline void clearRunTalkingAnim(void)
{
	CommData.AlienTalkDesc.AnimFlags &= ~WaitTalking;
}

static inline bool runningTalkingAnim(void)
{
	return (CommData.AlienTalkDesc.AnimFlags & WaitTalking);
}

static inline void setRunIntroAnim(void)
{
	CommData.AlienTransitionDesc.AnimFlags |= TalkIntro;
}

static inline bool runningIntroAnim(void)
{
	return (CommData.AlienTransitionDesc.AnimFlags & TalkIntro);
}

static inline void setStopTalkingAnim(void)
{
	CommData.AlienTalkDesc.AnimFlags |= TalkDone;
}

static inline void clearStopTalkingAnim(void)
{
	CommData.AlienTalkDesc.AnimFlags &= ~TalkDone;
}

static inline void restartStopTalkingAnim(void)
{
	CommData.AlienTalkDesc.AnimFlags &= TalkDone;
}

static inline bool signaledStopTalkingAnim(void)
{
	return CommData.AlienTalkDesc.AnimFlags & TalkDone;
}

static inline void freezeTalkingAnim(void)
{
	CommData.AlienTalkDesc.AnimFlags |= FreezeTalking;
}

static inline void unFreezeTalkingAnim(void)
{
	CommData.AlienTalkDesc.AnimFlags &= ~FreezeTalking;
}

static inline bool signaledFreezeTalkingAnim(void)
{
	return CommData.AlienTalkDesc.AnimFlags & FreezeTalking;
}

#endif

inline auto TextXOffs() { return ResScale(1); }

inline auto SisTextWidth() { return (SisScreenWidth() - (TextXOffs() << 1)); }

inline auto BackgroundVol() { return (usingSpeech && !VolasPackPresent ? (NormalVolume / 2) : NormalVolume); }


extern void init_communication(void);
extern void uninit_communication(void);

extern uint16_t InitCommunication(CONVERSATION which_comm);
extern void RaceCommunication(void);

inline constexpr auto WaitTrackAll {((uint16_t)~0)};
extern void AlienTalkSegue(uint16_t wait_track);
bool getLineWithinWidth(TEXT* pText, const char** startNext,
						int16_t maxWidth, uint16_t maxChars);

extern GFXRECT CommWndRect; /* comm window rect */

typedef enum
{
	CIM_CROSSFADE_SPACE,
	CIM_CROSSFADE_WINDOW,
	CIM_CROSSFADE_SCREEN,
	CIM_FADE_IN_SCREEN,

	CIM_DEFAULT = CIM_CROSSFADE_SPACE,
} CommIntroMode;
extern void SetCommIntroMode(CommIntroMode, TimePeriod howLong);

extern void EnableTalkingAnim(bool enable);
extern void SetCommDarkMode(bool state);
extern void RedrawSISComWindow(void);
extern void SetCustomBaseLine(uint16_t sentence, GFXPOINT bl, TEXT_ALIGN align);
extern void FlushCustomBaseLine(void);
extern void BlockTalkingAnim(uint16_t trackStart, uint16_t trackEnd);
extern void UpdateDuty(bool talk);

extern void DeltaLastTime(TimePeriod diff);

extern bool cwLock;

inline constexpr auto UseAltFrame {(1 << 0)};
inline constexpr auto UseAltColormap {(1 << 1)};
inline constexpr auto UseAltSong {(1 << 2)};
inline constexpr auto UseAltAll {(UseAltFrame | UseAltColormap | UseAltSong)};
extern uint8_t altResFlags;

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_COMM_H_ */
