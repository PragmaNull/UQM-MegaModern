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
#ifndef UQM_OSCILL_H
#define UQM_OSCILL_H

#include "core/misc/bittricks.h"
#include "libs/gfxlib.h"

namespace uqm
{
extern bool sliderDisabled;
extern bool oscillDisabled;

extern void InitOscilloscope(FrameRef scopeBg);
extern void DrawOscilloscope(void);
extern void UninitOscilloscope(void);
extern void FlattenOscilloscope(void);

extern void InitSlider(int x, int y, int width, FrameRef sliderFrame,
					   FrameRef buttonFrame);
extern void SetSliderImage(FrameRef f);
void DrawSlider(void);

} // namespace uqm

#endif // UQM_OSCILL_H
