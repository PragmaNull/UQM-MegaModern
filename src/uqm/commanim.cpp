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

#define COMM_INTERNAL
#include "commanim.h"

#include "comm.h"
#include "element.h"
#include "options.h"
#include "setup.h"
#include "core/misc/bittricks.h"
#include "libs/graphics/cmap.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "uqm/random/UQMRandom.h"


static TimeCount LastTime;
static AnimSequence Sequences[MaxAnimations + 2];
// 2 extra for Talk and Transition animations
static uint32_t ActiveMask;
// Bit mask of all animations that are currently active.
// Bit 'i' is set if the animation with index 'i' is active.
static AnimationDesc TalkDesc;
static AnimationDesc TransitDesc;
static AnimSequence* Talk;
static AnimSequence* Transit;
static uint16_t FirstAmbient;
static uint16_t TotalSequences;
static bool doFullRedraw = false;

//stuff for Alpha animation in HdResolutionFactor
bool filterEnabled = false;
AnimFilterSet FilterData;

static inline int16_t getAlphaChannel(uint8_t index)
{
	return (int16_t)GetColorMapColor(CommColormapIndex, index).r;
}

static inline uint32_t randomFrameRate(AnimSequence* pSeq)
{
	const AnimationDesc* ADPtr = pSeq->ADPtr;

	return ADPtr->BaseFrameRate + uqm::random().next() % (ADPtr->RandomFrameRate + 1);
}

static inline uint32_t randomRestartRate(AnimSequence* pSeq)
{
	const AnimationDesc* ADPtr = pSeq->ADPtr;

	return ADPtr->BaseRestartRate + uqm::random().next() % (ADPtr->RandomRestartRate + 1);
}

static inline uint16_t randomFrameIndex(AnimSequence* pSeq, uint16_t from)
{
	const AnimationDesc* ADPtr = pSeq->ADPtr;

	return from + uqm::random().next() % (ADPtr->NumFrames - from);
}

static void SetupAmbientSequences(AnimSequence* pSeq, uint16_t Num)
{
	for (uint16_t i {0}; i < Num; ++i, ++pSeq)
	{
		AnimationDesc* ADPtr = &CommData.AlienAmbientArray[i];

		memset(pSeq, 0, sizeof(*pSeq));

		pSeq->ADPtr = ADPtr;
		if (ADPtr->AnimFlags & ColorxformAnim)
		{
			pSeq->AnimType = COLOR_ANIM;
		}
		else
		{
			pSeq->AnimType = PICTURE_ANIM;
		}
		pSeq->Direction = UP_DIR;
		pSeq->FramesLeft = ADPtr->NumFrames;
		// Default: first frame is neutral
		if (ADPtr->AnimFlags & RandomAnim)
		{ // Set a random frame/colormap
			pSeq->NextIndex = uqm::random().next() % ADPtr->NumFrames;
		}
		else if (ADPtr->AnimFlags & YoyoAnim)
		{ // Skip the first frame/colormap (it's neutral)
			pSeq->NextIndex = 1;
			--pSeq->FramesLeft;
		}
		else if (ADPtr->AnimFlags & CircularAnim)
		{ // Exception that makes everything more painful:
			// *Last* frame is neutral
			pSeq->CurIndex = ADPtr->NumFrames - 1;
			pSeq->NextIndex = 0;
		}

		pSeq->Alarm = randomRestartRate(pSeq) + 1;
	}
}

static void SetupTalkSequence(AnimSequence* pSeq, AnimationDesc* ADPtr)
{
	memset(pSeq, 0, sizeof(*pSeq));
	// Initially disabled, and until needed
	ADPtr->AnimFlags |= AnimDisabled;
	pSeq->ADPtr = ADPtr;
	pSeq->AnimType = PICTURE_ANIM;
}

