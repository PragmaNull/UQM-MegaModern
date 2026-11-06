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

#include "libs/graphics/cmap.h"

#include "core/thread/Mutex.h"
#include "libs/time/timelib.h"
#include "libs/inplib.h"
#include "libs/strlib.h"
#include "core/log/log.h"
#include <string.h>
#include <stdlib.h>


struct ColorTransformControl
{
	int CMapIndex; // -1 means unused
	COLORMAPPTR CMapPtr;
	TimePeriod Ticks;
	TimeCount StartTime;
	TimeCount EndTime;
	Color OldCMap[NumberOfPlutvals];
};

static constexpr auto MaxXforms {16};
static struct
{
	uqm::Mutex Lock;
	ColorTransformControl TaskControl[MaxXforms] UQM_GUARDED_BY(Lock);
	int Highest UQM_GUARDED_BY(Lock);
	// 'pending' is Highest >= 0
} XFormControl;

static uqm::Mutex s_fadeLock {};
static int fadeAmount UQM_GUARDED_BY(s_fadeLock) = FadeNormalIntensity;
static int fadeDelta UQM_GUARDED_BY(s_fadeLock);
static TimeCount fadeStartTime UQM_GUARDED_BY(s_fadeLock);
static TimePeriod fadeInterval UQM_GUARDED_BY(s_fadeLock);

static constexpr auto SpareColormaps {20};

// Colormaps are rapidly replaced in some parts of the game, so
// it pays to have some spares on hand
static uqm::Mutex s_mapLock {};
static TFB_ColorMap* poolhead UQM_GUARDED_BY(s_mapLock);
static int poolcount UQM_GUARDED_BY(s_mapLock);

static TFB_ColorMap* colormaps[MaxColormaps] UQM_GUARDED_BY(s_mapLock);
static int mapcount UQM_GUARDED_BY(s_mapLock);


static void release_colormap(TFB_ColorMap* map) UQM_REQUIRES(s_mapLock);
static void delete_colormap(TFB_ColorMap* map);


void InitColorMaps(void)
{
	// init xform control
	const uqm::LockGuard lock {XFormControl.Lock};
	XFormControl.Highest = -1;
	for (int i {0}; i < MaxXforms; ++i)
	{
		XFormControl.TaskControl[i].CMapIndex = -1;
	}
}

void UninitColorMaps(void)
{
	TFB_ColorMap* next {};

	const uqm::LockGuard lock {s_mapLock};
	for (int i {0}; i < MaxColormaps; ++i)
	{
		TFB_ColorMap* map = colormaps[i];
		if (!map)
		{
			continue;
		}
		release_colormap(map);
		colormaps[i] = 0;
	}

	// free spares
	for (; poolhead; poolhead = next, --poolcount)
	{
		next = poolhead->next;
		delete_colormap(poolhead);
	}
}

static inline TFB_ColorMap* alloc_colormap(void) UQM_REQUIRES(s_mapLock)
// returns an addrefed object
{
	TFB_ColorMap* map {};

	if (poolhead)
	{ // have some spares
		map = poolhead;
		poolhead = map->next;
		--poolcount;
	}
	else
	{ // no spares, need a new one
		map = new TFB_ColorMap {};
		map->palette = AllocNativePalette();
	}
	map->next = nullptr;
	map->index = -1;
	map->refcount = 1;
	map->version = 0;

	return map;
}

static TFB_ColorMap* clone_colormap(TFB_ColorMap* from, int index) UQM_REQUIRES(s_mapLock)
// returns an addrefed object
{
	TFB_ColorMap* map {alloc_colormap()};
	if (!map)
	{
		uqm::log::warn("FATAL: clone_colormap(): "
					   "could not allocate a map");
		exit(EXIT_FAILURE);
	}
	else
	{ // fresh new map
		map->index = index;
		if (from)
		{
			map->version = from->version;
		}
	}
	map->version++;

	return map;
}

static void delete_colormap(TFB_ColorMap* map)
{
	FreeNativePalette(map->palette);
	delete map;
}

static inline void free_colormap(TFB_ColorMap* map) UQM_REQUIRES(s_mapLock)
{
	if (!map)
	{
		uqm::log::warn("free_colormap(): tried to free a nullptr map");
		return;
	}

	if (poolcount < SpareColormaps)
	{ // return to the spare pool
		map->next = poolhead;
		poolhead = map;
		++poolcount;
	}
	else
	{ // don't need any more spares
		delete_colormap(map);
	}
}

