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

#include "process.h"

#include "races.h"
#include "collide.h"
#include "options.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "hyper.h"
#include "element.h"
#include "battle.h"
#include "weapon.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/gfx_common.h"
#include "core/log/log.h"
#include "core/misc/FatalExit.h"
#include "intel.h"


//#define DEBUG_PROCESS

uint16_t DisplayFreeList;
PRIMITIVE DisplayArray[MaxDisplayPrims];
extern DPOINT SpaceOrg;

uint16_t zoom_out = 1 << ZoomShift;
static int16_t opt_max_zoom_out;

#if 0
static inline void CALC_ZOOM_STUFF (uint16_t* idx, uint16_t* sc)
{
	int i, z;

	z = 1 << ZoomShift;
	for (i = 0; (z <<= 1) <= zoom_out; i++)
		;
	*idx = i;
	*sc = ((1 << i) << (ZoomShift + 8)) / zoom_out;
}
#else
static inline void CALC_ZOOM_STUFF(uint16_t* idx, uint16_t* sc)
{
	int i {};

	if (zoom_out < 2 << ZoomShift)
	{
		i = 0;
	}
	else if (zoom_out < 4 << ZoomShift)
	{
		i = 1;
	}
	else
	{
		i = 2;
	}
	*idx = i;
	*sc = (1 << (i + ZoomShift + 8)) / zoom_out;
}
#endif

HELEMENT AllocElement(void)
{
	HELEMENT hElement {AllocLink(&disp_q)};
	if (hElement)
	{
		ELEMENT* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		memset(ElementPtr, 0, sizeof(*ElementPtr));
		ElementPtr->PrimIndex = AllocDisplayPrim();
		if (ElementPtr->PrimIndex == EndOfList)
		{
			uqm::log::error("AllocElement: Out of display prims!");
			uqm::fatalExit();
		}
		SetPrimType(&DisplayArray[ElementPtr->PrimIndex], NoPrim);
		SetPrimFlags(&DisplayArray[ElementPtr->PrimIndex], 0);
		UnlockElement(hElement);
	}

	return (hElement);
}

void FreeElement(HELEMENT hElement)
{
	if (hElement)
	{
		ELEMENT* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		FreeDisplayPrim(ElementPtr->PrimIndex);
		UnlockElement(hElement);

		FreeLink(&disp_q, hElement);
	}
}

void SetUpElement(ELEMENT* ElementPtr)
{
	ElementPtr->next = ElementPtr->current;
	if (CollidingElement(ElementPtr))
	{
		InitIntersectStartPoint(ElementPtr);
		InitIntersectEndPoint(ElementPtr);
		InitIntersectFrame(ElementPtr);
	}
}

static void PreProcess(ELEMENT* ElementPtr)
{
	ELEMENT_FLAGS state_flags {};

	if (ElementPtr->life_span == 0)
	{
		if (ElementPtr->pParent) /* untarget this dead element */
		{
			Untarget(ElementPtr);
		}

		ElementPtr->state_flags |= Disappearing;
		if (ElementPtr->death_func)
		{
			(*ElementPtr->death_func)(ElementPtr);
		}
	}

	state_flags = ElementPtr->state_flags;
	if (!(state_flags & Disappearing))
	{
		if (state_flags & Appearing)
		{
			SetUpElement(ElementPtr);

			if (state_flags & PlayerShip)
			{
				state_flags &= ~Appearing; /* want to preprocess ship */
			}
		}

		if (ElementPtr->preprocess_func && !(state_flags & Appearing))
		{
			(*ElementPtr->preprocess_func)(ElementPtr);

			state_flags = ElementPtr->state_flags;
			if ((state_flags & Changing) && CollidingElement(ElementPtr))
			{
				InitIntersectFrame(ElementPtr);
			}
		}

		if (!(state_flags & IgnoreVelocity))
		{
			int16_t delta_x {};
			int16_t delta_y {};

			GetNextVelocityComponents(&ElementPtr->velocity,
									  &delta_x, &delta_y, 1);
			if (delta_x != 0 || delta_y != 0)
			{
				state_flags |= Changing;
				ElementPtr->next.location.x += delta_x;
				ElementPtr->next.location.y += delta_y;
			}
		}

		if (CollidingElement(ElementPtr))
		{
			InitIntersectEndPoint(ElementPtr);
		}

		if (state_flags & FiniteLife)
		{
			--ElementPtr->life_span;
		}
	}

	ElementPtr->state_flags = (state_flags & ~(PostProcessFlag | Collision))
							| PreProcessFlag;
}