static inline bool animAtNeutralIndex(AnimSequence* pSeq)
{
	const AnimationDesc* ADPtr = pSeq->ADPtr;

	if (ADPtr->AnimFlags & CircularAnim)
	{ // CircularAnim's neutral frame is the last
		return pSeq->NextIndex == 0;
	}
	else
	{ // All others, neutral frame is the first
		return pSeq->CurIndex == 0;
	}
}

static inline bool conflictsWithTalkingAnim(AnimSequence* pSeq)
{
	const AnimationDesc* ADPtr = pSeq->ADPtr;

	return ADPtr->AnimFlags & CommData.AlienTalkDesc.AnimFlags & WaitTalking;
}

static void ProcessColormapAnims(AnimSequence* pSeq, uint16_t Num)
{
	for (uint16_t i {0}; i < Num; ++i, ++pSeq)
	{
		const AnimationDesc* ADPtr = pSeq->ADPtr;

		if ((ADPtr->AnimFlags & AnimDisabled)
			|| pSeq->AnimType != COLOR_ANIM
			|| !pSeq->Change)
		{
			continue;
		}

		XFormColorMap(GetColorMapAddress(CommData.AlienColorMap, ADPtr->StartIndex + pSeq->CurIndex),
					  TimePeriod {pSeq->Alarm - 1});
		pSeq->Change = false;
	}
}

static bool AdvanceAmbientSequence(AnimSequence* pSeq)
{
	bool active {};
	const AnimationDesc* ADPtr = pSeq->ADPtr;

	--pSeq->FramesLeft;
	// YoyoAnim does not actually end until it comes back
	// in reverse direction, even if FramesLeft gets to 0 here
	if (pSeq->FramesLeft
		|| ((ADPtr->AnimFlags & YoyoAnim) && pSeq->NextIndex != 0))
	{
		active = true;
		pSeq->Alarm = randomFrameRate(pSeq) + 1;
	}
	else
	{ // last animation frame
		active = false;
		pSeq->Alarm = randomRestartRate(pSeq) + 1;

		// RandomAnim must end on a neutral frame
		if (ADPtr->AnimFlags & RandomAnim)
		{
			pSeq->NextIndex = 0;
		}
	}

	// Will draw the next frame or change to next colormap
	pSeq->CurIndex = pSeq->NextIndex;
	pSeq->Change = true;

	if (pSeq->FramesLeft == 0)
	{ // Animation ended
		// Set it up for the next round
		pSeq->FramesLeft = ADPtr->NumFrames;

		if (ADPtr->AnimFlags & YoyoAnim)
		{ // YoyoAnim never draws the first frame
			// ("first" depends on direction)
			--pSeq->FramesLeft;
			pSeq->Direction = (ANIM_DIR)-pSeq->Direction;
		}
		else if (ADPtr->AnimFlags & CircularAnim)
		{ // Rewind the CircularAnim
			// NextIndex will be brought to 0 just below
			pSeq->NextIndex = -1;
		}
		// RandomAnim is setup just below
	}

	if (ADPtr->AnimFlags & RandomAnim)
	{
		pSeq->NextIndex = randomFrameIndex(pSeq, 0);
	}
	else
	{
		pSeq->NextIndex += pSeq->Direction;
	}

	return active;
}

static void ResetSequence(AnimSequence* pSeq)
{
	// Reset the animation and cause a redraw of the neutral frame,
	// assuming it is not AnimDisabled
	// NOTE: This does not handle CircularAnim properly
	pSeq->Direction = NO_DIR;
	pSeq->CurIndex = 0;
	pSeq->Change = true;
}

