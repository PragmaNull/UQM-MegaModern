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

/* Mixer for low-level sound output drivers
 */

#include "core/stl/stl.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mixer.h"
#include "mixerint.h"
#include "core/misc/FatalExit.h"
#include "libs/time/timelib.h"
#include "core/thread/Mutex.h"
#include "core/log/log.h"

static uint32_t mixer_initialized = 0;
static uint32_t mixer_format;
static uint32_t mixer_chansize;
static uint32_t mixer_sampsize;
static uint32_t mixer_freq;
static uint32_t mixer_channels;
static MixerError last_error {MixerError::NoError};
static MixerQuality mixer_quality;
static mixer_Resampling mixer_resampling;
static MixerFlags mixer_flags;

/* when locking more than one mutex
 * you must lock them in this order
 */
static uqm::RecursiveMutex src_mutex {};
static uqm::RecursiveMutex buf_mutex {};
static uqm::RecursiveMutex act_mutex {};

static constexpr auto MaxSources {8};
static mixer_Source* active_sources[MaxSources] UQM_GUARDED_BY(act_mutex);

/*************************************************
 *  Internals
 */

static void mixer_SetError(MixerError error)
{
	last_error = error;
}

/*************************************************
 *  General interface
 */

MixerError mixer_GetError(void)
{
	const MixerError error {last_error};
	last_error = MixerError::NoError;
	return error;
}

/* Initialize the mixer with a certain audio format */
bool mixer_Init(uint32_t frequency, uint32_t format, MixerQuality quality,
				MixerFlags flags)
{
	if (mixer_initialized)
	{
		mixer_Uninit();
	}

	last_error = MixerError::NoError;
	{
		const uqm::LockGuard lock {act_mutex};
		memset(active_sources, 0, sizeof(mixer_Source*) * MaxSources);
	}

	mixer_chansize = MixFormatBpc(format);
	mixer_channels = MixFormatChans(format);
	mixer_sampsize = MixFormatSampsize(format);
	mixer_freq = frequency;
	mixer_quality = quality;
	mixer_format = format;
	mixer_flags = flags;

	mixer_resampling.None = mixer_ResampleNone;
	mixer_resampling.Downsample = mixer_ResampleNearest;
	if (mixer_quality == MixerQuality::Default)
	{
		mixer_resampling.Upsample = mixer_UpsampleLinear;
	}
	else if (mixer_quality == MixerQuality::High)
	{
		mixer_resampling.Upsample = mixer_UpsampleCubic;
	}
	else
	{
		mixer_resampling.Upsample = mixer_ResampleNearest;
	}

	mixer_initialized = 1;

	return true;
}

/* Uninitialize the mixer */
void mixer_Uninit(void)
{
	if (mixer_initialized)
	{
		mixer_initialized = 0;
	}
}

/**********************************************************
 * THE mixer
 *
 */

void mixer_MixChannels(void* userdata, uint8_t* stream, int32_t len)
{
	const uint8_t* end_stream = stream + len;
	bool left = true;

	/* keep this order or die */
	src_mutex.lock();
	buf_mutex.lock();
	act_mutex.lock();

	for (; stream < end_stream; stream += mixer_chansize)
	{
		float fullsamp = 0;

		for (uint32_t i {0}; i < MaxSources; i++)
		{
			mixer_Source* src {};
			float samp = 0;

			/* find next source */
			for (; i < MaxSources && ((src = active_sources[i]) == 0 || src->state != MixerSourceState::Playing || !mixer_SourceGetNextSample(src, &samp, left));
				 i++)
				;

			if (i < MaxSources)
			{
				/* sample acquired */
				fullsamp += samp;
			}
		}

		/* clip the sample */
		if (mixer_chansize == 2)
		{
			/* check S16 clipping */
			if (fullsamp > INT16_MAX)
			{
				fullsamp = INT16_MAX;
			}
			else if (fullsamp < INT16_MIN)
			{
				fullsamp = INT16_MIN;
			}
		}
		else
		{
			/* check S8 clipping */
			if (fullsamp > INT8_MAX)
			{
				fullsamp = INT8_MAX;
			}
			else if (fullsamp < INT8_MIN)
			{
				fullsamp = INT8_MIN;
			}
		}

		mixer_PutSampleExt(stream, mixer_chansize, (int32_t)fullsamp);
		if (mixer_channels == 2)
		{
			left = !left;
		}
	}

	/* keep this order or die */
	act_mutex.unlock();
	buf_mutex.unlock();
	src_mutex.unlock();

	(void)userdata; // satisfying compiler - unused arg
}

/* fake mixer -- only process buffer and source states */
void mixer_MixFake(void* userdata, uint8_t* stream, int32_t len)
{
	const uint8_t* end_stream = stream + len;
	bool left = true;

	/* keep this order or die */
	src_mutex.lock();
	buf_mutex.lock();
	act_mutex.lock();

	for (; stream < end_stream; stream += mixer_chansize)
	{
		for (uint32_t i {0}; i < MaxSources; i++)
		{
			mixer_Source* src {};
			float samp {NAN};

			/* find next source */
			for (; i < MaxSources && ((src = active_sources[i]) == 0 || src->state != MixerSourceState::Playing || !mixer_SourceGetFakeSample(src, &samp, left));
				 i++)
				;
		}
		if (mixer_channels == 2)
		{
			left = !left;
		}
	}

	/* keep this order or die */
	act_mutex.unlock();
	buf_mutex.unlock();
	src_mutex.unlock();

	(void)userdata; // satisfying compiler - unused arg
}

