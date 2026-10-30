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
// for GetStringAddress()
#include "core/log/log.h"
#include <string.h>
#include <stdlib.h>


typedef struct xform_control
{
	int CMapIndex; // -1 means unused
	COLORMAPPTR CMapPtr;
	int16_t Ticks;
	uint32_t StartTime;
	uint32_t EndTime;
	Color OldCMap[NUMBER_OF_PLUTVALS];
} XFORM_CONTROL;

#define MAX_XFORMS 16
static struct
{
	uqm::Mutex Lock;
	XFORM_CONTROL TaskControl[MAX_XFORMS] UQM_GUARDED_BY(Lock);
	int Highest UQM_GUARDED_BY(Lock);
	// 'pending' is Highest >= 0
} XFormControl;

static uqm::Mutex s_fadeLock {};
static int fadeAmount UQM_GUARDED_BY(s_fadeLock) = FADE_NORMAL_INTENSITY;
static int fadeDelta UQM_GUARDED_BY(s_fadeLock);
static TimeCount fadeStartTime UQM_GUARDED_BY(s_fadeLock);
static int32_t fadeInterval UQM_GUARDED_BY(s_fadeLock);

#define SPARE_COLORMAPS 20

// Colormaps are rapidly replaced in some parts of the game, so
// it pays to have some spares on hand
static uqm::Mutex s_mapLock {};
static TFB_ColorMap* poolhead UQM_GUARDED_BY(s_mapLock);
static int poolcount UQM_GUARDED_BY(s_mapLock);

static TFB_ColorMap* colormaps[MAX_COLORMAPS] UQM_GUARDED_BY(s_mapLock);
static int mapcount UQM_GUARDED_BY(s_mapLock);


static void release_colormap(TFB_ColorMap* map) UQM_REQUIRES(s_mapLock);
static void delete_colormap(TFB_ColorMap* map);


void InitColorMaps(void)
{
	int i;

	// init xform control
	const uqm::LockGuard lock {XFormControl.Lock};
	XFormControl.Highest = -1;
	for (i = 0; i < MAX_XFORMS; ++i)
	{
		XFormControl.TaskControl[i].CMapIndex = -1;
	}
}

