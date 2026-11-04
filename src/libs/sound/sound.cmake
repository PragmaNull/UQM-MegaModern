############################################################


set(uqm_src_libs_sound_master_files "")

############################################################
# Sub-directories
include("src/libs/sound/decoders/decoders.cmake")
list(APPEND uqm_src_libs_sound_master_files ${uqm_src_libs_sound_decoders_master_files})
include("src/libs/sound/mixer/mixer.cmake")
list(APPEND uqm_src_libs_sound_master_files ${uqm_src_libs_sound_mixer_master_files})
include("src/libs/sound/openal/openal.cmake")
list(APPEND uqm_src_libs_sound_master_files ${uqm_src_libs_sound_openal_master_files})

############################################################
# Local files
set(uqm_src_libs_sound_files
	"src/libs/sound/audiocore.cpp"
	"src/libs/sound/audiocore.h"
	"src/libs/sound/audiocoredefs.h"
	"src/libs/sound/fileinst.cpp"
	"src/libs/sound/music.cpp"
	"src/libs/sound/resinst.cpp"
	"src/libs/sound/sfx.cpp"
	"src/libs/sound/sndintrn.h"
	"src/libs/sound/sound.cmake"
	"src/libs/sound/sound.cpp"
	"src/libs/sound/soundbank.cpp"
	"src/libs/sound/soundbank.h"
	"src/libs/sound/sound.h"
	"src/libs/sound/stream.cpp"
	"src/libs/sound/stream.h"
	"src/libs/sound/trackint.h"
	"src/libs/sound/trackplayer.cpp"
	"src/libs/sound/trackplayer.h"
)

source_group("src\\libs\\sound" FILES ${uqm_src_libs_sound_files})
list(APPEND uqm_src_libs_sound_master_files ${uqm_src_libs_sound_files})
