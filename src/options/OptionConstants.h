#pragma once
#ifndef UQM_OPTIONS_OPTIONCONSTANTS_H_
#define UQM_OPTIONS_OPTIONCONSTANTS_H_

#include "options/OptionTypes.h"
#include "libs/sound/audiocore.h"
#include "libs/graphics/gfx_common.h"

namespace uqm
{

//static inline constexpr uqstl::array<OptionListValueT<int>, 7> ScalerList {
//	{
//		{"bilinear", TFB_GFXFLAGS_SCALE_BILINEAR},
//		{"biadapt", TFB_GFXFLAGS_SCALE_BIADAPT},
//		{"biadv", TFB_GFXFLAGS_SCALE_BIADAPTADV},
//		{"triscan", TFB_GFXFLAGS_SCALE_TRISCAN},
//		{"hq", TFB_GFXFLAGS_SCALE_HQXX},
//		{"none", 0},
//		{"no", 0}, /* uqm.cfg value */
//	}
//};

//static inline constexpr uqstl::array<OptionListValueT<int>, 6> MeleeScaleList {
//	{{"smooth", TFB_SCALE_TRILINEAR},
//	 {"3do", TFB_SCALE_TRILINEAR},
//	 {"step", TFB_SCALE_STEP},
//	 {"pc", TFB_SCALE_STEP},
//	 {"bilinear", TFB_SCALE_BILINEAR},
//	 {"nearest", TFB_SCALE_NEAREST}}
//};

//static inline constexpr uqstl::array<OptionListValueT<int>, 4> AudioDriverList {
//	{{"openal", audio_DRIVER_OPENAL},
//	 {"mixsdl", audio_DRIVER_MIXSDL},
//	 {"none", audio_DRIVER_NOSOUND},
//	 {"nosound", audio_DRIVER_NOSOUND}}
//};

//static inline constexpr uqstl::array<OptionListValueT<int>, 3> AudioQualityList {
//	{{"low", audio_QUALITY_LOW},
//	 {"medium", audio_QUALITY_MEDIUM},
//	 {"high", audio_QUALITY_HIGH}}
//};

static inline constexpr uqstl::array<OptionListValueT<EmulationMode>, 2> EmulationChoiceList {

	{{"pc", EmulationMode::PC},
	 {"3do", EmulationMode::Console3DO}}
};

} // namespace uqm
#endif /* UQM_OPTIONS_OPTIONCONSTANTS_H_ */