static void PostProcess(ELEMENT* ElementPtr)
{
	if (ElementPtr->postprocess_func)
	{
		(*ElementPtr->postprocess_func)(ElementPtr);
	}
	ElementPtr->current = ElementPtr->next;

	if (CollidingElement(ElementPtr))
	{
		InitIntersectStartPoint(ElementPtr);
		InitIntersectEndPoint(ElementPtr);
	}

	ElementPtr->state_flags = (ElementPtr->state_flags
							   & ~(PreProcessFlag | Changing | Appearing))
							| PostProcessFlag;
}

static uint16_t CalcReduction(int32_t dx, int32_t dy)
{
	uint16_t next_reduction {};

#ifdef KDEBUG
	uqm::log::debug("CalcReduction:");
#endif

	if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
	{
		if (uqm::lowByte(uqm::currentActivity()) > IN_ENCOUNTER)
		{
			return (0);
		}

		const int32_t sdx {dx};
		const int32_t sdy {dy};
		for (next_reduction = MaxVisReduction;
			 (dx <<= ReductionShift) <= TransitionWidth()
			 && (dy <<= ReductionShift) <= TransitionHeight()
			 && next_reduction > 0;
			 next_reduction -= ReductionShift)
			;

		/* check for "real" zoom in */
		if (next_reduction < zoom_out
			&& zoom_out <= MaxVisReduction)
		{
			const auto HysteresisX = [&] {
				return DisplayToWorld(ResScale(24));
			};
			const auto HysteresisY = [&] {
				return DisplayToWorld(ResScale(20));
			};
			if (((sdx + HysteresisX())
				 << (MaxVisReduction - next_reduction))
					> TransitionWidth()
				|| ((sdy + HysteresisY())
					<< (MaxVisReduction - next_reduction))
					   > TransitionHeight())
			{
				/* if we don't zoom in, we want to stay at next+1 */
				next_reduction += ReductionShift;
			}
		}

		if (next_reduction == 0
			&& uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE)
		{
			next_reduction += ReductionShift;
		}
	}
	else
	{
		if (uqm::lowByte(uqm::currentActivity()) > IN_ENCOUNTER)
		{
			return (1 << ZoomShift);
		}

		dx = (dx * MaxZoomOut) / (LogSpaceWidth() >> 2);
		if (dx < (1 << ZoomShift))
		{
			dx = 1 << ZoomShift;
		}
		else if (dx > MaxZoomOut)
		{
			dx = MaxZoomOut;
		}

		dy = (dy * MaxZoomOut) / (LogSpaceHeight() >> 2);
		if (dy < (1 << ZoomShift))
		{
			dy = 1 << ZoomShift;
		}
		else if (dy > MaxZoomOut)
		{
			dy = MaxZoomOut;
		}

		if (dy > dx)
		{
			next_reduction = dy;
		}
		else
		{
			next_reduction = dx;
		}

		if (next_reduction < (2 << ZoomShift)
			&& uqm::lowByte(uqm::currentActivity()) == IN_LAST_BATTLE)
		{
			next_reduction = (2 << ZoomShift);
		}
	}

#ifdef KDEBUG
	uqm::log::debug("CalcReduction: exit");
#endif

	return (next_reduction);
}

static constexpr auto ZoomJump {((1 << ZoomShift) >> 3)};