static void AdvanceTalkingSequence(AnimSequence* pSeq, uint32_t ElapsedTicks)
{
	// We use the actual descriptor for flags processing and
	// a copied one for drawing. A copied one is updated only
	// when it is safe to do so.
	AnimationDesc* ADPtr = pSeq->ADPtr;

	if (pSeq->Direction == NO_DIR)
	{ // just starting now
		pSeq->Direction = UP_DIR;
		// It's now safe to pick up new Talk descriptor if changed
		// (e.g. Zoq and Pik taking turns to talk)
		if (CommData.AlienTalkDesc.StartIndex != ADPtr->StartIndex)
		{ // copy the new one
			*ADPtr = CommData.AlienTalkDesc;
		}

		assert(pSeq->CurIndex == 0);
		pSeq->Alarm = 0; // now!
		ADPtr->AnimFlags &= ~AnimDisabled;
	}

	if (pSeq->CurIndex == 0 && signaledFreezeTalkingAnim())
	{ // drop routine if we are freezed
		return;
	}

	if (pSeq->Alarm > ElapsedTicks)
	{ // Not time yet
		pSeq->Alarm -= ElapsedTicks;
		return;
	}

	// Time to start or advance the animation
	pSeq->Alarm = randomFrameRate(pSeq);
	pSeq->Change = true;
	// Talking animation is like RandomAnim, except that
	// random frames always alternate with the neutral one
	// The animation does not stop until we reset it
	// Kruzen: do not advance sequence if we're freezed
	// (all other animations still will be blocked)
	if (pSeq->CurIndex == 0 && !signaledFreezeTalkingAnim())
	{ // random frame next if not freezed
		pSeq->CurIndex = randomFrameIndex(pSeq, 1);
		pSeq->Alarm += randomRestartRate(pSeq);
	}
	else
	{ // neutral frame next
		pSeq->CurIndex = 0;
	}
}

static bool AdvanceTransitSequence(AnimSequence* pSeq, uint32_t ElapsedTicks)
{
	bool done = false;
	// We use the actual descriptor for flags processing and
	// a copied one for drawing. A copied one is updated only
	// when it is safe to do so.
	AnimationDesc* ADPtr = pSeq->ADPtr;

	if (pSeq->Direction == NO_DIR)
	{					 // just starting now
		pSeq->Alarm = 0; // now!
		ADPtr->AnimFlags &= ~AnimDisabled;
	}

	if (pSeq->Alarm > ElapsedTicks)
	{ // Not time yet
		pSeq->Alarm -= ElapsedTicks;
		return false;
	}

	// Time to start or advance the animation
	pSeq->Change = true;

	if (pSeq->Direction == NO_DIR)
	{ // just starting now
		pSeq->FramesLeft = ADPtr->NumFrames;
		// Both INTRO and DONE may be set at the same time,
		// when e.g. Zoq and Pik are taking turns to talk
		// Process the DONE transition first to go into
		// a neutral state before switching over.
		if (CommData.AlienTransitionDesc.AnimFlags & TalkDone)
		{
			pSeq->Direction = DOWN_DIR;
			pSeq->CurIndex = ADPtr->NumFrames - 1;
		}
		else if (CommData.AlienTransitionDesc.AnimFlags & TalkIntro)
		{
			pSeq->Direction = UP_DIR;
			// It's now safe to pick up new Transition descriptor if changed
			// (e.g. Zoq and Pik taking turns to talk)
			if (CommData.AlienTransitionDesc.StartIndex
				!= ADPtr->StartIndex)
			{ // copy the new one
				*ADPtr = CommData.AlienTransitionDesc;
			}

			pSeq->CurIndex = 0;
		}
	}

	--pSeq->FramesLeft;
	if (pSeq->FramesLeft == 0)
	{ // animation is done
		if (pSeq->Direction == UP_DIR)
		{ // done with TalkIntro transition
			CommData.AlienTransitionDesc.AnimFlags &= ~TalkIntro;
		}
		else if (pSeq->Direction == DOWN_DIR)
		{ // done with TalkDone transition
			CommData.AlienTransitionDesc.AnimFlags &= ~TalkDone;

			// Done with all transition frames
			ADPtr->AnimFlags |= AnimDisabled;
			done = true;
		}
		pSeq->Direction = NO_DIR;
	}
	else
	{ // next frame
		pSeq->Alarm = randomFrameRate(pSeq);
		pSeq->CurIndex += pSeq->Direction;
	}

	return done;
}

