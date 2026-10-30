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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
// for abs()
#include "sound.h"
#include "sndintrn.h"
#include "libs/time/timelib.h"
#include "core/thread/Mutex.h"
#include "core/thread/Thread.h"
#include "core/log/log.h"


static uqstl::jthread s_decoderThread {};

// Mutex protects fade structures
static uqm::Mutex fade_mutex {};
static TimeCount musicFadeStartTime UQM_GUARDED_BY(fade_mutex);
static int32_t musicFadeInterval UQM_GUARDED_BY(fade_mutex);
static int musicFadeStartVolume UQM_GUARDED_BY(fade_mutex);
static int musicFadeDelta UQM_GUARDED_BY(fade_mutex);

static void add_scope_data(TFB_SoundSource* source, uint32_t bytes) UQM_REQUIRES(source->stream_mutex);


void PlayStream(TFB_SoundSample* sample, uint32_t source, bool looping, bool scope,
				bool rewind)
{
	uint32_t i;
	int32_t offset;
	TFB_SoundDecoder* decoder;

	if (!sample)
	{
		return;
	}

	StopStream(source);
	if (sample->callbacks.OnStartStream && !sample->callbacks.OnStartStream(sample))
	{
		return; // callback failed
	}

	uqstl::ranges::fill(sample->buffer_tag, TFB_SoundTag {});

	decoder = sample->decoder;
	offset = sample->offset;
	if (rewind)
	{
		SoundDecoder_Rewind(decoder);
	}
	else
	{
		offset += (int32_t)(SoundDecoder_GetTime(decoder) * GameTicksPerSecond);
	}

	if (source == MUSIC_SOURCE)
	{
		soundSource[source].start_time = 0;
	}

	soundSource[source].sample = sample;
	decoder->looping = looping;
	audio_Sourcei(soundSource[source].handle, audio_LOOPING, false);

	if (scope)
	{ // Prealloc the scope buffer in advance so that we do not
		// realloc it a zillion times
		soundSource[source].sbuf_size = sample->num_buffers * decoder->buffer_size + PAD_SCOPE_BYTES;
		soundSource[source].sbuffer.assign(soundSource[source].sbuf_size, 0);
	}

	for (i = 0; i < sample->num_buffers; ++i)
	{
		uint32_t decoded_bytes;

		decoded_bytes = SoundDecoder_Decode(decoder);
#if 0		
		log_add (log_Debug, "PlayStream(): source:{} filename:{} start:{} "
				"position:{} bytes:{}\n",
				source, decoder->filename, decoder->start_sample,
				decoder->pos, decoded_bytes);
#endif
		if (decoded_bytes == 0)
		{
			break;
		}

		audio_BufferData(sample->buffer[i], decoder->format,
						 decoder->buffer, decoded_bytes, decoder->frequency);
		audio_SourceQueueBuffers(soundSource[source].handle, 1,
								 &sample->buffer[i]);
		if (sample->callbacks.OnQueueBuffer)
		{
			sample->callbacks.OnQueueBuffer(sample, sample->buffer[i]);
		}

		if (scope)
		{
			add_scope_data(&soundSource[source], decoded_bytes);
		}

		if (decoder->error != SOUNDDECODER_OK)
		{
			if (decoder->error != SOUNDDECODER_EOF || !sample->callbacks.OnEndChunk || !sample->callbacks.OnEndChunk(sample, sample->buffer[i]))
			{ // Decoder probably run out of data before we could fill
				// all buffers, and OnEndChunk() did not set a new one
				break;
			}
			else
			{ // OnEndChunk() probably set a new decoder, get it
				decoder = sample->decoder;
			}
		}
	}

	soundSource[source].sbuf_lasttime = GetTimeCounter();
	// Adjust the start time so it looks like the stream has been playing
	// from the very beginning
	soundSource[source].start_time = GetTimeCounter() - offset;
	soundSource[source].pause_time = 0;
	soundSource[source].stream_should_be_playing = true;
	audio_SourcePlay(soundSource[source].handle);
}

