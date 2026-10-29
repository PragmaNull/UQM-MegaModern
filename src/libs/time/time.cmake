############################################################


set(uqm_src_libs_time_master_files "")

############################################################
# Sub-directories
include("src/libs/time/sdl/sdl.cmake")
list(APPEND uqm_src_libs_time_master_files ${uqm_src_libs_time_sdl_master_files})

############################################################
# Local files
set(uqm_src_libs_time_files
	"src/libs/time/time.cmake"
	"src/libs/time/timecommon.cpp"
	"src/libs/time/timecommon.h"
	"src/libs/time/timelib.h"
)

source_group("src\\libs\\time" FILES ${uqm_src_libs_time_files})
list(APPEND uqm_src_libs_time_master_files ${uqm_src_libs_time_files})