static VIEW_STATE CalcView(DPOINT* pNewScrollPt, int16_t next_reduction,
						   int32_t* pdx, int32_t* pdy, uint16_t ships_alive)
{
	VIEW_STATE view_state;

#ifdef KDEBUG
	uqm::log::debug("CalcView:");
#endif
	int32_t dx {((int32_t)(LogSpaceWidth() >> 1) - pNewScrollPt->x)};
	int32_t dy {((int32_t)(LogSpaceHeight() >> 1) - pNewScrollPt->y)};
	dx = WrapDeltaX(dx);
	dy = WrapDeltaY(dy);
	const bool meleeScaleModeIsTFBStep {uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step};
	if (ships_alive == 1 && (!meleeScaleModeIsTFBStep || isNetwork()))
	{
		const auto OrgJumpX = [&] {
			return ((int32_t)DisplayAlign(LogSpaceWidth() / 75));
		};
		const auto OrgJumpY = [&] {
			return ((int32_t)DisplayAlign(LogSpaceHeight() / 75));
		};
		if (dx > OrgJumpX())
		{
			dx = OrgJumpX();
		}
		else if (dx < -OrgJumpX())
		{
			dx = -OrgJumpX();
		}
		if (dy > OrgJumpY())
		{
			dy = OrgJumpY();
		}
		else if (dy < -OrgJumpY())
		{
			dy = -OrgJumpY();
		}
	}

	if ((dx || dy) && inHQSpace())
	{
		MoveSIS(&dx, &dy);
	}

	if (zoom_out == next_reduction)
	{
		view_state = dx == 0 && dy == 0 && !inHQSpace() ? VIEW_STABLE : VIEW_SCROLL;
	}
	else
	{
		if (meleeScaleModeIsTFBStep)
		{
			SpaceOrg.x = (int32_t)(LogSpaceWidth() >> 1)
					   - (LogSpaceWidth() >> ((MaxReduction + 1)
											  - next_reduction));
			SpaceOrg.y = (int32_t)(LogSpaceHeight() >> 1)
					   - (LogSpaceHeight() >> ((MaxReduction + 1)
											   - next_reduction));
		}
		else
		{
			if (ships_alive == 1
				&& zoom_out > next_reduction
				&& zoom_out <= MaxZoomOut
				&& zoom_out - next_reduction > ZoomJump)
			{
				next_reduction = zoom_out - ZoomJump;
			}

			// Always align the origin on a whole pixel to reduce the
			// amount of object positioning jitter
			SpaceOrg.x = DisplayAlign((int)(LogSpaceWidth() >> 1) - (LogSpaceWidth() * next_reduction / (MaxZoomOut << 2)));
			SpaceOrg.y = DisplayAlign((int)(LogSpaceHeight() >> 1) - (LogSpaceHeight() * next_reduction / (MaxZoomOut << 2)));
		}
		zoom_out = next_reduction;
		view_state = VIEW_CHANGE;
	}

	if (uqm::lowByte(uqm::currentActivity()) <= IN_HYPERSPACE)
	{
		MoveGalaxy(view_state, dx, dy);
	}

	*pdx = dx;
	*pdy = dy;

#ifdef KDEBUG
	uqm::log::debug("CalcView: exit");
#endif
	return (view_state);
}