/*************************************************
 *  Sources interface
 */

/* generate n sources */
void mixer_GenSources(uint32_t n, mixer_Object* psrcobj)
{
	if (n == 0)
	{
		return; /* do nothing per OpenAL */
	}

	if (!psrcobj)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_GenSources() called with null ptr");
		return;
	}
	for (; n; n--, psrcobj++)
	{
		mixer_Source* src {new mixer_Source {}};
		src->magic = mixer_srcMagic;
		src->locked = false;
		src->state = MixerSourceState::Initial;
		src->looping = false;
		src->gain = MixGainAdj;
		src->leftGain = 1;
		src->rightGain = 1;
		src->cqueued = 0;
		src->cprocessed = 0;
		src->firstqueued = 0;
		src->nextqueued = 0;
		src->prevqueued = 0;
		src->lastqueued = 0;
		src->pos = 0;
		src->count = 0;

		*psrcobj = (mixer_Object)src;
	}
}

/* delete n sources */
void mixer_DeleteSources(uint32_t n, mixer_Object* psrcobj)
{
	uint32_t i {};
	const mixer_Object* pcurobj {};

	if (n == 0)
	{
		return; /* do nothing per OpenAL */
	}

	if (!psrcobj)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_DeleteSources() called with null ptr");
		return;
	}

	src_mutex.lock();

	/* check to make sure we can delete all sources */
	for (i = n, pcurobj = psrcobj; i && pcurobj; i--, pcurobj++)
	{
		const mixer_Source* src = (mixer_Source*)*pcurobj;

		if (!src)
		{
			continue;
		}

		if (src->magic != mixer_srcMagic)
		{
			break;
		}
	}

	if (i)
	{ /* some source failed */
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_DeleteSources(): not a source");
	}
	else
	{ /* all sources checked out */
		for (; n; n--, psrcobj++)
		{
			mixer_Source* src = (mixer_Source*)*psrcobj;

			if (!src)
			{
				continue;
			}

			/* stopping should not be necessary
			 * under ideal circumstances
			 */
			if (src->state != MixerSourceState::Initial)
			{
				mixer_SourceStop_internal(src);
			}

			/* unqueueing should not be necessary
			 * under ideal circumstances
			 */
			mixer_SourceUnqueueAll(src);
			delete src;
			*psrcobj = 0;
		}
	}

	src_mutex.unlock();
}

/* check if really is a source */
bool mixer_IsSource(mixer_Object srcobj)
{
	const mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		return false;
	}

	src_mutex.lock();
	const bool ret {src->magic == mixer_srcMagic};
	src_mutex.unlock();

	return ret;
}

/* set source integer property */
void mixer_Sourcei(mixer_Object srcobj, MixerSourceProp pname,
				   mixer_IntVal value)
{
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_Sourcei() called with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_Sourcei(): not a source");
	}
	else
	{
		switch (pname)
		{
			case MixerSourceProp::Looping:
				src->looping = value;
				break;
			case MixerSourceProp::Buffer:
				{
					mixer_Buffer* buf = (mixer_Buffer*)value;

					if (src->cqueued > 0)
					{
						mixer_SourceUnqueueAll(src);
					}

					if (buf && !mixer_CheckBufferState(buf, "mixer_Sourcei"))
					{
						break;
					}

					src->firstqueued = buf;
					src->nextqueued = src->firstqueued;
					src->prevqueued = 0;
					src->lastqueued = src->nextqueued;
					if (src->lastqueued)
					{
						src->lastqueued->next = 0;
					}
					src->cqueued = 1;
				}
				break;
			case MixerSourceProp::SourceState:
				if (value == MixerSourceState::Initial)
				{
					mixer_SourceRewind_internal(src);
				}
				else
				{
					uqm::log::debug("mixer_Sourcei(MIX_SOURCE_STATE): "
									"unsupported state, call ignored");
				}
				break;
			default:
				mixer_SetError(MixerError::InvalidEnum);
				uqm::log::debug("mixer_Sourcei() called with unsupported property {}", static_cast<uint32_t>(pname));
		}
	}

	src_mutex.unlock();
}

/* set source float property */
void mixer_Sourcef(mixer_Object srcobj, MixerSourceProp pname, float value)
{
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_Sourcef() called with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_Sourcef(): not a source");
	}
	else
	{
		switch (pname)
		{
			case MixerSourceProp::Gain:
				src->gain = value * MixGainAdj;
				break;
			default:
				uqm::log::debug("mixer_Sourcei() called with unsupported property {}", static_cast<uint32_t>(pname));
		}
	}

	src_mutex.unlock();
}

/* set source float array property */
void mixer_Sourcefv(mixer_Object srcobj, MixerSourceProp pname, float* value)
{
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_Sourcefv() called with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_Sourcefv(): not a source");
	}
	else
	{
		switch (pname)
		{
			case MixerSourceProp::Position:
				{
					const float dist = sqrt(value[0] * value[0] + value[2] * value[2]);
					const float invDist = 1.f / std::max(0.01f, dist);
					float pan {NAN};
					if (dist == 0 || mixer_channels == 1)
					{
						pan = 0;
					}
					else
					{
						pan = value[0] / dist;
					}
					src->leftGain = (1 - pan) * invDist;
					src->rightGain = (1 + pan) * invDist;
					break;
				}
			default:
				uqm::log::debug("mixer_Sourcefv() called with unsupported property {}", static_cast<uint32_t>(pname));
		}
	}

	src_mutex.unlock();
}

