############################################################


set(uqm_src_core_misc_master_files "")


############################################################
# Local files
set(uqm_src_core_misc_files
	"src/core/misc/bittricks.h"
	"src/core/misc/FatalExit.h"
	"src/core/misc/misc.cmake"
)

source_group("src\\core\\misc" FILES ${uqm_src_core_misc_files})
list(APPEND uqm_src_core_misc_master_files ${uqm_src_core_misc_files})
