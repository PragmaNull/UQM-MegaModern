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

/* background starfield - used to generate agalaxy.asm */

#include "element.h"
#include "globdata.h"
#include "init.h"
#include "process.h"
#include "units.h"
#include "options.h"
#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
#include "igfxres.h"
#include "nameref.h"
#include "uqm/random/UQMRandom.h"

extern uint16_t zoom_out;
extern PRIM_LINKS DisplayLinks;


static constexpr auto BigStarCount {30};
static constexpr auto MedStarCount {60};
static constexpr auto SmlStarCount {90};
static constexpr auto NumStars {(BigStarCount + MedStarCount + SmlStarCount)};

LargePoint SpaceOrg;
static LargePoint log_star_array[NumStars];
static uint16_t num_sceneries;

static constexpr auto NumStarPlanes {3};

struct StarPlaneBlock
{
	uint16_t min_star_index;
	uint16_t num_stars;
	LargePoint* star_array;
	LargePoint* pmin_star;
	LargePoint* plast_star;
};

StarPlaneBlock StarBlock[NumStarPlanes] =
	{
		{
			0,
			BigStarCount,
			&log_star_array[0],
			nullptr,
			nullptr,
		 },
		{
			0,
			MedStarCount,
			&log_star_array[BigStarCount],
			nullptr,
			nullptr,
		 },
		{
			0,
			SmlStarCount,
			&log_star_array[BigStarCount + MedStarCount],
			nullptr,
			nullptr,
		 },
};

static void SortStarBlock(StarPlaneBlock* pStarBlock)
{
	for (uint16_t i {0}; i < pStarBlock->num_stars; ++i)
	{
		uint16_t j {};

		for (j = pStarBlock->num_stars - 1; j > i; --j)
		{
			if (pStarBlock->star_array[i].y > pStarBlock->star_array[j].y)
			{
				LargePoint temp;

				temp = pStarBlock->star_array[i];
				pStarBlock->star_array[i] = pStarBlock->star_array[j];
				pStarBlock->star_array[j] = temp;
			}
		}
	}

	pStarBlock->min_star_index = 0;
	pStarBlock->pmin_star = &pStarBlock->star_array[0];
	pStarBlock->plast_star =
		&pStarBlock->star_array[pStarBlock->num_stars - 1];
}

static void WrapStarBlock(int16_t plane, int32_t dx, int32_t dy)
{
	int32_t offs_y {};
	uint16_t num_stars {};

	StarPlaneBlock* pStarBlock {&StarBlock[plane]};

	uint16_t i {pStarBlock->min_star_index};
	LargePoint* ppt {pStarBlock->pmin_star};
	num_stars = pStarBlock->num_stars;
	if (dy < 0)
	{
		const uint16_t first {i};

		dy = -dy;
		offs_y = (LogSpaceHeight() << plane) - dy;

		while (ppt->y < dy)
		{
			ppt->y += offs_y;
			ppt->x += dx;
			if (++i < num_stars)
			{
				++ppt;
			}
			else
			{
				i = 0;
				ppt = &pStarBlock->star_array[0];
			}

			if (i == first)
			{
				return;
			}
		}
		pStarBlock->min_star_index = i;
		pStarBlock->pmin_star = ppt;

		if (first <= i)
		{
			i = num_stars - i;
			do
			{
				ppt->y -= dy;
				ppt->x += dx;
				++ppt;
			} while (--i);
			ppt = &pStarBlock->star_array[0];
		}

		if (first > i)
		{
			i = first - i;
			do
			{
				ppt->y -= dy;
				ppt->x += dx;
				++ppt;
			} while (--i);
		}
	}
	else
	{
		--ppt;
		if (i-- == 0)
		{
			i = num_stars - 1;
			ppt = pStarBlock->plast_star;
		}

		const uint16_t last {i};

		if (dy > 0)
		{
			offs_y = (LogSpaceHeight() << plane) - dy;

			while (ppt->y >= offs_y)
			{
				ppt->y -= offs_y;
				ppt->x += dx;
				if (i-- > 0)
				{
					--ppt;
				}
				else
				{
					i = num_stars - 1;
					ppt = pStarBlock->plast_star;
				}

				if (i == last)
				{
					return;
				}
			}

			pStarBlock->pmin_star = ppt + 1;
			if ((pStarBlock->min_star_index = i + 1) == num_stars)
			{
				pStarBlock->min_star_index = 0;
				pStarBlock->pmin_star = &pStarBlock->star_array[0];
			}
		}

		if (last >= i)
		{
			++i;
			do
			{
				ppt->y += dy;
				ppt->x += dx;
				--ppt;
			} while (--i);
			i = num_stars - 1;
			ppt = pStarBlock->plast_star;
		}

		if (last < i)
		{
			i = i - last;
			do
			{
				ppt->y += dy;
				ppt->x += dx;
				--ppt;
			} while (--i);
		}
	}
}