/* get source integer property */
void mixer_GetSourcei(mixer_Object srcobj, MixerSourceProp pname,
					  mixer_IntVal* value)
{
	const mixer_Source* src = (mixer_Source*)srcobj;

	if (!src || !value)
	{
		mixer_SetError(src ? MixerError::InvalidValue : MixerError::InvalidName);
		uqm::log::debug("mixer_GetSourcei() called with null param");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_GetSourcei(): not a source");
	}
	else
	{
		switch (pname)
		{
			case MixerSourceProp::Looping:
				*value = src->looping;
				break;
			case MixerSourceProp::Buffer:
				*value = (mixer_IntVal)src->firstqueued;
				break;
			case MixerSourceProp::SourceState:
				*value = +src->state;
				break;
			case MixerSourceProp::BuffersQueued:
				*value = src->cqueued;
				break;
			case MixerSourceProp::BuffersProcessed:
				*value = src->cprocessed;
				break;
			default:
				mixer_SetError(MixerError::InvalidEnum);
				uqm::log::debug("mixer_GetSourcei() called with unsupported property {}", static_cast<uint32_t>(pname));
		}
	}

	src_mutex.unlock();
}

/* get source float property */
void mixer_GetSourcef(mixer_Object srcobj, MixerSourceProp pname,
					  float* value)
{
	const mixer_Source* src = (mixer_Source*)srcobj;

	if (!src || !value)
	{
		mixer_SetError(src ? MixerError::InvalidValue : MixerError::InvalidName);
		uqm::log::debug("mixer_GetSourcef() called with null param");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_GetSourcef(): not a source");
	}
	else
	{
		switch (pname)
		{
			case MixerSourceProp::Gain:
				*value = src->gain / MixGainAdj;
				break;
			default:
				uqm::log::debug("mixer_GetSourcef() called with unsupported property {}", static_cast<uint32_t>(pname));
		}
	}

	src_mutex.unlock();
}

/* start the source; add it to active array */
void mixer_SourcePlay(mixer_Object srcobj)
{
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourcePlay() called with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourcePlay(): not a source");
	}
	else /* should make the source active */
	{
		if (src->state < MixerSourceState::Playing)
		{
			if (src->firstqueued && !src->nextqueued)
			{
				mixer_SourceRewind_internal(src);
			}
			mixer_SourceActivate(src);
		}
		src->state = MixerSourceState::Playing;
	}

	src_mutex.unlock();
}

/* stop the source; remove it from active array and requeue buffers */
void mixer_SourceRewind(mixer_Object srcobj)
{
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourceRewind() called with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourcePlay(): not a source");
	}
	else
	{
		mixer_SourceRewind_internal(src);
	}

	src_mutex.unlock();
}

/* pause the source; keep in active array */
void mixer_SourcePause(mixer_Object srcobj)
{
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourcePause() called with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourcePause(): not a source");
	}
	else /* should keep all buffers and offsets */
	{
		if (src->state < MixerSourceState::Playing)
		{
			mixer_SourceActivate(src);
		}
		src->state = MixerSourceState::Paused;
	}

	src_mutex.unlock();
}

/* stop the source; remove it from active array
 * and unqueue 'queued' buffers
 */
void mixer_SourceStop(mixer_Object srcobj)
{
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourceStop() called with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourceStop(): not a source");
	}
	else /* should remove queued buffers */
	{
		if (src->state >= MixerSourceState::Playing)
		{
			mixer_SourceDeactivate(src);
		}
		mixer_SourceStop_internal(src);
		src->state = MixerSourceState::Stopped;
	}

	src_mutex.unlock();
}

/* queue buffers on the source */
void mixer_SourceQueueBuffers(mixer_Object srcobj, uint32_t n,
							  mixer_Object* pbufobj)
{
	uint32_t i {};
	const mixer_Object* pobj {};
	mixer_Source* src = (mixer_Source*)srcobj;

	if (!src || !pbufobj)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourceQueueBuffers() called "
						"with null param");
		return;
	}

	buf_mutex.lock();
	/* check to make sure we can safely queue all buffers */
	for (i = n, pobj = pbufobj; i; i--, pobj++)
	{
		mixer_Buffer* buf = (mixer_Buffer*)*pobj;
		if (!buf || !mixer_CheckBufferState(buf, "mixer_SourceQueueBuffers"))
		{
			break;
		}
	}
	buf_mutex.unlock();

	if (i == 0)
	{ /* all buffers checked out */
		src_mutex.lock();
		buf_mutex.lock();

		if (src->magic != mixer_srcMagic)
		{
			mixer_SetError(MixerError::InvalidName);
			uqm::log::debug("mixer_SourceQueueBuffers(): not a source");
		}
		else
		{
			for (i = n, pobj = pbufobj; i; i--, pobj++)
			{
				mixer_Buffer* buf = (mixer_Buffer*)*pobj;

				/* add buffer to the chain */
				if (src->lastqueued)
				{
					src->lastqueued->next = buf;
				}
				src->lastqueued = buf;

				if (!src->firstqueued)
				{
					src->firstqueued = buf;
					src->nextqueued = buf;
					src->prevqueued = 0;
				}
				src->cqueued++;
				buf->state = MixerBufferState::Queued;
			}
		}

		buf_mutex.unlock();
		src_mutex.unlock();
	}
}

