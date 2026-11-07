#pragma once
#ifndef UQM_LIBS_SOUND_AUDIOCOREDEFS_H
#define UQM_LIBS_SOUND_AUDIOCOREDEFS_H

#include "core/types/enum.h"
/* Available drivers */
enum class AudioDriverType
{
	NoSound,
	MixSDL,
	OpenAL,
};

/* Initialization flags */
enum class AudioFlags : uint32_t
{
	None,
	QualityLow = 1,
	QualityMedium = 1 << 1,
	QualityHigh = 1 << 2,
};
ENUM_BITWISE_OPS(AudioFlags)
static constexpr inline AudioFlags AudioQualityFlagMask {AudioFlags::QualityLow | AudioFlags::QualityMedium | AudioFlags::QualityHigh};

/* Interface Types */
using audio_Object = uintptr_t;
using audio_IntVal = intptr_t;

enum class AudioEnum
{
	/* Errors */
	NoError = 0,
	InvalidName,
	InvalidEnum,
	InvalidValue,
	InvalidOperation,
	OutOfMemory,
	DriverFailure,

	/* Source properties */
	Position,
	Looping,
	Buffer,
	Gain,
	SourceState,
	BuffersQueued,
	BuffersProcessed,

	/* Source state information */
	Initial,
	Stopped,
	Playing,
	Paused,

	/* Sound buffer properties */
	Frequency,
	Bits,
	Channels,
	Size,
	FormatMono16,
	FormatStereo16,
	FormatMono8,
	FormatStereo8,
	Count
};
template <>
inline constexpr bool IsArithmeticEnum<AudioEnum> = true;

using audio_SourceProp = AudioEnum;
using audio_BufferProp = AudioEnum;

#endif // UQM_LIBS_SOUND_AUDIOCOREDEFS_H