static inline TFB_ColorMap* get_colormap(int index) UQM_REQUIRES(s_mapLock)
{
	TFB_ColorMap* map {colormaps[index]};
	if (!map)
	{
		uqm::log::critical("BUG: get_colormap(): map not present");
		exit(EXIT_FAILURE);
	}

	map->refcount++;
	return map;
}

static void release_colormap(TFB_ColorMap* map)
{
	if (!map)
	{
		return;
	}

	if (map->refcount <= 0)
	{
		uqm::log::warn("BUG: release_colormap(): refcount not >0");
		return;
	}

	map->refcount--;
	if (map->refcount == 0)
	{
		free_colormap(map);
	}
}

void TFB_ReturnColorMap(TFB_ColorMap* map)
{
	const uqm::LockGuard lock {s_mapLock};
	release_colormap(map);
}

TFB_ColorMap* TFB_GetColorMap(int index)
{
	const uqm::LockGuard lock {s_mapLock};
	return get_colormap(index);
}

void GetColorMapColors(Color* colors, TFB_ColorMap* map)
{
	if (!map)
	{
		return;
	}

	for (int i {0}; i < NumberOfPlutvals; ++i)
	{
		colors[i] = GetNativePaletteColor(map->palette, i);
	}
}

bool SetColorMap(COLORMAPPTR map)
{
	const uint8_t* colors = (uint8_t*)map;

	if (!map)
	{
		return true;
	}

	int start {*colors++};
	int end {*colors++};
	if (start > end)
	{
		uqm::log::warn("ERROR: SetColorMap(): "
					   "starting map ({}) not less or eq ending ({})",
					   start, end);
		return false;
	}
	if (start >= MaxColormaps)
	{
		uqm::log::warn("ERROR: SetColorMap(): "
					   "starting map ({}) beyond range (0-{})",
					   start, (int)MaxColormaps - 1);
		return false;
	}
	if (end >= MaxColormaps)
	{
		uqm::log::warn("SetColorMap(): "
					   "ending map ({}) beyond range (0-{})\n",
					   end, (int)MaxColormaps - 1);
		end = MaxColormaps - 1;
	}

	const int total_size {end + 1};

	const uqm::LockGuard lock {s_mapLock};

	if (total_size > mapcount)
	{
		mapcount = total_size;
	}

	// parse the supplied PLUTs into our colormaps
	for (TFB_ColorMap** mpp {colormaps + start}; start <= end; ++start, ++mpp)
	{
		TFB_ColorMap* oldmap {*mpp};
		TFB_ColorMap* newmap {clone_colormap(oldmap, start)};

		for (int i {0}; i < NumberOfPlutvals; ++i, colors += PlutvalByteSize)
		{
			Color color;

			color.a = 0xff;
			color.r = colors[PlutvalRed];
			color.g = colors[PlutvalGreen];
			color.b = colors[PlutvalBlue];
			SetNativePaletteColor(newmap->palette, i, color);
		}

		*mpp = newmap;
		release_colormap(oldmap);
	}

	return true;
}

/* Fade Transforms */

int GetFadeAmount(void)
{
	int newAmount {};

	const uqm::LockGuard lock {s_fadeLock};

	if (fadeInterval != TimePeriod {})
	{ // have a pending fade
		const TimePeriod elapsed {uqstl::min(GetTimeCounter() - fadeStartTime, fadeInterval)};

		newAmount = static_cast<int>(fadeAmount + fadeDelta * elapsed.count() / fadeInterval.count());

		if (elapsed >= fadeInterval)
		{ // fade is over
			fadeAmount = newAmount;
			fadeInterval = TimePeriod {};
		}
	}
	else
	{ // no fade pending, return the current
		newAmount = fadeAmount;
	}

	return newAmount;
}

static void finishPendingFade(void) UQM_REQUIRES(s_fadeLock)
{
	if (fadeInterval != TimePeriod {})
	{ // end the fade immediately
		fadeAmount += fadeDelta;
		fadeInterval = TimePeriod {};
	}
}

static void FlushFadeXForms(void)
{
	const uqm::LockGuard lock {s_fadeLock};
	finishPendingFade();
}