/* unqueue buffers from the source */
void mixer_SourceUnqueueBuffers(mixer_Object srcobj, uint32_t n,
								mixer_Object* pbufobj)
{
	uint32_t i {};
	mixer_Source* src = (mixer_Source*)srcobj;
	const mixer_Buffer* curbuf = 0;

	if (!src || !pbufobj)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourceUnqueueBuffers() called "
						"with null source");
		return;
	}

	src_mutex.lock();

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_SourceUnqueueBuffers(): not a source");
	}
	else if (n > src->cqueued)
	{
		mixer_SetError(MixerError::InvalidOperation);
	}
	else
	{
		buf_mutex.lock();

		/* check to make sure we can unqueue all buffers */
		for (i = n, curbuf = src->firstqueued;
			 i && curbuf && curbuf->state != MixerBufferState::Playing;
			 i--, curbuf = curbuf->next)
			;

		if (i)
		{
			mixer_SetError(MixerError::InvalidOperation);
			uqm::log::debug("mixer_SourceUnqueueBuffers(): "
							"active buffer attempted");
		}
		else
		{ /* all buffers checked out */
			for (i = n; i; i--, pbufobj++)
			{
				mixer_Buffer* buf = src->firstqueued;

				/* remove buffer from the chain */
				if (src->nextqueued == buf)
				{
					src->nextqueued = buf->next;
				}
				if (src->prevqueued == buf)
				{
					src->prevqueued = 0;
				}
				if (src->lastqueued == buf)
				{
					src->lastqueued = 0;
				}
				src->firstqueued = buf->next;
				src->cqueued--;

				if (buf->state == MixerBufferState::Processed)
				{
					src->cprocessed--;
				}

				buf->state = MixerBufferState::Filled;
				buf->next = 0;
				*pbufobj = (mixer_Object)buf;
			}
		}

		buf_mutex.unlock();
	}

	src_mutex.unlock();
}

/*************************************************
 *  Sources internals
 */

static void mixer_SourceUnqueueAll(mixer_Source* src)
{
	mixer_Buffer* nextbuf {};

	if (!src)
	{
		uqm::log::debug("mixer_SourceUnqueueAll() called "
						"with null source");
		return;
	}

	buf_mutex.lock();

	for (mixer_Buffer* buf {src->firstqueued}; buf; buf = nextbuf)
	{
		if (buf->state == MixerBufferState::Playing)
		{
			uqm::log::debug("mixer_SourceUnqueueAll(): "
							"attempted on active buffer");
		}
		nextbuf = buf->next;
		buf->state = MixerBufferState::Filled;
		buf->next = 0;
	}

	buf_mutex.unlock();

	src->firstqueued = 0;
	src->nextqueued = 0;
	src->prevqueued = 0;
	src->lastqueued = 0;
	src->cqueued = 0;
	src->cprocessed = 0;
	src->pos = 0;
	src->count = 0;
}

/* add the source to the active array */
static void mixer_SourceActivate(mixer_Source* src)
{
	uint32_t i {};

	act_mutex.lock();

	/* check active sources, see if this source is there already */
	for (i = 0; i < MaxSources && active_sources[i] != src; i++)
		;
	if (i < MaxSources)
	{ /* source found */
		uqm::log::debug("mixer_SourceActivate(): "
						"source already active in slot {}",
						i);
		act_mutex.unlock();
		return;
	}

	/* find an empty slot */
	for (i = 0; i < MaxSources && active_sources[i] != 0; i++)
		;
	if (i < MaxSources)
	{ /* slot found */
		active_sources[i] = src;
	}
	else
	{
		uqm::log::debug("mixer_SourceActivate(): "
						"no more slots available (max={})",
						MaxSources);
	}

	act_mutex.unlock();
}

/* remove the source from the active array */
static void mixer_SourceDeactivate(mixer_Source* src)
{
	uint32_t i {};

	act_mutex.lock();

	/* check active sources, see if this source is there */
	for (i = 0; i < MaxSources && active_sources[i] != src; i++)
		;
	if (i < MaxSources)
	{ /* source found */
		active_sources[i] = 0;
	}
	else
	{ /* source not found */
		uqm::log::debug("mixer_SourceDeactivate(): source not active");
	}

	act_mutex.unlock();
}

static void mixer_SourceStop_internal(mixer_Source* src)
{
	mixer_Buffer* buf {};
	mixer_Buffer* nextbuf {};

	if (!src->firstqueued)
	{
		return;
	}

	/* assert the source buffers state */
	if (!src->lastqueued)
	{
		uqm::log::debug("mixer_SourceStop_internal(): "
						"desynced source state");
#ifdef DEBUG
		uqm::fatalExit();
#endif
	}

	buf_mutex.lock();

	/* find last 'processed' buffer */
	for (buf = src->firstqueued;
		 buf && buf->next && buf->next != src->nextqueued;
		 buf = buf->next)
		;
	src->lastqueued = buf;
	if (buf)
	{
		buf->next = 0; /* break the chain */
	}

	/* unqueue all 'queued' buffers */
	for (buf = src->nextqueued; buf; buf = nextbuf)
	{
		nextbuf = buf->next;
		buf->state = MixerBufferState::Filled;
		buf->next = 0;
		src->cqueued--;
	}

	if (src->cqueued == 0)
	{ /* all buffers were removed */
		src->firstqueued = 0;
		src->lastqueued = 0;
	}
	src->nextqueued = 0;
	src->prevqueued = 0;
	src->pos = 0;
	src->count = 0;

	buf_mutex.unlock();
}