void StopStream(uint32_t source)
{
	StopSource(source);

	soundSource[source].stream_should_be_playing = false;
	soundSource[source].sample = nullptr;

	soundSource[source].sbuffer = {};
	soundSource[source].sbuf_size = 0;
	soundSource[source].sbuf_head = 0;
	soundSource[source].sbuf_tail = 0;
	soundSource[source].pause_time = 0;
}

void PauseStream(uint32_t source)
{
	soundSource[source].stream_should_be_playing = false;
	if (!soundSource[source].pause_time)
	{
		soundSource[source].pause_time = GetTimeCounter();
	}
	audio_SourcePause(soundSource[source].handle);
}

void ResumeStream(uint32_t source)
{
	if (soundSource[source].pause_time)
	{ // Adjust the start time so it looks like the stream has
		// been playing all this time non-stop
		soundSource[source].start_time += GetTimeCounter()
										- soundSource[source].pause_time;
	}
	soundSource[source].pause_time = 0;
	soundSource[source].stream_should_be_playing = true;
	audio_SourcePlay(soundSource[source].handle);
}

void SeekStream(uint32_t source, uint32_t pos)
{
	TFB_SoundSample* sample = soundSource[source].sample;
	bool looping;
	bool scope;

	if (!sample)
	{
		return;
	}
	looping = sample->decoder->looping;
	scope = !soundSource[source].sbuffer.empty();

	StopSource(source);
	SoundDecoder_Seek(sample->decoder, pos);
	PlayStream(sample, source, looping, scope, false);
}

uint32_t
GetStreamFrame(uint32_t source)
{
	TFB_SoundSample* sample = soundSource[source].sample;

	if (!sample)
	{
		return 0;
	}

	return SoundDecoder_GetFrame(sample->decoder);
}

uint16_t
GetNumTrackerPos(uint32_t source)
{
	TFB_SoundSample* sample = soundSource[source].sample;

	if (!sample)
	{
		return 0;
	}

	return sample->decoder->numpos;
}

bool IsTracker(uint32_t source)
{
	TFB_SoundSample* sample = soundSource[source].sample;
	const char* filetype;

	if (!sample)
	{
		return false;
	}

	filetype = SoundDecoder_GetName(sample->decoder);

	return (bool)(strcmp(filetype, "MikMod") == 0);
}

float GetStreamLength(uint32_t source)
{
	TFB_SoundSample* sample = soundSource[source].sample;

	if (!sample)
	{
		return 0;
	}

	return sample->decoder->length * 1000;
}

uint32_t
GetStreamTime(uint32_t source)
{
	TFB_SoundSample* sample = soundSource[source].sample;

	if (!sample)
	{
		return 0;
	}

	return (uint32_t)SoundDecoder_GetTime(sample->decoder) * 1000;
}

bool PlayingStream(uint32_t source)
{
	const uqm::LockGuard lock {soundSource[source].stream_mutex};
	return soundSource[source].stream_should_be_playing;
}


TFB_SoundSample*
TFB_CreateSoundSample(TFB_SoundDecoder* decoder, uint32_t num_buffers,
					  const TFB_SoundCallbacks* pcbs /* can be nullptr */)
{
	auto* sample {new TFB_SoundSample {}};
	sample->decoder = decoder;
	sample->num_buffers = num_buffers;
	sample->buffer.assign(num_buffers, 0);
	audio_GenBuffers(num_buffers, sample->buffer.data());
	if (pcbs)
	{
		sample->callbacks = *pcbs;
	}

	return sample;
}

// Deletes all TFB_SoundSample data structures, except decoder
void TFB_DestroySoundSample(TFB_SoundSample* sample)
{
	audio_DeleteBuffers(sample->num_buffers, sample->buffer.data());
	delete sample;
}

void TFB_SetSoundSampleData(TFB_SoundSample* sample, void* data)
{
	sample->data = data;
}

void* TFB_GetSoundSampleData(TFB_SoundSample* sample)
{
	return sample->data;
}

void TFB_SetSoundSampleCallbacks(TFB_SoundSample* sample,
								 const TFB_SoundCallbacks* pcbs /* can be nullptr */)
{
	if (pcbs)
	{
		sample->callbacks = *pcbs;
	}
	else
	{
		memset(&sample->callbacks, 0, sizeof(sample->callbacks));
	}
}