TimeCount FadeScreen(ScreenFadeType fadeType, TimePeriod TimeInterval)
{
	TimeCount TimeOut;
	int FadeEnd {};

	switch (fadeType)
	{
		case FadeAllToBlack:
		case FadeSomeToBlack:
			FadeEnd = FadeNoIntensity;
			break;
		case FadeAllToColor:
		case FadeSomeToColor:
			FadeEnd = FadeNormalIntensity;
			break;
		case FadeAllToWhite:
		case FadeSomeToWhite:
			FadeEnd = FadeFullIntensity;
			break;
		default:
			return (GetTimeCounter());
	}

	// Don't make users wait for fades
	if (QuitPosted)
	{
		TimeInterval = TimePeriod {};
	}

	const uqm::LockGuard lock {s_fadeLock};

	finishPendingFade();

	if (TimeInterval <= TimePeriod {} || fadeAmount == FadeEnd)
	{ // end the fade immediately
		fadeAmount = FadeEnd;
		// cancel any pending fades
		fadeInterval = TimePeriod {};
		TimeOut = GetTimeCounter();
	}
	else
	{
		fadeInterval = TimeInterval;
		fadeDelta = FadeEnd - fadeAmount;
		fadeStartTime = GetTimeCounter();
		TimeOut = fadeStartTime + TimeInterval + TimePeriod {1};
	}

	return TimeOut;
}

/* Colormap Transforms */

static void finish_colormap_xform(int which) UQM_REQUIRES(XFormControl.Lock) UQM_EXCLUDES(s_mapLock)
{
	SetColorMap(XFormControl.TaskControl[which].CMapPtr);
	XFormControl.TaskControl[which].CMapIndex = -1;
	// check Highest ptr
	if (which == XFormControl.Highest)
	{
		do
		{
			--which;
		} while (which >= 0 && XFormControl.TaskControl[which].CMapIndex == -1);

		XFormControl.Highest = which;
	}
}

static inline uint8_t blendChan(uint8_t c1, uint8_t c2, int weight, int scale)
{
	return c1 + ((int)c2 - c1) * weight / scale;
}

/* This gives the XFormColorMap task a timeslice to do its thing
 * Only one thread should ever be allowed to be calling this at any time
 */
bool XFormColorMap_step(void)
{
	bool Changed = false;
	const TimeCount Now {GetTimeCounter()};

	const uqm::LockGuard lock {XFormControl.Lock};

	for (int x {0}; x <= XFormControl.Highest; ++x)
	{
		ColorTransformControl* control = &XFormControl.TaskControl[x];
		const int index = control->CMapIndex;
		const int TicksLeft {static_cast<int>((control->EndTime - Now).count())};
		TFB_ColorMap* curmap {};

		if (index < 0)
		{
			continue; // unused slot
		}

		{
			const uqm::LockGuard mapLock {s_mapLock};
			curmap = colormaps[index];
			if (curmap && TicksLeft > 0)
			{
				constexpr auto XformScale {0x10000};
				TFB_ColorMap* newmap = nullptr;
				int i {};

				newmap = clone_colormap(curmap, index);

				const Color* oldClr {control->OldCMap};
				const uint8_t* newClr {(uint8_t*)control->CMapPtr + 2};

				const int frac {static_cast<int>((control->Ticks.count() - TicksLeft) * XformScale / control->Ticks.count())};

				for (i = 0; i < NumberOfPlutvals; ++i, ++oldClr,
					newClr += PlutvalByteSize)
				{
					Color color;

					color.a = 0xff;
					color.r = blendChan(oldClr->r, newClr[PlutvalRed],
										frac, XformScale);
					color.g = blendChan(oldClr->g, newClr[PlutvalGreen],
										frac, XformScale);
					color.b = blendChan(oldClr->b, newClr[PlutvalBlue],
										frac, XformScale);
					SetNativePaletteColor(newmap->palette, i, color);
				}

				colormaps[index] = newmap;
				release_colormap(curmap);
			}
		}
		if (!curmap)
		{
			uqm::log::error("BUG: XFormColorMap_step(): no current map");
			finish_colormap_xform(x);
			continue;
		}

		if (TicksLeft <= 0)
		{ // asked for immediate xform or already done
			finish_colormap_xform(x);
		}

		Changed = true;
	}

	return Changed;
}

static void FlushPLUTXForms(void)
{
	const uqm::LockGuard lock {XFormControl.Lock};

	for (int i {0}; i <= XFormControl.Highest; ++i)
	{
		if (XFormControl.TaskControl[i].CMapIndex >= 0)
		{
			finish_colormap_xform(i);
		}
	}
	XFormControl.Highest = -1; // all gone
}