void InitGalaxy(void)
{
	uint16_t i {};
	LargePoint* ppt {};
	const bool useScenery = (IsExtended() && uqm::gameState().flag(uqm::GameFlag::URQUAN_PROTECTING_SAMATRA)
							 && uqm::activityKind(uqm::currentActivity()) != ActivityKind::InHyperspace
							 && uqm::activityKind(uqm::currentActivity()) != ActivityKind::InLastBattle);

	uqm::log::debug("InitGalaxy(): transition_width = {}, "
					"transition_height = {}",
					TransitionWidth(), TransitionHeight());

	PRIM_LINKS Links {uqm::makeDword(EndOfList, EndOfList)};
	uint16_t factor {OneShift + MaxReduction + (BackgroundShift - 3)};
	num_sceneries = 0;
	for (i = 0, ppt = log_star_array; i < NumStars; ++i, ++ppt)
	{
		uint16_t p {};

		p = AllocDisplayPrim();
		SetPrimFlags(&DisplayArray[p], 0);

		if (i == BigStarCount || i == BigStarCount + MedStarCount)
		{
			++factor;
		}

		ppt->x = (int32_t)((uint16_t)uqm::random().next() % SpaceWidth()) << factor;
		ppt->y = (int32_t)((uint16_t)uqm::random().next() % SpaceHeight()) << factor;

		if (i < BigStarCount + MedStarCount)
		{
			SetPrimType(&DisplayArray[p], PrimitiveType::Stamp);
			SetPrimColor(&DisplayArray[p],
						 BuildColor(MakeRgb15(0x0B, 0x0B, 0x1F), 0x09));

			if (useScenery && i == (BigStarCount + MedStarCount - 1))
			{ //Set SA-MATRA as background image on the second Star layer
				SetPrimType(&DisplayArray[p], PrimitiveType::Stamp);
				DisplayArray[p].Object.Stamp.frame = scenery;
				num_sceneries = 1;
			}
			else
			{
				DisplayArray[p].Object.Stamp.frame = stars_in_space;
			}
		}
		else
		{
			if (IsHd())
			{ // In HdResolutionFactor the smallest stars are images
				SetPrimType(&DisplayArray[p], PrimitiveType::Stamp);
				if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::InHyperspace)
				{
					SetPrimFlags(&DisplayArray[p], UnscaledStamp);
					DisplayArray[p].Object.Stamp.frame =
						SetAbsFrameIndex(stars_in_space, 3);
				}
				else
				{
					DisplayArray[p].Object.Stamp.frame =
						SetAbsFrameIndex(stars_in_space, 96);
					if (inQuasiSpace())
					{
						SetPrimFlags(&DisplayArray[p], HyperToQuasiColor);
					}
				}
			}
			else
			{ // Pixel starpoints in original res
				SetPrimType(&DisplayArray[p], PrimitiveType::Point);
				if (uqm::activityKind(uqm::currentActivity()) != ActivityKind::InHyperspace)
				{
					SetPrimColor(&DisplayArray[p],
								 BuildColor(
									 MakeRgb15(0x15, 0x15, 0x15), 0x07));
				}
				else if (uqm::gameState().flag(uqm::GameFlag::ARILOU_SPACE_SIDE) <= 1)
				{
					SetPrimColor(&DisplayArray[p],
								 BuildColor(
									 MakeRgb15(0x14, 0x00, 0x00), 0x8C));
				}
				else
				{
					SetPrimColor(&DisplayArray[p],
								 BuildColor(
									 MakeRgb15(0x00, 0x0E, 0x00), 0x8C));
				}
			}
		}

		InsertPrim(&Links, p, GetPredLink(Links));
	}

	SortStarBlock(&StarBlock[0]);
	SortStarBlock(&StarBlock[1]);
	SortStarBlock(&StarBlock[2]);
}