static void mixer_SourceRewind_internal(mixer_Source* src)
{
	/* should change the processed buffers to queued */
	mixer_Buffer* buf {};

	if (src->state >= MixerSourceState::Playing)
	{
		mixer_SourceDeactivate(src);
	}

	buf_mutex.lock();

	for (buf = src->firstqueued;
		 buf && buf->state != MixerBufferState::Queued;
		 buf = buf->next)
	{
		buf->state = MixerBufferState::Queued;
	}

	buf_mutex.unlock();

	src->pos = 0;
	src->count = 0;
	src->cprocessed = 0;
	src->nextqueued = src->firstqueued;
	src->prevqueued = 0;
	src->state = MixerSourceState::Initial;
}

/* get the sample next in queue in internal format */
static inline bool mixer_SourceGetNextSample(mixer_Source* src, float* psamp, bool left)
{
	/* fake the data if requested */
	if (any(mixer_flags, MixerFlags::FakeData))
	{
		return mixer_SourceGetFakeSample(src, psamp, left);
	}

	while (src->nextqueued)
	{
		mixer_Buffer* buf = src->nextqueued;

		if (!buf->data || buf->size < mixer_sampsize)
		{
			/* buffer invalid, go next */
			buf->state = MixerBufferState::Processed;
			src->pos = 0;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
			continue;
		}

		if (!left && buf->orgchannels == 1)
		{
			/* mono source so we can copy left channel to right */
			*psamp = src->samplecache;
		}
		else
		{
			*psamp = src->samplecache = buf->Resample(src, left) * src->gain;
		}
		*psamp *= left ? src->leftGain : src->rightGain;

		if (src->pos < buf->size || (left && buf->sampsize != mixer_sampsize))
		{
			buf->state = MixerBufferState::Playing;
		}
		else
		{
			/* buffer exhausted, go next */
			buf->state = MixerBufferState::Processed;
			src->pos = 0;
			src->prevqueued = src->nextqueued;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
		}

		return true;
	}

	/* no more playable buffers */
	if (src->state >= MixerSourceState::Playing)
	{
		mixer_SourceDeactivate(src);
	}

	src->state = MixerSourceState::Stopped;

	return false;
}

/* fake the next sample, but process buffers and states */
static inline bool mixer_SourceGetFakeSample(mixer_Source* src, float* psamp, bool left)
{
	while (src->nextqueued)
	{
		mixer_Buffer* buf = src->nextqueued;

		if (left || buf->orgchannels != 1)
		{
			if (mixer_freq == buf->orgfreq)
			{
				src->pos += mixer_chansize;
			}
			else
			{
				mixer_SourceAdvance(src, left);
			}
		}
		*psamp = 0;

		if (src->pos < buf->size || (left && buf->sampsize != mixer_sampsize))
		{
			buf->state = MixerBufferState::Playing;
		}
		else
		{
			/* buffer exhausted, go next */
			buf->state = MixerBufferState::Processed;
			src->pos = 0;
			src->prevqueued = src->nextqueued;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
		}

		return true;
	}

	/* no more playable buffers */
	if (src->state >= MixerSourceState::Playing)
	{
		mixer_SourceDeactivate(src);
	}

	src->state = MixerSourceState::Stopped;

	return false;
}

/* advance position in currently queued buffer */
static inline uint32_t mixer_SourceAdvance(mixer_Source* src, bool left)
{
	const mixer_Buffer* curr = src->nextqueued;
	if (curr->orgchannels == 2 && mixer_channels == 2)
	{
		if (!left)
		{
			src->pos += curr->high;
			src->count += curr->low;
			if (src->count > UINT16_MAX)
			{
				src->count -= UINT16_MAX;
				src->pos += curr->sampsize;
			}
			return mixer_chansize;
		}
	}
	else
	{
		src->pos += curr->high;
		src->count += curr->low;
		if (src->count > UINT16_MAX)
		{
			src->count -= UINT16_MAX;
			src->pos += curr->sampsize;
		}
	}
	return 0;
}

/*************************************************
 *  Buffers interface
 */

/* generate n buffer objects */
void mixer_GenBuffers(uint32_t n, mixer_Object* pbufobj)
{
	if (n == 0)
	{
		return; /* do nothing per OpenAL */
	}

	if (!pbufobj)
	{
		mixer_SetError(MixerError::InvalidValue);
		uqm::log::debug("mixer_GenBuffers() called with null ptr");
		return;
	}
	for (; n; n--, pbufobj++)
	{
		mixer_Buffer* buf {new mixer_Buffer {}};
		buf->magic = mixer_bufMagic;
		buf->locked = false;
		buf->state = MixerBufferState::Initial;
		buf->size = 0;
		buf->next = 0;
		buf->orgdata = 0;
		buf->orgfreq = 0;
		buf->orgsize = 0;
		buf->orgchannels = 0;
		buf->orgchansize = 0;

		*pbufobj = (mixer_Object)buf;
	}
}