static ELEMENT_FLAGS ProcessCollisions(HELEMENT hSuccElement, ELEMENT* ElementPtr,
									   TIME_VALUE min_time, ELEMENT_FLAGS process_flags)
{
	HELEMENT hTestElement {};

	while ((hTestElement = hSuccElement) != 0)
	{
		ELEMENT* TestElementPtr {};

		LockElement(hTestElement, &TestElementPtr);
		if (!(TestElementPtr->state_flags & process_flags))
		{
			PreProcess(TestElementPtr);
		}
		hSuccElement = GetSuccElement(TestElementPtr);

		if (TestElementPtr == ElementPtr)
		{
			UnlockElement(hTestElement);
			continue;
		}

		if (CollisionPossible(TestElementPtr, ElementPtr))
		{
			ELEMENT_FLAGS state_flags {};
			TIME_VALUE time_val {};

			state_flags = ElementPtr->state_flags;
			ELEMENT_FLAGS test_state_flags {TestElementPtr->state_flags};
			if (((state_flags | test_state_flags) & FiniteLife)
				&& (((state_flags & Appearing)
					 && ElementPtr->life_span > 1)
					|| ((test_state_flags & Appearing)
						&& TestElementPtr->life_span > 1)))
			{
				time_val = 0;
			}
			else
			{
				while ((time_val = DrawablesIntersect(&ElementPtr->IntersectControl,
													  &TestElementPtr->IntersectControl, min_time))
						   == 1
					   && !((state_flags | test_state_flags) & FiniteLife))
				{
#ifdef DEBUG_PROCESS
					uqm::log::debug("BAD NEWS {:#x} <--> {:#x}", ElementPtr,
									TestElementPtr);
#endif /* DEBUG_PROCESS */
					if (state_flags & Collision)
					{
						InitIntersectEndPoint(TestElementPtr);
						TestElementPtr->IntersectControl.IntersectStamp.origin =
							TestElementPtr->IntersectControl.EndPoint;
						time_val = DrawablesIntersect(&ElementPtr->IntersectControl,
													  &TestElementPtr->IntersectControl, 1);
						InitIntersectStartPoint(TestElementPtr);
					}

					if (time_val == 1)
					{
						FRAME CurFrame {ElementPtr->current.image.frame};
						FRAME NextFrame {ElementPtr->next.image.frame};
						FRAME TestCurFrame {TestElementPtr->current.image.frame};
						FRAME TestNextFrame {TestElementPtr->next.image.frame};
						if (NextFrame == CurFrame
							&& TestNextFrame == TestCurFrame)
						{
							if (test_state_flags & Appearing)
							{
								do_damage(TestElementPtr, TestElementPtr->hit_points);
								if (TestElementPtr->pParent) /* untarget this dead element */
								{
									Untarget(TestElementPtr);
								}

								TestElementPtr->state_flags |= (Collision | Disappearing);
								if (TestElementPtr->death_func)
								{
									(*TestElementPtr->death_func)(TestElementPtr);
								}
							}
							if (state_flags & Appearing)
							{
								do_damage(ElementPtr, ElementPtr->hit_points);
								if (ElementPtr->pParent) /* untarget this dead element */
								{
									Untarget(ElementPtr);
								}

								ElementPtr->state_flags |= (Collision | Disappearing);
								if (ElementPtr->death_func)
								{
									(*ElementPtr->death_func)(ElementPtr);
								}

								UnlockElement(hTestElement);
								return (Collision);
							}

							time_val = 0;
						}
						else
						{
							if (GetFrameIndex(CurFrame) != GetFrameIndex(NextFrame))
							{
								ElementPtr->next.image.frame =
									SetEquFrameIndex(NextFrame,
													 CurFrame);
							}
							else if (NextFrame != CurFrame)
							{
								ElementPtr->next.image =
									ElementPtr->current.image;
								if (ElementPtr->life_span > NormalLife)
								{
									ElementPtr->life_span = NormalLife;
								}
							}

							if (GetFrameIndex(TestCurFrame) != GetFrameIndex(TestNextFrame))
							{
								TestElementPtr->next.image.frame =
									SetEquFrameIndex(TestNextFrame,
													 TestCurFrame);
							}
							else if (TestNextFrame != TestCurFrame)
							{
								TestElementPtr->next.image =
									TestElementPtr->current.image;
								if (TestElementPtr->life_span > NormalLife)
								{
									TestElementPtr->life_span = NormalLife;
								}
							}

							InitIntersectStartPoint(ElementPtr);
							InitIntersectEndPoint(ElementPtr);
							InitIntersectFrame(ElementPtr);
							if (state_flags & PlayerShip)
							{
								STARSHIP* StarShipPtr {};

								GetElementStarShip(ElementPtr, &StarShipPtr);
								StarShipPtr->ShipFacing =
									GetFrameIndex(
										ElementPtr->next.image.frame);
							}

							InitIntersectStartPoint(TestElementPtr);
							InitIntersectEndPoint(TestElementPtr);
							InitIntersectFrame(TestElementPtr);
							if (test_state_flags & PlayerShip)
							{
								STARSHIP* StarShipPtr {};

								GetElementStarShip(TestElementPtr, &StarShipPtr);
								StarShipPtr->ShipFacing =
									GetFrameIndex(
										TestElementPtr->next.image.frame);
							}
						}
					}

					if (time_val == 0)
					{
						InitIntersectEndPoint(ElementPtr);
						InitIntersectEndPoint(TestElementPtr);

						break;
					}
				}
			}

			if (time_val > 0)
			{
				GFXPOINT SavePt, TestSavePt;

#ifdef DEBUG_PROCESS
				uqm::log::debug("{:#x} <--> {:#x} at {}", ElementPtr,
								TestElementPtr, time_val);
#endif /* DEBUG_PROCESS */
				SavePt = ElementPtr->IntersectControl.EndPoint;
				TestSavePt = TestElementPtr->IntersectControl.EndPoint;
				InitIntersectEndPoint(ElementPtr);
				InitIntersectEndPoint(TestElementPtr);
				if (time_val == 1
					|| (((state_flags & Collision)
						 || !ProcessCollisions(hSuccElement, ElementPtr,
											   time_val - 1, process_flags))
						&& ((test_state_flags & Collision)
							|| !ProcessCollisions(
								!(TestElementPtr->state_flags & Appearing) ?
									GetSuccElement(ElementPtr) :
									GetHeadElement(),
								TestElementPtr,
								time_val - 1, process_flags))))
				{
					state_flags = ElementPtr->state_flags;
					test_state_flags = TestElementPtr->state_flags;

#ifdef DEBUG_PROCESS
					uqm::log::debug("PROCESSING {:#x} <--> {:#x} at {}",
									ElementPtr, TestElementPtr, time_val);
#endif /* DEBUG_PROCESS */
					if (test_state_flags & PlayerShip)
					{
						(*TestElementPtr->collision_func)(
							TestElementPtr, &TestSavePt,
							ElementPtr, &SavePt);
						(*ElementPtr->collision_func)(
							ElementPtr, &SavePt,
							TestElementPtr, &TestSavePt);
					}
					else
					{
						(*ElementPtr->collision_func)(
							ElementPtr, &SavePt,
							TestElementPtr, &TestSavePt);
						(*TestElementPtr->collision_func)(
							TestElementPtr, &TestSavePt,
							ElementPtr, &SavePt);
					}

					if (TestElementPtr->state_flags & Collision)
					{
						if (!(test_state_flags & Collision))
						{
							TestElementPtr->IntersectControl.IntersectStamp.origin =
								TestSavePt;
							TestElementPtr->next.location.x =
								DisplayToWorld(TestSavePt.x);
							TestElementPtr->next.location.y =
								DisplayToWorld(TestSavePt.y);
							InitIntersectEndPoint(TestElementPtr);
						}
					}

					if (ElementPtr->state_flags & Collision)
					{
						if (!(state_flags & Collision))
						{
							ElementPtr->IntersectControl.IntersectStamp.origin =
								SavePt;
							ElementPtr->next.location.x =
								DisplayToWorld(SavePt.x);
							ElementPtr->next.location.y =
								DisplayToWorld(SavePt.y);
							InitIntersectEndPoint(ElementPtr);

							if (!(state_flags & FiniteLife) && !(test_state_flags & FiniteLife))
							{
								collide(ElementPtr, TestElementPtr);

								ProcessCollisions(GetHeadElement(), ElementPtr,
												  MaxTimeValue, process_flags);
								ProcessCollisions(GetHeadElement(), TestElementPtr,
												  MaxTimeValue, process_flags);
							}
						}
						UnlockElement(hTestElement);
						return (Collision);
					}

					if (!CollidingElement(ElementPtr))
					{
						ElementPtr->state_flags |= Collision;
						UnlockElement(hTestElement);
						return (Collision);
					}
				}
			}
		}

		UnlockElement(hTestElement);
	}

	return (ElementPtr->state_flags & Collision);
}

