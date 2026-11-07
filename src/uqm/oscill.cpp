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
#include "colors.h"
#include "libs/graphics/drawable.h"
#include "libs/sound/sound.h"
#include "libs/sound/stream.h"
#include "libs/sound/trackplayer.h"
#include "util.h"

namespace uqm
{
static FrameRef scope_frame;
static int scope_init = 0;
static FrameRef scopeWork;
static GfxExtent scopeSize;
static uqstl::vector<uint8_t> scopeData {};
bool oscillDisabled = false;

void InitOscilloscope(FrameRef scopeBg)
{
	scope_frame = scopeBg;
	if (!scope_init)
	{
		const GfxExtent size = GetFrameBounds(scope_frame);

		scopeWork = CaptureDrawable(CreateDrawable(
			CreateFlags::WantPixmap | CreateFlags::MappedToDisplay,
			size.width, size.height, 1));

		// assume and subtract the borders
		scopeSize.width = ResDescale(size.width);
		scopeSize.height = ResDescale(size.height);
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

uint8_t ScaleHeightByVolume(uint8_t scope_data, bool toScale)
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

static constexpr auto DimPercentage {0.77};

void DrawOscilloscopeLines(GfxStamp* s, uint8_t* scope_data, bool nonStop, bool toScale)
{
	int i {};
	Color scopeColor;

	GfxContextRef oldContext {SetContext(OffScreenContext)};
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
		scopeColor.r *= DimPercentage;
		scopeColor.g *= DimPercentage;
		scopeColor.b *= DimPercentage;
	}

	SetContextForeGroundColor(scopeColor);

	if (scope_data)
	{
		for (i = 0; i < scopeSize.width - 1; ++i)
		{
			GfxLine line;

			line.first.x = ResScale(i);
			line.first.y = ResScale(ScaleHeightByVolume(scope_data[i],
														toScale));
			line.second.x = ResScale(i + 1);
			line.second.y = ResScale(ScaleHeightByVolume(
				scope_data[i + 1], toScale));
			DrawLine(&line, ResScale(1));
		}
	}
	else
	{
		GfxLine line;

		line.first.x = 0;
		line.first.y = ResScale((scopeSize.height / 2));
		line.second.x = ResScale(scopeSize.width);
		line.second.y = line.first.y;
		DrawLine(&line, ResScale(1));
	}

	SetContext(oldContext);

	s->frame = scopeWork;
}

// draws the oscilloscope
void DrawOscilloscope(void)
{
	GfxStamp s;
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
	GfxStamp s;

	const ContextScope contextScope {RadarContext};

	DrawOscilloscopeLines(&s, nullptr, false, false);
	s.origin = MAKE_POINT(0, 0);
	DrawStamp(&s);
}

static GfxStamp sliderStamp;
static GfxStamp buttonStamp;
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

void InitSlider(int x, int y, int width, FrameRef sliderFrame, FrameRef buttonFrame)
{
	const GfxExtent sliderSize = GetFrameBounds(sliderFrame);
	const GfxExtent buttonSize = GetFrameBounds(buttonFrame);

	sliderStamp.origin.x = x;
	sliderStamp.origin.y = y;
	sliderStamp.frame = sliderFrame;

	buttonStamp.origin.x = x;
	buttonStamp.origin.y = y - ((buttonSize.height - sliderSize.height) / 2);
	buttonStamp.frame = buttonFrame;

	sliderSpace = width - buttonSize.width;
}

void SetSliderImage(FrameRef f)
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
		const BatchScope batch {};
		DrawStamp(&sliderStamp);
		DrawStamp(&buttonStamp);
	}
}

} // namespace uqm
