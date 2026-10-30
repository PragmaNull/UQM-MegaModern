############################################################


set(uqm_src_libs_master_files "")

############################################################
# Sub-directories
include("src/libs/callback/callback.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_callback_master_files})
include("src/libs/file/file.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_file_master_files})
include("src/libs/graphics/graphics.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_graphics_master_files})
include("src/libs/input/input.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_input_master_files})
include("src/libs/io/io.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_io_master_files})
include("src/libs/luauqm/luauqm.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_luauqm_master_files})
include("src/libs/math/math.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_math_master_files})
include("src/libs/network/network.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_network_master_files})
include("src/libs/resource/resource.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_resource_master_files})
include("src/libs/sound/sound.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_sound_master_files})
include("src/libs/strings/strings.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_strings_master_files})
include("src/libs/time/time.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_time_master_files})
include("src/libs/video/video.cmake")
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_video_master_files})

############################################################
# Local files
set(uqm_src_libs_files
	"src/libs/gfxlib.h"
	"src/libs/inplib.h"
	"src/libs/libs.cmake"
	"src/libs/mathlib.h"
	"src/libs/reslib.h"
#	"src/libs/sndlib.h"
	"src/libs/strlib.h"
	"src/libs/vidlib.h"
)

source_group("src\\libs" FILES ${uqm_src_libs_files})
list(APPEND uqm_src_libs_master_files ${uqm_src_libs_files})
