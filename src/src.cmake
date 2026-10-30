############################################################


set(uqm_src_master_files "")

############################################################
# Sub-directories
include("src/configuration/configuration.cmake")
list(APPEND uqm_src_master_files ${uqm_src_configuration_master_files})
include("src/core/core.cmake")
list(APPEND uqm_src_master_files ${uqm_src_core_master_files})
include("src/lib/lib.cmake")
list(APPEND uqm_src_master_files ${uqm_src_lib_master_files})
include("src/libs/libs.cmake")
list(APPEND uqm_src_master_files ${uqm_src_libs_master_files})
include("src/options/options.cmake")
list(APPEND uqm_src_master_files ${uqm_src_options_master_files})
include("src/res/res.cmake")
list(APPEND uqm_src_master_files ${uqm_src_res_master_files})
include("src/uqm/uqm.cmake")
list(APPEND uqm_src_master_files ${uqm_src_uqm_master_files})

############################################################
# Local files
set(uqm_src_files
#	"src/.clang-format"
	"src/UQMGame.cpp"
	"src/UQMGame.h"
	"src/config.h"
#	"src/config_unix.h.in"
	"src/config_vc6.h"
#	"src/config_win.h"
#	"src/config_win.h.in"
	"src/endian_uqm.h"
	"src/main.cpp"
	"src/options.cpp"
	"src/options.h"
	"src/port.cpp"
	"src/port.h"
	"src/src.cmake"
	"src/uqmversion.h"
)

source_group("src" FILES ${uqm_src_files})
list(APPEND uqm_src_master_files ${uqm_src_files})