/* delete n buffer objects */
void mixer_DeleteBuffers(uint32_t n, mixer_Object* pbufobj)
{
	uint32_t i {};
	const mixer_Object* pcurobj {};

	if (n == 0)
	{
		return; /* do nothing per OpenAL */
	}

	if (!pbufobj)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_DeleteBuffers() called with null ptr");
		return;
	}

	buf_mutex.lock();

	/* check to make sure we can delete all buffers */
	for (i = n, pcurobj = pbufobj; i && pcurobj; i--, pcurobj++)
	{
		const mixer_Buffer* buf = (mixer_Buffer*)*pcurobj;

		if (!buf)
		{
			continue;
		}

		if (buf->magic != mixer_bufMagic)
		{
			mixer_SetError(MixerError::InvalidName);
			uqm::log::debug("mixer_DeleteBuffers(): not a buffer");
			break;
		}
		else if (buf->locked)
		{
			mixer_SetError(MixerError::InvalidOperation);
			uqm::log::debug("mixer_DeleteBuffers(): locked buffer");
			break;
		}
		else if (buf->state >= MixerBufferState::Queued)
		{
			mixer_SetError(MixerError::InvalidOperation);
			uqm::log::debug("mixer_DeleteBuffers(): "
							"attempted on queued/active buffer");
			break;
		}
	}

	if (i == 0)
	{
		/* all buffers check out */
		for (; n; n--, pbufobj++)
		{
			const mixer_Buffer* buf = (mixer_Buffer*)*pbufobj;

			if (!buf)
			{
				continue;
			}

			delete buf;

			*pbufobj = 0;
		}
	}
	buf_mutex.unlock();
}

/* check if really a buffer object */
bool mixer_IsBuffer(mixer_Object bufobj)
{
	const mixer_Buffer* buf = (mixer_Buffer*)bufobj;

	if (!buf)
	{
		return false;
	}

	buf_mutex.lock();
	const bool ret {buf->magic == mixer_bufMagic};
	buf_mutex.unlock();

	return ret;
}

/* get buffer property */
void mixer_GetBufferi(mixer_Object bufobj, MixerBufferProp pname,
					  mixer_IntVal* value)
{
	const mixer_Buffer* buf = (mixer_Buffer*)bufobj;

	if (!buf || !value)
	{
		mixer_SetError(buf ? MixerError::InvalidValue : MixerError::InvalidName);
		uqm::log::debug("mixer_GetBufferi() called with null param");
		return;
	}

	buf_mutex.lock();

	if (buf->locked)
	{
		buf_mutex.unlock();
		mixer_SetError(MixerError::InvalidOperation);
		uqm::log::debug("mixer_GetBufferi() called with locked buffer");
		return;
	}

	if (buf->magic != mixer_bufMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_GetBufferi(): not a buffer");
	}
	else
	{
		/* Return original buffer values
		 */
		switch (pname)
		{
			case MixerBufferProp::Frequency:
				*value = buf->orgfreq;
				break;
			case MixerBufferProp::Bits:
				*value = buf->orgchansize << 3;
				break;
			case MixerBufferProp::Channels:
				*value = buf->orgchannels;
				break;
			case MixerBufferProp::Size:
				*value = buf->orgsize;
				break;
			case MixerBufferProp::Data:
				*value = (mixer_IntVal)buf->orgdata;
				break;
			default:
				mixer_SetError(MixerError::InvalidEnum);
				uqm::log::debug("mixer_GetBufferi() called with invalid property {}", static_cast<uint32_t>(pname));
		}
	}

	buf_mutex.unlock();
}

/* fill buffer with external data */
void mixer_BufferData(mixer_Object bufobj, uint32_t format, void* data,
					  uint32_t size, uint32_t freq)
{
	mixer_Buffer* buf = (mixer_Buffer*)bufobj;
	mixer_Convertion conv;
	uint32_t dstsize {};

	if (!buf || !data || !size)
	{
		mixer_SetError(buf ? MixerError::InvalidValue : MixerError::InvalidName);
		uqm::log::debug("mixer_BufferData() called with bad param");
		return;
	}

	buf_mutex.lock();

	if (buf->locked)
	{
		buf_mutex.unlock();
		mixer_SetError(MixerError::InvalidOperation);
		uqm::log::debug("mixer_BufferData() called "
						"with locked buffer");
		return;
	}

	if (buf->magic != mixer_bufMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("mixer_BufferData(): not a buffer");
	}
	else if (buf->state > MixerBufferState::Filled)
	{
		mixer_SetError(MixerError::InvalidOperation);
		uqm::log::debug("mixer_BufferData() attempted "
						"on in-use buffer");
	}
	else
	{
		buf->data.reset();
		buf->size = 0;

		/* Store original buffer values for OpenAL compatibility */
		buf->orgdata = data;
		buf->orgfreq = freq;
		buf->orgsize = size;
		buf->orgchannels = MixFormatChans(format);
		buf->orgchansize = MixFormatBpc(format);

		conv.srcsamples = conv.dstsamples =
			size / MixFormatSampsize(format);

		if (conv.dstsamples > UINT32_MAX / MixFormatSampsize(format))
		{
			mixer_SetError(MixerError::InvalidValue);
		}
		else
		{
			if (MixFormatChans(format) < MixFormatChans(mixer_format))
			{
				buf->sampsize = MixFormatBpc(mixer_format) * MixFormatChans(format);
			}
			else
			{
				buf->sampsize = MixFormatSampsize(mixer_format);
			}
			buf->size = dstsize = conv.dstsamples * buf->sampsize;

			/* only copy/convert the data if not faking */
			if (!any(mixer_flags, MixerFlags::FakeData))
			{
				buf->data = uqstl::make_unique_for_overwrite<uint8_t[]>(dstsize);

				if (MixFormatBpc(format) == MixFormatBpc(mixer_format) && MixFormatChans(format) <= MixFormatChans(mixer_format))
				{
					/* format is compatible with internal */
					buf->locked = true;
					buf_mutex.unlock();

					memcpy(buf->data.get(), data, size);
					if (MixFormatBpc(format) == 1)
					{
						/* convert buffer to S8 format internally */
						for (uint8_t* dst {buf->data.get()}; dstsize; dstsize--, dst++)
						{
							*dst ^= 0x80;
						}
					}

					buf_mutex.lock();
					buf->locked = false;
				}
				else
				{
					/* needs convertion */
					conv.srcfmt = format;
					conv.srcdata = data;
					conv.srcsize = size;

					if (MixFormatChans(format) < MixFormatChans(mixer_format))
					{
						conv.dstfmt = MixFormatMake(mixer_chansize,
													MixFormatChans(format));
					}
					else
					{
						conv.dstfmt = mixer_format;
					}
					conv.dstdata = buf->data.get();
					conv.dstsize = dstsize;

					buf->locked = true;
					buf_mutex.unlock();

					mixer_ConvertBuffer_internal(&conv);

					buf_mutex.lock();
					buf->locked = false;
				}
			}

			buf->state = MixerBufferState::Filled;
			buf->high = (buf->orgfreq / mixer_freq) * buf->sampsize;
			buf->low = (((buf->orgfreq % mixer_freq) << 16) / mixer_freq);
			if (mixer_freq == buf->orgfreq)
			{
				buf->Resample = mixer_resampling.None;
			}
			else if (mixer_freq > buf->orgfreq)
			{
				buf->Resample = mixer_resampling.Upsample;
			}
			else
			{
				buf->Resample = mixer_resampling.Downsample;
			}
		}
	}

	buf_mutex.unlock();
}