void InitCommAnimations(void)
{
	ActiveMask = 0;

	TalkDesc = CommData.AlienTalkDesc;
	TransitDesc = CommData.AlienTransitionDesc;

	// Animation sequences have to be drawn in reverse, and
	// talk animations have to be drawn last (so we add them first)
	TotalSequences = 0;
	// Transition animation last
	Transit = Sequences + TotalSequences;
	SetupTalkSequence(Transit, &TransitDesc);
	++TotalSequences;
	// Talk animation second last
	Talk = Sequences + TotalSequences;
	SetupTalkSequence(Talk, &TalkDesc);
	++TotalSequences;
	FirstAmbient = TotalSequences;
	SetupAmbientSequences(Sequences + FirstAmbient, CommData.NumAnimations);
	TotalSequences += CommData.NumAnimations;

	LastTime = GetTimeCounter();
}

bool ProcessCommAnimations(bool FullRedraw, bool paused)
{
	if (paused)
	{ // Drive colormap xforms and nothing else
		// Triggered only during seeking
		if (XFormColorMap_step())
		{ // Once seeking is done and colors have
			// changed - redraw a full frame
			doFullRedraw = true;
		}
		return false;
	}
	else
	{
		uint16_t i {};
		bool Change {};
		bool CanTalk = true;
		TimeCount CurTime;

		CurTime = GetTimeCounter();
		const uint32_t ElapsedTicks {static_cast<uint32_t>((CurTime - LastTime).count())};
		LastTime = CurTime;

		if (doFullRedraw)
		{ // to make frame colors in sync
			// mostly for HdResolutionFactor
			FullRedraw = true;
			doFullRedraw = false;
		}

		// Process ambient animations
		uint32_t NextActiveMask {ActiveMask};
		AnimSequence* pSeq {Sequences + FirstAmbient};
		for (i = 0; i < CommData.NumAnimations; ++i, ++pSeq)
		{
			const AnimationDesc* ADPtr = pSeq->ADPtr;
			const uint32_t ActiveBit = 1L << i;

			if (ADPtr->AnimFlags & AnimDisabled)
			{
				continue;
			}

			if (pSeq->Direction == NO_DIR)
			{ // animation is paused
				if (!conflictsWithTalkingAnim(pSeq))
				{ // start it up
					pSeq->Direction = UP_DIR;
				}
			}
			else if (pSeq->Alarm > ElapsedTicks)
			{ // not time yet
				pSeq->Alarm -= ElapsedTicks;
			}
			else if (ActiveMask & ADPtr->BlockMask)
			{ // animation is blocked
				assert(!(ActiveMask & ActiveBit) && "Check animations' mutual blocking masks");
				assert(animAtNeutralIndex(pSeq));
				// reschedule
				pSeq->Alarm = randomRestartRate(pSeq) + 1;
				continue;
			}
			else
			{ // Time to start or advance the animation
				if (AdvanceAmbientSequence(pSeq))
				{ // Animation is active this frame and the next
					ActiveMask |= ActiveBit;
					NextActiveMask |= ActiveBit;
				}
				else
				{ // Animation remains active this frame but not the next
					// This keeps any conflicting animations (BlockMask)
					// from activating in the same frame and scribbling over
					// our last image.
					NextActiveMask &= ~ActiveBit;
				}
			}

			if (pSeq->AnimType == PICTURE_ANIM && pSeq->Direction != NO_DIR
				&& conflictsWithTalkingAnim(pSeq))
			{
				// We want to talk, but this is a running picture animation
				// which conflicts with the talking animation
				// See if it is safe to stop it now.
				if (animAtNeutralIndex(pSeq))
				{ // pause the animation
					pSeq->Direction = NO_DIR;
					NextActiveMask &= ~ActiveBit;
					// Talk animation is drawn last, so it's not a conflict
					// for this frame. The talk animation will be drawn
					// over the neutral frame.
				}
				else
				{ // Otherwise, let the animation run until it's safe
					CanTalk = false;
				}
			}

			if (pSeq->Change && ADPtr->AnimFlags & TriggerFullRedraw)
			{
				FullRedraw = true;
			}
		}
		// All ambient animations have been processed. Advance the mask.
		ActiveMask = NextActiveMask;

		// Process the talking and transition animations
		if (CanTalk && haveTalkingAnim() && runningTalkingAnim())
		{
			bool done = false;

			if (signaledStopTalkingAnim() && haveTransitionAnim())
			{ // Run the transition. We will clear everything
				// when it is done
				CommData.AlienTransitionDesc.AnimFlags |= TalkDone;
			}

			if (CommData.AlienTransitionDesc.AnimFlags
					& (TalkIntro | TalkDone)
				&& !signaledFreezeTalkingAnim())
			{ // Transitioning in or out of talking
				if ((CommData.AlienTransitionDesc.AnimFlags & TalkDone)
					&& Transit->Direction == NO_DIR)
				{ // This is needed when switching talking anims
					ResetSequence(Talk);
				}
				done = AdvanceTransitSequence(Transit, ElapsedTicks);
			}
			else if (!signaledStopTalkingAnim())
			{ // Talking, transition is done
				AdvanceTalkingSequence(Talk, ElapsedTicks);
			}
			else
			{ // Not talking
				ResetSequence(Talk);
				done = true;
			}

			if (signaledStopTalkingAnim() && done)
			{
				clearRunTalkingAnim();
				clearStopTalkingAnim();
			}
		}
		else
		{ // Not talking -- disable talking anim if it is done
			if (Talk->Direction == NO_DIR)
			{
				TalkDesc.AnimFlags |= AnimDisabled;
			}
		}

		BatchGraphics();

		// Draw all animations
		{
			const bool ColorChange = XFormColorMap_step();

			if (ColorChange)
			{
				FullRedraw = true;
			}

			// Colormap animations are processed separately
			// from picture anims (see XFormColorMap_step)
			ProcessColormapAnims(Sequences + FirstAmbient,
								 CommData.NumAnimations);

			Change = DrawAlienFrame(Sequences, TotalSequences, FullRedraw);
			if (FullRedraw)
			{
				Change = true;
			}
		}

		UnbatchGraphics();

		// Post-process ambient animations
		pSeq = Sequences + FirstAmbient;
		for (i = 0; i < CommData.NumAnimations; ++i, ++pSeq)
		{
			AnimationDesc* ADPtr = pSeq->ADPtr;
			const uint32_t ActiveBit = 1L << i;

			if (ADPtr->AnimFlags & AnimDisabled)
			{
				continue;
			}

			// We can only disable a one-shot anim here, otherwise the
			// last frame will not be drawn
			if ((ADPtr->AnimFlags & OneShotAnim)
				&& !(NextActiveMask & ActiveBit))
			{ // One-shot animation, inactive next frame
				ADPtr->AnimFlags |= AnimDisabled;

				if (ADPtr->AnimFlags & RestartAllAfter)
				{
					SwitchSequences(true);
				}

				if (ADPtr->AnimFlags & StopAllAfter)
				{
					SwitchSequences(false);
					CommData.AlienTalkDesc.AnimFlags |= PauseTalking;
					TalkDesc.AnimFlags |= AnimDisabled;
					CommData.AlienFrame = SetAbsFrameIndex(CommData.AlienFrame, ADPtr->StartIndex);
				}
			}
		}

		return Change;
	}
}