static bool CmpMovePoints(const GfxPoint* pt1, const LargePoint* pt2, int32_t dx, int32_t dy,
						  int16_t reduction)
{
	if (uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step)
	{
		return (int)pt1->x != (int)((pt2->x - dx) >> reduction)
			|| (int)pt1->y != (int)((pt2->y - dy) >> reduction);
	}
	else
	{
		return (int)pt1->x != (int)(((pt2->x - dx) << ZoomShift) / reduction)
			|| (int)pt1->y != (int)(((pt2->y - dy) << ZoomShift) / reduction);
	}
}

void MoveGalaxy(ViewState view_state, int32_t dx, int32_t dy)
{
	Primitive* pprim {};
	const static uint16_t star_counts[] =
		{
			BigStarCount,
			MedStarCount,
			SmlStarCount};
	const static uint16_t star_frame_ofs[] = {32 + 26, 26, 0};
	const bool meleeScaleModeIsStep {uqm::toTFBScaleMode(uqm::UQMOptions::read().meleeScale) == uqm::TFBScaleMode::Step};

	if (view_state != ViewState::Stable)
	{
		uint16_t i = 0;
		uint16_t iss {};
		uint16_t scale = 0;
		LargePoint* ppt {};
		int wrap_around {};

		uint16_t reduction {zoom_out};

		if (view_state == ViewState::Change)
		{
			if (inHQSpace())
			{
				for (iss = 0, pprim = DisplayArray; iss < 2; ++iss)
				{
					for (i = star_counts[iss]; i > 0; --i, ++pprim)
					{
						pprim->Object.Stamp.frame = SetAbsFrameIndex(
							stars_in_space,
							(uint16_t)(uqm::random().next() & 31)
								+ star_frame_ofs[iss]);

						if (IsHd() && inQuasiSpace())
						{
							SetPrimFlags(pprim, HyperToQuasiColor);
						}
					}
				}
			}
			else
			{
				uint8_t star_object[2];
				FRAME star_frame[3];

				if (meleeScaleModeIsStep)
				{
					scale = reduction;
				}
				else
				{
					if (reduction == (1 << (ZoomShift + 2)))
					{
						scale = 2;
					}
					else if (reduction < (1 << (ZoomShift + 2)) && reduction >= (1 << (ZoomShift + 1)))
					{
						scale = 1;
					}
					else
					{
						scale = 0;
					}
				}

				if (!IsHd())
				{
					star_frame[0] = IncFrameIndex(stars_in_space);
					star_frame[1] = stars_in_space;

					if (meleeScaleModeIsStep)
					{ /* on PC, the closest stars are images when zoomed out */
						star_object[0] = *PrimitiveType::Stamp;
						if (reduction > 0)
						{
							star_object[1] = *PrimitiveType::Point;
							star_frame[0] = star_frame[1];
						}
						else
						{
							star_object[1] = *PrimitiveType::Stamp;
						}
					}
					else
					{ /* on 3DO, the closest stars are pixels when zoomed out */
						star_object[1] = *PrimitiveType::Point;
						if (reduction > (1 << ZoomShift))
						{
							star_object[0] = *PrimitiveType::Point;
						}
						else
						{
							star_object[0] = *PrimitiveType::Stamp;
						}
					}
				}
				else
				{
					star_frame[0] = stars_in_space;						 // large blue
					star_frame[1] = IncFrameIndex(stars_in_space);		 // mid blue
					star_frame[2] = SetAbsFrameIndex(stars_in_space, 2); // small blue

					star_object[0] = star_object[1] = *PrimitiveType::Stamp;

					if (meleeScaleModeIsStep && reduction > 0)
					{
						star_frame[0] = star_frame[1];
						star_frame[1] = star_frame[2];
					}
					else
					{
						if (scale == 2)
						{
							star_frame[0] = star_frame[1] = star_frame[2];
						}
						else if (scale == 1)
						{
							/* Kruzen: a hack to slip in unscaled flag */
							star_object[1] |= (UnscaledStamp << 7);
							star_frame[0] = star_frame[1];
							star_frame[1] = star_frame[2];
						}
					}
				}
				for (iss = 0, pprim = DisplayArray; iss < 2; ++iss)
				{
					for (i = star_counts[iss]; i > (num_sceneries & iss); --i, ++pprim)
					{
						SetPrimType(pprim, static_cast<PrimitiveType>(star_object[iss] & 15));
						SetPrimFlags(pprim, star_object[iss] >> 7);
						pprim->Object.Stamp.frame = star_frame[iss];
					}
				}
				if (i > 0)
				{
					pprim->Object.Stamp.frame = SetAbsFrameIndex(scenery, scale);
				}
			}
		}

		if (inHQSpace())
		{
			for (i = BigStarCount + MedStarCount, pprim = DisplayArray;
				 i > 0; --i, ++pprim)
			{
				uint16_t base_index {};

				base_index = GetFrameIndex(pprim->Object.Stamp.frame) - 26;
				pprim->Object.Stamp.frame =
					SetAbsFrameIndex(pprim->Object.Stamp.frame,
									 ((base_index & ~31) + ((base_index + 1) & 31)) + 26);
			}

			dx <<= 3;
			dy <<= 3;
		}

		WrapStarBlock(2, dx, dy);
		WrapStarBlock(1, dx, dy);
		WrapStarBlock(0, dx, dy);

		if (!inHQSpace())
		{
			dx = SpaceOrg.x;
			dy = SpaceOrg.y;
			if (meleeScaleModeIsStep)
			{
				reduction += OneShift;
			}
			else
			{
				reduction <<= OneShift;
			}
		}
		else
		{
			dx = (int16_t)(LogSpaceWidth() >> 1)
			   - (LogSpaceWidth() >> ((MaxReduction + 1)
									  - MaxVisReduction));
			dy = (int16_t)(LogSpaceHeight() >> 1)
			   - (LogSpaceHeight() >> ((MaxReduction + 1)
									   - MaxVisReduction));
			if (meleeScaleModeIsStep)
			{
				reduction = MaxVisReduction + OneShift;
			}
			else
			{
				reduction = MaxZoomOut << OneShift;
			}
		}

		ppt = log_star_array;
		for (iss = 0, pprim = DisplayArray, wrap_around = LogSpaceWidth();
			 iss < 3 && (view_state == ViewState::Change || CmpMovePoints(&pprim->Object.Point, ppt, dx, dy, reduction));
			 ++iss, wrap_around <<= 1, dx <<= 1, dy <<= 1)
		{
			for (i = star_counts[iss]; i > 0; --i, ++pprim, ++ppt)
			{
				// ppt->x &= (LogSpaceWidth() - 1);
				ppt->x = WrapVal(ppt->x, wrap_around);
				if (meleeScaleModeIsStep)
				{
					pprim->Object.Point.x = (ppt->x - dx) >> reduction;
					pprim->Object.Point.y = (ppt->y - dy) >> reduction;
				}
				else
				{
					pprim->Object.Point.x = ((ppt->x - dx) << ZoomShift)
										  / reduction;
					pprim->Object.Point.y = ((ppt->y - dy) << ZoomShift)
										  / reduction;
				}
			}
			if (meleeScaleModeIsStep)
			{
				++reduction;
			}
			else
			{
				reduction <<= 1;
			}
		}
	}

	DisplayLinks = uqm::makeDword(NumStars - 1, 0);
}

void SetStarPoint(GfxPoint pt, uint16_t i)
{
	log_star_array[i].x = pt.x;
	log_star_array[i].y = pt.y;
}