static VIEW_STATE PreProcessQueue(int32_t* pscroll_x, int32_t* pscroll_y)
{
	int16_t min_reduction {};
	int16_t max_reduction {};
	uint16_t sides_active {};
	DPOINT Origin;

#ifdef KDEBUG
	uqm::log::debug("PreProcess:");
#endif
	sides_active = (battle_counter[0] ? 1 : 0)
				 + (battle_counter[1] ? 1 : 0);

	if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
	{
		min_reduction = max_reduction = MaxVisReduction + 1;
	}
	else
	{
		min_reduction = max_reduction = MaxZoomOut + (1 << ZoomShift);
	}

	Origin.x = (int32_t)(LogSpaceWidth() >> 1);
	Origin.y = (int32_t)(LogSpaceHeight() >> 1);

	HELEMENT hElement {GetHeadElement()};
	uint16_t ships_alive {0};
	while (hElement != 0)
	{
		ELEMENT* ElementPtr {};

		LockElement(hElement, &ElementPtr);

		if (!(ElementPtr->state_flags & PreProcessFlag))
		{
			PreProcess(ElementPtr);
		}
		HELEMENT hNextElement {GetSuccElement(ElementPtr)};

		if (CollidingElement(ElementPtr)
			&& !(ElementPtr->state_flags & Collision))
		{
			ProcessCollisions(hNextElement, ElementPtr,
							  MaxTimeValue, PreProcessFlag);
		}

		if (ElementPtr->state_flags & PlayerShip)
		{
			ships_alive++;
			if (max_reduction > opt_max_zoom_out
				&& min_reduction > opt_max_zoom_out)
			{
				Origin.x = DisplayAlign(ElementPtr->next.location.x);
				Origin.y = DisplayAlign(ElementPtr->next.location.y);
			}

			int32_t dx {DisplayAlign(ElementPtr->next.location.x) - Origin.x};
			dx = WrapDeltaX(dx);
			int32_t dy {DisplayAlign(ElementPtr->next.location.y) - Origin.y};
			dy = WrapDeltaY(dy);

			if (sides_active <= 2 || ElementPtr->playerNr == 0)
			{
				Origin.x = DisplayAlign(Origin.x + (dx >> 1));
				Origin.y = DisplayAlign(Origin.y + (dy >> 1));

				if (dx < 0)
				{
					dx = -dx;
				}
				if (dy < 0)
				{
					dy = -dy;
				}
				max_reduction = CalcReduction(dx, dy);
			}
			else if (max_reduction > opt_max_zoom_out
					 && min_reduction <= opt_max_zoom_out)
			{
				Origin.x = DisplayAlign(Origin.x + (dx >> 1));
				Origin.y = DisplayAlign(Origin.y + (dy >> 1));

				if (dx < 0)
				{
					dx = -dx;
				}
				if (dy < 0)
				{
					dy = -dy;
				}
				min_reduction = CalcReduction(dx, dy);
			}
			else
			{
				int16_t reduction {};

				if (dx < 0)
				{
					dx = -dx;
				}
				if (dy < 0)
				{
					dy = -dy;
				}
				reduction = CalcReduction(dx << 1, dy << 1);

				if (min_reduction > opt_max_zoom_out
					|| reduction < min_reduction)
				{
					min_reduction = reduction;
				}
			}
			//			log_add (log_Debug, "dx = {} dy = {} min_red = {} max_red = {}",
			//					dx, dy, min_reduction, max_reduction);
		}

		UnlockElement(hElement);
		hElement = hNextElement;
	}

	if ((min_reduction > opt_max_zoom_out || min_reduction <= max_reduction)
		&& (min_reduction = max_reduction) > opt_max_zoom_out
		&& (min_reduction = zoom_out) > opt_max_zoom_out)
	{
		if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
		{
			min_reduction = 0;
		}
		else
		{
			min_reduction = 1 << ZoomShift;
		}
	}

#ifdef KDEBUG
	uqm::log::debug("PreProcess: exit");
#endif
	return (CalcView(&Origin, min_reduction, pscroll_x, pscroll_y, ships_alive));
}