TFB_SoundDecoder*
TFB_GetSoundSampleDecoder(TFB_SoundSample* sample)
{
	return sample->decoder;
}

TFB_SoundTag*
TFB_FindTaggedBuffer(TFB_SoundSample* sample, audio_Object buffer)
{
	uint32_t buf_num;

	if (sample->buffer_tag.empty())
	{
		return nullptr; // do not have any tags
	}

	for (buf_num = 0;
		 buf_num < sample->num_buffers && (!sample->buffer_tag[buf_num].in_use || sample->buffer_tag[buf_num].buf_name != buffer);
		 buf_num++)
		;

	return buf_num < sample->num_buffers ?
			 &sample->buffer_tag[buf_num] :
			 nullptr;
}

bool TFB_TagBuffer(TFB_SoundSample* sample, audio_Object buffer, intptr_t data)
{
	uint32_t buf_num;

	if (sample->buffer_tag.empty())
	{
		sample->buffer_tag.assign(sample->num_buffers, TFB_SoundTag {});
	}

	for (buf_num = 0;
		 buf_num < sample->num_buffers && sample->buffer_tag[buf_num].in_use && sample->buffer_tag[buf_num].buf_name != buffer;
		 buf_num++)
		;

	if (buf_num >= sample->num_buffers)
	{
		return false; // no empty slot
	}

	sample->buffer_tag[buf_num].in_use = 1;
	sample->buffer_tag[buf_num].buf_name = buffer;
	sample->buffer_tag[buf_num].data = data;

	return true;
}

void TFB_ClearBufferTag(TFB_SoundTag* ptag)
{
	ptag->in_use = 0;
	ptag->buf_name = 0;
}

static void
remove_scope_data(TFB_SoundSource* source, audio_Object buffer) UQM_REQUIRES(source->stream_mutex)
{
	audio_IntVal buf_size;

	audio_GetBufferi(buffer, audio_SIZE, &buf_size);
	source->sbuf_head += buf_size;
	// the buffer is cyclic
	source->sbuf_head %= source->sbuf_size;

	source->sbuf_lasttime = GetTimeCounter();
}

static void
add_scope_data(TFB_SoundSource* source, uint32_t bytes)
{
	uint8_t* sbuffer = source->sbuffer.data();
	uint8_t* dec_buf = (uint8_t*)source->sample->decoder->buffer;
	uint32_t tail_bytes;
	uint32_t wrap_bytes;

	if (source->sbuf_tail + bytes > source->sbuf_size)
	{ // does not fit at the tail, have to split it up
		tail_bytes = source->sbuf_size - source->sbuf_tail;
		wrap_bytes = bytes - tail_bytes;
	}
	else
	{ // all fits at the tail
		tail_bytes = bytes;
		wrap_bytes = 0;
	}

	if (wrap_bytes > source->sbuf_tail)
	{ // we can only wrap around to the current tail
		uqm::log::warn("add_scope_data: Has wrap_bytes {} "
					   "greater than source_buffer_tail {} "
					   "| total bytes {} | source_buffer_size {}",
					   wrap_bytes, source->sbuf_tail, bytes, source->sbuf_size);
		wrap_bytes = source->sbuf_tail;
	}

	if (tail_bytes)
	{
		memcpy(sbuffer + source->sbuf_tail, dec_buf, tail_bytes);
		source->sbuf_tail += tail_bytes;
	}

	if (wrap_bytes)
	{
		memcpy(sbuffer, dec_buf + tail_bytes, wrap_bytes);
		source->sbuf_tail = wrap_bytes;
	}
}

