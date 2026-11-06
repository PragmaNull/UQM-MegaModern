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

#ifndef UQM_COMMANIM_H_
#define UQM_COMMANIM_H_

#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

// Some background: every animation has a neutral frame which returns
// the image to the state it was in before the animation began. Which
// frame is neutral depends on the animation type.
// Animation types:
inline constexpr auto RandomAnim {(1 << 0)};
// The next index is randomly chosen.
// The first frame is neutral
inline constexpr auto CircularAnim {(1 << 1)};
// After the last index has been reached, the animation starts over.
// The last frame is neutral. This complicates the animation task.
inline constexpr auto YoyoAnim {(1 << 2)};
// After the last index has been reached, the order that the
// animation frames are used is reversed.
// The first frame is neutral
inline constexpr auto AnimMask {(RandomAnim | CircularAnim | YoyoAnim)};
// Mask of all animation types.
// Static frames do not have any of these flags set.

inline constexpr auto WaitTalking {(1 << 3)};
// This is set in AlienTalkDesc when the talking animation is active
// or should be active.
// In AlienAmbientArray, this is set for those ambient animations
// which can not be active while the talking animation is active.
// Such animations stop at the end of the current animation cycle
// when the talking animation activates.
inline constexpr auto PauseTalking {(1 << 4)};
// Set in AlienTalkDesc when we do not want the talking animation
inline constexpr auto TalkIntro {(1 << 5)};
// In AlienTransitionDesc: indicates a transition to talking state
inline constexpr auto TalkDone {(1 << 6)};
// In AlienTransitionDesc: indicates a transition to silent state
// In AlienTalkDesc: signals the end of talking animation

inline constexpr auto AnimDisabled {(1L << 8)}; // BW (needed for news anchor and animated background)

inline constexpr auto FreezeTalking {(1 << 9)};
// Kr (needed for blocking animations that are WaitTalking even
// if talking itself is stopped. Used only in PS-DOS style scrolling)

inline constexpr auto RestartAllAfter {(1 << 10)};
inline constexpr auto StopAllAfter {(1 << 11)};
// Kruzen: Needed for OneShotAnim in HdResolutionFactor to define - should we restart
// other ambient animations or no

inline constexpr auto ImmumeToRestart {(1 << 12)};
inline constexpr auto ImmumeToStop {(1 << 13)};
// Kruzen: Some animations needed to be handled individually

inline constexpr auto AlphaMaskAnim {(1 << 14)};
// Kruzen: New type of animations: Draw transparent frames
// on top of all (sets fullRedraw to true

inline constexpr auto TriggerFullRedraw {(1 << 15)};
// When animation plays - it triggers full redraw


// (otherwise there'll be nasty, unwanted pauses in the conversation.)

inline constexpr auto ColorxformAnim {PauseTalking};

inline constexpr auto OneShotAnim {TalkIntro};
// Set in AlienAmbientArray for animations that should be
// disabled after they run once.


struct AnimationDesc
{
	uint16_t StartIndex;
	// Index of the first image (for image animation) or
	// index of the first color map (for palette animation)
	uint8_t NumFrames;
	// Number of frames in the animation.

	uint16_t AnimFlags;
	// One of RandomAnim, CircularAnim, or YoyoAnim
	// plus flags (WaitTalking, AnimDisabled)
	// JMS: Changed from uint8_t to uint16_t to house more possible flags

	uint16_t BaseFrameRate;
	// Minimum interframe delay
	uint16_t RandomFrameRate;
	// Maximum additional interframe delay
	// Actual delay: BaseFrameRate + Random(0..RandomFrameRate)
	uint16_t BaseRestartRate;
	// Minimum delay before restarting animation
	uint16_t RandomRestartRate;
	// Maximum additional delay before restarting animation
	// Actual delay: BaseRestartRate + Random(0..RandomRestartRate)

	uint32_t BlockMask;
	// Bit mask of the indices of all animations that can not
	// be active at the same time as this animation, usually,
	// due to the image overlap conflicts.
};

inline constexpr auto MaxAnimations {30}; // JMS: Was 20


#ifdef COMM_INTERNAL

typedef enum
{
	DOWN_DIR = -1, // Animation indices are decreasing
	NO_DIR = 0,
	UP_DIR = 1, // Animation indices are increasing
} ANIM_DIR;

typedef enum
{
	PICTURE_ANIM,
	// Parts of a picture are replaced
	COLOR_ANIM
	// Colormap tricks on a picture
} ANIM_TYPE;

// Describes an active animation.
struct AnimSequence
{
	AnimationDesc* ADPtr;
	uint32_t Alarm;
	ANIM_DIR Direction;
	uint16_t CurIndex;
	uint16_t NextIndex;
	uint16_t FramesLeft;
	ANIM_TYPE AnimType;
	bool Change;
};
#endif

struct AnimSequence;

// Kruzen: HdResolutionFactor filters that are layered on top
// and all stuff needed.
// Because this basically imitates color xform in HdResolutionFactor
// in truecolor format - all color transformations
// are hadled by the same algorithms as in SD.
// We just need to take current color and draw whatever
// nessessary.
// Although new .ct need to be created

inline constexpr auto CommColormapIndex {10};
inline constexpr auto MaxFilters {3}; // Could be more, but for now more that enough

inline constexpr auto TurnOffOft {(1 << 0)};   // on full transparency
inline constexpr auto TurnOffOfo {(1 << 1)};   // on full opacity
inline constexpr auto FramedFilter {(1 << 2)}; // filter draws frame and nothing else
inline constexpr auto FilterDisabled {(1 << 3)};
inline constexpr auto SwitchOffAnims {(1 << 4)};
inline constexpr auto SwitchOnAnims {(1 << 5)};

struct AnimFilter
{
	uint8_t ColorIndex;
	// Index of filter color in color
	// table with index 10 aka alienrace.ct
	// Can be used as frame offset for FramedFilter

	uint8_t OpacityIndex;
	// Index of opacity color in color
	// table with index 10 aka alienrace.ct
	// RED channel would be used as alpha channel

	int16_t FrameIndex;
	// If we want to use frame from CommData.AlienFrame
	// If -1 then we will draw a rectangle that covers all context

	uint8_t Kind;
	// A drawkind from DrawKind enum

	uint8_t Flags;
	// Any possible flags
};

struct AnimFilterSet
{
	uint8_t NumFilters;

	AnimFilter FilterArray[MaxFilters];
};

extern AnimFilterSet FilterData;

// Returns true if there was an animation change
extern bool DrawAlienFrame(AnimSequence* pSeq, uint16_t Num, bool fullRedraw);
extern void InitCommAnimations(void);
extern bool ProcessCommAnimations(bool fullRedraw, bool paused);
extern void ShutYourMouth(void);
extern void SwitchSequences(bool enableAll);
extern void RunOneTimeSequence(uint16_t animIndex, uint16_t flags);
extern void EngageFilters(AnimFilterSet* f_desc);
extern void DisengageFilters(void);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_COMMANIM_H_ */