void InsertPrim(PRIM_LINKS* pLinks, uint16_t primIndex, uint16_t iPI)
{
	uint16_t Link {};
	PRIM_LINKS PL {};

	if (iPI == EndOfList)
	{
		Link = GetSuccLink(*pLinks); /* get tail */
		if (Link == EndOfList)
		{
			*pLinks = uqm::makeDword(primIndex, primIndex);
		}
		else
		{
			*pLinks = uqm::makeDword(GetPredLink(*pLinks), primIndex);
		}
	}
	else
	{
		PL = GetPrimLinks(&DisplayArray[iPI]);
		if (iPI != GetPredLink(*pLinks)) /* if not the head */
		{
			Link = GetPredLink(PL);
		}
		else
		{
			Link = EndOfList;
			*pLinks = uqm::makeDword(primIndex, GetSuccLink(*pLinks));
		}
		SetPrimLinks(&DisplayArray[iPI], primIndex, GetSuccLink(PL));
	}

	if (Link != EndOfList)
	{
		PL = GetPrimLinks(&DisplayArray[Link]);
		SetPrimLinks(&DisplayArray[Link], GetPredLink(PL), primIndex);
	}
	SetPrimLinks(&DisplayArray[primIndex], Link, iPI);
}

PRIM_LINKS DisplayLinks;

static inline int16_t CalcDisplayCoord(int32_t c, int32_t orgc, int16_t reduction)
{
	if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
	{ /* old fixed-step zoom style */
		return (c - orgc) >> reduction;
	}
	else
	{ /* new continuous zoom style */
		return ((c - orgc) << ZoomShift) / reduction;
	}
}

