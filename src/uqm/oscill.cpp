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

#include "oscill.h"
#include <cmath>
#include "setup.h"
// for OffScreenContext
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/sound/sound.h"
#include "libs/sound/trackplayer.h"
#include "core/log/log.h"
#include "core/stl/vector.h"
#include "colors.h"
#include "menustat.h"
#include "util.h"
#include "options.h"

static FRAME scope_frame;
static int scope_init = 0;
static FRAME scopeWork;
static EXTENT scopeSize;
static uqstl::vector<uint8_t> scopeData {};
bool oscillDisabled = false;

void InitOscilloscope(FRAME scopeBg)
{
	scope_frame = scopeBg;
	if (!scope_init)
	{
		const EXTENT size = GetFrameBounds(scope_frame);

		scopeWork = CaptureDrawable(CreateDrawable(
			WantPixmap | MappedToDisplay,
			size.width, size.height, 1));

		// assume and subtract the borders
		scopeSize.width = RES_DESCALE(size.width);
		scopeSize.height = RES_DESCALE(size.height);
		scopeData.resize(static_cast<size_t>(scopeSize.width));

		scope_init = 1;
	}
}

void UninitOscilloscope(void)
{
	DestroyDrawable(ReleaseDrawable(scopeWork));
	scopeWork = nullptr;
	scope_init = 0;
}

uint8_t
ScaleHeightByVolume(uint8_t scope_data, bool toScale)
{
	if (!toScale || musicVolume == NormalVolume)
	{
		return scope_data;
	}
	else
	{
		const float scale = (float)musicVolume / NormalVolume;
		float res {NAN};

		if (scope_data >= (scopeSize.height / 2))
		{
			res = scope_data - (scopeSize.height / 2);
			res *= scale;
			return (uint8_t)((scopeSize.height / 2) + res);
		}
		else
		{
			res = (scopeSize.height / 2) - scope_data;
			res *= scale;
			return (uint8_t)((scopeSize.height / 2) - res);
		}
	}
}

void DrawOscilloscopeLines(STAMP* s, uint8_t* scope_data, bool nonStop, bool toScale)
{
	int i {};
	Color scopeColor;

	GFXCONTEXT oldContext {SetContext(OffScreenContext)};
	SetContextFGFrame(scopeWork);
	SetContextClipRect(nullptr);

	// draw the background image
	s->origin.x = 0;
	s->origin.y = 0;
	s->frame = scope_frame;

	DrawStamp(s);

	// Set oscilloscope line color
	scopeColor = uqm::UQMOptions::read().scopeStyle != uqm::EmulationMode::PC ?
					 ScopeColor3do :
					 ScopeColorPc;

	if (nonStop)
	{ // Dim the oscilloscope lines for Non-Stop option
#define DIM_PERCENTAGE 0.77
		scopeColor.r *= DIM_PERCENTAGE;
		scopeColor.g *= DIM_PERCENTAGE;
		scopeColor.b *= DIM_PERCENTAGE;
	}

	SetContextForeGroundColor(scopeColor);

	if (scope_data)
	{
		for (i = 0; i < scopeSize.width - 1; ++i)
		{
			LINE line;

			line.first.x = RES_SCALE(i);
			line.first.y = RES_SCALE(ScaleHeightByVolume(scope_data[i],
														 toScale));
			line.second.x = RES_SCALE(i + 1);
			line.second.y = RES_SCALE(ScaleHeightByVolume(
				scope_data[i + 1], toScale));
			DrawLine(&line, RES_SCALE(1));
		}
	}
	else
	{
		LINE line;

		line.first.x = 0;
		line.first.y = RES_SCALE((scopeSize.height / 2));
		line.second.x = RES_SCALE(scopeSize.width);
		line.second.y = line.first.y;
		DrawLine(&line, RES_SCALE(1));
	}

	SetContext(oldContext);

	s->frame = scopeWork;
}

// draws the oscilloscope
void DrawOscilloscope(void)
{
	STAMP s;
	uint8_t* const scope_data {scopeData.data()};

	if (oscillDisabled)
	{
		return;
	}

	assert(scopeData.size() == static_cast<size_t>(scopeSize.width));
	assert(scopeSize.height < 128);

	if (GraphForegroundStream(
			scope_data, scopeSize.width, scopeSize.height, usingSpeech))
	{
		DrawOscilloscopeLines(&s, scope_data, false, !usingSpeech);
	}
	else if (GraphForegroundStream(
				 scope_data, scopeSize.width, scopeSize.height, false)
			 && usingSpeech && uqm::UQMOptions::read().nonStopOscill)
	{
		DrawOscilloscopeLines(&s, scope_data, true, true);
	}
	else
	{
		DrawOscilloscopeLines(&s, nullptr, false, false);
	}

	// draw the final scope image to screen
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp(&s);

	DrawRadarBorder();
}

void FlattenOscilloscope(void)
{
	STAMP s;

	GFXCONTEXT OldContext {SetContext(RadarContext)};

	DrawOscilloscopeLines(&s, nullptr, false, false);
	s.origin = MAKE_POINT(0, 0);
	DrawStamp(&s);
	SetContext(OldContext);
}

static STAMP sliderStamp;
static STAMP buttonStamp;
static bool sliderChanged = false;
int sliderSpace; // slider width - button width
bool sliderDisabled = false;

/*
 * Initialise the communication progress bar
 * x - x location of slider
 * y - y location of slider
 * width - width of slider
 * height - height of slider
 * bwidth - width of button indicating current progress
 * bheight - height of button indicating progress
 * f - image for the slider
 */

void InitSlider(int x, int y, int width, FRAME sliderFrame, FRAME buttonFrame)
{
	const EXTENT sliderSize = GetFrameBounds(sliderFrame);
	const EXTENT buttonSize = GetFrameBounds(buttonFrame);

	sliderStamp.origin.x = x;
	sliderStamp.origin.y = y;
	sliderStamp.frame = sliderFrame;

	buttonStamp.origin.x = x;
	buttonStamp.origin.y = y - ((buttonSize.height - sliderSize.height) / 2);
	buttonStamp.frame = buttonFrame;

	sliderSpace = width - buttonSize.width;
}

void SetSliderImage(FRAME f)
{
	sliderChanged = true;
	buttonStamp.frame = f;
}

void DrawSlider(void)
{
	static int last_offs = -1;

	if (sliderDisabled || (!usingSpeech && uqm::UQMOptions::read().smoothScroll == uqm::EmulationMode::PC))
	{
		return;
	}

	const int offs {GetTrackPosition(sliderSpace)};
	if (offs != last_offs || sliderChanged)
	{
		sliderChanged = false;
		last_offs = offs;
		buttonStamp.origin.x = sliderStamp.origin.x + offs;
		BatchGraphics();
		DrawStamp(&sliderStamp);
		DrawStamp(&buttonStamp);
		UnbatchGraphics();
	}
}