static void
process_stream(TFB_SoundSource* source) UQM_REQUIRES(source->stream_mutex)
{
	TFB_SoundSample* sample = source->sample;
	TFB_SoundDecoder* decoder = sample->decoder;
	bool end_chunk_failed = false;
	audio_IntVal processed;
	audio_IntVal queued;

	audio_GetSourcei(source->handle, audio_BUFFERS_PROCESSED, &processed);
	audio_GetSourcei(source->handle, audio_BUFFERS_QUEUED, &queued);

	if (processed == 0)
	{ // Nothing was played
		audio_IntVal state;

		audio_GetSourcei(source->handle, audio_SOURCE_STATE, &state);
		if (state != audio_PLAYING)
		{
			if (queued == 0 && decoder->error == SOUNDDECODER_EOF)
			{ // The stream has reached the end
				uqm::log::info("StreamDecoderTaskFunc(): "
							   "finished playing {}",
							   decoder->filename);
				source->stream_should_be_playing = false;

				if (sample->callbacks.OnEndStream)
				{
					sample->callbacks.OnEndStream(sample);
				}
			}
			else
			{
				uqm::log::warn("StreamDecoderTaskFunc(): "
							   "buffer underrun playing {}",
							   decoder->filename);
				audio_SourcePlay(source->handle);
			}
		}
	}

	// Unqueue processed buffers and replace them with new ones
	for (; processed > 0; --processed)
	{
		uint32_t error;
		audio_Object buffer;
		uint32_t decoded_bytes;

		audio_GetError(); // clear error state

		// Get the buffer that finished playing
		audio_SourceUnqueueBuffers(source->handle, 1, &buffer);
		error = audio_GetError();
		if (error != audio_NO_ERROR)
		{
			uqm::log::warn("StreamDecoderTaskFunc(): "
						   "error after audio_SourceUnqueueBuffers: {:x}, file {}",
						   error, decoder->filename);
			break;
		}

		// Process a callback on a tagged buffer, if any
		if (sample->callbacks.OnTaggedBuffer)
		{
			TFB_SoundTag* tag = TFB_FindTaggedBuffer(sample, buffer);
			if (tag)
			{
				sample->callbacks.OnTaggedBuffer(sample, tag);
			}
		}

		if (!source->sbuffer.empty())
		{
			remove_scope_data(source, buffer);
		}

		// See what state the decoder was left in last time around
		if (decoder->error != SOUNDDECODER_OK)
		{
			if (decoder->error == SOUNDDECODER_EOF)
			{
				if (end_chunk_failed)
				{
					continue; // should not do it again
				}

				if (!sample->callbacks.OnEndChunk || !sample->callbacks.OnEndChunk(sample, source->last_q_buf))
				{ // Reached the end of the current stream and we did not
					// get another sample to play (relevant for Trackplayer)
					end_chunk_failed = true;
					continue;
				}
				else
				{ // OnEndChunk succeeded, so someone (read: Trackplayer)
					// wants to keep going, probably with a new decoder.
					// Get the new decoder
					decoder = sample->decoder;
				}
			}
			else
			{ // Decoder returned a real error, keep going
#if 0
				log_add (log_Debug, "StreamDecoderTaskFunc(): "
						"decoder->error is {} for {}", decoder->error,
						decoder->filename);
#endif
				continue;
			}
		}

		// Now replace the unqueued buffer with a new one
		decoded_bytes = SoundDecoder_Decode(decoder);
		if (decoder->error == SOUNDDECODER_ERROR)
		{
			uqm::log::warn("StreamDecoderTaskFunc(): "
						   "SoundDecoder_Decode error {}, file {}",
						   decoder->error, decoder->filename);
			source->stream_should_be_playing = false;
			continue;
		}

		if (decoded_bytes == 0)
		{ // Nothing was decoded, keep going
			continue;
			// This loses a stream buffer, which we cannot get back
			// w/o restarting the stream, but we should never get here.
		}

		// And a new buffer is born
		audio_BufferData(buffer, decoder->format, decoder->buffer,
						 decoded_bytes, decoder->frequency);
		error = audio_GetError();
		if (error != audio_NO_ERROR)
		{
			uqm::log::warn("StreamDecoderTaskFunc(): "
						   "error after audio_BufferData: {:x}, file {}, decoded {}",
						   error, decoder->filename, decoded_bytes);
			continue;
		}

		// Now queue the buffer
		audio_SourceQueueBuffers(source->handle, 1, &buffer);
		error = audio_GetError();
		if (error != audio_NO_ERROR)
		{
			uqm::log::warn("StreamDecoderTaskFunc(): "
						   "error after audio_SourceQueueBuffers: {:x}, file {}, "
						   "decoded {}",
						   error, decoder->filename, decoded_bytes);
			continue;
		}

		// Remember the last queued buffer so we can pass it to callbacks
		source->last_q_buf = buffer;
		if (sample->callbacks.OnQueueBuffer)
		{
			sample->callbacks.OnQueueBuffer(sample, buffer);
		}

		if (!source->sbuffer.empty())
		{
			add_scope_data(source, decoded_bytes);
		}
	}
}