static void PostProcessQueue(VIEW_STATE view_state, int32_t scroll_x, int32_t scroll_y)
{
	DPOINT delta;
	int16_t reduction {};

#ifdef KDEBUG
	uqm::log::debug("PostProcess:");
#endif
	if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
	{
		reduction = zoom_out + OneShift;
	}
	else
	{
		reduction = zoom_out << OneShift;
	}

	HELEMENT hElement {GetHeadElement()};
	while (hElement != 0)
	{
		ELEMENT_FLAGS state_flags {};
		ELEMENT* ElementPtr {};
		HELEMENT hNextElement {};

		LockElement(hElement, &ElementPtr);

		state_flags = ElementPtr->state_flags;
		if (state_flags & PreProcessFlag)
		{
			if (!(state_flags & Collision))
			{
				ElementPtr->state_flags &= ~DefyPhysics;
			}
			else
			{
				ElementPtr->state_flags &= ~Collision;
			}

			if (state_flags & PostProcessFlag)
			{
				delta.x = 0;
				delta.y = 0;
			}
			else
			{
				delta.x = scroll_x;
				delta.y = scroll_y;
			}
		}
		else
		{
			HELEMENT hPostElement {hElement};
			do
			{
				ELEMENT* PostElementPtr {};

				LockElement(hPostElement, &PostElementPtr);
				if (!(PostElementPtr->state_flags & PreProcessFlag))
				{
					PreProcess(PostElementPtr);
				}
				hNextElement = GetSuccElement(PostElementPtr);

				if (CollidingElement(PostElementPtr)
					&& !(PostElementPtr->state_flags & Collision))
				{
					ProcessCollisions(GetHeadElement(), PostElementPtr,
									  MaxTimeValue, PreProcessFlag | PostProcessFlag);
				}
				UnlockElement(hPostElement);
				hPostElement = hNextElement;
			} while (hPostElement != 0);

			scroll_x = 0;
			scroll_y = 0;
			delta.x = 0;
			delta.y = 0;
			/* because these are newly added elements that are
					 * already in adjusted coordinates */
			state_flags = ElementPtr->state_flags;
		}

		if (state_flags & Disappearing)
		{
			hNextElement = GetSuccElement(ElementPtr);
			UnlockElement(hElement);
			RemoveElement(hElement);
			FreeElement(hElement);
		}
		else
		{
			const GRAPHICS_PRIM ObjType {GetPrimType(&DisplayArray[ElementPtr->PrimIndex])};
			if (view_state != VIEW_STABLE
				|| (state_flags & (Appearing | Changing)))
			{
				GFXPOINT next;

				if (ObjType == LINE_PRIM)
				{
					const int32_t dx {(int32_t)ElementPtr->next.location.x - (int32_t)ElementPtr->current.location.x};
					const int32_t dy {(int32_t)ElementPtr->next.location.y - (int32_t)ElementPtr->current.location.y};

					next.x = WrapX((int32_t)ElementPtr->current.location.x + (int32_t)delta.x);
					next.y = WrapY((int32_t)ElementPtr->current.location.y + (int32_t)delta.y);
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.x = CalcDisplayCoord(next.x, SpaceOrg.x, reduction);
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.y = CalcDisplayCoord(next.y, SpaceOrg.y, reduction);

					next.x += dx;
					next.y += dy;
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.x = CalcDisplayCoord(next.x, SpaceOrg.x, reduction);
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.y = CalcDisplayCoord(next.y, SpaceOrg.y, reduction);
				}
				else
				{
					next.x = WrapX((int32_t)ElementPtr->next.location.x + (int32_t)delta.x);
					next.y = WrapY((int32_t)ElementPtr->next.location.y + (int32_t)delta.y);

					DisplayArray[ElementPtr->PrimIndex].Object.Point.x = CalcDisplayCoord(next.x, SpaceOrg.x, reduction);
					DisplayArray[ElementPtr->PrimIndex].Object.Point.y = CalcDisplayCoord(next.y, SpaceOrg.y, reduction);

					if (ObjType == STAMP_PRIM || ObjType == STAMPFILL_PRIM)
					{
						if (view_state == VIEW_CHANGE
							|| (state_flags & (Appearing | Changing)))
						{
							uint16_t index {};
							uint16_t scale = GscaleIdentity;

							if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
							{
								index = zoom_out;
							}
							else
							{
								CALC_ZOOM_STUFF(&index, &scale);
							}

							ElementPtr->next.image.frame = SetEquFrameIndex(
								ElementPtr->next.image.farray[index],
								ElementPtr->next.image.frame);

							if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Trilinear && index < 2 && scale != GscaleIdentity)
							{
								// enqueues drawcommand to assign next
								// (smaller) zoom level image as mipmap,
								// needed for trilinear scaling

								FRAME frame = ElementPtr->next.image.frame;
								FRAME mmframe = SetEquFrameIndex(
									ElementPtr->next.image.farray[index + 1], frame);

								// TODO: This is currently hacky, this code
								//   really should not dereference FRAME.
								//   Perhaps make mipmap part of STAMP prim?
								if (frame && mmframe)
								{
									const HOT_SPOT mmhs = GetFrameHot(mmframe);
									TFB_DrawScreen_SetMipmap(frame->image,
															 mmframe->image, mmhs.x, mmhs.y);
								}
							}
						}
						DisplayArray[ElementPtr->PrimIndex].Object.Stamp.frame =
							ElementPtr->next.image.frame;
					}
				}

				ElementPtr->next.location = next;
			}

			PostProcess(ElementPtr);

			if (ObjType < NUM_PRIMS)
			{
				InsertPrim(&DisplayLinks, ElementPtr->PrimIndex, EndOfList);
			}

			hNextElement = GetSuccElement(ElementPtr);
			UnlockElement(hElement);
		}

		hElement = hNextElement;
	}
