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
#ifndef UQM_SETUP_H
#define UQM_SETUP_H

#include "displist.h"
#include "globdata.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "libs/gfxlib.h"
#include "libs/time/timelib.h"
#include "uqm/inteldefs.h"

namespace uqm
{
extern ResourceIndex hResIndex;

extern FrameRef Screen;
extern FrameRef ActivityFrame;
extern FrameRef StatusFrame;
extern FrameRef SubmenuFrame;
extern FrameRef FlagStatFrame;
extern FrameRef MiscDataFrame;
extern FrameRef visitedStarsFrame;
extern FrameRef FontGradFrame;
extern FrameRef BorderFrame;
extern FrameRef HDBorderFrame;
extern FrameRef CustBevelFrame;
extern FrameRef DefBevelFrame;

extern GfxContextRef OffScreenContext;
// OffScreenContext can often refer to a deleted ForeGroundFrame
// Always call SetContextFGFrame() before drawing anything to it
// Neither is the state of its ClipRect guaranteed.
extern GfxContextRef ScreenContext;
extern GfxContextRef SpaceContext;
extern GfxContextRef StatusContext;

extern int16_t screen_width, screen_height;

extern FontRef StarConFont;
extern FontRef MicroFont;
extern FontRef TinyFont;
extern FontRef TinyFontBold;
extern FontRef TinyFontCond;
extern FontRef PlyrFont;
extern FontRef LabelFont;
extern FontRef SlabFont;
extern FontRef SquareFont;
extern FontRef PlayMenuFont;

extern Queue race_q[];
/* Array of lists of ships involved in a battle, one queue per side;
		 * queue Element is Starship */


extern bool solTexturesPresent;
extern bool SyreenVoiceFix;
extern bool EndlessSCLoaded;
extern bool VolasPackPresent;
extern uint8_t Rando;
extern bool DeathBySurrender;
extern bool DeathByMelee;
extern bool DeathBySuicide;
extern bool SpaceMusicOK;
extern bool oldPlanetsPresent;
extern bool classicPackPresent;

extern ActivityWord LastActivity;

extern PlayerControlFlags PlayerControl[];

extern bool usingSpeech;
// Actual speech presence indicator which decouples reality from
// the user option, thus the user option remains as pure intent

bool InitContexts(void);
void UninitPlayerInput(void);
bool InitGameKernel(void);
void UninitGameKernel(void);

extern bool LoadKernel(int argc, char* argv[]);
extern void FreeKernel(void);

int initIO(void);
void uninitIO(void);

bool SetPlayerInput(uint16_t playerI);
bool SetPlayerInputAll(void);
void ClearPlayerInput(uint16_t playerI);
void ClearPlayerInputAll(void);

} // namespace uqm

#endif // UQM_SETUP_H