static void ApplyFilterToStamp(GfxStamp s)
{
	for (uint16_t i {0}; i < FilterData.NumFilters; i++)
	{
		DrawMode mode;
		DrawMode oldMode;
		Color oldColor;
		Color FGColor;
		const AnimFilter* FTPtr = &FilterData.FilterArray[i];

		if (FTPtr->Flags & FilterDisabled)
		{
			continue;
		}

		// Get color from colormap
		FGColor = GetColorMapColor(CommColormapIndex,
								   FTPtr->ColorIndex);

		// Get DRAW_FACTOR from red channel of color from colormap
		// with set index
		const int16_t factor {getAlphaChannel(FTPtr->OpacityIndex)};

		// Image is transparent anyway
		if (factor == 0x00 && FTPtr->Kind == DrawKind::Alpha)
		{
			continue;
		}

		mode = MAKE_DRAW_MODE((DrawKind)FTPtr->Kind, factor);
		oldMode = SetContextDrawMode(mode);
		oldColor = SetContextForeGroundColor(FGColor);

		DrawFilledStamp(&s);

		SetContextDrawMode(oldMode);
		SetContextForeGroundColor(oldColor);
	}
}

bool DrawAlienFrame(AnimSequence* Sequences, uint16_t Num, bool fullRedraw)
{
	int i {};
	GfxStamp s;
	bool Change = false;

	BatchGraphics();

	s.origin.x = -SafeX();
	s.origin.y = 0;

	if (fullRedraw)
	{
		// Draw the main frame
		s.frame = CommData.AlienFrame;
		DrawStamp(&s);

		// Draw any static frames (has to be in reverse)
		for (i = CommData.NumAnimations - 1; i >= 0; --i)
		{
			AnimationDesc* ADPtr = &CommData.AlienAmbientArray[i];

			if (ADPtr->AnimFlags & AnimMask)
			{
				continue;
			}

			ADPtr->AnimFlags |= AnimDisabled;

			if (!(ADPtr->AnimFlags & ColorxformAnim))
			{ // It's a static frame (e.g. Flagship picture at Starbase)
				s.frame = SetAbsFrameIndex(CommData.AlienFrame,
										   ADPtr->StartIndex);
				DrawStamp(&s);
			}
		}
	}

	if (Sequences)
	{ // Draw the animation sequences (has to be in reverse)
		for (i = Num - 1; i >= 0; --i)
		{
			AnimSequence* pSeq = &Sequences[i];
			const AnimationDesc* ADPtr = pSeq->ADPtr;

			if ((ADPtr->AnimFlags & AnimDisabled)
				|| pSeq->AnimType != PICTURE_ANIM)
			{
				continue;
			}

			// Draw current animation frame only if changed
			if (!fullRedraw && !pSeq->Change)
			{
				continue;
			}

			// Take into account masks to avoid forbidden overdraws
			if (fullRedraw && ADPtr->BlockMask & ActiveMask)
			{
				continue;
			}

			s.frame = SetAbsFrameIndex(CommData.AlienFrame,
									   ADPtr->StartIndex + pSeq->CurIndex);

			DrawStamp(&s);
			if (!fullRedraw && filterEnabled)
			{
				ApplyFilterToStamp(s);
			}

			pSeq->Change = false;

			Change = true;
		}
	}
	if (filterEnabled && fullRedraw)
	{ // Kruzen: draw any filter if there are any and we need to
		// No need to set Change to true since it's already true on
		// fullRedraw
		for (i = 0; i < FilterData.NumFilters; i++)
		{
			DrawMode mode;
			DrawMode oldMode;
			Color oldColor;
			Color FGColor;
			int16_t factor {};
			const AnimFilter* FTPtr = &FilterData.FilterArray[i];

			if (FTPtr->Flags & FilterDisabled)
			{
				continue;
			}

			if (FTPtr->Flags & FramedFilter)
			{ // Special case - just draw plain frame
				factor = getAlphaChannel(FTPtr->ColorIndex);

				// Don't draw if index is outside or no frame is set
				if (factor > FTPtr->OpacityIndex
					|| FTPtr->FrameIndex == -1)
				{
					continue;
				}

				s.frame = SetAbsFrameIndex(CommData.AlienFrame,
										   FTPtr->FrameIndex + factor);

				DrawStamp(&s);
			}
			else
			{ // Get color from colormap
				FGColor = GetColorMapColor(CommColormapIndex,
										   FTPtr->ColorIndex);

				// Get DRAW_FACTOR from red channel of color from colormap
				// with set index
				factor = getAlphaChannel(FTPtr->OpacityIndex);

				// Image is transparent anyway
				if (factor == 0x00 && FTPtr->Kind == DrawKind::Alpha)
				{
					goto postprocess;
				}

				mode = MAKE_DRAW_MODE((DrawKind)FTPtr->Kind, factor);
				oldMode = SetContextDrawMode(mode);
				oldColor = SetContextForeGroundColor(FGColor);

				if (FTPtr->FrameIndex == -1)
				{ // Don't have frame - draw rect on top of everything
					GfxRect r;

					GetContextClipRect(&r);
					r.corner.x = r.corner.y = 0;

					DrawFilledRectangle(&r);
				}
				else
				{ // Draw filled stamp
					s.frame = SetAbsFrameIndex(CommData.AlienFrame,
											   FTPtr->FrameIndex);

					DrawFilledStamp(&s);
				}

				SetContextDrawMode(oldMode);
				SetContextForeGroundColor(oldColor);
			}

			// postprocess stuff
postprocess:
			if (FTPtr->Flags & TurnOffOft && factor == 0x00)
			{
				filterEnabled = false;
			}

			if (FTPtr->Flags & TurnOffOfo && factor == 0xFF)
			{
				filterEnabled = false;
			}

			if (FTPtr->Flags & SwitchOffAnims && factor == 0xFF)
			{
				SwitchSequences(false);
				EnableTalkingAnim(false);
			}
			if (FTPtr->Flags & SwitchOnAnims && factor != 0xFF)
			{
				SwitchSequences(true);
				EnableTalkingAnim(true);
			}
		}
	}

	UnbatchGraphics();

	return Change;
}