#ifdef KDEBUG
	uqm::log::debug("PostProcess: exit");
#endif
}

void InitDisplayList(void)
{
	uint16_t i {};

	if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
	{
		zoom_out = MaxVisReduction + 1;
		opt_max_zoom_out = MaxVisReduction;
	}
	else
	{
		zoom_out = MaxZoomOut + (1 << ZoomShift);
		opt_max_zoom_out = MaxZoomOut;
	}

	ReinitQueue(&disp_q);

	for (i = 0; i < MaxDisplayPrims; ++i)
	{
		SetPrimLinks(&DisplayArray[i], EndOfList, i + 1);
	}
	SetPrimLinks(&DisplayArray[i - 1], EndOfList, EndOfList);
	DisplayFreeList = 0;
	DisplayLinks = uqm::makeDword(EndOfList, EndOfList);
}

uint16_t nth_frame = 0;

void RedrawQueue(bool clear)
{
	int32_t scroll_x {};
	int32_t scroll_y {};
	VIEW_STATE view_state;

	SetContext(StatusContext);

	view_state = PreProcessQueue(&scroll_x, &scroll_y);
	PostProcessQueue(view_state, scroll_x, scroll_y);

	if (uqm::UQMOptions::read().stereoSFX)
	{
		UpdateSoundPositions();
	}

	SetContext(SpaceContext);
	if (uqm::lowByte(uqm::currentActivity()) == SUPER_MELEE
		|| !(uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD)))
	{
		const uint8_t skip_frames {uqm::highByte(nth_frame)};
		if (skip_frames != (uint8_t)~0
			&& (skip_frames == 0 || (--nth_frame & 0x00FF) == 0))
		{
			nth_frame += skip_frames;
			if (clear)
			{
				ClearDrawable(); // this is for BatchBuildPage effect, but not scaled by SetGraphicScale
			}

			if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) != uqm::TFBScaleMode::Step)
			{
				uint16_t index {};
				uint16_t scale {};

				CALC_ZOOM_STUFF(&index, &scale);
				SetGraphicScale(scale);
			}

			DrawBatch(DisplayArray, DisplayLinks, 0); //BatchBuildPage);
			SetGraphicScale(0);
		}

		FlushSounds();
	}
	else
	{ // sfx queue needs to be flushed when aborting
		SuppressSounds();
		FlushSounds();
	}

	DisplayLinks = uqm::makeDword(EndOfList, EndOfList);
}

// Set the hTarget field to 0 for all elements in the display list that
// have hTarget set to ElementPtr.
void Untarget(ELEMENT* ElementPtr)
{
	HELEMENT hNextElement {};

	for (HELEMENT hElement {GetHeadElement()}; hElement; hElement = hNextElement)
	{
		HELEMENT hTarget {};
		ELEMENT* ListPtr {};

		LockElement(hElement, &ListPtr);
		hNextElement = GetSuccElement(ListPtr);

		hTarget = ListPtr->hTarget;
		if (hTarget)
		{
			ELEMENT* TargetElementPtr {};

			LockElement(hTarget, &TargetElementPtr);
			if (TargetElementPtr == ElementPtr)
			{
				ListPtr->hTarget = 0;
			}
			UnlockElement(hTarget);
		}

		UnlockElement(hElement);
	}
}

void RemoveElement(HLINK hLink)
{
	if (uqm::UQMOptions::read().stereoSFX)
	{
		ELEMENT* ElementPtr {};

		LockElement(hLink, &ElementPtr);
		if (ElementPtr != nullptr)
		{
			RemoveSoundsForObject(ElementPtr);
		}
		UnlockElement(hLink);
	}
	RemoveQueue(&disp_q, hLink);
}
