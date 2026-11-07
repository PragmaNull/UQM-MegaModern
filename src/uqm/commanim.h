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
#ifndef UQM_COMMANIM_H
#define UQM_COMMANIM_H

#include "core/misc/bittricks.h"
#include "core/types/enum.h"
#include "libs/gfxlib.h"

namespace uqm
{
// Some background: every animation has a neutral frame which returns
// the image to the state it was in before the animation began. Which
// frame is neutral depends on the animation type.
// Animation types:
enum class AnimationFlags : uint16_t
{
	None = 0,
	Random = 1 << 0,
	// The next index is randomly chosen.
	// The first frame is neutral
	Circular = 1 << 1,
	// After the last index has been reached, the animation starts over.
	// The last frame is neutral. This complicates the animation task.
	Yoyo = 1 << 2,
	// After the last index has been reached, the order that the
	// animation frames are used is reversed.
	// The first frame is neutral
	AnimMask = Random | Circular | Yoyo,
	// Mask of all animation types.
	// Static frames do not have any of these flags set.

	WaitTalking = 1 << 3,
	// This is set in AlienTalkDesc when the talking animation is active
	// or should be active.
	// In AlienAmbientArray, this is set for those ambient animations
	// which can not be active while the talking animation is active.
	// Such animations stop at the end of the current animation cycle
	// when the talking animation activates.
	PauseTalking = 1 << 4,
	// Set in AlienTalkDesc when we do not want the talking animation
	TalkIntro = 1 << 5,
	// In AlienTransitionDesc: indicates a transition to talking state
	TalkDone = 1 << 6,
	// In AlienTransitionDesc: indicates a transition to silent state
	// In AlienTalkDesc: signals the end of talking animation

	Disabled = 1 << 8, // BW (needed for news anchor and animated background)

	FreezeTalking = 1 << 9,
	// Kr (needed for blocking animations that are WaitTalking even
	// if talking itself is stopped. Used only in PS-DOS style scrolling)

	RestartAllAfter = 1 << 10,
	StopAllAfter = 1 << 11,
	// Kruzen: Needed for OneShotAnim in HdResolutionFactor to define - should we restart
	// other ambient animations or no

	ImmuneToRestart = 1 << 12,
	ImmuneToStop = 1 << 13,
	// Kruzen: Some animations needed to be handled individually

	AlphaMask = 1 << 14,
	// Kruzen: New type of animations: Draw transparent frames
	// on top of all (sets fullRedraw to true

	TriggerFullRedraw = 1 << 15,
	// When animation plays - it triggers full redraw


	// (otherwise there'll be nasty, unwanted pauses in the conversation.)

	ColorXform = PauseTalking,

	OneShot = TalkIntro,
	// Set in AlienAmbientArray for animations that should be
	// disabled after they run once.
};
ENUM_BITWISE_OPS(AnimationFlags)


struct AnimationDesc
{
	uint16_t StartIndex;
	// Index of the first image (for image animation) or
	// index of the first color map (for palette animation)
	uint8_t NumFrames;
	// Number of frames in the animation.

	AnimationFlags AnimFlags;
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

enum class AnimDir
{
	Down = -1, // Animation indices are decreasing
	None = 0,
	Up = 1, // Animation indices are increasing
};
consteval bool isArithmeticEnum(AnimDir) { return true; }

enum class AnimType
{
	Picture,
	// Parts of a picture are replaced
	Color
	// Colormap tricks on a picture
};

// Describes an active animation.
struct AnimSequence
{
	AnimationDesc* ADPtr;
	uint32_t Alarm;
	AnimDir Direction;
	uint16_t CurIndex;
	uint16_t NextIndex;
	uint16_t FramesLeft;
	AnimType AnimType;
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

enum class FilterFlags : uint8_t
{
	None = 0,
	TurnOffOft = 1 << 0, // on full transparency
	TurnOffOfo = 1 << 1, // on full opacity
	Framed = 1 << 2,	 // filter draws frame and nothing else
	Disabled = 1 << 3,
	SwitchOffAnims = 1 << 4,
	SwitchOnAnims = 1 << 5,
};
ENUM_BITWISE_OPS(FilterFlags)

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

	DrawKind Kind;
	// A drawkind from DrawKind enum

	FilterFlags Flags;
	// Any possible flags
};

struct AnimFilterSet
{
	uint8_t NumFilters;

	uqstl::array<AnimFilter, MaxFilters> FilterArray;
};

extern AnimFilterSet FilterData;

// Returns true if there was an animation change
extern bool DrawAlienFrame(AnimSequence* pSeq, uint16_t Num, bool fullRedraw);
extern void InitCommAnimations(void);
extern bool ProcessCommAnimations(bool fullRedraw, bool paused);
extern void ShutYourMouth(void);
extern void SwitchSequences(bool enableAll);
extern void RunOneTimeSequence(uint16_t animIndex, AnimationFlags flags);
extern void EngageFilters(AnimFilterSet* f_desc);
extern void DisengageFilters(void);

} // namespace uqm

#endif // UQM_COMMANIM_H