/*************************************************
 *  Buffer internals
 */

static inline bool mixer_CheckBufferState(mixer_Buffer* buf, const char* FuncName)
{
	if (!buf)
	{
		return false;
	}

	if (buf->magic != mixer_bufMagic)
	{
		mixer_SetError(MixerError::InvalidName);
		uqm::log::debug("{}(): not a buffer", FuncName);
		return false;
	}

	if (buf->locked)
	{
		mixer_SetError(MixerError::InvalidOperation);
		uqm::log::debug("{}(): locked buffer attempted", FuncName);
		return false;
	}

	if (buf->state != MixerBufferState::Filled)
	{
		mixer_SetError(MixerError::InvalidOperation);
		uqm::log::debug("{}: invalid buffer attempted", FuncName);
		return false;
	}
	return true;
}

static void mixer_ConvertBuffer_internal(mixer_Convertion* conv)
{
	conv->srcbpc = MixFormatBpc(conv->srcfmt);
	conv->srcchans = MixFormatChans(conv->srcfmt);
	conv->dstbpc = MixFormatBpc(conv->dstfmt);
	conv->dstchans = MixFormatChans(conv->dstfmt);

	conv->flags = MixerConvFlags::None;
	if (conv->srcbpc > conv->dstbpc)
	{
		conv->flags |= MixerConvFlags::SizeDown;
	}
	else if (conv->srcbpc < conv->dstbpc)
	{
		conv->flags |= MixerConvFlags::SizeUp;
	}
	if (conv->srcchans > conv->dstchans)
	{
		conv->flags |= MixerConvFlags::StereoDown;
	}
	else if (conv->srcchans < conv->dstchans)
	{
		conv->flags |= MixerConvFlags::StereoUp;
	}

	mixer_ResampleFlat(conv);
}

/*************************************************
 *  Resampling routines
 */

/* get a sample from external buffer
 * in internal format
 */
static inline int32_t mixer_GetSampleExt(void* src, uint32_t bpc)
{
	if (bpc == 2)
	{
		return *(int16_t*)src;
	}
	else
	{
		return (*(uint8_t*)src) - 128;
	}
}

/* get a sample from internal buffer */
static inline int32_t mixer_GetSampleInt(void* src, uint32_t bpc)
{
	if (bpc == 2)
	{
		return *(int16_t*)src;
	}
	else
	{
		return *(int8_t*)src;
	}
}

/* put a sample into an external buffer
 * from internal format
 */
static inline void mixer_PutSampleExt(void* dst, uint32_t bpc, int32_t samp)
{
	if (bpc == 2)
	{
		*(int16_t*)dst = samp;
	}
	else
	{
		*(uint8_t*)dst = samp ^ 0x80;
	}
}

/* put a sample into an internal buffer
 * in internal format
 */
static inline void mixer_PutSampleInt(void* dst, uint32_t bpc, int32_t samp)
{
	if (bpc == 2)
	{
		*(int16_t*)dst = samp;
	}
	else
	{
		*(int8_t*)dst = samp;
	}
}

/* get a sample from source */
static float mixer_ResampleNone(mixer_Source* src, bool left)
{
	uint8_t* d0 = src->nextqueued->data.get() + src->pos;
	src->pos += mixer_chansize;
	(void)left; // satisfying compiler - unused arg
	return mixer_GetSampleInt(d0, mixer_chansize);
}

/* get a resampled (up/down) sample from source (nearest neighbor) */
static float mixer_ResampleNearest(mixer_Source* src, bool left)
{
	uint8_t* d0 = src->nextqueued->data.get() + src->pos;
	d0 += mixer_SourceAdvance(src, left);
	return mixer_GetSampleInt(d0, mixer_chansize);
}

