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
 * Internals
 */

#ifndef LIBS_SOUND_MIXER_MIXERINT_H_
#define LIBS_SOUND_MIXER_MIXERINT_H_

#include "core/stl/stl.h"
#include "port.h"
#include <cstdint>

/*************************************************
 *  Internals
 */

/* Conversion info types and funcs */
typedef enum
{
	mixConvNone = 0,
	mixConvStereoUp = 1,
	mixConvStereoDown = 2,
	mixConvSizeUp = 4,
	mixConvSizeDown = 8

} mixer_ConvFlags;

mixer_ConvFlags operator|=(mixer_ConvFlags lhs, mixer_ConvFlags rhs)
{
	return static_cast<mixer_ConvFlags>(static_cast<std::underlying_type_t<mixer_ConvFlags>>(lhs) | static_cast<std::underlying_type_t<mixer_ConvFlags>>(rhs));
}

typedef struct
{
	uint32_t srcfmt;
	void* srcdata;
	uint32_t srcsize;
	uint32_t srcbpc; /* bytes/sample for 1 chan */
	uint32_t srcchans;
	uint32_t srcsamples;

	uint32_t dstfmt;
	void* dstdata;
	uint32_t dstsize;
	uint32_t dstbpc; /* bytes/sample for 1 chan */
	uint32_t dstchans;
	uint32_t dstsamples;

	mixer_ConvFlags flags;

} mixer_Convertion;

typedef struct
{
	float (*Upsample)(mixer_Source* src, bool left);
	float (*Downsample)(mixer_Source* src, bool left);
	float (*None)(mixer_Source* src, bool left);
} mixer_Resampling;

static void mixer_ConvertBuffer_internal(mixer_Convertion* conv);
static void mixer_ResampleFlat(mixer_Convertion* conv);

static inline int32_t mixer_GetSampleExt(void* src, uint32_t bpc);
static inline int32_t mixer_GetSampleInt(void* src, uint32_t bpc);
static inline void mixer_PutSampleInt(void* dst, uint32_t bpc,
									  int32_t samp);
static inline void mixer_PutSampleExt(void* dst, uint32_t bpc,
									  int32_t samp);

static float mixer_ResampleNone(mixer_Source* src, bool left);
static float mixer_ResampleNearest(mixer_Source* src, bool left);
static float mixer_UpsampleLinear(mixer_Source* src, bool left);
static float mixer_UpsampleCubic(mixer_Source* src, bool left);

/* Source manipulation */
static void mixer_SourceUnqueueAll(mixer_Source* src);
static void mixer_SourceStop_internal(mixer_Source* src);
static void mixer_SourceRewind_internal(mixer_Source* src);
static void mixer_SourceActivate(mixer_Source* src);
static void mixer_SourceDeactivate(mixer_Source* src);

static inline bool mixer_CheckBufferState(mixer_Buffer* buf,
										  const char* FuncName);

/* Clipping boundaries */
inline constexpr auto MixS16Max {((float)INT16_MAX)};
inline constexpr auto MixS16Min {((float)INT16_MIN)};
inline constexpr auto MixS8Max {((float)INT8_MAX)};
inline constexpr auto MixS8Min {((float)INT8_MIN)};

/* Channel gain adjustment for clipping reduction */
inline constexpr auto MixGainAdj {(0.75f)};

/* The Mixer */
static inline bool mixer_SourceGetNextSample(mixer_Source* src,
											 float* psamp, bool left);
static inline bool mixer_SourceGetFakeSample(mixer_Source* src,
											 float* psamp, bool left);
static inline uint32_t mixer_SourceAdvance(mixer_Source* src, bool left);

#endif /* LIBS_SOUND_MIXER_MIXERINT_H_ */