static void
processMusicFade(void)
{
	TimeCount Now;
	int32_t elapsed;
	int newVolume;

	fade_mutex.lock();

	if (!musicFadeInterval)
	{ // there is no fade set
		fade_mutex.unlock();
		return;
	}

	Now = GetTimeCounter();
	elapsed = Now - musicFadeStartTime;
	if (elapsed > musicFadeInterval)
	{
		elapsed = musicFadeInterval;
	}

	newVolume = musicFadeStartVolume + (long)musicFadeDelta * elapsed / musicFadeInterval;
	SetMusicVolume(newVolume);

	if (elapsed >= musicFadeInterval)
	{
		musicFadeInterval = 0; // fade is over
	}

	fade_mutex.unlock();
}

static void
StreamDecoderTaskFunc(uqstl::stop_token stopToken)
{
	int active_streams;
	int i;

	while (!stopToken.stop_requested())
	{
		active_streams = 0;

		processMusicFade();

		for (i = MUSIC_SOURCE; i < NUM_SOUNDSOURCES; ++i)
		{
			TFB_SoundSource* source = &soundSource[i];

			source->stream_mutex.lock();

			if (!source->sample || !source->sample->decoder || !source->stream_should_be_playing || source->sample->decoder->error == SOUNDDECODER_ERROR)
			{
				source->stream_mutex.unlock();
				continue;
			}

			process_stream(source);
			active_streams++;

			source->stream_mutex.unlock();
		}

		if (active_streams == 0)
		{ // Throttle down the thread when there are no active streams
			HibernateThread(getTicksForFramerate(10));
		}
		else
		{
			TaskSwitch();
		}
	}
}

static inline int32_t
readSoundSample(void* ptr, int sample_size)
{
	if (sample_size == sizeof(uint8_t))
	{
		return (*(uint8_t*)ptr - 128) << 8;
	}
	else
	{
		return *(int16_t*)ptr;
	}
}