void UninitColorMaps(void)
{
	int i;
	TFB_ColorMap* next;

	const uqm::LockGuard lock {s_mapLock};
	for (i = 0; i < MAX_COLORMAPS; ++i)
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

static inline TFB_ColorMap*
alloc_colormap(void) UQM_REQUIRES(s_mapLock)
// returns an addrefed object
{
	TFB_ColorMap* map;

	if (poolhead)
	{ // have some spares
		map = poolhead;
		poolhead = map->next;
		--poolcount;
	}
	else
	{ // no spares, need a new one
		map = (TFB_ColorMap*)HMalloc(sizeof(*map));
		map->palette = AllocNativePalette();
		if (!map->palette)
		{
			HFree(map);
			return nullptr;
		}
	}
	map->next = nullptr;
	map->index = -1;
	map->refcount = 1;
	map->version = 0;

	return map;
}

static TFB_ColorMap*
clone_colormap(TFB_ColorMap* from, int index) UQM_REQUIRES(s_mapLock)
// returns an addrefed object
{
	TFB_ColorMap* map;

	map = alloc_colormap();
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

static void
delete_colormap(TFB_ColorMap* map)
{
	FreeNativePalette(map->palette);
	HFree(map);
}

static inline void
free_colormap(TFB_ColorMap* map) UQM_REQUIRES(s_mapLock)
{
	if (!map)
	{
		uqm::log::warn("free_colormap(): tried to free a nullptr map");
		return;
	}

	if (poolcount < SPARE_COLORMAPS)
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

static inline TFB_ColorMap*
get_colormap(int index) UQM_REQUIRES(s_mapLock)
{
	TFB_ColorMap* map;

	map = colormaps[index];
	if (!map)
	{
		uqm::log::critical("BUG: get_colormap(): map not present");
		exit(EXIT_FAILURE);
	}

	map->refcount++;
	return map;
}

static void
release_colormap(TFB_ColorMap* map)
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

TFB_ColorMap*
TFB_GetColorMap(int index)
{
	const uqm::LockGuard lock {s_mapLock};
	return get_colormap(index);
}

void GetColorMapColors(Color* colors, TFB_ColorMap* map)
{
	int i;

	if (!map)
	{
		return;
	}

	for (i = 0; i < NUMBER_OF_PLUTVALS; ++i)
	{
		colors[i] = GetNativePaletteColor(map->palette, i);
	}
}

bool SetColorMap(COLORMAPPTR map)
{
	int start, end;
	int total_size;
	uint8_t* colors = (uint8_t*)map;
	TFB_ColorMap** mpp;

	if (!map)
	{
		return true;
	}

	start = *colors++;
	end = *colors++;
	if (start > end)
	{
		uqm::log::warn("ERROR: SetColorMap(): "
					   "starting map ({}) not less or eq ending ({})",
					   start, end);
		return false;
	}
	if (start >= MAX_COLORMAPS)
	{
		uqm::log::warn("ERROR: SetColorMap(): "
					   "starting map ({}) beyond range (0-{})",
					   start, (int)MAX_COLORMAPS - 1);
		return false;
	}
	if (end >= MAX_COLORMAPS)
	{
		uqm::log::warn("SetColorMap(): "
					   "ending map ({}) beyond range (0-{})\n",
					   end, (int)MAX_COLORMAPS - 1);
		end = MAX_COLORMAPS - 1;
	}

	total_size = end + 1;

	const uqm::LockGuard lock {s_mapLock};

	if (total_size > mapcount)
	{
		mapcount = total_size;
	}

	// parse the supplied PLUTs into our colormaps
	for (mpp = colormaps + start; start <= end; ++start, ++mpp)
	{
		int i;
		TFB_ColorMap* newmap;
		TFB_ColorMap* oldmap;

		oldmap = *mpp;
		newmap = clone_colormap(oldmap, start);

		for (i = 0; i < NUMBER_OF_PLUTVALS; ++i, colors += PLUTVAL_BYTE_SIZE)
		{
			Color color;

			color.a = 0xff;
			color.r = colors[PLUTVAL_RED];
			color.g = colors[PLUTVAL_GREEN];
			color.b = colors[PLUTVAL_BLUE];
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
	int newAmount;

	const uqm::LockGuard lock {s_fadeLock};

	if (fadeInterval)
	{ // have a pending fade
		TimeCount Now = GetTimeCounter();
		int32_t elapsed;

		elapsed = Now - fadeStartTime;
		if (elapsed > fadeInterval)
		{
			elapsed = fadeInterval;
		}

		newAmount = fadeAmount + (long)fadeDelta * elapsed / fadeInterval;

		if (elapsed >= fadeInterval)
		{ // fade is over
			fadeAmount = newAmount;
			fadeInterval = 0;
		}
	}
	else
	{ // no fade pending, return the current
		newAmount = fadeAmount;
	}

	return newAmount;
}

static void
finishPendingFade(void) UQM_REQUIRES(s_fadeLock)
{
	if (fadeInterval)
	{ // end the fade immediately
		fadeAmount += fadeDelta;
		fadeInterval = 0;
	}
}

static void
FlushFadeXForms(void)
{
	const uqm::LockGuard lock {s_fadeLock};
	finishPendingFade();
}

uint32_t
FadeScreen(ScreenFadeType fadeType, int16_t TimeInterval)
{
	TimeCount TimeOut;
	int FadeEnd;

	switch (fadeType)
	{
		case FadeAllToBlack:
		case FadeSomeToBlack:
			FadeEnd = FADE_NO_INTENSITY;
			break;
		case FadeAllToColor:
		case FadeSomeToColor:
			FadeEnd = FADE_NORMAL_INTENSITY;
			break;
		case FadeAllToWhite:
		case FadeSomeToWhite:
			FadeEnd = FADE_FULL_INTENSITY;
			break;
		default:
			return (GetTimeCounter());
	}

	// Don't make users wait for fades
	if (QuitPosted)
	{
		TimeInterval = 0;
	}

	const uqm::LockGuard lock {s_fadeLock};

	finishPendingFade();

	if (TimeInterval <= 0 || fadeAmount == FadeEnd)
	{ // end the fade immediately
		fadeAmount = FadeEnd;
		// cancel any pending fades
		fadeInterval = 0;
		TimeOut = GetTimeCounter();
	}
	else
	{
		fadeInterval = TimeInterval;
		fadeDelta = FadeEnd - fadeAmount;
		fadeStartTime = GetTimeCounter();
		TimeOut = fadeStartTime + TimeInterval + 1;
	}

	return TimeOut;
}

/* Colormap Transforms */

static void
finish_colormap_xform(int which) UQM_REQUIRES(XFormControl.Lock) UQM_EXCLUDES(s_mapLock)
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

static inline uint8_t
blendChan(uint8_t c1, uint8_t c2, int weight, int scale)
{
	return c1 + ((int)c2 - c1) * weight / scale;
}

/* This gives the XFormColorMap task a timeslice to do its thing
 * Only one thread should ever be allowed to be calling this at any time
 */
bool XFormColorMap_step(void)
{
	bool Changed = false;
	int x;
	uint32_t Now = GetTimeCounter();

	const uqm::LockGuard lock {XFormControl.Lock};

	for (x = 0; x <= XFormControl.Highest; ++x)
	{
		XFORM_CONTROL* control = &XFormControl.TaskControl[x];
		int index = control->CMapIndex;
		int TicksLeft = control->EndTime - Now;
		TFB_ColorMap* curmap;

		if (index < 0)
		{
			continue; // unused slot
		}

		{
			const uqm::LockGuard mapLock {s_mapLock};
			curmap = colormaps[index];
			if (curmap && TicksLeft > 0)
			{
#define XFORM_SCALE 0x10000
				TFB_ColorMap* newmap = nullptr;
				uint8_t* newClr;
				Color* oldClr;
				int frac;
				int i;

				newmap = clone_colormap(curmap, index);

				oldClr = control->OldCMap;
				newClr = (uint8_t*)control->CMapPtr + 2;

				frac = (int)(control->Ticks - TicksLeft) * XFORM_SCALE
					 / control->Ticks;

				for (i = 0; i < NUMBER_OF_PLUTVALS; ++i, ++oldClr,
					newClr += PLUTVAL_BYTE_SIZE)
				{
					Color color;

					color.a = 0xff;
					color.r = blendChan(oldClr->r, newClr[PLUTVAL_RED],
										frac, XFORM_SCALE);
					color.g = blendChan(oldClr->g, newClr[PLUTVAL_GREEN],
										frac, XFORM_SCALE);
					color.b = blendChan(oldClr->b, newClr[PLUTVAL_BLUE],
										frac, XFORM_SCALE);
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

static void
FlushPLUTXForms(void)
{
	int i;

	const uqm::LockGuard lock {XFormControl.Lock};

	for (i = 0; i <= XFormControl.Highest; ++i)
	{
		if (XFormControl.TaskControl[i].CMapIndex >= 0)
		{
			finish_colormap_xform(i);
		}
	}
	XFormControl.Highest = -1; // all gone
}

static uint32_t
XFormPLUT(COLORMAPPTR ColorMapPtr, int16_t TimeInterval)
{
	TFB_ColorMap* map;
	XFORM_CONTROL* control;
	int index;
	int x;
	int first_avail = -1;
	uint32_t EndTime;
	uint32_t Now;

	Now = GetTimeCounter();
	index = *(uint8_t*)ColorMapPtr;

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
	else if (x >= MAX_XFORMS)
	{ // flush some xforms if the queue is full
		uqm::log::debug("WARNING: XFormPLUT(): no slots available");
		x = XFormControl.Highest;
		finish_colormap_xform(x);
	}
	// take next unused one
	control = &XFormControl.TaskControl[x];
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
			return (0);
		}
		GetColorMapColors(control->OldCMap, map);
	}

	control->CMapIndex = index;
	control->CMapPtr = ColorMapPtr;
	control->Ticks = TimeInterval;
	if (control->Ticks < 0)
	{
		control->Ticks = 0; /* prevent negative fade */
	}
	control->StartTime = Now;
	control->EndTime = EndTime = Now + control->Ticks;

	return (EndTime);
}

uint32_t
XFormColorMap(COLORMAPPTR ColorMapPtr, int16_t TimeInterval)
{
	if (!ColorMapPtr)
	{
		return (0);
	}

	// Don't make users wait for transforms
	if (QuitPosted)
	{
		TimeInterval = 0;
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
COLORMAPPTR
GetColorMapAddress(COLORMAP colormap)
{
	return GetStringAddress(colormap);
}

static void
DoTransformColorMap(Color* colors, COLORMAPPTR ColorMapPtr, uint16_t from,
					uint16_t to)
{ // New fancy func to change colors of current colormap
	TFB_ColorMap* map;
	int p_index = *(uint8_t*)ColorMapPtr;
	const uqm::LockGuard lock {s_mapLock};

	map = colormaps[p_index];
	if (!map)
	{
		uqm::log::error("BUG: XFormColorMap_step(): no current map");
		return;
	}

	{
		TFB_ColorMap* newmap = nullptr;
		uint8_t* newPtr = (uint8_t*)ColorMapPtr + 2;
		Color* c;
		int i;

		newmap = clone_colormap(map, p_index);
		c = colors;

		for (i = from; i < to; ++i, ++c, newPtr += PLUTVAL_BYTE_SIZE)
		{
			newPtr[PLUTVAL_RED] = c->r;
			newPtr[PLUTVAL_GREEN] = c->g;
			newPtr[PLUTVAL_BLUE] = c->b;
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

uint8_t
GetColorMapTableIndex(COLORMAP map)
{
	uint8_t* index = (uint8_t*)GetColorMapAddress(map);

	return *index;
}
