############################################################


set(uqm_src_libs_input_master_files "")

############################################################
# Sub-directories
include("src/libs/input/sdl/sdl.cmake")
list(APPEND uqm_src_libs_input_master_files ${uqm_src_libs_input_sdl_master_files})

############################################################
# Local files
set(uqm_src_libs_input_files
	"src/libs/input/inpintrn.h"
	"src/libs/input/input.cmake"
	"src/libs/input/input_common.cpp"
	"src/libs/input/input_common.h"
	"src/libs/input/keyconfig.cpp"
	"src/libs/input/keyconfig.h"
)

source_group("src\\libs\\input" FILES ${uqm_src_libs_input_files})
list(APPEND uqm_src_libs_input_master_files ${uqm_src_libs_input_files})