// Graphs the current sound data for the oscilloscope.
// Includes a rudimentary automatic gain control (AGC) to properly graph
// the streams at different gain levels (based on running average).
// We use AGC because different pieces of music and speech can easily be
// at very different gain levels, because the game is moddable.
int GraphForegroundStream(uint8_t* data, int32_t width, int32_t height,
						  bool wantSpeech)
{
	int source_num;
	TFB_SoundSource* source;
	TFB_SoundDecoder* decoder;
	int channels;
	int sample_size;
	int full_sample;
	int step;
	long played_time;
	long delta;
	uint8_t* sbuffer;
	unsigned long pos;
	int scale;
	int32_t i;
	// AGC variables
#define DEF_PAGE_MAX 28000
#define AGC_PAGE_COUNT 16
	static int page_sum = DEF_PAGE_MAX * AGC_PAGE_COUNT;
	static int pages[AGC_PAGE_COUNT] =
		{
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
			DEF_PAGE_MAX,
		};
	static int page_head;
#define AGC_FRAME_COUNT 8
	static int frame_sum;
	static int frames;
	static int avg_amp = DEF_PAGE_MAX; // running amplitude (sort of) average
	int target_amp;
	int max_a;
#define VAD_MIN_ENERGY 100
	long energy;


	// Prefer speech to music
	source_num = SPEECH_SOURCE;
	source = &soundSource[source_num];
	source->stream_mutex.lock();
	if (wantSpeech && (!source->sample || !source->sample->decoder || !source->sample->decoder->is_null))
	{ // Use speech waveform, since it's available
		// Step is picked experimentally. Using step of 1 sample at 11025Hz,
		// because human speech is mostly in the low frequencies, and it looks
		// better this way.
		step = 1;
	}
	else
	{ // We do not have speech -- use music waveform
		source->stream_mutex.unlock();

		source_num = MUSIC_SOURCE;
		source = &soundSource[source_num];
		source->stream_mutex.lock();

		// Step is picked experimentally. Using step of 4 samples at 11025Hz.
		// It looks better this way.
		step = 4;
	}

	if (!source->stream_should_be_playing || !source->sample
		|| !source->sample->decoder || source->sbuffer.empty()
		|| source->sbuf_size == 0)
	{ // We don't have data to return, oh well.
		source->stream_mutex.unlock();
		return 0;
	}
	decoder = source->sample->decoder;

	if (!audio_GetFormatInfo(decoder->format, &channels, &sample_size))
	{
		source->stream_mutex.unlock();
		uqm::log::debug("GraphForegroundStream(): uknown format {}",
						(unsigned)decoder->format);
		return 0;
	}
	full_sample = channels * sample_size;

	// See how far into the buffer we should be now
	played_time = GetTimeCounter() - source->sbuf_lasttime;
	delta = played_time * decoder->frequency * full_sample / GameTicksPerSecond;
	// align delta to sample start
	delta = delta & ~(full_sample - 1);

	if (delta < 0)
	{
		uqm::log::debug("GraphForegroundStream(): something is messed"
						" with timing, delta %ld",
						delta);
		delta = 0;
	}
	else if (delta > (long)source->sbuf_size)
	{ // Stream decoder task has just had a heart attack, not much we can do
		delta = 0;
	}

	// Step is in 11025 Hz units, so we need to adjust to source frequency
	step = decoder->frequency * step / 11025;
	if (step == 0)
	{
		step = 1;
	}
	step *= full_sample;

	sbuffer = source->sbuffer.data();
	pos = source->sbuf_head + delta;

	// We are not basing the scaling factor on signal energy, because we
	// want it to *look* pretty instead of sounding nice and even
	target_amp = (height >> 1) >> 1;
	scale = avg_amp / target_amp;

	max_a = 0;
	energy = 0;
	for (i = 0; i < width; ++i, pos += step)
	{
		int32_t s;
		int t;

		pos %= source->sbuf_size;

		s = readSoundSample(sbuffer + pos, sample_size);
		if (channels > 1)
		{
			s += readSoundSample(sbuffer + pos + sample_size, sample_size);
		}

		energy += (s * s) / 0x10000;
		t = abs(s);
		if (t > max_a)
		{
			max_a = t;
		}

		s = (s / scale) + (height >> 1);
		if (s < 0)
		{
			s = 0;
		}
		else if (s > height - 1)
		{
			s = height - 1;
		}

		data[i] = s;
	}
	energy /= width;

	// Very basic VAD. We don't want to count speech pauses in the average
	if (energy > VAD_MIN_ENERGY)
	{
		// Record the maximum amplitude (sort of)
		frame_sum += max_a;
		++frames;
		if (frames == AGC_FRAME_COUNT)
		{ // Got a full page
			frame_sum /= AGC_FRAME_COUNT;
			// Record the page
			page_sum -= pages[page_head];
			page_sum += frame_sum;
			pages[page_head] = frame_sum;
			page_head = (page_head + 1) % AGC_PAGE_COUNT;

			frame_sum = 0;
			frames = 0;

			avg_amp = page_sum / AGC_PAGE_COUNT;
		}
	}

	source->stream_mutex.unlock();
	return 1;
}

// This function is normally called on the Starcon2Main thread
bool SetMusicStreamFade(int32_t howLong, int endVolume)
{
	bool ret = true;

	fade_mutex.lock();

	if (howLong < 0)
	{
		howLong = 0;
	}

	musicFadeStartTime = GetTimeCounter();
	musicFadeInterval = howLong;
	musicFadeStartVolume = musicVolume;
	musicFadeDelta = endVolume - musicFadeStartVolume;
	if (!musicFadeInterval)
	{
		ret = false; // reject
	}

	fade_mutex.unlock();

	return ret;
}

int InitStreamDecoder(void)
{
	s_decoderThread = uqm::makeThread("audio stream decoder", StreamDecoderTaskFunc);
	return 0;
}

void UninitStreamDecoder(void)
{
	s_decoderThread = {};
}