static TimeCount XFormPLUT(COLORMAPPTR ColorMapPtr, TimePeriod TimeInterval)
{
	TFB_ColorMap* map {};
	int x {};
	int first_avail = -1;
	TimeCount EndTime;
	TimeCount Now;

	Now = GetTimeCounter();
	const int index {*(uint8_t*)ColorMapPtr};

	const uqm::LockGuard lock {XFormControl.Lock};
	// Find an available slot, or reuse if required
	for (x = 0; x <= XFormControl.Highest
				&& index != XFormControl.TaskControl[x].CMapIndex;
		 ++x)
	{
		if (first_avail == -1 && XFormControl.TaskControl[x].CMapIndex == -1)
		{
			first_avail = x;
		}
	}

	if (index == XFormControl.TaskControl[x].CMapIndex)
	{ // already xforming this colormap -- cancel and reuse slot
		finish_colormap_xform(x);
	}
	else if (first_avail >= 0)
	{ // picked up a slot along the way
		x = first_avail;
	}
	else if (x >= MaxXforms)
	{ // flush some xforms if the queue is full
		uqm::log::debug("WARNING: XFormPLUT(): no slots available");
		x = XFormControl.Highest;
		finish_colormap_xform(x);
	}
	// take next unused one
	ColorTransformControl* control {&XFormControl.TaskControl[x]};
	if (x > XFormControl.Highest)
	{
		XFormControl.Highest = x;
	}

	// make a copy of the current map
	{
		const uqm::LockGuard mapLock {s_mapLock};
		map = colormaps[index];
		if (!map)
		{
			uqm::log::warn("BUG: XFormPLUT(): no current map");
			return TimeCount {};
		}
		GetColorMapColors(control->OldCMap, map);
	}

	control->CMapIndex = index;
	control->CMapPtr = ColorMapPtr;
	control->Ticks = uqstl::max(TimeInterval, TimePeriod {}); /* prevent negative fade */
	control->StartTime = Now;
	control->EndTime = EndTime = Now + control->Ticks;

	return (EndTime);
}

TimeCount XFormColorMap(COLORMAPPTR ColorMapPtr, TimePeriod TimeInterval)
{
	if (!ColorMapPtr)
	{
		return TimeCount {};
	}

	// Don't make users wait for transforms
	if (QuitPosted)
	{
		TimeInterval = TimePeriod {};
	}

	return XFormPLUT(ColorMapPtr, TimeInterval);
}

void FlushColorXForms(void)
{
	FlushFadeXForms();
	FlushPLUTXForms();
}

// The type conversions are implicit and will generate errors
// or warnings if types change imcompatibly
COLORMAPPTR GetColorMapAddress(COLORMAP map, size_t index)
{
	if (map == nullptr || map->size() == 0)
	{
		return nullptr;
	}
	const uqstl::string_view bytes {map->bytes(index % map->size())};
	return bytes.empty() ? nullptr : bytes.data();
}

static void DoTransformColorMap(Color* colors, COLORMAPPTR ColorMapPtr, uint16_t from,
								uint16_t to)
{ // New fancy func to change colors of current colormap
	const int p_index = *(uint8_t*)ColorMapPtr;
	const uqm::LockGuard lock {s_mapLock};

	TFB_ColorMap* map {colormaps[p_index]};
	if (!map)
	{
		uqm::log::error("BUG: XFormColorMap_step(): no current map");
		return;
	}

	{
		TFB_ColorMap* newmap = nullptr;
		uint8_t* newPtr = (uint8_t*)ColorMapPtr + 2;

		newmap = clone_colormap(map, p_index);
		const Color* c {colors};

		for (int i {from}; i < to; ++i, ++c, newPtr += PlutvalByteSize)
		{
			newPtr[PlutvalRed] = c->r;
			newPtr[PlutvalGreen] = c->g;
			newPtr[PlutvalBlue] = c->b;
			SetNativePaletteColor(newmap->palette, i, *c);
		}

		colormaps[p_index] = newmap;
		release_colormap(map);
	}
}

void SetColorMapColors(Color* colors, COLORMAPPTR ColorMapPtr, uint16_t from,
					   uint16_t to)
{
	if (!ColorMapPtr)
	{
		return;
	}

	DoTransformColorMap(colors, ColorMapPtr, from, to);
}

Color GetColorMapColor(uint16_t ColorMapIndex, uint16_t ColorIndex)
{
	const uqm::LockGuard lock {s_mapLock};
	return GetNativePaletteColor(colormaps[ColorMapIndex]->palette,
								 ColorIndex);
}

uint8_t GetColorMapTableIndex(COLORMAP map, size_t index)
{
	return *static_cast<const uint8_t*>(GetColorMapAddress(map, index));
}
