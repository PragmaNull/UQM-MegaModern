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

#pragma once
#ifndef UQM_FLASH_H
#define UQM_FLASH_H


#include "core/types/enum.h"
/*
 * This code can draw three kinds of flashing areas.
 * - a rectangular highlight area. The brightness of the area oscilates.
 * - an overlay; an image is laid over an area, with oscilating brightness.
 * - a transition/cross-fade between two images.
 *
 * NB. The graphics lock should not be held when any of the Flash functions
 *     is called.
 *
 *
 * Example:
 *
 * // We create the flash context; it is used to manipulate the flash
 * // rectangle while it exists.
 * FlashContext *fc = Flash_createHighlight (gfxContext, rect);
 * 
 * // Specify how bright the flash is at the beginning and ending of the
 * // sequence.
 * Flash_setMergeFactors(fc, 2, 3, 2);
 *
 * // We change the flashing speed from the defaults.
 * Flash_setSpeed (fc, GameTicksPerSecond, GameTicksPerSecond, GameTicksPerSecond, GameTicksPerSecond);
 * 
 * // During cross-fades, update 8 times per second.
 * Flash_setFrameTime (fc, c_gameSecond / 8);
 *
 * // We start the flashing. The default is to start from the "off" state.
 * Flash_start (fc);
 *
 * // Some other stuff happens
 * ...
 *
 * // The user has activated the selection. We pause for instance while
 * // a pop-up window is active.
 * Flash_pause (fc);
 *
 * // We set the flashing rectangle full on, to indicate the current
 * // selection while the popup is active.
 * Flash_setState (FlashState_on);
 * ...
 * // Continue the flashing.
 * Flash_continue (fc);
 * ...
 * // Modifying the graphics of the area that is flashing:
 * void Flash_preUpdate (fc);
 * ... // do drawing
 * void Flash_postUpdate (fc);
 * ...
 * // We're done. Terminating the flash restores the flash area to its
 * // original state.
 * Flash_terminate (fc);
 *
 *
 * Periodically, Flash_process() should be called on the flash context,
 * so that the flashing square is updated.
 * You can use Flash_nextTime() to determine when the next update is needed,
 * or just call Flash_process() to try (it does no updates if not needed).
 *
 * Limitations:
 *
 * * Functions that draw to the gfxContext or read the original gfxContext
 *   contents, which is most of them, must be called with gfxContext having
 *   the same clip-rect as it did when other drawing functions were called.
 *   Otherwise, original contents restoration may draw to the wrong area, or
 *   the wrong area may be read.
 *   There may be cases where one would *want* that to happen, and such
 *   cases are not covered by this limitation.
 * * Multiple flashes may be used simultaneously, but don't let them
 *   overlap; artifacts would occur.
 */

#include "core/stl/vector.h"
#include "libs/gfxlib.h"
#include "libs/time/timelib.h"

namespace uqm
{
enum class FlashState
{
	FadeIn = 0,
	// Someway between on and off, going towards on.
	On = 1,
	// The overlay image is visible at 100% opacity.
	FadeOut = 2,
	// Someway between on and off, going towards off.
	Off = 3,
	// The overlay image is not visible.
};
consteval bool isArithmeticEnum(FlashState) { return true; }

struct FlashContext;

#ifdef FLASH_INTERNAL
enum class FlashType
{
	Highlight,
	Transition,
	Overlay,
};

struct FlashContext
{
	GfxContextRef gfxContext;
	// The graphics context used for drawing.

	GfxRect rect;
	// The rectangle to flash.

	FrameRef original;
	// The original contents of the flash area.

	FlashType type;
	// The type of flash animation.

	union
	{
		/*struct {
		} highlight;*/
		struct
		{
			FrameRef first;
			// The first image from the transition (cross-fade).
			// (FrameRef) 0 means that the original is to be used.
			FrameRef final;
			// The last image from the transition.
			// (FrameRef) 0 means that the original is to be used.
		} transition;
		struct
		{
			FrameRef frame;
		} overlay;
	} u;

	int startNumer;
	// Numerator for the merge factor for the on state.
	int endNumer;
	// Numerator for the merge factor for the off state.
	int denom;
	// Denominator for the merge factor.

	TimePeriod fadeInTime;
	TimePeriod onTime;
	TimePeriod fadeOutTime;
	TimePeriod offTime;

	TimePeriod frameTime;

	FlashState state;
	TimeCount lastStateTime;
	// Time of the last state change.
	TimeCount lastFrameTime;
	// Time of the last frame draw.

	bool started;
	bool paused;
	bool isPulsing;

	uqstl::vector<FrameRef> cache;
	uint16_t cacheSize;

	uint16_t lastFrameIndex;
	// Last frame drawn; used to determine whether a frame needs to
	// be redawn. If a cache is used, this is the index in the cache.
	// If no cache is used, this is either 0, 1, or 2, for
	// the respectively first, last, or other frame for the flash
	// animation.
};

inline constexpr auto FlashDefaultFadeInTime {TimePeriod::zero()};
inline constexpr auto FlashDefaultOnTime {c_gameSecond / 8};
inline constexpr auto FlashDefaultFadeOutTime {TimePeriod::zero()};
inline constexpr auto FlashDefaultOffTime {c_gameSecond / 8};

inline constexpr auto FlashDefaultCacheSize {9};
#endif /* FLASH_INTERNAL */


FlashContext* Flash_createHighlight(GfxContextRef gfxContext, const GfxRect* rect);
FlashContext* Flash_createTransition(GfxContextRef gfxContext,
									 const GfxPoint* origin, FrameRef first, FrameRef final);
FlashContext* Flash_createOverlay(GfxContextRef gfxContext,
								  const GfxPoint* origin, FrameRef overlay);

void Flash_setState(FlashContext* context, FlashState state,
					TimePeriod timeSpentInState);
void Flash_start(FlashContext* context);
void Flash_terminate(FlashContext* context);
void Flash_pause(FlashContext* context);
void Flash_continue(FlashContext* context);
void Flash_process(FlashContext* context);
void Flash_setSpeed(FlashContext* context, TimePeriod fadeInTime,
					TimePeriod onTime, TimePeriod fadeOutTime, TimePeriod offTime);
void Flash_setMergeFactors(FlashContext* context, int startNumer,
						   int endNumer, int denom);
void Flash_setFrameTime(FlashContext* context, TimePeriod frameTime);
void Flash_setPulseBox(FlashContext* context, bool isPulsing);
bool Flash_getPulseBox(FlashContext* context);
TimeCount Flash_nextTime(FlashContext* context);
void Flash_setRect(FlashContext* context, const GfxRect* rect);
void Flash_getRect(FlashContext* context, GfxRect* rect);
void Flash_setOverlay(FlashContext* context, const GfxPoint* origin,
					  FrameRef overlay, bool cleanup);
void Flash_preUpdate(FlashContext* context);
void Flash_postUpdate(FlashContext* context);
void Flash_setCacheSize(FlashContext* context, uint16_t size);
uint16_t Flash_getCacheSize(const FlashContext* context);
void Flash_UpdateOriginal(FlashContext* context);

} // namespace uqm

#endif // UQM_FLASH_H