void ShutYourMouth(void)
{ // If talk animation is disabled set
	// talk frame to default index (closed mouth)
	if (Talk->CurIndex != 0)
	{
		Talk->CurIndex = 0;
	}
}

void SwitchSequences(bool enableAll)
{ // Kruzen: Needed for disabling animations during
	// HdResolutionFactor one-time transitions (i.e. orz frumple)

	for (uint16_t i {0}; i < CommData.NumAnimations; ++i)
	{
		// skip one-shot anim
		if (CommData.AlienAmbientArray[i].AnimFlags & OneShotAnim)
		{
			continue;
		}

		if (!enableAll)
		{
			if (CommData.AlienAmbientArray[i].AnimFlags & ImmumeToStop)
			{
				continue;
			}

			CommData.AlienAmbientArray[i].AnimFlags |= AnimDisabled;
		}
		else
		{
			if (CommData.AlienAmbientArray[i].AnimFlags
				& ImmumeToRestart)
			{
				continue;
			}

			CommData.AlienAmbientArray[i].AnimFlags &= ~AnimDisabled;
		}
	}
}

void RunOneTimeSequence(uint16_t animIndex, uint16_t flags)
{ // Kruzen: HdResolutionFactor-only
	if (!(CommData.AlienAmbientArray[animIndex].AnimFlags
		  & ColorxformAnim)
		&& CommData.AlienAmbientArray[animIndex].AnimFlags
			   & OneShotAnim)
	{
		CommData.AlienAmbientArray[animIndex].AnimFlags &= ~AnimDisabled;

		CommData.AlienAmbientArray[animIndex].AnimFlags |= flags;

		if (!(CommData.AlienAmbientArray[animIndex].AnimFlags
			  & AlphaMaskAnim))
		{
			SwitchSequences(false);
		}
	}
}

void EngageFilters(AnimFilterSet* f_desc)
{
	if (!f_desc)
	{
		return;
	}

	FilterData = *f_desc;
	filterEnabled = true;
}

void DisengageFilters(void)
{
	filterEnabled = false;
}

void DeltaLastTime(TimePeriod diff)
{
	LastTime += diff;
}