/* get an upsampled sample from source (linear interpolation) */
static float mixer_UpsampleLinear(mixer_Source* src, bool left)
{
	const mixer_Buffer* curr = src->nextqueued;
	const mixer_Buffer* next = src->nextqueued->next;
	uint8_t* d1 {};
	float s0 {NAN};
	float s1 {NAN};
	float t {NAN};

	t = src->count / 65536.0f;
	uint8_t* d0 {curr->data.get() + src->pos};
	d0 += mixer_SourceAdvance(src, left);

	if (d0 + curr->sampsize >= curr->data.get() + curr->size)
	{
		if (next && next->data && next->size >= curr->sampsize)
		{
			d1 = next->data.get();
			if (!left)
			{
				d1 += mixer_chansize;
			}
		}
		else
		{
			d1 = d0;
		}
	}
	else
	{
		d1 = d0 + curr->sampsize;
	}

	s0 = mixer_GetSampleInt(d0, mixer_chansize);
	s1 = mixer_GetSampleInt(d1, mixer_chansize);
	return s0 + t * (s1 - s0);
}

/* get an upsampled sample from source (cubic interpolation) */
static float mixer_UpsampleCubic(mixer_Source* src, bool left)
{
	const mixer_Buffer* prev = src->prevqueued;
	const mixer_Buffer* curr = src->nextqueued;
	const mixer_Buffer* next = src->nextqueued->next;
	uint8_t* d0 {};
	uint8_t* d1 {};
	uint8_t* d2 {};
	uint8_t* d3 {}; /* prev, curr, next, next + 1 */
	float t {NAN};
	float t2 {NAN};
	float a {NAN};
	float b {NAN};
	float c {NAN};
	float s0 {NAN};
	float s1 {NAN};
	float s2 {NAN};
	float s3 {NAN};

	t = src->count / 65536.0f;
	t2 = t * t;
	d1 = curr->data.get() + src->pos;
	d1 += mixer_SourceAdvance(src, left);

	if (d1 - curr->sampsize < curr->data.get())
	{
		if (prev && prev->data && prev->size >= curr->sampsize)
		{
			d0 = prev->data.get() + prev->size - curr->sampsize;
			if (!left)
			{
				d0 += mixer_chansize;
			}
		}
		else
		{
			d0 = d1;
		}
	}
	else
	{
		d0 = d1 - curr->sampsize;
	}

	if (d1 + curr->sampsize >= curr->data.get() + curr->size)
	{
		if (next && next->data && next->size >= curr->sampsize * 2)
		{
			d2 = next->data.get();
			if (!left)
			{
				d2 += mixer_chansize;
			}
			d3 = d2 + curr->sampsize;
		}
		else
		{
			d2 = d3 = d1;
		}
	}
	else
	{
		d2 = d1 + curr->sampsize;
		if (d2 + curr->sampsize >= curr->data.get() + curr->size)
		{
			if (next && next->data && next->size >= curr->sampsize)
			{
				d3 = next->data.get();
				if (!left)
				{
					d3 += mixer_chansize;
				}
			}
			else
			{
				d3 = d2;
			}
		}
		else
		{
			d3 = d2 + curr->sampsize;
		}
	}

	s0 = mixer_GetSampleInt(d0, mixer_chansize);
	s1 = mixer_GetSampleInt(d1, mixer_chansize);
	s2 = mixer_GetSampleInt(d2, mixer_chansize);
	s3 = mixer_GetSampleInt(d3, mixer_chansize);

	a = (3.0f * (s1 - s2) - s0 + s3) * 0.5f;
	b = 2.0f * s2 + s0 - ((5.0f * s1 + s3) * 0.5f);
	c = (s2 - s0) * 0.5f;

	return a * t2 * t + b * t2 + c * t + s1;
}

/* get next sample from external buffer
 * in internal format, while performing
 * convertion if necessary
 */
static inline int32_t mixer_GetConvSample(uint8_t** psrc, uint32_t bpc, MixerConvFlags flags)
{
	int32_t samp {mixer_GetSampleExt(*psrc, bpc)};
	*psrc += bpc;
	if (any(flags, MixerConvFlags::StereoDown))
	{
		/* downmix to mono - average up channels */
		samp = (samp + mixer_GetSampleExt(*psrc, bpc)) / 2;
		*psrc += bpc;
	}

	if (any(flags, MixerConvFlags::SizeUp))
	{
		/* convert S8 to S16 */
		samp <<= 8;
	}
	else if (any(flags, MixerConvFlags::SizeDown))
	{
		/* convert S16 to S8
		 * if arithmetic shift is available to the compiler
		 * it will use it to optimize this
		 */
		samp /= 0x100;
	}

	return samp;
}

/* put next sample into an internal buffer
 * in internal format, while performing
 * convertion if necessary
 */
static inline void mixer_PutConvSample(uint8_t** pdst, uint32_t bpc, MixerConvFlags flags, int32_t samp)
{
	mixer_PutSampleInt(*pdst, bpc, samp);
	*pdst += bpc;
	if (any(flags, MixerConvFlags::StereoUp))
	{
		mixer_PutSampleInt(*pdst, bpc, samp);
		*pdst += bpc;
	}
}

/* resampling with respect to sample size only */
static void mixer_ResampleFlat(mixer_Convertion* conv)
{
	const MixerConvFlags flags = conv->flags;
	uint8_t* src = (uint8_t*)conv->srcdata;
	uint8_t* dst = (uint8_t*)conv->dstdata;
	const uint32_t srcbpc = conv->srcbpc;
	const uint32_t dstbpc = conv->dstbpc;

	uint32_t samples {conv->srcsamples};
	if (!any(conv->flags, (MixerConvFlags::StereoUp | MixerConvFlags::StereoDown)))
	{
		samples *= conv->srcchans;
	}

	for (; samples; samples--)
	{
		const int32_t samp {mixer_GetConvSample(&src, srcbpc, flags)};
		mixer_PutConvSample(&dst, dstbpc, flags, samp);
	}
}
